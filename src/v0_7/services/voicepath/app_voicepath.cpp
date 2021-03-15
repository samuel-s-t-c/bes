/***************************************************************************
*
*Copyright 2015-2020 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include "cqueue.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "voice_sbc.h"
#include "app_audio.h"
#include "app_utils.h"
#include "apps.h"
#include "app_voicepath.h"
#include "app_ble_rx_handler.h"
#include "app_ble_mode_switch.h"
#include "gapm_task.h"
#include "nvrecord_extension.h"
#include "app_ai_manager_api.h"
#include "app_bt_media_manager.h"

//#define VOICEPATH_DC_FILTER_ENABLE

#ifdef VOICEPATH_DC_FILTER_ENABLE
#include "speech_memory.h"
#include "speech_cfg.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#include "gsound_custom_bt.h"
#include "gsound_custom_actions.h"
#endif

#ifdef VOICE_DETECTOR_EN
#include "app_voice_detector.h"

#ifdef VAD_USE_8K_SAMPLE_RATE
#include "app_bt_stream.h" //!< use the resample APIs
#endif
#endif

#ifdef GSOUND_HOTWORD_EXTERNAL
#include "cp_accel.h"
#endif

//#define VOICEPATH_DUMP_DATA

#if defined(VOICEPATH_DUMP_DATA)
#include "audio_dump.h"
#endif

/*********************external function declearation************************/
#ifdef VOICE_DETECTOR_EN
extern unsigned int vad_buf_len;
#endif

/************************private macro defination***************************/
#ifdef GSOUND_HOTWORD_EXTERNAL
#define AI_REQUIRED_SYSFREQ APP_SYSFREQ_26M
#else
#define AI_REQUIRED_SYSFREQ APP_SYSFREQ_78M
#endif

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/
static uint8_t app_voicepath_pending_streaming = 0;
static uint8_t app_voicepath_stream_state = 0;
osMutexDef(pcm_queue_mutex);
osMutexId pcm_queue_mutex_id;

static VOICEPATH_CTX_T VPctx;

#ifdef VOICE_DETECTOR_EN
static volatile int audioCaptureStartFlag = 0;

#ifdef VAD_USE_8K_SAMPLE_RATE
struct APP_RESAMPLE_T *resampler = NULL;
#endif
#endif

#ifdef VOICEPATH_DC_FILTER_ENABLE
#define APP_VOICEPATH_HEAP_SIZE (10*1024)
static uint8_t app_voicepath_heap[APP_VOICEPATH_HEAP_SIZE];

static SpeechDcFilterConfig app_voicepath_dc_config = {
    .bypass = 0,
};
static SpeechDcFilterState *app_voicepath_dc_filter = NULL;
#endif

#if !defined(__AUDIO_RESAMPLE__)
#define APP_VOICEPATH_RESAMPLE_RATIO                 ((float)AUD_SAMPRATE_16000/AUD_SAMPRATE_14700)
#define APP_VOICEPATH_BUF_SIZE_FOR_RESAMPLER         2000 //(APP_VOICEPATH_RESAMPLE_ITER_NUM+128)

static uint32_t app_voicepath_resample_iter_num;
static uint32_t app_voicepath_target_chunk_size;

static struct APP_RESAMPLE_T * app_voicepath_resampler = NULL;
static uint8_t app_voicepath_resampler_buf[APP_VOICEPATH_BUF_SIZE_FOR_RESAMPLER];
static uint8_t* app_voicepath_resample_source_buf;

static uint32_t app_voicepath_sample_rate;

static int app_voicepath_resample_iter(uint8_t *buf, uint32_t len)
{
    if (!buf)
    {
        TRACE(0, "NULL pointer received in %s", __func__);
        return -1;
    }

    //TRACE(1, "rs: get source %d", len);

    memcpy(buf, app_voicepath_resample_source_buf, len);

    return 0;
}
#endif

static void app_voicepath_update_config(VOICEPATH_CTX_T* pContext)
{
#if !defined(__AUDIO_RESAMPLE__)
    if (app_ai_if_is_music_or_prompt_ongoing() && 
        (AUD_SAMPRATE_44100 == app_ai_if_get_music_or_prompt_sample_rate()))
    {
        struct AF_STREAM_CONFIG_T* pConfig = &(pContext->streamCfg);
        
        pConfig->sample_rate = AUD_SAMPRATE_14700;
        //TRACE(1, "org data size %d", pConfig->data_size);
        app_voicepath_target_chunk_size = pConfig->data_size/2;
        
        pConfig->data_size = (pConfig->data_size*AUD_SAMPRATE_14700/AUD_SAMPRATE_16000);
        pConfig->data_size = (pConfig->data_size+15)/16*16;

        app_voicepath_resample_iter_num = pConfig->data_size/2;
        //TRACE(1, "APP_VOICEPATH_BUF_SIZE_FOR_RESAMPLER %d app_voicepath_resample_iter_num %d",
        //    APP_VOICEPATH_BUF_SIZE_FOR_RESAMPLER, app_voicepath_resample_iter_num);
                       
        // create sw resampler
        app_voicepath_resampler = app_capture_resample_14k7_to_16k(
            ( enum AUD_CHANNEL_NUM_T )pConfig->channel_num,
            app_voicepath_resample_iter,
            app_voicepath_resample_iter_num,
            APP_VOICEPATH_RESAMPLE_RATIO,
            app_voicepath_resampler_buf,
            APP_VOICEPATH_BUF_SIZE_FOR_RESAMPLER);

    }
#endif 
}

#if !defined(__AUDIO_RESAMPLE__)
extern void app_voicepath_reset_capture_buffer(void);
static int app_voicepath_restart_mic(void)
{
    TRACE(2,"%s VOICEPATH_STREAMING %d",__func__, 
        app_voicepath_get_stream_state(VOICEPATH_STREAMING));
    if (app_voicepath_get_stream_state(VOICEPATH_STREAMING))
    {
        app_voicepath_stop_audio_stream();
        app_voicepath_reset_capture_buffer();
        app_voicepath_start_audio_stream();
    }
    return 0;
}
#endif

void app_voicepath_pre_music_or_prompt_check(void)
{
#if !defined(__AUDIO_RESAMPLE__)
    if (app_voicepath_get_stream_state(VOICEPATH_STREAMING) &&
        (app_ai_if_get_music_or_prompt_sample_rate()%app_voicepath_sample_rate))
    {
        // reset ai voice
        TRACE(1, "restart app voicepath mic.");
        app_voicepath_restart_mic();
    }
#endif
}

#if !defined(GSOUND_HOTWORD_EXTERNAL)
typedef void(*VOICEPATH_DATA_HANDLER_T)(void);
static void voicepath_process(const void *args);
osThreadDef(voicepath_process, osPriorityNormal, 1, 4*1024, "voicepath_process");
static osThreadId voicepath_tid;
#endif

/****************************function defination****************************/
#ifdef VOICE_DETECTOR_EN
static void _voice_detector_run_done_cb(int state, void *argv)
{
    if (state == VOICE_DET_STATE_AUD_CAP_START)
    {
        audioCaptureStartFlag = 1;
    }
}

void app_voicepath_set_vad_audio_capture_start_flag(int flag)
{
    audioCaptureStartFlag = flag;
}

int app_voicepath_get_vad_audio_capture_start_flag(void)
{
    return audioCaptureStartFlag;
}

#ifdef VAD_USE_8K_SAMPLE_RATE
static void _resampled_data_come(uint8_t *ptrBuf, uint32_t length)
{
    TRACE(1, "Resampled data len:%d", length);

    /// enqueue data into pcm data queue
    app_voicepath_enqueue_pcm_data(ptrBuf, length);
}

void app_voicepath_init_resampler(uint8_t channelCnt,
                                  uint32_t outputBufSize,
                                  uint32_t srcSampleRate,
                                  uint32_t dstSampleRate,
                                  void *cb)
{
    APP_RESAMPLE_BUF_ALLOC_CALLBACK oldCb;

    if (NULL == resampler)
    {
        TRACE(3,
              "Create the resampler outputSize:%d, srcSampleRate:%d, dstSampleRate:%d",
              outputBufSize,
              srcSampleRate,
              dstSampleRate);

        oldCb = app_resample_set_buf_alloc_callback(app_capture_audio_mempool_get_buff);

        resampler = app_capture_resample_any_open((enum AUD_CHANNEL_NUM_T)channelCnt,
                                                  (APP_RESAMPLE_ITER_CALLBACK)cb,
                                                  outputBufSize,
                                                  ((float)srcSampleRate) / ((float)(dstSampleRate)));

        app_resample_set_buf_alloc_callback(oldCb);
    }
}

void app_voicepath_deinit_resample(void)
{
    if (resampler)
    {
        app_capture_resample_close(resampler);
        resampler = NULL;
    }
}

void app_voicepath_resample_run(uint8_t *ptrInput, uint32_t dataLen)
{
    TRACE(1, "Start resmple %d bytes", dataLen);
    uint32_t formerTicks = hal_sys_timer_get();
    app_capture_resample_run(resampler, ptrInput, dataLen);
    TRACE(1, "Resmple cost %d ms", TICKS_TO_MS(hal_sys_timer_get() - formerTicks));
}
#endif
#endif //!< VOICE_DETECTOR_EN

void app_voicepath_reset_env(void)
{
    memset((void *)&VPctx, 0, sizeof(VPctx));
    VPctx.pcmDataCacheBufMutexId = osMutexCreate((osMutex(pcm_queue_mutex)));
}

void *app_voicepth_get_env_ptr(void)
{
    return (void *)&VPctx;
}

int app_voicepath_enqueue_pcm_data(uint8_t *ptrBuf, uint32_t length)
{
    int ret = 0;
    uint32_t qAvaliable;

    osMutexWait(VPctx.pcmDataCacheBufMutexId, osWaitForever);
    qAvaliable = AvailableOfCQueue(&(VPctx.pcmDataCacheBufQueue));

    if (0 == qAvaliable)
    {
        TRACE(0, "voice PCM data buffer is full!");
        ret = -1;
    }
    else if (length > qAvaliable)
    {
        TRACE(0, "voice PCM data buffer exceed! dataLen:%d, available:%d", length, qAvaliable);
        ret = -1;
    }
    else
    {
        // push the pcm data pending for encoding and xfer.
        // As the encoding costs time and may cause confliction with the a2dp
        // streaming callback function, when the a2dp streaming is active, we will
        // execute the encoding at the end of the a2dp streaming callback
        EnCQueue(&(VPctx.pcmDataCacheBufQueue), ptrBuf, length);
    }

    osMutexRelease(VPctx.pcmDataCacheBufMutexId);

    return ret;
}

void app_voicepath_mic_data_process(void)
{
    osMutexWait(VPctx.pcmDataCacheBufMutexId, osWaitForever);
    uint32_t length = LengthOfCQueue(&(VPctx.pcmDataCacheBufQueue));
    osMutexRelease(VPctx.pcmDataCacheBufMutexId);

    /*
   * TODO(mfk): This queue is never empty once there has been at least one voice
   *            query.
   */
#ifdef GSOUND_HOTWORD_EXTERNAL
    if (length >= VOICE_SBC_PCM_DATA_SIZE_PER_FRAME)
#else
    while (length >= VOICE_SBC_PCM_DATA_SIZE_PER_FRAME)
#endif
    {
        osMutexWait(VPctx.pcmDataCacheBufMutexId, osWaitForever);
        app_pop_data_from_cqueue(&(VPctx.pcmDataCacheBufQueue),
                                 VPctx.pcmDataHandleBuf,
                                 VOICE_SBC_PCM_DATA_SIZE_PER_FRAME);
        length = LengthOfCQueue(&(VPctx.pcmDataCacheBufQueue));
        osMutexRelease(VPctx.pcmDataCacheBufMutexId);

        if (VPctx.captureStreamHandler.handler)
        {
            VPctx.captureStreamHandler.handler(
                VPctx.pcmDataHandleBuf,
                VOICE_SBC_PCM_DATA_SIZE_PER_FRAME,
                VPctx.encodeDataBuf);
        }
    }
}

static void _init_capture(void)
{
    if (VPctx.captureStreamHandler.initializer)
    {
        VPctx.captureStreamHandler.initializer();
    }
}

static void _dinit_capture(void)
{
    if (VPctx.captureStreamHandler.deinitializer)
    {
        VPctx.captureStreamHandler.deinitializer();
    }
}

#ifdef VOICE_DETECTOR_EN
static bool _store_vad_buf_data(void)
{
    uint32_t start_pos = 0;
    uint32_t len = 0;
    struct CODEC_VAD_BUF_INFO_T vad_buf_info;
    app_voice_detector_get_vad_data_info(VOICE_DETECTOR_ID_0, &vad_buf_info);
    if ((vad_buf_info.data_count > vad_buf_info.buf_size)|| (vad_buf_info.addr_count > vad_buf_info.buf_size))
    {
        TRACE(1, "%s data_count %d addr_count %d buf_size %d",
              __func__, vad_buf_info.data_count,
              vad_buf_info.addr_count, vad_buf_info.buf_size);
        return false;
    }

    if (vad_buf_info.addr_count >= vad_buf_info.data_count)
    {
        start_pos = vad_buf_info.addr_count - vad_buf_info.data_count;
    }
    else
    {
        // In this case (src_size == vad_data_cnt)
        start_pos = vad_buf_info.addr_count + vad_buf_info.buf_size - vad_buf_info.data_count;
    }

    len = MIN(vad_buf_info.buf_size, vad_buf_info.data_count);
    TRACE(1, "VAD incoming data len:%d", len);

    osMutexWait(VPctx.pcmDataCacheBufMutexId, osWaitForever);
    if ((uint32_t)AvailableOfCQueue(&(VPctx.pcmDataCacheBufQueue)) < len)
    {
        ASSERT("%s len:%d avail:%d", __func__, len, AvailableOfCQueue(&(VPctx.pcmDataCacheBufQueue)));
    }

    if (start_pos + len <= vad_buf_info.buf_size)
    {
        EnCQueue(&(VPctx.pcmDataCacheBufQueue), (CQItemType *)(vad_buf_info.base_addr + start_pos), len);
    }
    else
    {
        uint32_t len1, len2;
        len1 = vad_buf_info.buf_size - start_pos;
        len2 = len - len1;
        EnCQueue(&(VPctx.pcmDataCacheBufQueue), (CQItemType *)(vad_buf_info.base_addr + start_pos), len1);
        EnCQueue(&(VPctx.pcmDataCacheBufQueue), (CQItemType *)(vad_buf_info.base_addr), len2);
    }
    osMutexRelease(VPctx.pcmDataCacheBufMutexId);
    return true;
}
#endif

uint32_t app_voicepath_more_data(uint8_t *ptrBuf, uint32_t length)
{
#if defined(VOICEPATH_DUMP_DATA)
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, ptrBuf, length / sizeof(int16_t));
    audio_dump_run();
#endif

    // if the voice stream is already on, we won't start the a2dp stream right-now,
    // otherwise there will be confliction between the mic&speaker audio callback
    // triggering. So we put set the pending flag when opening a2dp stream and
    // trigger the a2dp stream at the time of catpure stream callback function here.
    if (app_voicepath_get_stream_pending_state(AUDIO_OUTPUT_STREAMING))
    {
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, true);
        app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, false);
    }

#if !defined(__AUDIO_RESAMPLE__)
    if (NULL != app_voicepath_resampler)
    {
        app_voicepath_resample_source_buf = ptrBuf;

        app_playback_resample_run(app_voicepath_resampler,
                                  VPctx.resampleTargetBuf,
                                  app_voicepath_target_chunk_size);

        length = app_voicepath_target_chunk_size; 

        //TRACE(1, "rs: iter %d target chunk size %d length %d", app_voicepath_resample_iter_num,
        //    app_voicepath_target_chunk_size, length);

        ptrBuf = VPctx.resampleTargetBuf;
    }
#endif

#if defined(VOICEPATH_DC_FILTER_ENABLE)
    speech_dc_filter_process(app_voicepath_dc_filter, (int16_t *)ptrBuf, length / sizeof(int16_t));
#endif

    // this part is used for mic data pre-process, NOTE:
    // 1. Finally enqueue the data
    // 2. Enqueue the data directly if there is no pro-process
#ifdef AI_ALGORITHM_ENABLE
    app_ai_algorithm_mic_data_handle(ptrBuf, length);
#else
#ifdef VOICE_DETECTOR_EN
    if (vad_buf_len > 0)
    {
        TRACE(2, "%s, vad_buf_len:%d", __func__, vad_buf_len);
#ifdef VAD_USE_8K_SAMPLE_RATE
        /// upsampling to 16K and enqueue resampled data
        app_voicepath_resample_run((uint8_t *)vad_buf,
                                   vad_buf_len);
#else
        _store_vad_buf_data();
#endif
        vad_buf_len = 0;
    }
#endif
    app_voicepath_enqueue_pcm_data(ptrBuf, length);
#endif

#if !defined(GSOUND_HOTWORD_EXTERNAL)
    osSignalSet(voicepath_tid, 0x01);
#else
    app_voicepath_mic_data_process();
#endif
    return length;
}

int app_voicepath_start_audio_stream(void)
{
    TRACE(1, "%s", __func__);

    app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, AI_REQUIRED_SYSFREQ);

    app_voicepath_update_config(&(VPctx));

    uint8_t *bt_audio_buff = NULL;
    app_capture_audio_mempool_get_buff(&bt_audio_buff, VPctx.streamCfg.data_size);
    VPctx.streamCfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
    VPctx.streamCfg.handler = app_voicepath_more_data;

    _init_capture();

#if defined(VOICEPATH_DUMP_DATA)
    audio_dump_init(VPctx.streamCfg.data_size / 2 / sizeof(int16_t), sizeof(int16_t), 1);
#endif

#ifdef VOICE_DETECTOR_EN
#ifdef VAD_USE_8K_SAMPLE_RATE
    app_voicepath_init_resampler(AUD_CHANNEL_NUM_1,
                                 (16 * 1024),
                                 AUD_SAMPRATE_8000,
                                 AUD_SAMPRATE_16000,
                                 (void *)_resampled_data_come);
#endif

    app_voice_detector_init();
    app_voice_detector_open(VOICE_DETECTOR_ID_0, AUD_VAD_TYPE_DIG);

    app_voice_detector_setup_stream(VOICE_DETECTOR_ID_0,
                                    AUD_STREAM_CAPTURE,
                                    &(VPctx.streamCfg));

    app_voice_detector_setup_callback(VOICE_DETECTOR_ID_0,
                                      VOICE_DET_CB_RUN_DONE,
                                      _voice_detector_run_done_cb,
                                      NULL);

    app_voice_detector_send_event(VOICE_DETECTOR_ID_0, VOICE_DET_EVT_VAD_START);
#else
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &(VPctx.streamCfg));

    // start the capture stream immediately
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#endif

#if !defined(__AUDIO_RESAMPLE__)
    app_voicepath_sample_rate = VPctx.streamCfg.sample_rate;
#endif

#if defined(VOICEPATH_DC_FILTER_ENABLE)
    speech_heap_init(&app_voicepath_heap[0], sizeof(app_voicepath_heap));

    int channel_num = VPctx.streamCfg.channel_num;
    int data_separation = 0;
    app_voicepath_dc_filter = speech_dc_filter_create(VPctx.streamCfg.sample_rate, &app_voicepath_dc_config);
    speech_dc_filter_ctl(app_voicepath_dc_filter, SPEECH_DC_FILTER_SET_CHANNEL_NUM, &channel_num);
    speech_dc_filter_ctl(app_voicepath_dc_filter, SPEECH_DC_FILTER_SET_DATA_SEPARATION, &data_separation);
#endif

    // mark that the voice-path streaming is on
    app_voicepath_set_stream_state(VOICEPATH_STREAMING, true);

    return 0;
}

int app_voicepath_stop_audio_stream(void)
{
#ifdef VOICE_DETECTOR_EN
    app_voice_detector_close(VOICE_DETECTOR_ID_0);
    TRACE(0, "voice detector off");
#ifdef VAD_USE_8K_SAMPLE_RATE
    app_voicepath_deinit_resample();
#endif
#else
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#endif

#if !defined(__AUDIO_RESAMPLE__)
    if (NULL != app_voicepath_resampler)
    {
        app_playback_resample_close(app_voicepath_resampler);
        app_voicepath_resampler = NULL;
    }
#endif

#if defined(VOICEPATH_DC_FILTER_ENABLE)
    if (NULL != app_voicepath_dc_filter)
    {
        speech_dc_filter_destroy(app_voicepath_dc_filter);
        app_voicepath_dc_filter = NULL;

    size_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(3, "VOICEPATH HEAP: total - %d, used - %d, max_used - %d.", total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);
    }
#endif

    _dinit_capture();

    // clear the voice path stream pending flag and streaming state flag
    app_voicepath_set_pending_started_stream(VOICEPATH_STREAMING, false);
    app_voicepath_set_stream_state(VOICEPATH_STREAMING, false);

    if (app_voicepath_get_stream_pending_state(AUDIO_OUTPUT_STREAMING))
    {
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, true);
        app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, false);
    }

    // release the acquired system clock
    app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, APP_SYSFREQ_32K);

    return 0;
}

void app_voicepath_set_pending_started_stream(uint8_t pendingStream, bool isEnabled)
{
    if (isEnabled)
    {
        app_voicepath_pending_streaming |= (1 << pendingStream);
    }
    else
    {
        app_voicepath_pending_streaming &= (~(1 << pendingStream));
    }
}

bool app_voicepath_get_stream_pending_state(uint8_t pendingStream)
{
    return app_voicepath_pending_streaming & (1 << pendingStream);
}

bool app_voicepath_get_stream_state(uint8_t stream)
{
    return (app_voicepath_stream_state & (1 << stream));
}

void app_voicepath_set_stream_state(uint8_t stream, bool isEnabled)
{
    if (isEnabled)
    {
        app_voicepath_stream_state |= (1 << stream);
    }
    else
    {
        app_voicepath_stream_state &= (~(1 << stream));
    }
}

void app_pop_data_from_cqueue(CQueue *ptrQueue, uint8_t *buff, uint32_t len)
{
    uint8_t *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    PeekCQueue(ptrQueue, len, &e1, &len1, &e2, &len2);
    if (len == (len1 + len2))
    {
        memcpy(buff, e1, len1);
        memcpy(buff + len1, e2, len2);
        DeCQueue(ptrQueue, 0, len);
    }
    else
    {
        memset(buff, 0x00, len);
    }
}

bool app_voicepath_check_hfp_active(void)
{
    return (bool)bt_media_is_media_active_by_type(BT_STREAM_VOICE);
}

extern uint8_t hfp_get_call_setup_running_on_state(void);

bool app_voicepath_check_hfp_callsetup_running(void)
{
    return hfp_get_call_setup_running_on_state();
}

bool app_voicepath_check_active()
{
    return (bool)bt_media_is_media_active_by_type(BT_STREAM_CAPTURE);
}

void app_voicepath_start_ble_adv_uuid(void)
{
    app_ble_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
}

bool voicepathInitializated = false;

void app_voicepath_init(void)
{
    if (!voicepathInitializated)
    {
        bool isEnable = true;
        voicepathInitializated = true;

        app_ble_rx_handler_init();

#ifdef BISTO_ENABLED
        if (app_ai_manager_is_in_multi_ai_mode())
        {
            if ((app_ai_manager_get_current_spec() != AI_SPEC_GSOUND) ||
                (app_ai_manager_spec_get_status_is_in_invalid() == true))
            {
                TRACE(1,"%s set gsound false", __func__);
                isEnable = false;
            }

            if (isEnable)
            {
                NV_EXTENSION_RECORD_T *pNvExtRec = nv_record_get_extension_entry_ptr();
                if (!(pNvExtRec->gsound_info.isGsoundEnabled))
                {
                    isEnable = false;
                }
            }
        }

        gsound_custom_init(isEnable);
#else // #ifdef BISTO_ENABLED
        app_voicepath_custom_init();
#endif

#if !defined(GSOUND_HOTWORD_EXTERNAL)
        voicepath_tid = osThreadCreate(osThread(voicepath_process), (void*)app_voicepath_mic_data_process);
#endif

    }
    else
    {
        TRACE(0, "already initialized");
    }
}

#if !defined(GSOUND_HOTWORD_EXTERNAL)
static void voicepath_process(const void *args)
{
    while (1)
    {
        osSignalWait(0x01, osWaitForever);
        ((VOICEPATH_DATA_HANDLER_T)args)();
    }
}
#endif

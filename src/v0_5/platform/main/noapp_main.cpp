/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "cmsis.h"
#include "analog.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_key.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "main_entry.h"
#include "pmu.h"

#ifdef SENSOR_TEST
#include "sensor_test.h"
#endif

#ifdef RTOS
#include "cmsis_os.h"
#ifdef KERNEL_RTX
#include "rt_Time.h"
#endif
#endif

#ifdef HWTEST
#include "hwtest.h"
#ifdef VD_TEST
#include "voice_detector.h"
#endif
#endif

#ifdef __cplusplus
#define EXTERN_C                        extern "C"
#else
#define EXTERN_C                        extern
#endif

#ifdef SLEEP_SIMU
#define TIMER_IRQ_PERIOD_MS             3
#define DELAY_PERIOD_MS                 4
#elif defined(SLEEP_TEST)
#define TIMER_IRQ_PERIOD_MS             3000
#define DELAY_PERIOD_MS                 4000
#else
#define TIMER_IRQ_PERIOD_MS             1000
#define DELAY_PERIOD_MS                 2000
#endif

#ifndef FLASH_FILL
#define FLASH_FILL                      1
#endif

#ifdef KERNEL_RTX
#define OS_TIME_STR                     "[%2u/%u]"
#define OS_CUR_TIME                     , SysTick->VAL, os_time
#else
#define OS_TIME_STR
#define OS_CUR_TIME
#endif

#if defined(MS_TIME)
#define TIME_STR                        "[%u]" OS_TIME_STR
#define CUR_TIME                        TICKS_TO_MS(hal_sys_timer_get())  OS_CUR_TIME
#elif defined(RAW_TIME)
#define TIME_STR                        "[0x%X]" OS_TIME_STR
#define CUR_TIME                        hal_sys_timer_get()  OS_CUR_TIME
#else
#define TIME_STR                        "[%u/0x%X]" OS_TIME_STR
#define CUR_TIME                        TICKS_TO_MS(hal_sys_timer_get()), hal_sys_timer_get() OS_CUR_TIME
#endif

const static unsigned char bytes[FLASH_FILL] = { 0x1, };

#if defined(DEBUG) && (DEBUG_PORT == 1)
static bool i2c_flag = false;

static void switch_between_i2c_and_uart(bool to_i2c, bool flush_trace)
{
    uint32_t lock;

    lock = int_lock();

    if (to_i2c) {
        TR_INFO(TR_MOD(MAIN), "Switch to I2C");
        if (flush_trace) {
            TRACE_FLUSH();
        }
        while (hal_trace_busy());
        hal_trace_pause();

        hal_iomux_set_analog_i2c();
    } else {
        hal_iomux_set_uart0();

        hal_trace_continue();
        if (flush_trace) {
            TRACE_FLUSH();
        }
        TR_INFO(TR_MOD(MAIN), "Switch to UART0");
    }

    int_unlock(lock);
}
#endif

#ifdef NO_TIMER
#ifdef SLEEP_TEST
static bool wakeup_flag = false;
#endif
#else
static HWTIMER_ID hw_timer = NULL;

static void timer_handler(void *param)
{
    TR_INFO(TR_MOD(MAIN), TIME_STR " Timer handler: %u", CUR_TIME, (uint32_t)param);
    hwtimer_start(hw_timer, MS_TO_TICKS(TIMER_IRQ_PERIOD_MS));
    TR_INFO(TR_MOD(MAIN), TIME_STR " Start timer %u ms", CUR_TIME, TIMER_IRQ_PERIOD_MS);
}
#endif

static int key_event_process(uint32_t key_code, uint8_t key_event)
{
    TR_INFO(TR_MOD(MAIN), "%s: code=0x%X event=%u", __FUNCTION__, key_code, key_event);

    if (key_code == HAL_KEY_CODE_PWR) {
        if (0) {
#if defined(SLEEP_TEST) && defined(NO_TIMER)
        } else if (key_event == HAL_KEY_EVENT_CLICK) {
            enum HAL_CMU_FREQ_T sys_freq;

            wakeup_flag = !wakeup_flag;
            if (wakeup_flag) {
                sys_freq = HAL_CMU_FREQ_26M;
            } else {
                sys_freq = HAL_CMU_FREQ_32K;
            }
            hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, sys_freq);

            TR_INFO(TR_MOD(MAIN), "%s: sleep=%d", __FUNCTION__, !wakeup_flag);
#endif
#if defined(DEBUG) && (DEBUG_PORT == 1)
        } else if (key_event == HAL_KEY_EVENT_LONGPRESS) {
            i2c_flag = !i2c_flag;
            switch_between_i2c_and_uart(i2c_flag, true);
#endif
        }
    }

    return 0;
}

// GDB can set a breakpoint on the main function only if it is
// declared as below, when linking with STD libraries.

int MAIN_ENTRY(void)
{
    int POSSIBLY_UNUSED ret;

    hwtimer_init();
    hal_audma_open();
    hal_gpdma_open();
#ifdef DEBUG
#if (DEBUG_PORT == 3)
    hal_iomux_set_analog_i2c();
    hal_iomux_set_uart2();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART2);
#elif (DEBUG_PORT == 2)
    hal_iomux_set_analog_i2c();
    hal_iomux_set_uart1();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#else
    hal_iomux_set_uart0();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
#endif
#endif

#if !defined(SIMU) && !defined(FPGA)
    uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
    hal_norflash_get_id(HAL_FLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
    TR_INFO(TR_MOD(MAIN), "FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
    hal_norflash_show_calib_result(HAL_FLASH_ID_0);
    ASSERT(hal_norflash_opened(HAL_FLASH_ID_0), "Failed to init flash: %d", hal_norflash_get_open_state(HAL_FLASH_ID_0));
#endif

    TR_INFO(TR_MOD(MAIN), TIME_STR " main started: filled@0x%08x", CUR_TIME, (uint32_t)bytes);

#ifndef NO_PMU
    ret = pmu_open();
    ASSERT(ret == 0, "Failed to open pmu");
#endif
    analog_open();

    hal_cmu_simu_pass();

#ifdef SIMU
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_FLASH_BOOT);
    pmu_reboot();
#else
    enum HAL_CMU_FREQ_T sys_freq;

#ifdef SLEEP_TEST
    sys_freq = HAL_CMU_FREQ_32K;
#elif defined(ULTRA_LOW_POWER)
    sys_freq = HAL_CMU_FREQ_52M;
#else
    sys_freq = HAL_CMU_FREQ_104M;
#endif

    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, sys_freq);
    TR_INFO(TR_MOD(MAIN), "CPU freq: %u", hal_sys_timer_calc_cpu_freq(5, 0));

#ifdef CHIP_WAKE_TEST
    hal_chip_wake_lock(HAL_CHIP_WAKE_LOCK_USER_31);
#endif

    hal_sleep_start_stats(10000, 10000);
#endif

#ifdef HWTEST

#ifdef USB_SERIAL_TEST
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    usb_serial_test();
#endif
#ifdef USB_SERIAL_DIRECT_XFER_TEST
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    usb_serial_direct_xfer_test();
#endif
#ifdef USB_AUDIO_TEST
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    usb_audio_test();
#endif
#ifdef I2C_TEST
    i2c_test();
#endif
#ifdef AF_TEST
    af_test();
#endif
#ifdef VD_TEST
    voice_detector_test();
#endif
#ifdef CP_TEST
    cp_test();
#endif
#ifdef SEC_ENG_TEST
    sec_eng_test();
#endif
#ifdef TDM_TEST
    tdm_test();
#endif
#ifdef A7_DSP_TEST
    a7_dsp_test();
#endif
#ifdef TRANSQ_TEST
    transq_test();
#endif
#ifdef FLASH_TEST
    flash_test();
#endif
#ifdef PSRAM_TEST
    psram_test();
#endif
#ifdef PSRAMUHS_TEST
    psramuhs_test();
#endif
#ifdef MBW_TEST
    mbw_test();
#endif
#ifdef SPI_NORFLASH_TEST
    spi_norflash_test();
#endif
#ifdef SENSOR_HUB_TEST
    sensor_hub_test();
#endif
#ifdef CODEC_SIMU
    codec_test();
#endif
#ifdef I2S_TEST
    i2s_test();
#endif
#ifdef SENSOR_TEST
    sensor_test();
#endif
#ifdef BECO_TEST
    beco_test();
#endif
#ifdef TZ_TEST
    tz_test();
#endif

#ifdef FT_TEST
    ft_test_main();
#endif

    SAFE_PROGRAM_STOP();

#endif // HWTEST

    hal_key_open(false, key_event_process);

#if defined(DEBUG) && (DEBUG_PORT == 1)
    TR_INFO(TR_MOD(MAIN), "TIP: Long press PwrKey to switch between I2C and UART0");
#endif

#ifdef NO_TIMER
#ifdef SLEEP_TEST
#if (SLEEP_TEST == 2)
    wakeup_flag = true;
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_26M);
    TR_INFO(TR_MOD(MAIN), "Single click PwrKey to sleep ...");
#else
    TR_INFO(TR_MOD(MAIN), "Enter sleep (and single click PwrKey to wakeup) ...");
#endif
#endif
#else
    hw_timer = hwtimer_alloc(timer_handler, 0);
    hwtimer_start(hw_timer, MS_TO_TICKS(TIMER_IRQ_PERIOD_MS));
    TR_INFO(TR_MOD(MAIN), TIME_STR " Start timer %u ms", CUR_TIME, TIMER_IRQ_PERIOD_MS);
#endif

    while (1) {
#if defined(SLEEP_TEST) && !defined(RTOS)
        hal_sleep_enter_sleep();
#else
#if defined(NO_TIMER) && defined(RTOS)
        osSignalWait(0x0, osWaitForever);
#else
        osDelay(DELAY_PERIOD_MS);
        TR_INFO(TR_MOD(MAIN), TIME_STR " Delay %u ms done", CUR_TIME, DELAY_PERIOD_MS);
#endif
#endif
    }

    SAFE_PROGRAM_STOP();
    return 0;
}


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
#include "plat_types.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "hal_chipid.h"
#include "bt_drv.h"
#include "bt_drv_internal.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "hal_sleep.h"
#include "watchdog.h"
#include "string.h"
#include "besbt_string.h"

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long unsigned int) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long unsigned int))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < (BIGBLOCKSIZE))

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define BT_DUMP_LEN_LINE    100
#define SLOT_SIZE           625
#if defined(__FPGA_BT_1500__) || defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
//bt max slot clock
#define MAX_SLOT_CLOCK      ((1L<<28) - 1)
#else
//bt max slot clock
#define MAX_SLOT_CLOCK      ((1L<<27) - 1)
#endif
#define CLK_SUB(clock_a, clock_b)     ((uint32_t)(((clock_a) - (clock_b)) & MAX_SLOT_CLOCK))
#define CLK_ADD_2(clock_a, clock_b)     ((uint32_t)(((clock_a) + (clock_b)) & MAX_SLOT_CLOCK))

struct bt_cb_tag bt_drv_func_cb = {NULL};
struct bt_cb_tag* bt_drv_get_func_cb_ptr(void)
{
    return &bt_drv_func_cb;
}

///only used for bt chip write patch data for speed up
void btdrv_memory_copy(uint32_t *dest,const uint32_t *src,uint16_t length)
{
    // memcpy(dest,src,length);
    uint16_t i;
    for(i=0; i<length/4; i++)
    {
        *dest++ = *src++;
    }
}

void *btdrv_memcpy_word(void * d, const void * s, size_t n)
{
    /* attempt word-sized copying only if buffers have identical alignment */
    unsigned char *d_byte = (unsigned char *)d;
    const unsigned char *s_byte = (const unsigned char *)s;
    const long unsigned int mask = sizeof(long unsigned int) - 1;

    if (unlikely(!TOO_SMALL(n))) {

       //let .txt code 4byte aligned in the loop
        __asm volatile("nop");

       if((((long unsigned int)d ^ (long unsigned int)s_byte) & mask) == 0)
        {
            /* do byte-sized copying until word-aligned or finished */
            while (((long unsigned int)d_byte) & mask) {
                if (n == 0) {
                    return d;
                }
                *(d_byte++) = *(s_byte++);
                n--;
            };

            /* do word-sized copying as long as possible */

            long unsigned int *d_word = (long unsigned int *)d_byte;
            const long unsigned int *s_word = (const long unsigned int *)s_byte;

            /* Copy 4X long words at a time if possible.  */
            while (n >= BIGBLOCKSIZE) {
                *(d_word++) = *(s_word++);
                *(d_word++) = *(s_word++);
                *(d_word++) = *(s_word++);
                *(d_word++) = *(s_word++);
                n -= BIGBLOCKSIZE;
            }

            /* Copy one long word at a time if possible.  */
            while (n >= LITTLEBLOCKSIZE) {
                *d_word++ = *s_word++;
                n -= LITTLEBLOCKSIZE;
            }

            d_byte = (unsigned char *)d_word;
            s_byte = (unsigned char *)s_word;
        }
    }

    //let .txt code 4byte aligned in the loop
    __asm volatile(".balign    4");
    __asm volatile("nop");

    /* do byte-sized copying until finished */
    while (n > 0) {
        *(d_byte++) = *(s_byte++);
        n--;
    }

    return d;
}

const uint8_t lmp_sniffer_filter_tab[51]=
{
    50,//array length
    LMP_NAME_REQ_OPCODE,
    LMP_NAME_RES_OPCODE,
    LMP_ACCEPTED_OPCODE,
    LMP_NOT_ACCEPTED_OPCODE,
    LMP_CLK_OFF_REQ_OPCODE,
    LMP_CLK_OFF_RES_OPCODE,
    //LMP_DETACH_OPCODE,
    LMP_INRAND_OPCODE,
    LMP_COMBKEY_OPCODE,
    LMP_UNITKEY_OPCODE,
    LMP_AURAND_OPCODE,
    LMP_SRES_OPCODE,
    LMP_TEMPRAND_OPCODE,
    LMP_TEMPKEY_OPCODE,
    LMP_ENC_MODE_REQ_OPCODE,
    LMP_ENC_KEY_SIZE_REQ_OPCODE,
    LMP_START_ENC_REQ_OPCODE,
    LMP_STOP_ENC_REQ_OPCODE,
    LMP_SWITCH_REQ_OPCODE,
    LMP_HOLD_OPCODE,
    LMP_HOLD_REQ_OPCODE,
    LMP_SNIFF_REQ_OPCODE,
    //LMP_UNSNIFF_REQ_OPCODE,
    LMP_PARK_REQ_OPCODE,
    LMP_SET_BSWIN_OPCODE,
    LMP_MODIF_BEACON_OPCODE,
    LMP_UNPARK_BD_REQ_OPCODE,
    LMP_UNPARK_PM_REQ_OPCODE,
    //LMP_INCR_PWR_REQ_OPCODE,
    //LMP_DECR_PWR_REQ_OPCODE,
    //LMP_MAX_PWR_OPCODE,
    //LMP_MIN_PWR_OPCODE,
    //LMP_AUTO_RATE_OPCODE,
    //LMP_PREF_RATE_OPCODE,
    LMP_VER_REQ_OPCODE,
    LMP_VER_RES_OPCODE,
    LMP_FEATS_REQ_OPCODE,
    LMP_FEATS_RES_OPCODE,
    //LMP_QOS_OPCODE,
    //LMP_QOS_REQ_OPCODE,
    LMP_SCO_LINK_REQ_OPCODE,
    LMP_RMV_SCO_LINK_REQ_OPCODE,
    //LMP_MAX_SLOT_OPCODE,
    //LMP_MAX_SLOT_REQ_OPCODE,
    LMP_TIMING_ACCU_REQ_OPCODE,
    LMP_TIMING_ACCU_RES_OPCODE,
    LMP_SETUP_CMP_OPCODE,
    LMP_USE_SEMI_PERM_KEY_OPCODE,
    LMP_HOST_CON_REQ_OPCODE,
    LMP_SLOT_OFF_OPCODE,
    LMP_PAGE_MODE_REQ_OPCODE,
    LMP_PAGE_SCAN_MODE_REQ_OPCODE,
    //LMP_SUPV_TO_OPCODE,
    LMP_TEST_ACTIVATE_OPCODE,
    LMP_TEST_CTRL_OPCODE,
    LMP_ENC_KEY_SIZE_MASK_REQ_OPCODE,
    LMP_ENC_KEY_SIZE_MASK_RES_OPCODE,
    //LMP_SET_AFH_OPCODE,
    LMP_ENCAPS_HDR_OPCODE,
    LMP_ENCAPS_PAYL_OPCODE,
    LMP_SP_CFM_OPCODE,
    LMP_SP_NB_OPCODE,
    LMP_DHKEY_CHK_OPCODE,
    LMP_PAUSE_ENC_AES_REQ_OPCODE,
};

const uint8_t lmp_ext_sniffer_filter_tab[18]=
{
    17,//array length
    LMP_ACCEPTED_EXT_EXTOPCODE,
    LMP_NOT_ACCEPTED_EXT_EXTOPCODE,
    LMP_FEATS_REQ_EXT_EXTOPCODE,
    LMP_FEATS_RES_EXT_EXTOPCODE,
    //LMP_CLK_ADJ_EXTOPCODE,
    //LMP_CLK_ADJ_ACK_EXTOPCODE,
    //LMP_CLK_ADJ_REQ_EXTOPCODE,
    //LMP_PKT_TYPE_TBL_REQ_EXTOPCODE,
    //LMP_ESCO_LINK_REQ_EXTOPCODE,
    //LMP_RMV_ESCO_LINK_REQ_EXTOPCODE,
    //LMP_CH_CLASS_REQ_EXTOPCODE,
    //LMP_CH_CLASS_EXTOPCODE,
    LMP_SSR_REQ_EXTOPCODE,
    LMP_SSR_RES_EXTOPCODE,
    LMP_PAUSE_ENC_REQ_EXTOPCODE,
    LMP_RESUME_ENC_REQ_EXTOPCODE,
    LMP_IO_CAP_REQ_EXTOPCODE,
    LMP_IO_CAP_RES_EXTOPCODE,
    LMP_NUM_COMPARISON_FAIL_EXTOPCODE,
    LMP_PASSKEY_FAIL_EXTOPCODE,
    LMP_OOB_FAIL_EXTOPCODE,
    LMP_KEYPRESS_NOTIF_EXTOPCODE,
    //LMP_PWR_CTRL_REQ_EXTOPCODE,
    LMP_PWR_CTRL_RES_EXTOPCODE,
    LMP_PING_REQ_EXTOPCODE,
    LMP_PING_RES_EXTOPCODE,
};

const uint8_t lmp_ext_sniffer_fast_cfm_tab[4]=
{
    3,//array length
    LMP_ACCEPTED_EXT_EXTOPCODE,
    LMP_NOT_ACCEPTED_EXT_EXTOPCODE,
    LMP_CH_CLASS_EXTOPCODE,
};

extern uint8_t sleep_param[];
void btdrv_set_lpo_times(void)
{
    BTDIGITAL_REG_SET_FIELD(0xD0330044, 0xFF, 0, (sleep_param[4] - 1));
}

int btdrv_slave2master_clkcnt_convert(uint32_t local_clk, uint16_t local_cnt,
                                      int32_t clk_offset, uint16_t bit_offset,
                                      uint32_t *master_clk, uint16_t *master_cnt)
{
    // Adjust bit offset and clock offset if needed
    uint32_t new_clk;
    int16_t new_cnt;

    new_clk = CLK_ADD_2(local_clk, clk_offset);
    new_cnt = (int16_t)local_cnt + bit_offset;

    if (new_cnt > SLOT_SIZE)
    {
        new_cnt -= SLOT_SIZE;
        new_clk = CLK_SUB(new_clk, 1);
    }

    *master_clk = new_clk;
    *master_cnt = new_cnt;
    return 0;
}



void btdrv_trigger_coredump(void)
{
    uint8_t *null_pointer = 0;
    *null_pointer = 0;//MPU trigger coredump
}

// This api should only be used to calculate dma capture clk diff, for chip after 1501, unit is hs and hus.
int btdrv_clkcnt_diff(int32_t clk1, int16_t cnt1,
                      int32_t clk2, int16_t cnt2,
                      int32_t *diff_clk, uint16_t *diff_bit)
{
    int32_t new_clk;
    int16_t new_cnt;
    int diff_us;

    new_clk = (int32_t)clk1 - (int32_t)clk2;
    new_cnt = cnt1 - cnt2;
    if (new_cnt < 0)
    {
        new_cnt += SLOT_SIZE;
    }else{
        new_clk -= 1;
    }

    *diff_clk = new_clk;
    *diff_bit = new_cnt;

    diff_us = new_clk * SLOT_SIZE + (SLOT_SIZE - new_cnt);

#if defined(__FPGA_BT_1500__) || defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
    return diff_us/2;
#else
    return diff_us;
#endif
}

// This api should only be used to calculate dma capture clk total us, for chip after 1501, unit is hs and hus.
int64_t btdrv_clkcnt_to_us(uint32_t clk, uint16_t cnt)
{
    int64_t total_cnt;
#if defined(__FPGA_BT_1500__) || defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
    total_cnt = ((int64_t)clk * (int64_t)SLOT_SIZE + (int64_t)cnt)/2;
#else
    total_cnt = (int64_t)clk * (int64_t)SLOT_SIZE + (int64_t)cnt;
#endif
    return total_cnt;
}

void WEAK btdrv_set_controller_trace_enable(uint8_t trace_level)
{
}

void WEAK btdrv_set_lmp_trace_enable(void)
{
}

void WEAK bt_drv_reg_op_enable_dma_tc(uint8_t adma_ch)
{
}

void WEAK bt_drv_reg_op_disable_dma_tc(uint8_t adma_ch)
{
}

void WEAK bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(uint32_t *btclk, uint16_t *btcnt, uint8_t adma_ch)
{
}

void WEAK bt_drv_reg_op_multi_ibrt_music_config(uint8_t* link_id, uint8_t* active, uint8_t num)
{
}

void WEAK bt_drv_digital_config_for_ble_adv(bool en)
{
}

void WEAK bt_drv_reg_op_clean_flags_of_ble_and_sco(void)
{
}

void WEAK bt_drv_reg_op_set_ibrt_reject_sniff_req(bool en)
{
}

void WEAK bt_drv_reg_op_set_replace_mobile_addr(uint8_t enable, uint8_t* addr)
{
}

bool WEAK btdrv_reg_op_get_local_name(uint8_t* name)
{
    return false;
}

uint8_t WEAK bt_drv_reg_op_get_sync_id_by_conhdl(uint16_t conhdl)
{
    return MAX_NB_SYNC;
}

void WEAK bt_drv_reg_op_clear_skip_flag(void)
{
}

void WEAK btdrv_reconn(bool en)
{
}

void WEAK bt_drv_reg_op_reset_agc_thd(void)
{
}

void WEAK bt_drv_reg_op_set_agc_thd(bool isMaster, bool isSco)
{

}

void WEAK btdrv_regist_le_teset_end_callback(le_test_end_handler_func cb)
{
}

int8_t WEAK btdrv_reg_op_txpwr_idx_to_rssidbm(uint8_t txpwr_idx)
{
    return 0;
}

static bool  g_controller_trace_dump_enable = false;
bool btdrv_get_controller_trace_dump_enable(void)
{
    return g_controller_trace_dump_enable;
}
void btdrv_set_controller_trace_dump_enable(void)
{
    g_controller_trace_dump_enable = true;
}

void btdrv_btc_fault_dump(void)
{
    watchdog_hw_stop();
    bt_drv_reg_op_crash_dump();
}

void btdrv_dump_mem(uint8_t *dump_mem_start, uint32_t dump_length, uint8_t dump_type)
{
#if CONTROLLER_DUMP_ENABLE
    uint32_t dump_counter=0;
    uint32_t dump_line=0;

    hal_sys_timer_delay(MS_TO_TICKS(100));
    switch (dump_type)
    {
        case BT_SUB_SYS_TYPE:
            BT_DRV_TRACE(1,"controller dump start, dump len=0x%x",dump_length);
            break;

        case MCU_SYS_TYPE:
            BT_DRV_TRACE(1,"mcu dump start, dump len=0x%x",dump_length);
            break;

        case BT_EM_AREA_1_TYPE:
            BT_DRV_TRACE(1,"em_area1 dump start, dump len=0x%x",dump_length);
            break;

        case BT_EM_AREA_2_TYPE:
            BT_DRV_TRACE(1,"em_area2 dump start, dump len=0x%x",dump_length);
            break;
    }
    while (dump_counter < dump_length)
    {
        uint32_t data_left = dump_length - dump_counter;
        dump_line += 1;
        if (data_left >= BT_DUMP_LEN_LINE)
        {
            BT_DRV_TRACE(1,"dump_line=%d", dump_line);
            DUMP8("%02x", dump_mem_start, BT_DUMP_LEN_LINE);
            hal_trace_flush_buffer();
            dump_mem_start += BT_DUMP_LEN_LINE;
            dump_counter += BT_DUMP_LEN_LINE;
            hal_sys_timer_delay(MS_TO_TICKS(50));
        }
        else
        {
            BT_DRV_TRACE(1,"dump_line=%d", dump_line);
            DUMP8("%02x", dump_mem_start, data_left);
            hal_trace_flush_buffer();
            hal_sys_timer_delay(MS_TO_TICKS(50));
            break;
        }
    }
    switch (dump_type)
    {
        case BT_SUB_SYS_TYPE:
            BT_DRV_TRACE(0,"controller dump end");
            break;

        case MCU_SYS_TYPE:
            BT_DRV_TRACE(0,"mcu dump end");
            break;

        case BT_EM_AREA_1_TYPE:
            BT_DRV_TRACE(0,"em_area1 dump end");
            break;

        case BT_EM_AREA_2_TYPE:
            BT_DRV_TRACE(0,"em_area2 dump end");
            break;
    }
#endif
}
void btdrv_trace_config(BT_CONTROLER_TRACE_TYPE trace_config)
{
    if (trace_config & BT_CONTROLER_TRACE_TYPE_INTERSYS)
    {
        btdrv_set_intersys_trace_enable();
    }

    if (trace_config & BT_CONTROLER_TRACE_TYPE_CONTROLLER)
    {
        btdrv_set_controller_trace_enable(0x03);
    }

    if (trace_config & BT_CONTROLER_TRACE_TYPE_LMP_TRACE)
    {
        btdrv_set_lmp_trace_enable();
    }

    if (trace_config & BT_CONTROLER_TRACE_TYPE_SPUV_HCI_BUFF)
    {
        btdrv_set_spuv_hci_buff_trace_enable();
    }

    if (trace_config & BT_CONTROLER_FILTER_TRACE_TYPE_A2DP_STREAM)
    {
        btdrv_set_a2dp_stream_trace_disable();
    }
    if (trace_config & BT_CONTROLER_TRACE_TYPE_SPUV_HCI_BUFF_HIGH)
    {
        btdrv_set_spuv_hci_buff_trace_enable_high();
    }
#if CONTROLLER_MEM_LOG_ENABLE
    btdrv_set_controller_trace_dump_enable();
#endif
}

void btdrv_enable_tports(void)
{
#ifdef __BT_DEBUG_TPORTS__
    hal_iomux_set_bt_tport();
    bt_drv_bt_tport_type_config();
#endif
}

bool bt_drv_is_enhanced_ibrt_rom(void)
{
    bool ret = false;

#ifdef CHIP_BEST2300P
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
        ret = true;
#elif CHIP_BEST1400
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
        ret = true;
#elif CHIP_BEST1402
    ret = true;
#elif CHIP_BEST2300A
    ret = true;
#elif FPGA
    ret = true;
#elif CHIP_BEST2300
    ret = true;
#elif CHIP_BEST1501
    ret = true;
#elif CHIP_BEST2003
    ret = true;
#elif CHIP_BEST1305
    ret = true;
#endif

    return ret;
}

bool bt_drv_is_esco_auto_accept_support(void)
{
    bool ret = false;

#ifdef CHIP_BEST2300P
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
        ret = true;
#elif CHIP_BEST1400
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_4)
        ret = true;
#elif CHIP_BEST1402
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
        ret = true;
#elif CHIP_BEST2300A
        ret = true;
#elif CHIP_BEST1501
        ret = true;
#elif CHIP_BEST2003
        ret = true;
#elif CHIP_BEST1305
        ret = true;
#endif

    return ret;
}

bool btdrv_is_acl_ecc_softbit_support(void)
{
    bool ret = false;
#ifdef CHIP_BEST2300A
        ret = true;
#elif CHIP_BEST1501
        ret = true;
#elif CHIP_BEST2003
        ret = true;
#elif CHIP_BEST1305
        ret = true;
#endif

    return ret;
}


void btdrv_softbit_enable(uint16_t connhdl, uint8_t type1,uint8_t type2,uint8_t type3, uint8_t num)
{
    if(btdrv_is_acl_ecc_softbit_support())
    {
#if (defined(SOFTBIT_EN))
        btdrv_softbit_config(connhdl,  type1, type2, type3,  num);
#endif
    }
}

void bt_drv_bt_tport_type_config(void)
{
    uint32_t tport_type = 0xb1b1;
#ifdef __BT_DEBUG_TPORTS__
    tport_type = TPORT_TYPE;
#endif
    BTDIGITAL_REG(0xd0220050) = tport_type;
    BTDIGITAL_REG(0xd0340000) = 0x23620200;
    BT_DRV_TRACE(1,"BT_DRV: tport type=0x%x",tport_type);
}

bool bt_drv_is_bes_fa_mode_en(void)
{
    bool ret = false;
#ifdef __BES_FA_MODE__
    ret = true;
#endif
    return ret;
}

bool bt_drv_is_support_multipoint_ibrt(void)
{
    bool ret = false;
#if defined(__FPGA_BT_1500__) || defined(CHIP_BEST1501) || defined(CHIP_BEST2003)
    ret = true;
#endif
    return ret;
}

void bt_drv_enable_hw_spi(uint8_t elt_idx, bool hwspien)
{
#if defined(CHIP_BEST2300A) || defined(CHIP_BEST1305)
    bt_drv_reg_op_hw_spi_en_setf(elt_idx, hwspien);
#endif
}

struct btstack_chip_config_t
{
    uint16_t hci_dbg_set_sync_config_cmd_opcode;
    uint16_t hci_dbg_set_sco_switch_cmd_opcode;
} g_bt_drv_btstack_chip_config = {
    HCI_DBG_SET_SYNC_CONFIG_CMD_OPCODE,
    HCI_DBG_SET_SCO_SWITCH_CMD_OPCODE,
};

void* bt_drv_get_btstack_chip_config(void)
{
    return &g_bt_drv_btstack_chip_config;
}

void bt_drv_hwspi_select(uint8_t link_id, uint8_t spi_idx)
{
#ifdef __FA_RX_GAIN_CTRL__
    btdrv_spi_trig_select(link_id, spi_idx);
#endif
}

static void bt_drv_sleep(void)
{
    bt_drv_reg_op_set_hwagc_read_en(0);
}

static void bt_drv_wakeup(void)
{
    bt_drv_reg_op_set_hwagc_read_en(1);
}

static int btdrv_pm_notif_handler(enum HAL_PM_STATE_T state)
{
    if (state == HAL_PM_STATE_NORMAL_SLEEP || state == HAL_PM_STATE_POWER_DOWN_SLEEP) {
        bt_drv_sleep();
    } else if (state == HAL_PM_STATE_NORMAL_WAKEUP || state == HAL_PM_STATE_POWER_DOWN_WAKEUP) {
        bt_drv_wakeup();
    }

    return 0;
}

void btdrv_pm_register_notif_handler(void)
{
    hal_pm_notif_register(HAL_PM_USER_DRV, btdrv_pm_notif_handler);
}

void btdrv_pm_deregister_notif_handler(void)
{
    hal_pm_notif_deregister(HAL_PM_USER_DRV, btdrv_pm_notif_handler);
}

/// BCH size
#define LD_BCH_SIZE                          8

static const uint8_t ld_util_bch_ppolynom[LD_BCH_SIZE] =
{
    0x83, 0x84, 0x8D, 0x96, 0xBB, 0xCC, 0x54, 0xFC
};

static const uint8_t ld_util_bch_gpolynom[LD_BCH_SIZE] =
{
    0x00, 0x00, 0x00, 0x05, 0x85, 0x71, 0x3D, 0xA9
};

static void ld_util_bch_modulo(uint8_t *dividend, const uint8_t *divisor, uint8_t *rest)
{
    uint8_t clock;
    uint8_t carry;
    uint8_t index;
    uint8_t dividend_tmp[LD_BCH_SIZE];

    // Copy dividend vector in a temporary vector
    // And reset rest vector
    for (index = 0 ; index < LD_BCH_SIZE ; index++)
    {
        dividend_tmp[index] = dividend[index];
        rest[index] = 0;
    }

    // Execute 64 times the LFSR process
    for (clock = 0 ; clock < 64 ; clock++)
    {
        // Store bit Rest(degree-1) in carry (assume degree of G is 32)
        carry = rest[3] & 0x02;

        // the rest is shifted of 1 bit Left
        // The MSB of rest if lost and the MSB of dividend is shifted in LSB
        rest[0] = (rest[0] << 1) | ((rest[1] & 0x80) >> 7);
        rest[1] = (rest[1] << 1) | ((rest[2] & 0x80) >> 7);
        rest[2] = (rest[2] << 1) | ((rest[3] & 0x80) >> 7);
        rest[3] = (rest[3] << 1) | ((rest[4] & 0x80) >> 7);
        rest[4] = (rest[4] << 1) | ((rest[5] & 0x80) >> 7);
        rest[5] = (rest[5] << 1) | ((rest[6] & 0x80) >> 7);
        rest[6] = (rest[6] << 1) | ((rest[7] & 0x80) >> 7);
        rest[7] = (rest[7] << 1) | ((dividend_tmp[0] & 0x80) >> 7);

        // the dividend_tmp is shifted of 1 bit Left (a 0 is shifted in LSB)
        dividend_tmp[0] = (dividend_tmp[0] << 1) | ((dividend_tmp[1] & 0x80) >> 7);
        dividend_tmp[1] = (dividend_tmp[1] << 1) | ((dividend_tmp[2] & 0x80) >> 7);
        dividend_tmp[2] = (dividend_tmp[2] << 1) | ((dividend_tmp[3] & 0x80) >> 7);
        dividend_tmp[3] = (dividend_tmp[3] << 1) | ((dividend_tmp[4] & 0x80) >> 7);
        dividend_tmp[4] = (dividend_tmp[4] << 1) | ((dividend_tmp[5] & 0x80) >> 7);
        dividend_tmp[5] = (dividend_tmp[5] << 1) | ((dividend_tmp[6] & 0x80) >> 7);
        dividend_tmp[6] = (dividend_tmp[6] << 1) | ((dividend_tmp[7] & 0x80) >> 7);
        dividend_tmp[7] = (dividend_tmp[7] << 1);

        // If bit carry value was 1
        if (carry != 0)
        {
            // rest = rest XOR Divisor
            for (index = 0 ; index < LD_BCH_SIZE ; index++)
            {
                rest[index] ^= divisor[index];
            }
        }
    }
}

void ld_util_bch_create(uint8_t *lap, uint8_t *bch)
{
    uint8_t vector[LD_BCH_SIZE];
    uint8_t vector2[LD_BCH_SIZE];
    uint8_t index;

    // Copy lap and Appended in bch
    if (lap[2] & 0x80)
    {
        vector[0] = 0x4C;                   // If a23 = 1
    }
    else
    {
        vector[0] = 0xB0;                   // If a23 = 0
    }

    vector[0] |= lap[2] >> 6;
    vector[1] = lap[2] << 2 | lap[1] >> 6;
    vector[2] = lap[1] << 2 | lap[0] >> 6;
    vector[3] = lap[0] << 2;

    // Xor Vector and PN (Vector contains only 30 significant bits)
    for (index = 0 ; index < 4; index++)
    {
        vector[index] ^= ld_util_bch_ppolynom[index];
    }

    // Reset 34 last bits
    vector[3] &= 0xFC;
    vector[4]  = 0;
    vector[5]  = 0;
    vector[6]  = 0;
    vector[7]  = 0;

    // Generate Parity bits Vector Modulo G
    ld_util_bch_modulo(vector, ld_util_bch_gpolynom, vector2);

    // Create CodeWord (concatenate Modulo result and Xored Vector)
    vector[3] |= vector2[3];
    vector[4]  = vector2[4];
    vector[5]  = vector2[5];
    vector[6]  = vector2[6];
    vector[7]  = vector2[7];

    // Xor codeWord and PN
    for (index = 0 ; index < 8; index++)
    {
        bch[7-index] = vector[index] ^ ld_util_bch_ppolynom[index];
    }
}

void bt_drv_rssi_dump_handler(void)
{
    rx_agc_t bt_agc ={0,0xf};
    for(int i = 0; i<MAX_NB_ACTIVE_ACL; i++)
    {
        bt_drv_reg_op_read_rssi_in_dbm(BT_ACL_CONHDL_BIT+i, &bt_agc);
        BT_DRV_TRACE(3,"BT=0x%x,rssi=%d,gain=%d",(BT_ACL_CONHDL_BIT+i), bt_agc.rssi, bt_agc.rxgain);
        bt_agc.rssi = 0;
        bt_agc.rxgain = 0xf;
    }

    rx_agc_t ble_agc ={0,0xf};
    bt_drv_reg_op_read_ble_rssi_in_dbm(0,&ble_agc);
    BT_DRV_TRACE(2,"BLE rssi=%d,gain=%d\n", ble_agc.rssi, ble_agc.rxgain);
}

#if !defined(CHIP_BEST1501) && !defined(CHIP_BEST2003)
bool bt_drv_error_check_handler(void)
{
    bool ret = false;
#if defined(CHIP_BEST2300) || defined(CHIP_BEST1400) || defined(CHIP_BEST1402) \
    || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) || defined(CHIP_BEST1305)
    if(BTDIGITAL_REG(BT_ERRORTYPESTAT_ADDR) !=0 ||
        (BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE) !=0 &&
        BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE) !=0x42))
    {
        BT_DRV_TRACE(1,"BT_DRV:digital assert,error code=0x%x", BTDIGITAL_REG(BT_ERRORTYPESTAT_ADDR));
        ret = true;
    }
#endif
    return ret;
}
#endif

size_t  memcpy_s(void *dst,size_t dstMax,const void *src, size_t srcMax)
{
    memcpy(dst, src, srcMax);
    return 0;
}

size_t memset_s(dst0,dstMax, c0, srcMax)
                void *dst0;
                size_t dstMax;
                int c0;
                size_t srcMax;
{
    memset(dst0, c0, srcMax);
    return 0;
}

void btdrv_le_rx_test_v2(uint8_t rx_channel, uint8_t phy, uint8_t mod_idx)
{
    BT_DRV_TRACE(0, "LE_RX_TEST: RX channel=%d, PHY=%d, mod idx=%d", rx_channel, phy, mod_idx);
    uint8_t hci_cmd[] =
    {
        0x01, 0x33, 0x20, 0x03, 0x00, 0x00, 0x00
    };

    hci_cmd[4] = rx_channel;
    hci_cmd[5] = phy;
    hci_cmd[6] = mod_idx;

    btdrv_SendData(hci_cmd, sizeof(hci_cmd));
}

void btdrv_le_tx_test_v2(uint8_t tx_channel, uint8_t data_len, uint8_t pkt_payload, uint8_t phy)
{
    BT_DRV_TRACE(0, "LE_TX_TEST: TX channel=%d,data len=%d, payload=%d, PHY=%d", tx_channel, data_len, pkt_payload, phy);
    uint8_t hci_cmd[] =
    {
        0x01, 0x34, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00
    };

    hci_cmd[4] = tx_channel;
    hci_cmd[5] = data_len;
    hci_cmd[6] = pkt_payload;
    hci_cmd[7] = phy;

    btdrv_SendData(hci_cmd, sizeof(hci_cmd));
}

void btdrv_le_test_end(void)
{
    BT_DRV_TRACE(0, "LE TEST END!!");
    uint8_t hci_cmd[] =
    {
        0x01, 0x1f, 0x20, 0x00
    };

    btdrv_SendData(hci_cmd, sizeof(hci_cmd));
}

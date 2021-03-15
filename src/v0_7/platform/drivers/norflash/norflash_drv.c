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
#include "cmsis.h"
#include "norflash_drv.h"
#include "cmsis.h"
#include "hal_cache.h"
#include "hal_norflaship.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "norflash_cfg.h"
#include "norflash_gd25q32c.h"
#include "plat_addr_map.h"
#include "string.h"
#include "tool_msg.h"

#ifdef PROGRAMMER
#include "sys_api_programmer.h"
#include "task_schedule.h"
#else
#define TASK_SCHEDULE                           true
#endif

#if defined(FLASH_DTR) && defined(CHIP_BEST1402)
#error "Not support FLASH_DTR on best1402 yet (be careful with 0x9F command issue)"
#endif
#if !defined(ROM_BUILD) && !defined(PROGRAMMER)
#if (CHIP_FLASH_CTRL_VER >= 2)
#define FLASH_BURST_WRAP
#endif
#if defined(FLASH_LOCK_CP_ACCESS) && defined(CHIP_HAS_CP) && (CHIP_FLASH_CTRL_VER >= 4)
#define FLASH_MEM_READ_BUS_LOCK
#endif
#endif

#define SAMP_DELAY_PRIO_FALLING_EDGE

#define WB_UNIQUE_ID_LEN                        8
#define XMC_UNIQUE_ID_LEN                       8
#define ZBIT_UNIQUE_ID_LEN                      16
#define PUYA_UNIQUE_ID_LEN                      16
#define GD_UNIQUE_ID_LEN                        18

#define NORFLASH_UNIQUE_ID_LEN                  18

#define NORFLASH_MAX_DIV                        0xFF

#define NORFLASH_DEFAULT_MAX_SPEED              (104 * 1000 * 1000)

#define NORFLASH_DIV1_MAX_SPEED                 (99 * 1000 * 1000)

#define NORFLASH_SPEED_RATIO_DENOMINATOR        8

#define XTS_UNIQUE_ID_LEN                       16
#define XTS_UNIQUE_ID_CMD                       0x5A
#define XTS_UNIQUE_ID_PARAM                     0x00019400

// GigaDevice
extern const struct NORFLASH_CFG_T gd25le128e_cfg;
extern const struct NORFLASH_CFG_T gd25lq64c_cfg;
extern const struct NORFLASH_CFG_T gd25lq32c_cfg;
extern const struct NORFLASH_CFG_T gd25lq16c_cfg;
extern const struct NORFLASH_CFG_T gd25lq80c_cfg;
extern const struct NORFLASH_CFG_T gd25q32c_cfg;
extern const struct NORFLASH_CFG_T gd25q80c_cfg;
extern const struct NORFLASH_CFG_T gd25d40c_cfg;
extern const struct NORFLASH_CFG_T gd25d20c_cfg;

// Puya
extern const struct NORFLASH_CFG_T p25q256l_cfg;
extern const struct NORFLASH_CFG_T p25q128l_cfg;
extern const struct NORFLASH_CFG_T p25q64l_cfg;
extern const struct NORFLASH_CFG_T p25q32l_cfg;
extern const struct NORFLASH_CFG_T p25q16l_cfg;
extern const struct NORFLASH_CFG_T p25q80h_cfg;
extern const struct NORFLASH_CFG_T p25q21h_cfg;
extern const struct NORFLASH_CFG_T p25q40h_cfg;

extern const struct NORFLASH_CFG_T p25q32sl_cfg;
extern const struct NORFLASH_CFG_T p25q16sl_cfg;

// XTS
extern const struct NORFLASH_CFG_T xt25q08b_cfg;

// XMC
extern const struct NORFLASH_CFG_T xm25qu128c_cfg;
extern const struct NORFLASH_CFG_T xm25qh16c_cfg;
extern const struct NORFLASH_CFG_T xm25qh80b_cfg;

// ZBIT
extern const struct NORFLASH_CFG_T zb25vq128b_cfg;

// EON
extern const struct NORFLASH_CFG_T en25s80b_cfg;

// WINBOND
extern const struct NORFLASH_CFG_T w25q32fw_cfg;
extern const struct NORFLASH_CFG_T w25q128jw_cfg;

static const struct NORFLASH_CFG_T * const flash_list[] = {
    // ----------------------
    // GigaDevice
    // ----------------------
#if defined(__NORFLASH_GD25LE128E__) //|| defined(__NORFLASH_ALL__)
    &gd25le128e_cfg,
#endif
#if defined(__NORFLASH_GD25LQ64C__) || defined(__NORFLASH_ALL__)
    &gd25lq64c_cfg,
#endif
#if defined(__NORFLASH_GD25LQ32C__) || defined(__NORFLASH_ALL__)
    &gd25lq32c_cfg,
#endif
#if defined(__NORFLASH_GD25LQ6C__) || defined(__NORFLASH_ALL__)
    &gd25lq16c_cfg,
#endif
#if defined(__NORFLASH_GD25LQ80C__) || defined(__NORFLASH_ALL__)
    &gd25lq80c_cfg,
#endif
#if defined(__NORFLASH_GD25Q32C__) || defined(__NORFLASH_ALL__)
    &gd25q32c_cfg,
#endif
#if defined(__NORFLASH_GD25Q80C__) || defined(__NORFLASH_ALL__)
    &gd25q80c_cfg,
#endif
#if defined(__NORFLASH_GD25D40C__) || defined(__NORFLASH_ALL__)
    &gd25d40c_cfg,
#endif
#if defined(__NORFLASH_GD25D20C__) || defined(__NORFLASH_ALL__)
    &gd25d20c_cfg,
#endif

    // ----------------------
    // Puya
    // ----------------------
#if defined(__NORFLASH_P25Q256L__) || defined(__NORFLASH_ALL__)
    &p25q256l_cfg,
#endif
#if defined(__NORFLASH_P25Q128L__) || defined(__NORFLASH_ALL__)
    &p25q128l_cfg,
#endif
#if defined(__NORFLASH_P25Q64L__) || defined(__NORFLASH_ALL__)
    &p25q64l_cfg,
#endif
#if defined(__NORFLASH_P25Q32L__) || defined(__NORFLASH_ALL__)
    &p25q32l_cfg,
#endif
#if defined(__NORFLASH_P25Q16L__) || defined(__NORFLASH_ALL__)
    &p25q16l_cfg,
#endif
#if defined(__NORFLASH_P25Q80H__) || defined(__NORFLASH_ALL__)
    &p25q80h_cfg,
#endif
#if defined(__NORFLASH_P25Q21H__) || defined(__NORFLASH_ALL__)
    &p25q21h_cfg,
#endif
#if defined(__NORFLASH_P25Q40H__) || defined(__NORFLASH_ALL__)
    &p25q40h_cfg,
#endif

#if defined(__NORFLASH_P25Q32SL__) || defined(__NORFLASH_ALL__)
    &p25q32sl_cfg,
#endif
#if defined(__NORFLASH_P25Q16SL__) || defined(__NORFLASH_ALL__)
    &p25q16sl_cfg,
#endif

    // ----------------------
    // Zbit
    // ----------------------
#if defined(__NORFLASH_ZB25VQ128B__) //|| defined(__NORFLASH_ALL__)
    &zb25vq128b_cfg,
#endif

    // ----------------------
    // Xinxin
    // ----------------------
#if defined(__NORFLASH_XM25QU128C__) //|| defined(__NORFLASH_ALL__)
    &xm25qu128c_cfg,
#endif
#if defined(__NORFLASH_XM25QH16C__) //|| defined(__NORFLASH_ALL__)
    &xm25qh16c_cfg,
#endif
#if defined(__NORFLASH_XM25QH80B__) || defined(__NORFLASH_ALL__)
    &xm25qh80b_cfg,
#endif

    // ----------------------
    // XTS
    // ----------------------
#if defined(__NORFLASH_XT25Q08B__)
    &xt25q08b_cfg,
#endif

    // ----------------------
    // EON
    // ----------------------
#if defined(__NORFLASH_EN25S80B__)
    &en25s80b_cfg,
#endif

    // ----------------------
    // WINBOND
    // ----------------------
#if defined(__NORFLASH_W25Q32FW__) || defined(__NORFLASH_SIMU__)
    &w25q32fw_cfg,
#endif
#if defined(__NORFLASH_W25Q128JV__) || defined(__NORFLASH_ALL__)
    &w25q128jw_cfg,
#endif
};

// Sample delay will be larger if:
// 1) flash speed is higher (major impact)
// 2) vcore voltage is lower (secondary major impact)
// 3) flash voltage is lower (minor impact)

// Sample delay unit:
// V1: 1/2 source_clk cycle when <= 2, 1 source_clk cycle when >= 2
// V2: 1/2 source_clk cycle when <= 4, 1 source_clk cycle when >= 4

// Flash clock low to output valid delay:
// T_clqv: 7 ns

// Flash IO latency:
// BEST1000/3001/1400: 4 ns
// BEST2000: 5 ns
// BEST2300: 2 ns

// Flash output time: T_clqv + T_io_latency
// Falling edge sample time: one spi_clk cycle (should > flash output time)

#ifdef CHIP_BEST2300
#define FALLING_EDGE_SAMPLE_ADJ_FREQ            (110 * 1000 * 1000) // about 9 ns
#else
#define FALLING_EDGE_SAMPLE_ADJ_FREQ            (77 * 1000 * 1000) // about 13 ns
#endif

#if (CHIP_FLASH_CTRL_VER <= 1)
#define SAM_EDGE_FALLING                        (1 << 4)
#define SAM_NEG_PHASE                           (1 << 5)
#define SAMDLY_MASK                             (0xF << 0)

#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
#define DIV2_SAMP_DELAY_FALLING_EDGE_IDX        1
#define DIVN_SAMP_DELAY_FALLING_EDGE_IDX        2
// Sample delays: 1, 1.5, 2, 3
static const uint8_t samdly_list_divn[] = { 1, SAM_NEG_PHASE | SAM_EDGE_FALLING | 2, 2, 3, };
#else
// Sample delays: 0, 0.5, 1, 1.5, 2, 3, 4, 5, 6, 7
static const uint8_t samdly_list_divn[] = { /*0,*/ SAM_EDGE_FALLING | 1, 1, SAM_NEG_PHASE | SAM_EDGE_FALLING | 2, 2, 3, 4, /*5, 6, 7,*/ };
#endif
#else
#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
#ifdef CHIP_BEST1402
#define DIV1_SAMP_DELAY_FALLING_EDGE_IDX        2
#else
#define DIV1_SAMP_DELAY_FALLING_EDGE_IDX        1
#endif
static const uint8_t samdly_list_div1[] = { 0, 1, 2, 3, };
#define DIV2_SAMP_DELAY_FALLING_EDGE_IDX        1
#define DIVN_SAMP_DELAY_FALLING_EDGE_IDX        3
static const uint8_t samdly_list_divn[] = { 2, 3, 4, 5, 6, 7, 8, 9, };
#else
static const uint8_t samdly_list_divn[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, };
#endif
#endif

static uint8_t flash_idx[HAL_FLASH_ID_NUM];
static uint8_t div_read[HAL_FLASH_ID_NUM];
static uint8_t div_std_read[HAL_FLASH_ID_NUM];
static uint8_t div_others[HAL_FLASH_ID_NUM];
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
static uint8_t div_dtr_read[HAL_FLASH_ID_NUM];
#endif
static uint32_t norflash_op_mode[HAL_FLASH_ID_NUM];
static bool falling_edge_adj[HAL_FLASH_ID_NUM];
static uint8_t sample_delay_index[HAL_FLASH_ID_NUM];

#ifdef FLASH_SUSPEND
static uint32_t check_irq[(USER_IRQn_QTY + 31) / 32];
static bool specific_irq_check;
#endif

#ifdef FLASH_MEM_READ_BUS_LOCK
static uint32_t mrb_status;
#endif

#ifdef FLASH_CALIB_DEBUG
static uint32_t norflash_source_clk;
static uint32_t norflash_speed;
static uint8_t calib_matched_idx[DRV_NORFLASH_CALIB_QTY];
static uint8_t calib_matched_cnt[DRV_NORFLASH_CALIB_QTY];
static uint8_t calib_final_idx[DRV_NORFLASH_CALIB_QTY];
#endif

static void norflash_delay(uint32_t us)
{
#ifdef CHIP_BEST1000
    hal_sys_timer_delay(US_TO_TICKS(us));
#else
    hal_sys_timer_delay_us(us);
#endif
}

#ifdef FLASH_HPM
static int norflash_set_hpm(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_cmd_addr(id, GD25Q32C_CMD_HIGH_PERFORMANCE, 0);
    } else {
        norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    }
    norflaship_busy_wait(id);

    return 0;
}
#endif

#ifdef FLASH_SUSPEND
static int norflash_suspend(enum HAL_FLASH_ID_T id)
{
    norflaship_clear_fifos(id);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_PROGRAM_ERASE_SUSPEND, 0);
    // Suspend time: 20~30 us
    norflash_delay(40);
    return 0;
}

static int norflash_resume(enum HAL_FLASH_ID_T id)
{
    norflaship_clear_fifos(id);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_PROGRAM_ERASE_RESUME, 0);
    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
        // PUYA flash requires the mean interval of resume to suspend >= 250us
        norflash_delay(250);
    } else {
        // At least resume the work for 100 us to avoid always staying in suspended state
        norflash_delay(100);
    }
    return 0;
}
#endif

uint8_t norflash_read_status_s0_s7(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    norflash_read_reg(id, GD25Q32C_CMD_READ_STATUS_S0_S7, &val, 1);
    return val;
}

uint8_t norflash_read_status_s8_s15(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    norflash_read_reg(id, GD25Q32C_CMD_READ_STATUS_S8_S15, &val, 1);
    return val;
}

static int norflash_status_WEL(enum HAL_FLASH_ID_T id)
{
    uint32_t status;
    status = norflash_read_status_s0_s7(id);
    return !!(status & GD25Q32C_WEL_BIT_MASK);
}

static int norflash_status_WIP(enum HAL_FLASH_ID_T id)
{
    uint32_t status;
    status = norflash_read_status_s0_s7(id);
    return !!(status & GD25Q32C_WIP_BIT_MASK);
}

void norflash_status_WEL_0_wait(enum HAL_FLASH_ID_T id)
{
    while (norflash_status_WEL(id) == 0 && TASK_SCHEDULE);
}

#ifdef FLASH_SUSPEND
int norflash_suspend_check_irq(uint32_t irq_num)
{
    uint32_t idx;
    uint32_t offset;

    idx = irq_num / 32;
    offset = irq_num % 32;

    if (idx >= ARRAY_SIZE(check_irq)) {
        return 1;
    }

    check_irq[idx] |= (1 << offset);
    specific_irq_check = true;

    return 0;
}

static int norflash_system_active(void)
{
    if (specific_irq_check) {
        return hal_sleep_specific_irq_pending(check_irq, ARRAY_SIZE(check_irq));
    } else {
        return hal_sleep_irq_pending();
    }
}
#endif

enum HAL_NORFLASH_RET_T norflash_status_WIP_1_wait(enum HAL_FLASH_ID_T id, int suspend)
{
    while (norflash_status_WIP(id) && TASK_SCHEDULE) {
#ifdef FLASH_SUSPEND
        if (suspend && norflash_system_active()) {
            norflash_suspend(id);
            return HAL_NORFLASH_SUSPENDED;
        }
#endif
    }

    return HAL_NORFLASH_OK;
}

#ifdef FLASH_BURST_WRAP
static int norflash_set_burst_wrap(enum HAL_FLASH_ID_T id, uint32_t len)
{
    uint8_t val;

    if (len == 64) {
        val = (1 << 6) | (1 << 5);
    } else if (len == 32) {
        val = (1 << 6);
    } else if (len == 16) {
        val = (1 << 5);
    } else if (len == 8) {
        val = 0;
    } else if (len == 0) {
        // Disable wrap around
        val = (1 << 4);
    } else {
        return 1;
    }

    norflaship_clear_txfifo(id);
    norflaship_write_txfifo(id, &val, 1);
    norflaship_cmd_addr(id, GD25Q32C_CMD_SET_BURST_WRAP, 0);
    norflash_status_WIP_1_wait(id, 0);

    return 0;
}
#endif

static int norflash_set_continuous_read(enum HAL_FLASH_ID_T id, uint8_t on)
{
    uint8_t cmd;

    if (on) {
        norflaship_continuous_read_on(id);

        norflaship_continuous_read_mode_bit(id, flash_list[flash_idx[id]]->crm_en_bits);

        // Continuous Read Mode takes effect after the first read
    } else {
        norflaship_continuous_read_off(id);

        norflaship_continuous_read_mode_bit(id, flash_list[flash_idx[id]]->crm_dis_bits);

        if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
            if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DTR) {
                cmd = GD25Q32C_CMD_FAST_DTR_QUAD_IO_READ;
            } else
#endif
            {
                cmd = GD25Q32C_CMD_FAST_QUAD_IO_READ;
            }
        } else {
            norflaship_quad_mode(id, 0);

            if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_IO) {
                cmd = GD25Q32C_CMD_FAST_DUAL_IO_READ;
            } else {
                norflaship_dual_mode(id, 0);

                // Command 0x03 and address 0xFFFFFE will make M4=1 (mode bit 4)
                // at both quad and dual continuous read modes, which will disable it
                cmd = GD25Q32C_CMD_STANDARD_READ;
            }
        }

        norflaship_clear_rxfifo(id);
        if (cmd == GD25Q32C_CMD_STANDARD_READ) {
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
            if (norflash_get_supported_mode(id) & HAL_NORFLASH_OP_MODE_DTR) {
                // Command 0x9F will make M4=1 (mode bit 4) at DTR quad and dual continuous read modes,
                // which will disable it
                // (NOTE: On best1402 command 0x9F must be sent via extension command mode, not normal
                // command address mode.)
                norflaship_blksize(id, 1);
                norflaship_cmd_addr(id, GD25Q32C_CMD_ID, 0);
                norflaship_clear_rxfifo(id);
            }
#endif
            if (div_std_read[id]) {
                norflaship_div(id, div_std_read[id]);
            }
        }
        norflaship_blksize(id, 2);
        norflaship_cmd_addr(id, cmd, 0xFFFFFE);
        // Now Continuous Read Mode has been disabled
        if (cmd == GD25Q32C_CMD_STANDARD_READ) {
            if (div_others[id]) {
                norflaship_div(id, div_others[id]);
            }
        }
        norflaship_clear_rxfifo(id);
    }

    norflaship_busy_wait(id);

    return 0;
}

static int norflash_set_quad(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    if (on) {
        flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_QE, 1);
    } else {
        flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_QE, 0);
    }
    return 0;
}

static int norflash_set_quad_io_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    norflash_set_quad(id, on);
    if (on) {
        norflaship_quad_mode(id, 1);
    } else {
        norflaship_quad_mode(id, 0);
    }
    norflaship_busy_wait(id);
    return 0;
}

static int norflash_set_quad_output_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    norflash_set_quad(id, on);
    if (on) {
        norflaship_rdcmd(id, GD25Q32C_CMD_FAST_QUAD_OUTPUT_READ);
    } else {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t norflash_set_dual_io_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_dual_mode(id, 1);
    } else {
        norflaship_dual_mode(id, 0);
    }
    norflaship_busy_wait(id);

    return 0;
}

static uint8_t norflash_set_dual_output_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_rdcmd(id, GD25Q32C_CMD_FAST_DUAL_OUTPUT_READ);
    } else {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t norflash_set_fast_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    if (on) {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_FAST_READ);
    } else {
        norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    }
    norflaship_busy_wait(id);
    return 0;
}

static uint8_t norflash_set_stand_mode(enum HAL_FLASH_ID_T id)
{
    norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
    norflaship_busy_wait(id);
    return 0;
}

#if defined(FLASH_PAGE_MPM_ENABLE)
static void norflash_set_mpm_mode(enum HAL_FLASH_ID_T id)
{
    uint8_t val;
    uint32_t mpm_mode;

    if (flash_list[flash_idx[id]]->page_size == 0x200){
        mpm_mode = PUYA_CR_MPM_MODE_1;
    }
    else if (flash_list[flash_idx[id]]->page_size == 0x400){
        mpm_mode = PUYA_CR_MPM_MODE_2;
    }
    else {
        mpm_mode = PUYA_CR_MPM_MODE_0;
    }

    norflash_read_reg(id, PUYA_FLASH_CMD_READ_CFGREG, &val, 1);

    val = (val & (~PUYA_CR_MPM_BIT_MASK)) | (mpm_mode << PUYA_CR_MPM_BIT_SHIFT);
    norflash_write_reg(id, PUYA_FLASH_CMD_WRITE_CFGREG, &val, 1);
}
#endif

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
static int norflash_set_dtr_quad_mode(enum HAL_FLASH_ID_T id, uint8_t on)
{
    uint8_t cmd;
    if (on) {
        norflaship_dummy_dtr4rd(flash_list[flash_idx[id]]->dtr_quad_cfg.s.dummy_cycles);
        cmd = GD25Q32C_CMD_FAST_DTR_QUAD_IO_READ;
    } else {
        cmd = GD25Q32C_CMD_FAST_QUAD_IO_READ;
    }
    norflaship_qrdcmd(id, cmd);
    norflaship_dtr_mode(id, on);
    norflaship_busy_wait(id);
    return 0;
}
#endif

uint32_t norflash_get_supported_mode(enum HAL_FLASH_ID_T id)
{
     return flash_list[flash_idx[id]]->mode;
}

uint32_t norflash_get_current_mode(enum HAL_FLASH_ID_T id)
{
     return norflash_op_mode[id];
}

union DRV_NORFLASH_SEC_REG_CFG_T norflash_get_security_register_config(enum HAL_FLASH_ID_T id)
{
    return flash_list[flash_idx[id]]->sec_reg_cfg;
}

uint32_t norflash_get_block_protect_mask(enum HAL_FLASH_ID_T id)
{
    return flash_list[flash_idx[id]]->block_protect_mask;
}

void norflash_reset(enum HAL_FLASH_ID_T id)
{
    norflaship_clear_fifos(id);

    // Release from deep power-down
    norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    // Wait 20us for flash to finish
    norflash_delay(40);

    // Quit from quad/dual continuous read mode
    norflaship_quad_mode(id, 0);
    norflaship_dual_mode(id, 0);
    norflaship_continuous_read_off(id);
    norflaship_busy_wait(id);
#if (CHIP_FLASH_CTRL_VER >= 3) && !defined(CHIP_BEST1402)
    // Command 0x9F will make M4=1 (mode bit 4) at DTR quad and dual continuous read modes,
    // which will disable it
    // (NOTE: On best1402 command 0x9F must be sent via extension command mode, not normal
    // command address mode.)
    norflaship_clear_rxfifo(id);
    norflaship_blksize(id, 1);
    norflaship_cmd_addr(id, GD25Q32C_CMD_ID, 0);
    norflaship_busy_wait(id);
#endif
    // Command 0x03 and address 0xFFFFFE will make M4=1 (mode bit 4)
    // at both quad and dual continuous read modes, which will disable it.
    norflaship_clear_rxfifo(id);
    norflaship_blksize(id, 2);
    norflaship_cmd_addr(id, GD25Q32C_CMD_STANDARD_READ, 0xFFFFFE);
    norflaship_busy_wait(id);

    norflaship_clear_rxfifo(id);

    // Software reset
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_ENABLE_RESET, 0);
    norflaship_ext_tx_cmd(id, GD25Q32C_CMD_RESET, 0);
    // Reset recovery time: 20~30 us
    norflash_delay(50);

    norflaship_cmd_done(id);

    // Reset cfg
    flash_idx[id] = 0;
    div_read[id] = 0;
    div_std_read[id] = 0;
    div_others[id] = 0;
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    div_dtr_read[id] = 0;
#endif
    norflash_op_mode[id] = 0;
    falling_edge_adj[id] = false;
}

int norflash_get_size(enum HAL_FLASH_ID_T id, uint32_t *total_size, uint32_t *block_size, uint32_t *sector_size, uint32_t *page_size)
{
    if (total_size) {
        *total_size  = flash_list[flash_idx[id]]->total_size;
    }
    if (block_size) {
        *block_size  = flash_list[flash_idx[id]]->block_size;
    }
    if (sector_size) {
        *sector_size = flash_list[flash_idx[id]]->sector_size;
    }
    if (page_size) {
        *page_size   = flash_list[flash_idx[id]]->page_size;
    }

    return 0;
}

static void norflash_set_cfg_div(enum HAL_FLASH_ID_T id)
{
    if (div_others[id]) {
        norflaship_div(id, div_others[id]);
    }
}

static void norflash_set_read_div(enum HAL_FLASH_ID_T id)
{
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_STAND_SPI) {
        if (div_std_read[id]) {
            norflaship_div(id, div_std_read[id]);
        }
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DTR) {
        if (div_dtr_read[id]) {
            norflaship_div(id, div_dtr_read[id]);
        }
#endif
    } else {
        if (div_read[id]) {
            norflaship_div(id, div_read[id]);
        }
    }
}

int norflash_set_mode(enum HAL_FLASH_ID_T id, uint32_t op)
{
    uint32_t read_mode = 0;
    uint32_t ext_mode = 0;
    uint32_t program_mode = 0;
    uint32_t self_mode;
    uint32_t mode;

    self_mode = norflash_get_supported_mode(id);
    mode = (self_mode & op);

    if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
        read_mode = HAL_NORFLASH_OP_MODE_QUAD_IO;
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
#if (CHIP_FLASH_CTRL_VER == 3)
        if ((mode & HAL_NORFLASH_OP_MODE_DTR) && div_dtr_read[id] != 2) {
            // For flash controller V3 (1402/2001/2300a), only div=2 can support DTR
            mode &= ~HAL_NORFLASH_OP_MODE_DTR;
        }
#endif
        if (mode & HAL_NORFLASH_OP_MODE_DTR) {
            ext_mode |= HAL_NORFLASH_OP_MODE_DTR;
#if defined(CHIP_BEST1402) || defined(CHIP_BEST2001)
            // ROM cannot recover from DTR continuous read modes
            mode &= ~HAL_NORFLASH_OP_MODE_CONTINUOUS_READ;
#endif
        }
#endif
    } else if (mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
        read_mode = HAL_NORFLASH_OP_MODE_QUAD_OUTPUT;
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
        read_mode = HAL_NORFLASH_OP_MODE_DUAL_IO;
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
        read_mode = HAL_NORFLASH_OP_MODE_DUAL_OUTPUT;
    } else if (mode & HAL_NORFLASH_OP_MODE_FAST_SPI) {
        read_mode = HAL_NORFLASH_OP_MODE_FAST_SPI;
    } else if(mode & HAL_NORFLASH_OP_MODE_STAND_SPI) {
        read_mode = HAL_NORFLASH_OP_MODE_STAND_SPI;
    } else {
        // Op error
        return  1;
    }

    if (mode & HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
        program_mode = HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM;
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM) {
        program_mode = HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM;
    } else if (mode & HAL_NORFLASH_OP_MODE_PAGE_PROGRAM) {
        program_mode = HAL_NORFLASH_OP_MODE_PAGE_PROGRAM;
    } else {
        // Op error
        return 1;
    }


#ifdef FLASH_HPM
    if (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
        ext_mode |= HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE;
    }
#endif

    if (mode & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DUAL_IO)) {
        if (mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
            ext_mode |= HAL_NORFLASH_OP_MODE_CONTINUOUS_READ;
        }
    }

#ifdef FLASH_BURST_WRAP
    if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
        if (mode & HAL_NORFLASH_OP_MODE_READ_WRAP) {
            ext_mode |= HAL_NORFLASH_OP_MODE_READ_WRAP;
        }
    }
#endif

    mode = (read_mode | ext_mode | program_mode);

    if (norflash_op_mode[id] != mode) {
        // Continuous read off if flash supported
        if ((self_mode & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DUAL_IO)) &&
                (self_mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ)) {
            norflash_set_continuous_read(id, 0);
        }

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
        norflash_set_dtr_quad_mode(id, 0);
#endif

        norflaship_quad_mode(id, 0);

        norflaship_dual_mode(id, 0);

#ifdef FLASH_HPM
        if (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
            // High performance mode on
            norflash_set_hpm(id, 1);
        }
#endif

        if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
            // Quad io mode
            norflash_set_quad_io_mode(id, 1);
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
            if (mode & HAL_NORFLASH_OP_MODE_DTR) {
                norflash_set_dtr_quad_mode(id, 1);
            }
#endif
        } else if (mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
            // Quad output mode
            norflash_set_quad_output_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
            // Dual io mode
            norflash_set_dual_io_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
            // Dual output mode
            norflash_set_dual_output_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_FAST_SPI) {
            // Fast mode
            norflash_set_fast_mode(id, 1);
        } else if (mode & HAL_NORFLASH_OP_MODE_STAND_SPI) {
            // Standard spi mode
            norflash_set_stand_mode(id);
        }

#ifdef FLASH_BURST_WRAP
        if (mode & HAL_NORFLASH_OP_MODE_READ_WRAP) {
            norflash_set_burst_wrap(id, 32);
            hal_cache_wrap_enable(HAL_CACHE_ID_I_CACHE);
            hal_cache_wrap_enable(HAL_CACHE_ID_D_CACHE);
        } else
#endif
        {
            hal_cache_wrap_disable(HAL_CACHE_ID_I_CACHE);
            hal_cache_wrap_disable(HAL_CACHE_ID_D_CACHE);
        }

#if defined(FLASH_PAGE_MPM_ENABLE)
    if (flash_list[flash_idx[id]] == &p25q128l_cfg)
        norflash_set_mpm_mode(id);
#endif
#ifdef FLASH_HPM
        if ((self_mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) &&
                (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) == 0) {
            // High performance mode off
            norflash_set_hpm(id, 0);
        }
#endif

        if (mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
            // Continuous read on
            norflash_set_continuous_read(id, 1);
        }

        norflaship_cmd_done(id);

        // Update current mode at last
        norflash_op_mode[id] = mode;
    }

    norflash_set_read_div(id);

    return 0;
}

int norflash_pre_operation(enum HAL_FLASH_ID_T id)
{
#ifdef FLASH_MEM_READ_BUS_LOCK
    mrb_status = norflaship_mem_read_bus_lock(id);
#endif

    norflash_set_cfg_div(id);

    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
        norflash_set_continuous_read(id, 0);
    }
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    if ((norflash_op_mode[id] & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) ==
            (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) {
        norflash_set_dtr_quad_mode(id, 0);
    }
#endif

    return 0;
}

int norflash_post_operation(enum HAL_FLASH_ID_T id)
{
#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    if ((norflash_op_mode[id] & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) ==
            (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DTR)) {
        norflash_set_dtr_quad_mode(id, 1);
    }
#endif
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
        norflash_set_continuous_read(id, 1);
    }

    norflaship_cmd_done(id);

    norflash_set_read_div(id);

#ifdef FLASH_MEM_READ_BUS_LOCK
    norflaship_mem_read_bus_unlock(id, mrb_status);
#endif

    return 0;
}

int norflash_read_reg(enum HAL_FLASH_ID_T id, uint8_t cmd, uint8_t *val, uint32_t len)
{
    int i;

    norflaship_clear_fifos(id);
#ifdef TRY_EMBEDDED_CMD
    if ((cmd == GD25Q32C_CMD_READ_STATUS_S0_S7 || cmd == GD25Q32C_CMD_READ_STATUS_S8_S15) && len == 1) {
        norflaship_cmd_addr(id, cmd, 0);
    } else {
        // (NOTE: On best1402 command 0x9F must be sent via extension command mode, not normal
        // command address mode.)
#ifndef CHIP_BEST1402
        if (cmd == GD25Q32C_CMD_ID) {
            norflaship_blksize(id, len);
            norflaship_cmd_addr(id, cmd, 0);
        } else
#endif
        {
            norflaship_ext_rx_cmd(id, cmd, 0, len);
        }
    }
#else
    norflaship_ext_rx_cmd(id, cmd, 0, len);
#endif
    norflaship_rxfifo_count_wait(id, len);
    for (i = 0; i < len; i++) {
        val[i] = norflaship_read_rxfifo(id);
    }

    return 0;
}

int norflash_read_reg_ex(enum HAL_FLASH_ID_T id, uint8_t cmd, uint8_t *param, uint32_t param_len, uint8_t *val, uint32_t len)
{
    int i;

    norflaship_clear_fifos(id);
    if (param && param_len > 0) {
        norflaship_write_txfifo(id, param, param_len);
    } else {
        param_len = 0;
    }
    norflaship_ext_rx_cmd(id, cmd, param_len, len);
    for (i = 0; i < len; i++) {
        norflaship_rxfifo_empty_wait(id);
        val[i] = norflaship_read_rxfifo(id);
    }

    return 0;
}

int norflash_write_reg(enum HAL_FLASH_ID_T id, uint8_t cmd, const uint8_t *val, uint32_t len)
{
    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, 0);
    norflash_status_WIP_1_wait(id, 0);
    norflash_status_WEL_0_wait(id);

    norflaship_clear_txfifo(id);
    norflaship_write_txfifo(id, val, len);

#ifdef TRY_EMBEDDED_CMD
    if (cmd == GD25Q32C_CMD_WRITE_STATUS_S0_S7) {
        norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_STATUS_S0_S7, 0);
    } else {
        norflaship_ext_tx_cmd(id, cmd, len);
    }
#else
    norflaship_ext_tx_cmd(id, cmd, len);
#endif

    norflash_status_WIP_1_wait(id, 0);

    return 0;
}

static int norflash_get_id_internal(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    if (len > NORFLASH_ID_LEN) {
        len = NORFLASH_ID_LEN;
    }
    norflash_read_reg(id, GD25Q32C_CMD_ID, value, len);

    return 0;
}

int norflash_get_id(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    int ret;

    norflash_pre_operation(id);

    ret = norflash_get_id_internal(id, value, len);

    norflash_post_operation(id);

    return ret;
}

int norflash_get_unique_id(enum HAL_FLASH_ID_T id, uint8_t *value, uint32_t len)
{
    uint32_t param;
    uint8_t cmd;

    norflash_pre_operation(id);

    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_XTS_ID_PREFIX) {
        if (len > XTS_UNIQUE_ID_LEN) {
            len = XTS_UNIQUE_ID_LEN;
        }
        param = XTS_UNIQUE_ID_PARAM;
        cmd = XTS_UNIQUE_ID_CMD;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_XMC_ID_PREFIX) {
        if (len > XMC_UNIQUE_ID_LEN) {
            len = XMC_UNIQUE_ID_LEN;
        }
        param = 0;
        cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_ZBIT_ID_PREFIX) {
        if (len > ZBIT_UNIQUE_ID_LEN) {
            len = ZBIT_UNIQUE_ID_LEN;
        }
        param = 0;
        cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_WB_ID_PREFIX) {
        if (len > WB_UNIQUE_ID_LEN) {
            len = WB_UNIQUE_ID_LEN;
        }
        param = 0;
        cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_GD_ID_PREFIX) {
        if (len > GD_UNIQUE_ID_LEN) {
            len = GD_UNIQUE_ID_LEN;
        }
        param = 0;
        cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
        if (len > PUYA_UNIQUE_ID_LEN) {
            len = PUYA_UNIQUE_ID_LEN;
        }
        param = 0;
        cmd = GD25Q32C_CMD_UNIQUE_ID;
    } else {
        if (len > NORFLASH_UNIQUE_ID_LEN) {
            len = NORFLASH_UNIQUE_ID_LEN;
        }
        param = 0;
        cmd = GD25Q32C_CMD_UNIQUE_ID;
    }
    norflash_read_reg_ex(id, cmd, (uint8_t *)&param, sizeof(param), value, len);

    norflash_post_operation(id);

    return 0;
}

static void norflash_get_samdly_list(uint32_t div, const uint8_t **samdly_list_p, uint32_t *size_p)
{
    const uint8_t *samdly_list = NULL;
    uint32_t size = 0;

#if (CHIP_FLASH_CTRL_VER <= 1)
    if (div >= 2) {
        samdly_list = samdly_list_divn;
        size = ARRAY_SIZE(samdly_list_divn);
    }
#else
    if (div >= 1) {
        if (div == 1) {
            samdly_list = samdly_list_div1;
            size = ARRAY_SIZE(samdly_list_div1);
        } else {
            samdly_list = samdly_list_divn;
            size = ARRAY_SIZE(samdly_list_divn);
        }
    }
#endif

    if (samdly_list_p) {
        *samdly_list_p = samdly_list;
    }
    if (size_p) {
        *size_p = size;
    }
}

void norflash_set_sample_delay_index(enum HAL_FLASH_ID_T id, uint32_t index)
{
    const uint8_t *samdly_list;
    uint32_t size;
    uint32_t div;

    sample_delay_index[id] = index;

    div = norflaship_get_div(id);

    norflash_get_samdly_list(div, &samdly_list, &size);

    if (index < size) {
#if (CHIP_FLASH_CTRL_VER <= 1)
        norflaship_pos_neg(id, samdly_list[index] & SAM_EDGE_FALLING);
        norflaship_neg_phase(id, samdly_list[index] & SAM_NEG_PHASE);
        norflaship_samdly(id, samdly_list[index] & SAMDLY_MASK);
#else
        norflaship_samdly(id, samdly_list[index]);
#endif
    }
}

uint32_t norflash_get_sample_delay_index(enum HAL_FLASH_ID_T id)
{
    return sample_delay_index[id];
}

static bool norflash_calib_flash_id_valid(enum HAL_FLASH_ID_T id)
{
    uint8_t dev_id[HAL_NORFLASH_DEVICE_ID_LEN];
    const uint8_t *cmp_id;

    norflash_get_id_internal(id, dev_id, sizeof(dev_id));
    cmp_id = flash_list[flash_idx[id]]->id;

    if (dev_id[0] == cmp_id[0] && dev_id[1] == cmp_id[1] && dev_id[2] == cmp_id[2]) {
        return true;
    }
    return false;
}
extern uint32_t __flash_start[];
static bool norflash_calib_magic_word_valid(enum HAL_FLASH_ID_T id)
{
    uint32_t magic;

#if (CHIP_FLASH_CTRL_VER <= 1)
    norflash_read(id, FLASH_NC_BASE, NULL, 1);
#endif
    norflaship_clear_rxfifo(id);
#if (FLASH_NC_BASE == FLASH_BASE)
    hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, FLASH_BASE, sizeof(magic));
    magic = *(volatile uint32_t *)FLASH_BASE;
#else
    magic = *(volatile uint32_t *)FLASH_C_TO_NC((uint32_t)__flash_start);
#endif

    if (magic == BOOT_MAGIC_NUMBER) {
        return true;
    }
    return false;
}

int norflash_sample_delay_calib(enum HAL_FLASH_ID_T id, enum DRV_NORFLASH_CALIB_T type)
{
    int i;
    uint32_t matched_cnt = 0;
    uint32_t matched_idx = 0;
    uint32_t div;
    uint32_t size;
    bool valid;

    if (type >= DRV_NORFLASH_CALIB_QTY) {
        return 1;
    }
#if defined(ROM_BUILD) || defined(PROGRAMMER)
    if (type != DRV_NORFLASH_CALIB_FLASH_ID) {
        return 0;
    }
#endif

    div = norflaship_get_div(id);

    if (div == 0) {
        return -1;
#if (CHIP_FLASH_CTRL_VER <= 1)
    } else if (div == 1) {
        return -2;
#endif
    }

    norflash_get_samdly_list(div, NULL, &size);

    for (i = 0; i < size; i++) {
        norflaship_busy_wait(id);

        norflash_set_sample_delay_index(id, i);

        if (type == DRV_NORFLASH_CALIB_FLASH_ID) {
            valid = norflash_calib_flash_id_valid(id);
        } else {
            valid = norflash_calib_magic_word_valid(id);
        }

        if (valid) {
            if (matched_cnt == 0) {
                matched_idx = i;
            }
            matched_cnt++;
        } else if (matched_cnt) {
            break;
        }
    }

#ifdef FLASH_CALIB_DEBUG
    calib_matched_idx[type] = matched_idx;
    calib_matched_cnt[type] = matched_cnt;
#endif

    if (matched_cnt) {
#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
        if (matched_cnt == 2) {
            uint32_t falling_edge_idx;

            if (0) {
#if (CHIP_FLASH_CTRL_VER >= 2)
            } else if (div == 1) {
                falling_edge_idx = DIV1_SAMP_DELAY_FALLING_EDGE_IDX;
#endif
            } else if (div == 2) {
                falling_edge_idx = DIV2_SAMP_DELAY_FALLING_EDGE_IDX;
                if (falling_edge_adj[id]) {
                    falling_edge_idx++;
                }
            } else {
                falling_edge_idx = DIVN_SAMP_DELAY_FALLING_EDGE_IDX;
            }
            if (matched_idx <= falling_edge_idx &&
                    falling_edge_idx < matched_idx + matched_cnt) {
                matched_idx = falling_edge_idx;
                matched_cnt = 1;
            }
        }
#endif
        matched_idx += matched_cnt / 2;
        norflash_set_sample_delay_index(id, matched_idx);

#ifdef FLASH_CALIB_DEBUG
        calib_final_idx[type] = matched_idx;
#endif

        return 0;
    }

#ifdef FLASH_CALIB_DEBUG
    calib_final_idx[type] = -1;
#endif

    return 1;
}

void norflash_show_calib_result(enum HAL_FLASH_ID_T id)
{
#ifdef FLASH_CALIB_DEBUG
    union DRV_NORFLASH_SPEED_RATIO_T ratio;
    uint32_t div;
    uint32_t size;
    const uint8_t *list;
    int i;
#ifdef PROGRAMMER
    unsigned char buf[16];
#endif

    TR_INFO(0, "FLASH_CALIB_RESULT:");
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<FREQ>\nsource_clk=%u speed=%u flash_max=%u",
        norflash_source_clk, norflash_speed, flash_list[flash_idx[id]]->max_speed);
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &norflash_source_clk, sizeof(norflash_source_clk));
    memcpy(buf + 4, &norflash_speed, sizeof(norflash_speed));
    memcpy(buf + 8, &flash_list[flash_idx[id]]->max_speed, sizeof(flash_list[flash_idx[id]]->max_speed));
    send_debug_event(buf, 12);
#endif

    ratio = flash_list[flash_idx[id]]->speed_ratio;
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<RATIO>\nstd_read=%u/8 others=%u/8 dtr_read=%u/8",
        (ratio.s.std_read + 1), (ratio.s.others + 1), (flash_list[flash_idx[id]]->dtr_quad_cfg.s.speed_ratio + 1));
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    buf[0] = (ratio.s.std_read + 1);
    buf[1] = (ratio.s.others + 1);
    buf[2] = (flash_list[flash_idx[id]]->dtr_quad_cfg.s.speed_ratio + 1);
    send_debug_event(buf, 3);
#endif

    div = norflaship_get_div(id);
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<DIV>\ndiv=%u", div);
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "div_read[id]=%u div_std_read[id]=%u div_others[id]=%u",
        div_read[id], div_std_read[id], div_others[id]);
#ifdef PROGRAMMER
    memset(buf, 0, sizeof(buf));
    buf[0] = div;
    buf[1] = div_read[id];
    buf[2] = div_std_read[id];
    buf[3] = div_others[id];
    send_debug_event(buf, 4);
#endif

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "div_dtr_read[id]=%u", div_dtr_read[id]);
#ifdef PROGRAMMER
    buf[0] = div_dtr_read[id];
    send_debug_event(buf, 1);
#endif
#endif

    norflash_get_samdly_list(div, &list, &size);
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<SAMDLY LIST>");
    if (list == NULL || size == 0) {
        TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "NONE");
    } else {
        DUMP8("%02X ", list, size);
#ifdef PROGRAMMER
        send_debug_event(list, size);
#endif
    }
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "<CALIB RESULT>");
    for (i = 0; i < DRV_NORFLASH_CALIB_QTY; i++) {
        TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "type=%d idx=%02u cnt=%02u final=%02u",
            i, calib_matched_idx[i], calib_matched_cnt[i], calib_final_idx[i]);
#ifdef PROGRAMMER
        memset(buf, 0, sizeof(buf));
        buf[0] = i;
        buf[1] = calib_matched_idx[i];
        buf[2] = calib_matched_cnt[i];
        buf[3] = calib_final_idx[i];
        send_debug_event(buf, 4);
#endif
    }
    TR_INFO(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "\t");
#endif
}

int norflash_init_sample_delay_by_div(enum HAL_FLASH_ID_T id, uint32_t div)
{
    if (div == 0) {
        return -1;
    } if (div == 1) {
#if (CHIP_FLASH_CTRL_VER <= 1)
        return -2;
#else
        norflaship_samdly(id, 1);
#endif
    } else if (div == 2 && !falling_edge_adj[id]) {
        // Set sample delay to clock falling edge
#if (CHIP_FLASH_CTRL_VER <= 1)
        norflaship_pos_neg(id, 1);
        norflaship_neg_phase(id, 1);
        norflaship_samdly(id, 2);
#else
        norflaship_samdly(id, 3);
#endif
    } else {
        // Set sample delay to nearest to but not later than clock falling edge
#if (CHIP_FLASH_CTRL_VER <= 1)
        norflaship_pos_neg(id, 0);
        norflaship_neg_phase(id, 0);
        norflaship_samdly(id, 2);
#else
        norflaship_samdly(id, 4);
#endif
    }

    return 0;
}

int norflash_init_div(enum HAL_FLASH_ID_T id, const struct HAL_NORFLASH_CONFIG_T *cfg)
{
    uint32_t max_speed;
    uint32_t read_speed;
    uint32_t std_read_speed;
    uint32_t others_speed;
    union DRV_NORFLASH_SPEED_RATIO_T ratio;
    uint32_t div;

#ifdef FLASH_CALIB_DEBUG
    norflash_source_clk = cfg->source_clk;
    norflash_speed = cfg->speed;
#endif

    max_speed = flash_list[flash_idx[id]]->max_speed;
    if (max_speed == 0) {
        max_speed = NORFLASH_DEFAULT_MAX_SPEED;
    }

    ratio = flash_list[flash_idx[id]]->speed_ratio;

    read_speed = max_speed;
    if (read_speed > cfg->speed) {
        read_speed = cfg->speed;
    }
    if (read_speed > cfg->source_clk) {
        read_speed = cfg->source_clk;
    }
    std_read_speed = max_speed * (1 + ratio.s.std_read) / NORFLASH_SPEED_RATIO_DENOMINATOR;
    if (std_read_speed > read_speed) {
        std_read_speed = read_speed;
    }
    others_speed = max_speed * (1 + ratio.s.others) / NORFLASH_SPEED_RATIO_DENOMINATOR;
    if (others_speed > read_speed) {
        others_speed = read_speed;
    }

    div = (cfg->source_clk + read_speed - 1) / read_speed;
    div_read[id] = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;
    div = (cfg->source_clk + std_read_speed - 1) / std_read_speed;
    div_std_read[id] = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;
    div = (cfg->source_clk + others_speed - 1) / others_speed;
    div_others[id] = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;

#if (CHIP_FLASH_CTRL_VER >= 3) && defined(FLASH_DTR)
    uint32_t dtr_speed;

    dtr_speed = max_speed * (1 + flash_list[flash_idx[id]]->dtr_quad_cfg.s.speed_ratio) / NORFLASH_SPEED_RATIO_DENOMINATOR;
    if (dtr_speed > read_speed) {
        dtr_speed = read_speed;
    }
    div = (cfg->source_clk + dtr_speed - 1) / dtr_speed;
    div_dtr_read[id] = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;
#endif

    if (div_read[id] == 2 && read_speed >= FALLING_EDGE_SAMPLE_ADJ_FREQ) {
        falling_edge_adj[id] = true;
    } else {
        falling_edge_adj[id] = false;
    }

    if (div_read[id] && div_std_read[id] && div_others[id]) {
#if (CHIP_FLASH_CTRL_VER <= 1)
        if (div_read[id] == 1) {
            return -1;
        }
#else
        if (div_read[id] == 1 && read_speed > NORFLASH_DIV1_MAX_SPEED) {
            return -1;
        }
#endif
        // Init sample delay according to div_read[id]
        norflash_init_sample_delay_by_div(id, div_read[id]);
        // Still in command mode
        norflaship_div(id, div_others[id]);
        return 0;
    }

    return 1;
}

int norflash_match_chip(enum HAL_FLASH_ID_T id, const uint8_t *dev_id, uint32_t len)
{
    const uint8_t *cmp_id;

    if (len == NORFLASH_ID_LEN) {
        for (flash_idx[id] = 0; flash_idx[id] < ARRAY_SIZE(flash_list); flash_idx[id]++) {
            cmp_id = flash_list[flash_idx[id]]->id;
            if (dev_id[0] == cmp_id[0] && dev_id[1] == cmp_id[1] && dev_id[2] == cmp_id[2]) {
                return true;
            }
        }
    }

    return false;
}

void norflash_get_flash_list(const struct NORFLASH_CFG_T **list, uint32_t *len)
{
    *list = (struct NORFLASH_CFG_T *)&flash_list[0];
    *len = ARRAY_SIZE(flash_list);
}

#if defined(CONFIG_SMP)
extern void nuttx_smp_end_protection();
extern void nuttx_smp_start_protection();
#endif

#ifdef PUYA_FLASH_ERASE_PAGE_ENABLE
static void norflaship_ext_cmd_addr(enum HAL_FLASH_ID_T id, uint8_t cmd, uint32_t addr)
{
    uint8_t buff[3];

    buff[2] = (uint8_t)(addr & 0xff);
    buff[1] = (uint8_t)((addr >> 8) & 0xff);
    buff[0] = (uint8_t)((addr >> 16) & 0xff);

    norflaship_clear_txfifo(id);
    norflaship_write_txfifo(id, buff, 3);
    norflaship_ext_tx_cmd(id, cmd, 3);
}
#endif

enum HAL_NORFLASH_RET_T norflash_erase(enum HAL_FLASH_ID_T id, uint32_t start_address, enum DRV_NORFLASH_ERASE_T type, int suspend)
{
    enum HAL_NORFLASH_RET_T ret;

    if (flash_list[flash_idx[id]]->mode & HAL_NORFLASH_OP_MODE_ERASE_IN_STD) {
        if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
            norflash_set_quad_io_mode(id, 0);
        } else if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
            norflash_set_quad_output_mode(id, 0);
        }
    }

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, start_address);
    // Need 1us. Or norflash_status_WEL_0_wait(id), which needs 6us.
    switch(type) {
#ifdef PUYA_FLASH_ERASE_PAGE_ENABLE
        case DRV_NORFLASH_ERASE_PAGE:
            norflaship_ext_cmd_addr(id, PUYA_FLASH_CMD_PAGE_ERASE, start_address);
            break;
#endif
        case DRV_NORFLASH_ERASE_SECTOR:
            norflaship_cmd_addr(id, GD25Q32C_CMD_SECTOR_ERASE, start_address);
            break;
        case DRV_NORFLASH_ERASE_BLOCK:
            norflaship_cmd_addr(id, GD25Q32C_CMD_BLOCK_ERASE, start_address);
            break;
        case DRV_NORFLASH_ERASE_CHIP:
            norflaship_cmd_addr(id, GD25Q32C_CMD_CHIP_ERASE, start_address);
            break;
    }

    norflaship_busy_wait(id);

#ifdef FLASH_SUSPEND
    // PUYA flash requires the first delay of erase >= 400us
    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
        norflash_delay(400);
    }
#endif

    ret = norflash_status_WIP_1_wait(id, suspend);

    if (flash_list[flash_idx[id]]->mode & HAL_NORFLASH_OP_MODE_ERASE_IN_STD) {
        if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
            norflash_set_quad_io_mode(id, 1);
        } else if(norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
            norflash_set_quad_output_mode(id, 1);
        }
    }

    norflaship_cmd_done(id);

    return ret;
}

enum HAL_NORFLASH_RET_T norflash_write(enum HAL_FLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len, int suspend)
{
    enum HAL_NORFLASH_RET_T ret;
    uint32_t POSSIBLY_UNUSED remains;

    if (len > flash_list[flash_idx[id]]->page_size) {
        return HAL_NORFLASH_ERR;
    }

    norflaship_clear_txfifo(id);

    remains = norflaship_write_txfifo(id, buffer, len);

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, start_address);
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
        norflaship_cmd_addr(id, GD25Q32C_CMD_QUAD_PAGE_PROGRAM, start_address);
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM) {
        norflaship_cmd_addr(id, GD25Q32C_CMD_DUAL_PAGE_PROGRAM, start_address);
    } else {
        norflaship_cmd_addr(id, GD25Q32C_CMD_PAGE_PROGRAM, start_address);
    }

#if (CHIP_FLASH_CTRL_VER >= 2)
    while (remains > 0) {
        buffer += len - remains;
        len = remains;
        remains = norflaship_write_txfifo_cont(id, buffer, len);
    }
#endif

    norflaship_busy_wait(id);

#ifdef FLASH_SUSPEND
    // PUYA flash requires the first delay of byte program >= 450us
    if (flash_list[flash_idx[id]]->id[0] == NORFLASH_PUYA_ID_PREFIX && len < GD25Q32C_PAGE_SIZE) {
        norflash_delay(450);
    }
#endif

    ret = norflash_status_WIP_1_wait(id, suspend);

    norflaship_cmd_done(id);

    return ret;
}

#ifdef FLASH_SUSPEND
enum HAL_NORFLASH_RET_T norflash_erase_resume(enum HAL_FLASH_ID_T id, int suspend)
{
    // TODO: Need to check SUS1 bit in status reg?

    enum HAL_NORFLASH_RET_T ret;
#if defined(CONFIG_SMP)
    nuttx_smp_start_protection(id);
#endif
    norflash_pre_operation(id);

    norflash_resume(id);

    ret = norflash_status_WIP_1_wait(id, suspend);

    norflash_post_operation(id);
#if defined(CONFIG_SMP)
    nuttx_smp_end_protection(id);
#endif
    return ret;
}

enum HAL_NORFLASH_RET_T norflash_write_resume(enum HAL_FLASH_ID_T id, int suspend)
{
    // TODO: Need to check SUS2 bit in status reg?

    return norflash_erase_resume(id, suspend);
}
#endif

int norflash_read(enum HAL_FLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint32_t index = 0;
    uint8_t val;

    if (len > NORFLASHIP_RXFIFO_SIZE) {
        return 1;
    }

    norflaship_clear_rxfifo(id);

    norflaship_busy_wait(id);

    norflaship_blksize(id, len);

    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_IO) {
        /* Quad , only fast */
        norflaship_cmd_addr(id, GD25Q32C_CMD_FAST_QUAD_IO_READ, start_address);
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
        /* Dual, only fast */
        norflaship_cmd_addr(id, GD25Q32C_CMD_FAST_QUAD_OUTPUT_READ, start_address);
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_IO) {
        /* Dual, only fast */
        norflaship_cmd_addr(id, GD25Q32C_CMD_FAST_DUAL_IO_READ, start_address);
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
        /* Dual, only fast */
        norflaship_cmd_addr(id, GD25Q32C_CMD_FAST_DUAL_OUTPUT_READ, start_address);
    } else if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_FAST_SPI){
        /* fast */
        norflaship_cmd_addr(id, GD25Q32C_CMD_STANDARD_FAST_READ, start_address);
    } else {
        /* normal */
        norflaship_cmd_addr(id, GD25Q32C_CMD_STANDARD_READ, start_address);
    }

    while (1) {
        norflaship_rxfifo_empty_wait(id);

        val = norflaship_read_rxfifo(id);
        if (buffer) {
            buffer[index] = val;
        }

        ++index;
        if (index >= len) {
            break;
        }
    }

    norflaship_cmd_done(id);

    return 0;
}

void norflash_sleep(enum HAL_FLASH_ID_T id)
{
    norflash_pre_operation(id);
#ifdef FLASH_HPM
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
        norflash_set_hpm(id, 0);
    }
#endif
    norflaship_cmd_addr(id, GD25Q32C_CMD_DEEP_POWER_DOWN, 0);
}

void norflash_wakeup(enum HAL_FLASH_ID_T id)
{
    norflaship_cmd_addr(id, GD25Q32C_CMD_RELEASE_FROM_DP, 0);
    // Wait 20us for flash to finish
    norflash_delay(40);
#ifdef FLASH_HPM
    if (norflash_op_mode[id] & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
        norflash_set_hpm(id, 1);
    }
#endif
    norflash_post_operation(id);
}

int norflash_read_status(enum HAL_FLASH_ID_T id, uint32_t *p_status)
{
    uint8_t status_s0_s7;
    uint8_t status_s8_s15;

    if (p_status == NULL) {
        return -1;
    }

    status_s0_s7 = norflash_read_status_s0_s7(id);
    status_s8_s15 = norflash_read_status_s8_s15(id);

    *p_status = status_s0_s7 | (status_s8_s15 << 8);

    return 0;
}

int norflash_init_status(enum HAL_FLASH_ID_T id, uint32_t status)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_INIT, status);

    return 0;
}

int norflash_set_block_protection(enum HAL_FLASH_ID_T id, uint32_t bp)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_BP, bp);

    return 0;
}

#ifdef FLASH_SECURITY_REGISTER
int norflash_security_register_get_lock_status(enum HAL_FLASH_ID_T id, uint32_t id_map, uint32_t *p_locked)
{
    union DRV_NORFLASH_SEC_REG_CFG_T cfg;
    uint8_t status_s8_s15;

    if (id_map == 0) {
        return 1;
    }

    cfg = norflash_get_security_register_config(id);
    if (!cfg.s.enabled) {
        return 2;
    }

    status_s8_s15 = norflash_read_status_s8_s15(id);

    if (cfg.s.lb == SEC_REG_LB_S11_S13) {
        if (id_map & ~0x7) {
            return 3;
        }
        *p_locked = ((status_s8_s15 >> STATUS_S11_LB1_BIT_SHIFT) & id_map);
    } else if (cfg.s.lb == SEC_REG_LB_S10) {
        if (id_map & ~0x1) {
            return 4;
        }
        *p_locked = ((status_s8_s15 >> STATUS_S10_LB_BIT_SHIFT) & 0x1);
    } else {
        return 5;
    }

    return 0;
}

int norflash_security_register_lock(enum HAL_FLASH_ID_T id, uint32_t index)
{
    if (flash_list[flash_idx[id]]->write_status == NULL) {
        return -1;
    }

    flash_list[flash_idx[id]]->write_status(id, DRV_NORFLASH_W_STATUS_LB, index);

    return 0;
}

enum HAL_NORFLASH_RET_T norflash_security_register_erase(enum HAL_FLASH_ID_T id, uint32_t start_address)
{
    enum HAL_NORFLASH_RET_T ret;

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, start_address);
    // Need 1us. Or norflash_status_WEL_0_wait(id), which needs 6us.

    norflaship_cmd_addr(id, GD25Q32C_CMD_SECURITY_REGISTER_ERASE, start_address);

    ret = norflash_status_WIP_1_wait(id, 0);

    norflaship_cmd_done(id);

    return ret;
}

enum HAL_NORFLASH_RET_T norflash_security_register_write(enum HAL_FLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
    enum HAL_NORFLASH_RET_T ret;
    uint32_t remains;

    // Security register page size might be larger than normal page size
    // E.g., the size of P25Q32L and P25Q64L is 1024

    norflaship_clear_txfifo(id);

#if (CHIP_FLASH_CTRL_VER <= 1)
    uint32_t div = 0;

    if (len > NORFLASHIP_TXFIFO_SIZE) {
        div = norflaship_get_div(id);

        // Slow down to avoid tx fifo underflow (it takes about 10 cpu cycles to fill one byte)
        norflaship_div(id, 16);
    }

    remains = norflaship_v1_write_txfifo_safe(id, buffer, len);
#else
    remains = norflaship_write_txfifo(id, buffer, len);
#endif

    norflaship_cmd_addr(id, GD25Q32C_CMD_WRITE_ENABLE, start_address);

    norflaship_cmd_addr(id, GD25Q32C_CMD_SECURITY_REGISTER_PROGRAM, start_address);

#if (CHIP_FLASH_CTRL_VER <= 1)
    if (remains) {
        norflaship_v1_write_txfifo_all(id, buffer, len);
    }
#else
    while (remains > 0) {
        buffer += len - remains;
        len = remains;
        remains = norflaship_write_txfifo_cont(id, buffer, len);
    }
#endif

    norflaship_busy_wait(id);

    ret = norflash_status_WIP_1_wait(id, 0);

#if (CHIP_FLASH_CTRL_VER <= 1)
    if (div) {
        // Restore the old div
        norflaship_div(id, div);
    }
#endif

    norflaship_cmd_done(id);

    return ret;
}

int norflash_security_register_read(enum HAL_FLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint32_t index = 0;

    if (len > NORFLASHIP_RXFIFO_SIZE) {
        return 1;
    }

    norflaship_clear_rxfifo(id);

    norflaship_busy_wait(id);

    norflaship_blksize(id, len);

    norflaship_cmd_addr(id, GD25Q32C_CMD_SECURITY_REGISTER_READ, start_address);

    while (1) {
        norflaship_rxfifo_empty_wait(id);

        buffer[index] = norflaship_read_rxfifo(id);

        ++index;
        if (index >= len) {
            break;
        }
    }

    norflaship_cmd_done(id);

    return 0;
}

uint32_t norflash_security_register_enable_read(enum HAL_FLASH_ID_T id)
{
    uint32_t mode;
    int result;

    mode = norflash_op_mode[id];

    result = norflash_set_mode(id, HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_PAGE_PROGRAM);
    ASSERT(result == 0, "Failed to set sec reg read mode");

    norflaship_busy_wait(id);
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflash_read(id, FLASH_NC_BASE, NULL, 1);
#endif
    norflaship_clear_rxfifo(id);
    norflaship_busy_wait(id);
    norflaship_rdcmd(id, GD25Q32C_CMD_SECURITY_REGISTER_READ);
#if (CHIP_FLASH_CTRL_VER >= 3)
    // Set the dummy cycles for security register read command
    norflaship_dummy_others(id, 8);
#endif
    norflaship_busy_wait(id);

    return mode;
}

void norflash_security_register_disable_read(enum HAL_FLASH_ID_T id, uint32_t mode)
{
    int result;

    norflaship_busy_wait(id);
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflash_read(FLASH_NC_BASE, NULL, 1);
#endif
    norflaship_clear_rxfifo(id);
    norflaship_busy_wait(id);
    norflaship_rdcmd(id, GD25Q32C_CMD_STANDARD_READ);
#if (CHIP_FLASH_CTRL_VER >= 3)
    // Set the dummy cycles for security register read command
    norflaship_dummy_others(id, 0);
#endif
    norflaship_busy_wait(id);

    result = norflash_set_mode(id, mode);
    ASSERT(result == 0, "Failed to restore normal mode after sec reg read");
}

#endif


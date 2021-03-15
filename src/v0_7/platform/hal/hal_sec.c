/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifdef CHIP_HAS_TRUSTZONE
#include "plat_addr_map.h"

#include "hal_sec.h"
#include "reg_sec.h"
#include "reg_mpc_spy.h"

//these sec regs can only be accessed in security state
static struct HAL_SEC_T *const sec = (struct HAL_SEC_T *)TRUST_ZONE_BASE;

void hal_sec_set_gdma_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= (SEC_CFG_NONSEC_GDMA1 | SEC_CFG_NONSEC_GDMA2);
    else
        sec->REG_00C &= ~(SEC_CFG_NONSEC_GDMA1 | SEC_CFG_NONSEC_GDMA2);
}

void hal_sec_set_adma_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= (SEC_CFG_NONSEC_ADMA1 | SEC_CFG_NONSEC_ADMA2);
    else
        sec->REG_00C &= ~(SEC_CFG_NONSEC_ADMA1 | SEC_CFG_NONSEC_ADMA2);
}

void hal_sec_set_bcm_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= SEC_CFG_NONSEC_BCM;
    else
        sec->REG_00C &= ~SEC_CFG_NONSEC_BCM;
}

void hal_sec_set_cksum_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= SEC_CFG_NONSEC_CKSUM;
    else
        sec->REG_00C &= ~SEC_CFG_NONSEC_CKSUM;
}

void hal_sec_set_crc_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= SEC_CFG_NONSEC_CRC;
    else
        sec->REG_00C &= ~SEC_CFG_NONSEC_CRC;
}

void hal_sec_set_usb_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= SEC_CFG_NONSEC_USB;
    else
        sec->REG_00C &= ~SEC_CFG_NONSEC_USB;
}

void hal_sec_set_i2c_slv_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= SEC_CFG_NONSEC_I2C_SLV;
    else
        sec->REG_00C &= ~SEC_CFG_NONSEC_I2C_SLV;
}

void hal_sec_set_bt2mcu_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= SEC_CFG_NONSEC_BT2MCU;
    else
        sec->REG_00C &= ~SEC_CFG_NONSEC_BT2MCU;
}

void hal_sec_set_wf2mcu_nonsec(bool nonsec)
{
    if (nonsec)
        sec->REG_00C |= SEC_CFG_NONSEC_WF2MCU;
    else
        sec->REG_00C &= ~SEC_CFG_NONSEC_WF2MCU;
}

void hal_mpc_spy_nonsec_bypass(uint32_t addr, bool bypass)
{
    struct HAL_MPC_SPY_T *base = (struct HAL_MPC_SPY_T *)addr;
    if (bypass)
        base->REG_000 |= MPC_SPY_MPC_CFG_NONSEC_BYPASS;
    else
        base->REG_000 &= ~MPC_SPY_MPC_CFG_NONSEC_BYPASS;
}
#endif

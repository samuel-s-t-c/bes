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
#include "pmu.h"
#include "analog.h"
#include "bt_drv.h"
#include "bsp_dbg.h"

int bsp_dbg_pmu_reg_read(uint16_t reg_pmu, uint16_t *val_pmu)
{
    int ret = 0;
    ret = pmu_read(reg_pmu, val_pmu);

    return ret;
}

int bsp_dbg_pmu_reg_write(uint16_t reg_pmu, uint16_t val_pmu)
{
    int ret = 0;
    ret = pmu_write(reg_pmu, val_pmu);

    return ret;
}

int bsp_dbg_ana_reg_read(uint16_t reg_ana, uint16_t *val_ana)
{
    int ret = 0;
    ret = analog_read(reg_ana, val_ana);

    return ret;
}

int bsp_dbg_ana_reg_write(uint16_t reg_ana, uint16_t val_ana)
{
    int ret = 0;
    ret = analog_write(reg_ana, val_ana);

    return ret;
}

int bsp_dbg_rf_reg_read(uint16_t reg_rf, uint16_t *val_rf)
{
    int ret = 0;
    ret = btdrv_read_rf_reg(reg_rf, val_rf);

    return ret;
}

int bsp_dbg_rf_reg_write(uint16_t reg_rf, uint16_t val_rf)
{
    int ret = 0;
    ret = btdrv_write_rf_reg(reg_rf, val_rf);

    return ret;
}

int bsp_dbg_dig_reg_read(uint32_t reg_dig, uint32_t *val_dig)
{
    *val_dig = BSP_DBG_DIG_REG(reg_dig);

    return 0;
}

int bsp_dbg_dig_reg_write(uint32_t reg_dig, uint32_t val_dig)
{
    BSP_DBG_DIG_REG(reg_dig) = val_dig;

    return 0;
}


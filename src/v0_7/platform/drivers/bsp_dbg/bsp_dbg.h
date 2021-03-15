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
#ifndef __DBG_INTERFACE_H_
#define __DBG_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_DBG_DIG_REG(a)                                  (*(volatile uint32_t *)(uintptr_t)(a))

int bsp_dbg_pmu_reg_read(uint16_t reg_pmu, uint16_t *val_pmu);
int bsp_dbg_pmu_reg_write(uint16_t reg_pmu, uint16_t val_pmu);
int bsp_dbg_ana_reg_read(uint16_t reg_ana, uint16_t *val_ana);
int bsp_dbg_ana_reg_write(uint16_t reg_ana, uint16_t val_ana);
int bsp_dbg_rf_reg_read(uint16_t reg_rf, uint16_t *val_rf);
int bsp_dbg_rf_reg_write(uint16_t reg_rf, uint16_t val_rf);
int bsp_dbg_dig_reg_read(uint32_t reg_dig, uint32_t *val_dig);
int bsp_dbg_dig_reg_write(uint32_t reg_dig, uint32_t val_dig);

#ifdef __cplusplus
}
#endif

#endif


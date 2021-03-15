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
#ifndef __HAL_PSC_H__
#define __HAL_PSC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "hal_gpio.h"

void hal_psc_jtag_select_sens(void);

void hal_psc_jtag_select_mcu(void);

void hal_psc_init(void);

void hal_psc_core_auto_power_down(void);

void hal_psc_mcu_auto_power_up(void);

void hal_psc_codec_enable(void);

void hal_psc_codec_disable(void);

void hal_psc_bt_enable(void);

void hal_psc_bt_disable(void);

void hal_psc_bt_enable_auto_power_down(void);

void hal_psc_sens_enable(void);

void hal_psc_sens_disable(void);

void hal_psc_wlan_enable(void);

void hal_psc_wlan_disable(void);

void hal_psc_a7_enable(void);

void hal_psc_a7_disable(void);

void hal_psc_gpio_irq_enable(enum HAL_GPIO_PIN_T pin);

void hal_psc_gpio_irq_disable(enum HAL_GPIO_PIN_T pin);

uint32_t hal_psc_gpio_irq_get_status(uint32_t *status, uint32_t cnt);

uint32_t hal_psc_get_power_loop_cycle_cnt(void);

#ifdef __cplusplus
}
#endif

#endif


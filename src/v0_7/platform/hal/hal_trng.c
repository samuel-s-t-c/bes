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
#ifdef CHIP_HAS_TRNG

#include "plat_addr_map.h"
#include "reg_trng.h"
#include "hal_trng.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_sleep.h"
#include "string.h"
#include "stdbool.h"
#include "cmsis_nvic.h"
#include "hal_timer.h"
#define READ_REG(b,a) \
     (*(volatile uint32_t *)(b+a))

#define WRITE_REG(v,b,a) \
     ((*(volatile uint32_t *)(b+a)) = v)

#define REG(a)          *(volatile uint32_t *)(a)

static struct TRNG_T * const trng = (struct TRNG_T *)TRNG_BASE;
static struct HAL_TRNG_CFG_T trng_cfg;

static volatile uint32_t ehr_data[6] = {0};
#ifdef TRNG_IRQ_MODE
static volatile uint32_t irq_flag = 0;
#endif

int hal_trace_printf_without_crlf_ts(const char *fmt, ...);
void print_binary(unsigned int n)
{
    unsigned int mask = 0;

    mask = 0x80000000;

    for(; mask != 0; mask >>= 1) {
        hal_trace_printf_without_crlf_ts("%c",(n & mask) ? '1' : '0');
    }

}

#ifdef TRNG_IRQ_MODE
static void hal_trng_irq_handler(void)
{
    //TRACE(0, "[TRNG_IRQ]0x%x, [TRNG_BUSY]0x%x, [TRNG_VALID]0x%x.", trng->RNG_ISR, trng->TRNG_BUSY, trng->TRNG_VALID);
    //TRACE(0, "%s 0x%x, 0x%x.", __func__, trng->RNG_ISR, trng->TRNG_VALID);

    trng->RNG_ICR = 0xf;
    if(trng->TRNG_VALID & TRNG_VALID_BIT) {
        ehr_data[0] = trng->EHR_DATA[0];
        ehr_data[1] = trng->EHR_DATA[1];
        ehr_data[2] = trng->EHR_DATA[2];
        ehr_data[3] = trng->EHR_DATA[3];
        ehr_data[4] = trng->EHR_DATA[4];
        ehr_data[5] = trng->EHR_DATA[5];

        //TRACE(0, "[TRNG_TEST]0x%x 0x%x 0x%x 0x%x 0x%x 0x%x.", ehr_data[0], ehr_data[1], ehr_data[2], ehr_data[3], ehr_data[4], ehr_data[5]);

        //output datastream as ascii '0'/'1'
        #if 0
        print_binary(ehr_data[0]);
        print_binary(ehr_data[1]);
        print_binary(ehr_data[2]);
        print_binary(ehr_data[3]);
        print_binary(ehr_data[4]);
        print_binary(ehr_data[5]);
        #endif


    }

    irq_flag = 1;
}
#endif

HAL_TRNG_RET_ENUM hal_trng_open(const struct HAL_TRNG_CFG_T *cfg)
{
    if (cfg == NULL) {
        return HAL_TRNG_RET_FAILED;
    }
    hal_cmu_clock_enable(HAL_CMU_MOD_P_TRNG);
    hal_cmu_reset_clear(HAL_CMU_MOD_P_TRNG);

    trng_cfg = *cfg;

    trng->TRNG_CONFIG = trng_cfg.rnd_src_sel & RND_SRC_SEL_MASK;
    trng->SAMPLE_CNT1 = trng_cfg.sample_cntr1;

    trng->RND_SOURCE_ENABLE |= RND_SOURCE_EN;
    trng->RNG_ICR = 0xf;
    trng->RNG_IMR = 0xe;
#ifdef TRNG_IRQ_MODE
    NVIC_SetVector(TRNG_IRQn, (uint32_t)hal_trng_irq_handler);
    NVIC_SetPriority(TRNG_IRQn, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(TRNG_IRQn);
    NVIC_EnableIRQ(TRNG_IRQn);
#endif
    return HAL_TRNG_RET_OK;
}
void hal_trng_close(void)
{
    //TRACE("%s.", __func__);
    trng->RND_SOURCE_ENABLE = 0;
#ifdef TRNG_IRQ_MODE
    NVIC_DisableIRQ(trng_irq_num);
#endif

    hal_cmu_reset_set(HAL_CMU_MOD_P_TRNG);
    hal_cmu_clock_disable(HAL_CMU_MOD_P_TRNG);

    return;
}

static uint32_t hal_trng_get_data(void)
{
    //TRACE(0, "[TRNG_IRQ]0x%x, [TRNG_BUSY]0x%x, [TRNG_VALID]0x%x.", trng->RNG_ISR, trng->TRNG_BUSY, trng->TRNG_VALID);
    //TRACE(0, "%s 0x%x, 0x%x.", __func__, trng->RNG_ISR, trng->TRNG_VALID);

    if(trng->TRNG_VALID & TRNG_VALID_BIT) {
        ehr_data[0] = trng->EHR_DATA[0];
        ehr_data[1] = trng->EHR_DATA[1];
        ehr_data[2] = trng->EHR_DATA[2];
        ehr_data[3] = trng->EHR_DATA[3];
        ehr_data[4] = trng->EHR_DATA[4];
        ehr_data[5] = trng->EHR_DATA[5];

        return 1;
    }
    //TRACE(0, "%s 0x%x, 0x%x.", __func__, trng->RNG_ISR, trng->TRNG_VALID);
    return 0;
}

void get_trngdata_192(uint32_t* data)
{
    struct HAL_TRNG_CFG_T trng_cfg = {0x20, HAL_TRNG_RND_SRC_SHORTEST};  //sample_cntr1 must not be less than 0x11
#ifdef TRNG_IRQ_MODE
    irq_flag = 0;
#endif
    hal_trng_open(&trng_cfg);
#ifdef TRNG_IRQ_MODE
    while(irq_flag == 0);
#else
    while(!hal_trng_get_data()) { hal_sys_timer_delay(MS_TO_TICKS(1));};
#endif
    memcpy(data, (uint8_t*)ehr_data, 24);
    hal_trng_close();
#ifdef TRNG_IRQ_MODE
    irq_flag = 1;
#endif
}

void get_trngdata(uint32_t* data)
{
    get_trngdata_192(data);
}


#endif // CHIP_HAS_TRNG

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
#ifndef __HAL_TRNG_H__
#define __HAL_TRNG_H__

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum HAL_TRNG_RET {
    HAL_TRNG_RET_OK = 0,
    HAL_TRNG_RET_FAILED,
};
typedef unsigned int HAL_TRNG_RET_ENUM;

enum HAL_TRNG_RND_SRC_SEL {
	HAL_TRNG_RND_SRC_SHORTEST = 0,
	HAL_TRNG_RND_SRC_SHORT,
	HAL_TRNG_RND_SRC_LONG,
	HAL_TRNG_RND_SRC_LONGEST,
};
typedef unsigned int HAL_TRNG_RND_SRC_SEL_ENUM;

struct HAL_TRNG_CFG_T {
    uint32_t 					sample_cntr1; // must not be less than 0x11
    HAL_TRNG_RND_SRC_SEL_ENUM   rnd_src_sel;
};

HAL_TRNG_RET_ENUM hal_trng_open(const struct HAL_TRNG_CFG_T *cfg);
void hal_trng_close(void);
void get_trngdata(uint32_t *data);

#ifdef __cplusplus
}
#endif

#endif //__HAL_TRNG_H__

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
#include "bt_drv.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "cmsis.h"
#include "hal_cmu.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "tgt_hardware.h"
#include "bt_drv_internal.h"
#include "bt_drv_2500i_internal.h"
#include "hal_chipid.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "pmu.h"
#include "iqcorrect.h"
#include "hal_btdump.h"

#define AUTO_CAL                                0

#ifndef BT_RF_MAX_XTAL_TUNE_PPB
// Default 10 ppm/bit or 10000 ppb/bit
#define BT_RF_MAX_XTAL_TUNE_PPB                 10000
#endif

#ifndef BT_RF_XTAL_TUNE_FACTOR
// Default 0.2 ppm/bit or 200 ppb/bit
#define BT_RF_XTAL_TUNE_FACTOR                  200
#endif


#define XTAL_FCAP_NORMAL_SHIFT                  0
#define XTAL_FCAP_NORMAL_MASK                   (0xFF << XTAL_FCAP_NORMAL_SHIFT)
#define XTAL_FCAP_NORMAL(n)                     BITFIELD_VAL(XTAL_FCAP_NORMAL, n)

#define BT_XTAL_CMOM_DR                         (1 << 13)

#define RF_REG_XTAL_FCAP                        0xE9
#define RF_REG_XTAL_CMOM_DR                     0xE8

#define SPI_TRIG_RX_NEG_TIMEOUT                 MS_TO_TICKS(3)

#define SPI_TRIG_NEG_TIMEOUT                    SPI_TRIG_RX_NEG_TIMEOUT


enum BIT_OFFSET_CMD_TYPE_T
{
    BIT_OFFSET_CMD_STOP = 0,
    BIT_OFFSET_CMD_START,
    BIT_OFFSET_CMD_ACK,
};

static uint16_t xtal_fcap = DEFAULT_XTAL_FCAP;
static uint16_t init_xtal_fcap = DEFAULT_XTAL_FCAP;

struct bt_drv_tx_table_t
{
    uint16_t tbl[16][3];
};


struct RF_SYS_INIT_ITEM
{
    uint16_t reg;
    uint16_t set;
    uint16_t mask;
    uint16_t delay;
};

static const struct RF_SYS_INIT_ITEM rf_sys_init_tbl[] =
{
    {0xb5,0x8000,0x8000,0},
    {0xc4,0x00df,0x0003,0},
};

#define REG_EB_VAL 0x083f
#define REG_181_VAL (0x00ce)
#define REG_EC_VAL 0x081c
#define REG_182_VAL 0x00ce
#define REG_ED_VAL 0x00c5
#define REG_183_VAL 0x00ce
#define REG_EE_VAL 0x01c5
#define REG_184_VAL 0x00be
#define REG_EF_VAL 0x0145
#define REG_185_VAL 0x00b6
#define REG_F0_VAL 0x0345
#define REG_186_VAL 0x0036
#define REG_F1_VAL 0x0345
#define REG_187_VAL 0x002d
#define REG_F2_VAL 0x0345
#define REG_188_VAL 0x000c

const uint16_t rf_init_tbl_1[][3] =
{
    {0xc2,0x7188,0},//vtoi normal
    {0x88,0x8640,0},
    {0x8b,0x784a,0},//set rx flt cap
    {0x8e,0x4728,0},
    {0x90,0x891f,0},
    {0x91,0x05c0,0},
    {0x92,0x6a8e,0},
    {0x96,0x508A,0},
    {0x97,0x2523,0},//0x2403 0x2413/20180703 0x2213/20180803 0x2013/20180814 0x2023/20181010
    {0x98,0x1224,0},//0x0323
    {0x9a,0x4470,0},//div2 rc
    {0x9b,0xfD22,0},//0xfe42
    {0x9c,0x180f,0},
    //{0x9d,0x086c,0},//enhance xtal drv
    {0xa3,0x0789,0},
    {0xa5,0x1004,0},//0x0c04
    {0xa6,0x2820,0},//0x100c
    {0xa8,0x2512,0},
    {0xab,0x300c,0}, //flt pwrup delay
    {0xb0,0x0000,0},
    {0xb1,0x0000,0},
    {0xb3,0x0000,0},//0xb30f3
    {0xb4,0x883c,0},
    {0xb6,0x3156,0},
    {0xb7,0x1820,0},//2g4_vco_div2=0
    {0xb9,0x8000,0},//cap3g6=1
    {0xba,0x104e,0},
    {0xc3,0x0048,0},//0x0090
    {0xc5,0x4b50,0},//vco ictrl dr
    {0xc9,0x3E08,0},//vco ictrl
    {0xcf,0x7f32,0},//rx lna att !!!!!!!!!!!!!!!!!!!!!!!!!!!!!0726 0x8000
    {0xd1,0x8412,0},//set gain 0dB, 0x843b 0x84Cb
    {0xd3,0xc1c1,0},//vcdl_bit=001110 delay ctrl
    {0xd4,0x000f,0},
    {0xd5,0x4000,0},
    {0xd6,0x7980,0},
    {0xe8,0xe000,0},//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!0726
    {0xdf,0x2006,0},//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!0726 0x0007
    {0xf3,0x0c01,0},//improve the problem of Rx BER bit error rate at low temperature by langwei 20201202

    {0xeb,REG_EB_VAL,0},//gain_idx:0
    {0x181,REG_181_VAL,0},
    {0xec,REG_EC_VAL,0},//gain_idx:1
    {0x182,REG_182_VAL,0},
    {0xed,REG_ED_VAL,0},//gain_idx:2
    {0x183,REG_183_VAL,0},
    {0xee,REG_EE_VAL,0},//gain_idx:3
    {0x184,REG_184_VAL,0},
    {0xef,REG_EF_VAL,0},//gain_idx:4
    {0x185,REG_185_VAL,0},
    {0xf0,REG_F0_VAL,0},//gain_idx:5
    {0x186,REG_186_VAL,0},
    {0xf1,REG_F1_VAL,0},//gain_idx:6
    {0x187,REG_187_VAL,0},
    {0xf2,REG_F2_VAL,0},//gain_idx:7
    {0x188,REG_188_VAL,0},

    {0x191,0x0035,0},

    {0x1b8,0x020a,0},
    {0x1b9,0x27F,0},
    {0x1ba,0x27F,0},
    {0x1bb,0x27F,0},
    {0x1bc,0x27F,0},
    {0x1bd,0x027f,0},
    {0x1be,0x027f,0},
    {0x1bf,0x027f,0},
    {0x1f4,0x2002,0},
    {0x2af,0x9,0},

    {0xad,0xa04a,0},
    {0xcd,0x0040,0},
    {0x8a,0x4EA4,0},
    {0xe9,0x8020,0},
    {0x85,0x7f00,0},//Walker 20181012
    {0xac,0x080e,0},

    //[BT vtoi]
    //{0x1c4,0xffff},
    //{0x1c5,0xffff},
    //{0x1c6,0xffff},
    //{0x1c7,0xffff},

    //[rxflt_gain]
    {0x1a6,0x0600,0},//IF=1.5M
    {0x1d1,0x0000,0},//lna_gain_max
    {0x1d2,0xe1f0,0},//lna_gain
    {0xd0,0x0000,0},//i2v_filter_gain i2v_gain=9, filter_gain=3

    //[rx tstbuf flt]
    {0xa2,0x67c2,0},//tstbuf
    {0x8c,0x9000,0},//flt_out

    {0x1b7,0x2a00,0},
    {0x1cc,0x668c,0},

};

#ifdef __HW_AGC__
const uint16_t rf_init_tbl_1_hw_agc[][3] = //hw agc table
{
};
#endif //__HW_AGC__

const uint16_t rf_init_tbl_1_sw_agc[][3] = //sw agc table
{
    {0xad,0xa00a,1},//hwagc en=0
    {0xCD,0x0000,0},//default 0x0000
    {0xcf,0x0000,0},//lna gain dr=0 //default 0x0000
    {0xd0,0x0000,0},//i2v flt gain dr=0 //default 0x0000
    {0xeb,REG_EB_VAL,0},//gain_idx:0
    {0xec,REG_EC_VAL,0},//gain_idx:1
    {0xed,REG_ED_VAL,0},//gain_idx:2
    {0xee,REG_EE_VAL,0},//gain_idx:3
    {0xef,REG_EF_VAL,0},//gain_idx:4
    {0xf0,REG_F0_VAL,0},//gain_idx:5
    {0xf1,REG_F1_VAL,0},//gain_idx:6
    {0xf2,REG_F2_VAL,0},//gain_idx:7
};

uint32_t btdrv_rf_get_max_xtal_tune_ppb(void)
{
    return BT_RF_MAX_XTAL_TUNE_PPB;
}

uint32_t btdrv_rf_get_xtal_tune_factor(void)
{
    return BT_RF_XTAL_TUNE_FACTOR;
}

void btdrv_rf_init_xtal_fcap(uint32_t fcap)
{
    xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
    btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
    init_xtal_fcap = xtal_fcap;
}

uint32_t btdrv_rf_get_init_xtal_fcap(void)
{
    return GET_BITFIELD(init_xtal_fcap, XTAL_FCAP_NORMAL);
}

uint32_t btdrv_rf_get_xtal_fcap(void)
{
    return GET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL);
}

void btdrv_rf_set_xtal_fcap(uint32_t fcap, uint8_t is_direct)
{
    xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
    btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
}

int btdrv_rf_xtal_fcap_busy(uint8_t is_direct)
{
    return 0;
}

void btdrv_rf_bit_offset_track_enable(bool enable)
{
    return;
}

uint32_t btdrv_rf_bit_offset_get(void)
{
    return 0;
}

uint16_t btdrv_rf_bitoffset_get(uint8_t conidx)
{
    uint16_t bitoffset;
    bitoffset = BTDIGITAL_REG(EM_BT_BITOFF_ADDR+conidx*BT_EM_SIZE) & 0x3ff;
    return bitoffset;
}

void btdrv_rf_log_delay_cal(void)
{
    unsigned short read_value;
    unsigned short write_value;
    BT_DRV_TRACE(0,"btdrv_rf_log_delay_cal\n");
    BTDIGITAL_REG(0xd0340020) = 0x010e01c0;
    BT_DRV_TRACE(1,"0xd0340020 =%x\n",BTDIGITAL_REG(0xd0340020) );

    btdrv_write_rf_reg(0xd4, 0x000f);
    btdrv_write_rf_reg(0xd5, 0x4000);
    btdrv_write_rf_reg(0xd2, 0x1003);
    btdrv_write_rf_reg(0xa7, 0x004e);
    btdrv_write_rf_reg(0xd4, 0x0000);
    btdrv_write_rf_reg(0xd5, 0x4002);

    BTDIGITAL_REG(0xd0340020) = 0x030e01c1;
    BT_DRV_TRACE(1,"0xd0340020 =%x\n",BTDIGITAL_REG(0xd0340020) );

    btdrv_delay(1);

    btdrv_write_rf_reg(0xd2, 0x5003);

    btdrv_delay(1);

    btdrv_read_rf_reg(0x1e2, &read_value);
    BT_DRV_TRACE(1,"0x1e2 read_value:%x\n",read_value);
    if(read_value == 0xff80)
    {
        btdrv_write_rf_reg(0xd3, 0xffff);
    }
    else
    {
        write_value = ((read_value>>7)&0x0001) | ((read_value & 0x007f)<<1) | ((read_value&0x8000)>>7) | ((read_value&0x7f00)<<1);
        BT_DRV_TRACE(1,"d3 write_value:%x\n",write_value);
        btdrv_write_rf_reg(0xd3, write_value);
    }
    btdrv_delay(1);

    BTDIGITAL_REG(0xd0340020) = 0x010e01c0;
    BT_DRV_TRACE(1,"0xd0340020 =%x\n",BTDIGITAL_REG(0xd0340020) );


    btdrv_write_rf_reg(0xd4, 0x000f);
    btdrv_write_rf_reg(0xd2, 0x1003);
    btdrv_write_rf_reg(0xd5, 0x4000);

}

void btdrv_rf_log_delay_cal_init(void)
{
    unsigned short read_value,read_val;
    BT_DRV_TRACE(0,"btdrv_rf_log_delay_cal_init\n");

    btdrv_write_rf_reg(0xa7, 0x004e);

    btdrv_write_rf_reg(0xd4, 0x0000);
    btdrv_write_rf_reg(0xd5, 0x4002);

    BTDIGITAL_REG(0xd0340020) = 0x030e01c1;
    BT_DRV_TRACE(1,"0xd0340020 =%x\n",BTDIGITAL_REG(0xd0340020) );

    btdrv_delay(1);

    btdrv_write_rf_reg(0xd2, 0x5003);

    btdrv_delay(1);

    btdrv_read_rf_reg(0x1e2, &read_value);
    BT_DRV_TRACE(1,"0x1e2 read_value:%x\n",read_value);

    if(read_value == 0x8080)
    {
        btdrv_read_rf_reg(0x9b, &read_val);
        read_val |= 1 << 9;
        read_val &= ~(1 << 8);
        btdrv_write_rf_reg(0x9b,read_val);

        btdrv_read_rf_reg(0x9b, &read_val);
        BT_DRV_TRACE(1,"final: 0x9b read_val:%x\n",read_val);
    }
    else if(read_value == 0xff80)
    {
        btdrv_read_rf_reg(0x9b, &read_val);
        read_val |= 1 << 8;
        read_val &= ~(1 << 9);
        btdrv_write_rf_reg(0x9b,read_val);

        btdrv_read_rf_reg(0x9b, &read_val);
        BT_DRV_TRACE(1,"final: 0x9b read_val:%x\n",read_val);
    }
}


void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain)
{
    return;
}

void btdrv_2500i_cap_delay_cal(void)
{
    uint16_t read_val = 0;
    uint8_t  cap_delay_val = 0;
    //read calibrated val from efuse 0x05 register
    pmu_get_efuse(PMU_EFUSE_PAGE_SW_CFG, &read_val);
    BT_DRV_TRACE(2,"%s efuse 05:%x\n",__func__,read_val);

    cap_delay_val = read_val & 0xf;     //takeout efuse [3:0]
    if (0 == cap_delay_val)
        cap_delay_val = 0xe;
    BT_DRV_TRACE(2,"%s cap_delay_val:%x\n",__func__,cap_delay_val);

    btdrv_read_rf_reg(0x97,&read_val);
    BT_DRV_TRACE(2,"%d 0x97 origin val:%x\n",__LINE__,read_val);

    read_val &= 0xf87f;                 //clear [10:7]
    read_val |= ((cap_delay_val & 0xffff) << 7);
    BT_DRV_TRACE(2,"%d 0x97 final val:%x\n",__LINE__,read_val);
    btdrv_write_rf_reg(0x97,read_val);
}

//rf Image calib
void btdtv_rf_image_calib(void)
{
    uint16_t read_val = 0;
    //read calibrated val from efuse 0x05 register
    pmu_get_efuse(PMU_EFUSE_PAGE_SW_CFG, &read_val);
    //check if bit 11 has been set
    uint8_t calb_done_flag = ((read_val &0x800)>>11);
    if(calb_done_flag)
    {
        BT_DRV_TRACE(1,"EFUSE REG[5]=%x",read_val);
    }
    else
    {
        BT_DRV_TRACE(0,"EFUSE REG[5] rf image has not been calibrated!");
        return;
    }
    //[bit 12] calib flag
    uint8_t calib_val = ((read_val &0x1000)>>12);
    btdrv_read_rf_reg(0x9b,&read_val);
    read_val&=0xfcff;

    if(calib_val==0)
    {
        read_val|= 1<<8;
    }
    else if(calib_val== 1)
    {
        read_val|= 1<<9;
    }

    BT_DRV_TRACE(1,"write rf image calib val=%x in REG[0x9b]", read_val);
    btdrv_write_rf_reg(0x9b,read_val);
}

#ifdef TX_IQ_CAL
const uint16_t tx_cal_rfreg_set[][3] =
{
    {0x81, 0x0001},
    {0x8c, 0x81f0},
    {0xaf, 0xc000},
    {0x1d2, 0xe1f0},
    {0x1d6, 0x0858},
    {0xa2, 0x67c2},
    {0x94, 0x87cf},
    {0xd1, 0x84bb},
    {0x95, 0xe4ff},
    {0xb1, 0x0300},
    {0xb2, 0x3030},
    {0xba, 0x304e},
    {0xaf, 0xc00c},
    {0xbf, 0x5000},
    {0xbf, 0x7000},
    {0x90, 0x89df},
    {0x8b, 0x404a},
};

const uint16_t tx_cal_rfreg_store[][1] =
{
    {0x81},
    {0x8c},
    {0xaf},
    {0x1d2},
    {0x1d6},
    {0xa2},
    {0x94},
    {0xd1},
    {0x95},
    {0xb1},
    {0xb2},
    {0xba},
    {0xaf},
    {0xbf},
    {0xbf},
    {0x90},
    {0x8b},
};

int bt_iqimb_ini ();
int bt_iqimb_test_ex (int mismatch_type);
void btdrv_tx_iq_cal(void)
{
    uint8_t i;
    const uint16_t (*tx_cal_rfreg_set_p)[3];
    const uint16_t (*tx_cal_rfreg_store_p)[1];
    uint32_t reg_set_tbl_size = 0;
    uint16_t value = 0;
    uint16_t RF_BF_VAl = 0;
    uint32_t tx_cal_digreg_store[6];

    tx_cal_rfreg_store_p = &tx_cal_rfreg_store[0];
    uint32_t reg_store_tbl_size = sizeof(tx_cal_rfreg_store)/sizeof(tx_cal_rfreg_store[0]);
    uint16_t tx_cal_rfreg_store[reg_store_tbl_size];
    BT_DRV_TRACE(0,"reg_store:\n");
    for(i=0; i< reg_store_tbl_size; i++)
    {
        btdrv_read_rf_reg(tx_cal_rfreg_store_p[i][0],&value);
        tx_cal_rfreg_store[i] = value;
        BT_DRV_TRACE(2,"reg=%x,v=%x",tx_cal_rfreg_store_p[i][0],value);
    }
    btdrv_read_rf_reg(0xBF,&RF_BF_VAl);
    BT_DRV_TRACE(2,"reg=%x,v=%x",0xBF,RF_BF_VAl);

    tx_cal_digreg_store[0] = BTDIGITAL_REG(0xd0340020);
    tx_cal_digreg_store[1] = BTDIGITAL_REG(0xd0330038);
    tx_cal_digreg_store[2] = BTDIGITAL_REG(0xd0350364);
    tx_cal_digreg_store[3] = BTDIGITAL_REG(0xd0350360);
    tx_cal_digreg_store[4] = BTDIGITAL_REG(0xd035037c);
    tx_cal_digreg_store[5] = BTDIGITAL_REG(0xd02201e4);
    BT_DRV_TRACE(1,"0xd0340020:%x\n",tx_cal_digreg_store[0]);
    BT_DRV_TRACE(1,"0xd0330038:%x\n",tx_cal_digreg_store[1]);
    BT_DRV_TRACE(1,"0xd0350364:%x\n",tx_cal_digreg_store[2]);
    BT_DRV_TRACE(1,"0xd0350360:%x\n",tx_cal_digreg_store[3]);
    BT_DRV_TRACE(1,"0xd035037c:%x\n",tx_cal_digreg_store[4]);
    BT_DRV_TRACE(1,"0xd02201e4:%x\n",tx_cal_digreg_store[5]);


    tx_cal_rfreg_set_p = &tx_cal_rfreg_set[0];
    reg_set_tbl_size = sizeof(tx_cal_rfreg_set)/sizeof(tx_cal_rfreg_set[0]);
    BT_DRV_TRACE(0,"reg_set:\n");
    for(i=0; i< reg_set_tbl_size; i++)
    {
        btdrv_write_rf_reg(tx_cal_rfreg_set_p[i][0],tx_cal_rfreg_set_p[i][1]);
        if(tx_cal_rfreg_set_p[i][2] !=0)
            btdrv_delay(tx_cal_rfreg_set_p[i][2]);//delay
        btdrv_read_rf_reg(tx_cal_rfreg_set_p[i][0],&value);
        BT_DRV_TRACE(2,"reg=%x,v=%x",tx_cal_rfreg_set_p[i][0],value);
    }

    BTDIGITAL_REG(0xd0330038) = 0x0030030d;
    btdrv_delay(1);
    BTDIGITAL_REG(0xd0340020) = 0x030e01c0;
    BTDIGITAL_REG(0xd0350364) = 0x002eb948;
    BTDIGITAL_REG(0xd0350360) = 0x007fc240;
    BTDIGITAL_REG(0xd035037c) = 0x00020405;

    BT_DRV_TRACE(1,"0xd0340020:%x\n",BTDIGITAL_REG(0xd0340020));
    BT_DRV_TRACE(1,"0xd0330038:%x\n",BTDIGITAL_REG(0xd0330038));
    BT_DRV_TRACE(1,"0xd0350364:%x\n",BTDIGITAL_REG(0xd0350364));
    BT_DRV_TRACE(1,"0xd0350360:%x\n",BTDIGITAL_REG(0xd0350360));
    BT_DRV_TRACE(1,"0xd035037c:%x\n",BTDIGITAL_REG(0xd035037c));
    BT_DRV_TRACE(1,"0xd02201e4:%x\n",BTDIGITAL_REG(0xd02201e4));

    bt_iqimb_ini();

    BTRF_REG_SET_FIELD(0xBF,0x3,8, 0x1);
    BTRF_REG_SET_FIELD(0xBF,0x3,8, 0x3);

    bt_iqimb_test_ex(1);

    BT_DRV_TRACE(0,"reg_reset:\n");
    for(i=0; i< reg_store_tbl_size; i++)
    {
        btdrv_write_rf_reg(tx_cal_rfreg_store_p[i][0],tx_cal_rfreg_store[i]);

        btdrv_read_rf_reg(tx_cal_rfreg_store_p[i][0],&value);
        BT_DRV_TRACE(2,"reg=%x,v=%x",tx_cal_rfreg_store_p[i][0],value);
    }
    btdrv_write_rf_reg(0xBF,RF_BF_VAl);
    btdrv_read_rf_reg(0xBF,&value);
    BT_DRV_TRACE(2,"reg=%x,v=%x",0xBF,value);

    BTDIGITAL_REG(0xd0340020) = tx_cal_digreg_store[0];
    BTDIGITAL_REG(0xd0330038) = tx_cal_digreg_store[1];
    BTDIGITAL_REG(0xd0350364) = tx_cal_digreg_store[2];
    BTDIGITAL_REG(0xd0350360) = tx_cal_digreg_store[3];
    BTDIGITAL_REG(0xd035037c) = tx_cal_digreg_store[4];
    BTDIGITAL_REG(0xd02201e4) = tx_cal_digreg_store[5];
    BT_DRV_TRACE(1,"0xd0340020:%x\n",BTDIGITAL_REG(0xd0340020));
    BT_DRV_TRACE(1,"0xd0330038:%x\n",BTDIGITAL_REG(0xd0330038));
    BT_DRV_TRACE(1,"0xd0350364:%x\n",BTDIGITAL_REG(0xd0350364));
    BT_DRV_TRACE(1,"0xd0350360:%x\n",BTDIGITAL_REG(0xd0350360));
    BT_DRV_TRACE(1,"0xd035037c:%x\n",BTDIGITAL_REG(0xd035037c));
    BT_DRV_TRACE(1,"0xd02201e4:%x\n",BTDIGITAL_REG(0xd02201e4));
}
#endif

uint8_t btdrv_rf_init(void)
{
    uint16_t value = 0;
    const uint16_t (*rf_init_tbl_p)[3] = NULL;
    uint32_t tbl_size = 0;
    //uint8_t ret;
    uint8_t i;

    for (i = 0; i < ARRAY_SIZE(rf_sys_init_tbl); i++)
    {
        btdrv_read_rf_reg(rf_sys_init_tbl[i].reg, &value);
        value = (value & ~rf_sys_init_tbl[i].mask) | (rf_sys_init_tbl[i].set & rf_sys_init_tbl[i].mask);
        if (rf_sys_init_tbl[i].delay)
        {
            btdrv_delay(rf_sys_init_tbl[i].delay);
        }
        btdrv_write_rf_reg(rf_sys_init_tbl[i].reg, value);
    }

    rf_init_tbl_p = &rf_init_tbl_1[0];
    tbl_size = sizeof(rf_init_tbl_1)/sizeof(rf_init_tbl_1[0]);

    for(i=0; i< tbl_size; i++)
    {
        btdrv_write_rf_reg(rf_init_tbl_p[i][0],rf_init_tbl_p[i][1]);
        if(rf_init_tbl_p[i][2] !=0)
            btdrv_delay(rf_init_tbl_p[i][2]);//delay
        btdrv_read_rf_reg(rf_init_tbl_p[i][0],&value);
        BT_DRV_TRACE(2,"reg=%x,v=%x",rf_init_tbl_p[i][0],value);
    }

#ifdef __HW_AGC__
    for(i=0; i< sizeof(rf_init_tbl_1_hw_agc)/sizeof(rf_init_tbl_1_hw_agc[0]); i++)
    {
        btdrv_write_rf_reg(rf_init_tbl_1_hw_agc[i][0],rf_init_tbl_1_hw_agc[i][1]);
        if(rf_init_tbl_1_hw_agc[i][2] !=0)
            btdrv_delay(rf_init_tbl_1_hw_agc[i][2]);//delay
    }
#endif

#ifdef __NEW_SWAGC_MODE__
    bt_drv_rf_set_bt_sync_agc_enable(true);
#endif

#ifdef __BLE_NEW_SWAGC_MODE__
    bt_drv_rf_set_ble_sync_agc_enable(true);
#endif

    //btdrv_2500i_cap_delay_cal();

    //need before rf log delay cal
    //btdtv_rf_image_calib();
    btdrv_rf_log_delay_cal_init();
    btdrv_rf_log_delay_cal();

    btdrv_spi_trig_init();

#ifdef TX_IQ_CAL
    hal_btdump_clk_enable();
    bt_iq_calibration_setup();
#ifndef IQ_CALI_TEST
    hal_btdump_clk_disable();
#endif
#endif

    return 1;
}

void bt_drv_rf_reset(void)
{
    btdrv_write_rf_reg(0x80,0xcafe);
    btdrv_write_rf_reg(0x80,0x5fee);
}

void bt_drv_rf_set_bt_sync_agc_enable(bool enable)
{
    uint16_t val_e2 = 0;
    btdrv_read_rf_reg(0xe2,&val_e2);
    if(enable)
    {
        //open rf new sync agc mode
        val_e2 |= (1<<6);
    }
    else
    {
        //close rf new sync agc mode
        val_e2 &= ~(1<<6);
    }
    btdrv_write_rf_reg(0xe2,val_e2);
}

void bt_drv_rf_set_ble_sync_agc_enable(bool enable)
{
    uint16_t val_e2 = 0;
    btdrv_read_rf_reg(0xe2,&val_e2);
    if(enable)
    {
        //open rf new sync agc mode
        val_e2 |= (1<<5);
    }
    else
    {
        //close rf new sync agc mode
        val_e2 &= ~(1<<5);
    }
    btdrv_write_rf_reg(0xe2,val_e2);
}

//switch old swagc and new sync swagc
void bt_drv_select_agc_mode(enum BT_WORK_MODE_T mode)
{
#ifdef __NEW_SWAGC_MODE__
    static uint8_t agc_mode_bak = SWAGC_INIT_MODE;
    uint8_t agc_mode = OLD_SWAGC_MODE;

    switch(mode)
    {
        case BT_A2DP_WORK_MODE:
            agc_mode = NEW_SYNC_SWAGC_MODE;
            break;
        case BT_IDLE_MODE:
        case BT_HFP_WORK_MODE:
            agc_mode = OLD_SWAGC_MODE;
            break;
        default:
            BT_DRV_TRACE(1,"BT_DRV:set error mork mode=%d",mode);
            break;
    }

    if(agc_mode_bak != agc_mode)
    {
        agc_mode_bak = agc_mode;
        BT_DRV_TRACE(1,"BT_DRV:use SW AGC mode=%d[2:OLD,1:NEW sync]",agc_mode);
        bt_drv_reg_op_swagc_mode_set(agc_mode);
    }
#endif
}

static uint16_t efuse;
static int16_t rf18b_6_4, rf18b_10_8, rf18b_14_12;
#define TX_POWET_CALIB_FACTOR 0x2

static int check_btpower_efuse_invalid(void)
{
    pmu_get_efuse(PMU_EFUSE_PAGE_BT_POWER, &efuse);
    BT_DRV_TRACE(1,"efuse_8=0x%x",efuse);

    rf18b_6_4   = (efuse & 0x70)    >> 4;                     //address_8 [6:4]
    rf18b_10_8  = (efuse & 0x700)   >> 8;                     //address_8 [10:8]
    rf18b_14_12 = (efuse & 0x7000)  >> 12;                    //address_8 [14:12]

    if((0 == efuse)       ||
       (rf18b_6_4   > 5)  ||
       (rf18b_10_8  > 5)  ||
       (rf18b_14_12 > 5))
    {
        BT_DRV_TRACE(0,"invalid efuse value.");
        return 0;
    }

    return 1;
}

void bt_drv_tx_pwr_init(void)
{
    //ble txpower need modify ble tx idx @ bt_drv_config.c
    //modify bit4~7 to change ble tx gain
    if (0 == check_btpower_efuse_invalid())
    {
        btdrv_write_rf_reg(0x189, 0x0076); // min tx gain  2019.02.26
        btdrv_write_rf_reg(0x18a, 0x0076); // mid tx gain  2019.02.26
        btdrv_write_rf_reg(0x18b, 0x00B6); // max tx gain 2019.02.26
    }
    else
    {
        uint16_t read_value = 0;
        btdrv_read_rf_reg(0x18b, &read_value);
        btdrv_write_rf_reg(0x189, read_value);
        btdrv_write_rf_reg(0x18a, read_value);
    }

    btdrv_write_rf_reg(0x18c, 0x0072); // binding with FA_FIX_TX_GIAN_IDX
}

void bt_drv_tx_pwr_init_for_testmode(void)
{
    //ble txpower need modify ble tx idx @ bt_drv_config.c
    //modify bit4~7 to change ble tx gain
    btdrv_write_rf_reg(0x189, 0x007f); // min tx gain  2019.02.26
    btdrv_write_rf_reg(0x18a, 0x007a); // mid tx gain  2019.02.26
    if (0 == check_btpower_efuse_invalid())
        btdrv_write_rf_reg(0x18b, 0x00B6); // max tx gain 2019.02.26
}

void btdrv_txpower_calib(void)
{
    uint16_t rf92_11_8 = 0;
    uint16_t tmp_val = 0;
    int16_t  average_value = 0;               //may be negative, so use signed numbers
    uint16_t read_value = 0;

    uint16_t bit7_symbol = 0, bit11_symbol = 0, bit15_symbol = 0;

    if(0 == check_btpower_efuse_invalid())
        return;

    bit7_symbol     = (efuse & 0x80)    >> 4;
    bit11_symbol    = (efuse & 0x800)   >> 8;
    bit15_symbol    = (efuse & 0x8000)  >> 12;
    rf92_11_8   = efuse & 0xf;                                //address_8 [3:0]

    BT_DRV_TRACE(3, "bit7_symbol=%d, bit11_symbol=%d, bit15_symbol=%d",
                 bit7_symbol,
                 bit11_symbol,
                 bit15_symbol);
    BT_DRV_TRACE(4, "rf92_11_8=%x, rf18b_6_4=%x, rf18b_10_8=%x, rf18b_14_12=%x",
                 rf92_11_8,
                 rf18b_6_4,
                 rf18b_10_8,
                 rf18b_14_12);

    rf18b_6_4   = (bit7_symbol == 0)  ? rf18b_6_4   : -rf18b_6_4;
    rf18b_10_8  = (bit11_symbol == 0) ? rf18b_10_8  : -rf18b_10_8;
    rf18b_14_12 = (bit15_symbol == 0) ? rf18b_14_12 : -rf18b_14_12;

    //set 0x92[11:8] begin
    btdrv_read_rf_reg(0x92,&tmp_val);
    BT_DRV_TRACE(1,"read reg 0x92 val=%x",tmp_val);
    tmp_val &= 0xf0ff;                                        //clear [11:8]
    tmp_val |= ((rf92_11_8 & 0xffff) << 8);                   //set efuse[3:0] to rf_92 [11:8]
    BT_DRV_TRACE(2,"%d write reg 0x92 val=%x",__LINE__,tmp_val);
    btdrv_write_rf_reg(0x92,tmp_val);
    //set 0x92[11:8] end

    //set 0x18b[3:0] begin
    btdrv_read_rf_reg(0x18b, &read_value);                  //0x18b

    average_value = (int16_t)(( (float)(rf18b_6_4 + rf18b_10_8 + rf18b_14_12) / 6.f) + 0.5f);
    BT_DRV_TRACE(2,"calc average_value=0x%x, dec:%d",average_value, average_value);

    if(0 < average_value)
    {
        btdrv_read_rf_reg(0x92,&tmp_val);
        tmp_val &= 0x0fff;                                      //clear [15:12]

        if(1 == average_value)
        {
            tmp_val |= ((0x8 & 0xffff) << 12);
        }
        else    //average_value [2,~]
        {
            tmp_val |= ((0xA & 0xffff) << 12);
        }
        BT_DRV_TRACE(2,"%d write reg 0x92 val=%x",__LINE__,tmp_val);
        btdrv_write_rf_reg(0x92,tmp_val);
    }

    tmp_val = TX_POWET_CALIB_FACTOR - average_value;
    if(average_value > TX_POWET_CALIB_FACTOR)
        tmp_val = 0;

    BT_DRV_TRACE(1,"finally tmp_val=%x",tmp_val);

    read_value &= 0xff00;                                  //clear [3:0] & [7:4]
    read_value |= (tmp_val | (0x7 << 4));                  //get 0x18b [3:0] & [7:4]
    btdrv_write_rf_reg(0x18b,read_value);
    BT_DRV_TRACE(1,"write reg 0x18b val=0x%x",read_value);
    //set 0x18b[3:0] end
}

void btdrv_reconn(bool en)
{
    uint16_t rf_val = 0;

    btdrv_read_rf_reg(0xD2, &rf_val);
    //TRACE(1,"origin: RF_D2=0x%x", rf_val);

    if(en){
        rf_val |= (1<<8);
    } else{
        rf_val &= ~(1<<8);
    }
    btdrv_write_rf_reg(0xD2, rf_val);

    pmu_bt_reconn(en);

    //btdrv_read_rf_reg(0xD2, &rf_val);
    //TRACE(1,"final: RF_D2=0x%x", rf_val);
}
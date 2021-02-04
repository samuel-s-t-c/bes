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
#include "plat_addr_map.h"		// 根据芯片类型, 导入对应的头文件 该头文件包含地址映射关系, 宏定义
#include "analog.h"				// 定义芯片模拟部分(包括射频,adc,降噪等)相关的函数,参数,宏等
#include "apps.h"				// 定义应用层(开关机,配对,录音,电池状态报告)相关的函数,参数,定时器等
#include "app_bt_stream.h"		// 定义蓝牙音频流(包括a2dp(aac,sbc),hfp)相关的函数,参数,音频流操作等
#include "cmsis.h"				// 定义中断相关优先级,以及处理中断的lock/unlock操作
#include "hal_bootmode.h"		// 定义启动过程中不同功能块对应的位(低4位用于硬启动,高28位用于软启动),以及定义读写启动模式的函数
#include "hal_cmu.h"			// 定义CMU(主控单元?)对应的晶振,时钟,定时器以及LPU(line protocol unit)相关的函数,参数,操作
#include "hal_dma.h"			// 定义DMA(direct memory access)相关的类型,操作,参数
#include "hal_gpio.h"			// 定义GPIO口的中断类型, 触发方式, pin脚的读写操作
#include "hal_iomux.h"			// 定义各种IO方式(uart,i2c,spi,电源键)相关的类型,操作,参数
#include "hal_location.h"		// 定义内存区域的分配以及不同类型(text,data,bss)的参数,宏
#include "hal_norflash.h"		// 定义访问norflash的模式,配置,操作(读,写,擦除),函数
#include "hal_sleep.h"			// 定义sys/cpu/chip的休眠类型,以及相关的休眠唤醒等操作函数
#include "hal_timer.h"			// 定义定时器相关的宏以及操作函数
#include "hal_trace.h"			// 定义不同详细程度的日志级别,以及分模块的不同操作的定义(打开,关闭,设置)
#include "hal_wdt.h"			// 定义看门狗定时器的操作(启动,停止,读,设置)
#include "hwtimer_list.h"		// 定义硬件定时器的类型和操作(初始化,启动,停止,设置)
#include "mpu.h"				// 定义mpu(memory protection unit)的针对内存的不同保护类型,以及相关操作(打开,关闭,设置,清除,休眠,唤醒)
#include "norflash_api.h"		// 定义norflash的分模块的操作(读,写,flush,擦除)
#include "pmu.h"				// 定义pmu(power management unit)的充电,放电,开机,关机,休眠,rtc,watchdog,不同状态(如anc,eq)下功耗参数设置
#include "stdlib.h"
#include "tgt_hardware.h"		// 定义adc,gpiokey,anc,音频输入输出,电池等硬件配置信息
#include "app_utils.h"			// 定义不同业务状态下(a2dp,sco等)对系统主频的要求(32K,...,104M,208M)
#include "watchdog/watchdog.h"	// 定义watchdogtimer的操作函数(open, stop, start等)

#ifdef RTOS
#include "cmsis_os.h"
#include "app_factory.h"		// 定义工厂模式的相关操作
#endif
#ifdef CORE_DUMP_TO_FLASH
#include "coredump_section.h"	// ?
#endif

extern "C" void log_dump_init(void);
extern "C" void crash_dump_init(void);

#ifdef FIRMWARE_REV
#define SYS_STORE_FW_VER(x) \
      if(fw_rev_##x) { \
        *fw_rev_##x = fw.softwareRevByte##x; \
      }

typedef struct
{
    uint8_t softwareRevByte0;
    uint8_t softwareRevByte1;
    uint8_t softwareRevByte2;
    uint8_t softwareRevByte3;
} FIRMWARE_REV_INFO_T;

static FIRMWARE_REV_INFO_T fwRevInfoInFlash __attribute((section(".fw_rev"))) = {0, 0, 1, 0};
FIRMWARE_REV_INFO_T fwRevInfoInRam;

extern "C" void system_get_info(uint8_t *fw_rev_0, uint8_t *fw_rev_1,
    uint8_t *fw_rev_2, uint8_t *fw_rev_3)
{
  FIRMWARE_REV_INFO_T fw = fwRevInfoInFlash;

  SYS_STORE_FW_VER(0);
  SYS_STORE_FW_VER(1);
  SYS_STORE_FW_VER(2);
  SYS_STORE_FW_VER(3);
}
#endif  /*END* FIRMWARE_REV */

#if defined(_AUTO_TEST_)
static uint8_t fwversion[4] = {0,0,1,0};

void system_get_fwversion(uint8_t *fw_rev_0, uint8_t *fw_rev_1,
    uint8_t *fw_rev_2, uint8_t *fw_rev_3)
{
    *fw_rev_0 = fwversion[0];
    *fw_rev_1 = fwversion[1];
    *fw_rev_2 = fwversion[2];
    *fw_rev_3 = fwversion[3];
}
#endif  /*END* defined(__AUTO_TEST) */

static osThreadId main_thread_tid = NULL;

extern "C" int system_shutdown(void)
{
    TRACE(0,"system_shutdown!!");
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x4);
    return 0;
}

int system_reset(void)
{
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x8);
    return 0;
}

int signal_send_to_main_thread(uint32_t signals)
{
    osSignalSet(main_thread_tid, signals);
    return 0;
}

int tgt_hardware_setup(void)
{
#ifdef __APP_USE_LED_INDICATE_IBRT_STATUS__
    for (uint8_t i=0;i<3;i++){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_ibrt_indication_pinmux_pwl[i], 1);
        if(i==0)
            hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_ibrt_indication_pinmux_pwl[i].pin, HAL_GPIO_DIR_OUT, 0);
        else
            hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_ibrt_indication_pinmux_pwl[i].pin, HAL_GPIO_DIR_OUT, 1);
    }
#endif	/* __APP_USE_LED_INDICATE_IBRT_STATUS__ */

	// Commented by csd
    //hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)cfg_hw_pinmux_pwl, sizeof(cfg_hw_pinmux_pwl)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    if (app_battery_ext_charger_indicator_cfg.pin != HAL_IOMUX_PIN_NUM){
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_battery_ext_charger_indicator_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_indicator_cfg.pin, HAL_GPIO_DIR_IN, 1);
    }
    return 0;
}

#if defined(ROM_UTILS_ON)
void rom_utils_init(void);
#endif  /*END* defined(ROM_UTILS_ON) */

#ifdef FPGA
uint32_t a2dp_audio_more_data(uint8_t *buf, uint32_t len);
uint32_t a2dp_audio_init(void);
extern "C" void app_audio_manager_open(void);
extern "C" void app_bt_init(void);
extern "C" uint32_t hal_iomux_init(const struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count);
void app_overlay_open(void);

extern "C" void BesbtInit(void);
extern "C" int app_os_init(void);
extern "C" uint32_t af_open(void);
extern "C" int list_init(void);
extern "C" void app_audio_open(void);


volatile uint32_t ddddd = 0;

#if defined(AAC_TEST)
#include "app_overlay.h"
int decode_aac_frame_test(unsigned char *pcm_buffer, unsigned int pcm_len);
#define AAC_TEST_PCM_BUFF_LEN (4096)
unsigned char aac_test_pcm_buff[AAC_TEST_PCM_BUFF_LEN];
#endif	/* defined(AAC_TEST) */

#endif	/* FPGA */

#if defined(_AUTO_TEST_)
extern int32_t at_Init(void);
#endif	/* defined(_AUTO_TEST_) */

#ifdef DEBUG_MODE_USB_DOWNLOAD
static void process_usb_download_mode(void)
{
    if (pmu_charger_get_status() == PMU_CHARGER_PLUGIN && hal_pwrkey_pressed()) {
        hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD);
        pmu_reboot();
    }
}
#endif	/* DEBUG_MODE_USB_DOWNLOAD */

#ifndef RAM_TOTAL_SIZE
#define RAM_TOTAL_SIZE RAM_SIZE
#endif	/* ! RAM_TOTAL_SIZE */

#if defined(__ARM_ARCH_8M_MAIN__)
/* Never overlap the memory region for armv8 MPU, or there will be data abort
 * When access overlaped memory
 */
static const  mpu_regions_t mpu_table[] = {
    {0,             0x800,              MPU_ATTR_EXEC,          MAIR_ATTR_INT_SRAM },
    {RAMX_BASE,     RAM_TOTAL_SIZE,     MPU_ATTR_READ_EXEC,     MAIR_ATTR_INT_SRAM },
};
#else	/* !defined(__ARM_ARCH_8M_MAIN__) */
/* Can overlap the memory region for armv7 MPU, the attribute of high number region
 * will override the low one
 */
static const  mpu_regions_t mpu_table[] = {
#ifdef CHIP_BEST1000
    {0,             0x400,          MPU_ATTR_NO_ACCESS },
#else	/* ! CHIP_BEST1000 */
    {0,             0x800,          MPU_ATTR_NO_ACCESS },
#endif	/*END* ! CHIP_BEST1000 */
    {RAMX_BASE,     RAM_TOTAL_SIZE,       MPU_ATTR_READ_EXEC },
    {RAM_BASE,      RAM_TOTAL_SIZE,       MPU_ATTR_READ_WRITE },
};
#endif /* !defined(__ARM_ARCH_8M_MAIN__) */
#ifdef BES_DEBUG_RAM
#include "cobuf.h"
extern void cobuf_print_statistic(void);
void debug_ram_info()
{
    cobuf_print_statistic();
}
#endif	/*END* BES_DEBUG_RAM */
int main(void)
{
    uint8_t sys_case = 0;
    int ret = 0;
#if !defined(BLE_ONLY_ENABLED)
    app_wdt_open(15);
#else
    app_wdt_open(30);
#endif

#ifdef __FACTORY_MODE_SUPPORT__
    uint32_t bootmode = hal_sw_bootmode_get();
#endif

#ifdef DEBUG_MODE_USB_DOWNLOAD
    process_usb_download_mode();
#endif

	/* Commited by Songde */
    //tgt_hardware_setup();

#if defined(ROM_UTILS_ON)
    rom_utils_init();
#endif

    main_thread_tid = osThreadGetId();
	MYTRACE(0, "main_thread id is %p", main_thread_tid);
    hwtimer_init();

    hal_dma_set_delay_func((HAL_DMA_DELAY_FUNC)osDelay);
    hal_audma_open();
    hal_gpdma_open();
    norflash_api_init();
#if defined(DUMP_LOG_ENABLE)
    log_dump_init();
#endif
#if (defined(DUMP_CRASH_ENABLE) || defined(TOTA_CRASH_DUMP_TOOL_ENABLE))
    crash_dump_init();
#endif
#ifdef CORE_DUMP_TO_FLASH
    coredump_to_flash_init();
#endif

#ifdef DEBUG
#if (DEBUG_PORT == 1)
    hal_iomux_set_uart0();
#ifdef __FACTORY_MODE_SUPPORT__
    if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif	/*END* __FACTORY_MODE_SUPPORT__ */
    {
        hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
    }
#endif	/*END* (DEBUG_PORT == 1) */

#if (DEBUG_PORT == 2)
#ifdef __FACTORY_MODE_SUPPORT__
    if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif	/*END* __FACTORY_MODE_SUPPORT__ */
    {
        hal_iomux_set_analog_i2c();
    }
    hal_iomux_set_uart1();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#endif	/*END* (DEBUG_PORT == 2) */

    hal_sleep_start_stats(10000, 10000);
    hal_trace_set_log_level(TR_LEVEL_DEBUG);
#endif	/*END* DEBUG */

    hal_iomux_ispi_access_init();

#ifndef FPGA
    uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
    hal_norflash_get_id(HAL_FLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
    TRACE(3,"FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
	hal_norflash_show_calib_result(HAL_FLASH_ID_0);
    ASSERT(hal_norflash_opened(HAL_FLASH_ID_0), "Failed to init flash: %d", hal_norflash_get_open_state(HAL_FLASH_ID_0));

    // Software will load the factory data and user data from the bottom TWO sectors from the flash,
    // the FLASH_SIZE defined is the common.mk must be equal or greater than the actual chip flash size,
    // otherwise the ota will load the wrong information
    uint32_t actualFlashSize = hal_norflash_get_flash_total_size(HAL_FLASH_ID_0);
    if (FLASH_SIZE > actualFlashSize)
    {
        TRACE_IMM(0,"Wrong FLASH_SIZE defined in target.mk!");
        TRACE_IMM(2,"FLASH_SIZE is defined as 0x%x while the actual chip flash size is 0x%x!", FLASH_SIZE, actualFlashSize);
        TRACE_IMM(1,"Please change the FLASH_SIZE in common.mk to 0x%x to enable the OTA feature.", actualFlashSize);
        ASSERT(false, " ");
    }
#endif /*END* !FPGA */

    pmu_open();

    analog_open();

    ret = mpu_setup(mpu_table, ARRAY_SIZE(mpu_table));
    if (ret) {
        TRACE(0,"Warning, MPU is not setup correctly!!!");
    }

    srand(hal_sys_timer_get());

#if defined(_AUTO_TEST_)
    at_Init();
#endif

#ifdef VOICE_DATAPATH
    app_audio_buffer_check();
#endif

#ifdef FPGA

    TRACE(0,"\n[best of best of best...]\n");
    TRACE(1,"\n[ps: w4 0x%x,2]", &ddddd);

    ddddd = 1;
    while (ddddd == 1);
    TRACE(0,"bt start");

    list_init();

    app_os_init();
    app_bt_init();
    a2dp_audio_init();

    af_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();

#if defined(AAC_TEST)
    app_overlay_select(APP_OVERLAY_A2DP_AAC);
    decode_aac_frame_test(aac_test_pcm_buff, AAC_TEST_PCM_BUFF_LEN);
#endif	/*END* defined(AAC_TEST) */

    SAFE_PROGRAM_STOP();

#else /* !defined(FPGA) */

#ifdef __FACTORY_MODE_SUPPORT__
    if (bootmode & HAL_SW_BOOTMODE_FACTORY){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_FACTORY);
        ret = app_factorymode_init(bootmode);

    }else if(bootmode & HAL_SW_BOOTMODE_CALIB){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CALIB);
        ret = app_factorymode_calib_only();
    }
#ifdef __USB_COMM__
    else if(bootmode & HAL_SW_BOOTMODE_CDC_COMM)
    {
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CDC_COMM);
        ret = app_factorymode_cdc_comm();
    }
#endif	/*END* __USB_COMM__ */
    else
#endif	/*END* __FACTORY_MODE_SUPPORT__ */
    {
#ifdef FIRMWARE_REV
        fwRevInfoInRam = fwRevInfoInFlash;
        TRACE(4,"The Firmware rev is %d.%d.%d.%d",
        fwRevInfoInRam.softwareRevByte0,
        fwRevInfoInRam.softwareRevByte1,
        fwRevInfoInRam.softwareRevByte2,
        fwRevInfoInRam.softwareRevByte3);
#endif	/*END* FIRMWARE_REV */
#ifdef CODEC_DAC_DC_AUTO_CALIB
        codec_dac_dc_auto_load();
#endif	/*END* CODEC_DAC_DC_AUTO_CALIB */
        ret = app_init();
    }
    if (!ret){
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("BT Init ok.");
#endif	/*END* defined(_AUTO_TEST_) */

#ifdef MYDEBUG
	MYTRACE(1, "List of macro in the %s\n", __FILE__);

#ifdef DEBUG
	MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "DEBUG");
#ifdef DEBUG_PORT
	MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "DEBUG_PORT = %d", DEBUG_PORT);
#endif		
#endif

#ifdef __FACTORY_MODE_SUPPORT__
	MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "__FACTORY_MODE_SUPPORT__");
#endif

#ifdef CHIP_HAS_USB
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "CHIP_HAS_USB");
#endif
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "CHIP_HAS_UART = %d", CHIP_HAS_UART);
#ifdef FORCE_TRACE_UART1
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "FORCE_TRACE_UART1");
#endif
#ifdef FORCE_TRACE_UART2
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "FORCE_TRACE_UART2");
#endif
#ifdef FAULT_DUMP
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "FAULT_DUMP");
#endif
#ifdef __ARM_ARCH_ISA_ARM
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "__ARM_ARCH_ISA_ARM");
#endif
#ifdef SLEEP_STATS_TRACE
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "SLEEP_STATS_TRACE");
#endif
#ifdef WAKEUP_SRC_STATS
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "WAKEUP_SRC_STATS");
#endif
#ifdef FLASH_CALIB_DEBUG
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "FLASH_CALIB_DEBUG");
#endif
#ifdef PROGRAMMER
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "PROGRAMMER");
#endif
#ifdef FLASH_SIZE
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "FLASH_SIZE = %d", FLASH_SIZE);
#endif
#ifdef FPGA
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "FPGA");
#endif
#ifdef PMU_INIT
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "PMU_INIT");
#endif
#ifdef VCODEC_OFF
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "VCODEC_OFF");
#endif
#ifdef DIG_DCDC_MODE
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "DIG_DCDC_MODE");
#endif
#ifdef ANA_DCDC_MODE
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "ANA_DCDC_MODE");
#endif
#ifdef __ARM_ARCH_8M_MAIN__
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "__ARM_ARCH_8M_MAIN__");
#endif
#ifdef BES_DEBUG_RAM
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "BES_DEBUG_RAM");
#endif
#ifdef BLE_ONLY_ENABLED
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "BLE_ONLY_ENABLED");
#endif
#ifdef DEBUG_MODE_USB_DOWNLOAD
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "DEBUG_MODE_USB_DOWNLOAD");
#endif
#ifdef PMU_HAS_LED_PIN
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "PMU_HAS_LED_PIN");
#endif
#ifdef CHIP_BEST2000
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "CHIP_BEST2000");
#endif
#ifdef PMU_LED_VIA_PWM
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "PMU_LED_VIA_PWM");
#endif
#ifdef GPIOAUX_BASE
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "GPIOAUX_BASE");
#endif
#ifdef GPIO1_BASE
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "GPIO1_BASE");
#endif
#ifdef GPIO2_BASE
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "GPIO2_BASE");
#endif
#ifdef CHIP_GPIO_VER
    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "CHIP_GPIO_VER = %d", CHIP_GPIO_VER);
#endif

    MYTRACE(TR_ATTR_NO_TS | TR_ATTR_NO_ID, "\nEnd of the list!");
#endif	/*END* MYDEBUG */
        while(1)
        {
            osEvent evt;
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
            osSignalClear (main_thread_tid, 0x0f);
#endif
            //wait any signal
            evt = osSignalWait(0x0, osWaitForever);

            //get role from signal value
            if(evt.status == osEventSignal)
            {
                if(evt.value.signals & 0x04)
                {
                    sys_case = 1;
                    break;
                }
                else if(evt.value.signals & 0x08)
                {
                    sys_case = 2;
                    break;
                }
            }else{
                sys_case = 1;
                break;
            }
         }/*END* while(1) */
    }/*END* if(!ret) */
#ifdef __WATCHER_DOG_RESET__
    system_shutdown_wdt_config(10);
#endif
    app_deinit(ret);
#ifdef BES_DEBUG_RAM    
    debug_ram_info();
#endif    
    TRACE(1,"byebye~~~ %d\n", sys_case);
    if ((sys_case == 1)||(sys_case == 0)){
        TRACE(0,"shutdown\n");
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("System shutdown.");
        osDelay(50);
#endif
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        pmu_shutdown();
    }else if (sys_case == 2){
        TRACE(0,"reset\n");
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("System reset.");
        osDelay(50);
#endif
        pmu_reboot();
    }

#endif /*END* !defiend(FPGA) */

    return 0;
}


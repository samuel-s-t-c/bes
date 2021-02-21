WDT

* 中断回调函数: `HAL_WDT_ID_0` ->`app_wdt_irq_handle`
* wdt_ping_timer定时器的周期是硬件wdt的1/4
  * `wdt_ping_period = seconds * 1000 / 4;`

PMU

* 中断处理函数: `pmu_wdt_irq_handle`

PWL

* 初始化的配置数组: `cfg_hw_pinmux_pwl`

TIMER

* 硬件定时器的中断处理函数: `hwtimer_handler`

DMA

* delay函数: `osDelay`

NORFLASH

* sleep hook: 
  * `HAL_SLEEP_HOOK_NORFLAHS_API`->`norflash_api_flush`
* mpu: 
  * `mpu_setup`,`mpu_table`配置表

TWS

* IBRT:
  * 宏`IBRT`,  宏`IBRT_SEARCH_UI`
  * 配置结构体: `userBle`, `userOta`, `userGfps`, `user_app_ai_t`, `user_ai_manager_t`, `user_app_dip_t`

THREAD

MAILBOX
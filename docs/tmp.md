TMP

* TRACE_CSD(0, "[%s]", __func__);
* TRACE_CSD(0, "[%s]*** leaving", __func__);

* * 

PMU

* 中断处理函数: `pmu_wdt_irq_handle`

PWL

* 初始化的配置数组: `cfg_hw_pinmux_pwl`

TIMER

* WDT
  * 中断回调函数: `HAL_WDT_ID_0` ->`app_wdt_irq_handle`
  * wdt_ping_timer定时器的周期是硬件wdt的1/4
    * `wdt_ping_period = seconds * 1000 / 4;`

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

BT

* user与对应的hook: `bt_hook_handler`数组
  * `BESBT_HOOK_USER_1`->`app_bt_mail_poll`

THREAD

* `app_thread`
  * 模块与对应的处理函数	
  * `APP_MODUAL_BATTERY`->`app_battery_handle_process`
  * `APP_MODUAL_KEY`->`app_key_handle_process`
  * `APP_MODUAL_BT`->`app_bt_handle_process`
  * `APP_MODUAL_CUSTOM_FUNCTION`->`app_custom_function_process`

MAILBOX

* `app_mailbox`: 邮件类型`APP_MESSAGE_BLOCK`
* `app_bt_mailbox`: 邮件类型`APP_BT_MAIL`

TIMER(os)

* `APP_BATTERY`:

  * 周期性
  * 处理函数`app_battery_timer_handler`

* `APP_BATTERY_PLUGINOUT_DEBOUNCE`

  * 一次性
  * 处理函数`app_battery_pluginout_debounce_handler(&app_battery_pluginout_debounce_ctx)`

  
TRACE_CSD(1, "[%s]+++", __func__);

TRACE_CSD(1, "[%s]---", __func__);

TRACE_CSD(1, "{%s}", __func__);

TRACE_CSD(0|TR_ATTR_NO_ID|TR_ATTR_NO_TS, "<>");

TRACE_CSD(0,"{} <>:()");

TRACE_CSD(0, "{osPoolCreate}-->(app_key_handle_mempool)");

DEGUB_BES_BUG

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

KEY

* `app_ibrt_ui_test_key_cfg`
* `key_event_process`

NORFLASH

* sleep hook: 
  * `HAL_SLEEP_HOOK_NORFLAHS_API`->`norflash_api_flush`
  * `HAL_SLEEP_HOOK_USER_NVRECORD`->`nv_record_flash_flush_in_sleep`
* mpu: 
  * `mpu_setup`,`mpu_table`配置表

BT

* `app_bt_global_handle`

* `app_bt_global_handle_hook_handler`
  * `APP_BT_GOLBAL_HANDLE_HOOK_USER_0`->`app_bt_manager_ibrt_role_process`
* `app_bt_handle_process`

* IBRT:
  * 宏`IBRT`,  宏`IBRT_SEARCH_UI`
  * 配置结构体: `userBle`, `userOta`, `userGfps`, `user_app_ai_t`, `user_ai_manager_t`, `user_app_dip_t`
  * cmdhandle
    * `TWS_CMD_IBRT`->`app_ibrt_cmd_table_get`
    * `TWS_CMD_CUSTOMER`->`app_ibrt_customif_cmd_table_get`
    * `TWS_CMD_IBRT_OTA`->`app_ibrt_ota_tws_cmd_table_get`
    * `TWS_CMD_OTA`->`app_ibrt_ota_cmd_table_get`
  * ui,  `app_ibrt_customif_ui_start`
    * `app_ibrt_customif_ui_global_handler_ind`
    * `app_ibrt_customif_ui_vender_event_handler_ind`
    * `app_ibrt_customif_ui_global_event_update`
    * `app_ibrt_customif_mobile_connected_ind`
    * `app_ibrt_customif_ibrt_connected_ind`
    * `app_ibrt_customif_tws_connected_ind`
    * `app_ibrt_customif_profile_state_change_ind`
    * `app_ibrt_customif_connect_mobile_needed_ind`
    * `app_ibrt_customif_ui_pairing_set`
    * `app_ibrt_customif_ui_pairing_clear`

THREAD

* `app_thread`
  * 模块与对应的处理函数	
  * `APP_MODUAL_BATTERY`->`app_battery_handle_process`
  * `APP_MODUAL_KEY`->`app_key_handle_process`
  * `APP_MODUAL_BT`->`app_bt_handle_process`
  * `APP_MODUAL_CUSTOM_FUNCTION`->`app_custom_function_process`
  * `APP_MODUAL_AUDIO`->`app_audio_handle_process`
  * `APP_MODUAL_AUDIO_MANAGE`->`app_audio_manager_handle_process`
* `af_thread`: audio flinger
  * `af_thread_stream_handler`
* `BesbtThread`, `besmain`: bes bt
  * user与对应的hook: `bt_hook_handler`数组
    * `BESBT_HOOK_USER_1`->`app_bt_mail_poll`
    * `BESBT_HOOK_USER_2`->`bt_key_handle`
    * `BESBT_HOOK_USER_3`->`app_hfp_battery_report_proc`
* `app_ibrt_peripheral_thread`
* `tws_ctrl_thread`: 

MAILBOX

* `app_mailbox`: 邮件类型`APP_MESSAGE_BLOCK`
* `app_bt_mailbox`: 邮件类型`APP_BT_MAIL`
* `app_ibrt_peripheral_mailbox`: 邮件类型`TWS_PD_MSG_BLOCK`

TIMER(os)

* `APP_BATTERY`:

  * 周期性
  * 处理函数`app_battery_timer_handler`

* `APP_BATTERY_PLUGINOUT_DEBOUNCE`

  * 一次性
  * 处理函数`app_battery_pluginout_debounce_handler(&app_battery_pluginout_debounce_ctx)`


* `APP_BT_ACCESSMODE_TIMER`
  * 一次性
  * 处理函数`app_bt_accessmode_timehandler(&app_bt_accessmode_timer_argument)`
* `BT_SCO_RECOV_TIMER`
  * 一次性
  * 处理函数`bt_sco_recov_timer_handler(NULL)`
* `APP_BOX_HANDLE`

  * 一次性
  * 处理函数`app_box_handle_timehandler(&box_event)`
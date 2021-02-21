tws

* `void app_tws_if_init(void)`
* `void app_ibrt_init(void)`, 在bt_thread中运行
* `void app_bt_manager_ibrt_role_proces(const btif_event_t *Event)`
* `int app_ibrt_customif_ui_start(void)`: 某些ibrt选项的开关
  * 如, 与手机断连是否进行配对
* `typedef uint8_t ibrt_event_type`

* ibrt配对超时时间: `IBRT_MAX_SEARCH_TIME`乘以`MAX_TWS_INQUIRY_TIMES`

bt

* `void BesbtInit(void)`
* `void app_bt_global_handle_init(void)`
* `void app_bt_global_handle(const btif_event_t *Event)`
* `MAX_BT_PAIRED_DEVICE_COUNT`: BT已配对设备信息的最大数量

* 开机时按键配置: 
  * `static void app_poweron_key_init(void)`
  * `const APP_KEY_HANDLE pwron_key_handle_cfg[]`
* `void app_key_init(void)`
  * `void app_ibrt_ui_test_key_init(void)`
    * `const APP_KEY_HANDLE app_ibrt_ui_test_key_cfg[]`
    * `app_ibrt_ui_test_key`
    * `app_ibrt_ui_test_key_io_event`

定时器

* `APP_10_SECOND_TIMER_STRUCT app_10_second_array[]`: 软定时器的配置

状态

* `app_status`

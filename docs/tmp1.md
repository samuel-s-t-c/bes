# 开机流程



# 蓝牙

`<APP_MODUAL_BT>:(app_bt_handle_process)`: app_thread中用于处理bt相关的模块

`g_bt_access_mode`

`<BESBT_HOOK_USER_1>:(app_bt_mail_poll)`

`app_bt_profile_connect_openreconnecting`

`bt_link_active_mode_bits`: 管理和记录active mode

`<APP_MODUAL_CUSTOM_FUNCTION>:(app_custom_function_process)`: app_thread中用于处理自定义函数的模块

`app_bt_handler.callback = app_bt_global_handle`

`app_bt_global_handle_hook_set(APP_BT_GOLBAL_HANDLE_HOOK_USER_0,app_bt_manager_ibrt_role_process);`

# 按键

说明

1. `app_key_config`变量: 一个列表, 其节点保存按键事件及其对应的handler
2. `app_ibrt_ui_test_key_cfg`配置数组: 按键配置; 用户在此进行配置
   * 电源键的handler: `app_ibrt_ui_test_key`
   * Fn1和Fn2的handler: `app_ibrt_ui_test_key_io_event`

按键初始化函数

* `app_ibrt_ui_test_key_init()`

按键配置清除函数

* `app_key_handle_clear()`

按键注册函数

* `app_key_handle_registration`

# Flash

`app_ibrt_nvrecord_delete_all_mobile_record()`


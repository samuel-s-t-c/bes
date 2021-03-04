```c
if (app_tws_ibrt_mobile_link_connected())
    {
        TRACE(1,"checker: IBRT_MASTER activeCons:%d", btif_me_get_activeCons());
        remDev = p_ibrt_ctrl->p_tws_remote_dev;
        if (remDev != NULL)
        {
            cmgrHandler = btif_cmgr_get_acl_handler(remDev);
            if (cmgrHandler)
            {
                app_bt_ibrt_profile_checker("tws peers", remDev, cmgrHandler, NULL, NULL);
            }
            else
            {
                TRACE(0,"checker: cmgrhandler not handle p_tws_remote_dev!");
            }
        }
        else
        {
            TRACE(0,"checker: tws_remote_dev is NULL!");
        }

        remDev = p_ibrt_ctrl->p_mobile_remote_dev;
        if (remDev != NULL)
        {
            cmgrHandler = btif_cmgr_get_acl_handler(remDev);
            if (cmgrHandler)
            {
                app_bt_ibrt_profile_checker("master mobile", remDev, cmgrHandler, app_bt_device.a2dp_connected_stream[0],
                                            app_bt_device.hf_conn_flag[0] ? app_bt_device.hf_channel[0] : NULL);
            }
            else
            {
                TRACE(0,"checker: cmgrhandler not handle mobile_remote_dev");
            }
        }
        else
        {
            TRACE(0,"checker: mobile_remote_dev is NULL!");
        }
    }
    else if (app_tws_ibrt_slave_ibrt_link_connected())
    {
        TRACE(1,"checker: IBRT_SLAVE activeCons:%d", btif_me_get_activeCons());
        remDev = p_ibrt_ctrl->p_tws_remote_dev;
        if (remDev != NULL)
        {
            cmgrHandler = btif_cmgr_get_acl_handler(remDev);
            if (cmgrHandler)
            {
                app_bt_ibrt_profile_checker("tws peers", remDev, cmgrHandler, NULL, NULL);
            }
            else
            {
                TRACE(0,"checker: cmgrhandler not handle p_tws_remote_dev!");
            }
        }
        else
        {
            TRACE(0,"checker: tws_remote_dev is NULL!");
        }
        if (app_ibrt_ui_is_profile_exchanged())
        {
            app_bt_ibrt_profile_checker("ibrt mobile", NULL, NULL, app_bt_device.a2dp_connected_stream[0],
                                        app_bt_device.hf_conn_flag[0] ? app_bt_device.hf_channel[0] : NULL);
        }
    }
    else
    {
        TRACE(1,"checker: IBRT_UNKNOW activeCons:%d", btif_me_get_activeCons());
    }
```
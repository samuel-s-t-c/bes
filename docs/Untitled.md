```
BTIF_BTEVENT_LINK_CONNECT_CNF:// An outgoing ACL connection is up
        //fall through
        case BTIF_BTEVENT_LINK_CONNECT_IND:
```

# 事件

`OUT_BOX`, `IDLE`, `PUT_IN_EVENT`

1. `app_ibrt_ui_put_in_box_event_handler`: `IDLE`->`IDLE`

`IN_BOX_OPEN`, `IDLE`, `CLOSE_BOX_EVENT`

1. `app_ibrt_ui_free_link_handler`:`IDLE`->`IDLE_WAIT`

2. `app_ibrt_ui_event_handler`: clear pairing mode

   * j_scan=CLOSE_BOX, l_type=NO_LINK_TYPE, mode=0x0, p_mode=0

   * set access mode=0

3. `app_ibrt_ui_free_link_handler`:`IDLE_WAIT`->`IDLE`

`IN_BOX_CLOSED`, `IDLE`,`FETCH_OUT_EVENT`

1. `app_ibrt_ui_common_sm`:`IDLE`->`TWS_CONNECTION`
   *   j_scan=CONNECT_TWS, l_type=TWS_LINK, mode=0x2, p_mod=0
   * set access mode=2

## 详细日志

`OUT_BOX`, `IDLE`, `PUT_IN_EVENT`

```
ibrt_ui_log:custom event entry enter=PUT_IN_EVENT
ibrt_ui_log: [IO] app_ibrt_ui_event_entry,event=PUT_IN_EVENT
ibrt_ui_log:current box state=OUT_BOX
ibrt_ui_log:current running event=PUT_IN_EVENT
ibrt_ui_log:current box state=IN_BOX_OPEN

ibrt_ui_log:active event=PUT_IN_EVENT, consume_ms=1
ibrt_ui_log:entry=app_ibrt_ui_put_in_box_event_handler, action=IDLE, status=0x0
ibrt_ui_log:enter=IDLE, exit=IDLE
```

`IN_BOX_OPEN`, `IDLE`, `CLOSE_BOX_EVENT`

```
ibrt_ui_log:custom event entry enter=CLOSE_BOX_EVENT
ibrt_ui_log: [IO] app_ibrt_ui_event_entry,event=CLOSE_BOX_EVENT
ibrt_ui_log:current box state=IN_BOX_OPEN
ibrt_ui_log:current running event=CLOSE_BOX_EVENT
ibrt_ui_log:current box state=IN_BOX_CLOSED

ibrt_ui_log:entry=app_ibrt_ui_free_link_handler, action=IDLE, status=0x0
ibrt_ui_log:enter=IDLE, exit=IDLE_WAIT

ibrt_ui_log:event_handler, action=IDLE_WAIT, state=IDLE_WAIT, status=0x0
ibrt_ui_log: CLOSE_BOX clear pairing mode
j_scan=CLOSE_BOX, l_type=NO_LINK_TYPE, mode=0x0, p_mode=0
ibrt_ui_log:set_access_mode=0, ca=0xc042ce5
write_scan_enable=0
ibrt_ui_log:btif_me_set_accessible_mode status:02

ibrt_ui_log:active event=CLOSE_BOX_EVENT, consume_ms=415
ibrt_ui_log:entry=app_ibrt_ui_free_link_handler, action=IDLE_WAIT, status=0x0
ibrt_ui_log:enter=IDLE_WAIT, exit=IDLE
```

`IN_BOX_CLOSED`, `IDLE`,`FETCH_OUT_EVENT`

```
ibrt_ui_log:custom event entry enter=FETCH_OUT_EVENT
ibrt_ui_log: [IO] app_ibrt_ui_event_entry,event=FETCH_OUT_EVENT
ibrt_ui_log:current box state=IN_BOX_CLOSED
ibrt_ui_log:current running event=FETCH_OUT_EVENT
ibrt_ui_log:current box state=OUT_BOX

ibrt_ui_log: super_state = IDLE
ibrt_ui_log:entry=app_ibrt_ui_common_sm, action=IDLE, status=0x0
ibrt_ui_log:enter=IDLE, exit=TWS_CONNECTION
j_scan=CONNECT_TWS, l_type=TWS_LINK, mode=0x2, p_mod=0
```


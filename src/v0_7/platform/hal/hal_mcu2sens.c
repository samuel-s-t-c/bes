/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "plat_addr_map.h"

#ifdef SENS_CMU_BASE

#include "cmsis_nvic.h"
#include "hal_location.h"
#include "hal_mcu2sens.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include CHIP_SPECIFIC_HDR(reg_cmu)
#include CHIP_SPECIFIC_HDR(reg_senscmu)

#define MCU2SENS_VERBOSE_TRACE              0 // ((1 << 0) | (1 << 1))

#define MAX_SEND_RECORD_COUNT               3

#define HAL_SYS_WAKE_LOCK_USER_MCU2SENS     HAL_SYS_WAKE_LOCK_USER_5

STATIC_ASSERT(sizeof(struct HAL_MCU2SENS_MSG_T *) * HAL_MCU2SENS_ID_QTY +
    sizeof(const struct HAL_MCU2SENS_MSG_T **) <= SENS_MAILBOX_SIZE, "SENS_MAILBOX_SIZE too small");

#define SENS_RX_MSG_PTR_LOC                 (SENS_MAILBOX_BASE + sizeof(struct HAL_MCU2SENS_MSG_T *) * HAL_MCU2SENS_ID_QTY)

enum HAL_MCU2SENS_IRQ_TYPE_T {
    HAL_MCU2SENS_IRQ_SEND_IND,
    HAL_MCU2SENS_IRQ_RECV_DONE,

    HAL_MCU2SENS_IRQ_TYPE_QTY
};

struct HAL_MCU2SENS_MSG_T {
    struct HAL_MCU2SENS_MSG_T *next;        // pointer to next element in the list
    unsigned int len;                       // message data length in bytes
    const void *data;                       // pointer to the message data
};

struct HAL_MCU2SENS_SEND_RECORD_T {
    struct HAL_MCU2SENS_MSG_T msg;
    bool in_use;
};

static const IRQn_Type rx_irq_id[HAL_MCU2SENS_ID_QTY] = {
    SENS2MCU_DATA_IRQn,
    SENS2MCU_DATA1_IRQn,
};

static const IRQn_Type tx_irq_id[HAL_MCU2SENS_ID_QTY] = {
    MCU2SENS_DONE_IRQn,
    MCU2SENS_DONE1_IRQn,
};

#ifdef CHIP_SUBSYS_SENS
static const struct HAL_MCU2SENS_MSG_T ** recv_msg_list_p;
#else
static const struct HAL_MCU2SENS_MSG_T ** const recv_msg_list_p =
    (const struct HAL_MCU2SENS_MSG_T **)SENS_MAILBOX_BASE;
#endif

#ifdef CHIP_SUBSYS_SENS
static struct HAL_MCU2SENS_MSG_T ** const send_msg_list_p =
    (struct HAL_MCU2SENS_MSG_T **)SENS_MAILBOX_BASE;
#else
static struct HAL_MCU2SENS_MSG_T * send_msg_list_p[HAL_MCU2SENS_ID_QTY];
#endif

static struct HAL_MCU2SENS_MSG_T * send_pending_list_p[HAL_MCU2SENS_ID_QTY];

static struct HAL_MCU2SENS_MSG_T recv_pending_head[HAL_MCU2SENS_ID_QTY];

static struct HAL_MCU2SENS_SEND_RECORD_T send_msgs[HAL_MCU2SENS_ID_QTY][MAX_SEND_RECORD_COUNT];

static HAL_MCU2SENS_RX_IRQ_HANDLER rx_irq_handler[HAL_MCU2SENS_ID_QTY];
static HAL_MCU2SENS_TX_IRQ_HANDLER tx_irq_handler[HAL_MCU2SENS_ID_QTY];

static bool chan_opened[HAL_MCU2SENS_ID_QTY] = { false, false, };

static bool need_flow_ctrl[HAL_MCU2SENS_ID_QTY] = { false, false, };

static bool chan_busy[HAL_MCU2SENS_ID_QTY] = { false, false, };

static bool busy_now = false;

static struct CMU_T * const cmu = (struct CMU_T *)CMU_BASE;
static struct SENSCMU_T * const senscmu = (struct SENSCMU_T *)SENS_CMU_BASE;

static void hal_mcu2sens_busy(enum HAL_MCU2SENS_ID_T id, bool busy)
{
    int i;
    bool new_state;

    if (chan_busy[id] == busy) {
        return;
    }

    chan_busy[id] = busy;

    if (busy_now == busy) {
        return;
    }

    if (busy) {
        hal_sys_wake_lock(HAL_SYS_WAKE_LOCK_USER_MCU2SENS);
        busy_now = true;
    } else {
        new_state = false;
        for (i = 0; i < HAL_MCU2SENS_ID_QTY; i++) {
            if (chan_busy[i]) {
                new_state = true;
                break;
            }
        }
        if (!new_state) {
            hal_sys_wake_unlock(HAL_SYS_WAKE_LOCK_USER_MCU2SENS);
            busy_now = false;
        }
    }
}

static int hal_mcu2sens_peer_irq_set(enum HAL_MCU2SENS_ID_T id, enum HAL_MCU2SENS_IRQ_TYPE_T type)
{
    uint32_t value;

    if (type != HAL_MCU2SENS_IRQ_SEND_IND) {
        return 1;
    }

#ifdef CHIP_SUBSYS_SENS
    if (id == HAL_MCU2SENS_ID_0) {
        value = SENS_CMU_SENS2MCU_DATA_IND_SET;
    } else {
        value = SENS_CMU_SENS2MCU_DATA1_IND_SET;
    }

    senscmu->SENS2MCU_IRQ_SET = value;
#else
    if (id == HAL_MCU2SENS_ID_0) {
        value = CMU_MCU2SENS_DATA_IND_SET;
    } else {
        value = CMU_MCU2SENS_DATA1_IND_SET;
    }

    cmu->MCU2SENS_IRQ_SET = value;
#endif

    return 0;
}

static int hal_mcu2sens_local_irq_clear(enum HAL_MCU2SENS_ID_T id, enum HAL_MCU2SENS_IRQ_TYPE_T type)
{
    uint32_t value;
    uint32_t lock;

#ifdef CHIP_SUBSYS_SENS
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_MCU2SENS_DATA_IND_CLR;
        } else {
            value = SENS_CMU_MCU2SENS_DATA_DONE_CLR;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_MCU2SENS_DATA1_IND_CLR;
        } else {
            value = SENS_CMU_MCU2SENS_DATA1_DONE_CLR;
        }
    }

    if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
        lock = int_lock_global();
        cmu->MCU2SENS_IRQ_CLR = value;
        // Flush the clear operation immediately
        cmu->MCU2SENS_IRQ_CLR;
        int_unlock_global(lock);
    } else {
        senscmu->SENS2MCU_IRQ_CLR = value;
    }
#else
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_SENS2MCU_DATA_IND_CLR;
        } else {
            value = CMU_SENS2MCU_DATA_DONE_CLR;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_SENS2MCU_DATA1_IND_CLR;
        } else {
            value = CMU_SENS2MCU_DATA1_DONE_CLR;
        }
    }

    if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
        lock = int_lock_global();
        senscmu->SENS2MCU_IRQ_CLR = value;
        // Flush the clear operation immediately
        senscmu->SENS2MCU_IRQ_CLR;
        int_unlock_global(lock);
    } else {
        cmu->MCU2SENS_IRQ_CLR = value;
    }
#endif

    return 0;
}

static int hal_mcu2sens_local_irq_mask_set(enum HAL_MCU2SENS_ID_T id, enum HAL_MCU2SENS_IRQ_TYPE_T type)
{
    uint32_t value;

#ifdef CHIP_SUBSYS_SENS
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_MCU2SENS_DATA_MSK_SET;
        } else {
            value = SENS_CMU_SENS2MCU_DATA_MSK_SET;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_MCU2SENS_DATA1_MSK_SET;
        } else {
            value = SENS_CMU_SENS2MCU_DATA1_MSK_SET;
        }
    }

    senscmu->SENS2MCU_IRQ_SET = value;
#else
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_SENS2MCU_DATA_MSK_SET;
        } else {
            value = CMU_MCU2SENS_DATA_MSK_SET;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_SENS2MCU_DATA1_MSK_SET;
        } else {
            value = CMU_MCU2SENS_DATA1_MSK_SET;
        }
    }

    cmu->MCU2SENS_IRQ_SET = value;
#endif

    return 0;
}

static int hal_mcu2sens_local_irq_mask_clear(enum HAL_MCU2SENS_ID_T id, enum HAL_MCU2SENS_IRQ_TYPE_T type)
{
    uint32_t value;

#ifdef CHIP_SUBSYS_SENS
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_MCU2SENS_DATA_MSK_CLR;
        } else {
            value = SENS_CMU_SENS2MCU_DATA_MSK_CLR;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_MCU2SENS_DATA1_MSK_CLR;
        } else {
            value = SENS_CMU_SENS2MCU_DATA1_MSK_CLR;
        }
    }

    senscmu->SENS2MCU_IRQ_CLR = value;
#else
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_SENS2MCU_DATA_MSK_CLR;
        } else {
            value = CMU_MCU2SENS_DATA_MSK_CLR;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_SENS2MCU_DATA1_MSK_CLR;
        } else {
            value = CMU_MCU2SENS_DATA1_MSK_CLR;
        }
    }

    cmu->MCU2SENS_IRQ_CLR = value;
#endif

    return 0;
}

static int hal_mcu2sens_irq_active(enum HAL_MCU2SENS_ID_T id, enum HAL_MCU2SENS_IRQ_TYPE_T type)
{
    uint32_t value;

#ifdef CHIP_SUBSYS_SENS
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_MCU2SENS_DATA_INTR_MSK;
        } else {
            value = SENS_CMU_SENS2MCU_DATA_INTR_MSK;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = SENS_CMU_MCU2SENS_DATA1_INTR_MSK;
        } else {
            value = SENS_CMU_SENS2MCU_DATA1_INTR_MSK;
        }
    }

    return !!(senscmu->SENS2MCU_IRQ_SET & value);
#else
    if (id == HAL_MCU2SENS_ID_0) {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_SENS2MCU_DATA_INTR_MSK;
        } else {
            value = CMU_MCU2SENS_DATA_INTR_MSK;
        }
    } else {
        if (type == HAL_MCU2SENS_IRQ_SEND_IND) {
            value = CMU_SENS2MCU_DATA1_INTR_MSK;
        } else {
            value = CMU_MCU2SENS_DATA1_INTR_MSK;
        }
    }

    return !!(cmu->MCU2SENS_IRQ_SET & value);
#endif
}

static void hal_mcu2sens_rx_irq_handler(void)
{
    int id;
    const struct HAL_MCU2SENS_MSG_T *msg_ptr;
    unsigned int processed;

    for (id = HAL_MCU2SENS_ID_0; id < HAL_MCU2SENS_ID_QTY; id++) {
        if (hal_mcu2sens_irq_active(id, HAL_MCU2SENS_IRQ_SEND_IND)) {
            hal_mcu2sens_local_irq_mask_clear(id, HAL_MCU2SENS_IRQ_SEND_IND);

            if (recv_pending_head[id].data) {
                // Previous unprocessed message
                msg_ptr = &recv_pending_head[id];
            } else {
                // New message
                msg_ptr = recv_msg_list_p[id];
            }
            while (msg_ptr) {
#if (MCU2SENS_VERBOSE_TRACE & (1 << 0))
                TR_INFO(0, "MCU2SENS-RX:%d: msg=%p len=%u data=%p content=0x%08X 0x%08X",
                    id, msg_ptr, msg_ptr->len, msg_ptr->data, msg_ptr->data ? *(uint32_t *)msg_ptr->data : 0,
                    msg_ptr->data ? *(uint32_t *)(msg_ptr->data + 4) : 0);
#endif
                if (rx_irq_handler[id]) {
                    processed = rx_irq_handler[id](msg_ptr->data, msg_ptr->len);
                    // Check if flow control needed
                    if (processed < msg_ptr->len) {
                        recv_pending_head[id].next = msg_ptr->next;
                        recv_pending_head[id].len = msg_ptr->len - processed;
                        recv_pending_head[id].data = msg_ptr->data + processed;
                        break;
                    }
                } else {
                    // Error
                    ASSERT(false, "MCU2SENS-RX: Handler missing");
                    break;
                }
                msg_ptr = msg_ptr->next;
            }

            if (msg_ptr == NULL) {
                if (!need_flow_ctrl[id]){
                    hal_mcu2sens_rx_done(id);
                }
                recv_pending_head[id].data = NULL;
            }
        }
    }
}

static void hal_mcu2sens_tx_irq_handler_by_id(enum HAL_MCU2SENS_ID_T id)
{
    struct HAL_MCU2SENS_MSG_T *msg_ptr;

    if (hal_mcu2sens_irq_active(id, HAL_MCU2SENS_IRQ_RECV_DONE)) {
        hal_mcu2sens_local_irq_clear(id, HAL_MCU2SENS_IRQ_RECV_DONE);

        msg_ptr = send_msg_list_p[id];
        while (msg_ptr) {
            if (tx_irq_handler[id]) {
                tx_irq_handler[id](msg_ptr->data, msg_ptr->len);
            };
            CONTAINER_OF(msg_ptr, struct HAL_MCU2SENS_SEND_RECORD_T, msg)->in_use = false;
            msg_ptr = msg_ptr->next;
        }

        if (send_pending_list_p[id]) {
            send_msg_list_p[id] = send_pending_list_p[id];
            send_pending_list_p[id] = NULL;
            hal_mcu2sens_peer_irq_set(id, HAL_MCU2SENS_IRQ_SEND_IND);
        } else {
            send_msg_list_p[id] = NULL;
            // Allow sleep
            hal_mcu2sens_busy(id, false);
        }
    }
}

static void hal_mcu2sens_tx_irq_handler(void)
{
    int id;

    for (id = HAL_MCU2SENS_ID_0; id < HAL_MCU2SENS_ID_QTY; id++) {
        hal_mcu2sens_tx_irq_handler_by_id(id);
    }
}

#ifdef CORE_SLEEP_POWER_DOWN
static int hal_mcu2sens_pm_notif_handler(enum HAL_PM_STATE_T state)
{
    if (state == HAL_PM_STATE_POWER_DOWN_WAKEUP) {
        for (int i = 0; i < HAL_MCU2SENS_ID_QTY; i++) {
            if (chan_opened[i]) {
                hal_mcu2sens_local_irq_mask_set(i, HAL_MCU2SENS_IRQ_SEND_IND);
                hal_mcu2sens_local_irq_mask_set(i, HAL_MCU2SENS_IRQ_RECV_DONE);
            }
        }
    }

    return 0;
}
#endif

int hal_mcu2sens_open(enum HAL_MCU2SENS_ID_T id, HAL_MCU2SENS_RX_IRQ_HANDLER rxhandler, HAL_MCU2SENS_TX_IRQ_HANDLER txhandler, bool rx_flowctrl)
{
    int i;

    if (id >= HAL_MCU2SENS_ID_QTY) {
        return 1;
    }

    if (!chan_opened[id]) {
        // Stop IRQs by default
        NVIC_DisableIRQ(rx_irq_id[id]);
        NVIC_DisableIRQ(tx_irq_id[id]);

        NVIC_SetVector(rx_irq_id[id], (uint32_t)hal_mcu2sens_rx_irq_handler);
        NVIC_SetPriority(rx_irq_id[id], IRQ_PRIORITY_NORMAL);

        NVIC_SetVector(tx_irq_id[id], (uint32_t)hal_mcu2sens_tx_irq_handler);
        NVIC_SetPriority(tx_irq_id[id], IRQ_PRIORITY_NORMAL);

        hal_mcu2sens_local_irq_mask_set(id, HAL_MCU2SENS_IRQ_SEND_IND);
        hal_mcu2sens_local_irq_mask_set(id, HAL_MCU2SENS_IRQ_RECV_DONE);

#ifdef CHIP_SUBSYS_SENS
        if (id == HAL_MCU2SENS_ID_0) {
            senscmu->SENS2MCU_IRQ_CLR = SENS_CMU_MCU2SENS_DATA_DONE_CLR | SENS_CMU_SENS2MCU_DATA_IND_CLR;
        } else {
            senscmu->SENS2MCU_IRQ_CLR = SENS_CMU_MCU2SENS_DATA1_DONE_CLR | SENS_CMU_SENS2MCU_DATA1_IND_CLR;
        }
        recv_msg_list_p = (const struct HAL_MCU2SENS_MSG_T **)*(uint32_t *)SENS_RX_MSG_PTR_LOC;
#else
        if (id == HAL_MCU2SENS_ID_0) {
            cmu->MCU2SENS_IRQ_CLR = CMU_SENS2MCU_DATA_DONE_CLR | CMU_MCU2SENS_DATA_IND_CLR;
        } else {
            cmu->MCU2SENS_IRQ_CLR = CMU_SENS2MCU_DATA1_DONE_CLR | CMU_MCU2SENS_DATA1_IND_CLR;
        }
        *(uint32_t *)SENS_RX_MSG_PTR_LOC = (uint32_t)&send_msg_list_p[0];
#endif

        send_msg_list_p[id] = NULL;
        send_pending_list_p[id] = NULL;
        recv_pending_head[id].data = NULL;
        for (i = 0; i < MAX_SEND_RECORD_COUNT; i++) {
            send_msgs[id][i].in_use = false;
        }
        need_flow_ctrl[id] = rx_flowctrl;

        TR_INFO(0, "%s:%d: rxPtr=%p txPtr=%p flowCtrl=%d", __func__, id, &recv_msg_list_p[id], &send_msg_list_p[id], need_flow_ctrl[id]);

        // Enable TX IRQ
        NVIC_ClearPendingIRQ(tx_irq_id[id]);
        NVIC_EnableIRQ(tx_irq_id[id]);
    } else {
        ASSERT(need_flow_ctrl[id] == rx_flowctrl, "MCU2SENS-OPEN: rx_flowctrl=%d (should be %d)", rx_flowctrl, need_flow_ctrl[id]);
        return 3;
    }

#ifdef CORE_SLEEP_POWER_DOWN
    for (i = 0; i < HAL_MCU2SENS_ID_QTY; i++) {
        if (chan_opened[i]) {
            break;
        }
    }
    if (i >= HAL_MCU2SENS_ID_QTY) {
        hal_pm_notif_register(HAL_PM_USER_HAL, hal_mcu2sens_pm_notif_handler);
    }
#endif

    chan_opened[id] = true;

    rx_irq_handler[id] = rxhandler;
    tx_irq_handler[id] = txhandler;

    return 0;
}

int hal_mcu2sens_close(enum HAL_MCU2SENS_ID_T id)
{
    if (id >= HAL_MCU2SENS_ID_QTY) {
        return 1;
    }

    chan_opened[id] = false;
    rx_irq_handler[id] = NULL;
    tx_irq_handler[id] = NULL;

    // Stop IRQs by default
    NVIC_DisableIRQ(rx_irq_id[id]);
    NVIC_DisableIRQ(tx_irq_id[id]);

    send_msg_list_p[id] = NULL;
    send_pending_list_p[id] = NULL;
    recv_pending_head[id].data = NULL;
    need_flow_ctrl[id] = false;

#ifdef CORE_SLEEP_POWER_DOWN
    int i;

    for (i = 0; i < HAL_MCU2SENS_ID_QTY; i++) {
        if (chan_opened[i]) {
            break;
        }
    }
    if (i >= HAL_MCU2SENS_ID_QTY) {
        hal_pm_notif_deregister(HAL_PM_USER_HAL, hal_mcu2sens_pm_notif_handler);
    }
#endif

    return 0;
}

int hal_mcu2sens_start_recv(enum HAL_MCU2SENS_ID_T id)
{
    if (id >= HAL_MCU2SENS_ID_QTY) {
        return 1;
    }

    NVIC_ClearPendingIRQ(rx_irq_id[id]);
    NVIC_EnableIRQ(rx_irq_id[id]);

    // Check if there is any previous unprocessed message
    if (recv_pending_head[id].data) {
        hal_mcu2sens_local_irq_mask_set(id, HAL_MCU2SENS_IRQ_SEND_IND);
    }

    return 0;
}

int hal_mcu2sens_stop_recv(enum HAL_MCU2SENS_ID_T id)
{
    if (id >= HAL_MCU2SENS_ID_QTY) {
        return 1;
    }

    NVIC_DisableIRQ(rx_irq_id[id]);

    return 0;
}

int hal_mcu2sens_send_seq(enum HAL_MCU2SENS_ID_T id, const void *data, unsigned int len, unsigned int *seq)
{
    uint32_t lock;
    int ret;
    struct HAL_MCU2SENS_SEND_RECORD_T *record;
    struct HAL_MCU2SENS_MSG_T *next;
    int i;

    if (id >= HAL_MCU2SENS_ID_QTY) {
        return 1;
    }
    if (!chan_opened[id]) {
        return 2;
    }

    ret = -1;
    record = &send_msgs[id][0];

    lock = int_lock();
    for (i = 0; i < MAX_SEND_RECORD_COUNT; i++) {
        if (record->in_use) {
            record++;
            continue;
        }
        record->in_use = true;
        record->msg.next = NULL;
        record->msg.len = len;
        record->msg.data = data;
#if (MCU2SENS_VERBOSE_TRACE & (1 << 1))
        TR_INFO(0, "MCU2SENS-TX:%d: msg=%p len=%u data=%p content=0x%08X 0x%08X",
            id, record, len, data, data ? *(uint32_t *)data : 0, data ? *(uint32_t *)(data + 4) : 0);
#endif
        if (send_msg_list_p[id] == NULL) {
            send_msg_list_p[id] = &record->msg;
            hal_mcu2sens_peer_irq_set(id, HAL_MCU2SENS_IRQ_SEND_IND);
        } else if (send_pending_list_p[id] == NULL) {
            send_pending_list_p[id] = &record->msg;
        } else {
            next = send_pending_list_p[id];
            while (next->next) {
                next = next->next;
            }
            next->next = &record->msg;
        }
        ret = 0;
        if (seq) {
            *seq = i;
        }
        // Prohibit sleep here
        hal_mcu2sens_busy(id, true);
        break;
    }
    int_unlock(lock);

    return ret;
}

int hal_mcu2sens_send(enum HAL_MCU2SENS_ID_T id, const void *data, unsigned int len)
{
    return hal_mcu2sens_send_seq(id, data, len, NULL);
}

void hal_mcu2sens_rx_done(enum HAL_MCU2SENS_ID_T id)
{
    hal_mcu2sens_local_irq_clear(id, HAL_MCU2SENS_IRQ_SEND_IND);
    hal_mcu2sens_local_irq_mask_set(id, HAL_MCU2SENS_IRQ_SEND_IND);
}

void hal_mcu2sens_tx_irq_run(enum HAL_MCU2SENS_ID_T id)
{
    hal_mcu2sens_tx_irq_handler_by_id(id);
}

int hal_mcu2sens_tx_active(enum HAL_MCU2SENS_ID_T id, unsigned int seq)
{
    if (seq < ARRAY_SIZE(send_msgs[0])) {
        return send_msgs[id][seq].in_use;
    }
    return false;
}

int hal_mcu2sens_opened(enum HAL_MCU2SENS_ID_T id)
{
    return !!chan_opened[id];
}

int hal_mcu2sens_rx_irq_pending(enum HAL_MCU2SENS_ID_T id)
{
    uint32_t value;

#ifdef CHIP_SUBSYS_SENS
    if (id == HAL_MCU2SENS_ID_0) {
        value = SENS_CMU_MCU2SENS_DATA_INTR;
    } else {
        value = SENS_CMU_MCU2SENS_DATA1_INTR;
    }

    return !!(senscmu->SENS2MCU_IRQ_SET & value);
#else
    if (id == HAL_MCU2SENS_ID_0) {
        value = CMU_SENS2MCU_DATA_INTR;
    } else {
        value = CMU_SENS2MCU_DATA1_INTR;
    }

    return !!(cmu->MCU2SENS_IRQ_SET & value);
#endif
}

#endif


/*
 * Emulator Control Server - Device Tethering Handler
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  KiTae Kim       <kt920.kim@samsung.com>
 *  JiHye Kim       <jihye1128.kim@samsung.com>
 *  YeongKyoon Lee  <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "ui/console.h"

#include "ecs.h"
#include "ecs_tethering.h"
#include "../tethering/app_tethering.h"

#include "../debug_ch.h"

MULTI_DEBUG_CHANNEL(tizen, ecs_tethering);

#define MSG_BUF_SIZE  255
#define MSG_LEN_SIZE    4

#if 0
// ecs <-> ecp messages
#define ECS_TETHERING_MSG_CATEGORY                      "tethering"

#define ECS_TETHERING_MSG_GROUP_ECP                     1
// #define TETHERING_MSG_GROUP_USB
// #define TETHERING_MSG_GROUP_WIFI

#define ECS_TETHERING_MSG_ACTION_CONNECT                1
#define ECS_TETHERING_MSG_ACTION_DISCONNECT             2
#define ECS_TETHERING_MSG_ACTION_CONNECTION_STATUS      3
#define ECS_TETHERING_MSG_ACTION_SENSOR_STATUS          4
#define ECS_TETHERING_MSG_ACTION_TOUCH_STATUS           5
#endif

// static bool send_tethering_ntf(const char *data, const int len);
static bool send_tethering_ntf(const char *data);
static void send_tethering_status_ntf(type_group group, type_action action);

static int tethering_port = 0;

void send_tethering_sensor_status_ecp(void)
{
    INFO(">> send tethering_event_status to ecp");
    send_tethering_status_ntf(ECS_TETHERING_MSG_GROUP_ECP,
            ECS_TETHERING_MSG_ACTION_SENSOR_STATUS);
}

void send_tethering_touch_status_ecp(void)
{
    send_tethering_status_ntf(ECS_TETHERING_MSG_GROUP_ECP,
            ECS_TETHERING_MSG_ACTION_TOUCH_STATUS);
}

void send_tethering_connection_status_ecp(void)
{
    INFO(">> send tethering_connection_status to ecp");
    send_tethering_status_ntf(ECS_TETHERING_MSG_GROUP_ECP,
            ECS_TETHERING_MSG_ACTION_CONNECTION_STATUS);
}

static void send_tethering_port_ecp(void)
{
    type_length length;
    type_group group = ECS_TETHERING_MSG_GROUP_ECP;
    type_action action = ECS_TETHERING_MSG_ACTION_CONNECT;
    uint8_t *msg = NULL;
    gchar data[12];

    msg = g_malloc(MSG_BUF_SIZE);
    if (!msg) {
        return;
    }

    TRACE(">> send port_num: %d", tethering_port);

    g_snprintf(data, sizeof(data) - 1, "%d", tethering_port);
    length = strlen(data);

    memcpy(msg, ECS_TETHERING_MSG_CATEGORY, 10);
    memcpy(msg + 10, &length, sizeof(unsigned short));
    memcpy(msg + 12, &group, sizeof(unsigned char));
    memcpy(msg + 13, &action, sizeof(unsigned char));
    memcpy(msg + 14, data, length);

    TRACE(">> send tethering_ntf to ecp. action=%d, group=%d, data=%s",
        action, group, data);

//    send_tethering_ntf((const char *)msg, MSG_BUF_SIZE);
    send_tethering_ntf((const char *)msg);

    if (msg) {
        g_free(msg);
    }
}

static void send_tethering_status_ntf(type_group group, type_action action)
{
    type_length length = 1;
    int status = 0;
    uint8_t *msg = NULL;
    gchar data[2];

    switch (action) {
    case ECS_TETHERING_MSG_ACTION_CONNECTION_STATUS:
        status = get_tethering_connection_status();
        if (status == CONNECTED) {
            send_tethering_port_ecp();
        }
        break;
    case ECS_TETHERING_MSG_ACTION_SENSOR_STATUS:
        status = get_tethering_sensor_status();
        break;
    case ECS_TETHERING_MSG_ACTION_TOUCH_STATUS:
        status = get_tethering_multitouch_status();
        break;
    default:
        break;
    }

    msg = g_malloc(MSG_BUF_SIZE);
    if (!msg) {
        return;
    }

    g_snprintf(data, sizeof(data), "%d", status);

    memcpy(msg, ECS_TETHERING_MSG_CATEGORY, 10);
    memcpy(msg + 10, &length, sizeof(unsigned short));
    memcpy(msg + 12, &group, sizeof(unsigned char));
    memcpy(msg + 13, &action, sizeof(unsigned char));
    memcpy(msg + 14, data, 1);

    TRACE(">> send tethering_ntf to ecp. action=%d, group=%d, data=%s",
        action, group, data);

//    send_tethering_ntf((const char *)msg, MSG_BUF_SIZE);
    send_tethering_ntf((const char *)msg);

    if (msg) {
        g_free(msg);
    }
}

// static bool send_tethering_ntf(const char *data, const int len)
static bool send_tethering_ntf(const char *data)
{
    type_length length = 0;
    type_group group = 0;
    type_action action = 0;

    const int catsize = 10;
    char cat[catsize + 1];
    memset(cat, 0, catsize + 1);

    read_val_str(data, cat, catsize);
    read_val_short(data + catsize, &length);
    read_val_char(data + catsize + 2, &group);
    read_val_char(data + catsize + 2 + 1, &action);

    const char* ijdata = (data + catsize + 2 + 1 + 1);

    TRACE("<< header cat = %s, length = %d, action=%d, group=%d", cat, length,action, group);

    ECS__Master master = ECS__MASTER__INIT;
    ECS__TetheringNtf ntf = ECS__TETHERING_NTF__INIT;

    ntf.category = (char*) g_malloc(catsize + 1);
    strncpy(ntf.category, cat, 10);

    ntf.length = length;
    ntf.group = group;
    ntf.action = action;

    if (length > 0) {
        ntf.has_data = 1;

        ntf.data.data = g_malloc(length);
        ntf.data.len = length;
        memcpy(ntf.data.data, ijdata, length);

        TRACE("data = %s, length = %hu", ijdata, length);
    }

    master.type = ECS__MASTER__TYPE__TETHERING_NTF;
    master.tethering_ntf = &ntf;

    send_to_ecp(&master);

    if (ntf.data.data && ntf.data.len > 0) {
        g_free(ntf.data.data);
    }

    if (ntf.category) {
        g_free(ntf.category);
    }

    return true;
}

void send_tethering_sensor_data(const char *data, int len)
{
    set_injector_data(data);
}

void send_tethering_touch_data(int x, int y, int index, int status)
{
    kbd_mouse_event(x, y, index, status);
}

// handle tethering_req message
bool msgproc_tethering_req(ECS_Client* ccli, ECS__TetheringReq* msg)
{
    gchar cmd[10] = {0};

    g_strlcpy(cmd, msg->category, sizeof(cmd));
    type_length length = (type_length) msg->length;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);

    TRACE(">> header = cmd = %s, length = %d, action=%d, group=%d",
            cmd, length, action, group);

    if (group == ECS_TETHERING_MSG_GROUP_ECP) {
        switch(action) {
        case ECS_TETHERING_MSG_ACTION_CONNECT:
        {
            if (msg->data.data && msg->data.len > 0) {
                const gchar *data = (const gchar *)msg->data.data;
                gint port = 0;

                port = g_ascii_strtoull(data, NULL, 10);

                TRACE(">> MSG_ACTION_CONNECT");
                TRACE(">> len = %zd, data\" %s\"", strlen(data), data);

                connect_tethering_app(port);
                tethering_port = port;

                TRACE(">> port_num: %d, %d", port, tethering_port);
            }
        }
            break;
        case ECS_TETHERING_MSG_ACTION_DISCONNECT:
            INFO(">> MSG_ACTION_DISCONNECT");
            disconnect_tethering_app();
            tethering_port = 0;
            break;
        case ECS_TETHERING_MSG_ACTION_CONNECTION_STATUS:
        case ECS_TETHERING_MSG_ACTION_SENSOR_STATUS:
        case ECS_TETHERING_MSG_ACTION_TOUCH_STATUS:
            TRACE(">> get_status_action");
            send_tethering_status_ntf(group, action);
            break;
        default:
            break;
        }
    }

    return true;
}

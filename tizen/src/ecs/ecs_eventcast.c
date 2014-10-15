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
#include "ecs_eventcast.h"
#include "eventcast/common.h"
#include "eventcast/sensor.h"
#include "eventcast/touch.h"
#include "hw/virtio/maru_virtio_touchscreen.h"
#include "hw/virtio/maru_virtio_hwkey.h"

#include "util/new_debug_ch.h"

DECLARE_DEBUG_CHANNEL(ecs_eventcast);

#define MSG_BUF_SIZE  255
#define MSG_LEN_SIZE    4

#define PRESSED     1
#define RELEASED    2

static bool send_eventcast_ntf(const char *data);
static void send_eventcast_status_ntf(type_group group, type_action action);

static int eventcast_port = 0;

void send_eventcast_sensor_status_ecp(void)
{
    LOG_INFO(">> send eventcast_event_status to ecp\n");
    send_eventcast_status_ntf(ECS_EVENTCAST_MSG_GROUP_ECP,
            ECS_EVENTCAST_MSG_ACTION_SENSOR_STATUS);
}

void send_eventcast_touch_status_ecp(void)
{
    send_eventcast_status_ntf(ECS_EVENTCAST_MSG_GROUP_ECP,
            ECS_EVENTCAST_MSG_ACTION_TOUCH_STATUS);
}

void send_eventcast_connection_status_ecp(void)
{
    LOG_INFO(">> send eventcast_connection_status to ecp\n");
    send_eventcast_status_ntf(ECS_EVENTCAST_MSG_GROUP_ECP,
            ECS_EVENTCAST_MSG_ACTION_CONNECTION_STATUS);
}

#if 0
static void send_eventcast_port_ecp(void)
{
    type_length length;
    type_group group = ECS_EVENTCAST_MSG_GROUP_ECP;
    type_action action = ECS_EVENTCAST_MSG_ACTION_CONNECT;
    uint8_t *msg = NULL;
    gchar data[12];

    msg = g_malloc(MSG_BUF_SIZE);
    if (!msg) {
        return;
    }

    LOG_TRACE(">> send port_num: %d\n", eventcast_port);
    g_snprintf(data, sizeof(data) - 1, "%d", eventcast_port);
    length = strlen(data);

    memcpy(msg, ECS_EVENTCAST_MSG_CATEGORY, 10);
    memcpy(msg + 10, &length, sizeof(unsigned short));
    memcpy(msg + 12, &group, sizeof(unsigned char));
    memcpy(msg + 13, &action, sizeof(unsigned char));
    memcpy(msg + 14, data, length);

    LOG_TRACE(">> send eventcast_ntf to ecp. action=%d, group=%d, data=%s\n",
        action, group, data);

    send_eventcast_ntf((const char *)msg);

    if (msg) {
        g_free(msg);
    }
}
#endif

static void send_eventcast_connection_info(void)
{
    type_length length;
    type_group group = ECS_EVENTCAST_MSG_GROUP_ECP;
    type_action action = ECS_EVENTCAST_MSG_ACTION_CONNECT;
    uint8_t *msg = NULL;
    gchar data[64];

    msg = g_malloc(MSG_BUF_SIZE);
    if (!msg) {
        LOG_SEVERE("failed to allocate memory\n");
        return;
    }

    LOG_INFO(">> send port_num: %d\n", eventcast_port);
    {
        const char *ip = get_eventcast_connected_ipaddr();
        int port = get_eventcast_connected_port();

        if (!ip) {
            LOG_SEVERE("invalid connected ip\n");
            return;
        }

        if (!port) {
            LOG_SEVERE("invalid connected port\n");
            return;
        }
        g_snprintf(data, sizeof(data) - 1, "%s:%d", ip, port);
        length = strlen(data);
        data[length] = '\0';
    }

    memcpy(msg, ECS_EVENTCAST_MSG_CATEGORY, 10);
    memcpy(msg + 10, &length, sizeof(unsigned short));
    memcpy(msg + 12, &group, sizeof(unsigned char));
    memcpy(msg + 13, &action, sizeof(unsigned char));
    memcpy(msg + 14, data, length);

    LOG_INFO(">> send connection msg to ecp. "
        "action=%d, group=%d, data=%s length=%d\n",
        action, group, data, length);

    send_eventcast_ntf((const char *)msg);

    g_free(msg);
}

static void send_eventcast_status_ntf(type_group group, type_action action)
{
    type_length length = 1;
    int status = 0;
    uint8_t *msg = NULL;
    gchar data[2];

    switch (action) {
        case ECS_EVENTCAST_MSG_ACTION_CONNECTION_STATUS:
            status = get_eventcast_connection_status();
            if (status == CONNECTED) {
                send_eventcast_connection_info();
            }
            break;
        case ECS_EVENTCAST_MSG_ACTION_SENSOR_STATUS:
            status = get_eventcast_sensor_status();
            break;
        case ECS_EVENTCAST_MSG_ACTION_TOUCH_STATUS:
            status = get_eventcast_touch_status();
            break;
        default:
            break;
    }

    msg = g_malloc(MSG_BUF_SIZE);
    if (!msg) {
        return;
    }

    g_snprintf(data, sizeof(data), "%d", status);

    memcpy(msg, ECS_EVENTCAST_MSG_CATEGORY, 10);
    memcpy(msg + 10, &length, sizeof(unsigned short));
    memcpy(msg + 12, &group, sizeof(unsigned char));
    memcpy(msg + 13, &action, sizeof(unsigned char));
    memcpy(msg + 14, data, 1);

    LOG_TRACE(">> send eventcast_ntf to ecp. action=%d, group=%d, data=%s\n",
        action, group, data);

    send_eventcast_ntf((const char *)msg);

    if (msg) {
        g_free(msg);
    }
}

static bool send_eventcast_ntf(const char *data)
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

    LOG_TRACE(">> header cat = %s, length = %d, action=%d, group=%d\n", cat, length,action, group);

    ECS__Master master = ECS__MASTER__INIT;
    ECS__EventCastNtf ntf = ECS__EVENT_CAST_NTF__INIT;

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

        LOG_TRACE("data = %s, length = %hu\n", ijdata, length);
    }

    master.type = ECS__MASTER__TYPE__EVENTCAST_NTF;
    master.eventcast_ntf = &ntf;

    pb_to_all_clients(&master);

    if (ntf.data.data && ntf.data.len > 0) {
        g_free(ntf.data.data);
    }

    if (ntf.category) {
        g_free(ntf.category);
    }

    return true;
}

void send_eventcast_sensor_data(const char *data, int len)
{
    set_injector_data(data);
}

void send_eventcast_touch_data(int x, int y, int index, int status)
{
    virtio_touchscreen_event(x, y, index, status);
}

void send_eventcast_hwkey_data(int keycode)
{
    maru_hwkey_event(PRESSED, keycode);
    maru_hwkey_event(RELEASED, keycode);
}

// handle eventcast_req message
bool msgproc_eventcast_req(ECS_Client* ccli, ECS__EventCastReq* msg)
{
    gchar cmd[10] = {0};
    gchar **server_addr = NULL;

    LOG_TRACE("enter %s\n", __func__);

    g_strlcpy(cmd, msg->category, sizeof(cmd));
    type_length length = (type_length) msg->length;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);

    LOG_TRACE("<< header = cmd = %s, length = %d, action=%d, group=%d\n",
            cmd, length, action, group);

    switch(action) {
        case ECS_EVENTCAST_MSG_ACTION_CONNECT:
            LOG_INFO("MSG_ACTION_CONNECT\n");

            if (msg->data.data && msg->data.len > 0) {
                const gchar *data = (const gchar *)msg->data.data;
                gchar *ip_address = NULL;
                guint64 port = 0;

                server_addr = g_strsplit(data, ":", 0);
                if (server_addr && server_addr[0]) {
                    int len = strlen(server_addr[0]);

                    if (len) {
                        ip_address = g_malloc(len + 1);
                        g_strlcpy(ip_address, server_addr[0], len + 1);
                    }
                    LOG_INFO("IP address: %s, length: %d\n", ip_address, len);
                }

                if (server_addr && server_addr[1]) {
                    port = g_ascii_strtoull(server_addr[1], NULL, 10);
                    LOG_INFO("port number: %d\n", port);
                } else {
                    LOG_SEVERE("failed to parse port number\n");
                }
                LOG_TRACE("len = %zd, data\" %s\"", strlen(data), data);

                connect_eventcast_app(ip_address, port);
                eventcast_port = port;

                LOG_TRACE(">> port_num: %d, %d\n", port, eventcast_port);
                g_free(ip_address);
                g_strfreev(server_addr);
            } else {
                LOG_INFO("ip address and port value are null\n");
            }
            break;
        case ECS_EVENTCAST_MSG_ACTION_DISCONNECT:
            LOG_INFO(">> MSG_ACTION_DISCONNECT\n");
            disconnect_eventcast_app();
            eventcast_port = 0;
            break;
        case ECS_EVENTCAST_MSG_ACTION_CONNECTION_STATUS:
        case ECS_EVENTCAST_MSG_ACTION_SENSOR_STATUS:
        case ECS_EVENTCAST_MSG_ACTION_TOUCH_STATUS:
            LOG_TRACE(">> get_status_action\n");
            send_eventcast_status_ntf(group, action);
            break;
        default:
            break;
    }

    LOG_TRACE("leave %s\n", __func__);

    return true;
}

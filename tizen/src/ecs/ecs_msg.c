/* Emulator Control Server
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Jinhyung choi   <jinhyung2.choi@samsung.com>
 *  MunKyu Im       <munkyu.im@samsung.com>
 *  Daiyoung Kim    <daiyoung777.kim@samsung.com>
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

#include <stdbool.h>
#include <pthread.h>
#include <glib.h>
#include <arpa/inet.h>

#include "hw/qdev.h"
#include "net/net.h"
#include "net/slirp.h"
#include "ui/console.h"
#include "migration/migration.h"
#include "qapi/qmp/qint.h"
#include "qapi/qmp/qbool.h"
#include "qapi/qmp/qjson.h"
#include "qapi/qmp/json-parser.h"
#include "ui/qemu-spice.h"
#include "qemu/queue.h"
#include "sysemu/char.h"
#include "qemu/main-loop.h"

#ifdef CONFIG_LINUX
#include <sys/epoll.h>
#endif

#include "qemu-common.h"
#include "ecs-json-streamer.h"
#include "qmp-commands.h"

#include "hw/virtio/maru_virtio_vmodem.h"
#include "hw/virtio/maru_virtio_evdi.h"

#include "ecs.h"

#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, ecs);

static void* build_master(ECS__Master* master, int* payloadsize)
{
    int len_pack = ecs__master__get_packed_size(master);
    *payloadsize = len_pack + 4;
    TRACE("pack size=%d\n", len_pack);
    void* buf = g_malloc(len_pack + 4);
    if (!buf)
        return NULL;

    ecs__master__pack(master, buf + 4);

    len_pack = htonl(len_pack);
    memcpy(buf, &len_pack, 4);

    return buf;
}

bool pb_to_all_clients(ECS__Master* master)
{
    int payloadsize = 0;
    void* buf = build_master(master, &payloadsize);
    if (!buf)
    {
        ERR("invalid buf\n");
        return false;
    }

    if (!send_to_all_client(buf, payloadsize))
        return false;

    if (buf)
    {
        g_free(buf);
    }
    return true;
}

bool pb_to_single_client(ECS__Master* master, ECS_Client *clii)
{
    int payloadsize = 0;
    void* buf = build_master(master, &payloadsize);
    if (!buf)
    {
        ERR("invalid buf\n");
        return false;
    }

    send_to_single_client(clii, buf, payloadsize);

    if (buf)
    {
        g_free(buf);
    }
    return true;
}

void msgproc_keepalive_ans (ECS_Client* ccli, ECS__KeepAliveAns* msg)
{
    ccli->keep_alive = 0;
}

void make_send_device_ntf (char* cmd, int group, int action, char* data)
{
    int msg_length = 15;
    type_length length = 0;
    if (data != NULL) {
        length = strlen(data);
    }
    msg_length += length;
    char* send_msg = (char*) malloc(msg_length);
    if(!send_msg)
        return;
    memcpy(send_msg, cmd, 10);
    memcpy(send_msg + 10, &length, sizeof(unsigned short));
    memcpy(send_msg + 12, &group, sizeof(unsigned char));
    memcpy(send_msg + 13, &action, sizeof(unsigned char));
    if (data != NULL) {
        memcpy(send_msg + 14, data, strlen(data));
    }
    send_device_ntf(send_msg, msg_length);
    free(send_msg);
}

bool send_msg_to_guest(ECS_Client* ccli, char* cmd, int group, int action, char* data, int data_len)
 {
    int sndlen = 15; // HEADER(CMD + LENGTH + GROUP + ACTION) + 1
    char* sndbuf;
    bool ret = false;

    if (data != NULL)
        sndlen += data_len;

    sndbuf = (char*) g_malloc0(sndlen);
    if (!sndbuf) {
        return false;
    }
    memcpy(sndbuf, cmd, 10);
    memcpy(sndbuf + 10, &data_len, 2);
    memcpy(sndbuf + 12, &group, 1);
    memcpy(sndbuf + 13, &action, 1);

    if (data != NULL) {
        memcpy(sndbuf + 14, data, data_len);
    }

    if(strcmp(cmd, "telephony") == 0) {
        TRACE("telephony msg >>");
        ret = send_to_vmodem(route_ij, sndbuf, sndlen);
    } else {
        INFO("evdi msg >> %s", cmd);
        ret = send_to_evdi(route_ij, sndbuf, sndlen);
    }

    g_free(sndbuf);

    if (!ret) {
        return false;
    }

    return true;
}

bool ntf_to_injector(const char* data, const int len) {
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

    const char *encoded_ijdata = "";
    TRACE("<< header cat = %s, length = %d, action=%d, group=%d\n", cat, length,
            action, group);

    QDict* obj_header = qdict_new();
    ecs_make_header(obj_header, length, group, action);

    QDict* objData = qdict_new();
    qobject_incref(QOBJECT(obj_header));

    qdict_put(objData, "cat", qstring_from_str(cat));
    qdict_put(objData, "header", obj_header);
    if(!strcmp(cat, "telephony")) {
        qdict_put(objData, "ijdata", qstring_from_str(encoded_ijdata));
    } else {
        qdict_put(objData, "ijdata", qstring_from_str(ijdata));
    }

    QDict* objMsg = qdict_new();
    qobject_incref(QOBJECT(objData));

    qdict_put(objMsg, "type", qstring_from_str("injector"));
    qdict_put(objMsg, "result", qstring_from_str("success"));
    qdict_put(objMsg, "data", objData);

    QString *json;
    json = qobject_to_json(QOBJECT(objMsg));

    assert(json != NULL);

    qstring_append_chr(json, '\n');
    const char* snddata = qstring_get_str(json);

    TRACE("<< json str = %s", snddata);

    send_to_all_client(snddata, strlen(snddata));

    QDECREF(json);

    QDECREF(obj_header);
    QDECREF(objData);
    QDECREF(objMsg);

    return true;
}


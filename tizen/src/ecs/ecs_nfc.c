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

#include "qemu-common.h"

#include "hw/virtio/maru_virtio_nfc.h"

#include "ecs.h"

#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, ecs);

bool msgproc_nfc_req(ECS_Client* ccli, ECS__NfcReq* msg)
{
    int datalen = msg->data.len;
    void* data = (void*)g_malloc(datalen);
    if(!data) {
        ERR("g_malloc failed!\n");
        return false;
    }

    memset(data, 0, datalen);
    memcpy(data, msg->data.data, msg->data.len);

    if (msg->has_data && msg->data.len > 0)
    {
        TRACE("recv from nfc injector: %s, %z\n", msg->has_data, msg->data.len);
        print_binary(data, datalen);
    }

    send_to_nfc(ccli->client_id, ccli->client_type, data, msg->data.len);
    g_free(data);
    return true;
}

bool send_nfc_ntf(struct nfc_msg_info* msg)
{
    const int catsize = 10;
    char cat[catsize + 1];
    ECS_Client *clii;
    memset(cat, 0, catsize + 1);

    print_binary((char*)msg->buf, msg->use);
    TRACE("id: %02x, type: %02x, use: %d\n", msg->client_id, msg->client_type, msg->use);
    clii =  find_client(msg->client_id, msg->client_type);
    if (clii) {
        if(clii->client_type == TYPE_SIMUL_NFC) {
            strncpy(cat, MSG_TYPE_NFC, 3);
        } else if (clii->client_type == TYPE_ECP) {
            strncpy(cat, MSG_TYPE_SIMUL_NFC, 9);
        }else {
            ERR("cannot find type! : %d\n", clii->client_type);
        }
        TRACE("header category = %s\n", cat);
    }
    else {
        ERR("cannot find client!\n");
    }

    ECS__Master master = ECS__MASTER__INIT;
    ECS__NfcNtf ntf = ECS__NFC_NTF__INIT;

    ntf.category = (char*) g_malloc(catsize + 1);
    strncpy(ntf.category, cat, 10);

    ntf.has_data = 1;

    ntf.data.data = g_malloc(NFC_MAX_BUF_SIZE);
    ntf.data.len = NFC_MAX_BUF_SIZE;
    memcpy(ntf.data.data, msg->buf, NFC_MAX_BUF_SIZE);

    TRACE("send to nfc injector: \n");
    master.type = ECS__MASTER__TYPE__NFC_NTF;
    master.nfc_ntf = &ntf;

    pb_to_all_clients(&master);

    if (ntf.data.data && ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

    if (ntf.category)
        g_free(ntf.category);

    return true;
}



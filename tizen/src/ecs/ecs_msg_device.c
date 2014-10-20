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
#include "qemu/option.h"
#include "qemu/config-file.h"

#include "qmp-commands.h"
#include "net/slirp.h"
#include "fsdev/qemu-fsdev.h"
#include "monitor/qdev.h"
#include "hw/virtio/maru_virtio_sensor.h"
#include "hw/virtio/maru_virtio_nfc.h"
#include "skin/maruskin_operation.h"
#include "skin/maruskin_server.h"

#include "util/maru_device_hotplug.h"
#include "emul_state.h"
#include "ecs.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, ecs);

static void msgproc_device_ans(ECS_Client* ccli, const char* category, bool succeed, char* data)
{
    if (ccli == NULL) {
        return;
    }
    int catlen = 0;
    ECS__Master master = ECS__MASTER__INIT;
    ECS__DeviceAns ans = ECS__DEVICE_ANS__INIT;

    TRACE("device ans - category : %s, succed : %d\n", category, succeed);

    catlen = strlen(category);
    ans.category = (char*) g_malloc0(catlen + 1);
    memcpy(ans.category, category, catlen);

    ans.errcode = !succeed;

    if (data != NULL) {
        ans.length = strlen(data);

        if (ans.length > 0) {
            ans.has_data = 1;
            ans.data.data = g_malloc(ans.length);
            ans.data.len = ans.length;
            memcpy(ans.data.data, data, ans.length);
            TRACE("data = %s, length = %hu\n", data, ans.length);
        }
    }

    master.type = ECS__MASTER__TYPE__DEVICE_ANS;
    master.device_ans = &ans;

    pb_to_all_clients(&master);

    if (ans.category)
        g_free(ans.category);
}

extern char tizen_target_img_path[];
void send_target_image_information(ECS_Client* ccli) {
    ECS__Master master = ECS__MASTER__INIT;
    ECS__DeviceAns ans = ECS__DEVICE_ANS__INIT;
    int length = strlen(tizen_target_img_path); // ??

    ans.category = (char*) g_malloc(10 + 1);
    strncpy(ans.category, "info", 10);

    ans.errcode = 0;
    ans.length = length;
    ans.group = 1;
    ans.action = 1;

    if (length > 0)
    {
        ans.has_data = 1;

        ans.data.data = g_malloc(length);
        ans.data.len = length;
        memcpy(ans.data.data, tizen_target_img_path, length);

        TRACE("data = %s, length = %hu\n", tizen_target_img_path, length);
    }

    master.type = ECS__MASTER__TYPE__DEVICE_ANS;
    master.device_ans = &ans;

    pb_to_single_client(&master, ccli);

    if (ans.data.len > 0)
    {
        g_free(ans.data.data);
    }

    g_free(ans.category);
}

static void msgproc_device_req_sensor(ECS_Client* ccli, ECS__DeviceReq* msg, char* cmd)
{
    char* data = NULL;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*) g_malloc0(msg->data.len + 1);
        memcpy(data, msg->data.data, msg->data.len);
    }

    if (group == MSG_GROUP_STATUS) {
        if (action == MSG_ACT_ACCEL) {
            get_sensor_accel();
        } else if (action == MSG_ACT_GYRO) {
            get_sensor_gyro();
        } else if (action == MSG_ACT_MAG) {
            get_sensor_mag();
        } else if (action == MSG_ACT_LIGHT) {
            get_sensor_light();
        } else if (action == MSG_ACT_PROXI) {
            get_sensor_proxi();
        }
    } else {
        if (data != NULL) {
            set_injector_data(data);
        } else {
            ERR("sensor set data is null\n");
        }
    }
    msgproc_device_ans(ccli, cmd, true, NULL);

    if (data) {
        g_free(data);
    }
}

static void msgproc_device_req_network(ECS_Client* ccli, ECS__DeviceReq* msg)
{
    char* data = NULL;

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*) g_malloc0(msg->data.len + 1);
        memcpy(data, msg->data.data, msg->data.len);
    }

    if (data != NULL) {
        TRACE(">>> Network msg: '%s'\n", data);
        if(net_slirp_redir(data) < 0) {
            ERR( "redirect [%s] fail\n", data);
        } else {
            TRACE("redirect [%s] success\n", data);
        }
    } else {
        ERR("Network redirection data is null.\n");
    }

    if (data) {
        g_free(data);
    }
}

static void msgproc_device_req_tgesture(ECS_Client* ccli, ECS__DeviceReq* msg)
{
    char* data = NULL;
    type_group group = (type_group) (msg->group & 0xff);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*) g_malloc0(msg->data.len + 1);
        memcpy(data, msg->data.data, msg->data.len);
    }

    /* release multi-touch */
#ifndef CONFIG_USE_SHM
    if (get_multi_touch_enable() != 0) {
        clear_finger_slot(false);
    }
#else
    // TODO:
#endif

    if (data == NULL) {
        ERR("touch gesture data is NULL\n");
        return;
    }

    TRACE("%s\n", data);

    char token[] = "#";

    if (group == 1) { /* HW key event */
        char *section = strtok(data, token);
        int event_type = atoi(section);

        section = strtok(NULL, token);
        int keycode = atoi(section);

        do_hw_key_event(event_type, keycode);
    } else { /* touch event */
        char *section = strtok(data, token);
        int event_type = atoi(section);

        section = strtok(NULL, token);
        int xx = atoi(section);

        section = strtok(NULL, token);
        int yy = atoi(section);

        section = strtok(NULL, token);
        int zz = atoi(section);

        do_mouse_event(1/* LEFT */, event_type, 0, 0, xx, yy, zz);
    }

    if (data) {
        g_free(data);
    }
}

static void msgproc_device_req_input(ECS_Client* ccli, ECS__DeviceReq* msg, char* cmd)
{
    char* data = NULL;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*) g_malloc0(msg->data.len + 1);
        memcpy(data, msg->data.data, msg->data.len);
    }

    // cli input
    TRACE("receive input message [%s]\n", data);

    if (group == 0) {
        TRACE("input keycode data : [%s]\n", data);

        char token[] = " ";
        char *section = strtok(data, token);
        int keycode = atoi(section);
        if (action == 1) {
            //action 1 press
            do_hw_key_event(KEY_PRESSED, keycode);

        } else if (action == 2) {
            //action 2 released
            do_hw_key_event(KEY_RELEASED, keycode);

        } else {
            ERR("unknown action : [%d]\n", (int)action);
        }
    } else if (group == 1) {
        //spec out
        TRACE("input category's group 1 is spec out\n");
    } else {
        ERR("unknown group [%d]\n", (int)group);
    }
    msgproc_device_ans(ccli, cmd, true, NULL);

    if (data) {
        g_free(data);
    }
}

static void msgproc_device_req_nfc(ECS_Client* ccli, ECS__DeviceReq* msg)
{
    char* data = NULL;
    type_group group = (type_group) (msg->group & 0xff);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*) g_malloc0(msg->data.len + 1);
        memcpy(data, msg->data.data, msg->data.len);
    }

    if (group == MSG_GROUP_STATUS) {
        get_nfc_data();
    } else {
        if (data != NULL) {
            send_to_nfc(ccli->client_id, ccli->client_type, data, msg->data.len);
        } else {
            ERR("nfc data is null\n");
        }
    }

    if (data) {
        g_free(data);
    }
}

static char hds_path[PATH_MAX];

static void msgproc_device_req_hds(ECS_Client* ccli, ECS__DeviceReq* msg, char * cmd)
{
    char* data = NULL;
    type_group group = (type_group) (msg->group & 0xff);
    type_action action = (type_action) (msg->action & 0xff);

    if (msg->has_data && msg->data.len > 0)
    {
        data = (char*) g_malloc0(msg->data.len + 1);
        memcpy(data, msg->data.data, msg->data.len);
    }

    INFO("hds group: %d, action : %d\n", group, action);
    if (group == MSG_GROUP_STATUS) {
        char hds_data_send[PATH_MAX + 3];
        if (is_hds_attached()) {
            sprintf(hds_data_send, "1, %s", hds_path);
        } else {
            sprintf(hds_data_send, "0, ");
        }
        make_send_device_ntf(cmd, group, 99, hds_data_send);
    } else if (group == 100 && action == 1) {
        INFO("try attach with is_hds_attached : %d\n", is_hds_attached());
        if (data != NULL && !is_hds_attached()) {
            do_hotplug(ATTACH_HDS, data, strlen(data) + 1);
            if (!is_hds_attached()) {
                ERR("failed to attach");
                make_send_device_ntf(cmd, 100, 2, NULL);
            } else {
                memset(hds_path, 0, sizeof(hds_path));
                memcpy(hds_path, data, sizeof(hds_path) - 1);
                INFO("send emuld to mount.\n");
                send_msg_to_guest(ccli, cmd, group, action, data, strlen(data));
            }
        } else {
            make_send_device_ntf(cmd, 100, 2, NULL);
        }
    } else if (group == 100 && action == 2) {
        INFO("try detach with is_hds_attached : %d\n", is_hds_attached());
        if (is_hds_attached()) {
            INFO("send emuld to umount.\n");
            send_msg_to_guest(ccli, cmd, group, action, NULL, 0);
        } else {
            INFO("hds is not attached. do not try detach it.\n");
        }
    } else {
        ERR("hds unknown command: group %d action %d\n", group, action);
    }

    if (data) {
        g_free(data);
    }
}

bool msgproc_device_req(ECS_Client* ccli, ECS__DeviceReq* msg)
{
    char cmd[10];
    memset(cmd, 0, 10);
    strcpy(cmd, msg->category);

    TRACE(">> device_req: header = cmd = %s, length = %d, action=%d, group=%d\n",
            cmd, msg->length, msg->action, msg->group);

    if (!strcmp(cmd, MSG_TYPE_SENSOR)) {
        msgproc_device_req_sensor(ccli, msg, cmd);
    } else if (!strcmp(cmd, "Network")) {
        msgproc_device_req_network(ccli, msg);
    } else if (!strcmp(cmd, "TGesture")) {
        msgproc_device_req_tgesture(ccli, msg);
    } else if (!strcmp(cmd, "info")) {
        // check to emulator target image path
        TRACE("receive info message %s\n", tizen_target_img_path);
        send_target_image_information(ccli);
    } else if (!strcmp(cmd, "hds")) {
        msgproc_device_req_hds(ccli, msg, cmd);
    } else if (!strcmp(cmd, "input")) {
        msgproc_device_req_input(ccli, msg, cmd);
    } else if (!strcmp(cmd, "vmname")) {
        char* vmname = get_emul_vm_name();
        msgproc_device_ans(ccli, cmd, true, vmname);
    } else if (!strcmp(cmd, "nfc")) {
        msgproc_device_req_nfc(ccli, msg);
    } else {
        ERR("unknown cmd [%s]\n", cmd);
    }

    return true;
}

bool send_device_ntf(const char* data, const int len)
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

    TRACE("<< header cat = %s, length = %d, action=%d, group=%d\n", cat, length,action, group);

    ECS__Master master = ECS__MASTER__INIT;
    ECS__DeviceNtf ntf = ECS__DEVICE_NTF__INIT;

    ntf.category = (char*) g_malloc(catsize + 1);
    strncpy(ntf.category, cat, 10);


    ntf.length = length;
    ntf.group = group;
    ntf.action = action;

    if (length > 0)
    {
        ntf.has_data = 1;

        ntf.data.data = g_malloc(length);
        ntf.data.len = length;
        memcpy(ntf.data.data, ijdata, length);

        TRACE("data = %s, length = %hu\n", ijdata, length);
    }

    master.type = ECS__MASTER__TYPE__DEVICE_NTF;
    master.device_ntf = &ntf;

    pb_to_all_clients(&master);

    if (ntf.data.data && ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

    if (ntf.category)
        g_free(ntf.category);

    return true;
}


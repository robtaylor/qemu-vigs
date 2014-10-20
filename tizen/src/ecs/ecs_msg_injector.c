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

#include "hw/virtio/maru_virtio_vmodem.h"
#include "hw/virtio/maru_virtio_evdi.h"
#include "hw/virtio/maru_virtio_jack.h"
#include "hw/virtio/maru_virtio_power.h"

#include "util/maru_device_hotplug.h"
#include "emul_state.h"
#include "ecs.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, ecs);

extern QemuMutex mutex_guest_connection;
static int guest_connection = 0;

extern QemuMutex mutex_location_data;
static char location_data[MAX_INJECTOR_REQ_DATA];

static void msgproc_injector_ans(ECS_Client* ccli, const char* category, bool succeed)
{
    if (ccli == NULL) {
        return;
    }
    int catlen = 0;
    ECS__Master master = ECS__MASTER__INIT;
    ECS__InjectorAns ans = ECS__INJECTOR_ANS__INIT;

    TRACE("injector ans - category : %s, succed : %d\n", category, succeed);

    catlen = strlen(category);
    ans.category = (char*) g_malloc0(catlen + 1);
    memcpy(ans.category, category, catlen);

    ans.errcode = !succeed;
    master.type = ECS__MASTER__TYPE__INJECTOR_ANS;
    master.injector_ans = &ans;

    pb_to_all_clients(&master);

    if (ans.category)
        g_free(ans.category);
}

static bool injector_send(ECS_Client* ccli, ECS__InjectorReq* msg, char* cmd)
{
    int sndlen = 15; // HEADER(CMD + LENGTH + GROUP + ACTION) + 1
    const char* msg_data;
    char* sndbuf;
    bool ret = false;
    type_group group;

    group = (type_group) (msg->group & 0xff);

    if (msg->has_data && msg->data.data && msg->data.len > 0)
        sndlen += msg->data.len;

    sndbuf = (char*) g_malloc0(sndlen);
    if (!sndbuf) {
        msgproc_injector_ans(ccli, cmd, false);
        return false;
    }

    memcpy(sndbuf, cmd, 10);
    memcpy(sndbuf + 10, &msg->length, 2);
    memcpy(sndbuf + 12, &msg->group, 1);
    memcpy(sndbuf + 13, &msg->action, 1);

    if (msg->has_data && msg->data.data && msg->data.len > 0) {
        msg_data = (const char*)msg->data.data;
        memcpy(sndbuf + 14, msg_data, msg->data.len);
        TRACE(">> print len = %zd, data\" %s\"\n", msg->data.len, msg_data);
    }

    if(strcmp(cmd, "telephony") == 0) {
        TRACE("telephony msg >>");
        ret = send_to_vmodem(route_ij, sndbuf, sndlen);
    } else {
        TRACE("evdi msg >> %s", cmd);
        ret = send_to_evdi(route_ij, sndbuf, sndlen);
    }

    g_free(sndbuf);

    if (group != MSG_GROUP_STATUS) {
        msgproc_injector_ans(ccli, cmd, ret);
    }

    if (!ret) {
        return false;
    }

    return true;
}

static char* get_emulator_sdcard_path(void)
{
    char *emulator_sdcard_path = NULL;
    char *tizen_sdk_data = NULL;

#ifndef CONFIG_WIN32
    char emulator_sdcard[] = "/emulator/sdcard/";
#else
    char emulator_sdcard[] = "\\emulator\\sdcard\\";
#endif

    TRACE("emulator_sdcard: %s, %zu\n", emulator_sdcard, sizeof(emulator_sdcard));

    tizen_sdk_data = get_tizen_sdk_data_path();
    if (!tizen_sdk_data) {
        ERR("failed to get tizen-sdk-data path.\n");
        return NULL;
    }

    emulator_sdcard_path =
        g_malloc(strlen(tizen_sdk_data) + sizeof(emulator_sdcard) + 1);
    if (!emulator_sdcard_path) {
        ERR("failed to allocate memory.\n");
        return NULL;
    }

    g_snprintf(emulator_sdcard_path, strlen(tizen_sdk_data) + sizeof(emulator_sdcard),
             "%s%s", tizen_sdk_data, emulator_sdcard);

    g_free(tizen_sdk_data);

    TRACE("sdcard path: %s\n", emulator_sdcard_path);
    return emulator_sdcard_path;
}

static char *get_old_tizen_sdk_data_path(void)
{
    char *tizen_sdk_data_path = NULL;

    INFO("try to search tizen-sdk-data path in another way.\n");

#ifndef CONFIG_WIN32
    char tizen_sdk_data[] = "/tizen-sdk-data";
    int tizen_sdk_data_len = 0;
    char *home_dir;

    home_dir = (char *)g_getenv("HOME");
    if (!home_dir) {
        home_dir = (char *)g_get_home_dir();
    }

    tizen_sdk_data_len = strlen(home_dir) + sizeof(tizen_sdk_data) + 1;
    tizen_sdk_data_path = g_malloc(tizen_sdk_data_len);
    if (!tizen_sdk_data_path) {
        ERR("failed to allocate memory.\n");
        return NULL;
    }
    g_strlcpy(tizen_sdk_data_path, home_dir, tizen_sdk_data_len);
    g_strlcat(tizen_sdk_data_path, tizen_sdk_data, tizen_sdk_data_len);

#else
    char tizen_sdk_data[] = "\\tizen-sdk-data\\";
    gint tizen_sdk_data_len = 0;
    HKEY hKey;
    char strLocalAppDataPath[1024] = { 0 };
    DWORD dwBufLen = 1024;

    RegOpenKeyEx(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        0, KEY_QUERY_VALUE, &hKey);

    RegQueryValueEx(hKey, "Local AppData", NULL,
                    NULL, (LPBYTE)strLocalAppDataPath, &dwBufLen);
    RegCloseKey(hKey);

    tizen_sdk_data_len = strlen(strLocalAppDataPath) + sizeof(tizen_sdk_data) + 1;
    tizen_sdk_data_path = g_malloc(tizen_sdk_data_len);
    if (!tizen_sdk_data_path) {
        ERR("failed to allocate memory.\n");
        return NULL;
    }

    g_strlcpy(tizen_sdk_data_path, strLocalAppDataPath, tizen_sdk_data_len);
    g_strlcat(tizen_sdk_data_path, tizen_sdk_data, tizen_sdk_data_len);
#endif

    INFO("tizen-sdk-data path: %s\n", tizen_sdk_data_path);
    return tizen_sdk_data_path;
}

/*
 *  get tizen-sdk-data path from sdk.info.
 */
char *get_tizen_sdk_data_path(void)
{
    char const *emul_bin_path = NULL;
    char *sdk_info_file_path = NULL;
    char *tizen_sdk_data_path = NULL;
#ifndef CONFIG_WIN32
    const char *sdk_info = "../../../sdk.info";
#else
    const char *sdk_info = "..\\..\\..\\sdk.info";
#endif
    const char sdk_data_var[] = "TIZEN_SDK_DATA_PATH";

    FILE *sdk_info_fp = NULL;
    int sdk_info_path_len = 0;

    TRACE("%s\n", __func__);

    emul_bin_path = get_bin_path();
    if (!emul_bin_path) {
        ERR("failed to get emulator path.\n");
        return NULL;
    }

    sdk_info_path_len = strlen(emul_bin_path) + strlen(sdk_info) + 1;
    sdk_info_file_path = g_malloc(sdk_info_path_len);
    if (!sdk_info_file_path) {
        ERR("failed to allocate sdk-data buffer.\n");
        return NULL;
    }

    g_snprintf(sdk_info_file_path, sdk_info_path_len, "%s%s",
                emul_bin_path, sdk_info);
    INFO("sdk.info path: %s\n", sdk_info_file_path);

    sdk_info_fp = fopen(sdk_info_file_path, "r");
    g_free(sdk_info_file_path);

    if (sdk_info_fp) {
        TRACE("Succeeded to open [sdk.info].\n");

        char tmp[256] = { '\0', };
        char *tmpline = NULL;
        while (fgets(tmp, sizeof(tmp), sdk_info_fp) != NULL) {
            if ((tmpline = g_strstr_len(tmp, sizeof(tmp), sdk_data_var))) {
                tmpline += strlen(sdk_data_var) + 1; // 1 for '='
                break;
            }
        }

        if (tmpline) {
            if (tmpline[strlen(tmpline) - 1] == '\n') {
                tmpline[strlen(tmpline) - 1] = '\0';
            }
            if (tmpline[strlen(tmpline) - 1] == '\r') {
                tmpline[strlen(tmpline) - 1] = '\0';
            }

            tizen_sdk_data_path = g_malloc(strlen(tmpline) + 1);
            g_strlcpy(tizen_sdk_data_path, tmpline, strlen(tmpline) + 1);

            INFO("tizen-sdk-data path: %s\n", tizen_sdk_data_path);

            fclose(sdk_info_fp);
            return tizen_sdk_data_path;
        }

        fclose(sdk_info_fp);
    }

    // legacy mode
    ERR("Failed to open [sdk.info].\n");

    return get_old_tizen_sdk_data_path();
}

static void handle_sdcard(char* dataBuf, size_t dataLen)
{

    char ret = 0;

    if (dataBuf != NULL){
        ret = dataBuf[0];

        if (ret == '0' ) {
            /* umount sdcard */
            do_hotplug(DETACH_SDCARD, NULL, 0);
        } else if (ret == '1') {
            /* mount sdcard */
            char sdcard_img_path[256];
            char* sdcard_path = NULL;

            sdcard_path = get_emulator_sdcard_path();
            if (sdcard_path) {
                g_strlcpy(sdcard_img_path, sdcard_path,
                        sizeof(sdcard_img_path));

                /* emulator_sdcard_img_path + sdcard img name */
                char* sdcard_img_name = dataBuf+2;
                if(dataLen > 3 && sdcard_img_name != NULL){
                    char* pLinechange = strchr(sdcard_img_name, '\n');
                    if(pLinechange != NULL){
                        sdcard_img_name = g_strndup(sdcard_img_name, pLinechange - sdcard_img_name);
                    }

                    g_strlcat(sdcard_img_path, sdcard_img_name, sizeof(sdcard_img_path));
                    TRACE("sdcard path: [%s]\n", sdcard_img_path);

                    do_hotplug(ATTACH_SDCARD, sdcard_img_path, strlen(sdcard_img_path) + 1);

                    /*if using strndup than free string*/
                    if(pLinechange != NULL && sdcard_img_name!= NULL){
                        free(sdcard_img_name);
                    }

                }

                g_free(sdcard_path);
            } else {
                ERR("failed to get sdcard path!!\n");
            }
        } else if(ret == '2'){
            TRACE("sdcard status 2 bypass" );
        }else {
            ERR("!!! unknown command : %c\n", ret);
        }

    }else{
        ERR("!!! unknown data : %c\n", ret);
    }
}

static bool injector_req_sdcard(ECS_Client* ccli, ECS__InjectorReq* msg, char *cmd)
{
    if (msg->has_data) {
        TRACE("msg(%zu) : %s\n", msg->data.len, msg->data.data);
        handle_sdcard((char*) msg->data.data, msg->data.len);
    } else {
        ERR("has no msg\n");
    }

    injector_send(ccli, msg, cmd);

    return true;
}

static void send_status_injector_ntf(const char* cmd, int cmdlen, int act, char* on)
{
    int msglen = 0, datalen = 0;
    type_length length  = 0;
    type_group group = MSG_GROUP_STATUS;
    type_action action = act;

    if (cmd == NULL || cmdlen > 10)
        return;

    if (on == NULL) {
        msglen = 14;
    } else {
        datalen = strlen(on);
        length  = (unsigned short)datalen;

        msglen = datalen + 15;
    }

    char* status_msg = (char*) malloc(msglen);
    if(!status_msg)
        return;

    memset(status_msg, 0, msglen);

    memcpy(status_msg, cmd, cmdlen);
    memcpy(status_msg + 10, &length, sizeof(unsigned short));
    memcpy(status_msg + 12, &group, sizeof(unsigned char));
    memcpy(status_msg + 13, &action, sizeof(unsigned char));

    if (on != NULL) {
        memcpy(status_msg + 14, on, datalen);
    }

    send_injector_ntf(status_msg, msglen);

    if (status_msg)
        free(status_msg);
}

static bool injector_req_sensor(ECS_Client* ccli, ECS__InjectorReq* msg, char *cmd)
{
    char data[MAX_INJECTOR_REQ_DATA];
    type_group group;
    type_action action;

    memset(data, 0, MAX_INJECTOR_REQ_DATA);
    group = (type_group) (msg->group & 0xff);
    action = (type_action) (msg->action & 0xff);

    if (group == MSG_GROUP_STATUS) {
        switch (action) {
        case MSG_ACT_BATTERY_LEVEL:
            sprintf(data, "%d", get_power_capacity());
            break;
        case MSG_ACT_BATTERY_CHARGER:
            sprintf(data, "%d", get_jack_charger());
            break;
        case MSG_ACT_USB:
            sprintf(data, "%d", get_jack_usb());
            break;
        case MSG_ACT_EARJACK:
            sprintf(data, "%d", get_jack_earjack());
            break;
        case MSG_ACT_LOCATION:
            qemu_mutex_lock(&mutex_location_data);
            sprintf(data, "%s", location_data);
            qemu_mutex_unlock(&mutex_location_data);
            break;
        default:
            return injector_send(ccli, msg, cmd);
        }
        TRACE("status : %s\n", data);
        send_status_injector_ntf(MSG_TYPE_SENSOR, 6, action, data);
        return true;
    } else if (msg->data.data && msg->data.len > 0) {
        set_injector_data((char*) msg->data.data);
        return injector_send(ccli, msg, cmd);
    }

    return false;
}

static bool injector_req_guest(void)
{
    int value = 0;
    qemu_mutex_lock(&mutex_guest_connection);
    value = guest_connection;
    qemu_mutex_unlock(&mutex_guest_connection);
    send_status_injector_ntf(MSG_TYPE_GUEST, 5, value, NULL);
    return true;
}

static bool injector_req_location(ECS_Client* ccli, ECS__InjectorReq* msg, char *cmd)
{
    if (msg->data.data != NULL && msg->data.len > 0) {
        qemu_mutex_lock(&mutex_location_data);
        snprintf(location_data, msg->data.len + 1, "%s", (char*)msg->data.data);
        qemu_mutex_unlock(&mutex_location_data);
        return injector_send(ccli, msg, cmd);
    }

    return false;
}

bool msgproc_injector_req(ECS_Client* ccli, ECS__InjectorReq* msg)
{
    char cmd[11];
    bool ret = false;

    strncpy(cmd, msg->category, sizeof(cmd) - 1);

    if (!strcmp(cmd, MSG_TYPE_SDCARD)) {
        ret = injector_req_sdcard(ccli, msg, cmd);
    } else if (!strcmp(cmd, MSG_TYPE_SENSOR)) {
        ret = injector_req_sensor(ccli, msg, cmd);
    } else if (!strcmp(cmd, MSG_TYPE_GUEST)) {
        ret = injector_req_guest();
    } else if (!strcmp(cmd, MSG_TYPE_LOCATION)) {
        ret = injector_req_location(ccli, msg, cmd);
    } else {
        ret = injector_send(ccli, msg, cmd);
    }

    return ret;
}

void ecs_suspend_lock_state(int state)
{
    int catlen;

    ECS__InjectorReq msg = ECS__INJECTOR_REQ__INIT;
    const char* category = "suspend";

    catlen = strlen(category);
    msg.category = (char*) g_malloc0(catlen + 1);
    memcpy(msg.category, category, catlen);

    msg.group = 5;
    msg.action = state;

    msgproc_injector_req(NULL, &msg);
}

#define MSG_GROUP_HDS   100
static bool injector_req_handle(char* cat, type_action action)
{
    /*SD CARD msg process*/
    if (!strcmp(cat, MSG_TYPE_SDCARD)) {
        return false;
    } else if (!strcmp(cat, "suspend")) {
        ecs_suspend_lock_state(ecs_get_suspend_state());
        return true;
    } else if (!strcmp(cat, MSG_TYPE_GUEST)) {
        INFO("emuld connection is %d\n", action);
        qemu_mutex_lock(&mutex_guest_connection);
        guest_connection = action;
        qemu_mutex_unlock(&mutex_guest_connection);
        return false;
    } else if (!strcmp(cat, "hds")) {
        INFO("hds status is %d\n", action);
        switch (action) {
            case 1:
                make_send_device_ntf(cat, MSG_GROUP_HDS, action, NULL);
                break;
            case 2:
                make_send_device_ntf(cat, MSG_GROUP_HDS, action, NULL);
                break;
            case 3:
                do_hotplug(DETACH_HDS, NULL, 0);
                if (!is_hds_attached()) {
                    make_send_device_ntf(cat, MSG_GROUP_HDS, 5, NULL);
                } else {
                    make_send_device_ntf(cat, MSG_GROUP_HDS, action, NULL);
                }
                break;
            case 4:
                make_send_device_ntf(cat, MSG_GROUP_HDS, action, NULL);
                break;
            default:
                ERR("unknown action: %s.\n", action);
                break;
        }
        return true;
    } else {
        ERR("unknown command: %s.\n", cat);
    }

    return false;
}

bool send_injector_ntf(const char* data, const int len)
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

    if (injector_req_handle(cat, action)) {
        return true;
    }

    const char* ijdata = (data + catsize + 2 + 1 + 1);

    TRACE("<< header cat = %s, length = %d, action=%d, group=%d\n", cat, length,action, group);

    ECS__Master master = ECS__MASTER__INIT;
    ECS__InjectorNtf ntf = ECS__INJECTOR_NTF__INIT;

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
    }

    master.type = ECS__MASTER__TYPE__INJECTOR_NTF;
    master.injector_ntf = &ntf;

    pb_to_all_clients(&master);

    if (ntf.data.len > 0)
    {
        g_free(ntf.data.data);
    }

    g_free(ntf.category);

    return true;
}


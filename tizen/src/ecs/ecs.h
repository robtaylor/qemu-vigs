/*
 * Emulator Control Server
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

#ifndef __ECS_H__
#define __ECS_H__

#ifdef CONFIG_LINUX
#include <sys/epoll.h>
#endif

#include "qapi/qmp/qerror.h"
#include "qemu-common.h"
#include "ecs-json-streamer.h"
#include "genmsg/ecs.pb-c.h"
#include "genmsg/ecs_ids.pb-c.h"

#define ECS_VERSION   "1.0"

#define ECS_OPTS_NAME           "ecs"
#define HOST_LISTEN_ADDR        "127.0.0.1"
#define HOST_LISTEN_PORT        0
#define EMULATOR_SERVER_NUM     10

#define COMMANDS_TYPE           "type"
#define COMMANDS_DATA           "data"


#define COMMAND_TYPE_INJECTOR   "injector"
#define COMMAND_TYPE_CONTROL    "control"
#define COMMAND_TYPE_MONITOR    "monitor"
#define COMMAND_TYPE_DEVICE     "device"

#define COMMAND_TYPE_TETHERING  "tethering"

#define MSG_TYPE_SENSOR         "sensor"
#define MSG_TYPE_LOCATION       "location"
#define MSG_TYPE_NFC            "nfc"
#define MSG_TYPE_SIMUL_NFC      "simul_nfc"
#define MSG_TYPE_SDCARD         "sdcard"
#define MSG_TYPE_GUEST          "guest"

#define MSG_GROUP_STATUS        15

enum message_action {
    MSG_ACT_BATTERY_LEVEL = 100,
    MSG_ACT_BATTERY_CHARGER = 101,
    MSG_ACT_USB = 102,
    MSG_ACT_EARJACK = 103,
    MSG_ACT_RSSI = 104,
    MSG_ACT_ACCEL = 110,
    MSG_ACT_GYRO = 111,
    MSG_ACT_MAG = 112,
    MSG_ACT_LIGHT = 113,
    MSG_ACT_PROXI = 114,
    MSG_ACT_MOTION = 115,
    MSG_ACT_LOCATION = 120
};

#define TIMER_ALIVE_S           60
#define TYPE_DATA_SELF          "self"

enum injector_level {
    level_accel = 1,
    level_proxi = 2,
    level_light = 3,
    level_gyro = 4,
    level_geo = 5,
    level_battery = 8,
    level_tilt = 12,
    level_magnetic = 13
};

#define MAX_CATEGORY_LEN        10
typedef unsigned short  type_length;
typedef unsigned char   type_group;
typedef unsigned char   type_action;

#define OUT_BUF_SIZE    4096
#define READ_BUF_LEN    4096
#define MAX_ID_SIZE     255
#define MAX_INJECTOR_REQ_DATA 32
typedef struct sbuf
{
    int _netlen;
    int _use;
    char _buf[OUT_BUF_SIZE];
}sbuf;

struct Monitor {
    int suspend_cnt;
    uint8_t outbuf[OUT_BUF_SIZE];
    int outbuf_index;
    void *password_opaque;
    QError *error;
    QLIST_HEAD(,mon_fd_t) fds;
    QLIST_ENTRY(Monitor) entry;
};

#define MAX_EVENTS  1000
#define MAX_FD_NUM  300
typedef struct ECS_State {
    int listen_fd;
#ifdef CONFIG_LINUX
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
#else
    fd_set reads;
#endif
    int is_unix;
    int ecs_running;
    QEMUTimer *alive_timer;
    Monitor *mon;
} ECS_State;

#define TYPE_NONE       0x00
#define TYPE_ECP        0x01
#define TYPE_SIMUL_NFC  0x02

typedef struct ECS_Client {
    int client_fd;
    unsigned char client_id;
    unsigned char client_type;
    int keep_alive;
    const char* type;

    sbuf sbuf;

    ECS_State *cs;
    JSONMessageParser parser;
    QTAILQ_ENTRY(ECS_Client) next;
} ECS_Client;

#define NFC_MAX_BUF_SIZE  4096

typedef struct nfc_msg_info {
    unsigned char client_id;
    unsigned char client_type;
    uint32_t use;
    unsigned char buf[NFC_MAX_BUF_SIZE];

}nfc_msg_info;

int start_ecs(void);

ECS_Client *find_client(unsigned char id, unsigned char type);
bool handle_protobuf_msg(ECS_Client* cli, char* data, const int len);

bool ntf_to_injector(const char* data, const int len);
bool ntf_to_control(const char* data, const int len);
bool ntf_to_monitor(const char* data, const int len);

bool send_msg_to_guest(ECS_Client* ccli, char* cmd, int group, int action,
                       char* data, int data_len);
void make_send_device_ntf (char* cmd, int group, int action, char* data);

bool pb_to_all_clients(ECS__Master* master);
bool pb_to_single_client(ECS__Master* master, ECS_Client *clii);

bool send_injector_ntf(const char* data, const int len);
bool send_monitor_ntf(const char* data, const int len);
bool send_device_ntf(const char* data, const int len);
bool send_nfc_ntf(struct nfc_msg_info *msg);

void send_to_single_client(ECS_Client *clii, const char* data, const int len);
bool send_to_all_client(const char* data, const int len);
void send_to_client(int fd, const char* data, const int len);

void ecs_client_close(ECS_Client* clii);
int ecs_write(int fd, const uint8_t *buf, int len);
void ecs_make_header(QDict* obj, type_length length, type_group group, type_action action);

void read_val_short(const char* data, unsigned short* ret_val);
void read_val_char(const char* data, unsigned char* ret_val);
void read_val_str(const char* data, char* ret_val, int len);
void print_binary(const char* data, const int len);

bool msgproc_injector_req(ECS_Client* ccli, ECS__InjectorReq* msg);
bool msgproc_monitor_req(ECS_Client *ccli, ECS__MonitorReq* msg);
bool msgproc_device_req(ECS_Client* ccli, ECS__DeviceReq* msg);
bool msgproc_nfc_req(ECS_Client* ccli, ECS__NfcReq* msg);
void msgproc_checkversion_req(ECS_Client* ccli, ECS__CheckVersionReq* msg);
void msgproc_keepalive_ans(ECS_Client* ccli, ECS__KeepAliveAns* msg);
bool msgproc_eventcast_req(ECS_Client* ccli, ECS__EventCastReq* msg);

char *get_tizen_sdk_data_path(void);

/* version check  */
//void send_ecs_version_check(ECS_Client* ccli);

/* Suspend/resume */
#define SUSPEND_LOCK   1
#define SUSPEND_UNLOCK 0
int ecs_get_suspend_state(void);
void ecs_set_suspend_state(int state);
void ecs_suspend_lock_state(int state);

void send_host_keyboard_ntf(int on);

void send_target_image_information(ECS_Client* ccli);

/* request */
int accel_min_max(double value);
void req_set_sensor_accel(int x, int y, int z);
void set_injector_data(const char* data);

/* Monitor */
void handle_qmp_command(JSONMessageParser *parser, QList *tokens, void *opaque);

static QemuOptsList qemu_ecs_opts = {
    .name = ECS_OPTS_NAME,
    .implied_opt_name = ECS_OPTS_NAME,
    .head = QTAILQ_HEAD_INITIALIZER(qemu_ecs_opts.head),
    .desc = {
        {
            .name = "host",
            .type = QEMU_OPT_STRING,
        },{
            .name = "port",
            .type = QEMU_OPT_STRING,
        },{
            .name = "localaddr",
            .type = QEMU_OPT_STRING,
        },{
            .name = "localport",
            .type = QEMU_OPT_STRING,
        },{
            .name = "to",
            .type = QEMU_OPT_NUMBER,
        },{
            .name = "ipv4",
            .type = QEMU_OPT_BOOL,
        },{
            .name = "ipv6",
            .type = QEMU_OPT_BOOL,
        },
        { /* End of list */ }
    },
};

#endif /* __ECS_H__ */

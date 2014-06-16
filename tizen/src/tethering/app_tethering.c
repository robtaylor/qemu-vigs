/*
 * emulator controller client
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  Kitae Kim <kt920.kim@samsung.com>
 *  JiHye Kim <jihye1128.kim@samsung.com>
 *  YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */
#ifndef __WIN32
#include <sys/ioctl.h>
#else
#define EISCONN WSAEISCONN
#endif

#include "qemu-common.h"
#include "qemu/main-loop.h"
#include "qemu/sockets.h"
#include "ui/console.h"

#include "emulator.h"
#include "emul_state.h"
#include "app_tethering.h"
#include "../ecs/ecs_tethering.h"
#include "genmsg/tethering.pb-c.h"

#include "../debug_ch.h"
MULTI_DEBUG_CHANNEL(tizen, app_tethering);

#define TETHERING_MSG_HANDSHAKE_KEY     100
#define MSG_BUF_SIZE    255
#define MSG_LEN_SIZE    4

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

typedef struct tethering_recv_buf {
    uint32_t len;
    uint32_t stack_size;
    char data[MSG_BUF_SIZE];
} tethering_recv_buf;

typedef struct _TetheringState {
    int sock_fd;
    int connection_status;
    tethering_recv_buf recv_buf;
} TetheringState;

#if 0
enum connection_status {
    CONNECTED = 1,
    DISCONNECTED,
    CONNECTING,
    CONNREFUSED,
};

enum device_status {
    ENABLED = 1,
    DISABLED,
};

enum touch_status {
    RELEASED = 0,
    PRESSED,
};
#endif

enum sensor_level {
    level_accel = 1,
    level_proxi = 2,
    level_light = 3,
    level_gyro = 4,
    level_geo = 5,
    level_tilt = 12,
    level_magnetic = 13
};

#ifndef DEBUG
const char *connection_status_str[4] = {"CONNECTED", "DISCONNECTED",
                                        "CONNECTING", "CONNREFUSED"};
#endif

static tethering_recv_buf recv_buf;

static int connection_status = DISCONNECTED;
static int tethering_sock = -1;
// static bool app_state = false;
static int sensor_device_status = DISABLED;
static int mt_device_status = DISABLED;

static void end_tethering_socket(int sockfd);
static void set_tethering_connection_status(int status);
static void set_tethering_sensor_status(int status);
static void set_tethering_multitouch_status(int status);
#if 0
static void set_tethering_app_state(bool state);
static bool get_tethering_app_state(void);
#endif

static void *build_tethering_msg(Tethering__TetheringMsg* msg, int *payloadsize)
{
    void *buf = NULL;
    int msg_packed_size = 0;

    msg_packed_size = tethering__tethering_msg__get_packed_size(msg);
    *payloadsize = msg_packed_size + MSG_LEN_SIZE;

    buf = g_malloc(*payloadsize);
    if (!buf) {
        ERR("failed to allocate memory\n");
        return NULL;
    }

    tethering__tethering_msg__pack(msg, buf + MSG_LEN_SIZE);

    msg_packed_size = htonl(msg_packed_size);
    memcpy(buf, &msg_packed_size, MSG_LEN_SIZE);

    return buf;
}

static bool send_msg_to_controller(Tethering__TetheringMsg *msg)
{
    void *buf = NULL;
    int payloadsize = 0, err = 0;
    int sockfd = 0;
    bool ret = true;

    buf = build_tethering_msg(msg, &payloadsize);
    if (!buf) {
        return false;
    }

    sockfd = tethering_sock;
    err = qemu_sendto(sockfd, buf, payloadsize, 0, NULL, 0);
    if (err < 0) {
        ERR("failed to send a message. err: %d\n", err);
        ret = false;
    }

    if (buf) {
        g_free(buf);
    }

    return ret;
}

static bool send_handshake_req_msg(void)
{
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;
    Tethering__HandShakeReq req = TETHERING__HAND_SHAKE_REQ__INIT;

    TRACE("enter: %s\n", __func__);

    req.key = TETHERING_MSG_HANDSHAKE_KEY;

    msg.type = TETHERING__TETHERING_MSG__TYPE__HANDSHAKE_REQ;
    msg.handshakereq = &req;

    TRACE("send handshake_req message\n");
    send_msg_to_controller(&msg);

    TRACE("leave: %s\n", __func__);

    return true;
}

#if 0
static bool send_emul_state_msg(void)
{
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;
    Tethering__EmulatorState emul_state = TETHERING__EMULATOR_STATE__INIT;

    TRACE("enter: %s\n", __func__);

    emul_state.state = TETHERING__CONNECTION_STATE__TERMINATE;

    msg.type = TETHERING__TETHERING_MSG__TYPE__EMUL_STATE;
    msg.emulstate = &emul_state;

    INFO("send emulator_state message\n");
    send_msg_to_controller(&msg);

    TRACE("leave: %s\n", __func__);

    return true;
}
#endif

static bool build_event_msg(Tethering__EventMsg *event)
{
    bool ret = false;
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;

    TRACE("enter: %s\n", __func__);

    msg.type = TETHERING__TETHERING_MSG__TYPE__EVENT_MSG;
    msg.eventmsg = event;

    ret = send_msg_to_controller(&msg);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_event_start_ans_msg(Tethering__MessageResult result)
{
    bool ret = false;
    Tethering__EventMsg event = TETHERING__EVENT_MSG__INIT;
    Tethering__StartAns start_ans = TETHERING__START_ANS__INIT;

    TRACE("enter: %s\n", __func__);

    start_ans.result = result;

    event.type = TETHERING__EVENT_MSG__TYPE__START_ANS;
    event.startans = &start_ans;

    TRACE("send event_start_ans message\n");
    ret = build_event_msg(&event);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_set_event_status_msg(Tethering__EventType event_type,
                                    Tethering__State status)
{
    bool ret = false;
    Tethering__EventMsg event = TETHERING__EVENT_MSG__INIT;
    Tethering__SetEventStatus event_status = TETHERING__SET_EVENT_STATUS__INIT;

    TRACE("enter: %s\n", __func__);

    event_status.type = event_type;
    event_status.state = status;

    event.type = TETHERING__EVENT_MSG__TYPE__EVENT_STATUS;
    event.setstatus = &event_status;

    TRACE("send event_set_event_status message\n");
    ret = build_event_msg(&event);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}


// create a sensor message.
static bool build_sensor_msg(Tethering__SensorMsg *sensor)
{
    bool ret = false;
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;

    TRACE("enter: %s\n", __func__);

    msg.type = TETHERING__TETHERING_MSG__TYPE__SENSOR_MSG;
    msg.sensormsg = sensor;

    ret = send_msg_to_controller(&msg);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_sensor_start_ans_msg(Tethering__MessageResult result)
{
    bool ret = false;
    Tethering__SensorMsg event = TETHERING__SENSOR_MSG__INIT;
    Tethering__StartAns start_ans = TETHERING__START_ANS__INIT;

    TRACE("enter: %s\n", __func__);

    start_ans.result = result;

    event.type = TETHERING__SENSOR_MSG__TYPE__START_ANS;
    event.startans = &start_ans;

    TRACE("send sensor_start_ans message\n");
    ret = build_sensor_msg(&event);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_set_sensor_status_msg(Tethering__SensorType sensor_type,
                                    Tethering__State status)
{
    bool ret = false;

    Tethering__SensorMsg sensor = TETHERING__SENSOR_MSG__INIT;
    Tethering__SetSensorStatus sensor_status =
                            TETHERING__SET_SENSOR_STATUS__INIT;

    TRACE("enter: %s\n", __func__);

    sensor_status.type = sensor_type;
    sensor_status.state = status;

    sensor.type = TETHERING__SENSOR_MSG__TYPE__SENSOR_STATUS;
    sensor.setstatus = &sensor_status;

    TRACE("send sensor_set_event_status message\n");
    ret = build_sensor_msg(&sensor);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static void set_sensor_data(Tethering__SensorData *data)
{
    /*
     * data format for sensor device
     * each value is classified by carriage return character
     * sensor_type/param numbers/parameters
     * ex) acceleration sensor: "level_accel\n3\nx\ny\nz\n"
     */

    switch(data->sensor) {
    case TETHERING__SENSOR_TYPE__ACCEL:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%s\n%s\n%s\n",
                level_accel, 3, data->x, data->y, data->z);
        send_tethering_sensor_data(tmp, strlen(tmp));

        TRACE("sensor_accel x: %s, y: %s, z: %s\n",
            data->x, data->y, data->z);
    }
        break;
    case TETHERING__SENSOR_TYPE__MAGNETIC:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%s\n%s\n%s\n",
                level_magnetic, 3, data->x, data->y, data->z);
        send_tethering_sensor_data(tmp, strlen(tmp));

        TRACE("sensor_mag x: %s, y: %s, z: %s\n",
            data->x, data->y, data->z);
    }
        break;
    case TETHERING__SENSOR_TYPE__GYROSCOPE:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%s\n%s\n%s\n",
                level_gyro, 3, data->x, data->y, data->z);
        send_tethering_sensor_data(tmp, strlen(tmp));

        TRACE("sensor_gyro x: %s, y: %s, z: %s\n",
            data->x, data->y, data->z);
    }
        break;
    case TETHERING__SENSOR_TYPE__PROXIMITY:
    {
        char tmp[255] = {0};
        double x = (double)(atoi(data->x));

        sprintf(tmp, "%d\n%d\n%.1f\n", level_proxi, 1, x);
        send_tethering_sensor_data(tmp, strlen(tmp));

        TRACE("sensor_proxi x: %.1f, %s\n", x, tmp);
    }
        break;
    case TETHERING__SENSOR_TYPE__LIGHT:
    {
        char tmp[255] = {0};

        sprintf(tmp, "%d\n%d\n%s%s\n", level_light, 2, data->x, data->y);
        send_tethering_sensor_data(tmp, strlen(tmp));

        TRACE("sensor_light x: %s\n", data->x);
    }
        break;
    default:
        TRACE("invalid sensor data\n");
        break;
    }

    // set ecs_sensor
}

static bool build_mulitouch_msg(Tethering__MultiTouchMsg *multitouch)
{
    bool ret = false;
    Tethering__TetheringMsg msg = TETHERING__TETHERING_MSG__INIT;

    TRACE("enter: %s\n", __func__);

    msg.type = TETHERING__TETHERING_MSG__TYPE__TOUCH_MSG;
    msg.touchmsg = multitouch;

    ret = send_msg_to_controller(&msg);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_mulitouch_start_ans_msg(Tethering__MessageResult result)
{
    bool ret = false;

    Tethering__MultiTouchMsg mt = TETHERING__MULTI_TOUCH_MSG__INIT;
    Tethering__StartAns start_ans = TETHERING__START_ANS__INIT;

    TRACE("enter: %s\n", __func__);

    start_ans.result = result;

    mt.type = TETHERING__MULTI_TOUCH_MSG__TYPE__START_ANS;
    mt.startans = &start_ans;

    ret = build_mulitouch_msg(&mt);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static bool send_set_multitouch_max_count(void)
{
    bool ret = false;

    Tethering__MultiTouchMsg mt = TETHERING__MULTI_TOUCH_MSG__INIT;
    Tethering__MultiTouchMaxCount touch_cnt =
        TETHERING__MULTI_TOUCH_MAX_COUNT__INIT;

    TRACE("enter: %s\n", __func__);

    touch_cnt.max = get_emul_max_touch_point();

    mt.type = TETHERING__MULTI_TOUCH_MSG__TYPE__MAX_COUNT;
    mt.maxcount = &touch_cnt;

    INFO("send multi-touch max count: %d\n", touch_cnt.max);
    ret = build_mulitouch_msg(&mt);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static void set_multitouch_data(Tethering__MultiTouchData *data)
{
    float x = 0.0, y = 0.0;
    int32_t index = 0, state = 0;

    switch(data->state) {
    case TETHERING__TOUCH_STATE__PRESSED:
        TRACE("touch pressed\n");
        index = data->index;
        x = data->xpoint;
        y = data->ypoint;
        state = PRESSED;
        break;
    case TETHERING__TOUCH_STATE__RELEASED:
        TRACE("touch released\n");
        index = data->index;
        x = data->xpoint;
        y = data->ypoint;
        state = RELEASED;
        break;
    default:
        TRACE("invalid multitouch data\n");
        break;
    }

    INFO("set touch_data. index: %d, x: %d, y: %d\n", index, x, y);
    // set ecs_multitouch
    send_tethering_touch_data(x, y, index, state);
}

static bool send_set_multitouch_resolution(void)
{
    bool ret = false;

    Tethering__MultiTouchMsg mt = TETHERING__MULTI_TOUCH_MSG__INIT;
    Tethering__Resolution resolution = TETHERING__RESOLUTION__INIT;

    TRACE("enter: %s\n", __func__);

    resolution.width = get_emul_resolution_width();
    resolution.height = get_emul_resolution_height();

    mt.type = TETHERING__MULTI_TOUCH_MSG__TYPE__RESOLUTION;
    mt.resolution = &resolution;

    INFO("send multi-touch resolution: %dx%d\n",
        resolution.width, resolution.height);
    ret = build_mulitouch_msg(&mt);

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static void msgproc_tethering_handshake_ans(Tethering__HandShakeAns *msg)
{
    // FIXME: handle handshake answer
    //  ans = msg->result;
}

static void msgproc_app_state_msg(Tethering__AppState *msg)
{
    if (msg->state == TETHERING__CONNECTION_STATE__TERMINATED) {
        INFO("App is terminated\n");

//      set_tethering_app_state(false);
        set_tethering_sensor_status(DISABLED);
        set_tethering_multitouch_status(DISABLED);

        disconnect_tethering_app();
    } else {
        // does nothing
    }
}


static bool msgproc_tethering_event_msg(Tethering__EventMsg *msg)
{
    bool ret = true;

    switch(msg->type) {
    case TETHERING__EVENT_MSG__TYPE__START_REQ:
    {
        int touch_status = 0;

        TRACE("EVENT_MSG_TYPE_START_REQ\n");
        send_set_event_status_msg(TETHERING__EVENT_TYPE__SENSOR,
                                TETHERING__STATE__ENABLED);

        // TODO: check sensor device whether it exists or not
        set_tethering_sensor_status(ENABLED);

        if (is_emul_input_touch_enable()) {
            touch_status = TETHERING__STATE__ENABLED;
            set_tethering_multitouch_status(ENABLED);
        } else {
            touch_status = TETHERING__STATE__DISABLED;
            set_tethering_multitouch_status(DISABLED);
        }

        TRACE("send multi-touch event_status msg: %d\n", touch_status);
        send_set_event_status_msg(TETHERING__EVENT_TYPE__TOUCH, touch_status);

        TRACE("send event_start_ans msg: %d\n", touch_status);
        send_event_start_ans_msg(TETHERING__MESSAGE_RESULT__SUCCESS);
    }
        break;
    case TETHERING__EVENT_MSG__TYPE__TERMINATE:
        break;
    default:
        TRACE("invalid event_msg type\n");
        ret = false;
        break;
    }

    return ret;
}

static bool msgproc_tethering_sensor_msg(Tethering__SensorMsg *msg)
{
    bool ret = true;

    switch(msg->type) {
    case TETHERING__SENSOR_MSG__TYPE__START_REQ:
        TRACE("SENSOR_MSG_TYPE_START_REQ\n");

        // set sensor type.
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__ACCEL,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__MAGNETIC,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__GYROSCOPE,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__PROXIMITY,
                                TETHERING__STATE__ENABLED);
        send_set_sensor_status_msg(TETHERING__SENSOR_TYPE__LIGHT,
                                TETHERING__STATE__ENABLED);

        TRACE("SENSOR_MSG_TYPE_START_ANS\n");
        send_sensor_start_ans_msg(TETHERING__MESSAGE_RESULT__SUCCESS);

        break;
    case TETHERING__SENSOR_MSG__TYPE__TERMINATE:
        TRACE("SENSOR_MSG_TYPE_TERMINATE\n");
        break;

    case TETHERING__SENSOR_MSG__TYPE__SENSOR_DATA:
        TRACE("SENSOR_MSG_TYPE_SENSOR_DATA\n");
        set_sensor_data(msg->data);
        break;
    default:
        TRACE("invalid sensor_msg type");
        ret = false;
        break;
    }

    return ret;
}

static bool msgproc_tethering_mt_msg(Tethering__MultiTouchMsg *msg)
{
    bool ret = true;

    switch(msg->type) {
    case TETHERING__MULTI_TOUCH_MSG__TYPE__START_REQ:
        TRACE("TOUCH_MSG_TYPE_START\n");

        send_set_multitouch_max_count();
        send_set_multitouch_resolution();

        ret = send_mulitouch_start_ans_msg(TETHERING__MESSAGE_RESULT__SUCCESS);
        break;
    case TETHERING__MULTI_TOUCH_MSG__TYPE__TERMINATE:
        TRACE("TOUCH_MSG_TYPE_TERMINATE\n");
        break;
    case TETHERING__MULTI_TOUCH_MSG__TYPE__TOUCH_DATA:
        set_multitouch_data(msg->touchdata);
        break;
    default:
        TRACE("invalid multitouch_msg\n");
        ret = false;
        break;
    }

    return ret;
}

static bool handle_tethering_msg_from_controller(char *data, int len)
{
    Tethering__TetheringMsg *tethering = NULL;
    bool ret = true;

    tethering = tethering__tethering_msg__unpack(NULL, (size_t)len,
                                            (const uint8_t *)data);

    if (!tethering) {
        // error message
        ERR("no tethering massage\n");
        return false;
    }

    switch (tethering->type) {
    case TETHERING__TETHERING_MSG__TYPE__HANDSHAKE_ANS:
    {
        // TODO: set the result of handshake_ans to
        Tethering__HandShakeAns *msg = tethering->handshakeans;
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_handshake_ans(msg);
            INFO("receive handshake answer\n");

            set_tethering_connection_status(CONNECTED);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__APP_STATE:
    {
        Tethering__AppState *msg = tethering->appstate;

        INFO("receive app_state msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_app_state_msg(msg);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__EVENT_MSG:
    {
        Tethering__EventMsg *msg = tethering->eventmsg;

        INFO("receive event_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_event_msg(msg);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__SENSOR_MSG:
    {
        Tethering__SensorMsg *msg = tethering->sensormsg;

        INFO("receive sensor_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_sensor_msg(msg);
        }
    }
        break;
    case TETHERING__TETHERING_MSG__TYPE__TOUCH_MSG:
    {
        Tethering__MultiTouchMsg *msg = tethering->touchmsg;

        INFO("receive multitouch_msg\n");
        if (!msg) {
            ret = false;
        } else {
            msgproc_tethering_mt_msg(msg);
        }
    }
        break;
    default:
        TRACE("invalid type message\n");
        ret = false;
        break;
    }

#if 1
    if (data) {
        g_free(data);
    }
#endif

    tethering__tethering_msg__free_unpacked(tethering, NULL);
    return ret;
}

static void reset_tethering_recv_buf(void *opaque)
{
    memset(opaque, 0x00, sizeof(tethering_recv_buf));
}

// tethering client socket
static void tethering_io_handler(void *opaque)
{
    int ret = 0;
    int payloadsize = 0, read_size = 0;
    int to_read_bytes = 0;

#ifndef CONFIG_WIN32
    ret = ioctl(tethering_sock, FIONREAD, &to_read_bytes);
    if (ret < 0) {
        ERR("invalid ioctl opertion\n");
    }
#else
    unsigned long to_read_bytes_long = 0;
    ret = ioctlsocket(tethering_sock, FIONREAD, &to_read_bytes_long);
    if (ret < 0) {
    }
    to_read_bytes = (int)to_read_bytes_long;
#endif
    TRACE("ioctl: ret: %d, FIONREAD: %d\n", ret, to_read_bytes);

    if (to_read_bytes == 0) {
        INFO("there is no read data\n");
        disconnect_tethering_app();
        return;
    }

    if (recv_buf.len == 0) {
        ret = qemu_recv(tethering_sock, &payloadsize, sizeof(payloadsize), 0);
        if (ret < sizeof(payloadsize)) {
            return;
        }

        payloadsize = ntohl(payloadsize);
        TRACE("payload size: %d\n", payloadsize);

#if 0
        if (payloadsize > to_read_bytes) {
            TRACE("invalid payload size: %d\n", payloadsize);
            return;
        }
#endif
        recv_buf.len = payloadsize;
        to_read_bytes -= sizeof(payloadsize);
    }

    if (to_read_bytes == 0) {
        return;
    }

    to_read_bytes = min(to_read_bytes, (recv_buf.len - recv_buf.stack_size));

    read_size =
        qemu_recv(tethering_sock, (char *)(recv_buf.data + recv_buf.stack_size),
                    to_read_bytes, 0);
    if (read_size == 0) {
        ERR("failed to read data\n");
        disconnect_tethering_app();
        return;
    } else {
        recv_buf.stack_size += read_size;
    }

    if (recv_buf.len == recv_buf.stack_size) {
        char *snd_buf = NULL;

        snd_buf = g_malloc(recv_buf.stack_size);
        memcpy(snd_buf, recv_buf.data, recv_buf.stack_size);

        handle_tethering_msg_from_controller(snd_buf,
                                            recv_buf.stack_size);
        reset_tethering_recv_buf(&recv_buf);
    }
}

static int register_tethering_io_handler(int fd)
{
    int ret = 0, err = 0;

    TRACE("enter: %s\n", __func__);

    /* register callbackfn for read */
    err = qemu_set_fd_handler(fd, tethering_io_handler, NULL, NULL);
    if (err) {
        ERR("failed to set event handler. fd: %d, err: %d\n", fd, err);
        ret = -1;
    }

    TRACE("leave: %s\n", __func__);

    return ret;
}

static int destroy_tethering_io_handler(int fd)
{
    int ret = 0, err = 0;

    TRACE("enter: %s\n", __func__);

    err = qemu_set_fd_handler(fd, NULL, NULL, NULL);

    if (err) {
        ERR("failed to set event handler. fd: %d, err: %d\n", fd, err);
        ret = -1;
    }

    TRACE("leave: %s, ret: %d\n", __func__, ret);

    return ret;
}

static int start_tethering_socket(const char *ipaddress, int port)
{
    struct sockaddr_in addr;

    gchar serveraddr[32] = { 0, };
    int sock = -1;
    int ret = 0;


    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // i.e. 1234

    if (ipaddress == NULL) {
        g_strlcpy(serveraddr, "127.0.0.1", sizeof(serveraddr));
    } else {
        g_strlcpy(serveraddr, ipaddress, sizeof(serveraddr));
    }

    INFO("server ip address: %s, port: %d\n", serveraddr, port);

    ret = inet_aton(serveraddr, &addr.sin_addr);

    if (ret == 0) {
        ERR("inet_aton failure\n");
        return -1;
    }

    sock = qemu_socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        // set_tethering_connection_status(DISCONNECTED);
        ERR("tethering socket creation is failed\n", sock);
        return -1;
    }
    INFO("tethering socket is created: %d\n", sock);

    qemu_set_nonblock(sock);

    set_tethering_connection_status(CONNECTING);
    do {
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("connect failure");
            INFO("tethering socket is connecting.\n");
            ret = -socket_error();
        } else {
            INFO("tethering socket is connected.\n");
            ret = 0;
            // set_tethering_app_state(true);
            break;
        }
        INFO("ret: %d\n", ret);
    } while (ret == -EINPROGRESS);

    if (ret < 0 && ret != -EISCONN) {
        if (ret == -ECONNREFUSED) {
            INFO("socket connection is refused\n");
            set_tethering_connection_status(CONNREFUSED);
        }
        INFO("close socket\n");
        end_tethering_socket(sock);
        sock = -1;
    }

    return sock;
}

static void end_tethering_socket(int sockfd)
{
    if (closesocket(sockfd) < 0) {
        perror("closesocket failure");
        return;
    }

    INFO("tethering socket is closed: %d\n", sockfd);
    set_tethering_connection_status(DISCONNECTED);
    set_tethering_sensor_status(DISABLED);
    set_tethering_multitouch_status(DISABLED);
}

#if 0
static void set_tethering_app_state(bool state)
{
    TRACE("set tethering_app state: %d", state);
    app_state = state;
}

static bool get_tethering_app_state(void)
{
    return app_state;
}
#endif

// ecs <-> tethering
int get_tethering_connection_status(void)
{
    return connection_status;
}

static void set_tethering_connection_status(int status)
{
    connection_status = status;
    if (status) {
        INFO("connection status: %s\n", connection_status_str[status - 1]);
    }

    send_tethering_connection_status_ecp();
}

int get_tethering_sensor_status(void)
{
    return sensor_device_status;
}

static void set_tethering_sensor_status(int status)
{
    sensor_device_status = status;
    send_tethering_sensor_status_ecp();
}

int get_tethering_multitouch_status(void)
{
    return mt_device_status;
}

static void set_tethering_multitouch_status(int status)
{
    mt_device_status = status;
    send_tethering_touch_status_ecp();
}

int disconnect_tethering_app(void)
{
    int sock = 0;

    INFO("disconnect app from ecp\n");

    sock = tethering_sock;
    if (sock < 0) {
        ERR("tethering socket is terminated or not ready\n");
    } else {
        destroy_tethering_io_handler(sock);
#if 0
        if (get_tethering_app_state()) {
            send_emul_state_msg();
        }
#endif
        end_tethering_socket(sock);
    }

    return 0;
}

static void tethering_notify_exit(Notifier *notifier, void *data) {
    disconnect_tethering_app();
}
static Notifier tethering_exit = { .notify = tethering_notify_exit };

int connect_tethering_app(const char *ipaddress, int port)
{
    int sock = 0, ret = 0;

    TRACE("connect ecp to app\n");

    sock = start_tethering_socket(ipaddress, port);
    if (sock < 0) {
        ERR("failed to start tethering_socket\n");
        tethering_sock = -1;
        return -1;
    }

    INFO("tethering_sock: %d\n", sock);
    tethering_sock = sock;

    reset_tethering_recv_buf(&recv_buf);
    ret = register_tethering_io_handler(sock);
    send_handshake_req_msg();

    emulator_add_exit_notifier(&tethering_exit);

    return ret;
}

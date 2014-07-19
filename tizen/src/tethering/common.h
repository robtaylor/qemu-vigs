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

#include "qemu/queue.h"

typedef struct input_device_list {
    int type;
    void *opaque;

    QTAILQ_ENTRY(input_device_list) node;
} input_device_list;

// common
enum connection_status {
    CONNECTED = 1,
    DISCONNECTED,
    CONNECTING,
    CONNREFUSED,
};

enum touch_status {
    RELEASED = 0,
    PRESSED,
};

bool send_msg_to_controller(void *msg);

int connect_tethering_app(const char *ipaddress, int port);

int disconnect_tethering_app(void);

int get_tethering_connection_status(void);

int add_input_device(void *opaque);

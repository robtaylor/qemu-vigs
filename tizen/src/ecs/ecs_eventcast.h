/*
 * Emulator Control Server - Device Tethering Handler
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  KiTae Kim       <kt920.kim@samsung.com>
 *  JiHey Kim       <jihye1128.kim@samsung.com>
 *  DaiYoung Kim    <daiyoung777.kim@samsung.com>
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

#ifndef __ECS_EVENTCAST_H__
#define __ECS_EVENTCAST_H__

/*
 * define eventcast messages between ecs and ecp
 */
#define ECS_EVENTCAST_MSG_CATEGORY                      "eventcast"
#define ECS_EVENTCAST_MSG_GROUP_ECP                     1

enum ECS_EVENTCAST_MSG_ACTION {
    ECS_EVENTCAST_MSG_ACTION_CONNECT = 1,
    ECS_EVENTCAST_MSG_ACTION_DISCONNECT,
    ECS_EVENTCAST_MSG_ACTION_CONNECTION_STATUS,
    ECS_EVENTCAST_MSG_ACTION_SENSOR_STATUS,
    ECS_EVENTCAST_MSG_ACTION_TOUCH_STATUS,
};

void send_eventcast_sensor_status_ecp(void);

void send_eventcast_touch_status_ecp(void);

void send_eventcast_connection_status_ecp(void);

void send_eventcast_sensor_data(const char *data, int len);

void send_eventcast_touch_data(int x, int y, int index, int status);

void send_eventcast_hwkey_data(int keycode);

#endif /* __ECS_EVENTCAST_H__ */

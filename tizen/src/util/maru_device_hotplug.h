/*
 * Maru device hotplug
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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

#ifndef _MARU_DEVICE_HOTPLUG_H_
#define _MARU_DEVICE_HOTPLUG_H_

enum command {
    ATTACH_HOST_KEYBOARD,
    DETACH_HOST_KEYBOARD,
    ATTACH_SDCARD,
    DETACH_SDCARD,
    ATTACH_HDS,
    DETACH_HDS,
};

void maru_device_hotplug_init(void);

void do_hotplug(int command, void *opaque, size_t size);

bool is_host_keyboard_attached(void);
bool is_sdcard_attached(void);
bool is_hds_attached(void);

#endif // _MARU_DEVICE_HOTPLUG_H_

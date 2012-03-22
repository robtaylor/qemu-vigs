/*
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * HyunJun Son <hj79.son@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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


#ifndef __EMUL_STATE_H__
#define __EMUL_STATE_H__


#include "maru_common.h"
#include "maru_finger.h"

/* keep it consistent with java definition */
enum {
    HARD_KEY_HOME = 101,
    HARD_KEY_POWER = 103,
};

enum {
    MOUSE_DOWN = 1,
    MOUSE_UP = 2,
    MOUSE_DRAG = 3,
};

enum {
    KEY_PRESSED = 1,
    KEY_RELEASED = 2,
};

enum {
    ROTATION_PORTRAIT = 0,
    ROTATION_LANDSCAPE = 1,
    ROTATION_REVERSE_PORTRAIT = 2,
    ROTATION_REVERSE_LANDSCAPE = 3,
};


typedef  struct EmulatorConfigInfo {
    char emulator_name[256];
    int lcd_size_w;
    int lcd_size_h;
    int dpi;
    //TODO:
} EmulatorConfigInfo;

typedef struct EmulatorConfigState {
    double scale_factor;
    short rotation_type;
    MultiTouchState qemu_mts;
    //TODO:
} EmulatorConfigState;


/* setter */
void set_emul_lcd_size(int width, int height);
void set_emul_win_scale(double scale);
void set_emul_rotation(short rotation_type);

/* getter */
int get_emul_lcd_width(void);
int get_emul_lcd_height(void);
double get_emul_win_scale(void);
short get_emul_rotation(void);
MultiTouchState *get_emul_multi_touch_state(void);


#endif /* __EMUL_STATE_H__ */

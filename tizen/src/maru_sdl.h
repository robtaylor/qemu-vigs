/*
 * SDL_WINDOWID hack
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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


#ifndef MARU_SDL_H_
#define MARU_SDL_H_

#include <SDL.h>
#include <SDL_syswm.h>
#include "ui/console.h"
#include "qemu-common.h"

extern DisplayChangeListenerOps maru_dcl_ops;

void maruskin_sdl_init(uint64 swt_handle,
    unsigned int display_width, unsigned int display_height,
    bool blank_guide);
void maruskin_sdl_resize(void);
void maruskin_sdl_update(void);
void maruskin_sdl_invalidate(bool on);
void maruskin_sdl_interpolation(bool on);
void maruskin_sdl_quit(void);

#endif /* MARU_SDL_H_ */

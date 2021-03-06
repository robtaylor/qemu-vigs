/*
 * yagl
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Stanislav Vorobiov <s.vorobiov@samsung.com>
 * Jinhyung Jo <jinhyung.jo@samsung.com>
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

#ifndef _QEMU_YAGL_EGL_NATIVE_CONFIG_H
#define _QEMU_YAGL_EGL_NATIVE_CONFIG_H

#include "yagl_types.h"
#include <EGL/egl.h>

struct yagl_egl_native_config
{
    /*
     * Filled by the driver.
     */

    EGLint red_size;
    EGLint green_size;
    EGLint blue_size;
    EGLint alpha_size;
    EGLint buffer_size;
    EGLenum caveat;
    EGLint config_id;
    EGLint frame_buffer_level;
    EGLint depth_size;
    EGLint max_pbuffer_width;
    EGLint max_pbuffer_height;
    EGLint max_pbuffer_size;
    EGLint max_swap_interval;
    EGLint min_swap_interval;
    EGLint native_visual_id;
    EGLint native_visual_type;
    EGLint samples_per_pixel;
    EGLint stencil_size;
    EGLenum transparent_type;
    EGLint trans_red_val;
    EGLint trans_green_val;
    EGLint trans_blue_val;

    void *driver_data;

    /*
     * Filled automatically by 'yagl_egl_config_xxx'.
     */

    EGLint surface_type;
    EGLBoolean native_renderable;
    EGLint renderable_type;
    EGLenum conformant;
    EGLint sample_buffers_num;
    EGLint match_format_khr;
    EGLBoolean bind_to_texture_rgb;
    EGLBoolean bind_to_texture_rgba;
};

/*
 * Initialize to default values.
 */
void yagl_egl_native_config_init(struct yagl_egl_native_config *cfg);

#endif

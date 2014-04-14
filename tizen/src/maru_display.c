/*
 * MARU display driver
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
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


#include "maru_common.h"
#include "maru_display.h"
#include "debug_ch.h"

#ifndef CONFIG_USE_SHM
#include "maru_sdl.h"
#else
#include "maru_shm.h"
#endif


MULTI_DEBUG_CHANNEL(tizen, display);

MaruScreenshot* maru_screenshot = NULL;

//TODO: interface
void maru_display_init(DisplayState *ds)
{
    INFO("init qemu display\n");

    /*  graphics context information */
    DisplayChangeListener *dcl;

    dcl = g_malloc0(sizeof(DisplayChangeListener));
    dcl->ops = &maru_dcl_ops;
    register_displaychangelistener(dcl);

    maru_screenshot = g_malloc0(sizeof(MaruScreenshot));
    maru_screenshot->pixel_data = NULL;
    maru_screenshot->request_screenshot = 0;
    maru_screenshot->isReady = 0;
}

void maru_display_fini(void)
{
    INFO("fini qemu display\n");

    g_free(maru_screenshot);

#ifndef CONFIG_USE_SHM
    maruskin_sdl_quit();
#else
    maruskin_shm_quit();
#endif
}

void maru_display_update(void)
{
#ifndef CONFIG_USE_SHM
    maruskin_sdl_update();
#else
    /* do nothing */
#endif
}

void maru_display_invalidate(bool on)
{
#ifndef CONFIG_USE_SHM
    maruskin_sdl_invalidate(on);
#else
    /* do nothing */
#endif
}

void maru_display_interpolation(bool on)
{
#ifndef CONFIG_USE_SHM
    maruskin_sdl_interpolation(on);
#else
    /* do nothing */
#endif
}

void maruskin_init(uint64 swt_handle,
    unsigned int display_width, unsigned int display_height,
    bool blank_guide)
{
#ifndef CONFIG_USE_SHM
    maruskin_sdl_init(swt_handle,
        display_width, display_height, blank_guide);
#else
    maruskin_shm_init(swt_handle,
        display_width, display_height, blank_guide);
#endif
}

MaruScreenshot *get_maru_screenshot(void)
{
    return maru_screenshot;
}
/* set_maru_screenshot() implemented in maruskin_operation.c */

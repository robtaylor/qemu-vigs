/*
 * Shared memory
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


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "qemu/main-loop.h"
#include "emul_state.h"
#include "maru_display.h"
#include "maru_shm.h"
#include "hw/pci/maru_brightness.h"
#include "skin/maruskin_server.h"
#include "util/maru_err_table.h"
#include "debug_ch.h"

#include "tethering/touch.h"

MULTI_DEBUG_CHANNEL(tizen, maru_shm);

static QEMUBH *shm_update_bh;

static DisplaySurface *dpy_surface;
static void *shared_memory = (void *) 0;
static int skin_shmid;
static bool is_fit_console_size;

static int shm_skip_update;
static int shm_skip_count;

static bool blank_guide_enable;
static int blank_cnt;
#define MAX_BLANK_FRAME_CNT 10

extern QemuMutex mutex_draw_display;
extern int draw_display_state;

//#define INFO_FRAME_DROP_RATE
#ifdef INFO_FRAME_DROP_RATE
static unsigned int draw_frame;
static unsigned int drop_frame;
#endif

/* Image processing functions using the pixman library */
static void maru_do_pixman_dpy_surface(pixman_image_t *dst_image)
{
    /* apply the brightness level */
    if (brightness_level < BRIGHTNESS_MAX) {
        pixman_image_composite(PIXMAN_OP_OVER,
                               brightness_image, NULL, dst_image,
                               0, 0, 0, 0, 0, 0,
                               pixman_image_get_width(dst_image),
                               pixman_image_get_height(dst_image));
    }
}

static void qemu_ds_shm_update(DisplayChangeListener *dcl,
                               int x, int y, int w, int h)
{
    if (shared_memory != NULL) {
        qemu_mutex_lock(&mutex_draw_display);

        if (draw_display_state == 0) {
            draw_display_state = 1;

            qemu_mutex_unlock(&mutex_draw_display);

            if (is_fit_console_size == true) {
                maru_do_pixman_dpy_surface(dpy_surface->image);

                save_screenshot(dpy_surface);

                memcpy(shared_memory,
                    surface_data(dpy_surface),
                    surface_stride(dpy_surface) *
                    surface_height(dpy_surface));
            } else {
                int shm_size =
                   get_emul_resolution_width() * get_emul_resolution_height() * 4;
                memset(shared_memory, 0x00, (size_t)shm_size);
            }

#ifdef INFO_FRAME_DROP_RATE
            draw_frame++;
#endif
            notify_draw_frame();
        } else {
#ifdef INFO_FRAME_DROP_RATE
            drop_frame++;
#endif
            qemu_mutex_unlock(&mutex_draw_display);
        }

#ifdef INFO_FRAME_DROP_RATE
        INFO("! frame drop rate = (%d/%d)\n",
             drop_frame, draw_frame + drop_frame);
#endif
    }

    set_display_dirty(true);
}

static void qemu_ds_shm_switch(DisplayChangeListener *dcl,
                        struct DisplaySurface *new_surface)
{
    int console_width = 0, console_height = 0;

    shm_skip_update = 0;
    shm_skip_count = 0;

    if (!new_surface) {
        ERR("qemu_ds_shm_switch : new_surface is NULL\n");
        return;
    }

    dpy_surface = new_surface;
    console_width = surface_width(new_surface);
    console_height = surface_height(new_surface);

    INFO("qemu_ds_shm_switch : (%d, %d)\n",
        console_width, console_height);

    if (console_width == get_emul_resolution_width() &&
        console_height == get_emul_resolution_height()) {
        is_fit_console_size = true;
    }
}

static void qemu_ds_shm_refresh(DisplayChangeListener *dcl)
{
    /* If the display is turned off,
    the screen does not update until the it is turned on */
    if (shm_skip_update && display_off) {
        if (blank_cnt > MAX_BLANK_FRAME_CNT) {
            /* do nothing */
            return;
        } else if (blank_cnt == MAX_BLANK_FRAME_CNT) {
            if (blank_guide_enable == true) {
                INFO("draw a blank guide image\n");

                if (get_emul_skin_enable()) {
                    /* draw guide image */
                    notify_draw_blank_guide();
                }
            }
        } else if (blank_cnt == 0) {
            INFO("skipping of the display updating is started\n");
        }

        blank_cnt++;

        return;
    } else {
        if (blank_cnt != 0) {
            INFO("skipping of the display updating is ended\n");
            blank_cnt = 0;
        }
    }

    graphic_hw_update(NULL);

    /* Usually, continuously updated.
    But when the display is turned off,
    ten more updates the surface for a black screen. */
    if (display_off) {
        if (++shm_skip_count > 10) {
            shm_skip_update = 1;
        } else {
            shm_skip_update = 0;
        }
    } else {
        shm_skip_count = 0;
        shm_skip_update = 0;
    }
}

static void maru_shm_update_bh(void *opaque)
{
    graphic_hw_invalidate(NULL);
}

DisplayChangeListenerOps maru_dcl_ops = {
    .dpy_name          = "maru_shm",
    .dpy_refresh       = qemu_ds_shm_refresh,
    .dpy_gfx_update    = qemu_ds_shm_update,
    .dpy_gfx_switch    = qemu_ds_shm_switch,
};

void maru_shm_pre_init(void)
{
    shm_update_bh = qemu_bh_new(maru_shm_update_bh, NULL);
}

void maru_shm_init(uint64 swt_handle,
    unsigned int display_width, unsigned int display_height,
    bool blank_guide)
{
    blank_guide_enable = blank_guide;

    INFO("maru shm init\n");

    set_emul_resolution(display_width, display_height);
    set_emul_sdl_bpp(32);

    if (blank_guide_enable == true) {
        INFO("blank guide is on\n");
    }

    /* byte */
    int shm_size =
        get_emul_resolution_width() * get_emul_resolution_height() * 4;

    /* base + 1 = sdb port */
    /* base + 2 = shared memory key */
    int mykey = get_emul_vm_base_port() + 2;

    INFO("shared memory key: %d, size: %d bytes\n", mykey, shm_size);

    /* make a shared framebuffer */
    skin_shmid = shmget((key_t)mykey, (size_t)shm_size, 0666 | IPC_CREAT);
    if (skin_shmid == -1) {
        ERR("shmget failed\n");
        perror("maru_vga: ");

        maru_register_exit_msg(MARU_EXIT_UNKNOWN,
            (char*) "Cannot launch this VM.\n"
            "Failed to get identifier of the shared memory segment.");
        exit(1);
    }

    shared_memory = shmat(skin_shmid, (void*)0, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        perror("maru_vga: ");

        maru_register_exit_msg(MARU_EXIT_UNKNOWN,
            (char*) "Cannot launch this VM.\n"
            "Failed to attach the shared memory segment.");
        exit(1);
    }

    /* default screen */
    memset(shared_memory, 0x00, (size_t)shm_size);
    INFO("Memory attached at 0x%X\n", (int)shared_memory);
}

void maru_shm_quit(void)
{
    struct shmid_ds shm_info;

    INFO("maru shm quit\n");

    if (shmctl(skin_shmid, IPC_STAT, &shm_info) == -1) {
        ERR("shmctl failed\n");
        shm_info.shm_nattch = -1;
    }

    if (shmdt(shared_memory) == -1) {
        ERR("shmdt failed\n");
        perror("maru_vga: ");
        return;
    }
    shared_memory = NULL;

    if (shm_info.shm_nattch == 1) {
        /* remove */
        if (shmctl(skin_shmid, IPC_RMID, 0) == -1) {
            INFO("segment was already removed\n");
            perror("maru_vga: ");
        } else {
            INFO("shared memory was removed\n");
        }
    } else if (shm_info.shm_nattch != -1) {
        INFO("number of current attaches = %d\n",
            (int)shm_info.shm_nattch);
    }
}

void maru_shm_resize(void)
{
    shm_skip_update = 0;
}

void maru_shm_update(void)
{
    if (shm_update_bh != NULL) {
        qemu_bh_schedule(shm_update_bh);
    }
}

bool maru_extract_framebuffer(void *buffer)
{
    uint32_t buffer_size = 0;

    if (!buffer) {
        ERR("given buffer is null\n");
        return false;
    }

    maru_do_pixman_dpy_surface(dpy_surface->image);

    buffer_size = surface_stride(dpy_surface) * surface_height(dpy_surface);
    TRACE("extract framebuffer %d\n", buffer_size);

    memcpy(buffer, surface_data(dpy_surface), buffer_size);
    return true;
}

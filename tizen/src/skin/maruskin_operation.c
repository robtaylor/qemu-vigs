/*
 * operation for emulator skin
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#include "qemu/sockets.h"
#include "sysemu/sysemu.h"
#include "hw/sysbus.h"

#include "maru_common.h"
#include "maruskin_operation.h"
#include "hw/maru_brightness.h"
#include "hw/maru_virtio_hwkey.h"
#include "hw/maru_virtio_touchscreen.h"
#include "maru_display.h"
#include "emulator.h"
#include "debug_ch.h"
#include "sdb.h"
#include "mloop_event.h"
#include "emul_state.h"
#include "maruskin_keymap.h"
#include "maruskin_server.h"
#include "maru_display.h"
#include "hw/maru_pm.h"
#include "ecs/ecs.h"

#ifdef CONFIG_HAX
#include "guest_debug.h"

#include "target-i386/hax-i386.h"
#endif

MULTI_DEBUG_CHANNEL(qemu, skin_operation);


#define RESUME_KEY_SEND_INTERVAL 500 /* milli-seconds */
#define CLOSE_POWER_KEY_INTERVAL 1200 /* milli-seconds */
#define DATA_DELIMITER "#" /* in detail info data */
#define TIMEOUT_FOR_SHUTDOWN 10 /* seconds */

static int requested_shutdown_qemu_gracefully = 0;

/* touch values */
static int guest_x, guest_y;
static int pressing_x = -1, pressing_y = -1;
static int pressing_origin_x = -1, pressing_origin_y = -1;

extern pthread_mutex_t mutex_screenshot;
extern pthread_cond_t cond_screenshot;

static void* run_timed_shutdown_thread(void* args);
static void send_to_emuld(const char* request_type,
    int request_size, const char* send_buf, int buf_size);


void start_display(uint64 handle_id,
    unsigned int display_width, unsigned int display_height,
    double scale_factor, short rotation_type, bool blank_guide)
{
    INFO("start display : handle_id=%ld, display size=%dx%d, "
        "scale factor=%f, rotation=%d, blank guide=%d\n",
        (long) handle_id, display_width, display_height,
        scale_factor, rotation_type, blank_guide);

    set_emul_win_scale(scale_factor);
    maru_ds_surface_init(handle_id,
        display_width, display_height, blank_guide);
}

void do_grabbing_enable(bool on)
{
    INFO("skin grabbing enable : %d\n", on);

    maru_display_invalidate(on);
}

void do_mouse_event(int button_type, int event_type,
    int origin_x, int origin_y, int x, int y, int z)
{
    if (brightness_off) {
        if (button_type == 0) {
            INFO("auto mouse release\n");
            virtio_touchscreen_event(0, 0, 0, 0);

            return;
        } else {
            TRACE("reject mouse touch in display off : "
                    "button=%d, type=%d, x=%d, y=%d, z=%d\n",
                    button_type, event_type, x, y, z);
            return;
        }
    }

    TRACE("mouse event : button=%d, type=%d, "
        "host=(%d, %d), x=%d, y=%d, z=%d\n",
        button_type, event_type, origin_x, origin_y, x, y, z);

#ifndef CONFIG_USE_SHM
    /* multi-touch */
    if (get_emul_multi_touch_state()->multitouch_enable == 1) {
        maru_finger_processing_1(event_type, origin_x, origin_y, x, y);

        maru_display_update();
        return;
    } else if (get_emul_multi_touch_state()->multitouch_enable == 2) {
        maru_finger_processing_2(event_type, origin_x, origin_y, x, y);

        maru_display_update();
        return;
    }
#endif

    /* single touch */
    switch(event_type) {
        case MOUSE_DOWN:
        case MOUSE_DRAG:
            pressing_x = guest_x = x;
            pressing_y = guest_y = y;
            pressing_origin_x = origin_x;
            pressing_origin_y = origin_y;

            virtio_touchscreen_event(x, y, z, 1);

            break;
        case MOUSE_UP:
            guest_x = x;
            guest_y = y;
            pressing_x = pressing_y = -1;
            pressing_origin_x = pressing_origin_y = -1;

            virtio_touchscreen_event(x, y, z, 0);

            break;
        case MOUSE_WHEELUP:
        case MOUSE_WHEELDOWN:
            if (is_emul_input_touch_enable() == true) {
                x -= guest_x;
                y -= guest_y;
                guest_x += x;
                guest_y += y;
            } else {
                guest_x = x;
                guest_y = y;
            }

            virtio_touchscreen_event(x, y, -z, event_type);

            break;
        case MOUSE_MOVE:
            guest_x = x;
            guest_y = y;

            virtio_touchscreen_event(x, y, z, event_type);

            break;
        default:
            ERR("undefined mouse event type passed : %d\n", event_type);
            break;
    }
}

void do_keyboard_key_event(int event_type,
    int keycode, int state_mask, int key_location)
{
    int scancode = -1;

    TRACE("Keyboard Key : event_type=%d, keycode=%d,"
        "state_mask=%d, key_location=%d\n",
        event_type, keycode, state_mask, key_location);

#ifndef CONFIG_USE_SHM
    if (get_emul_max_touch_point() > 1) {
        /* multi-touch checking */
        int state_mask_temp = state_mask & ~JAVA_KEYCODE_NO_FOCUS;

        if ((keycode == JAVA_KEYCODE_BIT_SHIFT &&
            state_mask_temp == JAVA_KEYCODE_BIT_CTRL) ||
            (keycode == JAVA_KEYCODE_BIT_CTRL &&
            state_mask_temp == JAVA_KEYCODE_BIT_SHIFT))
        {
            if (KEY_PRESSED == event_type) {
                get_emul_multi_touch_state()->multitouch_enable = 2;

                /* add a finger before start the multi-touch processing
                if already exist the pressed touch in display */
                if (pressing_x != -1 && pressing_y != -1 &&
                    pressing_origin_x != -1 && pressing_origin_y != -1) {
                    add_finger_point(
                        pressing_origin_x, pressing_origin_y,
                        pressing_x, pressing_y);
                    pressing_x = pressing_y = -1;
                    pressing_origin_x = pressing_origin_y = -1;
                }

                INFO("enable multi-touch = mode 2\n");
            }
        }
        else if (keycode == JAVA_KEYCODE_BIT_CTRL ||
            keycode == JAVA_KEYCODE_BIT_SHIFT)
        {
            if (KEY_PRESSED == event_type) {
                get_emul_multi_touch_state()->multitouch_enable = 1;

                /* add a finger before start the multi-touch processing
                if already exist the pressed touch in display */
                if (pressing_x != -1 && pressing_y != -1 &&
                    pressing_origin_x != -1 && pressing_origin_y != -1) {
                    add_finger_point(
                        pressing_origin_x, pressing_origin_y,
                        pressing_x, pressing_y);
                    pressing_x = pressing_y = -1;
                    pressing_origin_x = pressing_origin_y = -1;
                }

                INFO("enable multi-touch = mode 1\n");
            } else if (KEY_RELEASED == event_type) {
                if (state_mask_temp == (JAVA_KEYCODE_BIT_CTRL | JAVA_KEYCODE_BIT_SHIFT)) {
                    get_emul_multi_touch_state()->multitouch_enable = 1;
                    INFO("enabled multi-touch = mode 1\'\n");
                } else {
                    clear_finger_slot(false);
                    INFO("disable multi-touch\n");
                }

                maru_display_update();
            }
        }

    }
#endif

#if defined(TARGET_I386)
    if (!mloop_evcmd_get_hostkbd_status()) {
        TRACE("ignore keyboard input because usb keyboard is dettached.\n");
        return;
    }
#endif

#if defined(TARGET_ARM)
    if (!mloop_evcmd_get_usbkbd_status()) {
        TRACE("ignore keyboard input because usb keyboard is dettached.\n");
        return;
    }
#endif

    scancode = javakeycode_to_scancode(event_type, keycode, state_mask, key_location);

    if (scancode == -1) {
        INFO("cannot find scancode\n");
        return;
    }

    if (KEY_PRESSED == event_type) {
        TRACE("key pressed: %d\n", scancode);
        virtio_keyboard_event(scancode);
    } else if (KEY_RELEASED == event_type) {
        TRACE("key released: %d\n", scancode);
        virtio_keyboard_event(scancode | 0x80);
    }
}

void do_hw_key_event(int event_type, int keycode)
{
    INFO("HW Key : event_type=%d, keycode=%d\n", event_type, keycode);

    // TODO: remove workaround
    if (runstate_check(RUN_STATE_SUSPENDED)) {
        if (KEY_PRESSED == event_type) {
            /* home key or power key is used for resume */
            if ((HARD_KEY_HOME == keycode) || (HARD_KEY_POWER == keycode)) {
                INFO("user requests system resume\n");
                resume();

#ifdef CONFIG_WIN32
                Sleep(RESUME_KEY_SEND_INTERVAL);
#else
                usleep(RESUME_KEY_SEND_INTERVAL * 1000);
#endif
            }
        }
    }

    maru_hwkey_event(event_type, keycode);
}

void do_scale_event(double scale_factor)
{
    INFO("do_scale_event scale_factor : %lf\n", scale_factor);

    set_emul_win_scale(scale_factor);
}

void do_rotation_event(int rotation_type)
{
    INFO("do_rotation_event rotation_type : %d\n", rotation_type);

#if 1
    int x = 0, y = 0, z = 0;

    switch (rotation_type) {
        case ROTATION_PORTRAIT:
            x = 0;
            y = accel_min_max(9.80665);
            z = 0;
            break;
        case ROTATION_LANDSCAPE:
            x = accel_min_max(9.80665);
            y = 0;
            z = 0;
            break;
        case ROTATION_REVERSE_PORTRAIT:
            x = 0;
            y = accel_min_max(-9.80665);
            z = 0;
            break;
        case ROTATION_REVERSE_LANDSCAPE:
            x = accel_min_max(-9.80665);
            y = 0;
            z = 0;
            break;
        default:
            break;
    }

    req_set_sensor_accel(x, y, z);
#else
    char send_buf[32] = { 0 };

    switch ( rotation_type ) {
        case ROTATION_PORTRAIT:
            sprintf( send_buf, "1\n3\n0\n9.80665\n0\n" );
            break;
        case ROTATION_LANDSCAPE:
            sprintf( send_buf, "1\n3\n9.80665\n0\n0\n" );
            break;
        case ROTATION_REVERSE_PORTRAIT:
            sprintf( send_buf, "1\n3\n0\n-9.80665\n0\n" );
            break;
        case ROTATION_REVERSE_LANDSCAPE:
            sprintf(send_buf, "1\n3\n-9.80665\n0\n0\n");
            break;

        default:
            break;
    }

    send_to_emuld( "sensor\n\n\n\n", 10, send_buf, 32 );
#endif
}

void set_maru_screenshot(DisplaySurface *surface)
{
    pthread_mutex_lock(&mutex_screenshot);

    MaruScreenshot *maru_screenshot = get_maru_screenshot();
    if (maru_screenshot) {
        maru_screenshot->isReady = 1;
        if (maru_screenshot->request_screenshot == 1) {
            memcpy(maru_screenshot->pixel_data,
                   surface_data(surface),
                   surface_stride(surface) *
                   surface_height(surface));
            maru_screenshot->request_screenshot = 0;

            pthread_cond_signal(&cond_screenshot);
        }
    }
    pthread_mutex_unlock(&mutex_screenshot);
}

QemuSurfaceInfo *request_screenshot(void)
{
    const int length = get_emul_resolution_width() * get_emul_resolution_height() * 4;
    INFO("screenshot data length : %d\n", length);

    if (0 >= length) {
        return NULL;
    }

    QemuSurfaceInfo *info = (QemuSurfaceInfo *)g_malloc0(sizeof(QemuSurfaceInfo));
    if (!info) {
        ERR("Fail to malloc for QemuSurfaceInfo.\n");
        return NULL;
    }

    info->pixel_data = (unsigned char *)g_malloc0(length);
    if (!info->pixel_data) {
        g_free(info);
        ERR("Fail to malloc for pixel data.\n");
        return NULL;
    }

    /* If display has been turned off, return empty buffer.
       Because the empty buffer is seen as a black. */
    if (brightness_off == 0) {
        pthread_mutex_lock(&mutex_screenshot);

        MaruScreenshot* maru_screenshot = get_maru_screenshot();
        if (!maru_screenshot || maru_screenshot->isReady != 1) {
            ERR("maru screenshot is NULL or not ready.\n");
            memset(info->pixel_data, 0x00, length);
        } else {
            maru_screenshot->pixel_data = info->pixel_data;
            maru_screenshot->request_screenshot = 1;
            maru_display_update();

            // TODO : do not wait on communication thread
            pthread_cond_wait(&cond_screenshot, &mutex_screenshot);
        }

        pthread_mutex_unlock(&mutex_screenshot);
    }

    info->pixel_data_length = length;

    return info;
}

void free_screenshot_info(QemuSurfaceInfo *info)
{
    if (info) {
        if(info->pixel_data) {
            g_free(info->pixel_data);
        }

        g_free(info);
    }
}

DetailInfo* get_detail_info(int qemu_argc, char** qemu_argv)
{
    DetailInfo* detail_info = g_malloc0( sizeof(DetailInfo) );
    if ( !detail_info ) {
        ERR( "Fail to malloc for DetailInfo.\n" );
        return NULL;
    }

    int i = 0;
    int total_len = 0;
    int delimiter_len = strlen( DATA_DELIMITER );

    /* collect QEMU information */
    for ( i = 0; i < qemu_argc; i++ ) {
        total_len += strlen( qemu_argv[i] );
        total_len += delimiter_len;
    }

#ifdef CONFIG_WIN32
    /* collect HAXM information */
    const int HAX_LEN = 32;
    char hax_error[HAX_LEN];
    memset( hax_error, 0, HAX_LEN );

    int hax_err_len = 0;
    hax_err_len = sprintf( hax_error + hax_err_len, "%s", "hax_error=" );

    int error = 0;
    if ( !ret_hax_init ) {
        if ( -ENOSPC == ret_hax_init ) {
            error = 1;
        }
    }
    hax_err_len += sprintf( hax_error + hax_err_len, "%s#", error ? "true" : "false" );
    total_len += (hax_err_len + 1);
#endif

    /* collect log path information */
#define LOGPATH_TEXT "log_path="
    const char* log_path = get_log_path();
    int log_path_len = strlen(LOGPATH_TEXT) + strlen(log_path) + delimiter_len;
    total_len += (log_path_len + 1);

    /* memory allocation */
    char* info_data = g_malloc0( total_len + 1 );
    if ( !info_data ) {
        g_free( detail_info );
        ERR( "Fail to malloc for info data.\n" );
        return NULL;
    }


    /* write informations */
    int len = 0;
    total_len = 0; //recycle

    for ( i = 0; i < qemu_argc; i++ ) {
        len = strlen( qemu_argv[i] );
        sprintf( info_data + total_len, "%s%s", qemu_argv[i], DATA_DELIMITER );
        total_len += len + delimiter_len;
    }

#ifdef CONFIG_WIN32
    snprintf( info_data + total_len, hax_err_len + 1, "%s#", hax_error );
    total_len += hax_err_len;
#endif

    snprintf( info_data + total_len, log_path_len + 1, "%s%s#", LOGPATH_TEXT, log_path );
    total_len += log_path_len;

    INFO( "################## detail info data ####################\n" );
    INFO( "%s\n", info_data );

    detail_info->data = info_data;
    detail_info->data_length = total_len;

    return detail_info;
}

void free_detail_info(DetailInfo* detail_info)
{
    if (detail_info) {
        if (detail_info->data) {
            g_free(detail_info->data);
        }

        g_free(detail_info);
    }
}

void do_open_shell(void)
{
    INFO("open shell\n");

    /* do nothing */
}

void do_host_kbd_enable(bool on)
{
    INFO("host kbd enable : %d\n", on);

#if defined(TARGET_ARM)
    mloop_evcmd_usbkbd(on);
#elif defined(TARGET_I386)
    mloop_evcmd_hostkbd(on);
#endif
}

void do_interpolation_enable(bool on)
{
    INFO("interpolation enable : %d\n", on);

    maru_display_interpolation(on);
}

void do_ram_dump(void)
{
    INFO("dump ram!\n");

    mloop_evcmd_ramdump();
}


void do_guestmemory_dump(void)
{
    INFO("dump guest memory!\n");

    /* TODO: */
}

void request_close(void)
{
    INFO("request_close\n");

    // TODO: convert to device emulation
    do_hw_key_event(KEY_PRESSED, HARD_KEY_POWER);

#ifdef CONFIG_WIN32
        Sleep(CLOSE_POWER_KEY_INTERVAL);
#else
        usleep(CLOSE_POWER_KEY_INTERVAL * 1000);
#endif

    do_hw_key_event(KEY_RELEASED, HARD_KEY_POWER);
}

void shutdown_qemu_gracefully(void)
{
    if (is_requested_shutdown_qemu_gracefully() != 1) {
        requested_shutdown_qemu_gracefully = 1;

        INFO("shutdown_qemu_gracefully was called\n");
        QemuThread thread_id;
        qemu_thread_create(&thread_id, "shutdown_thread", run_timed_shutdown_thread,
                           NULL, QEMU_THREAD_DETACHED);
    } else {
        INFO("shutdown_qemu_gracefully was called twice\n");
        qemu_system_shutdown_request();
    }
}

int is_requested_shutdown_qemu_gracefully(void)
{
    return requested_shutdown_qemu_gracefully;
}

static void* run_timed_shutdown_thread(void* args)
{
    send_to_emuld("system\n\n\n\n", 10, "shutdown", 8);

    int sleep_interval_time = 1000; /* milli-seconds */

    int i;
    for (i = 0; i < TIMEOUT_FOR_SHUTDOWN; i++) {
#ifdef CONFIG_WIN32
        Sleep(sleep_interval_time);
#else
        usleep(sleep_interval_time * 1000);
#endif
        /* do not use logger to help user see log in console */
        fprintf(stdout, "Wait for shutdown qemu...%d\n", (i + 1));
    }

    INFO("Shutdown qemu !!!\n");

    qemu_system_shutdown_request();

    return NULL;
}

static void send_to_emuld(const char* request_type,
    int request_size, const char* send_buf, int buf_size)
{
    char addr[128];
    int s = 0;
    int device_serial_number = get_device_serial_number();
    snprintf(addr, 128, ":%u", (uint16_t) ( device_serial_number + SDB_TCP_EMULD_INDEX));

    //TODO: Error handling
    s = inet_connect(addr, NULL);

    if ( s < 0 ) {
        ERR( "can't create socket to emulator daemon in guest\n" );
        ERR( "[127.0.0.1:%d/tcp] connect fail (%d:%s)\n" , device_serial_number + SDB_TCP_EMULD_INDEX , errno, strerror(errno) );
        return;
    }

    if(send( s, (char*)request_type, request_size, 0 ) < 0) {
        ERR("failed to send to emuld\n");
    }
    if(send( s, &buf_size, 4, 0 ) < 0) {
        ERR("failed to send to emuld\n");
    }
    if(send( s, (char*)send_buf, buf_size, 0 ) < 0) {
        ERR("failed to send to emuld\n");
    }

    INFO( "send to emuld [req_type:%s, send_data:%s, send_size:%d] 127.0.0.1:%d/tcp \n",
            request_type, send_buf, buf_size, device_serial_number + SDB_TCP_EMULD_INDEX );

#ifdef CONFIG_WIN32
    closesocket( s );
#else
    close( s );
#endif

}

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

#include "qemu/main-loop.h"
#include "qemu/config-file.h"
#include "hw/qdev.h"
#include "monitor/qdev.h"
#include "fsdev/qemu-fsdev.h"
#include "qmp-commands.h"
#include "sysemu/blockdev.h"
#include "qemu/event_notifier.h"

#include "emulator.h"
#include "maru_device_hotplug.h"

#define HOST_KEYBOARD_DRIVER        "virtio-keyboard-pci"
#define HOST_KEYBOARD_DEFAULT_ID    "HOSTKBD0"

#define SDCARD_DRIVE_DEFAULT_ID     "SDCARD_DRIVE0"
#define SDCARD_DRIVER               "virtio-blk-pci"
#define SDCARD_DEFAULT_ID           "SDCARD0"

#define FS_MOUNT_TAG                "fileshare"

struct maru_device_hotplug {
    EventNotifier notifier;

    char *opaque;
    int command;

    // FIXME: Should we query device every time ??
    bool host_keyboard_attached;
    bool sdcard_attached;
    bool hds_attached;
};

static struct maru_device_hotplug *state;

static bool do_host_keyboard_attach(void)
{
    QDict *qdict = qdict_new();
    qdict_put(qdict, "driver", qstring_from_str(HOST_KEYBOARD_DRIVER));
    qdict_put(qdict, "id", qstring_from_str(HOST_KEYBOARD_DEFAULT_ID));

    if (do_device_add(default_mon, qdict, NULL)) {
        QDECREF(qdict);
        // TODO error reporting
        return false;
    }

    QDECREF(qdict);

    state->host_keyboard_attached = true;

    return true;
}

static bool do_host_keyboard_detach(void)
{
    QDict *qdict = qdict_new();
    qdict_put(qdict, "id", qstring_from_str(HOST_KEYBOARD_DEFAULT_ID));

    if (qmp_marshal_input_device_del(default_mon, qdict, NULL)) {
        QDECREF(qdict);
        // TODO error reporting
        return false;
    }

    QDECREF(qdict);

    state->host_keyboard_attached = false;

    return true;
}

static bool do_sdcard_attach(const char * const file)
{
    QDict *qdict = qdict_new();
    QDict *qdict_file = qdict_new();
    QDict *qdict_options = qdict_new();

    qdict_put(qdict_file, "driver", qstring_from_str("file"));
    qdict_put(qdict_file, "filename", qstring_from_str(file));
    qdict_put(qdict_options, "file", qdict_file);
    qdict_put(qdict_options, "driver", qstring_from_str("qcow2"));
    qdict_put(qdict_options, "id", qstring_from_str(SDCARD_DRIVE_DEFAULT_ID));
    qdict_put(qdict, "options", qdict_options);

    if (qmp_marshal_input_blockdev_add(default_mon, qdict, NULL)) {
        QDECREF(qdict);
    }

    QDECREF(qdict);

    qdict = qdict_new();
    qdict_put(qdict, "driver", qstring_from_str(SDCARD_DRIVER));
    qdict_put(qdict, "drive", qstring_from_str(SDCARD_DRIVE_DEFAULT_ID));
    qdict_put(qdict, "id", qstring_from_str(SDCARD_DEFAULT_ID));

    if (do_device_add(default_mon, qdict, NULL)) {
        QDECREF(qdict);
        // TODO error reporting
        return false;
    }

    QDECREF(qdict);

    state->sdcard_attached = true;

    return true;
}

static bool do_sdcard_detach(void) {
    QDict *qdict = qdict_new();
    qdict_put(qdict, "id", qstring_from_str(SDCARD_DEFAULT_ID));

    if (qmp_marshal_input_device_del(cur_mon, qdict, NULL)) {
        QDECREF(qdict);
        // TODO error reporting
        return false;
    }

    QDECREF(qdict);

    state->sdcard_attached = false;

    return true;
}

static bool do_hds_attach(const char * const file)
{
    QemuOpts *fsdev;
    int ret;
    QDict *qdict = qdict_new();

    fsdev = qemu_opts_create(qemu_find_opts("fsdev"),
                             FS_MOUNT_TAG, 0, NULL);
    if (!fsdev) {
        return false;
    }

    qemu_opt_set(fsdev, "fsdriver", "local");
    qemu_opt_set(fsdev, "path", file);
    qemu_opt_set(fsdev, "security_model", "none");

    ret = qemu_fsdev_add(fsdev);
    if (ret != 0) {
        return false;
    }

    qdict = qdict_new();
    qdict_put(qdict, "driver", qstring_from_str("virtio-9p-pci"));
    qdict_put(qdict, "fsdev", qstring_from_str(FS_MOUNT_TAG));
    qdict_put(qdict, "mount_tag", qstring_from_str(FS_MOUNT_TAG));
    qdict_put(qdict, "id", qstring_from_str(FS_MOUNT_TAG));

    if (do_device_add(default_mon, qdict, NULL)) {
        QDECREF(qdict);
        return false;
    }

    QDECREF(qdict);

    state->hds_attached = true;

    return true;
}

static bool do_hds_detach(void)
{
    QDict *qdict = qdict_new();
    qemu_fsdev_remove(FS_MOUNT_TAG);

    qdict_put(qdict, "id", qstring_from_str(FS_MOUNT_TAG));

    if (qmp_marshal_input_device_del(cur_mon, qdict, NULL)) {
        QDECREF(qdict);
        return false;
    }

    QDECREF(qdict);

    state->hds_attached = false;

    return true;
}

void do_hotplug(int command, void *opaque, size_t size)
{
    if (command == ATTACH_SDCARD) {
        state->opaque = g_malloc(size);
        memcpy(state->opaque, opaque, size);
    }
    state->command = command;

    event_notifier_set(&state->notifier);
}

static void device_hotplug_handler(EventNotifier *e)
{
    event_notifier_test_and_clear(e);

    switch(state->command) {
    case ATTACH_HOST_KEYBOARD:
        do_host_keyboard_attach();
        break;
    case DETACH_HOST_KEYBOARD:
        do_host_keyboard_detach();
        break;
    case ATTACH_SDCARD:
        do_sdcard_attach(state->opaque);
        g_free(state->opaque);
        break;
    case DETACH_SDCARD:
        do_sdcard_detach();
        break;
    case ATTACH_HDS:
        do_hds_attach(state->opaque);
        break;
    case DETACH_HDS:
        do_hds_detach();
        break;
    default:
        break;
    }
}

static void maru_device_hotplug_deinit(Notifier *notifier, void *data)
{
    event_notifier_cleanup(&state->notifier);

    g_free(state);
}

static Notifier maru_device_hotplug_exit = { .notify = maru_device_hotplug_deinit };

void maru_device_hotplug_init(void)
{
    state = g_malloc0(sizeof(struct maru_device_hotplug));

    event_notifier_init(&state->notifier, 0);
    event_notifier_set_handler(&state->notifier, device_hotplug_handler);

    emulator_add_exit_notifier(&maru_device_hotplug_exit);
}

bool is_host_keyboard_attached(void)
{
    return state->host_keyboard_attached;
}

bool is_sdcard_attached(void)
{
    return state->sdcard_attached;
}

bool is_hds_attached(void)
{
    return state->hds_attached;
}


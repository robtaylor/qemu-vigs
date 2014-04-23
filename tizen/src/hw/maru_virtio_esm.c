/*
 * Virtio EmulatorStatusMedium Device
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 *  SeokYeon Hwang <syeon.hwang@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "maru_device_ids.h"
#include "maru_virtio_esm.h"
#include "skin/maruskin_server.h"
#include "emul_state.h"
#include "debug_ch.h"

MULTI_DEBUG_CHANNEL(qemu, virtio-esm);


#define SYSTEM_MODE_LAYER 1
#define USER_MODE_LAYER 0
static uint8_t boot_complete;

struct progress_info {
    char mode;
    uint16_t percentage;
};

static VirtQueueElement elem;
struct progress_info progress;

static void virtio_esm_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOESM *vesm = VIRTIO_ESM(vdev);
    int index = 0;

    TRACE("virtqueue handler.\n");
    if (virtio_queue_empty(vesm->vq)) {
        INFO("virtqueue is empty.\n");
        return;
    }

    // Get a queue buffer.
    index = virtqueue_pop(vq, &elem);
    TRACE("virtqueue pop. index: %d\n", index);

    TRACE("virtio element out number : %d\n", elem.out_num);
    if (elem.out_num != 1) {
        ERR("virtio element out number is wierd.\n");
    }
    else {
        TRACE("caramis elem.out_sg[0].iov_len : %x\n", elem.out_sg[0].iov_len);
        TRACE("caramis elem.out_sg[0].iov_base : %x\n", elem.out_sg[0].iov_base);
        if (elem.out_sg[0].iov_len != 4) {
            ERR("out lenth is wierd.\n");
        }
        else {
            progress = *((struct progress_info*)elem.out_sg[0].iov_base);
            TRACE("Boot up progress is [%u] percent done at %s.\n",
                progress.percentage,
                progress.mode == 's' || progress.mode == 'S' ? "system mode" : "user mode");

            /* notify to skin */
            if (progress.mode == 's' || progress.mode == 'S') {
                if (progress.percentage >= 100) {
                    boot_complete |= (1 << SYSTEM_MODE_LAYER);
                }

                notify_booting_progress(SYSTEM_MODE_LAYER, progress.percentage);
            } else {
                if (progress.percentage >= 100) {
                    boot_complete |= (1 << USER_MODE_LAYER);
                }

                notify_booting_progress(USER_MODE_LAYER, progress.percentage);
            }

            /* booting complete check */
            if ((boot_complete & (1 << SYSTEM_MODE_LAYER)) &&
                (boot_complete & (1 << USER_MODE_LAYER))) {
                set_emulator_condition(BOOT_COMPLETED);
            }
        }
    }

    // There is no data to copy into guest.
    virtqueue_push(vesm->vq, &elem, 0);
    virtio_notify(&vesm->vdev, vesm->vq);
}

static uint32_t virtio_esm_get_features(VirtIODevice *vdev, uint32_t feature)
{
    TRACE("virtio_esm_get_features.\n");
    return feature;
}

static void virtio_esm_reset(VirtIODevice* vdev)
{
    TRACE("virtio_esm_reset.\n");

    progress.mode = '\0';
    progress.percentage = 0;
}


static void virtio_esm_device_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOESM *vesm = VIRTIO_ESM(dev);

    INFO("initialize virtio-esm device\n");
    virtio_init(vdev, "virtio-esm", VIRTIO_ID_ESM, 0);

    vesm->vq = virtio_add_queue(vdev, 1, virtio_esm_handle);

    virtio_esm_reset(vdev);
}

static void virtio_esm_device_unrealize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);

    INFO("destroy device\n");
    virtio_cleanup(vdev);
}

static void virtio_esm_class_init(ObjectClass *klass, void *data)
{
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->realize = virtio_esm_device_realize;
    vdc->unrealize = virtio_esm_device_unrealize;
    vdc->get_features = virtio_esm_get_features;
    // This device is no need to reset.
    //vdc->reset = virtio_esm_reset;
}

static const TypeInfo virtio_device_info = {
    .name = TYPE_VIRTIO_ESM,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOESM),
    .class_init = virtio_esm_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_device_info);
}

type_init(virtio_register_types)

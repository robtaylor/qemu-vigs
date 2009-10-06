/*
 * QEMU IDE Emulation: ISA Bus support.
 *
 * Copyright (c) 2003 Fabrice Bellard
 * Copyright (c) 2006 Openedhand Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <hw/hw.h>
#include <hw/pc.h>
#include <hw/isa.h>
#include "block.h"
#include "block_int.h"
#include "sysemu.h"
#include "dma.h"

#include <hw/ide/internal.h>

/***********************************************************/
/* ISA IDE definitions */

typedef struct ISAIDEState {
    ISADevice dev;
    IDEBus    bus;
    uint32_t  iobase;
    uint32_t  iobase2;
    uint32_t  isairq;
    qemu_irq  irq;
} ISAIDEState;

static void isa_ide_save(QEMUFile* f, void *opaque)
{
    ISAIDEState *s = opaque;

    idebus_save(f, &s->bus);
    ide_save(f, &s->bus.ifs[0]);
    ide_save(f, &s->bus.ifs[1]);
}

static int isa_ide_load(QEMUFile* f, void *opaque, int version_id)
{
    ISAIDEState *s = opaque;

    idebus_load(f, &s->bus, version_id);
    ide_load(f, &s->bus.ifs[0], version_id);
    ide_load(f, &s->bus.ifs[1], version_id);
    return 0;
}

static int isa_ide_initfn(ISADevice *dev)
{
    ISAIDEState *s = DO_UPCAST(ISAIDEState, dev, dev);

    ide_bus_new(&s->bus, &s->dev.qdev);
    ide_init_ioport(&s->bus, s->iobase, s->iobase2);
    isa_init_irq(dev, &s->irq, s->isairq);
    ide_init2(&s->bus, NULL, NULL, s->irq);
    register_savevm("isa-ide", 0, 3, isa_ide_save, isa_ide_load, s);
    return 0;
};

int isa_ide_init(int iobase, int iobase2, int isairq,
                 DriveInfo *hd0, DriveInfo *hd1)
{
    ISADevice *dev;
    ISAIDEState *s;

    dev = isa_create("isa-ide");
    qdev_prop_set_uint32(&dev->qdev, "iobase",  iobase);
    qdev_prop_set_uint32(&dev->qdev, "iobase2", iobase2);
    qdev_prop_set_uint32(&dev->qdev, "irq",     isairq);
    if (qdev_init(&dev->qdev) < 0)
        return -1;

    s = DO_UPCAST(ISAIDEState, dev, dev);
    if (hd0)
        ide_create_drive(&s->bus, 0, hd0);
    if (hd1)
        ide_create_drive(&s->bus, 1, hd1);
    return 0;
}

static ISADeviceInfo isa_ide_info = {
    .qdev.name  = "isa-ide",
    .qdev.size  = sizeof(ISAIDEState),
    .init       = isa_ide_initfn,
    .qdev.props = (Property[]) {
        DEFINE_PROP_HEX32("iobase",  ISAIDEState, iobase,  0x1f0),
        DEFINE_PROP_HEX32("iobase2", ISAIDEState, iobase2, 0x3f6),
        DEFINE_PROP_UINT32("irq",    ISAIDEState, isairq,  14),
        DEFINE_PROP_END_OF_LIST(),
    },
};

static void isa_ide_register_devices(void)
{
    isa_qdev_register(&isa_ide_info);
}

device_init(isa_ide_register_devices)

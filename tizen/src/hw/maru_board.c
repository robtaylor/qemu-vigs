/*
 * TIZEN base board
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * SangJin Kim <sangjin3.kim@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * JiHye Kim <jihye1128.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * DongKyun Yun
 * DoHyung Hong
 * Hyunjun Son
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
 * x86 board from pc_piix.c...
 * add some TIZEN-speciaized device...
 */

#include <glib.h>

#include "hw/hw.h"
#include "hw/i386/pc.h"
#include "hw/i386/apic.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_ids.h"
#include "hw/usb.h"
#include "net/net.h"
#include "hw/boards.h"
#include "hw/ide.h"
#include "sysemu/kvm.h"
#include "hw/kvm/clock.h"
#include "sysemu/sysemu.h"
#include "hw/sysbus.h"
#include "hw/cpu/icc_bus.h"
#include "sysemu/arch_init.h"
#include "sysemu/blockdev.h"
#include "hw/i2c/smbus.h"
#include "hw/xen/xen.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "hw/acpi/acpi.h"
#include "cpu.h"
#ifdef CONFIG_XEN
#  include <xen/hvm/hvm_info_table.h>
#endif

#include "maru_common.h"
#include "guest_debug.h"
#include "maru_pm.h"

#define MAX_IDE_BUS 2

static const int ide_iobase[MAX_IDE_BUS] = { 0x1f0, 0x170 };
static const int ide_iobase2[MAX_IDE_BUS] = { 0x3f6, 0x376 };
static const int ide_irq[MAX_IDE_BUS] = { 14, 15 };

/* maru specialized device init */
static void maru_device_init(void)
{
    // do nothing for now...
}

extern void maru_pc_init_pci(QEMUMachineInitArgs *args);
static void maru_x86_board_init(QEMUMachineInitArgs *args)
{
    maru_pc_init_pci(args);

    maru_device_init();
}

static QEMUMachine maru_x86_machine = {
    PC_DEFAULT_MACHINE_OPTIONS,
    .name = "maru-x86-machine",
    .alias = "maru-x86-machine",
    .desc = "Maru Board (x86)",
    .init = maru_x86_board_init,
    .hot_add_cpu = pc_hot_add_cpu,
    .no_parallel = 1,
    .no_floppy = 1,
    .no_cdrom = 1,
    .no_sdcard = 1,
    .default_machine_opts = "firmware=bios-256k.bin",
    .default_boot_order = "c",
};

static void maru_machine_init(void)
{
    qemu_register_machine(&maru_x86_machine);
}

machine_init(maru_machine_init);

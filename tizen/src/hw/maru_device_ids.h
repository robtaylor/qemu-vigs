/*
 * Maru Device IDs
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * SangJin Kim <sangjin3.kim@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * JiHye Kim <jihye1128.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * DaiYoung Kim <daiyoung777.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 */


#ifndef MARU_DEVICE_IDS_H_
#define MARU_DEVICE_IDS_H_


/* PCI */
#define PCI_VENDOR_ID_TIZEN              0xC9B5
#define PCI_DEVICE_ID_VIRTUAL_BRIGHTNESS 0x1014
#define PCI_DEVICE_ID_VIRTUAL_CAMERA     0x1018
#define PCI_DEVICE_ID_VIRTUAL_CODEC      0x101C
/* Device ID 0x1000 through 0x103F inclusive is a virtio device */
#define PCI_DEVICE_ID_VIRTIO_TOUCHSCREEN 0x101D
#define PCI_DEVICE_ID_VIRTIO_KEYBOARD    0x1020
#define PCI_DEVICE_ID_VIRTIO_ESM         0x1024
#define PCI_DEVICE_ID_VIRTIO_HWKEY       0x1028
#define PCI_DEVICE_ID_VIRTIO_EVDI        0x102C
#define PCI_DEVICE_ID_VIRTIO_SENSOR      0x1034
#define PCI_DEVICE_ID_VIRTIO_POWER       0x1035
#define PCI_DEVICE_ID_VIRTIO_JACK        0x1036
#define PCI_DEVICE_ID_VIRTIO_NFC         0x1038
#define PCI_DEVICE_ID_VIRTIO_VMODEM      0x103C
#define PCI_DEVICE_ID_VIRTUAL_BRILL_CODEC  0x1040

/* Virtio */
/*
+----------------------+--------------------+---------------+
| Subsystem Device ID  |   Virtio Device    | Specification |
+----------------------+--------------------+---------------+
+----------------------+--------------------+---------------+
|          1           |   network card     |  Appendix C   |
+----------------------+--------------------+---------------+
|          2           |   block device     |  Appendix D   |
+----------------------+--------------------+---------------+
|          3           |      console       |  Appendix E   |
+----------------------+--------------------+---------------+
|          4           |  entropy source    |  Appendix F   |
+----------------------+--------------------+---------------+
|          5           | memory ballooning  |  Appendix G   |
+----------------------+--------------------+---------------+
|          6           |     ioMemory       |       -       |
+----------------------+--------------------+---------------+
|          7           |       rpmsg        |  Appendix H   |
+----------------------+--------------------+---------------+
|          8           |     SCSI host      |  Appendix I   |
+----------------------+--------------------+---------------+
|          9           |   9P transport     |       -       |
+----------------------+--------------------+---------------+
|         10           |   mac80211 wlan    |       -       |
+----------------------+--------------------+---------------+
*/

#define VIRTIO_ID_TOUCHSCREEN   31
#define VIRTIO_ID_KEYBOARD      32
#define VIRTIO_ID_ESM           33
#define VIRTIO_ID_HWKEY         34
#define VIRTIO_ID_EVDI          35
#define VIRTIO_ID_SENSOR        37
#define VIRTIO_ID_NFC           38
#define VIRTIO_ID_JACK          39
#define VIRTIO_ID_POWER         40
#define VIRTIO_ID_VMODEM        41

#endif /* MARU_DEVICE_IDS_H_ */

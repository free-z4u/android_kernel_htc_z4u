/* linux/arch/arm/mach-sc/include/mach/__memlayout-cp5dug.h
 * Copyright (C) 2013 HTC Corporation.
 * Author: Yili Xie <yili_xie@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/


#ifndef __MEMLAYOUT_CP5DUG_H__
#define __MEMLAYOUT_CP5DUG_H__


/*
 * SPL remap the PA, so PA is continous.
 * Don't care the CS connection
 *
 *
 *                        0xC0000000
 *   --------------------
 *   | RC      |  1MB   |
 *   --------------------
 *   | FB      |  6MB   |
 *   --------------------
 *   | ION     |  124MB |
 *   --------------------
 *   | LINU    |  593MB | 575MB if sm
 *   --------------------
 *   | W-M     |  46MB  | 64MB if SM
 *   --------------------
 *   | LINUX   |  104MB | 89MB if SM
 *   --------------------
 *   | TD      |  28MB  | 43MB if SM
 *   --------------------
 *   | LINUX   |  122MB |
 *   -------------------- 0x80000000
 *
 * */

#include <asm/sizes.h>

#define PROJECT_SPECIAL_LAYOUT

#define LINUX_BANK_NUM            3

#define LINUX_BASE_ADDR1		0x80000000
#define LINUX_SIZE_ADDR1		0x07A00000  /* (122 * 1024 * 1024) */
#define LINUX_VIRT_ADDR1		(PAGE_OFFSET)

/* without SM log */
#define TD_BASE_ADDR			0x87A00000
#define TD_SIZE_ADDR			0x01C00000  /* (28 * 1024 * 1024) */
#define TD_VIRT_ADDR			(LINUX_VIRT_ADDR1 + LINUX_SIZE_ADDR1)

#define LINUX_BASE_ADDR2		0x89600000
#define LINUX_SIZE_ADDR2		0x06800000  /* (104 * 1024 * 1024) */
#define LINUX_VIRT_ADDR2		(TD_VIRT_ADDR + TD_SIZE_ADDR)

#define W_BASE_ADDR				0x8FE00000
#define W_SIZE_ADDR				0x02E00000  /* (46 * 1024 * 1024) */
#define W_VIRT_ADDR				(LINUX_VIRT_ADDR2 + LINUX_SIZE_ADDR2)

#define LINUX_BASE_ADDR3		0x92C00000
#define LINUX_SIZE_ADDR3		0x25100000  /* (593 * 1024 * 1024) */
#define LINUX_VIRT_ADDR3		(W_VIRT_ADDR + W_SIZE_ADDR)

/* with SM log */
#define TDS_BASE_ADDR			0x87A00000
#define TDS_SIZE_ADDR			0x02B00000  /* (43 * 1024 * 1024) */
#define TDS_VIRT_ADDR			(LINUX_VIRT_ADDR1 + LINUX_SIZE_ADDR1)

#define LINUXS_BASE_ADDR2		0x8A500000
#define LINUXS_SIZE_ADDR2		0x05900000  /* (89 * 1024 * 1024) */
#define LINUXS_VIRT_ADDR2		(TDS_VIRT_ADDR + TDS_SIZE_ADDR)

#define WS_BASE_ADDR			0x8FE00000
#define WS_SIZE_ADDR			0x04000000  /* (64 * 1024 * 1024) */
#define WS_VIRT_ADDR			(LINUXS_VIRT_ADDR2 + LINUXS_SIZE_ADDR2)

#define LINUXS_BASE_ADDR3		0x93E00000
#define LINUXS_SIZE_ADDR3		0x23F00000  /* (575 * 1024 * 1024) */
#define LINUXS_VIRT_ADDR3		(WS_VIRT_ADDR + WS_SIZE_ADDR)

/* The virtual addr of following sections are same no matter SM log */
#define ION_BASE_ADDR			0xB7D00000
#define ION_SIZE_ADDR			0x07C00000  /* (124 * 1024 * 1024) */
#define ION_VIRT_ADDR			(LINUX_VIRT_ADDR3 + LINUX_SIZE_ADDR3)

#define FB_BASE_ADDR			0xBF900000
#define FB_SIZE_ADDR			0x00600000  /* (6 * 1024 * 1024) */
#define FB_VIRT_ADDR			(ION_VIRT_ADDR + ION_SIZE_ADDR)

#define RC_BASE_ADDR			0xBFF00000
#define RC_SIZE_ADDR			0x00100000  /* (1 * 1024 * 1024) */
#define RC_VIRT_ADDR			(FB_VIRT_ADDR + FB_SIZE_ADDR)

#define ELF_HEADER_ADDR			0xBF400000
#define ELF_HEADER_SIZE			0x00100000  /* (1 * 1024 * 1024) */

#ifdef CONFIG_ION
#define SPRD_ION_OVERLAY_SIZE		(CONFIG_SPRD_ION_OVERLAY_SIZE * SZ_1M)
#define SPRD_ION_SIZE				(ION_SIZE_ADDR - SPRD_ION_OVERLAY_SIZE)
#else
#define SPRD_ION_OVERLAY_SIZE		(0 * SZ_1M)
#define SPRD_ION_SIZE			(0 * SZ_1M)
#endif

#define SPRD_IO_MEM_SIZE		(ION_SIZE_ADDR)
#define SPRD_IO_MEM_BASE		(ION_BASE_ADDR)

#define SPRD_ION_BASE		(ION_BASE_ADDR)
#define SPRD_ION_OVERLAY_BASE		(ION_BASE_ADDR + SPRD_ION_SIZE)

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define SPRD_RAM_CONSOLE_SIZE		(RC_SIZE_ADDR)
#define SPRD_RAM_CONSOLE_START		(RC_BASE_ADDR)
#endif

#ifdef CONFIG_HTC_DBG_UNCACHE_FTRACE
#define FT_BASE_PHY (246 * SZ_1M + 0x80000000)
#define FT_SIZE_PHY (8 * SZ_1M)
#endif

#endif

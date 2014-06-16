/* arch/arm/mach-sc/include/mach/BOARD_HTC.h
 * Copyright (C) 2007-2009 HTC Corporation.
 * Author: Future Zhou <future_zhou@htc.com>
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
#ifndef __ASM_ARCH_SC_BOARD_HTC_H
#define __ASM_ARCH_SC_BOARD_HTC_H

#include <asm/setup.h>

/*-------------------------board revision START------------------*/
#define HTC_PCBID_EVT_MIN   0   /* 0x00 */
#define HTC_PCBID_EVT_MAX   7   /* 0x07 */
#define HTC_PCBID_PVT_MIN   128 /* 0x80 */
#define HTC_PCBID_PVT_MAX   135 /* 0x87 */

#define BOARD_UNKNOWN   (-1)
#define BOARD_EVM       0

#define BOARD_EVT_XA    1       /* 0x00 +1 */
#define BOARD_EVT_XB    2       /* 0x01 +1 */
#define BOARD_EVT_XC    3       /* 0x02 +1 */
#define BOARD_EVT_XD    4       /* 0x03 +1 */
#define BOARD_EVT_XE    5       /* 0x04 +1 */
#define BOARD_EVT_XF    6       /* 0x05 +1 */
#define BOARD_EVT_XG    7       /* 0x06 +1 */
#define BOARD_EVT_XH    8       /* 0x07 +1 */

#define BOARD_PVT_A     128     /* 0x80 */
#define BOARD_PVT_B     129     /* 0x81 */
#define BOARD_PVT_C     130     /* 0x82 */
#define BOARD_PVT_D     131     /* 0x83 */
#define BOARD_PVT_E     132     /* 0x84 */
#define BOARD_PVT_F     133     /* 0x85 */
#define BOARD_PVT_G     134     /* 0x86 */
#define BOARD_PVT_H     135     /* 0x87 */
/*-------------------------board revision END------------------*/

/* HTC CONFIG */
#define HTC_DBG_FLAG_INNER				0
#define HTC_FLAG_RESERVED1				1
#define HTC_FLAG_RESERVED2				2
#define HTC_DBG_FLAG_DRIVER				3
#define HTC_FLAG_RESERVED4				4
#define HTC_DBG_FLAG_SUPERMAN			5
#define HTC_DBG_FLAG_POWER				6
#define HTC_DBG_FLAG_HBOOT				7
#define HTC_DBG_FLAG_CP0_W				8
#define HTC_DBG_FLAG_CP1_TD				9

/* HTC_DBG_FLAG_SUPERMAN */
enum {
	SUPERMAN_FLAG_RAMDUMP			=	BIT(0),
	SUPERMAN_FLAG_LAST_IO			=	BIT(1),
	SUPERMAN_FLAG_HW_LAST_IO		=	BIT(2),
	SUPERMAN_FLAG_FTRACE			=	BIT(3),
	SUPERMAN_FLAG_FTRACE_HANG		=	BIT(4),
	SUPERMAN_FLAG_FTRACE_MEM		=	BIT(5),
	SUPERMAN_FLAG_FTRACE_CPU		=	BIT(6),
	SUPERMAN_FLAG_FTRACE_SIGNAL		=	BIT(7),
	SUPERMAN_FLAG_FTRACE_BINDER		=	BIT(8),
	SUPERMAN_FLAG_FTRACE_POWER		=	BIT(9),
	SUPERMAN_FLAG_DBG_UART			=	BIT(10),
	SUPERMAN_FLAG_DBG_HBOOTLOG		=	BIT(11),
	SUPERMAN_FLAG_REBOOT_DUMP		=	BIT(12),
	SUPERMAN_FLAG_PANIC_EMMC		=	BIT(13),
};

/* HTC_DBG_FLAG_KERNEL */
enum {
	KERNEL_FLAG_SERIAL_HSL_ENABLE	=	BIT(1),
};

/* HTC_DBG_FLAG_CP0 */
enum {
	CP0_FLAG_TRIG_MODEM_ASSERT	=	BIT(1),
	CP0_FLAG_SIM_RETRY_DISABLE	=	BIT(2),
	CP0_FLAG_MODEM_DUMP_ENABLE	=	BIT(3),
	CP0_FLAG_MEM_LAYOUT_SM		=	BIT(4),
	CP0_FLAG_ONLINE_LOG_ENABLE	=	BIT(9),
	CP0_FLAG_OFFLINE_LOG_ENABLE	=	BIT(10),
	CP0_FLAG_SMARTLOG_ENABLE	=	BIT(15),
};

/* HTC_DBG_FLAG_CP1 */
enum {
	CP1_FLAG_TRIG_MODEM_ASSERT	=	BIT(1),
	CP1_FLAG_SIM_RETRY_DISABLE	=	BIT(2),
	CP1_FLAG_MODEM_DUMP_ENABLE	=	BIT(3),
	CP1_FLAG_MEM_LAYOUT_SM		=	BIT(4),
	CP1_FLAG_ONLINE_LOG_ENABLE	=	BIT(9),
	CP1_FLAG_OFFLINE_LOG_ENABLE	=	BIT(10),
	CP1_FLAG_SMARTLOG_ENABLE	=	BIT(15),
};

unsigned htc_get_config(unsigned id);
unsigned htc_get_bomid(void);

int board_emmc_boot(void);

extern int z4dtg_get_board_revision(void);
extern int z4td_get_board_revision(void);
extern int cp5dtu_get_board_revision(void);
extern int cp5dug_get_board_revision(void);
extern int cp5dwg_get_board_revision(void);
extern int cp5vedtu_get_board_revision(void);
extern int cp5vedwg_get_board_revision(void);

int board_mfg_modem_mode(void);

char *board_serialno(void);
unsigned long get_kernel_flag(void);
#define get_radio_flag() htc_get_config(8)
#define get_radio2_flag() htc_get_config(9)
#define get_radio_flag_via() htc_get_config(10)

#endif

/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
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

#ifndef __ASM_ARCH_BOARD_H
#define __ASM_ARCH_BOARD_H

#ifdef	CONFIG_MACH_SP8825EA
#include "__board-sp8825ea.h"
#endif

#ifdef	CONFIG_MACH_SP8825EA_TIGER_EVM
#include "__board-sp8825ea_tiger_evm.h"
#endif

#ifdef CONFIG_MACH_SPX35EA
#include "__board-sp8830ea.h"
#endif

#ifdef	CONFIG_MACH_SPX35FPGA
#include "__board-sp8830fpga.h"
#endif

#ifdef	CONFIG_MACH_Z4DTG
#include "__board-z4dtg.h"
#include "__memlayout-z4dtg.h"
#endif

#ifdef	CONFIG_MACH_Z4TD
#include "__board-z4td.h"
#include "__memlayout-z4td.h"
#endif

#ifdef	CONFIG_MACH_CP5DTU
#include "__board-cp5dtu.h"
#include "__memlayout-cp5dtu.h"
#endif

#ifdef	CONFIG_MACH_CP5DUG
#include "__board-cp5dug.h"
#include "__memlayout-cp5dug.h"
#endif

#ifdef	CONFIG_MACH_CP5DWG
#include "__board-cp5dwg.h"
#include "__memlayout-cp5dwg.h"
#endif

#ifdef	CONFIG_MACH_CP5VEDTU
#include "__board-cp5vedtu.h"
#include "__memlayout-cp5vedtu.h"
#endif

#ifdef	CONFIG_MACH_CP5VEDWG
#include "__board-cp5vedwg.h"
#include "__memlayout-cp5vedwg.h"
#endif

#include <asm/sizes.h>
void msm_hsusb_set_vbus_state(int online);
enum usb_connect_type {
	CONNECT_TYPE_CLEAR = -2,
	CONNECT_TYPE_UNKNOWN = -1,
	CONNECT_TYPE_NONE = 0,
	CONNECT_TYPE_USB,
	CONNECT_TYPE_AC,
	CONNECT_TYPE_9V_AC,
	CONNECT_TYPE_WIRELESS,
	CONNECT_TYPE_INTERNAL,
	CONNECT_TYPE_UNSUPPORTED,
#ifdef CONFIG_MACH_VERDI_LTE
	/* Y cable with USB and 9V charger */
	CONNECT_TYPE_USB_9V_AC,
#endif
};

/* START: add USB connected notify function */
struct t_usb_status_notifier{
	struct list_head notifier_link;
	const char *name;
	void (*func)(int cable_type);
};
int usb_register_notifier(struct t_usb_status_notifier *notifer);
//static LIST_HEAD(g_lh_usb_notifier_list);

/***********************************
Direction: cable detect drvier -> battery driver or other
***********************************/
struct t_cable_status_notifier{
	struct list_head cable_notifier_link;
	const char *name;
	void (*func)(int cable_type);
};
int cable_detect_register_notifier(struct t_cable_status_notifier *);
//static LIST_HEAD(g_lh_calbe_detect_notifier_list);

/***********************************
 Direction: sii9234 drvier -> cable detect driver
***********************************/
struct t_mhl_status_notifier{
	struct list_head mhl_notifier_link;
	const char *name;
	void (*func)(bool isMHL, int charging_type);
};
int mhl_detect_register_notifier(struct t_mhl_status_notifier *);
/* END: add USB connected notify function */

#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
/***********************************
Direction: cable detect drvier -> usb driver
 ***********************************/
struct t_usb_host_status_notifier{
	struct list_head usb_host_notifier_link;
	const char *name;
	void (*func)(bool cable_in);
};
int usb_host_detect_register_notifier(struct t_usb_host_status_notifier *);
//static LIST_HEAD(g_lh_usb_host_detect_notifier_list);
#endif
/* END: add USB connected notify function */

#ifndef PROJECT_SPECIAL_LAYOUT
#ifdef CONFIG_ION

    #if defined(CONFIG_CAMERA_8M)
    #define SPRD_ION_SIZE	(23*1024*1024)
    #elif defined(CONFIG_CAMERA_5M)
    #define SPRD_ION_SIZE	(19*1024*1024)
    #elif defined(CONFIG_CAMERA_3M)
    #define SPRD_ION_SIZE	(13*1024*1024)
    #elif defined(CONFIG_CAMERA_2M)
        #ifdef CONFIG_CAMERA_ROTATION
        #define SPRD_ION_SIZE	(13*1024*1024)
        #else
        #define SPRD_ION_SIZE	(8*1024*1024)
        #endif
    #else
    #define SPRD_ION_SIZE	(CONFIG_SPRD_ION_SIZE * SZ_1M)
    #endif

#define SPRD_ION_OVERLAY_SIZE   (CONFIG_SPRD_ION_OVERLAY_SIZE * SZ_1M)

#else /* !ION */
#define SPRD_ION_SIZE           (0 * SZ_1M)
#define SPRD_ION_OVERLAY_SIZE   (0 * SZ_1M)
#endif

#define SPRD_IO_MEM_SIZE	(SPRD_ION_SIZE + SPRD_ION_OVERLAY_SIZE)
#define SPRD_IO_MEM_BASE	\
	((CONFIG_PHYS_OFFSET & (~(SZ_256M - 1))) + SZ_256M - SPRD_IO_MEM_SIZE)

#define SPRD_ION_BASE		(SPRD_IO_MEM_BASE)
#define SPRD_ION_OVERLAY_BASE   (SPRD_ION_BASE + SPRD_ION_SIZE)

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define SPRD_RAM_CONSOLE_SIZE	0x20000
#define SPRD_RAM_CONSOLE_START	(SPRD_IO_MEM_BASE - SPRD_RAM_CONSOLE_SIZE)
#endif
#endif

extern int emmc_partition_read_proc(char *page, char **start, off_t off,
		                                int count, int *eof, void *data);

extern int dying_processors_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data);

struct sysdump_mem {
	unsigned long paddr;
	unsigned long vaddr;
	unsigned long soff;
	size_t size;
	int type;
	char name[32];
};

enum sysdump_type {
	SYSDUMP_RAM,
	SYSDUMP_MODEM,
	SYSDUMP_IOMEM,
};

extern int sprd_dump_mem_num;
extern struct sysdump_mem sprd_dump_mem[];
int board_mfg_mode(void);

#endif

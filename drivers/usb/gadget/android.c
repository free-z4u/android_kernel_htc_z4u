/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *         Benoit Goby <benoit@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>

#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/usb/htc_usb3_0.h>
#include <linux/usb/android_board.h>
#include <linux/gpio.h>
#include <asm/current.h>

#include "gadget_chips.h"
//#include <mach/perflock.h>
enum {
	OS_NOT_YET,
	OS_MAC,
	OS_LINUX,
	OS_WINDOWS,
};

static int mac_mtp_mode;
static int os_type;

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

#include "f_fs.c"
//#include "f_audio_source.c"
#include "f_mass_storage.c"
#include "u_serial.c"
#include "f_acm.c"
#include "f_adb.c"
#if defined(CONFIG_USB_ANDROID_PHONET)
#include "f_phonet.c"
#endif
#include "f_mtp.c"
#include "f_accessory.c"
#ifdef CONFIG_USB_SPRD_DWC
#include "f_vserial.c"
#endif
#define USB_ETH_RNDIS y
#include "f_rndis.c"
#include "rndis.c"
#include "f_ecm.c"
#include "u_ether.c"
#include "f_serial.c"
#include "f_projector.c"

#ifdef CONFIG_USB_FUNCTION_PERFLOCK
#include <mach/perflock.h>
#endif

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";
extern int in_calibration(void);
/* Default vendor and product IDs, overridden by userspace */
#define VENDOR_ID		0xbb4
#define PRODUCT_ID		0xcd3
#define PID_RNDIS		0x0ffe
#define PID_RNDIS_ADB	0x0ffc
#define PID_ECM			0x0ff8
/* #define PID_ACM			0x0ff4 */

static bool connect2pc;

struct android_usb_function {
	char *name;
	void *config;

	struct device *dev;
	char *dev_name;
	struct device_attribute **attributes;

	/* for android_dev.enabled_functions */
	struct list_head enabled_list;

	/* Optional: initialization during gadget bind */
	int (*init)(struct android_usb_function *, struct usb_composite_dev *);
	/* Optional: cleanup during gadget unbind */
	void (*cleanup)(struct android_usb_function *);
	/* Optional: called when the function is added the list of
	 *		enabled functions */
	void (*enable)(struct android_usb_function *);
	/* Optional: called when it is removed */
	void (*disable)(struct android_usb_function *);

	int (*bind_config)(struct android_usb_function *,
			   struct usb_configuration *);

	/* Optional: called when the configuration is removed */
	void (*unbind_config)(struct android_usb_function *,
			      struct usb_configuration *);
	/* Optional: handle ctrl requests before the device is configured */
	int (*ctrlrequest)(struct android_usb_function *,
					struct usb_composite_dev *,
					const struct usb_ctrlrequest *);

	/* for performance requirement */
	int performance_lock;

};

struct android_dev {
	struct android_usb_function **functions;
	struct list_head enabled_functions;
	unsigned int enabled_function_flag;
	struct usb_composite_dev *cdev;
	struct device *dev;
	//struct android_usb_platform_data *pdata;

	bool enabled;
	int disable_depth;
	struct mutex mutex;
	bool connected;
	bool sw_connected;
	struct work_struct work;
	struct android_usb_platform_data *platform_data;
	int autobot_mode;
	char ffs_aliases[256];
};

static struct class *android_class;
static struct android_dev *_android_dev;
struct work_struct	switch_adb_work;
static char enable_adb;

static int android_enable_function(struct android_dev *dev, char *name);

#ifdef CONFIG_USB_FUNCTION_PERFLOCK
static 	struct perf_lock musb_perf_lock;
static int perf_lock_init_flag = 0;
#endif

static void do_switch_adb_work(struct work_struct *work)
{
	int	call_us_ret = -1;
	char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/system/sbin:/system/bin:/system/xbin",
		NULL,
	};
	char *exec_path[2] = {"/system/bin/stop", "/system/bin/start" };
	char *argv_stop[] = { exec_path[0], "adbd", NULL, };
	char *argv_start[] = { exec_path[1], "adbd", NULL, };

	if (enable_adb) {
		call_us_ret = call_usermodehelper(exec_path[1],
			argv_start, envp, UMH_WAIT_PROC);
	} else {
		call_us_ret = call_usermodehelper(exec_path[0],
			argv_stop, envp, UMH_WAIT_PROC);
	}
	htc_usb_enable_function("adb", enable_adb);
}

static int android_switch_setup(struct usb_gadget *gadget,
		const struct usb_ctrlrequest *c)
{
	int value = -EOPNOTSUPP;
	u16 wIndex = le16_to_cpu(c->wIndex);
	u16 wValue = le16_to_cpu(c->wValue);

	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	struct usb_request *req = cdev->req;
	/* struct android_dev *dev = _android_dev; */

	switch (c->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_VENDOR:

		switch (c->bRequest) {
		case USB_REQ_HTC_FUNCTION:

			switch (wValue) {
			case USB_WVAL_ADB:

				value = 0;
				req->zero = 0;
				req->length = value;
				if (usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC))
					printk(KERN_ERR "ep0 in queue failed\n");

				if (wIndex == 1) {
					enable_adb = 1;
					schedule_work(&switch_adb_work);
				} else if (wIndex == 0) {
					enable_adb = 0;
					schedule_work(&switch_adb_work);
				}
				break;

			default:
				break;
			}

			break;

		default:
			break;
		}
		break;
	default:
		break;
	}

	return value;
}

static int android_bind_config(struct usb_configuration *c);
static void android_unbind_config(struct usb_configuration *c);

//static struct perf_lock android_usb_perf_lock;

/* string IDs are assigned dynamically */
#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

static char manufacturer_string[256];
static char product_string[256];
static char serial_string[256];

static struct usb_string_node usb_string_array[] = {
	{
		.usb_function_flag = 1 << USB_FUNCTION_RNDIS,
		.name = "rndis",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_UMS,
		.name = "mass_storage",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_ADB,
		.name = "adb",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_ECM,
		.name = "ecm",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_SERIAL,
		.name = "serial",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_MTP,
		.name = "mtp",
	},
	/* HTC_KER_START, ken, enable STE phonet function */
	{
		.usb_function_flag = 1 << USB_FUNCTION_ACM,
		.name = "acm",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_PHONET,
		.name = "phonet",
	},
	/* HTC_KER_END */
	{
		.usb_function_flag = 1 << USB_FUNCTION_STE_MODEM,
		.name = "ste_modem",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_AUTOBOT,
		.name = "auto_bot",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_PROJECTOR,
		.name = "projector",
	},
	{
		.usb_function_flag = 1 << USB_FUNCTION_ACCESSORY,
		.name = "accessory",
	},
};

static struct usb_pid_table usb_products[] = {
	{/*ums*/
		.usb_function_flag = 1 << USB_FUNCTION_UMS,
		.pid = 0x0ff9,
	},
	{/*mtp+ums*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS),
		.pid = 0x0f91,
	},
	{/*ums+adb : default board pid*/
		.usb_function_flag = (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB),
		.pid = 0x0cd3,
	},
	{/*mtp+ums+adb : default sense 4+ board pid*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB),
		.pid = 0x0f90,
	},
	{/*ums+adb+serial*/
		.usb_function_flag = (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB) | (1 << USB_FUNCTION_SERIAL) ,
		.pid = 0x0fd1,
	},
	{/*mtp+ums+adb+serial*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB) | (1 << USB_FUNCTION_SERIAL),
		.pid = 0x0fa0,
	},
	{/*rndis+adb*/
		.usb_function_flag = (1 << USB_FUNCTION_RNDIS) | (1 << USB_FUNCTION_ADB),
		.pid = 0x0ffc,
	},
	{/*mtp+rndis+ums+adb*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) |(1 << USB_FUNCTION_RNDIS) | (1 << USB_FUNCTION_UMS)  | (1 << USB_FUNCTION_ADB),
		.pid = 0x0fb4,
	},
	{/*mtp+rndis+ums*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) |(1 << USB_FUNCTION_RNDIS) | (1 << USB_FUNCTION_UMS),
		.pid = 0x0fb5,
	},
	{
		.usb_function_flag = (1 << USB_FUNCTION_RNDIS),
		.pid = 0x0ffe,
	},
	{
		.usb_function_flag = (1 << USB_FUNCTION_ECM),
		.pid = 0x0ff8,
	},
	{/*ecm+mtp+adb+ums*/
		.usb_function_flag = (1 << USB_FUNCTION_ECM) |(1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_ADB) | (1 << USB_FUNCTION_UMS),
		.pid = 0x0f88,
	},
	{/*ecm+mtp+ums*/
		.usb_function_flag = (1 << USB_FUNCTION_ECM) |(1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS),
		.pid = 0x0f89,
	},
	{
		.usb_function_flag = (1 << USB_FUNCTION_ADB) |(1 << USB_FUNCTION_MTP) ,
		.pid = 0x0ca8,
	},
	/* HTC_KER_START, ken, enable STE phonet function */
	{
		.usb_function_flag = (1 << USB_FUNCTION_ACM) | (1 << USB_FUNCTION_ECM) | (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB) | (1 << USB_FUNCTION_PHONET),
		.pid = 0x2323,
	},
	/* HTC_KER_END */
	{/*ums+adb+serial+daig+modem*/
		.usb_function_flag = (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB) | (1 << USB_FUNCTION_STE_MODEM) ,
		.pid = 0xfe8,
	},
	{/*mtp+ums+adb+serial+diag+modem*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS)  | (1 << USB_FUNCTION_ADB) | (1 << USB_FUNCTION_STE_MODEM) ,
		.pid = 0xfa6,
	},
	{/*ums+serial+diag+modem*/
		.usb_function_flag = (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_STE_MODEM) ,
		.pid = 0xfe9,
	},
	{/*mtp+ums+serial+diag+modem*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS)| (1 << USB_FUNCTION_STE_MODEM) ,
		.pid = 0xfa7,
	},
	{/*ums+projector*/
		.usb_function_flag = (1 << USB_FUNCTION_UMS)  | (1 << USB_FUNCTION_PROJECTOR) ,
		.pid = 0xc05,
	},
	{/*ums+projector*/
		.usb_function_flag =  (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS)  | (1 << USB_FUNCTION_PROJECTOR) ,
		.pid = 0xf98,
	},
	{/*ums+adb+projector*/
		.usb_function_flag = (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB)  | (1 << USB_FUNCTION_PROJECTOR) ,
		.pid = 0xc06,
	},
	{/*mtp+ums+adb+projector*/
		.usb_function_flag = (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB)  | (1 << USB_FUNCTION_PROJECTOR) ,
		.pid = 0xf97,
	},
	{
		.usb_function_flag = (1 << USB_FUNCTION_ACCESSORY) | (1 << USB_FUNCTION_ADB),
		.pid = 0x2d01,
	},
	{
		.usb_function_flag = (1 << USB_FUNCTION_ACCESSORY),
		.pid = 0x2d00,
	},
};


static struct htc_usb_pid htc_usb_pid_table = {
	.usb_pid_table_array = usb_products,
	.length = ARRAY_SIZE(usb_products),
};
/* String Table */
static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer_string,
	[STRING_PRODUCT_IDX].s = product_string,
	[STRING_SERIAL_IDX].s = serial_string,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof(otg_descriptor),
	.bDescriptorType =	USB_DT_OTG,
#ifdef CONFIG_USB_OTG
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
	.bcdOTG =		0x0200,
#endif
};
#if 0
static struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};
#endif

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.unbind		= android_unbind_config,
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= CONFIG_USB_GADGET_VBUS_DRAW / 2,
};

enum android_device_state {
	USB_DISCONNECTED,
	USB_CONNECTED,
	USB_CONFIGURED,
};

static unsigned int htc_usb_get_func_combine_value(void)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_function *f;
	int i;
	unsigned int val = 0;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		for (i = 0; i < ARRAY_SIZE(usb_string_array); i++)
			if (!strcmp(usb_string_array[i].name, f->name)) {
				val |= usb_string_array[i].usb_function_flag;
				break;
			}
	}
	return val;
}


static unsigned int Name2Flag(char *name)
{
	int x, length = ARRAY_SIZE(usb_string_array);
	for (x = 0; x < length; x++) {
		if (strcmp(usb_string_array[x].name, name) == 0)
			return usb_string_array[x].usb_function_flag;
	}
	printk(KERN_INFO "[USB]Name2Flag fail to match string\n");
	return 0;
}

static int GetEnableFunctionFlag(struct android_dev *dev)
{
	return dev->enabled_function_flag;
}

static void ClearEnableFunction(struct android_dev *dev)
{
	INIT_LIST_HEAD(&dev->enabled_functions);
	dev->enabled_function_flag = 0;
}

/*-------------------------------------------------------------------------*/
/* Supported functions initialization */
static ssize_t func_en_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct android_usb_function *func = dev_get_drvdata(dev);
	struct android_usb_function *f;
	int ebl = 0;

	list_for_each_entry(f, &_android_dev->enabled_functions, enabled_list) {
		if (!strcmp(func->name, f->name)) {
			ebl = 1;
			break;
		}
	}
	return sprintf(buf, "%d", ebl);
}

static ssize_t func_en_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct android_usb_function *func = dev_get_drvdata(dev);
	struct android_usb_function *f;
	int ebl = 0;
	int value;
	unsigned int buff = GetEnableFunctionFlag(_android_dev);

	sscanf(buf, "%d", &value);
	list_for_each_entry(f, &_android_dev->enabled_functions, enabled_list) {
		if (!strcmp(func->name, f->name)) {
			ebl = 1;
			break;
		}
	}
	if (!!value == ebl) {
		pr_info("%s function is already %s\n", func->name
			, ebl ? "enable" : "disable");
		return size;
	}
	//if diag_init == 1, we already enable serial when bootup, so prevent framwork request to enable serial again
	if(_android_dev->platform_data->diag_init == 1 && !strcmp(func->name, "serial")) {
		pr_info("serial function is already enabled\n");
		return size;
	}

	if (value)
		buff += Name2Flag(func->name);
	else
		buff = buff & ~(Name2Flag(func->name));

	if (value)
		htc_usb_enable_function(func->name, 1);
	else
		htc_usb_enable_function(func->name, 0);

	return size;
}
static DEVICE_ATTR(on, S_IRUGO | S_IWUSR | S_IWGRP, func_en_show, func_en_store);

static int get_product_id(struct android_dev *dev)
{
	struct android_usb_function *f;
	int ret = 0, ret2, x;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		ret2 = Name2Flag(f->name);
		if (ret2 == 0) {
			printk(KERN_ERR "[USB] enabed_function name doesn't match usb_string_array\n");
			return 0;
		} else
			ret += ret2;
	}

	for (x = 0; x < htc_usb_pid_table.length ; x++) {
		if (htc_usb_pid_table.usb_pid_table_array[x].usb_function_flag == ret)
			return htc_usb_pid_table.usb_pid_table_array[x].pid;
	}
	/* return project pid */
	return dev->platform_data->product_id;
}
#if 1
static DEFINE_MUTEX(function_bind_sem);
int android_switch_function(unsigned func)
{
       struct android_dev *dev = _android_dev;
       struct android_usb_function **functions = dev->functions;
       struct android_usb_function *f;
#ifdef CONFIG_SENSE_4_PLUS
	struct android_usb_function *fadb = NULL;
	struct android_usb_function *fums = NULL;
#endif
       unsigned val;
	int accessory_enable = 0;
	int autobot_mode = 0;
	int ret;
	int ste_modem_enabled = 0;

       mutex_lock(&function_bind_sem);

       val = htc_usb_get_func_combine_value();

       pr_info("[USB] %s: %u, before %u\n", __func__, func, val);

       if (func == val) {
               pr_info("[USB] %s: SKIP due the function is the same ,%u\n", __func__, func);
               mutex_unlock(&function_bind_sem);
               return 0;
       }

       usb_gadget_disconnect(dev->cdev->gadget);
       usb_remove_config(dev->cdev, &android_config_driver);

       msleep(10);
       INIT_LIST_HEAD(&dev->enabled_functions);

       while ((f = *functions++)) {
               if ((func & (1 << USB_FUNCTION_UMS)) && !strcmp(f->name, "mass_storage")) {
#ifdef CONFIG_SENSE_4_PLUS
			if (func == ((1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB)))
				fums = f;
			else
				list_add_tail(&f->enabled_list, &dev->enabled_functions);
#else
			list_add_tail(&f->enabled_list, &dev->enabled_functions);
#endif
               } else if ((func & (1 << USB_FUNCTION_ADB)) && !strcmp(f->name, "adb")) {
#ifdef CONFIG_SENSE_4_PLUS
			if (func == ((1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB)))
				fadb = f;
			else
				list_add_tail(&f->enabled_list, &dev->enabled_functions);
#else
			list_add_tail(&f->enabled_list, &dev->enabled_functions);
#endif
               } else if ((func & (1 << USB_FUNCTION_ECM)) && !strcmp(f->name, "ecm"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               else if ((func & (1 << USB_FUNCTION_RNDIS)) && !strcmp(f->name, "rndis"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               else if ((func & (1 << USB_FUNCTION_SERIAL)) && !strcmp(f->name, "serial"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               else if ((func & (1 << USB_FUNCTION_MTP)) && !strcmp(f->name, "mtp"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               /* HTC_KER_START, ken, enable STE phonet function */
               else if ((func & (1 << USB_FUNCTION_ACM)) && !strcmp(f->name, "acm"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               else if ((func & (1 << USB_FUNCTION_PHONET)) && !strcmp(f->name, "phonet"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               /* HTC_KER_END */
               else if ((func & (1 << USB_FUNCTION_STE_MODEM)) && !strcmp(f->name, "ste_modem"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               else if ((func & (1 << USB_FUNCTION_AUTOBOT)) && !strcmp(f->name, "auto_bot")) {
                       autobot_mode = 1;
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
               } else if ((func & (1 << USB_FUNCTION_PROJECTOR)) && !strcmp(f->name, "projector"))
                       list_add_tail(&f->enabled_list, &dev->enabled_functions);
		else if ((func & (1 << USB_FUNCTION_ACCESSORY)) && !strcmp(f->name, "accessory")) {
			list_add_tail(&f->enabled_list, &dev->enabled_functions);
			accessory_enable = 1;
		}
       }

#ifdef CONFIG_SENSE_4_PLUS
	/* exchange the order ADB & UMS inferface for sense 4+*/
	if (func == ((1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB))) {
		if (fums)
			list_add_tail(&fums->enabled_list, &dev->enabled_functions);
		if (fadb)
			list_add_tail(&fadb->enabled_list, &dev->enabled_functions);
	}
#endif

       list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
			   if(!strcmp(f->name,"ste_modem"))
					   ste_modem_enabled = 1;
               printk(KERN_INFO "[USB] %s %d: Enable %s\n", __func__, __LINE__, f->name);
	   }
	/* initial function depends on radio flag */
	if (dev->platform_data->diag_init && !ste_modem_enabled) {
		ret = android_enable_function(dev, "ste_modem");
		if (ret)
			pr_err("android_usb: Cannot enable '%s'", "diag");
	}
       device_desc.idProduct = __constant_cpu_to_le16(get_product_id(_android_dev));
       /* HTC_KER_START, ken, enable STE phonet function */
	if (accessory_enable == 1) {
		device_desc.idVendor = __constant_cpu_to_le16(0x18d1);
	} else if (device_desc.idProduct == 0x2323) {
		device_desc.idVendor = __constant_cpu_to_le16(0x4cc);
	} else {
		device_desc.idVendor = __constant_cpu_to_le16(_android_dev->platform_data->vendor_id);
	}
       /* HTC_KER_END */
       dev->cdev->desc.idVendor = device_desc.idVendor;
       dev->cdev->desc.idProduct = device_desc.idProduct;

	dev->autobot_mode = autobot_mode;

       printk(KERN_INFO "[USB] %s %d: vendor_id=0x%x, product_id=0x%x\n", __func__, __LINE__, device_desc.idVendor, device_desc.idProduct);

       /* We need to specify the COMM class in the device descriptor
        * if we are using RNDIS.
        */
       if (device_desc.idProduct == PID_RNDIS || device_desc.idProduct == PID_ECM || device_desc.idProduct == PID_ACM)
               dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
       else
               dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;

       device_desc.bDeviceClass = dev->cdev->desc.bDeviceClass;

       usb_add_config(dev->cdev, &android_config_driver, android_bind_config);

       /* reset usb controller/phy for USB stability */
       /* TODO : it is better to call a HW reset USB function*/

       mdelay(200);
       usb_gadget_connect(dev->cdev->gadget);
       dev->enabled = true;

       mutex_unlock(&function_bind_sem);
       return 0;
}
int htc_usb_enable_function(char *name, int ebl)
{
	int i;
	unsigned val;

	mutex_lock(&function_bind_sem);

	val = htc_usb_get_func_combine_value();
	printk("[USB] %s %d: Current funcs = %u. Going to make %s to %d\n", __func__, __LINE__, val, name, ebl);

	for (i = 0; i < ARRAY_SIZE(usb_string_array); i++) {
		if (!strcmp(usb_string_array[i].name, name)) {
			if (ebl) {
				if (val & usb_string_array[i].usb_function_flag) {
					pr_info("[USB] %s: '%s' is already enabled\n", __func__, name);
					mutex_unlock(&function_bind_sem);
					return 0;
				}
				val |= usb_string_array[i].usb_function_flag;
			} else {
				if (!(val & usb_string_array[i].usb_function_flag)) {
					pr_info("[USB] %s: '%s' is already disabled\n", __func__, name);
					mutex_unlock(&function_bind_sem);
					return 0;
				}

				val &= ~usb_string_array[i].usb_function_flag;
			}
			break;
		}
	}
	mutex_unlock(&function_bind_sem);
	return android_switch_function(val);
}
#else
int android_switch_function(unsigned func)
{
	int product_id;
	printk(KERN_INFO "[USB] android_switch_function 0x%x\n", func);
	usb_gadget_disconnect(_android_dev->cdev->gadget);
	usb_remove_config(_android_dev->cdev, &android_config_driver);

	ClearEnableFunction(_android_dev);

	if (func & (1 << USB_FUNCTION_RNDIS))
		android_enable_function(_android_dev, "rndis");
	if (func & (1 << USB_FUNCTION_UMS))
		android_enable_function(_android_dev, "mass_storage");
	if (func & (1 << USB_FUNCTION_ADB))
		android_enable_function(_android_dev, "adb");
	if (func & (1 << USB_FUNCTION_ECM))
		android_enable_function(_android_dev, "ecm");
	if (func & (1 << USB_FUNCTION_SERIAL))
		android_enable_function(_android_dev, "serial");

	/* get pid */
	product_id = get_product_id(_android_dev);
	device_desc.idProduct = __constant_cpu_to_le16(product_id);
	_android_dev->cdev->desc.idProduct = device_desc.idProduct;

	/* if tethering is enabed, change device class */
	if (product_id == PID_RNDIS || product_id == PID_ECM)
		device_desc.bDeviceClass = USB_CLASS_COMM;
	else
		device_desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
	_android_dev->cdev->desc.bDeviceClass = device_desc.bDeviceClass;

	usb_add_config(_android_dev->cdev, &android_config_driver, android_bind_config);
	usb_gadget_connect(_android_dev->cdev->gadget);
	return 0;
}
#endif

static bool is_mtp_enable(void)
{
	unsigned val;

	mutex_lock(&function_bind_sem);
	val = htc_usb_get_func_combine_value();
	mutex_unlock(&function_bind_sem);

	if (val & (1 << USB_FUNCTION_MTP))
		return true;
	else
		return false;
}

void android_switch_default(void)
{
	unsigned val;

	mutex_lock(&function_bind_sem);
	val = htc_usb_get_func_combine_value();
	mutex_unlock(&function_bind_sem);

	if (val & (1 << USB_FUNCTION_ADB))
		android_switch_function(
				(1 << USB_FUNCTION_ADB) |
				(1 << USB_FUNCTION_UMS));
	else
		android_switch_function(
				(1 << USB_FUNCTION_UMS));
}


static void android_set_serialno(char *serialno)
{
	strings_dev[STRING_SERIAL_IDX].s = serialno;
}

int android_show_function(char *buf)
{

	struct android_dev *dev = _android_dev;
	struct android_usb_function *f1,*f2;
	struct android_usb_function **functions = dev->functions;
	unsigned length = 0;
	char *buff = buf;
	int flag;

	while ((f1 = *functions++)) {
		flag = 0;
		list_for_each_entry(f2, &dev->enabled_functions, enabled_list) {
			if (f1 == f2) {
				flag = 1;
				break;
			}
		}
		if (flag == 1)
			length += sprintf(buff + length, "%s:%s\n", f1->name, "enable");
		else
			length += sprintf(buff + length, "%s:%s\n", f1->name, "disable");
	}
/*
	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		if (strcmp(f->name, "rndis") == 0)
			length += sprintf(buff + length, "%s:%s\n", "ether", "enable");
		else
			length += sprintf(buff + length, "%s:%s\n", f->name, "enable");
	}*/
	return length;

}

/* ===== create attr file for generic using ===== */
#ifdef CONFIG_SYSFS

static ssize_t show_usb_function_switch(struct device *dev, struct device_attribute *attr, char *buf)
{
	return android_show_function(buf);
}
static ssize_t store_usb_function_switch(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned u;
	ssize_t  ret;

	pr_info("%s, buff: %s\n", __func__, buf);
	ret = strict_strtoul(buf, 10, (unsigned long *)&u);
	if (ret < 0) {
		printk(KERN_ERR "[USB]%s: %d\n", __func__, ret);
		return 0;
	}

	ret = android_switch_function(u);

	if (ret == 0)
		return count;
	else
		return 0;
}

static DEVICE_ATTR(usb_function_switch, 0664, show_usb_function_switch, store_usb_function_switch);

static ssize_t show_usb_cable_connect(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned length;
	struct android_dev *adev = _android_dev;

	length = sprintf(buf, "%d", adev->connected ? 1 : 0);
	return length;
}
static DEVICE_ATTR(usb_cable_connect, 0444, show_usb_cable_connect, NULL);

static ssize_t show_USB_ID_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct android_usb_platform_data *pdata = dev->platform_data;
	int value = 1;
	unsigned length;

	if (pdata->usb_id_pin_gpio != 0) {
		value = gpio_get_value(_android_dev->platform_data->usb_id_pin_gpio);
		printk(KERN_INFO"[USB] id pin status %d\n", value);
	}

	length = sprintf(buf, "%d", value);
	return length;
}
static DEVICE_ATTR(USB_ID_status, 0444, show_USB_ID_status, NULL);

static char mfg_df_serialno[16];
static int use_mfg_serialno;

static ssize_t show_usb_serial_number(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned length;
	struct android_usb_platform_data *pdata = dev->platform_data;

	length = sprintf(buf, "%s", pdata->serial_number);
	return length;
}
static ssize_t store_usb_serial_number(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct android_usb_platform_data *pdata = dev->platform_data;
	struct android_dev *adev = _android_dev;
	char *serialno = "000000000000";

	if (buf[0] == '0' || buf[0] == '1') {
		memset(mfg_df_serialno, 0x0, sizeof(mfg_df_serialno));
		if (buf[0] == '0') {
			strncpy(mfg_df_serialno, serialno, strlen(serialno));
			use_mfg_serialno = 1;
			android_set_serialno(mfg_df_serialno);
		} else {
			strncpy(mfg_df_serialno, pdata->serial_number,
				strlen(pdata->serial_number));
			use_mfg_serialno = 0;
			android_set_serialno(pdata->serial_number);
		}
		/* reset_device */
		usb_composite_force_reset(adev->cdev);
	}

	return count;
}

static DEVICE_ATTR(usb_serial_number, 0644, show_usb_serial_number, store_usb_serial_number);

static ssize_t show_dummy_usb_serial_number(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned length;
	struct android_usb_platform_data *pdata = dev->platform_data;

	if (use_mfg_serialno)
		length = sprintf(buf, "%s", mfg_df_serialno); /* dummy */
	else
		length = sprintf(buf, "%s", pdata->serial_number); /* Real */
	return length;
}
static ssize_t store_dummy_usb_serial_number(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct android_dev *adev = _android_dev;
	int data_buff_size = (sizeof(mfg_df_serialno) > strlen(buf))?
		strlen(buf):sizeof(mfg_df_serialno);
	int loop_i;

	/* avoid overflow, mfg_df_serialno[16] always is 0x0 */
	if (data_buff_size == 16)
		data_buff_size--;

	for (loop_i = 0; loop_i < data_buff_size; loop_i++)	{
		if (buf[loop_i] >= 0x30 && buf[loop_i] <= 0x39) /* 0-9 */
			continue;
		else if (buf[loop_i] >= 0x41 && buf[loop_i] <= 0x5A) /* A-Z */
			continue;
		if (buf[loop_i] == 0x0A) /* Line Feed */
			continue;
		else {
			printk(KERN_INFO "%s(): get invaild char (0x%2.2X)\n",
				__func__, buf[loop_i]);
			return -EINVAL;
		}
	}

	use_mfg_serialno = 1;
	memset(mfg_df_serialno, 0x0, sizeof(mfg_df_serialno));
	strncpy(mfg_df_serialno, buf, data_buff_size);
	android_set_serialno(mfg_df_serialno);
	/*device_reset */
	usb_composite_force_reset(adev->cdev);

	return count;
}
static DEVICE_ATTR(dummy_usb_serial_number, 0644, show_dummy_usb_serial_number, store_dummy_usb_serial_number);

/* show current os type for mac or non-mac */
static ssize_t show_os_type(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned length;

	length = sprintf(buf, "%d\n", os_type);
	printk(KERN_INFO "[USB]%s: %s\n", __func__, buf);
	return length;
}

static DEVICE_ATTR(os_type, 0444, show_os_type, NULL);

/* Check if USB function is available for user process */
static ssize_t show_is_usb_denied(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned length;
	int deny = 0;

	if (usb_autobot_mode()) {
		/* In HTC mode, USB function change by
		 * user space should be denied.
		 */
		deny = 1;;
	}

	length = sprintf(buf, "%d\n", deny);
	printk(KERN_INFO "[USB]%s: %s\n", __func__, buf);
	return length;
}

static DEVICE_ATTR(usb_denied, 0444, show_is_usb_denied, NULL);

/* ats mode */
static ssize_t show_ats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned length = 0;

	length = sprintf(buf, "%d\n", board_get_usb_ats());
	printk(KERN_INFO "[USB]%s: %s\n", __func__, buf);
	return length;
}
static DEVICE_ATTR(ats, 0444, show_ats, NULL);


static ssize_t store_usb_disable_setting(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int disable_usb_function;
	ssize_t  ret;
	ret = kstrtouint(buf, 2, &disable_usb_function);
	if (ret < 0) {
			pr_err("%s: %d\n", __func__, ret);
			return count;
	}
	printk(KERN_INFO "[USB] USB_disable set %d\n", disable_usb_function);
	htc_disable_usb_set(disable_usb_function);
	return count;
}

static ssize_t show_usb_disable_setting(struct device *dev,
		struct device_attribute *attr,  char *buf)
{
	unsigned length = 0;
	length = sprintf(buf, "%d\n", htc_disable_usb_get());
	return length;

}

static DEVICE_ATTR(usb_disable, 0664, show_usb_disable_setting, store_usb_disable_setting);

static struct attribute *gb_android_usb_attributes[] = {
	&dev_attr_usb_function_switch.attr,
	&dev_attr_usb_cable_connect.attr,
	&dev_attr_USB_ID_status.attr, /* for MFG */
	&dev_attr_usb_serial_number.attr, /* for MFG */
	&dev_attr_dummy_usb_serial_number.attr, /* for MFG */
	&dev_attr_usb_disable.attr,
	&dev_attr_usb_denied.attr,
	&dev_attr_os_type.attr,
	&dev_attr_ats.attr,
	NULL
};

static const struct attribute_group android_usb_attr_group = {
	.attrs = gb_android_usb_attributes,
};
#endif /* CONFIG_SYSFS */

static int usb_autobot_mode(void)
{
	if (_android_dev->autobot_mode)
		return 1;
	else
		return 0;
}


extern void scxx30_fix_max_freq(int enable);
static void android_work(struct work_struct *data)
{
#ifdef CONFIG_USB_FUNCTION_PERFLOCK
	int count = 0;
	struct android_usb_function *f;
#endif
	struct android_dev *dev = container_of(data, struct android_dev, work);
	struct usb_composite_dev *cdev = dev->cdev;
	char *disconnected[2] = { "USB_STATE=DISCONNECTED", NULL };
	char *connected[2]    = { "USB_STATE=CONNECTED", NULL };
	char *configured[2]   = { "USB_STATE=CONFIGURED", NULL };
	char **uevent_envp = NULL;
	static enum android_device_state last_uevent, next_state;
	unsigned long flags;
	//int pm_qos_vote = -1;
#ifdef CONFIG_USB_FUNCTION_PERFLOCK
/* release cpu frequency.*/
	if (perf_lock_init_flag == 0) {
		perf_lock_init_flag = 1;
		perf_lock_init(&musb_perf_lock, TYPE_PERF_LOCK, PERF_LOCK_HIGHEST, "musb");
	}
	if (is_perf_lock_active(&musb_perf_lock)) {
		printk("[USB]release perf_lock\n");
		perf_unlock(&musb_perf_lock);
		printk("[USB]release ddr freq\n");
		scxx30_fix_max_freq(0);
	}

#endif

	spin_lock_irqsave(&cdev->lock, flags);
        if (cdev->config) {
		uevent_envp = configured;
		next_state = USB_CONFIGURED;
#ifdef CONFIG_USB_FUNCTION_PERFLOCK
		list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
			if (f->performance_lock) {
				printk(KERN_INFO "[USB]Performance lock for '%s'\n", f->name);
				count++;
			}
		}
#endif
        }
	else if (dev->connected != dev->sw_connected) {
		uevent_envp = dev->connected ? connected : disconnected;
		next_state = dev->connected ? USB_CONNECTED : USB_DISCONNECTED;
	}
	dev->sw_connected = dev->connected;
	spin_unlock_irqrestore(&cdev->lock, flags);

#ifdef CONFIG_USB_FUNCTION_PERFLOCK
	if (count) {
		/* hold cpu frequecy*/
		if (!is_perf_lock_active(&musb_perf_lock)) {
			perf_lock(&musb_perf_lock);
			printk(KERN_INFO "[USB]raise perflock\n");
			scxx30_fix_max_freq(1);
			printk("[USB]raise ddr freq\n");
		}
	}

#endif


	if (uevent_envp) {
		/*
		 * Some userspace modules, e.g. MTP, work correctly only if
		 * CONFIGURED uevent is preceded by DISCONNECT uevent.
		 * Check if we missed sending out a DISCONNECT uevent. This can
		 * happen if host PC resets and configures device really quick.
		 */
		if (((uevent_envp == connected) &&
		      (last_uevent != USB_DISCONNECTED)) ||
		    ((uevent_envp == configured) &&
		      (last_uevent == USB_CONFIGURED))) {
			pr_info("%s: sent missed DISCONNECT event\n", __func__);
			kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE,
								disconnected);
			msleep(20);
		/* in mfg case, to change the dummy_usb_serial_number, the mtp may
		 * wake up very slowly. we send one more connect event here.
		 */
			pr_info("%s: sent CONNECT event\n", __func__);
			kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE,
					connected);
			msleep(20);
		}
		/*
		 * Before sending out CONFIGURED uevent give function drivers
		 * a chance to wakeup userspace threads and notify disconnect
		 */
		if (uevent_envp == configured)
			msleep(50);

		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, uevent_envp);
		last_uevent = next_state;
		pr_info("%s: sent uevent %s\n", __func__, uevent_envp[0]);
	} else {
		pr_info("%s: did not send uevent (%d %d %p)\n", __func__,
			 dev->connected, dev->sw_connected, cdev->config);
	}

	if (connect2pc != dev->sw_connected) {
		connect2pc = dev->sw_connected;
		switch_set_state(&cdev->sw_connect2pc, connect2pc ? 1 : 0);
		pr_info("set usb_connect2pc = %d\n", connect2pc);
		if (!connect2pc) {
			pr_info("%s: OS_NOT_YET\n", __func__);
			os_type = OS_NOT_YET;
			mtp_update_mode(0);
			fsg_update_mode(0);
		}
	}

	if (dev->connected == 0 && check_htc_mode_status() != NOT_ON_AUTOBOT) {
		htc_mode_enable(0);
		android_switch_default();
	}

}


/*-------------------------------------------------------------------------*/
/* Supported functions initialization */

static int adb_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	return adb_setup();
}

static void adb_function_cleanup(struct android_usb_function *f)
{
	adb_cleanup();
}

static void android_enable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;

	if (WARN_ON(!dev->disable_depth))
		return;

	if (--dev->disable_depth == 0) {
		usb_add_config(cdev, &android_config_driver,
					android_bind_config);
		usb_gadget_connect(cdev->gadget);
	}
}

static void android_disable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;

	if (dev->disable_depth++ == 0) {
		usb_gadget_disconnect(cdev->gadget);
		/* Cancel pending control requests */
		usb_ep_dequeue(cdev->gadget->ep0, cdev->req);
		usb_remove_config(cdev, &android_config_driver);
	}
}

/*-------------------------------------------------------------------------*/
/* Supported functions initialization */

struct functionfs_config {
	bool opened;
	bool enabled;
	struct ffs_data *data;
};

static int ffs_function_init(struct android_usb_function *f,
			     struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct functionfs_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	return functionfs_init();
}

static void ffs_function_cleanup(struct android_usb_function *f)
{
	functionfs_cleanup();
	kfree(f->config);
}

static void ffs_function_enable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = f->config;

	config->enabled = true;

	/* Disable the gadget until the function is ready */
	if (!config->opened)
		android_disable(dev);
}

static void ffs_function_disable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = f->config;

	config->enabled = false;

	/* Balance the disable that was called in closed_callback */
	if (!config->opened)
		android_enable(dev);
}

static int ffs_function_bind_config(struct android_usb_function *f,
				    struct usb_configuration *c)
{
	struct functionfs_config *config = f->config;
	return functionfs_bind_config(c->cdev, c, config->data);
}

static ssize_t
ffs_aliases_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = _android_dev;
	int ret;

	mutex_lock(&dev->mutex);
	ret = sprintf(buf, "%s\n", dev->ffs_aliases);
	mutex_unlock(&dev->mutex);

	return ret;
}

static ssize_t
ffs_aliases_store(struct device *pdev, struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct android_dev *dev = _android_dev;
	char buff[256];

	mutex_lock(&dev->mutex);

	if (dev->enabled) {
		mutex_unlock(&dev->mutex);
		return -EBUSY;
	}

	strlcpy(buff, buf, sizeof(buff));
	strlcpy(dev->ffs_aliases, strim(buff), sizeof(dev->ffs_aliases));

	mutex_unlock(&dev->mutex);

	return size;
}

static DEVICE_ATTR(aliases, S_IRUGO | S_IWUSR, ffs_aliases_show,
					       ffs_aliases_store);
static struct device_attribute *ffs_function_attributes[] = {
	&dev_attr_aliases,
	NULL
};

static struct android_usb_function ffs_function = {
	.name		= "ffs",
	.init		= ffs_function_init,
	.enable		= ffs_function_enable,
	.disable	= ffs_function_disable,
	.cleanup	= ffs_function_cleanup,
	.bind_config	= ffs_function_bind_config,
	.attributes	= ffs_function_attributes,
};

static int functionfs_ready_callback(struct ffs_data *ffs)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = ffs_function.config;
	int ret = 0;

	mutex_lock(&dev->mutex);

	ret = functionfs_bind(ffs, dev->cdev);
	if (ret)
		goto err;

	config->data = ffs;
	config->opened = true;

	if (config->enabled)
		android_enable(dev);

err:
	mutex_unlock(&dev->mutex);
	return ret;
}

static void functionfs_closed_callback(struct ffs_data *ffs)
{
	struct android_dev *dev = _android_dev;
	struct functionfs_config *config = ffs_function.config;

	mutex_lock(&dev->mutex);

	if (config->enabled)
		android_disable(dev);

	config->opened = false;
	config->data = NULL;

	functionfs_unbind(ffs);

	mutex_unlock(&dev->mutex);
}

static int functionfs_check_dev_callback(const char *dev_name)
{
	return 0;
}


struct adb_data {
	bool opened;
	bool enabled;
};

static int
adb_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return adb_bind_config(c);
}

static void adb_android_function_enable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = f->config;

	data->enabled = true;

	/* Disable the gadget until adbd is ready */
	if (!data->opened)
		android_disable(dev);
}

static void adb_android_function_disable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = f->config;

	data->enabled = false;

	/* Balance the disable that was called in closed_callback */
	if (!data->opened)
		android_enable(dev);
}

static struct android_usb_function adb_function = {
	.name		= "adb",
	.enable		= adb_android_function_enable,
	.disable	= adb_android_function_disable,
	.init		= adb_function_init,
	.cleanup	= adb_function_cleanup,
	.bind_config	= adb_function_bind_config,
};

#if 0
static void adb_ready_callback(void)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = adb_function.config;

	mutex_lock(&dev->mutex);

	data->opened = true;

	if (data->enabled)
		android_enable(dev);

	mutex_unlock(&dev->mutex);
}

static void adb_closed_callback(void)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = adb_function.config;

	mutex_lock(&dev->mutex);

	data->opened = false;

	if (data->enabled)
		android_disable(dev);

	mutex_unlock(&dev->mutex);
}
#endif

static void adb_read_timeout(void)
{
	struct android_dev *dev = _android_dev;

	pr_info("%s: adb read timeout, re-connect to PC\n",__func__);

	if (dev) {
		android_disable(dev);
		mdelay(100);
		android_enable(dev);
	}
}

#define MAX_ACM_INSTANCES 4
struct acm_function_config {
	int instances;
};

static int
acm_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	struct acm_function_config *config;

	config = kzalloc(sizeof(struct acm_function_config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	config->instances = 1;
	f->config = config;
	return gserial_setup(cdev->gadget, MAX_ACM_INSTANCES);
}

static void acm_function_cleanup(struct android_usb_function *f)
{
	gserial_cleanup();
	kfree(f->config);
	f->config = NULL;
}

static int
acm_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	int i;
	int ret = 0;
	struct acm_function_config *config = f->config;

	for (i = 0; i < config->instances; i++) {
		ret = acm_bind_config(c, i);
		if (ret) {
			pr_err("Could not bind acm%u config\n", i);
			break;
		}
	}

	return ret;
}

static ssize_t acm_instances_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->instances);
}

static ssize_t acm_instances_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	int value;

	sscanf(buf, "%d", &value);
	if (value > MAX_ACM_INSTANCES)
		value = MAX_ACM_INSTANCES;
	config->instances = value;
	return size;
}

static DEVICE_ATTR(instances, S_IRUGO | S_IWUSR, acm_instances_show,
						 acm_instances_store);
static struct device_attribute *acm_function_attributes[] = {
	&dev_attr_instances,
	NULL
};

static struct android_usb_function acm_function = {
	.name		= "acm",
	.init		= acm_function_init,
	.cleanup	= acm_function_cleanup,
	.bind_config	= acm_function_bind_config,
	.attributes	= acm_function_attributes,
};


static int
mtp_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
#ifdef CONFIG_USB_FUNCTION_PERFLOCK
	struct android_dev *dev = _android_dev;
	int ret;
	ret = mtp_setup();
	mtp_setup_perflock(dev->platform_data->mtp_perf_lock_on?true:false);
	return ret;
#else
	return mtp_setup();
#endif
}

static void mtp_function_cleanup(struct android_usb_function *f)
{
	mtp_cleanup();
}

static int
mtp_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return mtp_bind_config(c, false);
}

static int
ptp_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	/* nothing to do - initialization is handled by mtp_function_init */
	return 0;
}

static void ptp_function_cleanup(struct android_usb_function *f)
{
	/* nothing to do - cleanup is handled by mtp_function_cleanup */
}

static int
ptp_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return mtp_bind_config(c, true);
}

static int mtp_function_ctrlrequest(struct android_usb_function *f,
					struct usb_composite_dev *cdev,
					const struct usb_ctrlrequest *c)
{
	return mtp_ctrlrequest(cdev, c);
}

#ifdef CONFIG_USB_FUNCTION_PERFLOCK
static ssize_t mtp_debug_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", htc_mtp_performance_debug);
}

static ssize_t mtp_debug_level_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if (buf[0] >= '0' && buf[0] <= '9' && buf[1] == '\n')
		htc_mtp_performance_debug = buf[0] - '0';
	return size;
}

static ssize_t mtp_open_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", htc_mtp_open_state);
}
#endif

#ifdef CONFIG_USB_FUNCTION_PERFLOCK
static DEVICE_ATTR(mtp_debug_level, S_IRUGO | S_IWUSR, mtp_debug_level_show,
						    mtp_debug_level_store);
static DEVICE_ATTR(mtp_open_state, S_IRUGO, mtp_open_state_show, NULL);
static struct device_attribute *mtp_function_attributes[] = {
	&dev_attr_mtp_debug_level,
	&dev_attr_mtp_open_state,
	NULL
};
#endif

static struct android_usb_function mtp_function = {
	.name		= "mtp",
	.init		= mtp_function_init,
	.cleanup	= mtp_function_cleanup,
	.bind_config	= mtp_function_bind_config,
	.ctrlrequest	= mtp_function_ctrlrequest,
#ifdef CONFIG_USB_FUNCTION_PERFLOCK
	.attributes 	= mtp_function_attributes,
#endif
};

/* PTP function is same as MTP with slightly different interface descriptor */
static struct android_usb_function ptp_function = {
	.name		= "ptp",
	.init		= ptp_function_init,
	.cleanup	= ptp_function_cleanup,
	.bind_config	= ptp_function_bind_config,
};

struct ecm_function_config {
	u8      ethaddr[ETH_ALEN];
	u32     vendorID;
	char	manufacturer[256];
};
static int ecm_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	struct ecm_function_config *ecm;
	f->config = kzalloc(sizeof(struct ecm_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	ecm = f->config;

	ecm->vendorID = _android_dev->platform_data->ecmVendorID;
	ecm->ethaddr[0] = _android_dev->platform_data->ecmEthaddr[0];
	ecm->ethaddr[1] = _android_dev->platform_data->ecmEthaddr[1];
	ecm->ethaddr[2] = _android_dev->platform_data->ecmEthaddr[2];
	ecm->ethaddr[3] = _android_dev->platform_data->ecmEthaddr[3];
	ecm->ethaddr[4] = _android_dev->platform_data->rndisEthaddr[4];
	ecm->ethaddr[5] = _android_dev->platform_data->rndisEthaddr[5];
	strcpy(ecm->manufacturer, _android_dev->platform_data->ecmVendorDescr);

/*	gether_setup(cdev->gadget, ecm->ethaddr);*/

	return 0;
}

static void ecm_function_cleanup(struct android_usb_function *f)
{
	gether_cleanup();
	kfree(f->config);
	f->config = NULL;
}

static int ecm_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	int ret = 0;
	struct ecm_function_config *ecm = f->config;

	if (!ecm) {
			pr_err("%s: ecm_pdata\n", __func__);
			return -1;
		}


	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);


	ret = gether_setup(c->cdev->gadget, ecm->ethaddr);

	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	return ecm_bind_config(c, ecm->ethaddr);
}

static void ecm_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
}

static ssize_t ecm_manufacturer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->manufacturer);
}

static ssize_t ecm_manufacturer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;

	if (size >= sizeof(config->manufacturer))
		return -EINVAL;
	if (sscanf(buf, "%s", config->manufacturer) == 1)
		return size;
	return -1;
}

static DEVICE_ATTR(manufactur, S_IRUGO | S_IWUSR, ecm_manufacturer_show,
						    ecm_manufacturer_store);

static ssize_t ecm_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);
}

static ssize_t ecm_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&ecm->ethaddr[0], (int *)&ecm->ethaddr[1],
		    (int *)&ecm->ethaddr[2], (int *)&ecm->ethaddr[3],
		    (int *)&ecm->ethaddr[4], (int *)&ecm->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(ethadd, S_IRUGO | S_IWUSR, ecm_ethaddr_show,
					       ecm_ethaddr_store);
/*
static ssize_t ecm_vendorID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;
	return sprintf(buf, "%04x\n", config->vendorID);
}

static ssize_t ecm_vendorID_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%04x", &value) == 1) {
		config->vendorID = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(vendor, S_IRUGO | S_IWUSR, ecm_vendorID_show,
						ecm_vendorID_store);
*/
static struct device_attribute *ecm_function_attributes[] = {
	&dev_attr_ethadd,
/*	&dev_attr_vendor,*/
	&dev_attr_manufactur,
	NULL
};

static struct android_usb_function ecm_function = {
	.name		= "ecm",
	.init		= ecm_function_init,
	.cleanup	= ecm_function_cleanup,
	.bind_config	= ecm_function_bind_config,
	.unbind_config	= ecm_function_unbind_config,
	.attributes	= ecm_function_attributes,
	.performance_lock	= 1,
};


struct rndis_function_config {
	u8      ethaddr[ETH_ALEN];
	u32     vendorID;
	char	manufacturer[256];
	/* "Wireless" RNDIS; auto-detected by Windows */
	bool	wceis;
};

static int
rndis_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	struct rndis_function_config *rndis;
	f->config = kzalloc(sizeof(struct rndis_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	rndis  = f->config;

	rndis->vendorID = _android_dev->platform_data->rndisVendorID;
	rndis->ethaddr[0] = _android_dev->platform_data->rndisEthaddr[0];
	rndis->ethaddr[1] = _android_dev->platform_data->rndisEthaddr[1];
	rndis->ethaddr[2] = _android_dev->platform_data->rndisEthaddr[2];
	rndis->ethaddr[3] = _android_dev->platform_data->rndisEthaddr[3];
	rndis->ethaddr[4] = _android_dev->platform_data->rndisEthaddr[4];
	rndis->ethaddr[5] = _android_dev->platform_data->rndisEthaddr[5];
	strcpy(rndis->manufacturer, _android_dev->platform_data->rndisVendorDescr);

/*	gether_setup(cdev->gadget, rndis->ethaddr);*/

	return 0;
}

static void rndis_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int
rndis_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	int ret;
	struct rndis_function_config *rndis = f->config;

	if (!rndis) {
		pr_err("%s: rndis_pdata\n", __func__);
		return -1;
	}

	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);


	ret = gether_setup_name(c->cdev->gadget, rndis->ethaddr, "usb");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	if (rndis->wceis) {
		/* "Wireless" RNDIS; auto-detected by Windows */
		rndis_iad_descriptor.bFunctionClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_iad_descriptor.bFunctionSubClass = 0x01;
		rndis_iad_descriptor.bFunctionProtocol = 0x03;
		rndis_control_intf.bInterfaceClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_control_intf.bInterfaceSubClass =	 0x01;
		rndis_control_intf.bInterfaceProtocol =	 0x03;
	}

	return rndis_bind_config(c, rndis->ethaddr, rndis->vendorID,
				    rndis->manufacturer);
}

static void rndis_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_cleanup();
}

static ssize_t rndis_manufacturer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->manufacturer);
}

static ssize_t rndis_manufacturer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;

	if (size >= sizeof(config->manufacturer))
		return -EINVAL;
	if (sscanf(buf, "%s", config->manufacturer) == 1)
		return size;
	return -1;
}

static DEVICE_ATTR(manufacturer, S_IRUGO | S_IWUSR, rndis_manufacturer_show,
						    rndis_manufacturer_store);

static ssize_t rndis_wceis_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->wceis);
}

static ssize_t rndis_wceis_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%d", &value) == 1) {
		config->wceis = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(wceis, S_IRUGO | S_IWUSR, rndis_wceis_show,
					     rndis_wceis_store);

static ssize_t rndis_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);
}

static ssize_t rndis_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&rndis->ethaddr[0], (int *)&rndis->ethaddr[1],
		    (int *)&rndis->ethaddr[2], (int *)&rndis->ethaddr[3],
		    (int *)&rndis->ethaddr[4], (int *)&rndis->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(ethaddr, S_IRUGO | S_IWUSR, rndis_ethaddr_show,
					       rndis_ethaddr_store);

static ssize_t rndis_vendorID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%04x\n", config->vendorID);
}

static ssize_t rndis_vendorID_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%04x", &value) == 1) {
		config->vendorID = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(vendorID, S_IRUGO | S_IWUSR, rndis_vendorID_show,
						rndis_vendorID_store);

static struct device_attribute *rndis_function_attributes[] = {
	&dev_attr_manufacturer,
	&dev_attr_wceis,
	&dev_attr_ethaddr,
	&dev_attr_vendorID,
	NULL
};

static struct android_usb_function rndis_function = {
	.name		= "rndis",
	.init		= rndis_function_init,
	.cleanup	= rndis_function_cleanup,
	.bind_config	= rndis_function_bind_config,
	.unbind_config	= rndis_function_unbind_config,
	.attributes	= rndis_function_attributes,
	.performance_lock	= 1,
};


struct mass_storage_function_config {
	struct fsg_config fsg;
	struct fsg_common *common;
};

static int mass_storage_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	struct mass_storage_function_config *config;
	struct fsg_common *common;
	int err,i;
	struct android_dev *dev = _android_dev;

	config = kzalloc(sizeof(struct mass_storage_function_config),
								GFP_KERNEL);
	if (!config)
		return -ENOMEM;


	if (dev->platform_data->nluns) {
		config->fsg.nluns = dev->platform_data->nluns;
		if (config->fsg.nluns > FSG_MAX_LUNS)
			config->fsg.nluns = FSG_MAX_LUNS;
		for (i = 0; i < config->fsg.nluns; i++) {
			if (dev->platform_data->cdrom_lun & (1 << i)) {
				config->fsg.luns[i].cdrom = 1;
				config->fsg.luns[i].removable = 1;
				config->fsg.luns[i].ro = 1;
			} else {
				config->fsg.luns[i].cdrom = 0;
				config->fsg.luns[i].removable = 1;
				config->fsg.luns[i].ro = 0;
			}
		}
	} else {
		/* default value */
		config->fsg.nluns = 2;
		config->fsg.luns[0].removable = 1;
		config->fsg.luns[1].removable = 1;
	}

	config->fsg.vendor_name = dev->platform_data->manufacturer_name;
	config->fsg.product_name = dev->platform_data->product_name;

	common = fsg_common_init(NULL, cdev, &config->fsg);
	if (IS_ERR(common)) {
		kfree(config);
		return PTR_ERR(common);
	}

	for (i = 0; i < config->fsg.nluns; i++) {
		err = sysfs_create_link(&f->dev->kobj,
					&common->luns[i].dev.kobj,
					common->luns[i].dev.kobj.name);
		if (err) {
			fsg_common_release(&common->ref);
			kfree(config);
			return err;
		}
	}
#if 0
	err = sysfs_create_link(&f->dev->kobj,
				&common->luns[0].dev.kobj,
				common->luns[0].dev.kobj.name); /* modify to use lun0 as path*/
	if (err) {
		kfree(config);
		return err;
	}

	err = sysfs_create_link(&f->dev->kobj,
				&common->luns[1].dev.kobj,
				common->luns[1].dev.kobj.name); /* modify to use lun1 as path*/
	if (err) {
		kfree(config);
		return err;
	}
#endif
	config->common = common;
	f->config = config;
	return 0;
}

static void mass_storage_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int mass_storage_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct mass_storage_function_config *config = f->config;
	return fsg_bind_config(c->cdev, c, config->common);
}

static ssize_t mass_storage_inquiry_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->common->inquiry_string);
}

static ssize_t mass_storage_inquiry_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	if (size >= sizeof(config->common->inquiry_string))
		return -EINVAL;
	if (sscanf(buf, "%s", config->common->inquiry_string) != 1)
		return -EINVAL;
	return size;
}

static DEVICE_ATTR(inquiry_string, S_IRUGO | S_IWUSR,
					mass_storage_inquiry_show,
					mass_storage_inquiry_store);

static ssize_t mass_storage_support_luns_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->common->board_support_luns);
}

static ssize_t mass_storage_support_luns_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	int value;

	sscanf(buf, "%d", &value);
	if (value > (int)(config->common->luns)||value <= 0)
		return size;
	config->common->board_support_luns = value;
	return size;
}

static DEVICE_ATTR(board_support_luns, S_IRUGO | S_IWUSR,
					mass_storage_support_luns_show,
					mass_storage_support_luns_store);

static struct device_attribute *mass_storage_function_attributes[] = {
	&dev_attr_inquiry_string,
	&dev_attr_board_support_luns,
	NULL
};

static struct android_usb_function mass_storage_function = {
	.name		= "mass_storage",
	.init		= mass_storage_function_init,
	.cleanup	= mass_storage_function_cleanup,
	.bind_config	= mass_storage_function_bind_config,
	.attributes	= mass_storage_function_attributes,
};

#if defined(CONFIG_USB_ANDROID_PHONET)
static int phonet_function_init(struct android_usb_function *f,
			struct usb_composite_dev *cdev)
{
	return gphonet_setup(cdev->gadget);
}

static void phonet_function_cleanup(struct android_usb_function *f)
{
	gphonet_cleanup();
}

static int phonet_function_bind_config(struct android_usb_function *f,
			struct usb_configuration *c)
{
	return phonet_bind_config(c);
}

static struct android_usb_function phonet_function = {
	.name		= "phonet",
	.init		= phonet_function_init,
	.cleanup	= phonet_function_cleanup,
	.bind_config	= phonet_function_bind_config,
};
#endif

static int accessory_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	return acc_setup();
}

static void accessory_function_cleanup(struct android_usb_function *f)
{
	acc_cleanup();
}

static int accessory_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	return acc_bind_config(c);
}

static int accessory_function_ctrlrequest(struct android_usb_function *f,
						struct usb_composite_dev *cdev,
						const struct usb_ctrlrequest *c)
{
	return acc_ctrlrequest(cdev, c);
}

static struct android_usb_function accessory_function = {
	.name		= "accessory",
	.init		= accessory_function_init,
	.cleanup	= accessory_function_cleanup,
	.bind_config	= accessory_function_bind_config,
	.ctrlrequest	= accessory_function_ctrlrequest,
};

static char serial_transports[64];	/*enabled FSERIAL ports - "tty[,sdio]"*/

static int serial_driver_init(struct usb_configuration *c)
{
	char *name, *str[2];
	char buf[80],*b;
	int err = -1;
	static int serial_initialized = 0, ports = 0;

	if (serial_initialized) {
		pr_info("%s: already initial\n", __func__);
		return 0;
	}
	serial_initialized = 1;


	if (_android_dev->platform_data->fserial_init_string)
		strcpy(serial_transports, _android_dev->platform_data->fserial_init_string);
	else
		strcpy(serial_transports, "tty:modem,tty:serial,tty:serial");

	strncpy(buf, serial_transports, sizeof(buf));
	buf[79] = 0;
	printk(KERN_INFO "[USB]%s: init string: %s\n", __func__, buf);

	b = strim(buf);

	while (b) {
		str[0] = str[1] = 0;
		name = strsep(&b, ",");
		if (name) {
			str[0] = strsep(&name, ":");
			if (str[0])
				str[1] = strsep(&name, ":");
		}
		err = gserial_init_port(ports, str[0], str[1]);
		printk(KERN_INFO "[USB] serial_driver_init create ttyport %d %s %s\n",ports ,str[0] ,str[1]);
		if (err) {
			pr_err("serial: Cannot open port '%s'\n", str[0]);
			return -1;
		}
		ports++;
	}

	if (gport_setup(c)) {
		pr_err("serial: Cannot setup transports");
		return -1;
	}
	return 0;
/*
	return f_serial_init(cdev->gadget);
*/
}


static int serial_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	int err;
	if ((err = serial_driver_init(c))) {
		pr_err("serial: Cannot setup transports %d",err);
		return err;
	}

	if((err = gser_bind_config(c,0))) {
		pr_err("serial: Cannot gser_bind_configs port 0");
		return err;
	}
	return 0;
}

static struct android_usb_function serial_function = {
	.name		= "serial",
/*	.init		= serial_function_init,*/
	.bind_config	= serial_function_bind_config,
};
/* STE Modem */
static int ste_modem_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	int err;
	if ((err =serial_driver_init(c))) {
		pr_err("modem: Cannot setup transports %d",err);
		return err;
	}
	/* diag */
	if((err = gser_bind_config(c,1))) {
		pr_err("serial: Cannot gser_bind_configs port 1");
		return err;
	}
	/* modem */
	if((err = gser_bind_config(c,0))) {
		pr_err("serial: Cannot gser_bind_configs port 2");
		return err;
	}
	/* serial */
	if((err = gser_bind_config(c,2))) {
		pr_err("serial: Cannot gser_bind_configs port 0");
		return err;
	}

	return 0;
}

static struct android_usb_function ste_modem_function = {
	.name		= "ste_modem",
	.bind_config	= ste_modem_function_bind_config,
	.performance_lock	= 1,
};


static int projector_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct htcmode_protocol), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	return projector_setup(f->config);
}

static void projector_function_cleanup(struct android_usb_function *f)
{
	projector_cleanup();
	kfree(f->config);
}

static int projector_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return projector_bind_config(c);
}


static ssize_t projector_width_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%d\n", config->server_info.width);
}

static DEVICE_ATTR(width, S_IRUGO | S_IWUSR, projector_width_show,
						    NULL);

static ssize_t projector_height_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%d\n", config->server_info.height);
}

static DEVICE_ATTR(height, S_IRUGO | S_IWUSR, projector_height_show,
						    NULL);

static ssize_t projector_rotation_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%d\n", (config->client_info.display_conf & CLIENT_INFO_SERVER_ROTATE_USED));
}

static DEVICE_ATTR(rotation, S_IRUGO | S_IWUSR, projector_rotation_show,
						    NULL);

static ssize_t projector_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%d\n", config->version);
}

static DEVICE_ATTR(version, S_IRUGO | S_IWUSR, projector_version_show,
						    NULL);

static ssize_t projector_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%d\n", config->vendor);
}

static DEVICE_ATTR(vendor, S_IRUGO | S_IWUSR, projector_vendor_show,
						    NULL);

static ssize_t projector_server_nonce_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	memcpy(buf, config->nonce, HSML_SERVER_NONCE_SIZE);
	return HSML_SERVER_NONCE_SIZE;
}

static DEVICE_ATTR(server_nonce, S_IRUGO | S_IWUSR, projector_server_nonce_show,
						    NULL);

static ssize_t projector_client_sig_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	memcpy(buf, config->client_sig, HSML_CLIENT_SIG_SIZE);
	return HSML_CLIENT_SIG_SIZE;
}

static DEVICE_ATTR(client_sig, S_IRUGO | S_IWUSR, projector_client_sig_show,
						    NULL);

static ssize_t projector_server_sig_store(
		struct device *dev, struct device_attribute *attr,
		const char *buff, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	memcpy(config->server_sig, buff, HSML_SERVER_SIG_SIZE);
	return HSML_SERVER_SIG_SIZE;
}

static DEVICE_ATTR(server_sig, S_IWUSR, NULL,
		projector_server_sig_store);

static ssize_t projector_auth_store(
		struct device *dev, struct device_attribute *attr,
		const char *buff, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	memcpy(&config->auth_result, buff, sizeof(config->auth_result));
	config->auth_in_progress = 0;
	return sizeof(config->auth_result);
}

static DEVICE_ATTR(auth, S_IWUSR, NULL,
		projector_auth_store);

static ssize_t projector_debug_mode_store(
		struct device *dev, struct device_attribute *attr,
		const char *buff, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct htcmode_protocol *config = f->config;
	int value, i;
	int framesize = DEFAULT_PROJ_HEIGHT * DEFAULT_PROJ_WIDTH;

	if (sscanf(buff, "%d", &value) == 1) {

		if (!test_frame)
			test_frame = kzalloc(framesize * 2, GFP_KERNEL);

		if (test_frame)
			for (i = 0 ; i < framesize ; i++)
				if (i < framesize/4)
					test_frame[i] = 0xF800;
				else if (i < framesize*2/4)
					test_frame[i] = 0x7E0;
				else if (i < framesize*3/4)
					test_frame[i] = 0x1F;
				else
					test_frame[i] = 0xFFFF;

		config->debug_mode = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(debug_mode, S_IWUSR, NULL,
		projector_debug_mode_store);

static struct device_attribute *projector_function_attributes[] = {
	&dev_attr_width,
	&dev_attr_height,
	&dev_attr_rotation,
	&dev_attr_version,
	&dev_attr_vendor,
	&dev_attr_server_nonce,
	&dev_attr_client_sig,
	&dev_attr_server_sig,
	&dev_attr_auth,
	&dev_attr_debug_mode,
	NULL
};


struct android_usb_function projector_function = {
	.name		= "projector",
	.init		= projector_function_init,
	.cleanup	= projector_function_cleanup,
	.bind_config	= projector_function_bind_config,
	.attributes = projector_function_attributes
};

static int auto_bot_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	int err;
	if ((err = serial_driver_init(c))) {
		pr_err("auto_bot: Cannot setup transports %d",err);
		return err;
	}
	/* GPS, CAND, Command */
	if((err = gser_bind_config(c,1))) {
		pr_err("auto_bot: Cannot gser_bind_configs port 0");
		return err;
	}

	if((err = gser_bind_config(c,2))) {
		pr_err("auto_bot: Cannot gser_bind_configs port 1");
		return err;
	}

	if((err = gser_bind_config(c,3))) {
		pr_err("auto_bot: Cannot gser_bind_configs port 2");
		return err;
	}

	return 0;
}

static struct android_usb_function auto_bot_function = {
	.name		= "auto_bot",
	.bind_config	= auto_bot_function_bind_config,
};


#ifdef CONFIG_SENSE_4_PLUS
static struct android_usb_function *supported_functions[] = {
	&rndis_function,
	&accessory_function,
	&mtp_function,
	&ptp_function,
	&adb_function,
	&mass_storage_function,
	&ecm_function,
	&ste_modem_function,
	&serial_function,
	&auto_bot_function,
	&projector_function,
#if defined(CONFIG_USB_ANDROID_PHONET)
	&phonet_function,
#endif
	&acm_function, /* HTC_KER_MOD, ken, enable STE phonet function */
	NULL
};

#else
static struct android_usb_function *supported_functions[] = {
	&rndis_function,
	&accessory_function,
	&mass_storage_function,
	&adb_function,
	&ecm_function,
	&ste_modem_function,
	&serial_function,
	&auto_bot_function,
	&projector_function,
#if defined(CONFIG_USB_ANDROID_PHONET)
	&phonet_function,
#endif
	&acm_function, /* HTC_KER_MOD, ken, enable STE phonet function */
	&ptp_function,
	NULL
};
#endif



static int android_init_functions(struct android_usb_function **functions,
				  struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_function *f;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int err = 0;
	int index = 0;

	for (; (f = *functions++); index++) {
		f->dev_name = kasprintf(GFP_KERNEL, "f_%s", f->name);
		f->dev = device_create(android_class, dev->dev,
				MKDEV(0, index), f, f->dev_name);
		if (IS_ERR(f->dev)) {
			pr_err("%s: Failed to create dev %s", __func__,
							f->dev_name);
			err = PTR_ERR(f->dev);
			goto err_create;
		}

		if (device_create_file(f->dev, &dev_attr_on) < 0) {
			pr_err("%s: Failed to create dev file %s", __func__,
							f->dev_name);
			goto err_create;
		}

		if (f->init) {
			err = f->init(f, cdev);
			if (err) {
				pr_err("%s: Failed to init %s", __func__,
								f->name);
				goto err_out;
			}
		}

		attrs = f->attributes;
		if (attrs) {
			while ((attr = *attrs++) && !err)
				err = device_create_file(f->dev, attr);
		}
		if (err) {
			pr_err("%s: Failed to create function %s attributes",
					__func__, f->name);
			goto err_out;
		}
		pr_info("%s %s init\n", __func__, f->name); /* HTC_KER_ADD ken */
	}
	return 0;

err_out:
	device_destroy(android_class, f->dev->devt);
err_create:
	kfree(f->dev_name);
	return err;
}

static void android_cleanup_functions(struct android_usb_function **functions)
{
	struct android_usb_function *f;

	while (*functions) {
		f = *functions++;

		if (f->dev) {
			device_destroy(android_class, f->dev->devt);
			kfree(f->dev_name);
		}

		if (f->cleanup)
			f->cleanup(f);
	}
}

static int
android_bind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;
	int ret;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		pr_info("[USB] %s bind name: %s\n", __func__, f->name);
		ret = f->bind_config(f, c);
		if (ret) {
			pr_err("%s: %s failed, ret:%d\n", __func__, f->name, ret);
			return ret;
		}
	}
	return 0;
}

static void
android_unbind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		if (f->unbind_config)
			f->unbind_config(f, c);
	}
}

static int android_enable_function(struct android_dev *dev, char *name)
{
	struct android_usb_function **functions = dev->functions;
	struct android_usb_function *f;
	while ((f = *functions++)) {
		if (!strcmp(name, f->name)) {
			pr_info("[USB] %s: %s enabled\n", __func__, name);
			list_add_tail(&f->enabled_list, &dev->enabled_functions);
			dev->enabled_function_flag += Name2Flag(f->name);
			return 0;
		}
	}
	pr_info("[USB] %s: %s failed\n", __func__, name);
	return -EINVAL;
}

/*-------------------------------------------------------------------------*/
/* /sys/class/android_usb/android%d/ interface */

static ssize_t
functions_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct android_usb_function *f;
	char *buff = buf;

	mutex_lock(&dev->mutex);

	list_for_each_entry(f, &dev->enabled_functions, enabled_list)
		buff += sprintf(buff, "%s,", f->name);

	mutex_unlock(&dev->mutex);

	if (buff != buf)
		*(buff-1) = '\n';
	return buff - buf;
}

static ssize_t
functions_store(struct device *pdev, struct device_attribute *attr,
			       const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	char *name;
	char buf[256], *b;
	int x, length = ARRAY_SIZE(usb_string_array);
	unsigned char enable_flag[ARRAY_SIZE(usb_string_array)];

	pr_info("%s, buff: %s\n", __func__, buff);

	for (x = 0; x < length; x++)
		enable_flag[x] = 0;

	ClearEnableFunction(dev);

	strncpy(buf, buff, sizeof(buf));
	b = strim(buf);

	while (b) {
		name = strsep(&b, ",");
		if (name) {
			for (x = 0; x < length; x++) {
				if (strcmp(name, usb_string_array[x].name) == 0) {
					enable_flag[x] = 1;
					break;
				}
			}
			if (x == length) {
				pr_err("[USB]android_usb: 1 Cannot enable '%s'\n", name);
				return size;
			}
		}
	}

	for (x = 0; x < length; x++) {
		if (enable_flag[x] == 1) {
			if (android_enable_function(dev, usb_string_array[x].name))
				printk(KERN_ERR "[USB]android_usb: 2 Cannot enable '%s'\n", usb_string_array[x].name);
		}
	}
	return size;
}

static ssize_t enable_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	return sprintf(buf, "%d\n", dev->enabled);
}

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	int product_id;
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	int enabled = 0;

	sscanf(buff, "%d", &enabled);

	if (enabled)
		htc_usb_enable_function("adb", 1);

	pr_info("%s, buff: %s\n", __func__, buff);

	/* temporaily return immediately to prevent framework change usb behavior
	 */
	return size;

	if (enabled && !dev->enabled) {
		printk(KERN_INFO "[USB]enable %s\n", buff);
		product_id = get_product_id(dev);
		/* HTC_KER_START, ken, enable STE phonet function */
		if (product_id == 0x2323) {
			device_desc.idVendor = 0x4cc;
		} else {
			device_desc.idVendor = __constant_cpu_to_le16(_android_dev->platform_data->vendor_id);
		}
		/* HTC_KER_END */
		device_desc.idProduct = __constant_cpu_to_le16(product_id);
		/* update values in composite driver's copy of device descriptor */
		cdev->desc.idVendor = device_desc.idVendor;
		cdev->desc.idProduct = device_desc.idProduct;
		cdev->desc.bcdDevice = device_desc.bcdDevice;
		cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;
		/* if tethering is enabed, change device class */
		if (product_id == PID_RNDIS || product_id == PID_ECM)
			device_desc.bDeviceClass = USB_CLASS_COMM;
		else
			device_desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
		_android_dev->cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		usb_add_config(cdev, &android_config_driver,
					android_bind_config);
		usb_gadget_connect(cdev->gadget);
		dev->enabled = true;
	} else if (!enabled && dev->enabled) {
		printk(KERN_INFO "[USB]disable %s\n", buff);
		usb_gadget_disconnect(cdev->gadget);
		usb_remove_config(cdev, &android_config_driver);
		usb_ep_autoconfig_reset(cdev->gadget);
		dev->enabled = false;
	} else {
		pr_err("android_usb: already %s\n",
				dev->enabled ? "enabled" : "disabled");
	}
	return size;
}


static ssize_t enable_phonet_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	int enable_diag = 0;

	sscanf(buff, "%d", &enable_diag);
	if (enable_diag) {
		usb_gadget_disconnect(cdev->gadget);
		usb_remove_config(cdev, &android_config_driver);
		dev->enabled = false;

		INIT_LIST_HEAD(&dev->enabled_functions);

		android_enable_function(dev, "acm");
		android_enable_function(dev, "ecm");
		android_enable_function(dev, "mass_storage");
		android_enable_function(dev, "adb");
		android_enable_function(dev, "phonet");

		/* update values in composite driver's copy of device descriptor */
		cdev->desc.idVendor = __constant_cpu_to_le16(0x4cc);
		cdev->desc.idProduct = __constant_cpu_to_le16(0x2323);
		cdev->desc.bcdDevice = device_desc.bcdDevice;
		cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;
		strncpy(serial_string, "8B8D3725893782CE18E4FAA90E2191B", sizeof(serial_string) - 1);
		if (usb_add_config(cdev, &android_config_driver, android_bind_config))
			return size;

		usb_gadget_connect(cdev->gadget);
		dev->enabled = true;
	}
	return size;
}

static ssize_t state_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	char *state = "DISCONNECTED";
	unsigned long flags;

	if (!cdev)
		goto out;

	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config)
		state = "CONFIGURED";
	else if (dev->connected)
		state = "CONNECTED";
	spin_unlock_irqrestore(&cdev->lock, flags);
out:
	return sprintf(buf, "%s\n", state);
}

static ssize_t bugreport_debug_store(struct device *pdev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	int enable = 0, ats = 0;
	sscanf(buff, "%d", &enable);
	ats = board_get_usb_ats();
/*
	if (enable && ats)
		bugreport_debug = 1;
	else {
		bugreport_debug = 0;
		del_timer(&adb_read_timer);
	}
*/

	if (enable == 5 && ats)
		bugreport_debug = 1;
	else if (enable == 0 && ats) {
		bugreport_debug = 0;
		del_timer(&adb_read_timer);
	}

	pr_info("bugreport_debug = %d, enable=%d, ats = %d\n", bugreport_debug, enable, ats);

	return size;
}

#define DESCRIPTOR_ATTR(field, format_string)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, format_string, device_desc.field);		\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	int value;							\
	if (sscanf(buf, format_string, &value) == 1) {			\
		device_desc.field = value;				\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

#define DESCRIPTOR_STRING_ATTR(field, buffer)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, "%s", buffer);				\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)		       		\
{									\
	if (size >= sizeof(buffer)) return -EINVAL;			\
	if (sscanf(buf, "%s", buffer) == 1) {			       	\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);


DESCRIPTOR_ATTR(idVendor, "%04x\n")
DESCRIPTOR_ATTR(idProduct, "%04x\n")
DESCRIPTOR_ATTR(bcdDevice, "%04x\n")
DESCRIPTOR_ATTR(bDeviceClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceSubClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceProtocol, "%d\n")
DESCRIPTOR_STRING_ATTR(iManufacturer, manufacturer_string)
DESCRIPTOR_STRING_ATTR(iProduct, product_string)
DESCRIPTOR_STRING_ATTR(iSerial, serial_string)

static DEVICE_ATTR(functions, S_IRUGO | S_IWUSR, functions_show, functions_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);
static DEVICE_ATTR(enable_phonet, S_IWUSR, NULL, enable_phonet_store);
static DEVICE_ATTR(bugreport_debug, 0664, NULL, bugreport_debug_store);

static struct device_attribute *android_usb_attributes[] = {
	&dev_attr_idVendor,
	&dev_attr_idProduct,
	&dev_attr_bcdDevice,
	&dev_attr_bDeviceClass,
	&dev_attr_bDeviceSubClass,
	&dev_attr_bDeviceProtocol,
	&dev_attr_iManufacturer,
	&dev_attr_iProduct,
	&dev_attr_iSerial,
	&dev_attr_functions,
	&dev_attr_enable,
	&dev_attr_state,
	&dev_attr_enable_phonet,
	&dev_attr_bugreport_debug,
	NULL
};

/*-------------------------------------------------------------------------*/
/* Composite driver */

static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret = 0;

#ifdef CONFIG_USB_OTG_20
	if (gadget_is_otg(c->cdev->gadget)) {
			c->descriptors = otg_desc;
			c->bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}
#endif

	ret = android_bind_enabled_functions(dev, c);
	if (ret)
		return ret;

	return 0;
}

static void android_unbind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	android_unbind_enabled_functions(dev, c);
}

static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, ret;

	ret = android_init_functions(dev->functions, cdev);
	if (ret)
		return ret;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	/* Default strings - should be updated by userspace */
	strncpy(manufacturer_string, "HTC", sizeof(manufacturer_string) - 1);
	strncpy(product_string, "Android Phone", sizeof(product_string) - 1);
	strncpy(serial_string, dev->platform_data->serial_number, sizeof(serial_string) - 1);

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	if(in_calibration()){
		device_desc.iSerialNumber = 0;
		cdev->desc.bDeviceClass = 0xff;
	}else
		device_desc.iSerialNumber = id;

	/*
	 * As per USB compliance update, a device that is actively drawing
	 * more than 100mA from USB must report itself as bus-powered in
	 * the GetStatus(DEVICE) call.
	 */
	if (android_config_driver.bMaxPower <=
			(USB_SELF_POWER_VBUS_MAX_DRAW / 2)) {
		android_config_driver.bmAttributes =
			USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER;
		usb_gadget_set_selfpowered(gadget);
	} else
		android_config_driver.bmAttributes = USB_CONFIG_ATT_ONE;

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	dev->cdev = cdev;

	cdev->sw_connected.name = "usb_connected";
	ret = switch_dev_register(&cdev->sw_connected);
	if (ret < 0) {
		printk(KERN_ERR "[USB]fail to register switch usb_connected\n");
		return ret;
	}

	cdev->sw_config.name = "usb_configuration";
	ret = switch_dev_register(&cdev->sw_config);
	if (ret < 0) {
		printk(KERN_ERR "[USB]fail to register switch usb_configuration\n");
		return ret;
	}

	cdev->sw_connect2pc.name = "usb_connect2pc";
	ret = switch_dev_register(&cdev->sw_connect2pc);
	if (ret < 0) {
		printk(KERN_ERR "[USB]fail to register switch usb_connect2pc\n");
		return ret;
	}
	cdev->desc.idVendor = device_desc.idVendor;
	cdev->desc.bcdDevice = device_desc.bcdDevice;
	cdev->desc.bDeviceClass = device_desc.bDeviceClass;
	cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
	cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;

	/* clear name list */
	ClearEnableFunction(_android_dev);

#ifdef CONFIG_SENSE_4_PLUS
	if (board_mfg_mode() != 2) {
		if(android_enable_function(dev, "mtp"))
			printk(KERN_ERR "android_usb: Cannot enable mtp");
	}
#endif
	if(android_enable_function(dev, "mass_storage"))
		printk(KERN_ERR "android_usb: Cannot enable mass_storage");

	/* initial function depends on radio flag */
	if (dev->platform_data->diag_init) {
		ret = android_enable_function(dev, "ste_modem");
		if (ret)
			pr_err("android_usb: Cannot enable '%s'", "diag");
	}

	device_desc.idProduct = __constant_cpu_to_le16(get_product_id(dev));
	cdev->desc.idProduct = device_desc.idProduct;

	_android_dev->enabled = true;
	usb_add_config(cdev, &android_config_driver,android_bind_config);
	return 0;
}

static int android_usb_unbind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;

	cancel_work_sync(&dev->work);
	android_cleanup_functions(dev->functions);

	switch_dev_unregister(&cdev->sw_connected);
	switch_dev_unregister(&cdev->sw_config);
	switch_dev_unregister(&cdev->sw_connect2pc);

	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.unbind		= android_usb_unbind,
	.max_speed  = USB_SPEED_HIGH,
};

static int
android_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *c)
{
	struct android_dev		*dev = _android_dev;
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_request		*req = cdev->req;
	struct android_usb_function	*f;
	int value = -EOPNOTSUPP;
	unsigned long flags;

	req->zero = 0;
	req->complete = composite_setup_complete;
	req->length = 0;
	gadget->ep0->driver_data = cdev;


	value = android_switch_setup(gadget, c);
	if (value >= 0)
		return value;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		if (f->ctrlrequest) {
			value = f->ctrlrequest(f, cdev, c);
			if (value >= 0)
				break;
		}
	}

	/* Special case the accessory function.
	 * It needs to handle control requests before it is enabled.
	 */
	if (value < 0)
		value = acc_ctrlrequest(cdev, c);

	if (value < 0)
		value = projector_ctrlrequest(cdev, c);

	if (value < 0)
		value = composite_setup(gadget, c);

	spin_lock_irqsave(&cdev->lock, flags);
	if (!dev->connected) {
		dev->connected = 1;
		schedule_work(&dev->work);
	}
	else if (c->bRequest == USB_REQ_SET_CONFIGURATION && cdev->config) {
		schedule_work(&dev->work);
	}
	spin_unlock_irqrestore(&cdev->lock, flags);

	return value;
}

static void android_disconnect(struct usb_gadget *gadget)
{
	struct android_dev *dev = _android_dev;
	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	unsigned long flags;

	composite_disconnect(gadget);

	spin_lock_irqsave(&cdev->lock, flags);
	dev->connected = 0;
	schedule_work(&dev->work);
	spin_unlock_irqrestore(&cdev->lock, flags);
}

static int android_create_device(struct android_dev *dev)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;
	int err;

	dev->dev = device_create(android_class, NULL,
					MKDEV(0, 0), NULL, "android0");
	if (IS_ERR(dev->dev))
		return PTR_ERR(dev->dev);

	dev_set_drvdata(dev->dev, dev);

	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
		if (err) {
			device_destroy(android_class, dev->dev->devt);
			return err;
		}
	}
	return 0;
}

/* compatible old style */
static int android_probe(struct platform_device *pdev)
{
	int x;
#ifdef CONFIG_SENSE_4_PLUS /* replace default pid by board id*/
	unsigned func = (1 << USB_FUNCTION_MTP) | (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB);
#else
	unsigned func = (1 << USB_FUNCTION_UMS) | (1 << USB_FUNCTION_ADB);
#endif
	_android_dev->platform_data = pdev->dev.platform_data;
	device_desc.idVendor = __constant_cpu_to_le16(_android_dev->platform_data->vendor_id);

#ifdef CONFIG_SYSFS
	if (sysfs_create_group(&pdev->dev.kobj, &android_usb_attr_group))
		printk(KERN_ERR "%s: fail to create sysfs\n", __func__);
#endif

	for (x = 0; x < htc_usb_pid_table.length ; x++) {
		if (htc_usb_pid_table.usb_pid_table_array[x].usb_function_flag == func) {
			htc_usb_pid_table.usb_pid_table_array[x].pid = _android_dev->platform_data->product_id;
			break;
		}
	}

	return 0;
}

static int android_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "%s\n", __func__);

#ifdef CONFIG_SYSFS
	sysfs_remove_group(&pdev->dev.kobj, &android_usb_attr_group);
#endif

	return 0;
}

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb", },
	.probe = android_probe,
	.remove = android_remove
};
/* compatible old style -- end */

static int __init init(void)
{
	struct android_dev *dev;
	int err;

	connect2pc = false;

	android_class = class_create(THIS_MODULE, "android_usb");
	if (IS_ERR(android_class))
		return PTR_ERR(android_class);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->functions = supported_functions;
	ClearEnableFunction(dev);
	INIT_WORK(&dev->work, android_work);
	INIT_WORK(&switch_adb_work, do_switch_adb_work);

	err = android_create_device(dev);
	if (err) {
		class_destroy(android_class);
		kfree(dev);
		return err;
	}
	mutex_init(&dev->mutex);
	_android_dev = dev;

	err = platform_driver_register(&android_platform_driver);
	if (err) {
		printk(KERN_ERR "[USB] fail to register  android_platform_driver\n");
		return err;
	}
	/* Override composite driver functions */
	composite_driver.setup = android_setup;
	composite_driver.disconnect = android_disconnect;

	return usb_composite_probe(&android_usb_driver, android_bind);
}
module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&android_usb_driver);
	class_destroy(android_class);
	kfree(_android_dev);
	_android_dev = NULL;
}
module_exit(cleanup);

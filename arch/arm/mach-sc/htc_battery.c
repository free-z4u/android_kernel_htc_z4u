/* arch/arm/mach-msm/htc_battery.c
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2008 Google, Inc.
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
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/power_supply.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/board_htc.h>
#include <mach/htc_battery.h>
#include <mach/adc.h>
#include <linux/rtc.h>
#include <linux/workqueue.h>
#include <linux/tps65200.h>
#include <linux/irq.h>
#include <linux/max17050_battery.h>
#include <linux/time.h>
#include "../../../drivers/staging/android/android_alarm.h"
#include <linux/alarmtimer.h>
#include <linux/interrupt.h>

#include <mach/hardware.h>
#include <mach/adi.h>
#include <mach/usb.h>
#include <mach/sci_glb_regs.h>
#include <mach/sci.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/fs.h>

#define BATTERY_TIME_VERSION "2014.01.28 v2"

#define USB_DM_GPIO 240
#define USB_DP_GPIO 241        //GPIO 为241 usb dp
static struct wake_lock vbus_wake_lock;

enum {
	HTC_BATT_DEBUG_M2A_RPC = 1U << 0,
	HTC_BATT_DEBUG_A2M_RPC = 1U << 1,
	HTC_BATT_DEBUG_UEVT = 1U << 2,
	HTC_BATT_DEBUG_USER_QUERY = 1U << 3,
	HTC_BATT_DEBUG_USB_NOTIFY = 1U << 4,
};
static int htc_batt_debug_mask = HTC_BATT_DEBUG_M2A_RPC | HTC_BATT_DEBUG_A2M_RPC
	| HTC_BATT_DEBUG_UEVT | HTC_BATT_DEBUG_USB_NOTIFY;
module_param_named(debug_mask, htc_batt_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define BATT_LOG(x...) do { \
struct timespec ts; \
struct rtc_time tm; \
getnstimeofday(&ts); \
rtc_time_to_tm(ts.tv_sec, &tm); \
printk(KERN_INFO "[BATT] " x); \
printk(" at(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", \
tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, \
tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec); \
} while (0)

#define BATT_ERR(x...) do { \
struct timespec ts; \
struct rtc_time tm; \
getnstimeofday(&ts); \
rtc_time_to_tm(ts.tv_sec, &tm); \
printk(KERN_ERR "[BATT] err:" x); \
printk(" at(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", \
tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, \
tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec); \
} while (0)



#if 0   ///should be removed, but, we keep this for function implementation
/* rpc related */

#define APP_BATT_PROG			0x30100001
#define APP_BATT_VER			MSM_RPC_VERS(0, 0)
#define HTC_PROCEDURE_BATTERY_NULL	0
#define HTC_PROCEDURE_GET_BATT_LEVEL	1
#define HTC_PROCEDURE_GET_BATT_INFO	2
#define HTC_PROCEDURE_GET_CABLE_STATUS	3
#define HTC_PROCEDURE_SET_BATT_DELTA	4
#define HTC_PROCEDURE_CHARGER_SWITCH    6
#define HTC_PROCEDURE_SET_FULL_LEVEL	7
#define HTC_PROCEDURE_GET_USB_ACCESSORY_ADC_LEVEL	10
#define HTC_PROCEDURE_GET_THERMAL_ADC	11
#endif

#define HTC_BATT_BOOTUP_VOLTAGE (3600)

const char *charger_tags[] = {"none", "USB", "AC", "SUPER AC", "WIRELESS CHARGER"};

struct htc_battery_info {
	int device_id;
	int present;
	unsigned long update_time;

	/* lock to protect the battery info */
	struct mutex lock;

	/* lock held while calling the arm9 to query the battery info */
/*	struct mutex rpc_lock; */
	struct battery_info_reply rep;
	int (*func_show_batt_attr)(struct device_attribute *attr, char *buf);
	int (*func_show_htc_extension_attr)(struct device_attribute *attr, char *buf);
	int gpio_mbat_in;
	int has_mbat_in_irq;
	int irq_mbat_in;
	int mbat_in_trigger_level;
	int mbat_in_keep_charging;
	int mbat_in_unreg_rmt;
	int gpio_usb_id;
	int gpio_vbus_det;
	int gpio_mchg_en_n;
	int gpio_iset;
	int guage_driver;
	int m2a_cable_detect;
	int charger;
	int gpio_adp_9v;
	unsigned int option_flag;
	int (*func_battery_charging_ctrl)(enum batt_ctl_t ctl);
	int (*func_htc_batt_context_event_handler)(enum batt_context_event event);
	int charger_re_enable;
	int irq;
};
/*
static struct msm_rpc_endpoint *endpoint;
*/
static struct htc_battery_info htc_batt_info;

/* Remove cache mechanism to prevent cable status not sync. */
static unsigned int cache_time;

static int htc_battery_initial = 0;
static int htc_full_level_flag = 0;
static int htc_is_DMB = 0;
static int charger_internal_power_enabled = 0;
static int otg_counter = 0;

static struct alarm batt_charger_ctrl_alarm;
static struct work_struct batt_charger_ctrl_work;
struct workqueue_struct *batt_charger_ctrl_wq;
static unsigned int charger_ctrl_stat;

/* to support bliss#C: force slow charging when playing audio with BT dock */
static unsigned int g_audio_stat = 0;	/* 1: playing */
//static unsigned int g_owe_docked = 0;	/* 1: one-wire device docked */

/* for context_state */
#define CONTEXT_STATE_BIT_TALK			(1)
#define CONTEXT_STATE_BIT_SEARCH		(1<<1)
static int context_state;

/* for limited charge */
static int chg_limit_active_mask;

/* for suspend high frequency (5min) */
static int suspend_highfreq_check_reason;
static int htc_batt_phone_call;
static int is_phone_call_set;

static int test_power_monitor;

struct workqueue_struct *vbus_int_wq;
struct work_struct vbus_int_work;
/*
#ifdef CONFIG_HTC_BATTCHG_SMEM
static void vbus_check_worker(struct work_struct *work);
static DECLARE_DELAYED_WORK(vbus_check_work, vbus_check_worker);
#endif
*/
/* for boot-up voltage notify */
static int enable_bootup_voltage = HTC_BATT_BOOTUP_VOLTAGE; // default value

static void mbat_in_func(struct work_struct *work);
static DECLARE_DELAYED_WORK(mbat_in_work, mbat_in_func);

extern struct timespec timespec_set(const long secs, const unsigned long nsecs);
static enum power_supply_property htc_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property htc_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

/* HTC dedicated attributes */
static ssize_t htc_battery_show_property(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static int htc_power_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val);

static int htc_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val);

static struct power_supply htc_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = htc_battery_properties,
		.num_properties = ARRAY_SIZE(htc_battery_properties),
		.get_property = htc_battery_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = htc_power_properties,
		.num_properties = ARRAY_SIZE(htc_power_properties),
		.get_property = htc_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = htc_power_properties,
		.num_properties = ARRAY_SIZE(htc_power_properties),
		.get_property = htc_power_get_property,
	},
	{
		.name = "wireless",
		.type = POWER_SUPPLY_TYPE_WIRELESS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = htc_power_properties,
		.num_properties = ARRAY_SIZE(htc_power_properties),
		.get_property = htc_power_get_property,
	},
};

static int update_batt_info(void);
static void usb_status_notifier_func(int online);

/*static int g_usb_online;*/
static struct t_usb_status_notifier usb_status_notifier = {
	.name = "htc_battery",
	.func = usb_status_notifier_func,
};
static int plugin_callback(int usb_cable, void *data);
static int plugout_callback(int usb_cable, void *data);
/*
 * we need usb module to detect
 * 1, plug in/out
 * 2, whether it is usb cable
 */
static struct usb_hotplug_callback power_cb = {
	.plugin = plugin_callback,
	.plugout = plugout_callback,
	.data = NULL,
};

/* Move cable detection/notification to standard PMIC RPC. */
static BLOCKING_NOTIFIER_HEAD(cable_status_notifier_list);
int register_notifier_cable_status(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&cable_status_notifier_list, nb);
}

int unregister_notifier_cable_status(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&cable_status_notifier_list, nb);
}

static BLOCKING_NOTIFIER_HEAD(wireless_charger_notifier_list);
int register_notifier_wireless_charger(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&wireless_charger_notifier_list, nb);
}

int unregister_notifier_wireless_charger(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&wireless_charger_notifier_list, nb);
}

/* -------------------------------------------------------------------------- */
#if 1 // enable it
/* For sleep charging screen. */
static int zcharge_enabled;
int htc_battery_get_zcharge_mode(void)
{
	return zcharge_enabled;
}
static int __init enable_zcharge_setup(char *str)
{
	int cal = simple_strtol(str, NULL, 0);

	zcharge_enabled = cal;
	return 1;
}
__setup("enable_zcharge=", enable_zcharge_setup);

/* For touch panel, touch panel may loss wireless charger notification when system boot up */
int htc_is_wireless_charger(void)
{
	if (htc_battery_initial)
		return (htc_batt_info.rep.charging_source == CHARGER_WIRELESS) ? 1 : 0;
	else
		return -1;
}
#endif

/*force_charger_type 1:for AC   2:for USB*/
static int force_charger_type = 0;
static int __init force_charger_type_setup(char *str)
{
	int ret;
	long cal;

	ret = strict_strtol(str, 0, &cal);

	if (ret) {
		printk("[BATT] error in force_charger_type_setup\n");
		return 1;
	}

	force_charger_type = (int)cal;
	printk("[BATT] [%s] set force_charger_type to %d ", __FUNCTION__, force_charger_type);
	return 1;
}

__setup("battery_fake_charge=", force_charger_type_setup);

// ,does not used, remove it or not
unsigned int batt_get_status(enum power_supply_property psp)
{

	union power_supply_propval val;

	if (update_batt_info())
		return -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY:
		mutex_lock(&htc_batt_info.lock);
		val.intval = htc_batt_info.rep.level;
		mutex_unlock(&htc_batt_info.lock);
		/* prevent shutdown before battery driver ready. */
		if (htc_batt_info.device_id == 0)
			val.intval = 55; /* 55 == ?? */
		break;
	case POWER_SUPPLY_PROP_TEMP:
		mutex_lock(&htc_batt_info.lock);
		val.intval = htc_batt_info.rep.batt_temp;
		mutex_unlock(&htc_batt_info.lock);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		mutex_lock(&htc_batt_info.lock);
		val.intval = htc_batt_info.rep.batt_vol;
		mutex_unlock(&htc_batt_info.lock);
		break;
	default:
		return -EINVAL;
	}

	return val.intval;

}

#if defined(CONFIG_DEBUG_FS)
static int htc_battery_set_charging(enum batt_ctl_t ctl);
static int batt_debug_set(void *data, u64 val)
{
	return htc_battery_set_charging((enum batt_ctl_t) val);
}

static int batt_debug_get(void *data, u64 *val)
{
	return -ENOSYS;
}

DEFINE_SIMPLE_ATTRIBUTE(batt_debug_fops, batt_debug_get, batt_debug_set, "%llu\n");
static int __init batt_debug_init(void)
{

	struct dentry *dent;

	dent = debugfs_create_dir("htc_battery", 0);
	if (IS_ERR(dent))
		return PTR_ERR(dent);

	debugfs_create_file("charger_state", 0644, dent, NULL, &batt_debug_fops);

	return 0;
}

device_initcall(batt_debug_init);
#endif

static int init_batt_gpio(void)
{ // enable it,for SPRD battery
	BATT_LOG("[%s] in ", __FUNCTION__);
	if (htc_batt_info.has_mbat_in_irq == 0 &&
		htc_batt_info.gpio_mbat_in > 0 &&
		gpio_request(htc_batt_info.gpio_mbat_in, "batt_detect") < 0)
		goto gpio_failed;
	if (htc_batt_info.gpio_mchg_en_n > 0 &&
		gpio_request(htc_batt_info.gpio_mchg_en_n, "charger_en") < 0)
		goto gpio_failed;
	if (htc_batt_info.gpio_iset > 0 &&
		gpio_request(htc_batt_info.gpio_iset, "charge_current") < 0)
		goto gpio_failed;
	if (htc_batt_info.gpio_adp_9v > 0 &&
		gpio_request(htc_batt_info.gpio_adp_9v, "super_charge_current") < 0)
		goto gpio_failed;

	return 0;

gpio_failed:
	return -EINVAL;

}

/*
 *	battery_charging_ctrl - battery charing control.
 * 	@ctl:			battery control command
 *
 */
int battery_charging_ctrl(enum batt_ctl_t ctl)
{
	int result = 0;

	BATT_LOG(" [%s] in ", __FUNCTION__);
#if 0  // enable it or not
	switch (ctl) {
	case DISABLE:
		BATT_LOG("charger OFF");
		/* 0 for enable; 1 disable */
		result = gpio_direction_output(htc_batt_info.gpio_mchg_en_n, 1);
		break;
	case ENABLE_SLOW_CHG:
		BATT_LOG("charger ON (SLOW)");
		result = gpio_direction_output(htc_batt_info.gpio_iset, 0);
		result = gpio_direction_output(htc_batt_info.gpio_mchg_en_n, 0);
		if (htc_batt_info.gpio_adp_9v > 0)
			result = gpio_direction_output(htc_batt_info.gpio_adp_9v, 0);
		break;
	case ENABLE_FAST_CHG:
		BATT_LOG("charger ON (FAST)");
		result = gpio_direction_output(htc_batt_info.gpio_iset, 1);
		result = gpio_direction_output(htc_batt_info.gpio_mchg_en_n, 0);
		if (htc_batt_info.gpio_adp_9v > 0)
			result = gpio_direction_output(htc_batt_info.gpio_adp_9v, 0);
		break;
	case ENABLE_SUPER_CHG:
		BATT_LOG("charger ON (SUPER)");
		result = gpio_direction_output(htc_batt_info.gpio_iset, 1);
		result = gpio_direction_output(htc_batt_info.gpio_mchg_en_n, 0);
		if (htc_batt_info.gpio_adp_9v > 0)
			result = gpio_direction_output(htc_batt_info.gpio_adp_9v, 1);
		break;
	default:
		BATT_ERR("%s: Not supported battery ctr called.!", __func__);
		result = -EINVAL;
		break;
	}
#endif
	return result;
}

static int htc_battery_set_charging(enum batt_ctl_t ctl)
{
	int rc;
	BATT_LOG("[%s] in", __FUNCTION__);
#if 1  //  enable it or not, nothing in battery_charging ctl.
	if ((rc = battery_charging_ctrl(ctl)) < 0)
		goto result;

	if (!htc_battery_initial) {
		htc_batt_info.rep.charging_enabled = ctl & 0x3;
	} else {
		mutex_lock(&htc_batt_info.lock);
		htc_batt_info.rep.charging_enabled = ctl & 0x3;
		mutex_unlock(&htc_batt_info.lock);
	}
result:
#endif
	return rc;
}

static int htc_battery_status_update(u32 curr_level)
{
	int notify;
	BATT_LOG("[%s]", __FUNCTION__);
#if 1  // enable it
	if (!htc_battery_initial)
		return 0;

	mutex_lock(&htc_batt_info.lock);
	notify = (htc_batt_info.rep.level != curr_level);
#if 0
	htc_batt_info.rep.level = 60; /* temporarily fake capacity */
#else
	htc_batt_info.rep.level = curr_level;
#endif
	mutex_unlock(&htc_batt_info.lock);

	/* we don't check level here for charging over temp RPC call */

	power_supply_changed(&htc_power_supplies[BATTERY_SUPPLY]);
	if (htc_batt_debug_mask & HTC_BATT_DEBUG_UEVT)
		BATT_LOG("power_supply_changed: battery");

#endif
	return 0;
}

static void update_wake_lock(int status)
{
	BATT_LOG("[%s]", __FUNCTION__);
#if 1  // enable it
	if (status == CHARGER_USB) {
		BATT_LOG("Keep device awake by battery");
		wake_lock(&vbus_wake_lock);
	} else {
		/* give userspace some time to see the uevent and update
		 * LED state or whatnot...
		 */
		wake_lock_timeout(&vbus_wake_lock, HZ * 5);
	}
#endif
}
#if 0
static int htc_cable_status_update(int status)
{
	int rc = 0;
	unsigned last_source;
	BATT_LOG("[%s]", __FUNCTION__);
#if 1
	//status = CHARGER_USB;//  work around it for it,2013/5/7
	if (!htc_battery_initial)
		return 0;

	if (status < CHARGER_BATTERY || status > CHARGER_WIRELESS) {
		BATT_ERR("%s: Not supported cable status received!", __func__);
		return -EINVAL;
	}

	mutex_lock(&htc_batt_info.lock);

	BATT_LOG("%s: charging source = %d -> status = %d\n", __func__, htc_batt_info.rep.charging_source, status);
#if 1  // change it to 0
	if (charger_internal_power_enabled != 0) {
		BATT_LOG("Ignore cable_status_update(%d) since boost mode enabled\n", status);
		mutex_unlock(&htc_batt_info.lock);
		return 0;
	}
	// htc_is_DMB need to be removed
	if ((status == htc_batt_info.rep.charging_source) && !(htc_is_DMB)) {
	/* When cable overvoltage(5V => 7V) A9 will report the same source, so only sent the uevent */
		if (status == CHARGER_USB) {
			power_supply_changed(&htc_power_supplies[USB_SUPPLY]);
			if (htc_batt_debug_mask & HTC_BATT_DEBUG_UEVT)
			BATT_LOG("(htc_cable_status_update)power_supply_changed: OverVoltage");
		}
		mutex_unlock(&htc_batt_info.lock);
		return 0;
	}

	last_source = htc_batt_info.rep.charging_source;
	/* TODO: replace charging_source to vbus_present */
	htc_batt_info.rep.charging_source = status;
	/* ARM9 should know the status it notifies,
	 * keep this code for old projects. */
	/* htc_set_smem_cable_type(status); */

	update_wake_lock(status);
	//blocking_notifier_call_chain(&cable_rpc_notifier_list, status, NULL); // remove RPC part

	/*ARM9 report CHARGER_AC while plug in htc_adaptor which is identify by usbid*/
	/*don't need to notify usb driver*/
	/* These are legacy codes for MSM7200 */
	if (status == CHARGER_AC) {
		//htc_set_smem_cable_type(CHARGER_AC);  // mark it
		power_supply_changed(&htc_power_supplies[AC_SUPPLY]);
	} else {
		if (status == CHARGER_WIRELESS) {
			BATT_LOG("batt: Wireless charger detected. "
				"We don't need to inform USB driver.");
			blocking_notifier_call_chain(&wireless_charger_notifier_list, status, NULL);
			power_supply_changed(&htc_power_supplies[WIRELESS_SUPPLY]);
			update_wake_lock(htc_batt_info.rep.charging_source);
		} else {
			/* We need to notify other driver as wireless charger out. */
			if (last_source == CHARGER_WIRELESS)
				blocking_notifier_call_chain(&wireless_charger_notifier_list, status, NULL);
		}
	}

	if (status == CHARGER_BATTERY) {
		power_supply_changed(&htc_power_supplies[BATTERY_SUPPLY]);
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_UEVT)
		BATT_LOG("(htc_cable_status_update)power_supply_changed: battery");
	}

#else
	/* A9 reports USB charging when helf AC cable in and China AC charger. */
	/* notify userspace USB charging first,
	and then usb driver will notify AC while D+/D- Line short. */
	/* China AC detection:
	 * Write SMEM as USB first, and update SMEM to AC
	 * if receives AC notification */
	last_source = htc_batt_info.rep.charging_source;
	if (status == CHARGER_USB && g_usb_online == 0) {
		//htc_set_smem_cable_type(CHARGER_USB);
		htc_batt_info.rep.charging_source = CHARGER_USB;
	} else {
		//htc_set_smem_cable_type(status);
		htc_batt_info.rep.charging_source  = status;
		/* usb driver will not notify usb offline. */
		if (status == CHARGER_BATTERY && g_usb_online != 0)
			g_usb_online = 0;
	}

	//msm_hsusb_set_vbus_state(status == CHARGER_USB);

	blocking_notifier_call_chain(&cable_status_notifier_list,
		htc_batt_info.rep.charging_source, NULL);

	if (htc_batt_info.rep.charging_source != last_source) {
		/* Lock suspend only when USB in for ADB or other USB functions. */
		if (htc_batt_info.rep.charging_source == CHARGER_USB) {
			wake_lock(&vbus_wake_lock);
		} else if (__htc_power_policy()) {
			/* Lock suspend for DOPOD charging animation */
			wake_lock(&vbus_wake_lock);
		} else {
			if (htc_batt_info.rep.charging_source == CHARGER_AC
				&& last_source == CHARGER_USB)
				BATT_ERR("%s: USB->AC\n", __func__);
			/* give userspace some time to see the uevent and update
			 * LED state or whatnot...
			 */
			wake_lock_timeout(&vbus_wake_lock, HZ * 5);
		}
		if (htc_batt_info.rep.charging_source == CHARGER_BATTERY || last_source == CHARGER_BATTERY)
			power_supply_changed(&htc_power_supplies[BATTERY_SUPPLY]);
		if (htc_batt_info.rep.charging_source == CHARGER_USB || last_source == CHARGER_USB)
			power_supply_changed(&htc_power_supplies[USB_SUPPLY]);
		if (htc_batt_info.rep.charging_source == CHARGER_AC || last_source == CHARGER_AC)
			power_supply_changed(&htc_power_supplies[AC_SUPPLY]);
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_UEVT)
			BATT_LOG("power_supply_changed: %s -> %s",
				charger_tags[last_source], charger_tags[htc_batt_info.rep.charging_source]);
	}
#endif
	mutex_unlock(&htc_batt_info.lock);
	htc_is_DMB = 0;
#endif
	return rc;
}
#endif
#if 0 // , remove it, change to ADC read
#ifdef CONFIG_USB_ACCESSORY_DETECT_BY_ADC
int htc_get_usb_accessory_adc_level(uint32_t *buffer)
{
	struct rpc_request_hdr req;

	struct htc_get_usb_adc_value_rep {
		struct rpc_reply_hdr hdr;
		uint32_t adc_value;
	} rep;

	int rc;
	BATT_LOG(KERN_INFO "[BATT] %s\n", __func__);

	if (buffer == NULL) {
		BATT_LOG(KERN_INFO "[BATT] %s: buffer null\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR(endpoint))
		return -EINVAL;

	rc = msm_rpc_call_reply(endpoint, HTC_PROCEDURE_GET_USB_ACCESSORY_ADC_LEVEL,
				&req, sizeof(req),
				&rep, sizeof(rep),
				5 * HZ);
	if (rc < 0) {
		BATT_ERR("%s: msm_rpc_call_reply failed (%d)!", __func__, rc);
		return rc;
	}
	*buffer 		= be32_to_cpu(rep.adc_value);

	printk(KERN_INFO "[BATT] %s: adc = %d\n", __func__, *buffer);
	return 0;
}
EXPORT_SYMBOL(htc_get_usb_accessory_adc_level);
#endif
#endif

#if 0  //, remove it, change to read data from gauge IC
#ifdef CONFIG_THERMAL_TEMPERATURE_READ
int htc_get_thermal_adc_level(uint32_t *buffer)
{
	struct rpc_request_hdr req;

	struct htc_get_thermal_adc_value_rep {
		struct rpc_reply_hdr hdr;
		uint32_t adc_value;
	} rep;

	int rc;

	if (buffer == NULL) {
		BATT_LOG(KERN_INFO "[BATT] %s: buffer null\n", __func__);
		return -EINVAL;
	}

	rc = msm_rpc_call_reply(endpoint, HTC_PROCEDURE_GET_THERMAL_ADC,
				&req, sizeof(req),
				&rep, sizeof(rep),
				5 * HZ);
	if (rc < 0) {
		BATT_ERR("%s: msm_rpc_call_reply failed (%d)!", __func__, rc);
		return rc;
	}
	*buffer 		= be32_to_cpu(rep.adc_value);

	BATT_LOG(KERN_INFO "[BATT] %s: adc = %d\n", __func__, *buffer);
	return 0;
}
EXPORT_SYMBOL(htc_get_thermal_adc_level);
#endif
#endif
static int htc_batt_internal_power_handle_lock(int online)
{
	int handled = 0;
	BATT_LOG("[%s]", __FUNCTION__);
#if 1 // enable it
	switch (online) {
	case CONNECT_TYPE_INTERNAL:
		if (htc_batt_info.charger == SWITCH_CHARGER_TPS65200) {
			tps_set_charger_ctrl(POWER_SUPPLY_ENABLE_INTERNAL);
			charger_internal_power_enabled = 1;
			otg_counter ++;
		}
		handled = 1;
		break;
	case CONNECT_TYPE_CLEAR:
		if (charger_internal_power_enabled != 0) {
			charger_internal_power_enabled = 0;
			if (htc_batt_info.charger == SWITCH_CHARGER_TPS65200) {
				tps_set_charger_ctrl(POWER_SUPPLY_DISABLE_CHARGE);
			}
			handled = 1;
		}
		break;
	default:
		if (charger_internal_power_enabled != 0) {
			BATT_LOG("[Warning] Unexpected online(%d) after boost power!!\n", online);
			charger_internal_power_enabled = 0;
		}
	};
#endif
	return handled;
}

int check_boost_mode_function(void)
{
	return charger_internal_power_enabled;
}
EXPORT_SYMBOL(check_boost_mode_function);

static void peripheral_cable_update(int online, int from_owe)
{
#if 1 // change it to 0
	int ret = 0;
	BATT_LOG("%s( online = %d) ", __func__, online);
	//htc_cable_status_update(online);
	mutex_lock(&htc_batt_info.lock);

	/* both usb and owe drivers callback to this function
		to update charger type.
		if from owe, online = {1,2}
		if from usb, online = {0,1,2,3}
	Here, owe's notify is our first priority, if we got it. */

	ret = htc_batt_internal_power_handle_lock(online);
	if (ret != 0) {
		/* online type is handled */
		mutex_unlock(&htc_batt_info.lock);
		return;
	}

	/* TODO: replace charging_source to usb_status */
	/*In case of DMB inserting, do not drain power from dongle */
	if (online == CHARGER_CLEAR) {
		online = CHARGER_BATTERY;
		htc_is_DMB = 1;
	}

	htc_batt_info.rep.charging_source = online;

	update_wake_lock(htc_batt_info.rep.charging_source);
	/* TODO: use power_supply_change to notify battery drivers. */
	/* If more than one Gauge IC enable, add function here.*/
	blocking_notifier_call_chain(&cable_status_notifier_list,
			htc_batt_info.rep.charging_source, NULL);
	if (!htc_battery_initial) {
		pr_err("\n\n[BATT] ### htc_battery_code is not inited yet! ###\n\n");
	}
	mutex_unlock(&htc_batt_info.lock);
#else
	mutex_lock(&htc_batt_info.lock);
	if (htc_batt_debug_mask & HTC_BATT_DEBUG_USB_NOTIFY)
		BATT_LOG("%s: online=%d, g_usb_online=%d", __func__, online, g_usb_online);
	if (g_usb_online != online) {
		g_usb_online = online;
		if (online == CHARGER_AC && htc_batt_info.rep.charging_source == CHARGER_USB) {
			mutex_unlock(&htc_batt_info.lock);
			htc_cable_status_update(CHARGER_AC);
			mutex_lock(&htc_batt_info.lock);
		}
	}
	mutex_unlock(&htc_batt_info.lock);
#endif
}

/* A9 reports USB charging when helf AC cable in and China AC charger. */
/* notify userspace USB charging first,
and then usb driver will notify AC while D+/D- Line short. */
static void usb_status_notifier_func(int online)
{
	BATT_LOG("[%s]", __FUNCTION__);

	peripheral_cable_update(online, 0);

	return;
}
static int htc_get_batt_info(struct battery_info_reply *buffer)
{

#if 1 // replace it with our code, battery info
	int rc;
	struct battery_info_reply batt_info;
	if(htc_battery_initial){
		rc = max17050_get_battery_info(&batt_info); //get battery info from gauge IC
		if (rc < 0) {
			BATT_ERR("%s: max17050_get_battery_info failed (%d)!", __func__, rc);
			return rc;
		}
	}
	mutex_lock(&htc_batt_info.lock);
	buffer->batt_id 		= batt_info.batt_id;
	buffer->batt_vol 		= batt_info.batt_vol;
	buffer->batt_temp 		= batt_info.batt_temp;
	buffer->batt_current 		= batt_info.batt_current;
	buffer->level 			= batt_info.level;
	/* Move the rules of charging_source to cable_status_update. */
	/* buffer->charging_source 	= batt_info.charging_source; */
	buffer->charging_enabled 	= batt_info.charging_enabled;
	buffer->full_bat 		= batt_info.full_bat;
	buffer->over_vchg               = htc_batt_info.rep.over_vchg;
	buffer->otg_count               = otg_counter;
	/* Over_vchg only update in SMEM from A9 */
	mutex_unlock(&htc_batt_info.lock);

	/*BATT_LOG("get_batt_info: batt_id=%d, batt_vol=%d, batt_temp=%d, "
			"batt_current=%d, level=%d, charging_source=%d, "
			"charging_enabled=%d, full_bat=%d, over_vchg=%d",
			buffer->batt_id, buffer->batt_vol, buffer->batt_temp,
			buffer->batt_current, buffer->level, buffer->charging_source,
			buffer->charging_enabled, buffer->full_bat, buffer->over_vchg);*/

#endif
	return 0;
}

static ssize_t htc_battery_show_batt_attr(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	//for file node: batt_attr_text write function
	// enable it, remember to enable htc_battery_show_attr in max17050_battery.c
	//if (htc_batt_info.guage_driver == GAUGE_MAX17050)
	return htc_batt_info.func_show_batt_attr(attr, buf);

}

static ssize_t htc_battery_show_htc_extension_attribute(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	if (htc_batt_info.func_show_htc_extension_attr)
		return htc_batt_info.func_show_htc_extension_attr(attr, buf);
	return 0;
}
/* -------------------------------------------------------------------------- */
static int htc_power_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{ //Note, this function is needed by Probe funcition, so ,enable it
	enum charger_type_t charger;

	mutex_lock(&htc_batt_info.lock);

	charger = htc_batt_info.rep.charging_source; //CHARGER_USB; //
	/* ARM9 decides charging_enabled value by battery id */
	if (htc_batt_info.rep.batt_id == 255)
		charger = CHARGER_BATTERY;

	mutex_unlock(&htc_batt_info.lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			if (charger == CHARGER_AC || charger == CHARGER_9V_AC)
				val->intval = 1;
			else
				val->intval = 0;
			BATT_LOG("%s: %s: online=%d", __func__, psy->name, val->intval);
		} else if (psy->type == POWER_SUPPLY_TYPE_USB) {
			val->intval = (charger ==  CHARGER_USB ? 1 : 0);
			BATT_LOG("%s: %s: online=%d", __func__, psy->name, val->intval);
		} else if (psy->type == POWER_SUPPLY_TYPE_WIRELESS) {
			val->intval = (charger ==  CHARGER_WIRELESS ? 1 : 0);
			BATT_LOG("%s: %s: online=%d", __func__, psy->name, val->intval);
		} else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void tps_int_notifier_func(int int_reg, int value);
static struct tps65200_chg_int_notifier tps_int_notifier = {
	.name = "htc_battery",
	.func = tps_int_notifier_func,
};

static void tps_int_notifier_func(int int_reg, int value)
{
	BATT_LOG("[%s]", __FUNCTION__);
/* This order is for Projects use DS2746 and SMEM, in this situation,
use htc_batt_info.rep.over_vchg only*/
	if(int_reg == CHECK_INT1) {
		htc_batt_info.rep.over_vchg = (unsigned int)value;
		if(!value) tps_set_charger_ctrl(htc_batt_info.rep.charging_source);
		power_supply_changed(&htc_power_supplies[BATTERY_SUPPLY]);
	} else if (int_reg == CHECK_INT2) {
		// do nothing for our device
	}

}

/* Once charge full, set this flag */
static int htc_charge_full = 0;

static int htc_battery_get_charging_status(void)
{ //Note, this funtion it needed in Probe function, so enable it
	int ret = 0;
#if 1
	u32 level;
	enum charger_type_t charger;

	mutex_lock(&htc_batt_info.lock);

	charger = htc_batt_info.rep.charging_source; // CHARGER_USB; //

	/* ARM9 decides charging_enabled value by battery id */
	/* if (htc_batt_info.rep.batt_id == 255)
		charger = CHARGER_UNKNOWN;*/
	if (htc_batt_info.rep.batt_id == 255 || htc_batt_info.rep.batt_id == BATTERY_ID_UNKNOWN)
		charger = CHARGER_UNKNOWN;

	//charger = CHARGER_USB;
	switch (charger) {
	case CHARGER_BATTERY:
		htc_charge_full = 0;
		ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case CHARGER_USB:
	case CHARGER_AC:
	case CHARGER_9V_AC:
	case CHARGER_WIRELESS:
		level = htc_batt_info.rep.level;
		if (level == 100)
			htc_charge_full = 1;
		else
			htc_charge_full = 0;

		if (htc_charge_full)
			ret = POWER_SUPPLY_STATUS_FULL;
		else if (htc_batt_info.rep.charging_enabled != 0)
			ret = POWER_SUPPLY_STATUS_CHARGING;
		else
			ret = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	default:
		ret = POWER_SUPPLY_STATUS_UNKNOWN;
	}
	mutex_unlock(&htc_batt_info.lock);
#endif
	BATT_LOG("[%s]ret = %d, charger=%d", __FUNCTION__, ret, charger);
	return ret;
}

static int htc_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{ //Note, this funtion is needed by Probe function, so enable it.
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = htc_battery_get_charging_status();
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_USER_QUERY)
			BATT_LOG("%s: %s: status=%d", __func__, psy->name, val->intval);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		if (htc_batt_info.guage_driver == GAUGE_MAX17050) {
			if (htc_batt_info.rep.temp_fault)
				val->intval =  POWER_SUPPLY_HEALTH_OVERHEAT;
		} else {
			if (htc_batt_info.rep.batt_temp > 480 ||
				htc_batt_info.rep.batt_temp < 0)
				val->intval =  POWER_SUPPLY_HEALTH_OVERHEAT;
		}

		if (htc_batt_debug_mask & HTC_BATT_DEBUG_USER_QUERY)
			BATT_LOG("%s: %s: health=%d", __func__, psy->name, val->intval);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = htc_batt_info.present;
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_USER_QUERY)
			BATT_LOG("%s: %s: present=%d", __func__, psy->name, val->intval);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_USER_QUERY)
			BATT_LOG("%s: %s: technology=%d", __func__, psy->name, val->intval);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		mutex_lock(&htc_batt_info.lock);
		/* [PRIMODD][PRIMOTD]
		 * return fake batt level to framework for temp
		 * remove it after radio image update
		 */
		/* overwrite fake info if test by power monitor flag is set */
		if (test_power_monitor)
			val->intval = 77;
		else
			val->intval = htc_batt_info.rep.level;

		/* prevent shutdown before battery driver ready. */
		if (htc_batt_info.device_id == 0)
			val->intval = 55; /* 55 == ?? */
		mutex_unlock(&htc_batt_info.lock);
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_USER_QUERY)
			BATT_LOG("%s: %s: capacity=%d, test_power_monitor=%d.", __func__, psy->name, val->intval, test_power_monitor);
		break;
	case POWER_SUPPLY_PROP_OVERLOAD:
		val->intval = htc_batt_info.rep.overloading_charge;
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_USER_QUERY)
			BATT_LOG("%s: %s: overloading_charge=%d", __func__, psy->name, val->intval);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* The *owner field is no longer used since x86 tree has been cleaned up. */
#define HTC_BATTERY_ATTR(_name)							\
{										\
	.attr = { .name = #_name, .mode = S_IRUGO},	\
	.show = htc_battery_show_property,					\
	.store = NULL,								\
}

static struct device_attribute htc_battery_attrs[] = {
	HTC_BATTERY_ATTR(batt_id),
	HTC_BATTERY_ATTR(batt_vol),
	HTC_BATTERY_ATTR(batt_temp),
	HTC_BATTERY_ATTR(batt_current),
	HTC_BATTERY_ATTR(charging_source),
	HTC_BATTERY_ATTR(charging_enabled),
	HTC_BATTERY_ATTR(full_bat),
	HTC_BATTERY_ATTR(over_vchg),
/*	HTC_BATTERY_ATTR(thermal_temp),*/
	HTC_BATTERY_ATTR(batt_state),
	HTC_BATTERY_ATTR(otg_count),
/*[FIXME]__ATTR(batt_attr_raw, S_IRUGO, htc_battery_show_batt_attr, NULL),*/
	__ATTR(batt_attr_text, S_IRUGO, htc_battery_show_batt_attr, NULL),
	__ATTR(htc_extension, S_IRUGO, htc_battery_show_htc_extension_attribute, NULL),
};

enum {
	BATT_ID = 0,
	BATT_VOL,
	BATT_TEMP,
	BATT_CURRENT,
	CHARGING_SOURCE,
	CHARGING_ENABLED,
	FULL_BAT,
	OVER_VCHG,
	/*THERMAL_TEMP,*/
	BATT_STATE,
	OTG_COUNT,
};

static int htc_charger_switch(unsigned enable)
{
	int ret = 0;
	BATT_LOG("%s: switch charger to mode: %u", __func__, enable);
	if (enable == ENABLE_LIMIT_CHARGER)
		ret = tps_set_charger_ctrl(ENABLE_LIMITED_CHG);
	else if (enable == DISABLE_LIMIT_CHARGER)
		ret = tps_set_charger_ctrl(CLEAR_LIMITED_CHG);
	else {
		if (htc_batt_info.guage_driver == GAUGE_MAX17050)
			max17050_charger_switch(enable);

		power_supply_changed(&htc_power_supplies[CHARGER_BATTERY]);
	}
	return ret;
}

#if 0   //enable it
static int htc_rpc_set_full_level(unsigned level)
{
	int ret = 0;
	struct set_batt_full_level_req {
		struct rpc_request_hdr hdr;
		uint32_t data;
	} req;

	req.data = cpu_to_be32(level);
	ret = msm_rpc_call(endpoint, HTC_PROCEDURE_SET_FULL_LEVEL,
			    &req, sizeof(req), 5 * HZ);
	if (ret < 0)
		BATT_ERR("%s: msm_rpc_call failed (%d)!", __func__, ret);
	return ret;
}

#endif

/*
*	For PA and QA test
*	0x10-> fake temp to 250
*	0x11->TBD if needed
*	0x12->TBD if needed
*	....
*/
static ssize_t htc_battery_debug_flag(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{  //enanle it for PA test, write file node: batt_debug_flag
#if 1

	unsigned long debug_flag;
	debug_flag = simple_strtoul(buf, NULL, 10);

	if (debug_flag > 100 || debug_flag == 0)
		return -EINVAL;

	mutex_lock(&htc_batt_info.lock);
	blocking_notifier_call_chain(&cable_status_notifier_list,
		debug_flag, 0);
	mutex_unlock(&htc_batt_info.lock);
	BATT_LOG("[%s]", __FUNCTION__);
	return count;
#endif
    return 0;
}

int htc_battery_charger_disable()
{//not used, remove it(for USB using...)
	int rc = 0;
#if 1  // enable it
	//mutex_lock(&htc_batt_info.rpc_lock);
	rc = htc_charger_switch(0);
	//mutex_unlock(&htc_batt_info.rpc_lock);
#endif
	return rc;
}

static ssize_t htc_battery_charger_stat(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{// file node: read charger_control
	int i = 0;

	i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", charger_ctrl_stat);

	return i;
}

static void __context_event_handler(enum batt_context_event event)
{
	int rc;
	BATT_LOG("handle context event(%d)", event);
#if 1  // enable it
	switch (event) {
	case EVENT_TALK_START:
		if (chg_limit_active_mask & HTC_BATT_CHG_LIMIT_BIT_TALK) {
			rc = htc_charger_switch(ENABLE_LIMIT_CHARGER);
			if (rc < 0)
				return;

			alarm_cancel(&batt_charger_ctrl_alarm);
			charger_ctrl_stat = ENABLE_LIMIT_CHARGER;
		}
		if (suspend_highfreq_check_reason & SUSPEND_HIGHFREQ_CHECK_BIT_TALK) {
			htc_batt_phone_call = 1;
		if (htc_batt_info.guage_driver == GAUGE_MAX17050)
			max17050_phone_call_in(htc_batt_phone_call);
		}
		break;
	case EVENT_TALK_STOP:
		if (chg_limit_active_mask & HTC_BATT_CHG_LIMIT_BIT_TALK) {
			rc = htc_charger_switch(DISABLE_LIMIT_CHARGER);
			if (rc < 0)
				return;

			alarm_cancel(&batt_charger_ctrl_alarm);
			charger_ctrl_stat = DISABLE_LIMIT_CHARGER;
		}
		if (suspend_highfreq_check_reason & SUSPEND_HIGHFREQ_CHECK_BIT_TALK) {
			htc_batt_phone_call = 0;
			if (htc_batt_info.guage_driver == GAUGE_MAX17050)
				max17050_phone_call_in(htc_batt_phone_call);
		}
		break;
	case EVENT_NETWORK_SEARCH_START:
		break;
	case EVENT_NETWORK_SEARCH_STOP:
		break;
	default:
		pr_warn("unsupported context event (%d)\n", event);
		break;
	}
#endif
	// htc_batt_schedule_batt_info_update();
}

struct mutex context_event_handler_lock; /* synchroniz context_event_handler */
static int htc_batt_context_event_handler(enum batt_context_event event)
{
#if 1  // enable it,handle phone call info
	int prev_context_state;
	mutex_lock(&context_event_handler_lock);
	prev_context_state = context_state;

	/* STEP.1: check if state not changed then return */
	switch (event) {
	case EVENT_TALK_START:
		if (context_state & CONTEXT_STATE_BIT_TALK)
			goto exit;
		context_state |= CONTEXT_STATE_BIT_TALK;
		break;
	case EVENT_TALK_STOP:
		if (!(context_state & CONTEXT_STATE_BIT_TALK))
			goto exit;
		context_state &= ~CONTEXT_STATE_BIT_TALK;
		break;
	case EVENT_NETWORK_SEARCH_START:
		if (context_state & CONTEXT_STATE_BIT_SEARCH)
			goto exit;
		context_state |= CONTEXT_STATE_BIT_SEARCH;
		break;
	case EVENT_NETWORK_SEARCH_STOP:
		if (!(context_state & CONTEXT_STATE_BIT_SEARCH))
			goto exit;
		context_state &= ~CONTEXT_STATE_BIT_SEARCH;
		break;
	default:
		pr_warn("unsupported context event (%d)\n", event);
		goto exit;
	}
	BATT_LOG("context_state: 0x%x -> 0x%x", prev_context_state, context_state);

	/* STEP.2: handle incoming event */
	__context_event_handler(event);

exit:
	mutex_unlock(&context_event_handler_lock);
#endif
	return 0;
}

static ssize_t htc_battery_charger_switch(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
#if 1 // enable it for SSD tool, write charger_control
	int rc;
	unsigned long enable = 0;

	enable = simple_strtoul(buf, NULL, 10);

	if (enable >= END_CHARGER)
		return -EINVAL;

	/* To check if limit charging is seeting by phone call or not */
	if (enable == DISABLE_LIMIT_CHARGER) {
		htc_batt_context_event_handler(EVENT_TALK_STOP);
		return count;
	}
	if (enable == ENABLE_LIMIT_CHARGER) {
		htc_batt_context_event_handler(EVENT_TALK_START);
		return count;
	}
	/* Remove when all setting unify to attribute phone_call*/

	rc = htc_charger_switch(enable);

	if (rc < 0)
		return rc;

	alarm_cancel(&batt_charger_ctrl_alarm);
	charger_ctrl_stat = (unsigned int)enable;

	return count;
#endif
    return 0;
}

static ssize_t htc_battery_charger_timer(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
#if 1 // enable it for SSD Tool, charger_timer
	int rc;
	unsigned long time_out = 0;
	struct timespec interval;
	struct timespec next_alarm;

	time_out = simple_strtoul(buf, NULL, 10);

	if (time_out > 65536)
		return -EINVAL;

	if (time_out > 0) {
		rc = htc_charger_switch(STOP_CHARGER);
		if (rc < 0)
			return rc;
		interval = timespec_set(time_out, 0);
		get_monotonic_boottime(&next_alarm);
		next_alarm = timespec_add(next_alarm, interval);
		alarm_start(&batt_charger_ctrl_alarm, timespec_to_ktime(interval));
		charger_ctrl_stat = STOP_CHARGER;
	} else if (time_out == 0) {
		rc = htc_charger_switch(ENABLE_CHARGER);
		if (rc < 0)
			return rc;
		alarm_cancel(&batt_charger_ctrl_alarm);
		charger_ctrl_stat = ENABLE_CHARGER;
	}

#endif
	return count;
}

static ssize_t htc_battery_set_full_level(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{  // enable it for SSD Tool, full_level
	int rc = 0;
	unsigned long percent = 100;

	percent = simple_strtoul(buf, NULL, 10);
#if 1
	if (percent > 100 || percent == 0)
		return -EINVAL;

	if (htc_batt_info.guage_driver == GAUGE_MAX17050) {
		if (htc_full_level_flag == 0) {
			mutex_lock(&htc_batt_info.lock);
			htc_full_level_flag = 1;
			htc_batt_info.rep.full_level = percent;
			blocking_notifier_call_chain(&cable_status_notifier_list,
				0xff, (void *) &htc_batt_info.rep.full_level);
			mutex_unlock(&htc_batt_info.lock);
			}
		rc = 0;
	}
	if (rc < 0)
		return rc;
#endif
	return count;
}

static ssize_t htc_battery_get_audio_stat(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;

	i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", g_audio_stat);
	return i;
}

static ssize_t htc_battery_set_audio_stat(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
#if 0  // enable it for audio
	unsigned long stat = 0;
	int charging_enable = htc_batt_info.rep.charging_source;
	stat = simple_strtoul(buf, NULL, 10);

	if (stat > 0)
		g_audio_stat = 1;
	else
		g_audio_stat = 0;

	if (htc_batt_info.option_flag & OPTION_FLAG_BT_DOCK_CHARGE_CTL) {
		if (g_audio_stat && g_owe_docked)
			charging_enable = POWER_SUPPLY_ENABLE_SLOW_CHARGE;
		else
			charging_enable = htc_batt_info.rep.charging_source;
		pr_info("[BATT] %s() BT_DOCK_CHARGE_CTL:%d\n",
			__func__, charging_enable);
		tps_set_charger_ctrl(charging_enable);
	}
#endif
	return count;
}

static ssize_t htc_battery_set_phone_call(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{  // enable it, phone_call
	unsigned long phone_call = 0;
	int rc = 0;

	rc = strict_strtoul(buf, 10, &phone_call);
	if (rc)
		return rc;
#if 1
	BATT_LOG("suspend_highfreq_check_reason/chg_limit_active_mask=%d/%d",
		suspend_highfreq_check_reason, chg_limit_active_mask);

	if (phone_call) {
		htc_batt_info.func_htc_batt_context_event_handler(EVENT_TALK_START);
	} else {
		htc_batt_info.func_htc_batt_context_event_handler(EVENT_TALK_STOP);
	}
	BATT_LOG("set context phone_call=%lu", phone_call);
#endif
	return count;
}


 static ssize_t htc_battery_get_phone_call(struct device *dev,
		 						struct device_attribute *attr,
								char *buf)
{
	int i = 0;
	i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", htc_batt_phone_call);
	return i;
}

static ssize_t htc_battery_get_current_now(struct device *dev,
								struct device_attribute *attr,
													char *buf)
{
	int i = 0;
//	int current_now;
//	current_now = max17050_read_current();
	i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",  max17050_read_current());
	return i;
}
static struct device_attribute htc_set_delta_attrs[] = {
	/*__ATTR(delta, S_IWUSR | S_IWGRP, NULL, htc_battery_set_delta),*/
	__ATTR(full_level, S_IWUSR | S_IWGRP, NULL,
						htc_battery_set_full_level),
	__ATTR(batt_debug_flag, S_IWUSR | S_IWGRP, NULL,
						htc_battery_debug_flag),
	__ATTR(charger_control, S_IWUSR | S_IWGRP, htc_battery_charger_stat,
						htc_battery_charger_switch),
	__ATTR(charger_timer, S_IWUSR | S_IWGRP, NULL,
						htc_battery_charger_timer),
	__ATTR(audio_stat, S_IWUSR | S_IWGRP | S_IRUSR, htc_battery_get_audio_stat,
						htc_battery_set_audio_stat),
	__ATTR(phone_call, S_IRUGO|S_IWUSR|S_IWGRP, htc_battery_get_phone_call,
						htc_battery_set_phone_call),
	__ATTR(batt_current_now, S_IROTH|S_IRUSR|S_IRGRP, htc_battery_get_current_now,
						NULL),
};

static int htc_battery_create_attrs(struct device * dev)
{
	int i = 0, j = 0, rc = 0;

	for (i = 0; i < ARRAY_SIZE(htc_battery_attrs); i++) {
		rc = device_create_file(dev, &htc_battery_attrs[i]);
		if (rc)
			goto htc_attrs_failed;
	}

	for (j = 0; j < ARRAY_SIZE(htc_set_delta_attrs); j++) {
		rc = device_create_file(dev, &htc_set_delta_attrs[j]);
		if (rc)
			goto htc_delta_attrs_failed;
	}
	goto succeed;

htc_attrs_failed:
	while (i--)
		device_remove_file(dev, &htc_battery_attrs[i]);
htc_delta_attrs_failed:
	while (j--)
		device_remove_file(dev, &htc_set_delta_attrs[j]);
succeed:
	return rc;
}

static int update_batt_info(void)
{
	int ret = 0;
	/* FIXME */
	if (htc_batt_info.guage_driver == GAUGE_MAX17050)
		htc_get_batt_info(&htc_batt_info.rep);

	return ret;
}

static ssize_t htc_battery_show_property(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - htc_battery_attrs;

	/* rpc lock is used to prevent two threads from calling
	 * into the get info rpc at the same time
	 */
	/* check cache time to decide if we need to update */
	if (htc_batt_info.update_time &&
			time_before(jiffies, htc_batt_info.update_time +
			msecs_to_jiffies(cache_time))) {
		BATT_LOG("%s: use cached values", __func__);
		goto dont_need_update;
	}

	if (!update_batt_info())
		htc_batt_info.update_time = jiffies;
dont_need_update:
	mutex_lock(&htc_batt_info.lock);
	switch (off) {
	case BATT_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.batt_id);
		break;
	case BATT_VOL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.batt_vol);
		break;
	case BATT_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.batt_temp);
		break;
	case BATT_CURRENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.batt_current);
		break;
	case CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.charging_source);
		break;
	case CHARGING_ENABLED:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.charging_enabled);
		break;
	case FULL_BAT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.full_bat);
		break;
	case OVER_VCHG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.over_vchg);
		break;
	case BATT_STATE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.batt_state);
		break;
	/*case THERMAL_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.thermal_temp);
		break;*/
	case OTG_COUNT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       htc_batt_info.rep.otg_count);
		break;
	default:
		i = -EINVAL;
	}
	mutex_unlock(&htc_batt_info.lock);

	if (htc_batt_debug_mask & HTC_BATT_DEBUG_USER_QUERY) {
		if (i < 0)
			BATT_LOG("%s: battery: attribute is not supported: %d", __func__, off);
		else
			BATT_LOG("%s: battery: %s=%s", __func__, attr->attr.name, buf);
	}
	return i;
}

static void batt_charger_ctrl_func(struct work_struct *work)
{
//by init timer, batt_charger_ctrl_alarm_handler
	int rc;
	rc = htc_charger_switch(ENABLE_CHARGER);

	if (rc)
		return;

	charger_ctrl_stat = (unsigned int)ENABLE_CHARGER;
}

static enum alarmtimer_restart batt_charger_ctrl_alarm_handler(struct alarm *alarm, ktime_t now)
{
	BATT_LOG("charger control alarm is timeout.");

	queue_work(batt_charger_ctrl_wq, &batt_charger_ctrl_work);
	return ALARMTIMER_NORESTART;
}

/* MBAT_IN interrupt handler */
static void mbat_in_func(struct work_struct *work)
{
	BATT_LOG("shut down device due to MBAT_IN interrupt");

	/*if (gpio_get_value(htc_batt_info.gpio_mbat_in) != (htc_batt_info.mbat_in_trigger_level - 1)) {
		BATT_LOG("re-enable MBAT_IN irq!! due to false alarm\n");
		enable_irq(htc_batt_info.irq_mbat_in);
		return;
	}*/

	/* in the case that plug out battery during charging,
	   if stop charging here, after device power off,
	   device will not be triggered to power on by charger(VBUS) existed.
	   mbat_in_keep_charging=0: disable charge before power off
	   mbat_in_keep_charging=1: do not disable charge since some devices with different HW design
				    will stop vbus power source totally after disabling charging
	*/

	if (htc_batt_info.mbat_in_keep_charging == 0) {
		if (htc_batt_info.charger == SWITCH_CHARGER_TPS65200) {
			htc_batt_info.rep.batt_id = 0;
			tps_set_charger_ctrl(0);
			power_supply_changed(&htc_power_supplies[BATTERY_SUPPLY]);
			BATT_LOG("Battery status is wrong, please check ur ID Pin.");
		}
		else if (htc_batt_info.charger == SWITCH_CHARGER)
			blocking_notifier_call_chain(&cable_status_notifier_list,
					0, NULL);
		else
			htc_battery_set_charging(0);
	}

	/* unregister rmt due to it will spend more 5 secs to handle rmt fs sync*/
	if (htc_batt_info.mbat_in_unreg_rmt)
		emergency_sync();

	max17050_batt_softPOR();
	kernel_power_off();
}

static irqreturn_t mbat_int_handler(int irq, void *data)
{
	disable_irq_nosync(htc_batt_info.irq_mbat_in);
	BATT_LOG("mbat_int_handler: device will be turned off 0.3s later.");
	schedule_delayed_work(&mbat_in_work, msecs_to_jiffies(300));
	return IRQ_HANDLED;
}
/*  MBAT_IN interrupt handler end   */

#if CONFIG_HAS_EARLYSUSPEND
static void htc_battery_early_suspend(struct early_suspend * h)
{
#if 1  //enable it
	int rc;

	if (htc_batt_phone_call) {
		if (htc_batt_info.guage_driver == GUAGE_MODEM) {
			rc = htc_charger_switch(PHONE_CALL_IN);
			if (rc)
				BATT_ERR("A2M PHONE CALL IN Failed!\n");
			else {
				BATT_LOG("A2M PHONE CALL IN Successful!");
				is_phone_call_set = 1;
			}
		}
	}
#endif
}
static void htc_battery_late_resume(struct early_suspend * h)
{
#if 1
	int rc;

	if (is_phone_call_set) {
		if (htc_batt_info.guage_driver == GUAGE_MODEM) {
			rc = htc_charger_switch(PHONE_CALL_STOP);
			if (rc)
				BATT_ERR("A2M PHONE CALL STOP Failed!\n");
			else {
				BATT_LOG("A2M PHONE CALL STOP Successful!\n");
				is_phone_call_set = 0;
			}
		}
	}
#endif
}
static struct early_suspend htc_battery_suspend = {
	.suspend = htc_battery_early_suspend,
	.resume = htc_battery_late_resume,
};
#endif

/* batt_mtoa server definitions */
#define BATT_MTOA_PROG				0x30100000
#define BATT_MTOA_VERS				0
//#define RPC_BATT_MTOA_NULL			0
//#define RPC_BATT_MTOA_SET_CHARGING_PROC		1
//#define RPC_BATT_MTOA_CABLE_STATUS_UPDATE_PROC 	2
//#define RPC_BATT_MTOA_LEVEL_UPDATE_PROC		3
#if 0
struct rpc_batt_mtoa_set_charging_args {
	int enable;
};

struct rpc_batt_mtoa_cable_status_update_args {
	int status;
};

struct rpc_dem_battery_update_args {
	uint32_t level;
};
#endif
#if 0
static int handle_battery_call(struct msm_rpc_server *server,
			       struct rpc_request_hdr *req, unsigned len)
{

	switch (req->procedure) {
	case RPC_BATT_MTOA_NULL:
		return 0;

	case RPC_BATT_MTOA_SET_CHARGING_PROC: {
		struct rpc_batt_mtoa_set_charging_args *args;
		args = (struct rpc_batt_mtoa_set_charging_args *)(req + 1);
		args->enable = be32_to_cpu(args->enable);
		if (htc_batt_info.option_flag & OPTION_FLAG_BT_DOCK_CHARGE_CTL) {
			if (g_audio_stat && g_owe_docked &&
				POWER_SUPPLY_ENABLE_FAST_CHARGE == args->enable) {
				args->enable = POWER_SUPPLY_ENABLE_SLOW_CHARGE;
				pr_info("[BATT]: %s() BT_DOCK_CHARGE_CTL: %d\n",
					__func__, args->enable);
			}
		}
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_M2A_RPC)
			BATT_LOG("M2A_RPC: set_charging: %d", args->enable);
		if (htc_batt_info.charger == SWITCH_CHARGER_TPS65200) {
			mutex_lock(&htc_batt_info.lock);
			if (charger_internal_power_enabled != 0) {
				pr_info("[BATT] Ignore set_charging(%d) since boost mode enabled\n", args->enable);
				mutex_unlock(&htc_batt_info.lock);
				return 0;
			}
			mutex_unlock(&htc_batt_info.lock);
			tps_set_charger_ctrl(args->enable);
			if (args->enable == CHECK_CHG && htc_batt_info.charger_re_enable)
				tps65200_kick_charger_ic(htc_batt_info.rep.charging_enabled);
		}
		else if (htc_batt_info.charger == SWITCH_CHARGER)
			blocking_notifier_call_chain(&cable_status_notifier_list,
				args->enable, NULL);
		else {
			htc_battery_set_charging(args->enable);
		}
		return 0;
	}
	case RPC_BATT_MTOA_CABLE_STATUS_UPDATE_PROC: {
		struct rpc_batt_mtoa_cable_status_update_args *args;
		args = (struct rpc_batt_mtoa_cable_status_update_args *)(req + 1);
		args->status = be32_to_cpu(args->status);
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_M2A_RPC)
			BATT_LOG("M2A_RPC: cable_update: %s", charger_tags[args->status]);
#if 0
		/* FIXME: work arround for usb function, remove it after battery driver ready */
		if (machine_is_incrediblec() && args->status == CHARGER_AC)
			args->status = CHARGER_USB;
#endif
#if (defined(CONFIG_TPS65200) && (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_MAGNIDS)))
		tps65200_mask_interrupt_register(args->status);
#endif
		htc_cable_status_update(args->status);
#if defined(CONFIG_TROUT_BATTCHG_DOCK)
		dock_detect_start(args->status);
#endif
		return 0;
	}
	case RPC_BATT_MTOA_LEVEL_UPDATE_PROC: {
		struct rpc_dem_battery_update_args *args;
		args = (struct rpc_dem_battery_update_args *)(req + 1);
		args->level = be32_to_cpu(args->level);
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_M2A_RPC)
			BATT_LOG("M2A_RPC: level_update: %d", args->level);
		htc_battery_status_update(args->level);
		return 0;
	}
	default:
		BATT_ERR("%s: program 0x%08x:%d: unknown procedure %d",
		       __func__, req->prog, req->vers, req->procedure);
		return -ENODEV;
	}

}

static struct msm_rpc_server battery_server = {
	.prog = BATT_MTOA_PROG,
	.vers = BATT_MTOA_VERS,
	.rpc_call = handle_battery_call,
};
#endif

static int max17050_notifier_func(struct notifier_block *nfb, unsigned long action, void *param)
{
	u8 arg = 0;

	if (param)
		arg = *(u8 *)param;

	BATT_LOG(" max17050_notify: action = %ld, arg = %d", action, arg);
	switch (action) {
	case MAX17050_CHARGING_CONTROL:
		if (htc_batt_info.charger == LINEAR_CHARGER)
			htc_batt_info.func_battery_charging_ctrl(arg);
		else if (htc_batt_info.charger == SWITCH_CHARGER_TPS65200)
			tps_set_charger_ctrl(arg);
		break;
	case MAX17050_LEVEL_UPDATE:
		htc_battery_status_update(arg);
		break;
	default:
		return NOTIFY_BAD;
	}

	return NOTIFY_OK; /* we did not care other action here */
}

extern void dump_cp_status(void);
static struct notifier_block max17050_notifier = {
	.notifier_call = max17050_notifier_func,
};

/* this function is called by max17050_battery.c to update batt_info */
int htc_battery_update_change(int force_update)
{
	struct battery_info_reply new_batt_info_rep;
	int is_send_batt_uevent = 0, is_send_acusb_uevent = 0;
	/* MATT: this is not good to update batt_info like this */

		max17050_get_battery_info(&new_batt_info_rep);

#if 1
	/* update batt_info */
	mutex_lock(&htc_batt_info.lock);
	if (htc_batt_info.rep.charging_enabled != new_batt_info_rep.charging_enabled) {
		htc_batt_info.rep.charging_enabled = new_batt_info_rep.charging_enabled;
		if(new_batt_info_rep.charging_enabled > 0)
			is_send_acusb_uevent = 1;
		else
			is_send_batt_uevent = 1;
	}

	if (htc_batt_info.rep.level != new_batt_info_rep.level) {
		is_send_batt_uevent = 1;
		if (htc_batt_info.rep.charging_enabled > 0) {
			/* Charging: smoothing percentage when battery full */
			if (new_batt_info_rep.level == 100 && new_batt_info_rep.level > htc_batt_info.rep.level) {
				/* Smoothing level: increase 1% every time */
				pr_info("[BATT] Enter full smoothing algorithm, ori:%d\n",htc_batt_info.rep.level);
				htc_batt_info.rep.level = htc_batt_info.rep.level + 1;
			} else {
				htc_batt_info.rep.level = new_batt_info_rep.level;
			}
		} else {
			/* Discharge: percentage should not increase */
			if (new_batt_info_rep.level < htc_batt_info.rep.level || force_update)
				htc_batt_info.rep.level = new_batt_info_rep.level;
		}
	}

	if ((htc_batt_info.rep.level != new_batt_info_rep.level) ||
		(htc_batt_info.rep.batt_temp != new_batt_info_rep.batt_temp) ||
		(htc_batt_info.rep.batt_state != new_batt_info_rep.batt_state)) {
		is_send_batt_uevent = 1;
	}

	/* Notify reach bootup voltage */
	if (htc_batt_info.rep.batt_vol < enable_bootup_voltage &&
		new_batt_info_rep.batt_vol >= enable_bootup_voltage) {
		BATT_LOG("Send uevent notify enable boot up. vol:%d",new_batt_info_rep.batt_vol);
		is_send_batt_uevent = 1;
	}

	if (force_update) {
		is_send_acusb_uevent = 1;
		is_send_batt_uevent = 1;
	}
	htc_batt_info.rep.batt_id = new_batt_info_rep.batt_id;
	htc_batt_info.rep.batt_vol = new_batt_info_rep.batt_vol;
	htc_batt_info.rep.batt_current = new_batt_info_rep.batt_current;
	htc_batt_info.rep.full_bat = new_batt_info_rep.full_bat;
	htc_batt_info.rep.level = new_batt_info_rep.level;
	htc_batt_info.rep.batt_temp = new_batt_info_rep.batt_temp;
	htc_batt_info.rep.temp_fault = new_batt_info_rep.temp_fault;
	htc_batt_info.rep.batt_state = new_batt_info_rep.batt_state;
	//htc_batt_info.rep.thermal_temp= 250;// mark it,new_batt_info_rep.thermal_temp;

	mutex_unlock(&htc_batt_info.lock);
	/* send uevent if need */
	if (is_send_batt_uevent) {
		power_supply_changed(&htc_power_supplies[BATTERY_SUPPLY]);
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_UEVT)
		pr_info("[BATT] %s() power_supply_changed: battery ", __func__);
	}

	if (is_send_acusb_uevent) {
		power_supply_changed(&htc_power_supplies[AC_SUPPLY]);
		power_supply_changed(&htc_power_supplies[USB_SUPPLY]);
		if (htc_batt_debug_mask & HTC_BATT_DEBUG_UEVT)
		pr_info("[BATT] %s() power_supply_changed: ac/usb ", __func__);
	}

	int vbus_v = htc_adc_to_vol(ADC_CHANNEL_VCHGSEN, sci_adc_get_value(ADC_CHANNEL_VCHGSEN, 1));
	BATT_LOG("ID=%d, level=%d, vol=%d, temp=%d, "
		"batt_current=%d, chg_src=%d, "
		"chg_en=%d, full_bat=%d, over_vchg=%d, otg_count=%d, vBus_vol=%d.",
		htc_batt_info.rep.batt_id,
		htc_batt_info.rep.level,
		htc_batt_info.rep.batt_vol,
		htc_batt_info.rep.batt_temp,
		htc_batt_info.rep.batt_current,
		htc_batt_info.rep.charging_source,
		htc_batt_info.rep.charging_enabled,
		htc_batt_info.rep.full_bat,
		htc_batt_info.rep.over_vchg,
		htc_batt_info.rep.otg_count,
		vbus_v);
	dump_cp_status();
#endif
	return 0;
}
/*enum sprd_adapter_type {
	ADP_TYPE_UNKNOW = 0, //unknow adapter type
	ADP_TYPE_CDP	= 1, //Charging Downstream Port,USB&standard charger
	ADP_TYPE_DCP	= 2, //Dedicated Charging Port, standard charger
	ADP_TYPE_SDP	= 4, //Standard Downstream Port,USB and nonstandard charge
};*/

static int sprd_charger_is_adapter(void)
{
	uint32_t ret;
	unsigned long irq_flag = 0;
	uint32_t reg = 0;

	mdelay(300);

	gpio_request(USB_DM_GPIO, "sprd_charge");
	gpio_request(USB_DP_GPIO, "sprd_charge");
	gpio_direction_input(USB_DM_GPIO);
	gpio_direction_input(USB_DP_GPIO);

	udc_enable();
	udc_phy_down();
	local_irq_save(irq_flag);


	reg = sci_glb_read(REG_AP_APB_USB_PHY_CTRL , 0xFFFFFFFF);
	sci_glb_clr(REG_AP_APB_USB_PHY_CTRL,
			(BIT_DMPULLDOWN | BIT_DPPULLDOWN));

	/* Identify USB charger */
	sci_glb_set(REG_AP_APB_USB_PHY_CTRL, BIT_DMPULLUP);
	mdelay(20);
	ret = gpio_get_value(USB_DM_GPIO);
	sci_glb_clr(REG_AP_APB_USB_PHY_CTRL, (BIT_DMPULLUP));

	/* normal charger */
	if (ret) {
		/* Identify standard adapter */
		sci_glb_set(REG_AP_APB_USB_PHY_CTRL, BIT_DMPULLUP | BIT_DPPULLDOWN);
		mdelay(20);
		if (gpio_get_value(USB_DM_GPIO)
				== gpio_get_value(USB_DP_GPIO)) {
			ret = 1;	/* adapter */
			BATT_LOG("standard adapter inserted");
		} else {
			BATT_LOG("Non standard adapter inserted");
			ret = 0;	/* non standard adapter */
		}
		sci_glb_clr(REG_AP_APB_USB_PHY_CTRL, (BIT_DMPULLDOWN));
	}

	sci_glb_write(REG_AP_APB_USB_PHY_CTRL, reg, 0xFFFFFFFF);

	BATT_LOG("[%s], charger_status = %d(%s).", __FUNCTION__, ret, (ret == 1)?"AC":"USB" );
#if 0
	/*add for force_charger_type, 1 for AC, 2 for USB*/
	if (force_charger_type == 1){
		BATT_LOG("[%s], force charger type to AC\n", __FUNCTION__);
		return CHARGER_AC;
	}else if (force_charger_type == 2){
		BATT_LOG("[%s], force charger type to USB\n", __FUNCTION__);
		return CHARGER_USB;
	}
#endif
#if 0
	int ret = CHARGER_UNKNOWN;
	int charger_status;
	charger_status = (sci_adi_read(ANA_REG_GLB_CHGR_STATUS) & (BIT_CDP_INT|BIT_DCP_INT|BIT_SDP_INT));
	BATT_LOG("[%s], charger_status = %d.\n", __FUNCTION__, charger_status);
	switch (charger_status) {
	case BIT_CDP_INT: //bit5
		ret = CHARGER_USB;//ADP_TYPE_CDP; //for USB2.0
		break;
	case BIT_DCP_INT: //bit6
		ret = CHARGER_AC;//ADP_TYPE_DCP;  //AC
		break;
	case BIT_SDP_INT: //bit7
		ret = CHARGER_USB;//ADP_TYPE_SDP; //USB 500mA
		break;
	default:
		break;
	}

#endif
	local_irq_restore(irq_flag);
	udc_disable();
	gpio_free(USB_DM_GPIO);
	gpio_free(USB_DP_GPIO);
	return ret+1;
}
static int plugin_callback(int usb_cable, void *data)
{
	int charge_status = 0;
	wake_lock_timeout(&vbus_wake_lock, HZ * 3);
	charge_status = sprd_charger_is_adapter();

	BATT_LOG("charger plug in interrupt, charger is [%s].", (charge_status == CHARGER_AC)? "charger AC":"charger USB");

	peripheral_cable_update(charge_status, 0);

	return 0;
}

static int plugout_callback(int usb_cable, void *data)
{
	int charge_status = 0;
	BATT_LOG("charger plug out interrupt happen.");

	peripheral_cable_update(charge_status, 0);

	return 0;
}

static int htc_battery_core(struct platform_device *pdev)
{

	int i, rc;

	/* init battery gpio */
	if (htc_batt_info.charger == LINEAR_CHARGER) {
		if ((rc = init_batt_gpio()) < 0) {
			BATT_ERR("%s: init battery gpio failed!", __func__);
			return rc;
		}
	}
	/* init structure data member */
	htc_batt_info.update_time 	= jiffies;
	/* A9 will shutdown the phone if battery is pluged out, so this value is always 1.
	htc_batt_info.present 		= gpio_get_value(GPIO_TROUT_MBAT_IN);
	*/
	htc_batt_info.present 		= 1;

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(htc_power_supplies); i++) {
		printk("[BATT], &pdev->dev.name= %p, power_name = %s.\n", &pdev->dev, htc_power_supplies[i].name);
		rc = power_supply_register(&pdev->dev, &htc_power_supplies[i]);
		if (rc)
			BATT_ERR("%s: Failed to register power supply (%d)", __func__, rc);
	}

	/* create htc detail attributes */
	htc_battery_create_attrs(htc_power_supplies[BATTERY_SUPPLY].dev);

	/* After battery driver gets initialized, send rpc request to inquiry
	 * the battery status in case of we lost some info
	 */

	htc_batt_info.rep.charging_source = CHARGER_BATTERY;
	if (htc_get_batt_info(&htc_batt_info.rep) < 0)
		BATT_ERR("%s: get info failed", __func__);

	if (htc_batt_info.charger == SWITCH_CHARGER_TPS65200)
		tps_register_notifier(&tps_int_notifier);
#if CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&htc_battery_suspend);
#endif
	if (htc_batt_info.guage_driver == GAUGE_MAX17050)
			htc_batt_info.rep.batt_state = 0;
	else
			htc_batt_info.rep.batt_state = 1;

	htc_battery_initial = 1;
	return 0;
}

static int htc_battery_probe(struct platform_device *pdev)
{
	int ret;
	struct htc_battery_platform_data *pdata = pdev->dev.platform_data;

	BATT_LOG("battery version:%s",BATTERY_TIME_VERSION);
	htc_batt_info.device_id = pdev->id;
	htc_batt_info.gpio_usb_id = pdata->gpio_usb_id;
	htc_batt_info.guage_driver = pdata->guage_driver;
	htc_batt_info.m2a_cable_detect = pdata->m2a_cable_detect;
	htc_batt_info.func_show_batt_attr = pdata->func_show_batt_attr;
	htc_batt_info.func_show_htc_extension_attr = pdata->func_show_htc_extension_attr;
	htc_batt_info.charger = pdata->charger;
	htc_batt_info.option_flag = pdata->option_flag;
	htc_batt_info.rep.full_level = 100;
	htc_batt_info.charger_re_enable = pdata->charger_re_enable;
	chg_limit_active_mask = pdata->chg_limit_active_mask;
	suspend_highfreq_check_reason = pdata->suspend_highfreq_check_reason;
	htc_batt_info.func_htc_batt_context_event_handler = htc_batt_context_event_handler;
	if (pdata->enable_bootup_voltage > 0)
		enable_bootup_voltage = pdata->enable_bootup_voltage;
	test_power_monitor = /*(get_kernel_flag() & KERNEL_FLAG_TEST_PWR_SUPPLY) ? 1 : */0;

	if (htc_batt_info.charger == LINEAR_CHARGER) {
		htc_batt_info.gpio_mbat_in = pdata->gpio_mbat_in;
		htc_batt_info.gpio_mchg_en_n = pdata->gpio_mchg_en_n;
		htc_batt_info.gpio_iset = pdata->gpio_iset;
	}
	if (pdata->gpio_mbat_in_trigger_level != MBAT_IN_NO_IRQ && pdata->gpio_mbat_in > 0) {
		htc_batt_info.has_mbat_in_irq = 1;
		htc_batt_info.gpio_mbat_in = pdata->gpio_mbat_in;
		ret = gpio_request(pdata->gpio_mbat_in, NULL);
		htc_batt_info.irq_mbat_in = gpio_to_irq(pdata->gpio_mbat_in);
		htc_batt_info.mbat_in_trigger_level = pdata->gpio_mbat_in_trigger_level;
		htc_batt_info.mbat_in_keep_charging = pdata->mbat_in_keep_charging;
		htc_batt_info.mbat_in_unreg_rmt = pdata->mbat_in_unreg_rmt;
		INIT_DELAYED_WORK(&mbat_in_work, mbat_in_func);
		if (pdata->gpio_mbat_in_trigger_level == MBAT_IN_HIGH_TRIGGER)
			ret = request_irq(htc_batt_info.irq_mbat_in,
			mbat_int_handler, IRQF_TRIGGER_HIGH,
				"mbat_in", NULL);
		else if (pdata->gpio_mbat_in_trigger_level == MBAT_IN_LOW_TRIGGER)
			ret = request_irq(htc_batt_info.irq_mbat_in,
			mbat_int_handler, IRQF_TRIGGER_LOW,
			"mbat_in", NULL);
		if (ret)
			BATT_ERR("request mbat_in irq failed!");
		else
			irq_set_irq_wake(htc_batt_info.irq_mbat_in, 1);
	} else
		htc_batt_info.has_mbat_in_irq = 0;

	printk("[BATT] htc_battery probe.\n");
	//// GPIO part
	htc_batt_info.gpio_vbus_det = pdata->gpio_vbus_det;

	ret = usb_register_hotplug_callback(&power_cb);

	if (ret)
		printk("[BATT][error]failed to request irq.\n");
	if (pdata->func_is_support_super_charger != NULL) {
		if (pdata->func_is_support_super_charger() == 1)
			htc_batt_info.gpio_adp_9v = pdata->gpio_adp_9v;
	}

	if (pdata->func_battery_charging_ctrl != NULL && pdata->func_battery_gpio_init != NULL) {
		htc_batt_info.func_battery_charging_ctrl = pdata->func_battery_charging_ctrl;
		pdata->func_battery_gpio_init();
	} else
		htc_batt_info.func_battery_charging_ctrl = battery_charging_ctrl;

	if (pdata->guage_driver == GAUGE_MAX17050)
		max17050_register_notifier(&max17050_notifier);

	charger_ctrl_stat = ENABLE_CHARGER;
	INIT_WORK(&batt_charger_ctrl_work, batt_charger_ctrl_func);
	alarm_init(&batt_charger_ctrl_alarm,
		    ALARM_BOOTTIME/*ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP*/,
		    batt_charger_ctrl_alarm_handler);
	batt_charger_ctrl_wq =
			create_singlethread_workqueue("charger_ctrl_timer");
	htc_battery_core(pdev);

	return 0;
}

int get_cable_status(void)
{
/*	if(htc_batt_info.rep.charging_source == CHARGER_AC || htc_batt_info.rep.charging_source == CHARGER_USB)
		htc_cable_status_update(htc_batt_info.rep.charging_source);
*/	return htc_batt_info.rep.charging_source; //CHARGER_USB;//
}

static struct platform_driver htc_battery_driver = {
	.probe	= htc_battery_probe,
	.driver	= {
		.name	= "htc_battery",
		.owner	= THIS_MODULE,
	},
};

static BLOCKING_NOTIFIER_HEAD(battery_notifier_list);
int batt_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&battery_notifier_list, nb);
}

int batt_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&battery_notifier_list, nb);
}

int batt_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&battery_notifier_list, val, v);
}

int get_batt_id(void)
{
#if 0
    if (smem_batt_info) {
		if (smem_batt_info->batt_id == 255) {
			return BATTERY_ID_UNKNOWN;
		}
		return smem_batt_info->batt_id;
	} else {
		return BATTERY_ID_UNKNOWN;
	}
#endif
    return 1;
}
#if 0
void set_smem_chg_avalible(int chg_avalible)
{
	smem_batt_info->charging_enabled = chg_avalible;
}
#endif
int get_cable_type(void)
{
#if 0
	if (smem_batt_info)
		return smem_batt_info->a2m_cable_type;
	else
		return -EINVAL;
#endif
    return 0;
}

static int __init htc_battery_init(void)
{
	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
	mutex_init(&htc_batt_info.lock);
	mutex_init(&context_event_handler_lock);
	usb_register_notifier(&usb_status_notifier);

	platform_driver_register(&htc_battery_driver);
	return 0;
}

module_init(htc_battery_init);
MODULE_DESCRIPTION("HTC Battery Driver");
MODULE_LICENSE("GPL");

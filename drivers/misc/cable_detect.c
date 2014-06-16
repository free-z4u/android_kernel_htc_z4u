/* drivers/misc/cable_detect.c - cable detect driver
 *
 * Copyright (C) 2009 HTC Corporation.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <mach/board.h>
#include <linux/wakelock.h>

#include <mach/cable_detect.h>
#include <linux/switch.h>
#include <mach/adc.h>

#ifdef CONFIG_HTC_HEADSET_MGR
#include <mach/htc_headset_mgr.h>
#ifdef CONFIG_HTC_HEADSET_MISC
#include <mach/htc_headset_misc.h>
#endif
#endif

static int vbus;
static LIST_HEAD(g_lh_calbe_detect_notifier_list);
static LIST_HEAD(g_lh_usb_host_detect_notifier_list);

static struct switch_dev dock_switch = {
	.name = "dock",
};

struct cable_detect_info {
	spinlock_t lock;

	enum usb_connect_type connect_type;
	/*for accessory*/
	int usb_id_pin_gpio;
	__u8 detect_type;
	int accessory_type;
	int idpin_irq;
	u8 mfg_usb_carkit_enable;
	int mhl_reset_gpio;
	bool mhl_version_ctrl_flag;
	struct workqueue_struct *cable_detect_wq;
	struct delayed_work cable_detect_work;
	struct wake_lock cable_detect_wlock;
	void (*usb_uart_switch)(int);
	void (*usb_dpdn_switch)(int);
	void (*config_usb_id_gpios)(bool enable);
	void (*mhl_1v2_power)(bool enable);
	int (*is_wireless_charger)(void);
	u8 cable_redetect;
	u8 detect_running;

	int ac_9v_gpio;
	void (*configure_ac_9v_gpio) (int);
	u8 mhl_internal_3v3;

#ifdef CONFIG_CABLE_DETECT_GPIO_DOCK
	bool dock_detect;
	int dockpin_irq;
	int dock_pin_gpio;
	uint8_t dock_pin_state;
	struct delayed_work dock_work_isr;
	struct delayed_work dock_work;
#endif

	int64_t (*get_adc_cb)(void);
	struct ab8500_gpadc *gpadc;
	void (*enable_host_mode)(bool enable);
} the_cable_info;

static int cable_detect_get_adc(void);
static int mhl_detect(struct cable_detect_info *pInfo);
static void usb_id_detect_init(struct cable_detect_info *info);

#ifdef CONFIG_CABLE_DETECT_GPIO_DOCK
static void dock_isr_work(struct work_struct *w);
static void dock_detect_work(struct work_struct *w);
static void dock_detect_init(struct cable_detect_info *pInfo);
#endif
#if 0
static DEFINE_MUTEX(cable_notify_sem);
static void send_cable_connect_notify(int cable_type)
{
	static struct t_cable_status_notifier *notifier;
	struct cable_detect_info *pInfo = &the_cable_info;

	mutex_lock(&cable_notify_sem);
	CABLE_DEBUG("%s: cable_type = %d\n", __func__, cable_type);

	if (cable_type == CONNECT_TYPE_UNKNOWN)
		cable_type = CONNECT_TYPE_USB;

	if (pInfo->ac_9v_gpio && (cable_type == CONNECT_TYPE_USB
				|| cable_type == CONNECT_TYPE_AC)) {
		if (pInfo->configure_ac_9v_gpio)
			pInfo->configure_ac_9v_gpio(1);

		mdelay(5);
		if (gpio_get_value(pInfo->ac_9v_gpio)) {
			CABLE_INFO("%s detect 9v charger\n", __func__);
			cable_type = CONNECT_TYPE_9V_AC;
		}

		if (pInfo->configure_ac_9v_gpio)
			pInfo->configure_ac_9v_gpio(0);
	}

	list_for_each_entry(notifier,
		&g_lh_calbe_detect_notifier_list,
		cable_notifier_link) {
			if (notifier->func != NULL) {
				CABLE_INFO("Send to: %s, type %d\n",
						notifier->name, cable_type);
				/* Notify other drivers about connect type. */
				/* use slow charging for unknown type*/
				notifier->func(cable_type);
			}
		}
	mutex_unlock(&cable_notify_sem);
}

int cable_detect_register_notifier(struct t_cable_status_notifier *notifier)
{
	if (!notifier || !notifier->name || !notifier->func)
		return -EINVAL;

	mutex_lock(&cable_notify_sem);
	list_add(&notifier->cable_notifier_link,
		&g_lh_calbe_detect_notifier_list);
	mutex_unlock(&cable_notify_sem);
	return 0;
}
#endif

#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
static DEFINE_MUTEX(usb_host_notify_sem);
static void send_usb_host_connect_notify(int cable_in)
{
	struct t_usb_host_status_notifier *notifier;

	mutex_lock(&usb_host_notify_sem);
	list_for_each_entry(notifier,
		&g_lh_usb_host_detect_notifier_list,
		usb_host_notifier_link) {
		if (notifier->func != NULL) {
			CABLE_INFO("[HostNotify] Send to: %s: %d\n",
					notifier->name, cable_in);
			/* Notify other drivers about connect type. */
			/* use slow charging for unknown type*/
			notifier->func(cable_in);
		}
	}
	mutex_unlock(&usb_host_notify_sem);
}

int usb_host_detect_register_notifier(struct t_usb_host_status_notifier *notifier)
{
	if (!notifier || !notifier->name || !notifier->func)
		return -EINVAL;

	mutex_lock(&usb_host_notify_sem);
	list_add(&notifier->usb_host_notifier_link,
			&g_lh_usb_host_detect_notifier_list);
	mutex_unlock(&usb_host_notify_sem);
	return 0;
}
#endif

static int cable_detect_get_type(struct cable_detect_info *pInfo)
{
	int id_pin, adc, type;
	static int prev_type, stable_count;

	if (stable_count >= ADC_RETRY)
		stable_count = 0;

	id_pin = gpio_get_value(pInfo->usb_id_pin_gpio);
	if (id_pin == 0 || pInfo->cable_redetect) {
		CABLE_INFO("%s: id pin low\n", __func__);

		adc = cable_detect_get_adc();
		CABLE_INFO("%s: ADC = %d\n", __func__, adc);

		if (adc > -100 && adc < 100)
			type = mhl_detect(pInfo);
		else {
#ifdef CONFIG_MACH_CSNDUG
			if (adc > 135 && adc < 220)
#else
			if (adc > 150 && adc < 235)
#endif
				type = DOCK_STATE_CAR;
			else if (adc > 370 && adc < 440)
				type = DOCK_STATE_USB_HEADSET;
			else if (adc > 550 && adc < 900)
				type = DOCK_STATE_DESK;
			else
				type = DOCK_STATE_UNDEFINED;
		}
	} else {
		CABLE_INFO("%s: id pin high\n", __func__);
		type = DOCK_STATE_UNDOCKED;
	}

	if (prev_type == type)
		stable_count++;
	else
		stable_count = 0;

	CABLE_INFO("%s prev_type %d, type %d, stable_count %d\n",
				__func__, prev_type, type, stable_count);

	prev_type = type;
	return (stable_count >= ADC_RETRY) ? type : -2;
}

static void cable_detect_handler_recheck(struct cable_detect_info *pInfo)
{
	if (pInfo->accessory_type == DOCK_STATE_UNDOCKED)
		return;
	else
	{
		CABLE_INFO("remove the pre cable because the pre cable remove event is missed\n");
		switch(pInfo->accessory_type)
		{
			case DOCK_STATE_DESK:
				CABLE_INFO("cradle removed\n");
				switch_set_state(&dock_switch, DOCK_STATE_UNDOCKED);
				pInfo->accessory_type = DOCK_STATE_UNDOCKED;
				break;
			case DOCK_STATE_CAR:
				CABLE_INFO("Car kit removed\n");
				switch_set_state(&dock_switch, DOCK_STATE_UNDOCKED);
				pInfo->accessory_type = DOCK_STATE_UNDOCKED;
				break;
			case DOCK_STATE_USB_HEADSET:
				CABLE_INFO("USB headset removed\n");
#ifdef CONFIG_HTC_HEADSET_MGR
				headset_ext_detect(USB_NO_HEADSET);
#endif
				if (pInfo->usb_dpdn_switch)
					pInfo->usb_dpdn_switch(PATH_USB);
				pInfo->accessory_type = DOCK_STATE_UNDOCKED;
				break;
#ifdef CONFIG_STE_HDMI_MHL
			case DOCK_STATE_MHL:
				CABLE_INFO("MHL removed\n");
				switch_set_state(&dock_switch, DOCK_STATE_UNDOCKED);
				break;
#endif
#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
			case DOCK_STATE_USB_HOST:
				CABLE_INFO("USB host cable removed\n");
				pInfo->accessory_type = DOCK_STATE_UNDOCKED;
				send_usb_host_connect_notify(0);
				if (pInfo->enable_host_mode)
					pInfo->enable_host_mode(false);
				break;
#endif
			case DOCK_STATE_UNDEFINED:
				CABLE_INFO("Unknown cable removed\n");
				pInfo->accessory_type = DOCK_STATE_UNDOCKED;
				break;
			default:
				break;
		}
	}
	return;
}


static void cable_detect_handler(struct work_struct *w)
{
	struct cable_detect_info *pInfo = container_of(
			w, struct cable_detect_info, cable_detect_work.work);
	int value;
	int accessory_type;

	if (pInfo == NULL)
		return;

	if (pInfo->mhl_reset_gpio != 0)
		gpio_set_value(pInfo->mhl_reset_gpio, 0); /* Reset Low */

	if (pInfo->detect_type >= CABLE_TYPE_PMIC_ADC) {
		accessory_type = cable_detect_get_type(pInfo);
		if (accessory_type == -2) {
			queue_delayed_work(pInfo->cable_detect_wq,
				&pInfo->cable_detect_work, ADC_DELAY);
			return;
		}
	} else
		accessory_type = DOCK_STATE_UNDOCKED;

	if (pInfo->mhl_reset_gpio != 0)
		gpio_set_value(pInfo->mhl_reset_gpio, 1);	/* Reset High */

	if(accessory_type != DOCK_STATE_UNDOCKED ){
		cable_detect_handler_recheck(pInfo);
	}

	switch (accessory_type) {
	case DOCK_STATE_DESK:
		CABLE_INFO("cradle inserted\n");
		switch_set_state(&dock_switch, DOCK_STATE_DESK);
		pInfo->accessory_type = DOCK_STATE_DESK;
		break;
	case DOCK_STATE_CAR:
		CABLE_INFO("Car kit inserted\n");
		switch_set_state(&dock_switch, DOCK_STATE_CAR);
		pInfo->accessory_type = DOCK_STATE_CAR;
		break;
	case DOCK_STATE_USB_HEADSET:
		CABLE_INFO("USB headset inserted\n");
		pInfo->accessory_type = DOCK_STATE_USB_HEADSET;
		if (pInfo->usb_dpdn_switch)
			pInfo->usb_dpdn_switch(PATH_USB_AUD);
#ifdef CONFIG_HTC_HEADSET_MGR
		headset_ext_detect(USB_AUDIO_OUT);
#endif
		break;
#ifdef CONFIG_STE_HDMI_MHL
	case DOCK_STATE_MHL:
		CABLE_INFO("MHL inserted\n");
		switch_set_state(&dock_switch, DOCK_STATE_MHL);
		pInfo->accessory_type = DOCK_STATE_MHL;

		if (pInfo->usb_dpdn_switch)
			pInfo->usb_dpdn_switch(PATH_MHL);
#if 0
		if (!pInfo->mhl_internal_3v3 && !vbus)
			send_cable_connect_notify(CONNECT_TYPE_INTERNAL);
#endif
		break;
#endif
#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
	case DOCK_STATE_USB_HOST:
		CABLE_INFO("USB Host inserted\n");
		send_usb_host_connect_notify(1);
		pInfo->accessory_type = DOCK_STATE_USB_HOST;
		if (pInfo->enable_host_mode)
			pInfo->enable_host_mode(true);
		break;
#endif
	case DOCK_STATE_UNDEFINED:
		CABLE_INFO("Unknown cable detected\n");
		pInfo->accessory_type = DOCK_STATE_UNDEFINED;
		break;
	case DOCK_STATE_UNDOCKED:
		switch (pInfo->accessory_type) {
		case DOCK_STATE_DESK:
			CABLE_INFO("cradle removed\n");
			switch_set_state(&dock_switch, DOCK_STATE_UNDOCKED);
			pInfo->accessory_type = DOCK_STATE_UNDOCKED;
			break;
		case DOCK_STATE_CAR:
			CABLE_INFO("Car kit removed\n");
			switch_set_state(&dock_switch, DOCK_STATE_UNDOCKED);
			pInfo->accessory_type = DOCK_STATE_UNDOCKED;
			break;
		case DOCK_STATE_USB_HEADSET:
			CABLE_INFO("USB headset removed\n");
#ifdef CONFIG_HTC_HEADSET_MGR
			headset_ext_detect(USB_NO_HEADSET);
#endif
			if (pInfo->usb_dpdn_switch)
				pInfo->usb_dpdn_switch(PATH_USB);
			pInfo->accessory_type = DOCK_STATE_UNDOCKED;
			break;
#ifdef CONFIG_STE_HDMI_MHL
		case DOCK_STATE_MHL:
			CABLE_INFO("MHL removed\n");
			switch_set_state(&dock_switch, DOCK_STATE_UNDOCKED);
			break;
#endif
#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
		case DOCK_STATE_USB_HOST:
			CABLE_INFO("USB host cable removed\n");
			pInfo->accessory_type = DOCK_STATE_UNDOCKED;
			send_usb_host_connect_notify(0);
			if (pInfo->enable_host_mode)
				pInfo->enable_host_mode(false);
			break;
#endif
		case DOCK_STATE_UNDEFINED:
			CABLE_INFO("Unknown cable removed\n");
			pInfo->accessory_type = DOCK_STATE_UNDOCKED;
			break;
		}
	default :
		break;
	}

	value = gpio_get_value(pInfo->usb_id_pin_gpio);
	CABLE_INFO("%s ID pin %d, type %d\n", __func__,
				value, pInfo->accessory_type);
#ifdef CONFIG_STE_HDMI_MHL
	if (pInfo->accessory_type == DOCK_STATE_MHL) {
		pInfo->detect_running = 0;
		wake_unlock(&pInfo->cable_detect_wlock);
		return;
	}
#endif

	if (pInfo->accessory_type == DOCK_STATE_UNDOCKED)
		irq_set_irq_type(pInfo->idpin_irq,
			value ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
	else
		irq_set_irq_type(pInfo->idpin_irq, IRQF_TRIGGER_HIGH);

	enable_irq(pInfo->idpin_irq);
	pInfo->detect_running = 0;
	printk("==cable_detect_handler out\n");
	wake_unlock(&pInfo->cable_detect_wlock);
}

int cable_get_connect_type(void)
{
	struct cable_detect_info *pInfo = &the_cable_info;

	return pInfo->connect_type;
}

void set_mfg_usb_carkit_enable(int enable)
{
	the_cable_info.mfg_usb_carkit_enable = enable;
}

int cable_get_accessory_type(void)
{
	return the_cable_info.accessory_type;
}
/*
static int get_adc_up(void)
{
	uint8_t data[2];
	int ret = 0;
	int result = 0;

	memset(data, 0x0, 2);
	ret = microp_read_adc(data);
	if (ret)
		CABLE_ERR("%s: Get adc failed.\n", __func__);

	result = (data[0] << 8) + data[1];
	return MICROPADC_3P0_10BIT(result);
}

static int get_adc_ab8500(void)
{
	struct cable_detect_info *pInfo = &the_cable_info;

	return ab8500_gpadc_convert(pInfo->gpadc, ACC_DETECT1);
}
*/
static int cable_detect_get_adc(void)
{
	struct cable_detect_info *pInfo = &the_cable_info;
	int result = 0;
	uint16_t vol;
	if (pInfo->get_adc_cb) {
		result = pInfo->get_adc_cb();
		printk(KERN_INFO "[CABLE]cable_detect_get_adc %d\n",result);
		vol = htc_adc_to_vol(ADC_CHANNEL_2, result);
		CABLE_INFO("%s: ADC = %d vol = %d\n", __func__, result, vol);
		return vol;
	} else {
		printk(KERN_INFO "[CABLE]cable_detect_get_adc: no callback function definition\n");
		return -200;
	}
		
/*

	if (pInfo->detect_type == CABLE_TYPE_MICROP_ADC)
		result = get_adc_up();
	else if (pInfo->detect_type == CABLE_TYPE_AB8500)
		result = get_adc_ab8500();

	return result;
*/
}

int cable_get_usb_id_level(void)
{
	struct cable_detect_info *pInfo = &the_cable_info;

	if (pInfo->usb_id_pin_gpio)
		return gpio_get_value(pInfo->usb_id_pin_gpio);
	else {
		printk(KERN_INFO "usb id is not defined\n");
		return 1;
	}
}

static int mhl_detect(struct cable_detect_info *pInfo)
{
	uint32_t adc_value = 0xffffffff;
	int type;

	if (pInfo->config_usb_id_gpios)
		pInfo->config_usb_id_gpios(1);

	adc_value = cable_detect_get_adc();
	CABLE_INFO("[2nd] accessory adc = %d\n", adc_value);

	if ((pInfo->mhl_version_ctrl_flag) || (adc_value >= 776 && adc_value <= 1020))
#ifdef CONFIG_STE_HDMI_MHL
		type = DOCK_STATE_MHL;
#else
		type = DOCK_STATE_UNDEFINED;
#endif
	else
#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
		type = DOCK_STATE_USB_HOST;
#else
		type = DOCK_STATE_UNDEFINED;
#endif

	if (pInfo->config_usb_id_gpios)
		pInfo->config_usb_id_gpios(0);

	return type;
}

static int get_usb_id_adc(char *buffer, struct kernel_param *kp)
{
	unsigned length = 0;
	int adc;

	adc = cable_detect_get_adc();

	length += sprintf(buffer, "%d\n", adc);

	return length;
}
module_param_call(usb_id_adc, NULL, get_usb_id_adc, NULL, 0664);

static ssize_t dock_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cable_detect_info *pInfo = &the_cable_info;

	if (pInfo->accessory_type == 1)
		return sprintf(buf, "online\n");
	else if (pInfo->accessory_type == 3) /*desk dock*/
		return sprintf(buf, "online\n");
	else
		return sprintf(buf, "offline\n");
}
static DEVICE_ATTR(status, S_IRUGO | S_IWUSR, dock_status_show, NULL);

static irqreturn_t usbid_interrupt(int irq, void *data)
{
	struct cable_detect_info *pInfo = (struct cable_detect_info *)data;

	disable_irq_nosync(pInfo->idpin_irq);
	if (pInfo->detect_running) {
		CABLE_INFO("%s: detect is running.\n", __func__);
		return IRQ_HANDLED;
	} else if (pInfo->accessory_type == DOCK_STATE_MHL) {
		CABLE_INFO("%s: MHL is online\n", __func__);
		return IRQ_HANDLED;
	}

	CABLE_INFO("%s: id interrupt\n", __func__);
	pInfo->cable_redetect = 0;
	pInfo->detect_running = 1;
	queue_delayed_work(pInfo->cable_detect_wq,
			&pInfo->cable_detect_work, ADC_DELAY);

	wake_lock_timeout(&pInfo->cable_detect_wlock, HZ*2);
	return IRQ_HANDLED;
}

static void usb_id_detect_init(struct cable_detect_info *pInfo)
{
	int ret;
	CABLE_INFO("%s: id pin %d\n", __func__,
		pInfo->usb_id_pin_gpio);

	if (pInfo->usb_id_pin_gpio == 0)
		return;
	if (pInfo->idpin_irq == 0)
		pInfo->idpin_irq = gpio_to_irq(pInfo->usb_id_pin_gpio);

	set_irq_flags(pInfo->idpin_irq, IRQF_VALID | IRQF_NOAUTOEN);
	ret = request_any_context_irq(pInfo->idpin_irq, usbid_interrupt,
				IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND, "idpin_irq", pInfo);
	if (ret < 0) {
		CABLE_ERR("%s: request_irq failed\n", __func__);
		return;
	}

#if 0
	ret = set_irq_wake(pInfo->idpin_irq, 1);
	if (ret < 0) {
		CABLE_ERR("%s: set_irq_wake failed\n", __func__);
		goto err;
	}
#endif
	pInfo->detect_running = 1;
	queue_delayed_work(pInfo->cable_detect_wq,
			&pInfo->cable_detect_work, 0);

	enable_irq(pInfo->idpin_irq);
	return;
	free_irq(pInfo->idpin_irq, 0);
}

#ifdef CONFIG_STE_HDMI_MHL
static void mhl_status_notifier_func(bool isMHL, int charging_type)
{
	struct cable_detect_info *pInfo = &the_cable_info;
	int id_pin = gpio_get_value(pInfo->usb_id_pin_gpio);
	static uint8_t mhl_connected;

	CABLE_INFO("%s: isMHL %d, charging type %d, id_pin %d\n",
				__func__, isMHL, charging_type, id_pin);
	if (pInfo->accessory_type != DOCK_STATE_MHL) {
		CABLE_INFO("%s: accessory is not MHL, type %d\n",
					__func__, pInfo->accessory_type);
		return;
	}

#ifdef CONFIG_HTC_HEADSET_MISC
	headset_mhl_audio_jack_enable(isMHL);
#endif

	if (!isMHL) {
		CABLE_INFO("MHL removed\n");

		if (pInfo->usb_dpdn_switch)
			pInfo->usb_dpdn_switch(PATH_USB);

		if (pInfo->mhl_1v2_power)
			pInfo->mhl_1v2_power(0);
#if 0
		send_cable_connect_notify(CONNECT_TYPE_NONE);
#endif
#ifdef MHL_REDETECT
		if (mhl_connected == 0) {
			CABLE_INFO("MHL re-detect\n");
			pInfo->cable_redetect = 1;
		}
#endif
		mhl_connected = 0;

		pInfo->accessory_type = DOCK_STATE_UNDOCKED;

		id_pin = gpio_get_value(pInfo->usb_id_pin_gpio);
		if (id_pin == 0) {
			CABLE_INFO("%s: ID=%d, Re-detect\n", __func__, id_pin);
			wake_lock_timeout(&pInfo->cable_detect_wlock, HZ*2);
			pInfo->detect_running = 1;
			queue_delayed_work(pInfo->cable_detect_wq,
					&pInfo->cable_detect_work, 0);
		}
		return;
	} else {
		mhl_connected = 1;
#if 0
		if (charging_type > CONNECT_TYPE_NONE)
			send_cable_connect_notify(charging_type);
		else if (vbus)
			send_cable_connect_notify(CONNECT_TYPE_USB);
#endif
	}
}

static struct t_mhl_status_notifier mhl_status_notifier = {
	.name = "mhl_detect",
	.func = mhl_status_notifier_func,
};
#endif

#ifdef CONFIG_CABLE_DETECT_GPIO_DOCK
static irqreturn_t dock_interrupt(int irq, void *data)
{
	struct cable_detect_info *pInfo = data;
	disable_irq_nosync(pInfo->dockpin_irq);
	cancel_delayed_work(&pInfo->dock_work);
	queue_delayed_work(pInfo->cable_detect_wq,
			&pInfo->dock_work_isr, DOCK_DET_DELAY);
	return IRQ_HANDLED;
}
static void dock_isr_work(struct work_struct *w)
{
	struct cable_detect_info *pInfo = container_of(
			w, struct cable_detect_info, dock_work_isr.work);
	pInfo->dock_pin_state = gpio_get_value(pInfo->dock_pin_gpio);

	if (pInfo->dock_pin_state == 1)
		set_irq_type(pInfo->dockpin_irq, IRQF_TRIGGER_LOW);
	else
		set_irq_type(pInfo->dockpin_irq, IRQF_TRIGGER_HIGH);
	queue_delayed_work(pInfo->cable_detect_wq,
			&pInfo->dock_work, DOCK_DET_DELAY);
	enable_irq(pInfo->dockpin_irq);
}
static void dock_detect_work(struct work_struct *w)
{
	struct cable_detect_info *pInfo = container_of(
			w, struct cable_detect_info, dock_work.work);
	int value;

	value = gpio_get_value(pInfo->dock_pin_gpio);
	CABLE_INFO("%s: dock_pin = %s\n", __func__, value ? "high" : "low");
	if (pInfo->dock_pin_state != value && (pInfo->dock_pin_state & 0x80) == 0) {
		CABLE_ERR("%s: dock_pin_state changed\n", __func__);
		return;
	}

	if (value == 0 && vbus) {
		if (pInfo->accessory_type == DOCK_STATE_DESK)
			return;
		set_irq_type(pInfo->dockpin_irq, IRQF_TRIGGER_HIGH);
		switch_set_state(&dock_switch, DOCK_STATE_DESK);
		pInfo->accessory_type = DOCK_STATE_DESK;
		CABLE_INFO("dock: set state %d\n", DOCK_STATE_DESK);
	} else {
		if (pInfo->accessory_type == DOCK_STATE_UNDOCKED)
			return;
		set_irq_type(pInfo->dockpin_irq, IRQF_TRIGGER_LOW);
		switch_set_state(&dock_switch, DOCK_STATE_UNDOCKED);
		pInfo->accessory_type = DOCK_STATE_UNDOCKED;
		CABLE_INFO("dock: set state %d\n", DOCK_STATE_UNDOCKED);
	}
}
static void dock_detect_init(struct cable_detect_info *pInfo)
{
	int ret;

	CABLE_INFO("%s in, gpio %d, irq %d\n",
			__func__, pInfo->dock_pin_gpio, pInfo->dockpin_irq);
	if (pInfo->dock_pin_gpio == 0)
		return;
	if (pInfo->dockpin_irq == 0)
		pInfo->dockpin_irq = gpio_to_irq(pInfo->dock_pin_gpio);

	set_irq_flags(pInfo->dockpin_irq, IRQF_VALID | IRQF_NOAUTOEN);
	ret = request_any_context_irq(pInfo->dockpin_irq, dock_interrupt,
				IRQF_TRIGGER_LOW, "dock_irq", pInfo);
	if (ret < 0) {
		CABLE_ERR("[GPIO DOCK] %s: request_irq failed\n", __func__);
		return;
	}
	CABLE_INFO("%s: dock irq %d\n", __func__, pInfo->dockpin_irq);

	if (vbus)
		enable_irq(pInfo->dockpin_irq);
}
#endif

static ssize_t adc_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int adc;
	uint16_t vol;

	adc = cable_detect_get_adc();
	vol = htc_adc_to_vol(ADC_CHANNEL_2, adc);
	CABLE_INFO("%s: ADC = %d vol = %d\n", __func__, adc, vol);
	return sprintf(buf, "%d\n", adc);
}
static DEVICE_ATTR(adc, S_IRUGO | S_IWUSR, adc_status_show, NULL);

static int cable_detect_probe(struct platform_device *pdev)
{
	int ret;
	struct cable_detect_platform_data *pdata = pdev->dev.platform_data;
	struct cable_detect_info *pInfo = &the_cable_info;

	spin_lock_init(&the_cable_info.lock);

	if (pdata) {
		pInfo->usb_uart_switch = pdata->usb_uart_switch;
		pInfo->usb_dpdn_switch = pdata->usb_dpdn_switch;
		if (pInfo->usb_dpdn_switch)
			pInfo->usb_dpdn_switch(PATH_USB);
		pInfo->ac_9v_gpio = pdata->ac_9v_gpio;
		pInfo->configure_ac_9v_gpio = pdata->configure_ac_9v_gpio;
		pInfo->mhl_internal_3v3 = pdata->mhl_internal_3v3;

		pInfo->detect_type = pdata->detect_type;
		pInfo->usb_id_pin_gpio = pdata->usb_id_pin_gpio;
		pInfo->mhl_reset_gpio = pdata->mhl_reset_gpio;
		pInfo->config_usb_id_gpios = pdata->config_usb_id_gpios;
		pInfo->mhl_version_ctrl_flag = pdata->mhl_version_ctrl_flag;
		pInfo->mhl_1v2_power = pdata->mhl_1v2_power;
		pInfo->get_adc_cb = pdata->get_adc_cb;
		pInfo->enable_host_mode = pdata->enable_host_mode;

		if (pInfo->config_usb_id_gpios)
			pInfo->config_usb_id_gpios(0);
/*
		if (pInfo->detect_type == CABLE_TYPE_AB8500)
			pInfo->gpadc = ab8500_gpadc_get();
*/
#ifdef CONFIG_CABLE_DETECT_GPIO_DOCK
		pInfo->dock_detect = pdata->dock_detect;
		pInfo->dock_pin_gpio = pdata->dock_pin_gpio;
#endif

		if (pdata->is_wireless_charger)
			pInfo->is_wireless_charger = pdata->is_wireless_charger;
		INIT_DELAYED_WORK(&pInfo->cable_detect_work, cable_detect_handler);

		pInfo->cable_detect_wq = create_singlethread_workqueue("cable_detect");
		if (pInfo->cable_detect_wq == 0) {
			CABLE_ERR("usb: fail to create workqueue\n");
			return -ENOMEM;
		}
		wake_lock_init(&pInfo->cable_detect_wlock,
				WAKE_LOCK_SUSPEND, "cable_detect_lock");
	}

	if (switch_dev_register(&dock_switch) < 0) {
		CABLE_ERR("fail to register dock switch!\n");
		return 0;
	}

	ret = device_create_file(dock_switch.dev, &dev_attr_status);
	if (ret != 0)
		CABLE_ERR("dev_attr_status failed\n");

	ret = device_create_file(dock_switch.dev, &dev_attr_adc);
	if (ret != 0)
		CABLE_ERR("dev_attr_adc failed\n");

	CABLE_INFO("USB_ID ADC = %d\n", cable_detect_get_adc());

#ifdef CONFIG_CABLE_DETECT_GPIO_DOCK
	if (pInfo->dock_detect) {
		INIT_DELAYED_WORK(&pInfo->dock_work_isr, dock_isr_work);
		INIT_DELAYED_WORK(&pInfo->dock_work, dock_detect_work);
		dock_detect_init(pInfo);
	}
#endif

	usb_id_detect_init(pInfo);

	return 0;
}

struct platform_driver cable_detect_driver = {
	.probe = cable_detect_probe,
	/*.remove = __devexit_p(vbus_cable_detect_remove),*/
	.driver = {
		.name	= "cable_detect",
		.owner = THIS_MODULE,
	},
};

static int __init cable_detect_init(void)
{
	vbus = 0;
	the_cable_info.connect_type = CONNECT_TYPE_NONE;
#ifdef CONFIG_STE_HDMI_MHL
	mhl_detect_register_notifier(&mhl_status_notifier);
#endif
	return platform_driver_register(&cable_detect_driver);

}

static void __exit cable_detect_exit(void)
{
	platform_driver_unregister(&cable_detect_driver);
}

MODULE_DESCRIPTION("CABLE_DETECT");
MODULE_LICENSE("GPL");

module_init(cable_detect_init);
module_exit(cable_detect_exit);

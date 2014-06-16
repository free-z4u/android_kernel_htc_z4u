/*
 * linux/drivers/leds-pwm.c
 *
 * simple PWM based LED control
 *
 * Copyright 2009 Luotao Fu @ Pengutronix (l.fu@pengutronix.de)
 *
 * based on leds-gpio.c by Raphael Assenat <raph@8d.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/leds_pwm.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>/* Enable Backlight earlysuspend function */
//#include <video/disp_log_prefix_overwrite.h>
#include <linux/delay.h>

#if defined(CONFIG_ARCH_SCX35)
#include "../video/sc8830/sprdfb_panel.h" /* for sprd platform lcd backlight control */
#elif defined(CONFIG_ARCH_SC8825)
#include "../video/sc8825/sprdfb_panel.h"
#endif

extern struct ops_mipi sprdfb_mipi_ops;

struct led_pwm_data {
	struct led_classdev	cdev;
//	struct pwm_device	*pwm;
	unsigned int 		active_low;
	unsigned int		lth_brightness;
	unsigned int		frequency_hz;
#ifdef CONFIG_HAS_EARLYSUSPEND
       struct early_suspend early_suspend;
#ifdef CONFIG_HTC_ONMODE_CHARGING
       struct early_suspend onchg_suspend;
#endif
#endif
	unsigned char (*shrink_pwm)(struct led_classdev *, int);

};

u8 led_value = 86;
//bool led_saved= false;
volatile bool is_late_resume = true;
bool is_bl_gpio_disable = true;
extern uint16_t sprdfb_enable;

#if defined (CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255
#define BRI_SETTING_ONCHG               120

#define AUO_PWM_MIN                     13
#define AUO_PWM_DEFAULT                 79
#define AUO_PWM_MAX                     255

#elif defined(CONFIG_MACH_CP5VEDTU)
#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255
#define BRI_SETTING_ONCHG               120

#define AUO_PWM_MIN                     11
#define AUO_PWM_DEFAULT                 79
#define AUO_PWM_MAX                     255

#else
#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255
#define BRI_SETTING_ONCHG               120

#define AUO_PWM_MIN                     14	/* 5.5% of max pwm  */
#define AUO_PWM_DEFAULT                 103	/* 40% of max pwm  */
#define AUO_PWM_MAX                     255	/* 100% of max pwm  */
#endif

extern void cp5_disable_bl_gpio(void);
extern void cp5_enable_bl_gpio(void);


static unsigned char sprd_shrink_pwm(struct led_classdev *led_cdev, int br)
{
	unsigned char shrink_br;

	if (br <= 0) {
		shrink_br = 0;
	} else if (br > 0 && br <= BRI_SETTING_MIN) {
		shrink_br = AUO_PWM_MIN;
	} else if (br > BRI_SETTING_MIN && br <= BRI_SETTING_DEF) {
		shrink_br = (AUO_PWM_MIN + (br - BRI_SETTING_MIN) *
				(AUO_PWM_DEFAULT - AUO_PWM_MIN) /
				(BRI_SETTING_DEF - BRI_SETTING_MIN));
	} else if (br > BRI_SETTING_DEF && br <= BRI_SETTING_MAX) {
		shrink_br = (AUO_PWM_DEFAULT + (br - BRI_SETTING_DEF) *
				(AUO_PWM_MAX - AUO_PWM_DEFAULT) /
				(BRI_SETTING_MAX - BRI_SETTING_DEF));
	} else if (br > BRI_SETTING_MAX)
		shrink_br = AUO_PWM_MAX;
	dev_dbg(led_cdev->dev, "[DISP]brightness orig=%d, transformed=%d\n", br, shrink_br);

	return shrink_br;
}

#define SLEEP_DURING_UPDATE_MIN_US (10 * 1000)
#define SLEEP_DURING_UPDATE_MAX_US (12 * 1000)

#ifdef CONFIG_MACH_Z4TD
static LCM_Init_Code set_led_ctl =  {LCM_SEND(2), {0x53,0x24}};
#endif
static LCM_Init_Code set_led_ctl_close =  {LCM_SEND(2), {0x53,0x0}};
static LCM_Init_Code set_bri =  {LCM_SEND(2), {0x51,0xFF}};

static void cp5_handle_bl_gpio(u8 value)
{
   if(value == 0)
   {
	  cp5_disable_bl_gpio();
	  is_bl_gpio_disable = true;
   }
   else
   {
	  if(is_bl_gpio_disable)
	  {
	     cp5_enable_bl_gpio();
	  }
   }
   udelay(20);
}

static void led_pwm_set(struct led_classdev *led_cdev,
	enum led_brightness brightness)
{
	struct led_pwm_data *led_dat =
		container_of(led_cdev, struct led_pwm_data, cdev);

	mipi_gen_write_t mipi_gen_write = sprdfb_mipi_ops.mipi_gen_write;


	u8 value = (led_dat->shrink_pwm(led_cdev, brightness)) & 0xFF;
	bool did_write = false;

//	if ((brightness == BRI_SETTING_ONCHG) && (onchg_enabled == true))
//		value = led_value;

#if 0
	if (!value) {
		/* Disable dimming before suspend */
		set_led_ctl.data[1] = 0x24;

		mipi_gen_write(set_led_ctl.data, (set_led_ctl.tag & LCM_TAG_MASK));
	}
#endif

	if (sprdfb_enable && is_late_resume) {
		did_write = true;
		set_bri.data[1] = value;

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
		cp5_handle_bl_gpio(value);
#endif

		mipi_gen_write(set_bri.data, (set_bri.tag & LCM_TAG_MASK));
		udelay(20);
		printk("[DISP] led_pwm_set: set brightness to %d\n", value);
	}
	if (did_write) {
		/* wait (without holding the lock) for approx one update */
		usleep_range(SLEEP_DURING_UPDATE_MIN_US,
						SLEEP_DURING_UPDATE_MAX_US);
	}

	if (value) {
		led_value = value;
	}
}


static LCM_Init_Code set_brightness =  {LCM_SEND(2), {0x51,0xFF}};

void set_backlight(bool value)
{
	mipi_gen_write_t mipi_gen_write = sprdfb_mipi_ops.mipi_gen_write;

	if (false == value) {
		set_brightness.data[1] = 0;

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
		cp5_handle_bl_gpio(set_brightness.data[1]);
#endif

		mipi_gen_write(set_brightness.data, (set_brightness.tag & LCM_TAG_MASK));
		udelay(20);

#ifdef CONFIG_MACH_Z4TD
		mipi_gen_write(set_led_ctl_close.data, (set_led_ctl_close.tag & LCM_TAG_MASK));
		udelay(20);
#endif
	} else {
#ifdef CONFIG_MACH_Z4TD
		mipi_gen_write(set_led_ctl.data, (set_led_ctl.tag & LCM_TAG_MASK));
		udelay(20);
#endif

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
		set_brightness.data[1] = 79;
#elif defined CONFIG_MACH_Z4TD
		set_brightness.data[1] = 0x67;
#endif


#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
#ifdef CONFIG_FB_ESD_SUPPORT
		is_bl_gpio_disable = true;
#endif
		cp5_handle_bl_gpio(set_brightness.data[1]);
#endif
		mipi_gen_write(set_brightness.data, (set_brightness.tag & LCM_TAG_MASK));
		udelay(20);
	}

	printk("[DISP] set_backlight value=%d\n", set_brightness.data[1]);

	return;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void backlight_early_suspend(struct early_suspend *handler)
{
	mipi_gen_write_t mipi_gen_write = sprdfb_mipi_ops.mipi_gen_write;

	is_late_resume = false;

	set_bri.data[1] = 0;
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
		cp5_handle_bl_gpio(set_bri.data[1]);
#endif
	mipi_gen_write(set_bri.data, (set_bri.tag & LCM_TAG_MASK));
	udelay(20);

	mipi_gen_write(set_led_ctl_close.data, (set_led_ctl_close.tag & LCM_TAG_MASK));
	udelay(20);
	printk("[DISP] backlight_early_suspend: brightness = 0\n");
	printk("[DISP] panel off\n");
}
static void backlight_late_resume(struct early_suspend *handler)
{
	mipi_gen_write_t mipi_gen_write = sprdfb_mipi_ops.mipi_gen_write;

#ifdef CONFIG_MACH_Z4TD
	hr_msleep(125);

	mipi_gen_write(set_led_ctl.data, (set_led_ctl.tag & LCM_TAG_MASK));
	udelay(20);
#endif

	set_bri.data[1] = led_value;

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
	cp5_handle_bl_gpio(set_bri.data[1]);
#endif
	mipi_gen_write(set_bri.data, (set_bri.tag & LCM_TAG_MASK));
	udelay(20);
	printk("[DISP] backlight_late_resume: brightness = %d\n", led_value);
	printk("[DISP] panel on\n");
	is_late_resume = true;
}
#ifdef CONFIG_HTC_ONMODE_CHARGING
static void backlight_onchg_suspend(struct early_suspend *handler)
{
	printk("[DISP] backlight_onchg_suspend %s\n", __func__);
	is_late_resume = false;
}
static void backlight_onchg_resume(struct early_suspend *handler)
{
	mipi_gen_write_t mipi_gen_write = sprdfb_mipi_ops.mipi_gen_write;

#ifdef CONFIG_MACH_Z4TD
	hr_msleep(300);

	mipi_gen_write(set_led_ctl.data, (set_led_ctl.tag & LCM_TAG_MASK));
	udelay(20);
#endif

	set_bri.data[1] = led_value;

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
	cp5_handle_bl_gpio(set_bri.data[1]);
#endif

	mipi_gen_write(set_bri.data, (set_bri.tag & LCM_TAG_MASK));
	udelay(20);
	printk("[DISP] backlight_onchg_resume: brightness = %d\n", led_value);

	is_late_resume = true;
}
#endif
#endif
 
 static int led_pwm_probe(struct platform_device *pdev)
 {
	struct led_pwm_data *led_dat;
	int ret = 0;
 
	led_value = 0xFF;
 
	led_dat = kzalloc(sizeof(struct led_pwm_data), GFP_KERNEL);
	if (!led_dat)
		return -ENOMEM;
 
	led_dat->cdev.name = pdev->name;
//	led_dat->cdev.default_trigger = "none";
//	led_dat->active_low = cur_led->active_low;
//	led_dat->frequency_hz = cur_led->pwm_frequency_hz;
	led_dat->cdev.brightness_set = led_pwm_set;
//	led_dat->cdev.brightness_get = NULL;
	led_dat->cdev.brightness = LED_OFF;
	led_dat->cdev.max_brightness = 0xFF;
	led_dat->cdev.flags |= LED_CORE_SUSPENDRESUME;

	ret = led_classdev_register(&pdev->dev, &led_dat->cdev);
	if (ret < 0) {
		goto err;
 	}
 
	led_dat->shrink_pwm = sprd_shrink_pwm;

#ifdef CONFIG_HAS_EARLYSUSPEND
	led_dat->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	led_dat->early_suspend.suspend =  backlight_early_suspend;
	led_dat->early_suspend.resume = backlight_late_resume;
	register_early_suspend(&led_dat->early_suspend);
#ifdef CONFIG_HTC_ONMODE_CHARGING
	led_dat->onchg_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	led_dat->onchg_suspend.suspend =  backlight_onchg_suspend;
	led_dat->onchg_suspend.resume = backlight_onchg_resume;
	register_onchg_suspend(&led_dat->onchg_suspend);
#endif
#endif

	platform_set_drvdata(pdev, led_dat);

	pr_info("[DISP] %s OK \n", __func__);
 
 	return 0;
 
 err:
	led_classdev_unregister(&led_dat->cdev);
	kfree(led_dat);
 
 	return ret;
 }
 
 static int __devexit led_pwm_remove(struct platform_device *pdev)
 {
	struct led_pwm_data *led_dat;
 
	led_dat = platform_get_drvdata(pdev);

	led_classdev_unregister(&led_dat->cdev);

	kfree(led_dat);
 
 	return 0;
 }
 static struct platform_driver led_pwm_driver = {
 	.probe		= led_pwm_probe,
 	.remove		= __devexit_p(led_pwm_remove),
 	.driver		= {
		.name	= "lcd-backlight",
 		.owner	= THIS_MODULE,
 	},
 };
 
 static int __init led_pwm_init(void)
 {
	return platform_driver_register(&led_pwm_driver);
 }

 static void __exit led_pwm_exit(void)
 {
	platform_driver_unregister(&led_pwm_driver);
 }

 module_init(led_pwm_init);
 module_exit(led_pwm_exit);
 
 MODULE_AUTHOR("Luotao Fu <l.fu@pengutronix.de>");
 MODULE_DESCRIPTION("PWM LED driver for PXA");
 MODULE_LICENSE("GPL");
 MODULE_ALIAS("platform:leds-pwm");




#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/rtc.h>
#include <linux/wakelock.h>
#include <asm/atomic.h>
#include <linux/leds.h>
#include <mach/sci.h>
#include <mach/adi.h>
#include <mach/hardware.h>
#include <mach/sci_glb_regs.h>
#include <linux/leds-htc-sprd.h>
#include <linux/gpio.h>
#include <mach/board_htc.h>

/*#define DEBUG*/
#ifdef DEBUG
#define LED_DBG_LOG(fmt, ...) \
		printk(KERN_DEBUG "[LED]" fmt, ##__VA_ARGS__)
#else
#define LED_DBG_LOG(fmt, ...)
#endif
#define LED_INFO_LOG(fmt, ...) \
		printk(KERN_INFO "[LED]" fmt, ##__VA_ARGS__)
#define LED_ERR_LOG(fmt, ...) \
		printk(KERN_ERR "[LED][ERR]" fmt, ##__VA_ARGS__)

#define LED_ALM(x...) do { \
struct timespec ts; \
struct rtc_time tm; \
getnstimeofday(&ts); \
rtc_time_to_tm(ts.tv_sec, &tm); \
printk(KERN_INFO "[LED-ALM] " x); \
printk(" at %lld (%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", \
ktime_to_ns(ktime_get()), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, \
tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec); \
} while (0)

static struct workqueue_struct *led_wq;

#define SCI_BLTC_BASE (SPRD_MISC_BASE + 0x83c0)
#define GPIO_BUTTON_BACKLIGHT 234
#define KPLED_CTL ANA_REG_GLB_ANA_DRV_CTRL
#define KPLED_PD_SET (0x01 << 1)
#define KPLED_V_SHIFT 4
#define KPLED_V_MSK (0x0f << KPLED_V_SHIFT)

#if defined (CONFIG_MACH_CP5DTU)
#define AMBER_DEFAULT_BRIGHTNESS 30
#define GREEN_DEFAULT_BRIGHTNESS 30
#else
#define AMBER_DEFAULT_BRIGHTNESS 50
#define GREEN_DEFAULT_BRIGHTNESS 50
#endif

atomic_t led_board_version;
static struct mutex led_sprd_muxtex;

#if 0
static void __sci_blc_glb_enable(bool on)
{
	int i;

	if (on) {
		sci_adi_set(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_BLTC_EN);
		sci_adi_set(ANA_REG_GLB_RTC_CLK_EN, BIT_RTC_BLTC_EN);
		sci_adi_set(ANA_REG_GLB_ANA_DRV_CTRL, BIT_SLP_RGB_PD_EN |
					BIT_RGB_PD_HW_EN );
		sci_adi_set(ANA_REG_GLB_ARM_RST, BIT_ANA_BLTC_SOFT_RST);
		for (i = 0; i < 0xf00; i++);
		sci_adi_clr(ANA_REG_GLB_ARM_RST, BIT_ANA_BLTC_SOFT_RST);
	} else {
		sci_adi_clr(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_BLTC_EN);
		sci_adi_clr(ANA_REG_GLB_RTC_CLK_EN, BIT_RTC_BLTC_EN);
		sci_adi_clr(ANA_REG_GLB_ANA_DRV_CTRL, BIT_SLP_RGB_PD_EN |
					BIT_RGB_PD_HW_EN );
	}
}
#endif

/*
 *brightness:	0-255
 *blink mode:	0.	aways on.
 *		1.	blink 62ms per 2s.
 *		2.	blink 1s per 2s.
 * */
static void htc_sprd_led_control(struct htc_sprd_led_data *ldata)
{
	int blink = atomic_read(&ldata->blink);
	int brightness = atomic_read(&ldata->brightness);

	mutex_lock(&led_sprd_muxtex);

	if (ldata->out_current < 0 || ldata->out_current > 255 ||
			brightness < 0 || brightness >255 ||
			blink < 0 || blink > 3)
		LED_ERR_LOG("%s: Wrong LED data. brightness = %d, blink mode = %d\n",
				__func__, brightness, blink);

	if (0 == strcmp(ldata->ldev.name, "amber")) {

		if (1 == brightness)
			brightness = AMBER_DEFAULT_BRIGHTNESS; //Set leds default lux
		sci_adi_raw_write(SCI_BLTC_BASE + 0x08, (brightness << 8) + 0xff);

		switch (blink) {
			case 0:
				if (brightness == 0)
					LED_DBG_LOG("%s: %s turn off\n",__func__, ldata->ldev.name);
				break;
			case 1:
				sci_adi_raw_write(SCI_BLTC_BASE + 0x0c, 0x0101);
				sci_adi_raw_write(SCI_BLTC_BASE + 0x10, 0x0d01);
				break;
			case 2:
				sci_adi_raw_write(SCI_BLTC_BASE + 0x0c, 0x0101);
				sci_adi_raw_write(SCI_BLTC_BASE + 0x10, 0x0707);
				break;
		}
		if(blink)
			sci_adi_clr(SCI_BLTC_BASE, BIT(1));
		else
			sci_adi_set(SCI_BLTC_BASE, BIT(1));
		if(brightness){
			sci_adi_set(SCI_BLTC_BASE, BIT(0));
			LED_DBG_LOG("%s: %s turn on, brightness=%d\n", __func__, ldata->ldev.name, brightness);
		}else
			sci_adi_clr(SCI_BLTC_BASE, BIT(0));
	}

	if (0 == strcmp(ldata->ldev.name, "green")) {

		if (1 == brightness)
			brightness = GREEN_DEFAULT_BRIGHTNESS; //Set leds default lux
		sci_adi_raw_write(SCI_BLTC_BASE + 0x18, (brightness << 8) + 0xff);

		switch (blink) {
			case 0:
				if (brightness == 0)
				LED_DBG_LOG("%s: %s turn off\n", __func__, ldata->ldev.name);
				break;
			case 1:
				sci_adi_raw_write(SCI_BLTC_BASE + 0x1c, 0x0101);
				sci_adi_raw_write(SCI_BLTC_BASE + 0x20, 0x0d01);
				break;
			case 2:
				sci_adi_raw_write(SCI_BLTC_BASE + 0x1c, 0x0101);
				sci_adi_raw_write(SCI_BLTC_BASE + 0x20, 0x0707);
				break;
		}
		if(blink)
			sci_adi_clr(SCI_BLTC_BASE, BIT(5));
		else
			sci_adi_set(SCI_BLTC_BASE, BIT(5));
		if(brightness){
			sci_adi_set(SCI_BLTC_BASE, BIT(4));
			LED_DBG_LOG("%s: %s turn on, brightness=%d\n", __func__, ldata->ldev.name, brightness);
		}else
			sci_adi_clr(SCI_BLTC_BASE, BIT(4));
	}

	if (0 == strcmp(ldata->ldev.name, "button-backlight")) {
		if (!atomic_read(&led_board_version)) {
			gpio_request(GPIO_BUTTON_BACKLIGHT,"button_backlight");/* GPIO234 */
			if(brightness) {
				gpio_direction_output(GPIO_BUTTON_BACKLIGHT,1);
				LED_DBG_LOG("%s: %s turn on\n", __func__, ldata->ldev.name);
			} else {
				gpio_direction_output(GPIO_BUTTON_BACKLIGHT,0);
			}
		} else {
			/* if (brightness > 255)
				brightness = 255;
			else if (brightness < 16)
				brightness = 0; */
			if (brightness) {
				brightness = brightness/16;
				sci_adi_write(KPLED_CTL, ((brightness << KPLED_V_SHIFT) & KPLED_V_MSK), KPLED_V_MSK);
				sci_adi_clr(KPLED_CTL, KPLED_PD_SET);
				} else {
					sci_adi_set(KPLED_CTL, KPLED_PD_SET);
				}
		}
		switch (blink) {
			case 0:
				if (brightness == 0)
				LED_INFO_LOG("%s: %s turn off\n",	__func__, ldata->ldev.name);
				break;
			case 1:
				break;
			case 2:
				break;
		}
	}
	ldata->ldev.brightness = brightness;
	LED_INFO_LOG("%s(%d): KPLED_CTL = %X,LED_VERSION=%d,\n",
						__func__,brightness,sci_adi_read(KPLED_CTL),atomic_read(&led_board_version));

	mutex_unlock(&led_sprd_muxtex);
}

static void led_do_blink(struct work_struct *work)
{
	struct htc_sprd_led_data *ldata;
	LED_DBG_LOG("%s()\n", __func__);
	ldata = container_of(work, struct htc_sprd_led_data, blink_work.work);

	htc_sprd_led_control(ldata);
}

static void htc_sprd_led_brightness_set(struct led_classdev *led_cdev,
					   enum led_brightness brightness)
{
	struct htc_sprd_led_data *ldata;

	ldata = container_of(led_cdev, struct htc_sprd_led_data, ldev);
	if (brightness > 0 && (ldata->flag & FIX_BRIGHTNESS))
		brightness = ldata->init_pwm_brightness;

	if (atomic_read(&ldata->brightness) != brightness){
		atomic_set(&ldata->brightness, brightness);
		/*disable blink*/
		atomic_set(&ldata->blink, BLINK_DISABLE);
		atomic_set(&ldata->blink_old, 0);
		/*control led*/
		htc_sprd_led_control(ldata);
		LED_INFO_LOG("%s:%s LED %s\n",__func__, ldata->ldev.name,brightness ? "on":"off");
	} else {
		LED_INFO_LOG("%s:%s same brightness %d\n",__func__, ldata->ldev.name,atomic_read(&ldata->brightness));
		return;
	}

}

static ssize_t htc_sprd_led_blink_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct htc_sprd_led_data *ldata;
	struct led_classdev *led_cdev;
	uint32_t val;

	sscanf(buf, "%u", &val);

	if (val < 0 || val > 4){
		LED_ERR_LOG("%s: Wrong blink mode %u", __func__, val);
		return -EINVAL;
	}

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct htc_sprd_led_data, ldev);

	if (atomic_read(&ldata->blink_old) != val){
		atomic_set(&ldata->blink, val);
		atomic_set(&ldata->blink_old, val);
		LED_INFO_LOG("%s:blink mode %u", __func__, val);
	} else {
		LED_INFO_LOG("%s:%s same blink mode %u", __func__,ldata->ldev.name, val);
		return count;
	}

	switch(atomic_read(&ldata->blink)){
		case 0:
			atomic_set(&ldata->blink, BLINK_DISABLE);
			htc_sprd_led_control(ldata);
			break;
		case 1:
			cancel_delayed_work_sync(&ldata->blink_work);
			atomic_set(&ldata->blink, BLINK_64MS_PER_2S);
			queue_delayed_work(led_wq, &ldata->blink_work, msecs_to_jiffies(314));
			break;
		case 2:
			atomic_set(&ldata->blink, BLINK_1S_PER_2S);
			htc_sprd_led_control(ldata);
			break;
		case 3:
			cancel_delayed_work_sync(&ldata->blink_work);
			atomic_set(&ldata->blink, BLINK_64MS_PER_2S);
			queue_delayed_work(led_wq, &ldata->blink_work, msecs_to_jiffies(1000));
			break;
		case 4:
			atomic_set(&ldata->blink, BLINK_1S_PER_2S);
			htc_sprd_led_control(ldata);
			break;
		default:
			return -EINVAL;
	}

	return count;
}

static ssize_t htc_sprd_led_blink_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct htc_sprd_led_data *ldata;
        struct led_classdev *led_cdev;

        led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
        ldata = container_of(led_cdev, struct htc_sprd_led_data, ldev);

	return sprintf(buf, "%d\n", atomic_read(&ldata->blink));
}
static DEVICE_ATTR(blink, 0664, htc_sprd_led_blink_show, htc_sprd_led_blink_store);

static ssize_t htc_sprd_brightness_test_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev;
	struct htc_sprd_led_data *ldata;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct htc_sprd_led_data, ldev);

	return sprintf(buf, "%d\n", atomic_read(&ldata->brightness));
}

static ssize_t htc_sprd_brightness_test_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int test_brightness = 0;
	struct led_classdev *led_cdev;
	struct htc_sprd_led_data *ldata;

	sscanf(buf, "%d", &test_brightness);
	if (test_brightness < 0)
		return -EINVAL;

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct htc_sprd_led_data, ldev);

	atomic_set(&ldata->brightness, test_brightness);
	/*disable blink*/
	atomic_set(&ldata->blink, BLINK_DISABLE);
	atomic_set(&ldata->blink_old, 0);
	/*control led*/
	htc_sprd_led_control(ldata);

	LED_INFO_LOG("%s: brightness = %d\n",__func__, test_brightness);
	return count;
}

static DEVICE_ATTR(brightness_test, 0664, htc_sprd_brightness_test_show,
		   htc_sprd_brightness_test_store);


static ssize_t htc_sprd_led_off_timer_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev;
	struct htc_sprd_led_data *ldata;

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct htc_sprd_led_data, ldev);
	return sprintf(buf, "%d s\n", atomic_read(&ldata->off_timer));
}

static ssize_t htc_sprd_led_off_timer_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct led_classdev *led_cdev;
	struct htc_sprd_led_data *ldata;
	int min, sec;
	ktime_t interval;
	ktime_t next_alarm;

	min = -1;
	sec = -1;
	sscanf(buf, "%d %d", &min, &sec);

	if (min < 0 || min > 255 || sec < 0 || sec > 255) {
		LED_ERR_LOG("%s: min=%d, sec=%d,Invalid off_timer!\n",
				__func__, min, sec);
		return -EINVAL;
	}

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct htc_sprd_led_data, ldev);

	LED_INFO_LOG("Setting %s off_timer to %d min %d sec\n",
					   led_cdev->name, min, sec);

	atomic_set(&ldata->off_timer, min * 60 + sec);
	alarm_cancel(&ldata->off_timer_alarm);
	cancel_work_sync(&ldata->off_timer_work);
	if (atomic_read(&ldata->off_timer)) {
		interval = ktime_set(atomic_read(&ldata->off_timer), 0);
		next_alarm = ktime_add(ktime_get_boottime(), interval);
		alarm_start(&ldata->off_timer_alarm, next_alarm);
		LED_ALM("led alarm start -");
	}
	return count;
}

static DEVICE_ATTR(off_timer, 0664, htc_sprd_led_off_timer_show,
				      htc_sprd_led_off_timer_store);

static void led_work_func(struct work_struct *work)
{
	struct htc_sprd_led_data *ldata;

	ldata = container_of(work, struct htc_sprd_led_data, off_timer_work);
	LED_ALM("%s led alarm led work +" , ldata->ldev.name);
	/*disable LED*/
	atomic_set(&ldata->brightness, 0);
	atomic_set(&ldata->blink, 0);
	atomic_set(&ldata->blink_old, 0);
	atomic_set(&ldata->off_timer, 0);
	htc_sprd_led_control(ldata);
	//LED_ALM("%s led alarm led work -" , ldata->ldev.name);
}

static enum alarmtimer_restart led_alarm_handler(struct alarm *alarm, ktime_t now)
{
	struct htc_sprd_led_data *ldata;

	ldata = container_of(alarm, struct htc_sprd_led_data, off_timer_alarm);
	LED_ALM("%s led alarm trigger +", ldata->ldev.name);
	queue_work(led_wq, &ldata->off_timer_work);
	//LED_ALM("%s led alarm trigger -", ldata->ldev.name);
	return ALARMTIMER_NORESTART;
}

static int htc_sprd_led_probe(struct platform_device *pdev)
{
	struct htc_sprd_led_platform_data *pdata;
	struct htc_sprd_led_data *ldata;
	int i = 0, ret = -ENOMEM;
	atomic_set(&led_board_version,0);

	pdata = pdev->dev.platform_data;
	if (pdata == NULL) {
		LED_ERR_LOG("%s: platform data is NULL\n", __func__);
		return -ENODEV;
	}

	ldata = kzalloc(sizeof(struct htc_sprd_led_data)
			* pdata->num_leds, GFP_KERNEL);
	if (!ldata && pdata->num_leds) {
		ret = -ENOMEM;
		LED_ERR_LOG("%s: failed on allocate ldata\n", __func__);
		goto err_exit;
	}

	dev_set_drvdata(&pdev->dev, ldata);
	mutex_init(&led_sprd_muxtex);
	led_wq = create_singlethread_workqueue("led_blink");
	if (!led_wq){
		LED_ERR_LOG("%s: failed on create workqueue!\n", __func__);
		goto err_create_work_queue;
	}

	/*register led_classdev and brightness sysfs for white/green/amber LED*/
	for (i = 0; i < pdata->num_leds; i++) {
		ldata[i].ldev.name = pdata->led_config[i].name;
		ldata[i].bank = pdata->led_config[i].bank;
		ldata[i].init_pwm_brightness =  pdata->led_config[i].init_pwm_brightness;
		ldata[i].out_current =  pdata->led_config[i].out_current;
		ldata[i].flag = pdata->led_config[i].flag;
		atomic_set(&ldata[i].off_timer, 0);
		atomic_set(&ldata[i].brightness, -1);
		atomic_set(&ldata[i].blink, 0);
		atomic_set(&ldata[i].blink_old, -1);

		ldata[i].ldev.brightness_set = htc_sprd_led_brightness_set;

		ret = led_classdev_register(&pdev->dev, &ldata[i].ldev);
		if (ret < 0) {
			LED_ERR_LOG("%s: failed on led_classdev_register [%s]\n",
				__func__, ldata[i].ldev.name);
			goto err_register_led_cdev;
		}
	}

	/*register blink sysfs for green/amber LED*/
	for (i = 0; i < pdata->num_leds; i++) {
		ret = device_create_file(ldata[i].ldev.dev, &dev_attr_blink);
		if (ret < 0){
			LED_ERR_LOG("%s: Failed to create attr blink [%d]\n",
					__func__, i);
			goto err_register_attr_blink;
		}
	}

	/*register fix_brightness sysfs for green/amber LED*/
	for (i = 0; i < pdata->num_leds; i++) {
		ret = device_create_file(ldata[i].ldev.dev, &dev_attr_brightness_test);
		if (ret < 0){
			LED_ERR_LOG("%s: Failed to create attr brightness_test [%d]\n",
					__func__, i);
			goto err_register_attr_brightness_test;
		}
	}

	/*register offtimer sysfs for green/amber LED*/
    for (i = 0; i < pdata->num_leds; i++) {
            ret = device_create_file(ldata[i].ldev.dev, &dev_attr_off_timer);
            if (ret < 0){
                    LED_ERR_LOG("%s: Failed to vreate attr off_timer [%d]\n",
                                    __func__, i);
                    goto err_register_attr_off_timer;
            }
	INIT_DELAYED_WORK(&ldata[i].blink_work, led_do_blink);
	INIT_WORK(&ldata[i].off_timer_work, led_work_func);
	alarm_init(&ldata[i].off_timer_alarm, ALARM_BOOTTIME, led_alarm_handler);
    }

	if (pdata->led_get_board_version)
		atomic_set(&led_board_version,pdata->led_get_board_version());

	LED_INFO_LOG("%s: ANA_DRV_CTRL = %X ,led_board_version = %d\n",
		__func__,sci_adi_read(ANA_REG_GLB_ANA_DRV_CTRL),atomic_read(&led_board_version));

	//Init in hboot
#if 0
	__sci_blc_glb_enable(true);
#endif

	sci_adi_raw_write(SCI_BLTC_BASE + 0x0c, 0x0101);
	sci_adi_raw_write(SCI_BLTC_BASE + 0x10, 0x0d01);
	sci_adi_raw_write(SCI_BLTC_BASE + 0x1c, 0x0101);
	sci_adi_raw_write(SCI_BLTC_BASE + 0x20, 0x0d01);

	LED_INFO_LOG("%s: probe ok!\n",__func__);

	return 0;

err_register_attr_off_timer:
	for (i--; i >= 0; i--) {
                device_remove_file(ldata[i].ldev.dev, &dev_attr_off_timer);
        }
err_register_attr_brightness_test:
	for (i--; i >= 0; i--) {
				device_remove_file(ldata[i].ldev.dev, &dev_attr_brightness_test);
		}
err_register_attr_blink:
	i = pdata->num_leds;
	for (i--; i >= 0; i--) {
		device_remove_file(ldata[i].ldev.dev, &dev_attr_blink);
	}
err_register_led_cdev:
	for (i--; i >= 0; i--) {
		led_classdev_unregister(&ldata[i].ldev);
	}
	destroy_workqueue(led_wq);
err_create_work_queue:
	mutex_destroy(&led_sprd_muxtex);
	kfree(ldata);
err_exit:

	LED_ERR_LOG("%s: probe failed!\n",__func__);
	return ret;
}

static int __devexit htc_sprd_led_remove(struct platform_device *pdev)
{
	struct htc_sprd_led_platform_data *pdata;
	struct htc_sprd_led_data *ldata;

	pdata = pdev->dev.platform_data;
	ldata = platform_get_drvdata(pdev);

	kfree(ldata);

	return 0;
}

static struct platform_driver htc_sprd_led_driver = {
	.probe = htc_sprd_led_probe,
	.remove = __devexit_p(htc_sprd_led_remove),
	.driver = {
		   .name = "leds-htc-sprd",
		   .owner = THIS_MODULE,
		   },
};

int __init htc_sprd_led_init(void)
{
	return platform_driver_register(&htc_sprd_led_driver);
}

void htc_sprd_led_exit(void)
{
	platform_driver_unregister(&htc_sprd_led_driver);
}

module_init(htc_sprd_led_init);
module_exit(htc_sprd_led_exit);

MODULE_DESCRIPTION("HTC sprd led driver");
MODULE_LICENSE("GPL");

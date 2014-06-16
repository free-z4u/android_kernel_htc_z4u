
#ifndef _LINUX_LEDS_HTC_SPRD_H
#define _LINUX_LEDS_HTC_SPRD_H

#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/alarmtimer.h>

/*LED blink mode for modem*/
#define BLINK_DISABLE		0
#define BLINK_64MS_PER_2S	1
#define BLINK_1S_PER_2S		2

#define FIX_BRIGHTNESS		(1 << 0)
#define DYNAMIC_BRIGHTNESS	(1 << 1)
struct htc_sprd_led_config {
	const char *name;
	uint32_t bank;
	uint32_t init_pwm_brightness;
	uint32_t out_current;
	uint32_t flag;
};

struct htc_sprd_led_platform_data {
	struct htc_sprd_led_config *led_config;
	uint32_t num_leds;
	int (*led_get_board_version)(void);
};

struct htc_sprd_led_data {
	struct led_classdev ldev;
	struct pm8029_led_config *led_config;
	struct delayed_work blink_work;
	struct work_struct off_timer_work;
	struct alarm off_timer_alarm;
	atomic_t brightness;
	atomic_t blink;
	atomic_t blink_old;
	atomic_t off_timer;
	uint8_t bank;
	uint8_t init_pwm_brightness;
	uint8_t out_current;
	uint32_t flag;
};

#endif /* _LINUX_LEDS_HTC_SPRD_H */


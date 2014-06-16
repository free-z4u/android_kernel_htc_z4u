#ifndef _CABLE_DETECT_H_
#define _CABLE_DETECT_H_

#define DOCK_STATE_UNDEFINED		-1
#define DOCK_STATE_UNDOCKED		0
#define DOCK_STATE_DESK			(1 << 0)
#define DOCK_STATE_CAR			(1 << 1)
#define DOCK_STATE_USB_HEADSET		(1 << 2)
#define DOCK_STATE_MHL			(1 << 3)
#define DOCK_STATE_USB_HOST		(1 << 4)

#define DOCK_DET_DELAY		HZ/4

#define ADC_RETRY 3
#define ADC_DELAY HZ/8

#define PM8058ADC_15BIT(adc) ((adc * 2200) / 32767) /* vref=2.2v, 15-bits resolution */
#define MICROPADC_3P0_10BIT(adc) ((adc * 3000) / 1023)

#define CABLE_ERR(fmt, args...) \
	printk(KERN_ERR "[CABLE:ERR] " fmt, ## args)
#define CABLE_WARNING(fmt, args...) \
	printk(KERN_WARNING "[CABLE] " fmt, ## args)
#define CABLE_INFO(fmt, args...) \
	printk(KERN_INFO "[CABLE] " fmt, ## args)
#define CABLE_DEBUG(fmt, args...) \
	printk(KERN_DEBUG "[CABLE] " fmt, ## args)

enum detect_type {
	CABLE_TYPE_UNKOWN = 0,
	CABLE_TYPE_ID_PIN,
	CABLE_TYPE_PMIC_ADC,
	CABLE_TYPE_MICROP_ADC,
	CABLE_TYPE_AB8500,
};

enum dpdn_path_type {
	PATH_USB = 0,
	PATH_MHL,
	PATH_USB_AUD,
	PATH_UART,
};

 struct cable_detect_platform_data {
	/* 1 : uart, 0 : usb */
	void (*usb_uart_switch)(int);
	void (*usb_dpdn_switch)(int);

	/* for accessory detection */
	u8 accessory_type;
	u8 mfg_usb_carkit_enable;
	int usb_id_pin_gpio;
	__u8 detect_type;
	int mhl_reset_gpio;
	bool mhl_version_ctrl_flag;
	void (*config_usb_id_gpios)(bool enable);
	void (*mhl_1v2_power)(bool enable);
	int (*is_wireless_charger)(void);
	int64_t (*get_adc_cb)(void);

	int ac_9v_gpio;
	void (*configure_ac_9v_gpio) (int);
	u8 mhl_internal_3v3;

#ifdef CONFIG_CABLE_DETECT_GPIO_DOCK
	bool dock_detect;
	int dock_pin_gpio;
#endif
	void (*enable_host_mode)(bool enable);
};

#ifndef CONFIG_CABLE_DETECT_ACCESSORY
static inline int cable_get_connect_type(void) {return 0; }
static inline void set_mfg_usb_carkit_enable(int enable) {return; }
static inline int cable_get_accessory_type(void) {return 0; }
static inline int cable_get_usb_id_level(void) {return 0; }
static inline void cable_set_uart_switch(int path) {return; }
#else
extern int cable_get_connect_type(void);
extern void set_mfg_usb_carkit_enable(int enable);
extern int cable_get_accessory_type(void);
extern int cable_get_usb_id_level(void);
extern void cable_set_uart_switch(int);
#endif
#endif

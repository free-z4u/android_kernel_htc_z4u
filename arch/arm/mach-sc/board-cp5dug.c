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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach/time.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/localtimer.h>

#include <mach/hardware.h>
#include <linux/i2c.h>
#include <linux/i2c/ft5306_ts.h>
#include <linux/i2c/lis3dh.h>
#include <linux/i2c/ltr_558als.h>
#include <linux/akm8975.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi_gpio.h>
#include <mach/board.h>
#include <mach/serial_sprd.h>
#include <mach/adi.h>
#include <mach/adc.h>
#include <mach/pinmap.h>
#include <linux/akm8975.h>
#include <linux/akm8963.h>
#include <linux/bma250.h>
#include <linux/irq.h>
#include <linux/persistent_ram.h>

#include <mach/sci.h>
#include <mach/hardware.h>
#include <mach/sci_glb_regs.h>
#include <mach/htc_util.h>

//HTC_AUD_START Yangyi
#ifdef CONFIG_AMP_RT5501
#include <mach/rt5501.h>
#endif
//HTC_AUD_END

#include "devices.h"
#include <linux/tps65200.h>
/*  HTC_PROC_EMMC_START */
#include <mach/board.h>
#include <mach/board_htc.h>
#include <linux/proc_fs.h>
/*  HTC_PROC_EMMC_END */

/* IRQ's for the multi sensor board */
#define MPUIRQ_GPIO 212
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#include <linux/spi/mxd_cmmb_026x.h>

#ifdef CONFIG_SENSORS_NFC_PN544
#include <linux/pn544.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
#include <linux/synaptics_i2c_rmi.h>
#endif
#ifdef CONFIG_INPUT_CAPELLA_CM36282
//#include <linux/cm36282.h>
#include <linux/cm3629.h>
#endif
#ifdef CONFIG_CABLE_DETECT_ACCESSORY
#include <mach/cable_detect.h>
#endif
#include <linux/usb/android_board.h>

#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_pmic.h>
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
#include <mach/htc_headset_one_wire.h>
#endif
#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif

#include <linux/leds-htc-sprd.h>
#include <linux/gpio_keys.h>

#ifdef CONFIG_PERFLOCK_HTC
#include <mach/perflock.h>
#include <mach/freq_table.h>
#endif /* CONFIG_PERFLOCK_HTC */


/* Battery, icey add for charger INT*/
#define GPIO_CHG_STAT 	111
#define GPIO_CHG_INT 	110

extern void __init sci_reserve(void);
extern void __init sci_map_io(void);
extern void __init sci_init_irq(void);
extern void __init sci_timer_init(void);
extern int __init sci_clock_init(void);
extern int __init sci_regulator_init(void);
#ifdef CONFIG_ANDROID_RAM_CONSOLE
extern int __init sprd_ramconsole_init(void);
#endif
extern void __init board_init_modem(void);

static struct platform_device rfkill_device;
static struct platform_device brcm_bluesleep_device;
static struct platform_device kb_backlight_device;
static struct platform_device android_usb_device;
static struct platform_device spigpio_device;
static struct platform_device cable_detect_device;
static struct platform_device htc_headset_mgr;
static struct platform_device htc_sprd_leds_device;

#ifdef RAWCHIP_GPIO_SPI
static struct platform_device spigpio_device;
#endif

struct platform_device rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		//.platform_data = &msm_rawchip_data,
	},
};

static struct platform_device *devices[] __initdata = {
	&sprd_serial_device0,
	&sprd_serial_device1,
	&sprd_serial_device2,
#ifdef CONFIG_VIA_MODEM
	&sprd_serial_device4,
#endif
	&sprd_serial_device3,
	/* FIXME: jianjun.he */
	&sprd_device_rtc,
	&sprd_eic_gpio_device,
	&sprd_nand_device,
	&sprd_lcd_device0,
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&sprd_ram_console,
#endif
//	&sprd_backlight_device,
	&led_pwm_device,
	&sprd_i2c_device0,
	&sprd_i2c_device1,
	&sprd_i2c_device2,
	&sprd_i2c_device3,
	&sprd_spi0_device,
	&sprd_spi1_device,
	&sprd_spi2_device,
	&sprd_audio_platform_pcm_device,
	&sprd_audio_cpu_dai_vaudio_device,
	&sprd_audio_cpu_dai_vbc_device,
	&sprd_audio_codec_sprd_codec_device,
	&sprd_audio_cpu_dai_i2s_device,
	&sprd_audio_cpu_dai_i2s_device1,
	&sprd_audio_cpu_dai_i2s_device2,
	&sprd_audio_cpu_dai_i2s_device3,
	&sprd_audio_codec_null_codec_device,
#ifdef CONFIG_SND_SOC_TFA9887_CODEC
	&sprd_audio_codec_tfa9887_codec_device,
#endif
	//&sprd_battery_device,
#ifdef CONFIG_ION
	&sprd_ion_dev,
#endif
	&sprd_emmc_device,
	&sprd_sdio0_device,
	&sprd_sdio1_device,//wifi
	&sprd_sdio2_device,
	&sprd_vsp_device,
	&sprd_jpg_device,
	&sprd_dcam_device,
	&sprd_gsp_device,
	&sprd_scale_device,
	&sprd_rotation_device,
	&sprd_sensor_device,
	&sprd_isp_device,
	&sprd_dma_copy_device,
	&sprd_ahb_bm_device,
	&sprd_axi_bm_device,
#ifdef CONFIG_BT
	&rfkill_device,
	&brcm_bluesleep_device,
#endif
	&kb_backlight_device,
	&sprd_a7_pmu_device,
#ifdef CONFIG_USB_G_ANDROID
	&android_usb_device,
#endif
#ifdef CONFIG_CABLE_DETECT_ACCESSORY
	&cable_detect_device,
#endif

#ifdef RAWCHIP_GPIO_SPI
	&spigpio_device,
#endif

	&htc_headset_mgr,
	&htc_sprd_leds_device,
	&htc_battery_pdev,
	&max17050_battery_pdev,
	&rawchip_device,
        &sprd_thm_device,
//        &sprd_thm_a_device,
};

#define PB_INT	EIC_KEY_POWER

/* Keys/buttons connected directly to GPIO pins */
static struct gpio_keys_button gpio_keys[] = {
	{
		.gpio		= PB_INT,//GPIO_KEY_POWER,
		.code		= KEY_POWER,
		.desc		= "Power",
		.active_low	= 0,//1,
		/* This is the power key, wake from platform sleep */
		.wakeup		= 0,//1,
		.debounce_interval = 20,
	},
	{
		.gpio		= GPIO_KEY_VOLUMEUP,
		.code		= KEY_VOLUMEUP,
		.desc		= "VolUp",
		.active_low	= 1,
		.debounce_interval = 20,
	},
	{
		.gpio		= GPIO_KEY_VOLUMEDOWN,
		.code		= KEY_VOLUMEDOWN,
		.desc		= "VolDown",
		.active_low	= 1,
		.debounce_interval = 20,
	},
};

static struct gpio_keys_platform_data gpio_keys_data = {
	.buttons	= gpio_keys,
	.nbuttons	= ARRAY_SIZE(gpio_keys),
};

static struct platform_device gpio_keys_device = {
	.name			= "gpio-keys",
	.id				= -1,
	.num_resources	= 0,
	.dev			=	{
							.platform_data	= &gpio_keys_data,
						}
};

static void __init add_gpio_keys(void)
{
	int ret;
	ret = gpio_request(GPIO_KEY_IN0,"key_gnd");
	if(ret < 0)
		pr_err("unable to request KEY_IN0\n");

	gpio_direction_output(GPIO_KEY_IN0,0);
	platform_device_register(&gpio_keys_device);
}

static void __init add_keypad(void)
{
	platform_device_register(&sprd_keypad_device);
}

static struct htc_sprd_led_config pm_led_config[] = {
	{
		.name = "button-backlight",
		.init_pwm_brightness = 70,
		.flag = FIX_BRIGHTNESS,
	},
	{
		.name = "green",
		.init_pwm_brightness = 255,
		.flag = DYNAMIC_BRIGHTNESS,
	},
	{
		.name = "amber",
		.init_pwm_brightness = 240,
		.flag = DYNAMIC_BRIGHTNESS,
	}
};

static __init void init_HW_CHG_LED(void)
{
	gpio_request(GPIO_HW_CHG_LED_OFF, "HW_CHG_LED_OFF");
	gpio_direction_output(GPIO_HW_CHG_LED_OFF, 1);
}

static int led_board_revision(void)
{
	int led_version = 0;
	int pcbid = cp5dug_get_board_revision();
	int bomid = htc_get_bomid();

	if (pcbid >= BOARD_EVT_XB)
		led_version = 1;

	pr_info("led_board_revision:pcbid(0x%X),bomid(0x%X),led_version(%d)\n",pcbid ,bomid ,led_version);

	return led_version;
}

static struct htc_sprd_led_platform_data htc_sprd_leds_data = {
	.led_config = pm_led_config,
	.num_leds = ARRAY_SIZE(pm_led_config),
	.led_get_board_version = led_board_revision,
};

static struct platform_device htc_sprd_leds_device = {
	.name   = "leds-htc-sprd",
	.id     = -1,
	.dev    = {
		.platform_data  = &htc_sprd_leds_data,
	},
};

/* HTC_HEADSET_GPIO Driver */
/* HTC_HEADSET_ONE_WIRE Driver */
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
static struct htc_headset_1wire_platform_data htc_headset_1wire_data = {
	.tx_level_shift_en	= GPIO_AUD_UART_OEz,
	.uart_sw		= 0,/* GPIO_AUD_UART_SEL */
	.remote_press	    	= 0,
	.one_wire_remote    	= {0x7E, 0x7F, 0x7D, 0x7F, 0x7B, 0x7F},
	.onewire_tty_dev	= "/dev/ttyS2",
};

static struct platform_device htc_headset_one_wire = {
	.name	= "HTC_HEADSET_1WIRE",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_1wire_data,
	},
};
#endif

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_UNPLUG, 
		.adc_max = 4096,
		.adc_min = 3646,
	},
	{
		.type = HEADSET_MIC,
		.adc_max = 3645,
		.adc_min = 3005,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 3004,
		.adc_min = 2213,
	},
	{
		.type = HEADSET_BEATS_SOLO,
		.adc_max = 2212,
		.adc_min = 1367,
	},
	{
		.type = HEADSET_INDICATOR,
		.adc_max = 1366,
		.adc_min = 561,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 560,
		.adc_min = 0,
	},
};

/* HTC_HEADSET_PMIC Driver */
static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag	= 0,
	.hpin_gpio		= GPIO_AUD_HP_INz,
	.hpin_irq		= 0,
	.key_gpio		= GPIO_AUD_REMO_PRESz,
	.key_irq		= 0,
	.key_enable_gpio= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 164, 165, 383, 384,820},
	//.adc_mpp		= PM8XXX_AMUX_MPP_11,
	//.adc_amux		= ADC_MPP_1_AMUX6,
	.hs_controller	= 0,
	.hs_switch		= 0,
};

static struct platform_device htc_headset_pmic = {
	.name	= "HTC_HEADSET_PMIC",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_pmic_data,
	},
};

#ifdef CONFIG_HTC_HEADSET_ONE_WIRE

typedef struct {
	uint32_t reg;
	uint32_t val;
} pinmap_t;

static pinmap_t pinmap_uart_tx_gpo[] = {
	{REG_PIN_U2TXD,               BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_U2TXD,               BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_U2TXD,               BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
};

static void uart_tx_gpo(int mode)
{
	__raw_writel(pinmap_uart_tx_gpo[mode].val, CTL_PIN_BASE + pinmap_uart_tx_gpo[mode].reg);
	switch (mode) {
		case 0:
			gpio_request(GPIO_CPU_1WIRE_TX, "HS_TX_GPO");
			gpio_direction_output(GPIO_CPU_1WIRE_TX, 0);
			break;
		case 1:
			gpio_request(GPIO_CPU_1WIRE_TX, "HS_TX_GPO");
			gpio_direction_output(GPIO_CPU_1WIRE_TX, 1);
			break;
		case 2:
			gpio_request(GPIO_CPU_1WIRE_TX, "HS_TX_UART");
			break;
	}
}

static void uart_lv_shift_en(int enable)
{
	gpio_set_value_cansleep(GPIO_AUD_UART_OEz, enable);
	pr_info("[HS_BOARD]level shift %d\n", enable);
}
#endif
static void headset_power(int hs_enable)
{
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
	gpio_direction_output(GPIO_AUD_UART_OEz, 1);
#endif
	gpio_direction_output(GPIO_AUD_2V85_EN, hs_enable);
}

static void headset_init(void)
{
	int ret;
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
	unsigned int reg_val = 0;

	pr_info("Headset 1-wire init");
	ret = gpio_request(GPIO_AUD_UART_OEz, "HS_LVL_SHF");
	if (ret < 0) {
			pr_err("[HS] %s: ERROR: Unable to get H6SDB GPIO %d \n",
					__func__, GPIO_AUD_UART_OEz);
		}

	// Enable UART2 in PIN_CTRL_REG2
	pr_info("[hs_1wire](%s):PIN_CTRL_REG2[0x%X] = 0x%X ->",
		__func__, (CTL_PIN_BASE+REG_PIN_CTRL2), __raw_readl(CTL_PIN_BASE+REG_PIN_CTRL2));
	reg_val = __raw_readl(CTL_PIN_BASE+REG_PIN_CTRL2);
	reg_val &= ~(0x07<<10);
	__raw_writel(reg_val, (CTL_PIN_BASE+REG_PIN_CTRL2));
	pr_info("[hs_1wire]PIN_CTRL_REG2[0x%X] = 0x%X\n",
		(CTL_PIN_BASE+REG_PIN_CTRL2), __raw_readl(CTL_PIN_BASE+REG_PIN_CTRL2));

	// Enable UART2 in APB_CTRL_REG
	pr_info("[hs_1wire](%s):APB_CTRL_REG[0x%X] = 0x%X ->",
		__func__, SPRD_APBREG_BASE, __raw_readl(SPRD_APBREG_BASE));
	reg_val = __raw_readl(SPRD_APBREG_BASE);
	reg_val |= 0x01<<15;
	__raw_writel(reg_val, SPRD_APBREG_BASE);
	pr_info("[hs_1wire]APB_CTRL_REG[0x%X] = 0x%X\n",
		SPRD_APBREG_BASE, __raw_readl(SPRD_APBREG_BASE));

#if 0
	ret = gpio_request(GPIO_AUD_UART_SEL, "UART_SWI");
	if (ret < 0) {
			pr_err("[HS] %s: ERROR: Unable to get H6SDB GPIO %d \n",
					__func__, GPIO_AUD_UART_SEL);
		}
#endif

	gpio_direction_output(GPIO_AUD_UART_OEz, 1); /*Disable 1-wire level shift by default*/
	/* gpio_direction_output(GPIO_AUD_UART_SEL, 0); */
#endif

	ret = gpio_request(GPIO_AUD_2V85_EN, "HS_MIC_BIAS_SW");
	if (ret < 0) {
			pr_err("[HS] %s: ERROR: Unable to get H6SAB GPIO %d \n",
					__func__, GPIO_AUD_2V85_EN);
	}
	gpio_direction_output(GPIO_AUD_2V85_EN, 0);

}
/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	//&htc_headset_gpio,
	&htc_headset_pmic,
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
	&htc_headset_one_wire,
#endif
	/* Please put the headset detection driver on the last */
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag			= DRIVER_HS_MGR_FLOAT_DET,
	.headset_devices_num		= ARRAY_SIZE(headset_devices),
	.headset_devices		= headset_devices,
	.headset_config_num		= ARRAY_SIZE(htc_headset_mgr_config),
	.headset_config			= htc_headset_mgr_config,
	.headset_power			= headset_power,
	.headset_init			= headset_init,
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
	.uart_tx_gpo			= uart_tx_gpo,
	.uart_lv_shift_en		= uart_lv_shift_en,
#endif
};
static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
			.platform_data	= &htc_headset_mgr_data,
		},
};

#ifdef CONFIG_CABLE_DETECT_ACCESSORY
#define CP5DUG_GPIO_USB_ID_PIN	102

void config_cp5dug_usb_id_gpios(bool output)
{
	gpio_request(CP5DUG_GPIO_USB_ID_PIN, "cable_detect");
	if (output) {
		gpio_direction_output(CP5DUG_GPIO_USB_ID_PIN, 1);
		gpio_set_value(CP5DUG_GPIO_USB_ID_PIN, 1);
		pr_info("[CABLE] %s %d output high\n",
			__func__, CP5DUG_GPIO_USB_ID_PIN);
	} else {
		gpio_direction_input(CP5DUG_GPIO_USB_ID_PIN);
		pr_info("[CABLE] %s %d input none pull\n",
			__func__, CP5DUG_GPIO_USB_ID_PIN);
	}
}

#define CP5DUG_GPIO_USB_HOST_ENABLE_PIN        222

void cp5dug_enable_host_mode(bool enable)
{
	gpio_request(CP5DUG_GPIO_USB_HOST_ENABLE_PIN, "cable_detect");
	if (enable) {
		gpio_direction_output(CP5DUG_GPIO_USB_HOST_ENABLE_PIN, 1);
		gpio_set_value(CP5DUG_GPIO_USB_HOST_ENABLE_PIN, 1);
		pr_info("[CABLE] %s %d output high\n",
				__func__, CP5DUG_GPIO_USB_HOST_ENABLE_PIN);
	} else {
		gpio_direction_output(CP5DUG_GPIO_USB_HOST_ENABLE_PIN, 0);
		gpio_set_value(CP5DUG_GPIO_USB_HOST_ENABLE_PIN, 0);
		pr_info("[CABLE] %s %d output low\n",
				__func__, CP5DUG_GPIO_USB_HOST_ENABLE_PIN);
	}
}


static int64_t cp5dug_get_usbid_adc(void)
{
	return sci_adc_get_value(ADC_CHANNEL_2, true);
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type            = CABLE_TYPE_AB8500,
	.usb_id_pin_gpio        = CP5DUG_GPIO_USB_ID_PIN,
	.config_usb_id_gpios    = config_cp5dug_usb_id_gpios,
	.get_adc_cb             = cp5dug_get_usbid_adc,
	//.enable_host_mode	= cp5dug_enable_host_mode,
};

static struct platform_device cable_detect_device = {
	.name   = "cable_detect",
	.id     = -1,
	.dev    = {
		.platform_data = &cable_detect_pdata,
	},
};
#endif

#ifdef CONFIG_FLASHLIGHT_TPS61310
#include <linux/htc_flashlight.h>
#ifdef CONFIG_MSM_CAMERA_FLASH
static int flashlight_control(int mode)
{
	return tps61310_flashlight_control(mode);
}
#endif
static void config_flashlight_gpios(void);
static struct TPS61310_flashlight_platform_data tiger_flashlight_data = {
	.gpio_init = config_flashlight_gpios,
	.tps61310_strb0 = GPIO_FLASH_EN,
	.tps61310_strb1 = GPIO_TORCH_EN,
	.flash_duration_ms = 600,
	.led_count = 1,
	.mode_pin_suspend_state_low = 1,
	//.Tx_mask = 131,
};

//HTC_CAM_START chuck config the IP_GPIO for accessied GPIO for ISP_FW
static void config_flashlight_gpios(void){
        gpio_request(GPIO_FLASH_EN,"flash_en");
        gpio_direction_output(GPIO_FLASH_EN, 0);
        gpio_request(GPIO_TORCH_EN,"torch_en");
        gpio_direction_output(GPIO_TORCH_EN, 0);
}
//HTC_CAM_END
#endif

// HTC_AUD_ADD	Yangyi
#ifdef CONFIG_AMP_RT5501
struct rt5501_platform_data rt5501_data={
         .gpio_rt5501_hp_en = (112),
};
#endif
// HTC_AUD_END

#ifdef CONFIG_BT
/* BT suspend/resume */
static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= GPIO_BT2AP_WAKE,
		.end	= GPIO_BT2AP_WAKE,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= GPIO_AP2BT_WAKE,
		.end	= GPIO_AP2BT_WAKE,
		.flags	= IORESOURCE_IO,
	},
};

static struct platform_device brcm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

static struct resource rfkill_resources[] = {
	{
		.name   = "bt_power",
		.start  = GPIO_BT_POWER,
		.end    = GPIO_BT_POWER,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "bt_reset",
		.start  = GPIO_BT_RESET,
		.end    = GPIO_BT_RESET,
		.flags  = IORESOURCE_IO,
	},
};
static struct platform_device rfkill_device = {
	.name = "rfkill",
	.id = -1,
	.num_resources	= ARRAY_SIZE(rfkill_resources),
	.resource	= rfkill_resources,
};
#endif

/* keypad backlight */
static struct platform_device kb_backlight_device = {
	.name           = "keyboard-backlight",
	.id             =  -1,
};

static struct sys_timer __timer = {
	.init = sci_timer_init,
};

static int calibration_mode = false;
static int __init calibration_start(char *str)
{
	int calibration_device =0;
	int mode=0,freq=0,device=0;
	if(str){
		pr_info("modem calibartion:%s\n", str);
		sscanf(str, "%d,%d,%d", &mode,&freq,&device);
	}
	if(device & 0x80){
		calibration_device = device & 0xf0;
		calibration_mode = true;
		pr_info("calibration device = 0x%x\n",calibration_device);
	}
	return 1;
}
__setup("calibration=", calibration_start);

int in_calibration(void)
{
	return (int)(calibration_mode == true);
}

EXPORT_SYMBOL(in_calibration);

static void __init sprd_add_otg_device(void)
{
	/*
	 * if in calibrtaion mode, we do nothing, modem will handle everything
	 */
	platform_device_register(&sprd_otg_device);
}

static struct serial_data plat_data0 = {
	.wakeup_type = BT_RTS_HIGH_WHEN_SLEEP,
	.clk = 48000000,
};
static struct serial_data plat_data1 = {
	.wakeup_type = BT_RTS_HIGH_WHEN_SLEEP,
	.clk = 26000000,
};
static struct serial_data plat_data2 = {
	.wakeup_type = BT_RTS_HIGH_WHEN_SLEEP,
	.clk = 26000000,
};
static struct serial_data plat_data3 = {
	.wakeup_type = BT_RTS_HIGH_WHEN_SLEEP,
	.clk = 26000000,
};

#ifdef CONFIG_VIA_MODEM
static struct serial_data plat_data4 = {
	.wakeup_type = BT_RTS_HIGH_WHEN_SLEEP,
	.clk = 26000000,
};
#endif

static struct ft5x0x_ts_platform_data ft5x0x_ts_info = {
	.irq_gpio_number	= GPIO_TOUCH_IRQ,
	.reset_gpio_number	= GPIO_TOUCH_RESET,
	.vdd_name 			= "vdd28",
};

/* TODO: mpu sensor drivers not merged */
#if 0
static struct lis3dh_acc_platform_data lis3dh_plat_data = {
	.poll_interval = 10,
	.min_interval = 10,
	.g_range = LIS3DH_ACC_G_2G,
	.axis_map_x = 1,
	.axis_map_y = 0,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 1
};

struct akm8975_platform_data akm8975_platform_d = {
	.mag_low_x = -20480,
	.mag_high_x = 20479,
	.mag_low_y = -20480,
	.mag_high_y = 20479,
	.mag_low_z = -20480,
	.mag_high_z = 20479,
};

static struct mpu_platform_data mpu9150_platform_data = {
	.int_config = 0x00,
	.level_shifter = 0,
	.orientation = { -1, 0, 0,
					  0, -1, 0,
					  0, 0, +1 },
	.sec_slave_type = SECONDARY_SLAVE_TYPE_COMPASS,
	.sec_slave_id = COMPASS_ID_AK8963,
	.secondary_i2c_addr = 0x0C,
	.secondary_orientation = { 0, -1, 0,
					1, 0, 0,
					0, 0, 1 },
	.key = {0xec, 0x06, 0x17, 0xdf, 0x77, 0xfc, 0xe6, 0xac,
			0x7b, 0x6f, 0x12, 0x8a, 0x1d, 0x63, 0x67, 0x37},
};


static struct i2c_board_info i2c2_boardinfo[] = {
	{ I2C_BOARD_INFO(LIS3DH_ACC_I2C_NAME, LIS3DH_ACC_I2C_ADDR),
	  .platform_data = &lis3dh_plat_data,
	},
	{ I2C_BOARD_INFO("mpu9150", 0x68),
	  .irq = MPUIRQ_GPIO,
	  .platform_data = &mpu9150_platform_data,
	},
	{ I2C_BOARD_INFO(LTR558_I2C_NAME,  LTR558_I2C_ADDR),
	  .platform_data = &ltr558_pls_info,
	},
/*	{ I2C_BOARD_INFO(AKM8975_I2C_NAME,    AKM8975_I2C_ADDR),
	  .platform_data = &akm8975_platform_d,
	},*/
};
#endif

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
#define SEMI_DLX_1_SENSOR_ID  		0x10008
#define SEMI_DLX_2_SENSOR_ID		0x10088
#define JTOUCH_SENSOR_ID			0x10080
#define PANASONIC_SENDOR_ID			0x10000


static ssize_t syn_vkeys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	int id = get_tp_sensor_id();
	printk(KERN_INFO "[TP]: %s: enter. after  get_tp_sensor_id  is=%x \n", __func__, id);

	if(  SEMI_DLX_1_SENSOR_ID == id || SEMI_DLX_2_SENSOR_ID == id ){
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":120:960:90:90"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":300:960:90:90"
			":" __stringify(EV_KEY) ":" __stringify(KEY_APP_SWITCH) ":480:960:90:90"
			"\n");
		}
	else if(PANASONIC_SENDOR_ID == id || JTOUCH_SENSOR_ID == id ){
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":128:1015:100:100"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":415:1015:100:100"
			"\n");
		}
	else{
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":128:1015:100:100"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":415:1015:100:100"
			"\n");

		}

}

static struct kobj_attribute syn_vkeys_attr = {
	.attr = {
		.name = "virtualkeys.synaptics-rmi-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &syn_vkeys_show,
};

static struct attribute *syn_properties_attrs[] = {
	&syn_vkeys_attr.attr,
	NULL
};

static struct attribute_group syn_properties_attr_group = {
	.attrs = syn_properties_attrs,
};

static void syn_init_vkeys(void)
{
	int rc;
	static struct kobject *syn_properties_kobj;

	syn_properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (syn_properties_kobj)
		rc = sysfs_create_group(syn_properties_kobj, &syn_properties_attr_group);
	if (!syn_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);

	return;
}

int touch_power(int on)
{
	int ret = 0;

	printk(KERN_INFO "[TP]: %s: enter. on:0x%x", __func__, on);

	ret = gpio_request(GPIO_V_LED_3V3EN, "touch power");

	if (on){
		gpio_direction_output(GPIO_V_LED_3V3EN, 1);
		gpio_set_value(GPIO_V_LED_3V3EN, 1);

		ret = gpio_request(GPIO_TP_RSTz, "touch reset");
		gpio_direction_output(GPIO_TP_RSTz, 0);

		ret = gpio_request(GPIO_TP_ATTz, "touch interrupt");
		gpio_direction_input(GPIO_TP_ATTz);

		gpio_set_value(GPIO_TP_RSTz, 0);
		hr_msleep(10);
		gpio_set_value(GPIO_TP_RSTz, 1);
		gpio_free(GPIO_TP_RSTz);
		hr_msleep(70);
	}else{
		gpio_direction_output(GPIO_V_LED_3V3EN, 0);
		gpio_set_value(GPIO_V_LED_3V3EN, 0);
	}
	gpio_free(GPIO_V_LED_3V3EN);

	return 0;
}

static struct synaptics_i2c_rmi_platform_data syn_ts_3k_data[] = {
	{	.packrat_number = 1471960,
		.abs_x_min = 15,
		.abs_x_max = 1605,
		.abs_y_min = 15,
		.abs_y_max = 2743,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x00,  //CP5 Panasonic sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x43,	0x50,	0x56,	0x37,	0x00,	0x7F,	0x03,	0x1E,
					0x05,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x1E,	0x05,	0x50,	0x18,	0x32,
					0x3E,	0x02,	0x01,	0x19,	0x03,	0x19,	0x02,	0x48,
					0x43,	0x00,	0x48,	0xB7,	0xDB,	0x96,	0xCF,	0x01,
					0xA0,	0x00,	0x00,	0x00,	0x00,	0x0A,	0x04,	0xC0,
					0x1A,	0x0F,	0x0A,	0x96,	0x07,	0xF3,	0xC8,	0xBE,
					0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,	0x4C,
					0x6C,	0x74,	0x3C,	0x32,	0x00,	0x00,	0x00,	0x4C,
					0x6C,	0x74,	0x1E,	0x05,	0x00,	0x02,	0xFA,	0x00,
					0x80,	0x03,	0x0E,	0x1F,	0x19,	0x38,	0x00,	0x13,
					0x04,	0x1B,	0x00,	0x10,	0x0A,	0x80,	0x80,	0x88,
					0x68,	0x60,	0x60,	0x68,	0x68,	0x3B,	0x38,	0x36,
					0x33,	0x30,	0x2D,	0x2A,	0x28,	0x00,	0x00,	0x00,
					0x01,	0x06,	0x0B,	0x10,	0x15,	0x00,	0x88,	0x13,
					0x00,	0x64,	0x00,	0xC8,	0x00,	0x80,	0x0A,	0xA6,
					0x88,	0x13,	0x00,	0xC0,	0x80,	0x02,	0x02,	0x02,
					0x02,	0x02,	0x02,	0x02,	0x02,	0x20,	0x20,	0x20,
					0x20,	0x20,	0x10,	0x10,	0x10,	0x58,	0x5E,	0x63,
					0x68,	0x6F,	0x3B,	0x3F,	0x43,	0x00,	0xA0,	0x00,
					0x10,	0x28,	0x00,	0x00,	0x00,	0x05,	0x0A,	0x0E,
					0x10,	0x12,	0x15,	0x18,	0x00,	0x31,	0x04,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0x51,	0x51,	0x51,	0x51,
					0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,	0x00,
					0x06,	0x0D,	0x15,	0x16,	0x19,	0x1B,	0x04,	0x03,
					0x01,	0x11,	0x12,	0x0E,	0x0A,	0x0C,	0x0B,	0x17,
					0x18,	0x1A,	0x05,	0x02,	0x00,	0x14,	0x0F,	0x09,
					0x08,	0x07,	0xFF,	0x0D,	0x12,	0x10,	0x0F,	0x0E,
					0x08,	0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,	0x04,
					0x0A,	0x0B,	0xFF,	0x00,	0x10,	0x00,	0x10,	0x00,
					0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0xA6,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,	0x0F,
					0x00,	0x02,	0x36,	0x3F,	0x04,	0xA4,	0x10,	0x14,
					0x0A
		},
	},
	{	.packrat_number = 1471960,
		.abs_x_min = 15,
		.abs_x_max = 1605,
		.abs_y_min = 15,
		.abs_y_max = 2743,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x80, //Jtouch sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x43,	0x50,	0x56,	0x37,	0x00,	0x7F,	0x03,	0x1E,
					0x05,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x1E,	0x05,	0x50,	0x18,	0x32,
					0x3E,	0x02,	0x01,	0x19,	0x03,	0x19,	0x02,	0x48,
					0x43,	0x00,	0x48,	0xB7,	0xDB,	0x96,	0xCF,	0x01,
					0xA0,	0x00,	0x00,	0x00,	0x00,	0x0A,	0x04,	0xC0,
					0x1A,	0x0F,	0x0A,	0x96,	0x07,	0xF3,	0xC8,	0xBE,
					0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,	0x4C,
					0x6C,	0x74,	0x3C,	0x32,	0x00,	0x00,	0x00,	0x4C,
					0x6C,	0x74,	0x1E,	0x05,	0x00,	0x02,	0xFA,	0x00,
					0x80,	0x03,	0x0E,	0x1F,	0x19,	0x38,	0x00,	0x13,
					0x04,	0x1B,	0x00,	0x10,	0x0A,	0x80,	0x80,	0x88,
					0x68,	0x60,	0x60,	0x68,	0x68,	0x3B,	0x38,	0x36,
					0x33,	0x30,	0x2D,	0x2A,	0x28,	0x00,	0x00,	0x00,
					0x01,	0x06,	0x0B,	0x10,	0x15,	0x00,	0x88,	0x13,
					0x00,	0x64,	0x00,	0xC8,	0x00,	0x80,	0x0A,	0xA6,
					0x88,	0x13,	0x00,	0xC0,	0x80,	0x02,	0x02,	0x02,
					0x02,	0x02,	0x02,	0x02,	0x02,	0x20,	0x20,	0x20,
					0x20,	0x20,	0x10,	0x10,	0x10,	0x58,	0x5E,	0x63,
					0x68,	0x6F,	0x3B,	0x3F,	0x43,	0x00,	0xA0,	0x00,
					0x10,	0x28,	0x00,	0x00,	0x00,	0x05,	0x0A,	0x0E,
					0x10,	0x12,	0x15,	0x18,	0x00,	0x31,	0x04,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0x51,	0x51,	0x51,	0x51,
					0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,	0x00,
					0x06,	0x0D,	0x15,	0x16,	0x19,	0x1B,	0x04,	0x03,
					0x01,	0x11,	0x12,	0x0E,	0x0A,	0x0C,	0x0B,	0x17,
					0x18,	0x1A,	0x05,	0x02,	0x00,	0x14,	0x0F,	0x09,
					0x08,	0x07,	0xFF,	0x0D,	0x12,	0x10,	0x0F,	0x0E,
					0x08,	0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,	0x04,
					0x0A,	0x0B,	0xFF,	0x00,	0x10,	0x00,	0x10,	0x00,
					0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0xA6,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,	0x0F,
					0x00,	0x02,	0x36,	0x3F,	0x04,	0xA4,	0x10,	0x14,
					0x0A
		},
	},
	{	.packrat_number = 1471960,
		.abs_x_min = 0,
		.abs_x_max = 1600,
		.abs_y_min = 0,
		.abs_y_max = 2710,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x08 , //DLX sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x33,	0x33,	0x00,	0x08,	0x04,	0x7F,	0x03,	0x1E,
					0x05,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x1E,	0x05,	0x50,	0x18,	0x28,
					0x1E,	0x03,	0x01,	0x1B,	0x01,	0x1A,	0x01,	0x14,
					0x4E,	0x3D,	0x52,	0xC4,	0xB6,	0xC5,	0xC7,	0x00,
					0xC0,	0x00,	0x00,	0x00,	0x00,	0x0A,	0x04,	0xB7,
					0x17,	0x0D,	0x0A,	0x78,	0x07,	0xF6,	0xC8,	0xC0,
					0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,	0x54,
					0x40,	0xB6,	0x3C,	0x32,	0x00,	0x00,	0x00,	0x54,
					0x40,	0xB6,	0x1E,	0x05,	0x00,	0x02,	0xFA,	0x00,
					0x80,	0x03,	0x0E,	0x1F,	0x11,	0x50,	0x00,	0x14,
					0x04,	0x1B,	0x00,	0x10,	0x0A,	0xC0,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x47,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,
					0x00,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0x00,
					0xFF,	0xFF,	0x00,	0xC0,	0x19,	0x05,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x10,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x68,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,
					0x10,	0x28,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x31,	0x04,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0x51,	0x51,	0x51,	0x51,
					0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,	0x00,
					0x11,	0x14,	0x12,	0x0F,	0x0E,	0x09,	0x0A,	0x07,
					0x02,	0x01,	0x00,	0x03,	0x08,	0x0C,	0x0D,	0x0B,
					0x15,	0x17,	0x16,	0x18,	0x19,	0x1A,	0x1B,	0xFF,
					0xFF,	0xFF,	0xFF,	0x12,	0x0F,	0x10,	0x0E,	0x08,
					0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,	0x04,	0x0A,
					0xFF,	0xFF,	0xFF,	0x00,	0x10,	0x00,	0x10,	0x00,
					0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,	0x0F,
					0x00,	0x02,	0x36,	0x44,	0x04,	0x84,	0x10,	0x14,
					0x42
		},
	},
	{	.packrat_number = 1471960,
		.abs_x_min = 0,
		.abs_x_max = 1600,
		.abs_y_min = 0,
		.abs_y_max = 2710,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x88, //DLX sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x33,	0x33,	0x00,	0x08,	0x04,	0x7F,	0x03,	0x1E,
					0x05,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x1E,	0x05,	0x50,	0x18,	0x28,
					0x1E,	0x03,	0x01,	0x1B,	0x01,	0x1A,	0x01,	0x14,
					0x4E,	0x3D,	0x52,	0xC4,	0xB6,	0xC5,	0xC7,	0x00,
					0xC0,	0x00,	0x00,	0x00,	0x00,	0x0A,	0x04,	0xB7,
					0x17,	0x0D,	0x0A,	0x78,	0x07,	0xF6,	0xC8,	0xC0,
					0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,	0x54,
					0x40,	0xB6,	0x3C,	0x32,	0x00,	0x00,	0x00,	0x54,
					0x40,	0xB6,	0x1E,	0x05,	0x00,	0x02,	0xFA,	0x00,
					0x80,	0x03,	0x0E,	0x1F,	0x11,	0x50,	0x00,	0x14,
					0x04,	0x1B,	0x00,	0x10,	0x0A,	0xC0,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x47,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,
					0x00,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0x00,
					0xFF,	0xFF,	0x00,	0xC0,	0x19,	0x05,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x10,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x68,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,
					0x10,	0x28,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x31,	0x04,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0x51,	0x51,	0x51,	0x51,
					0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,	0x00,
					0x11,	0x14,	0x12,	0x0F,	0x0E,	0x09,	0x0A,	0x07,
					0x02,	0x01,	0x00,	0x03,	0x08,	0x0C,	0x0D,	0x0B,
					0x15,	0x17,	0x16,	0x18,	0x19,	0x1A,	0x1B,	0xFF,
					0xFF,	0xFF,	0xFF,	0x12,	0x0F,	0x10,	0x0E,	0x08,
					0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,	0x04,	0x0A,
					0xFF,	0xFF,	0xFF,	0x00,	0x10,	0x00,	0x10,	0x00,
					0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,	0x0F,
					0x00,	0x02,	0x36,	0x44,	0x04,	0x84,	0x10,	0x14,
					0x42
		},
	},
	{	.packrat_number = 1293981,
		.abs_x_min = 0,
		.abs_x_max = 1600,
		.abs_y_min = 0,
		.abs_y_max = 2710,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x00,  //CP5 Panasonic sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x43,	0x50,	0x56,	0x31,	0x04,	0x7F,	0x03,	0x1E,
					0x14,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x02,	0x14,	0x1E,	0x05,	0x4B,
					0x26,	0x2E,	0x6F,	0x01,	0x01,	0x3C,	0x0C,	0x03,
					0x10,	0x03,	0x29,	0x44,	0xC3,	0x45,	0x50,	0xC3,
					0x50,	0xC3,	0x00,	0xA0,	0x00,	0x00,	0x00,	0x00,
					0x0A,	0x04,	0xC0,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x19,	0x01,	0x00,	0x0A,	0x1A,	0x0F,	0x0A,
					0x00,	0x14,	0x0A,	0x40,	0x96,	0x07,	0xF3,	0xC8,
					0xBE,	0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,
					0x4C,	0x6C,	0x74,	0x3C,	0x32,	0x00,	0x00,	0x00,
					0x4C,	0x6C,	0x74,	0x1E,	0x05,	0x00,	0x02,	0x0E,
					0x01,	0x80,	0x03,	0x0E,	0x1F,	0x11,	0x38,	0x00,
					0x13,	0x04,	0x1B,	0x00,	0x10,	0x0A,	0x80,	0x68,
					0x60,	0x68,	0x68,	0x40,	0x48,	0x40,	0x35,	0x33,
					0x30,	0x2D,	0x2A,	0x27,	0x24,	0x22,	0x00,	0x00,
					0x00,	0x03,	0x08,	0x0D,	0x14,	0x1B,	0x00,	0x88,
					0x13,	0x00,	0x64,	0x00,	0xC8,	0x00,	0x80,	0x0A,
					0xCD,	0x88,	0x13,	0x00,	0xC0,	0x19,	0x02,	0x02,
					0x02,	0x02,	0x02,	0x02,	0x03,	0x02,	0x20,	0x20,
					0x20,	0x20,	0x10,	0x10,	0x20,	0x10,	0x58,	0x5E,
					0x64,	0x6A,	0x39,	0x3D,	0x58,	0x47,	0x00,	0x8C,
					0x00,	0x10,	0x28,	0x00,	0x00,	0x00,	0x05,	0x0B,
					0x0E,	0x11,	0x14,	0x17,	0x1A,	0x00,	0x31,	0x04,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0x9A,	0x9A,	0x51,
					0x51,	0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,
					0x00,	0x06,	0x0D,	0x15,	0x16,	0x19,	0x1B,	0x04,
					0x03,	0x01,	0x11,	0x12,	0x0E,	0x0A,	0x0C,	0x0B,
					0x17,	0x18,	0x1A,	0x05,	0x02,	0x00,	0x14,	0x0F,
					0x09,	0x08,	0x07,	0xFF,	0x0D,	0x12,	0x10,	0x0F,
					0x0E,	0x08,	0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,
					0x04,	0x0A,	0x0B,	0xFF,	0x00,	0x10,	0x00,	0x10,
					0x00,	0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,
					0x0F,	0x00,	0x4F,	0x53
		},
	},
	{	.packrat_number = 1293981,
		.abs_x_min = 0,
		.abs_x_max = 1600,
		.abs_y_min = 0,
		.abs_y_max = 2710,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x80, //Jtouch sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x43,	0x50,	0x56,	0x31,	0x04,	0x7F,	0x03,	0x1E,
					0x14,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x02,	0x14,	0x1E,	0x05,	0x4B,
					0x26,	0x2E,	0x6F,	0x01,	0x01,	0x3C,	0x0C,	0x03,
					0x10,	0x03,	0x29,	0x44,	0xC3,	0x45,	0x50,	0xC3,
					0x50,	0xC3,	0x00,	0xA0,	0x00,	0x00,	0x00,	0x00,
					0x0A,	0x04,	0xC0,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x19,	0x01,	0x00,	0x0A,	0x1A,	0x0F,	0x0A,
					0x00,	0x14,	0x0A,	0x40,	0x96,	0x07,	0xF3,	0xC8,
					0xBE,	0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,
					0x4C,	0x6C,	0x74,	0x3C,	0x32,	0x00,	0x00,	0x00,
					0x4C,	0x6C,	0x74,	0x1E,	0x05,	0x00,	0x02,	0x0E,
					0x01,	0x80,	0x03,	0x0E,	0x1F,	0x11,	0x38,	0x00,
					0x13,	0x04,	0x1B,	0x00,	0x10,	0x0A,	0x80,	0x68,
					0x60,	0x68,	0x68,	0x40,	0x48,	0x40,	0x35,	0x33,
					0x30,	0x2D,	0x2A,	0x27,	0x24,	0x22,	0x00,	0x00,
					0x00,	0x03,	0x08,	0x0D,	0x14,	0x1B,	0x00,	0x88,
					0x13,	0x00,	0x64,	0x00,	0xC8,	0x00,	0x80,	0x0A,
					0xCD,	0x88,	0x13,	0x00,	0xC0,	0x19,	0x02,	0x02,
					0x02,	0x02,	0x02,	0x02,	0x03,	0x02,	0x20,	0x20,
					0x20,	0x20,	0x10,	0x10,	0x20,	0x10,	0x58,	0x5E,
					0x64,	0x6A,	0x39,	0x3D,	0x58,	0x47,	0x00,	0x8C,
					0x00,	0x10,	0x28,	0x00,	0x00,	0x00,	0x05,	0x0B,
					0x0E,	0x11,	0x14,	0x17,	0x1A,	0x00,	0x31,	0x04,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0x9A,	0x9A,	0x51,
					0x51,	0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,
					0x00,	0x06,	0x0D,	0x15,	0x16,	0x19,	0x1B,	0x04,
					0x03,	0x01,	0x11,	0x12,	0x0E,	0x0A,	0x0C,	0x0B,
					0x17,	0x18,	0x1A,	0x05,	0x02,	0x00,	0x14,	0x0F,
					0x09,	0x08,	0x07,	0xFF,	0x0D,	0x12,	0x10,	0x0F,
					0x0E,	0x08,	0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,
					0x04,	0x0A,	0x0B,	0xFF,	0x00,	0x10,	0x00,	0x10,
					0x00,	0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,
					0x0F,	0x00,	0x4F,	0x53
		},
	},
	{	.packrat_number = 1293981,
		.abs_x_min = 0,
		.abs_x_max = 1600,
		.abs_y_min = 0,
		.abs_y_max = 2710,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x08 , //DLX sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x33,	0x32,	0x00,	0x07,	0x00,	0x7F,	0x03,	0x1E,
					0x05,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x02,	0x14,	0x1E,	0x05,	0x50,
					0x18,	0x28,	0x1E,	0x03,	0x01,	0x3C,	0x1B,	0x01,
					0x1A,	0x01,	0x14,	0x4E,	0x3D,	0x52,	0xC4,	0xB6,
					0xC5,	0xC7,	0x00,	0xC0,	0x00,	0x00,	0x00,	0x00,
					0x0A,	0x04,	0xB7,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x19,	0x01,	0x00,	0x0A,	0x17,	0x0D,	0x0A,
					0x00,	0x14,	0x0A,	0x40,	0x78,	0x07,	0xF6,	0xC8,
					0xC0,	0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,
					0x54,	0x40,	0xB6,	0x3C,	0x32,	0x00,	0x00,	0x00,
					0x54,	0x40,	0xB6,	0x1E,	0x05,	0x00,	0x02,	0xFA,
					0x00,	0x80,	0x03,	0x0E,	0x1F,	0x11,	0x50,	0x00,
					0x14,	0x04,	0x1B,	0x00,	0x10,	0x0A,	0x60,	0x60,
					0x60,	0x68,	0x48,	0x48,	0x28,	0x20,	0x2E,	0x2B,
					0x29,	0x28,	0x26,	0x23,	0x21,	0x1F,	0x00,	0x00,
					0x00,	0x00,	0x02,	0x08,	0x0E,	0x17,	0x00,	0x7C,
					0x15,	0x00,	0x64,	0x00,	0xC8,	0x00,	0xCD,	0x0A,
					0xCD,	0x4C,	0x1D,	0x00,	0xC0,	0x19,	0x02,	0x02,
					0x04,	0x02,	0x03,	0x03,	0x03,	0x02,	0x10,	0x10,
					0x30,	0x10,	0x20,	0x20,	0x20,	0x10,	0x39,	0x3D,
					0x60,	0x43,	0x5E,	0x64,	0x6A,	0x56,	0x00,	0x96,
					0x00,	0x10,	0x28,	0x00,	0x00,	0x01,	0x08,	0x0E,
					0x14,	0x19,	0x1C,	0x1F,	0x23,	0x00,	0x31,	0x04,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0x51,	0x51,	0x51,
					0x51,	0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,
					0x00,	0x11,	0x14,	0x12,	0x0F,	0x0E,	0x09,	0x0A,
					0x07,	0x02,	0x01,	0x00,	0x03,	0x08,	0x0C,	0x0D,
					0x0B,	0x15,	0x17,	0x16,	0x18,	0x19,	0x1A,	0x1B,
					0xFF,	0xFF,	0xFF,	0xFF,	0x12,	0x0F,	0x10,	0x0E,
					0x08,	0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,	0x04,
					0x0A,	0xFF,	0xFF,	0xFF,	0x00,	0x10,	0x00,	0x10,
					0x00,	0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x5A,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,
					0x0F,	0x01,	0x4F,	0x53
		},
	},
	{	.packrat_number = 1293981,
		.abs_x_min = 0,
		.abs_x_max = 1600,
		.abs_y_min = 0,
		.abs_y_max = 2710,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = GPIO_TP_ATTz,
		.gpio_reset = GPIO_TP_RSTz,
		.default_config = 1,
		.tw_pin_mask = 0x0088,
		.sensor_id = SENSOR_ID_CHECKING_EN |0x88, //DLX sensor
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.support_htc_event   = 1,
		.i2c_err_handler_en  = 1,
		.reduce_report_level = {60, 60, 50, 0, 0},
		.power = touch_power,
		.config = {	0x33,	0x32,	0x00,	0x07,	0x00,	0x7F,	0x03,	0x1E,
					0x05,	0x09,	0x00,	0x01,	0x01,	0x00,	0x10,	0x54,
					0x06,	0x40,	0x0B,	0x02,	0x14,	0x1E,	0x05,	0x50,
					0x18,	0x28,	0x1E,	0x03,	0x01,	0x3C,	0x1B,	0x01,
					0x1A,	0x01,	0x14,	0x4E,	0x3D,	0x52,	0xC4,	0xB6,
					0xC5,	0xC7,	0x00,	0xC0,	0x00,	0x00,	0x00,	0x00,
					0x0A,	0x04,	0xB7,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x19,	0x01,	0x00,	0x0A,	0x17,	0x0D,	0x0A,
					0x00,	0x14,	0x0A,	0x40,	0x78,	0x07,	0xF6,	0xC8,
					0xC0,	0x43,	0x2A,	0x05,	0x00,	0x00,	0x00,	0x00,
					0x54,	0x40,	0xB6,	0x3C,	0x32,	0x00,	0x00,	0x00,
					0x54,	0x40,	0xB6,	0x1E,	0x05,	0x00,	0x02,	0xFA,
					0x00,	0x80,	0x03,	0x0E,	0x1F,	0x11,	0x50,	0x00,
					0x14,	0x04,	0x1B,	0x00,	0x10,	0x0A,	0x60,	0x60,
					0x60,	0x68,	0x48,	0x48,	0x28,	0x20,	0x2E,	0x2B,
					0x29,	0x28,	0x26,	0x23,	0x21,	0x1F,	0x00,	0x00,
					0x00,	0x00,	0x02,	0x08,	0x0E,	0x17,	0x00,	0x7C,
					0x15,	0x00,	0x64,	0x00,	0xC8,	0x00,	0xCD,	0x0A,
					0xCD,	0x4C,	0x1D,	0x00,	0xC0,	0x19,	0x02,	0x02,
					0x04,	0x02,	0x03,	0x03,	0x03,	0x02,	0x10,	0x10,
					0x30,	0x10,	0x20,	0x20,	0x20,	0x10,	0x39,	0x3D,
					0x60,	0x43,	0x5E,	0x64,	0x6A,	0x56,	0x00,	0x96,
					0x00,	0x10,	0x28,	0x00,	0x00,	0x01,	0x08,	0x0E,
					0x14,	0x19,	0x1C,	0x1F,	0x23,	0x00,	0x31,	0x04,
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
					0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0x51,	0x51,	0x51,
					0x51,	0x51,	0x51,	0x51,	0x51,	0xCD,	0x0D,	0x04,
					0x00,	0x11,	0x14,	0x12,	0x0F,	0x0E,	0x09,	0x0A,
					0x07,	0x02,	0x01,	0x00,	0x03,	0x08,	0x0C,	0x0D,
					0x0B,	0x15,	0x17,	0x16,	0x18,	0x19,	0x1A,	0x1B,
					0xFF,	0xFF,	0xFF,	0xFF,	0x12,	0x0F,	0x10,	0x0E,
					0x08,	0x07,	0x0C,	0x01,	0x06,	0x02,	0x05,	0x04,
					0x0A,	0xFF,	0xFF,	0xFF,	0x00,	0x10,	0x00,	0x10,
					0x00,	0x10,	0x00,	0x10,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x5A,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,
					0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x80,	0x00,
					0x0F,	0x01,	0x4F,	0x53
		},
	},
};
#endif

#if 0
static struct bma250_platform_data gsensor_evm_platform_data =
{
	.intr = SHARK_EVM_GPIO_GSENSOR_INT,
	.chip_layout = 0,
	.layouts = TIGER_EVM_LAYOUTS,
};
static struct bma250_platform_data gsensor_evt_platform_data =
{
	.intr = SHARK_EVM_GPIO_GSENSOR_INT,
	.chip_layout = 1,
	.layouts = TIGER_EVT_LAYOUTS,
};
#endif

static struct bma250_platform_data gsensor_evm_platform_data =
{
    .intr = SHARK_EVM_GPIO_GSENSOR_INT,
    .chip_layout = 1,
    .axis_map_x = 0,
    .axis_map_y = 1,
    .axis_map_z = 2,
    .negate_x = 0,
    .negate_y = 0,
    .negate_z = 0,
};

static uint8_t cm3629_mapping_table[] = {0x0, 0x3, 0x6, 0x9, 0xC,
							   0xF,  0x12, 0x15, 0x18, 0x1B,
                               0x1E, 0x21, 0x24, 0x27, 0x2A,
                               0x2D, 0x30, 0x33, 0x36, 0x39,
                               0x3C, 0x3F, 0x43, 0x47, 0x4B,
                               0x4F, 0x53, 0x57, 0x5B, 0x5F,
                               0x63, 0x67, 0x6B, 0x70, 0x75,
                               0x7A, 0x7F, 0x84, 0x89, 0x8E,
                               0x93, 0x98, 0x9D, 0xA2, 0xA8,
                               0xAE, 0xB4, 0xBA, 0xC0, 0xC6,
                               0xCC, 0xD3, 0xDA, 0xE1, 0xE8,
                               0xEF, 0xF6, 0xFF};

#ifdef CONFIG_INPUT_CAPELLA_CM36282
static int capella_cm36282_power(int pwr_device, uint8_t enable)
{
	return 0;
}
static struct cm3629_platform_data cm36282_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = SHARK_EVM_GPIO_PS_INT,
	.levels = { 1, 9, 18, 133, 316, 1587,
				2537, 5025, 7513, 65535},
	.correction = {100, 400, 900, 1600, 2500, 3600, 4900, 6400, 8100, 10000},
	.ps_th_add = 0x5,
	.golden_adc = 0x71A,
#ifdef CONFIG_WSENSOR_ENABLE
    .w_golden_adc = 0x1AE0,
#endif
	.power = capella_cm36282_power,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 0x15,
	.ps1_thh_diff = 0x0A,
	.ps1_thd_no_cal = 0x90,
	.ps1_thd_with_cal = 0xD,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_80 | CM3629_PS_IT_1_6T |
					CM3629_PS1_PERS_2,
	.ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
    .dark_level = 3,
    .dynamical_threshold = 1,
    .mapping_table = cm3629_mapping_table,
    .mapping_size = ARRAY_SIZE(cm3629_mapping_table),
};
#endif

#ifdef CONFIG_SENSORS_NFC_PN544
static void nfc_gpio_deinit(void)
{
}

#define WITHOUT_NFC_CHIP 0x00039405
#define WITH_NFC_CHIP    0x00039400
extern unsigned htc_get_skuid(void);
static int nfc_init_check(void)
{
	unsigned int htc_skuid = 0;

#if 1
	/* Get HTC skuid */
	htc_skuid = htc_get_skuid();

	if ( WITHOUT_NFC_CHIP == htc_skuid ) {
		printk(KERN_INFO "%s: htc_skuid=[0x%08X], without NFC chip\n", __func__, htc_skuid);
		return 0;
	}
	else if ( WITH_NFC_CHIP == htc_skuid ) {
		printk(KERN_INFO "%s: htc_skuid=[0x%08X], with NFC chip.\n", __func__, htc_skuid);
		return 1;
	}
	else {
		printk(KERN_INFO "%s: htc_skuid=[0x%08X], default without NFC chip.\n", __func__, htc_skuid);
		return 0;
	}
#else
	return 1;
#endif
}

static struct pn544_i2c_platform_data nfc_platform_data = {
	.irq_gpio		= GPIO_NFC_IRQ,
	.ven_gpio	= GPIO_NFC_VEN,
	.firm_gpio 	= GPIO_NFC_DL_MODE,
	.ven_isinvert	= 1,
	.gpio_deinit = nfc_gpio_deinit,
	.check_nfc_exist = nfc_init_check,
};
#endif

#ifdef CONFIG_SENSORS_AKM8963
static struct akm8963_platform_data akm8963_pdata = {
        .layout = 1,
        .outbit = 1,
        .gpio_DRDY = AKM8963_GPIO_INT,
        //.gpio_RST = AKM8963_GPIO_RST,
};
#endif

static struct i2c_board_info i2c2_boardinfo[] = {
#ifdef CONFIG_SENSORS_AKM8963
    {
        I2C_BOARD_INFO(AKM8963_I2C_NAME,0x0D),
        .platform_data = &akm8963_pdata,
    },
#endif
#ifdef CONFIG_SENSORS_NFC_PN544
	{
		I2C_BOARD_INFO(PN544_I2C_NAME,0x50 >> 1),
		.platform_data = &nfc_platform_data,
	},
#endif
#ifdef CONFIG_INPUT_CAPELLA_CM36282
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_pdata,
		/*.irq = GPIO_TO_IRQ(CP2DUG_PS_VOUT),*/
	},
#endif
#ifdef CONFIG_SENSORS_ONLY_BMA250E
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME_REMOVE_ECOMPASS, \
				0x32 >> 1),
		.platform_data = &gsensor_evm_platform_data,
		/*.irq = gpio_to_irq(TIGER_EVM_GPIO_GSENSOR_INT),*/
	},
#endif
#ifdef CONFIG_BOSCH_BMA250
    {
        I2C_BOARD_INFO(SENSOR_NAME,0x32 >> 1),
        .platform_data = &gsensor_evm_platform_data,
        /*.irq = gpio_to_irq(TIGER_EVM_GPIO_GSENSOR_INT),*/
    },
#endif
#ifdef CONFIG_FLASHLIGHT_TPS61310
	{
		I2C_BOARD_INFO("TPS61310_FLASHLIGHT", 0x66 >> 1),
		.platform_data = &tiger_flashlight_data,
	},
#endif
};

static struct i2c_board_info i2c1_boardinfo[] = {
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
	{
		I2C_BOARD_INFO(SYNAPTICS_3200_NAME, 0x40 >> 1),
		.platform_data = &syn_ts_3k_data,
	},
#endif
	//{I2C_BOARD_INFO("sensor_main",0x3C),},
	//{I2C_BOARD_INFO("sensor_sub",0x21),},
};

static struct i2c_board_info i2c0_boardinfo[] = {
	{
		I2C_BOARD_INFO(FT5206_TS_DEVICE, FT5206_TS_ADDR),
		.platform_data = &ft5x0x_ts_info,
	},
	{I2C_BOARD_INFO("sensor_main", 0x3C),},
	{I2C_BOARD_INFO("sensor_sub",   0x21),},

};

static struct tps65200_platform_data tps65200_data = {
	.gpio_chg_stat = GPIO_CHG_STAT,
	.gpio_chg_int = GPIO_CHG_INT,
};

#ifdef CONFIG_SUPPORT_DQ_BATTERY
static int __init check_dq_setup(char *str)
{
	if (!strcmp(str, "PASS"))
		tps65200_data.dq_result = 1;
	else
		tps65200_data.dq_result = 0;
	return 1;
}
__setup("androidboot.dq=", check_dq_setup);
#endif
// HTC_AUD_ADD	Yangyi
static struct i2c_board_info i2c3_boardinfo[] = {
#ifdef CONFIG_AMP_RT5501
	{
		I2C_BOARD_INFO("rt5501", 0xF0 >> 1),
		.platform_data= &rt5501_data,
	},
#endif
#if (defined(CONFIG_SND_AMP_TFA9887))
        {
                I2C_BOARD_INFO("tfa9887", 0x68 >> 1),
                //.platform_data = &cp5_speaker_amp_data,
                .irq = -1,
        },
#endif
#if (defined(CONFIG_SND_AMP_TFA9887L))
        {
                I2C_BOARD_INFO("tfa9887l", 0x6A >> 1),
                //.platform_data = &cp5_speaker_amp_data,
                .irq = -1,
        },
#endif
	{
		I2C_BOARD_INFO("tps65200", 0xD4 >> 1),
		.platform_data = &tps65200_data,
	},
	{
		I2C_BOARD_INFO("pwm_i2c", 0x70 >> 1),
		 .irq = -1,
	},
	{
		I2C_BOARD_INFO("max17050", 0x36),
		.platform_data = NULL,
	},
};
// HTC_AUD_END

static int sc8810_add_i2c_devices(void)
{
	i2c_register_board_info(1, i2c1_boardinfo, ARRAY_SIZE(i2c1_boardinfo));
	i2c_register_board_info(0, i2c0_boardinfo, ARRAY_SIZE(i2c0_boardinfo));
	i2c_register_board_info(2, i2c2_boardinfo, ARRAY_SIZE(i2c2_boardinfo));
	i2c_register_board_info(3, i2c3_boardinfo, ARRAY_SIZE(i2c3_boardinfo));

	return 0;
}

struct platform_device audio_pa_amplifier_device = {
	.name = "speaker-pa",
	.id = -1,
};

static int audio_pa_amplifier_l(u32 cmd, void *data)
{
	int ret = 0;
	if (cmd < 0) {
		/* get speaker amplifier status : enabled or disabled */
		ret = 0;
	} else {
		/* set speaker amplifier */
	}
	return ret;
}

#ifdef RAWCHIP_GPIO_SPI
struct spi_gpio_platform_data spigpio_platform_data = {
	.sck = 70,
	.mosi = 68,
	.miso = 69,
	.num_chipselect = 1,
};

static struct platform_device spigpio_device = {
	.name = "spi_gpio",
	.id   = 6,
	.dev = {
		.platform_data = &spigpio_platform_data,
	},
};

static struct spi_board_info rawchip_spi_board_info[] = {
	{
		.modalias = "spi_rawchip",
		.controller_data = 67,
		.chip_select = 0,
		.bus_num = 6,
		.max_speed_hz = 30 * 1000,
		.mode = SPI_MODE_0,
	},
};
#else
static struct spi_board_info rawchip_spi_board_info[] = {
	{
		.modalias = "spi_rawchip",
		.chip_select = 0,
		.bus_num = 0,
		.max_speed_hz = 24 * 1000 * 1000,
		.mode = SPI_MODE_0,
	},
};
#endif

#if 0
/* Control ldo for maxscend cmmb chip according to HW design */
static struct regulator *cmmb_regulator_1v8 = NULL;

#define SPI_PIN_FUNC_MASK  (0x3<<4)
#define SPI_PIN_FUNC_DEF   (0x0<<4)
#define SPI_PIN_FUNC_GPIO  (0x3<<4)

struct spi_pin_desc {
	const char   *name;
	unsigned int pin_func;
	unsigned int reg;
	unsigned int gpio;
};

static struct spi_pin_desc spi_pin_group[] = {
	{"SPI_DI",  SPI_PIN_FUNC_DEF,  REG_PIN_SPI0_DI   + CTL_PIN_BASE,  158},
	{"SPI_CLK", SPI_PIN_FUNC_DEF,  REG_PIN_SPI0_CLK  + CTL_PIN_BASE,  159},
	{"SPI_DO",  SPI_PIN_FUNC_DEF,  REG_PIN_SPI0_DO   + CTL_PIN_BASE,  157},
	{"SPI_CS0", SPI_PIN_FUNC_GPIO, REG_PIN_SPI0_CSN  + CTL_PIN_BASE,  156}
};
static void sprd_restore_spi_pin_cfg(void)
{
	unsigned int reg;
	unsigned int  gpio;
	unsigned int  pin_func;
	unsigned int value;
	unsigned long flags;
	int i = 0;
	int regs_count = sizeof(spi_pin_group)/sizeof(struct spi_pin_desc);

	for (; i < regs_count; i++) {
	    pin_func = spi_pin_group[i].pin_func;
	    gpio = spi_pin_group[i].gpio;
	    if (pin_func == SPI_PIN_FUNC_DEF) {
		 reg = spi_pin_group[i].reg;
		 /* free the gpios that have request */
		 gpio_free(gpio);
		 local_irq_save(flags);
		 /* config pin default spi function */
		 value = ((__raw_readl(reg) & ~SPI_PIN_FUNC_MASK) | SPI_PIN_FUNC_DEF);
		 __raw_writel(value, reg);
		 local_irq_restore(flags);
	    }
	    else {
		 /* CS should config output */
		 gpio_direction_output(gpio, 1);
	    }
	}

}


static void sprd_set_spi_pin_input(void)
{
	unsigned int reg;
	unsigned int value;
	unsigned int  gpio;
	unsigned int  pin_func;
	const char    *name;
	unsigned long flags;
	int i = 0;

	int regs_count = sizeof(spi_pin_group)/sizeof(struct spi_pin_desc);

	for (; i < regs_count; i++) {
	    pin_func = spi_pin_group[i].pin_func;
	    gpio = spi_pin_group[i].gpio;
	    name = spi_pin_group[i].name;

	    /* config pin GPIO function */
	    if (pin_func == SPI_PIN_FUNC_DEF) {
		 reg = spi_pin_group[i].reg;

		 local_irq_save(flags);
		 value = ((__raw_readl(reg) & ~SPI_PIN_FUNC_MASK) | SPI_PIN_FUNC_GPIO);
		 __raw_writel(value, reg);
		 local_irq_restore(flags);
		 if (gpio_request(gpio, name)) {
		     printk("smsspi: request gpio %d failed, pin %s\n", gpio, name);
		 }

	    }

	    gpio_direction_input(gpio);
	}

}


static void mxd_cmmb_poweron(void)
{
        regulator_set_voltage(cmmb_regulator_1v8, 1700000, 1800000);
        regulator_disable(cmmb_regulator_1v8);
        msleep(3);
        regulator_enable(cmmb_regulator_1v8);
        msleep(5);

        /* enable 26M external clock */
        gpio_direction_output(GPIO_CMMB_26M_CLK_EN, 1);
}

static void mxd_cmmb_poweroff(void)
{
        regulator_disable(cmmb_regulator_1v8);
        gpio_direction_output(GPIO_CMMB_26M_CLK_EN, 0);
}

static int mxd_cmmb_init(void)
{
         int ret=0;
         ret = gpio_request(GPIO_CMMB_26M_CLK_EN,   "MXD_CMMB_CLKEN");
         if (ret)
         {
                   pr_debug("mxd spi req gpio clk en err!\n");
                   goto err_gpio_init;
         }
         gpio_direction_output(GPIO_CMMB_26M_CLK_EN, 0);
         cmmb_regulator_1v8 = regulator_get(NULL, "vddcmmb1p8");
         return 0;

err_gpio_init:
	 gpio_free(GPIO_CMMB_26M_CLK_EN);
         return ret;
}

static struct mxd_cmmb_026x_platform_data mxd_plat_data = {
	.poweron  = mxd_cmmb_poweron,
	.poweroff = mxd_cmmb_poweroff,
	.init     = mxd_cmmb_init,
	.set_spi_pin_input   = sprd_set_spi_pin_input,
	.restore_spi_pin_cfg = sprd_restore_spi_pin_cfg,
};

static int spi_cs_gpio_map[][2] = {
    {SPI0_CMMB_CS_GPIO,  0},
    {SPI0_CMMB_CS_GPIO,  0},
    {SPI0_CMMB_CS_GPIO,  0},
} ;

static struct spi_board_info spi_boardinfo[] = {
	{
	.modalias = "cmmb-dev",
	.bus_num = 0,
	.chip_select = 0,
	.max_speed_hz = 8 * 1000 * 1000,
	.mode = SPI_CPOL | SPI_CPHA,
        .platform_data = &mxd_plat_data,
	},
	{
	.modalias = "spidev",
	.bus_num = 1,
	.chip_select = 0,
	.max_speed_hz = 1000 * 1000,
	.mode = SPI_CPOL | SPI_CPHA,
	},
	{
	.modalias = "spidev",
	.bus_num = 2,
	.chip_select = 0,
	.max_speed_hz = 1000 * 1000,
	.mode = SPI_CPOL | SPI_CPHA,
	}
};
#endif

#ifdef CONFIG_PERFLOCK_HTC
static unsigned cp5dug_perf_acpu_table[] = {
        FREQ_LOWEST * 1000, /* PERF_LOCK_LOWEST */
        FREQ_LOW * 1000, /* PERF_LOCK_LOW */
        FREQ_MEDIUM * 1000, /* PERF_LOCK_MEDIUM */
        FREQ_HIGH * 1000, /* PERF_LOCK_HIGH */
        FREQ_HIGHEST * 1000, /* PERF_LOCK_HIGHEST */
};
static struct perflock_platform_data cp5dug_perflock_data = {
	.perf_acpu_table = cp5dug_perf_acpu_table,
	.table_size = ARRAY_SIZE(cp5dug_perf_acpu_table),
};

/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
static struct perflock_platform_data cp5dug_cpufreq_ceiling_data = {
	.perf_acpu_table = cp5dug_perf_acpu_table,
	.table_size = ARRAY_SIZE(cp5dug_perf_acpu_table),
};
/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */
#endif /* CONFIG_PERFLOCK_HTC */


#ifdef CONFIG_USB_G_ANDROID
static struct android_usb_platform_data android_usb_data = {
	.vendor_id	= 0xbb4,
	.product_id	= 0x60b,
	.usb_id_pin_gpio	= 102,

	.product_name   = "Android Phone",
	.manufacturer_name = "HTC",
	.serial_number  = "0123456789ABCDEF0123456789ABCDE",

	.rndisVendorDescr = "HTC",
	.rndisVendorID = 0xbb4,
	.rndisEthaddr = {0x01, 0x11, 0x22, 0x33, 0x44, 0x55},

	.ecmVendorDescr = "HTC",
	.ecmVendorID = 0xbb4,
	.ecmEthaddr = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55},
	.fserial_init_string = "tty:modem,tty:serial,tty:serial,tty:serial",
};
static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_data,
	},
};
static void fetch_usb_serial_no()
{
	android_usb_data.serial_number = board_serialno();
}
static void cp5dug_usb_init()
{
	if ((get_radio_flag() & 0x200) || (get_radio2_flag() & 0x200))
		android_usb_data.diag_init = 1;
	/* add cdrom support in normal mode */
	if (board_mfg_mode() == 0) {
		android_usb_data.nluns = 1;
		android_usb_data.cdrom_lun = 0x1;
	}
}
#endif
static void sprd_spi_init(void)
{
	spi_register_board_info(rawchip_spi_board_info,
				ARRAY_SIZE(rawchip_spi_board_info));
#if 0
	int busnum, cs, gpio;
	int i;

	struct spi_board_info *info = spi_boardinfo;

	for (i = 0; i < ARRAY_SIZE(spi_boardinfo); i++) {
		busnum = info[i].bus_num;
		cs = info[i].chip_select;
		gpio   = spi_cs_gpio_map[busnum][cs];

		info[i].controller_data = (void *)gpio;
	}

        spi_register_board_info(info, ARRAY_SIZE(spi_boardinfo));
#endif
}

static int sc8810_add_misc_devices(void)
{
	if (0) {
		platform_set_drvdata(&audio_pa_amplifier_device, audio_pa_amplifier_l);
		if (platform_device_register(&audio_pa_amplifier_device))
			pr_err("faile to install audio_pa_amplifier_device\n");
	}
	return 0;
}

int __init __clock_init_early(void)
{
	pr_info("ahb ctl0 %08x, ctl2 %08x glb aon apb0 %08x aon apb1 %08x clk_en %08x\n",
		sci_glb_raw_read(REG_AP_AHB_AHB_EB),
		sci_glb_raw_read(REG_AP_AHB_AHB_RST),
		sci_glb_raw_read(REG_AON_APB_APB_EB0),
		sci_glb_raw_read(REG_AON_APB_APB_EB1),
		sci_glb_raw_read(REG_AON_CLK_PUB_AHB_CFG));

	sci_glb_clr(REG_AP_AHB_AHB_EB,
		BIT_BUSMON2_EB		|
		BIT_BUSMON1_EB		|
		BIT_BUSMON0_EB		|
		//BIT_SPINLOCK_EB	|
		BIT_GPS_EB		|
		//BIT_EMMC_EB		|
		//BIT_SDIO2_EB		|
		//BIT_SDIO1_EB		|
		//BIT_SDIO0_EB		|
		BIT_DRM_EB		|
		BIT_NFC_EB		|
		//BIT_DMA_EB		|
		//BIT_USB_EB		|
		//BIT_GSP_EB		|
		//BIT_DISPC1_EB		|
		//BIT_DISPC0_EB		|
		//BIT_DSI_EB		|
		0);
	sci_glb_clr(REG_AP_APB_APB_EB,
		BIT_INTC3_EB        |
		BIT_INTC2_EB        |
		BIT_INTC1_EB        |
		BIT_IIS1_EB         |
		BIT_UART2_EB        |
		BIT_UART0_EB        |
		BIT_SPI1_EB         |
		BIT_SPI0_EB         |
		BIT_IIS0_EB         |
		BIT_I2C0_EB         |
		BIT_SPI2_EB         |
		BIT_UART3_EB        |
		0);
	sci_glb_clr(REG_AON_APB_APB_RTC_EB,
		BIT_KPD_RTC_EB      |
		BIT_KPD_EB          |
		BIT_EFUSE_EB        |
		0);
	sci_glb_clr(REG_AON_APB_APB_EB0,
			BIT_AUDIF_EB       |
			BIT_VBC_EB         |
			BIT_PWM3_EB        |
			BIT_PWM1_EB        |
			0);
	sci_glb_clr(REG_AON_APB_APB_EB1,
			BIT_AUX1_EB        |
			BIT_AUX0_EB        |
			0);
	printk("sc clock module early init ok\n");
	return 0;
}

static inline int	__sci_get_chip_id(void)
{
	return __raw_readl(CHIP_ID_LOW_REG);
}

static bool htc_monitor = false;

static void __init cp5dug_init_machine(void)
{
	struct proc_dir_entry *entry = NULL; /*  HTC_KER_ADD robin_peng Sync from QCT- Create /proc/dying_processes to get latest 10 records of killed processes. */
	int ver = cp5dug_get_board_revision();

	printk("sci get chip id = 0x%x\n",__sci_get_chip_id());

	sci_adc_init((void __iomem *)ADC_BASE);
	sci_regulator_init();
	sprd_add_otg_device();
	platform_device_add_data(&sprd_serial_device0,(const void*)&plat_data0,sizeof(plat_data0));
	platform_device_add_data(&sprd_serial_device1,(const void*)&plat_data1,sizeof(plat_data1));
	platform_device_add_data(&sprd_serial_device2,(const void*)&plat_data2,sizeof(plat_data2));
#ifdef CONFIG_VIA_MODEM
	platform_device_add_data(&sprd_serial_device4,(const void*)&plat_data4,sizeof(plat_data4));
#endif
	platform_device_add_data(&sprd_serial_device3,(const void*)&plat_data3,sizeof(plat_data3));
	if (board_mfg_modem_mode() != 0){
		htc_headset_mgr_data.headset_devices_num--;
		htc_headset_mgr_data.headset_devices[1] = NULL;
		}
#ifdef CONFIG_SPRD_MODEM
	board_init_modem();
#endif
	platform_add_devices(devices, ARRAY_SIZE(devices));
	if(BOARD_EVM == ver) {
		add_keypad();
	} else if(BOARD_EVT_XA <= ver){
		add_gpio_keys();
	} else{
		pr_err("Unsupported board!board ID=%d\n",ver);
	}
	init_HW_CHG_LED();
	sc8810_add_i2c_devices();
	sc8810_add_misc_devices();
	sprd_spi_init();
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	sprd_ramconsole_init();
#endif
#ifdef CONFIG_PERFLOCK_HTC
	perflock_init(&cp5dug_perflock_data);
	cpufreq_ceiling_init(&cp5dug_cpufreq_ceiling_data);
#endif
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
	syn_init_vkeys();
#endif
#ifdef CONFIG_USB_G_ANDROID
	fetch_usb_serial_no();
	cp5dug_usb_init();
#endif
#ifdef CONFIG_BT
	bt_export_bd_address();
#endif
	/* HTC_PROC_EMMC_START */
	if (board_emmc_boot()) {
		/* rmt_storage_add_ramfs(); */
		create_proc_read_entry("emmc", 0, NULL, emmc_partition_read_proc, NULL);
	} else {
		printk(KERN_INFO "!board_emmc_boot(), do not read proc/emmc\n");
	}
	/* HTC_PROC_EMMC_END */

	/* HTC_KER_START robin_peng Sync from QCT- Create /proc/dying_processes to get latest 10 records of killed processes. */
	entry = create_proc_read_entry("dying_processes", 0, NULL, dying_processors_read_proc, NULL);
	if (!entry)
		printk(KERN_ERR"Create /proc/dying_processes FAILED!\n");
	/* HTC_KER_END robin_peng Sync from QCT- Create /proc/dying_processes to get latest 10 records of killed processes. */
	if (htc_monitor){
		htc_monitor_init();
		htc_pm_monitor_init();
	}
}

static int __init __suspend_flag(char *str)
{
	int flag = simple_strtol(str, NULL, 0);
	if (flag & 0x100)
		htc_monitor = true;
	return 0;
}

early_param("power_flag", __suspend_flag);

extern void __init  sci_enable_timer_early(void);
static void __init cp5dug_init_early(void)
{
	/* earlier init request than irq and timer */
	__clock_init_early();
	sci_enable_timer_early();
	sci_adi_init();
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	persistent_ram_early_init(&sprd_console_ram);
#endif
}

/*
 * Setup the memory banks.
 */

int __init parse_tag_htc_config(const struct tag *tag);
static void __init cp5dug_fixup(struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = LINUX_BANK_NUM;

	BUG_ON(mi->nr_banks <= 0);

	mi->bank[0].start = LINUX_BASE_ADDR1;
	mi->bank[0].size  = LINUX_SIZE_ADDR1;
	mi->bank[1].start = LINUX_BASE_ADDR2;
	mi->bank[1].size  = LINUX_SIZE_ADDR2;
	mi->bank[2].start = LINUX_BASE_ADDR3;
	mi->bank[2].size  = LINUX_SIZE_ADDR3;

	parse_tag_htc_config((const struct tag *)tags);
	if (htc_get_config(HTC_DBG_FLAG_CP1_TD) & CP1_FLAG_MEM_LAYOUT_SM) {
		mi->bank[1].start = LINUXS_BASE_ADDR2;
		mi->bank[1].size  = LINUXS_SIZE_ADDR2;
		printk(KERN_INFO "CP1 memory layout with SM log\n");
	}

	if (htc_get_config(HTC_DBG_FLAG_CP0_W) & CP0_FLAG_MEM_LAYOUT_SM) {
		mi->bank[2].start = LINUXS_BASE_ADDR3;
		mi->bank[2].size  = LINUXS_SIZE_ADDR3;
		printk(KERN_INFO "CP0 memory layout with SM log\n");
	}

	printk(KERN_INFO "[K] Setup %d memoy banks done \n", mi->nr_banks);
}

MACHINE_START(CP5DUG, "UNKNOWN")
	.reserve	= sci_reserve,
	.map_io		= sci_map_io,
	.fixup		= cp5dug_fixup,
	.init_early	= cp5dug_init_early,
	.handle_irq	= gic_handle_irq,
	.init_irq	= sci_init_irq,
	.timer		= &__timer,
	.init_machine	= cp5dug_init_machine,
MACHINE_END


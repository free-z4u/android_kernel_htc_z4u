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
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board_htc.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/export.h>
#include <linux/fb.h>


#include "sprdfb.h"
#include "sprdfb_panel.h"
#include "sprdfb_dispc_reg.h"
#include "sprdfb_lcdc_reg.h"

static LIST_HEAD(panel_list_main);/* for main_lcd*/
static LIST_HEAD(panel_list_sub);/* for sub_lcd */
static DEFINE_MUTEX(panel_mutex);

static uint32_t lcd_id_from_uboot = 0;
static struct i2c_client *blk_pwm_client;


extern struct panel_if_ctrl sprdfb_mipi_ctrl;
extern struct regulator *lcd_regulator_1v8;
extern struct regulator *lcd_regulator_3v0;
extern bool is_bl_gpio_disable;

#ifdef CONFIG_FB_ESD_SUPPORT
extern void set_backlight(bool value);
extern int esd_suspend_resume(struct sprdfb_device *dev);
#endif

extern void sprdfb_panel_remove(struct sprdfb_device *dev);

#ifdef CONFIG_FB_SC8825
typedef struct {
	uint32_t reg;
	uint32_t val;
} panel_pinmap_t;

panel_pinmap_t panel_rstpin_map[] = {
	{REG_PIN_LCD_RSTN, BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_WPU|BIT_PIN_SLP_OE},
	{REG_PIN_LCD_RSTN, BITS_PIN_DS(3)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_NUL|BIT_PIN_SLP_OE},
};

static void sprd_panel_set_rstn_prop(unsigned int if_slp)
{
	int i;

	if (if_slp){
		panel_rstpin_map[0].val = __raw_readl(CTL_PIN_BASE+REG_PIN_LCD_RSTN);
		i = 1;
	}else{
		i = 0;
	}

	__raw_writel(panel_rstpin_map[i].val, CTL_PIN_BASE + panel_rstpin_map[i].reg);
}
#endif


#define GPIO_LCD_ID0        108
#define GPIO_LCD_ID1        109
#define GPIO_LCD_RST        214
#define GPIO_LCD_1V8        180
#define GPIO_LCD_3V0_EVM    182
#define GPIO_LCD_3V0_EVT    239
#define BL_HW_EN            159
#define GPIO_LCD_GPIO132    132

enum {
	NOVATEK_NT35512_JDI,
	NOVATEK_NT35512_AUO,
	NOVATEK_NT35517,
	NOVATEK_NT35517_JDI,
	NO_PANEL,
} lcm_vendor;


int panel_init(struct sprdfb_device *dev);

static int __init lcd_id_get(char *str)
{
	if ((str != NULL) && (str[0] == 'I') && (str[1] == 'D') && (str[2] == 'J') && (str[3] == 'D') && (str[4] == 'I')) {
		sscanf(&str[5], "%x", &lcd_id_from_uboot);
		lcm_vendor = NOVATEK_NT35512_JDI;
		printk(KERN_INFO "[DISP] NOVATEK_NT35512_JDI\n");
	} else if ((str != NULL) && (str[0] == 'I') && (str[1] == 'D') && (str[2] == 'A') && (str[3] == 'U') && (str[4] == 'O')) {
		sscanf(&str[5], "%x", &lcd_id_from_uboot);
		lcm_vendor = NOVATEK_NT35512_AUO;
		printk(KERN_INFO "[DISP] NOVATEK_NT35512_AUO\n");
	} else if ((str != NULL) && (str[0] == 'I') && (str[1] == 'D') && (str[2] == '1') && (str[3] == '7') && (str[4] == 'J')&&(str[5] == 'D') && (str[6] == 'I')) {
		sscanf(&str[7], "%x", &lcd_id_from_uboot);
		lcm_vendor = NOVATEK_NT35517_JDI;
		printk(KERN_INFO "[DISP] NOVATEK_NT35517_JDI\n");
	} else if ((str != NULL) && (str[0] == 'I') && (str[1] == 'D') && (str[2] == 'L') && (str[3] == 'G')) {
		sscanf(&str[4], "%x", &lcd_id_from_uboot);
		lcm_vendor = NOVATEK_NT35517;
		printk(KERN_INFO "[DISP] NOVATEK_NT35517\n");
	}else if ((str != NULL) && (str[0] == 'I') && (str[1] == 'D') && (str[2] == 'N') && (str[3] == 'O'))
	{
	  sscanf(&str[4], "%x", &lcd_id_from_uboot);
	  lcm_vendor = NO_PANEL;
	  printk(KERN_INFO "[DISP] NO_PANEL\n");
	}

	printk(KERN_INFO "[DISP] sprdfb: [%s]LCD Panel ID from uboot: 0x%x\n", __FUNCTION__, lcd_id_from_uboot);
	return 1;
}
__setup("lcd_id=", lcd_id_get);


static const struct i2c_device_id pwm_i2c_id[] = {
	{ "pwm_i2c", 0 },
	{ }
};

static int pwm_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
	{
		printk("KERN_INFO [DISP]i2c_check_functionality fail\n");
		return -ENODEV;
	}

	blk_pwm_client = client;

	return rc;
}

static struct i2c_driver pwm_i2c_driver = {
	.driver = {
		.name = "pwm_i2c",
		.owner = THIS_MODULE,
	},
	.probe = pwm_i2c_probe,
	.remove =  __exit_p( pwm_i2c_remove),
	.id_table =  pwm_i2c_id,
};
static void __exit pwm_i2c_remove(void)
{
	i2c_del_driver(&pwm_i2c_driver);
}


int z4_enable_gpio(struct panel_spec *panel, struct sprdfb_device *dev)
{
	int ret = 0, err = 0;

	printk("[DISP] > %s\n", __func__);

	lcd_regulator_1v8 = regulator_get(NULL, REGU_NAME_LCD_1V8);
	if (NULL == lcd_regulator_1v8) {
		printk("[DISP] could not get 1.8v regulator\n");
		return -1;
	}
	err = regulator_set_voltage(lcd_regulator_1v8,1800000,1800000);
	if (err)
		printk("[DISP] could not set to 1800mv.\n");

	lcd_regulator_3v0 = regulator_get(NULL, REGU_NAME_LCD_3V0);
	if (NULL == lcd_regulator_3v0) {
		printk("[DISP] could not get 3.0v regulator\n");
		return -1;
	}
	err = regulator_set_voltage(lcd_regulator_3v0,3000000,3000000);
	if (err)
		printk("[DISP] could not set to 3000mv.\n");

//	gpio_request(GPIO_LCD_ID0, "LCD_ID0");
//	gpio_request(GPIO_LCD_ID1, "LCD_ID1");
	gpio_request(GPIO_LCD_RST, "LCD_RST");

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_request(GPIO_LCD_1V8, "LCD_1V8");
		gpio_request(GPIO_LCD_3V0_EVM, "LCD_3V0");
		printk("[DISP] > %s GPIO_LCD_3V0_EVM\n", __func__);
	}

//	gpio_direction_input(GPIO_LCD_ID0);
//	gpio_direction_input(GPIO_LCD_ID1);

	gpio_direction_output(GPIO_LCD_RST, 0);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_direction_output(GPIO_LCD_1V8, 0);
		gpio_direction_output(GPIO_LCD_3V0_EVM, 0);
	}

	hr_msleep(2); //after rst

//	gpio_direction_output(GPIO_LCD_ID0, 0);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_1V8, 1);
	} else {
		if (NULL != lcd_regulator_1v8) {
			regulator_enable(lcd_regulator_1v8);
		}
	}

	hr_msleep(7);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_3V0_EVM, 1);
	} else {
		if (NULL != lcd_regulator_3v0) {
			regulator_enable(lcd_regulator_3v0);
		}
	}

	hr_msleep(5);
	panel_init(dev);

	hr_msleep(50);
	gpio_set_value(GPIO_LCD_RST, 1);
	hr_msleep(12);
	gpio_set_value(GPIO_LCD_RST, 0);
	hr_msleep(12);
	gpio_set_value(GPIO_LCD_RST, 1);
	hr_msleep(12);

	printk("[DISP] < %s\n", __func__);
	return ret;
}

int z4_disable_gpio(void)
{
	int ret = 0;

	printk("[DISP] > %s\n", __func__);

	hr_msleep(12);

	gpio_set_value(GPIO_LCD_RST, 0);
	hr_msleep(12);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_3V0_EVM, 0);
	} else {
		if (NULL != lcd_regulator_3v0) {
			regulator_disable(lcd_regulator_3v0);
		}
	}

	hr_msleep(7);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_1V8, 0);
	} else {
		if (NULL != lcd_regulator_1v8) {
			regulator_disable(lcd_regulator_1v8);
		}
	}

	hr_msleep(2);

//	gpio_free(GPIO_LCD_ID0);
//	gpio_free(GPIO_LCD_ID1);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_free(GPIO_LCD_1V8);
		gpio_free(GPIO_LCD_3V0_EVM);
	} else  {
		if (NULL != lcd_regulator_1v8) {
			regulator_put(lcd_regulator_1v8);
			lcd_regulator_1v8 = NULL;
		}
		if (NULL != lcd_regulator_3v0) {
			regulator_put(lcd_regulator_3v0);
			lcd_regulator_3v0 = NULL;
		}
	}

	gpio_free(GPIO_LCD_RST);

	printk("[DISP] < %s\n", __func__);
	return ret;
}


int cp5_enable_gpio(struct panel_spec *panel, struct sprdfb_device *dev)
{
	int ret = 0, err = 0;
	printk("[DISP] > %s\n", __func__);

	lcd_regulator_1v8 = regulator_get(NULL, REGU_NAME_LCD_1V8);
	if (NULL == lcd_regulator_1v8) {
		printk("[DISP] could not get 1.8v regulator\n");
		return -1;
	}
	err = regulator_set_voltage(lcd_regulator_1v8,1800000,1800000);
	if (err)
		printk("[DISP] could not set to 1800mv.\n");

	lcd_regulator_3v0 = regulator_get(NULL, REGU_NAME_LCD_3V0);
	if (NULL == lcd_regulator_3v0) {
		printk("[DISP] could not get 3.0v regulator\n");
		return -1;
	}
	err = regulator_set_voltage(lcd_regulator_3v0,3000000,3000000);
	if (err)
		printk("[DISP] could not set to 3000mv.\n");


//	gpio_request(GPIO_LCD_ID0, "LCD_ID0");
//	gpio_request(GPIO_LCD_ID1, "LCD_ID1");
	gpio_request(GPIO_LCD_RST, "LCD_RST");

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_request(GPIO_LCD_1V8, "LCD_1V8");
		gpio_request(GPIO_LCD_3V0_EVM, "LCD_3V0");
		printk("[DISP] > %s GPIO_LCD_3V0_EVM\n", __func__);
	}

//#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
   // cp5_enable_bl_gpio();
//#endif

//	gpio_direction_input(GPIO_LCD_ID0);
//	gpio_direction_input(GPIO_LCD_ID1);

	gpio_direction_output(GPIO_LCD_RST, 0);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_direction_output(GPIO_LCD_1V8, 0);
		gpio_direction_output(GPIO_LCD_3V0_EVM, 0);
	}

	hr_msleep(5); //after rst

//	gpio_direction_output(GPIO_LCD_ID0, 0);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_1V8, 1);
	} else {
		if (NULL != lcd_regulator_1v8) {
			regulator_enable(lcd_regulator_1v8);
		}
	}

	hr_msleep(5);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_3V0_EVM, 1);
	} else {
		if (NULL != lcd_regulator_3v0) {
			regulator_enable(lcd_regulator_3v0);
		}
	}

	hr_msleep(5);
	panel_init(dev);

	hr_msleep(5);
	gpio_set_value(GPIO_LCD_RST, 1);
	hr_msleep(5);
	gpio_set_value(GPIO_LCD_RST, 0);
	hr_msleep(5);
	gpio_set_value(GPIO_LCD_RST, 1);
	hr_msleep(35);

	printk("[DISP] < %s\n", __func__);
	return ret;
}

void cp5_disable_bl_gpio(void)
{
    gpio_set_value(BL_HW_EN, 0);
    hr_msleep(5);
    gpio_set_value(GPIO_LCD_GPIO132, 0);
    hr_msleep(5);
}

void cp5_enable_bl_gpio(void)
{
  int rc = 0;
  printk("[DISP] cp5_enable_bl_gpio\n");
  if((cp5dtu_get_board_revision() != BOARD_EVM)&&( lcm_vendor != NO_PANEL ))
  {
     printk("[DISP] set BL_HW_EN to high.\n");
     gpio_request(BL_HW_EN, "BL_HW_EN");
     gpio_request(GPIO_LCD_GPIO132,"GPIO_LCD_GPIO132");
     gpio_direction_output(GPIO_LCD_GPIO132,0);
     gpio_direction_output(BL_HW_EN, 1);
         rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x10, 0xC1);
         if (rc)
                 printk("[DISP] i2c write fail at reg = 0x10, val = 0xC1\n");

         rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x19, 0x13);
         if (rc)
                 printk("[DISP] i2c write fail at reg = 0x19, val = 0x13\n");

         rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x14, 0xC2);
         if (rc)
                 printk("[DISP] i2c write fail at reg = 0x14, val = 0xC2\n");

         rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x79, 0xFF);
         if (rc)
                 printk("[DISP] i2c write fail at reg = 0x79, val = 0xFF\n");

         rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x1D, 0xFA);
         if (rc)
                 printk("[DISP] i2c write fail at reg = 0x1D, val = 0xFA\n");

		 is_bl_gpio_disable = false;
  }
}
int cp5_disable_gpio(void)
{
	int ret = 0;

	printk("[DISP] > %s\n", __func__);

	gpio_set_value(GPIO_LCD_RST, 0);
	hr_msleep(10);

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
    if(cp5dtu_get_board_revision() != BOARD_EVM)
    {
       gpio_set_value(BL_HW_EN, 0);
       hr_msleep(5);
       gpio_set_value(GPIO_LCD_GPIO132, 0);
       hr_msleep(5);
    }
#endif

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_3V0_EVM, 0);
	} else {
		if (NULL != lcd_regulator_3v0) {
			regulator_disable(lcd_regulator_3v0);
		}
	}

	hr_msleep(5);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_set_value(GPIO_LCD_1V8, 0);
	} else {
		if (NULL != lcd_regulator_1v8) {
			regulator_disable(lcd_regulator_1v8);
		}
	}

	hr_msleep(1);

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
    if(cp5dtu_get_board_revision() != BOARD_EVM)
    {
        gpio_free(BL_HW_EN);
        gpio_free(GPIO_LCD_GPIO132);
    }
#endif
//	gpio_free(GPIO_LCD_ID0);
//	gpio_free(GPIO_LCD_ID1);

	if(z4dtg_get_board_revision() == BOARD_EVM) {
		gpio_free(GPIO_LCD_1V8);
		gpio_free(GPIO_LCD_3V0_EVM);
	} else  {
		if (NULL != lcd_regulator_1v8) {
			regulator_put(lcd_regulator_1v8);
			lcd_regulator_1v8 = NULL;
		}
		if (NULL != lcd_regulator_3v0) {
			regulator_put(lcd_regulator_3v0);
			lcd_regulator_3v0 = NULL;
		}
	}

	gpio_free(GPIO_LCD_RST);

	is_bl_gpio_disable = true;

	printk("[DISP] < %s\n", __func__);
	return ret;
}


static int32_t panel_reset_dispc(struct panel_spec *self,struct sprdfb_device *dev)
{
	printk("[DISP] %s\n", __func__);

	if (0x12 == lcd_id_from_uboot) {
		z4_enable_gpio(self,dev);
	} else if (0x17 == lcd_id_from_uboot) {
		cp5_enable_gpio(self,dev);
	} else {
		printk("[DISP] %s Error lcd_id_from_uboot: 0x%x\n", __func__, lcd_id_from_uboot);
	}

#if 0
	dispc_write(1, DISPC_RSTN);
	hr_msleep(20);
	dispc_write(0, DISPC_RSTN);
	hr_msleep(20);
	dispc_write(1, DISPC_RSTN);

	/* wait 10ms util the lcd is stable */
	hr_msleep(120);
#endif
	return 0;
}

static int32_t panel_reset_lcdc(struct panel_spec *self,struct sprdfb_device *dev)
{
	lcdc_write(0, LCM_RSTN);
	hr_msleep(20);
	lcdc_write(1, LCM_RSTN);

	/* wait 10ms util the lcd is stable */
	hr_msleep(20);
	return 0;
}

#if 0
static int32_t panel_set_resetpin_dispc( uint32_t status)
{
	if(0 == status){
		dispc_write(0, DISPC_RSTN);
	}else{
		dispc_write(1, DISPC_RSTN);
	}
	return 0;
}

#ifdef CONFIG_FB_SC8825
static int32_t panel_set_resetpin_lcdc(uint32_t status)
{
	if(0 == status){
		lcdc_write(0, LCM_RSTN);
	}else{
		lcdc_write(1, LCM_RSTN);
	}
	return 0;
}
#endif
#endif

static int panel_reset(struct sprdfb_device *dev)
{
	if((NULL == dev) || (NULL == dev->panel)){
		printk(KERN_ERR "sprdfb: [%s]: Invalid param\n", __FUNCTION__);
		return -1;
	}

	pr_debug("sprdfb: [%s], enter\n",__FUNCTION__);

	//clk/data lane enter LP
	if(NULL != dev->panel->if_ctrl->panel_if_before_panel_reset){
		dev->panel->if_ctrl->panel_if_before_panel_reset(dev);
	}
	hr_msleep(5);

	//reset panel
	dev->panel->ops->panel_reset(dev->panel, dev);

	return 0;
}

#if 0
static void panel_set_resetpin(uint16_t dev_id,  uint32_t status, struct panel_spec *panel )
{
	pr_debug("sprdfb: [%s].\n",__FUNCTION__);

	/*panel set reset pin status*/
	if(SPRDFB_MAINLCD_ID == dev_id){
		panel_set_resetpin_dispc(status);
	}else{
	#ifdef CONFIG_FB_SC8825
		panel_set_resetpin_lcdc(status);
	#endif
	}
}


static int32_t panel_before_resume(struct sprdfb_device *dev)
{
#ifdef CONFIG_FB_SC8825
	/*restore the reset pin status*/
	sprd_panel_set_rstn_prop(0);
#endif
	/*restore  the reset pin to high*/
	panel_set_resetpin(dev->dev_id, 1, dev->panel);
	return 0;
}

static int32_t panel_after_suspend(struct sprdfb_device *dev)
{
	/*set the reset pin to low*/
	panel_set_resetpin(dev->dev_id, 0, dev->panel);
#ifdef CONFIG_FB_SC8825
	/*set the reset pin status and set */
	sprd_panel_set_rstn_prop(1);
#endif
	return 0;
}
#endif

static bool panel_check(struct panel_cfg *cfg)
{
	bool rval = true;

	if(NULL == cfg || NULL == cfg->panel){
		printk(KERN_ERR "[DISP] sprdfb: [%s] :Invalid Param!\n", __FUNCTION__);
		return false;
	}

	pr_debug("[DISP] sprdfb: [%s], dev_id = %d, lcd_id = 0x%x, type = %d\n",__FUNCTION__, cfg->dev_id, cfg->lcd_id, cfg->panel->type);

	switch(cfg->panel->type){
	case SPRDFB_PANEL_TYPE_MIPI:
		cfg->panel->if_ctrl = &sprdfb_mipi_ctrl;
		break;
	default:
		printk("[DISP] sprdfb: [%s]: erro panel type.(%d,%d, %d)",__FUNCTION__, cfg->dev_id, cfg->lcd_id, cfg->panel->type);
		cfg->panel->if_ctrl = NULL;
		break;
	};

	if(cfg->panel->if_ctrl->panel_if_check){
		rval = cfg->panel->if_ctrl->panel_if_check(cfg->panel);
	}
	return rval;
}

static int panel_mount(struct sprdfb_device *dev, struct panel_spec *panel)
{
	printk("[DISP] sprdfb: [%s], dev_id = %d\n",__FUNCTION__, dev->dev_id);

	/* TODO: check whether the mode/res are supported */
	dev->panel = panel;

	if(NULL == dev->panel->ops->panel_reset){
		if(SPRDFB_MAINLCD_ID == dev->dev_id){
			dev->panel->ops->panel_reset = panel_reset_dispc;
		}else{
			dev->panel->ops->panel_reset = panel_reset_lcdc;
		}
	}

	panel->if_ctrl->panel_if_mount(dev);

	return 0;
}


int panel_init(struct sprdfb_device *dev)
{
	if((NULL == dev) || (NULL == dev->panel)){
		printk(KERN_ERR "[DISP] sprdfb: [%s]: Invalid param\n", __FUNCTION__);
		return -1;
	}

	printk("[DISP] sprdfb: [%s], dev_id= %d, type = %d\n",__FUNCTION__, dev->dev_id, dev->panel->type);

	if(!dev->panel->if_ctrl->panel_if_init(dev)){
		printk(KERN_ERR "[DISP] sprdfb: [%s]: panel_if_init fail!\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

int panel_ready(struct sprdfb_device *dev)
{
	if((NULL == dev) || (NULL == dev->panel)){
		printk(KERN_ERR "[DISP] sprdfb: [%s]: Invalid param\n", __FUNCTION__);
		return -1;
	}

	pr_debug("[DISP] sprdfb: [%s], dev_id= %d, type = %d\n",__FUNCTION__, dev->dev_id, dev->panel->type);

	if(NULL != dev->panel->if_ctrl->panel_if_ready){
		dev->panel->if_ctrl->panel_if_ready(dev);
	}

	return 0;
}


static struct panel_spec *adapt_panel_from_uboot(uint16_t dev_id)
{
	struct panel_cfg *cfg;
	struct list_head *panel_list;

	pr_debug("[DISP] sprdfb: [%s], dev_id = %d\n",__FUNCTION__, dev_id);

	if (lcd_id_from_uboot == 0) {
		printk("[DISP] sprdfb: [%s]: Not got lcd id from uboot\n", __FUNCTION__);
		return NULL;
	}

	if(SPRDFB_MAINLCD_ID == dev_id){
		panel_list = &panel_list_main;
	}else{
		panel_list = &panel_list_sub;
	}

	list_for_each_entry(cfg, panel_list, list) {
		if(lcd_id_from_uboot == cfg->lcd_id) {
			printk(KERN_INFO "[DISP] sprdfb: [%s]: LCD Panel 0x%x is attached!\n", __FUNCTION__,cfg->lcd_id);
			return cfg->panel;
		}
	}
	printk(KERN_ERR "[DISP] sprdfb: [%s]: Failed to match LCD Panel from uboot!\n", __FUNCTION__);

	return NULL;
}

static struct panel_spec *adapt_panel_from_readid(struct sprdfb_device *dev)
{
	struct panel_cfg *cfg;
	struct list_head *panel_list;
	int id;

	printk("[DISP] sprdfb: [%s], dev_id = %d\n",__FUNCTION__, dev->dev_id);

	if(SPRDFB_MAINLCD_ID == dev->dev_id){
		panel_list = &panel_list_main;
	}else{
		panel_list = &panel_list_sub;
	}

	list_for_each_entry(cfg, panel_list, list) {
		printk("[DISP] sprdfb: [%s]: try panel 0x%x\n", __FUNCTION__, cfg->lcd_id);
		panel_mount(dev, cfg->panel);
		dev->ctrl->update_clk(dev);

		//panel_init(dev);
		panel_reset(dev);

		id = dev->panel->ops->panel_readid(dev->panel);
		if(id == cfg->lcd_id) {
			pr_debug(KERN_INFO "[DISP] sprdfb: [%s]: LCD Panel 0x%x is attached!\n", __FUNCTION__, cfg->lcd_id);
			dev->panel->ops->panel_init(dev->panel);
			panel_ready(dev);
			return cfg->panel;
		}
		sprdfb_panel_remove(dev);
	}
	printk(KERN_ERR "[DISP] sprdfb:  [%s]: failed to attach LCD Panel!\n", __FUNCTION__);
	return NULL;
}


bool sprdfb_panel_get(struct sprdfb_device *dev)
{
	struct panel_spec *panel = NULL;
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
    int ret =0;
#endif

	if(NULL == dev){
		printk("[DISP] sprdfb: [%s]: Invalid param\n", __FUNCTION__);
		return false;
	}

	printk("[DISP] sprdfb: [%s], dev_id = %d\n",__FUNCTION__, dev->dev_id);

	panel = adapt_panel_from_uboot(dev->dev_id);
	if (panel) {
		dev->panel_ready = true;
		panel_mount(dev, panel);
		panel_init(dev);

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
		if(cp5dtu_get_board_revision() != BOARD_EVM)
			ret = i2c_add_driver(&pwm_i2c_driver);

		if (ret)
			printk(KERN_ERR "[DISP] sprdfb: [%s]: i2c_add_driver fail!\n", __FUNCTION__);
#endif
		printk("[DISP] sprdfb: [%s] got panel\n", __FUNCTION__);
		return true;
	}

	printk("[DISP] sprdfb: [%s] can not got panel\n", __FUNCTION__);

	return false;
}


bool sprdfb_panel_probe(struct sprdfb_device *dev)
{
	struct panel_spec *panel;

	if(NULL == dev){
		printk("[DISP] sprdfb: [%s]: Invalid param\n", __FUNCTION__);
		return false;
	}

	printk("[DISP] sprdfb: [%s], dev_id = %d\n",__FUNCTION__, dev->dev_id);

	/* can not be here in normal; we should get correct device id from uboot */
	panel = adapt_panel_from_readid(dev);

	if (panel) {
		printk("[DISP] sprdfb: [%s] got panel\n", __FUNCTION__);
		return true;
	}

	printk("[DISP] sprdfb: [%s] can not got panel\n", __FUNCTION__);

	return false;
}

void sprdfb_panel_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	/*Jessica TODO: */
	if(NULL != self->ops->panel_invalidate_rect){
		self->ops->panel_invalidate_rect(self, left, top, right, bottom);
	}
	/*Jessica TODO: Need set timing to GRAM timing*/
}

void sprdfb_panel_invalidate(struct panel_spec *self)
{
	/*Jessica TODO:*/
	if(NULL != self->ops->panel_invalidate){
		self->ops->panel_invalidate(self);
	}
	/*Jessica TODO: Need set timing to GRAM timing*/
}

void sprdfb_panel_before_refresh(struct sprdfb_device *dev)
{
	if(NULL != dev->panel->if_ctrl->panel_if_before_refresh){
		dev->panel->if_ctrl->panel_if_before_refresh(dev);
	}
}

void sprdfb_panel_after_refresh(struct sprdfb_device *dev)
{
	if(NULL != dev->panel->if_ctrl->panel_if_after_refresh){
		dev->panel->if_ctrl->panel_if_after_refresh(dev);
	}
}

#ifdef CONFIG_FB_DYNAMIC_FPS_SUPPORT
void sprdfb_panel_change_fps(struct sprdfb_device *dev, int fps_level)
{
	if (dev->panel->ops->panel_change_fps!= NULL) {
        printk("[DISP] sprdfb: [%s] fps_level= %d\n", __FUNCTION__,fps_level);
		dev->panel->ops->panel_change_fps(dev->panel,fps_level);
	}
}
#endif

#ifdef CONFIG_FB_ESD_SUPPORT
/*return value:  0--panel OK.1-panel has been reset*/
uint32_t sprdfb_panel_ESD_check(struct sprdfb_device *dev)
{
	int32_t result = 0;
	//uint32_t if_status = 0;

	pr_debug("[DISP] sprdfb: [%s] (%d, %d, %d)\n",__FUNCTION__, dev->check_esd_time, dev->panel_reset_time, dev->reset_dsi_time);

	dev->check_esd_time++;

	if(SPRDFB_PANEL_IF_EDPI == dev->panel_if_type){
		if (dev->panel->ops->panel_esd_check != NULL) {
			result = dev->panel->ops->panel_esd_check(dev->panel);
			pr_debug("[DISP] sprdfb: [%s] panel check return %d\n", __FUNCTION__, result);
		}
	}else if(SPRDFB_PANEL_IF_DPI == dev->panel_if_type){
#ifdef FB_CHECK_ESD_BY_TE_SUPPORT
	#ifdef CONFIG_MACH_CP5DWG
		int pcbid = cp5dwg_get_board_revision();
		int bomid = htc_get_bomid();

		if ((pcbid < 2) || ((pcbid == 2) && ((bomid <= 2) || (bomid == 0xFF)))) {
			dev->esd_te_waiter = 0;
			dev->esd_te_done = 0;
			result = 1;
			pr_debug("[DISP] W no ESD pcbid=%d bomid=0x%X\n", pcbid, bomid);
		} else {
			dev->esd_te_waiter++;
			dev->esd_te_done = 0;
			dispc_set_bits(BIT(1), DISPC_INT_EN);
			result  = wait_event_interruptible_timeout(dev->esd_te_queue,
				          dev->esd_te_done, msecs_to_jiffies(3000));
			pr_debug("[DISP] sprdfb: after wait (%d)\n", result);
			dispc_clear_bits(BIT(1), DISPC_INT_EN);
			if(!result){ /*time out*/
				printk("[DISP] sprdfb: [%s] esd check  not got te signal!!!!\n", __FUNCTION__);
				dev->esd_te_waiter = 0;
				result = 0;
			}else{
				pr_debug("[DISP] sprdfb: [%s] esd check  got te signal!\n", __FUNCTION__);
				result = 1;
			}
		}
	#else
		dev->esd_te_waiter++;
		dev->esd_te_done = 0;
		dispc_set_bits(BIT(1), DISPC_INT_EN);
		result	= wait_event_interruptible_timeout(dev->esd_te_queue,
					  dev->esd_te_done, msecs_to_jiffies(3000));
		pr_debug("[DISP] sprdfb: after wait (%d)\n", result);
		dispc_clear_bits(BIT(1), DISPC_INT_EN);
		if(!result){ /*time out*/
			printk("[DISP] sprdfb: [%s] esd check  not got te signal!!!!\n", __FUNCTION__);
			dev->esd_te_waiter = 0;
			result = 0;
		}else{
			pr_debug("[DISP] sprdfb: [%s] esd check  got te signal!\n", __FUNCTION__);
			result = 1;
		}
	#endif
#else
		if (dev->panel->ops->panel_esd_check != NULL) {
			result = dev->panel->ops->panel_esd_check(dev->panel);
			pr_debug("[DISP] sprdfb: [%s] panel check return %d\n", __FUNCTION__, result);
		}

#endif
	}


	if(0 == dev->enable){
		pr_debug("[DISP] sprdfb: [%s] leave (Invalid device status)!\n", __FUNCTION__);
		return 0;
	}

	if(result == 0){
		dev->panel_reset_time++;

#if 0
		if(SPRDFB_PANEL_IF_EDPI == dev->panel_if_type){
			if(NULL != dev->panel->if_ctrl->panel_if_get_status){
				if_status = dev->panel->if_ctrl->panel_if_get_status(dev);
			}
		}else if(SPRDFB_PANEL_IF_DPI == dev->panel_if_type){
			if_status = 2; /*need reset dsi as default for dpi mode*/
		}

		if(0 == if_status){
			printk("[DISP] sprdfb: [%s] fail! Need reset panel\n",__FUNCTION__);
			panel_reset(dev);

			if(0 == dev->enable){
				printk("[DISP] sprdfb: [%s] leave (Invalid device status)!\n", __FUNCTION__);
				return 0;
			}

			dev->panel->ops->panel_init(dev->panel);
			panel_ready(dev);
		}else{
#endif
			//set_backlight(false);

			printk("[DISP] sprdfb: [%s] fail! Need reset panel and panel if!!!!\n",__FUNCTION__);
			dev->reset_dsi_time++;

#if 0
			if(NULL != dev->panel->if_ctrl->panel_if_suspend){
				dev->panel->if_ctrl->panel_if_suspend(dev);
			}

			hr_msleep(10);

			if(0 == dev->enable){
				pr_debug("[DISP] sprdfb: [%s] leave (Invalid device status)!\n", __FUNCTION__);
				return 0;
			}

//			panel_init(dev);
			panel_reset(dev);

			if(0 == dev->enable){
				pr_debug("[DISP] sprdfb: [%s] leave (Invalid device status)!\n", __FUNCTION__);
				return 0;
			}

			dev->panel->ops->panel_init(dev->panel);
			panel_ready(dev);
#endif

			if (!esd_suspend_resume(dev))
				return 0;
#if 0
		}
#endif
		pr_debug("[DISP] sprdfb: [%s]return 1\n",__FUNCTION__);
		return 1;
	}
	pr_debug("[DISP] sprdfb: [%s]return 0\n",__FUNCTION__);
	return 0;
}
#endif

void sprdfb_panel_suspend(struct sprdfb_device *dev)
{
	if(NULL == dev->panel){
		return;
	}

	printk("[DISP] sprdfb: [%s], dev_id = %d\n",__FUNCTION__, dev->dev_id);

#if 1
	//step1-1 clk/data lane enter LP
	if(NULL != dev->panel->if_ctrl->panel_if_before_panel_reset){
		dev->panel->if_ctrl->panel_if_before_panel_reset(dev);
	}

	//step1-2 enter sleep  (another way : reset panel)
	/*Jessica TODO: Need do some I2c, SPI, mipi sleep here*/
	/* let lcdc sleep in */
	if (dev->panel->ops->panel_enter_sleep != NULL) {
		dev->panel->ops->panel_enter_sleep(dev->panel,1);
	}

	if (0x12 == lcd_id_from_uboot) {
		;
	} else if (0x17 == lcd_id_from_uboot) {
		hr_msleep(100);
	} else {
		printk("[DISP] %s Error lcd_id_from_uboot: 0x%x\n", __func__, lcd_id_from_uboot);
	}
#else
	//step1 reset panel
	panel_reset(dev);
#endif

	//step2 clk/data lane enter ulps
	if(NULL != dev->panel->if_ctrl->panel_if_enter_ulps){
		dev->panel->if_ctrl->panel_if_enter_ulps(dev);
		printk("[DISP] sprdfb: enter ulps end");
	}

	//step3 turn off mipi
	if(NULL != dev->panel->if_ctrl->panel_if_suspend){
		dev->panel->if_ctrl->panel_if_suspend(dev);
	}

#if 0
	//step4 reset pin to low
	if (dev->panel->ops->panel_after_suspend != NULL) {
		//himax mipi lcd may define empty function
		dev->panel->ops->panel_after_suspend(dev->panel);
	} else{
		panel_after_suspend(dev);
	}
#endif

	if (0x12 == lcd_id_from_uboot) {
		z4_disable_gpio();
	} else if (0x17 == lcd_id_from_uboot) {
		cp5_disable_gpio();
	} else {
		printk("[DISP] %s Error lcd_id_from_uboot: 0x%x\n", __func__, lcd_id_from_uboot);
	}
}

void sprdfb_panel_resume(struct sprdfb_device *dev, bool from_deep_sleep)
{
	if(NULL == dev->panel){
		return;
	}

	printk(KERN_INFO "[DISP] sprdfb:[%s], dev->enable= %d, from_deep_sleep = %d\n",__FUNCTION__, dev->enable, from_deep_sleep);

#if 0
	/*Jessica TODO: resume i2c, spi, mipi*/
	if(NULL != dev->panel->if_ctrl->panel_if_resume){
		dev->panel->if_ctrl->panel_if_resume(dev);
	}
	panel_ready(dev);

	//step1 reset pin to high
	if (dev->panel->ops->panel_before_resume != NULL) {
		//himax mipi lcd may define empty function
		dev->panel->ops->panel_before_resume(dev->panel);
	}
	else{
		panel_before_resume(dev);
	}
#endif

	if(from_deep_sleep){
		//step2 turn on mipi
		//panel_init(dev);

		//step3 reset panel
		panel_reset(dev);

		//step4 panel init
		dev->panel->ops->panel_init(dev->panel);

		//step5 clk/data lane enter HS
		panel_ready(dev);
	}else{
		//step2 turn on mipi
		/*Jessica TODO: resume i2c, spi, mipi*/
		if(NULL != dev->panel->if_ctrl->panel_if_resume){
			dev->panel->if_ctrl->panel_if_resume(dev);
		}

		//step3 sleep out
		if(NULL != dev->panel->ops->panel_enter_sleep){
			dev->panel->ops->panel_enter_sleep(dev->panel,0);
		}

		//step4 clk/data lane enter HS
		panel_ready(dev);
	}

}

void sprdfb_panel_remove(struct sprdfb_device *dev)
{
	if(NULL == dev->panel){
		return;
	}
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
    if(cp5dtu_get_board_revision() != BOARD_EVM)
	   i2c_del_driver(&pwm_i2c_driver);
#endif

	/*Jessica TODO:close panel, i2c, spi, mipi*/
	if(NULL != dev->panel->if_ctrl->panel_if_uninit){
		dev->panel->if_ctrl->panel_if_uninit(dev);
	}
	dev->panel = NULL;
}


int sprdfb_panel_register(struct panel_cfg *cfg)
{
	pr_debug("[DISP] sprdfb: [%s], panel id = %d\n",__FUNCTION__, cfg->dev_id);

	if(!panel_check(cfg)){
		printk("[DISP] sprdfb: [%s]: panel check fail!id = %d\n",__FUNCTION__,  cfg->dev_id);
		return -1;
	}

	mutex_lock(&panel_mutex);

	if (cfg->dev_id == SPRDFB_MAINLCD_ID) {
		list_add_tail(&cfg->list, &panel_list_main);
	} else if (cfg->dev_id == SPRDFB_SUBLCD_ID) {
		list_add_tail(&cfg->list, &panel_list_sub);
	} else {
		list_add_tail(&cfg->list, &panel_list_main);
		list_add_tail(&cfg->list, &panel_list_sub);
	}

	mutex_unlock(&panel_mutex);

	return 0;
}

 

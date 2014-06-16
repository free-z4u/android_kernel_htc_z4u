/* linux/arch/arm/mach-sc/adc_debug_htc.c
 * Copyright (C) 2013 HTC Corporation.
 * Author: Oliver Fu <oliver_fu@htc.com>
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

#include <linux/bug.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/debugfs.h>

#include <mach/hardware.h>
#include <mach/sci_glb_regs.h>
#include <mach/adi.h>
#include <mach/adc.h>
#include <mach/arch_lock.h>

static const char *const channel_desc[] = {
	"ADC_CHANNEL_0",
	"ADC_CHANNEL_1",
	"ADC_CHANNEL_2",
	"ADC_CHANNEL_3",
	"ADC_CHANNEL_PROG",
	"ADC_CHANNEL_VBAT",
	"ADC_CHANNEL_VCHGSEN",
	"ADC_CHANNEL_VCHGBG",
	"ADC_CHANNEL_ISENSE",
	"ADC_CHANNEL_TPYD",
	"ADC_CHANNEL_TPYU",
	"ADC_CHANNEL_TPXR",
	"ADC_CHANNEL_TPXL",
	"ADC_CHANNEL_DCDCCORE",
	"ADC_CHANNEL_DCDCARM",
	"ADC_CHANNEL_DCDCMEM",
	"ADC_CHANNEL_DCDCLDO",
	"ADC_CHANNEL_DCDCGPU",
	"ADC_CHANNEL_DCDCWRF",
	"ADC_CHANNEL_VBATBK",
	"ADC_CHANNEL_HEADMIC",
	"ADC_CHANNEL_LDO0",
	"ADC_CHANNEL_LDO1",
	"ADC_CHANNEL_LDO2",
	"ADC_CHANNEL_WHTLED",
	"ADC_CHANNEL_OTP",
	"ADC_CHANNEL_LPLDO0",	/*SIM0/SIM1/SIM2/EMMCCORE/VDD28/VDD25/USB, Low power mode reference*/
	"ADC_CHANNEL_LPLDO1",	/*CAMD/EMMCIO/VDD18/AVDD18/CAMIO/CLSG, Low power mode reference*/
	"ADC_CHANNEL_LPLDO2",	/*RF0/RF1/RF2/CAMA/SD/CAMMOT, Low power mode reference*/
	"ADC_CHANNEL_WHTLED_VFB",
	"ADC_CHANNEL_USBDP",
	"ADC_CHANNEL_USBDM"
};

static int adc_show_all(struct seq_file *s, void *v)
{
	int channel;

	seq_printf(s, "\nCalibration: %s\n\n", htc_adc_is_calibrated() ? "OK" : "NONE");

	for (channel = 0; channel <= ADC_MAX; channel++)
	{
		int val;
		uint16_t mV;

		if (channel_desc[channel] == NULL) continue;

		val = sci_adc_get_value(channel, true);
		mV = htc_adc_to_vol(channel, val);

		seq_printf(s, "[%02u] %-20s %4u %4u(mV)\n", channel, channel_desc[channel], val, mV);
	}

	return 0;
}

static int adc_all_open(struct inode *inode, struct file *file)
{
	return single_open(file, adc_show_all, inode->i_private);
}

static const struct file_operations adc_all_fops = {
	.open		= adc_all_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init adc_debugfs_init(void)
{
	static struct dentry *debug_root = NULL;

	debug_root = debugfs_create_dir("htc_adc", NULL);

	if (IS_ERR_OR_NULL(debug_root)) {
		pr_err("%s return %p\n", __FUNCTION__, debug_root);
		return PTR_ERR(debug_root);
	}

	debugfs_create_file("all", S_IRUSR | S_IRGRP, debug_root, NULL, &adc_all_fops);

	return 0;
}

module_init(adc_debugfs_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Fu <oliver_fu@htc.com>");
MODULE_DESCRIPTION("adc debugfs");

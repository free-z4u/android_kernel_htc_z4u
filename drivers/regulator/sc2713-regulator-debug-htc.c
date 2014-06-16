/* linux/drivers/regulator/sc2713-regulator-debug-htc.c
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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include <mach/sci.h>
#include <mach/hardware.h>
#include <mach/adi.h>

struct regulator {
	struct device *dev;
	struct list_head list;
	int uA_load;
	int min_uV;
	int max_uV;
	char *supply_name;
	struct device_attribute dev_attr;
	struct regulator_dev *rdev;
	struct dentry *debugfs;
};

struct sci_regulator_regs {
	int typ;
	u32 pd_set, pd_set_bit;
	u32 pd_rst, pd_rst_bit;
	u32 slp_ctl, slp_ctl_bit;
	u32 vol_trm, vol_trm_bits;
	u32 cal_ctl, cal_ctl_bits;
	u32 vol_def;
	u32 vol_ctl, vol_ctl_bits;
	u32 vol_sel_cnt, vol_sel[];
};

struct sci_regulator_data {
	struct delayed_work dwork;
	struct regulator_dev *rdev;
};

struct sci_regulator_desc {
	struct regulator_desc desc;
	struct sci_regulator_ops *ops;
	const struct sci_regulator_regs *regs;
	struct sci_regulator_data data;	/* FIXME: dynamic */
#if defined(CONFIG_DEBUG_FS)
	struct dentry *debugfs;
#endif
};

static const char *const types[] = {
	"LDO",
	"LDO_D",
	"DCDC"
};

#define SCI_REGU_REG(VDD, ...) \
	regulator_show(s, #VDD)

static int flags = 0;

#define SEQ_PRINTF(s,  fmt, args...) \
	do {					\
		if( NULL == s)		\
			printk(fmt, ##args);		\
		else			\
			seq_printf(s, fmt, ##args);	\
	}while(0)

extern int regu_adc_voltage(struct regulator_dev *rdev);

static void regulator_show(struct seq_file *s, const char *id)
{
	struct regulator *regulator = regulator_get(NULL, id);
	if (IS_ERR_OR_NULL(regulator)) {
		SEQ_PRINTF(s, "%-20s (unknown error)\n", id);
		return;
	}

	{
		struct regulator_dev *rdev = regulator->rdev;
		struct sci_regulator_desc *desc = (struct sci_regulator_desc *)rdev->desc;
		const struct sci_regulator_regs *regs = desc->regs;

		int count;
		int i;
		int is_enabled;
		int volc_ctl_shft = __ffs(regs->vol_ctl_bits);

		is_enabled = regulator_is_enabled(regulator);
		// EINVAL (-22) EACCES(-13)
		SEQ_PRINTF(s, "%-20s %-5s %-3s ", id, types[regs->typ], is_enabled < 0 ? "U" : (is_enabled ? "ON" : "OFF"));

		count = regs->vol_sel_cnt;
		if (count > 0) {
			//int current_vol = regulator_get_voltage(regulator) / 1000;
			int found = 0;

			for (i = 0; i < count; i++) {
				//int mV = regulator_list_voltage(regulator, i) / 1000;
				int mV = regs->vol_sel[i];
				//if (mV == current_vol)
				if (!found && regs->vol_ctl) {
					int sel = (sci_adi_read(regs->vol_ctl) & regs->vol_ctl_bits) >> volc_ctl_shft;
					if (sel == i) {
						SEQ_PRINTF(s, "*");
						found = 1;
					}
				}
				SEQ_PRINTF(s, "%u ", mV);
			}
			//if (!found) seq_printf(s, "(%u)", current_vol);
		}

		SEQ_PRINTF(s, " (%u)\n",is_enabled==0 ? 0:regu_adc_voltage(rdev));

		if (flags == 1) {
			if (regs->pd_set) {
				SEQ_PRINTF(s, "PD_SET(%08x) = %08x ", regs->pd_set, sci_adi_read(regs->pd_set));
			}
			if (regs->pd_rst) {
				SEQ_PRINTF(s, "PD_RST(%08x) = %08x ", regs->pd_rst, sci_adi_read(regs->pd_rst));
			}
			if (regs->slp_ctl) {
				SEQ_PRINTF(s, "SLP_CTL(%08x) = %08x ", regs->slp_ctl, sci_adi_read(regs->slp_ctl));
			}
			if (regs->vol_ctl) {
				SEQ_PRINTF(s, "VOL_CTL(%08x) = %08x ", regs->vol_ctl, sci_adi_read(regs->vol_ctl));
			}
			SEQ_PRINTF(s, "\n");
		}
	}

	regulator_put(regulator);
}

int show_regulators(struct seq_file *s, void *v)
{
#include CONFIG_REGULATOR_SPRD_MAP

	return 0;
}

static int regulator_open(struct inode *inode, struct file *file)
{
	flags = (int)inode->i_private;

	return single_open(file, show_regulators, inode->i_private);
}

static const struct file_operations regulator_fops = {
	.open		= regulator_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init regulator_debugfs_init(void)
{
	static struct dentry *debug_root = NULL;

	debug_root = debugfs_create_dir("htc_regulator", NULL);

	if (IS_ERR_OR_NULL(debug_root)) {
		pr_err("%s return %p\n", __FUNCTION__, debug_root);
		return PTR_ERR(debug_root);
	}

	debugfs_create_file("regulators", S_IRUSR | S_IRGRP, debug_root, (void *)0, &regulator_fops);
	debugfs_create_file("regulators_regs", S_IRUSR | S_IRGRP, debug_root, (void *)1, &regulator_fops);

	return 0;
}

module_init(regulator_debugfs_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Fu <oliver_fu@htc.com>");
MODULE_DESCRIPTION("regulator debugfs");

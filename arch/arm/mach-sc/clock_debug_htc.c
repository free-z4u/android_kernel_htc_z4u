/* linux/arch/arm/mach-sc/clock_debug_htc.c
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

#include <linux/clk.h>
#include <linux/clkdev.h>

#include <mach/sci.h>
#include <mach/sci_glb_regs.h>
#include <mach/hardware.h>

#include "clock.h"

extern const u32 __clkinit0 __clkinit_begin;
extern const u32 __clkinit2 __clkinit_end;

static int flags = 0;

static int sci_clk_is_enable(struct clk *c)
{
	int enable;

	pr_debug("clk %p (%s) enb %08x\n", c, c->regs->name, c->regs->enb.reg);

	BUG_ON(!c->regs->enb.reg);
	if (!c->regs->enb.mask) {	/* check matrix clock */
		enable = ! !sci_clk_is_enable((struct clk *)c->regs->enb.reg);
	} else {
		enable =
		    ! !sci_glb_read(c->regs->enb.reg & ~1, c->regs->enb.mask);
	}

	if (c->regs->enb.reg & 1)
		enable = !enable;
	return enable;
}

static bool clk_is_enable(struct clk *c)
{
	return c->enable == NULL || sci_clk_is_enable(c) ? true : false;
}

#define SEQ_PRINTF(s,  fmt, args...) \
	do {					\
		if( NULL == s)		\
			printk(fmt, ##args);		\
		else			\
			seq_printf(s, fmt, ##args);	\
	}while(0)

static void show_clock(struct seq_file *s, struct clk *clk, int indent)
{
	struct clk_lookup *cl = (struct clk_lookup *)(&__clkinit_begin + 1);
	int i;

	for (i = 0; i < indent; i++) {
		SEQ_PRINTF(s, "  |");
	}

	if (indent == 0) {
		SEQ_PRINTF(s, flags ? "clocks(active)\n" : "clocks\n");
	} else {
		if (flags == 1) {
			// show active only
			SEQ_PRINTF(s, "- %s %lu (usage %d)\n",
				clk->regs->name, clk_get_rate(clk), clk->usage);
		} else {
			SEQ_PRINTF(s, "- %s %s %lu (usage %d)\n",
				clk->regs->name, clk_is_enable(clk) ? "ON" : "OFF", clk_get_rate(clk), clk->usage);
		}
	}

	while (cl < (struct clk_lookup *)&__clkinit_end)
	{
		struct clk *c = cl->clk;
		struct clk *p = clk_get_parent(c);

		if (p != clk) {
			cl++;
			continue;
		}

		if (flags == 1) {
			// show active only
			if (!clk_is_enable(c)) {
				cl++;
				continue;
			}
		}

		show_clock(s, c, indent + 1);
		cl++;
	}
}

int show_clocktree(struct seq_file *s, void *v)
{
	struct clk *mm_clock;
	struct clk_lookup *cl = (struct clk_lookup *)(&__clkinit_begin + 1);

	while (cl < (struct clk_lookup *)&__clkinit_end){
	struct clk *c = cl->clk;

	if(!strcmp("clk_mm_i",c->regs->name))
		mm_clock=c;
		cl++;
	}
	clk_enable(mm_clock);
	show_clock(s, NULL, 0);
	clk_disable(mm_clock);

	return 0;

}

static int clock_all_open(struct inode *inode, struct file *file)
{
	flags = (int)inode->i_private;

	return single_open(file, show_clocktree, inode->i_private);
}

static const struct file_operations clock_all_fops = {
	.open		= clock_all_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init clock_debugfs_init(void)
{
	static struct dentry *debug_root = NULL;

	debug_root = debugfs_create_dir("htc_clock", NULL);

	if (IS_ERR_OR_NULL(debug_root)) {
		pr_err("%s return %p\n", __FUNCTION__, debug_root);
		return PTR_ERR(debug_root);
	}

	debugfs_create_file("clocks", S_IRUSR | S_IRGRP, debug_root, (void *)0, &clock_all_fops);
	debugfs_create_file("clocks_active", S_IRUSR | S_IRGRP, debug_root, (void *)1, &clock_all_fops);

	return 0;
}

module_init(clock_debugfs_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Fu <oliver_fu@htc.com>");
MODULE_DESCRIPTION("clock debugfs");

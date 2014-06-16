/* linux/arch/arm/mach-sc/htc-debug-ftrace.c
 * Copyright (C) 2013 HTC Corporation.
 * Author: Yili Xie <yili_xie@htc.com>
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


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/genalloc.h>
#include <linux/ftrace_event.h>
#include <mach/board.h>
#include <mach/board_htc.h>

extern int tracing_update_buffers(void);
extern int tracing_set_tracer(const char *buf);

unsigned long ft_pool_phys = FT_BASE_PHY;
struct gen_pool *ft_pool;
void * ft_virt_base;

unsigned long ft__get_free_page(unsigned int ignored)
{
	if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE) {
		if (!ft_pool) {
			printk(KERN_ERR "FT buffer is not initial\n");
			return 0;
		}

		return gen_pool_alloc(ft_pool, PAGE_SIZE);
	}
	else
		return __get_free_page(ignored);
}

void ft_free_page(unsigned long page)
{
	if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE) {
		if(!page)
			return;

		gen_pool_free(ft_pool, page, PAGE_SIZE);
	}
	else
		free_page(page);
}

static int __init ft_pool_init(void)
{
	int err;

	if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE) {
		ft_virt_base = ioremap(ft_pool_phys, SZ_8M);
		if (!ft_virt_base)
			return -ENOMEM;

		printk(KERN_INFO "[k] Genalloc for FT uncached buffer base:%p \n", ft_virt_base);

		ft_pool = gen_pool_create(PAGE_SHIFT, -1);
		if (!ft_pool) {
			printk(KERN_ERR "Create pool for FT fail\n");
			return -ENOMEM;
		}

		err = gen_pool_add(ft_pool, (unsigned long) ft_virt_base, SZ_8M, -1);
		if (err) {
			printk(KERN_ERR "Add memory to FT pool fail\n");
			return err;
		}
	}

	return 0;
}

static int __init ft_late(void)
{
	if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE) {
		printk(KERN_INFO "[k] Uncache FT is enabled\n");
		tracing_update_buffers();

		if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE_HANG) {
			trace_set_clr_event("irq", "irq_handler_entry", 1);
			trace_set_clr_event("irq", "irq_handler_exit", 1);
			trace_set_clr_event("sched", "sched_switch", 1);
			trace_set_clr_event("workqueue", "workqueue_execute_start", 1);
			trace_set_clr_event("workqueue", "workqueue_execute_end", 1);
			trace_set_clr_event("power", "clock_enable", 1);
			trace_set_clr_event("power", "clock_disable", 1);
			trace_set_clr_event("regulator", "regulator_enable", 1);
			trace_set_clr_event("regulator", "regulator_disable", 1);
			printk(KERN_INFO "[k] Enable FT for hang issue debug\n");
		}

		if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE_MEM) {
			trace_set_clr_event("kmem", NULL, 1);
			trace_set_clr_event("oom", NULL, 1);
			printk(KERN_INFO "[k] Enable FT for memory issue debug\n");
		}

		if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE_CPU) {
			trace_set_clr_event("sched", NULL, 1);
			trace_set_clr_event("irq", NULL, 1);
			trace_set_clr_event("workqueue", NULL, 1);
			trace_set_clr_event("rcu", NULL, 1);
			printk(KERN_INFO "[k] Enable FT for SCHED issue debug\n");
		}

		if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE_SIGNAL) {
			trace_set_clr_event("signal", NULL, 1);
			printk(KERN_INFO "[k] Enable FT for SIGNAL issue debug\n");
		}

		if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE_BINDER) {
			trace_set_clr_event("binder", NULL, 1);
			printk(KERN_INFO "[k] Enable FT for BINDER issue debug\n");
		}

		if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) & SUPERMAN_FLAG_FTRACE_POWER) {
			trace_set_clr_event("power", NULL, 1);
			trace_set_clr_event("regulator", NULL, 1);
			trace_set_clr_event("gpio", NULL, 1);
			printk(KERN_INFO "[k] Enable FT for POWER issue debug\n");
		}
	}

	return 0;
}

core_initcall(ft_pool_init);
late_initcall(ft_late);

MODULE_AUTHOR("Yili Xie <yili_xie@htc.com>");
MODULE_DESCRIPTION("Uncached Ftrace function");
MODULE_LICENSE("GPL v2");

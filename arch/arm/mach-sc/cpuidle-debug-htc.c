/* linux/arch/arm/mach-sc/cpuidle-debug-htc.c
 * Copyright (C) 2013 HTC Corporation.
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
#include <linux/io.h>
#include <mach/htc_util.h>
#include <linux/clockchips.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <asm/div64.h>

ktime_t enter;
extern struct st_htc_idle_statistic htc_idle_Stat[][3];
extern void htc_idle_stat_clear(void);

static int stats_print(struct seq_file *s, void *p)
{
	int i = 0, cpu = 0;
	u32 exit = ktime_to_ms(ktime_sub(ktime_get(), enter));
	u32 per = 0, cpu_time = 0;
	u32 total_time = 0;

	seq_printf(s, "total time : %d\n", exit);
	seq_printf(s, "[Idle]        cpu_id|     cpu_state|    idle_count|     idle_time| PS(%%)        |\n");
	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		total_time = 0;
		for (i = 0; i < 3 ; i++) {
			if (htc_idle_Stat[cpu][i].count) {
				cpu_time =  htc_idle_Stat[cpu][i].time / 1000;
				total_time += cpu_time;
				seq_printf(s, "       %13d| %13d| %13d| %10d ms|\n",
					cpu, i, htc_idle_Stat[cpu][i].count, cpu_time);
			}
		}
		if (total_time) {
			per = 0;
			per = ((total_time  * 100 )/ exit);
			seq_printf(s, "                                                                 %14d|\n", per);
		}
	}
	return 0;
}

static int stats_open_file(struct inode *inode, struct file *file)
{
	return single_open(file, stats_print, inode->i_private);
}

static ssize_t stats_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	htc_idle_stat_clear();
	enter = ktime_get();

	return count;
}

static const struct file_operations idle_ops_stats = {
	.open		= stats_open_file,
	.write		= stats_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static int __init cpuidle_debugfs_init(void)
{
	static struct dentry *debug_root = NULL;

	debug_root = debugfs_create_dir("cpuidle", NULL);

	if (IS_ERR_OR_NULL(debug_root)) {
		pr_err("%s return %p\n", __FUNCTION__, debug_root);
		return PTR_ERR(debug_root);
	}

	debugfs_create_file("stats", S_IRUSR | S_IRGRP, debug_root, NULL, &idle_ops_stats);

	return 0;
}

module_init(cpuidle_debugfs_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("cpuidle debugfs");
/* arch/arm/mach-msm/perflock.c
 *
 * Copyright (C) 2008 HTC Corporation
 * Author: Eiven Peng <eiven_peng@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/earlysuspend.h>
#include <linux/cpufreq.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <mach/perflock.h>
#include <mach/freq_table.h>
#include <linux/sprd_thm.h>

/*
 * #include "proc_comm.h"
 * #include "acpuclock.h"
 * #include <mach/socinfo.h>
 */

#define PERF_LOCK_INITIALIZED	(1U << 0)
#define PERF_LOCK_ACTIVE	(1U << 1)

unsigned int CONFIG_PERFLOCK_SCREEN_ON_MIN=300000;
unsigned int CONFIG_PERFLOCK_SCREEN_ON_MAX=1200000;
enum {
	PERF_LOCK_DEBUG = 1U << 0,
	PERF_EXPIRE_DEBUG = 1U << 1,
	PERF_CPUFREQ_NOTIFY_DEBUG = 1U << 2,
	PERF_CPUFREQ_LOCK_DEBUG = 1U << 3,
	PERF_SCREEN_ON_POLICY_DEBUG = 1U << 4,
};

static LIST_HEAD(active_perf_locks);
static LIST_HEAD(inactive_perf_locks);
/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
static LIST_HEAD(active_cpufreq_ceiling_locks);
static LIST_HEAD(inactive_cpufreq_ceiling_locks);
/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */
static DEFINE_SPINLOCK(list_lock);
static DEFINE_SPINLOCK(policy_update_lock);
static int initialized;
static int cpufreq_ceiling_initialized;                 /* HTC_KER_ADD, Winster @Perflock & Ceiling. */
static unsigned int *perf_acpu_table;
static unsigned int *cpufreq_ceiling_acpu_table;        /* HTC_KER_ADD, Winster @Perflock & Ceiling. */
static unsigned int table_size;
/*static unsigned int curr_lock_speed;*/
static struct cpufreq_policy *cpufreq_policy;

#ifdef CONFIG_PERF_LOCK_DEBUG
static int debug_mask = PERF_LOCK_DEBUG | PERF_EXPIRE_DEBUG |
	PERF_CPUFREQ_LOCK_DEBUG | PERF_SCREEN_ON_POLICY_DEBUG;
#else
static int debug_mask = PERF_CPUFREQ_LOCK_DEBUG | PERF_SCREEN_ON_POLICY_DEBUG;
#endif
static int param_set_debug_mask(const char *val, struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	return ret;
}
module_param_call(debug_mask, param_set_debug_mask, param_get_int,
		&debug_mask, S_IWUSR | S_IRUGO);

unsigned int get_perflock_speed(void);
unsigned int get_cpufreq_ceiling_speed(void);    /* HTC_KER_ADD, Winster @Perflock & Ceiling. */
/*static void print_active_locks(void);*/

#if 0
/* Increase cpufreq minumum frequency when screen on.
    Pull down to lowest speed when screen off. */
static unsigned int screen_off_policy_req;
static unsigned int screen_on_policy_req;
static void perflock_early_suspend(struct early_suspend *handler)
{
	unsigned long irqflags;

	spin_lock_irqsave(&policy_update_lock, irqflags);
	screen_on_policy_req = 0;
	screen_off_policy_req = 1;
#if 0
	if (screen_on_policy_req) {
		screen_on_policy_req--;
		spin_unlock_irqrestore(&policy_update_lock, irqflags);
		return;
	}
	screen_off_policy_req++;
#endif
	spin_unlock_irqrestore(&policy_update_lock, irqflags);

	if (cpufreq_policy) {
		/* update cpufreq: the real min & max frequencies are determined by perflock_notifier_call() */
		cpufreq_update_freq(0, CONFIG_PERFLOCK_SCREEN_ON_MIN, U8500_PERFLOCK_SCREEN_ON_MAX);
	}
#if 0
	prcmu_qos_force_opp(PRCMU_QOS_DDR_OPP, 25);
	prcmu_qos_force_opp(PRCMU_QOS_APE_OPP, 50);
#endif
}

static void perflock_late_resume(struct early_suspend *handler)
{
	unsigned long irqflags;
/*
 * This workaround is for hero project
 * May cause potential bug:
 * Accidentally set cpu in high freq in screen off mode.
 * senario: in screen off early suspended state, runs the following sequence:
 * 1.perflock_late_resume():acpuclk_set_rate(high freq);screen_on_pilicy_req=1;
 * 2.perflock_early_suspend():if(screen_on_policy_req) return;
 * 3.perflock_notifier_call(): only set policy's min and max
 */
#ifdef CONFIG_MACH_HERO
	/* Work around for display driver,
	 * need to increase cpu speed immediately.
	 */
	unsigned int lock_speed = get_perflock_speed() / 1000;
	if (lock_speed > CONFIG_PERFLOCK_SCREEN_ON_MIN)
		acpuclk_set_rate(lock_speed * 1000, 0);
	else
		acpuclk_set_rate(CONFIG_PERFLOCK_SCREEN_ON_MIN * 1000, 0);
#endif

	spin_lock_irqsave(&policy_update_lock, irqflags);
#if 0
	if (screen_off_policy_req) {
		screen_off_policy_req--;
		spin_unlock_irqrestore(&policy_update_lock, irqflags);
		return;
	}
	screen_on_policy_req++;
#endif
	screen_on_policy_req = 1;
	screen_off_policy_req = 0;
	spin_unlock_irqrestore(&policy_update_lock, irqflags);

	if (cpufreq_policy) {
		/* update cpufreq: the real min & max frequencies are determined by perflock_notifier_call() */
		cpufreq_update_freq(0, CONFIG_PERFLOCK_SCREEN_ON_MIN, U8500_PERFLOCK_SCREEN_ON_MAX);
	}
#if 0
	prcmu_qos_force_opp(PRCMU_QOS_DDR_OPP, 100);
	prcmu_qos_force_opp(PRCMU_QOS_APE_OPP, 100);
#endif
}

static struct early_suspend perflock_power_suspend = {
	.suspend = perflock_early_suspend,
	.resume = perflock_late_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 10,
};
#if 0
/* 7k projects need to raise up cpu freq before panel resume for stability */
#if defined(CONFIG_HTC_ONMODE_CHARGING) && \
	(defined(CONFIG_ARCH_MSM7225) || \
	defined(CONFIG_ARCH_MSM7227) || \
	defined(CONFIG_ARCH_MSM7201A))
static struct early_suspend perflock_onchg_suspend = {
	.suspend = perflock_early_suspend,
	.resume = perflock_late_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
};
#endif
#endif
static int __init perflock_screen_policy_init(void)
{
	register_early_suspend(&perflock_power_suspend);
/* 7k projects need to raise up cpu freq before panel resume for stability */
#if 0
#if defined(CONFIG_HTC_ONMODE_CHARGING) && \
	(defined(CONFIG_ARCH_MSM7225) || \
	defined(CONFIG_ARCH_MSM7227) || \
	defined(CONFIG_ARCH_MSM7201A))
	register_onchg_suspend(&perflock_onchg_suspend);
#endif
#endif
	screen_on_policy_req++;
	if (cpufreq_policy)
		cpufreq_update_policy(cpufreq_policy->cpu);

	return 0;
}

late_initcall(perflock_screen_policy_init);
#endif

#if 0
static unsigned int policy_min = CONFIG_MSM_CPU_FREQ_ONDEMAND_MIN;
static unsigned int policy_max = CONFIG_MSM_CPU_FREQ_ONDEMAND_MAX;
#else
static unsigned int policy_min;
static unsigned int policy_max;
#endif
static int param_set_cpu_min_max(const char *val, struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	if (cpufreq_policy)
		cpufreq_update_policy(cpufreq_policy->cpu);
	return ret;
}

module_param_call(min_cpu_khz, param_set_cpu_min_max, param_get_int,
	&policy_min, S_IWUSR | S_IRUGO);
module_param_call(max_cpu_khz, param_set_cpu_min_max, param_get_int,
	&policy_max, S_IWUSR | S_IRUGO);

static int perflock_notifier_call(struct notifier_block *self,
			       unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	//unsigned int lock_speed = 0;
	unsigned long irqflags;

	spin_lock_irqsave(&policy_update_lock, irqflags);
	if (debug_mask & PERF_CPUFREQ_NOTIFY_DEBUG)
		pr_info("%s: event=%ld, policy->min=%d, policy->max=%d",
			__func__, event, policy->min, policy->max);

	if (event == CPUFREQ_START){
		cpufreq_policy = policy;/* HTC_KER_MARK, Sai_Shen one bug that only cpu3 init */
        }
	else if (event == CPUFREQ_NOTIFY) {
		/* Each time cpufreq_update_policy,
		 * min/max will reset, need to set it again. */
		/* HTC_KER_MOD, End, Winster @Perflock & Ceiling. */

		/* curr_lock_speed = lock_speed; */     /* HTC_KER_MARK, Winster @Perflock & Ceiling. */

	}
	spin_unlock_irqrestore(&policy_update_lock, irqflags);

	return 0;
}

static struct notifier_block perflock_notifier = {
	.notifier_call = perflock_notifier_call,
};

unsigned int get_perflock_speed(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;
	unsigned int perf_level = 0;

	/* Get the maxmimum perf level. */
	if (list_empty(&active_perf_locks))
		return 0;

	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_perf_locks, link) {
		if (lock->level > perf_level)
			perf_level = lock->level;
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
    return perf_acpu_table[perf_level];
}

/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
unsigned int get_cpufreq_ceiling_speed(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;
	unsigned int perf_level = 0;

	/* Get the maxmimum perf level. */
	if (list_empty(&active_cpufreq_ceiling_locks))
		return 0;
	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
		if (lock->level > perf_level)
			perf_level = lock->level;
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
	return cpufreq_ceiling_acpu_table[perf_level];
}
/*HTC_KER_ADD, End, Winster @Perflock & Ceiling. */

#if 0
static void print_active_locks(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;

	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_perf_locks, link) {
		pr_info("active perf lock '%s'\n", lock->name);
	}
	/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
	list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
		pr_info("active cpufreq_ceiling_locks '%s'\n", lock->name);
	}
	/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */
	spin_unlock_irqrestore(&list_lock, irqflags);
}
#endif

/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
void htc_print_active_perf_locks(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;

	spin_lock_irqsave(&list_lock, irqflags);
	if (!list_empty(&active_perf_locks)) {
		pr_info("perf_lock:");
		list_for_each_entry(lock, &active_perf_locks, link) {
			pr_info(" '%s' ", lock->name);
		}
		pr_info("\n");
	}
	if (!list_empty(&active_cpufreq_ceiling_locks)) {
		printk(KERN_WARNING"ceiling_lock:");
		list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
			printk(KERN_WARNING" '%s' ", lock->name);
		}
		pr_info("\n");
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}
/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */

/**
 * perf_lock_init - acquire a perf lock
 * @lock: perf lock to acquire
 * @type: the type of @lock
 * @level: performance level of @lock
 * @name: the name of @lock
 *
 * Acquire @lock with @name and @level. (It doesn't activate the lock.)
 */
void perf_lock_init(struct perf_lock *lock, unsigned int type,
			unsigned int level, const char *name)
{
	unsigned long irqflags = 0;

	WARN_ON(!name);
	WARN_ON(level >= PERF_LOCK_INVALID);
	WARN_ON(lock->flags & PERF_LOCK_INITIALIZED);

	if ((!name) || (level >= PERF_LOCK_INVALID) ||
			(lock->flags & PERF_LOCK_INITIALIZED)) {
		pr_err("%s: ERROR \"%s\" flags %x level %d\n",
			__func__, name, lock->flags, level);
		return;
	}
	lock->name = name;
	lock->flags = PERF_LOCK_INITIALIZED;
	lock->level = level;
	lock->type = type;

	INIT_LIST_HEAD(&lock->link);
	spin_lock_irqsave(&list_lock, irqflags);
	/* HTC_KER_MOD, Start, Winster @Perflock & Ceiling. */
	if (lock->type == TYPE_PERF_LOCK)
		list_add(&lock->link, &inactive_perf_locks);
	if (lock->type == TYPE_CPUFREQ_CEILING)
		list_add(&lock->link, &inactive_cpufreq_ceiling_locks);
	/* HTC_KER_MOD, End, Winster @Perflock & Ceiling. */
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(perf_lock_init);

/**
 * perf_lock - activate a perf lock
 * @lock: perf lock to activate
 *
 * Activate @lock.(Need to init_perf_lock before activate)
 */
void perf_lock(struct perf_lock *lock)
{
	unsigned long irqflags;

	WARN_ON(!initialized);
	WARN_ON((lock->flags & PERF_LOCK_INITIALIZED) == 0);
	WARN_ON(lock->flags & PERF_LOCK_ACTIVE);
	/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
	if (lock->type == TYPE_PERF_LOCK) {
		WARN_ON(!initialized);
		if (!initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because perflock is not initialized\n", __func__);
			return;
		}
	} else if (lock->type == TYPE_CPUFREQ_CEILING) {
		WARN_ON(!cpufreq_ceiling_initialized);
		if (!cpufreq_ceiling_initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because cpufreq_ceiling is not initialized\n", __func__);
			return;
		}
	}
	/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */

	spin_lock_irqsave(&list_lock, irqflags);
	/* HTC_KER_MOD, Start, Winster @Perflock & Ceiling. */
	if (debug_mask & PERF_LOCK_DEBUG)
		pr_info("%s: '%s', flags %d level %d type %u\n",
			__func__, lock->name, lock->flags, lock->level, lock->type);
	if (lock->flags & PERF_LOCK_ACTIVE) {
		pr_err("%s:type(%u) over-locked\n", __func__, lock->type);
		spin_unlock_irqrestore(&list_lock, irqflags);
		return;
	}
	/* HTC_KER_MOD, End, Winster @Perflock & Ceiling. */
	lock->flags |= PERF_LOCK_ACTIVE;
	list_del(&lock->link);
	/* HTC_KER_MOD, Start, Winster @Perflock & Ceiling. */
	if (lock->type == TYPE_PERF_LOCK)
		list_add(&lock->link, &active_perf_locks);
	else if (lock->type == TYPE_CPUFREQ_CEILING)
		list_add(&lock->link, &active_cpufreq_ceiling_locks);
	/* HTC_KER_MOD, End, Winster @Perflock & Ceiling. */
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(perf_lock);

/**
 * perf_unlock - de-activate a perf lock
 * @lock: perf lock to de-activate
 *
 * de-activate @lock.
 */
void perf_unlock(struct perf_lock *lock)
{
	unsigned long irqflags;

	WARN_ON(!initialized);
	WARN_ON((lock->flags & PERF_LOCK_ACTIVE) == 0);
	/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
	if (lock->type == TYPE_PERF_LOCK) {
		WARN_ON(!initialized);
		if (!initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because perflock is not initialized\n", __func__);
			return;
		}
	}
	if (lock->type == TYPE_CPUFREQ_CEILING) {
		WARN_ON(!cpufreq_ceiling_initialized);
		if (!cpufreq_ceiling_initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because cpufreq_ceiling is not initialized\n", __func__);
			return;
		}
	}
	/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */

	spin_lock_irqsave(&list_lock, irqflags);
	if (debug_mask & PERF_LOCK_DEBUG)
		pr_info("%s: '%s', flags %d level %d\n",
			__func__, lock->name, lock->flags, lock->level);
	if (!(lock->flags & PERF_LOCK_ACTIVE)) {
		pr_err("%s: under-locked\n", __func__);
		spin_unlock_irqrestore(&list_lock, irqflags);   /* HTC_KER_ADD, Winster @Perflock & Ceiling. */
		return;
	}
	lock->flags &= ~PERF_LOCK_ACTIVE;
	list_del(&lock->link);
	/* HTC_KER_MOD, Start, Winster @Perflock & Ceiling. */
	if (lock->type == TYPE_PERF_LOCK)
		list_add(&lock->link, &inactive_perf_locks);
	else if (lock->type == TYPE_CPUFREQ_CEILING)
		list_add(&lock->link, &inactive_cpufreq_ceiling_locks);
	/* HTC_KER_MOD, End, Winster @Perflock & Ceiling. */

	spin_unlock_irqrestore(&list_lock, irqflags);
	/* HTC_KER_MOD, End, Winster @Perflock & Ceiling. */
}
EXPORT_SYMBOL(perf_unlock);

/**
 * is_perf_lock_active - query if a perf_lock is active or not
 * @lock: target perf lock
 * RETURN: 0: inactive; 1: active
 *
 * query if @lock is active or not
 */
inline int is_perf_lock_active(struct perf_lock *lock)
{
	return (lock->flags & PERF_LOCK_ACTIVE);
}
EXPORT_SYMBOL(is_perf_lock_active);

/**
 * is_perf_locked - query if there is any perf lock activates
 * RETURN: 0: no perf lock activates 1: at least a perf lock activates
 */
int is_perf_locked(void)
{
	return (!list_empty(&active_perf_locks));
}
EXPORT_SYMBOL(is_perf_locked);

/**
 * perflock_find - find perflock by given name
 * INPUT: name   - string name of the perflock
 * RETURN: lock  - pointer of the perflock
 */
static struct perf_lock *perflock_find(const char *name)
{
	struct perf_lock *lock;
	unsigned long irqflags;

	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_perf_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	list_for_each_entry(lock, &inactive_perf_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	list_for_each_entry(lock, &inactive_cpufreq_ceiling_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);

	return NULL;
}

/**
 * perflock_acquire - acquire a perflock
 * INPUT: name      - string name of this perflock
 * RETURN: lock     - pointer of this perf_lock
 */
struct perf_lock *perflock_acquire(const char *name)
{
	struct perf_lock *lock = NULL;

	lock = perflock_find(name);
	if(lock)
		return lock;

	lock = kzalloc(sizeof(struct perf_lock), GFP_KERNEL);
	if(!lock) {
		pr_err("%s: fail to alloc perflock %s\n", __func__, name);
		return NULL; //ENOMEM
	}
	lock->name = name;
	/* Caller MUST init this lock before using it!! */
	lock->flags = 0; //None-INITIALIZED

	return lock;
}
EXPORT_SYMBOL(perflock_acquire);

/**
 * perflock_release - release a perflock
 * INPUT: name - the string name of this perf_lock
 * RETURN: 0   - successful
 *     -ENOMEM - no memory ??
 */
int perflock_release(const char *name)
{
	struct perf_lock *lock = NULL;
	unsigned long irqflags;

	lock = perflock_find(name);
	if(!lock)
		return -ENODEV;

	/* Unlock (move to inactive list), delete and kfree it */
	if(is_perf_lock_active(lock))
		perf_unlock(lock);

	spin_lock_irqsave(&list_lock, irqflags);
	list_del(&lock->link);
	spin_unlock_irqrestore(&list_lock, irqflags);
	kfree(lock);
	return 0;
}
EXPORT_SYMBOL(perflock_release);
#ifdef CONFIG_PERFLOCK_BOOT_LOCK
/* Stop cpufreq and lock cpu, shorten boot time. */
#define BOOT_LOCK_TIMEOUT	(60 * HZ)
static struct perf_lock boot_perf_lock;

static void do_expire_boot_lock(struct work_struct *work)
{
	perf_unlock(&boot_perf_lock);
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
	down_all_cpu(false);
#else
	down_all_cpu();
#endif
	pr_info("Release 'boot-time' perf_lock\n");
}
static DECLARE_DELAYED_WORK(work_expire_boot_lock, do_expire_boot_lock);
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
#define TEMP_HANDLE_TIMEOUT	(10 * HZ)
static void high_temp_handle(struct work_struct *work)
{
	int temp=0;
	temp = sprd_thm_temp_read(SPRD_ARM_SENSOR);
	pr_info("Now the temp is %d\n",temp);
	if (temp > 80)
		down_all_cpu(true);
}
static DECLARE_DELAYED_WORK(work_handle_high_temp, high_temp_handle);
#endif
#endif

static void perf_acpu_table_fixup(void)
{
	int i;
	for (i = 0; i < table_size; ++i) {
		if (perf_acpu_table[i] > policy_max * 1000)
			perf_acpu_table[i] = policy_max * 1000;
		else if (perf_acpu_table[i] < policy_min * 1000)
			perf_acpu_table[i] = policy_min * 1000;
	}

	if (table_size >= 1)
		if (perf_acpu_table[table_size - 1] < policy_max * 1000)
			perf_acpu_table[table_size - 1] = policy_max * 1000;
}

/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
static void cpufreq_ceiling_acpu_table_fixup(void)
{
	int i;
	for (i = 0; i < table_size; ++i) {
		if (cpufreq_ceiling_acpu_table[i] > policy_max * 1000)
			cpufreq_ceiling_acpu_table[i] = policy_max * 1000;
		else if (cpufreq_ceiling_acpu_table[i] < policy_min * 1000)
			cpufreq_ceiling_acpu_table[i] = policy_min * 1000;
	}
}
/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */


void __init perflock_init(struct perflock_platform_data *pdata)
{
	struct cpufreq_policy policy;
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
	struct cpufreq_frequency_table table[] = {
		{ 0, FREQ_LOWEST },
		{ 1, FREQ_MEDIUM },
		{ 2, FREQ_HIGH },
		{ 3, FREQ_HIGHEST },
		{ 4, CPUFREQ_TABLE_END },
	};
#else
	struct cpufreq_frequency_table table[] = {
		{ 0, FREQ_LOWEST },
		{ 1, FREQ_MEDIUM },
		{ 2, FREQ_HIGH },
		{ 3, CPUFREQ_TABLE_END },
	};
#endif
	BUG_ON(cpufreq_frequency_table_cpuinfo(&policy, table));
	policy_min = policy.cpuinfo.min_freq;
	policy_max = policy.cpuinfo.max_freq;

	if (!pdata)
		goto invalid_config;

	perf_acpu_table = pdata->perf_acpu_table;
	table_size = pdata->table_size;
	if (!perf_acpu_table || !table_size)
		goto invalid_config;
	if (table_size < PERF_LOCK_INVALID)
		goto invalid_config;

	perf_acpu_table_fixup();
	cpufreq_register_notifier(&perflock_notifier, CPUFREQ_POLICY_NOTIFIER);

	initialized = 1;
	pr_info("perflock floor init done\n");

#ifdef CONFIG_PERFLOCK_BOOT_LOCK
	/* Stop cpufreq and lock cpu, shorten boot time. */
	up_all_cpu();
	perf_lock_init(&boot_perf_lock, TYPE_PERF_LOCK, PERF_LOCK_HIGHEST, "boot-time");
	perf_lock(&boot_perf_lock);
	schedule_delayed_work(&work_expire_boot_lock, BOOT_LOCK_TIMEOUT);
	pr_info("Acquire 'boot-time' perf_lock\n");
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
	schedule_delayed_work(&work_handle_high_temp, TEMP_HANDLE_TIMEOUT);
#endif
#endif

	return;

invalid_config:
	pr_err("%s: invalid configuration data, %p %d %d\n", __func__,
		perf_acpu_table, table_size, PERF_LOCK_INVALID);
}

/* HTC_KER_ADD, Start, Winster @Perflock & Ceiling. */
void __init cpufreq_ceiling_init(struct perflock_platform_data *pdata)
{
	struct cpufreq_policy policy;
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
	struct cpufreq_frequency_table table[] = {
		{ 0, FREQ_LOWEST },
		{ 1, FREQ_MEDIUM },
		{ 2, FREQ_HIGH },
		{ 3, FREQ_HIGHEST },
		{ 4, CPUFREQ_TABLE_END },
	};
#else
	struct cpufreq_frequency_table table[] = {
		{ 0, FREQ_LOWEST },
		{ 1, FREQ_MEDIUM },
		{ 2, FREQ_HIGH },
		{ 3, CPUFREQ_TABLE_END },
	};
#endif
	BUG_ON(cpufreq_frequency_table_cpuinfo(&policy, table));
	policy_min = policy.cpuinfo.min_freq;
	policy_max = policy.cpuinfo.max_freq;

	if (!pdata)
		goto invalid_config;

	cpufreq_ceiling_acpu_table = pdata->perf_acpu_table;
	table_size = pdata->table_size;
	if (!cpufreq_ceiling_acpu_table || !table_size)
		goto invalid_config;
	if (table_size < PERF_LOCK_INVALID)
		goto invalid_config;

	cpufreq_ceiling_acpu_table_fixup();

	cpufreq_ceiling_initialized = 1;
	pr_info("perflock ceiling init done\n");
	return;

invalid_config:
	pr_err("%s: invalid configuration data, %p %d %d\n", __func__,
		cpufreq_ceiling_acpu_table, table_size, PERF_LOCK_INVALID);
}
/* HTC_KER_ADD, End, Winster @Perflock & Ceiling. */


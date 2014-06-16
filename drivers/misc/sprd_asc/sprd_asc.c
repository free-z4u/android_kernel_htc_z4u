/*
 * =====================================================================================
 *
 *       Filename:  sprd_asc.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/13/2013 09:44:52 AM
 *       Revision:  none
 *       Compiler:  arm-eabi-gcc
 *
 *         Author:  chuck_huang
 *        Company:  hTc
 *
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <linux/sprd_asc.h>

#include <asm/gpio.h>
#include <asm/uaccess.h>


#define LOG_ERR		0
#define LOG_WARN	1
#define LOG_INFO	2
#define LOG_NOTICE	3
#define LOG_DEBUG	4

#define ASCLOG(lvl,x...) do{ \
	if(lvl < (sprd_asc_debug + 1)) \
		printk("[SPRD_ASC] " x); \
	} while (0)


extern int board_mfg_mode(void);

static int sprd_asc_debug = 4;

enum {
	MODEM_ASSERT = 1,	/* Modem Assert */
	MODEM_ATC_READY,	/* Atc Ready */
	RESERVE,		/* Reserve */
	GPIO27_UP,		/* PULL GPIO27 UP */
	GPIO27_DOWN,		/* PULL GPIO27 DOWN */
};

static void trigger_cp_stop(struct sprd_asc_host *host)
{
	struct sprd_asc_platform_data *plat = NULL;

	if (host == NULL){
		ASCLOG(LOG_ERR, "%s @ %d, error parameter\n", __func__, __LINE__);
		return;
	}

	ASCLOG(LOG_INFO, "%s @ %d!\n", __func__, __LINE__);
	plat = host->plat;

#ifdef CONFIG_MACH_Z4TD
	__raw_writel(0x02, host->vbase + SZ_1M - SZ_4K - SZ_2K - SZ_1K);
#else
	__raw_writel(0x02, host->vbase + SZ_1M - SZ_8K - SZ_2K - SZ_1K);
#endif

	plat->ntfirq_trigger();
	msleep(200);

	ASCLOG(LOG_INFO, "%s %s: end!\n", __func__, plat->devname);

}
static int sprd_asc_clean_modem_mem(struct sprd_asc_host *host, struct sprd_asc_platform_data *plat)
{
#ifdef CONFIG_MODEM_SILENT_RESET
	void *vmem;
	uint32_t size;

	ASCLOG(LOG_INFO, "%s %s: begin!\n", __func__, plat->devname);
	vmem = host->vbase;
	size = plat->maxsz;
	memset(vmem, 0, size);
	ASCLOG(LOG_INFO, "%s %s: modem clean buf finished!\n", __func__, plat->devname);
#endif
	return 0;
}

static unsigned int sprd_asc_slot_status(struct sprd_asc_host *host)
{
	unsigned int status = -1;

	if ((host->plat->status_gpio != -1))
		status = gpio_get_value(host->plat->status_gpio);

	return status;
}

#ifdef CONFIG_SIM_SWITCH
static void sprd_asc_simswitch_event(struct work_struct *work)
{
	char *envp_switch_on[] = {"SIM_SWITCH_ON", NULL};
	char *envp_switch_off[] = {"SIM_SWITCH_OFF", NULL};
	struct sprd_asc_host *host =
			container_of(work, struct sprd_asc_host, simswitch_work);

	if (!host)
		return;

	if (host->sim_switchstat == 1) {
		printk("%s: %s\n", __func__, envp_switch_on[0]);
		kobject_uevent_env(host->sprd_asc_kobj, KOBJ_CHANGE, envp_switch_on);
	} else if (host->sim_switchstat == 0) {
		printk("%s: %s\n", __func__, envp_switch_off[0]);
		kobject_uevent_env(host->sprd_asc_kobj, KOBJ_CHANGE, envp_switch_off);
	}
}
#endif

static void sprd_asc_simstat_event(struct work_struct *work)
{
	char *envp_sim_in[] = {"SIM_PLUG_IN", NULL};
	char *envp_sim_out[] = {"SIM_PLUG_OUT", NULL};
	unsigned int status;
	struct sprd_asc_platform_data *plat;
	struct sprd_asc_host *host =
			container_of(work, struct sprd_asc_host, simstat_work.work);

	if (!host)
		return;

	plat = host->plat;
	if (!plat)
		return;

	status = sprd_asc_slot_status(host);

	if (host->oldstat != status) {
		host->oldstat = status;
	} else {
		ASCLOG(LOG_INFO, "%s: find same status %d\n, and filter it",
					__func__, status);
		return;
	}

	if (status == 0) {
		ASCLOG(LOG_INFO, "%s: %s\n", plat->devname, envp_sim_out[0]);
		kobject_uevent_env(host->sprd_asc_kobj, KOBJ_CHANGE, envp_sim_out);
       } else {
		ASCLOG(LOG_INFO, "%s: %s\n", plat->devname, envp_sim_in[0]);
		kobject_uevent_env(host->sprd_asc_kobj, KOBJ_CHANGE, envp_sim_in);
	}
}


static irqreturn_t sim_irq_handler(int irq, void *dev_id)
{
	struct sprd_asc_host *host = dev_id;
	unsigned int delay = 600;

	if (!host)
		return IRQ_HANDLED;

	if (!host->plat)
		return IRQ_HANDLED;

	ASCLOG(LOG_INFO, "%s @ %s\n", host->plat->devname, __func__);
	if (host->event_wq != NULL){
		if (sprd_asc_slot_status(host) == 0){
			delay = 400;
		}
		queue_delayed_work(host->event_wq, &host->simstat_work, msecs_to_jiffies(delay));
	}

	return IRQ_HANDLED;
}


static void sprd_asc_assert_event(struct work_struct *work)
{
	struct sprd_asc_host *host =
			container_of(work, struct sprd_asc_host, assert_work);
	char *envp[] = {"MDM_ASSERT", NULL};

#define ASSERT_MSG_LEN	128
	char assert_msg[ASSERT_MSG_LEN];
	int i;

	if (*(char *)(host->vbase + SZ_1K) == '@') { // check valid head '@'
		for (i = 0; i < ASSERT_MSG_LEN; i++) {
			assert_msg[i] = *(char *)(host->vbase + SZ_1K + i);
			if (i && assert_msg[i] == '@') { /* reach msg end */
				assert_msg[i] = '\0';
				break;
			}
		}
		assert_msg[ASSERT_MSG_LEN - 1] = '\0';
		ASCLOG(LOG_INFO, "%s assert msg: %s\n", host->plat? host->plat->devname: "", assert_msg);
	}

	kobject_uevent_env(host->sprd_asc_kobj, KOBJ_CHANGE, envp);
}


static irqreturn_t cp_notify_irq(int irq, void *dev_id)
{
	struct sprd_asc_host *host;
	struct sprd_asc_platform_data *plat;
	u8 msg;

	host = (struct sprd_asc_host *) dev_id;
	plat = host->plat;

	ASCLOG(LOG_INFO, "%s: %s @ %d\n", plat->devname, __func__, __LINE__);

	plat->ntfirq_clear();

#ifdef CONFIG_MACH_Z4TD
	msg = *(u8*)(host->vbase + SZ_1M - SZ_4K - SZ_4K);
#else
	msg = *(u8*)(host->vbase + SZ_1M - SZ_8K - SZ_4K);
#endif


	if (msg <= 0)
		ASCLOG(LOG_ERR, "unknow cp notify msg\n");


	switch (msg) {
		case MODEM_ASSERT:
		ASCLOG(LOG_INFO, "%s assert!\n", plat->devname);
		host->power_stat = 2;
		queue_work(host->event_wq, &host->assert_work);
		break;
		case MODEM_ATC_READY:
		ASCLOG(LOG_INFO, "%s at ready.\n", plat->devname);
		host->atc_ready = 1;
		break;
#ifdef CONFIG_AP_CTRL_RF
		case GPIO27_UP:
		ASCLOG(LOG_INFO, "%s set gpio27 high.\n", plat->devname);
		gpio_direction_output(GPIO27_GPIO, 1);
		break;
		case GPIO27_DOWN:
		ASCLOG(LOG_INFO, "%s set gpio27 low.\n", plat->devname);
		gpio_direction_output(GPIO27_GPIO, 0);
		break;
#endif
		default:
		break;
	}

	return IRQ_HANDLED;
}


static irqreturn_t cp_watchdog_irq(int irq, void *dev_id)
{
	ASCLOG(LOG_INFO, "%s @ %d\n", __func__, __LINE__);

	return IRQ_HANDLED;
}


static ssize_t cp_power_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct platform_device *sprd_asc_dev = to_platform_device(dev);
	struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);

	char *s = buf;
	s += sprintf(s, "%d\n", host->power_stat);

	return (s - buf);
}


static ssize_t cp_power_set(struct device *dev,struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *sprd_asc_dev = to_platform_device(dev);
	struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);
	struct sprd_asc_platform_data *plat = host->plat;

	if (count > 0) {
		switch (buf[0]) {
		case '0':
		/*set_modem_power(0);*/
		ASCLOG(LOG_INFO, "%s: modem power off begin! pid %d tskname %s\n", __func__, current->pid, current->comm);
		if(host->power_stat == 1){
			trigger_cp_stop(host);
		}
		plat->stop(NULL);
		msleep(100);/* in order to make sure modem really power off */
		plat->reset();/*reset sbuf and sblock*/
		msleep(100);
		host->atc_ready = 0;
		host->power_stat = 0;
		ASCLOG(LOG_INFO, "%s: modem power off successful!\n", __func__);
		sprd_asc_clean_modem_mem(host, plat);
		break;

		case '1':
		plat->restart();
		plat->start(NULL);
		host->power_stat = 1;
		ASCLOG(LOG_INFO, "%s: modem power on successful!\n", __func__);
		break;

		default:
		break;
		}
	}

	return count;
}

static const DEVICE_ATTR(cp_power, S_IRUGO|S_IWUSR, cp_power_get, cp_power_set);


static ssize_t cp_bootrdy_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct platform_device *sprd_asc_dev = to_platform_device(dev);
	struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);

	char *s = buf;
	s += sprintf(s, "%d\n", host->atc_ready);

	return (s - buf);
}

static const DEVICE_ATTR(cp_bootrdy, S_IRUGO, cp_bootrdy_get, NULL);

static ssize_t cp_simstat_get(struct device *dev, struct device_attribute *attr,
                               char *buf)
{
       struct platform_device *sprd_asc_dev = to_platform_device(dev);
       struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);

       char *s = buf;
       s += sprintf(s, "%d\n", host->oldstat);

       return (s - buf);
}

static const DEVICE_ATTR(cp_simstat, S_IRUGO, cp_simstat_get, NULL);


static unsigned long addr = 0;

static ssize_t cp_dbgstat_set(struct device *dev,struct device_attribute *attr,
				const char *buf, size_t count)
{
	ASCLOG(LOG_INFO, "%s\n", __func__);

	if (strict_strtoul(buf, 16, &addr))
		return -EINVAL;

	ASCLOG(LOG_INFO, "%s @ addr = 0x%08x\n", __func__, (unsigned int)addr);

	return count;
}

static ssize_t cp_dbgstat_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct platform_device *sprd_asc_dev = to_platform_device(dev);
	struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);
	struct sprd_asc_platform_data *plat = host->plat;
	u32 state;

	char *s = buf;

	if (!plat || !plat->get_cp_stat)
		return -ENXIO;

	ASCLOG(LOG_INFO, "%s @ addr = 0x%08x\n", __func__,(unsigned int)addr);

	state = plat->get_cp_stat(addr);
	if (state < 0) {
		return -EINVAL;
	}

	ASCLOG(LOG_INFO, "%s @ reg[0x%08x] = 0x%08x\n", __func__, (unsigned int)addr, state);

	s += sprintf(s, "0x%08x\n", state);

	return (s - buf);
}

static const DEVICE_ATTR(cp_dbgstat, S_IRUGO|S_IWUSR, cp_dbgstat_get, cp_dbgstat_set);

#ifdef CONFIG_SIM_SWITCH
static ssize_t cp_simswitch_set(struct device *dev,struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *sprd_asc_dev = to_platform_device(dev);
	struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);
	struct sprd_asc_platform_data *plat = host->plat;

	if (count > 0) {
		switch (buf[0]) {
		case '0':
		if (!host->sim_switchstat)
			ASCLOG(LOG_INFO, "already turn off sim switch");
		else {
			ASCLOG(LOG_INFO, "turn off sim switch");
			plat->simswitch(0);
#ifdef CONFIG_VIA_SIM_SWITCH
			gpio_direction_output(GPIO29_GPIO, 1);
#else
			gpio_direction_output(GPIO29_GPIO, 0);
#endif
			host->sim_switchstat = 0;
			queue_work(host->event_wq, &host->simswitch_work);
		}
		break;

		case '1':
		if (!host->sim_switchstat) {
			ASCLOG(LOG_INFO, "turn on sim switch");
			plat->simswitch(1);
#ifdef CONFIG_VIA_SIM_SWITCH
			gpio_direction_output(GPIO29_GPIO, 0);
#else
			gpio_direction_output(GPIO29_GPIO, 1);
#endif
			host->sim_switchstat = 1;
			queue_work(host->event_wq, &host->simswitch_work);
		} else
			ASCLOG(LOG_INFO, "already turn on sim switch");
		break;

		default:
		break;
		}
	}

	return count;
}


static ssize_t cp_simswitch_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct platform_device *sprd_asc_dev = to_platform_device(dev);
	struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);

	char *s = buf;
	s += sprintf(s, "%d\n", host->sim_switchstat);

	return (s - buf);
}

static const DEVICE_ATTR(cp_simswitch, S_IRUGO|S_IWUSR, cp_simswitch_get, cp_simswitch_set);
#endif

static void trigger_cp_assert(struct sprd_asc_host *host)
{
	struct sprd_asc_platform_data *plat = host->plat;

	ASCLOG(LOG_INFO, "%s @ %d!\n", __func__, __LINE__);

#ifdef CONFIG_MACH_Z4TD
	__raw_writel(0x01, host->vbase + SZ_1M - SZ_4K - SZ_2K - SZ_1K);
#else
	__raw_writel(0x01, host->vbase + SZ_1M - SZ_8K - SZ_2K - SZ_1K);
#endif

	plat->ntfirq_trigger();
}

static ssize_t trigger_assert_set(struct device *dev,struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *sprd_asc_dev = to_platform_device(dev);
	struct sprd_asc_host *host = platform_get_drvdata(sprd_asc_dev);

	if (!host)
		return -ENXIO;

	if (count > 0 && buf[0] == '1') {
		trigger_cp_assert(host);
	}

	return count;
}

static const DEVICE_ATTR(trigger_assert, S_IWUSR, NULL, trigger_assert_set);

static const struct attribute *sprd_asc_attrs[] = {
	&dev_attr_cp_power.attr,
	&dev_attr_cp_bootrdy.attr,
	&dev_attr_cp_simstat.attr,
	&dev_attr_trigger_assert.attr,
	NULL,
};


static int sprd_asc_proc_open(struct inode *inode, struct file *filp)
{
	struct sprd_asc_proc_entry *entry = (struct sprd_asc_proc_entry *)PDE(inode)->data;

	filp->private_data = entry;

	return 0;
}

static int sprd_asc_proc_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;

	return 0;
}

static ssize_t sprd_asc_proc_read(struct file *filp, char __user *buf,
					size_t count, loff_t *ppos)
{
	struct sprd_asc_platform_data *plat;
	struct sprd_asc_proc_entry *entry;
	struct sprd_asc_host *host;
	uint32_t base, size, offset;
	char *type;
	void *vmem;

	entry = (struct sprd_asc_proc_entry *)filp->private_data;
	type = entry->name;
	host = entry->host;
	plat = host->plat;

	if (!strcmp(type, "smem") || !strcmp(type, "modem")) {
		base = plat->segs[0].base;
		size = plat->segs[0].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "dsp")){
		base = plat->segs[1].base;
		size = plat->segs[1].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "dlnv")){
		base = plat->segs[2].base;
		size = plat->segs[2].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "fixnv")){
		base = plat->segs[3].base;
		size = plat->segs[3].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "runtimenv")){
		base = plat->segs[4].base;
		size = plat->segs[4].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "protnv")){
		base = plat->segs[5].base;
		size = plat->segs[5].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "rfnv")){
		base = plat->segs[6].base;
		size = plat->segs[6].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "c_cmdline")){
		base = plat->segs[7].base;
		size = plat->segs[7].maxsz;
		offset = *ppos;
	} else if (!strcmp(type, "n_cmdline")){
		base = plat->segs[8].base;
		size = plat->segs[8].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "smsg") == 0) {
		base = plat->segs[9].base;
		size = plat->segs[9].maxsz;
		offset = *ppos;
	} else {
		return -EINVAL;
	}

	if (size < *ppos)
		return -EINVAL;
	if (size == *ppos)
		return 0;

	if ((*ppos + count) > size) {
		count = size - *ppos;
	}

	ASCLOG(LOG_INFO, "devname %s type %s read: 0x%08x, 0x%08x!\n",
				plat->devname, type, base + offset, count);
	vmem = host->vbase + (base - plat->base) + offset;
	if (copy_to_user(buf, vmem, count)) {
		return -EFAULT;
	}

	*ppos += count;

	return count;
}


static ssize_t sprd_asc_proc_write(struct file *filp,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct sprd_asc_platform_data *plat;
	struct sprd_asc_proc_entry *entry;
	struct sprd_asc_host *host;
	uint32_t base, size, offset;
	char *type;
	void *vmem;

	entry = (struct sprd_asc_proc_entry *)filp->private_data;
	type = entry->name;
	host = entry->host;
	plat = host->plat;

	if (strcmp(type, "modem") == 0) {
		base = plat->segs[0].base;
		size = plat->segs[0].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "dsp") == 0) {
		base = plat->segs[1].base;
		size = plat->segs[1].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "dlnv") == 0) {
		base = plat->segs[2].base;
		size = plat->segs[2].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "fixnv") == 0) {
		base = plat->segs[3].base;
		size = plat->segs[3].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "runtimenv") == 0) {
		base = plat->segs[4].base;
		size = plat->segs[4].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "protnv") == 0) {
		base = plat->segs[5].base;
		size = plat->segs[5].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "rfnv") == 0) {
		base = plat->segs[6].base;
		size = plat->segs[6].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "c_cmdline") == 0) {
		base = plat->segs[7].base;
		size = plat->segs[7].maxsz;
		offset = *ppos;
	} else if (strcmp(type, "n_cmdline") == 0) {
		base = plat->segs[8].base;
		size = plat->segs[8].maxsz;
		offset = *ppos;
	} else {
		return -EINVAL;
	}

	ASCLOG(LOG_INFO, "devname %s type %s write: 0x%08x, 0x%08x!\n",
				plat->devname, type, base + offset, count);

	if(offset + count > size){
		ASCLOG(LOG_WARN, "devname %s type %s write: offset = 0x%08x, count = 0x%08x, max_size = 0x%08x!\n",
				plat->devname, type, offset, count, size);
		return -EFAULT;
	}

	vmem = host->vbase + (base - plat->base) + offset;

	if (copy_from_user(vmem, buf, count)) {
		return -EFAULT;
	}

	*ppos += count;

	return count;
}


static loff_t sprd_asc_proc_lseek(struct file* filp, loff_t off, int whence)
{
	struct sprd_asc_platform_data *plat;
	struct sprd_asc_proc_entry *entry;
	struct sprd_asc_host *host;
	char *type;
	loff_t new;

	entry = (struct sprd_asc_proc_entry *)filp->private_data;
	type = entry->name;
	host = entry->host;
	plat = host->plat;

	ASCLOG(LOG_INFO, "devname %s type %s\n", plat->devname, type);

	switch (whence) {
		case 0:
		new = off;
		filp->f_pos = new;
		break;

		case 1:
		new = filp->f_pos + off;
		filp->f_pos = new;
		break;

		case 2:
		/* fix me ... */
		if (strcmp(type, "mem") == 0) {
			new = plat->maxsz + off;
			filp->f_pos = new;
		} else {
			return -EINVAL;
		}
		break;

		default:
		return -EINVAL;
	}
	return (new);
}


static unsigned int sprd_asc_proc_poll(struct file *filp, poll_table *wait)
{
	struct sprd_asc_platform_data *plat;
	struct sprd_asc_proc_entry *entry;
	struct sprd_asc_host *host;
	char *type;

	entry = (struct sprd_asc_proc_entry *)filp->private_data;
	type = entry->name;
	host = entry->host;
	plat = host->plat;

	ASCLOG(LOG_INFO, "devname %s type %s\n", plat->devname, type);

	return -EINVAL;
}


struct file_operations sprd_asc_proc_fs_fops = {
	.open		= sprd_asc_proc_open,
	.release	= sprd_asc_proc_release,
	.read		= sprd_asc_proc_read,
	.write		= sprd_asc_proc_write,
	.llseek 	= sprd_asc_proc_lseek,
	.poll		= sprd_asc_proc_poll,
};


static inline void sprd_asc_proc_fs_init(struct sprd_asc_host *host)
{
	struct sprd_asc_platform_data *plat;
	int i;

	plat = host->plat;
	host->procfs.procdir = proc_mkdir(plat->devname, NULL);

	for (i = 0; i < plat->segnr; i++) {
		host->procfs.element[i].name = plat->segs[i].name;
		host->procfs.element[i].entry =
				proc_create_data(host->procfs.element[i].name, plat->segs[i].mode,
							host->procfs.procdir, &sprd_asc_proc_fs_fops,
							&(host->procfs.element[i]));
		host->procfs.element[i].host = host;
	}
}


static inline void sprd_asc_proc_fs_exit(struct sprd_asc_host *host)
{
	struct sprd_asc_platform_data *plat;
	int segnr, i;

	if (!host)
		return;

	plat = host->plat;

	if (host->procfs.procdir && plat) {
		segnr = plat->segnr;
		for (i = 0; i < segnr; i++)
			remove_proc_entry(plat->segs[i].name, host->procfs.procdir);
		remove_proc_entry(plat->devname, NULL);
	}
}


int sprd_asc_probe(struct platform_device *pdev)
{
	struct sprd_asc_platform_data *plat;
	struct sprd_asc_host *host;
	int ret = 0;

	plat = pdev->dev.platform_data;
	if (!plat) {
		ASCLOG(LOG_ERR, "%s: Platform data is not avaliable.\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ASCLOG(LOG_INFO, "%s @ %s\n", __func__, plat->devname);

	if (pdev->id < 0) {
		ASCLOG(LOG_ERR, "%s: platform dev id is not avaliable.\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	host = kzalloc(sizeof(struct sprd_asc_host), GFP_KERNEL);
	if (!host) {
		ASCLOG(LOG_ERR, "%s: request memory failed!\n", __func__);
		host = NULL;
		ret = -ENOMEM;
		goto out;
	}

	host->pdev_id = pdev->id;
	host->plat = plat;

	host->vbase = ioremap(plat->base, plat->maxsz);
	if (!host->vbase) {
		ASCLOG(LOG_ERR, "Unable to map sprd asc base");
		ret = -ENOMEM;
		goto err_remap;
	}

	sprd_asc_proc_fs_init(host);

	host->sprd_asc_kobj = &pdev->dev.kobj;

	if (plat->devtype) {
		sprintf(host->wq_name, "%s-wq", plat->devname);
		host->event_wq = create_workqueue(host->wq_name);
		if (!host->event_wq)
			goto err_create_wq;

		INIT_WORK(&host->assert_work, sprd_asc_assert_event);
		INIT_DELAYED_WORK(&host->simstat_work, sprd_asc_simstat_event);

		if (sysfs_create_files(host->sprd_asc_kobj, sprd_asc_attrs) < 0)
			ASCLOG(LOG_WARN, "failed to create sysfs files for sprd asc t attrs.\n");

	} else {
		if (sysfs_create_file(host->sprd_asc_kobj,
						(const struct attribute *)&dev_attr_cp_dbgstat.attr) < 0)
						ASCLOG(LOG_WARN, "failed to create sysfs files for sprd slp state attr.\n");
	}

	/* register cp notify irq */
	if (plat->ntf_irq != -1) {
		ret = request_irq(plat->ntf_irq, cp_notify_irq, IRQF_NO_SUSPEND,
					plat->ntf_irqname, host);
		if (ret)
			goto err_req_ntfirq;
	}

	/* register cp watchdog irq */
	if (plat->wtd_irq != -1) {
		ret = request_irq(plat->wtd_irq, cp_watchdog_irq, IRQF_NO_SUSPEND,
					plat->wtd_irqname, host);
		if (ret)
			goto err_req_wtdirq;
	}

	if (plat->status_gpio != -1) {
		ret = gpio_request(plat->status_gpio, plat->sim_irqname);
		if (ret) {
			ASCLOG(LOG_ERR, "fail to request gpio%d!\n", plat->status_gpio);
			goto err_req_status_gpio;
		} else {
			gpio_direction_input(plat->status_gpio);
		}

		ret = request_irq(gpio_to_irq(plat->status_gpio), sim_irq_handler,
					IRQF_SHARED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
					plat->sim_irqname, host);
		if (ret)
			goto err_req_simirq;
	}

#ifdef CONFIG_AP_CTRL_RF
	if (plat->ap_ctrl_rf) {
		ret = gpio_request(GPIO27_GPIO, "RF Switch");
		if (ret) {
			ASCLOG(LOG_ERR, "fail to request gpio27 !\n");
			goto err_req_gpio27;
		} else
			gpio_direction_output(GPIO27_GPIO, 1);
	}
#endif

#ifdef CONFIG_AP_CTRL_ANT
	if (plat->ap_ctrl_ant) {
		ret = gpio_request(GPIO29_GPIO, "ANT Switch");
		if (ret) {
			ASCLOG(LOG_ERR, "fail to request gpio29 !\n");
			goto err_req_gpio29;
		} else {
#ifdef CONFIG_VIA_SIM_SWITCH
			if (board_mfg_mode() == 11)
				gpio_direction_output(GPIO29_GPIO, 0);
			else
				gpio_direction_output(GPIO29_GPIO, 1);
#else
			gpio_direction_output(GPIO29_GPIO, 0);
#endif
		}
	}
#endif

#ifdef CONFIG_SIM_SWITCH
	if (plat->sim_switch_enable == 1) {
#ifdef CONFIG_VIA_SIM_SWITCH
		ret = gpio_request(GPIO152_GPIO, "Sim Switch");
		if (ret) {
			ASCLOG(LOG_ERR, "fail to request gpio152 !\n");
			goto err_req_gpio152;
		} else {
			if (board_mfg_mode() == 11) {
				gpio_direction_output(GPIO152_GPIO, 1);
				host->sim_switchstat = 1;
			} else {
				gpio_direction_output(GPIO152_GPIO, 0);
				host->sim_switchstat = 0;
			}
		}
#else
		host->sim_switchstat = 0;
#endif
		INIT_WORK(&host->simswitch_work, sprd_asc_simswitch_event);
		if (sysfs_create_file(host->sprd_asc_kobj,
				(const struct attribute *)&dev_attr_cp_simswitch.attr) < 0)
				ASCLOG(LOG_WARN, "failed to create sysfs files for sprd simswitch attr.\n");
	}
#endif

	host->oldstat = sprd_asc_slot_status(host);
	host->power_stat = 0;

	ASCLOG(LOG_INFO, "%s slot stat %d gpio = %d\n", __func__, host->oldstat, plat->status_gpio);

	platform_set_drvdata(pdev, host);

	return 0;

#ifdef CONFIG_VIA_SIM_SWITCH
err_req_gpio152:
	if (plat->ap_ctrl_ant)
		gpio_free(GPIO29_GPIO);
#endif
#ifdef CONFIG_AP_CTRL_ANT
err_req_gpio29:
	if (plat->ap_ctrl_ant)
		gpio_free(GPIO27_GPIO);
#endif
#ifdef CONFIG_AP_CTRL_RF
err_req_gpio27:
	if (plat->status_gpio != -1)
		free_irq(gpio_to_irq(plat->status_gpio), host);
#endif
err_req_simirq:
	if (plat->status_gpio != -1)
		gpio_free(plat->status_gpio);
err_req_status_gpio:
	if (plat->wtd_irq != -1)
		free_irq(plat->wtd_irq, host);
err_req_wtdirq:
	if (plat->ntf_irq != -1)
		free_irq(plat->ntf_irq, host);
err_req_ntfirq:
	if (plat->devtype)
		destroy_workqueue(host->event_wq);
err_create_wq:
	iounmap(host->vbase);
err_remap:
	kfree(host);
	host = NULL;
out:
	return ret;
}


int sprd_asc_remove(struct platform_device *pdev)
{
	struct sprd_asc_host *host = platform_get_drvdata(pdev);
	struct sprd_asc_platform_data *plat = host->plat;

	if (plat->sim_switch_enable)
		gpio_free(GPIO152_GPIO);

#ifdef CONFIG_AP_CTRL_ANT
	if (plat->ap_ctrl_ant)
		gpio_free(GPIO29_GPIO);
#endif

#ifdef CONFIG_AP_CTRL_RF
	if (plat->ap_ctrl_rf)
		gpio_free(GPIO27_GPIO);
#endif

	if (plat->status_gpio != -1) {
		free_irq(gpio_to_irq(plat->status_gpio), host);
		gpio_free(plat->status_gpio);
	}

	if (plat->wtd_irq != -1)
		free_irq(plat->wtd_irq, host);

	if (plat->ntf_irq != -1)
		free_irq(plat->ntf_irq, host);

	if (plat->devtype)
		destroy_workqueue(host->event_wq);

	sprd_asc_proc_fs_exit(host);

	iounmap(host->vbase);

	kfree(host);

	return 0;
}


static struct platform_driver sprd_asc_driver = {
	.probe	= sprd_asc_probe,
	.remove	= sprd_asc_remove,
	.driver	= {
		.owner = THIS_MODULE,
		.name = "sprd_asc",
	},
};


static int __init sprd_asc_init(void)
{
	ASCLOG(LOG_INFO, "%s\n", SPRD_ASC_DRV_VER);
	if (platform_driver_register(&sprd_asc_driver) != 0) {
		printk(KERN_ERR "sprd_asc platform drv register Failed \n");
		return -1;
	}

	return 0;
}


static void __exit sprd_asc_exit(void)
{
	platform_driver_unregister(&sprd_asc_driver);
}


module_init(sprd_asc_init);
module_exit(sprd_asc_exit);

MODULE_DESCRIPTION("AP SYNC SPRD CP Driver");
MODULE_LICENSE("GPL");


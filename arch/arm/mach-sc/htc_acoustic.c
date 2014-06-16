/* arch/arm/mach-sc/htc_acoustic.c
 *
 * Copyright (C) 2007-2008 HTC Corporation
 * Author: Laurence Chen <Laurence_Chen@htc.com>
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
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <linux/switch.h>

#define ACOUSTIC_IOCTL_MAGIC 'p'
#define ACOUSTIC_UPDATE_BEATS_STATUS	_IOW(ACOUSTIC_IOCTL_MAGIC, 47, unsigned)
#define ACOUSTIC_UPDATE_LISTEN_STATUS	_IOW(ACOUSTIC_IOCTL_MAGIC, 48, unsigned)

static struct acoustic_ops default_acoustic_ops;
static struct acoustic_ops * the_ops = &default_acoustic_ops;

static struct switch_dev sdev_beats;
static struct switch_dev sdev_listen_notification;

static int acoustic_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -ENOSYS;
}

static int acoustic_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int acoustic_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long acoustic_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	int rc = -1;
	switch (cmd) {
	case ACOUSTIC_UPDATE_BEATS_STATUS: {
		int new_state = -1;

		if (copy_from_user(&new_state, (void *)arg, sizeof(new_state))) {
			rc = -EFAULT;
			break;
		}
		pr_info("[AUD]: Update Beats Status : %d\n", new_state);
		if (new_state < -1 || new_state > 1) {
			pr_err("[AUD]: Invalid Beats status update");
			rc = -EINVAL;
			break;
		}

		sdev_beats.state = -1;
		switch_set_state(&sdev_beats, new_state);
		rc = 0;
		break;
	}
	case ACOUSTIC_UPDATE_LISTEN_STATUS: {
		int new_state = -1;

		if (copy_from_user(&new_state, (void *)arg, sizeof(new_state))) {
			rc = -EFAULT;
			break;
		}
		pr_info("[AUD]: Update listen notification Status : %d\n", new_state);
		if (new_state < -1 || new_state > 1) {
			pr_err("[AUD]: Invalid listen notification status update");
			rc = -EINVAL;
			break;
		}

		sdev_listen_notification.state = -1;
		switch_set_state(&sdev_listen_notification, new_state);
		rc = 0;
		break;
	}
	default:
		rc = -EINVAL;
	}
	return rc;
}

static ssize_t beats_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "Beats\n");
}

static ssize_t listen_notification_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "Listen_notification\n");
}

static struct file_operations acoustic_fops = {
	.owner = THIS_MODULE,
	.open = acoustic_open,
	.release = acoustic_release,
	.mmap = acoustic_mmap,
	.unlocked_ioctl = acoustic_ioctl,
};

static struct miscdevice acoustic_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "htc-acoustic",
	.fops = &acoustic_fops,
};

static int __init acoustic_init(void)
{
	int ret;

	ret = misc_register(&acoustic_misc);
	if (ret < 0) {
		pr_err("[AUD]: failed to register misc device!\n");
		return ret;
	}
	sdev_beats.name = "Beats";
	sdev_beats.print_name = beats_print_name;

	ret = switch_dev_register(&sdev_beats);
	if (ret < 0) {
		pr_err("[AUD]: failed to register beats switch device!\n");
		return ret;
	}

	sdev_listen_notification.name = "Listen_notification";
	sdev_listen_notification.print_name = listen_notification_print_name;

	ret = switch_dev_register(&sdev_listen_notification);
	if (ret < 0) {
		switch_dev_unregister(&sdev_beats);
		pr_err("[AUD]: failed to register listen notification switch device!\n");
		return ret;
	}

	return 0;

}

static void __exit acoustic_exit(void)
{
	int ret;

	switch_dev_unregister(&sdev_beats);
	switch_dev_unregister(&sdev_listen_notification);
	ret = misc_deregister(&acoustic_misc);
	if (ret < 0)
		pr_err("[AUD]: failed to unregister misc device!\n");
}

module_init(acoustic_init);
module_exit(acoustic_exit);

MODULE_AUTHOR("Laurence Chen <Laurence_Chen@htc.com>");
MODULE_DESCRIPTION("HTC acoustic driver");
MODULE_LICENSE("GPL");

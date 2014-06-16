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
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/sipc.h>
#include <linux/spipe.h>
#include <linux/sched.h>

#define SPIPE_STATE_IDLE	0
#define SPIPE_STATE_READY	1

int sipc_suspend = 0;
EXPORT_SYMBOL(sipc_suspend);

extern int sipc_enable_irq_wake(uint8_t dst);
extern int sipc_disable_irq_wake(uint8_t dst);

struct spipe_device {
	struct spipe_init_data	*init;
	int			major;
	int			minor;
	struct kobject	*spipe_kobj;
	struct cdev		cdev;
};

struct spipe_sbuf {
	uint8_t			dst;
	uint8_t			channel;
	uint32_t		bufid;
};

static struct class		*spipe_class;

static int spipe_ready(uint8_t dst, uint8_t channel)
{
	u32 ret, state;

	state = sbuf_status(dst, channel);
	if (!state)
		ret = SPIPE_STATE_READY;
	else
		ret = SPIPE_STATE_IDLE;

	return ret;
}

static ssize_t spipe_state_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spipe_init_data *init = pdev->dev.platform_data;

	u32 state;

	char *s = buf;

	state = spipe_ready(init->dst, init->channel);

	s += sprintf(s, "%d\n", state);

	return (s - buf);
}

static const DEVICE_ATTR(spipe_state, S_IRUGO, spipe_state_get, NULL);


static int spipe_open(struct inode *inode, struct file *filp)
{
	int minor = iminor(filp->f_path.dentry->d_inode);
	struct spipe_device *spipe;
	struct spipe_sbuf *sbuf;

	spipe = container_of(inode->i_cdev, struct spipe_device, cdev);
	if (!spipe_ready(spipe->init->dst, spipe->init->channel)) {
		LOGI("spipe-%d-%d-%d not ready to open!\n", spipe->init->dst, spipe->init->channel, minor- spipe->minor);
		filp->private_data = NULL;
		return -ENODEV;
	}

	sbuf = kmalloc(sizeof(struct spipe_sbuf), GFP_KERNEL);
	if (!sbuf) {
		return -ENOMEM;
	}
	filp->private_data = sbuf;

	sbuf->dst = spipe->init->dst;
	sbuf->channel = spipe->init->channel;
	sbuf->bufid = minor - spipe->minor;

	LOGI("spipe-%d-%d-%d open successfully!\n", sbuf->dst, sbuf->channel, sbuf->bufid);
	LOGI("current id = %d, name = %s\n", current->pid, current->comm);

	return 0;
}

static int spipe_release(struct inode *inode, struct file *filp)
{
	struct spipe_sbuf *sbuf = filp->private_data;

	if (sbuf) {
		kfree(sbuf);
	}

	return 0;
}

static ssize_t spipe_read(struct file *filp,
		char __user *buf, size_t count, loff_t *ppos)
{
	struct spipe_sbuf *sbuf = filp->private_data;
	int timeout = -1;

	if (filp->f_flags & O_NONBLOCK) {
		timeout = 0;
	}

	return sbuf_read(sbuf->dst, sbuf->channel, sbuf->bufid,
			(void *)buf, count, timeout);
}

static ssize_t spipe_write(struct file *filp,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct spipe_sbuf *sbuf = filp->private_data;
	int timeout = -1;

	if (filp->f_flags & O_NONBLOCK) {
		timeout = 0;
	}

	return sbuf_write(sbuf->dst, sbuf->channel, sbuf->bufid,
			(void *)buf, count, timeout);
}

static unsigned int spipe_poll(struct file *filp, poll_table *wait)
{
	struct spipe_sbuf *sbuf = filp->private_data;

	return sbuf_poll_wait(sbuf->dst, sbuf->channel, sbuf->bufid,
			filp, wait);
}

static long spipe_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations spipe_fops = {
	.open		= spipe_open,
	.release	= spipe_release,
	.read		= spipe_read,
	.write		= spipe_write,
	.poll		= spipe_poll,
	.unlocked_ioctl	= spipe_ioctl,
	.owner		= THIS_MODULE,
	.llseek		= default_llseek,
};

static int __devinit spipe_probe(struct platform_device *pdev)
{
	struct spipe_init_data *init = pdev->dev.platform_data;
	struct spipe_device *spipe;
	dev_t devid;
	int i, rval;

	rval = sbuf_create(init->dst, init->channel, init->ringnr,
		init->txbuf_size, init->rxbuf_size);
	if (rval != 0) {
		LOGE("Failed to create sbuf-%d-%d: %d\n", init->dst, init->channel, rval);
		return rval;
	}

	spipe = kzalloc(sizeof(struct spipe_device), GFP_KERNEL);
	if (spipe == NULL) {
		sbuf_destroy(init->dst, init->channel);
		LOGE("Failed to allocate spipe_device\n");
		return -ENOMEM;
	}

	rval = alloc_chrdev_region(&devid, 0, init->ringnr, init->name);
	if (rval != 0) {
		sbuf_destroy(init->dst, init->channel);
		kfree(spipe);
		LOGE("Failed to alloc spipe chrdev\n");
		return rval;
	}

	cdev_init(&(spipe->cdev), &spipe_fops);
	rval = cdev_add(&(spipe->cdev), devid, init->ringnr);
	if (rval != 0) {
		sbuf_destroy(init->dst, init->channel);
		kfree(spipe);
		unregister_chrdev_region(devid, init->ringnr);
		LOGE("Failed to add spipe cdev\n");
		return rval;
	}

	spipe->major = MAJOR(devid);
	spipe->minor = MINOR(devid);
	if (init->ringnr > 1) {
		for (i = 0; i < init->ringnr; i++) {
			device_create(spipe_class, NULL,
				MKDEV(spipe->major, spipe->minor + i),
				NULL, "%s%d", init->name, i);
		}
	} else {
		device_create(spipe_class, NULL,
			MKDEV(spipe->major, spipe->minor),
			NULL, "%s", init->name);
	}

	spipe->init = init;
	spipe->spipe_kobj = &pdev->dev.kobj;

	if (sysfs_create_file(spipe->spipe_kobj,
			(const struct attribute *)&dev_attr_spipe_state.attr) < 0)
			LOGW("failed to create sysfs file for sipc chanel attr.\n");

	platform_set_drvdata(pdev, spipe);

	return 0;
}

static int __devexit spipe_remove(struct platform_device *pdev)
{
	struct spipe_device *spipe = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < spipe->init->ringnr; i++) {
		device_destroy(spipe_class,
				MKDEV(spipe->major, spipe->minor + i));
	}
	cdev_del(&(spipe->cdev));
	unregister_chrdev_region(
		MKDEV(spipe->major, spipe->minor), spipe->init->ringnr);

	kfree(spipe);
	sbuf_destroy(spipe->init->dst, spipe->init->channel);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int spipe_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct spipe_init_data *init = pdev->dev.platform_data;
	int i, ret;

	ret = i = 0;
	sipc_suspend = 1;

	LOGI("dst=%d channel=%d +++\n", init->dst, init->channel);

	if (sipc_rx_wake_lock_active(init->dst))
		ret = -EBUSY;

	if (spipe_ready(init->dst, init->channel)) {
		if (init->channel == 4 || init-> channel == 6)
			for (i = 0; i < init->ringnr; i++)
				if (sbuf_has_data(init->dst, init->channel, i)) {
					ret = -EBUSY;
					break;
				}
		if (ret != 0)
			return ret;
	}

	sipc_enable_irq_wake(init->dst);
	LOGI("dst=%d channel=%d ---\n", init->dst, init->channel);

	return 0;
}

static int spipe_resume(struct platform_device *pdev)
{
	struct spipe_init_data *init = pdev->dev.platform_data;

	LOGI("dst=%d channel=%d +++\n", init->dst, init->channel);

	sipc_disable_irq_wake(init->dst);
	sipc_suspend = 0;

	LOGI("dst=%d channel=%d ---\n", init->dst, init->channel);

	return 0;
}


static struct platform_driver spipe_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "spipe",
	},
	.probe = spipe_probe,
	.remove = __devexit_p(spipe_remove),
	.suspend = spipe_suspend,
	.resume = spipe_resume,
};

static int __init spipe_init(void)
{
	spipe_class = class_create(THIS_MODULE, "spipe");
	if (IS_ERR(spipe_class))
		return PTR_ERR(spipe_class);

	return platform_driver_register(&spipe_driver);
}

static void __exit spipe_exit(void)
{
	class_destroy(spipe_class);
	platform_driver_unregister(&spipe_driver);
}

module_init(spipe_init);
module_exit(spipe_exit);

MODULE_AUTHOR("Chen Gaopeng");
MODULE_DESCRIPTION("SIPC/SPIPE driver");
MODULE_LICENSE("GPL");

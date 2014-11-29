#ifdef CONFIG_ARCH_MSM7X27A
#include <mach/rpc_pmapp.h>
#include "../../../arch/arm/mach-msm/devices-msm7x2xa.h"
#else
#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/platform_device.h>
/* replace with plaftform specific changes */
#endif

#include "core.h"
#include "debug.h"

/* BeginMMC polling stuff */
#ifdef CONFIG_ARCH_MSM7X27A
#define MMC_PLATFORM_DEV "msm_sdcc.2"
#else
#define MMC_PLATFORM_DEV "sdcc"
#endif

/* End MMC polling stuff */

#define GET_INODE_FROM_FILEP(filp) \
	((filp)->f_path.dentry->d_inode)



#define ATH6KL_INIT_TIMEOUT	(6 * HZ)

wait_queue_head_t init_wq;
static atomic_t init_done = ATOMIC_INIT(0);


int android_readwrite_file(const char *filename,
			   char *rbuf, const char *wbuf, size_t length)
{
	int ret = 0;
	struct file *filp = (struct file *)-ENOENT;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	do {
		int mode = (wbuf) ? O_RDWR : O_RDONLY;
		filp = filp_open(filename, mode, S_IRUSR);

		if (IS_ERR(filp) || !filp->f_op) {
			ret = -ENOENT;
			break;
		}

		if (length == 0) {
			/* Read the length of the file only */
			struct inode    *inode;

			inode = GET_INODE_FROM_FILEP(filp);
			if (!inode) {
				ath6kl_dbg(ATH6KL_DBG_BOOT,  "android_readwrite_file: Error 2\n");
				ret = -ENOENT;
				break;
			}
			ret = i_size_read(inode->i_mapping->host);
			break;
		}

		if (wbuf) {
			ret = filp->f_op->write(filp, wbuf, length, &filp->f_pos);
			if (ret < 0) {
				ath6kl_dbg(ATH6KL_DBG_BOOT, 
				       "android_readwrite_file: Error 3\n");
				break;
			}
		} else {
			ret = filp->f_op->read(filp, rbuf, length, &filp->f_pos);
			if (ret < 0) {
				ath6kl_dbg(ATH6KL_DBG_BOOT, 
				       "android_readwrite_file: Error 4\n");
				break;
			}
		}
	} while (0);

	if (!IS_ERR(filp))
		filp_close(filp, NULL);

	set_fs(oldfs);
	ath6kl_dbg(ATH6KL_DBG_BOOT,  "android_readwrite_file: ret=%d file=%s\n", ret, filename);

	return ret;
}

static int ath6kl_pm_probe(struct platform_device *pdev)
{
	ar600x_wlan_power(1);
	return 0;
}

static int ath6kl_pm_remove(struct platform_device *pdev)
{
	ar600x_wlan_power(0);
	return 0;
}

static int ath6kl_pm_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static inline void *ar6k_priv(struct net_device *dev)
{
	return wdev_priv(dev->ieee80211_ptr);
}

static int ath6kl_pm_resume(struct platform_device *pdev)
{
	return 0;
}


static struct platform_driver ath6kl_pm_device = {
	.probe      = ath6kl_pm_probe,
	.remove     = ath6kl_pm_remove,
	.suspend    = ath6kl_pm_suspend,
	.resume     = ath6kl_pm_resume,
	.driver     = {
		.name = "wlan_ar6000_pm_dev",
	},
};

int __init ath6kl_sdio_init_platform(void)
{
	char buf[3];
	int length, ret;

	ret = platform_driver_register(&ath6kl_pm_device);
	if (ret) {
		ath6kl_dbg(ATH6KL_DBG_BOOT,  "platform driver registration failed: %d\n",
		       ret);
		return ret;
	}

	length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
	android_readwrite_file("/sys/devices/platform/" MMC_PLATFORM_DEV
			       "/polling", NULL, buf, length);
	length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
	android_readwrite_file("/sys/devices/platform/" MMC_PLATFORM_DEV
			       "/polling", NULL, buf, length);

	mdelay(50);

	return ret;
}

void ath6kl_sdio_exit_platform(void)
{
	char buf[3];
	int length;

	platform_driver_unregister(&ath6kl_pm_device);

	length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
	/* fall back to polling */
	android_readwrite_file("/sys/devices/platform/" MMC_PLATFORM_DEV
			       "/polling", NULL, buf, length);
	length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
	/* fall back to polling */
	android_readwrite_file("/sys/devices/platform/" MMC_PLATFORM_DEV
			       "/polling", NULL, buf, length);
	mdelay(1000);

}

int ath6kl_wait_for_init_comp(void)
{
	int left, ret = 0;

	if (atomic_read(&init_done) == 1)
		return ret;

	left = wait_event_interruptible_timeout(init_wq,
						atomic_read(&init_done) == 1,
						ATH6KL_INIT_TIMEOUT);
	if (left == 0) {
		ath6kl_dbg(ATH6KL_DBG_BOOT,  "timeout while waiting for init operation\n");
		ret = -ETIMEDOUT;
	} else if (left < 0) {
		ath6kl_dbg(ATH6KL_DBG_BOOT,  "wait for init operation failed: %d\n", left);
		ret = left;
	}

	return ret;
}
void ath6kl_notify_init_done(void)
{
	atomic_set(&init_done, 1);
	wake_up(&init_wq);
}

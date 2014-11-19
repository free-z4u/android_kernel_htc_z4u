#include <linux/platform_device.h>
#include "../../../arch/arm/mach-msm/devices-msm7x2xa.h"
#include "platform.h"

static int msm_wlan_ar6000_pm_device_suspend(struct platform_device *pdev, pm_message_t state) {
    printk("%s: %s\n", __func__, pdev->name);
    return 0;
}

static int msm_wlan_ar6000_pm_device_resume(struct platform_device *pdev) {
    printk("%s: %s\n", __func__, pdev->name);
    return 0;
}

static int msm_wlan_ar6000_pm_device_remove(struct platform_device *pdev) {
    printk("%s: %s\n", __func__, pdev->name);
    ar600x_wlan_power(0);
    return 0;
}

static int msm_wlan_ar6000_pm_device_probe(struct platform_device *pdev) {
    printk("%s: %s\n", __func__, pdev->name);
    ar600x_wlan_power(1);
    return 0;
}

static void msm_wlan_ar6000_pm_device_shutdown(struct platform_device *pdev) {
    printk("%s: %s\n", __func__, pdev->name);
}

static struct platform_driver wlan_ar6000_driver = {
    .probe = msm_wlan_ar6000_pm_device_probe,
    .remove = msm_wlan_ar6000_pm_device_remove,
    .suspend = msm_wlan_ar6000_pm_device_suspend,
    .resume = msm_wlan_ar6000_pm_device_resume,
    .shutdown = msm_wlan_ar6000_pm_device_shutdown,
    .driver.name = "wlan_ar6000_pm_dev"
};

int ar6k_platform_register(void)
{
    printk(KERN_DEBUG "wlan ar6000 driver!\n");
    if (platform_driver_register(&wlan_ar6000_driver)) {
		printk("failed register driver for ar6k");
    };
    return 0;
}

void ar6k_platform_unregister(void)
{
    printk(KERN_DEBUG "wlan ar6000 driver!\n");
    platform_driver_unregister(&wlan_ar6000_driver);
}

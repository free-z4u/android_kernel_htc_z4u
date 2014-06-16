/* Code to extract Camera point information from ATAG
set up by the bootloader.

Copyright (C) 2008 Google, Inc.
Author: Dmitry Shmidt <dimitrysh@google.com>

This software is licensed under the terms of the GNU General Public
License version 2, as published by the Free Software Foundation, and
may be copied, distributed, and modified under those terms.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <asm/setup.h>
#include <linux/uaccess.h>
#include <linux/fs.h>


#ifdef CONFIG_MACH_Z4TD
static const char *SensorNAME_front = "s5k6a2ya";
static const char *SensorSize_front = "1.6M";
#else
static const char *SensorNAME_front = "ov2722";
static const char *SensorSize_front = "2M";
#endif
static ssize_t sensor_front_mode_change(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return 0;
}

static ssize_t sensor_front_mode_get(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
    return sprintf(buf, "%s %s\n", SensorNAME_front,SensorSize_front);
}

static DEVICE_ATTR(sensor, 644, sensor_front_mode_change, sensor_front_mode_get);

static struct kobject *android_sensor_front = NULL;

static int cam_set_front_point(void)
{

    int ret = 0;
	printk("hanker,%s \n", __func__);
	printk("hanker,sensor:kobject creat and add\n");

	android_sensor_front = kobject_create_and_add("android_camera2", NULL);
	if (android_sensor_front == NULL) {
		printk("hanker,sensor_sysfs_init: subsystem_register failed\n");
		ret = -ENOMEM;
		return ret ;
	}

	printk("hanker,sensor:sysfs_create_file\n");
	ret = sysfs_create_file(android_sensor_front, &dev_attr_sensor.attr);
	if (ret) {
	    printk("hanker,sensor_sysfs_init: sysfs_create_file failed\n");
	    ret = -EFAULT;
	    goto error;
	}

return ret;

error:
	kobject_del(android_sensor_front);
	return ret;
}

late_initcall(cam_set_front_point);


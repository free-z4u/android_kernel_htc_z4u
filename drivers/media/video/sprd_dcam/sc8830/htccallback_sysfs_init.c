/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/device.h>
#include <linux/switch.h>
#include "htccallback_sysfs_init.h"

static struct switch_dev htccallback_switch = {
	.name = "htccallback",
};

static struct kobject *htccallback_obj = NULL;

static uint32_t htccallback_value;

static ssize_t htccallback_set(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
	char *tmp;
	htccallback_value = simple_strtoul(buf, &tmp, 0);

	switch_set_state(&htccallback_switch, htccallback_value);

	pr_info("[CAM]htccallback_value = %d\n", htccallback_value);

	return count;
}

static DEVICE_ATTR(htccallback, 0666,
    NULL,
    htccallback_set);

int htccallback_sysfs_init(void)
{
  int ret = 0;
	pr_info("[CAM] htccallback:kobject creat and add\n");

       htccallback_obj = kobject_create_and_add("camera_htccallback", NULL);
       if (htccallback_obj == NULL) {
              pr_info("[CAM]htccallback: subsystem_register_htccallback failed\n");
              ret = -ENOMEM;
              goto error;
       }

       ret = sysfs_create_file(htccallback_obj,
                  &dev_attr_htccallback.attr);
	if (ret) {
		pr_info("[CAM]htccallback: sysfs_create_htccallback_file failed\n");
		ret = -EFAULT;
		goto error;
	}
	if (switch_dev_register(&htccallback_switch) < 0) {
		pr_info("[CAM]htccallback : switch_dev_register error\n");
	}

  return ret;
  
error:
  kobject_del(htccallback_obj);
  return ret;
}

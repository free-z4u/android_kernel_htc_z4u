/* linux/arch/arm/mach-sc/htc-debug-last-io.c
 * Copyright (C) 2013 HTC Corporation.
 * Author: Yili Xie <yili_xie@htc.com>
 *
 * It is a debug function without clock protection in SMP env.
 * Performance is the first priority here.
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

#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <mach/hardware.h>
#include <mach/htc-last-io.h>
#include <mach/board_htc.h>


int last_io_init = 0;
struct last_io_percpu *htc_last_io;

static void __iomem *io_sci_base;
static void __iomem *io_sci_end;
static dma_addr_t htc_last_io_phys;

static int __init htc_last_io_init(void)
{
	size_t size;
	int cpu_num;

	if (!(htc_get_config(HTC_DBG_FLAG_SUPERMAN) &
		SUPERMAN_FLAG_LAST_IO)) {
		printk(KERN_INFO "[k] Disable last io function! \n");
		return -EPERM;
	}

	cpu_num = num_possible_cpus();
	printk(KERN_INFO "[k] Initialize %d buffers for last io ...\n", cpu_num);
	size = sizeof(struct last_io_percpu) * cpu_num;

	htc_last_io = dma_alloc_coherent(NULL, size, &htc_last_io_phys,
								GFP_KERNEL);
	if (!htc_last_io) {
		printk(KERN_ERR"%s: Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	memset(htc_last_io, 0, size);

	if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) &
		SUPERMAN_FLAG_HW_LAST_IO) {
		printk(KERN_INFO "Record HW IO only.\n");
		io_sci_base = __io(SCI_IOMAP_BASE);
		io_sci_end  = __io(SPRD_ADISLAVE_BASE + SPRD_ADISLAVE_SIZE);
	}

	last_io_init = 1;
	printk(KERN_INFO "[k] Debug IO superman is ready.\n");

	return 0;
}

EXPORT_SYMBOL(last_io_init);
EXPORT_SYMBOL(htc_last_io);
arch_initcall(htc_last_io_init);

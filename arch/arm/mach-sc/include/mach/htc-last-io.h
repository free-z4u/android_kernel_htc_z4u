/* driver/staging/htc/htc-last-io.c
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

#ifndef __HTC_LAST_IO_H__
#define __HTC_LAST_IO_H__

#ifdef CONFIG_HTC_DBG_LAST_IO
#include <linux/jiffies.h>

#define NUM_LAST_IO_PER_CPU 4

struct __last_io {
	void __iomem *vaddr;
	u32 value;
	u32 pc;
	u32 lr;
	u64 jiffies;
	u32 state;
};

struct last_io_percpu {
	u32 index;
	struct __last_io _last_io[NUM_LAST_IO_PER_CPU];
};

extern int last_io_init;
extern struct last_io_percpu *htc_last_io;

static inline void __iomem *__last_io_start(u32 value, void __iomem *vaddr)
{
	u32 cpu, pc, lr, index;

	if (last_io_init) {
		asm volatile (
				"	mrc p15, 0, %0, c0, c0, 5\n"	\
				"	ands %0, %0, #0xf\n"		\
				"	mov %1, r15\n"		\
				"	mov %2, lr\n"		\
				:	"=&r" (cpu), "=&r" (pc), "=&r" (lr)	\
				:	\
				:	"memory"	\
				);
		index = htc_last_io[cpu].index;
		htc_last_io[cpu].index = (index + 1) & (NUM_LAST_IO_PER_CPU - 1);
		htc_last_io[cpu]._last_io[index].jiffies = jiffies_64;
		htc_last_io[cpu]._last_io[index].vaddr = vaddr;
		htc_last_io[cpu]._last_io[index].value = value;
		htc_last_io[cpu]._last_io[index].pc = pc;
		htc_last_io[cpu]._last_io[index].lr = lr;
		htc_last_io[cpu]._last_io[index].state = 0;
	}
	return vaddr;
}

static inline void __last_io_done(void __iomem *vaddr)
{
	u32 cpu, index;

	if(last_io_init) {
		asm volatile (
				"	mrc p15, 0, %0, c0, c0, 5\n"	\
				"	ands %0, %0, #0xf\n"	\
				:	"=&r" (cpu)	\
				:	\
				:	"memory"		\
				);
		index = (htc_last_io[cpu].index == 0)?(NUM_LAST_IO_PER_CPU - 1):(htc_last_io[cpu].index - 1);
		htc_last_io[cpu]._last_io[index].jiffies = jiffies_64;
		htc_last_io[cpu]._last_io[index].state = 1;
	}
}

#define __mem_io_w(v, a)		__last_io_start((u32)(v), (void __iomem *)(a))
#define __mem_io_r(a)			__last_io_start(0, (void __iomem *)(a))
#define __io_done(a)			__last_io_done((void __iomem *)(a))
#endif

#endif

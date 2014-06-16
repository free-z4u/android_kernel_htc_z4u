/* arch/arm/mach-sc/include/mach/freq_table.h
*
* DVFS freq table header
*
* Copyright (C) 2008 HTC Corporation
* Author: Sai Shen <sai_shen@htc.com>
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

#define FREQ_LOWEST    600000
#define FREQ_LOW       600000
#define FREQ_MEDIUM    768000
#define FREQ_HIGH      1000000

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
#define FREQ_HIGHEST   1200000
#endif

#if defined(CONFIG_SPRD_CPU_DYNAMIC_HOTPLUG)
void up_all_cpu(void);
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
void down_all_cpu(bool lock_plug);
#else
void down_all_cpu(void);
#endif
#endif

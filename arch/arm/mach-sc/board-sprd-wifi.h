/* linux/arch/arm/mach-msm/board-sprd-wifi.h
 *
 * Copyright (C) 2008 HTC Corporation.
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
#ifdef CONFIG_MACH_SP8825EA_TIGER_EVM
#define SPRD_WIFI_PMENA_GPIO  203
#define SPRD_WIFI_IRQ_GPIO    118
#endif

#if defined(CONFIG_MACH_Z4DTG) || defined(CONFIG_MACH_Z4TD) || defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
#define SPRD_WIFI_PMENA_GPIO  115
#define SPRD_WIFI_IRQ_GPIO    219

#define WIFI_SDIO1_CLK       88
#define WIFI_SDIO1_CMD      89
#define WIFI_SDIO1_DATA0   90
#define WIFI_SDIO1_DATA1   91
#define WIFI_SDIO1_DATA2   92
#define WIFI_SDIO1_DATA3   93
#endif

extern unsigned char *get_wifi_nvs_ram(void);

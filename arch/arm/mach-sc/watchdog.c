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
#include <mach/hardware.h>
#include <mach/watchdog.h>
#include <mach/adi.h>
#include <linux/string.h>
#include <asm/bitops.h>
#include <mach/htc_bootreason.h>
#include <mach/sci_glb_regs.h>

void sprd_set_reboot_mode(const char *cmd)
{
	uint16_t restart_reason = 0;
	int error;

	printk(KERN_EMERG "[debug]%s,restart cmd : %s+\n", __func__,cmd);
	if (cmd) {
		if (!strcmp(cmd, "bootloader")) {
			restart_reason = RESET_REASON_FASTBOOT;
		} else if (!strcmp(cmd, "recovery")) {
			restart_reason = RESET_REASON_RECOVERY;
		} else if (!strcmp(cmd, "factory-reset")){
			restart_reason = RESET_REASON_RECOVERY;
		} else if (!strcmp(cmd, "eraseflash")) {
			restart_reason = RESET_REASON_ERASEFLASH;
		} else if (!strcmp(cmd, "ramdump")) {
			restart_reason = RESET_REASON_RAMDUMP;
		} else if (!strncmp(cmd, "oem-", 4)) {
			unsigned long code;
			error = strict_strtoul(cmd + 4, 16, &code) & 0xff;
			if (error) {
				printk(KERN_ERR "parsing oem-code fail!\n");
				return;
			}
			restart_reason = RESET_REASON_OEM_BASE | code;
		} else {
			restart_reason = RESET_REASON_NORMAL;
		}
	} else {
		restart_reason = RESET_REASON_NORMAL;
	}

	/*set bootreason into pimc ANA_REG_GLB_POR_RST_MONITOR register*/
	if(restart_reason){
		int ret = -1;

		printk(KERN_INFO"restart_reason = 0x%x\n",restart_reason);
		printk(KERN_INFO"the ANA_REG_GLB_POR_RST_MONITOR register value is 0x%x\n",sci_adi_read(ANA_REG_GLB_POR_RST_MONITOR));
		printk(KERN_INFO"the ANA_REG_GLB_WDG_RST_MONITOR register value is 0x%x\n",sci_adi_read(ANA_REG_GLB_WDG_RST_MONITOR));
		printk(KERN_INFO"the ANA_REG_GLB_POR_PIN_RST_MONITOR register value is 0x%x\n",sci_adi_read(ANA_REG_GLB_POR_PIN_RST_MONITOR));

		ret = sci_adi_raw_write(ANA_REG_GLB_POR_RST_MONITOR, restart_reason);
		printk(KERN_INFO"setting reboot reason, ret=%d, the ANA_REG_GLB_POR_RST_MONITOR register value is 0x%x\n",
			ret, sci_adi_read(ANA_REG_GLB_POR_RST_MONITOR));
	}
}

#ifdef CONFIG_ARCH_SCX35
void sprd_turnon_watchdog(unsigned int ms)
{
	uint32_t cnt;

	cnt = (ms * 1000) / WDG_CLK;

	/*enable interface clk*/
	sci_adi_set(ANA_AGEN, AGEN_WDG_EN);
	/*enable work clk*/
	sci_adi_set(ANA_RTC_CLK_EN, AGEN_RTC_WDG_EN);
	sci_adi_raw_write(WDG_LOCK, WDG_UNLOCK_KEY);
	sci_adi_set(WDG_CTRL, WDG_NEW_VER_EN);
	WDG_LOAD_TIMER_VALUE(cnt);
	sci_adi_set(WDG_CTRL, WDG_CNT_EN_BIT | WDG_RST_EN_BIT);
	sci_adi_raw_write(WDG_LOCK, (uint16_t) (~WDG_UNLOCK_KEY));
}

void sprd_turnoff_watchdog(void)
{
	sci_adi_raw_write(WDG_LOCK, WDG_UNLOCK_KEY);
	/*wdg counter stop*/
	sci_adi_clr(WDG_CTRL, WDG_CNT_EN_BIT);
	/*disable the reset mode*/
	sci_adi_clr(WDG_CTRL, WDG_RST_EN_BIT);
	sci_adi_raw_write(WDG_LOCK, (uint16_t) (~WDG_UNLOCK_KEY));
	/*stop the interface and work clk*/
	sci_adi_clr(ANA_AGEN, AGEN_WDG_EN);
	sci_adi_clr(ANA_RTC_CLK_EN, AGEN_RTC_WDG_EN);
}

#else

void sprd_turnon_watchdog(unsigned int ms)
{
	uint32_t cnt;

	cnt = (ms * 1000) / WDG_CLK;
	/* turn on watch dog clock */
	sci_adi_set(ANA_AGEN, AGEN_WDG_EN | AGEN_RTC_ARCH_EN | AGEN_RTC_WDG_EN);
	sci_adi_raw_write(WDG_LOCK, WDG_UNLOCK_KEY);
	sci_adi_clr(WDG_CTRL, WDG_INT_EN_BIT);
	WDG_LOAD_TIMER_VALUE(cnt);
	sci_adi_set(WDG_CTRL, WDG_CNT_EN_BIT | BIT(3));
	sci_adi_raw_write(WDG_LOCK, (uint16_t) (~WDG_UNLOCK_KEY));
}

void sprd_turnoff_watchdog(void)
{
	sci_adi_clr(ANA_AGEN, AGEN_WDG_EN | AGEN_RTC_ARCH_EN | AGEN_RTC_WDG_EN);
	sci_adi_raw_write(WDG_LOCK, WDG_UNLOCK_KEY);
	sci_adi_clr(WDG_CTRL, WDG_INT_EN_BIT);
	sci_adi_raw_write(WDG_LOCK, (uint16_t) (~WDG_UNLOCK_KEY));
}
#endif

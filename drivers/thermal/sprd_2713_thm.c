/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <mach/hardware.h>
#include <mach/sci.h>
#include <mach/sci_glb_regs.h>
#include <linux/io.h>
#include <mach/adi.h>
#include "thm.h"
#include <linux/sprd_thm.h>

#define SPRD_THM_DEBUG
#ifdef SPRD_THM_DEBUG
#define THM_DEBUG(format, arg...) printk(format, ## arg)
#else
#define THM_DEBUG(format, arg...)
#endif

#define THM_CTRL            (0x0000)
#define THM_INT_CTRL       (0x0004)
#define SENSOR_CTRL         (0x0020)
#define SENSOR_INT_CTRL     (0x0028)
#define SENSOR_INT_STS      (0x002C)
#define SENSOR_INT_RAW_STS      (0x0030)
#define SENSOR_INT_CLR      (0x0034)
#define SENSOR_OVERHEAD_HOT_THRES   (0X0040)
#define SENSOR_HOT2NOR__HIGHOFF_THRES   (0X0044)
#define SENSOR_LOWOFF_THRES   (0X0048)
#define SENSOR_TEMPER0_READ	(0x0058)

#define SEN_OVERHEAT_INT_BIT (1 << 5)
#define SEN_HOT_INT_BIT      (1 << 4)
#define SEN_HOT2NOR_INT_BIT  (1 << 3)
#define SEN_HIGHOFF_INT_BIT  (1 << 2)
#define SEN_LOWOFF_INT_BIT  (1 << 1)

#define RAW_TEMP_OFFSET 8
#define RAW_TEMP_RANGE_MSK  0x7F

#define HIGH_BITS_OFFSET   4

#define HIGH_TAB_SZ 8
#define LOW_TAB_SZ  16

#define HOT2NOR_RANGE   4  //cuke
#define LOCAL_SENSOR_ADDR_OFF 0x100

static const short temp_search_high_40nm[HIGH_TAB_SZ] =
    { -41, -14, 14, 41, 68, 95, 122, 150 };
static const short temp_search_low_40nm[LOW_TAB_SZ] =
    { 0, 2, 4, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22, 24, 26 };
static const short temp_search_high_152nm[HIGH_TAB_SZ] =
    { -45, -19, 7, 33, 59, 85, 111, 137 };
static const short temp_search_low_152nm[LOW_TAB_SZ] =
    { 0, 2, 3, 5, 6, 8, 10, 11, 13, 14, 16, 18, 19, 21, 22, 24 };

static int arm_sen_cal_offset = 0;
static int pmic_sen_cal_offset = 0;

struct thermal_table{
    //unsigned int index;
    unsigned int low_threshold;
    unsigned int high_threshold;
} ;

struct thermal_table shark_thermal_table[] = {
#if 0  //only for test
    {35, 38},
    {42, 45},
    {46, 49},
    {51, 54},
#else
    {50, 55},
    {58, 62},
    {75, 80},
    {85, 90},
    #endif
    //{NULL,NULL}, // only
};

static inline int __thm_reg_write(u32 reg, u16 bits, u16 clear_msk)
{
	if (reg >= SPRD_THM_BASE && reg <= (SPRD_THM_BASE + SPRD_THM_SIZE)) {
		__raw_writel(((__raw_readl(reg) & ~clear_msk) | bits), (reg));
	} else if (reg >= ANA_THM_BASE && reg <= (ANA_THM_BASE + SPRD_THM_SIZE)) {
		sci_adi_write(reg, bits, clear_msk);
	} else {
		printk(KERN_ERR "[sprd thm] error thm reg0x:%x \n", reg);
	}
	return 0;
}

static inline u32 __thm_reg_read(u32 reg)
{
	if (reg >= SPRD_THM_BASE && reg <= (SPRD_THM_BASE + SPRD_THM_SIZE)) {
		return __raw_readl(reg);
	} else if (reg >= ANA_THM_BASE && reg <= (ANA_THM_BASE + SPRD_THM_SIZE)) {
		return sci_adi_read(reg);
	} else {
		printk(KERN_ERR "[sprd thm] error thm reg0x:%x \n", reg);
	}
	return 0;
}

u32 sprd_thm_temp2rawdata(u32 sensor, int temp)
{
	u32 high_bits, low_bits;
	int i;
	const short *high_tab;
	const short *low_tab;

	//return temp;

	if (SPRD_ARM_SENSOR == sensor) {
		high_tab = temp_search_high_40nm;
		low_tab = temp_search_low_40nm;
	} else if (SPRD_PMIC_SENSOR == sensor) {
		high_tab = temp_search_high_152nm;
		low_tab = temp_search_low_152nm;
	} else {
		printk(KERN_ERR "[sprd thm] error thm sensor id:%d \n", sensor);
		return 0;
	}

	if (temp < high_tab[0]) {
		printk(KERN_ERR "[sprd thm] temp over low temp:%d \n", temp);
		return 0;
	}

	for (i = HIGH_TAB_SZ - 1; i >= 0; i--) {
		if (high_tab[i] <= temp)
			break;
	}
	if (i < 0) {
		i = 0;
	}
	temp -= high_tab[i];
	high_bits = i;

	for (i = LOW_TAB_SZ - 1; i >= 0; i--) {
		if (low_tab[i] <= temp)
			break;
	}
	if (i < 0) {
		i = 0;
	}
	low_bits = i;
	return ((high_bits << HIGH_BITS_OFFSET) | low_bits);

}

int sprd_thm_rawdata2temp(u32 sensor, int rawdata)
{
	const short *high_tab;
	const short *low_tab;

	if (SPRD_ARM_SENSOR == sensor) {
		high_tab = temp_search_high_40nm;
		low_tab = temp_search_low_40nm;
	} else if (SPRD_PMIC_SENSOR == sensor) {
		high_tab = temp_search_high_152nm;
		low_tab = temp_search_low_152nm;
	} else {
		printk(KERN_ERR "[sprd thm] error thm sensor id:%d \n", sensor);
		return 0;
	}

	return high_tab[(rawdata >> HIGH_BITS_OFFSET) & 0x07] +
	    low_tab[rawdata & 0x0F];
}

int sprd_thm_temp_read(u32 sensor)
{
	u32 rawdata = 0;
	int cal_offset = 0;

	if (SPRD_ARM_SENSOR == sensor) {
		rawdata = __thm_reg_read((SPRD_THM_BASE + SENSOR_TEMPER0_READ));
		cal_offset = arm_sen_cal_offset;
	} else if (SPRD_PMIC_SENSOR == sensor) {
		rawdata = __thm_reg_read(ANA_THM_BASE + SENSOR_TEMPER0_READ);
		cal_offset = pmic_sen_cal_offset;
	} else {
		printk(KERN_ERR "[sprd thm] error thm sensor id:%d \n", sensor);
		return 0;
	}
	//THM_DEBUG("[sprd thm] thm sensor id:%d, cal_offset:%d, rawdata:0x%x\n", sensor,
		//  cal_offset, rawdata);
	return sprd_thm_rawdata2temp(sensor, rawdata) + cal_offset;
}

#include <linux/kernel.h>
#include <linux/err.h>
#include <mach/hardware.h>
#include <mach/sci.h>
#include <mach/sci_glb_regs.h>
#include <linux/io.h>
#include <mach/adi.h>
#include <linux/wakelock.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>


static struct timer_list fgu_timer;

int sprd_thm_dump_reg(void);
static void fgu_handler(unsigned long data) {
		sprd_thm_dump_reg();
};

int sprd_thm_hw_init(struct sprd_thermal_zone *pzone)
{
	u32 local_sensor_addr, base_addr = 0;
	u32 local_sen_id = 0;
	u32 raw_temp = 0;
	int cal_offset = 0;
	struct sprd_thm_platform_data *trip_tab = pzone->trip_tab;

	base_addr = (u32) pzone->reg_base;

	printk(KERN_NOTICE "1024[sprd thm] sprd_thm_hw_init id:%d,base 0x%x\n",
	       pzone->sensor_id, base_addr);

	if (SPRD_ARM_SENSOR == pzone->sensor_id) {
			init_timer(&fgu_timer);
				fgu_timer.function = fgu_handler;
					mod_timer(&fgu_timer, jiffies + HZ);


		//ddie thm en
		sci_glb_set(REG_AON_APB_APB_EB1, BIT_THM_EB);
		sci_glb_set(REG_AON_APB_APB_RTC_EB,
			    (BIT_THM_RTC_EB | BIT_ARM_THMA_RTC_EB |
			     BIT_ARM_THMA_RTC_AUTO_EN));
		cal_offset = arm_sen_cal_offset = 0;
		local_sen_id = 0;
	} else if (SPRD_PMIC_SENSOR == pzone->sensor_id) {
	#if 0
		//adie thm en
		sci_adi_set(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_THM_EN);
		sci_adi_set(ANA_REG_GLB_RTC_CLK_EN,
			    BIT_RTC_THMA_AUTO_EN | BIT_RTC_THMA_EN |
			    BIT_RTC_THM_EN);
		cal_offset = pmic_sen_cal_offset = 0;
		local_sen_id = 0;
		#endif
	} else {
		printk(KERN_ERR "[sprd thm] error thm sensor id:%d \n", pzone->sensor_id);
		return -EINVAL;
	}
	__thm_reg_write((base_addr + THM_CTRL), 0x1 >> local_sen_id, 0);
	__thm_reg_write((base_addr + THM_INT_CTRL), 0x3, 0);	//enable top int

	local_sensor_addr = base_addr + local_sen_id * LOCAL_SENSOR_ADDR_OFF;

	__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL), 0, ~0);	//disable all int
	__thm_reg_write((local_sensor_addr + SENSOR_INT_CLR), ~0, 0);	//clr all int

	//set int
	if (trip_tab->num_trips > 0) {
		
		if (trip_tab->trip_points[0].type == THERMAL_TRIP_ACTIVE) {
					//set the int low offset
			raw_temp =
			    sprd_thm_temp2rawdata(pzone->sensor_id,
						  shark_thermal_table[0].low_threshold);
			//if (raw_temp < RAW_TEMP_RANGE_MSK) {
				//raw_temp++;
			//}
				printk(KERN_NOTICE "[sprd thm] r1:0x%x\n",raw_temp);

			__thm_reg_write((local_sensor_addr +
					 SENSOR_LOWOFF_THRES), (raw_temp<<RAW_TEMP_OFFSET),
					RAW_TEMP_RANGE_MSK<< RAW_TEMP_OFFSET);

			//set the high offset  , need to modify ,no proper bit ???
			raw_temp =
			    sprd_thm_temp2rawdata(pzone->sensor_id,
						  shark_thermal_table[0].high_threshold);
			printk(KERN_NOTICE "[sprd thm] r2:0x%x\n",raw_temp);
			__thm_reg_write((local_sensor_addr +
					SENSOR_HOT2NOR__HIGHOFF_THRES), raw_temp,
					RAW_TEMP_RANGE_MSK);

			//set hot int temp value
			raw_temp =
			    sprd_thm_temp2rawdata(pzone->sensor_id,
						  shark_thermal_table[1].high_threshold);
			printk(KERN_NOTICE "[sprd thm] r3:0x%x\n",raw_temp);
			__thm_reg_write((local_sensor_addr +
					 SENSOR_OVERHEAD_HOT_THRES), raw_temp,
					RAW_TEMP_RANGE_MSK);
			//set hot2nor init
			raw_temp =
			    sprd_thm_temp2rawdata(pzone->sensor_id,
						  shark_thermal_table[1].low_threshold);
			printk(KERN_NOTICE "[sprd thm] r4:0x%x\n",raw_temp);

			__thm_reg_write((local_sensor_addr +
					 SENSOR_HOT2NOR__HIGHOFF_THRES),
					raw_temp << RAW_TEMP_OFFSET,
					RAW_TEMP_RANGE_MSK << RAW_TEMP_OFFSET);

			__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL),	//enable int
					SEN_HOT2NOR_INT_BIT | SEN_HOT_INT_BIT|SEN_HIGHOFF_INT_BIT,
					0);
		}
		//set overheat
		if (trip_tab->trip_points[trip_tab->num_trips - 1].type ==
		    THERMAL_TRIP_CRITICAL) {
			raw_temp =
			    sprd_thm_temp2rawdata(pzone->sensor_id,
						  trip_tab->trip_points
						  [trip_tab->num_trips -
						   1].temp - cal_offset);
			//set overheat int temp value
			__thm_reg_write((local_sensor_addr +
					 SENSOR_OVERHEAD_HOT_THRES),
					raw_temp << RAW_TEMP_OFFSET,
					RAW_TEMP_RANGE_MSK << RAW_TEMP_OFFSET);
			__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL),	//enable int
					SEN_OVERHEAT_INT_BIT, 0);
		}
	}

	printk(KERN_NOTICE "[sprd thm] sprd_thm_hw_init addr 0x:%x,int ctrl 0x%x\n",
	       local_sensor_addr,
	       __thm_reg_read((local_sensor_addr + SENSOR_INT_CTRL)));

	__thm_reg_write((local_sensor_addr + SENSOR_CTRL), 0x9, 0); //restart the status machine

	return 0;

}

thermal_table_type previous_level = 0;
int sprd_thm_thermal_table_interrupt_registry(thermal_table_type curr_level)
{
	//when the thermal up to 62 degree, the thermal solution can not block temp trend.
	//so now need update the thermal table,only update when up to high threshold

	u32 local_sensor_addr, base_addr = 0;
	u32 local_sen_id = 0;  // use the arm temp sensor
	u32 raw_temp = 0;
//	int cal_offset = 0;
	//struct sprd_thm_platform_data *trip_tab = pzone->trip_tab;

	base_addr = (u32) SPRD_THM_BASE;

	pr_info("interrupt_registry id:%d,base 0x%x,level:%d\n",
	       local_sen_id, base_addr,curr_level);
	if(( curr_level == THERMAL_TABLE_NULL )||(previous_level == curr_level))
		return 0;

	previous_level = curr_level;
	
            local_sensor_addr = base_addr + local_sen_id * LOCAL_SENSOR_ADDR_OFF;

           // __thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL), 0, ~0);	//disable all int
           // __thm_reg_write((local_sensor_addr + SENSOR_INT_CLR), ~0, 0);	//clr all int

            //set the int low offset
            raw_temp = sprd_thm_temp2rawdata(local_sen_id,shark_thermal_table[curr_level].low_threshold);
            pr_info("[sprd thm] r1:%d\n",raw_temp);

            __thm_reg_write((local_sensor_addr +
                SENSOR_LOWOFF_THRES), (raw_temp<<RAW_TEMP_OFFSET),
                RAW_TEMP_RANGE_MSK<< RAW_TEMP_OFFSET);

            //set the high offset  , need to modify ,no proper bit ???
            raw_temp = sprd_thm_temp2rawdata(local_sen_id,shark_thermal_table[curr_level].high_threshold);
            pr_info("[sprd thm] r2:%d\n",raw_temp);
            __thm_reg_write((local_sensor_addr +
                SENSOR_HOT2NOR__HIGHOFF_THRES), raw_temp,
                RAW_TEMP_RANGE_MSK);

            //set hot int temp value
            raw_temp = sprd_thm_temp2rawdata(local_sen_id,shark_thermal_table[curr_level+1].high_threshold);
            pr_info("[sprd thm] r3:%d\n",raw_temp);
            __thm_reg_write((local_sensor_addr +
                SENSOR_OVERHEAD_HOT_THRES), raw_temp,
                RAW_TEMP_RANGE_MSK);
            //set hot2nor init
            raw_temp = sprd_thm_temp2rawdata(local_sen_id,shark_thermal_table[curr_level+1].low_threshold);
            pr_info("[sprd thm] r4:%d\n",raw_temp);

            __thm_reg_write((local_sensor_addr +
                SENSOR_HOT2NOR__HIGHOFF_THRES),
                raw_temp << RAW_TEMP_OFFSET,
                RAW_TEMP_RANGE_MSK << RAW_TEMP_OFFSET);

            __thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL),	//enable int
                SEN_HOT2NOR_INT_BIT | SEN_HOT_INT_BIT|SEN_LOWOFF_INT_BIT,
                0);  // the SEN_HOT2NOR_INT_BIT will never triggled at first temp run up to this spot

	//pr_info("[sprd thm] sprd_thm_hw_init addr 0x:%x,int ctrl 0x%x\n",
	    //   local_sensor_addr,__thm_reg_read((local_sensor_addr + SENSOR_INT_CTRL)));

	__thm_reg_write((local_sensor_addr + SENSOR_CTRL), 0x9, 0); //restart the status machine

	return 0;

}

u16 int_ctrl_reg[SPRD_MAX_SENSOR];
int sprd_thm_hw_suspend(struct sprd_thermal_zone *pzone)
{
	u32 local_sen_id = 0;
	u32 local_sensor_addr;
	//u32 int_sts;
	//int ret = 0;

	local_sensor_addr =
	    (u32) pzone->reg_base + local_sen_id * LOCAL_SENSOR_ADDR_OFF;
	int_ctrl_reg[pzone->sensor_id] = __thm_reg_read((local_sensor_addr + SENSOR_INT_CTRL));

	__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL), 0, ~0);	//disable all int
	__thm_reg_write((local_sensor_addr + SENSOR_INT_CLR), ~0, 0);	//clr all int
	return 0;
}
int sprd_thm_hw_resume(struct sprd_thermal_zone *pzone)
{
	u32 local_sen_id = 0;
	u32 local_sensor_addr;
	//u32 int_sts;
	//int ret = 0;

	local_sensor_addr =
	    (u32) pzone->reg_base + local_sen_id * LOCAL_SENSOR_ADDR_OFF;

	__thm_reg_write((local_sensor_addr + SENSOR_INT_CLR), ~0, 0);	//clr all int
	__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL), int_ctrl_reg[pzone->sensor_id], ~0);	//enable int of saved
	__thm_reg_write((local_sensor_addr + SENSOR_CTRL), 0x9, 0);
	return 0;
}

int sprd_thm_dump_reg(void)
{
	    printk("sprd_thm_dump_reg......... \n");
	        printk("SENSOR_INT_CTRL:0x%x \n",__thm_reg_read(SPRD_THM_BASE + SENSOR_INT_CTRL));
		    printk("SENSOR_OVERHEAD_HOT_THRES:0x%x \n",__thm_reg_read(SPRD_THM_BASE
					    + SENSOR_OVERHEAD_HOT_THRES));
		        printk("SENSOR_HOT2NOR__HIGHOFF_THRES:0x%x \n",__thm_reg_read (SPRD_THM_BASE + SENSOR_HOT2NOR__HIGHOFF_THRES));
			    printk("SENSOR_HOT2NOR__HIGHOFF_THRES+0x4:0x%x \n",__thm_reg_read (SPRD_THM_BASE + SENSOR_HOT2NOR__HIGHOFF_THRES+0x4));
			        printk("SENSOR_TEMPER0_READ:0x%x \n",__thm_reg_read(SPRD_THM_BASE + SENSOR_TEMPER0_READ));
				    printk("SPRD_THM_BASE ---:0x%x \n",SPRD_THM_BASE);
				        printk("sprd_thm_dump_reg.........end \n");
	return 1;
}
//					sprd_thm_dump_reg
//u32 int_sts;
int sprd_thm_hw_irq_handle(struct sprd_thermal_zone *pzone)
{
	u32 local_sen_id = 0;
	u32 local_sensor_addr;
	u32 int_sts;
	int ret = 0;

	local_sensor_addr =
	    (u32) pzone->reg_base + local_sen_id * LOCAL_SENSOR_ADDR_OFF;
	int_sts = __thm_reg_read(local_sensor_addr + SENSOR_INT_STS);
	__thm_reg_write((local_sensor_addr + SENSOR_INT_CLR), 0xFFFF, ~0);	//CLR INT
	//printk("raw 0x%x",__thm_reg_write((local_sensor_addr+);
	if(int_sts & SEN_HIGHOFF_INT_BIT) {
		__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL),	//enable int
			SEN_LOWOFF_INT_BIT|SEN_HOT2NOR_INT_BIT|SEN_HOT_INT_BIT,
			SEN_HIGHOFF_INT_BIT);
	}
	if(int_sts & SEN_LOWOFF_INT_BIT) {

			__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL),	//enable int
					SEN_HIGHOFF_INT_BIT|SEN_HOT2NOR_INT_BIT|SEN_HOT_INT_BIT,
			SEN_LOWOFF_INT_BIT);
	}
	if(int_sts & SEN_HOT_INT_BIT) {

			__thm_reg_write((local_sensor_addr + SENSOR_INT_CTRL),	//enable int
					SEN_LOWOFF_INT_BIT|SEN_HIGHOFF_INT_BIT|SEN_HOT2NOR_INT_BIT|SEN_HOT_INT_BIT,
			0);
	}
	__thm_reg_write((local_sensor_addr + SENSOR_INT_CLR), 0xFFFF, ~0);	//CLR INT

	printk("thm interrupt:0x%x, int_sts :0x%x,temp:%d \n", __thm_reg_read(local_sensor_addr + SENSOR_INT_CTRL), int_sts,sprd_thm_temp_read(pzone->sensor_id));
	//printk("Thermal temp:%d\n", );
	sprd_thm_dump_reg();
	return ret;
}

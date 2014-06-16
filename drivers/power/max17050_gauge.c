/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2012 High Tech Computer Corporation

Module Name:

		max17050_gauge.c

Abstract:

		This module implements the battery formula based on power spec, including below concepts:
		1. adc converter
		2. voltage mapping to capacity
		3. over temperature algorithm
		4. id range algorithm
		5. ACR maintainance

		Add from TPE PMA:
		1. temperature index
		2. pd_m_coef_boot
		3. preserved_capacity_by_temp
		Remove from TAO PMA:
		1. pd_temp

		To adapt different PMA/projects, we need to modify below tables:
		1. ID_RANGE: which battery is used in the project?
		2. FL_25: the full capacity in temp 25C.
		3. pd_m_bias_mA: the discharge current threshold to calculating pd_m
		4. M_PARAMTER_TABLE: the voltage-capacity mapping table
		5. TEMP_RANGE: how many temp condition we need to consider
		6. PD_M_COEF_TABLE(BOOT)/PD_M_RESL_TABLE(BOOT): voltage compensation based on current
		7. PD_T_COEF: voltage compensation based on temp
		8. CAPACITY_DEDUCTION_01p: the capacity deduction due to low temperature
---------------------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/max17050_battery.h>
#include <linux/max17050_gauge.h>
#include <mach/htc_battery_types.h>
#include <linux/time.h>
//#include <asm/setup.h>
/* for i2c_adapter, i2c_client define*/
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/debugfs.h>
#include <linux/export.h>
#include <linux/reboot.h>

#if defined(DRIVER_ZONE)
#undef DRIVER_ZONE
#endif
#define DRIVER_ZONE     "[BATT][max17050]"

/*========================================================================================

build flags

========================================================================================*/

#define HTC_ENABLE_POWER_DEBUG  		0
#define HTC_ENABLE_DUMMY_BATTERY		0
#define HTC_PARAM_MAX17050_DEBUG_ENABLE		1
#define XA_board	0
#define XB_board 	1

/*========================================================================================

Critical gauge register default value

========================================================================================*/
#define TEMPNOM_DEFAULT 	0x1400
#define LOCK_GAUGE_ACCESS 	0x0000
#define MASKSOC_DEFAULT 	0x5A00

/*========================================================================================

battery common parameter defines (independent on battery id yet...)

========================================================================================*/

#define BATTERY_VOLTAGE_MIN 2000
#define BATTERY_VOLTAGE_MAX 20000
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))

/*========================================================================================

battery parameter defines (depend on board design)

========================================================================================*/
/* use default parameter if it doesn't be passed from board */
#define CAPACITY_DEDUCTION_DEFAULT	(0)

/*========================================================================================

battery formula coef definition, can be re-programable

========================================================================================*/
/* over temperature algorithm*/

static INT32 over_high_temp_lock_01c = 600;
static INT32 over_high_temp_release_01c = 570;
static INT32 over_low_temp_lock_01c = 0;
static INT32 over_low_temp_release_01c = 30;

/*dead battery is voltage < M_0*/
#define BATTERY_DEAD_VOLTAGE_LEVEL  	3420
#define BATTERY_DEAD_VOLTAGE_RELEASE	3450

/* MAXIM 17050 I2C I/O*/
static struct i2c_adapter *i2c2 = NULL;
static struct i2c_client *max17050_i2c = NULL;
struct max17050_fg *max17050_fg_log = NULL;
extern int tps65200_dump_register(void);

int max17050_i2c_read(u8 addr, u8 *values, size_t len)
{
	int retry;
	uint8_t buf[1];
#if MAXIM_I2C_DEBUG
	int i;
#endif

	struct i2c_msg msg[] = {
		{
			.addr = max17050_i2c->addr,
			.flags = 0,
			.len = 1,
			.buf = buf,
		},
		{
			.addr = max17050_i2c->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = values,
		}
	};

	buf[0] = addr & 0xFF;

	for (retry = 0; retry < MAX17050_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(max17050_i2c->adapter, msg, 2) == 2)
			break;
		mdelay(10);
	}

	if (retry == MAX17050_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_read_block retry over %d\n",
		MAX17050_I2C_RETRY_TIMES);
		return -EIO;
	}

#if MAXIM_I2C_DEBUG
	printk(KERN_ERR "%s, slave_id=0x%x(0x%x), addr=0x%x, len=%d\n", __func__, max17050_i2c->addr, max17050_i2c->addr << 1, addr, len);
	for (i = 0; i < len; i++)
		printk(KERN_ERR " 0x%x", values[i]);
	printk(KERN_ERR "\n");
#endif

	return 0;
}

int max17050_i2c_write(u8 addr, const u8 *values, size_t len)
{
	int retry, i;
	uint8_t buf[len + 1];

	struct i2c_msg msg[] = {
		{
			.addr = max17050_i2c->addr,
			.flags = 0,
			.len = len + 1,
			.buf = buf,
		}
	};

	buf[0] = addr & 0xFF;

	for (i = 0; i < len; i++)
		buf[i + 1] = values[i];

	for (retry = 0; retry < MAX17050_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(max17050_i2c->adapter, msg, 1) == 1)
			break;
		mdelay(10);
	}


#if MAXIM_I2C_DEBUG
	printk(KERN_ERR "%s, slave_id=0x%x(0x%x), addr=0x%x, len=%d\n", __func__, max17050_i2c->addr, max17050_i2c->addr << 1, addr, len+1);
	for (i = 0; i < len+1 ; i++)
		printk(KERN_ERR " 0x%x", buf[i]);
	printk(KERN_ERR "\n");
#endif

	if (retry == MAX17050_I2C_RETRY_TIMES) {
		printk(KERN_ERR "%s: i2c_write_block retry over %d\n",
			__func__, MAX17050_I2C_RETRY_TIMES);
		return -EIO;
	}

	return TRUE;
}

void max17050_i2c_exit(void)
{
	if (max17050_i2c != NULL){
		kfree(max17050_i2c);
		max17050_i2c = NULL;
	}

	if (i2c2 != NULL){
		i2c_put_adapter(i2c2);
		i2c2 = NULL;
	}
}

int max17050_i2c_init(void)
{
	i2c2 = i2c_get_adapter(MAX17050_I2C_BUS_ID);
	max17050_i2c = kzalloc(sizeof(*max17050_i2c), GFP_KERNEL);

	if (i2c2 == NULL || max17050_i2c == NULL){
		printk("[%s] fail (0x%x, 0x%x).\n",
			__func__,
			(int) i2c2,
			(int) max17050_i2c);
		return -ENOMEM;
	}

	max17050_i2c->adapter = i2c2;
	max17050_i2c->addr = MAX17050_I2C_SLAVE_ADDR;

	return 0;
}

/*========================================================================================

battery maxim 17050 gauge debug function

========================================================================================*/

#if MAXIM_BATTERY_FG_LOG

#define MAXIM_BATTERY_FG_LOG_REG_BLK1_START	0x00
#define MAXIM_BATTERY_FG_LOG_REG_BLK1_END	0x4F
#define MAXIM_BATTERY_FG_LOG_REG_BLK2_START	0xE0
#define MAXIM_BATTERY_FG_LOG_REG_BLK2_END	0xFF

#define FG_LOG_DIR "/sdcard/fg_log"
#define FG_LOG_BUFFER_SIZE 2048
#define FG_LOG_PERIOD_IN_SEC 15

int fg_log_enabled = 0;

long sys_mkdir(const char *pathname, int mode);
long sys_open(const char *filename, int flags, int mode);
long sys_close(unsigned int fd);
long sys_read(unsigned int fd, char *buf, size_t count);
long sys_write(unsigned int fd, const char *buf, size_t count);

static int htc_battery_get_fg_log(char *buf, u8 start_addr, u8 end_addr)
{
	u16 reg_val = 0;
	int readLen = 0;
	int rc = 0;
	int count = 0, i = 0;

	if (unlikely(!max17050_fg_log)) {
		pr_err("%s: max17050_fg_log is not initialized", __func__);
		return 0;
	}

	count = end_addr - start_addr;
#if MAXIM_BATTERY_FG_LOG_DEBUG
	pr_err("%s: read from 0x%02X to 0x%02X for %d registers\n", __func__, start_addr, end_addr, count + 1);
#endif
	for (i = 0; i <= count; i++) {
		reg_val = 0;
		rc = max17050_i2c_read(start_addr + i, (u8 *)&reg_val, 2);
		if (unlikely(rc < 0))
			pr_err("%s: Failed to read reg 0x%x, rc=%d", __func__, MAXIM_BATTERY_FG_LOG_REG_BLK1_START + i, rc);
#if MAXIM_BATTERY_FG_LOG_DEBUG
		/*
		pr_err("%s: update buffer offset %d, reg 0x%04x, val 0x%04x\n", __func__, readLen, MAXIM_BATTERY_FG_LOG_REG_BLK2_START + i, reg_val);
		*/
#endif
		readLen += sprintf(buf + readLen, "%04X ", reg_val);
	}

	return readLen;
}

static void htc_battery_dump_fg_reg(char *buf, int fd)
{
#if 0
	u16 reg_val = 0;
	int len = 0;
	int count = 0, i = 0;
	int ret = 0;
    /* TODO, Need to be reprogrmed on SPRD platform*/
#if MAXIM_BATTERY_FG_LOG_DEBUG
	pr_err("%s: +, fd=%d, fg_log_enabled=%d\n", __func__, fd, fg_log_enabled);
#endif

	if (unlikely(!buf)) {
		pr_err("%s: invalid buffer\n", __func__);
		return;
	}

	if (unlikely(fd < 0)) {
		pr_err("%s: invalid file handler %d", __func__, fd);
		return;
	}

	if (unlikely(!max17050_fg_log)) {
		pr_err("%s: max17050_fg_log is not initialized", __func__);
		return;
	}

	if (!fg_log_enabled)
		len += sprintf(buf, "\n\n");

	len += sprintf(buf + len, "Dump Fuel Gauge Registers:\n\n");

	count = (MAXIM_BATTERY_FG_LOG_REG_BLK2_END - MAXIM_BATTERY_FG_LOG_REG_BLK1_START) / 2;
	/* Dump all registers when fg_log start */
	for (i = MAXIM_BATTERY_FG_LOG_REG_BLK1_START; i <= count; i++)
		len += sprintf(buf + len, "0x%02X ", MAXIM_BATTERY_FG_LOG_REG_BLK1_START + i);

	/* Add the end of line */
	*(buf + len - 1) = '\n';

	/*
	ret = sys_write(fd, (char *)buf, len);
	if (ret < 0)
		goto err;
	*/

	for (i = MAXIM_BATTERY_FG_LOG_REG_BLK1_START; i <= count; i++)
		len += sprintf(buf + len, "---- ");

	/* Add the end of line */
	*(buf + len - 1) = '\n';

	ret = sys_write(fd, (char *)buf, len);
	if (ret < 0)
		goto err;

	/* Unlock modem access */
	reg_val = 0x0059;
	ret = max17050_i2c_write(0x62, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x62, rc=%d", __func__, ret);

	reg_val = 0x00C4;
	ret = max17050_i2c_write(0x63, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x63, rc=%d", __func__, ret);

	len = 0;
	len += htc_battery_get_fg_log(buf + len, MAXIM_BATTERY_FG_LOG_REG_BLK1_START, count);

	/* Lock modem access */
	reg_val = 0x0000;
	ret = max17050_i2c_write(0x62, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x62, rc=%d", __func__, ret);

	ret = max17050_i2c_write(0x63, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x63, rc=%d", __func__, ret);

	/* Add the end of line */
	len++;
	sprintf(buf + len - 2, "\n\n");

	ret = sys_write(fd, (char *)buf, len);
	if (ret < 0)
		goto err;

	len = 0;
	/* Dump all registers when fg_log start */
	for (i = count + 1; i <= MAXIM_BATTERY_FG_LOG_REG_BLK2_END; i++)
		len += sprintf(buf + len, "0x%02X ", i);

	/* Add the end of line */
	*(buf + len - 1) = '\n';

	/*
	ret = sys_write(fd, (char *)buf, len);
	if (ret < 0)
		goto err;
	*/

	for (i = count + 1; i <= MAXIM_BATTERY_FG_LOG_REG_BLK2_END; i++)
		len += sprintf(buf + len, "---- ");

	/* Add the end of line */
	*(buf + len - 1) = '\n';

	ret = sys_write(fd, (char *)buf, len);
	if (ret < 0)
		goto err;

	/* Unlock modem access */
	reg_val = 0x0059;
	ret = max17050_i2c_write(0x62, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x62, rc=%d", __func__, ret);

	reg_val = 0x00C4;
	ret = max17050_i2c_write(0x63, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x63, rc=%d", __func__, ret);

	len = 0;
	len += htc_battery_get_fg_log(buf + len, count + 1, MAXIM_BATTERY_FG_LOG_REG_BLK2_END);

	/* Lock modem access */
	reg_val = 0x0000;
	ret = max17050_i2c_write(0x62, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x62, rc=%d", __func__, ret);

	ret = max17050_i2c_write(0x63, (u8 *)&reg_val, 2);
	if (unlikely(ret < 0))
		pr_err("%s: Failed to write reg 0x63, rc=%d", __func__, ret);

	if (fg_log_enabled) {
		len += 3;
		sprintf(buf + len - 3, "\n\n\n");
	}

	ret = sys_write(fd, (char *)buf, len);
	if (ret < 0)
		goto err;

#if MAXIM_BATTERY_FG_LOG_DEBUG
	pr_err("%s: -\n", __func__);
#endif
	return;

err:
	pr_err("%s: failed to write file, ret=%d\n", __func__, ret);
#endif
	return;
}

static void htc_battery_fg_log_work_func(struct work_struct *work)
{
	static char filename[128];
	static int fd = -1;
	static char *buf = NULL;
	mm_segment_t old_fs;
	int len = 0, ret = 0;
	struct timespec ts;
	struct rtc_time tm;
	int count = 0, i = 0;

	if (unlikely(!max17050_fg_log)) {
		pr_err("%s: max17050_fg_log is not initialized", __func__);
		return;
	}

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);

#if MAXIM_BATTERY_FG_LOG_DEBUG
	pr_err("%s: fd=%d, fg_log_enabled=%d\n", __func__, fd, fg_log_enabled);
#endif

	if (!buf) {
		buf = kzalloc(FG_LOG_BUFFER_SIZE, GFP_KERNEL);
		if (unlikely(!buf)) {
			pr_err("%s: failed to allocate buffer %d bytes\n", __func__, FG_LOG_BUFFER_SIZE);
			return;
		}
	}

	if (fg_log_enabled) {
		/* Keep the system awake */
		wake_lock_timeout(&max17050_fg_log->fg_log_wake_lock, HZ * (FG_LOG_PERIOD_IN_SEC + 5));

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		if (fd < 0) {
			ret = sys_mkdir(FG_LOG_DIR, 0644);
			if (ret < 0 && ret != -EEXIST) {
				pr_err("%s: failed to create directory %s, ret=%d\n", __func__, FG_LOG_DIR, ret);
				goto err;
			}
#if MAXIM_BATTERY_FG_LOG_DEBUG
			else
				pr_err("%s: create directory %s success\n", __func__, FG_LOG_DIR);
#endif

			sprintf(filename, "%s/%d%02d%02d%02d%02d%02d.txt", FG_LOG_DIR,
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

			fd = sys_open(filename, O_RDWR | O_APPEND | O_CREAT, 0644);
			if (fd < 0) {
				pr_err("%s: failed to open file %s, ret=%d\n", __func__, filename, fd);
				fd = 0;
				return;
			}
#if MAXIM_BATTERY_FG_LOG_DEBUG
			else
				pr_err("%s: open file %s success, fd=%d\n", __func__, filename, fd);
#endif

			/* Dump all registers when fg_log start */
			htc_battery_dump_fg_reg(buf, fd);

			len = 0;
			len += sprintf(buf + len, "Fuel Guage Log Start: \n\n");

			/* Generate the header of fg log file */
			len += sprintf(buf + len, "%19s ", "Time");
			count = MAXIM_BATTERY_FG_LOG_REG_BLK1_END - MAXIM_BATTERY_FG_LOG_REG_BLK1_START;
			for (i = 0; i <= count; i++)
				len += sprintf(buf + len, "0x%02X ", MAXIM_BATTERY_FG_LOG_REG_BLK1_START + i);

			count = MAXIM_BATTERY_FG_LOG_REG_BLK2_END - MAXIM_BATTERY_FG_LOG_REG_BLK2_START;
			for (i = 0; i <= count; i++)
				len += sprintf(buf + len, "0x%02X ", MAXIM_BATTERY_FG_LOG_REG_BLK2_START + i);

			/* Add the end of line */
			*(buf + len - 1) = '\n';

			len += sprintf(buf + len, "------------------- ");
			count = MAXIM_BATTERY_FG_LOG_REG_BLK1_END - MAXIM_BATTERY_FG_LOG_REG_BLK1_START;
			for (i = 0; i <= count; i++)
				len += sprintf(buf + len, "---- ");

			count = MAXIM_BATTERY_FG_LOG_REG_BLK2_END - MAXIM_BATTERY_FG_LOG_REG_BLK2_START;
			for (i = 0; i <= count; i++)
				len += sprintf(buf + len, "---- ");

			/* Add the end of line */
			*(buf + len - 1) = '\n';

			ret = sys_write(fd, (char *)buf, len);
			if (ret < 0) {
				pr_err("%s: failed to write file %s, ret=%d\n", __func__, filename, ret);
				goto err;
			}
		}

		len = 0;
		len += sprintf(buf + len, "%04d/%02d/%02d %02d:%02d:%02d ",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		len += htc_battery_get_fg_log(buf + len, MAXIM_BATTERY_FG_LOG_REG_BLK1_START, MAXIM_BATTERY_FG_LOG_REG_BLK1_END);
		len += htc_battery_get_fg_log(buf + len, MAXIM_BATTERY_FG_LOG_REG_BLK2_START, MAXIM_BATTERY_FG_LOG_REG_BLK2_END);

		/* Add the end of line */
		*(buf + len - 1) = '\n';

		ret = sys_write(fd, (char *)buf, len);
		if (ret < 0) {
			pr_err("%s: failed to write file %s, ret=%d\n", __func__, filename, ret);
			goto err;
		}
#if MAXIM_BATTERY_FG_LOG_DEBUG
		else
			pr_err("%s: write file %s success, len=%d\n", __func__, filename, len);
#endif

		set_fs(old_fs);

		schedule_delayed_work(&max17050_fg_log->fg_log_work, msecs_to_jiffies(FG_LOG_PERIOD_IN_SEC * 1000));
	} else {
		/* Dump all registers when fg_log stop */
		htc_battery_dump_fg_reg(buf, fd);

		sys_close(fd);
		fd = -1;
	}

	return;

err:
	if (buf)
		kzfree(buf);

	set_fs(old_fs);

	if (fd > 0)
		sys_close(fd);
	fd = 0;

	return;
}

#if defined(CONFIG_DEBUG_FS)
static int fg_log_debug_set(void *data, u64 val)
{
	val = (val != 0) ? 1 : 0;

	/* Return if the setting is not changed */
	if (fg_log_enabled == val) {
#if MAXIM_BATTERY_FG_LOG_DEBUG
		pr_err("%s: fg_log_enabled(%d) not changed, skip\n", __func__, fg_log_enabled);
#endif
		return 0;
	}

	fg_log_enabled = val;

	/* Cancel queued work first when disable fg_log */
	if (!fg_log_enabled)
		cancel_delayed_work_sync(&max17050_fg_log->fg_log_work);

	schedule_delayed_work(&max17050_fg_log->fg_log_work, 0);

	return 0;
}

static int fg_log_debug_get(void *data, u64 *val)
{
	*val = fg_log_enabled;

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fg_log_debug_fops, fg_log_debug_get, fg_log_debug_set, "%llu\n");
#endif

static int __init batt_debug_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("max17050", 0);
	if (IS_ERR(dent)) {
		pr_err("%s: failed to create debugfs dir for htc_battery\n", __func__);
		return PTR_ERR(dent);
	}

#if MAXIM_BATTERY_FG_LOG
	dent = debugfs_create_dir("fg_log", dent);
	if (IS_ERR(dent)) {
		pr_err("%s: failed to create debugfs dir for fuel_gauge_log\n", __func__);
		return PTR_ERR(dent);
	}

	debugfs_create_file("enable", 0644, dent, NULL, &fg_log_debug_fops);
#endif

	return 0;
}

device_initcall(batt_debug_init);
#endif

/*========================================================================================

over temperature protection

========================================================================================*/

static BOOL is_over_temp(struct battery_type *battery)
{
	/* stop charging*/
	if (battery->temp_01c < over_low_temp_lock_01c || battery->temp_01c >= over_high_temp_lock_01c) {
		return TRUE;
	}

	return FALSE;
}

static BOOL is_not_over_temp(struct battery_type *battery)
{
	/* start charging*/
	if (battery->temp_01c >= over_low_temp_release_01c &&
		battery->temp_01c < over_high_temp_release_01c) {
		return TRUE;
	}

	return FALSE;
}

static void __protect_flags_update(struct battery_type *battery,
	struct protect_flags_type *flags, int update_log)
{
	/* Flags:
	is_charging_enable_available		- Over temperature, need to stop charging
	is_charging_high_current_avaialble	- Temperature is too high so that we have to slow charge*/

	if( NULL != flags->func_update_charging_protect_flag)
	{
		int pstate = flags->func_update_charging_protect_flag(
				battery->current_mA, battery->voltage_mV,
				battery->temp_01c,//250, /*icey fake temp here*/
				&(flags->is_charging_enable_available),
				&(flags->is_charging_high_current_avaialble),
				&(flags->is_temperature_fault));
		if (flags->is_fake_room_temp) {
			flags->is_charging_enable_available = TRUE;
			flags->is_charging_high_current_avaialble = TRUE;
		}
		if (!update_log)
			printk(DRIVER_ZONE "batt: protect pState=%d,allow(chg,hchg)=(%d,%d)\n",
				pstate,
				flags->is_charging_enable_available,
				flags->is_charging_high_current_avaialble);
	}
	else
	{
		if (is_over_temp(battery)) {
			/* Ex: T<0 or T>45 */
			flags->is_charging_enable_available = FALSE;
			flags->is_charging_high_current_avaialble = FALSE;
#if 0
			flags->is_battery_overtemp = TRUE;
#endif
		} else if (is_not_over_temp(battery)) {
			/* Ex: T<57 or T>3*/
			flags->is_charging_enable_available = TRUE;
			flags->is_charging_high_current_avaialble = TRUE;
#if 0
			flags->is_battery_overtemp = FALSE;
#endif
		}
	}

	/* Flags:
	is_battery_dead			- If battery is dead, show special indicator for it*/
	if (battery->voltage_mV < BATTERY_DEAD_VOLTAGE_LEVEL) {
		flags->is_battery_dead = TRUE;
	}
	else if (battery->voltage_mV > BATTERY_DEAD_VOLTAGE_RELEASE) {
		flags->is_battery_dead = FALSE;
	}
}

/*========================================================================================

Voltage-Percentage mapping

========================================================================================*/

/*------------------------------------------------------------------------
 Example:
	p0 = (4200, 10000); 	4.2V for 100%
	p1 = (3900, 8000);  	3.9V for 80%
	p2 = (3700, 2000);  	3.7V for 20%
	p3 = (3300, 0); 		3.3V for 0%

	if V = 4000, (3900<4000<4200)
	P = (4000-3900) * (10000-8000)/(4200-3900) + 8000 = 8666*/

#define NUM_SAMPLED_POINTS_MAX 12

struct sampled_point_type {

	DWORD voltage;
	DWORD capacity;
};

struct voltage_curve_translator {

	DWORD voltage_min;
	DWORD voltage_max;
	DWORD capacity_min;
	DWORD capacity_max;
	int sampled_point_count;
	struct sampled_point_type sampled_points[NUM_SAMPLED_POINTS_MAX];
};

#if 0
static void voltage_curve_translator_init(struct voltage_curve_translator *t)
{
	memset(t, 0, sizeof(*t));
}

static void voltage_curve_translator_add(struct voltage_curve_translator *t, DWORD voltage, DWORD capacity)
{
	struct sampled_point_type *pt;

	if (t->sampled_point_count >= NUM_SAMPLED_POINTS_MAX) {
		return;
	}

	t->sampled_points[t->sampled_point_count].voltage = voltage;
	t->sampled_points[t->sampled_point_count].capacity = capacity;
	pt = &t->sampled_points[t->sampled_point_count];

	t->sampled_point_count++;

	if (pt->voltage > t->voltage_max)
		t->voltage_max = pt->voltage;
	if (pt->voltage < t->voltage_min)
		t->voltage_min = pt->voltage;
	if (pt->capacity > t->capacity_max)
		t->capacity_max = pt->capacity;
	if (pt->capacity < t->capacity_min)
		t->capacity_min = pt->capacity;

#if HTC_ENABLE_POWER_DEBUG
	printk(DRIVER_ZONE " kadc t: capacity=%d voltage=%d\n", capacity, voltage);
#endif /* HTC_ENABLE_POWER_DEBUG*/
}

static INT32 voltage_curve_translator_get(struct voltage_curve_translator *t, DWORD voltage)
{
	struct sampled_point_type *p0, *p1;
	INT32 capacity;
	int i;

	if (voltage > t->voltage_max)
		voltage = t->voltage_max;
	if (voltage < t->voltage_min)
		voltage = t->voltage_min;

	p0 = &t->sampled_points[0];
	p1 = p0 + 1;
	for (i = 0; i < t->sampled_point_count - 1 && voltage < p1->voltage; i++) {
		p0++;
		p1++;
	}

	/* DIV ZERO check*/
	if (p0->voltage - p1->voltage == 0) {
		return 0;
	}

	/* INT32 overflow check: mv(4200) * capacity(10000), shall be no problem at all...*/
	capacity = (voltage - p1->voltage) * (p0->capacity - p1->capacity) / (p0->voltage - p1->voltage) + p1->capacity;
	if (capacity > t->capacity_max) {
		capacity = t->capacity_max;
	}
	if (capacity < t->capacity_min) {
		capacity = t->capacity_min;
	}
	return capacity;
}

/*========================================================================================

KADC mapping functions

========================================================================================*/

static struct voltage_curve_translator __get_kadc_t;

static INT32 get_kadc_001p(struct battery_type *battery)
{
	INT32 pd_m = 0;
	INT32 pd_temp = 0;

	INT32 temp_01c = battery->temp_01c;
	INT32 current_mA = battery->current_mA;

	UINT32 *m_paramtable;

	INT32 pd_m_coef;
	INT32 pd_m_resl;

	INT32 pd_t_coef;

	INT32 capacity_deduction_01p;
	INT32 capacity_predict_001p;

	struct battery_parameter* batt_param = container_of(
		battery, struct poweralg_type, battery)->pdata->batt_param;
	/* 1. INT32 overflow check: assert abs(iChgCurrent_ma) <= 3000, iBattTemp_01c>-250, pd_t_coef <= 1000
		when calculating pd_temp: 0x7FFFFFFF / (500 * 3000 * 1000) =:= 1.4*/

	if (battery->current_mA > 3000)
		current_mA = 3000;
	else if (battery->current_mA < -3000)
		current_mA = -3000;
	if (battery->temp_01c <= -250)
		temp_01c = -250;

	/* 2. calculate pd_m and pd_temp*/

	if (battery->is_power_on_reset) {
		if (batt_param->pd_m_coef_tbl_boot)
			pd_m_coef = batt_param->pd_m_coef_tbl_boot[battery->temp_index][battery->id_index];
		else
			pd_m_coef = PD_M_COEF_DEFAULT;
		if (batt_param->pd_m_resl_tbl_boot)
			pd_m_resl = batt_param->pd_m_resl_tbl_boot[battery->temp_index][battery->id_index];
		else
			pd_m_resl = PD_M_RESL_DEFAULT;
	}
	else{
		if (batt_param->pd_m_coef_tbl)
			pd_m_coef = batt_param->pd_m_coef_tbl[battery->temp_index][battery->id_index];
		else
			pd_m_coef = PD_M_COEF_DEFAULT;
		if (batt_param->pd_m_resl_tbl)
			pd_m_resl = batt_param->pd_m_resl_tbl[battery->temp_index][battery->id_index];
		else
			pd_m_resl = PD_M_RESL_DEFAULT;
	}

	if (battery->current_mA < -pd_m_bias_mA) {
		/* ex: -150mA < -130mA*/
		pd_m = (abs(battery->current_mA) - pd_m_bias_mA) * pd_m_coef /pd_m_resl;
	}

	if (battery->temp_01c < 250) {
		if (batt_param->pd_t_coef)
			pd_t_coef = batt_param->pd_t_coef[battery->id_index];
		else
			pd_t_coef = PD_T_COEF_DEFAULT;
		pd_temp = ((250 - battery->temp_01c) * (abs(battery->current_mA) * pd_t_coef)) / (10 * 10000);
	}
	battery->pd_m = pd_m;

	/* 3. calculate KADC using M_PARAMTER_TABLE*/

	if (batt_param->m_param_tbl)
		m_paramtable = batt_param->m_param_tbl[battery->id_index];
	else
		m_paramtable = M_PARAMETER_DEFAULT;
	if (m_paramtable) {
		int i = 0; /* assume that m_paramtable has at least 2 items...the last capacity item must be 0 to end the loop...*/

		voltage_curve_translator_init(&__get_kadc_t);

		while (1) {
			INT32 capacity = m_paramtable[i];
			INT32 voltage = m_paramtable[i + 1];
			if (capacity == 10000) {
				/* full capacity, no need to fix voltage level*/
				voltage_curve_translator_add(&__get_kadc_t, voltage, capacity);
			}
			else {
				voltage_curve_translator_add(&__get_kadc_t, voltage - pd_temp, capacity);
			}

			if (capacity == 0)
				break;

			i += 2;
		}

#if HTC_ENABLE_POWER_DEBUG
		printk(DRIVER_ZONE " pd_m=%d, pd_temp=%d\n", pd_m, pd_temp);
#endif /* HTC_ENABLE_POWER_DEBUG*/

		capacity_predict_001p = voltage_curve_translator_get(&__get_kadc_t, battery->voltage_mV + pd_m);
	}
	else{
		capacity_predict_001p = (battery->voltage_mV - 3400) * 10000 / (4200 - 3400);
	}

	if (batt_param->capacity_deduction_tbl_01p)
		capacity_deduction_01p = batt_param->capacity_deduction_tbl_01p[battery->temp_index];
	else
		capacity_deduction_01p = CAPACITY_DEDUCTION_DEFAULT;
	return (capacity_predict_001p - capacity_deduction_01p * 10) * 10000 / (10000 - capacity_deduction_01p * 10);
}

/*========================================================================================

coulomb counter+curve tracer

========================================================================================*/

static INT32 get_software_acr_revise(struct battery_type *battery, UINT32 ms)
{
	INT32 kadc_01p = battery->KADC_01p;
	INT32 ccbi_01p = battery->RARC_01p;
	INT32 delta_01p = kadc_01p - ccbi_01p;

	DWORD C = 5;						/* KADC = 15%~100%*/
	if (kadc_01p <= 150) {
		C = 5;
	}   	/* KADC = 0%~15%*/

	if (delta_01p < 0) {
		/* if KADC is less than RARC, p shall be lower*/
		return -(INT32) (C * ms * delta_01p * delta_01p) / 1000;
	}
	else{
		/* if KADC is larger than RARC, p shall be higher*/
		return (INT32) (C * ms * delta_01p * delta_01p) / 1000;
	}
}

/*========================================================================================

max17050 gauge ic functions, to access max17050 registers and convert ADC to battery param

========================================================================================*/

static void __max17050_clear_porf(void)
{
	UINT8 reg_data;
	if (!max17050_i2c_read_u8(&reg_data, 0x01)) {
		printk(DRIVER_ZONE " clear porf error in read.\n");
		return;
	}

	if (!max17050_i2c_write_u8((reg_data & (~MAX17050_STATUS_PORF)), 0x01)) {
		printk(DRIVER_ZONE " clear porf error in write.\n");
		return;
	}
}

static void __max17050_acr_update(struct battery_type *battery, int capacity_01p)
{
#if HTC_ENABLE_POWER_DEBUG
	printk(DRIVER_ZONE " acr update: P=%d, C=%d.\n",
		capacity_01p,
		battery->charge_counter_adc);
#endif
	max17050_i2c_write_u8((battery->charge_counter_adc & 0xFF00) >> 8, 0x10);
	max17050_i2c_write_u8((battery->charge_counter_adc & 0x00FF), 0x11);

	if (battery->is_power_on_reset) {
		__max17050_clear_porf();
	}
}

static void __max17050_init_config(struct battery_type *battery)
{
	UINT8 reg_data;

	if (!max17050_i2c_read_u8(&reg_data, 0x01)) {
		printk(DRIVER_ZONE " init config error in read.\n");
		return;
	}

	/* Erase SMOD and NBEN value in MAX17050 status/config register*/
	reg_data &= ~(MAX17050_STATUS_SMOD | MAX17050_STATUS_NBEN);
	if (!max17050_i2c_write_u8(reg_data, 0x01)) {
		printk(DRIVER_ZONE " init config error in write.\n");
		return;
	}
}

static BOOL __max17050_get_reg_data(UINT8 *reg)
{
	memset(reg, 0, 12);

	if (!max17050_i2c_read_u8(&reg[0], 0x01))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[2], 0x08))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[3], 0x09))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[4], 0x0a))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[5], 0x0b))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[6], 0x0c))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[7], 0x0d))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[8], 0x0e))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[9], 0x0f))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[10], 0x10))
		return FALSE;
	if (!max17050_i2c_read_u8(&reg[11], 0x11))
		return FALSE;

	return TRUE;
}

static BOOL __max17050_battery_adc_udpate(struct battery_type *battery)
{
	UINT8 reg[12];

	if (!__max17050_get_reg_data((UINT8 *) &reg)) {
		printk(DRIVER_ZONE " get max17050 data failed...\n");
		return FALSE;
	}

#if HTC_PARAM_MAX17050_DEBUG_ENABLE
	printk(DRIVER_ZONE " [x0]%x [x8]%x %x %x %x %x %x %x %x %x %x\n",
		reg[0],
		reg[2],
		reg[3],
		reg[4],
		reg[5],
		reg[6],
		reg[7],
		reg[8],
		reg[9],
		reg[10],
		reg[11]);
#endif

	if (!(reg[0] & MAX17050_STATUS_AIN0) || !(reg[0] & MAX17050_STATUS_AIN1)) {
		printk(DRIVER_ZONE " AIN not ready...\n");
		return FALSE;
	}

	if (reg[0] & MAX17050_STATUS_PORF) {
		battery->is_power_on_reset = TRUE;
	}
	else{
		battery->is_power_on_reset = FALSE;
	}

	/* adc register value*/
	battery->voltage_adc = MAKEWORD(reg[7], reg[6]) >> 4;
	battery->current_adc = MAKEWORD(reg[9], reg[8]);
	if (battery->current_adc & 0x8000) {
		battery->current_adc = -(0x10000 - battery->current_adc);
	}
	battery->current_adc /= 4;
	battery->charge_counter_adc = MAKEWORD(reg[11], reg[10]);
	if (battery->charge_counter_adc & 0x8000) {
		battery->charge_counter_adc = -(0x10000 - battery->charge_counter_adc);
	}
	battery->id_adc = MAKEWORD(reg[5], reg[4]) >> 4;
	battery->temp_adc = MAKEWORD(reg[3], reg[2]) >> 4;
	if (support_max17050_gauge_ic) {
		/* we preserve 500mAh for capacity lower than 0%, however the 500mAh is still drained out...we need to do predict for correct ACR*/
		if ((battery->charge_counter_adc & 0xFFFF) >= 0xF000){
			printk(DRIVER_ZONE " ACR out of range (x%x)...\n",
				battery->charge_counter_adc);
			battery->is_power_on_reset = TRUE;
		}
	}

	return TRUE;
}
#endif
/*========================================================================================

battery param update, the coef are referenced from power spec

========================================================================================*/
static BOOL __battery_param_udpate(struct battery_type *battery, int update_log)
{
	max17050_i2c_read(MAX17050_FG_VCELL, (u8 *)&battery->voltage_adc, 2);
	max17050_i2c_read(MAX17050_FG_Current, (u8 *)&battery->current_adc, 2);
	max17050_i2c_read(MAX17050_FG_TEMP, (u8 *)&battery->temp_adc, 2);
	max17050_i2c_read(MAX17050_FG_FullCAP, (u8 *)&battery->charge_full_real_mAh, 2);

	battery->voltage_mV = battery->voltage_adc * 20 / 256;
	battery->current_mA = battery->current_adc * 5 / 32;
	battery->temp_01c = (battery->temp_adc / 256) * 10;
	battery->charge_full_real_mAh = battery->charge_full_real_mAh / 2;

	if (!update_log){
		printk("[BATT] V=%d(%x) I=%d(%x) C=%d.%d/%d(%x) id=%d(%x) T=%d(%x)\n",
			battery->voltage_mV,
			battery->voltage_adc,
			battery->current_mA,
			battery->current_adc,
			battery->charge_counter_mAh,
			battery->software_charge_counter_mAms,
			battery->charge_full_real_mAh,
			battery->charge_counter_adc,
			battery->id_index,
			battery->id_adc,
			battery->temp_01c,
			battery->temp_adc);
		tps65200_dump_register();
	}

	return TRUE;
}
int max17050_read_current(void)
{
	s16 current_adc = 0;

	max17050_i2c_read(MAX17050_FG_Current, (u8 *)&current_adc, 2);
	return (current_adc * 5)/32;
}
int max17050_get_batt_level(struct battery_type *battery)
{
       int level = 0;
       int rc = 0;

       rc = max17050_i2c_read(MAX17050_FG_RepSOC, (u8 *)&battery->capacity_raw, 2);
       if (unlikely(rc < 0))
               printk("%s: Failed to read MAX17050_FG_RepSOC: 0x%x", __func__, rc);

       level = (battery->capacity_raw * 10) / 256; /* High byte has unit 1 percent */

       if (level >= 1000)
               level = 1000;

	   if (level < 0)
               level = 0;

       return level;
}
int max17050_get_average_vol(void)
{
	int vol = 0;
	int times = 3;
	u16 buffer;
	while(times){
		max17050_i2c_read(MAX17050_FG_VCELL, (u8 *)&buffer, 2);
		buffer = (buffer*20)>>8;
		if(buffer > 4400|| buffer < 2500){
			printk(DRIVER_ZONE" Alert:battery voltage is out of range!\n");
			buffer = 0;
			continue;
		}
		vol += buffer;
		times--;
		msleep(3);
	}
	vol = vol/3;
	printk(DRIVER_ZONE" We got the real voltage here:%4d\n",vol);
	if(vol<= 3000){
		printk(DRIVER_ZONE" Battery voltage extremely low, that's abnormal. Shut down!\n");
		/**********TODO**********/
		/********Do we need to inform user here?********/
		/***********Here need a small delay before power off to make sure works done*************/
		emergency_sync();
		kernel_power_off();
	}
	return vol;
}
/*========================================================================================

time functions

========================================================================================*/

DWORD BAHW_MyGetMSecs(void)
{
	struct timespec now;
	getnstimeofday(&now);
	/*struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC, &t);*/
	return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

void max17050_batt_softPOR(void)
{
	u16 write_value;
	u16 reg_value_LOCK1 = 0, reg_value_LOCK2 = 0, reg_value_STATUS = 0;
	printk("[BATT]:%s try to set POR\n",__func__);
	do{/**************************************
		*1,Lock the model and clear the POR bit
		**************************************/
		write_value = 0x0000;
		max17050_i2c_write(MAX17050_FG_LOCK_I , (u8 *)&write_value, 2);
		max17050_i2c_write(MAX17050_FG_LOCK_II , (u8 *)&write_value, 2);
		max17050_i2c_write(MAX17050_FG_STATUS , (u8 *)&write_value, 2);
		/*************************************
		 *Verify model lock
		 *************************************/
		max17050_i2c_read(MAX17050_FG_LOCK_I, (u8 *)&reg_value_LOCK1, 2);
		max17050_i2c_read(MAX17050_FG_LOCK_II, (u8 *)&reg_value_LOCK2, 2);
		max17050_i2c_read(MAX17050_FG_STATUS, (u8 *)&reg_value_STATUS, 2);
	}while(reg_value_LOCK1||reg_value_LOCK2||reg_value_STATUS);
	printk("[BATT]%s Max17050 locked\n",__func__);
	do{
		/*************************************
		*Send SoftPOR
		**************************************/
		write_value = 0x000F;
		max17050_i2c_write(0x60 , (u8 *)&write_value, 2);
		msleep(20);
		max17050_i2c_read(MAX17050_FG_STATUS, (u8 *)&reg_value_STATUS, 2);
	}while(!(reg_value_STATUS&0x02));//Verify POR bit is set
	printk("[BATT] Set POR finished, MAX17050 status register 0x%4x.\n",reg_value_STATUS);
}

enum {
	TEMP_NORMAL,
	TEMP_LOW_N5,
	TEMP_LOW_N15,
};
static int max_status = TEMP_NORMAL;
/*========================================================================================

 To set TGAIN/TOFF register to improve the temperature accuracy from -20 degC ~ +70 degC

========================================================================================*/
static void max17050_batt_temp_accuracy(struct battery_type *battery)
{
	u16 reg_val_TGAIN, reg_val_TOFF = 0;
	s16 reg_AvCap = 0;
	u32 voltage_mV = 0;
	u16 reg_val_QR00, reg_val_QR10;
	int old_state = 0;

#if MAXIM_BATTERY_DEBUG
	u32 val_TGAIN = 0;
	u32 val_TOFF = 0;
#endif

	battery->shutdown = false;

	old_state = max_status;
	max17050_i2c_read(MAX17050_FG_QRtable10 , (u8 *)&reg_val_QR10, 2);
	max17050_i2c_read(MAX17050_FG_QRtable00 , (u8 *)&reg_val_QR00, 2);
	switch(max_status){
		case TEMP_NORMAL:
			if(battery->temp_01c < -50)
				max_status = TEMP_LOW_N5;
			break;
		case TEMP_LOW_N5:
			if(battery->temp_01c > -30)
				max_status = TEMP_NORMAL;
			break;
	}
	if(old_state != max_status){  // Update the QR table only status changed,
		switch(max_status){
			case TEMP_NORMAL:
				printk(DRIVER_ZONE " Normal temperature:%x\n",battery->temp_01c);
#if defined(CONFIG_MACH_Z4TD)
				reg_val_QR00 = 0x2801;
				reg_val_QR10 = 0x1080;
#else
				if(battery->id_index == 1){
					reg_val_QR00 = 0x3802;
					reg_val_QR10 = 0x1903;
				}
				if(battery->id_index == 2){
					reg_val_QR00 = 0x3D1B;
					reg_val_QR10 = 0x2280;
				}
#endif
				break;
			default:
				printk(DRIVER_ZONE " temperature low:%x set reg_val_QR00 = 0x02ff, reg_val_QR10 = 0x0000.\n",battery->temp_01c);
#if defined(CONFIG_MACH_Z4TD)
				reg_val_QR00 = 0x0078;
				reg_val_QR10 = 0x0008;
#else
				reg_val_QR00 = 0x02ff;
				reg_val_QR10 = 0x0000;
#endif
				break;
		}
		max17050_i2c_write(MAX17050_FG_QRtable10 , (u8 *)&reg_val_QR10, 2);
		max17050_i2c_write(MAX17050_FG_QRtable00 , (u8 *)&reg_val_QR00, 2);

	}

	if(battery->temp_01c <-50){
		printk(DRIVER_ZONE " Current temperature:%dC\n",battery->temp_01c/10);
		max17050_i2c_read(MAX17050_FG_AvCap, (u8 *)&reg_AvCap, 2);
		battery->shutdown = (!!(reg_AvCap<30));
		printk(DRIVER_ZONE " Current value 0f 0x%x:%d(0x%x) %s\n",MAX17050_FG_AvCap,reg_AvCap/2,reg_AvCap,battery->shutdown? "shutdown":"");
		if(false == battery->shutdown){
			voltage_mV = max17050_get_average_vol();
			battery->shutdown =(!!(voltage_mV<3300));
			printk(DRIVER_ZONE " Current value 0f voltage:%dmv %s\n",voltage_mV, battery->shutdown? "shoutdown":"");
		}
	}


	if ((battery->temp_01c >= -200) && (battery->temp_01c < 0)) {
			reg_val_TGAIN		= 0xDF97;
			reg_val_TOFF		= 0x3326;

			max17050_i2c_write(MAX17050_FG_TGAIN , (u8 *)&reg_val_TGAIN, 2);
			max17050_i2c_write(MAX17050_FG_TOFF, (u8 *)&reg_val_TOFF, 2);
#if MAXIM_BATTERY_DEBUG
			max17050_i2c_read(MAX17050_FG_TGAIN, (u8 *)&val_TGAIN, 2);
			max17050_i2c_read(MAX17050_FG_TOFF, (u8 *)&val_TOFF, 2);
			printk(DRIVER_ZONE "To read MAX17050_FG_TGAIN: 0x%x\n", val_TGAIN);
			printk(DRIVER_ZONE "To read MAX17050_FG_TOFF: 0x%x\n", val_TOFF);
#endif
	} else if ((battery->temp_01c >= 0) && (battery->temp_01c <= 400)) {
			reg_val_TGAIN		= 0xEAC0;
			reg_val_TOFF		= 0x21E2;
			max17050_i2c_write(MAX17050_FG_TGAIN , (u8 *)&reg_val_TGAIN, 2);
			max17050_i2c_write(MAX17050_FG_TOFF, (u8 *)&reg_val_TOFF, 2);
#if MAXIM_BATTERY_DEBUG
			max17050_i2c_read(MAX17050_FG_TGAIN, (u8 *)&val_TGAIN, 2);
			max17050_i2c_read(MAX17050_FG_TOFF, (u8 *)&val_TOFF, 2);
			printk(DRIVER_ZONE "To read MAX17050_FG_TGAIN: 0x%x\n", val_TGAIN);
			printk(DRIVER_ZONE "To read MAX17050_FG_TOFF: 0x%x\n", val_TOFF);
#endif
	} else if ((battery->temp_01c > 400) && (battery->temp_01c <= 700)) {
			reg_val_TGAIN		= 0xDE3E;
			reg_val_TOFF		= 0x2A5A;
			max17050_i2c_write(MAX17050_FG_TGAIN , (u8 *)&reg_val_TGAIN, 2);
			max17050_i2c_write(MAX17050_FG_TOFF, (u8 *)&reg_val_TOFF, 2);
#if MAXIM_BATTERY_DEBUG
			max17050_i2c_read(MAX17050_FG_TGAIN, (u8 *)&val_TGAIN, 2);
			max17050_i2c_read(MAX17050_FG_TOFF, (u8 *)&val_TOFF, 2);
			printk(DRIVER_ZONE "To read MAX17050_FG_TGAIN: 0x%x\n", val_TGAIN);
			printk(DRIVER_ZONE "To read MAX17050_FG_TOFF: 0x%x\n", val_TOFF);
#endif
	}
}

/*========================================================================================

 To check gauge setting if correctly through FG_TempNom, FG_LOCK and FG_MaskSOC register

========================================================================================*/
static void maxim_batt_INI_param_check(void)
{
	u16 tmp[48] = {0};
	int i;
	bool lock = TRUE;
	u16 val_TempNom, val_Msk_SOC, val_LockI, val_LockII = 0;
	u16 reg_gauge_lock, reg_val_TempNom, reg_MskSOC = 0;

	max17050_i2c_read(MAX17050_FG_TempNom, (u8 *)&val_TempNom, 2);

	if(val_TempNom != TEMPNOM_DEFAULT) {
		reg_val_TempNom = TEMPNOM_DEFAULT;
		max17050_i2c_write(MAX17050_FG_TempNom , (u8 *)&reg_val_TempNom, 2);
		printk(DRIVER_ZONE "Gauge TempNom is incorrect ->0x%x\n", val_TempNom);
	} else
		printk(DRIVER_ZONE "Gauge TempNom is correct ->0x%x\n", val_TempNom);

	max17050_i2c_read(MAX17050_FG_LOCK_I, (u8 *)&val_LockI, 2);
	max17050_i2c_read(MAX17050_FG_LOCK_II, (u8 *)&val_LockII, 2);

	max17050_i2c_read(MAX17050_FG_OCV, (u8*)&tmp, 48 * 2);

	for(i = 0; i < 48; i++) {
		if (tmp[i] != 0) {
			lock = FALSE;
			break;
		}
	}

	if(lock == FALSE) {
		printk(DRIVER_ZONE "Gauge model is unlocked-> 0x%x, 0x%x, 0x%x\n", val_LockI, val_LockII, tmp[2]);
		reg_gauge_lock = LOCK_GAUGE_ACCESS;
		max17050_i2c_write(MAX17050_FG_LOCK_I , (u8 *)&reg_gauge_lock, 2);
		max17050_i2c_write(MAX17050_FG_LOCK_II , (u8 *)&reg_gauge_lock, 2);
	} else
		printk(DRIVER_ZONE "Gauge model is locked-> 0x%x, 0x%x, 0x%x\n", val_LockI, val_LockII, tmp[2]);

	max17050_i2c_read(MAX17050_FG_MaskSOC, (u8 *)&val_Msk_SOC, 2);

	if(val_Msk_SOC != MASKSOC_DEFAULT) {
		reg_MskSOC = MASKSOC_DEFAULT;
		max17050_i2c_write(MAX17050_FG_MaskSOC , (u8 *)&reg_MskSOC, 2);
		printk(DRIVER_ZONE "Gauge MaskSOC is incorrect-> 0x%x\n", val_Msk_SOC);
	} else
		printk(DRIVER_ZONE "Gauge MaskSOC is correct-> 0x%x\n", val_Msk_SOC);
}

/*========================================================================================

 To monitor critical gauge register value for maxim vendor debugging

========================================================================================*/
static void get_maxim_batt_INI_info(int update_log)
{
	u16 val_TGAIN, val_TOFF, val_QRtable00, val_DesignCap, val_AvCap, val_CONFIG = 0;
	u16 val_ICHGTerm, val_QRtable10, val_FullCAPNom, val_LearnCFG, val_SHFTCFG, val_MiscCFG = 0;
	u16 val_QRtable20, val_RCOMP0, val_TempCo, val_V_empty, val_QRtable30, val_TempNom = 0;
	u16 val_Lock_I, val_Lock_II, val_MaskSOC, val_QH, batt_cycle, batt_age = 0;
	u16 val_RepCap, val_Qre, val_RemCap, val_FullCap, val_AtRate = 0;

	max17050_i2c_read(MAX17050_FG_Age, (u8 *)&batt_age, 2);
	max17050_i2c_read(MAX17050_FG_QRtable00, (u8 *)&val_QRtable00, 2);
	max17050_i2c_read(MAX17050_FG_Cycles, (u8 *)&batt_cycle, 2);
	max17050_i2c_read(MAX17050_FG_DesignCap, (u8 *)&val_DesignCap, 2);
	max17050_i2c_read(MAX17050_FG_CONFIG, (u8 *)&val_CONFIG, 2);
	max17050_i2c_read(MAX17050_FG_ICHGTerm, (u8 *)&val_ICHGTerm, 2);
	max17050_i2c_read(MAX17050_FG_QRtable10, (u8 *)&val_QRtable10, 2);
	max17050_i2c_read(MAX17050_FG_FullCAPNom, (u8 *)&val_FullCAPNom, 2);
	max17050_i2c_read(MAX17050_FG_LearnCFG, (u8 *)&val_LearnCFG, 2);
	max17050_i2c_read(MAX17050_FG_SHFTCFG, (u8 *)&val_SHFTCFG, 2);
	max17050_i2c_read(MAX17050_FG_MiscCFG, (u8 *)&val_MiscCFG, 2);
	max17050_i2c_read(MAX17050_FG_TGAIN, (u8 *)&val_TGAIN, 2);
	max17050_i2c_read(MAX17050_FG_TOFF, (u8 *)&val_TOFF, 2);
	max17050_i2c_read(MAX17050_FG_QRtable20, (u8 *)&val_QRtable20, 2);
	max17050_i2c_read(MAX17050_FG_RCOMP0, (u8 *)&val_RCOMP0, 2);
	max17050_i2c_read(MAX17050_FG_TempCo, (u8 *)&val_TempCo, 2);
	max17050_i2c_read(MAX17050_FG_V_empty, (u8 *)&val_V_empty, 2);
	max17050_i2c_read(MAX17050_FG_QRtable30, (u8 *)&val_QRtable30, 2);
	max17050_i2c_read(MAX17050_FG_QH, (u8 *)&val_QH, 2);
	max17050_i2c_read(MAX17050_FG_TempNom, (u8 *)&val_TempNom, 2);
	max17050_i2c_read(MAX17050_FG_LOCK_I, (u8 *)&val_Lock_I, 2);
	max17050_i2c_read(MAX17050_FG_LOCK_II, (u8 *)&val_Lock_II, 2);
	max17050_i2c_read(MAX17050_FG_MaskSOC, (u8 *)&val_MaskSOC, 2);
	max17050_i2c_read(MAX17050_FG_AvCap, (u8 *)&val_AvCap, 2);
	max17050_i2c_read(MAX17050_FG_RepCap, (u8 *)&val_RepCap, 2);
	max17050_i2c_read(MAX17050_FG_Qresidual, (u8 *)&val_Qre, 2);
	max17050_i2c_read(MAX17050_FG_RemCap, (u8 *)&val_RemCap, 2);
	max17050_i2c_read(MAX17050_FG_FullCAP, (u8 *)&val_FullCap, 2);
	max17050_i2c_read(MAX17050_FG_AtRate, (u8 *)&val_AtRate, 2);

	if (!update_log) {
		printk(DRIVER_ZONE " 0x04=%4x, 0x05=%4x, 0x07=%4x, 0x0c=%4x, 0x12=%4x, 0x17=%4x, 0x18=%4x, 0x1D=%4x, "
			"0x1E=%4x, 0x1F=%4x, 0x22=%4x, 0x23=%4x, 0x28=%4x, 0x29=%4x, 0x2B=%4x\n",
			val_AtRate, val_RepCap, batt_age, val_Qre, val_QRtable00, batt_cycle, val_DesignCap, val_CONFIG,
			val_ICHGTerm, val_AvCap, val_QRtable10, val_FullCAPNom, val_LearnCFG,
			val_SHFTCFG, val_MiscCFG);

		printk(DRIVER_ZONE " 0x0F=%4x, 0x10=%4x, 0x2C=%4x, 0x2D=%4x, 0x32=%4x, 0x38=%4x, 0x39=%4x, "
			"0x3A=%4x, 0x42=%4x, 0x4D=%4x ,0x24=%4x, 0x62=%4x, 0x63=%4x, 0x33=%4x\n",
			val_RemCap, val_FullCap, val_TGAIN, val_TOFF, val_QRtable20, val_RCOMP0, val_TempCo, val_V_empty,
			val_QRtable30, val_QH, val_TempNom, val_Lock_I, val_Lock_II, val_MaskSOC);
	}
}

BOOL battery_param_update(struct battery_type *battery,	struct protect_flags_type *flags, int update_log)
{
	if (!__battery_param_udpate(battery, update_log)) {
		return FALSE;
	}


	if (flags->is_fake_room_temp) {
		/* Here we still need to know correct temp value,
		and also enable charging when over temp in writeconfig 6 4 status */
		//icey change it
		battery->temp_01c = 250;
#if 0
		printk(DRIVER_ZONE "fake temp=%d(%x)\n",
		battery->temp_01c,
		battery->temp_adc);
#endif
	}

	max17050_batt_temp_accuracy(battery);
	get_maxim_batt_INI_info(update_log);

	__protect_flags_update(battery, flags, update_log);

#if ! HTC_ENABLE_DUMMY_BATTERY
	if (battery->id_index == BATTERY_ID_UNKNOWN) {
		flags->is_charging_enable_available = FALSE;
	}
#else /* HTC_ENABLE_DUMMY_BATTERY*/
	/* do not disable charging for debug stage*/
	flags->is_charging_enable_available = TRUE;
#endif /* HTC_ENABLE_DUMMY_BATTERY*/

	return TRUE;
}

BOOL battery_param_init(struct battery_type *battery)
{
	/* default to 25C unless we can get valid battery temp from adc*/
	battery->temp_01c = 250;

	if (!__battery_param_udpate(battery, 0)) {
		return FALSE;
	}

	/* this is used when accumulate current by software; initial it as 0mAs*/
	battery->software_charge_counter_mAms = 0;

	/*switch (battery->thermal_id) {

	case THERMAL_1000_100_4360:
		TEMP_MAP = TEMP_MAP_1000K_100_4360;
		printk(DRIVER_ZONE "Use 1000(100/4360) Kohm thermal resistance");
		break;
	case THERMAL_300_47_3440:
		TEMP_MAP = TEMP_MAP_300K_47_3440;
		printk(DRIVER_ZONE "Use 300(47/3440) Kohm thermal resistance");
		break;
	case THERMAL_470_100_4360:
		TEMP_MAP = TEMP_MAP_470K_100_4360;
		printk(DRIVER_ZONE "Use 470(100/4360) Kohm thermal resistance\n");
		break;
	default:
		printk(DRIVER_ZONE "Use 300(100/4360) Kohm thermal resistance");
		break;
	}*/

	printk( "[BATT]battery param inited with board name <%s>\n", __FUNCTION__);
	return TRUE;
}

int max17050_gauge_init(void)
{
	int ret;

	printk(DRIVER_ZONE "%s\n",__func__);

	ret = max17050_i2c_init();
	if (ret < 0){
		return ret;
	}

	maxim_batt_INI_param_check();

	max17050_fg_log = kzalloc(sizeof(*max17050_fg_log), GFP_KERNEL);
	if (unlikely(!max17050_fg_log))
		return -ENOMEM;

#if MAXIM_BATTERY_FG_LOG
	INIT_DELAYED_WORK(&max17050_fg_log->fg_log_work,
			htc_battery_fg_log_work_func);

	wake_lock_init(&max17050_fg_log->fg_log_wake_lock, WAKE_LOCK_SUSPEND, "fg_log_enabled");
#endif
	return TRUE;
}

void max17050_gauge_exit(void)
{
	max17050_i2c_exit();
}


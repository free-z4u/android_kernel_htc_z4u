/* arch/arm/mach-sc/board-[project]-bm.c
 *
 * HTC battery, implemented by HTC power team @ Shanghai
 *
 * Date: 2013-04-28
 *
 */

#include <linux/platform_device.h>
#include "devices.h"
#include <linux/max17050_battery.h>
#include <linux/tps65200.h>
#include <mach/htc_battery.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/adc.h>

static int get_thermal_id(void)
{
	return 1; // THERMAL_470_100_4360;
}

struct battery_id_type {
        char * name;
        int id_volt_high;
        int id_volt_low;
};


#define BATTERY_UNKNOWN 0

static struct battery_id_type batt_tbl[] = {
        [BATTERY_UNKNOWN] = {
                .name = "Unknown",
                .id_volt_high = 1,
                .id_volt_low = 0,
        },
        {
                .name = "Maxcell1",
                .id_volt_high = 385,
                .id_volt_low = 0,
        },
        {
                .name = "Maxcell2",
                .id_volt_high = 700,
                .id_volt_low = 385,
        },
};

static int g_batt_id = 0;
int get_battery_id(void)
{
        int id_volt = 0;
        int size, i;

        pr_debug("get_batt_id\n");

        id_volt = htc_adc_to_vol( ADC_CHANNEL_0, sci_adc_get_value( ADC_CHANNEL_0, 1));

        if (id_volt < 0) {
                pr_info("get battery id fail, adc_value=%d.", id_volt);
                return g_batt_id;
        }
        size = ARRAY_SIZE(batt_tbl);

        g_batt_id = BATTERY_UNKNOWN;

        for (i = BATTERY_UNKNOWN + 1; i < size; i++) {
                if ((id_volt < batt_tbl[i].id_volt_high) &&
                        (id_volt >= batt_tbl[i].id_volt_low)) {
                        g_batt_id = i;
                        break;
                }
        }
        pr_debug("battery ID = %d, id_volt = %d\n", g_batt_id, id_volt);

        return g_batt_id;
}

static void cp5dug_poweralg_config_init(struct poweralg_config_type *config)
{
	INT32 batt_id = 0;

	batt_id = get_battery_id();//get_batt_id();
	if (batt_id == BATTERY_ID_UNKNOWN) { //Need to change it to DQ detect
		pr_info("[BATT] %s() is used, batt_id -> %d\n",__func__, batt_id);
		config->full_charging_mv = 4100;
		config->voltage_exit_full_mv = 4000;
		config->capacity_recharge_p = 98;
	} else {
		pr_info("[BATT] %s() is used, batt_id -> %d\n",__func__, batt_id);
		config->full_charging_mv = 4250;
		config->voltage_exit_full_mv = 4000;
		config->capacity_recharge_p = 98;
	}

	config->full_charging_ma = 210; 	/* 0.1C */
	config->full_pending_ma = 50;		/* Enabled*/
	config->full_charging_timeout_sec = 60 * 60; /* keep charging 60 min to 100 */
	config->min_taper_current_mv = 0;	/* disabled */
	config->min_taper_current_ma = 0;	/* disabled */
	config->wait_votlage_statble_sec = 1 * 60;
	config->predict_timeout_sec = 10;
	config->polling_time_in_charging_sec = 30;

	config->enable_full_calibration = TRUE;
	config->enable_weight_percentage = TRUE;
	config->software_charger_timeout_sec = 57600; /* 16hr */
	config->superchg_software_charger_timeout_sec = 0;	/* disabled */
	config->charger_hw_safety_timer_watchdog_sec =  0;	/* disabled */

	config->debug_disable_shutdown = FALSE;
	config->debug_fake_room_temp = FALSE;
	config->debug_fake_percentage = FALSE;

	config->debug_disable_hw_timer = FALSE;
	config->debug_always_predict = FALSE;
	config->full_level = 0;

}

static int cp5dug_update_charging_protect_flag(int ibat_ma, int vbat_mv, int temp_01c, BOOL* chg_allowed, BOOL* hchg_allowed, BOOL* temp_fault)
{
	static int pState = 0;
	int old_pState = pState;
	/* pStates:
		0: initial (temp detection)
		1: temp < 0 degree c
		2: 0 <= temp <= 45 degree c
		3: 45 < temp <= 48 degree c
		4: 48 < temp <= 60 degree c // not used
		5: 60 < temp
	*/
	enum {
		PSTAT_DETECT = 0,
		PSTAT_LOW_STOP,
		PSTAT_NORMAL,
		PSTAT_SLOW,
		PSTAT_LIMITED, /* Keep here, only remove for SPRD battery*/
		PSTAT_HIGH_STOP
	};
	/* generally we assumed that pState implies last temp.
		it won't hold if temp changes faster than sample rate */

	// step 1. check if change state condition is hit
	pr_debug("[BATT] %s(i=%d, v=%d, t=%d, %d, %d)\n",__func__, ibat_ma, vbat_mv, temp_01c, *chg_allowed, *hchg_allowed);
	switch(pState) {
		default:
			pr_info("[BATT] error: unexpected pState\n");
		case PSTAT_DETECT:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			if ((0 <= temp_01c) && (temp_01c <= 480))
				pState = PSTAT_NORMAL;
			if ((480 < temp_01c) && (temp_01c < 580))
				pState = PSTAT_SLOW;
			if (580 <= temp_01c)
				pState = PSTAT_HIGH_STOP;
			break;
		case PSTAT_LOW_STOP:
			if (30 <= temp_01c)
				pState = PSTAT_NORMAL;
			/* suppose never jump to LIMITED/HIGH_STOP from here */
			break;
		case PSTAT_NORMAL:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			else if (580 <= temp_01c)
				pState = PSTAT_HIGH_STOP;
			else if (480 < temp_01c) /* also implies t <= 550 */
				pState = PSTAT_SLOW;
			break;
		case PSTAT_SLOW:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			else if (580 <= temp_01c)
				pState = PSTAT_HIGH_STOP;
			else if (temp_01c <= 480)
				pState = PSTAT_NORMAL;
			break;
		case PSTAT_HIGH_STOP:
			if (temp_01c <= 480)
				pState = PSTAT_NORMAL;
			else if (temp_01c < 550)
				pState = PSTAT_SLOW;
			/* suppose never jump to LOW_STOP from here */
			break;
	}
	if (old_pState != pState)
		pr_info("[BATT] Protect pState changed from %d to %d\n", old_pState, pState);

	/* step 2. check state protect condition */
	/* chg_allowed = TRUE:only means it's allowed no matter it has charger.
		same as hchg_allowed. */
	switch(pState) {
		default:
		case PSTAT_DETECT:
			pr_info("[BATT] error: unexpected pState\n");
			break;
		case PSTAT_LOW_STOP:
			*chg_allowed = FALSE;
			*hchg_allowed = FALSE;
			break;
		case PSTAT_NORMAL:
			*chg_allowed = TRUE;
			*hchg_allowed = FALSE;
			break;
		case PSTAT_SLOW:	/* 4.1V Charge Full, 3.8V recharge */
			*chg_allowed = TRUE;
			*hchg_allowed = TRUE;
			break;
		case PSTAT_HIGH_STOP:
			*chg_allowed = FALSE;
			*hchg_allowed = FALSE;
			break;
	}

	/* update temp_fault */
	if (PSTAT_NORMAL == pState)
		*temp_fault = FALSE;
	else
		*temp_fault = TRUE;
	return pState;
}

/* battery parameters */
UINT32 m_parameter_TWS_SDI_1650mah[] = {
	/* capacity (in 0.01%) -> voltage (in mV)*/
	10000, 4250, 8100, 4101, 5100, 3846,
	2000, 3711, 900, 3641, 0, 3411,
};
UINT32 m_parameter_Formosa_Sanyo_1650mah[] = {
	/* capacity (in 0.01%) -> voltage (in mV)*/
	10000, 4250, 8100, 4145, 5100, 3845,
	2000, 3735, 900, 3625, 0, 3405,
};
UINT32 m_parameter_PydTD_1520mah[] = {
	10000, 4100, 5500, 3826, 2000, 3739,
	500, 3665, 0, 3397,
};
UINT32 m_parameter_unknown_1650mah[] = {
	10000, 4250, 8100, 4145, 5100, 3845,
	2000, 3735, 900, 3625, 0, 3405,
};

static UINT32* m_param_tbl[] = {
	m_parameter_unknown_1650mah,
	m_parameter_TWS_SDI_1650mah,
	m_parameter_Formosa_Sanyo_1650mah,
	m_parameter_PydTD_1520mah
};

static UINT32 pd_m_coef[] = {22, 26, 26, 26};
static UINT32 pd_m_resl[] = {100, 220, 220, 220};
static UINT32 pd_t_coef[] = {100, 220, 220, 220};
static INT32 padc[] = {200, 200, 200, 200};
static INT32 pw[] = {5, 5, 5, 5};

static UINT32* pd_m_coef_tbl[] = {pd_m_coef,};
static UINT32* pd_m_resl_tbl[] = {pd_m_resl,};
static UINT32 capacity_deduction_tbl_01p[] = {0,};

static struct battery_parameter cp5dug_battery_parameter = {
	/* TODO, icey mark it after device ready*/
	.fl_25 = NULL,
	.pd_m_coef_tbl = pd_m_coef_tbl,
	.pd_m_coef_tbl_boot = pd_m_coef_tbl,
	.pd_m_resl_tbl = pd_m_resl_tbl,
	.pd_m_resl_tbl_boot = pd_m_resl_tbl,
	.pd_t_coef = pd_t_coef,
	.padc = padc,
	.pw = pw,
	.capacity_deduction_tbl_01p = capacity_deduction_tbl_01p,
	.id_tbl = NULL,
	.temp_index_tbl = NULL,
	.m_param_tbl = m_param_tbl,
	.m_param_tbl_size = sizeof(m_param_tbl)/sizeof(UINT32*),
};


static struct htc_battery_platform_data htc_battery_pdev_data = {
	/* TODO, icey mark it after device ready*/
	.gpio_mbat_in = 87,
	.gpio_mbat_in_trigger_level = MBAT_IN_HIGH_TRIGGER,
	.mbat_in_keep_charging = 0,
	.mbat_in_unreg_rmt = 1,
	.gpio_vbus_det = EIC_CHARGER_DETECT,
	.chg_limit_active_mask = HTC_BATT_CHG_LIMIT_BIT_TALK,
	.func_show_batt_attr = htc_battery_show_attr,
	.suspend_highfreq_check_reason = SUSPEND_HIGHFREQ_CHECK_BIT_TALK,
	.guage_driver = GAUGE_MAX17050,
	.charger = SWITCH_CHARGER_TPS65200,
	.m2a_cable_detect = 1,
	.enable_bootup_voltage = 3400,
	.func_show_htc_extension_attr = htc_battery_show_htc_extension_attr,
};

struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev	= {
		.platform_data = &htc_battery_pdev_data,
	},
};

static max17050_platform_data max17050_pdev_data = {
	/* TODO, icey mark it after device ready*/
	.func_get_thermal_id = get_thermal_id,
	.func_get_battery_id = get_battery_id,
	.func_poweralg_config_init = cp5dug_poweralg_config_init,
	.func_update_charging_protect_flag = cp5dug_update_charging_protect_flag,
	.r2_kohm = 0,	// use get_battery_id, doesn't need this
	.batt_param = &cp5dug_battery_parameter,
	.func_kick_charger_ic = tps65200_kick_charger_ic,
};

struct platform_device max17050_battery_pdev = {
	.name = "max17050-battery",
	.id = -1,
	.dev = {
		.platform_data = &max17050_pdev_data,
	},
};

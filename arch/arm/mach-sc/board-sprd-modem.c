/*
 * =====================================================================================
 *
 *       Filename:  board-z4dtg-modem.c
 *
 *    Description:  modem basic info.
 *
 *        Version:  1.0
 *        Created:  04/18/2013 05:01:11 PM
 *       Revision:  none
 *       Compiler:  arm-eabi-gcc
 *
 *         Author:  Chuck Huang
 *        Company:  hTc
 *
 * =====================================================================================
 */

#include <linux/platform_device.h>
#include <linux/io.h>

#include <linux/sprd_cproc.h>
#include <linux/sprd_asc.h>
#include <linux/sipc.h>
#include <linux/spipe.h>
#include <linux/spool.h>
#include <linux/seth.h>
#include <sound/saudio.h>
#include <asm/pmu.h>
#include <asm/gpio.h>
#include <mach/board_htc.h>
#include <mach/hardware.h>
#include <mach/pinmap.h>
#include <mach/sci_glb_regs.h>


#define AP2CP_INT_CTRL		(SPRD_IPI_BASE + 0x00)
#define CP2AP_INT_CTRL		(SPRD_IPI_BASE + 0x04)

#ifdef CONFIG_SIPC_TD /* CONFIG_SIPC_TD START*/

#define AP2CPT_IRQ0_TRIG	0x10
#define CPT2AP_IRQ0_CLR		0x10

static int native_tdmodem_reset(void);
static int native_tdmodem_restart(void);

uint32_t cpt_rxirq_status(void)
{
	return 1;
}

void cpt_rxirq_clear(void)
{
	__raw_writel(CPT2AP_IRQ0_CLR, CP2AP_INT_CTRL);
}

void cpt_txirq_trigger(void)
{
	__raw_writel(AP2CPT_IRQ0_TRIG, AP2CP_INT_CTRL);
}

#define AP2CPT_IRQ1_TRIG	0x40
#define CPT2AP_IRQ1_CLR		0x40

uint32_t cpt_rxirq1_status(void)
{
	return 1;
}

void cpt_rxirq1_clear(void)
{
	__raw_writel(CPT2AP_IRQ1_CLR, CP2AP_INT_CTRL);
}

void cpt_txirq1_trigger(void)
{
	__raw_writel(AP2CPT_IRQ1_TRIG, AP2CP_INT_CTRL);
}


#define TD_REG_CLK_ADDR		(SPRD_PMU_BASE + 0x50)
#define TD_REG_RESET_ADDR	(SPRD_PMU_BASE + 0xA8)
#define TD_REG_STATUS_ADDR	(SPRD_PMU_BASE + 0xBC)

static int native_tdmodem_start(void *arg)
{
	u32 state;
	u32 cp1data[3] = {0xe59f0000, 0xe12fff10, CPT_MODEM_ADDR};

	memcpy((void *)(SPRD_IRAM1_BASE + 0x1800), cp1data, sizeof(cp1data));	/* copy cp1 source code */

	*((volatile u32*)TD_REG_RESET_ADDR) |=  0x00000002;	/* reset cp1 */
	*((volatile u32*)TD_REG_CLK_ADDR) &= ~0x02000000;	/* clear cp1 force shutdown */

	while (1) {
		state = *((volatile u32*)TD_REG_STATUS_ADDR);
		if (!(state & (0xf<<16)))
			break;
	}

	*((volatile u32*)TD_REG_CLK_ADDR) &= ~0x10000000;	/* clear cp1 force deep sleep */
	*((volatile u32*)TD_REG_RESET_ADDR) &= ~0x00000002;	/* clear reset cp0 cp1 */

	return 0;
}

static int native_tdmodem_stop(void *arg)
{
	*((volatile u32*)TD_REG_RESET_ADDR) |=  0x00000002;	/* reset cp1 */
	*((volatile u32*)TD_REG_CLK_ADDR) |= 0x10000000;	/* cp1 force deep sleep */
	*((volatile u32*)TD_REG_CLK_ADDR) |= 0x02000000;	/* cp1 force shutdown */

	return 0;
}


#ifdef CONFIG_SPRD_SIM_SWITCH
static int native_sprd_sim_switch(int arg)
{
	u32 val;

	val = *(volatile u32*)(CTL_PIN_BASE + REG_PIN_CTRL2);

	if (arg == 1) {
		val &= ~(0xf << 20);
		val |= (0x01 << 22);
		__raw_writel(val, CTL_PIN_BASE + REG_PIN_CTRL2);
	} else if (arg == 0) {
		val &= ~(0xf << 20);
		val |= (0x1 << 20);
		__raw_writel(val, CTL_PIN_BASE + REG_PIN_CTRL2);
	}

	return 0;
}
#endif


static struct sprd_asc_platform_data sprd_asc_td_pdata = {
	.devname	= "cpt",
	.base           = CPT_START_ADDR,
	.maxsz          = CPT_TOTAL_SIZE,
	.start		= native_tdmodem_start,
	.stop		= native_tdmodem_stop,
	.reset		= native_tdmodem_reset,
	.restart	= native_tdmodem_restart,
	.ntf_irqname	= "cpt_ntf_irq",
	.ntf_irq	= IRQ_CP1_MCU1_INT,
	.ntfirq_status	= cpt_rxirq1_status,
	.ntfirq_clear	= cpt_rxirq1_clear,
	.ntfirq_trigger	= cpt_txirq1_trigger,
	.wtd_irqname	= "cpt_wtd_irq",
	.wtd_irq	= IRQ_CP1_WDG_INT,
	.sim_irqname	= "cpt_sim_irq",
	.status_gpio	= -1,
	.ap_ctrl_rf	= 0,
	.ap_ctrl_ant	= 0,
#ifdef CONFIG_SPRD_SIM_SWITCH
	.sim_switch_enable = 1,
	.simswitch	= native_sprd_sim_switch,
#else
	.sim_switch_enable = 0,
#endif
	.devtype	= 1,
	.segnr		= 10,
	.segs		= {
		{
			.name	= "modem",
			.mode	= S_IWUSR,
			.base	= CPT_MODEM_ADDR,
			.maxsz	= CPT_MODEM_SIZE,
		},
		{
			.name	= "dsp",
			.mode	= S_IWUSR,
			.base	= CPT_DSP_ADDR,
			.maxsz	= CPT_DSP_SIZE,
		},
		{
			.name	= "dlnv",
			.mode	= S_IWUSR,
			.base	= CPT_NVD_ADDR,
			.maxsz	= CPT_NVD_SIZE,
		},
		{
			.name	= "fixnv",
			.mode	= S_IWUSR,
			.base	= CPT_NVF_ADDR,
			.maxsz	= CPT_NVF_SIZE,
		},
		{
			.name	= "runtimenv",
			.mode	= S_IWUSR,
			.base	= CPT_NVR_ADDR,
			.maxsz	= CPT_NVR_SIZE,
		},
		{
			.name	= "protnv",
			.mode	= S_IWUSR,
			.base	= CPT_NVP_ADDR,
			.maxsz	= CPT_NVP_SIZE,
		},
		{
			.name	= "rfnv",
			.mode	= S_IWUSR,
			.base	= CPT_NVC_ADDR,
			.maxsz	= CPT_NVC_SIZE,
		},
		{
			.name	= "c_cmdline",
			.mode	= S_IWUSR,
			.base	= CPT_C_CMDLINE_ADDR,
			.maxsz	= CPT_C_CMDLINE_SIZE,
		},
		{
			.name	= "n_cmdline",
			.mode	= S_IWUSR,
			.base	= CPT_N_CMDLINE_ADDR,
			.maxsz	= CPT_N_CMDLINE_SIZE,
		},
		{
			.name	= "smsg",
			.mode	= S_IRUSR,
			.base	= CPT_RING_ADDR,
			.maxsz	= CPT_RING_SIZE,
		},
	},
};


struct platform_device sprd_asc_td_device = {
	.name		= "sprd_asc",
	.id		= 1,
	.dev		= {.platform_data = &sprd_asc_td_pdata},
};


static struct spipe_init_data sprd_spipe_td_pdata = {
	.name		= "spipe_td",
	.dst		= SIPC_ID_CPT,
	.channel	= SMSG_CH_PIPE,
	.ringnr		= 8,
	.txbuf_size	= 4096,
	.rxbuf_size	= 4096,
};

struct platform_device sprd_spipe_td_device = {
	.name           = "spipe",
	.id             = 0,
	.dev		= {.platform_data = &sprd_spipe_td_pdata},
};

static struct spipe_init_data sprd_slog_td_pdata = {
	.name		= "slog_td",
	.dst		= SIPC_ID_CPT,
	.channel	= SMSG_CH_PLOG,
	.ringnr		= 1,
	.txbuf_size	= 32*1024,
	.rxbuf_size	= 256 * 1024,
};

struct platform_device sprd_slog_td_device = {
	.name           = "spipe",
	.id             = 1,
	.dev		= {.platform_data = &sprd_slog_td_pdata},
};

static struct spipe_init_data sprd_stty_td_pdata = {
	/* to be compatible with vlx mux dev nodes */
	.name		= "stty_td",
	.dst		= SIPC_ID_CPT,
	.channel	= SMSG_CH_TTY,
	.ringnr		= 32,
	.txbuf_size	= 1024,
	.rxbuf_size	= 1024,
};

struct platform_device sprd_stty_td_device = {
	.name           = "spipe",
	.id             = 2,
	.dev		= {.platform_data = &sprd_stty_td_pdata},
};

static struct spool_init_data sprd_spool_td_pdata = {
	.name		= "spool_td",
	.dst		= SIPC_ID_CPT,
	.channel	= SMSG_CH_CTRL,/*TODO*/
	.txblocknum	= 64,
	.txblocksize	= 1516,
	.rxblocknum	= 64,
	.rxblocksize	= 1516,
};

struct platform_device sprd_spool_td_device = {
	.name		= "spool",
	.id		= 0,
	.dev		= {.platform_data = &sprd_spool_td_pdata},
};


static struct seth_init_data sprd_seth0_td_pdata = {
	.name		= "seth_td0",
	.dst		= SIPC_ID_CPT,
	.channel	= SMSG_CH_DATA0,
};

struct platform_device sprd_seth0_td_device = {
	.name           = "seth",
	.id             =  0,
	.dev		= {.platform_data = &sprd_seth0_td_pdata},
};

static struct seth_init_data sprd_seth1_td_pdata = {
	.name		= "seth_td1",
	.dst		= SIPC_ID_CPT,
	.channel	= SMSG_CH_DATA1,
};

struct platform_device sprd_seth1_td_device = {
	.name           = "seth",
	.id             =  1,
	.dev		= {.platform_data = &sprd_seth1_td_pdata},
};

static struct seth_init_data sprd_seth2_td_pdata = {
	.name		= "seth_td2",
	.dst		= SIPC_ID_CPT,
	.channel	= SMSG_CH_DATA2,
};

struct platform_device sprd_seth2_td_device = {
	.name           = "seth",
	.id             =  2,
	.dev		= {.platform_data = &sprd_seth2_td_pdata},
};

static struct saudio_init_data sprd_saudio_td={
	"VIRTUAL AUDIO",
	SIPC_ID_CPT,
	SMSG_CH_VBC,
	SMSG_CH_PLAYBACK,
	SMSG_CH_CAPTURE,
};

struct platform_device sprd_saudio_td_device = {
	.name       = "saudio",
	.id         = 0,
	.dev        = {.platform_data=&sprd_saudio_td},
};

//HTC_AUD_START add for voip
static struct saudio_init_data sprd_saudio_voip = {
	"saudiovoip",
	SIPC_ID_CPT,
	SMSG_CH_CTRL_VOIP,
	SMSG_CH_PLAYBACK_VOIP,
	SMSG_CH_CAPTURE_VOIP,
};

struct platform_device sprd_saudio_voip_device = {
	.name       = "saudio",
	.id         = 2,
	.dev        = {.platform_data=&sprd_saudio_voip},
};
//HTC_AUD_END add for voip

#endif /* CONFIG_SIPC_TD END*/

#ifdef CONFIG_SIPC_WCDMA /* CONFIG_SIPC_WCDMA START */

#define AP2CPW_IRQ0_TRIG	0x01
#define CPW2AP_IRQ0_CLR		0x01

static int native_wcdmamodem_reset(void);
static int native_wcdmamodem_restart(void);

uint32_t cpw_rxirq_status(void)
{
	return 1;
}

void cpw_rxirq_clear(void)
{
	__raw_writel(CPW2AP_IRQ0_CLR, CP2AP_INT_CTRL);
}

void cpw_txirq_trigger(void)
{
	__raw_writel(AP2CPW_IRQ0_TRIG, AP2CP_INT_CTRL);
}

#define AP2CPW_IRQ1_TRIG	0x04
#define CPW2AP_IRQ1_CLR		0x04

uint32_t cpw_rxirq1_status(void)
{
	return 1;
}

void cpw_rxirq1_clear(void)
{
	__raw_writel(CPW2AP_IRQ1_CLR, CP2AP_INT_CTRL);
}


void cpw_txirq1_trigger(void)
{
	__raw_writel(AP2CPW_IRQ1_TRIG, AP2CP_INT_CTRL);
}


#define WCDMA_REG_CLK_ADDR	(SPRD_PMU_BASE + 0x3C)
#define WCDMA_REG_RESET_ADDR	(SPRD_PMU_BASE + 0xA8)
#define WCDMA_REG_STATUS_ADDR	(SPRD_PMU_BASE + 0xB8)

static int native_wcdmamodem_start(void *arg)
{
	u32 state;
	u32 cp0data[3] = {0xe59f0000, 0xe12fff10, CPW_MODEM_ADDR};

	memcpy((void *)SPRD_IRAM1_BASE, cp0data, sizeof(cp0data));		/* copy cp0 source code */

	*((volatile u32*)WCDMA_REG_RESET_ADDR) |= 0x00000001;	/* reset cp0 */
	*((volatile u32*)WCDMA_REG_CLK_ADDR) &= ~0x02000000;	/* clear cp0 force shutdown */

	while (1) {
		state = *((volatile u32*)WCDMA_REG_STATUS_ADDR);
		if (!(state & (0xf<<28)))
			break;
	}

	*((volatile u32*)WCDMA_REG_CLK_ADDR) &= ~0x10000000;	/* clear cp0 force deep sleep */
	*((volatile u32*)WCDMA_REG_RESET_ADDR) &= ~0x00000001;	/* clear reset cp0 */

	return 0;
}

static int native_wcdmamodem_stop(void *arg)
{
	*((volatile u32*)WCDMA_REG_RESET_ADDR) |= 0x00000001;	/* reset cp0 */
	*((volatile u32*)WCDMA_REG_CLK_ADDR) |= 0x10000000;	/* cp0 force deep sleep */
	*((volatile u32*)WCDMA_REG_CLK_ADDR) |= 0x02000000;	/* cp0 force shutdown */

	return 0;
}


#ifdef CONFIG_VIA_SIM_SWITCH
static int native_via_sim_switch(int arg)
{
	if (arg == 1)
		gpio_direction_output(GPIO152_GPIO, 1);
	else
		gpio_direction_output(GPIO152_GPIO, 0);

	return 0;
}
#endif


static struct sprd_asc_platform_data sprd_asc_wcdma_pdata = {
	.devname	= "cpw",
	.base           = CPW_START_ADDR,
	.maxsz          = CPW_TOTAL_SIZE,
	.start		= native_wcdmamodem_start,
	.stop		= native_wcdmamodem_stop,
	.reset		= native_wcdmamodem_reset,
	.restart	= native_wcdmamodem_restart,
	.ntf_irqname	= "cpw_ntf_irq",
	.ntf_irq	= IRQ_CP0_MCU1_INT,
	.ntfirq_status	= cpw_rxirq1_status,
	.ntfirq_clear	= cpw_rxirq1_clear,
	.ntfirq_trigger = cpw_txirq1_trigger,
	.wtd_irqname	= "cpw_wtd_irq",
	.wtd_irq	= IRQ_CP0_WDG_INT,
	.sim_irqname	= "cpw_sim_irq",
	.status_gpio	= -1,
#ifdef CONFIG_AP_CTRL_RF
	.ap_ctrl_rf	= 1,
#else
	.ap_ctrl_rf	= 0,
#endif
#ifdef CONFIG_AP_CTRL_ANT
	.ap_ctrl_ant	= 1,
#else
	.ap_ctrl_ant	= 0,
#endif
#ifdef CONFIG_VIA_SIM_SWITCH
	.sim_switch_enable = 1,
	.simswitch	= native_via_sim_switch,
#else
	.sim_switch_enable = 0,
#endif
	.devtype	= 1,
	.segnr		= 10,
	.segs		= {
		{
			.name	= "modem",
			.mode	= S_IWUSR,
			.base	= CPW_MODEM_ADDR,
			.maxsz	= CPW_MODEM_SIZE,
		},
		{
			.name	= "dsp",
			.mode	= S_IWUSR,
			.base	= CPW_DSP_ADDR,
			.maxsz	= CPW_DSP_SIZE,
		},
		{
			.name	= "dlnv",
			.mode	= S_IWUSR,
			.base	= CPW_NVD_ADDR,
			.maxsz	= CPW_NVD_SIZE,
		},
		{
			.name	= "fixnv",
			.mode	= S_IWUSR,
			.base	= CPW_NVF_ADDR,
			.maxsz	= CPW_NVF_SIZE,
		},
		{
			.name	= "runtimenv",
			.mode	= S_IWUSR,
			.base	= CPW_NVR_ADDR,
			.maxsz	= CPW_NVR_SIZE,
		},
		{
			.name	= "protnv",
			.mode	= S_IWUSR,
			.base	= CPW_NVP_ADDR,
			.maxsz	= CPW_NVP_SIZE,
		},
		{
			.name	= "rfnv",
			.mode	= S_IWUSR,
			.base	= CPW_NVC_ADDR,
			.maxsz	= CPW_NVC_SIZE,
		},
		{
			.name	= "c_cmdline",
			.mode	= S_IWUSR,
			.base	= CPW_C_CMDLINE_ADDR,
			.maxsz	= CPW_C_CMDLINE_SIZE,
		},
		{
			.name	= "n_cmdline",
			.mode	= S_IWUSR,
			.base	= CPW_N_CMDLINE_ADDR,
			.maxsz	= CPW_N_CMDLINE_SIZE,
		},
		{
			.name	= "smsg",
			.mode	= S_IRUSR,
			.base	= CPW_RING_ADDR,
			.maxsz	= CPW_RING_SIZE,
		},
	},

};

struct platform_device sprd_asc_wcdma_device = {
	.name		= "sprd_asc",
	.id		= 0,
	.dev		= {.platform_data = &sprd_asc_wcdma_pdata},
};

static struct spipe_init_data sprd_spipe_wcdma_pdata = {
	.name		= "spipe_w",
	.dst		= SIPC_ID_CPW,
	.channel	= SMSG_CH_PIPE,
	.ringnr		= 8,
	.txbuf_size	= 4096,
	.rxbuf_size	= 4096,
};
struct platform_device sprd_spipe_wcdma_device = {
	.name           = "spipe",
	.id             = 3,
	.dev		= {.platform_data = &sprd_spipe_wcdma_pdata},
};

static struct spipe_init_data sprd_slog_wcdma_pdata = {
	.name		= "slog_w",
	.dst		= SIPC_ID_CPW,
	.channel	= SMSG_CH_PLOG,
	.ringnr		= 1,
	.txbuf_size	= 32 * 1024,
	.rxbuf_size	= 256 * 1024,
};
struct platform_device sprd_slog_wcdma_device = {
	.name           = "spipe",
	.id             = 4,
	.dev		= {.platform_data = &sprd_slog_wcdma_pdata},
};

static struct spipe_init_data sprd_stty_wcdma_pdata = {
	.name		= "stty_w",
	.dst		= SIPC_ID_CPW,
	.channel	= SMSG_CH_TTY,
	.ringnr		= 32,
	.txbuf_size	= 1024,
	.rxbuf_size	= 1024,
};
struct platform_device sprd_stty_wcdma_device = {
	.name           = "spipe",
	.id             = 5,
	.dev		= {.platform_data = &sprd_stty_wcdma_pdata},
};

static struct seth_init_data sprd_seth0_wcdma_pdata = {
	.name		= "seth_w0",
	.dst		= SIPC_ID_CPW,
	.channel	= SMSG_CH_DATA0,
};
struct platform_device sprd_seth0_wcdma_device = {
	.name           = "seth",
	.id             =  3,
	.dev		= {.platform_data = &sprd_seth0_wcdma_pdata},
};

static struct seth_init_data sprd_seth1_wcdma_pdata = {
	.name		= "seth_w1",
	.dst		= SIPC_ID_CPW,
	.channel	= SMSG_CH_DATA1,
};
struct platform_device sprd_seth1_wcdma_device = {
	.name           = "seth",
	.id             =  4,
	.dev		= {.platform_data = &sprd_seth1_wcdma_pdata},
};

static struct seth_init_data sprd_seth2_wcdma_pdata = {
	.name		= "seth_w2",
	.dst		= SIPC_ID_CPW,
	.channel	= SMSG_CH_DATA2,
};
struct platform_device sprd_seth2_wcdma_device = {
	.name           = "seth",
	.id             =  5,
	.dev		= {.platform_data = &sprd_seth2_wcdma_pdata},
};

static struct saudio_init_data sprd_saudio_wcdma={
	"VIRTUAL AUDIO W",
	SIPC_ID_CPW,
	SMSG_CH_VBC,
	SMSG_CH_PLAYBACK,
	SMSG_CH_CAPTURE,
};

struct platform_device sprd_saudio_wcdma_device = {
	.name       = "saudio",
	.id         = 1,
	.dev        = {.platform_data=&sprd_saudio_wcdma},
};

#endif /* CONFIG_SIPC_WCDMA END */

#ifdef CONFIG_SIPC_DEBUG
static unsigned int get_cp_dbgstat(unsigned int arg)
{
	u32 state;

	if (arg > 0 && arg < 0x128 && arg % 4 == 0)
		state = *((volatile u32*)(SPRD_PMU_BASE + arg));
	else
		state = -1;

	return state;
}


static struct sprd_asc_platform_data sprd_asc_debug_pdata = {
	.devname	= "cpm",
	.base           = SIPC_SMEM_ADDR,
	.maxsz          = SZ_1M * 4,
	.get_cp_stat	= get_cp_dbgstat,
	.ntf_irq	= -1,
	.wtd_irq	= -1,
	.status_gpio	= -1,
	.ap_ctrl_rf	= 0,
	.ap_ctrl_ant	= 0,
	.devtype	= 0,
	.segnr		= 1,
	.segs		= {
		{
			.name	= "smem",
			.mode	= S_IRUSR,
			.base	= SIPC_SMEM_ADDR,
			.maxsz	= SZ_1M * 4,
		},
	},
};

struct platform_device sprd_asc_debug_device = {
	.name		= "sprd_asc",
	.id		= 2,
	.dev		= {.platform_data = &sprd_asc_debug_pdata},
};
#endif

static struct platform_device *modem_devices[] __initdata = {
#ifdef CONFIG_SIPC_TD
	&sprd_spipe_td_device,
	&sprd_slog_td_device,
	&sprd_stty_td_device,
	&sprd_seth0_td_device,
	&sprd_seth1_td_device,
	&sprd_seth2_td_device,
	&sprd_asc_td_device,
	&sprd_saudio_td_device,
	&sprd_saudio_voip_device, //add for voip support
#endif
#ifdef CONFIG_SIPC_WCDMA
       &sprd_spipe_wcdma_device,
       &sprd_slog_wcdma_device,
       &sprd_stty_wcdma_device,
       &sprd_seth0_wcdma_device,
       &sprd_seth1_wcdma_device,
       &sprd_seth2_wcdma_device,
       &sprd_asc_wcdma_device,
       &sprd_saudio_wcdma_device,
#endif
#ifdef CONFIG_SIPC_DEBUG
	&sprd_asc_debug_device,
#endif
};

#ifdef CONFIG_MODEM_SILENT_RESET
#ifdef CONFIG_SIPC_TD
struct spipe_init_data *sprd_spipe_resource_td[] = {
	&sprd_spipe_td_pdata,
	&sprd_slog_td_pdata,
	&sprd_stty_td_pdata
};

struct seth_init_data *sprd_seth_resource_td[] = {
	&sprd_seth0_td_pdata,
	&sprd_seth1_td_pdata,
	&sprd_seth2_td_pdata
};

static int native_tdmodem_reset(void)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(sprd_spipe_resource_td); i++ )
	{
		sbuf_reset(sprd_spipe_resource_td[i]->dst, sprd_spipe_resource_td[i]->channel);
	}

	for(i = 0; i < ARRAY_SIZE(sprd_seth_resource_td); i++ )
	{
		sblock_reset(sprd_seth_resource_td[i]->dst, sprd_seth_resource_td[i]->channel);
		seth_reset(sprd_seth_resource_td[i]->dst, sprd_seth_resource_td[i]->channel);
	}

	sblock_reset(sprd_saudio_td.dst, sprd_saudio_td.ctrl_channel);
	sblock_reset(sprd_saudio_td.dst, sprd_saudio_td.playback_channel);
	sblock_reset(sprd_saudio_td.dst, sprd_saudio_td.capture_channel);

		//HTC_AUD_START add for voip support
	sblock_reset(sprd_saudio_voip.dst, sprd_saudio_voip.ctrl_channel);
        sblock_reset(sprd_saudio_voip.dst, sprd_saudio_voip.playback_channel);
        sblock_reset(sprd_saudio_voip.dst, sprd_saudio_voip.capture_channel);
	//HTC_AUD_END
	return 0;
}

static int native_tdmodem_restart(void)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(sprd_spipe_resource_td); i++ )
	{
		sbuf_restart(sprd_spipe_resource_td[i]->dst, sprd_spipe_resource_td[i]->channel);
	}

	for(i = 0; i < ARRAY_SIZE(sprd_seth_resource_td); i++ )
	{
		sblock_restart(sprd_seth_resource_td[i]->dst, sprd_seth_resource_td[i]->channel);
		seth_restart(sprd_seth_resource_td[i]->dst, sprd_seth_resource_td[i]->channel);
	}

	sblock_restart(sprd_saudio_td.dst, sprd_saudio_td.ctrl_channel);
	sblock_restart(sprd_saudio_td.dst, sprd_saudio_td.playback_channel);
	sblock_restart(sprd_saudio_td.dst, sprd_saudio_td.capture_channel);

		//HTC_AUD_START add for voip support
	sblock_restart(sprd_saudio_voip.dst, sprd_saudio_voip.ctrl_channel);
	sblock_restart(sprd_saudio_voip.dst, sprd_saudio_voip.playback_channel);
	sblock_restart(sprd_saudio_voip.dst, sprd_saudio_voip.capture_channel);
	//HTC_AUD_END
	return 0;
}
#else
static inline int native_tdmodem_reset(void)
{
	return 0;
}

static inline int native_tdmodem_restart(void)
{
	return 0;
}
#endif


#ifdef CONFIG_SIPC_WCDMA
struct spipe_init_data *sprd_spipe_resource_wcdma[] = {
	&sprd_spipe_wcdma_pdata,
	&sprd_slog_wcdma_pdata,
	&sprd_stty_wcdma_pdata
};

struct seth_init_data *sprd_seth_resource_wcdma[] = {
	&sprd_seth0_wcdma_pdata,
	&sprd_seth1_wcdma_pdata,
	&sprd_seth2_wcdma_pdata
};

static int native_wcdmamodem_reset(void)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(sprd_spipe_resource_wcdma); i++ )
	{
		sbuf_reset(sprd_spipe_resource_wcdma[i]->dst, sprd_spipe_resource_wcdma[i]->channel);
	}

	for(i = 0; i < ARRAY_SIZE(sprd_seth_resource_wcdma); i++ )
	{
		sblock_reset(sprd_seth_resource_wcdma[i]->dst, sprd_seth_resource_wcdma[i]->channel);
		seth_reset(sprd_seth_resource_wcdma[i]->dst, sprd_seth_resource_wcdma[i]->channel);
	}

	sblock_reset(sprd_saudio_wcdma.dst, sprd_saudio_wcdma.ctrl_channel);
	sblock_reset(sprd_saudio_wcdma.dst, sprd_saudio_wcdma.playback_channel);
	sblock_reset(sprd_saudio_wcdma.dst, sprd_saudio_wcdma.capture_channel);

	return 0;
}

static int native_wcdmamodem_restart(void)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(sprd_spipe_resource_wcdma); i++ )
	{
		sbuf_restart(sprd_spipe_resource_wcdma[i]->dst, sprd_spipe_resource_wcdma[i]->channel);
	}

	for(i = 0; i < ARRAY_SIZE(sprd_seth_resource_wcdma); i++ )
	{
		sblock_restart(sprd_seth_resource_wcdma[i]->dst, sprd_seth_resource_wcdma[i]->channel);
		seth_restart(sprd_seth_resource_wcdma[i]->dst, sprd_seth_resource_wcdma[i]->channel);
	}

	sblock_restart(sprd_saudio_wcdma.dst, sprd_saudio_wcdma.ctrl_channel);
	sblock_restart(sprd_saudio_wcdma.dst, sprd_saudio_wcdma.playback_channel);
	sblock_restart(sprd_saudio_wcdma.dst, sprd_saudio_wcdma.capture_channel);

	return 0;
}
#else
static inline int native_wcdmamodem_reset(void)
{
	return 0;
}

static inline int native_wcdmamodem_restart(void)
{
	return 0;
}
#endif
#endif


static void sim_ctrl_init(void)
{
	u32 val;

	val = *(volatile u32*)(CTL_PIN_BASE + REG_PIN_CTRL2);

#if defined(CONFIG_MACH_Z4DTG) || defined(CONFIG_MACH_Z4TD) || defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5VEDTU)
	val &= ~(0xf << 20);
	val |= (0x1 << 20);
	__raw_writel(val, CTL_PIN_BASE + REG_PIN_CTRL2);
#elif defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDWG)
	val &= ~(0xf << 20);
	val |= (0x1 << 22);
	__raw_writel(val, CTL_PIN_BASE + REG_PIN_CTRL2);
#endif
}


void __init board_init_modem(void)
{
	int ver;

#ifdef CONFIG_MACH_Z4DTG
       	ver = z4dtg_get_board_revision();
#elif defined CONFIG_MACH_Z4TD
        ver = z4td_get_board_revision();
#elif defined CONFIG_MACH_CP5DTU
       	ver = cp5dtu_get_board_revision();
#elif defined CONFIG_MACH_CP5DWG
       	ver = cp5dwg_get_board_revision();
#elif defined CONFIG_MACH_CP5VEDTU
       	ver = cp5vedtu_get_board_revision();
#elif defined CONFIG_MACH_CP5VEDWG
       	ver = cp5vedwg_get_board_revision();
#elif defined CONFIG_MACH_CP5DUG
       	ver = cp5dug_get_board_revision();
#endif
	printk(KERN_INFO "%s\n", __func__);

	if (ver > BOARD_EVM) {
#if defined(CONFIG_MACH_Z4DTG) || defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5VEDTU)
#ifdef CONFIG_SIPC_TD
		sprd_asc_td_pdata.status_gpio = 181;
#endif
#ifdef CONFIG_SIPC_WCDMA
		sprd_asc_wcdma_pdata.status_gpio = 182;
#endif
#elif defined CONFIG_MACH_Z4TD
#ifdef CONFIG_SIPC_TD
		sprd_asc_td_pdata.status_gpio = 181;
#endif
#elif defined CONFIG_MACH_CP5DUG
#ifdef CONFIG_SIPC_TD
		sprd_asc_td_pdata.status_gpio = 182;
#endif
#ifdef CONFIG_SIPC_WCDMA
		sprd_asc_wcdma_pdata.status_gpio = 181;
#endif
#elif defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDWG)
#ifdef CONFIG_SIPC_TD
		sprd_asc_td_pdata.status_gpio = 182;
#endif
#ifdef CONFIG_SIPC_WCDMA
		sprd_asc_wcdma_pdata.status_gpio = 181;
#endif
#endif
	} else if (ver == BOARD_EVM) {
		printk(KERN_INFO "evm board for modem, no sim detect\n");
	} else
		printk(KERN_ERR "unknown board");

	sim_ctrl_init();

	platform_add_devices(modem_devices, ARRAY_SIZE(modem_devices));
}

/* linux/arch/arm/mach-sc/devices_htc.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (C) 2007-2009 HTC Corporation.
 * Author: Future Zhou <future_zhou@htc.com>
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
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/module.h>

#include <asm/mach/flash.h>
#include <asm/setup.h>
#include <linux/notifier.h>

#include <mach/board_htc.h>

static int mfg_modem_mode = 0;
static char *sc88xx_chipset_id;
static char *df_serialno = "000000000000";
static char *board_sn;
#define SHIP_BUILD	0
#define MFG_BUILD	1
#define ENG_BUILD	2


int __init board_mfg_mdoem_mode_init(char *s)
{
	if (!strcmp(s, "normal"))
		mfg_modem_mode = 0;
	else if (!strcmp(s, "radiorouterA"))
		mfg_modem_mode = 2;
	else if (!strcmp(s, "radiorouter"))
		mfg_modem_mode = 1;

	return 1;
}
__setup("androidboot.testmode=", board_mfg_mdoem_mode_init);


int board_mfg_modem_mode(void)
{
	return mfg_modem_mode;
}

EXPORT_SYMBOL(board_mfg_modem_mode);

static unsigned long kernel_flag;
int __init kernel_flag_init(char *s)
{
	int ret;
	ret = strict_strtoul(s, 16, &kernel_flag);
	return 1;
}
__setup("kernelflag=", kernel_flag_init);

unsigned long get_kernel_flag(void)
{
	return kernel_flag;
}

static unsigned g_htc_skuid;
unsigned htc_get_skuid(void)
{
	return g_htc_skuid;
}
EXPORT_SYMBOL(htc_get_skuid);

static unsigned g_htc_pcbid;
unsigned htc_get_pcbid(void)
{
	return g_htc_pcbid;
}
EXPORT_SYMBOL(htc_get_pcbid);

static unsigned g_htc_bomid;
unsigned htc_get_bomid(void)
{
	return g_htc_bomid;
}
EXPORT_SYMBOL(htc_get_bomid);
/*------------------------CMDLINE start-----------------------------*/

static int mfg_mode;
int __init board_mfg_mode_init(char *s)
{
	if (!strcmp(s, "normal"))
		mfg_mode = 0;
	else if (!strcmp(s, "factory2"))
		mfg_mode = 1;
	else if (!strcmp(s, "recovery"))
		mfg_mode = 2;
	else if (!strcmp(s, "charge"))
		mfg_mode = 3;
	else if (!strcmp(s, "power_test"))
		mfg_mode = 4;
	else if (!strcmp(s, "offmode_charging"))
		mfg_mode = 5;
	else if (!strcmp(s, "mfgkernel:diag58"))
		mfg_mode = 6;
	else if (!strcmp(s, "gift_mode"))
		mfg_mode = 7;
	else if (!strcmp(s, "mfgkernel"))
		mfg_mode = 8;
	else if (!strcmp(s, "krouter_via"))
		mfg_mode = 9;
	else if (!strcmp(s, "radiorouter_via") || !strcmp(s, "krouter_ets") || !strcmp(s, "krouter_usb"))
		mfg_mode = 10;
	else if (!strcmp(s, "td_router_uart") || !strcmp(s, "w_router_uart") || !strcmp(s, "radiorouter_sprd_td") || !strcmp(s, "radiorouter_sprd_w")) {
		mfg_mode = 11;
	}
	return 1;
}
__setup("androidboot.mode=", board_mfg_mode_init);


int board_mfg_mode(void)
{
	return mfg_mode;
}

EXPORT_SYMBOL(board_mfg_mode);


/* HTC_PROC_EMMC_START */
static char *emmc_tag;
static int __init board_set_emmc_tag(char *get_hboot_emmc)
{
	if (strlen(get_hboot_emmc))
		emmc_tag = get_hboot_emmc;
	else
		emmc_tag = NULL;
	return 1;
}
__setup("androidboot.emmc=", board_set_emmc_tag);

int board_emmc_boot(void)
{
	if (emmc_tag) {
	if (!strcmp(emmc_tag, "true"))
			return 1;
	}

	return 0;
}
struct flash_platform_data sc_nand_data = {
	.parts		= NULL,
	.nr_parts	= 0,
};
/* HTC_PROC_EMMC_END */

static int __init board_get_chipset_id(char *get_chipset_id)
{
	if (strlen(get_chipset_id))
		sc88xx_chipset_id = get_chipset_id;
	else
		sc88xx_chipset_id = NULL;
	printk(KERN_DEBUG "Chipset ID: %s\n", sc88xx_chipset_id);
	return 1;
}
__setup("chipset_id=", board_get_chipset_id);

char *get_sc88xx_chipset_id(void)
{
	return sc88xx_chipset_id;
}

EXPORT_SYMBOL(get_sc88xx_chipset_id);


/* For off-mode alarm */
static int offalarm_enabled;
static int __init enable_offalarm_setup(char *str)
{
	int cal = simple_strtol(str, NULL, 0);

	offalarm_enabled = cal;
	return 1;
}
__setup("enable_offalarm=", enable_offalarm_setup);

static int htc_is_offalarm_enabled(void)
{
	return offalarm_enabled;
}
EXPORT_SYMBOL(htc_is_offalarm_enabled);


static int __init board_serialno_setup(char *serialno)
{
	char *str;

	if (board_mfg_mode() || !strlen(serialno))
		str = df_serialno;
	else
		str = serialno;
#ifdef CONFIG_USB_FUNCTION
	msm_hsusb_pdata.serial_number = str;
#endif
#ifdef CONFIG_USB_ANDROID
	android_usb_pdata.serial_number = str;
#endif
	printk(KERN_INFO "board_serialno_setup: board_sn = %s\n", str);
	board_sn = str;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

char *board_serialno(void)
{
	if(board_sn)
		return board_sn;
	else
		return df_serialno;
}


static int usb_ats;
int __init board_ats_init(char *s)
{
	usb_ats = simple_strtoul(s, 0, 10);
	return 1;
}
__setup("ats=", board_ats_init);


int board_get_usb_ats(void)
{
	return usb_ats;
}

EXPORT_SYMBOL(board_get_usb_ats);
static int build_flag;
static int __init board_bootloader_setup(char *str)
{
	char temp[strlen(str) + 1];
	char *p = NULL;
	char *build = NULL;
	char *args = temp;

	printk(KERN_INFO "[K] %s: %s\n", __func__, str);

	strcpy(temp, str);

	/*parse the last parameter*/
	while ((p = strsep(&args, ".")) != NULL) build = p;

	/* Sometime hboot version would change from .X000 to .X001, .X002,...
	 * So compare the first character to avoid unnecessary error.
	 */
	if (build) {
		if (build[0] == '0') {
			printk(KERN_INFO "[K] %s: SHIP BUILD\n", __func__);
			build_flag = SHIP_BUILD;
		} else if (build[0] == '2') {
			printk(KERN_INFO "[K] %s: ENG BUILD\n", __func__);
			build_flag = ENG_BUILD;
		} else if (build[0] == '1') {
			printk(KERN_INFO "[K] %s: MFG BUILD\n", __func__);
			build_flag = MFG_BUILD;
		} else {
			printk(KERN_INFO "[K] %s: default ENG BUILD\n", __func__);
			build_flag = ENG_BUILD;
		}
	}
	return 1;
}
__setup("androidboot.bootloader=", board_bootloader_setup);

int board_build_flag(void)
{
	return build_flag;
}

EXPORT_SYMBOL(board_build_flag);

/*------------------------CMDLINE end------------------------------*/

/*------------------------TAG Start------------------------------*/
/*htc writeconfig*/
#define ATAG_HTCCONFIG 0x54410449
#define HTC_CONFIG_SIZE 11

static unsigned g_htcconfig[HTC_CONFIG_SIZE];
int __init parse_tag_htc_config(const struct tag *tag)
{
	int i;
	int find = 0;
	struct tag *t = (struct tag *)tag;

	memset(g_htcconfig, 0x0, sizeof(g_htcconfig));

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_HTCCONFIG) {
			find = 1;
			break;
		}
	}

	if (find) {
		for(i=0; i<HTC_CONFIG_SIZE; i++){
			g_htcconfig[i] = *((unsigned *)(&t->u)+i);
			printk("HTC config%d: %x \n", i, g_htcconfig[i]);
		}
	}

	return 0;
}
__tagtable(ATAG_HTCCONFIG, parse_tag_htc_config);


unsigned htc_get_config(unsigned id)
{
	if( id>=HTC_CONFIG_SIZE ) return 0;

	return g_htcconfig[id];
}
EXPORT_SYMBOL(htc_get_config);

unsigned int als_kadc;
static int __init parse_tag_als_calibration(const struct tag *tag)
{
	als_kadc = tag->u.als_kadc.kadc;
	return 0;
}
__tagtable(ATAG_ALS, parse_tag_als_calibration);

#define ATAG_WS		0x54410023
unsigned int ws_kadc;
static int __init parse_tag_ws_calibration(const struct tag *tag)
{
	ws_kadc = tag->u.als_kadc.kadc;
	return 0;
}
__tagtable(ATAG_WS, parse_tag_ws_calibration);

unsigned int gs_kvalue;
static int __init parse_tag_gs_calibration(const struct tag *tag)
{
	gs_kvalue = tag->u.revision.rev;
	printk(KERN_DEBUG "[K] %s: gs_kvalue = 0x%x\n", __func__, gs_kvalue);
	return 0;
}
__tagtable(ATAG_GS, parse_tag_gs_calibration);

unsigned int ps_kparam1;
unsigned int ps_kparam2;
static int __init parse_tag_ps_calibration(const struct tag *tag)
{
	ps_kparam1 = tag->u.serialnr.low;
	ps_kparam2 = tag->u.serialnr.high;
	printk(KERN_INFO "[K] %s: ps_kparam1 = 0x%x, ps_kparam2 = 0x%x\n",
		__func__, ps_kparam1, ps_kparam2);
	return 0;
}
__tagtable(ATAG_PS, parse_tag_ps_calibration);

BLOCKING_NOTIFIER_HEAD(psensor_notifier_list);
int register_notifier_by_psensor(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&psensor_notifier_list, nb);
}
int unregister_notifier_by_psensor(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&psensor_notifier_list, nb);
}

#define ATAG_SKUID 0x4d534D73
int __init parse_tag_skuid(const struct tag *tags)
{
	int skuid = 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_SKUID) {
			printk(KERN_DEBUG "[K] find the skuid tag\n");
			find = 1;
			break;
		}
	}

	/* HTC_KER_MOD_START, Winster @API for checking PCB ID. */
	if (find) {
		skuid = t->u.revision.rev;
		g_htc_skuid = skuid;
	}
	/* HTC_KER_MOD_END, Winster @API for checking PCB ID. */
	printk(KERN_DEBUG "[K] parse_tag_skuid: hwid = 0x%x\n", skuid);
	return skuid;
}
__tagtable(ATAG_SKUID, parse_tag_skuid);

#define ATAG_HTC_PCBID 0x54410008
int __init parse_tag_pcbid(const struct tag *tags)
{
	int pcbid= 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_HTC_PCBID) {
			printk(KERN_DEBUG "find the pcbid tag\n");
			find = 1;
			break;
		}
	}

	if (find) {
		pcbid = t->u.revision.rev;
		g_htc_pcbid = pcbid;
	}
	printk(KERN_DEBUG "[K] parse_tag_pcbid: pcbid = 0x%x\n", pcbid);
	return pcbid;
}
__tagtable(ATAG_HTC_PCBID, parse_tag_pcbid);

#define ATAG_HTC_BOMID 0x5441D116
int __init parse_tag_bomid(const struct tag *tags)
{
	int bomid= 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_HTC_BOMID) {
			printk(KERN_DEBUG "find the bomid tag\n");
			find = 1;
			break;
		}
	}

	if (find) {
		bomid = t->u.revision.rev;
		g_htc_bomid = bomid;
	}
	printk(KERN_DEBUG "[K] parse_tag_bomid: bomid = 0x%x\n", bomid);
	return bomid;
}
__tagtable(ATAG_HTC_BOMID, parse_tag_bomid);

#define ATAG_ENGINEERID 0x4d534D75
unsigned engineerid;

EXPORT_SYMBOL(engineerid);

int __init parse_tag_engineerid(const struct tag *tags)
{
	int find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_ENGINEERID) {
			printk(KERN_DEBUG "[K] find the engineer tag\n");
			find = 1;
			break;
		}
	}

	if (find)
		engineerid = t->u.revision.rev;
	printk(KERN_DEBUG "[K] parse_tag_engineerid: hwid = 0x%x\n", engineerid);
	return engineerid;
}
__tagtable(ATAG_ENGINEERID, parse_tag_engineerid);

#define ATAG_COMPASS_TYPE 0x4d534D79
int compass_type;
int __init tag_compass_parsing(const struct tag *tags)
{
    compass_type = tags->u.revision.rev;

    printk(KERN_DEBUG "%s: Compass type = 0x%x\n", __func__,compass_type);

    return compass_type;
}
__tagtable(ATAG_COMPASS_TYPE, tag_compass_parsing);

#define ATAG_RFSKU 0x59504652
static struct tag_rfsku tag_rfsku_buf;
static struct tag_rfsku * rf_sku = &tag_rfsku_buf;
unsigned char * get_htc_smem(void)
{
	return (unsigned char *)rf_sku;
}
EXPORT_SYMBOL(get_htc_smem);

int __init parse_tag_rfsku(const struct tag *tags)
{
	int find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_RFSKU) {
			printk(KERN_DEBUG "find the RFSKU tag\n");
			find = 1;
			break;
		}
	}

	if (find) {
		memcpy(rf_sku,&(t->u.rfsku),sizeof(t->u.rfsku));
	}
	return 0;
}
__tagtable(ATAG_RFSKU, parse_tag_rfsku);
/*------------------------TAG End-------------------------------*/

int z4dtg_get_board_revision(void)
{
    unsigned int htc_skuid = 0, htc_pcbid = 0;

    /* Get HTC skuid & pcbid. */
    htc_skuid = htc_get_skuid();
    htc_pcbid = htc_get_pcbid();

    /* Check the board revision. */
    if (htc_skuid == 0) {
        return BOARD_EVM;       /* 0: EVM */
    }
    else {
        if ((htc_pcbid >= HTC_PCBID_EVT_MIN) && (htc_pcbid <= HTC_PCBID_EVT_MAX)) {
            /*
             * EVT/DVT build: htc_pcbid = 0(0x00) ~ 7(0x07)
             * htc_pcbid: XA=>0, XB=>1, XC=>2, ..., XH=>7,
             * return (htc_pcbid + 1): XA=>(0+1)=1, XB=>(1+1)=2, XC=>(2+1)=3, ..., XH=>(7+1)=8.
             */
            return (htc_pcbid + 1);
        }
        else if ((htc_pcbid >= HTC_PCBID_PVT_MIN) && (htc_pcbid <= HTC_PCBID_PVT_MAX)) {
            /*
             * PVT build: htc_pcbid = 128(0x80) ~ 135(0x87)
             * htc_pcbid: A=>128, B=>129, C=>130, ..., H=>135,
             * return (htc_pcbid): A=>128, B=>129, C=>130, ..., H=>135.
             */
            return htc_pcbid;
        }
        else {
            printk(KERN_ERR "%s(%d): Unknown board revision!! skuid=[0x%08X], pcbid=[0x%02X].\n", __func__, __LINE__, htc_skuid, htc_pcbid);
            return BOARD_UNKNOWN;   /* (-1): Unknown */
        }
    }
}

int z4td_get_board_revision(void)
{
    unsigned int htc_skuid = 0, htc_pcbid = 0;

    /* Get HTC skuid & pcbid. */
    htc_skuid = htc_get_skuid();
    htc_pcbid = htc_get_pcbid();

    /* Check the board revision. */
    if (htc_skuid == 0) {
        return BOARD_EVM;       /* 0: EVM */
    }
    else {
        if ((htc_pcbid >= HTC_PCBID_EVT_MIN) && (htc_pcbid <= HTC_PCBID_EVT_MAX)) {
            /*
             * EVT/DVT build: htc_pcbid = 0(0x00) ~ 7(0x07)
             * htc_pcbid: XA=>0, XB=>1, XC=>2, ..., XH=>7,
             * return (htc_pcbid + 1): XA=>(0+1)=1, XB=>(1+1)=2, XC=>(2+1)=3, ..., XH=>(7+1)=8.
             */
            return (htc_pcbid + 1);
        }
        else if ((htc_pcbid >= HTC_PCBID_PVT_MIN) && (htc_pcbid <= HTC_PCBID_PVT_MAX)) {
            /*
             * PVT build: htc_pcbid = 128(0x80) ~ 135(0x87)
             * htc_pcbid: A=>128, B=>129, C=>130, ..., H=>135,
             * return (htc_pcbid): A=>128, B=>129, C=>130, ..., H=>135.
             */
            return htc_pcbid;
        }
        else {
            printk(KERN_ERR "%s(%d): Unknown board revision!! skuid=[0x%08X], pcbid=[0x%02X].\n", __func__, __LINE__, htc_skuid, htc_pcbid);
            return BOARD_UNKNOWN;   /* (-1): Unknown */
        }
    }
}

int cp5dtu_get_board_revision(void)
{
    unsigned int htc_skuid = 0, htc_pcbid = 0;

    /* Get HTC skuid & pcbid. */
    htc_skuid = htc_get_skuid();
    htc_pcbid = htc_get_pcbid();

    /* Check the board revision. */
    if (htc_skuid == 0) {
        return BOARD_EVM;       /* 0: EVM */
    }
    else {
        if ((htc_pcbid >= HTC_PCBID_EVT_MIN) && (htc_pcbid <= HTC_PCBID_EVT_MAX)) {
            /*
             * EVT/DVT build: htc_pcbid = 0(0x00) ~ 7(0x07)
             * htc_pcbid: XA=>0, XB=>1, XC=>2, ..., XH=>7,
             * return (htc_pcbid + 1): XA=>(0+1)=1, XB=>(1+1)=2, XC=>(2+1)=3, ..., XH=>(7+1)=8.
             */
            return (htc_pcbid + 1);
        }
        else if ((htc_pcbid >= HTC_PCBID_PVT_MIN) && (htc_pcbid <= HTC_PCBID_PVT_MAX)) {
            /*
             * PVT build: htc_pcbid = 128(0x80) ~ 135(0x87)
             * htc_pcbid: A=>128, B=>129, C=>130, ..., H=>135,
             * return (htc_pcbid): A=>128, B=>129, C=>130, ..., H=>135.
             */
            return htc_pcbid;
        }
        else {
            printk(KERN_ERR "%s(%d): Unknown board revision!! skuid=[0x%08X], pcbid=[0x%02X].\n", __func__, __LINE__, htc_skuid, htc_pcbid);
            return BOARD_UNKNOWN;   /* (-1): Unknown */
        }
    }
}


int cp5dug_get_board_revision(void)
{
    unsigned int htc_skuid = 0, htc_pcbid = 0;

    /* Get HTC skuid & pcbid. */
    htc_skuid = htc_get_skuid();
    htc_pcbid = htc_get_pcbid();

    /* Check the board revision. */
    if (htc_skuid == 0) {
        return BOARD_EVM;       /* 0: EVM */
    }
    else {
        if ((htc_pcbid >= HTC_PCBID_EVT_MIN) && (htc_pcbid <= HTC_PCBID_EVT_MAX)) {
            /*
             * EVT/DVT build: htc_pcbid = 0(0x00) ~ 7(0x07)
             * htc_pcbid: XA=>0, XB=>1, XC=>2, ..., XH=>7,
             * return (htc_pcbid + 1): XA=>(0+1)=1, XB=>(1+1)=2, XC=>(2+1)=3, ..., XH=>(7+1)=8.
             */
            return (htc_pcbid + 1);
        }
        else if ((htc_pcbid >= HTC_PCBID_PVT_MIN) && (htc_pcbid <= HTC_PCBID_PVT_MAX)) {
            /*
             * PVT build: htc_pcbid = 128(0x80) ~ 135(0x87)
             * htc_pcbid: A=>128, B=>129, C=>130, ..., H=>135,
             * return (htc_pcbid): A=>128, B=>129, C=>130, ..., H=>135.
             */
            return htc_pcbid;
        }
        else {
            printk(KERN_ERR "%s(%d): Unknown board revision!! skuid=[0x%08X], pcbid=[0x%02X].\n", __func__, __LINE__, htc_skuid, htc_pcbid);
            return BOARD_UNKNOWN;   /* (-1): Unknown */
        }
    }
}

int cp5dwg_get_board_revision(void)
{
    unsigned int htc_skuid = 0, htc_pcbid = 0;

    /* Get HTC skuid & pcbid. */
    htc_skuid = htc_get_skuid();
    htc_pcbid = htc_get_pcbid();

    /* Check the board revision. */
    if (htc_skuid == 0) {
        return BOARD_EVM;       /* 0: EVM */
    }
    else {
        if ((htc_pcbid >= HTC_PCBID_EVT_MIN) && (htc_pcbid <= HTC_PCBID_EVT_MAX)) {
            /*
             * EVT/DVT build: htc_pcbid = 0(0x00) ~ 7(0x07)
             * htc_pcbid: XA=>0, XB=>1, XC=>2, ..., XH=>7,
             * return (htc_pcbid + 1): XA=>(0+1)=1, XB=>(1+1)=2, XC=>(2+1)=3, ..., XH=>(7+1)=8.
             */
            return (htc_pcbid + 1);
        }
        else if ((htc_pcbid >= HTC_PCBID_PVT_MIN) && (htc_pcbid <= HTC_PCBID_PVT_MAX)) {
            /*
             * PVT build: htc_pcbid = 128(0x80) ~ 135(0x87)
             * htc_pcbid: A=>128, B=>129, C=>130, ..., H=>135,
             * return (htc_pcbid): A=>128, B=>129, C=>130, ..., H=>135.
             */
            return htc_pcbid;
        }
        else {
            printk(KERN_ERR "%s(%d): Unknown board revision!! skuid=[0x%08X], pcbid=[0x%02X].\n", __func__, __LINE__, htc_skuid, htc_pcbid);
            return BOARD_UNKNOWN;   /* (-1): Unknown */
        }
    }
}

int cp5vedtu_get_board_revision(void)
{
    unsigned int htc_skuid = 0, htc_pcbid = 0;

    /* Get HTC skuid & pcbid. */
    htc_skuid = htc_get_skuid();
    htc_pcbid = htc_get_pcbid();

    /* Check the board revision. */
    if (htc_skuid == 0) {
        return BOARD_EVM;       /* 0: EVM */
    }
    else {
        if ((htc_pcbid >= HTC_PCBID_EVT_MIN) && (htc_pcbid <= HTC_PCBID_EVT_MAX)) {
            /*
             * EVT/DVT build: htc_pcbid = 0(0x00) ~ 7(0x07)
             * htc_pcbid: XA=>0, XB=>1, XC=>2, ..., XH=>7,
             * return (htc_pcbid + 1): XA=>(0+1)=1, XB=>(1+1)=2, XC=>(2+1)=3, ..., XH=>(7+1)=8.
             */
            return (htc_pcbid + 1);
        }
        else if ((htc_pcbid >= HTC_PCBID_PVT_MIN) && (htc_pcbid <= HTC_PCBID_PVT_MAX)) {
            /*
             * PVT build: htc_pcbid = 128(0x80) ~ 135(0x87)
             * htc_pcbid: A=>128, B=>129, C=>130, ..., H=>135,
             * return (htc_pcbid): A=>128, B=>129, C=>130, ..., H=>135.
             */
            return htc_pcbid;
        }
        else {
            printk(KERN_ERR "%s(%d): Unknown board revision!! skuid=[0x%08X], pcbid=[0x%02X].\n", __func__, __LINE__, htc_skuid, htc_pcbid);
            return BOARD_UNKNOWN;   /* (-1): Unknown */
        }
    }
}

int cp5vedwg_get_board_revision(void)
{
    unsigned int htc_skuid = 0, htc_pcbid = 0;

    /* Get HTC skuid & pcbid. */
    htc_skuid = htc_get_skuid();
    htc_pcbid = htc_get_pcbid();

    /* Check the board revision. */
    if (htc_skuid == 0) {
        return BOARD_EVM;       /* 0: EVM */
    }
    else {
        if ((htc_pcbid >= HTC_PCBID_EVT_MIN) && (htc_pcbid <= HTC_PCBID_EVT_MAX)) {
            /*
             * EVT/DVT build: htc_pcbid = 0(0x00) ~ 7(0x07)
             * htc_pcbid: XA=>0, XB=>1, XC=>2, ..., XH=>7,
             * return (htc_pcbid + 1): XA=>(0+1)=1, XB=>(1+1)=2, XC=>(2+1)=3, ..., XH=>(7+1)=8.
             */
            return (htc_pcbid + 1);
        }
        else if ((htc_pcbid >= HTC_PCBID_PVT_MIN) && (htc_pcbid <= HTC_PCBID_PVT_MAX)) {
            /*
             * PVT build: htc_pcbid = 128(0x80) ~ 135(0x87)
             * htc_pcbid: A=>128, B=>129, C=>130, ..., H=>135,
             * return (htc_pcbid): A=>128, B=>129, C=>130, ..., H=>135.
             */
            return htc_pcbid;
        }
        else {
            printk(KERN_ERR "%s(%d): Unknown board revision!! skuid=[0x%08X], pcbid=[0x%02X].\n", __func__, __LINE__, htc_skuid, htc_pcbid);
            return BOARD_UNKNOWN;   /* (-1): Unknown */
        }
    }
}


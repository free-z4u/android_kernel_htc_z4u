/*
 * =====================================================================================
 *
 *       Filename:  sprd_asc.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/13/2013 04:56:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  chuck_huang 
 *        Company:  hTc
 *
 * =====================================================================================
 */

#ifndef _SPRD_ASC_H
#define _SPRD_ASC_H

#include <linux/dcache.h>

#define SPRD_ASC_DRV_VER "SPRD_ASC_DRV_VER_1.2"

#define MAX_SEG		10

struct cp_segments {
	char* name;
	umode_t mode;
	uint32_t base;	/* segment addr */
	uint32_t maxsz;	/* segment size */
};

struct sprd_asc_platform_data {
	char *devname;	/*  modem name */
	uint32_t base;	/* CP base addr */
	uint32_t maxsz;	/* CP max size */

	int (*start)(void *arg);
	int (*stop)(void *arg);
	int (*simswitch)(int arg);
	unsigned int (*get_cp_stat)(unsigned int arg);

	int (*reset)(void);
	int (*restart)(void);
	char *ntf_irqname;
	int ntf_irq;
	uint32_t (*ntfirq_status)(void);
	void (*ntfirq_clear)(void);
	void (*ntfirq_trigger)(void);

	char *wtd_irqname;
	int wtd_irq;
	uint32_t (*wtdirq_status)(void);
	void (*wtdirq_clear)(void);
	void (*wtdirq_trigger)(void);

	char *sim_irqname;
	int status_gpio;

	int ap_ctrl_rf;
	int ap_ctrl_ant;
	int sim_switch_enable;
	/*
	 * 1: actually modem
	 * 0: smem device, used to debug shrm
	 */
	int devtype;

	uint32_t segnr;
	struct cp_segments segs[];
};

/*  cp to ap message definition */
enum {
	MSG_ASSERT = 1,
	MSG_READY,
	MSG_RESERVE,
	MSG_GPIO_UP,
	MSG_GPIO_DOWN,
	MSG_NR,
};

struct sprd_asc_proc_entry {
	char *name;
	struct proc_dir_entry *entry;
	void *host;
};

struct sprd_asc_proc_fs {
	struct proc_dir_entry	*procdir;
	struct sprd_asc_proc_entry element[MAX_SEG];
};

struct sprd_asc_host {
	struct kobject		*sprd_asc_kobj;

	unsigned int		pdev_id;

	void __iomem 		*vbase;

	/* when modem boot ready */
	unsigned int		atc_ready;

	unsigned int		power_stat;

	/* work queue for each modem event */
	char			wq_name[16];
	struct workqueue_struct	*event_wq;
	struct work_struct	assert_work;

	struct delayed_work	simstat_work;
	unsigned int		oldstat;

	struct work_struct	simswitch_work;
	unsigned int		sim_switchstat;

	struct sprd_asc_proc_fs procfs;

	struct sprd_asc_platform_data *plat;
};

#define GPIO27_GPIO	27
#define GPIO29_GPIO	29
#define GPIO152_GPIO	152

#endif

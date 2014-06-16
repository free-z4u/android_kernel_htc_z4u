/*
 * Definitions for rt5501 speaker amp chip.
 */
#ifndef RT5501_H
#define RT5501_H

#include <linux/ioctl.h>
#include <linux/wakelock.h>

#define RT5501_I2C_NAME "rt5501"
#define SPKR_OUTPUT 0
#define HEADSET_OUTPUT 1
#define DUAL_OUTPUT 2
#define HANDSET_OUTPUT 3
#define LINEOUT_OUTPUT 4
#define NO_OUTPUT 5
#define MODE_CMD_LEM 9
#define MAX_REG_DATA 15

// HTC_AUD_START    Yangyi
// AMP REGS
#define AMP_EN			0x00	// AMP SW ON/OFF
#define AMP_VOL			0x01	// Headphone volume
/*
#define AMP_NG			0x02	// Noise gate enable
#define IMP_OM_VAL		0x04	// Resulting impedance
#define NG_TRIG_TIME		0x07	// Impedance sensing enable
#define NG_ATCK_TIME		0x08	// Noise gate attack time
#define NG_HOLD_TIME		0x09	// Noise gate hold time
#define NG_RELS_TIME		0x0a	// Noise gate release time
#define NG_AMP_ON		0x0b	// Noise gate enbale while amp off
#define EXT_NG_BW		0x81	// Extend noise gate bandwidth
#define AMP_BIAS_CUR		0x87	// HP AMP bias current, trade-off between THD and Iq
#define CP_MODE_SET		0x90	// CP auto/fixed mode setting
#define CP_FREQ_SET		0x93	// CP operating frequency setting
#define DET_HOLD_TIME		0x95	// Amplitude detection holdtime setting (high to low)
#define THRE_FINE_TUNE		0x96	// CP threshold fine tune (85% ~ 130%)
#define THRE_16OM_33_50		0x97	// CP change mode threshold setting for 16ohm, 0.33X to 0.5X
#define THRE_500OM_33_50	0x98	// CP change mode threshold setting for 500ohm, 0.33X to 0.5X
#define THRE_16OM_50_100	0x99	// CP change mode threshold setting for 16ohm, 0.5X to 1X
#define THRE_500OM_50_100	0x9a	// CP change mode threshold setting for 500ohm, 0.5X to 1X
#define THRE_16OM_100_150	0x9b	// CP change mode threshold setting for 16ohm, 1X to 1.5X 
#define THRE_500OM_100_150	0x9c	// CP change mode threshold setting for 500ohm, 1X to 1.5X 
#define THRE_16OM_150_200	0x9d	// CP change mode threshold setting for 16ohm, 1.5X to 2X
#define THRE_500OM_150_200	0x9e	// CP change mode threshold setting for 500ohm, 1.5X to 2X
#define IMP_REL_SET		0xa4	// Impedance sensing related setting
*/
 
// REG VALUE
#define VOL_0_DB		0x1c	// Headphone 0dB
#define VOL_MUTE		0xc7	// Headphone Mute
#define AMP_OFF			0x01	// Amplifier OFF
#define AMP_ON			0xc0	// Amplifier ON
// HTC_AUD_END    Yangyi

struct rt5501_platform_data {
	uint32_t gpio_rt5501_hp_en;
	unsigned char spkr_cmd[7];
	unsigned char hsed_cmd[7];
	unsigned char rece_cmd[7];
	/* for spk enable gpio on cpu */
	uint32_t gpio_rt5501_spk_en_cpu;
};

// HTC_AUD_START    Yangyi
#ifdef CONFIG_DEBUG_FS
struct rt5501_reg_addr{
    unsigned char addr;
};
#endif
// HTC_AUD_END    Yangyi

struct rt5501_reg_data {
	unsigned char addr;
	unsigned char val;
};

struct rt5501_config {
	unsigned int reg_len;
        struct rt5501_reg_data reg[MAX_REG_DATA];
};

struct rt5501_comm_data {
	unsigned int out_mode;
        struct rt5501_config config;
};

struct rt5501_config_data {
	unsigned int mode_num;
	struct rt5501_comm_data *cmd_data;  /* [mode][mode_kind][reserve][cmds..] */
};

enum {
        RT5501_INIT = 0,
        RT5501_MUTE,
        RT5501_MAX_FUNC
};

enum RT5501_Mode {
	RT5501_MODE_OFF = RT5501_MAX_FUNC,
	RT5501_MODE_PLAYBACK,
	RT5501_MODE_PLAYBACK8OH,
	RT5501_MODE_PLAYBACK16OH,
	RT5501_MODE_PLAYBACK32OH,
	RT5501_MODE_PLAYBACK64OH,
	RT5501_MODE_PLAYBACK128OH,
	RT5501_MODE_PLAYBACK256OH,
	RT5501_MODE_PLAYBACK500OH,
	RT5501_MODE_PLAYBACK1KOH,
	RT5501_MODE_VOICE,
	RT5501_MODE_TTY,
	RT5501_MODE_FM,
	RT5501_MODE_RING,
	RT5501_MODE_MFG,
	RT5501_MODE_BEATS_8_64,
	RT5501_MODE_BEATS_128_500,
	RT5501_MODE_MONO,
	RT5501_MODE_MONO_BEATS,
	RT5501_MAX_MODE
};

enum HEADSET_QUERY_STATUS{
    RT5501_QUERY_OFF = 0,
    RT5501_QUERY_HEADSET,
    RT5501_QUERY_FINISH,
};


enum RT5501_STATUS{
    RT5501_OFF = 0,
    RT5501_PLAYBACK,
    RT5501_SUSPEND,

};

enum HEADSET_OM {
    HEADSET_8OM = 0,
    HEADSET_16OM,
    HEADSET_32OM,
    HEADSET_64OM,
    HEADSET_128OM,
    HEADSET_256OM,
    HEADSET_500OM,
    HEADSET_1KOM,
    HEADSET_MONO,
    HEADSET_OM_UNDER_DETECT,
};

enum AMP_GPIO_STATUS {
     AMP_GPIO_OFF = 0,
     AMP_GPIO_ON,
     AMP_GPIO_QUERRTY_ON,
};

enum AMP_S4_STATUS {
     AMP_S4_AUTO = 0,
     AMP_S4_PWM,
};

#define QUERY_IMMED           msecs_to_jiffies(0)
#define QUERY_LATTER          msecs_to_jiffies(200)
#define RT5501_SENSE_READY    0x80

#define RT5501_IOCTL_MAGIC 'g'
#define RT5501_SET_CONFIG	_IOW(RT5501_IOCTL_MAGIC, 0x01,	unsigned)
#define RT5501_READ_CONFIG	_IOW(RT5501_IOCTL_MAGIC, 0x02, unsigned)
#define RT5501_SET_MODE        _IOW(RT5501_IOCTL_MAGIC, 0x03, unsigned)
#define RT5501_SET_PARAM       _IOW(RT5501_IOCTL_MAGIC, 0x04,  unsigned)
#define RT5501_WRITE_REG       _IOW(RT5501_IOCTL_MAGIC, 0x07,  unsigned)
#define RT5501_QUERY_OM       _IOW(RT5501_IOCTL_MAGIC, 0x08,  unsigned)

int query_rt5501(void);
void set_rt5501_amp(int on);
#endif


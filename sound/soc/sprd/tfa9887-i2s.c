/*
 * sound/soc/sprd/bt-i2s.c
 *
 * Copyright (C) 2013 SpreadTrum Ltd.
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
#define pr_fmt(fmt) "[audio:tfa9887i2s] " fmt

#include <linux/module.h>
#include <sound/soc.h>
#include "dai/i2s/i2s.h"

#include <mach/pinmap.h>

#include <linux/gpio.h>
#include <asm/io.h>
#include <linux/i2c/tfa9887.h>

#ifdef CONFIG_SPRD_AUDIO_DEBUG
#define tfa9887_i2s_dbg pr_debug
#else
#define tfa9887_i2s_dbg(...)
#endif

#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
#define AUD_RECEIVER_SEL	(133)
#endif

//0: pcm card closed; 1: pcm card opened
static unsigned short pcm_card_state = 0;

/* HTC_AUD_ADD */
#define SNDRV_CTL_IOCTL_TFA9887_I2S1_SWITCH		_IOWR('U', 0xfe, int)
static int tfa9887_i2s1_switch_ioctl(struct snd_card *card,struct snd_ctl_file *control,unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long reg;
	int  val;

	switch (cmd) {
	case SNDRV_CTL_IOCTL_TFA9887_I2S1_SWITCH:
		if (get_user(val, (int __user *)arg))
			return -EFAULT;

		printk("[AUD-eleven]: do i2s1 select: %d\n", val);

		reg = __raw_readl(CTL_PIN_BASE+REG_PIN_CTRL3);

		reg = (reg & (~(0x07<<9))) | (val << 9);
		__raw_writel(reg, CTL_PIN_BASE+REG_PIN_CTRL3);

// add for config tfa9887 power up / down
		ret = gpio_request(AUD_RECEIVER_SEL, "AUD_RECEIVER_SEL");

		if (ret < 0)
			pr_err("Request GPIO AUD_RECEIVER_SEL 133 Failed !");

		if(val == 0 && pcm_card_state == 0) {
			tfa9887_i2s_dbg("Exit Speaker phone call, so do switch speaker to Earpiece from TFA9887L !\n");
			gpio_direction_output(AUD_RECEIVER_SEL, 1);
			gpio_free(AUD_RECEIVER_SEL);

			set_tfa9887_spkamp(0, 0);
#ifdef CONFIG_SND_AMP_TFA9887L
			set_tfa9887l_spkamp(0,0);
#endif
		} else {
			tfa9887_i2s_dbg("Enter Speaker phone call, so do switch speaker to TFA9887L from Earpiece !\n");
			gpio_direction_output(AUD_RECEIVER_SEL, 0);
			gpio_free(AUD_RECEIVER_SEL);

			set_tfa9887_spkamp(1, 0);
#ifdef CONFIG_SND_AMP_TFA9887L
			set_tfa9887l_spkamp(1,0);
#endif
		}
// end
		break;
	default:
		ret = -ENOIOCTLCMD;
	}

	return ret;

}
/* HTC_AUD_END */

/* Jeffrey debug add tfa9887 i2s config */
static struct i2s_config tfa9887_i2s_config = {
	.fs = 48000,
	.slave_timeout = 0xF11,
	.bus_type = I2S_BUS,
	.byte_per_chan = I2S_BPCH_16,
	.mode = I2S_MASTER,
	.lsb = I2S_MSB,
	.rtx_mode = I2S_RTX_MODE,
	.sync_mode = I2S_LRCK,
	.lrck_inv = I2S_L_RIGTH,
	.clk_inv = I2S_CLK_R,
	.i2s_bus_mode = I2S_COMPATIBLE,
	.pcm_slot = 0x1,
	.pcm_cycle = 1,
	.tx_watermark = 12,
	.rx_watermark = 20,
};


/* HTC_AUD_ADD */
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
static int tfa9887_codec_prepare(struct snd_pcm_substream *stream)
{
	int ret = gpio_request(AUD_RECEIVER_SEL, "AUD_RECEIVER_SEL");

	if (ret < 0)
		return ret;
//HTC_AUD_START ZM add to enable codec digital clock for i2s using
//	arch_audio_codec_digital_enable();
//HTC_AUD_END
	tfa9887_i2s_dbg("Finishing %s\n", __func__);
	gpio_direction_output(AUD_RECEIVER_SEL, 0);
	gpio_free(AUD_RECEIVER_SEL);

	return 0;
}

static int tfa9887_codec_startup(struct snd_pcm_substream *substream)
{
       pcm_card_state = 1;
       if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		set_tfa9887_spkamp(1, 0);
#ifdef CONFIG_SND_AMP_TFA9887L
		set_tfa9887l_spkamp(1,0);
#endif
       }
	return 0;
}

static int tfa9887_codec_shutdown(struct snd_pcm_substream *substream)
{
        pcm_card_state = 0;
	int ret = gpio_request(AUD_RECEIVER_SEL, "AUD_RECEIVER_SEL");

	if (ret < 0)
		return ret;

	tfa9887_i2s_dbg("Finishing %s\n", __func__);
	gpio_direction_output(AUD_RECEIVER_SEL, 1);
	gpio_free(AUD_RECEIVER_SEL);

       if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		set_tfa9887_spkamp(0, 0);
#ifdef CONFIG_SND_AMP_TFA9887L
		set_tfa9887l_spkamp(0,0);
#endif
       }
	return 0;
}
#else
#define tfa9887_codec_startup NULL
#define tfa9887_codec_prepare NULL
#define tfa9887_codec_shutdown NULL
#endif
/* HTC_AUD_END */

static struct snd_soc_ops tfa9887_codec_ops = {
      .startup = tfa9887_codec_startup,
	.prepare = tfa9887_codec_prepare,
	.shutdown = tfa9887_codec_shutdown,
};

static struct snd_soc_dai_link tfa9887_i2s_dai[] = {
	{
	 .name = "tfa9887-i2s",
	 .stream_name = "tfa",

	 .codec_name = "tfa9887-codec",
	 .platform_name = "sprd-pcm-audio",
	 .cpu_dai_name = "i2s.1",
	 .codec_dai_name = "tfa9887-codec-dai",
	 .ops = &tfa9887_codec_ops,
	 },
};


struct i2s_private tfa9887_i2s_priv = { 0 };

static int tfa9887_i2s_late_probe(struct snd_soc_card *card)
{
	int i;
	tfa9887_i2s_dbg("Entering %s\n", __func__);
	tfa9887_i2s_priv.config = &tfa9887_i2s_config;
	for (i = 0; i < card->num_links; i++) {
		struct snd_soc_dai *cpu_dai = card->rtd[i].cpu_dai;
		cpu_dai->ac97_pdata = &tfa9887_i2s_priv;
	}

	
	/* HTC_AUD_ADD */
#if defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
#define AUD_DCDC       (230)
		   int ret = gpio_request(AUD_DCDC, "AUD_DCDC");
		   if(ret < 0)
				   pr_err("Request GPIO 230 Failed !");
	
		 tfa9887_i2s_dbg("Finishing AUD_DCDC power up !");
		   gpio_direction_output(AUD_DCDC, 1);
		   gpio_free(AUD_DCDC);
#endif
	/* HTC_AUD_END */

	snd_ctl_register_ioctl(tfa9887_i2s1_switch_ioctl); // add for i2s select: ap/cp0/cp1

	return 0;
}

static struct snd_soc_card tfa9887_i2s_card = {
	.name = "tfa9887",
	.dai_link = tfa9887_i2s_dai,
	.num_links = ARRAY_SIZE(tfa9887_i2s_dai),
	.owner = THIS_MODULE,
	.late_probe = tfa9887_i2s_late_probe,
};

static struct platform_device *tfa9887_i2s_snd_device;

static int __init tfa9887_i2s_modinit(void)
{
	int ret;

	tfa9887_i2s_dbg("Entering %s\n", __func__);

	tfa9887_i2s_snd_device = platform_device_alloc("soc-audio", 0);
	if (!tfa9887_i2s_snd_device)
		return -ENOMEM;

	platform_set_drvdata(tfa9887_i2s_snd_device, &tfa9887_i2s_card);
	ret = platform_device_add(tfa9887_i2s_snd_device);

	if (ret)
		platform_device_put(tfa9887_i2s_snd_device);

	return ret;
}

static void __exit tfa9887_i2s_modexit(void)
{
	platform_device_unregister(tfa9887_i2s_snd_device);
}

module_init(tfa9887_i2s_modinit);
module_exit(tfa9887_i2s_modexit);

MODULE_DESCRIPTION("ALSA SoC SpreadTrum IIS+BT");
MODULE_AUTHOR("Ken Kuang <ken.kuang@spreadtrum.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("machine:i2s+tfa");

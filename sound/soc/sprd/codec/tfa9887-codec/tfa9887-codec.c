/*
 * sound/soc/sprd/codec/tfa9887-codec/tfa9887-codec.c
 *
 * NULL-CODEC -- SpreadTrum just for codec code.
 *
 * Copyright (C) 2012 SpreadTrum Ltd.
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
#define pr_fmt(fmt) "[audio:tfa9887-codec] " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/stat.h>

#include <sound/core.h>
#include <sound/soc.h>

#include "tfa9887-codec.h"

#ifdef CONFIG_SPRD_AUDIO_DEBUG
#define tfa9887_codec_dbg pr_debug
#else
#define tfa9887_codec_dbg(...)
#endif

struct tfa9887_codec_data {
	struct snd_soc_codec codec;
};

/* PCM Playing and Recording default in full duplex mode */
struct snd_soc_dai_driver tfa9887_codec_dai[] = {
	{
	 .name = "tfa9887-codec-dai",
	 .playback = {
		      .stream_name = "Playback",
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SNDRV_PCM_FMTBIT_S16_LE,
		      },
	 .capture = {
		     .stream_name = "Capture",
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SNDRV_PCM_FMTBIT_S16_LE,
		     },
	 },
};

static int tfa9887_codec_soc_probe(struct snd_soc_codec *codec)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);

	return 0;
}

static int tfa9887_codec_soc_remove(struct snd_soc_codec *codec)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);
	return 0;
}

static int tfa9887_codec_soc_suspend(struct snd_soc_codec *codec)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);
	return 0;
}

static int tfa9887_codec_soc_resume(struct snd_soc_codec *codec)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);
	return 0;
}

static int tfa9887_codec_read(struct snd_soc_codec *codec)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);
	return 0;
}

static int tfa9887_codec_write(struct snd_soc_codec *codec)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_tfa9887_codec = {
	.probe = tfa9887_codec_soc_probe,
	.remove = tfa9887_codec_soc_remove,
	.suspend = tfa9887_codec_soc_suspend,
	.resume = tfa9887_codec_soc_resume,
	.read = tfa9887_codec_read,
	.write = tfa9887_codec_write,
};

static __devinit int tfa9887_codec_codec_probe(struct platform_device *pdev)
{
	int ret;

	tfa9887_codec_dbg("Entering %s\n", __func__);

	ret = snd_soc_register_codec(&pdev->dev,
				     &soc_codec_dev_tfa9887_codec, tfa9887_codec_dai,
				     ARRAY_SIZE(tfa9887_codec_dai));
	if (ret != 0) {
		pr_err("Failed to register CODEC: %d\n", ret);
		return ret;
	}

	return 0;

}

static int __devexit tfa9887_codec_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver tfa9887_codec_codec_driver = {
	.driver = {
		   .name = "tfa9887-codec",
		   .owner = THIS_MODULE,
		   },
	.probe = tfa9887_codec_codec_probe,
	.remove = __devexit_p(tfa9887_codec_codec_remove),
};

static __init int tfa9887_codec_init(void)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);
	return platform_driver_register(&tfa9887_codec_codec_driver);
}

module_init(tfa9887_codec_init);

static __exit void tfa9887_codec_exit(void)
{
	tfa9887_codec_dbg("Entering %s\n", __func__);
	platform_driver_unregister(&tfa9887_codec_codec_driver);
}

module_exit(tfa9887_codec_exit);

MODULE_DESCRIPTION("NULL-CODEC ALSA SoC codec driver");
MODULE_AUTHOR("Ken Kuang <ken.kuang@spreadtrum.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("codec:tfa9887-codec");

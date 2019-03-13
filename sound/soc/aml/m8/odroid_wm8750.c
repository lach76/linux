/*
    odroid - wm8750
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/jack.h>
#include <linux/switch.h>
#include <linux/amlogic/iomap.h>

#include "aml_i2s.h"
#include "odroid_wm8750.h"
#include "aml_audio_hw.h"
#include <linux/amlogic/sound/audin_regs.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/of_gpio.h>
#include <linux/io.h>

#define DRV_NAME "odroid_wm8750"

static int odroid_wm8750_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_codec *codec = rtd->codec;
    struct snd_soc_dapm_context *dapm = &codec->dapm;

    /* These endpoints are not being used. */
    snd_soc_dapm_nc_pin(dapm, "LINPUT2");
    snd_soc_dapm_nc_pin(dapm, "RINPUT2");
    snd_soc_dapm_nc_pin(dapm, "LINPUT3");
    snd_soc_dapm_nc_pin(dapm, "RINPUT3");
    snd_soc_dapm_nc_pin(dapm, "OUT3");
    snd_soc_dapm_nc_pin(dapm, "MONO1");

    return 0;
}

static int wm8750_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret;

    printk("set wm8750_hw_params\n");
    /* set codec DAI configuration */
    ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0)
        return ret;

    /* set cpu DAI configuration */
    ret = snd_soc_dai_set_fmt( cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
    if (ret < 0)
        return ret;

    /* set cpu DAI clock */
    ret = snd_soc_dai_set_sysclk( cpu_dai, 0, params_rate(params)*256, SND_SOC_CLOCK_OUT);
    if (ret < 0)
        return ret;

    ret = snd_soc_dai_set_sysclk(codec_dai, 0, params_rate(params)*256, SND_SOC_CLOCK_IN);
    if (ret < 0)
        return ret;
    return 0;
}

static struct snd_soc_ops odroid_ops = {
    .hw_params = wm8750_hw_params,
};

static struct snd_soc_dai_link wm8750_dai_link[] = {
    {
        .name = "SND_WM8750",
        .stream_name = "WM8750",
        .cpu_dai_name = "I2S",
        .platform_name = "i2s_platform",
        .codec_dai_name = "wm8750-hifi",
        .codec_name = "wm8750.1-001a",
        .ops = &odroid_ops,
        .init = odroid_wm8750_init,
    },
};

static struct snd_soc_card aml_snd_soc_card = {
    //.driver_name = "SOC-Audio",
    .name = "ODROID_WM8750",
    .owner = THIS_MODULE,
    .dai_link = wm8750_dai_link,
    .num_links = ARRAY_SIZE(wm8750_dai_link),
    //.set_bias_level = wm8750_set_bias_level,
};

static int wm8750_probe(struct platform_device *pdev)
{
    struct snd_soc_card *card = &aml_snd_soc_card;
    struct wm8750_private_data *p_dac2;
    int ret = 0;

#ifdef CONFIG_OF
    p_dac2 = devm_kzalloc(&pdev->dev,
    sizeof(struct wm8750_private_data), GFP_KERNEL);
    if (!p_dac2) {
        dev_err(&pdev->dev, "Can't allocate wm8750_private_data\n");
        ret = -ENOMEM;
        goto err;
    }

    card->dev = &pdev->dev;
    platform_set_drvdata(pdev, card);
    snd_soc_card_set_drvdata(card, p_dac2);
    if (!(pdev->dev.of_node)) {
        dev_err(&pdev->dev, "Must be instantiated using device tree\n");
        ret = -EINVAL;
        goto err;
    }
    ret = of_property_read_string(pdev->dev.of_node,
                    "pinctrl-names",
                    &p_dac2->pinctrl_name);
    p_dac2->pin_ctl =
        devm_pinctrl_get_select(&pdev->dev, p_dac2->pinctrl_name);

    ret = snd_soc_of_parse_card_name(card, "aml,sound_card");
    if (ret)
        goto err;

    ret = snd_soc_register_card(card);
    if (ret) {
        dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
        ret);
        goto err;
    }
    return 0;
#endif

err:
    return ret;
}

static int wm8750_remove(struct platform_device *pdev)
{
    struct snd_soc_card *card = platform_get_drvdata(pdev);
    struct wm8750_private_data *p_dac2;

    p_dac2 = snd_soc_card_get_drvdata(card);
    if (p_dac2->pin_ctl)
        devm_pinctrl_put(p_dac2->pin_ctl);

    snd_soc_unregister_card(card);
    return 0;
}

#ifdef CONFIG_USE_OF
static const struct of_device_id wm8750_dt_match[] = {
    { .compatible = "sound_card, odroid_wm8750", },
    {},
};
#else
#define wm8750_dt_match NULL
#endif

static struct platform_driver wm8750_driver = {
    .probe = wm8750_probe,
    .remove = wm8750_remove,
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
        .pm = &snd_soc_pm_ops,
        .of_match_table = wm8750_dt_match,
    },
};

static int __init wm8750_init(void)
{
    return platform_driver_register(&wm8750_driver);
}

static void __exit wm8750_exit(void)
{
    platform_driver_unregister(&wm8750_driver);
}

#ifdef CONFIG_DEFERRED_MODULE_INIT
deferred_module_init(wm8750_init);
#else
module_init(wm8750_init);
#endif
module_exit(wm8750_exit);

/* Module information */
MODULE_AUTHOR("Anonymous");
MODULE_DESCRIPTION("ODROID WM-9750 Custom Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);

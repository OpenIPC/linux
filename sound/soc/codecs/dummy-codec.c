// ### SIPEED EDIT ###
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>

struct snd_soc_dai_driver dummy_dai = {
	.name = "dummy_codec",
	.playback = {
		.stream_name = "Dummy Playback",
		.channels_min = 1,
		.channels_max = 384,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.formats = (SNDRV_PCM_FMTBIT_S8 |
			    SNDRV_PCM_FMTBIT_S16_LE |
			    SNDRV_PCM_FMTBIT_S20_3LE |
			    SNDRV_PCM_FMTBIT_S24_LE |
			    SNDRV_PCM_FMTBIT_S32_LE),
	},
	.capture = {
		.stream_name = "Dummy Capture",
		.channels_min = 1,
		.channels_max = 384,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.formats = (SNDRV_PCM_FMTBIT_S8 |
			    SNDRV_PCM_FMTBIT_S16_LE |
			    SNDRV_PCM_FMTBIT_S20_3LE |
			    SNDRV_PCM_FMTBIT_S24_LE |
			    SNDRV_PCM_FMTBIT_S32_LE),
	},
};

static const struct snd_soc_component_driver soc_dummy_codec;

static int dummy_codec_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev, &soc_dummy_codec,
					       &dummy_dai, 1);
}

static const struct of_device_id dummy_codec_of_match[] = {
	{ .compatible = "dummy-codec", },
	{},
};
MODULE_DEVICE_TABLE(of, dummy_codec_of_match);

static struct platform_driver dummy_codec_driver = {
	.driver = {
		.name = "dummy_codec",
		.of_match_table = of_match_ptr(dummy_codec_of_match),
	},
	.probe = dummy_codec_probe,
};

module_platform_driver(dummy_codec_driver);

MODULE_DESCRIPTION("Dummy Codec Driver");
MODULE_LICENSE("GPL v2");
// ### SIPEED EDIT END ###
/*!
********************************************************************************
*  Copyright (c) Novatek Microelectronics Corp., Ltd. All Rights Reserved.
*  \file    nvt-machine.c
*  \brief   For add dummy audio soc card
*  \project kernel mode driver
********************************************************************************
*/
/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
//! Standard
#include <linux/init.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#define NVT_I2S_TEST 0

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define NVT_CARD_MAXNUM       3
#define NVT_PLAT_DEV_NUM      3


#define NVT_MACHINE_NAME_R     "nvt,alsa_mach"
#define NVT_PLATFORM_NAME_R    "nvt,alsa_plat_r"
#define NVT_CODEC_NAME_R       "nvt,embd_codec_r"
#define NVT_CODECDAI_NAME_R    "nvt,codec_dai_r"
#define NVT_CPUDAI_NAME0_R     "nvt,cpu_dai_r"


#define NVT_MACHINE_NAME_P     "nvt,alsa_mach_p"
#define NVT_PLATFORM_NAME_P    "nvt,alsa_plat_p"
#define NVT_CODEC_NAME_P       "nvt,embd_codec_p"
#define NVT_CODECDAI_NAME_P    "nvt,codec_dai_p"
#define NVT_CPUDAI_NAME0_P     "nvt,cpu_dai_p"


#define NVT_MACHINE_NAME_L     "nvt,alsa_mach_l"
#define NVT_PLATFORM_NAME_L    "nvt,alsa_plat_l"
#define NVT_CODEC_NAME_L       "nvt,embd_codec_l"
#define NVT_CODECDAI_NAME_L    "nvt,codec_dai_l"
#define NVT_CPUDAI_NAME0_L     "nvt,cpu_dai_l"


/*-----------------------------------------------------------------------------*/
/* ALSA Debug                                                                  */
/*-----------------------------------------------------------------------------*/
#define NVTALSA_PREFIX "[alsa]"

#define NVTALSA_TRACE( fmt,...)   //printk( NVTALSA_PREFIX "[%s]" fmt "\n",__FUNCTION__, ##__VA_ARGS__);
#define NVTALSA_ERR( fmt,...)     printk( NVTALSA_PREFIX "[%s:%d]" fmt , __FUNCTION__, __LINE__, ##__VA_ARGS__);


/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

struct nvtalsa_platform_device
{
	const char		       *name;
	struct platform_device *pdev;
};

typedef struct _ST_FAKE_CARD
{
	struct snd_soc_card *soc_card;

} ST_FAKE_CARD;

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/
static int nvt_machine_probe(struct platform_device *pdev);
static int nvt_machine_remove(struct platform_device *pdev);

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static struct platform_driver nvt_machine_driver = {
    .driver = {
		.name	= NVT_MACHINE_NAME_R,
	    .owner  = THIS_MODULE,
    },

	.probe		= nvt_machine_probe,
	.remove		= nvt_machine_remove,

};

static struct snd_soc_dai_link fakecard_dailink[] = {
	{
		.name 			= "NT520EMBD_R",
		.stream_name    = "EMBD_R",

		.platform_name  = NVT_PLATFORM_NAME_R,
		.cpu_dai_name   = NVT_CPUDAI_NAME0_R,

		.codec_name     = NVT_CODEC_NAME_R,
		.codec_dai_name = NVT_CODECDAI_NAME_R,

		#if NVT_I2S_TEST
		.dai_fmt		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
						| SND_SOC_DAIFMT_CBS_CFS,
		#endif
	},

	{
		.name 			= "NT520EMBD_P",
		.stream_name    = "EMBD_P",

		.platform_name  = NVT_PLATFORM_NAME_P,
		.cpu_dai_name   = NVT_CPUDAI_NAME0_P,

		.codec_name     = NVT_CODEC_NAME_P,
		.codec_dai_name = NVT_CODECDAI_NAME_P,

		#if NVT_I2S_TEST
		.dai_fmt		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
						| SND_SOC_DAIFMT_CBS_CFS,
		#endif
	},

	{
		.name 			= "NT520EMBD_L",
		.stream_name    = "EMBD_L",

		.platform_name  = NVT_PLATFORM_NAME_L,
		.cpu_dai_name   = NVT_CPUDAI_NAME0_L,

		.codec_name     = NVT_CODEC_NAME_L,
		.codec_dai_name = NVT_CODECDAI_NAME_L,

		#if NVT_I2S_TEST
		.dai_fmt		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
						| SND_SOC_DAIFMT_CBS_CFS,
		#endif
	},

};

/* SoC card */
static struct snd_soc_card snd_soc_fakecard[NVT_CARD_MAXNUM] = {
	{
		.name 		= "nvt_card_rx",
		.owner 		= THIS_MODULE,
		.dai_link 	= &fakecard_dailink[0],
		.num_links 	= 1,
	},

	{
		.name 		= "nvt_card_tx1",
		.owner 		= THIS_MODULE,
		.dai_link 	= &fakecard_dailink[1],
		.num_links 	= 1,
	},

	{
		.name 		= "nvt_card_lb",
		.owner 		= THIS_MODULE,
		.dai_link 	= &fakecard_dailink[2],
		.num_links 	= 1,
	},

};

static struct nvtalsa_platform_device g_stNvtAudPlatformDevice[] =
{
	{.name = NVT_MACHINE_NAME_R,   .pdev = NULL,},
	{.name = NVT_MACHINE_NAME_P,   .pdev = NULL,},
	{.name = NVT_MACHINE_NAME_L,   .pdev = NULL,},
};


ST_FAKE_CARD g_stFakeCards[NVT_CARD_MAXNUM] =
{
	{.soc_card= NULL},
	{.soc_card= NULL},
	{.soc_card= NULL},
};

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/

//! machine
static int nvt_machine_probe(struct platform_device *pdev)
{
	NVTALSA_TRACE( "Called");
	return 0;
}

static int nvt_machine_remove(struct platform_device *pdev)
{
	NVTALSA_TRACE( "Called");
	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#define __INTERFACE_FUNCTION__

int nvt_nvtalsa_card_init(int iCardIdx)
{
	int iRet = 0, iIdx = 0, iTargetCard = 0;

	NVTALSA_TRACE( "Called [Card%d]",iCardIdx);

	for (iTargetCard = 0; iTargetCard < NVT_CARD_MAXNUM; iTargetCard++)
	{
		if((g_stFakeCards[iTargetCard].soc_card != NULL))
		{
			iTargetCard = NVT_CARD_MAXNUM;
			break;
		}
		else if(g_stFakeCards[iTargetCard].soc_card == NULL)
		{
			g_stFakeCards[iTargetCard].soc_card = &snd_soc_fakecard[iTargetCard];
			//break;
		}

	}


	{
		iRet = platform_driver_register(&nvt_machine_driver);
		if (iRet)
		{
			NVTALSA_ERR("platform_driver_register failed(%d)!\n", iRet);
		}


		//! Allocate device
		for (iIdx = 0; iIdx < NVT_PLAT_DEV_NUM; iIdx++)
		{
			g_stNvtAudPlatformDevice[iIdx].pdev = platform_device_alloc(g_stNvtAudPlatformDevice[iIdx].name,-1);
			if (!g_stNvtAudPlatformDevice[iIdx].pdev)
			{
				NVTALSA_ERR( "platform_device_alloc failed!!\n");
				return -ENOMEM;
			}

			iRet = platform_device_add(g_stNvtAudPlatformDevice[iIdx].pdev);
			if (iRet)
			{
				platform_device_put(g_stNvtAudPlatformDevice[iIdx].pdev);
				NVTALSA_ERR( "platform_device_add failed(%d)! \n", iRet);
			}
		}
	}


	for (iTargetCard = 0; iTargetCard < NVT_CARD_MAXNUM; iTargetCard++)
	{
		//! use same mc fake device for fake card
		snd_soc_fakecard[iTargetCard].dev = &g_stNvtAudPlatformDevice[0].pdev->dev;
		iRet = snd_soc_register_card(&snd_soc_fakecard[iTargetCard]);
		if (iRet)
		{
			NVTALSA_ERR("snd_soc_register_card idx:%d failed(%d)!\n", iTargetCard, iRet);
		}

		NVTALSA_TRACE( "Init done [Card%d]",iCardIdx);
	}
	return iRet;

}

//! Remove all fake card
void nvt_nvtalsa_card_exit(void)
{
	int iRet = 0, iIdx = 0, iTargetCard = 0;

	NVTALSA_TRACE( "Called");

	for (iTargetCard = 0; iTargetCard < NVT_CARD_MAXNUM; iTargetCard++)
	{
		if(g_stFakeCards[iTargetCard].soc_card != NULL)
		{
			iRet = snd_soc_unregister_card(&snd_soc_fakecard[iTargetCard]);
			if (iRet)
			{
				NVTALSA_ERR("snd_soc_unregister_card failed(%d)!\n", iRet);
			}
			g_stFakeCards[iTargetCard].soc_card = NULL;
		}
	}

	//! release platform driver
	platform_driver_unregister(&nvt_machine_driver);

	for (iIdx = 0; iIdx < NVT_PLAT_DEV_NUM; iIdx++)
	{
		platform_device_unregister(g_stNvtAudPlatformDevice[iIdx].pdev);
	}
}

int __init _nvt_audio_alsa_init(void)
{
    nvt_nvtalsa_card_init(0);

    return 0;
}

void __exit _nvt_audio_alsa_exit(void)
{
	nvt_nvtalsa_card_exit();
}

module_init(_nvt_audio_alsa_init);
module_exit(_nvt_audio_alsa_exit);

MODULE_AUTHOR("NOVATEK");
MODULE_DESCRIPTION("NVT ASoC driver");
MODULE_LICENSE("GPL");

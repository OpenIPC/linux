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

#define NVT_I2S_TEST 1

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define NVT_CARD_MAXNUM       2
#define NVT_PLAT_DEV_NUM      2


#define NVT_MACHINE_NAME       "xmos,alsa_mach"
#define NVT_MACHINE_NAME_P       "xmos,alsa_mach_playback"
#define NVT_PLATFORM_NAME_R    "nvt,alsa_plat_r"
#define NVT_PLATFORM_NAME_P    "nvt,alsa_plat_p"

#define NVT_CODEC_NAME_R         "xmos,embd_codec"
#define NVT_CODEC_NAME_P       "nvt,embd_codec_p"

#define NVT_CODECDAI_NAME_P    "nvt,codec_dai_p"
#define NVT_CODECDAI_NAME_R      "xmos,codec_dai"

#define NVT_CPUDAI_NAME0_R     "nvt,cpu_dai_r"
#define NVT_CPUDAI_NAME0_P     "nvt,cpu_dai_p"

/*-----------------------------------------------------------------------------*/
/* ALSA Debug                                                                  */
/*-----------------------------------------------------------------------------*/
#define NVTALSA_PREFIX "[alsa]"

#define NVTALSA_TRACE( fmt,...)   //printk( NVTALSA_PREFIX "[%s]" fmt "\n",__FUNCTION__, ##__VA_ARGS__);
#define NVTALSA_ERR( fmt,...)     printk( NVTALSA_PREFIX " [%s:%d]" fmt , __FUNCTION__, __LINE__, ##__VA_ARGS__);


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
		.name	= NVT_MACHINE_NAME,
	    .owner  = THIS_MODULE,
    },

	.probe		= nvt_machine_probe,
	.remove		= nvt_machine_remove,

};

static int xmos_hw_params(struct snd_pcm_substream *substream,
             struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    //struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret=0;

	NVTALSA_TRACE("in\n");
	if(cpu_dai != NULL){
    	ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 64);//64 for xmos
		if(ret < 0){
			printk("snd_soc_dai_set_bclk_ratio ret:%d\n",ret);
			return ret;
		}
	}
	else{
		printk("cpu_dai null can not set bclk_ratio\n");
		return -1;
	}
    return 0;
}




static struct snd_soc_ops xmos_ops = {
    .hw_params = xmos_hw_params,
};



static struct snd_soc_dai_link fakecard_dailink[] = {
	{
		.name 			= "XMOS_DAI_NAME_CAPTURE",					//Main Port
		.stream_name    = "XMOS_DAI_STREAM_CAPTURE",

		.platform_name  = NVT_PLATFORM_NAME_R,
		.cpu_dai_name   = NVT_CPUDAI_NAME0_R,

		.codec_name     = NVT_CODEC_NAME_R,
		.codec_dai_name = NVT_CODECDAI_NAME_R,
		.ops 			= &xmos_ops, 
		#if NVT_I2S_TEST
		.dai_fmt		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
						| SND_SOC_DAIFMT_CBS_CFS,
		#endif
	},

    {
        .name           = "XMOS_DAI_NAME_PLAYBACK",
        .stream_name    = "XMOS_DAI_STEAM_PLAYBACK",

        .platform_name  = NVT_PLATFORM_NAME_P,
        .cpu_dai_name   = NVT_CPUDAI_NAME0_P,

        .codec_name     = NVT_CODEC_NAME_P,
        .codec_dai_name = NVT_CODECDAI_NAME_P,

        #if NVT_I2S_TEST
        .dai_fmt        = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
                        | SND_SOC_DAIFMT_CBS_CFS,
        #endif
    },




};

/* SoC card */
static struct snd_soc_card snd_soc_fakecard[NVT_CARD_MAXNUM] = {
	{
		.name 		= "xmosc0capture",
		.owner 		= THIS_MODULE,
		.dai_link 	= &fakecard_dailink[0],
		.num_links 	= 1,
	},
	{
		.name 		= "xmosc0playback",
		.owner 		= THIS_MODULE,
		.dai_link 	= &fakecard_dailink[1],
		.num_links 	= 1,
	},
	
};

static struct nvtalsa_platform_device g_stNvtAudPlatformDevice[] =
{
	{.name = NVT_MACHINE_NAME,     .pdev = NULL,},
	{.name = NVT_MACHINE_NAME_P,     .pdev = NULL,},
};


ST_FAKE_CARD g_stFakeCards[NVT_CARD_MAXNUM] =
{
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

int xmos_nvtalsa_card_init(int iCardIdx)
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
			break;
		}
	}

	if(iTargetCard == NVT_CARD_MAXNUM)
	{
		NVTALSA_ERR("failed[Card%d]!\n", iCardIdx);
		return EMFILE;
	}
	else if(iTargetCard == 0)
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

	for(iTargetCard = 0; iTargetCard < NVT_CARD_MAXNUM; iTargetCard++){
		snd_soc_fakecard[iTargetCard].dev = &g_stNvtAudPlatformDevice[iTargetCard].pdev->dev;
		iRet = snd_soc_register_card(&snd_soc_fakecard[iTargetCard]);
		if (iRet)
		{
			NVTALSA_ERR(">>>snd_soc_register_card idx:%d failed(%d)!\n", iTargetCard, iRet);
		}
		NVTALSA_TRACE( "Init done [Card%d]",iCardIdx);
	}
	return iRet;

}

//! Remove all fake card
void xmos_nvtalsa_card_exit(void)
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

int __init _xmos_audio_alsa_init(void)
{
    xmos_nvtalsa_card_init(0);

    return 0;
}
void __exit _xmos_audio_alsa_exit(void)
{
	xmos_nvtalsa_card_exit();
}
module_init(_xmos_audio_alsa_init);
module_exit(_xmos_audio_alsa_exit);
MODULE_AUTHOR("NOVATEK");
MODULE_DESCRIPTION("NVT_XMOS ASoC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.00.001");

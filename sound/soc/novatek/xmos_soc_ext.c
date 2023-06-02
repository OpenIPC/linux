/*
 * AMD ALSA SoC PCM Driver for ACP 2.x
 *
 * Copyright 2014-2015 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/sizes.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>

#include <sound/soc.h>
#include <sound/tlv.h>

#include <linux/i2c.h>
#include "xmos_soc_ext.h"

#define DBG_WRN(fmt, args...) //	printk( fmt, ##args)
#define ENABLE					1
#define DISABLE					0

#define EAC_REG_BASE 0xFD640000

#define EAC_SPK_OUT_VOL			0xAC
#define EAC_MIC_IN_VOL			0x20

#define XMOS_I2C_NAME "xmos_i2c"
#define XMOS_I2C_ADDR 0x2c

int xmos_i2c_transmit(char *cmd, int value  ,char *out_buf,int *out_len, int Isset );
/*				DECLARE_TLV_DB_SCALE(name,  min,step, mute) */
static const DECLARE_TLV_DB_SCALE(spk_tlv, -9750,  50, 1);// -97 ~ +30dB
static const DECLARE_TLV_DB_SCALE(mic_tlv, -2100, 150, 0);// -21 ~ +25.5dB
#if 0
static struct snd_kcontrol_new nvt_embedded_controls[] = {
SOC_DOUBLE_TLV("Lineout Playback Volume",EAC_SPK_OUT_VOL, 0, 8, 255, 0, spk_tlv),
SOC_DOUBLE_TLV("MIC Capture Volume",EAC_MIC_IN_VOL, 0, 8, 31, 0, mic_tlv),
};
#endif
#if 1


static int xmos_control_version_info(struct snd_kcontrol *kcontrol,
                  struct snd_ctl_elem_info *uinfo){

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
    uinfo->value.integer.max = 65535;
	return 0;
};
static int xmos_control_version_get(struct snd_kcontrol *kcontrol,
                 struct snd_ctl_elem_value *ucontrol){

	char *out_buf = kmalloc(32, GFP_KERNEL);
	char cmd[1]={0};
	int len=0;
	int ret =0;

	memset(out_buf,0x0,32);
	cmd[0]=AP_CONTROL_CMD_GET_VERSION;
	ret = xmos_i2c_transmit(cmd,0,out_buf,&len, 0);
	if(ret < 0){
		printk("xmos_i2c_transmit fail\n");
	}
	//fake value
	ucontrol->value.integer.value[0]=500;
	return 0;

};



static struct snd_kcontrol_new xmos_controls[]= {

	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.info = xmos_control_version_info,
		.name = "XMOS VERSION",
		.access = SNDRV_CTL_ELEM_ACCESS_READ,
		.get	= xmos_control_version_get
	}
};
#if 0
int xmos_build_controls(struct snd_card * card){

	int ret=0;
	ret = snd_ctl_add(pt->card, snd_ctl_new1(&knew, pt));
	if (ret < 0){
		printk("%s snd_ctl_add fail  ret:%d\n",ret);
		return ret;
	}
}
#endif

typedef enum _XMOS_I2C_ID {
	XMOS_I2C_ID_1 = 0,
	XMOS_I2C_ID_2 = 1,
	XMOS_I2C_ID_3 = 2,
	xMOS_I2C_ID_4 = 3,
	XMOS_I2C_ID_5 = 4,
} XMOS_I2C_ID;


typedef struct _xmos_i2c_info {
    struct i2c_client  *xmos_i2c_client;
    struct i2c_adapter *xmos_i2c_adapter;
} XMOS_I2C_INFO;

XMOS_I2C_INFO *g_xmos_i2c_info = NULL;

static struct i2c_board_info xmos_i2c_device = {

	.type = XMOS_I2C_NAME,
	.addr = XMOS_I2C_ADDR
};

static const struct i2c_device_id xmos_i2c_id[2] = {

	{ XMOS_I2C_NAME, 0 },
	{ }
};


static int xmos_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	DBG_WRN("%s in\n",__FUNCTION__);
	g_xmos_i2c_info = kmalloc(sizeof(XMOS_I2C_INFO), GFP_KERNEL);

	if(g_xmos_i2c_info == NULL){

		printk("kmalloc g_xmos_i2c_info fail\n");
		return -1;
	}
	g_xmos_i2c_info->xmos_i2c_client = client;
	g_xmos_i2c_info->xmos_i2c_adapter = client->adapter;


    i2c_set_clientdata(client, g_xmos_i2c_info);

    return 0;
}

static int xmos_i2c_remove(struct i2c_client *client){


	DBG_WRN("%s in\n",__FUNCTION__);
	kfree(g_xmos_i2c_info);
	g_xmos_i2c_info = NULL;
	return 0;

}


static struct i2c_driver xmos_i2c_driver = {

	.driver = {
		.name  = XMOS_I2C_NAME,
		.owner = THIS_MODULE,
	},
	.probe    = xmos_i2c_probe,
	.remove   = xmos_i2c_remove,
	.id_table = xmos_i2c_id
};

static int xmos_i2c_init(void){

	int ret=0;

	DBG_WRN("%s in\n",__FUNCTION__);
	    // add i2c_device to i2c bus 0
    if (i2c_new_device(i2c_get_adapter(XMOS_I2C_ID_3), &xmos_i2c_device) == NULL) {
        printk("i2c_new_device fail. \r\n");
        return -1;
    }

    // bind i2c client driver to i2c bus
    if ((ret = i2c_add_driver(&xmos_i2c_driver)) != 0) {
        printk("i2c_add_driver fail. \r\n");
		return -1;
    }


	return 0;
}


static void xmos_i2c_release(void){

	DBG_WRN("%s in\n",__FUNCTION__);
	i2c_unregister_device(g_xmos_i2c_info->xmos_i2c_client);
	i2c_del_driver(&xmos_i2c_driver);

	return;
}
#if 1
int xmos_get_cmd_index(char cmd){

	int i=0;
	for(i=0; i< sizeof(xmos_command_map)/sizeof(XMOS_COMMAND_MAP); i++){
		if(xmos_command_map[i].command_id == cmd){
			return i;
		}
	}
	printk("can not find xmos cmd mapping\n");
	return -1;
}


int xmos_i2c_transmit(char *cmd, int value  ,char *out_buf,int *out_len, int Isset ){

#if 1
	struct i2c_msg  msgs[2];
	unsigned char buf[20]={0};
	int ret=0;
	int i=0;
	DBG_WRN("%s in\n",__FUNCTION__);
	i = xmos_get_cmd_index(cmd[0]);
	if(i<0){
		printk("xmos_get_cmd_index fail i=%d\n",i);
		return -1;
	}

	//buf[0]=0x58 & 0xff;
	buf[0]= xmos_command_map[i].request_id & 0xff;
	buf[1]= xmos_command_map[i].command_id & 0xff;
	sprintf(&buf[3],"%d",value);
	if(Isset){
		DBG_WRN("set i2c cmd\n");
		buf[2]= xmos_command_map[i].playload_len  & 0xff;
		msgs[0].flags= !I2C_M_RD;
		msgs[0].addr= XMOS_I2C_ADDR;
		msgs[0].len= (xmos_command_map[i].playload_len  & 0xff)+3;//3 --> include resource ID+ command id + payload len
		msgs[0].buf = buf;
		ret = i2c_transfer(g_xmos_i2c_info->xmos_i2c_adapter, msgs, 1); //2: read   1: write
		*out_len = 0;
	}
	else{

		DBG_WRN("get i2c cmd\n");
		buf[2]= (xmos_command_map[i].playload_len  & 0xff)+1;//1 --> include status
		msgs[0].flags= !I2C_M_RD;
		msgs[0].addr= XMOS_I2C_ADDR;
		msgs[0].len= 3;
		msgs[0].buf = buf;
		msgs[1].flags= I2C_M_RD;
		msgs[1].addr= XMOS_I2C_ADDR;
		msgs[1].len= (xmos_command_map[i].playload_len  & 0xff) +1;//1 --> include status
		msgs[1].buf =out_buf;
		ret = i2c_transfer(g_xmos_i2c_info->xmos_i2c_adapter, msgs, 2); //2: read   1: write
		*out_len = msgs[1].len;
	}
	if(ret< 0){

		DBG_WRN("i2c_transfer ret:%d\n",ret);
		return -1;
	}

#endif
	return 0;

}
#endif

#endif

static int nvt_embedded_probe(struct snd_soc_component *codec)
{
	int status;

	//status = snd_soc_add_codec_controls(codec, nvt_embedded_controls,ARRAY_SIZE(nvt_embedded_controls));
	status = snd_soc_add_component_controls(codec, xmos_controls,1);
	DBG_WRN("nvt_embedded_probe\n");
	if (status < 0) {
		DBG_WRN("%s: failed to add xmos_controls (%d).\n",__func__, status);
		return status;
	}

	return 0;
}

static void nvt_embedded_remove(struct snd_soc_component *codec)
{
	//struct adau1701 *adau1701 = snd_soc_codec_get_drvdata(codec);
	DBG_WRN("nvt_embedded_remove\n");
	return;
}

static int nvt_embedded_set_bias_level(struct snd_soc_component *codec, enum snd_soc_bias_level level)
{
	//struct adau1701 *adau1701 = snd_soc_codec_get_drvdata(codec);

	DBG_WRN("nvt_embedded_set_bias_level %d\n",(int)level);

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}

	return 0;
}


static int nvt_embedded_read_reg(void *context, unsigned int reg,
				 unsigned int *value)
{
	DBG_WRN("nvt_embedded_read_reg\n");

	return 0;
}

/* Write to a register in the audio-bank of AB8500 */
static int nvt_embedded_write_reg(void *context, unsigned int reg,
				  unsigned int value)
{

	DBG_WRN("nvt_embedded_write_reg 0x%08x = 0x%x\n",EAC_REG_BASE+reg,value);

	return 0;
}

static const struct regmap_config nvt_embedded_regmap = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_read = nvt_embedded_read_reg,
	.reg_write = nvt_embedded_write_reg,
};


//! codec
static struct snd_soc_component_driver nvt_embedded_codec = {
	.probe			= nvt_embedded_probe,
	.remove			= nvt_embedded_remove,
	//.resume		= adau1701_resume,
	//.suspend		= adau1701_suspend,
	.set_bias_level		= nvt_embedded_set_bias_level,
	.idle_bias_on		= false,


	/*
	.component_driver = {
		.controls		= adau1701_controls,
		.num_controls		= ARRAY_SIZE(adau1701_controls),
		.dapm_widgets		= adau1701_dapm_widgets,
		.num_dapm_widgets	= ARRAY_SIZE(adau1701_dapm_widgets),
		.dapm_routes		= adau1701_dapm_routes,
		.num_dapm_routes	= ARRAY_SIZE(adau1701_dapm_routes),
	},
	*/

	//.set_sysclk		= adau1701_set_sysclk,
	//.set_pll
};




static int nvt_embd_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	//struct adau1373 *adau1373 = snd_soc_codec_get_drvdata(codec);
	char *out_buf = kmalloc(32, GFP_KERNEL);
	char cmd[1]={0};
	int len=0;
	int ret =0;
	int i=0;
	DBG_WRN("nvt_embd_hw_params!!!\r\n");

	memset(out_buf,0x0,32);
	cmd[0]=SUP_CMD_GET_NOISE_SUPPRESSION_ENABLED;
	ret = xmos_i2c_transmit(cmd,0,out_buf,&len, 0);
	if(ret < 0){
		printk("xmos_i2c_transmit fail\n");
	}
	printk("nr enabled:\n");
	for(i=0;i< len;i++){
		printk("%x\n",out_buf[i]);
	}
	kfree(out_buf);

	return 0;
}


static int nvt_embd_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{

	DBG_WRN("nvt_embd_prepare!!!\r\n");


	return 0;
}


static const struct snd_soc_dai_ops nvt_embd_dai_ops = {
	.hw_params		= nvt_embd_hw_params,
	.prepare        = nvt_embd_prepare,
	//.set_sysclk	= adau1373_set_dai_sysclk,
	//.set_fmt		= adau1373_set_dai_fmt,
	//.set_bclk_ratio
};


static struct snd_soc_dai_driver nvt_embd_codec_dais[] = {

	{
		.name = "xmos,codec_dai",
		.id = 0,
		#if 0
		.playback = {
			.stream_name 	= "lineout",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 			= SNDRV_PCM_RATE_8000_48000,
			.rate_min 		= 8000,
			.rate_max 		= 48000,
			.formats 		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U8,
		},
		#endif
		.capture = {
			.stream_name 	= "mic",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 			= SNDRV_PCM_RATE_8000_48000,
			.rate_min 		= 8000,
			.rate_max 		= 48000,
		.formats 		= SNDRV_PCM_FMTBIT_S32_LE |SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U8,
		},
		.ops = &nvt_embd_dai_ops,
	},
};


static int nvt_embd_codec_device_probe(struct platform_device *pdev)
{
	//struct clk *eac_clk;
	struct regmap *embbed_regmap;
	int status;
	int ret=0;
	DBG_WRN( "nvt_embd_codec_device_probe\n");

	DBG_WRN("i2c init start\n");
	ret = xmos_i2c_init();

	if(ret != 0){
		printk("xmos_i2c_init fail\n");
		return -1;
	}



	embbed_regmap = devm_regmap_init(&pdev->dev, NULL, &pdev->dev,
					   &nvt_embedded_regmap);
	if (IS_ERR(embbed_regmap)) {
		status = PTR_ERR(embbed_regmap);
		dev_err(&pdev->dev, "%s: Failed to allocate regmap: %d\n",
			__func__, status);
		return status;
	}
	return snd_soc_register_component(&pdev->dev, &nvt_embedded_codec, nvt_embd_codec_dais, ARRAY_SIZE(nvt_embd_codec_dais));
}

static int nvt_embd_codec_device_remove (struct platform_device *pdev)
{
	DBG_WRN( "nvt_embd_codec_device_remove\n");
	xmos_i2c_release();
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}


static struct platform_driver nvt_embd_codec_driver = {
	.driver		= {
		.name		= "xmos,embd_codec",
		.owner		= THIS_MODULE,
	},
	.probe		= nvt_embd_codec_device_probe,
	.remove		= nvt_embd_codec_device_remove,
};


static struct platform_device *nvt_pdev;


int __init xmos_alsa_embd_module_init(void)
{
	int iRet;
	struct platform_device *pdev;

	DBG_WRN("xmos_alsa_embd_module_init\r\n");

	iRet = platform_driver_register(&nvt_embd_codec_driver);
	if (iRet) {
		printk("xmos_alsa_embd_module_init platform_driver_register error\n");
	}

	pdev = platform_device_alloc("xmos,embd_codec",-1);
	if (!pdev)
	{
		printk("platform_device_alloc failed!!\n");
	}

	iRet = platform_device_add(pdev);
	if (iRet)
	{
		platform_device_put(pdev);
		printk( "platform_device_add failed(%d)! \n", iRet);
	}
	nvt_pdev = pdev;

	return 0;
}

void __exit xmos_alsa_embd_module_exit(void)
{
	DBG_WRN("xmos_alsa_embd_module_exit\r\n");

	platform_device_unregister(nvt_pdev);
	platform_driver_unregister(&nvt_embd_codec_driver);
}

module_init(xmos_alsa_embd_module_init);
module_exit(xmos_alsa_embd_module_exit);


//module_platform_driver(nvt_dma_driver);

MODULE_AUTHOR("Charlie_Cnag@novatek.com.tw");
MODULE_DESCRIPTION("XMOS ALSA Embedded Codec Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:snd-soc-xmos-embd");
MODULE_VERSION("1.00.001");

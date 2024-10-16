/*
 * tas5805m.c - Driver for the TAS5805M Audio Amplifier
 *
 * Copyright (C) 2017 Texas Instruments Incorporated -  http://www.ti.com
 *
 * Author: Andy Liu <andy-liu@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/pcm_params.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/string.h>
#include "tas5805m.h"

#define TAS5805M_BURST_INIT

#ifdef TAS5805M_BURST_INIT
#include "tas5805m_burst.h"
#endif

#define TAS5805M_DRV_NAME "tas5805m"

#define TAS5805M_RATES (SNDRV_PCM_RATE_8000_96000)

#define TAS5805M_FORMATS     (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			                  SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

#define TAS5805M_REG_00      (0x00)
#define TAS5805M_REG_03      (0x03)
#define TAS5805M_REG_24      (0x24)
#define TAS5805M_REG_25      (0x25)
#define TAS5805M_REG_26      (0x26)
#define TAS5805M_REG_27      (0x27)
#define TAS5805M_REG_28      (0x28)
#define TAS5805M_REG_29      (0x29)
#define TAS5805M_REG_2A      (0x2a)
#define TAS5805M_REG_2B      (0x2b)
#define TAS5805M_REG_35      (0x35)
#define TAS5805M_REG_7F      (0x7f)

#define TAS5805M_PAGE_00     (0x00)
#define TAS5805M_PAGE_2A     (0x2a)

#define TAS5805M_BOOK_00     (0x00)
#define TAS5805M_BOOK_8C     (0x8c)

#define TAS5805M_VOLUME_MAX  (158)
#define TAS5805M_VOLUME_MIN  (0)

#define ON  1
#define OFF 0

#define CHILD  1
#define NORMAL 0

extern void sunxi_daudio_i2s_out_control(int daudio_num, int onoff);
static void tas5805m_i2s_off(void)
{
    /*sunxi_daudio_i2s_out_control(0, 0);*/
}

void tas5805m_i2s_on(void)
{
    /*sunxi_daudio_i2s_out_control(0, 1);*/
}

static const struct reg_sequence reset_seq[] = {
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x02 }, // Hi-Z
    { 0x01, 0x11 }, // reset and deep sleep
};

static const struct reg_sequence deepsleep_seq[] = {
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x02 }, // Hi-Z
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x00 }, //deep sleep
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x46, 0x11 },
};

static const struct reg_sequence hiz_seq[] = {
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x02 }, // Hi-Z
    { 0x00, 0x00 },
};

const uint32_t tas5805m_volume[] = {
		0x0000001B,    //0, -110dB
		0x0000001E,    //1, -109dB
		0x00000021,    //2, -108dB
		0x00000025,    //3, -107dB
		0x0000002A,    //4, -106dB
		0x0000002F,    //5, -105dB
		0x00000035,    //6, -104dB
		0x0000003B,    //7, -103dB
		0x00000043,    //8, -102dB
		0x0000004B,    //9, -101dB
		0x00000054,    //10, -100dB
		0x0000005E,    //11, -99dB
		0x0000006A,    //12, -98dB
		0x00000076,    //13, -97dB
		0x00000085,    //14, -96dB
		0x00000095,    //15, -95dB
		0x000000A7,    //16, -94dB
		0x000000BC,    //17, -93dB
		0x000000D3,    //18, -92dB
		0x000000EC,    //19, -91dB
		0x00000109,    //20, -90dB
		0x0000012A,    //21, -89dB
		0x0000014E,    //22, -88dB
		0x00000177,    //23, -87dB
		0x000001A4,    //24, -86dB
		0x000001D8,    //25, -85dB
		0x00000211,    //26, -84dB
		0x00000252,    //27, -83dB
		0x0000029A,    //28, -82dB
		0x000002EC,    //29, -81dB
		0x00000347,    //30, -80dB
		0x000003AD,    //31, -79dB
		0x00000420,    //32, -78dB
		0x000004A1,    //33, -77dB
		0x00000532,    //34, -76dB
		0x000005D4,    //35, -75dB
		0x0000068A,    //36, -74dB
		0x00000756,    //37, -73dB
		0x0000083B,    //38, -72dB
		0x0000093C,    //39, -71dB
		0x00000A5D,    //40, -70dB
		0x00000BA0,    //41, -69dB
		0x00000D0C,    //42, -68dB
		0x00000EA3,    //43, -67dB
		0x0000106C,    //44, -66dB
		0x0000126D,    //45, -65dB
		0x000014AD,    //46, -64dB
		0x00001733,    //47, -63dB
		0x00001A07,    //48, -62dB
		0x00001D34,    //49, -61dB
		0x000020C5,    //50, -60dB
		0x000024C4,    //51, -59dB
		0x00002941,    //52, -58dB
		0x00002E49,    //53, -57dB
		0x000033EF,    //54, -56dB
		0x00003A45,    //55, -55dB
		0x00004161,    //56, -54dB
		0x0000495C,    //57, -53dB
		0x0000524F,    //58, -52dB
		0x00005C5A,    //59, -51dB
		0x0000679F,    //60, -50dB
		0x00007444,    //61, -49dB
		0x00008274,    //62, -48dB
		0x0000925F,    //63, -47dB
		0x0000A43B,    //64, -46dB
		0x0000B845,    //65, -45dB
		0x0000CEC1,    //66, -44dB
		0x0000E7FB,    //67, -43dB
		0x00010449,    //68, -42dB
		0x0001240C,    //69, -41dB
		0x000147AE,    //70, -40dB
		0x00016FAA,    //71, -39dB
		0x00019C86,    //72, -38dB
		0x0001CEDC,    //73, -37dB
		0x00020756,    //74, -36dB
		0x000246B5,    //75, -35dB
		0x00028DCF,    //76, -34dB
		0x0002DD96,    //77, -33dB
		0x00033718,    //78, -32dB
		0x00039B87,    //79, -31dB
		0x00040C37,    //80, -30dB
		0x00048AA7,    //81, -29dB
		0x00051884,    //82, -28dB
		0x0005B7B1,    //83, -27dB
		0x00066A4A,    //84, -26dB
		0x000732AE,    //85, -25dB
		0x00081385,    //86, -24dB
		0x00090FCC,    //87, -23dB
		0x000A2ADB,    //88, -22dB
		0x000B6873,    //89, -21dB
		0x000CCCCD,    //90, -20dB
		0x000E5CA1,    //91, -19dB
		0x00101D3F,    //92, -18dB
		0x0012149A,    //93, -17dB
		0x00144961,    //94, -16dB
		0x0016C311,    //95, -15dB
		0x00198A13,    //96, -14dB
		0x001CA7D7,    //97, -13dB
		0x002026F3,    //98, -12dB
		0x00241347,    //99, -11dB
		0x00287A27,    //100, -10dB
		0x002D6A86,    //101, -9dB
		0x0032F52D,    //102, -8dB
		0x00392CEE,    //103, -7dB
		0x004026E7,    //104, -6dB
		0x0047FACD,    //105, -5dB
		0x0050C336,    //106, -4dB
		0x005A9DF8,    //107, -3dB
		0x0065AC8C,    //108, -2dB
		0x00721483,    //109, -1dB
		0x00800000,    //110, 0dB
		0x008F9E4D,    //111, 1dB
		0x00A12478,    //112, 2dB
		0x00B4CE08,    //113, 3dB
		0x00CADDC8,    //114, 4dB
		0x00E39EA9,    //115, 5dB
		0x00FF64C1,    //116, 6dB
		0x011E8E6A,    //117, 7dB
		0x0141857F,    //118, 8dB
		0x0168C0C6,    //119, 9dB
		0x0194C584,    //120, 10dB
		0x01C62940,    //121, 11dB
		0x01FD93C2,    //122, 12dB
		0x023BC148,    //123, 13dB
		0x02818508,    //124, 14dB
		0x02CFCC01,    //125, 15dB
		0x0327A01A,    //126, 16dB
		0x038A2BAD,    //127, 17dB
		0x03F8BD7A,    //128, 18dB
		0x0474CD1B,    //129, 19dB
		0x05000000,    //130, 20dB
		0x059C2F02,    //131, 21dB
		0x064B6CAE,    //132, 22dB
		0x07100C4D,    //133, 23dB
		0x07ECA9CD,    //134, 24dB
		0x08E43299,    //135, 25dB
		0x09F9EF8E,    //136, 26dB
		0x0B319025,    //137, 27dB
		0x0C8F36F2,    //138, 28dB
		0x0E1787B8,    //139, 29dB
		0x0FCFB725,    //140, 30dB
		0x11BD9C84,    //141, 31dB
		0x13E7C594,    //142, 32dB
		0x16558CCB,    //143, 33dB
		0x190F3254,    //144, 34dB
		0x1C1DF80E,    //145, 35dB
		0x1F8C4107,    //146, 36dB
		0x2365B4BF,    //147, 37dB
		0x27B766C2,    //148, 38dB
		0x2C900313,    //149, 39dB
		0x32000000,    //150, 40dB
		0x3819D612,    //151, 41dB
		0x3EF23ECA,    //152, 42dB
		0x46A07B07,    //153, 43dB
		0x4F3EA203,    //154, 44dB
		0x58E9F9F9,    //155, 45dB
		0x63C35B8E,    //156, 46dB
		0x6FEFA16D,    //157, 47dB
		0x7D982575,    //158, 48dB
};

static struct tas5805m_priv *g_tas5805m;
static struct notifier_block tas5805m_fb_notifier;

struct tas5805m_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct mutex lock;
	int ctrl_gpio;
	int power_gpio;
	int mute_gpio;
	int status_gpio;
	int vol;
	int irq;
	bool fb_off;
	bool in_playback;
	bool mute;
	int eqmode;
	int control;
	struct delayed_work mute_work;
	struct work_struct interrupt_work;
	struct regulator *dvdd;
	char odmname[20];
};

const struct regmap_config tas5805m_regmap = {
		.reg_bits = 8,
		.val_bits = 8,
		.cache_type = REGCACHE_RBTREE,
};

static void tas5805m_power(struct tas5805m_priv *tas5805m,int value);
static void tas5805m_pdn_mute(struct tas5805m_priv *tas5805m,int mute);
static int  tas5805m_i2c_init(struct tas5805m_priv *tas5805m,int eqmode,int prepare);


static int  tas5805m_prepare_init(struct tas5805m_priv *tas5805m)
{
	int ret = 0;
	int i;

	pr_info("%s\n",__func__);
	for(i=0;i< ARRAY_SIZE(reset_seq);i++){
		ret = snd_soc_write(tas5805m->codec, reset_seq[i].reg,reset_seq[i].def);
		if (ret != 0)
		{
		pr_err("Failed to initialize reset_seq ret: %d\n",ret);
		}
	}

	for(i=0;i< ARRAY_SIZE(deepsleep_seq);i++){
		ret = snd_soc_write(tas5805m->codec, deepsleep_seq[i].reg,deepsleep_seq[i].def);
		if (ret != 0)
		{
		pr_err("Failed to initialize deepsleep_seq ret: %d\n",ret);
		}
	}

	msleep(40);

	for(i=0;i< ARRAY_SIZE(hiz_seq);i++){
		ret = snd_soc_write(tas5805m->codec, hiz_seq[i].reg,hiz_seq[i].def);
		if (ret != 0)
		{
		pr_err("Failed to initialize hiz_seq ret: %d\n",ret);
		}
	}

	return ret;
}

static ssize_t tas5805m_eqmode_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

	pr_info("%s eqmode: %d\n",__func__,tas5805m->eqmode);
	return sprintf(buf,"%d\n",tas5805m->eqmode);
}

static ssize_t tas5805m_eqmode_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned int val;
	int error;
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

	pr_info("%s\n",__func__);
	error = kstrtoint(buf, 10, &val);
	if (error)
		return error;

	tas5805m->eqmode = val;
	pr_info("%s eqmode val to %d in %d \n",__func__,tas5805m->eqmode, tas5805m->in_playback);

	return count;
}

static DEVICE_ATTR(eqmode, 0664, tas5805m_eqmode_show, tas5805m_eqmode_store);

static ssize_t tas5805m_odmname_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

	pr_info("%s odmname: %s\n",__func__,tas5805m->odmname);
	return sprintf(buf,"%s\n",tas5805m->odmname);
}

static ssize_t tas5805m_odmname_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

	pr_info("%s buf:%s\n",__func__,buf);
	strcpy(tas5805m->odmname,buf);
	pr_info("%s tas5805m->odmname:%s\n",__func__,tas5805m->odmname);

	return count;
}

static DEVICE_ATTR(odmname, 0664, tas5805m_odmname_show, tas5805m_odmname_store);

static ssize_t tas5805m_eqversion_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

	pr_info("%s odmname: %s\n",__func__,tas5805m->odmname);
	return sprintf(buf,"%s\n",tas5805m->odmname);
}

static ssize_t tas5805m_eqversion_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

	pr_info("%s buf:%s\n",__func__,buf);
	strcpy(tas5805m->odmname,buf);
	pr_info("%s tas5805m->odmname:%s\n",__func__,tas5805m->odmname);

	return count;
}

static DEVICE_ATTR(eqversion, 0664, tas5805m_eqversion_show, tas5805m_eqversion_store);

/* eq value checking: for debug only */
static int tas5805m_get_eqindex(const struct reg_sequence *eq_sequence, int size)
{
	int i;

	for(i = 3; i < size; i++) {
		if ((eq_sequence[i].reg == 0x7f && eq_sequence[i].def == 0x8c) &&
		    (eq_sequence[i-3].reg == 0x00 && eq_sequence[i-3].def == 0x00))
			return (i-3);
	}

	return -1;
}

static bool tas5805m_eqcheck(struct tas5805m_priv *tas5805m,
			     const struct reg_sequence *eq_sequence,
		             int eq_start, int eq_end)
{
	int i, val, ret;

	if (eq_start < 0 || eq_start >= eq_end) {
		pr_err("%s: invalid EQ sequence\n", __func__);
		return false;
	}

	// do not need check reset and register tuning part
	for (i = eq_start; i < eq_end; i++) {
		if ((eq_sequence[i].reg == 0x00) ||
		    (eq_sequence[i].reg == 0x7f && eq_sequence[i-1].reg != 0x7e)) {
			if (eq_sequence[i].reg == 0x7f && eq_sequence[i].def == 0xaa)
				val = 0x8c;
			else
				val = eq_sequence[i].def;
			ret = snd_soc_write(tas5805m->codec,
					    eq_sequence[i].reg,
					    val);
			if (ret != 0) {
				pr_err("%s: write failed, ret: %d\n", __func__, ret);
				return false;
			}
		} else {
			ret = snd_soc_read(tas5805m->codec,
					   eq_sequence[i].reg);
			if (ret != eq_sequence[i].def) {
				pr_err("%s: reg[%d-%#x] should be %#x, but real value is %#x\n",
					__func__, i, eq_sequence[i].reg,
					eq_sequence[i].def, ret);
				return false;
			}
		}
	}

	return true;
}

static ssize_t tas5805m_eqcheck_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int eq_start, eq_end;
	bool eq_ok = true;
	char odm_name[16] = {0};
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

        pr_info("%s (odm %s) eqmode:%d\n",__func__, tas5805m->odmname, tas5805m->eqmode);
        strncpy(odm_name, tas5805m->odmname, sizeof(odm_name));
	mutex_lock(&tas5805m->lock);
	if (tas5805m->control == OFF) {
		tas5805m_power(tas5805m,ON);
		tas5805m_pdn_mute(tas5805m,OFF);
		tas5805m_i2s_on();
		msleep(2);
		tas5805m_i2c_init(tas5805m,tas5805m->eqmode, 0);
		tas5805m_i2s_off();
		enable_irq(tas5805m->irq);
	}

	if (tas5805m->eqmode == NORMAL) {
		if (!strcmp(odm_name, "EQ_CONFIG_1")) {
			eq_start = tas5805m_get_eqindex(tas5805m_init_sequence_1,
							ARRAY_SIZE(tas5805m_init_sequence_1));
			eq_end = ARRAY_SIZE(tas5805m_init_sequence_1) - 8;
			eq_ok = tas5805m_eqcheck(tas5805m, tas5805m_init_sequence_1, eq_start, eq_end);
		} else if (!strcmp(odm_name, "EQ_CONFIG_2")) {
                        eq_start = tas5805m_get_eqindex(tas5805m_init_sequence_2,
                                                        ARRAY_SIZE(tas5805m_init_sequence_2));
                        eq_end = ARRAY_SIZE(tas5805m_init_sequence_2) - 8;
                        eq_ok = tas5805m_eqcheck(tas5805m, tas5805m_init_sequence_2, eq_start, eq_end);
                } else {
			pr_info("%s check eq, %s not found!\n", __func__, odm_name);
			eq_start = tas5805m_get_eqindex(tas5805m_init_sequence_1,
							ARRAY_SIZE(tas5805m_init_sequence_1));
			eq_end = ARRAY_SIZE(tas5805m_init_sequence_1) - 8;
			eq_ok = tas5805m_eqcheck(tas5805m, tas5805m_init_sequence_1, eq_start, eq_end);
		}

	} else if (tas5805m->eqmode == CHILD) {

	} else {
		pr_err("%s: error eqmode %d\n", __func__, tas5805m->eqmode);
		eq_ok = false;
	}

	if (!tas5805m->in_playback && tas5805m->fb_off && (tas5805m->control == ON)) {
		disable_irq(tas5805m->irq);
		tas5805m_pdn_mute(tas5805m,ON);
		tas5805m_power(tas5805m,OFF);
	}

	mutex_unlock(&tas5805m->lock);
	if (eq_ok)
		return sprintf(buf,"%s eqmode %d is ok\n", odm_name, tas5805m->eqmode);
	else
		return sprintf(buf,"%s eqmode %d failed, pls try again, data regs may not stable!\n",
			       odm_name, tas5805m->eqmode);
}

static DEVICE_ATTR(eqcheck, 0444, tas5805m_eqcheck_show, NULL);

static struct attribute *tas5805m_attributes[] = {
	&dev_attr_eqmode.attr,
	&dev_attr_eqcheck.attr,
	&dev_attr_odmname.attr,
	&dev_attr_eqversion.attr,
	NULL
};

static const struct attribute_group tas5805m_attr_group = {
	.attrs = tas5805m_attributes,
};

static void tas5805m_clear_fault(struct tas5805m_priv *tas5805m)
{
	int fault[4] = {0};

	// changes book and page.
	snd_soc_write(tas5805m->codec, 0x00, 0x00);
	snd_soc_write(tas5805m->codec, 0x7f, 0x00);
	// read fault
	fault[0] = snd_soc_read(tas5805m->codec, 0x70);
	fault[1] = snd_soc_read(tas5805m->codec, 0x71);
	fault[2] = snd_soc_read(tas5805m->codec, 0x72);
	fault[3] = snd_soc_read(tas5805m->codec, 0x73);
	// clear fault
	snd_soc_write(tas5805m->codec, 0x78, 0x80);

	// This fault cannot auto recovery, SW can just power off to protect HW.
	if ((fault[0] & 0x0f) || (fault[2] & 0x1)) {
		pr_info("%s: meet unrecovery fault\n", __func__);
		disable_irq(tas5805m->irq);
		tas5805m_pdn_mute(tas5805m,ON);
		tas5805m_power(tas5805m,OFF);
	}

	pr_info("%s: fault reg 0x70-0x73: %#x, %#x, %#x, %#x\n",
		__func__, fault[0], fault[1], fault[2], fault[3]);
}

static int tas5805m_vol_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = (SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE);
	uinfo->count  = 1;

	uinfo->value.integer.min  = TAS5805M_VOLUME_MIN;
	uinfo->value.integer.max  = TAS5805M_VOLUME_MAX;
	uinfo->value.integer.step = 1;

	return 0;
}

static int tas5805m_vol_locked_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5805m_priv *tas5805m = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&tas5805m->lock);
	ucontrol->value.integer.value[0] = tas5805m->vol;
	mutex_unlock(&tas5805m->lock);

	return 0;
}

static inline int get_volume_index(int vol)
{
	int index;

	index = vol;

	if (index < TAS5805M_VOLUME_MIN)
		index = TAS5805M_VOLUME_MIN;

	if (index > TAS5805M_VOLUME_MAX)
		index = TAS5805M_VOLUME_MAX;

	return index;
}

static void tas5805m_set_volume(struct snd_soc_codec *codec, int vol)
{
	unsigned int index;
	uint32_t volume_hex;
	uint8_t byte4;
	uint8_t byte3;
	uint8_t byte2;
	uint8_t byte1;

	index = get_volume_index(vol);
	volume_hex = tas5805m_volume[index];

	byte4 = ((volume_hex >> 24) & 0xFF);
	byte3 = ((volume_hex >> 16) & 0xFF);
	byte2 = ((volume_hex >> 8)	& 0xFF);
	byte1 = ((volume_hex >> 0)	& 0xFF);

	//w 58 00 00
	snd_soc_write(codec, TAS5805M_REG_00, TAS5805M_PAGE_00);
	//w 58 7f 8c
	snd_soc_write(codec, TAS5805M_REG_7F, TAS5805M_BOOK_8C);
	//w 58 00 2a
	snd_soc_write(codec, TAS5805M_REG_00, TAS5805M_PAGE_2A);
	//w 58 24 xx xx xx xx
	snd_soc_write(codec, TAS5805M_REG_24, byte4);
	snd_soc_write(codec, TAS5805M_REG_25, byte3);
	snd_soc_write(codec, TAS5805M_REG_26, byte2);
	snd_soc_write(codec, TAS5805M_REG_27, byte1);
	//w 58 28 xx xx xx xx
	snd_soc_write(codec, TAS5805M_REG_28, byte4);
	snd_soc_write(codec, TAS5805M_REG_29, byte3);
	snd_soc_write(codec, TAS5805M_REG_2A, byte2);
	snd_soc_write(codec, TAS5805M_REG_2B, byte1);

}

static int tas5805m_vol_locked_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5805m_priv *tas5805m = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&tas5805m->lock);

	tas5805m->vol = ucontrol->value.integer.value[0];
	tas5805m_set_volume(codec, tas5805m->vol);

	mutex_unlock(&tas5805m->lock);

	return 0;
}

static int tas5805m_mute_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5805m_priv *tas5805m = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&tas5805m->lock);
	ucontrol->value.integer.value[0] = tas5805m->mute;
	mutex_unlock(&tas5805m->lock);

	return 0;
}

static void tas5805m_check_crc(struct tas5805m_priv *tas5805m)
{
	int eq_crc[2] = {0};

	snd_soc_write(tas5805m->codec, 0x00, 0x00);
	snd_soc_write(tas5805m->codec, 0x7f, 0x8c);
	eq_crc[0] = snd_soc_read(tas5805m->codec, 0x7d);
	eq_crc[1] = snd_soc_read(tas5805m->codec, 0x7e);

	pr_info("%s: crc: %#x, %#x\n", __func__, eq_crc[0], eq_crc[1]);
}

#ifdef TAS5805M_BURST_INIT
static int tas5805m_burst_write(struct tas5805m_priv *tas5805m, const struct cfg_reg *r, int size)
{
	int i = 0;
	int ret = 0;
	unsigned char *addr;

	while (i < size) {
		switch (r[i].command) {
		case CFG_META_SWITCH:
			// Used in legacy applications.  Ignored here.
			break;
		case CFG_META_DELAY:
			msleep(r[i].param);
			break;
		case CFG_META_BURST:
			// The val stored in first addr of r[i+1] is reg.
			addr = ((unsigned char *)&r[i+1]) + 1;
			ret = regmap_bulk_write(tas5805m->regmap, r[i+1].command, addr, r[i].param - 1);
			if (ret != 0) {
				pr_err("Failed to initialize TAS5805M ret: %d\n",ret);
			}
			i +=  (r[i].param + 1)/2;
			break;
		default:
			ret = snd_soc_write(tas5805m->codec, r[i].command, r[i].param);
			if (ret != 0) {
				pr_err("Failed to initialize TAS5805M ret: %d\n",ret);
			}
			break;
		}
		i++;
	}

	return ret;
}

static int  tas5805m_i2c_init(struct tas5805m_priv *tas5805m,int eqmode,int prepare)
{
	int ret = 0;

	if(prepare){
		tas5805m_prepare_init(tas5805m);
	}

        pr_info("%s_burst(odm %s) eqmode:%d\n",__func__, tas5805m->odmname, tas5805m->eqmode);

	if (!strcmp(tas5805m->odmname, "EQ_CONFIG_1")) {
		pr_info("%s use EQ_CONFIG_1 \n",__func__);
		ret = tas5805m_burst_write(tas5805m, tas5805m_init_regs_1, ARRAY_SIZE(tas5805m_init_regs_1));
	} else if (!strcmp(tas5805m->odmname, "EQ_CONFIG_2")) {
		pr_info("%s use EQ_CONFIG_2 \n",__func__);
		ret = tas5805m_burst_write(tas5805m, tas5805m_init_regs_2, ARRAY_SIZE(tas5805m_init_regs_2));
        } else {
		pr_info("%s can not find odm name, write tongli eq default!\n",__func__);
		ret = tas5805m_burst_write(tas5805m, tas5805m_init_regs_1, ARRAY_SIZE(tas5805m_init_regs_1));
        }
	tas5805m_check_crc(tas5805m);

	return ret;
}
#else
static int  tas5805m_i2c_init(struct tas5805m_priv *tas5805m,int eqmode,int prepare)
{
	int ret = 0;
	int i;

	if(prepare){
		tas5805m_prepare_init(tas5805m);
	}

	pr_info("%s(odm %s) eqmode:%d\n",__func__, tas5805m->odm_name, tas5805m->eqmode);

/*
	TODO
	for(i=0;i< ARRAY_SIZE(tas5805m_init_sequence);i++){
		ret = snd_soc_write(tas5805m->codec, tas5805m_init_sequence[i].reg,tas5805m_init_sequence[i].def);
		if (ret != 0) {
			pr_err("Failed to initialize TAS5805M to normal mode ret: %d\n",ret);
		}
	}
*/
	tas5805m_check_crc(tas5805m);

	return ret;
}
#endif
// the caller must take the tas5805m->lock
static void tas5805m_mute_locked(struct tas5805m_priv *tas5805m)
{
	int reg03_value, reg35_value;
	bool unmute = tas5805m->in_playback && !tas5805m->mute;

	pr_info("%s: in_playback %d, mute %d\n", __func__, tas5805m->in_playback, tas5805m->mute);
	if (unmute) {
		// unmute and play mode
		reg03_value = 0x03;
		reg35_value = 0x11;
	} else {
		// mute and deep sleep
		reg03_value = 0x08;
		reg35_value = 0x00;
	}

	snd_soc_write(tas5805m->codec, TAS5805M_REG_00, TAS5805M_PAGE_00);
	snd_soc_write(tas5805m->codec, TAS5805M_REG_7F, TAS5805M_BOOK_00);
	snd_soc_write(tas5805m->codec, TAS5805M_REG_00, TAS5805M_PAGE_00);
	// entry hiz mode first, then goto deep sleep or play mode.
	snd_soc_write(tas5805m->codec, TAS5805M_REG_03, 0x02);
	snd_soc_write(tas5805m->codec, TAS5805M_REG_03, reg03_value);
	snd_soc_write(tas5805m->codec, TAS5805M_REG_35, reg35_value);
}

static void tas5805m_mute_work(struct work_struct *work)
{
	struct tas5805m_priv *tas5805m = container_of(work,struct tas5805m_priv,mute_work.work);

	mutex_lock(&tas5805m->lock);
	// recheck in_playback once more in case it changes after mutex unlock.
	if (tas5805m->in_playback)
		tas5805m_mute_locked(tas5805m);
	mutex_unlock(&tas5805m->lock);
	return;
}

static int tas5805m_mute_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5805m_priv *tas5805m = snd_soc_codec_get_drvdata(codec);
	bool mute;

	mutex_lock(&tas5805m->lock);
	mute = ucontrol->value.integer.value[0];
	if (mute != tas5805m->mute) {
		tas5805m->mute = mute;
		if (tas5805m->in_playback)
			mod_delayed_work(system_wq, &tas5805m->mute_work, msecs_to_jiffies(50));
	}
	mutex_unlock(&tas5805m->lock);

	return 0;
}

static const struct snd_kcontrol_new tas5805m_vol_control[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name  = "Master Playback Volume",
		.info  = tas5805m_vol_info,
		.get   = tas5805m_vol_locked_get,
		.put   = tas5805m_vol_locked_put,
	},
	SOC_SINGLE_BOOL_EXT("mute_control",
		0,
		tas5805m_mute_get,
		tas5805m_mute_put),
};

static int tas5805m_snd_probe(struct snd_soc_codec *codec)
{
	int ret;
	struct tas5805m_priv *tas5805m = snd_soc_codec_get_drvdata(codec);

	tas5805m->codec = codec;
	ret = snd_soc_add_codec_controls(codec, tas5805m_vol_control, 2);
	// never use the regmap cache for ti have init/book/page sequeue.
	regcache_cache_bypass(tas5805m->regmap, true);

	return ret;
}

static struct snd_soc_codec_driver soc_codec_tas5805m = {
		.probe = tas5805m_snd_probe,
};

static void tas5805m_power(struct tas5805m_priv *tas5805m,int value)
{
#if 1
	if (value == ON) {
		tas5805m->control = ON;
	} else if (value == OFF) {
		tas5805m->control = OFF;
	}

	return;
#else
	if(value == ON){
		if (gpio_is_valid(tas5805m->ctrl_gpio)) {
			pr_info("%s pjaudio set ctrl en gpio to 0 !\n",__func__);
			gpio_set_value(tas5805m->ctrl_gpio, 0);
		}
		if (gpio_is_valid(tas5805m->power_gpio)) {
			pr_info("%s pjaudio v2 power on!\n",__func__);
			gpio_set_value(tas5805m->power_gpio, 1);
		}
		tas5805m->control = ON;
		msleep(21);
	}
	if(value == OFF){
		if (gpio_is_valid(tas5805m->power_gpio)) {
			pr_info("%s  power off!\n",__func__);
			gpio_set_value(tas5805m->power_gpio, 0);
		}
		if (gpio_is_valid(tas5805m->ctrl_gpio)) {
			pr_info("%s set ctrl en gpio to 1 !\n",__func__);
			gpio_set_value(tas5805m->ctrl_gpio, 1);
		}
		tas5805m->control = OFF;
	}
#endif
}

static void  tas5805m_pdn_mute(struct tas5805m_priv *tas5805m,int mute)
{
	if (gpio_is_valid(tas5805m->mute_gpio)){
		if(mute == ON){
			//mute
			gpio_set_value(tas5805m->mute_gpio, 0);
		}
		if(mute == OFF){
			//unmute
			gpio_set_value(tas5805m->mute_gpio, 1);
		}
	}
	msleep(5);
}

static int tas5805m_startup(struct snd_pcm_substream *substream,
							struct snd_soc_dai *dai)
{
	struct tas5805m_priv *tas5805m = snd_soc_codec_get_drvdata(dai->codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		pr_info("%s  play begin %d\n",__func__, tas5805m->control);

	mutex_lock(&tas5805m->lock);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		tas5805m->in_playback = true;
	}
	if (tas5805m->control == OFF) {
		tas5805m_power(tas5805m,ON);
		tas5805m_pdn_mute(tas5805m,OFF);
		tas5805m_i2s_on();
		msleep(2);
		tas5805m_i2c_init(tas5805m,tas5805m->eqmode, 0);
		enable_irq(tas5805m->irq);
		tas5805m_mute_locked(tas5805m);
	}

	mutex_unlock(&tas5805m->lock);
	return 0;
}
static void tas5805m_shutdown(struct snd_pcm_substream *substream,
							  struct snd_soc_dai *dai)
{
	struct tas5805m_priv *tas5805m = snd_soc_codec_get_drvdata(dai->codec);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return;
	pr_info("tas5805m play end: fb %d, control %d \n", tas5805m->fb_off, tas5805m->control);
	mutex_lock(&tas5805m->lock);
	tas5805m->in_playback = false;
	if (tas5805m->fb_off && (tas5805m->control == ON)) {
		disable_irq(tas5805m->irq);
		tas5805m_pdn_mute(tas5805m,ON);
		tas5805m_i2s_off();
		tas5805m_power(tas5805m,OFF);
	}
	mutex_unlock(&tas5805m->lock);
}

static const struct snd_soc_dai_ops tas5805m_dai_ops = {
		.startup = tas5805m_startup,
		.shutdown = tas5805m_shutdown,
};

static int tas5805m_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct tas5805m_priv *tas5805m = g_tas5805m;
	struct fb_event *evdata = data;
	int blank;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;
	pr_info("%s: blank=%d \n", __func__, blank);

	switch (blank) {
	case FB_BLANK_UNBLANK:
		mutex_lock(&tas5805m->lock);
		tas5805m->fb_off = false;
		mutex_unlock(&tas5805m->lock);
		break;
	case FB_BLANK_POWERDOWN:
		mutex_lock(&tas5805m->lock);
                // power off 5805 if not in playback
                if (!tas5805m->in_playback && (tas5805m->control == ON)) {
                        disable_irq(tas5805m->irq);
                        tas5805m_pdn_mute(tas5805m,ON);
                        tas5805m_i2s_off();
                        tas5805m_power(tas5805m,OFF);
                }
		tas5805m->fb_off = true;
		mutex_unlock(&tas5805m->lock);
		break;
	default:
		break;
	}

	return 0;
}

static struct snd_soc_dai_driver tas5805m_dai = {
	.name = "tas5805m-i2s",
	.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = TAS5805M_RATES,
			.formats = TAS5805M_FORMATS,
		},
	.capture        = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= TAS5805M_RATES,
		.formats	= TAS5805M_FORMATS,
	},
	.ops = &tas5805m_dai_ops,
};
static void  tas5805m_interrupt_work(struct work_struct *work)
{
	struct tas5805m_priv *tas5805m = container_of(work,struct tas5805m_priv,interrupt_work);

	pr_info("%s:control %d\n",__func__, tas5805m->control);
	mutex_lock(&tas5805m->lock);
	if (tas5805m->control == ON)
		tas5805m_clear_fault(tas5805m);
	mutex_unlock(&tas5805m->lock);
}
static irqreturn_t tas5805m_interrupt_handler(int irq, void *data)
{
	struct tas5805m_priv *tas5805m = data;

	printk("%s warning:tas5805m fault gpio detected!\n",__func__);

	schedule_work(&tas5805m->interrupt_work);

	return IRQ_HANDLED;
}
static int parse_dt(struct device *dev,struct tas5805m_priv *tas5805m)
{
	int ret;
	struct device_node *np = dev->of_node;

#if 0
	tas5805m->power_gpio = of_get_named_gpio(np, "power-gpio", 0);
	if (!gpio_is_valid(tas5805m->power_gpio)) {
		pr_err("%s get invalid power_gpio %d\n", __func__,
		       tas5805m->power_gpio);
		ret = -EINVAL;
	}

	ret = devm_gpio_request_one(dev, tas5805m->power_gpio,
				    GPIOF_OUT_INIT_LOW, "tas5805m power");
	if (ret < 0)
		return ret;

	tas5805m->ctrl_gpio = of_get_named_gpio(np, "ctrl-gpio", 0);
	if (!gpio_is_valid(tas5805m->ctrl_gpio)) {
		pr_err("%s get invalid ctrl_gpio %d\n", __func__,
			tas5805m->ctrl_gpio);
		ret = -EINVAL;
	}

	ret = devm_gpio_request_one(dev, tas5805m->ctrl_gpio,
			    GPIOF_OUT_INIT_HIGH, "tas5805m ctrl");
	if (ret < 0)
		return ret;
#endif
	tas5805m->mute_gpio = of_get_named_gpio(np, "mute-gpio", 0);

	if (!gpio_is_valid(tas5805m->mute_gpio)) {
		pr_err("%s get invalid extamp-mute-gpio %d\n", __func__,
		       tas5805m->mute_gpio);
		ret = -EINVAL;
	}

	ret = devm_gpio_request_one(dev, tas5805m->mute_gpio,
				    GPIOF_OUT_INIT_LOW, "tas5805m mute");
	if (ret < 0)
		return ret;

	tas5805m->status_gpio = of_get_named_gpio(np, "fault-gpio", 0);

	if( gpio_is_valid(tas5805m->status_gpio) ) {
		pr_err("status_gpio pin(%u) \n", tas5805m->status_gpio);
        ret = gpio_request(tas5805m->status_gpio, "tas5805_status_gpio");
        if (ret != 0) {
            pr_err(" gpio_request error pin(%u) \n", tas5805m->status_gpio);
            return -1;
        }

        gpio_direction_input(tas5805m->status_gpio);
        tas5805m->irq = gpio_to_irq(tas5805m->status_gpio);
        printk("irq num:%d\n",tas5805m->irq);
        if (tas5805m->irq < 0) {
            pr_err(" gpio_to_irq error pin(%u) \n", tas5805m->status_gpio);
            gpio_free(tas5805m->status_gpio);
            return -1;
        }
        ret = request_irq(tas5805m->irq, (irq_handler_t)tas5805m_interrupt_handler, IRQF_TRIGGER_FALLING, "tas5805_status", tas5805m);
        if (ret != 0) {
            pr_err(" request_irq error pin(%u) \n", tas5805m->status_gpio);
            gpio_free(tas5805m->status_gpio);
            return -1;
        }
        disable_irq(tas5805m->irq);
	}

	return 0;
}

static int tas5805m_probe(struct device *dev, struct regmap *regmap)
{
	struct tas5805m_priv *tas5805m;

	struct i2c_client *client = to_i2c_client(dev);
	int ret = 0;

	pr_info("%s\n",__func__);
	tas5805m = devm_kzalloc(dev, sizeof(struct tas5805m_priv), GFP_KERNEL);
	if (!tas5805m)
		return -ENOMEM;

	dev_set_drvdata(dev, tas5805m);

	i2c_set_clientdata(client, tas5805m);

	tas5805m->regmap = regmap;
	tas5805m->vol    = 100;         //100, -10dB
	tas5805m->in_playback = false;
	tas5805m->mute = false;
	tas5805m->control = OFF;

	mutex_init(&tas5805m->lock);


	if (!dev->of_node) {
		dev_err(dev, "%s, cannot find dts node!\n", __func__);
		return ret;
	}

	dev_set_name(dev, "%s", TAS5805M_DRV_NAME);

	INIT_WORK(&tas5805m->interrupt_work,tas5805m_interrupt_work);
	INIT_DELAYED_WORK(&tas5805m->mute_work, tas5805m_mute_work);

	g_tas5805m = tas5805m;
	tas5805m_fb_notifier.notifier_call = tas5805m_fb_notifier_callback;
	if (fb_register_client(&tas5805m_fb_notifier))
		pr_err("%s: register fb_notifier fail!\n", __func__);

	/*TODO*/
	strcpy(tas5805m->odmname, "EQ_CONFIG_1");
	parse_dt(dev,tas5805m);
	tas5805m->eqmode = NORMAL;

	ret = snd_soc_register_codec(dev, &soc_codec_tas5805m, &tas5805m_dai,
				     1);
	if (ret != 0) {
		dev_err(dev, "Failed to register CODEC: %d\n", ret);
	}

	ret = sysfs_create_group(&client->dev.kobj, &tas5805m_attr_group);
	if(ret){
		pr_info("%s sysfs_create_group failed!\n",__func__);
	}

	pr_info("%s end ret:%d\n",__func__,ret);
	return ret;
}

static int tas5805m_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct regmap *regmap;
	struct regmap_config config = tas5805m_regmap;

	regmap = devm_regmap_init_i2c(i2c, &config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	return tas5805m_probe(&i2c->dev, regmap);
}

static int tas5805m_remove(struct device *dev)
{
	snd_soc_unregister_codec(dev);

	return 0;
}

static int tas5805m_i2c_remove(struct i2c_client *i2c)
{
	tas5805m_remove(&i2c->dev);

	return 0;
}

static const struct i2c_device_id tas5805m_i2c_id[] = {{"tas5805m", 0}, {} };
MODULE_DEVICE_TABLE(i2c, tas5805m_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id tas5805m_of_match[] = {
	{
		.compatible = "ti,tas5805m",
	},
	{} };
MODULE_DEVICE_TABLE(of, tas5805m_of_match);
#endif

#ifdef CONFIG_PM_SLEEP
/*****************************************
 *** tas5805m_suspend
 *****************************************/
static int tas5805m_suspend(struct device *dev)
{
	struct tas5805m_priv *tas5805m = dev_get_drvdata(dev);

	if (tas5805m->control == ON) {
		disable_irq(tas5805m->irq);
		tas5805m_pdn_mute(tas5805m, ON);
		tas5805m_i2s_off();
		tas5805m_power(tas5805m, OFF);
	}
	return 0;
}

/*****************************************
 *** tas5805m_resume
 *****************************************/
static int tas5805m_resume(struct device *dev)
{
	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops tas5805m_i2c_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(tas5805m_suspend, tas5805m_resume)
};
#endif

static void tas5805m_i2c_shutdown(struct i2c_client *client)
{
	struct tas5805m_priv *tas5805m = (struct tas5805m_priv *)i2c_get_clientdata(client);

	pr_info("%s \n",__func__);
	disable_irq(tas5805m->irq);
	tas5805m_pdn_mute(tas5805m,ON);
	tas5805m_power(tas5805m,OFF);
}

static struct i2c_driver tas5805m_i2c_driver = {
	.probe = tas5805m_i2c_probe,
	.remove = tas5805m_i2c_remove,
	.shutdown = tas5805m_i2c_shutdown,
	.id_table = tas5805m_i2c_id,
	.driver = {
			.name = TAS5805M_DRV_NAME,
			.of_match_table = tas5805m_of_match,
#ifdef CONFIG_PM_SLEEP
			.pm = &tas5805m_i2c_pm_ops,
#endif
		},
};

module_i2c_driver(tas5805m_i2c_driver);

MODULE_AUTHOR("Andy Liu <andy-liu@ti.com>");
MODULE_DESCRIPTION("TAS5805M Audio Amplifier Driver");
MODULE_LICENSE("GPL v2");

/*
 *  pcm for anyka chip
 *  Copyright (c) by Anyka, Inc.
 *  Create by panqihe 2014-06-09
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <mach/akpcmL0.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

#include "aec.h"
#include "capture.h"
#include "playback.h"

MODULE_AUTHOR("Anyka, Inc.");
MODULE_DESCRIPTION("akpcm device");
MODULE_LICENSE("GPL");

#define MALLOC_CB_DEBUG     (0)

static struct akpcm akxpcm;

static inline unsigned int bytes_to_samples(struct akpcm_runtime *rt,
                                unsigned int size)
{
	return (size * 8 / rt->cfg.sample_bits);
}

static inline unsigned int samples_to_bytes(struct akpcm_runtime *rt,
                                unsigned int size)
{
	return (size * rt->cfg.sample_bits / 8);
}

static inline unsigned int bytes_to_frames(struct akpcm_runtime *rt,
                                unsigned int size)
{
	unsigned int frame_bits = rt->cfg.channels * rt->cfg.sample_bits;
	return (size * 8 / frame_bits);
}

inline void akpcm_set_mic_gain(unsigned long gain)
{
    ak39_set_mic_gain(gain);
}

inline void akpcm_set_hp_gain(unsigned long gain)
{
    ak39_set_hp_gain(gain);
}

static int pcm_major = 0;
static struct class *pcm_class[2];

/* AK39 ALSA interfaces*/
unsigned long *ppb_statu;

static inline void lock_playback_io(struct akpcm *pcm)
{
	mutex_lock(&pcm->io_lock);
}

static inline void unlock_playback_io(struct akpcm *pcm)
{
	mutex_unlock(&pcm->io_lock);
}

static inline void lock_capture_io(struct akpcm *pcm)
{
	mutex_lock(&pcm->io_lock);
}

static inline void unlock_capture_io(struct akpcm *pcm)
{
	mutex_unlock(&pcm->io_lock);
}

/**
 * set source for OUTPUT(HP or ADC2)
 * author: panqihe 2014-06-25
 * @addr: MIXER_SRC_HP or MIXER_SRC_ADC2
 * @src: source
 */
void set_channel_source(struct akpcm *pcm, int addr, int src)
{
	int pa_ctrl;
	int s_dac, s_linein, s_mic;
	ak_pcm_debug_ex("addr=%d, src=%d", addr, src);

    switch (addr) {
    case MIXER_SRC_HP:
        if (pcm->bIsHPmuteUsed) {
			ak_gpio_setpin(pcm->hpmute_gpio.pin, pcm->hpmute_EnValue);
		}
		pa_ctrl = ak_gpio_getpin(pcm->SPdown_gpio.pin);
		ak_gpio_setpin(pcm->SPdown_gpio.pin, pcm->SPdown_gpio.value);
		ak39_set_hp_power((bool)src, pcm->bIsHPmuteUsed == 0);
		ak_gpio_setpin(pcm->SPdown_gpio.pin, pa_ctrl);
		if(pcm->bIsHPmuteUsed){
			ak_gpio_setpin(pcm->hpmute_gpio.pin, pcm->hpmute_DisValue);
		}
		ak39_set_hp_in(src);
		/*AK37_power_source(src, addr, pcm->mixer_source);*/
		//ak39_set_src_power(0, src);
        break;
    case MIXER_SRC_LO:
        ak_pcm_debug("No MIXER SOURECE FOR LO!"); //while(1);
		//ak39_set_src_power(addr, src);
		//ak39_set_adc2_in(src);
		/* set ADC2 poweron in PCM interface function: capture_prepare() */
        break;
    case MIXER_SRC_ADC2:
        /*AK37_power_source(src, addr, pcm->mixer_source);*/
		//ak39_set_src_power(1, src);
		ak39_set_adc2_in(src);
		/* ADC2 is poweron in capture_prepare() */
        break;
    default:
        ak_pcm_debug("Invalid mixer addr!");
		return;
    }

	pcm->mixer_source[addr] = src;

	/* get the route status */
	s_dac = (!!(pcm->mixer_source[MIXER_SRC_HP] & SOURCE_DAC)) |
		(!!(pcm->mixer_source[MIXER_SRC_ADC2] & SOURCE_ADC_DAC));

	s_linein = (!!(pcm->mixer_source[MIXER_SRC_HP] & SOURCE_LINEIN)) |
		(!!(pcm->mixer_source[MIXER_SRC_ADC2] & SOURCE_ADC_LINEIN));

	s_mic = (!!(pcm->mixer_source[MIXER_SRC_HP] & SOURCE_MIC)) |
		(!!(pcm->mixer_source[MIXER_SRC_ADC2] & SOURCE_ADC_MIC));

	ak_pcm_debug_ex("src=%d, s_dac=%d, s_linein=%d, s_mic=%d",
			src, s_dac, s_linein, s_mic);

	/* set the power according to the route status */
	ak39_set_linein_power(s_linein);
	ak39_set_mic_power(s_mic);
}

/**
 * change source for OUTPUT(HP or ADC2)
 * author panqihe 2014-06-25
 * @addr: MIXER_SRC_HP or MIXER_SRC_ADC2
 * @src: source
 */
int change_channel_source(struct akpcm *pcm, int addr, int src)
{
    int change = 0;
	if (pcm->mixer_source[addr] != src) {
	    ak_pcm_debug_ex("source=%d, src=%x", pcm->mixer_source[addr], src);
	    change = 1;
		set_channel_source(pcm, addr, src);
	}

	return change;
}

inline void akpcm_notify_cb(T_U32 code)
{
	//printf("~~~~notify: 0x%x\n", code);
}

#if MALLOC_CB_DEBUG
/* for test mallic size and time cnt */
#define  ONCE_MALLOC_SIZE 4096
static int realTotalMallocSize = 0;
static int needTotalMallocSize = 0;
static int maxMallocSize = 0;
static int malloc_time_cnt = 0;
static int free_time_cnt = 0;

inline void *akpcm_malloc_cb(unsigned long size)
{
	int realSize = size;
	void *pbuf = AK_NULL;

	if (realSize%ONCE_MALLOC_SIZE)
	{
		realSize = (realSize/ONCE_MALLOC_SIZE*ONCE_MALLOC_SIZE) + ONCE_MALLOC_SIZE;
	}
	needTotalMallocSize += size;
	realTotalMallocSize += realSize;
	if (realTotalMallocSize > maxMallocSize)
	{
		maxMallocSize = realTotalMallocSize;
	}

	pbuf = (void *)kmalloc(size, GFP_KERNEL);
	printk("~~~~malloc: %d,  %d,  %d,  %d, addr=0x%x\n", size, realSize, needTotalMallocSize,realTotalMallocSize, pbuf);

	malloc_time_cnt++;
	printk("    malloc_time_cnt = %d\n", malloc_time_cnt);

	//memset(pbuf, 0, size);
	return pbuf;
}

inline void akpcm_free_cb(void * mem)
{
	free_time_cnt++;
	printk("    free_time_cnt = %d\n", free_time_cnt);
	printk("----free, addr=0x%x\n", mem);

	kfree(mem);

	return;
}
#else
inline void* akpcm_malloc_cb(unsigned long size)
{
	return (void *)kmalloc(size, GFP_KERNEL);
}

inline void akpcm_free_cb(void * mem)
{
	kfree(mem);
}
#endif

struct akpcm* get_akpcm_ptr(void)
{
    return &akxpcm;
}

static const struct file_operations pcm_fops[2] =
{
	[0] = {
		.owner   = THIS_MODULE,
		.open    = playback_open,
		.release = playback_close,
		.unlocked_ioctl   = playback_ioctl,
		.write   = playback_write,
		//.read    = capture_read,
	},
	[1] = {
		.owner   = THIS_MODULE,
		.open    = capture_open,
		.release = capture_close,
		.unlocked_ioctl   = capture_ioctl,
		//.write   = playback_write,
		.read    = capture_read,
	},
};

static void akpcm_cdev_setup(int i)
{
	int err = 0;
	dev_t devno = MKDEV(pcm_major, i);
	struct device *dev;
	char name[2][13] = {"akpcm_class0","akpcm_class1"};


	cdev_init(&(akxpcm.cdevice[i]), &pcm_fops[i]);
	akxpcm.cdevice[i].owner = THIS_MODULE;
	akxpcm.cdevice[i].ops = &pcm_fops[i];
	err = cdev_add(&(akxpcm.cdevice[i]), devno, 1);
	if (err) {
		ak_pcm_info_ex("Error %d adding anyka akpcm[%d] cdevice", err, i);
		while(1);
	}
	/* auto mknod device node */
	pcm_class[i] = class_create(THIS_MODULE, name[i]);
	if (IS_ERR(pcm_class[i])) {
		err = PTR_ERR(pcm_class[i]);
		ak_pcm_info_ex("akpcm: can't register class[%d]", i);
		while(1);
	}
	dev = device_create(pcm_class[i], NULL, devno, &akxpcm, "akpcm_cdev" "%d", i);
	if (IS_ERR(dev)) {
		ak_pcm_info_ex("akpcm: can't create akpcm[%d] files", i);
		while(1);
	}
}

static void akpcm_cdev_release(int i)
{
	dev_t devno = MKDEV(pcm_major, i);
	device_destroy(pcm_class[i], devno);
	class_destroy(pcm_class[i]);
	cdev_del(&(akxpcm.cdevice[i]));
}

/* return: successful(=0) or failed(<0) */
static int __devinit akpcm_probe(struct platform_device *devptr)
{
	struct akpcm *pcm;
	/*
	struct resource *Analog_Regs;
	struct resource *I2S_Regs;
	struct resource *ADC2_Regs;
	*/
	struct ak39_codec_platform_data *platfm_data;
	struct device *dev;
	dev_t devno;
	int i;
	int err;

    ak_pcm_func("enter");
	ppb_statu = &akxpcm.outflag;
	ak39_codec_probe(devptr);

	dev = &devptr->dev;
	platfm_data = (struct ak39_codec_platform_data *)devptr->dev.platform_data;

	/*
    Analog_Regs = platform_get_resource_byname(devptr, IORESOURCE_MEM, "ak37pcm_AnalogCtrlRegs");
    if (!Analog_Regs) {
        printk("no memory resource for Analog_Regs\n");
        return -ENXIO;
    }
    I2S_Regs = platform_get_resource_byname(devptr, IORESOURCE_MEM, "ak37pcm_I2SCtrlRegs");
    if (!I2S_Regs) {
        printk("no memory resource for I2S_Regs\n");
        return -ENXIO;
    }
    ADC2_Regs = platform_get_resource_byname(devptr, IORESOURCE_MEM, "ak37pcm_ADC2ModeCfgRegs");
    if (!ADC2_Regs) {
        printk("no memory resource for ADC2_Regs\n");
        return -ENXIO;
    }
	*/

	/* M by panqihe 2014-06-13 */
	if(pcm_major){
		devno = MKDEV(pcm_major, 0);
		err = register_chrdev_region(devno, 2, "pcmchar");
		if (err < 0) return err;
	}else{
		err = alloc_chrdev_region(&devno, 0, 2, "pcmchar");
		if (err < 0) return err;
		pcm_major = MAJOR(devno);
	}
	for (i=0; i<2; i++){
		akpcm_cdev_setup(i);
	}
	pcm = &akxpcm;
	ak_pcm_debug_ex("akpcm initialize OK!");

	/*
	pcm->Analog_Regs = ioremap(Analog_Regs->start, Analog_Regs->end - Analog_Regs->start + 1);
    if (!pcm->Analog_Regs) {
        printk("could not remap Analog_Regs memory");
        goto __out_free_cdev;
    }

    pcm->I2S_Regs = ioremap(I2S_Regs->start, I2S_Regs->end - I2S_Regs->start + 1);
    if (!pcm->I2S_Regs) {
        printk("could not remap I2S_Regs memory");
        goto __out_unmap_AnalogCtrlRegs;
    }

    pcm->ADC2_Regs = ioremap(ADC2_Regs->start, ADC2_Regs->end - ADC2_Regs->start + 1);
    if (!pcm->ADC2_Regs) {
        printk("could not remap ADC2_Regs memory");
        goto __out_unmap_I2SCtrlRegs;
    }

    reg_audio.base0800 = pcm->Analog_Regs;
    reg_audio.base2002E = pcm->I2S_Regs;
    reg_audio.base20072 = pcm->ADC2_Regs;
	*/

	/* init L2 buffer */
	pcm->L2BufID_DAC = BUF_NULL;
	pcm->L2BufID_ADC2 = BUF_NULL;

	/* init default value for mixers */
	pcm->mixer_volume[MIXER_VOL_HP] = DEFAULT_HPVOL;
	akpcm_set_hp_gain(DEFAULT_HPVOL);
	pcm->mixer_volume[MIXER_VOL_LI] = DEFAULT_LINEINVOL;
	ak39_set_linein_gain(DEFAULT_LINEINVOL);
	pcm->mixer_volume[MIXER_VOL_MIC] = DEFAULT_MICVOL;
	akpcm_set_mic_gain(DEFAULT_MICVOL);

#if defined CONFIG_SPKHP_SWITCH_AUTO || defined CONFIG_SPKHP_SWITCH_MIXER
#ifdef CONFIG_SPKHP_SWITCH_AUTO
	INIT_DELAYED_WORK(&pcm->DelayWork, hpDet_wq_work);
#endif

	/* config hp det gpio */
	pcm->HPDet_gpio = platfm_data->hpdet_gpio;
	ak_gpio_set(&(pcm->HPDet_gpio));

	/* set hp det pin irq */
	pcm->HPDet_irq = platfm_data->hpdet_irq;
	pcm->HPDet_on_value = platfm_data->hp_on_value;
	pcm->bfExistPlayDev = platfm_data->boutput_only;
	if (AK_GPIO_OUT_LOW == pcm->HPDet_on_value) {
		pcm->HPon_irqType = IRQ_TYPE_LEVEL_LOW;
		pcm->HPoff_irqType = IRQ_TYPE_LEVEL_HIGH;
	} else if (AK_GPIO_OUT_HIGH == pcm->HPDet_on_value) {
		pcm->HPon_irqType = IRQ_TYPE_LEVEL_HIGH;
		pcm->HPoff_irqType = IRQ_TYPE_LEVEL_LOW;
	} else if (-1 == pcm->HPDet_on_value){
		goto NextSet;
	}

	if (ak_gpio_getpin(pcm->HPDet_gpio.pin) == pcm->HPDet_on_value) {
		/* hp is plugged in */
		if(!(EXIST_OUT_DEV_HP & pcm->bfExistPlayDev)) {
			/* Use SP, if no HP device exist */
			pcm->play_dectect |= SWITCH_DET_SPEAKER;
		} else {
			pcm->play_dectect &= ~SWITCH_DET_SPEAKER;
			pcm->play_dectect |= SWITCH_DET_HEADPHONE;
		}
		irq_set_irq_type(pcm->HPDet_irq, pcm->HPoff_irqType);
		ak_pcm_debug_ex("akpcm probe: hp on");
	} else {
		/* hp is pulled out */
		if(!(EXIST_OUT_DEV_SP & pcm->bfExistPlayDev)) {
			/* Use HP, if no SP device exist */
			pcm->play_dectect |= SWITCH_DET_HEADPHONE;
		} else {
			pcm->play_dectect &= ~SWITCH_DET_HEADPHONE;
			pcm->play_dectect |= SWITCH_DET_SPEAKER;
		}
		irq_set_irq_type(pcm->HPDet_irq, pcm->HPon_irqType);
		ak_pcm_debug_ex("akpcm probe: hp off");
	}

	err = request_irq(pcm->HPDet_irq, HPDet_interrupt, IRQF_DISABLED, devptr->name, pcm);
	if (err) {
		ak_pcm_info_ex("request irq error!");
		goto __out_unmap_ADC2ModeCfgRegs;
	}
#else
	/* No HPDet pin */
	memset(&(pcm->HPDet_gpio), 0, sizeof(struct gpio_info));
#endif

#if defined CONFIG_SPKHP_SWITCH_AUTO || defined CONFIG_SPKHP_SWITCH_MIXER
NextSet:
#endif
	/* config speaker shutdown gpio */
	pcm->SPdown_gpio = platfm_data->spk_down_gpio;
	ak_gpio_set(&(pcm->SPdown_gpio));
	/* HPMute */
	pcm->bIsHPmuteUsed = platfm_data->bIsHPmuteUsed;
	pcm->hpmute_EnValue = platfm_data->hp_mute_enable_value;
	if (AK_GPIO_OUT_HIGH == pcm->hpmute_EnValue) {
		pcm->hpmute_DisValue = AK_GPIO_OUT_LOW;
	} else if (AK_GPIO_OUT_LOW == pcm->hpmute_EnValue) {
		pcm->hpmute_DisValue = AK_GPIO_OUT_HIGH;
	}
	if(pcm->bIsHPmuteUsed){
		/* config hp mute gpio */
		pcm->hpmute_gpio = platfm_data->hpmute_gpio;
		ak_gpio_set(&(pcm->hpmute_gpio));
	}

	/* config linein detecting gpio */
	pcm->linindet_gpio = platfm_data->linindet_gpio;
	if (platfm_data->detect_flag == AKPCM_LINEIN_ALWAY) {
		pcm->capture_dectect |= CAPTURE_DET_LINEIN;
		set_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_LINEIN);
		pcm->cptrdev = CPTRDEV_LI;
		pr_info("akpcm probe: linein alway on\n");
	} else if ((platfm_data->detect_flag != AKPCM_MIC_ALWAY) && (pcm->linindet_gpio.pin >= 0)) {
		ak_gpio_set(&(pcm->linindet_gpio));
		pcm->linindet_irq = platfm_data->linindet_irq;
		if (AK_GPIO_OUT_LOW == pcm->linindet_gpio.value) {
			pcm->lininon_irqType = IRQ_TYPE_LEVEL_LOW;
			pcm->lininoff_irqType = IRQ_TYPE_LEVEL_HIGH;
		} else {
			pcm->lininon_irqType = IRQ_TYPE_LEVEL_HIGH;
			pcm->lininoff_irqType = IRQ_TYPE_LEVEL_LOW;
		}

		if (ak_gpio_getpin(pcm->linindet_gpio.pin) ==
				pcm->linindet_gpio.value) {
			/* linein is plugged in */
			pcm->capture_dectect |= CAPTURE_DET_LINEIN;
			irq_set_irq_type(pcm->linindet_irq, pcm->lininoff_irqType);
			set_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_LINEIN);
			pcm->cptrdev = CPTRDEV_LI;
			pr_info("akpcm probe: linein on\n");
		} else {
			pcm->capture_dectect &= ~CAPTURE_DET_LINEIN;
			irq_set_irq_type(pcm->linindet_irq, pcm->lininon_irqType);
			set_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_MIC);
			pcm->cptrdev = CPTRDEV_MIC;
			pr_info("akpcm probe: linein off\n");
		}
		err = request_irq(pcm->linindet_irq, linindet_interrupt,
				IRQF_DISABLED, "linin-det", pcm);
		if (err) {
			pr_err("request linein irq error!\n");
			goto __out_failed_linin_config;
		}

		INIT_DELAYED_WORK(&pcm->LininWork, LininDet_wq_work);
	} else {
		pcm->capture_dectect &= CAPTURE_DET_LINEIN;
		set_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_MIC);
		pcm->cptrdev = CPTRDEV_MIC;
		pr_info("akpcm probe: mic on\n");
	}

	/* init timer to stop output channel */
	init_timer(&(pcm->timer_stop_output));
	pcm->timer_stop_output.function = playback_stop_hw;
	pcm->timer_stop_output.data = (unsigned long)pcm;
	/* wait queue */
	init_waitqueue_head(&(pcm->play_wq));
	init_waitqueue_head(&(pcm->capt_wq));
	/* running flag */
	clear_bit(IO_BIT_STREAM, &(pcm->outflag));
	clear_bit(IO_BIT_DMA, &(pcm->outflag));
	clear_bit(IO_BIT_STREAM, &(pcm->inflag));
	clear_bit(IO_BIT_DMA, &(pcm->inflag));
	/* dev sm flag */
	clear_bit(STATUS_BIT_OPENED, &(pcm->playback_sm_flag));
	clear_bit(STATUS_BIT_PREPARED, &(pcm->playback_sm_flag));
	clear_bit(STATUS_BIT_OPENED, &(pcm->capture_sm_flag));
	clear_bit(STATUS_BIT_PREPARED, &(pcm->capture_sm_flag));
	pcm->play_rt = NULL;
	pcm->cptr_rt = NULL;
	pcm->dac_opened_count = 0;
	mutex_init(&pcm->io_lock);
	/* aec */
	aec_init(pcm);

	pr_info("akpcm_probe: OK\n\n");
	return 0;

__out_failed_linin_config:
#if defined CONFIG_SPKHP_SWITCH_AUTO || defined CONFIG_SPKHP_SWITCH_MIXER
	free_irq(pcm->HPDet_irq, pcm);
__out_unmap_ADC2ModeCfgRegs:
#endif
	/*iounmap(pcm->ADC2_Regs);*/
	/*__out_unmap_I2SCtrlRegs:*/
	/*iounmap(pcm->I2S_Regs);*/
	/*__out_unmap_AnalogCtrlRegs:*/
	/*iounmap(pcm->Analog_Regs);*/
	/*__out_free_cdev:*/
	for (i=0; i<2; i++){
		akpcm_cdev_release(i);
	}
	unregister_chrdev_region(devno, 2);

	ak_pcm_debug_ex("failed");
	return err;
}

static inline void akpcm_close(struct akpcm *pcm)
{
	/* stop dma opration */
	playback_pause(pcm);
	capture_pause(pcm);

	aec_exit(pcm);

	/* close analog module */
	ak39_codec_dac_close();
	if (pcm->bIsHPmuteUsed) {
		ak_gpio_setpin(pcm->hpmute_gpio.pin, pcm->hpmute_EnValue);
	}
	ak39_set_hp_power(0, pcm->bIsHPmuteUsed == 0);
	if (pcm->bIsHPmuteUsed) {
		ak_gpio_setpin(pcm->hpmute_gpio.pin, pcm->hpmute_DisValue);
	}

	ak39_set_hp_in(0);
	ak39_set_sp_power(pcm->SPdown_gpio.pin, 0);
	ak39_set_adc2_in(0);
	//ak39_set_vcm_ref_power(0);
}

/* return: successful(=0) or failed(<0) */
static int __devexit akpcm_remove(struct platform_device *devptr)
{
    int i = 0;
	struct akpcm *pcm = &akxpcm;

	ak_pcm_func("enter");
	akpcm_close(pcm);
#ifdef CONFIG_SPKHP_SWITCH_AUTO
	cancel_delayed_work_sync(&pcm->DelayWork);
#endif
	del_timer(&(pcm->timer_stop_output));
	free_irq(pcm->HPDet_irq, pcm);

	if (pcm->linindet_gpio.pin >= 0) {
		free_irq(pcm->linindet_irq, pcm);
		cancel_delayed_work_sync(&pcm->LininWork);
	}

	/*iounmap(pcm->Analog_Regs);
	  iounmap(pcm->I2S_Regs);
	  iounmap(pcm->ADC2_Regs);*/

	/* by panqihe */
	for (i=0; i<2; i++) {
		dev_t devno = MKDEV(pcm_major, i);
		//device_destroy(pcm_class[i], devno);
		//class_destroy(pcm_class[i]);
		cdev_del(&(pcm->cdevice[i]));
		unregister_chrdev_region(devno, 1);
	}

	ak39_codec_remove(devptr);

	return 0;
}

#ifdef CONFIG_PM
static void ADC_CfgVoiceW(unsigned short freq, unsigned short ref,
                unsigned short time)
{
	REG32(AK_VA_SYSCTRL + 0x120) &= ~(0x3 << 30);
	REG32(AK_VA_SYSCTRL + 0x120) &= ~(0x7 << 27);
	REG32(AK_VA_SYSCTRL + 0x120) &= ~(0x3 << 25);
	REG32(AK_VA_SYSCTRL + 0x120) |= ((freq & 0x3) << 30);
	REG32(AK_VA_SYSCTRL + 0x120) |= ((ref & 0x7) << 27);
	REG32(AK_VA_SYSCTRL + 0x120) |= ((time & 0x3) << 25);
}

static void pcm_voice_wakeup(unsigned short enable)
{
	if (!enable) {
		REG32(AK_VA_SYSCTRL + 0x120) |= (1 << 15);  //power down voice wake up
		REG32(AK_VA_SYSCTRL + 0xd8) &= ~(1 << 5);   //disable voice wakeup interrupt
		return;
	}

	REG32(AK_VA_SYSCTRL + 0x120) |= (1 << 24);    //poweroff mic_n
	REG32(AK_VA_SYSCTRL + 0x120) |= (1 << 23);    //poweroff mic_p
	REG32(AK_VA_SYSCTRL + 0x120) &= ~(0x7 << 2);  // set no input to adc2
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(0x7 << 12); // set hp mute

	//power off voice wakeup before
	REG32(AK_VA_SYSCTRL + 0x120) |= (1 << 15);    //power down voice wake up
	REG32(AK_VA_SYSCTRL + 0x11c) |= (0x4 << 12);  // set hp from mic
	REG32(AK_VA_SYSCTRL + 0x11c) |= (0x1 << 17);  // HP with 3k to gnd

	mdelay(5);

	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(0x1 << 2);  // no pl_vcm2 with 2k to gnd
	REG32(AK_VA_SYSCTRL + 0x120) &= ~(0x1<<0);    // no Pl_vcm3 with 2k to gnd
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(0x1F << 4); //disable descharge for VCM2
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(1 << 0);
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(1 << 1);  // power on vcm2
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(1 << 23); // vcm3 from refrec1.5
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(1 << 3);  // power on  vcm3
	REG32(AK_VA_SYSCTRL + 0xd8) &= ~(1 << 5);   //disable voice wakeup interrupt
	REG32(AK_VA_SYSCTRL + 0xd8) |= (1 << 3);    //cle ar voice wakeup interrupt status
	REG32(AK_VA_SYSCTRL + 0xd8) &= ~(1 << 3);

	mdelay(500);

	REG32(AK_VA_SYSCTRL + 0x11c) |= (1 << 1);     // power off vcm2
	REG32(AK_VA_SYSCTRL + 0x11c) |= (1 << 3);     // power off vcm3
	REG32(AK_VA_SYSCTRL + 0x11c) |= (1 << 0);
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(0x7 << 12);   // set hp mute
	REG32(AK_VA_SYSCTRL + 0x11c) &= ~(0x1 << 17);   //no HP with 3k to gnd
	REG32(AK_VA_SYSCTRL + 0xd8) &= ~(1 << 1);       //rising

	ADC_CfgVoiceW(0x0, 0x0, 0x0);

	REG32(AK_VA_SYSCTRL + 0xd8) |= (1 << 5);        //enable voice wakeup interrupt
	REG32(AK_VA_SYSCTRL + 0x120) &= ~(1 << 26);     //power on pd_vw, vcm3 from avcc

	mdelay(10);
}

static int akpcm_suspend(struct platform_device *pdev, pm_message_t msg)
{
	struct akpcm *pcm = &akxpcm;

	//ak_pcm_debug_ex("enter");

	akpcm_close(pcm);

	/* set voice wake up function */
	pcm_voice_wakeup(true);

	return 0;
}

static int akpcm_resume(struct platform_device *pdev)
{
	//ak_pcm_debug_ex("enter");

	/* disable voice wakeup */
	pcm_voice_wakeup(false);

	return 0;
}
#else
#define akpcm_suspend   NULL
#define akpcm_resume    NULL
#endif

static struct platform_driver akpcm_driver = {
	.probe      = akpcm_probe,
	.remove     = __devexit_p(akpcm_remove),
	.suspend    = akpcm_suspend,
	.resume     = akpcm_resume,
	.driver     = {
		.name   = "ak39-codec"
	},
};

static int __init akpcm_init(void)
{
    int err = 0;

	ak_pcm_func("enter");
	err = platform_driver_register(&akpcm_driver);
	if (err < 0){
		return err;
	}else{
		return 0;
	}
}

static void __exit akpcm_exit(void)
{
	platform_driver_unregister(&akpcm_driver);
}

module_init(akpcm_init)
module_exit(akpcm_exit)

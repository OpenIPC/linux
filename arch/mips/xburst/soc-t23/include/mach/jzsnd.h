/**
 * jzsnd.h
 *
 **/
#ifndef __MACH_JZSND_H__
#define __MACH_JZSND_H__

#include <linux/list.h>
#include <linux/sound.h>
#include <linux/platform_device.h>

enum snd_codec_route_t {
	SND_ROUTE_NONE = 0,
	/*
	 * clear route
	 */
	SND_ROUTE_ALL_CLEAR,
	SND_ROUTE_REPLAY_CLEAR,
	SND_ROUTE_RECORD_CLEAR,
	/*
	 * replay route
	 */
	SND_ROUTE_REPLAY_ROUTE_START,
	/*internal codec: dacrl to lineout*/
	SND_ROUTE_REPLAY_DACRL_TO_LO = SND_ROUTE_REPLAY_ROUTE_START,
	/*internal codec: dacrl to hprl*/
	SND_ROUTE_REPLAY_DACRL_TO_HPRL,
	/*internal codec: ..*/
	SND_ROUTE_REPLAY_DACRL_TO_ALL,
	/*internal codec replay route end*/
	SND_ROUTE_REPLAY_ROUTE_END = SND_ROUTE_REPLAY_DACRL_TO_ALL,
	/*
	 * record route
	 */
	SND_ROUTE_RECORD_ROUTE_START,
	/*internal codec: mic 1 to adcrl*/
	SND_ROUTE_RECORD_MIC1_AN1 = SND_ROUTE_RECORD_ROUTE_START,
	/*internal codec: mic 2 to adcrl*/
	SND_ROUTE_RECORD_MIC1_SIN_AN2,
	/*internal codec: mic 2 to adcrl*/
	SND_ROUTE_RECORD_MIC2_SIN_AN3,
	/*internal codec: linein 1 to adcrl*/
	SND_ROUTE_RECORD_LINEIN1_DIFF_AN2,
	/*internal codec: linein 2 to adcrl*/
	SND_ROUTE_RECORD_LINEIN2_SIN_AN3,
	/*internal codec: call record mic bypass to hprl*/
	SND_ROUTE_RECORD_LINEIN1_AN2_SIN_TO_ADCL_AND_LINEIN2_AN3_SIN_TO_ADCR,

	SND_ROUTE_CALL_RECORD,
	/*internal codec record route end*/
	//SND_ROUTE_RECORD_ROUTE_END = SND_ROUTE_RECORD_LINEIN1_DIFF_AN1,

	SND_ROUTE_RECORD_ROUTE_END = SND_ROUTE_CALL_RECORD,
	/*
	 * loop route and bypass rount
	 */
	SND_ROUTE_LOOP_ROUTE_START,
	/*internal codec: linein2 bypass to lineout */
	SND_ROUTE_REPLAY_LINEIN2_BYPASS_TO_LINEOUT = SND_ROUTE_LOOP_ROUTE_START,
	/*internal codec: linein2 bypass to hprl */
	SND_ROUTE_REPLAY_LINEIN2_BYPASS_TO_HPRL,
	/*internal codec: mic bypass to hprl*/
	SND_ROUTE_CALL_MIC_BYPASS_TO_HPRL,
	/*internal codec: record mic1 an1 to adclr replay hp stereo*/
	SND_ROUTE_LINE1IN_BYPASS_TO_HP,
	SND_ROUTE_LOOP_MIC1_AN1_LOOP_TO_HP,
	SND_ROUTE_LOOP_ROUTE_END = SND_ROUTE_LOOP_MIC1_AN1_LOOP_TO_HP,

	SND_ROUTE_MIC2_AN3_TO_AD_AND_DA_TO_LO,
	/*
	 *route count
	 */
	SND_ROUTE_COUNT,
#if 0
    SND_ROUTE_NONE,
	ROUTE_ALL_CLEAR,
	ROUTE_REPLAY_CLEAR,
	ROUTE_RECORD_CLEAR,
	RECORD_MIC1_MONO_DIFF_WITH_BIAS,
	RECORD_MIC2_MONO_DIFF_WITH_BIAS,
	RECORD_LININL_AND_LININR,
	REPLAY_HP_STEREO_CAP_LESS,
	REPLAY_HP_STEREO_WITH_CAP,
	REPLAY_HP_STEREO_CAP_LESS_AND_LINEOUT,
	REPLAY_HP_STEREO_WITH_CAP_AND_LINEOUT,
	REPLAY_LINEOUT,
	BYPASS_MIC1_DIFF_WITH_BIAS_TO_HP_CAP_LESS,
	BYPASS_MIC1_DIFF_WITH_BIAS_TO_LINEOUT,
	BYPASS_MIC2_DIFF_WITH_BIAS_TO_HP_CAP_LESS,
	BYPASS_MIC2_DIFF_WITH_BIAS_TO_LINEOUT,
	BYPASS_LINEIN_TO_HP_CAP_LESS,
	BYPASS_LINEIN_TO_LINEOUT,
	RECORD_STEREO_MIC_DIFF_WITH_BIAS_BYPASS_MIXER_MIC2_TO_HP_CAP_LESS,
	RECORD_STEREO_MIC_DIFF_WITH_BIAS_BYPASS_MIXER_MIC2_TO_LINEOUT,
	ROUTE_COUNT
#endif
};

typedef enum {
	KEEP_OR_IGNORE = 0, //default
	STATE_DISABLE,
	STATE_ENABLE,
} audio_gpio_state_t;

struct snd_board_route {
	enum snd_codec_route_t route;
	//val : 0 is mean not set use default volume base
	int replay_volume_base;				//val: +6 ~ +1, 32(0dB), -1 ~ -25 (dB);
	int record_volume_base;				//val: 32(0dB), +4, +8, +16, +20 (dB);
	int record_digital_volume_base;		//val: 32(0dB), +1 ~ +23 (dB);
	int replay_digital_volume_base;		//val: 32(0dB), -1 ~ -31 (dB);
	int bypass_l_volume_base;			//val: +6 ~ +1, 32(0dB), -1 ~ -25 (dB);
	int bypass_r_volume_base;			//val: +6 ~ +1, 32(0dB), -1 ~ -25 (dB);
	audio_gpio_state_t gpio_hp_mute_stat;
	audio_gpio_state_t gpio_spk_en_stat;
	audio_gpio_state_t gpio_handset_en_stat;
	audio_gpio_state_t gpio_buildin_mic_en_stat;
};

struct snd_board_gpio {
	int gpio;			/* -1: gpio is unavailable*/
	int active_level;	/* -1 , invalidation 0 ,low avlible  1,high valible */
};

struct snd_codec_data {
	/* clk */
	int codec_sys_clk;
	int codec_dmic_clk;

	/* volume */
	int replay_volume_base;						/*hp*/
	int record_volume_base;						/*mic*/
	int record_digital_volume_base;				/*adc*/
	int replay_digital_volume_base;				/*dac*/
	int bypass_l_volume_base;					/*call uplink or other use*/
	int bypass_r_volume_base;					/*call downlink or other use*/

	/* default route */
	struct snd_board_route replay_def_route;
	struct snd_board_route record_def_route;
	//struct snd_board_route call_record_def_route;

	/* device <-> route map record*/
	struct snd_board_route record_headset_mic_route;
	struct snd_board_route record_buildin_mic_route;
	struct snd_board_route record_linein_route;
	struct snd_board_route record_linein1_route;
	struct snd_board_route record_linein2_route;
	struct snd_board_route record_linein3_route;
    struct snd_board_route record_linein4_route;

	/* device <-> route map replay*/
	struct snd_board_route replay_headset_route;
	struct snd_board_route replay_speaker_route;
	struct snd_board_route replay_headset_and_speaker_route;
	struct snd_board_route fm_speaker_route;
	struct snd_board_route fm_headset_route;
	struct snd_board_route bt_route;
	struct snd_board_route call_handset_route;
	struct snd_board_route call_headset_route;
	struct snd_board_route call_speaker_route;
	struct snd_board_route record_buildin_incall_route;
	struct snd_board_route record_headset_incall_route;
	struct snd_board_route replay_loop_route;

	/* gpio */
	struct snd_board_gpio gpio_hp_mute;
	struct snd_board_gpio gpio_spk_en;
	struct snd_board_gpio gpio_handset_en;
	struct snd_board_gpio gpio_hp_detect;
	struct snd_board_gpio gpio_mic_detect;
	struct snd_board_gpio gpio_mic_detect_en;
	struct snd_board_gpio gpio_buildin_mic_select;

	/* other */
	int hpsense_active_level;		//no use for the moment ,use menuconfig
    int hook_active_level;   /* -1: no hook detect,
								 0: hook low level available,
                                 1: hook high level available,
                                 2: hook adc enable
								 */
};


/*####################################################*\
* common, used for sound devices
\*####################################################*/
/**
 * device mame and minor
 **/
#define MAX_SND_MINOR  	128
#define SOUND_STEP		16

#define DEV_DSP_NAME	"dsp"

#define SND_DEV_DSP0  	(SND_DEV_DSP + SOUND_STEP * 0)
#define SND_DEV_DSP1  	(SND_DEV_DSP + SOUND_STEP * 1)
#define SND_DEV_DSP2  	(SND_DEV_DSP + SOUND_STEP * 2)
#define SND_DEV_DSP3  	(SND_DEV_DSP + SOUND_STEP * 3)

#define DEV_MIXER_NAME	"mixer"

#define SND_DEV_MIXER0	(SND_DEV_CTL + SOUND_STEP * 0)
#define SND_DEV_MIXER1 	(SND_DEV_CTL + SOUND_STEP * 1)
#define SND_DEV_MIXER2 	(SND_DEV_CTL + SOUND_STEP * 2)
#define SND_DEV_MIXER3 	(SND_DEV_CTL + SOUND_STEP * 3)

#define minor2index(x)	((x) / SOUND_STEP)

/**
 * struct snd_dev_data: sound device platform_data
 * @list: the list is usd to point to next or pre snd_dev_data.
 * @ext_data: used for dsp device, point to
 * (struct dsp_endpoints *) of dsp device.
 * @minor: the minor of the specific device. value will be
 * SND_DEV_DSP(0...), SND_DEV_MIXER(0...) and so on.
 * @is_suspend: indicate if the device is suspend or not.
 * @dev_ioctl: privide by the specific device, used
 * to control the device by command.
 * @init: specific device init.
 * @shutdown: specific device shutdown.
 * @suspend: specific device suspend.
 * @resume: specific device resume.
 **/
struct snd_dev_data {
	struct list_head list;
	struct device *dev;
	void *ext_data;
	/*********for update*********/
	void *priv_data;
	/*********for update*********/
	int minor;
	bool is_suspend;
	long (*dev_ioctl) (unsigned int cmd, unsigned long arg);
	/*********for update**********/
	long (*dev_ioctl_2)(struct snd_dev_data *dev_data, unsigned int cmd, unsigned long arg);
	struct dsp_endpoints * (*get_endpoints)(struct snd_dev_data *dev_data);

	int (*open)(struct snd_dev_data *ddata, struct file *file);
	ssize_t (*read)(struct snd_dev_data *ddata,
			char __user *buffer, size_t count, loff_t *ppos);
	int (*release)(struct snd_dev_data *ddata, struct file *file);

	/************end**************/
	int (*init)(struct platform_device *pdev);
	int (*exit)(struct platform_device *pdev);
	void (*shutdown)(struct platform_device *pdev);
	int (*suspend)(struct platform_device *pdev, pm_message_t state);
	int (*resume)(struct platform_device *pdev);
};

#endif //__MACH_JZSND_H__

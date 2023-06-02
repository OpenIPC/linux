#ifndef _NVP6124_VIDEO_HI_
#define _NVP6124_VIDEO_HI_

#include "common.h"

void nvp6124_common_init(void);
void nvp6124_set_portmode(unsigned char chip, unsigned char portsel, unsigned char portmode, unsigned char chid);
void nvp6114a_set_portmode(unsigned char chip, unsigned char portsel, unsigned char portmode, unsigned char chid);
int nvp6124b_set_portmode(unsigned char chip, unsigned char portsel, unsigned char portmode, unsigned char chid);
int nvp6124_set_chnmode(unsigned char ch, unsigned char vfmt, unsigned char chnmode);
void nvp6124_set_clockmode(unsigned char chip, unsigned char is297MHz);

void nvp6124_each_mode_setting(nvp6124_video_mode *pvmode );
void video_fmt_det(nvp6124_input_videofmt *pvideofmt);
unsigned int nvp6124_getvideoloss(void);
void nvp6124_set_equalizer(unsigned char ch);
void nvp6124b_set_equalizer(unsigned char ch);

void nvp6124_video_set_contrast(unsigned int ch, unsigned int value, unsigned int v_format);
void nvp6124_video_set_brightness(unsigned int ch, unsigned int value, unsigned int v_format);
void nvp6124_video_set_saturation(unsigned int ch, unsigned int value, unsigned int v_format);
void nvp6124_video_set_hue(unsigned int ch, unsigned int value, unsigned int v_format);
void nvp6124_video_set_sharpness(unsigned int ch, unsigned int value);

enum
{
	NVP6124_VI_SD = 0,       	//960x576i(480)
	NVP6124_VI_720P_2530,		//1280x720@25p(30)
	NVP6124_VI_720P_5060,		//1280x720@50p(60)
	NVP6124_VI_1080P_2530,		//1920x1080@25p(30)
	NVP6124_VI_1920H,			//1920x576i(480)
	NVP6124_VI_720H,			//720x576i(480)
	NVP6124_VI_1280H,			//1280x576i(480)
	NVP6124_VI_1440H,			//1440x576i(480)
	NVP6124_VI_960H2EX,   		//3840x576i(480)
	NVP6124_VI_HDEX,			//2560x720@25p(30)
	NVP6124_VI_BUTT
};

typedef enum _nvp6124_outmode_sel
{
	NVP6124_OUTMODE_1MUX_SD = 0,
	NVP6124_OUTMODE_1MUX_HD,
	NVP6124_OUTMODE_1MUX_HD5060,
	NVP6124_OUTMODE_1MUX_FHD,
	NVP6124_OUTMODE_2MUX_SD,
	NVP6124_OUTMODE_2MUX_HD_X,
	NVP6124_OUTMODE_2MUX_HD,
	NVP6124_OUTMODE_2MUX_FHD_X,
	NVP6124_OUTMODE_4MUX_SD,
	NVP6124_OUTMODE_4MUX_HD_X,
	NVP6124_OUTMODE_4MUX_HD,
	NVP6124_OUTMODE_2MUX_FHD,
	NVP6124_OUTMODE_1MUX_HD_X,   //nvp6124b
	NVP6124_OUTMODE_1MUX_FHD_X,
	NVP6124_OUTMODE_4MUX_FHD_X,
	NVP6124_OUTMODE_4MUX_MIX,
	NVP6124_OUTMODE_2MUX_MIX,
	NVP6124_OUTMODE_BUTT
}NVP6124_OUTMODE_SEL;

#endif

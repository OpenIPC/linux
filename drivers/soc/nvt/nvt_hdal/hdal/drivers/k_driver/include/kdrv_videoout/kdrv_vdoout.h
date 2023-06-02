/**
 * @file kdrv_vdoout.h
 * @brief type definition of KDRV API.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _KDRV_VDOOUT_H_
#define _KDRV_VDOOUT_H_

#include "kdrv_type.h"

/**
	define for KDRV_REG_IF when using gpio
*/
#if defined(_BSP_NA51000_)
#define C_GPIO(pin)			(pin)
#define P_GPIO(pin)			(pin + 0x40)
#define S_GPIO(pin)			(pin + 0x80)
#define L_GPIO(pin)			(pin + 0xA0)
#define H_GPIO(pin)			(pin + 0xC0)
#define D_GPIO(pin)			(pin + 0xE0)
#else
#define C_GPIO(pin)			(pin)
#define P_GPIO(pin)			(pin + 0x20)
#define S_GPIO(pin)			(pin + 0x40)
#define L_GPIO(pin)			(pin + 0x60)
#define D_GPIO(pin)			(pin + 0x80)
#define H_GPIO(pin)			(pin + 0xA0)
#define A_GPIO(pin)			(pin + 0xC0)
#endif

#define RE_INIT -2

typedef enum {
	VDDO_DEV_TV_NTSC,      ///< SELECT DISPLAY DEVICE: TV NTSC(not support)
	VDDO_DEV_TV_PAL,       ///< SELECT DISPLAY DEVICE: TV PAL(not support)
	VDDO_DEV_HDMI,         ///< SELECT DISPLAY DEVICE: HDMI(not support)
	VDDO_DEV_PANEL,        ///< SELECT DISPLAY DEVICE: SERIAL
	VDDO_DEV_MIPIDSI,      ///< SELECT DISPLAY DEVICE: MIPI DSI

	VDDO_DEV_MAX,
	VDDO_DEV_NULL,
	ENUM_DUMMY4WORD(KDRV_VDDO_DEV_TYPE)
} KDRV_VDDO_DEV_TYPE;

typedef enum {
	VDDO_HDMI_640X480P60         = 1,    ///< HDMI VIDEO FORMAT IS 640X480 & PROGRESSIVE 60FPS
	VDDO_HDMI_720X480P60         = 2,    ///< HDMI VIDEO FORMAT IS 720X480 & PROGRESSIVE 60FPS & 4:3
	VDDO_HDMI_720X480P60_16X9    = 3,    ///< HDMI VIDEO FORMAT IS 720X480 & PROGRESSIVE 60FPS & 16:9
	VDDO_HDMI_1280X720P60        = 4,    ///< HDMI VIDEO FORMAT IS 1280X720 & PROGRESSIVE 60FPS
	VDDO_HDMI_1920X1080I60       = 5,    ///< HDMI VIDEO FORMAT IS 1920X1080 & INTERLACED 60FPS
	VDDO_HDMI_720X480I60         = 6,    ///< HDMI VIDEO FORMAT IS 720X480 & INTERLACED 60FPS
	VDDO_HDMI_720X480I60_16X9    = 7,    ///< HDMI VIDEO FORMAT IS 720X480 & INTERLACED 60FPS & 16:9
	VDDO_HDMI_720X240P60         = 8,    ///< HDMI VIDEO FORMAT IS 720X240 & PROGRESSIVE 60FPS
	VDDO_HDMI_720X240P60_16X9    = 9,    ///< HDMI VIDEO FORMAT IS 720X240 & PROGRESSIVE 60FPS & 16:9
	VDDO_HDMI_1440X480I60        = 10,   ///< HDMI VIDEO FORMAT IS 1440X480 & INTERLACED 60FPS
	VDDO_HDMI_1440X480I60_16X9   = 11,   ///< HDMI VIDEO FORMAT IS 1440X480 & INTERLACED 60FPS & 16:9
	VDDO_HDMI_1440X240P60        = 12,   ///< HDMI VIDEO FORMAT IS 1440X240 & PROGRESSIVE 60FPS
	VDDO_HDMI_1440X240P60_16X9   = 13,   ///< HDMI VIDEO FORMAT IS 1440X240 & PROGRESSIVE 60FPS & 16:9
	VDDO_HDMI_1440X480P60        = 14,   ///< HDMI VIDEO FORMAT IS 1440X480 & PROGRESSIVE 60FPS
	VDDO_HDMI_1440X480P60_16X9   = 15,   ///< HDMI VIDEO FORMAT IS 1440X480 & PROGRESSIVE 60FPS & 16:9
	VDDO_HDMI_1920X1080P60       = 16,   ///< HDMI VIDEO FORMAT IS 1920X1080 & PROGRESSIVE 60FPS
	VDDO_HDMI_720X576P50         = 17,   ///< HDMI VIDEO FORMAT IS 720X576 & PROGRESSIVE 50FPS
	VDDO_HDMI_720X576P50_16X9    = 18,   ///< HDMI VIDEO FORMAT IS 720X576 & PROGRESSIVE 50FPS & 16:9
	VDDO_HDMI_1280X720P50        = 19,   ///< HDMI VIDEO FORMAT IS 1280X720 & PROGRESSIVE 50FPS
	VDDO_HDMI_1920X1080I50       = 20,   ///< HDMI VIDEO FORMAT IS 1920X1080 & INTERLACED 50FPS
	VDDO_HDMI_720X576I50         = 21,   ///< HDMI VIDEO FORMAT IS 720X576 & INTERLACED 50FPS
	VDDO_HDMI_720X576I50_16X9    = 22,   ///< HDMI VIDEO FORMAT IS 720X576 & INTERLACED 50FPS & 16:9
	VDDO_HDMI_720X288P50         = 23,   ///< HDMI VIDEO FORMAT IS 720X288 & PROGRESSIVE 50FPS
	VDDO_HDMI_720X288P50_16X9    = 24,   ///< HDMI VIDEO FORMAT IS 720X288 & PROGRESSIVE 50FPS & 16:9
	VDDO_HDMI_1440X576I50        = 25,   ///< HDMI VIDEO FORMAT IS 1440X576 & INTERLACED 50FPS
	VDDO_HDMI_1440X576I50_16X9   = 26,   ///< HDMI VIDEO FORMAT IS 1440X576 & INTERLACED 50FPS & 16:9
	VDDO_HDMI_1440X288P50        = 27,   ///< HDMI VIDEO FORMAT IS 1440X288 & PROGRESSIVE 50FPS
	VDDO_HDMI_1440X288P50_16X9   = 28,   ///< HDMI VIDEO FORMAT IS 1440X288 & PROGRESSIVE 50FPS & 16:9
	VDDO_HDMI_1440X576P50        = 29,   ///< HDMI VIDEO FORMAT IS 1440X576 & PROGRESSIVE 50FPS
	VDDO_HDMI_1440X576P50_16X9   = 30,   ///< HDMI VIDEO FORMAT IS 1440X576 & PROGRESSIVE 50FPS & 16:9
	VDDO_HDMI_1920X1080P50       = 31,   ///< HDMI VIDEO FORMAT IS 1920X1080 & PROGRESSIVE 50FPS
	VDDO_HDMI_1920X1080P24       = 32,   ///< HDMI VIDEO FORMAT IS 1920X1080 & PROGRESSIVE 24FPS
	VDDO_HDMI_1920X1080P25       = 33,   ///< HDMI VIDEO FORMAT IS 1920X1080 & PROGRESSIVE 25FPS
	VDDO_HDMI_1920X1080P30       = 34,   ///< HDMI VIDEO FORMAT IS 1920X1080 & PROGRESSIVE 30FPS
	VDDO_HDMI_1920X1080I50_VT1250 = 39,  ///< HDMI VIDEO FORMAT IS 1920X1080 & INTERLACED  50FPS & V-TOTAL IS 1250 LINES
	VDDO_HDMI_1920X1080I100      = 40,   ///< HDMI VIDEO FORMAT IS 1920X1080 & INTERLACED  100FPS
	VDDO_HDMI_1280X720P100       = 41,   ///< HDMI VIDEO FORMAT IS 1280X720  & PROGRESSIVE 100FPS
	VDDO_HDMI_720X576P100        = 42,   ///< HDMI VIDEO FORMAT IS 720X576   & PROGRESSIVE 100FPS
	VDDO_HDMI_720X576P100_16X9   = 43,   ///< HDMI VIDEO FORMAT IS 720X576   & PROGRESSIVE 100FPS & 16:9
	VDDO_HDMI_720X576I100        = 44,   ///< HDMI VIDEO FORMAT IS 720X576  & INTERLACED 100FPS
	VDDO_HDMI_720X576I100_16X9   = 45,   ///< HDMI VIDEO FORMAT IS 720X576  & INTERLACED 100FPS & 16:9
	VDDO_HDMI_1920X1080I120      = 46,   ///< HDMI VIDEO FORMAT IS 1920X1080 & INTERLACED 120FPS
	VDDO_HDMI_1280X720P120       = 47,   ///< HDMI VIDEO FORMAT IS 1280X720  & PROGRESSIVE 120FPS
	VDDO_HDMI_720X480P120        = 48,   ///< HDMI VIDEO FORMAT IS 720X480   & PROGRESSIVE 120FPS
	VDDO_HDMI_720X480P120_16X9   = 49,   ///< HDMI VIDEO FORMAT IS 720X480   & PROGRESSIVE 120FPS & 16:9
	VDDO_HDMI_720X480I120        = 50,   ///< HDMI VIDEO FORMAT IS 720X480  & INTERLACED 120FPS
	VDDO_HDMI_720X480I120_16X9   = 51,   ///< HDMI VIDEO FORMAT IS 720X480  & INTERLACED 120FPS & 16:9
	VDDO_HDMI_720X576P200        = 52,   ///< HDMI VIDEO FORMAT IS 720X576  & PROGRESSIVE 200FPS
	VDDO_HDMI_720X576P200_16X9   = 53,   ///< HDMI VIDEO FORMAT IS 720X576  & PROGRESSIVE 200FPS & 16:9
	VDDO_HDMI_720X576I200        = 54,   ///< HDMI VIDEO FORMAT IS 720X576  & INTERLACED 200FPS
	VDDO_HDMI_720X576I200_16X9   = 55,   ///< HDMI VIDEO FORMAT IS 720X576  & INTERLACED 200FPS & 16:9
	VDDO_HDMI_720X480P240        = 56,   ///< HDMI VIDEO FORMAT IS 720X480  & PROGRESSIVE 240FPS
	VDDO_HDMI_720X480P240_16X9   = 57,   ///< HDMI VIDEO FORMAT IS 720X480  & PROGRESSIVE 240FPS & 16:9
	VDDO_HDMI_720X480I240        = 58,   ///< HDMI VIDEO FORMAT IS 720X480  & INTERLACED 240FPS
	VDDO_HDMI_720X480I240_16X9   = 59,   ///< HDMI VIDEO FORMAT IS 720X480  & INTERLACED 240FPS & 16:9

	VDDO_HDMI_3840X2160P24       = 93,   ///< HDMI VIDEO FORMAT IS 3840X2160 & PROGRESSIVE 24FPS
	VDDO_HDMI_3840X2160P25       = 94,   ///< HDMI VIDEO FORMAT IS 3840X2160 & PROGRESSIVE 25FPS
	VDDO_HDMI_3840X2160P30       = 95,   ///< HDMI VIDEO FORMAT IS 3840X2160 & PROGRESSIVE 30FPS
	VDDO_HDMI_4096X2160P24       = 98,   ///< HDMI VIDEO FORMAT IS 4096X2160 & PROGRESSIVE 24FPS
	VDDO_HDMI_4096X2160P25       = 99,   ///< HDMI VIDEO FORMAT IS 4096X2160 & PROGRESSIVE 25FPS
	VDDO_HDMI_4096X2160P30       = 100,  ///< HDMI VIDEO FORMAT IS 4096X2160 & PROGRESSIVE 30FPS

	VDDO_HDMI14B_3840X2160P30    = 95+128,///< HDMI 1.4B Extended foramt HDMI_VIC=1. Video format is 3840x2160 & Progressive 30fps
	VDDO_HDMI14B_3840X2160P25    = 94+128,///< HDMI 1.4B Extended foramt HDMI_VIC=2. Video format is 3840x2160 & Progressive 30fps
	VDDO_HDMI14B_3840X2160P24    = 93+128,///< HDMI 1.4B Extended foramt HDMI_VIC=3. Video format is 3840x2160 & Progressive 24fps
	VDDO_HDMI14B_4096X2160P24    = 98+128,///< HDMI 1.4B Extended foramt HDMI_VIC=4. Video format is 4096x2160 & Progressive 24fps

	// Non-std HDMI formats
	VDDO_HDMI_1024X768P60        = 35+128,///< HDMI Video format is 1024X768  & Progressive 60fps
	VDDO_HDMI_1280X1024P60       = 36+128,///< HDMI Video format is 1280X1024  & Progressive 60fps
	VDDO_HDMI_1600X1200P60       = 37+128,///< HDMI Video format is 1600X1200  & Progressive 60fps
	VDDO_HDMI_2560X1440P60       = 38+128,///< HDMI Video format is 2560X1440  & Progressive 60fps

    VDDO_HDMI_NO_CHANGE        = 0XFF, ///< HDMI VIDEO FORMAT USE CURRENT SETTING

	ENUM_DUMMY4WORD(KDRV_VDDO_HDMI)
} KDRV_VDDO_HDMI;

typedef enum {
	VDDO_HDMI_AUDIO32KHZ,                ///< HDMI Audio sampling rate uses 32KHz.
	VDDO_HDMI_AUDIO44_1KHZ,              ///< HDMI Audio sampling rate uses 44.1KHz.
	VDDO_HDMI_AUDIO48KHZ,                ///< HDMI Audio sampling rate uses 48KHz.

	VDDO_HDMI_AUDIO_NO_CHANGE  = 0xFF,   ///< Reserve the curretn HDMI audio sampling rate.

	ENUM_DUMMY4WORD(KDRV_VDDO_HDMI_AUDIOFMT)
} KDRV_VDDO_HDMI_AUDIOFMT;

typedef struct {
	KDRV_VDDO_HDMI  video_id;        ///< The Video Format which TV supports.
	UINT32          vdo_3d_ability;   ///< The 3D format supported for this video ID. The format uses the "bit-wise OR" of HDMI_3DFMT.
} KDRV_VDDO_HDMI_VDOABI;

typedef enum {
	VDDO_DISPDEV_LCDMODE_RGB_SERIAL           =    0,     ///< LCD MODE is RGB Serial or UPS051
	VDDO_DISPDEV_LCDMODE_RGB_PARALL           =    1,     ///< LCD MODE is RGB Parallel (888)
	VDDO_DISPDEV_LCDMODE_YUV640               =    2,     ///< LCD MODE is YUV640
	VDDO_DISPDEV_LCDMODE_YUV720               =    3,     ///< LCD MODE is YUV720
	VDDO_DISPDEV_LCDMODE_RGBD360              =    4,     ///< LCD MODE is RGB Dummy 360
	VDDO_DISPDEV_LCDMODE_RGBD320              =    5,     ///< LCD MODE is RGB Dummy 320
	VDDO_DISPDEV_LCDMODE_RGB_THROUGH          =    6,     ///< LCD MODE is RGB through mode
	VDDO_DISPDEV_LCDMODE_CCIR601              =    7,     ///< LCD MODE is CCIR601
	VDDO_DISPDEV_LCDMODE_CCIR656              =    8,     ///< LCD MODE is CCIR656
	VDDO_DISPDEV_LCDMODE_RGB_PARALL666        =    9,     ///< LCD MODE is RGB Parallel 666
	VDDO_DISPDEV_LCDMODE_RGB_PARALL565        =   10,     ///< LCD MODE is RGB Parallel 565
	VDDO_DISPDEV_LCDMODE_RGB_PARALL_DELTA     =   11,     ///< LCD MODE is RGB Parallel Delta
	VDDO_DISPDEV_LCDMODE_MIPI                 =   12,     ///< LCD MODE is MIPI Display

	VDDO_DISPDEV_LCDMODE_MI_OFFSET            = 32,       ///< Memory LCD MODE offset
	VDDO_DISPDEV_LCDMODE_MI_FMT0              = 32 + 0,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT1              = 32 + 1,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT2              = 32 + 2,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT3              = 32 + 3,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT4              = 32 + 4,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT5              = 32 + 5,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT6              = 32 + 6,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT7              = 32 + 7,   ///< LCD MODE is Memory(Parallel Interface) 9bits
	VDDO_DISPDEV_LCDMODE_MI_FMT8              = 32 + 8,   ///< LCD MODE is Memory(Parallel Interface) 16bits
	VDDO_DISPDEV_LCDMODE_MI_FMT9              = 32 + 9,   ///< LCD MODE is Memory(Parallel Interface) 16bits
	VDDO_DISPDEV_LCDMODE_MI_FMT10             = 32 + 10,  ///< LCD MODE is Memory(Parallel Interface) 18bits
	VDDO_DISPDEV_LCDMODE_MI_FMT11             = 32 + 11,  ///< LCD MODE is Memory(Parallel Interface)  8bits
	VDDO_DISPDEV_LCDMODE_MI_FMT12             = 32 + 12,  ///< LCD MODE is Memory(Parallel Interface) 16bits
	VDDO_DISPDEV_LCDMODE_MI_FMT13             = 32 + 13,  ///< LCD MODE is Memory(Parallel Interface) 16bits
	VDDO_DISPDEV_LCDMODE_MI_FMT14             = 32 + 14,  ///< LCD MODE is Memory(Parallel Interface) 16bits
	VDDO_DISPDEV_LCDMODE_MI_SERIAL_BI         = 32 + 20,  ///< LCD MODE is Serial Interface bi-direction
	VDDO_DISPDEV_LCDMODE_MI_SERIAL_SEP        = 32 + 21,  ///< LCD MODE is Serial Interface separation

	VDDO_DISPDEV_LCDMODE_NO_CHANGE            = 0xFF,     ///< LCD MODE use current setting

	ENUM_DUMMY4WORD(KDRV_VDDO_DISPDEV_LCDMODE)
} KDRV_VDDO_DISPDEV_LCDMODE;

typedef enum {
	VDDO_DISPLAYER_OSD1 = 0x02,                  ///< Select display layer osd 1
	VDDO_DISPLAYER_VDO1 = 0x08,                  ///< Select display layer video 1
	VDDO_DISPLAYER_VDO2 = 0x10,                  ///< Select display layer video 2
	VDDO_DISPLAYER_FD   = 0x20,                  ///< Select display layer FD

	ENUM_DUMMY4WORD(KDRV_VDDO_DISPLAYER)
} KDRV_VDDO_DISPLAYER;

typedef enum {
	VDDO_DISPBUFFORMAT_PAL8 = 3,                 ///< Buffer uses format 8bit palette.

	VDDO_DISPBUFFORMAT_ARGB8565 = 8,             ///< Buffer uses format ARGB 8565.

	VDDO_DISPBUFFORMAT_YUV422PACK = 9,           ///< Buffer uses format YUV422.
	VDDO_DISPBUFFORMAT_YUV420PACK = 10,           ///< Buffer uses format YUV420.

	VDDO_DISPBUFFORMAT_ARGB8888 = 11,             ///< Buffer uses format ARGB 8888.

	VDDO_DISPBUFFORMAT_ARGB4444 = 12,             ///< Buffer uses format ARGB 4444.
	VDDO_DISPBUFFORMAT_ARGB1555 = 13,             ///< Buffer uses format ARGB 1555.

	ENUM_DUMMY4WORD(KDRV_VDDO_DISPBUFFORMAT)
} KDRV_VDDO_DISPBUFFORMAT;

typedef enum {
	VDDO_DISPOUTDIR_NORMAL       = 0x0,          ///< Normal output direction
	VDDO_DISPOUTDIR_HRZ_FLIP     = 0x1,          ///< Horizontal flip
	VDDO_DISPOUTDIR_VTC_FLIP     = 0x2,          ///< Vertical flip
	VDDO_DISPOUTDIR_ROT_180      = 0x3,          ///< Rotate 180 degree

	ENUM_DUMMY4WORD(KDRV_VDDO_DISPOUTDIR)
} KDRV_VDDO_DISPOUTDIR;

typedef enum {
	VDDO_DISPCTRL_BLEND_TYPE_NOALPHA,
	VDDO_DISPCTRL_BLEND_TYPE_GLOBAL,
	VDDO_DISPCTRL_BLEND_TYPE_GLOBAL_BACK,
	VDDO_DISPCTRL_BLEND_TYPE_SOURCE,	///< only support in osd
	VDDO_DISPCTRL_BLEND_TYPE_SOURCE_BACK,

	VDDO_DISPCTRL_BLEND_SEL_DEFAULT = VDDO_DISPCTRL_BLEND_TYPE_NOALPHA,

	ENUM_DUMMY4WORD(KDRV_VDDO_DISPBLENDTYPE)
} KDRV_VDDO_DISPBLENDTYPE;

typedef enum {
	VDDO_DISPVDOCK_CMP_V2 = 0x0,                ///< Colorkey compare with video2
	VDDO_DISPVDOCK_CMP_V1 = 0x1,                ///< Colorkey compare with video1
	ENUM_DUMMY4WORD(KDRV_VDDO_DISPVDOCKCMPSOURCE)
} KDRV_VDDO_DISPVDOCKCMPSOURCE;

typedef enum {
	VDDO_DISPCK_OP_OFF = 0x0,                 ///< For VDO: vdo color key enable =0 (videock = video1 or video2; refer to setting of CKsrc). For OSD: osd color key enable = 0
	VDDO_DISPCK_OP_YSMALLKEY = 0x1,           ///< ck = (Y < YKEY) ?    only for video layer
	VDDO_DISPCK_OP_YEQUKEY = 0x2,             ///< ck = (Y == YKEY && CB == CBKEY && CR == CRKEY) ?
	VDDO_DISPCK_OP_YBIGKEY = 0x3,             ///< ck = (Y > YKEY) ?    only for video layer
	ENUM_DUMMY4WORD(KDRV_VDDO_DISPCKOP)
} KDRV_VDDO_DISPCKOP;

typedef enum {
	VDDO_DISPFD_NUM0  = 0x00000001,             ///< Number 0
	VDDO_DISPFD_NUM1  = 0x00000002,             ///< Number 1
	VDDO_DISPFD_NUM2  = 0x00000004,             ///< Number 2
	VDDO_DISPFD_NUM3  = 0x00000008,             ///< Number 3
	VDDO_DISPFD_NUM4  = 0x00000010,             ///< Number 4
	VDDO_DISPFD_NUM5  = 0x00000020,             ///< Number 5
	VDDO_DISPFD_NUM6  = 0x00000040,             ///< Number 6
	VDDO_DISPFD_NUM7  = 0x00000080,             ///< Number 7
	VDDO_DISPFD_NUM8  = 0x00000100,             ///< Number 8
	VDDO_DISPFD_NUM9  = 0x00000200,             ///< Number 9
	VDDO_DISPFD_NUM10 = 0x00000400,             ///< Number 10
	VDDO_DISPFD_NUM11 = 0x00000800,             ///< Number 11
	VDDO_DISPFD_NUM12 = 0x00001000,             ///< Number 12
	VDDO_DISPFD_NUM13 = 0x00002000,             ///< Number 13
	VDDO_DISPFD_NUM14 = 0x00004000,             ///< Number 14
	VDDO_DISPFD_NUM15 = 0x00008000,             ///< Number 15
	ENUM_DUMMY4WORD(KDRV_VDDO_DISPFDNUM)
} KDRV_VDDO_DISPFDNUM;

typedef enum {
	VDDO_DISPDEV_LCD_ROTATE_NONE,    ///< Config Panel Register to Set LCD None of Rotation.
	VDDO_DISPDEV_LCD_ROTATE_180,     ///< Config Panel Register to Set LCD Rotation 180 degrees.

	ENUM_DUMMY4WORD(KDRV_VDDO_DISPDEV_LCD_ROTATE)
} KDRV_VDDO_DISPDEV_LCD_ROTATE;

typedef enum {
	VDDO_DISPDEV_LCDCTRL_SIF,        ///< LCD Control Interface use SIF engine. User must assign SIF channel number "ui_sif_ch".
	VDDO_DISPDEV_LCDCTRL_GPIO,       ///< LCD Control Interface use SW controlled GPIO pins. User must assign SIF pins: "ui_gpio_sen/ui_gpio_clk/ui_gpio_data".
	VDDO_DISPDEV_LCDCTRL_MI,         ///< LCD control Interface use Embedded MI engine.

	VDDO_DISPDEV_LCDCTRL_NONE,       ///< Default value as None.
	ENUM_DUMMY4WORD(KDRV_VDDO_DISPDEV_LCDCTRL)
} KDRV_VDDO_DISPDEV_LCDCTRL;

typedef enum {
	VDDO_SIF_CH0,                ///< SIF channel 0
	VDDO_SIF_CH1,                ///< SIF channel 1
	VDDO_SIF_CH2,                ///< SIF channel 2
	VDDO_SIF_CH3,                ///< SIF channel 3
	VDDO_SIF_CH4,                ///< SIF channel 4
	VDDO_SIF_CH5,                ///< SIF channel 5
	VDDO_SIF_CH6,                ///< SIF channel 6
	VDDO_SIF_CH7,                ///< SIF channel 7

	VDDO_SIF_TOTAL_CHANNEL,      ///< Total SIF channel number
	ENUM_DUMMY4WORD(KDRV_VDDO_SIF_CH)
} KDRV_VDDO_SIF_CH;

/*
 * structured passed to callback's 2nd parameter "user_data"
 */
typedef struct _KDRV_VDDO_EVENT_CB_INFO {
	UINT32       handle;     ///< handle feed by kdrv_vddo_trigger()
	UINT32       timestamp;  ///< timestamp when job complete
} KDRV_VDDO_EVENT_CB_INFO;

/**
    VDDO Display Control Parameter Definition

    This definition is used in kdrv_vddo_set.
*/
typedef struct {
	union {
		/**
		    Set Display Engine Enable/Disable

		    This structure is used for display control operation code "VDDO_DISPCTRL_ENABLE"
		    to set the display enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.KDRV_VDDO_ENABLE.en     = TRUE;
		\n  kdrv_vddo_set(handel, DISPCTRL_SET_ENABLE, &disp_ctrl);
		*/
		struct { // VDDO_DISPCTRL_ENABLE
			BOOL            en;            ///< Display Enable/Disable
		} KDRV_VDDO_ENABLE;

		/**
		    Set Display Engine layers enable/disable

		    This structure is used for display control operation code "VDDO_DISPCTRL_ALL_LYR_EN"
		    to set the display layers enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.KDRV_VDDO_ALL_LYR_EN.en     = TRUE;
		\n  disp_ctrl.SEL.KDRV_VDDO_ALL_LYR_EN.disp_lyr = VDDO_DISPLAYER_VDO1|VDDO_DISPLAYER_OSD1;    // Enable the specified layers.
		\n  kdrv_vddo_set(handel, VDDO_DISPCTRL_ALL_LYR_EN, &disp_ctrl);
		\n  Or Using like this:
		\n  DispCtrl.SEL.SET_ALL_LYR_EN.bEn     = FALSE;
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { //VDDO_DISPCTRL_ALL_LYR_EN
			BOOL                      en;            ///< For ALL display layers ON/OFF. TRUE: Enable. FALSE:Disable.
			KDRV_VDDO_DISPLAYER       disp_lyr;      ///< If bEn = TRUE, the specified display layers would be enabled.
			///< If bEn = FALSE, "ALL" the display layers would be disabled. This parameter would be useless.
		} KDRV_VDDO_ALL_LYR_EN;

		/**
		    Set the Display default background color

		    This structure is used for display control operation code "VDDO_DISPCTRL_BACKGROUND"
		    to set the display default background color.
		    If none of the display layer is enabled, the default color on the display screen is the background color.
		    This structure shuld be used like this:
		\n  disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_y     = 0x00;
		\n  disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cb    = 0x80;
		\n  disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cr    = 0x80;
		\n  kdrv_vddo_set(handel, VDDO_DISPCTRL_SET_BACKGROUND, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { // VDDO_DISPCTRL_BACKGROUND
			UINT32          color_y;       ///< Background color  Y value. Must be within 0~0xFF.
			UINT32          color_cb;      ///< Background color CB value. Must be within 0~0xFF.
			UINT32          color_cr;      ///< Background color CR value. Must be within 0~0xFF.
		} KDRV_VDDO_BACKGROUND;
		/**
		    Set Display DAM Engine ABORT Enable/Disable

		    This structure is used for display control operation code "VDDO_DISPCTRL_DMA_ABORT"
		    to set the display dma engine abort enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.KDRV_VDDO_DMAABORT.en     = TRUE;
		\n  kdrv_vddo_set(handel, VDDO_DISPCTRL_SET_DMA_ABORT, &disp_ctrl);
		*/
		struct { // VDDO_DISPCTRL_DMA_ABORT
			BOOL            en;            ///< dmaa bort Enable/Disable
		} KDRV_VDDO_DMAABORT;
	} SEL;
} KDRV_VDDO_DISPCTRL_PARAM;

/**
    Display Device Control Parameter Definition

    This definition is used as the second operand in the devCtrl API to specify the display device control parameters
    for each of the display device control opeation.
*/
typedef struct {
	union {
		/**
		    Open Display Device as specified DevID

		    This structure is used for display device control operation code "VDDO_DISPDEV_OPEN_DEVICE"
		    to open display device. This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_VDDO_OPEN_DEVICE.dev_id    = VDDO_DEV_PANEL_SERIAL;
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_OPEN_DEVICE, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_OPEN_DEVICE
			KDRV_VDDO_DEV_TYPE          dev_id;      ///< Open specified display device.
			BOOL						user_data_en;
			UINT32						user_data;
		} KDRV_VDDO_OPEN_DEVICE;


		/**
		    Set HDMI display Video and Audio Mode

		    This structure is used for display device control operation code "VDDO_DISPDEV_HDMIMODE"
		    to configure the HDMI Video/Audio mode when HDMI cable connected.
		    If the TV do not support the selected video ID, the HDMI basic format would be displayed.(480P60)
		    This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_VDDO_HDMIMODE.video_id    = VDDO_HDMI_1280X720P60;
		\n  disp_dev.SEL.KDRV_VDDO_HDMIMODE.audio_id    = VDDO_HDMI_AUDIO32KHZ;
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_HDMIMODE, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_HDMIMODE
			KDRV_VDDO_HDMI            video_id;        ///< Select HDMI default Video Mode ID
			KDRV_VDDO_HDMI_AUDIOFMT   audio_id;        ///< Select HDMI default Audio Mode ID
		} KDRV_VDDO_HDMIMODE;

		/**
		    Set LCD Mode

		    This structure is used for display device control operation code "VDDO_DISPDEV_LCDMODE"
		    to configure the LCD mode when change LCD mode
		    This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_VDDO_LCDMODE.mode    = VDDO_DISPDEV_LCDMODE_YUV640;
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_LCDMODE, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_LCDMODE
			KDRV_VDDO_DISPDEV_LCDMODE  mode;      ///< Select LCD mode
		} KDRV_VDDO_LCDMODE;


		/**
		    Set Display Device Rotation

		    This structure is used for display device control operation code "VDDO_DISPDEV_ROTATE"
		    to configure the display device(LCD) rotation configuration.
		    This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_VDDO_ROTATE.rot    = VDDO_DISPDEV_LCD_ROTATE_180;
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_ROTATE, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_ROTATE
			KDRV_VDDO_DISPDEV_LCD_ROTATE    rot;        ///< Select display device rotate
		} KDRV_VDDO_ROTATE;

		/**
		    Set PowerDown

		    This structure is used for display device control operation code "VDDO_DISPDEV_POWERDOWN"
		    to set the Device powerdown or not.
		    This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_VDDO_POWERDOWN.power_down    = (project defined levels);
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_POWERDOWN, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_POWERDOWN
			BOOL        power_down;
		} KDRV_VDDO_POWERDOWN;

		/**
		    Get the Display optimal buffer size

		    This structure is used for display device control operation code "VDDO_DISPDEV_DISPSIZE"
		    to get the current active display's optimal buffer size. This structure shuld be used like this:
		\n  kdrv_vddo_get(handel, VDDO_DISPDEV_DISPSIZE, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_DISPSIZE
			KDRV_VDDO_DEV_TYPE      dev_id;          ///< Get specified display device.
			UINT32          buf_width;     ///< Get display optimal buffer width.
			UINT32          buf_height;    ///< Get  display optimal buffer height.
			UINT32			win_width;     ///< Get  display optimal window width.
			UINT32          win_height;    ///< Get  display optimal window height.
		} KDRV_VDDO_DISPSIZE;

		/**
		   Set output to dram parameter

		   This structure is used for output IDE internal data output to dram via VDO2 channel.
		   So that VDO2 can not active at the same time
		   This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.y_addr     = (project defined levels);
		\n  disp_dev.SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.uv_addr    = (project defined levels);
		\n  disp_dev.SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.output_width    = 640;
		\n  disp_dev.SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.output_height  = 480;
		\n  disp_dev.SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.output_line_ofs = 640;
		\n  disp_dev.SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.is_yuv422= TRUE(TRUE for yuv422, FALSE for yuv420);
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_OUTPUT_DRAM, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_OUTPUT_DRAM
			UINT32      y_addr;                  ///< Output Y address
			UINT32      uv_addr;                 ///< Output UV address
			UINT32      output_width;            ///< Output width
			UINT32      output_height;           ///< Output height
			UINT32      output_line_ofs;         ///< Output line offset(unit:byte)
			BOOL        is_yuv422;               ///< output format (0:420/1:422)
		} KDRV_VDDO_OUTPUT_DRAM_PAR;


		/**
		   Set TV Full screen

		   This structure is used for display device control operation code "VDDO_DISPDEV_TVFULL"
		   to set the TV's parameters.
		   This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_VDDO_TVFULL.en_full  = TRUE or FALSE;
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_TVFULL, &disp_dev);
		*/
		struct { // VDDO_DISPDEV_TVFULL
			BOOL        en_full;               ///< Enable/Disable TV Full screen
		} KDRV_VDDO_TVFULL;


		/**
		    Set LCD Device control interface

		    This structure is used for display device control operation code "VDDO_DISPDEV_REG_IF"
		    to specify the LCD control interface. This structure shuld be used like this:
		\n  disp_dev.SEL.KDRV_REG_IF.lcd_ctrl    = VDDO_DISPDEV_LCDCTRL_SIF;
		\n  disp_dev.SEL.KDRV_REG_IF.ui_sif_ch    = SIF_CH0;
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_REG_IF, &disp_dev);
		\n  Or Using like this:
		\n  disp_dev.SEL.KDRV_REG_IF.lcd_ctrl    = VDDO_DISPDEV_LCDCTRL_GPIO;
		\n  disp_dev.SEL.KDRV_REG_IF.ui_gpio_sen  = P_GPIO(0);
		\n  disp_dev.SEL.KDRV_REG_IF.ui_gpio_clk  = P_GPIO(1);
		\n  disp_dev.SEL.KDRV_REG_IF.ui_gpio_data = P_GPIO(2);
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_REG_IF, &disp_dev);
		\n  Or Using like this:
		\n  disp_dev.SEL.SET_REG_IF.lcd_ctrl    = VDDO_DISPDEV_LCDCTRL_MI;
		\n  kdrv_vddo_set(handel, VDDO_DISPDEV_REG_IF, &disp_dev);
		*/
		struct { // DISPDEV_SET_REG_IF
			KDRV_VDDO_DISPDEV_LCDCTRL lcd_ctrl;        ///< select the LCD Control Interface.

			KDRV_VDDO_SIF_CH ui_sif_ch;        ///< If lcd_ctrl=VDDO_DISPDEV_LCDCTRL_SIF, this field is the assigned SIF channel number.
			UINT32           ui_gpio_sen;      ///< If lcd_ctrl=VDDO_DISPDEV_LCDCTRL_GPIO,this field is the assigned GPIO pin name for SIF SEN.
			UINT32           ui_gpio_clk;      ///< If lcd_ctrl=VDDO_DISPDEV_LCDCTRL_GPIO,this field is the assigned GPIO pin name for SIF CLK.
			UINT32           ui_gpio_data;     ///< If lcd_ctrl=VDDO_DISPDEV_LCDCTRL_GPIO,this field is the assigned GPIO pin name for SIF DATA.
		} KDRV_VDDO_REG_IF;

        /**
            get hdmi dev display ability
            This structure is used for display device control operation code "VDDO_DISPDEV_HDMI_ABI"
        */
        struct {
        	KDRV_VDDO_HDMI_VDOABI  video_abi[32]; ///< The structure of the supported video format and its 3D ability.
        	UINT32          len;              ///< Number of video formats supported by the TV/Display.
        } KDRV_VDDO_HDMI_ABILITY;

	} SEL;
} KDRV_VDDO_DISPDEV_PARAM;

/**
    Display Layer Control Parameter Definition

    This definition is used as the third operand in the dispLyrCtrl API to specify the display layer control parameters
    for each of the layer control opeation.
*/
typedef struct {
	union {
		/**
		    Set Specified Display Layer Enable/Disable

		    This structure is used for layer operation code "VDDO_DISPLAYER_ENABLE" to set the specified "DISPLAYER" enable/disable.
		    This structure shuld be used like this:
		\n  disp_lyr.SEL.KDRV_VDDO_ENABLE.en = TRUE;  //Or FALSE
		\n  disp_lyr.SEL.KDRV_VDDO_ENABLE.layer = VDDO_DISPLAYER_VDO1;
		\n  kdrv_vddo_set(handel, VDDO_DISPLAYER_ENABLE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { // VDDO_DISPLAYER_ENABLE
			BOOL                   en;            ///< Set specified display layer ON/OFF. TRUE: Enable. FALSE:Disable.
			KDRV_VDDO_DISPFDNUM    fd_num;        ///< for layer == VDDO_DISPLAYER_FD, you can select which FD to enable.
			KDRV_VDDO_DISPLAYER    layer;
		} KDRV_VDDO_ENABLE;

		/**
		    Set Specified Display VIDEO Layer Buffer Address

		    This structure is used for layer operation code "VDDO_DISPLAYER_BUFADDR"
		    to set the specified "DISPLAYER" buffer address.
		    This structure shuld be used like this:
		\n  disp_lyr.SEL.KDRV_VDDO_VDOBUFADDR.addr_y     = 0x300000;
		\n  disp_lyr.SEL.KDRV_VDDO_VDOBUFADDR.addr_cb   = 0x400000;
		\n  disp_lyr.SEL.KDRV_VDDO_VDOBUFADDR.layer = VDDO_DISPLAYER_VDO1;
		\n  kdrv_vddo_set(handel, VDDO_DISPLAYER_VDOBUFADDR, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { // VDDO_DISPLAYER_BUFADDR
			UINT32                 addr_y;       ///< Buffer Starting Address for  Y(or RGB) plane
			UINT32                 addr_cbcr;    ///< Buffer Starting Address for  CbCR(or A)  plane
			KDRV_VDDO_DISPLAYER    layer;
		} KDRV_VDDO_BUFADDR;


		/**
		    Set Specified Display Layer Buffer output direction

		    This structure is used for layer operation code "VDDO_DISPLAYER_OUTDIR"
		    to set the specified "DISPLAYER" buffer output direction. This structure shuld be used like this:
		\n  disp_lyr.SEL.KDRV_VDDO_OUTDIR.buf_out_dir  = DISPOUTDIR_NORMAL;
		\n  disp_lyr.SEL.KDRV_VDDO_OUTDIR.layer = VDDO_DISPLAYER_VDO1;
		\n  kdrv_vddo_set(handel, VDDO_DISPLAYER_OUTDIR, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { // VDDO_DISPLAYER_OUTDIR
			KDRV_VDDO_DISPOUTDIR   buf_out_dir;      ///< Select buffer output as Horizontal/Vertical-Flip, or Rotation 90/180/270 degrees.
			KDRV_VDDO_DISPLAYER    layer;
		} KDRV_VDDO_OUTDIR;

		/**
		    Set Specified Display OSD Layer palette table

		    This structure is used for layer operation code "VDDO_DISPLAYER_PALETTE"
		    to set the specified "DISPLAYER" palette table. This structure shuld be used like this:
		\n  disp_lyr.SEL.KDRV_VDDO_PALETTE.start     = 0;
		\n  disp_lyr.SEL.KDRV_VDDO_PALETTE.number    = 256;
		\n  disp_lyr.SEL.KDRV_VDDO_PALETTE.pale_entry  = (Address of palette table);
		\n  kdrv_vddo_set(handel, VDDO_DISPLAYER_PALETTE, &disp_lyr);
		*/
		struct { // VDDO_DISPLAYER_PALETTE
			UINT32          start;        ///< Start entry id. Valid range from 0~255.
			UINT32          number;       ///< Total number of palette entry to be set. Valid range from 1~256
			UINT32          *p_pale_entry;  ///< p_pale_entry[7~0]:      B  value of the palette.
			///< p_pale_entry[15~8]:     G  value of the palette.
			///< p_pale_entry[23~16]:    R  value of the palette.
			///< p_pale_entry[31~24]:    Alpha value of the palette.
		} KDRV_VDDO_PALETTE;

		struct { // VDDO_DISPLAYER_BLEND
			KDRV_VDDO_DISPBLENDTYPE       type;
			UINT32                        global_alpha;
			BOOL                          global_alpha5;
			KDRV_VDDO_DISPLAYER           layer;
		} KDRV_VDDO_BLEND;

		struct { // VDDO_DISPLAYER_VDOCOLORKEY_SRC
			KDRV_VDDO_DISPVDOCKCMPSOURCE  colorkey_src;          ///< color key source select
		} KDRV_VDDO_VDOCOLORKEY_SRC;

		struct { // VDDO_DISPLAYER_COLORKEY
			KDRV_VDDO_DISPCKOP            colorkey_op;           ///< color key operation
			UINT32                        key_y;          ///< color key color Y
			UINT32                        key_cb;         ///< color key color Cb
			UINT32                        key_cr;         ///< color key color Cr
			KDRV_VDDO_DISPLAYER           layer;
		} KDRV_VDDO_COLORKEY;

		/**
		    Set FD Layer size

		    This structure is used for layer operation code "VDDO_DISPLAYER_FD_CONFIG"
		    to set the specified "FD" size. This structure shuld be used like this:
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_num = DISPFD_NUM0;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_x = 0;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_y = 0;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_w = 40;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_h = 40;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_bord_w = 4;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_bord_h = 4;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_y = 0;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_cb = 128;
		\n  disp_lyr.SEL.KDRV_VDDO_FDSIZE.fd_cr = 128;
		\n  kdrv_vddo_set(handel, VDDO_DISPLAYER_FD_CONFIG, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { // VDDO_DISPLAYER_FD_CONFIG
			KDRV_VDDO_DISPFDNUM        fd_num;        ///< FD channel number
			UINT32                     fd_x;         ///< FD channel position X
			UINT32                     fd_y;         ///< FD channel position Y
			UINT32                     fd_w;         ///< FD channel width
			UINT32                     fd_h;         ///< FD channel height
			UINT32                     fd_bord_w;    ///< FD channel border width
			UINT32                     fd_bord_h;    ///< FD channel border heigh

			UINT32                     fd_color_y;       ///< FD channel y color
			UINT32                     fd_color_cb;      ///< FD channel cb color
			UINT32                     fd_color_cr;      ///< FD channel cr color
		} KDRV_VDDO_FDSIZE;

		/**
		    Set Specified Display Layer buffer source position.

		    This structure is used for layer operation code "VDDO_DISPLAYER_OP_BUFXY"
		    to set the specified "DISPLAYER" buffer source position. This structure shuld be used like this:
		\n  disp_lyr.SEL.KDRV_VDDO_BUFXY.buf_ofs_x    = 0;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFXY.buf_ofs_y    = 0;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_BUFXY, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { // VDDO_DISPLAYER_OP_BUFXY
			UINT32          buf_ofs_x;       ///< The display Video/OSD layer buffer source offset X
			UINT32          buf_ofs_y;       ///< The display Video/OSD layer buffer source offset Y
			KDRV_VDDO_DISPLAYER    layer;
		} KDRV_VDDO_BUFXY;

		/**
		    Set Specified Display Layer Buffer and Window Size

		    This structure is used for layer operation code "VDDO_DISPLAYER_BUFWINSIZE"
		    to set the specified "DISPLAYER" buffer Width/Height/LineOffset and window size. This structure shuld be used like this:
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.buf_width  = 320;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.buf_weight = 240;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs= 320;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.win_width  = 320;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.win_height = 240;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x    = 0;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x    = 0;
		\n  disp_lyr.SEL.KDRV_VDDO_BUFWINSIZE.layer = VDDO_DISPLAYER_VDO1;
		\n  kdrv_vddo_set(handel, VDDO_DISPLAYER_BUFWINSIZE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set kdrv_vddo_trigger(handler) to validate the configurations.
		*/
		struct { // VDDO_DISPLAYER_BUFWINSIZE
			KDRV_VDDO_DISPBUFFORMAT format; ///< Buffer format
			UINT32          buf_width;     ///< Set display buffer width in PIXEL COUNT for Video/OSD layer.
			UINT32          buf_height;    ///< Set display buffer height in PIXEL COUNT for Video/OSD layer.
			UINT32          buf_line_ofs;   ///< Set display buffer lineoffset in BYTES for Video/OSD layer.

			UINT32          win_width;     ///< The display Video/OSD layer output window width
			UINT32          win_height;    ///< The display Video/OSD layer output window height

			INT32           win_ofs_x;       ///< The display Video/OSD layer output window offset X
			INT32           win_ofs_y;       ///< The display Video/OSD layer output window offset Y
			KDRV_VDDO_DISPLAYER    layer;
		} KDRV_VDDO_BUFWINSIZE;

        /**
		    Get CST value of RGB to YUV for color key @ ARGB

		    This structure is used only for OSD layer

		*/
		struct { //DISPLAYER_OP_GET_CST_FROM_RGB_TO_YUV
			UINT8    r_to_y;    ///< Point of R data and output for Y data (need shift to 8bit)
			UINT8    g_to_u;    ///< Point of G data and output for U data (need shift to 8bit)
			UINT8    b_to_v;    ///< Point of B data and output for V data (need shift to 8bit)
			UINT8    alpha;     ///< Use for align to 4 byte (alpha here is useless)
		} KDRV_VDDO_CST_OF_RGB_TO_YUV;

	} SEL;
} KDRV_VDDO_DISPLAYER_PARAM;


/**
    KDRV VDDO parameter select id

    This is used in kdrv_vddo_set and kdrv_vddo_get to set and get the VDDO configuration parameter.
*/
typedef enum {
	/*
		dispaly control group
	*/
	VDDO_DISPCTRL_BASE = 0x00000000,
	VDDO_DISPCTRL_WAIT_FRM_END,                  ///< [set] Wait display engine's frame end.
	VDDO_DISPCTRL_WAIT_DRAM_OUT_DONE,            ///< [set] Wait display engine's dram out done.
	VDDO_DISPCTRL_ENABLE,                        ///< [set/get] Set display engine's global enable/disable.
	VDDO_DISPCTRL_ALL_LYR_EN,                    ///< [set/get] Set ALL selected display layers Enable/Disable.
	VDDO_DISPCTRL_BACKGROUND,                    ///< [set/get] Set Display background color.
	VDDO_DISPCTRL_DMA_ABORT,                    ///< [set/get] Set Display DMA ABORT.
	
	/*
		dispaly device group
	*/
	VDDO_DISPDEV_BASE = 0x10000000,
	VDDO_DISPDEV_OPEN_DEVICE,                    ///< [set/get] Open spcified display device.
	VDDO_DISPDEV_CLOSE_DEVICE,                   ///< [set]     Close current display active device.
	VDDO_DISPDEV_REG_IF,                         ///< [set/get] Set current display device control interface. Please set before VDDO_DISPDEV_OPEN_DEVICE.
	VDDO_DISPDEV_HDMIMODE,                       ///< [set/get] Change the HDMI Audio/Video Mode.(not support)
	VDDO_DISPDEV_ROTATE,                         ///< [set]     Set display Device rotation for panel.
	VDDO_DISPDEV_LCDMODE,                        ///< [set/get] Change the Panel's mode
	VDDO_DISPDEV_POWERDOWN,                      ///< [set/get] Set Powerdown for HDMI and TV
	VDDO_DISPDEV_OUTPUT_DRAM,                    ///< [set]     Set IDE data output to dram(only available in IDE1 with VDO2 disable)
	VDDO_DISPDEV_TVFULL,                         ///< [set]     Set TV full screen(not support)
	VDDO_DISPDEV_DISPSIZE,                       ///< [get] Pre Get display device optimal buffer/window size before open device.
	VDDO_DISPDEV_HDMI_ABI,                       ///< [get] Get hdmi dev display ability.(not support)

	/*
		dispaly layer group
	*/
	VDDO_DISPLAYER_BASE = 0x20000000,
	VDDO_DISPLAYER_ENABLE,                       ///< [set/get] Set specified display Layer Enable/Disable.
	VDDO_DISPLAYER_BUFADDR,                      ///< [set/get] Set display Buffer Address
	VDDO_DISPLAYER_OUTDIR,                       ///< [set/get] Set Buffer Output Order such as Flip / Rotation
	VDDO_DISPLAYER_PALETTE,                      ///< [set/get] Set the OSD palette table. Valid for OSD layer only.
	VDDO_DISPLAYER_BLEND,                        ///< [set/get] Set the layer's blending
	VDDO_DISPLAYER_VDOCOLORKEY_SRC,              ///< [set/get] Set the Video color key source
	VDDO_DISPLAYER_COLORKEY,                     ///< [set/get] Set the layer's color key
	VDDO_DISPLAYER_FD_CONFIG,                    ///< [set/get] Set the FD rect. size
	VDDO_DISPLAYER_OP_BUFXY,                     ///< [set/get] Set the x/y of buffer source window

	VDDO_DISPLAYER_BUFWINSIZE,                   ///< [set/get] Set the layer's buffer size format and window size
	VDDO_DISPLAYER_CST_FROM_RGB_TO_YUV,          ///< [get] Get the OSD layer's YUV data of color key(RGB->YUV)(Only for OSD1)

	ENUM_DUMMY4WORD(KDRV_VDDO_PARAM_ID)
} KDRV_VDDO_PARAM_ID;

/*!
 * @fn INT32 kdrv_vddo_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_vddo_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_vddo_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_vddo_close(UINT32 chip, UINT32 engine);

/*!
 * @brief trigger hardware engine to load new config
 * @param handler 				the handler of hardware
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_vddo_trigger(UINT32 handler, KDRV_CALLBACK_FUNC *p_cb_func);

/*!
 * @fn INT32 kdrv_vddo_set(UINT32 handler, VDDO_ID id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param handler        the handler of hardware
 * @param id             the id of parameters
 * @param param          the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_vddo_set(UINT32 handler, KDRV_VDDO_PARAM_ID id, VOID *p_param);

/*!
 * @fn INT32 kdrv_vddo_get(UINT32 handler, VDDO_ID id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param handler        the handler of hardware
 * @param id             the id of parameters
 * @param param          the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_vddo_get(UINT32 handler, KDRV_VDDO_PARAM_ID id, VOID *p_param);

#endif

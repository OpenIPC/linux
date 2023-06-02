/**
    Public header file for ctl_audio

    This file is the header file that define the API and data type for ctl_audio.

    @file       ctl_aud.h
    @ingroup    mIDrvSensor
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _CTL_AUD_H
#define _CTL_AUD_H

#define CTL_AUD_NAME_LEN 32

typedef enum {
	/* set only*/
	CTL_AUDDRV_CFGID_SET_BASE = 0x00000000,
	CTL_AUDDRV_CFGID_SET_SAMPLE_RATE,
	CTL_AUDDRV_CFGID_SET_CHANNEL,
	CTL_AUDDRV_CFGID_SET_VOLUME,

	/* get only */
	CTL_AUDDRV_CFGID_GET_BASE = 0x01000000,

	/* set & get */
	CTL_AUDDRV_CFGID_BASE = 0x02000000,

	/* user define */
	CTL_AUDDRV_CFGID_USER_BASE = 0x03000000,
	CTL_AUDDRV_CFGID_USER_DEFINE1,      ///< user define 1
	CTL_AUDDRV_CFGID_USER_DEFINE2,      ///< user define 2
	CTL_AUDDRV_CFGID_USER_DEFINE3,      ///< user define 3
	CTL_AUDDRV_CFGID_USER_DEFINE4,      ///< user define 4
	CTL_AUDDRV_CFGID_USER_DEFINE5,      ///< user define 5

	ENUM_DUMMY4WORD(CTL_AUDDRV_CFGID)
} CTL_AUDDRV_CFGID;

typedef enum {
	CTL_AUD_ID_CAP = 0,   //record
	CTL_AUD_ID_OUT,       //playback
	CTL_AUD_ID_MAX = CTL_AUD_ID_OUT + 1,
	ENUM_DUMMY4WORD(CTL_AUD_ID)
} CTL_AUD_ID;

typedef struct {
	UINT32 audio_pinmux;       ///< need mapping to top.h define (PIN_AUDIO_CFG_I2S/PIN_AUDIO_CFG_MCLK)
	UINT32 cmd_if_pinmux;      ///< need mapping to top.h define (PIN_I2C_CFG)
} CTL_AUD_PINMUX_CFG;

typedef struct {
	CTL_AUD_PINMUX_CFG pinmux;
} CTL_AUD_INIT_PIN_CFG;

typedef struct {
	UINT32 bit_width;
	UINT32 bit_clk_ratio;
	UINT32 op_mode;
	UINT32 tdm_ch;
} CTL_AUD_INIT_I2S_CFG;

typedef struct {
	CTL_AUD_INIT_PIN_CFG pin_cfg;
	CTL_AUD_INIT_I2S_CFG i2s_cfg;
} CTL_AUD_INIT_CFG_OBJ;


typedef struct {
	ER(*open)(CTL_AUD_ID id);                                               ///< initial audio codec flow
	ER(*close)(CTL_AUD_ID id);                                              ///< un-initial audio codec flow
	ER(*start)(CTL_AUD_ID id);                                              ///< start audio codec flow
	ER(*stop)(CTL_AUD_ID id);                                               ///< stop audio codec flow
	ER(*set_cfg)(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);   ///< set codec information
	ER(*get_cfg)(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);   ///< get codec information
} CTL_AUD_DRV_TAB;


/*export*/
INT32 ctl_aud_reg_auddrv(CHAR *name, CTL_AUD_DRV_TAB *reg_tab);
INT32 ctl_aud_unreg_auddrv(CHAR *name);
INT32 ctl_aud_module_init_cfg(CTL_AUD_ID id, CHAR *name, CTL_AUD_INIT_CFG_OBJ *init_cfg_obj);

#endif

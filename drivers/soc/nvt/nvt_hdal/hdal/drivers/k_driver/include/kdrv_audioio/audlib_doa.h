/**
    Audio Direction-Of-Arrival (DOA) and Voice-Active-Detection (VAD) library

	Audio DOA and VAD library. This is pure software audio library.

    @file       doa.h
    @ingroup    mIAudDOA
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _AUDLIB_DOA_H
#define _AUDLIB_DOA_H

#ifdef __KERNEL__
#include "kdrv_type.h"
#else
#include "kdrv_type.h"
#endif

#ifndef FLOAT
#define FLOAT float
#endif

#define AUDUOA_PRECISION		100000000

/**
	Audio DOA Mode selection

	This is used in audlib_doa_set_config(AUDDOA_CONFIG_ID_MODE) to assign operation mode.
*/
typedef enum {
	AUDDOA_MODE_VAD_ONLY,	///< Only VAD
	AUDDOA_MODE_VAD_DOA,	///< Both VAD and DOA

	ENUM_DUMMY4WORD(AUDDOA_MODE)
} AUDDOA_MODE;


/**
    Audio Direction-Of-Arrival (DOA) Configuration Select ID

    This is used in audlib_doa_set_config() as the select ID to assign new configurations
    for Audio Direction-Of-Arrival (DOA) library.
*/
typedef enum {
	AUDDOA_CONFIG_ID_MODE,				///< Specify the DOA library operation mode as AUDDOA_MODE_VAD_DOA(default) or AUDDOA_MODE_VAD_ONLY only.
										///< The mode AUDDOA_MODE_VAD_ONLY needs smaller buffer size @AUDDOA_CONFIG_ID_BUFFER_SIZE.
	AUDDOA_CONFIG_ID_SAMPLERATE,		///< Set SampleRate in Hertz. Please use AUDIO_SR as input.
	AUDDOA_CONFIG_ID_CHANNEL_NO,		///< Valid settings can be 2 or 4(TDM-4-chaanels).

	AUDDOA_CONFIG_ID_MIC_DISTANCE,		///< Microphone distance in meters. Please use AUDDOA_MIC_DISTANCE() and enter float input.
										///< For example if distance is 8.127 cm. Please enter AUDDOA_MIC_DISTANCE(0.08127) as input parameter.
	AUDDOA_CONFIG_ID_OPERATION_SIZE,	///< Specify the basic operation size during audlib_doa_run_vad(). Currently allow 2048/4096/8192.
										///< The larger size can get better prediction performance. Default suggest to use 4096.
	AUDDOA_CONFIG_ID_FRAME_SIZE,		///< DOA frame size per 1024 / 2048 / 4096 samples. Default is 1024.

	AUDDOA_CONFIG_ID_BUFFER_ADDR,		///< DOA internal needed buffer size.
	AUDDOA_CONFIG_ID_BUFFER_SIZE,		///< The requirement buffer size shall be at least ((AUDDOA_CONFIG_ID_FRAME_SIZE x 48)+ 6144) bytes for DOA.
										///< If only using VAD, the buffer size needs 7168 bytes.(Can't use audlib_doa_get_direction())
	AUDDOA_CONFIG_ID_AVERGARE,			///< DOA Sample Avergae times. default is 1. The larger value consumes larger CPU computing power.
	AUDDOA_CONFIG_ID_VAD_FRONT_EXT,		///< The Vad stable front extention period.
	AUDDOA_CONFIG_ID_VAD_BACK_EXT,		///< The Vad stable backward extention period.
	AUDDOA_CONFIG_ID_VAD_SENSITIVITY,	///< Configure the VAD detect sensitivity. Default value is 10. valid seeting is 0 ~ 20.
										///< Smaller than 10 is less sensitive. Larger than 10 is more sensitive.
										///< The user shall know that the MORE sensitive would decrease the DOA prediction accuracy.
	AUDDOA_CONFIG_ID_RESAMPLE_EN,		///< ENABLE/DISABLE the sample rate auto resample to 16KHz for DOA detection. Default is ENABLE.
	AUDDOA_CONFIG_ID_DEFAULT_CFG,		///< ENABLE/DISABLE the default configrations for parameters: AUDDOA_CONFIG_ID_VAD_FRONT_EXT / AUDDOA_CONFIG_ID_VAD_BACK_EXT.
										///< The user needs not configuring these settings if setting this field to ENABLE. Default is ENABLE.

	AUDDOA_CONFIG_ID_VAD_MIC_IDX,		///< VAD needs only one channel audio data for use. Select one channel audio for VAD usages.
										///< For 2 channels audio data, please input 0 or 1. For 4 channels audio data, please input 0/1/2/3.
										///< Default value is 0.
	AUDDOA_CONFIG_ID_INDEX_MIC0,		///< Specified MIC0 index for the dram data channel index. default=0.
										///< Valid setting range 0~1 for 2 channels. Valid setting range 0~3 for 4 channels.
	AUDDOA_CONFIG_ID_INDEX_MIC1,		///< Specified MIC1 index for the dram data channel index. default=1.
										///< Valid setting range 0~1 for 2 channels. Valid setting range 0~3 for 4 channels.
	AUDDOA_CONFIG_ID_INDEX_MIC2,		///< Specified MIC2 index for the dram data channel index. default=2.
										///< Not Valid for 2 channels. Valid setting range 0~3 for 4 channels.
	AUDDOA_CONFIG_ID_INDEX_MIC3,		///< Specified MIC3 index for the dram data channel index. default=3.
										///< Not Valid for 2 channels. Valid setting range 0~3 for 4 channels.

	AUDDOA_CONFIG_ID_DBG_MSG,			///< ENABLE/DISABLE the VAD/DOA runtime debug monitor message.

	ENUM_DUMMY4WORD(AUDDOA_CONFIG_ID)
} AUDDOA_CONFIG_ID;


/**
	This is used in audlib_doa_set_config(AUDDOA_CONFIG_ID_MIC_DISTANCE, AUDDOA_MIC_DISTANCE(?)) to assign microphone distance.
*/
#define AUDDOA_MIC_DISTANCE(x)	((INT32)((FLOAT)(x)/343.2*AUDUOA_PRECISION))




/**
    Open the Direction-Of-Arrival (DOA) library.

    This API must be called to initialize the Audio Direction-Of-Arrival (DOA) basic settings.

    @return
     - @b E_OK:   Open Done and success.
*/
extern ER       audlib_doa_open(void);

/**
    Check if the Audio Direction-Of-Arrival (DOA) library is opened or not.

    Check if the Audio Direction-Of-Arrival (DOA) library is opened or not.

    @return
     - @b TRUE:  Already Opened.
     - @b FALSE: Have not opened.
*/
extern BOOL     audlib_doa_is_opened(void);

/**
    Close the Audio Direction-Of-Arrival (DOA) library.

    Close the Audio Audio Direction-Of-Arrival (DOA) library.
    After closing the Audio Audio Direction-Of-Arrival (DOA) library,
    any accessing for the Audio Audio Direction-Of-Arrival (DOA) APIs are forbidden except audlib_doa_open().

    @return
     - @b E_OK:  Close Done and success.
*/
extern ER       audlib_doa_close(void);

/**
    Set specified Audio Direction-Of-Arrival (DOA) configurations.

    This API is used to set the specified Audio Direction-Of-Arrival (DOA) configurations.

    @param[in] doa_config_id      Select which of the DOA options is selected for configuring.
    @param[in] doa_config         The configurations of the specified DOA option.

    @return void
*/
extern void     audlib_doa_set_config(AUDDOA_CONFIG_ID doa_config_id, INT32 doa_config);

/**
    Get specified Audio Direction-Of-Arrival (DOA) configurations.

    This API is used to get the specified Audio Direction-Of-Arrival (DOA) configurations.

    @param[in] doa_config_id      Select which of the DOA options is selected for configuring.

    @return The configuration value of the specified DOA doa_config_id.
*/
extern INT32    audlib_doa_get_config(AUDDOA_CONFIG_ID doa_config_id);

/**
    Initialize the Audio Direction-Of-Arrival (DOA) before recording start

    This API would reset the DOA internal temp buffer and variables.
    The user must call this before starting of any new BitStream.

    @return
     - @b TRUE:  Done and success.
     - @b FALSE: Operation fail.
*/
extern BOOL     audlib_doa_init(void);

/**
    Apply Audio Direction-Of-Arrival (DOA)'s Voice Active Detect(VAD) to audio stream

    Apply Audio Direction-Of-Arrival (DOA)'s Voice Active Detect(VAD) to audio stream.
    The return value would be the VAD detection probability. If the DOA would be performed after VAD,
    we suggest that only the probability value 100's audio frame is adopted for DOA prediction.
    Smaller probability value would cause DOA prediction less accurately.

    @param[in] doa_input_addr    Input audio buffer address in AUDDOA_CONFIG_ID_OPERATION_SIZE size..

    @return 0~100 percentage of the VAD detected probability.
*/
extern UINT32   audlib_doa_run_vad(UINT32 doa_input_addr);

/**
	Get the audio direction of arrival result in 0-359 degrees

	Get the audio direction of arrival result in 0-359 degrees.
	The direction result would be 0~359 if 4 microphones(channel) used.
	The direction result would be 0~179 if 2 microphones(channel) used.

    @return 0~359 degrees of the audio direction
*/
extern INT32    audlib_doa_get_direction(void);

#endif

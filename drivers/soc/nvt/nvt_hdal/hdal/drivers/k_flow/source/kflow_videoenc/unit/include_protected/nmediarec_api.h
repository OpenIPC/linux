/**
    Header file of NMedia Recorder Task

    Exported header file of media recorder task.

    @file       NMediaRecAPI.h
    @ingroup    mIAPPNMEDIAREC
    @note       add gNMediaRecPwrOffPT

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _NMEDIARECAPI_H
#define _NMEDIARECAPI_H

#include "kflow_videoenc/media_def.h"
#include "movieinterface_def.h"

#ifdef __KERNEL__
#define _SECTION(sec)
#endif

/**
    @name NMedia record event ID

    Callback ID of Smedia recorder.
*/
//@{
#define NMEDIAREC_EVENT_RESULT_NORMAL  		MOVREC_EVENT_RESULT_NORMAL ///< Finish normally
#define NMEDIAREC_EVENT_RESULT_FULL         MOVREC_EVENT_RESULT_FULL   ///< Card full
#define NMEDIAREC_EVENT_RESULT_SLOW         MOVREC_EVENT_RESULT_SLOW   ///< Write card too slowly
#define NMEDIAREC_EVENT_RESULT_OVERTIME     MOVREC_EVENT_RESULT_OVERTIME///< Overtime
#define NMEDIAREC_EVENT_RESULT_WRITE_ERR    MOVREC_EVENT_RESULT_WRITE_ERR///< Filesystem write fails
#define NMEDIAREC_EVENT_EMR_FILECOMPLETE    MOVREC_EVENT_EMR_FILECOMPLETE///< enc one frame
//@}


/**
    @name Valid file type

    Valid file type.
*/
//@{
#define NMEDIAREC_AVI    					MEDIA_FILEFORMAT_AVI    ///< .avi file
#define NMEDIAREC_MOV    					MEDIA_FILEFORMAT_MOV    ///< .mov file
#define NMEDIAREC_MP4    					MEDIA_FILEFORMAT_MP4    ///< .mp4 file
#define NMEDIAREC_TS    					MEDIA_FILEFORMAT_TS     ///< .ts file
#define NMEDIAREC_JPG   					MEDIA_FILEFORMAT_JPG    ///< .jpg file
//@}

/**
    @name Valid video encode codec

    Valid video encode codec.
*/
//@{
#define NMEDIAREC_ENC_MJPG    				MEDIAVIDENC_MJPG    ///< motion JPG
#define NMEDIAREC_ENC_H264   				MEDIAVIDENC_H264    ///< H.264
#define NMEDIAREC_ENC_H265   				MEDIAVIDENC_H265    ///< H.265
//@}

/**
    @name Valid audio encode codec

    Valid audio encode codec.
*/
//@{
#define NMEDIAREC_ENC_PCM    				MOVAUDENC_PCM   ///< PCM
#define NMEDIAREC_ENC_AAC    				MOVAUDENC_AAC   ///< AAC
#define NMEDIAREC_ENC_PPCM   				MOVAUDENC_PPCM  ///< Packet PCM
#define NMEDIAREC_ENC_ULAW   				MOVAUDENC_ULAW  ///< PCM uLaw
#define NMEDIAREC_ENC_ALAW   				MOVAUDENC_ALAW  ///< PCM aLaw
//@}


/**
    @name Valid end type

    Valid recording end type. Normal end, cut overlap, or cut till card full.
*/
//@{
#define NMEDIAREC_ENDTYPE_NORMAL             MOVREC_ENDTYPE_NORMAL ///< normal end, such as press key to stop recording
#define NMEDIAREC_ENDTYPE_CUTOVERLAP         MOVREC_ENDTYPE_CUTOVERLAP///< cut file and continue recording automatically; overlap the old files (cyclic recording)
#define NMEDIAREC_ENDTYPE_CUT_TILLCARDFULL   MOVREC_ENDTYPE_CUT_TILLCARDFULL///< cut file and continue recording automatically till card full
#define NMEDIAREC_ENDTYPE_MAX                MOVREC_ENDTYPE_MAX    ///< maximum number of recording end type
//@}


//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
/**
    @name Smedia rotation angle information

    NMedia recording roation angle information.
*/
//@{
#define NMEDIAREC_MOV_ROTATE_0       		MOVREC_MOV_ROTATE_0   ///< angle 0
#define NMEDIAREC_MOV_ROTATE_90      		MOVREC_MOV_ROTATE_90   ///< angle 90
#define NMEDIAREC_MOV_ROTATE_180     		MOVREC_MOV_ROTATE_180   ///< angle 180
#define NMEDIAREC_MOV_ROTATE_270     		MOVREC_MOV_ROTATE_270   ///< angle 270
//@}
//#NT#2013/04/17#Calvin Chang -endan

#define NMEDIAREC_TS_PES_HEADER_LENGTH     	14
#define NMEDIAREC_TS_NAL_LENGTH            	7    // H264 need 6 (0x00 00 00 01 09 10) / h265 need 7 (0x00 00 00 01 46 01 10)
#define NMEDIAREC_TS_ADTS_LENGTH           	7

#define NMEDIAREC_JPG_EXIF_HEADER_LENGTH    65536


extern void NMR_ImgCap_InstallID(void) _SECTION(".kercfg_text");
extern void nmr_vdoenc_install_id(void) _SECTION(".kercfg_text");
extern void nmr_vdocodec_install_id(void) _SECTION(".kercfg_text");
extern void nmr_audenc_install_id(void) _SECTION(".kercfg_text");
extern void NMR_BsMux_InstallID(void) _SECTION(".kercfg_text");
extern void NMR_FileOut_InstallID(void) _SECTION(".kercfg_text");
extern void isf_vdoenc_install_id(void) _SECTION(".kercfg_text");
extern void ISF_AudEnc_InstallID(void) _SECTION(".kercfg_text");

extern void nmr_audenc_uninstall_id(void);
extern void isf_vdoenc_uninstall_id(void);
extern void nmr_vdoenc_uninstall_id(void);
extern void nmr_vdocodec_uninstall_id(void);



#endif//_NMEDIARECAPI_H

//@}

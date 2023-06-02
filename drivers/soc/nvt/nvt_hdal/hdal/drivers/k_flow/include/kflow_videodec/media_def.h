/**
    Header file of media

    media definition

    @file       media_def.h
    @ingroup    mIMEDIAWRITE

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _MEDIADEF_H
#define _MEDIADEF_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#else
#ifdef __UITRON
#include "Type.h"
#else
#endif
#endif

#define MEDIAREC_VER_INIT    0
#define MEDIAREC_VER_2_0     20
#define MEDIAREC_VER_3_0     30


#define MEDIAREC_GOP_IPONLY     0 ///< H264 IP only
#define MEDIAREC_GOP_IPB        1 ///< H264 IPB

/**
    @name video codec type for mov writer

    video codec type for mov writer
*/
//@{
#define MEDIAVIDENC_MJPG            1       ///< motion jpeg
#define MEDIAVIDENC_H264            2       ///< h.264
#define MEDIAVIDENC_H265            3      ///< h.265
//@}

/**
    @name audio codec type for mov writer

    audio codec type for mov writer
*/
//@{
#define MOVAUDENC_PCM   1   ///< PCM
#define MOVAUDENC_AAC   2   ///< AAC
#define MOVAUDENC_PPCM  3   ///< Packed PCM for SMedia 3.0
#define MOVAUDENC_ULAW  4   ///< PCM uLaw
#define MOVAUDENC_ALAW  5   ///< PCM aLaw
//@}


/**
    @name recoding type

     Record a video-only/video-audio media file. Used for uiRecFormat in MediaRec_Open().
*/
//@{
#define MEDIAREC_VID_ONLY           1 //video only, normal speed
#define MEDIAREC_AUD_VID_BOTH       2 //video/audio sync, normal speed
#define MEDIAREC_FAST_FWD           3 //video only, no used now
#define MEDIAREC_GOLFSHOT           4 //rec 60/120 fps, play 30 fps, no audio
#define MEDIAREC_TIMELAPSE          5 //rec n fps (n < 30), play 30 fps, no audio
#define MEDIAREC_MERGEYUV           6 //merge given YUV pic, no audio
#define MEDIAREC_LIVEVIEW           7 //live view with small memory, write no card
#define MEDIAREC_MERGEBS            8 //merge bitstream

#define MEDIAREC_FLASH_RECORD_AV    0x40 //flash record audio/video
#define MEDIAREC_FLASH_RECORD_VO    0x41 //flash record video only
//@}


/**
    @name media file type

    Probe a media file to check file type.
*/
//@{
#define MEDIA_FILEFORMAT_MOV    0x1    ///< MOV file format
#define MEDIA_FILEFORMAT_AVI    0x2    ///< AVI file format
#define MEDIA_FILEFORMAT_MP4    0x4    ///< MP4 file format
#define MEDIA_FILEFORMAT_TS     0x8    ///< TS file format
#define MEDIA_FILEFORMAT_JPG    0x10    ///< JPG file format
#define MEDIA_FILFFORMAT_TOTAL  5      ///< Totoal of file format type
//@}


/**
    @name media parser index

    Get a media file read object by parser index.
*/
//@{
#define MEDIA_PARSEINDEX_MOV    0x1    ///< MOV file type
#define MEDIA_PARSEINDEX_AVI    0x2    ///< AVI file type
#define MEDIA_PARSEINDEX_TS     0x8    ///< TS  file type
//@}

/**
    @name media video codec type

    Read mov header to check video codec type.
*/
//@{
#define MEDIAPLAY_VIDEO_MJPG    0x1  ///< motion-jpeg
#define MEDIAPLAY_VIDEO_H264    0x2  ///< H.264
#define MEDIAPLAY_VIDEO_H265    0x3  ///< H.265
#define MEDIAPLAY_VIDEO_YUV     0x4  ///< YUV
//@}


/**
    @name Audio codec type for MOV file

    Getting information type from file reading object library.
*/
//@{
#define MEDIAAUDIO_CODEC_SOWT  1     ///< PCM audio format
#define MEDIAAUDIO_CODEC_MP4A  2     ///< AAC audio format
#define MEDIAAUDIO_CODEC_RAW8  3     ///< RAW audio format
#define MEDIAAUDIO_CODEC_ULAW  4     ///< uLaw audio format
#define MEDIAAUDIO_CODEC_ALAW  5     ///< aLaw audio format
#define MEDIAAUDIO_CODEC_XX    6     ///< Unknown audio format
//@}


/**
    Video Display Aspect Ratio
*/
typedef enum {
	MEDIAREC_DAR_DEFAULT,         			///< default video display aspect ratio (the same as encoded image)
	MEDIAREC_DAR_16_9,            			///< 16:9 video display aspect ratio
	MEDIAREC_DAR_MAX,             			///< maximum video display aspect ratio
	ENUM_DUMMY4WORD(MEDIAREC_DAR)
} MEDIAREC_DAR;

#endif

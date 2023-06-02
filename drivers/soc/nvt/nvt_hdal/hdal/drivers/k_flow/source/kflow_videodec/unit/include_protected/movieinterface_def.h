/**
    Header file of movie interface

    movie interface

    @file       MovieInterface.h
    @ingroup    mIAPPMOVIEINTERFACE

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _MOVIEINTERFACEDEF_H
#define _MOVIEINTERFACEDEF_H

#include "kwrap/type.h"

//////#include "FileSysTsk.h"
//////#include "SysKer.h"

//#define MODULE_DBGLVL           2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER


/**
    Stop type for MovRec_Stop()
*/
typedef enum {
	MOVREC_NO_WAIT  = 0x0,  ///< stop recording and no wait
	MOVREC_WAIT_END = 0x1,   ///< stop recording and wait this action finished
	ENUM_DUMMY4WORD(MOVREC_STOP_TYPE)
} MOVREC_STOP_TYPE;

/**
    Stop type for MovRec_Stop()
*/
typedef enum {
	MOVREC_EXTENDUNIT_10  = 10,
	MOVREC_EXTENDUNIT_20  = 20,
	MOVREC_EXTENDUNIT_30 = 30,
	ENUM_DUMMY4WORD(MOVREC_EXTENDCRASH_UNIT)
} MOVREC_EXTENDCRASH_UNIT;

#define MOVIEREC_AVIREC     0
#define MOVIEREC_MEDIAREC   1

#define MOVIEREC_JPGFMT_444  1
#define MOVIEREC_JPGFMT_422  2
#define MOVIEREC_JPGFMT_420  3

#define MOVIEREC_RECFORMAT_VIDEO_ONLY       1
#define MOVIEREC_RECFORMAT_AUD_VID_BOTH     2

#define MOVIEREC_1STQP_BEST     1280
#define MOVIEREC_1STQP_NORMAL   2560
#define MOVIEREC_1STQP_DRAFT    6400

#define MOVIEREC_ENDTYPE_CUT    1
#define MOVIEREC_ENDTYPE_NORMAL 2

#define MOVREC_VIDBS_REVSEC       3           //reserved 4 sec BS//2015/06/17 5->4
#define MOVREC_FAKE_H264BS_BUF    0x64000 //400K

#define MOVREC_VIDBS_REVSEC_S     3           //reserved 3 sec BS

#define MOVREC_VIDBS_NUMINBS      10  //if not tiny = 6+4
#define MOVREC_VIDBS_NUMINBS_S    2  //if tiny = 2+3, if writefile=0

#define MOVREC_AVI_HDR            0x40000     //one big cluster align//2013/05/28 Meg
#define MOVREC_MAX_CLUSIZE        MOVREC_AVI_HDR

#define MOVREC_FILESAVE_NONE        0 //no write
#define MOVREC_FILESAVE_PART1       1 //partition 1
#define MOVREC_FILESAVE_PART2        2 //partition 2

typedef void (MovieRecCallback)(UINT32 event_id, UINT32 e_value);

typedef enum {
	MOVREC_STATUS_NOT_OPENED        = 0x1,  ///< media recorder task is not open
	MOVREC_STATUS_OPENED_NOT_RECORD = 0x2,  ///< media recorder task is open, not recording
	MOVREC_STATUS_RECORDING         = 0x3,  ///< media recorder task is recording
	ENUM_DUMMY4WORD(MOVREC_STATUS_TYPE)
} MOVREC_STATUS_TYPE;

/**
    check type for MediaRec_GetLastV_BS()
*/
typedef enum {
	MEDIAREC_VIDBS_PATH1  = 0x0,  ///< video path1 bitstream
	MEDIAREC_VIDBS_PATH2  = 0x1,  ///< video path2 bitstream
	ENUM_DUMMY4WORD(MEDIAREC_VIDBS_TYPE)
} MEDIAREC_VIDBS_TYPE;

/**
    check type for MediaRec_SetDisk2SecRestriction()
*/
typedef enum {
	MEDIAREC_RESTR_INDEXBUF = 0x0,  ///< check index buffer to calculate max recording time
	MEDIAREC_RESTR_FSLIMIT  = 0x1,  ///< check fs limitation, 4GB now
	ENUM_DUMMY4WORD(MEDIAREC_RESTR_TYPE)
} MEDIAREC_RESTR_TYPE;
//#NT#2010/05/17#Meg Lin -end

/**
    @name IPL mode change callback event

    Event ID for changing IPL mode callback function.
*/
//@{
#define MOVREC_IPLCHG_PREVIEW     1   ///< Set IPL to preview mode
#define MOVREC_IPLCHG_VIDEO       2   ///< Set IPL to video mode
#define MOVREC_DIRECT_FAKESTOPIPL 3   ///< Set IPL to preview mode for IME/ALL direct
#define MOVREC_IPLCHG_INITBS      4   ///< initbs
#define MOVREC_DIRECT_FAKESTART   5   ///< Set IPL to video mode for IME/ALL direct
#define MOVREC_DIRECT_FAKESTART_E 6   ///< Set IPL to video mode for IME/ALL direct
//@}

/**
    @name Media ImmediatelyProc callback ID

    Callback ID of media ImmediatelyProc.
*/
//@{
#define     MOVREC_IMMPROC_CUTFILE_END        1  ///< end of one cut file, no parameter
#define     MOVREC_IMMPROC_DELETE_FILE        2  ///< end of deleting file, p1: directory ID, p2: file ID
#define     MOVREC_IMMPROC_ENC_SETTING        3  ///< special encoding parameters setting, such as H.264 3DNR
#define     MOVREC_IMMPROC_FILEFINISH         4  ///< end of one file, no parameter
//@}

#define MOVREC_IPLGET_READYBUF    1   ///< Get IPL ready buffer

#define MOVREC_RECPARAM_WIDTH             0x01 ///< width, p1 valid, p2 fileid
#define MOVREC_RECPARAM_HEIGHT            0x02 ///< height, p1 valid, p2 fileid
#define MOVREC_RECPARAM_FRAMERATE         0x03 ///< frameRate, p1 valid
#define MOVREC_RECPARAM_QUALITY           0x04 ///< quality, p1 valid
#define MOVREC_RECPARAM_TARGETRATE        0x05 ///< target rate (bytes per second), p1 valid, p2 fileid
#define MOVREC_RECPARAM_GOLFSHOT_ON       0x08 ///< change recFormat to golfshot, p1 0 or 1
#define MOVREC_RECPARAM_CUTSEC            0x09 ///< seamless cut sec
#define MOVREC_RECPARAM_CUTSIZE           0x0A ///< seamless cut size
#define MOVREC_RECPARAM_ENDTYPE           0x0B ///< end type
#define MOVREC_RECPARAM_FLASH_ON          0x0C ///< flash recording on/off, p1 0 or 1
#define MOVREC_RECPARAM_PWROFFPT          0x0D ///< enable/disable poweroff_protection, default: on
#define MOVREC_RECPARAM_GPSON             0x0E ///< enable/disable gps info on, default: off
#define MOVREC_RECPARAM_TIMELAPSE         0x0F ///< set time lapse recording mode (ON/OFF)
#define MOVREC_RECPARAM_TIMELAPSE_TIME    0x10 ///< set time lapse recording time interval (in unit of ms, should be larger than 1/30 sec)
#define MOVREC_RECPARAM_THUMB_DATA        0x11 ///< movie thumbnail settings, p1:on/off, p2:width, p3:height
#define MOVREC_RECPARAM_AUD_SAMPLERATE    0x12 ///< audio sample rate, AUDIO_SR_8000 or others
#define MOVREC_RECPARAM_AUD_CHS           0x13 ///< audio channels, 1 or 2
#define MOVREC_RECPARAM_AUD_SRC           0x14 ///< audio source, MEDIAREC_AUDTYPE_LEFT or others
#define MOVREC_RECPARAM_AUD_CODEC         0x15 ///< audio codec type, MOVAUDENC_PCM or others
#define MOVREC_RECPARAM_RECFORMAT         0x16 ///< recording format, MEDIAREC_AUD_VID_BOTH or others
#define MOVREC_RECPARAM_EN_CUSTOMUDATA    0x17 ///< enable custom user data or not
#define MOVREC_RECPARAM_VIDEOCODEC        0x18 ///< video codec type, MEDIAREC_ENC_H264 or others
#define MOVREC_RECPARAM_H264GOPTYPE       0x19 ///< h264 GOP type, MEDIAREC_GOP_IPONLY or others
#define MOVREC_RECPARAM_H2643DNRLEVEL     0x1A ///< h264 3DNR level
#define MOVREC_RECPARAM_RAWENC_Q          0x1B ///< Q value for raw-encoding
#define MOVREC_RECPARAM_EN_AUDFILTER      0x1C ///< enable audio filter or not
#define MOVREC_RECPARAM_DROP_FRAME_NUM    0x1D ///< drop frame number as start recording (for AV sync)
#define MOVREC_RECPARAM_H264QPLEVEL       0x1E ///< h264 QP initial level
#define MOVREC_RECPARAM_MOV_ROTATE        0x1F ///< MOV/MP4 file rotation, MEDIAREC_MOV_ROTATE_0
#define MOVREC_RECPARAM_FILETYPE_2        0x20 ///< file1 type, MEDIAREC_AVI as default
#define MOVREC_RECPARAM_MERGEYUV          0x21
#define MOVREC_RECPARAM_FILETYPE          0x87 ///< file type, MEDIAREC_MOV, MEDIAREC_AVI, MEDIAREC_MP4
#define MOVREC_RECPARAM_JPG_YUVFORMAT     0x22 ///< set jpeg yuv format
#define MOVREC_RECPARAM_MULTIREC_ON       0x23
#define MOVREC_RECPARAM_LIVEVIEW          0x24 ///< set MEDIAREC_LIVEVIEW recformat 2013/10/02 Meg
#define MOVREC_RECPARAM_DAR               0x25 ///< video display aspect ratio
#define MOVREC_RECPARAM_PRE_ENCODE        0x26 ///< video pre-encode (for applying 3DNR to 1st frame)
#define MOVREC_RECPARAM_SENSOR_FPS        0x27 ///< video sensor fps
#define MOVREC_RECPARAM_SENSOR_NUM        0x28 ///< input sensor number (for encoding path judgement)
#define MOVREC_RECPARAM_SVC               0x29 ///< set SVC disable/enable and GOP number, p1 value, p2 vsID
#define MOVREC_RECPARAM_TINYMODE_ON       0x2A ///< tiny mode on/off, p1 0 or 1
#define MOVREC_RECPARAM_RAWENC_ON         0x2B ///< rawencode on/off, p1 0 or 1
#define MOVREC_RECPARAM_Y_LINEOFT         0x2C ///< y lineoffset, p1 valid, p2 pathid
#define MOVREC_RECPARAM_UV_LINEOFT        0x2D ///< uv lineoffset, p1 valid, p2 pathid

#define MOVREC_RECPARAM_MAX_WIDTH         0x2E ///< MAX width, p1 valid, p2 fileid
#define MOVREC_RECPARAM_MAX_HEIGHT        0x2F ///< MAX height, p1 valid, p2 fileid
#define MOVREC_RECPARAM_MAX_TBR           0x30 ///< MAX TBR, p1 valid, p2 fileid
#define MOVREC_RECPARAM_H264GOPNUM        0x31 ///< change GOP number, p1 valid, p2 vsID
#define MOVREC_RECPARAM_VID_ACTCODEC_ID   0x32 ///< change videc codecid, p1 valid, p2 vsID
#define MOVREC_RECPARAM_VID_EN_2NDCODEC   0x33 ///< enable 2nd codec, p1 valid (pathid)
#define MOVREC_RECPARMA_EMRLOOP_ON        0x34 ///< enable emrloop on, p1 valid, p2 vsID
#define MOVREC_RECPARAM_CBRINFO           0x35 ///< CBR, p1 MP_VDOENC_CBR_INFO, p2 vsID
#define MOVREC_RECPARAM_VBRINFO           0x36 ///< VBR, p1 MP_VDOENC_VBR_INFO, p2 vsID
//#NT#2016/09/20#Bob Huang -begin
//#NT#Support HDMI Display with 3DNR Out
#define MOVREC_RECPARAM_H264_3DNROUT      0x37 ///< enable h264 3DNR Out or not
//#NT#2016/09/20#Bob Huang -end
//#NT#2016/10/05#Bob Huang -begin
//#NT#Support Multi 2P Enc
#define MOVREC_RECPARAM_MULTI_2PENC_ON    0x38 ///< enable h264 multi 2P enc or not
//#NT#2016/10/05#Bob Huang -end
#define MOVREC_RECPARAM_NOALLOCFILE       0x39 /// disable FS allocateFile, p1 on/off, default: off
#define MOVREC_RECPARAM_ROLLBACK2M        0x3A /// enable rollback 2m, p1 on/off
#define MOVREC_RECPARAM_H264ROTATE        0x3B /// h264 rotate

#define MOVREC_GETRECPARAM_TARGETBR       0x01 ///< target bitrate (MB per sec), p1: (in)pathid, p2: (out)
#define MOVREC_GETRECPARAM_FRAMERATE      0x02 ///< framerate, p1: (in)pathid, p2: (out)
#define MOVREC_GETRECPARAM_H264GOPNUM     0x03 ///< h264 gop num, p1: (out), p2: vidID (in)

#define     MOVREC_EVENT_RESULT_NORMAL        1   ///< Finish normally
#define     MOVREC_EVENT_RESULT_HW_ERR        2   ///< Hardware error
#define     MOVREC_EVENT_RESULT_FULL          3   ///< Card full
#define     MOVREC_EVENT_RESULT_SLOW          4   ///< Write card too slowly
#define     MOVREC_EVENT_RESULT_NOTREADY      5   ///< Not ready
#define     MOVREC_EVENT_THRESHOLD            6   ///< Threshold meets
#define     MOVREC_EVENT_RESULT_OVERTIME      7   ///< Overtime
#define     MOVREC_EVENT_RESULT_WRITE_ERR     10  ///< Filesystem write fails
#define     MOVREC_EVENT_STOPPED              11  ///< Callback when stops totally
#define     MOVREC_EVENT_FILECOMPLETE         13  ///< Write file completely
#define     MOVREC_EVENT_RESULT_CUT_FAIL      19  ///< Cut, overlap fail (write slowly)
#define     MOVREC_EVENT_CUSTOM_UDATA         21  ///< Set customized user data
#define     MOVREC_EVENT_ENCONE_OK            22  ///< enc one frame
#define     MOVREC_EVENT_VIDEO_BS_CB          23  ///< path1 video enc ok
#define     MOVREC_EVENT_H264BSCB_P1          24  ///< path2 video enc ok
#define     MOVREC_EVENT_RESULT_FLASHSTOP     25  ///< 30 sec after flash stop
#define     MOVREC_EVENT_AUDBSCB              26  ///< audio bitstream callback
#define     MOVREC_EVENT_LOOPREC_FULL         27  ///< no space for seamless recording
#define     MOVREC_EVENT_AUDREC_START         28  ///< similar to MOVREC_EVENT_START
#define     MOVREC_EVENT_RESULT_PARAMERR      29  ///< parameter error
#define     MOVREC_EVENT_RESULT_YUVQ_FULL     30  ///< yuv q full
#define     MOVREC_EVENT_EMR_FILECOMPLETE     31  ///< Write EMR file completely, p1=pathid(0, 1..)
#define     MOVREC_EVENT_H264BSCB_P2          32  ///< path3 video enc ok
#define     MOVREC_EVENT_H264BSCB_P3          33  ///< path4 video enc ok
#define     MOVREC_EVENT_CRASH_FILECOMPLETE   34  ///< crash file completely, p1=pathid(0, 1..)
#define     MOVREC_EVENT_AUDREC_STOP          35  ///< audio recording stop
#define     MOVREC_EVENT_START                0x80 ///< start recording
#define     MOVREC_EVENT_TIMELAPSE_ONESHOT    0x81 ///< timelapse oneshot
#define     MOVREC_EVENT_SLOWSTART            0x82 ///< slow stop begins
#define     MOVREC_EVENT_ADDONEVIDBS          0x83 ///< add one video BS
#define     MOVREC_EVENT_GETNAME_PATH0        0x84 ///< get custom filename
#define     MOVREC_EVENT_GETNAME_EMRPATH      0x85 ///<
#define     MOVREC_EVENT_VIDEO_INFO_READY     0x86 ///< h264 sps ready, param=pathid
#define     MOVREC_EVENT_IMEDIRECT_STOPH264   0x90
#define     MOVREC_EVENT_IMEDIRECT_IPLNOEND   0x91
#define     MOVREC_EVENT_FILEOUT_WRITEOK      0x92
//#NT#2016/09/20#Bob Huang -begin
//#NT#Support HDMI Display with 3DNR Out
#define     MOVREC_EVENT_3DNR_DISPSTART            0xA0 ///< 3dnr start display
#define     MOVREC_EVENT_3DNR_DISPSTOP             0xA1 ///< 3dnr stop display
#define     MOVREC_EVENT_3DNR_ONE_DISPLAYFRAME     0xA2 ///< 3dnr draw One display frame
#define     MOVREC_EVENT_3DNR_MEDIAINFO_READY      0xA3 ///< 3dnr media information ready to config dispsrv
//#NT#2016/09/20#Bob Huang -end

#define     MOVREC_EVENT_FRAME_INTERVAL            0xB0

#define     MOVREC_SETCRASH_OK              0  ///< last file ro, now ro
#define     MOVREC_SETCRASH_NOLASTFILE      1  ///< no last file
#define     MOVREC_SETCRASH_LASTFILERO      2  ///< last file has been read-only

#define     MOVREC_CRASHTYPE_RO          1
#define     MOVREC_CRASHTYPE_EMR         2
#define     MOVREC_CRASHTYPE_EV1         0x81 //event 1
#define     MOVREC_CRASHTYPE_EV2         0x82 //event 2
#define     MOVREC_CRASHTYPE_EV3         0x83 //event 3
#define     MOVREC_CRASHTYPE_PREV_EMR    0x84
/**
    @name Valid video input YCbCr format

    Valid video input YCbCr format.
*/
//@{
#define MOVREC_JPEG_FORMAT_420    0  ///< Y:Cb:Cr = 4:1:1
#define MOVREC_JPEG_FORMAT_422    1  ///< Y:Cb:Cr = 2:1:1
#define MOVREC_JPEG_FORMAT_444    2  ///< Y:Cb:Cr = 1:1:1
#define MOVREC_JPEG_FORMAT_100    3  ///< Y:Cb:Cr = 4:1:1
//@}


/**
    @name Valid counting type

    Valid counting type for MediaRec_Disk2SecondWithType.
*/
//@{
#define MOVREC_COUNTTYPE_FS    1  ///< get freespace in storage
#define MOVREC_COUNTTYPE_4G    2  ///< using 4G to calculate
#define MOVREC_COUNTTYPE_USER  3  ///< using given size to calculate
//@}

/**
    @name Valid end type

    Valid recording end type. Normal end, cut overlap, or cut till card full.
*/
//@{
#define MOVREC_ENDTYPE_NORMAL             1 ///< normal end, such as press key to stop recording
#define MOVREC_ENDTYPE_CUTOVERLAP         2 ///< cut file and continue recording automatically; overlap the old files (cyclic recording)
#define MOVREC_ENDTYPE_CUT_TILLCARDFULL   3 ///< cut file and continue recording automatically till card full
#define MOVREC_ENDTYPE_MAX                3 ///< maximum number of recording end type
//@}

//#NT#Add media recording source type (for test mode)
/**
    @name Media recording source type

    Media recording video source type: normal, fixed YUV pattern, bitstream;
    Media recording audio source type: normal, fixed PCM pattern.
*/
//@{
#define MOVREC_SRC_NORMAL         0   ///< normal source (video: sensor, audio: mic-in)
#define MOVREC_SRC_FIXED_YUV      1   ///< fixed YUV pattern (for video)
#define MOVREC_SRC_BS_ONLY        2   ///< bitstream only (for video)
#define MOVREC_SRC_FIXED_PCM      1   ///< fixed PCM pattern (for audio)
//@}

/**
    @name Get IPL ready buffer callback event

    Event ID for getting IPL ready buffer callback function.
*/
//@{
#define MOVREC_IPLGET_READYBUF    1   ///< Get IPL ready buffer
//@}

/**
    @name media record information

    Media recording getting information.
*/
//@{
#define     MOVREC_GETINFO_WIDTH_HEIGHT   1   ///< p1: width (out), p2: height (out), p3:id (in)
#define     MOVREC_GETINFO_VIDEO_CODEC    2   ///< p1: videocodec type(out), MEDIAREC_ENC_JPG or others
///< p2: pathid (in), 0:path1 1:path2
///< p3: 0 or activeID
#define     MOVREC_GETINFO_AUDIO_SR       3   ///< p1: audio samplerate(out), AUDIO_SR_32000 or others//2013/01/11 Meg
#define     MOVREC_GETINFO_AUDIO_CHS      4   ///< p1: audio channels(out), 1 or 2//2013/01/11 Meg
#define     MOVREC_GETINFO_AUDIO_BITS     5   ///< p1: audio bits per sample, 16
#define     MOVREC_GETINFO_H264_SPS       6   ///< p1: addr (out), p2: size (out), p3: num(0 or 1)
#define     MOVREC_GETINFO_VIDEO_FPS      7   ///< p1: v fps (out), p3: id (in)
#define     MOVREC_GETINFO_WIDTH_HEIGHT_P2 8  ///< p1: width of Path2 (out), p2: height of Path2(out)
#define     MOVREC_GETINFO_AUDIO_CODEC    9   ///< p1: audio codec type of 1stpath(out), MEDIAREC_ENC_PCM or others
#define     MOVREC_GETINFO_SEAMLESSSEC    10  ///< p1: seamless Sec(out),
#define     MOVREC_GETINFO_RECFORMAT      11  ///< p1: recformat of 1stpath(out), MEDIAREC_AUD_VID_BOTH or others
#define     MOVREC_GETINFO_PATHOPEN       12  ///< p1: pathid (in), p2: onoff(out)

#define     MOVREC_GETINFO_CARDWRITE_P2   13//< p1: cardwrite[path2]
//#define     MOVREC_GETINFO_RECFORMAT      14//< p1: recformat of path1
#define     MOVREC_GETINFO_40_VIDTBR      0x80///< p1: vsid(in) p2: TBR (out) only 4.0
//@}

/**
    @name media audio type information

    Media recording audio type information.
*/
//@{
#define     MOVREC_AUDTYPE_LEFT   1   ///< left
#define     MOVREC_AUDTYPE_RIGHT  2   ///< right
#define     MOVREC_AUDTYPE_STEREO 3   ///< stereo
#define     MOVREC_AUDTYPE_MONO   4   ///< mono
//@}

//#NT#2013/04/17#Calvin Chang#Support Rotation information in Mov/Mp4 File format -begin
/**
    @name media rotation angle information

    Media recording roation angle information.
*/
//@{
#define     MOVREC_MOV_ROTATE_0       0   ///< angle 0
#define     MOVREC_MOV_ROTATE_90      1   ///< angle 90
#define     MOVREC_MOV_ROTATE_180     2   ///< angle 180
#define     MOVREC_MOV_ROTATE_270     3   ///< angle 270
//@}
//#NT#2013/04/17#Calvin Chang -end

/**
    @name raw encoding EVENT ID
    @note callback ID of raw encoder
*/
//@{
#define MOVREC_RAWENC_EVENT_RESULT_OK          1     ///< finish normally
#define MOVREC_RAWENC_EVENT_RESULT_ERR         2     ///< encode error
#define MOVREC_RAWENC_EVENT_RESULT_WRITE_ERR   3     ///< filesystem write fails
#define MOVREC_RAWENC_EVENT_RESULT_DCF_FULL    4     ///< DCF ID full
#define MOVREC_RAWENC_EVENT_RESULT_JPG_OK      5    ///< jpg encoding ok
#define MOVREC_RAWENC_EVENT_EN_IMEOUT          0x10  ///< enable IME output
#define MOVREC_RAWENC_EVENT_DIS_IMEOUT         0x11  ///< disable IME output
//@}

/**
    @name raw encoding results
*/
//@{
#define MOVREC_RAWENC_OK                       0   ///< encoding successfully
#define MOVREC_RAWENC_ERR_NOT_OPEN             1   ///< raw encode task is not opened
#define MOVREC_RAWENC_ERR_BUSY                 2   ///< raw encode task is busy
#define MOVREC_RAWENC_ERR_IMG_NOT_READY        3   ///< YUV raw image is not ready
#define MOVREC_RAWENC_ERR_UNSAFE_TO_WRITE      4   ///< memory may be not enough for writing JPEG file
//@}

#define MOV_AUDIOBR_16000         16000
#define MOV_AUDIOBR_32000         32000
#define MOV_AUDIOBR_48000         48000
#define MOV_AUDIOBR_64000         64000
#define MOV_AUDIOBR_80000         80000
#define MOV_AUDIOBR_96000         96000
#define MOV_AUDIOBR_112000        112000
#define MOV_AUDIOBR_128000        128000
#define MOV_AUDIOBR_144000        144000
#define MOV_AUDIOBR_160000        160000
#define MOV_AUDIOBR_192000        192000

#define MOVREC_GOLFSHOT_FR1        1
#define MOVREC_GOLFSHOT_FR5        5
#define MOVREC_GOLFSHOT_FR15       15
#define MOVREC_GOLFSHOT_FR30       30
#define MOVREC_TIMELAPSE_FR30       30


//2015/11/23 only for Media 4.0
#define     VIDEOENC_CODECTYPE_MAX   2//one path can match two codec,
//but only one do in the same time

#define MOVREC_RECSTATE_NONE   0
#define MOVREC_RECSTATE_START  1
#define MOVREC_RECSTATE_DONE   2

#define MOVREC_VIDEOPES_HEADERLENGTH 14
#define MOVREC_AUDIOPES_HEADERLENGTH 14
#define MOVREC_TS_NAL_LENGTH         6
#define MOVREC_TS_ADTS_LENGTH        0  //add ADTS by audEnc
#define MOVREC_TS_SPSPPS_LENGTH      33
#define MOVREC_TS_PTS_LENGTH         5

#define MOVREC_TS_PACKET_SIZE        188 //(188 = 4 head + 184 padload)
#define MOVREC_TS_HEAD_SIZE  		 4


#define MOVREC_TS_PMT_PID            64
#define MOVREC_TS_PCR_PID            256
#define MOVREC_TS_VIDEO_PID          512
#define MOVREC_TS_AUDIO_PID          513
#define MOVREC_TS_GPS_PID            768
#define MOVREC_TS_THUMBNAIL_PID      769



typedef struct {
	UINT32              uiMemAddr;          ///< starting address for recording
	UINT32              uiMemSize;          ///< size for recording
	MovieRecCallback    *CallBackFunc;      ///< event inform callback
	UINT32              uiRecFormat;        ///< recording type, MEDIAREC_AUD_VID_BOTH
	UINT32              uiVidFrameRate;     ///< recording video frame rate, MEDIAREC_VRATE_30
	UINT32              ui1stFrameQp;       ///< recording video quality
	UINT32              uiCBthreshold;      ///< video frames for callback, usually equals to frame rate
	UINT32              uiMaxWidth;         ///< recording max video width, 1440x1088 or 1280x720
	UINT32              uiMaxHeight;        ///< recording max video height
	UINT32              uiWidth;            ///< now recording video width, 1440x1088/1280x720/640x480/320x240
	UINT32              uiHeight;           ///< now recording video height
	UINT32              uiJpegFormat;
	UINT32              uiAudSampleRate;    ///< recording audio sample rate, AUDIO_SR_8000
	UINT32              uiAudBits;          ///< audio setting, bits per sample, 8 or 16
	UINT32              uiAudChannels;      ///< audio setting, channels, 1 or 2
	UINT32              audioCh;            ///< audio recording channel, AUDIO_CH_LEFT or others//2009/12/22 Meg
	UINT16              ucSeamlessSec;      ///< seamless cut second,
	UINT8               ucEngType;          ///< engine, MOVIEREC_AVIREC, MOVIEREC_MEDIAREC
	UINT8               ucEndType;          ///< end type, CUTOVERLAP, NORMAL
	UINT32              uiNoiseFilterLevel;       // 0: close noise filter
	UINT32              uiNoiseFilterNoiseDecay;  //0: noise*(1/2), 1: noise*(1/4), 2: noise*(1/8)
	UINT8               ucSaveFrameNum; //if memory enough, can be frame number. else, 3,5,6,10,15,30
	UINT8               rev;
	UINT32              uiUserDataAddr;
	UINT32              uiUserDataSize;
	UINT32              uiTargetFramesize;
	UINT32              uiAudioBufferAddr;
	UINT32              uiAudioBufferSize;
	UINT32              uiMaxRecSecs;
	UINT32              uiPanelWidth;       ///< input: panel size, 2011/06/10 Meg
	UINT32              uiPanelHeight;      ///< input: panel size, 2011/06/10 Meg

} MOVIEREC_OBJ;

typedef struct {
	UINT32 Addr;            ///< Memory buffer starting address
	UINT32 Size;            ///< Memory buffer size
	UINT32 RawYAddr;        ///< thisBS Y address
	UINT32 uiSVCSize;       ///< SVC header size
	UINT32 uiTemporalId;    ///< SVC temporal layer ID (0, 1, 2)
	UINT32 uiy_time;        ///< raw create time //2016/08/02
} MEDIAREC_MEM_RANGE, *PMEDIAREC_MEM_RANGE;

typedef struct {
	UINT32 Yaddr;             ///< y addr
	UINT32 UVaddr;            ///< uv addr
	UINT32 Ylot;              ///< y lineoffset
	UINT32 UVlot;             ///< uv lineoffset
} MEDIAREC_YUV_INFO, *PMEDIAREC_YUV_INFO;



typedef struct {
	UINT64     nowFSFreeSpace;      //in: free space
	UINT32     vidfps;              //in: video frame per second
	UINT32     vidTBR;              //in: now video target bit rate
	UINT32     audSampleRate;       //in: audio sample rate
	UINT32     audCodec;            //in: MEDIAREC_ENC_PCM or MEDIAREC_ENC_AAC
	UINT32     audChs;              //in: ahdio channels
	UINT32     clustersize;         //in: now cluster size
	UINT32     gpson;               //in: if gps on
	UINT32     nidxon;              //in: if nidx on
} MEDIAREC_CALCSEC_SETTING;

typedef struct {
	UINT32     bsOutAddr;           //in: bitstream address
	UINT32     bsOutSize;           //in: bitstream size
	UINT32     bsKey;               //in: if key-frame or not

} MEDIAREC_VIDEO_BS_INFO;

typedef struct {
	UINT32      adr;
	UINT32      sze;
	char        name[12];
	UINT32      id;
} MEDIAREC_DUMPMEM_INFO;


/**
    Stucture for MovRec_AddVideoOne()
*/

typedef struct {
	UINT32 width;           //vid width
	UINT32 height;          //vid height
	UINT32 codectype[VIDEOENC_CODECTYPE_MAX];  //MEDIAVIDENC_H264/MEDIAVIDENC_MJPG
	UINT32 targetbitrate;   //Byte per second
	UINT32 vidframerate;    //FPS
	UINT32 writeCard;       //write to card
	UINT32 filetype;        //MEDIA_FILEFORMAT_MOV or others
	UINT32 seamlessOn;      //seamless or not
	UINT32 seamlessSec;     //seamless sec
	UINT32 EMRon;           //EMR on/off
	UINT32 pureStreaming;   //streaming or not
	UINT32 maxwidth;           //max vid width
	UINT32 maxheight;          //max vid height
	UINT32 maxtbr;             //max vid tbr
	UINT32 matchsensor;     //match which sensor, path0 or path1
	UINT32 vsid;            //out, vsid to start/stop
} MOVREC_ADD_INFO;

typedef struct {
	UINT64                 pcrValue;
	UINT64                 emrVidPts;
	UINT64                 emrAudPts;
	UINT32                 vidContinuityCnt;
	UINT32                 audContinuityCnt;
	UINT32                 patContinuityCnt;
	UINT32                 pmtContinuityCnt;
	UINT32                 pcrContinuityCnt;
	UINT32                 gpsContinuityCnt;
	UINT32                 thumbContinuityCnt;
	UINT32                 vidContCntAfterPSI;
	UINT32                 vidFrameCount;
	UINT32                 recordSec;
	BOOL                   bReConfigMuxer;
	BOOL                   bReady2WriteCard;
	BOOL                   bLastFrameMuxed;//for flush
	BOOL                   bFlush2CutFile;
	BOOL                   bReadyToFillPSI;
} SMEDIA_TS_RECORD_INFO;

typedef struct {
	UINT64                 pcrValue;
	UINT32                 vidContinuityCnt;
	UINT32                 audContinuityCnt;
	UINT32                 psiContinuityCnt;
	UINT32                 vidContCntAfterPSI;
	UINT32                 vidFrameCount;
	BOOL                   bReConfigMuxer;
	BOOL                   bReady2WriteCard;
	BOOL                   bLastFrameMuxed;//for flush
	BOOL                   bFlush2CutFile;
	BOOL                   bReadyToFillPSI;
} MOV_TS_RECORD_INFO;
// Audio Filter export API
extern BOOL     AudFilter_Open(UINT32 uiChannels, BOOL bSmooth);
extern BOOL     AudFilter_Close(void);
extern BOOL     AudNoiseReduce_Open(void);
extern BOOL     AudNoiseReduce_Close(void);
typedef ER(MediaRecCB_Start)(UINT32 p1, UINT32 p2, UINT32 p3); //2016/03/17
typedef ER(MediaRecCB_Get)(UINT32 p1, UINT32 p2, UINT32 p3); //2016/03/17

#endif

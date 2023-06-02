/**
 * @file medialib_global.h
 * @brief Define the global public types for media lib, video lib and audio lib
 *
 * Copyright (C) 2020 Anyka (Guangzhou) Microelectronics Technology Co., Ltd.
 * @author Huang Liang
 * @update date 2020-01-07
 * @version 4.0
 */

#ifndef _MEDIA_LIB_GLOBAL_H_
#define _MEDIA_LIB_GLOBAL_H_

#include "anyka_types.h"

typedef struct
{
    T_U16   ResourceID;
    T_U8    *Buff;
    T_U32   Resource_len;
}T_AUDIO_LOADRESOURCE_CB_PARA;

typedef T_VOID (*MEDIALIB_CALLBACK_FUN_PRINTF)(T_pCSTR format, ...);

#if 0
typedef T_S32   SEM_ID;
typedef SEM_ID (*MEDIALIB_CALLBACK_FUN_SEM_CREATE)(T_U32 nSemType, T_U32 nMaxLockCount);
typedef T_BOOL (*MEDIALIB_CALLBACK_FUN_SEM_TAKE)(SEM_ID semID, T_S32 nTimeOut);
typedef T_BOOL (*MEDIALIB_CALLBACK_FUN_SEM_GIVE)(SEM_ID semID);
typedef T_BOOL (*MEDIALIB_CALLBACK_FUN_SEM_FLUSH)(SEM_ID semID);
typedef T_BOOL (*MEDIALIB_CALLBACK_FUN_SEM_DELETE)(SEM_ID semID);
#endif

//typedef T_S32 (*MEDIALIB_CALLBACK_FUN_OPEN)(T_pVOID lpData);
typedef T_S32 (*MEDIALIB_CALLBACK_FUN_READ)(T_S32 hFile, T_pVOID buf, T_S32 size);
typedef T_S32 (*MEDIALIB_CALLBACK_FUN_WRITE)(T_S32 hFile, T_pVOID buf, T_S32 size);
typedef T_S32 (*MEDIALIB_CALLBACK_FUN_SEEK)(T_S32 hFile, T_S32 offset, T_S32 whence); 
typedef T_S32 (*MEDIALIB_CALLBACK_FUN_TELL)(T_S32 hFile);
//typedef T_VOID (*MEDIALIB_CALLBACK_FUN_CLOSE)(T_S32 hFile);

typedef T_pVOID (*MEDIALIB_CALLBACK_FUN_MALLOC)(T_U32 size);
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_FREE)(T_pVOID mem);

typedef T_VOID (*MEDIALIB_CALLBACK_FUN_LOADRESOURCE)(T_AUDIO_LOADRESOURCE_CB_PARA *pPara);  //载入资源
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_RELEASERESOURCE)(T_U8 *Buff);    //释放载入的资源

typedef T_BOOL (*MEDIALIB_CALLBACK_FUN_RTC_DELAY) (T_U32 ulTicks);

//typedef T_VOID    (*MEDIALIB_CALLBACK_FUN_SHOW_FRAME)(T_pDATA srcImg, T_U16 src_width, T_U16 src_height);

typedef T_pVOID (*MEDIALIB_CALLBACK_FUN_DMA_MEMCPY)(T_pVOID dest, T_pCVOID src, T_U32 size);
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_MMU_INVALIDATEDCACHE)(void);
typedef T_BOOL (*MEDIALIB_CALLBACK_FUN_CHECK_DEC_BUF)(T_pDATA pBuf);

typedef T_S32 (*MEDIALIB_CALLBACK_FUN_FILE_HANDLE_EXIST)(T_S32 hFile);
typedef T_U32 (*MEDIALIB_CALLBACK_FUN_FILE_GET_LENGTH)(T_S32 hFile);

//just for audio codec lib
typedef T_S32 (*MEDIALIB_CALLBACK_FUN_CMMBSYNCTIME)(T_VOID *pHandle, T_U32 timestamp);
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_CMMBAUDIORECDATA)(T_VOID *pHandle, T_U8 *buf, T_S32 len);
//end of just for audio codec lib

typedef T_BOOL (*MEDIALIB_CALLBACK_FUN_FILESYS_ISBUSY)(void);

typedef T_pVOID (*MEDIALIB_CALLBACK_FUN_DMA_MALLOC)(T_U32 size);
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_DMA_FREE)(T_pVOID mem);

typedef T_pVOID (*MEDIALIB_CALLBACK_FUN_VADDR_TO_PADDR)(T_pVOID mem); 
typedef T_U32 (*MEDIALIB_CALLBACK_FUN_MAP_ADDR)(T_U32 phyAddr, T_U32 size); 
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_UNMAP_ADDR)(T_U32 addr, T_U32 size);

//common register operate
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_REG_BITS_WRITE)(T_U32 phyAddr, T_U32 val, T_U32 mask);

//hardware mutex
typedef T_pVOID (*MEDIALIB_CALLBACK_FUN_VIDEO_HW_LOCK)(T_S32 hw_id);
typedef T_S32 (*MEDIALIB_CALLBACK_FUN_VIDEO_HW_UNLOCK)(T_pVOID hLock);

//invalid Dcache
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_INVALID_DCACHE) (T_VOID);
typedef T_VOID (*MEDIALIB_CALLBACK_FUN_FLUSH_DCACHE_RANGE) (T_U32 start, T_U32 size); // start should be 32byte aligned

typedef enum
{
    MEDIALIB_AUDIO_TYPE_UNKNOWN = 0,
    MEDIALIB_AUDIO_TYPE_MIDI ,
    MEDIALIB_AUDIO_TYPE_MP3 ,
    MEDIALIB_AUDIO_TYPE_AMR ,
    MEDIALIB_AUDIO_TYPE_AAC ,
    MEDIALIB_AUDIO_TYPE_WMA ,
    MEDIALIB_AUDIO_TYPE_PCM ,
    MEDIALIB_AUDIO_TYPE_ADPCM_IMA ,
    MEDIALIB_AUDIO_TYPE_ADPCM_MS ,
    MEDIALIB_AUDIO_TYPE_ADPCM_FLASH ,
    MEDIALIB_AUDIO_TYPE_APE ,
    MEDIALIB_AUDIO_TYPE_FLAC ,
    MEDIALIB_AUDIO_TYPE_OGG_FLAC ,
    MEDIALIB_AUDIO_TYPE_RA8LBR ,
    MEDIALIB_AUDIO_TYPE_DRA,
    MEDIALIB_AUDIO_TYPE_OGG_VORBIS,
    MEDIALIB_AUDIO_TYPE_AC3,
    MEDIALIB_AUDIO_TYPE_PCM_ALAW,
    MEDIALIB_AUDIO_TYPE_PCM_ULAW,
    MEDIALIB_AUDIO_TYPE_SBC,
    MEDIALIB_AUDIO_TYPE_MSBC,
    MEDIALIB_AUDIO_TYPE_SPEEX,
    MEDIALIB_AUDIO_TYPE_SPEEX_WB,
    MEDIALIB_AUDIO_TYPE_OPUS,
    MEDIALIB_AUDIO_TYPE_G722
}T_eMEDIALIB_AUDIO_TYPE;

typedef enum
{
    VIDEO_DRV_UNKNOWN = 0,
    VIDEO_DRV_H263,
    VIDEO_DRV_MPEG,
    VIDEO_DRV_FLV263,
    VIDEO_DRV_H264,  //4
    VIDEO_DRV_RV,
    VIDEO_DRV_AVC1, //6
    VIDEO_DRV_MJPEG,
    VIDEO_DRV_MPEG2,
    VIDEO_DRV_H264DMX,
    VIDEO_DRV_H265,  //10
    VIDEO_DRV_HVC1, //11
}T_eVIDEO_DRV_TYPE;

typedef enum
{
    MEDIALIB_ROTATE_0,
    MEDIALIB_ROTATE_90,
    MEDIALIB_ROTATE_180,
    MEDIALIB_ROTATE_270
}T_eMEDIALIB_ROTATE;

typedef enum 
{
    AUDIOLIB_CHIP_UNKNOW,
    AUDIOLIB_CHIP_AK10XX,
    AUDIOLIB_CHIP_AK10XXC,
    AUDIOLIB_CHIP_AK10XXL,
    AUDIOLIB_CHIP_AK10XXT,
    AUDIOLIB_CHIP_AK11XX,
    AUDIOLIB_CHIP_AK37XX,
    AUDIOLIB_CHIP_AK37XXL,
    AUDIOLIB_CHIP_AK37XXC,
    AUDIOLIB_CHIP_AK39XX,
    AUDIOLIB_CHIP_AK98XX,
    AUDIOLIB_CHIP_AK39XXE,
    AUDIOLIB_CHIP_AK10XXT2,
    AUDIOLIB_CHIP_AK39XXEV2,
    AUDIOLIB_CHIP_AK39XXEV3,
    AUDIOLIB_CHIP_AK10XXT3,
    AUDIOLIB_CHIP_AK39XXEV5,
    AUDIOLIB_CHIP_AK37XXD,
    AUDIOLIB_CHIP_AK10XXF
}T_AUDIO_CHIP_ID;

typedef struct
{
    T_U32   m_SampleRate;       //sample rate, sample per second
    T_U16   m_Channels;         //channel number
    T_U16   m_BitsPerSample;    //bits per sample 
}T_AUDIO_OUT_INFO;

typedef struct
{
    T_U16   m_OutWidth;         //output width
    T_U16   m_OutHeight;        //output height
}T_VIDEO_OUT_INFO;

typedef struct
{
    T_S32   real_time;

    union {
        struct{
            T_U32 nCurBitIndex;  //不需要用
            T_U32 nFrameIndex;   //第几帧
        } m_ape;
        struct{
            T_U32 Indx;          //seek位置开始，当前page中有的packet数
            T_U32 offset;        //seek后，当前page剩余需要解码的数据大小
            T_BOOL flag;         //seek标志位，seek时置1
            T_U32  last_granu;   //解码完上一个page时解出的总sample数 
            T_U32  now_granu;    //解码完当前page时接出的总sample数
            T_BOOL is_eos;       //码流结束标志位
            T_U32  re_data;      //seek后，当前page含有不完整packet的大小
            T_U32 pack_no;       //seek的位置在当前page中的第几个packet
            T_U32 list[255];     //记录一个ogg page中每个packet的偏移位置
        }m_speex;
        struct{
            T_U32 Indx;          //seek位置开始，当前page中有的packet数
            T_U32 offset;        //seek后，当前page剩余需要解码的数据大小
            T_BOOL flag;         //seek标志位，seek时置1
            T_U32  last_granu;   //解码完上一个page时解出的总sample数 
            T_U32  now_granu;    //解码完当前page时接出的总sample数
            T_BOOL is_eos;       //码流结束标志位
            T_U32  re_data;      //seek后，当前page含有不完整packet的大小
            T_U32 pack_no;       //seek的位置在当前page中的第几个packet
            T_U32 list[255];     //记录一个ogg page中每个packet的偏移位置
        }m_speexwb;
        struct{
            T_U8    secUse;     //已经读取的section数目
            T_U8    secLen;     //一个page中包含的section数
            T_U8    tmpSec;     //已经解码的section数目
            T_BOOL  is_eos;     //是不是最后一个page
            T_BOOL  is_bos;     //是不是第一个page
            T_U8    endpack;    //当前page中最后一个packet的位置
            //解码出的sample数是一个64位的数，目前只取低32位
            T_U32   gos;        //解码完当前page后解码出总的sample数，低32位
            T_U32   high_gos;   //解码完当前page后解码出总的sample数，高32位，(暂时不用，留给以后需要)
            T_U8    list[255];  //记录一个page中每个section的大小，一个page中最多含有255个section
        }m_vorbis;
        struct{
            T_U32 Indx;         //seek位置开始，当前page中有的packet数
            T_U32 offset;       //seek后，当前page剩余需要解码的数据大小
            T_BOOL flag;        //seek标志位，seek时置1
            T_U32  last_granu;  //解码完上一个page时解出的总sample数 
            T_U32  now_granu;   //解码完当前page时接出的总sample数
            T_BOOL is_eos;      //码流结束标志位
            T_U32  re_data;     //seek后，当前page含有不完整packet的大小
            T_U32 pack_no;      //seek的位置在当前page中的第几个packet
            T_U32 list[255];    //记录一个ogg page中每个packet的偏移位置
        }m_opus;
    }m_Private;
}T_AUDIO_SEEK_INFO;

#endif//_MEDIA_LIB_GLOBAL_H_

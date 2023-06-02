/**
 * @file      sdfilter.h
 * @brief    Anyka Sound Device Module interfaces header file.
 *
 * This file declare Anyka Sound Device Module interfaces.\n
 * Copyright (C) 2008 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author    Deng Zhou
 * @date    2008-04-10
 * @version V0.0.1
 * @ref
 */

#ifndef __SOUND_FILTER_H__
#define __SOUND_FILTER_H__

#include "medialib_global.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup Audio Filter library
 * @ingroup ENG
 */
/*@{*/

/* @{@name Define audio version*/
/** Use this to define version string */   
/* 注意：如果结构体有修改，则必须修改中版本号 */
#define AUDIO_FILTER_VERSION_STRING        (T_U8 *)"AudioFilter Version V1.13.09"
/** @} */

   
#define    _SD_VOLCTL_VOLDB_Q  10
#define    _SD_EQ_MAX_BANDS    10
#define    _SD_MDRC_MAX_BANDS  4
#define    _SD_REECHO_DECAY_Q  10

typedef enum
{
    _SD_FILTER_UNKNOWN ,
    _SD_FILTER_EQ ,
    _SD_FILTER_WSOLA ,
    _SD_FILTER_RESAMPLE,
    _SD_FILTER_3DSOUND,
    _SD_FILTER_DENOICE,
    _SD_FILTER_AGC,
    _SD_FILTER_VOICECHANGE,
    _SD_FILTER_PCMMIXER,
    _SD_FILTER_3DENHANCE,
    _SD_FILTER_MVBASS,
    _SD_FILTER_ASLC,
    _SD_FILTER_TONE_DETECTION,
    _SD_FILTER_VOLUME_CONTROL,
    _SD_FILTER_REECHO,
    _SD_FILTER_MDRC,
    _SD_FILTER_DEVOCAL,
    _SD_FILTER_TYPE_MAX
}T_AUDIO_FILTER_TYPE;

typedef enum
{
    _SD_EQ_MODE_NORMAL,
    _SD_EQ_MODE_CLASSIC,
    _SD_EQ_MODE_JAZZ,
    _SD_EQ_MODE_POP,
    _SD_EQ_MODE_ROCK,
    _SD_EQ_MODE_EXBASS,
    _SD_EQ_MODE_SOFT,
    _SD_EQ_USER_DEFINE,
} T_EQ_MODE;

//to define the filter type
typedef enum
{
    FILTER_TYPE_NO ,
    FILTER_TYPE_HPF ,
    FILTER_TYPE_LPF ,
    FILTER_TYPE_HSF ,
    FILTER_TYPE_LSF ,
    FILTER_TYPE_PF1    //PeaKing filter
}T_EQ_FILTER_TYPE;


typedef enum
{
    _SD_WSOLA_0_5 ,
    _SD_WSOLA_0_6 ,
    _SD_WSOLA_0_7 ,
    _SD_WSOLA_0_8 ,
    _SD_WSOLA_0_9 ,
    _SD_WSOLA_1_0 ,
    _SD_WSOLA_1_1 ,
    _SD_WSOLA_1_2 ,
    _SD_WSOLA_1_3 ,
    _SD_WSOLA_1_4 ,
    _SD_WSOLA_1_5 ,
    _SD_WSOLA_1_6 ,
    _SD_WSOLA_1_7 ,
    _SD_WSOLA_1_8 ,
    _SD_WSOLA_1_9 ,
    _SD_WSOLA_2_0 
}T_WSOLA_TEMPO;

typedef enum
{
    _SD_WSOLA_ARITHMATIC_0 , // 0:WSOLA, fast but tone bab
    _SD_WSOLA_ARITHMATIC_1   // 1:PJWSOLA, slow but tone well
}T_WSOLA_ARITHMATIC;


typedef enum
{
    RESAMPLE_ARITHMETIC_0 = 0,  // 0: 音质好，只能固定采样率之间采样
    RESAMPLE_ARITHMETIC_1         // 1: 音质差，可以任意采样率之间采样
}RESAMPLE_ARITHMETIC;

typedef enum
{
    _SD_OUTSR_UNKNOW = 0,
    _SD_OUTSR_48KHZ = 1,
    _SD_OUTSR_44KHZ,
    _SD_OUTSR_32KHZ,
    _SD_OUTSR_24KHZ,
    _SD_OUTSR_22KHZ,
    _SD_OUTSR_16KHZ,
    _SD_OUTSR_12KHZ,
    _SD_OUTSR_11KHZ,
    _SD_OUTSR_8KHZ
}T_RES_OUTSR;

typedef enum
{
    PITCH_NORMAL = 0,
    PITCH_CHILD_VOICE ,
    PITCH_MACHINE_VOICE,
    PITCH_ECHO_EFFECT,
    PITCH_ROBOT_VOICE,
    PITCH_RESERVE
}T_PITCH_MODES;

typedef enum
{
    VOLCTL_VOL_MUTIPLE = 0,
    VOLCTL_VOL_DB = 2,
}VOLCTL_VOL_MODE;

typedef struct
{
    int num;
    struct 
    {
        int x;
        int y;
    }stone[10];
    int lookAheadTime;  //ms
    int gainAttackTime;  //ms
    int gainReleaseTime;  //ms
}T_FILTER_MILESTONE;

typedef struct
{
    T_U8  bands;
    /* 
     whether need to bypass total limiter
     0: do total limit; 
     1: bypass total limit
    */
    T_U8  limiterBypass;
    /* 
     whether does bandi Chorus to output  
     0: bandi chorus
     1: bandi doff 
    */
    T_U8  bandiDoff[_SD_MDRC_MAX_BANDS];
    /* 
     whether does bandi's DRC bypass
     0: do bandi's DRC; 
     1: bypass bandi's DRC
    */
    T_U8  bandiDrcBypass[_SD_MDRC_MAX_BANDS];

    /* set boundary freqs */
    T_U32 boundaryFreqs[_SD_MDRC_MAX_BANDS-1];

    // define bands' drc para
    int bandsLookAheadTime;  //ms
    struct
    {
        struct 
        {
            int x;
            int y;
        }stone[2];
        int gainAttackTime;  //ms
        int gainReleaseTime;  //ms
    }drcband[_SD_MDRC_MAX_BANDS];

    // define output limiter para
    struct
    {
        struct 
        {
            int x;
            int y;
        }stone[2];
        int lookAheadTime;  //ms
        int gainAttackTime;  //ms
        int gainReleaseTime;  //ms
    }drctotal;
}T_FILTER_MDRC_PARA;


typedef struct
{
    MEDIALIB_CALLBACK_FUN_MALLOC                Malloc;
    MEDIALIB_CALLBACK_FUN_FREE                  Free;
    MEDIALIB_CALLBACK_FUN_PRINTF                printf;
    MEDIALIB_CALLBACK_FUN_FLUSH_DCACHE_RANGE    flushDCache;
    MEDIALIB_CALLBACK_FUN_RTC_DELAY             delay;
    MEDIALIB_CALLBACK_FUN_INVALID_DCACHE        invDcache;
}T_AUDIO_FILTER_CB_FUNS;

struct sd_param_eq {
    T_U32 eqmode; // T_EQ_MODE
    
    /* 
    设置总增益值(db)，注意：preGain 赋值形式为 (T_S16)(x.xxx*(1<<10))
    */
    T_S16 preGain;      //-12 <= x.xxx <= 12
    
    // For User Presets
    T_U32 bands;      //1~10
    T_U32 bandfreqs[_SD_EQ_MAX_BANDS];
    /* 
    设置每个频带的增益值，注意：bandgains 赋值形式为 (T_S16)(x.xxx*(1<<10))
    */
    T_S16 bandgains[_SD_EQ_MAX_BANDS];  // -32.0 < x.xxx < 32.0
    /* 
    设置每个频带的Q值，注意：
    1. bandQ赋值形式为 (T_U16)(x.xxx*(1<<10))
    2. bandQ如果设置为0，则采用库内部的默认值为 (T_U16)(1.22*(1<<10))
    3. x.xxx < 采样率/(2*该频带的中心频点), 并且x.xxx值必须小于64.000
    */
    T_U16 bandQ[_SD_EQ_MAX_BANDS];     // q < sr/(2*f)
    T_U16 bandTypes[_SD_EQ_MAX_BANDS]; // T_EQ_FILTER_TYPE

    /*
    在调用"_SD_Filter_SetParam()"函数，改变EQ参数的时候，模式间的平滑过渡参数
    */
    T_U8  smoothEna;   // 0-不做平滑处理； 1-做平滑处理
    T_U16 smoothTime;  //平滑时间(ms)。若设置0，则采用库内部默认值(256.0*1000/采样率)

    /*** for ffeq dc_remove ***/
    T_U8  dcRmEna;
    T_U32 dcfb;

    /*** for EQ aslc ***/
    T_U8  aslcEna;
    T_U16 aslcLevelMax;

    /*** hw specific params ***/
    T_U8  numFrameDescriptor; // frame descriptor 的个数
    T_U16 frameSize; // frame 大小，samples
};
struct sd_param_devocal {
    T_U16 frameSize; // frame 大小，samples
    T_U16 bassFreq;  // 低频
    T_U16 trebleFreq;// 高频
    T_U16 strength; // 1~5, bigger is more cancelling
};
struct sd_param_wsola {
    T_U32 tempo;            // T_WSOLA_TEMPO
    T_U32 arithmeticChoice; // T_WSOLA_ARITHMATIC
};
struct sd_param_3dsound {
    T_U8 is3DSurround;
};
struct sd_param_resample {
    //目标采样率 1:48k 2:44k 3:32k 4:24K 5:22K 6:16K 7:12K 8:11K 9:8K
    T_U32 outSrindex; // T_RES_OUTSR

    //设置最大输入长度(bytes)，open时需要用作动态分配的依据。
    //后面具体调用重采样时，输入长度不能超过这个值
    T_U32 maxinputlen; 

    // 由于outSrindex这个限制只能是enum中的几个，当希望的目标采样率是enum之外的值的时候，用这个参数。
    // 这个参数不是采样率的索引了，直接是目标采样率的值。例如8000， 16000 ...
    // 如果想让这个参数生效，必须设置outSrindex=0
    T_U32 outSrFree; 
    
    T_U32 reSampleArithmetic;
    T_U32 outChannel;
};
struct sd_param_agc {
    T_U16 AGClevel;  // make sure AGClevel < 32767
    /* used in AGC_1 */
    T_U32  max_noise;
    T_U32  min_noise;
    /* used in AGC_2 */
    T_U8  noiseReduceDis;  // 是否屏蔽自带的降噪功能
    T_U8  agcDis;  // 是否屏蔽自带的AGC功能
    /*
    agcPostEna：在agcDis==0的情况下，设置是否真正的AGC2库里面做AGC：
    0：表示真正在库里面做agc，即filter_control出来的数据是已经做好agc的；
    1: 表示库里面只要计算agc的gain值，不需要真正做agc处理；真正的agc由外面的调用者后续处理
    */
    T_U8  agcPostEna;  
    T_U16 maxGain;  // 最大放大倍数
    T_U16 minGain;  // 最小放大倍数
    T_U32 dc_freq;  // hz
    T_U32 nr_range; // 1~300,越低降噪效果越明显
};
struct sd_param_nr {
    T_U32 ASLC_ena;  // 0:disable aslc;  1:enable aslc
    T_U32 NR_Level;  //  0 ~ 4 越大,降噪越狠
};
struct sd_param_pitch {
    T_U32 pitchMode;  // T_PITCH_MODES
    /*
     只在 PITCH_CHILD_VOICE==pitchMode 时，pitchTempo参数生效。
     pitchTempo 参数的范围是[0-10], 0~5提高音调，5正常音调，5~10降低音调
    */
    T_U8      pitchTempo; 
};
struct sd_param_reecho {
    /*是否使能混响效果，1为使能，0为关闭*/
    T_S32 reechoEna;  
    /*
    衰减因子，格式为 (T_S32)(0.xx * (1<<_SD_REECHO_DECAY_Q))
    例如要设置参数为0.32， 则给这个变量赋值 (T_S32)(0.32 * (1<<_SD_REECHO_DECAY_Q))
    */
    T_S32 degree;      //0-无混响效果
    /*
    设置房间大小，建议设置0-300 
    */
    T_U16 roomsize;    //0-采用默认值(71)。
    /*
    设置最长混响时间(ms)，即多长时间后混响消失。
    注意：这个设置的越长，需要的缓冲就越大，所以不建议设置太大。
          一般建议设置1000以内，如果内存足够，也可以设置大些。
          如果内存不够，则减小这个值。
    */
    T_U16 reechoTime;  //0-采用默认值(840)
    /*
    是否需要把原始主声音同时输出。 
    0: 不输出原始主声音，输出的都是反射之后的声音；
    1：主声音和反射之后的声音一起输出。
    */
    T_U8  needMainBody; //0 or 1
};
struct sd_param_3DEnhance {
    /* 
    设置总增益值(db)，
    注意：preGain 赋值形式为 (T_S16)(x.xxx*(1<<10))， 
    限制 -12 <= x.xxx <= 12
    */
    T_S16 preGain;  
    T_S16 cutOffFreq;
    /* 
    设置3D深度，
    注意: depth赋值形式为 (T_S16)(x.xxx*(1<<10)), 
    限制 -1 < x.xxx < 1
    */
    T_S16 depth;   
    /*** for 3D Enhance's aslc, resvered***/
    T_U8   aslcEna;
    T_U16  aslcLevelMax;
};
struct sd_param_mvBass {
    /* 
    设置总增益值(db)，
    注意：preGain 赋值形式为 (T_S16)(x.xxx*(1<<10))， 
    限制 -12 <= x.xxx <= 12
    */
    T_S16 preGain;
    T_S16 cutOffFreq;  
    /* 
    设置增强幅度，
    注意: bassGain 赋值形式为 (T_S16)(x.xxx*(1<<10)), 
    限制 0 < x.xxx < 12
    */
    T_S16 bassGain;
    /*** for MVBass's aslc ***/
    T_U8   aslcEna;
    T_U16  aslcLevelMax;
};
struct sd_param_aslc {
    T_BOOL aslcEna;
    T_U16  aslcLimitLevel;  //限幅阈值
    T_U16  aslcStartLevel;  //要限幅的起始能量
    /* 
    jointChannels:
       0: 两个声道独立增益计算，及独立增益处理输出；
       1: 两个声道合叠加取均值合并，然后计算一个增益，左右声道输出相同的数据；
       2: 两个声道的数据交错混合寻找最大值及增益计算，然后用一个增益值分别对左右声道做增益处理输出。
    */            
    T_U16  jointChannels;

    /*
     maxLenin：设置最大的输入pcm数据长度，字节为单位。
     为了在设置参数的时候，能预先按照最大长度分配好内存。
     解决数据长度发生改变时多次分配内存，导致内存分配失败的问题
    */
    T_U16  maxLenin;  
};
struct sd_param_volumeControl {
    /* 
    set volume mode::
    VOLCTL_VOL_MUTIPLE: 音量值是 volume 的值，即外部传入音量倍数值
    VOLCTL_VOL_DB:      音量值是 voldb 的值， 即外部出入db值
    */
    T_U16 setVolMode;

    /* 
    设置音量倍数值, (T_U16)(x.xx*(1<<10)), x.xx=[0.00~7.99]表示倍数
    建议设置的音量值不要超过1.00*(1<<10)，因为超过可能会导致数据溢出，声音产生失真
    */
    T_U16 volume; 

    /* 
    设置音量DB, 赋值形式为(T_S32)(x.xx*(1<<10)), x.xx=[-60.00~8.00]
    建议设置的音量值不要超过0db，因为超过可能会导致数据溢出，声音产生失真
    若 x.xxx<=-79db, 则输出无声； 若x.xxx>8.0, 可能会导致输出噪音。
    */
    T_S32 voldb;

    /* 为了防止音量变换过程产生pipa音，对音量进行平滑处理，这里设置平滑的过渡时间 */
    T_U16 volSmoothTime;  //ms
};
struct sd_param_toneDetection {
    T_U32 baseFreq;
};
struct sd_param_mdrc {
    /*
     maxLenin：设置最大的输入pcm数据长度，字节为单位。
     为了在设置参数的时候，能预先按照最大长度分配好内存。
     解决数据长度发生改变时多次分配内存，导致内存分配失败的问题
    */
    T_U16  maxLenin;  
};

typedef struct
{
    T_U32    m_Type;             //T_AUDIO_FILTER_TYPE
    T_U32    m_SampleRate;       //sample rate, sample per second
    T_U16    m_Channels;         //channel number
    T_U16    m_BitsPerSample;    //bits per sample 

    union {
        struct sd_param_eq              m_eq;
        struct sd_param_devocal         m_devocal;
        struct sd_param_wsola           m_wsola;
        struct sd_param_3dsound         m_3dsound;
        struct sd_param_resample        m_resample;
        struct sd_param_agc             m_agc;
        struct sd_param_nr              m_NR;
        struct sd_param_pitch           m_pitch;
        struct sd_param_reecho          m_reecho;
        struct sd_param_3DEnhance       m_3DEnhance;
        struct sd_param_mvBass          m_mvBass;
        struct sd_param_aslc            m_aslc;
        struct sd_param_volumeControl   m_volumeControl;
        struct sd_param_toneDetection   m_toneDetection;
        struct sd_param_mdrc            m_mdrc;
    }m_Private;
}T_AUDIO_FILTER_IN_INFO;

typedef struct
{
    const char              *strVersion;
    T_AUDIO_CHIP_ID          chip;
    T_AUDIO_FILTER_CB_FUNS   cb_fun;
    T_AUDIO_FILTER_IN_INFO   m_info;

    const T_VOID            *ploginInfo;
}T_AUDIO_FILTER_INPUT;

typedef struct
{
    T_VOID *buf_in;
    T_U32   len_in;
    T_VOID *meta_in;
    
    T_VOID *buf_out;
    T_U32   len_out;
    T_VOID *meta_out;
    
    T_VOID *buf_in2;  //for mix pcm samples
    T_U32   len_in2;
}T_AUDIO_FILTER_BUF_STRC;

typedef struct
{
    T_AUDIO_FILTER_CB_FUNS cb;
    T_U32    m_Type;
}T_AUDIO_FILTER_LOG_INPUT;

//////////////////////////////////////////////////////////////////////////

/**
 * @brief    获取音效处理库版本信息.
 * @author    Deng Zhou
 * @date    2009-04-21
 * @param    [in] T_VOID
 * @return    T_S8 *
 * @retval    返回音效处理库版本号
 */
T_S8 *_SD_GetAudioFilterVersionInfo(void);

/**
 * @brief    获取音效库版本信息, 包括支持哪些功能.
 * @author  Tang Xuechai
 * @date    2014-05-05
 * @param    [in] T_AUDIO_FILTER_CB_FUNS
 * @return    T_S8 *
 * @retval    返回库版本号
 */
T_S8 *_SD_GetAudioFilterVersions(T_AUDIO_FILTER_CB_FUNS *cb);

/**
 * @brief    检查头文件版本是否与库版本匹配
 * @author  Huang Liang
 * @date    2019-08-09
 * @param   [in] filter_input
 * @return    T_S32
 * @retval    T_TRUE or  T_FALSE
 */
T_S32 _SD_CheckAudioFilterVersion(T_AUDIO_FILTER_INPUT *filter_input);

/**
 * @brief    打开音效处理设备.
 * @author    Deng Zhou
 * @date    2008-04-10
 * @param    [in] filter_input:
 * 音效处理的输入结构
 * @return    T_VOID *
 * @retval    返回音效库内部结构的指针，空表示失败
 */
T_VOID *_SD_Filter_Open(T_AUDIO_FILTER_INPUT *filter_input);

/**
 * @brief    音效处理.
 * @author    Deng Zhou
 * @date    2008-04-10
 * @param    [in] audio_filter:
 * 音效处理内部解码保存结构
 * @param    [in] audio_filter_buf:
 * 输入输出buffer结构
 * @return    T_S32
 * @retval    返回音效库处理后的音频数据大小，以byte为单位
 */
T_S32 _SD_Filter_Control(T_VOID *audio_filter, T_AUDIO_FILTER_BUF_STRC *audio_filter_buf);

/**
 * @brief    关闭音效处理设备.
 * @author    Deng Zhou
 * @date    2008-04-10
 * @param    [in] audio_decode:
 * 音效处理内部解码保存结构
 * @return    T_S32
 * @retval    AK_TRUE :  关闭成功
 * @retval    AK_FALSE :  关闭异常
 */
T_S32 _SD_Filter_Close(T_VOID *audio_filter);

/**
 * @brief    设置音效参数:播放速度,EQ模式.
 *          如果m_SampleRate,m_BitsPerSample,m_Channels三个有1个为0,则不改变任何音效,返回AK_TRUE
 * @author    Wang Bo
 * @date    2008-10-07
 * @param    [in] audio_filter:
 * 音效处理内部解码保存结构
 * @param    [in] info:
 * 音效信息保存结构
 * @return    T_S32
 * @retval    AK_TRUE :  设置成功
 * @retval    AK_FALSE :  设置异常
 */
T_S32 _SD_Filter_SetParam(T_VOID *audio_filter, T_AUDIO_FILTER_IN_INFO *info);

/**
 * @brief    设置ASLC模块的限幅曲线.
 * @author    Tang Xuechai
 * @date    2015-04-17
 * @param    [in] audio_filter: 音效处理内部解码保存结构
 * @param    [in] fmileStones: ASLC的限幅曲线参数，具体参考音频库接口说明文档
 * @return    T_S32
 * @retval    AK_TRUE :  设置成功
 * @retval    AK_FALSE :  设置异常
 */
T_S32 _SD_Filter_SetAslcMileStones(T_VOID *audio_filter, T_FILTER_MILESTONE *fmileStones);

/**
 * @brief     设置ASLC模块的静音检测参数之静音幅度阈值.
 * @author    Tang Xuechai
 * @date    2018-01-22
 * @param    [in] audio_filter: 音效处理内部解码保存结构
 * @param    [in] silenceLevel: 静音阈值，当pcm幅值小于这个数值认为是静音
 * @return    T_S32
 * @retval    AK_TRUE :  设置成功
 * @retval    AK_FALSE :  设置异常
 */
T_S32 _SD_Filter_SetAslcSilenceLevel(T_VOID *audio_filter, T_U32 silenceLevel);

/**
 * @brief     设置ASLC模块的静音检测参数之连续静音时间阈值.
 * @author    Tang Xuechai
 * @date    2018-01-22
 * @param    [in] audio_filter: 音效处理内部解码保存结构
 * @param    [in] silenceTime: 静音时间阈值，当连续静音时间超过这个阈值，认为真正进入静音状态
 * @return    T_S32
 * @retval    AK_TRUE :  设置成功
 * @retval    AK_FALSE :  设置异常
 */
T_S32 _SD_Filter_SetAslcSilenceTime(T_VOID *audio_filter, T_U32 silenceTime);

/**
 * @brief    设置MDRC模块的限幅曲线.
 * @author    Tang Xuechai
 * @date    2017-07-21
 * @param    [in] audio_filter: 音效处理内部解码保存结构
 * @param    [in] fmdrc: MDRC的参数，具体参考音频库接口说明文档
 * @return    T_S32
 * @retval    AK_TRUE :  设置成功
 * @retval    AK_FALSE :  设置异常
 */
T_S32 _SD_Filter_SetMdrcPara(T_VOID *audio_filter, T_FILTER_MDRC_PARA *fmdrc);

/**
 * @brief    快速重采样
 * @author    Tang_Xuechai
 * @date        2013-07-03
 * @param    [in] audio_filter:
 *               音效处理内部解码保存结构
 * @param    [out] dstData 
 *               输出的pcm数据
 * @param    [in] srcData:
 *               输入的pcm数据
 * @param    [in] srcLen 
 *               输入pcm数据的byte数
 * @return    T_S32
 * @retval    >=0 :  重采样后的输出pcm数据的byte数
 * @retval    <0  :  重采样失败
 */
T_S32  _SD_Filter_Audio_Scale(T_VOID *audio_filter, T_S16 dstData[], T_S16 srcData[], T_U32 srcLen);


/**
* @brief    把EQ的频域参数转为时域参数.
* @author    Tang Xuechai
* @date        2015-03-24
* @param    [in] audio_filter:
*           音效处理内部保存结构，即_SD_Filter_Open的返回指针
* @param    [in] info:
*           音效信息保存结构
* @return    T_VOID *
* @retval    返回EQ库内部获取的时域参数的指针，空表示失败
*/
T_VOID *_SD_Filter_GetEqTimePara(T_VOID *audio_filter, T_AUDIO_FILTER_IN_INFO *info);

/**
* @brief    把当前要使用的EQ时域参数传递给EQ库.
* @author    Tang Xuechai
* @date        2015-03-24
* @param    [in] audio_filter:
*           音效处理内部保存结构，即_SD_Filter_Open的返回指针
* @param    [in] peqTime:
*           时域参数指针
* @return    T_S32
* @retval    AK_TRUE :  设置成功
* @retval    AK_FALSE:  设置异常
*/
T_S32 _SD_Filter_SetEqTimePara(T_VOID *audio_filter, T_VOID *peqTime);

/**
* @brief    释放EQ时域参数占用的空间.
* @author    Tang Xuechai
* @date        2015-03-24
* @param    [in] audio_filter:
*           音效处理内部保存结构，即_SD_Filter_Open的返回指针
* @param    [in] peqTime:
*           时域参数指针
* @return    T_S32
* @retval    AK_TRUE :  设置成功
* @retval    AK_FALSE:  设置异常
*/
T_S32 _SD_Filter_DestoryEqTimePara(T_VOID *audio_filter, T_VOID *peqTime);


/**
 * @brief    设置音量控制模块的音量值.
 * @author    Tang Xuechai
 * @date    2015-08-11
 * @param    [in] audio_filter: 音效处理内部解码保存结构
 * @param    [in] volume: 目标音量值。
 *    音量倍数值, (T_U16)(x.xx*(1<<10)), x.xx=[0.00~7.99]表示倍数
 *    建议设置的音量值不要超过1.00*(1<<10)，因为超过可能会导致数据溢出，声音产生失真
 * @return    T_S32
 * @retval    AK_TRUE :  设置成功
 * @retval    AK_FALSE :  设置异常
 */
T_S32 _SD_Filter_SetVolume(T_VOID *audio_filter, T_U16 volume);

/**
 * @brief    设置音量控制模块的音量值.
 * @author    Tang Xuechai
 * @date    2015-08-11
 * @param    [in] audio_filter: 音效处理内部解码保存结构
 * @param    [in] volume: 目标音量DB值。
 *    音量DB, 赋值形式为(T_S32)(x.xx*(1<<10)), x.xx=[-100.00~8.00], 在[-60.00~8.00]之间步长1db有效
 *    建议设置的音量值不要超过0db，因为超过可能会导致数据溢出，声音产生失真
 *    若 x.xxx<=-79db, 则输出无声； 若x.xxx>8.0, 可能会导致输出噪音。
 * @return    T_S32
 * @retval    AK_TRUE :  设置成功
 * @retval    AK_FALSE :  设置异常
 */

T_S32 _SD_Filter_SetVolumeDB(T_VOID *audio_filter, T_S32 volume);

/**
* @brief    设置音效参数:  设置音量变化的平滑时间
* @param    [in] audio_filter:  音效处理内部解码保存结构
* @param    [in] stime: 要设置的平滑时间，单位ms，指无声到0db需要的时间。
* @return      T_S32
* @retval       AK_TRUE :  设置成功
* @retval       AK_FALSE :  设置异常
*/
T_S32 _SD_Filter_Volctl_SetSmoothTime(T_VOID *audio_filter, T_U32 stime);

/**
* @brief    获取音效参数:  获取当前正在生效音量值
* @param    [in] audio_filter:  音效处理内部解码保存结构
* @return      T_S32
* @retval       >=0 :  获取到的音量倍数值
* @retval       <0:     获取失败
*/
T_S32 _SD_Filter_Volctl_GetCurVolume(T_VOID *audio_filter);

const T_VOID *_SD_EQ_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_3DEnhance_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_3DSound_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_ASLC_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_mvBass_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_NR_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_AGC_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_VolCtl_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_WSOLA_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_pitch_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_Mixer_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_Reecho_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_Resample_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_toneDetection_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_MDRC_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);
const T_VOID *_SD_Devocal_login(T_AUDIO_FILTER_LOG_INPUT *plogInput);

#ifdef __cplusplus
}
#endif

#endif
/* end of sdfilter.h */
/*@}*/

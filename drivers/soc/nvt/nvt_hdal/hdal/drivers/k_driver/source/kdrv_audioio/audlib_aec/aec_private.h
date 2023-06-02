#ifndef _AUD_AEC_API_H_
#define _AUD_AEC_API_H_

#define AEC_QCONST16(x,bits) ((short)(.5+(x)*(((int)1)<<(bits))))


typedef struct _ST_AUD_AEC_INFO
{
    int u32FrameSize;       //Number of samples in a frame
    int u32FilterLen;       //The length of adaptive impulse response.
    int u32NumMic;          //Number of microphones.
    int u32NumSpeaker;      //Number of speakers.
    int u32SamplingRate;    //Sampling rate of processed signal.
	int u32SpkrMixIn;       //Boolean param for mixing stereo speaker to mono or not before AEC. if enable , u32SpkrDualMono must be 0.
	int u32SpkrDualMono;    //Boolean param for dual mono speaker or not. if enable, u32SpkrMixIn must be 0.
} ST_AUD_AEC_INFO, *PST_AUD_AEC_INFO;

typedef struct _ST_AUD_AEC_RTN
{
    int u32MicBufSize;              //Return Input buffer size
    int u32EchoBufSize;             //Return Input buffer size
    int u32OutBufSize;              //Return Output buffer size
    int u32InternalBufSize;         //Return AEC internal buffer size
} ST_AUD_AEC_RTN, *PST_AUD_AEC_RTN;

typedef struct _ST_AUD_AEC_PRELOAD
{
    int u32PreloadEnable;           //Enable/Disable preload funtion (set by user)
    int u32ForegroundSize;          //Return foreground filter size
    short *ps16Foreground;          //Return foreground filter coef
    int u32BackgroundSize;          //Return background filter size
    int *ps32Background;            //Return background filter coef
} ST_AUD_AEC_PRELOAD, *PST_AUD_AEC_PRELOAD;


typedef enum _EN_AUD_AEC_PARAMS
{
    /*AEC*/
    EN_AUD_AEC_ENABLE_AR,               //Enable amplitude rate function.
    EN_AUD_AEC_AMP_RATE,                //Enlarge mic/speaker signal.   (default=1)
    EN_AUD_AEC_PREEMPH,                 //Parameter for high pass filter. The bigger of this param, the higher gain of high frequency band. (default=0.9)
    EN_AUD_AEC_NOTCH_RADIUS,            //Parameter for DC filter. The smaller of this param, the faster of the convergency rate. (default=0.9 for 8k/ default=0.992 for 48k)
    EN_AUD_AEC_LEAK_ESTIMATE,           //Means the proportion of leaked echo. Note that the setting works only when leak estimate is enabled. (default=0.25)
    EN_AUD_AEC_DISABLE_LEAK_ESTIMTAE,   //Disable leak estimate. (1:disable 0:enable) (default=0)
    EN_AUD_AEC_DISABLE_DC_FILTER,       //Disable DC filter for mic signal.  (1:disable 0:enable) (default=0)
    EN_AUD_AEC_DISABLE_ECHO_SMOOTH,     //Disable smoothing echo signal.  (1:disable 0:enable) (default=0)
    EN_AUD_AEC_ECHO_END,

    /*Noise Suppression*/
    EN_AUD_AEC_BANK_SCALE,              //Band scale method, 1: linear scale  0: bark scale (default=1 (linear))
    EN_AUD_AEC_NOISE_SUPPRESS,          //Noise suppression level (defualt=-15)
    EN_AUD_AEC_ECHO_SUPPRESS,           //Echo suppression level (defualt=-40)
    EN_AUD_AEC_ECHO_SUPPRESS_ACTIVE,    //Echo suppression level (defualt=-15)
    EN_AUD_AEC_ECHO_NOISE_RATIO,        //Means the proportion of depending on the old echo infornmation. (default=0.8)
    EN_AUD_AEC_NLP_ENABLE,              //NLP enable (defualt=1)
    EN_AUD_AEC_PARAMS_TOTAL
} EN_AUD_AEC_PARAMS;

typedef enum _EN_AUD_AEC_ERR
{
    EN_AUD_AEC_ENOERR,      //No error.
    EN_AUD_AEC_EINITFAIL,   //AUD_AEC_Init fail.
    EN_AUD_AEC_EINVALCMD,   //AUD_AEC_SetParam fail due to invalid command.
    EN_AUD_AEC_ERR_TOTAL
} EN_AUD_AEC_ERR;


void AUD_AEC_PreInit(PST_AUD_AEC_INFO pstAecInfo, PST_AUD_AEC_RTN pstAecRtn);
int AUD_AEC_Init(void *pInternalBuf, int u32BufSize, PST_AUD_AEC_PRELOAD pstAecPreload);
void AUD_AEC_Run(short *ps16MicBuf, short *ps16SpeakerBuf, short *ps16OutBuf, short s16DisNoiseSuppr, PST_AUD_AEC_PRELOAD pstAecPreload);
int AUD_AEC_SetParam(EN_AUD_AEC_PARAMS enParamsCMD, void *pParamsValue);
int AUD_AEC_GetVersion(void);

#endif //#ifndef _AUD_AEC_API_H_

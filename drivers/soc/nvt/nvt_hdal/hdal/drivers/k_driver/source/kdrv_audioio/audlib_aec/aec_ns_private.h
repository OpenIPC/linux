#ifndef _AUD_NS_API_H_
#define _AUD_NS_API_H_

#define NS_QCONST16(x,bits) ((short)(.5+(x)*(((int)1)<<(bits))))


typedef struct _ST_AUD_NS_INFO
{
    int s32FrameSize;       //Number of samples in a frame
    int s32ChannelNum;      //Number of channels
    int s32SamplingRate;    //Sampling rate of processed signal.
} ST_AUD_NS_INFO, *PST_AUD_NS_INFO;

typedef struct _ST_AUD_NS_RTN
{
    int u32InBufSize;               //Return Input buffer size
    int u32OutBufSize;              //Return Output buffer size
    int u32InternalBufSize;         //Return NS internal buffer size
} ST_AUD_NS_RTN, *PST_AUD_NS_RTN;

typedef enum _EN_AUD_NS_PARAMS
{
    EN_AUD_NS_BANK_SCALE,               //Band scale method, 1: linear scale  0: bark scale (default=1 (linear))
    EN_AUD_NS_NOISE_SUPPRESS,           //Noise suppression level (defualt=-15)
    EN_AUD_NS_PARAMS_TOTAL
} EN_AUD_NS_PARAMS;

typedef enum _EN_AUD_NS_ERR
{
    EN_AUD_NS_ENOERR,       //No error.
    EN_AUD_NS_EINITFAIL,    //AUD_AEC_Init fail.
    EN_AUD_NS_EINVALCMD,    //AUD_AEC_SetParam fail due to invalid command.
    EN_AUD_NS_ERR_TOTAL
} EN_AUD_NS_ERR;


void AUD_NS_PreInit(PST_AUD_NS_INFO pstNsInfo, PST_AUD_NS_RTN pstNsRtn);
int AUD_NS_Init(void *pInternalBuf, int u32BufSize);
void AUD_NS_Run(short *ps16InBuf, short *ps16OutBuf);
int AUD_NS_SetParam(EN_AUD_NS_PARAMS enParamsCMD, void *pParamsValue);
//int AUD_NS_GetVersion(void);

#endif //#ifndef _AUD_NS_API_H_

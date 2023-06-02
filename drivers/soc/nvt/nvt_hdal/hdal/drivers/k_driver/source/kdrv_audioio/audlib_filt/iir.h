#ifndef __IIR_H__
#define __IIR_H__

/*******************************************************************************
*  Common Definition
*******************************************************************************/
typedef signed char         s8;
typedef signed int          s32;
typedef unsigned int        u32;
typedef short int           s16;
typedef unsigned short int  u16;

#ifdef WIN32
typedef unsigned _int64     u64;
#else
typedef unsigned long long  u64;
#endif  //WIN32

#ifdef WIN32
typedef _int64              s64;
#else
typedef long long           s64;
#endif  //WIN32

#ifndef TRUE
#define TRUE (0 == 0)
#endif

#ifndef FALSE
#define FALSE (!TRUE)
#endif

//#ifndef BOOL
//typedef int BOOL;
//#endif

#ifndef NULL
#define NULL        0
#endif

/*******************************************************************************
*  IIR Filter Configuration (DO NOT CHANGE)
*******************************************************************************/

#define MAX_NUM_SAMPLES (256)         // Maximum number of sample per frame
#define MAX_NUM_CHS (2)               // Maximum number of channel
#define IIR_MAX_FILTER_ORDER (10)     // Maximum supported number of order in a filter, must be even
#define SECTION_ORDER (2)

#define INPUT_FORMAT (15)             // Input format (Q1.15)

#define COEF_FORMAT (27)              // Coef. format (Q16.16)
#define COEF_IWLEN (32-COEF_FORMAT)    // Coef. format (Q16.16)

#define GAIN_FORMAT (16)              // Gain format (Q16.16)
#define GAIN_IWLEN (32-GAIN_FORMAT)

// Fixed-point conversion function
//#define FLOATING_POINT // for floating point
#ifdef FLOATING_POINT
#define APU_AUD_POST_FXP32(val, iwl)  ((float)val)
#else
#define APU_AUD_POST_FXP32(val, shift)  ((s32)(val*(0x1<<shift)))
#endif

#define AUDFILT_MAX_SECNO  5
#define AUDFILT_MAX_BandNum 12

/*******************************************************************************
*  Structures
*******************************************************************************/
typedef enum _EN_AUD_IIR_TYPE {
	EN_AUD_IIR_TYPE_NOTCH_1,
	EN_AUD_IIR_TYPE_NOTCH_2,
	EN_AUD_IIR_TYPE_NOTCH_3,
	EN_AUD_IIR_TYPE_NOTCH_4,
	EN_AUD_IIR_TYPE_NOTCH_5,
	EN_AUD_IIR_TYPE_HIGH_PASS_1,
	EN_AUD_IIR_TYPE_BAND_STOP_1,
	EN_AUD_IIR_TYPE_LOW_PASS_1,
	EN_AUD_IIR_TYPE_TOTAL
} EN_AUD_IIR_TYPE;

typedef enum _EN_AUD_IIR_CHANNEL {
	EN_AUD_IIR_CHANNEL_MONO = 1,
	EN_AUD_IIR_CHANNEL_STEREO,
	EN_AUD_IIR_CHANNEL_TOTAL
} EN_AUD_IIR_CHANNEL;

typedef struct _ST_AUD_IIR_FILTER_CONFIG {
	EN_AUD_IIR_TYPE enIIRType;      // filter type
	u32 u32FilterOrder;             // order, must be even
#ifdef FLOATING_POINT
	float s32totalGain;
	float as32Coef[IIR_MAX_FILTER_ORDER >> 1][6];
	float as32Gain[IIR_MAX_FILTER_ORDER >> 1];
#else
	s32 s32totalGain;    // total gain (Q16 format)
	s32 as32Coef[IIR_MAX_FILTER_ORDER >> 1][6]; // filter coef. (Q27 format)
	s32 as32Gain[IIR_MAX_FILTER_ORDER >> 1]; // gain of each section (Q16 format)
#endif
} ST_AUD_IIR_FILTER_CONFIG, *PST_AUD_IIR_FILTER_CONFIG;

typedef struct _ST_AUD_IIR_INPUT_CONFIG {
	EN_AUD_IIR_CHANNEL enChannel;
	s16 *ps16IPBufAddr;    // input buffer Address
	s16 *ps16OPBufAddr;    // output buffer Address
	u32 u32Len;         // data len in buffer (bytes)
} ST_AUD_IIR_INPUT_CONFIG, *PST_AUD_IIR_INPUT_CONFIG;

typedef enum {
	AUDFILT_TYPE_BYPASS,     ///< Bypass
	AUDFILT_TYPE_LOWPASS,    ///< Low Pass Filter
	AUDFILT_TYPE_HIGHPASS,   ///< High Pass Filter
	AUDFILT_TYPE_PEAKING,    ///< Peaking Filter
	AUDFILT_TYPE_HIGHSHELF,  ///< High Shelf Filter
	AUDFILT_TYPE_LOWSHELF,   ///< Low Shelf Filter

	AUDFILT_TYPE_NUM,      // Total Number of the filter type
	ENUM_DUMMY4WORD(AUDFILT_FILTTYPE)
} AUDFILT_FILTTYPE;

typedef struct {
	AUDFILT_FILTTYPE FiltType;
	double FiltCoef[6];
} AUDFILT_IIRDESIGN, *PAUDFILT_IIRDESIGN;

/*******************************************************************************
*  Prototype of functions
*******************************************************************************/
BOOL AUD_IIR_Open(EN_AUD_IIR_CHANNEL enIIRChannel, s32 *ps32InternalBufAddr, u32 u32InternalBufsize, BOOL b8EnableSmooth);
BOOL AUD_IIR_Initial(void);

BOOL AUD_IIR_SetParams(EN_AUD_IIR_TYPE enIIRType, PST_AUD_IIR_FILTER_CONFIG pstIIRFilterCfg);
BOOL AUD_IIR_Enable(EN_AUD_IIR_TYPE enIIRType);
BOOL AUD_IIR_Disable(EN_AUD_IIR_TYPE enIIRType);
BOOL AUD_IIR_Main(PST_AUD_IIR_INPUT_CONFIG pstIIRInputCfg);

BOOL AUD_PEQ_SetParams(u32 BandIndex, u32 BandNum, PST_AUD_IIR_FILTER_CONFIG pstIIRFilterCfg);
BOOL AUD_PEQ_Enable(u32 BandIndex, u32 BandNum);
BOOL AUD_PEQ_Main(u32 BandNums, PST_AUD_IIR_INPUT_CONFIG pstIIRInputCfg);

BOOL AUD_IIR_End(void);
s16  AUD_IIR_GetVersion(void);

BOOL FiltDesign_FilterDesign(PAUDFILT_IIRDESIGN pstIIRDesign, double f0, double fs, double dB, double BW);
BOOL FiltDesign_FilterTblFloat2FXP(PAUDFILT_IIRDESIGN pstIIRDesign, s32 *s32DestTableAddr, s32 s32Shift);


#endif //__IIR_H__



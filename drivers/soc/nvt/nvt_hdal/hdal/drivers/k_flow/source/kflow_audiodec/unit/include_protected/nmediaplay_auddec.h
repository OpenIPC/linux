#ifndef _NMEDIARECAUDDEC_H
#define _NMEDIARECAUDDEC_H

#include "nvt_media_interface.h"

/**
    NMP Audio Decoder Maximum Sample Rate
*/
#define  NMI_AUDDEC_MAX_SAMPLERATE						48000

/**
    NMP Audio Decoder Bit-Stream Decode Sample Per One Entry
*/
#define  NMI_AUDDEC_BS_DECODE_SAMPLES					1024

/**
    NMP Audio Decoder Decode Block Count Per Second
*/
#define  NMI_AUDDEC_DECODE_BLOCK_CNT					5		///< 1/5 second per block

/**
    NMP Audio Decoder Bit-Stream Set Maximum Size Of One Block
*/
#define  NMI_AUDDEC_BSSET_MAX							(UINT32) ((NMI_AUDDEC_MAX_SAMPLERATE/NMI_AUDDEC_BS_DECODE_SAMPLES)/NMI_AUDDEC_DECODE_BLOCK_CNT)

typedef enum {
	NMI_AUDDEC_PARAM_ALLOC_SIZE,
	NMI_AUDDEC_PARAM_DECODER_OBJ,
	NMI_AUDDEC_PARAM_MEM_RANGE,
	NMI_AUDDEC_PARAM_MAX_MEM_INFO,      ///< Set max buf info and get calculated buf size
	NMI_AUDDEC_PARAM_MAX_ALLOC_SIZE,
	NMI_AUDDEC_PARAM_DECTYPE,
	NMI_AUDDEC_PARAM_SAMPLERATE,
	NMI_AUDDEC_PARAM_CHANNELS,
	NMI_AUDDEC_PARAM_BITS,
	NMI_AUDDEC_PARAM_RAW_STARADDR,
	NMI_AUDDEC_PARAM_RAW_BUFSIZE,
	NMI_AUDDEC_PARAM_REG_CB,
	NMI_AUDDEC_PARAM_DROP_FRAME,
	NMI_AUDDEC_PARAM_ADTS_EN,
} NMI_AUDDEC_AUDDEC_PARAM;

typedef enum {
	NMI_AUDDEC_ACTION_START,
	NMI_AUDDEC_ACTION_STOP,
	NMI_AUDDEC_ACTION_MAX,
} NMI_AUDDEC_ACTION;

typedef enum {
	NMI_AUDDEC_EVENT_RAW_CB,
	NMI_AUDDEC_EVENT_MAX,
	ENUM_DUMMY4WORD(NMI_AUDDEC_CB_EVENT)
} NMI_AUDDEC_CB_EVENT;

typedef struct {
	UINT32 Addr;            ///< Memory buffer starting address
	UINT32 Size;            ///< Memory buffer size
	UINT32 BufID;         	///< Buffer ID
	UINT64 TimeStamp;       ///< TimeStamp
} NMI_AUDDEC_BS_INFO, *PNMI_AUDDEC_BS_INFO;

typedef struct {
	UINT32 Addr;    ///< Memory buffer starting address
	UINT32 Size;    ///< Memory buffer size
} NMI_AUDDEC_MEM_RANGE, *PNMI_AUDDEC_MEM_RANGE;

/**
    Struct definition of Media Audio Decoder Bit-Stream Source
*/
/*typedef struct {
	UINT32						uiBSAddr;						///< Bit-Stream address
	UINT32						uiBSSize;						///< Bit-Stream size
	UINT32						uiThisFrmIdx;					///< This Frame Index
	UINT32						bThisIsEOF;						///< This End of Frame flag
	UINT32                      PathID;                         ///< Path ID
	UINT32						BufID;							///< Buffer ID
	UINT64						TimeStamp;						///< Timestamp
} NMI_AUDDEC_BS_SRC, *PNMI_AUDDEC_BS_SRC;*/

/**
    Struct definition of Media Audio Decoder Bit-Stream Source Set
*/
/*typedef struct {
	NMI_AUDDEC_BS_SRC			BSSrc[NMI_AUDDEC_BSSET_MAX];
	UINT32						uiThisBSSetSize;				///< UINT: 0.25 second
} NMI_AUDDEC_BSSRC_SET, *PNMI_AUDDEC_BSSRC_SET;*/

/**
    Struct definition of Media Audio Decoder Output RAW Data
*/
typedef struct {
	UINT32                      PathID;                         ///< Path ID
	UINT32                      BufID;                          ///< Memory block ID
	UINT32                      Addr;                           ///< Memory buffer starting address
	UINT32                      Size;                           ///< Memory buffer size
	UINT32                      Occupied;                       ///< Has remaining BS need to be decoded
	UINT64                      TimeStamp;                      ///< Timestamp
} NMI_AUDDEC_RAW_INFO, *PNMI_AUDDEC_RAW_INFO;

/**
    Struct definition of Media Audio Decode Configuration
*/
typedef struct {
	UINT32                      uiAudioDecType;					///< Audio frame type
	UINT32                      uiAudioSampleRate;				///< Audio sample rate
	UINT32                      uiAudioChannels;				///< Audio channels
	UINT32                      uiAudioBits;                    ///< Audio bits
	UINT32                      uiPreDecBSSetNum;				///< Pre-decode BS set number
	UINT32                      uiRawStartAddr;					///< RAW data buffer start address
	UINT32                      uiRawBufSize;					///< RAW data buffer size
	BOOL                        uiADTSHeader;                   ///< audio adts header (for AAC)
} NMI_AUDDEC_CONFIG, *PNMI_AUDDEC_CONFIG;

typedef struct {
	UINT32                      uiAudCodec;                     ///< in: audio codec type
	UINT32                      uiAudChannels;                  ///< in: channels, 1 or 2
	UINT32                      uiAudioSR;                      ///< in: audio sample rate
	UINT32                      uiAudioBits;                    ///< in: bit width, 8 or 16
	BOOL                        bRelease;
    UINT32                      uiDecsize;		                ///< out: dec buf size
} NMI_AUDDEC_MAX_MEM_INFO, *PNMI_AUDDEC_MAX_MEM_INFO;

typedef void (NMI_AUDDEC_CB)(UINT32 event, UINT32 param);

extern void NMP_AudDec_AddUnit(void);

extern UINT32 g_auddec_path_max_count;
#define PATH_MAX_COUNT g_auddec_path_max_count

#endif //_NMEDIARECAUDDEC_H

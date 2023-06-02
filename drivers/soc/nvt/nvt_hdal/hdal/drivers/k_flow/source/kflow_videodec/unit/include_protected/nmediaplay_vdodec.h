#ifndef _NMEDIAPLAYVDODEC_H
#define _NMEDIAPLAYVDODEC_H

#include "nvt_media_interface.h"

#define _USE_COMMON_FOR_RAW               1         ///< use common buffer for decoded raw, user AP needs allocate common buffer for vdodec.

#define NMI_VDODEC_H26X_NAL_MAXSIZE       512       ///< H26X Nal unit type max size

#define RESOLUTION_LIMIT                  0         ///< check YUV width/heigh condition

typedef enum {
	NMI_VDODEC_PARAM_ALLOC_SIZE,
	NMI_VDODEC_PARAM_DECODER_OBJ,       ///< media plugin decoder obj
	NMI_VDODEC_PARAM_MEM_RANGE,
	NMI_VDODEC_PARAM_MAX_MEM_INFO,      ///< Set max buf info and get calculated buf size
	NMI_VDODEC_PARAM_MAX_ALLOC_SIZE,
	NMI_VDODEC_PARAM_CODEC,
	NMI_VDODEC_PARAM_DESC_ADDR,
	NMI_VDODEC_PARAM_DESC_LEN,
	NMI_VDODEC_PARAM_WIDTH,
	NMI_VDODEC_PARAM_HEIGHT,
	NMI_VDODEC_PARAM_GOP,
	NMI_VDODEC_PARAM_RAW_STARADDR,
	NMI_VDODEC_PARAM_RAW_BUFSIZE,
	NMI_VDODEC_PARAM_REG_CB,
	//NMI_VDODEC_PARAM_REFRESH_VDO,       ///< refresh video queue
	NMI_VDODEC_PARAM_DROP_FRAME,
	NMI_VDODEC_PARAM_YUV_SIZE,            ///< get one yuv size
	NMI_VDODEC_PARAM_REFFRM_CB,
	NMI_VDODEC_PARAM_VUI_INFO,
	NMI_VDODEC_PARAM_JPEGINFO,
} NMI_VDODEC_PARAM;

typedef enum {
	NMI_VDODEC_ACTION_START,
	NMI_VDODEC_ACTION_STOP,
	NMI_VDODEC_ACTION_MAX,
} NMI_VDODEC_ACTION;

typedef enum {
	NMI_VDODEC_EVENT_RAW_READY,
	NMI_VDODEC_EVENT_CUR_VDOBS,   ///< callback current used video bitstream address & size
	NMI_VDODEC_EVENT_RELEASE_BS,
	NMI_VDODEC_EVENT_MAX,
	ENUM_DUMMY4WORD(NMI_VDODEC_EVENT)
} NMI_VDODEC_EVENT;

typedef struct {
	UINT32 Addr;    ///< Memory buffer starting address
	UINT32 Size;    ///< Memory buffer size
} NMI_VDODEC_MEM_RANGE, *PNMI_VDODEC_MEM_RANGE;

/**
    Struct definition of Media Video Decoder Bit-Stream Source
*/
/*typedef struct {
	UINT32						uiBSAddr;						///< Bit-Stream address
	UINT32						uiBSSize;						///< Bit-Stream size
	UINT32						uiThisFrmIdx;					///< This Frame Index
	UINT32						bIsEOF;						    ///< End of Frame flag
	UINT32						BufID;							///< Buffer ID
	UINT64						TimeStamp;						///< Timestamp
} NMI_VDODEC_BS_SRC, *PNMI_VDODEC_BS_SRC;*/
typedef struct {
	UINT32						uiBSAddr;						///< Bit-Stream address
	UINT32						uiBSSize;						///< Bit-Stream size
	UINT32						uiBSTotalSizeOneFrm;			///< Bit-Stream size
	UINT32                      uiRawAddr;                      ///< output raw addr (should use common buffer)
	UINT32                      uiRawSize;                      ///< output raw size (should use common buffer)
	UINT32                      uiWidth;                        ///< [out] decoded image width
	UINT32                      uiHeight;                       ///< [out] decoded image height
	UINT32						BsBufID;                        ///< Bs Buffer ID
	UINT32						RawBufID;                       ///< Raw Buffer ID
	UINT32						bIsEOF;						    ///< End of Frame flag
	UINT64						uiThisFrmIdx;					///< This Frame Index
	UINT64						TimeStamp;						///< Timestamp
	UINT32						uiCommBufBlkID;					///< user common buffer blkID
	INT32						WaitMs;					        ///< waiting time ms
} NMI_VDODEC_BS_INFO, *PNMI_VDODEC_BS_INFO;

typedef struct {
	UINT32						uiBSNXTAddr;						///< Bit-Stream address
	UINT32						uiBSNXTSize;						///< Bit-Stream size
	UINT32						uiTotalBsSize;						///< Frame total size
} NMI_VDODEC_BSNXT_INFO, *PNMI_VDODEC_BSNXT_INFO;

/**
    Struct definition of Media Video Decoder Output RAW Data
*/
typedef struct {
	UINT32                      PathID;                         ///< Path ID
	UINT32						BsBufID;                        ///< Bs Buffer ID
	UINT32						RawBufID;                       ///< Raw Buffer ID
	UINT32						uiRawAddr;						///< Raw address
	UINT32						uiRawSize;						///< Raw address
	UINT32                      uiYAddr;                        ///< Y address
	UINT32                      uiUVAddr;                       ///< UV address
	UINT32                      uiWidth;                        ///< [out] decoded image width
	UINT32                      uiHeight;                       ///< [out] decoded image height
	UINT32                      uiYLineOffset;                  ///< [out] decoded image Y line offset
	UINT32                      uiUVLineOffset;                 ///< [out] decoded image UV line offset
	UINT32                      uiYuvFmt;                       ///< [out] decoded yuv format
	UINT32						bIsEOF;						    ///< End of Frame flag
	UINT32                      Occupied;
	UINT64						uiThisFrmIdx;					///< This Frame Index
	UINT64						TimeStamp;					///< Timestamp
	UINT32						uiCommBufBlkID;					///< User Common Buffer BlkID
	BOOL                                            isStop;                                         ///< decoder stop
	BOOL                                            bReleaseBsOnly;
	ER                          dec_er;                         ///< decoded error code
} NMI_VDODEC_RAW_INFO, *PNMI_VDODEC_RAW_INFO;

/**
    Struct definition of Media Video Decode Configuration
*/
typedef struct {
	UINT32  Y_Addr;                                  ///< Image Y address
	UINT32  UV_Addr;                                 ///< Image UV address
} NMI_VDODEC_RAWDATA_SRC, *PNMI_VDODEC_SRC;

typedef struct {
	UINT32                      uiVdoCodec;                     ///< in: codec type
	UINT32                      uiWidth;                  		///< in: user allocate max mem width
	UINT32                      uiHeight;                      	///< in: user allocate max mem height
	BOOL                        bRelease;
    UINT32                      uiDecsize;		                ///< out: dec buf size
} NMI_VDODEC_MAX_MEM_INFO, *PNMI_VDODEC_MAX_MEM_INFO;

typedef void (NMI_VDODEC_CB)(UINT32 event, UINT32 param);

extern void NMP_VdoDec_AddUnit(void);

#endif //_NMEDIAPLAYVDODEC_H
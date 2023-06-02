/**
    Media plug-in for H.265 header

    Media plug-in for H.265 header

    @file       MP_H265ReOrder.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _MP_H265REORDER_H
#define _MP_H265REORDER_H

#include "video_codec_h265.h"

/** \addtogroup */
//@{

#define H265ENC_ENABLE_GOP_30       0   // set 1 to enable GOP 30 for test; set 0 to disable
// => not support anymore! should always set to 0!

#define H265ENC_GOP_2               2    // 2 frames in one GOP (I+1P)
#define H265ENC_GOP_5               5    // 5 frames in one GOP (I+4P)
#define H265ENC_GOP_10              10  // 10 frames in one GOP (for 10fps, I, P only)
#define H265ENC_GOP_11              11  // 11 frames in one GOP (for 55fps)
#define H265ENC_GOP_12              12  // 12 frames in one GOP (for 12fps, 24fps, 48fps)
#define H265ENC_GOP_14              14  // 14 frames in one GOP (for 28fps)
#define H265ENC_GOP_15              15  // 15 frames in one GOP (for 15fps, 30fps)
#define H265ENC_GOP_20              20  // 20 frames in one GOP (for 20fps, I, P only)
#define H265ENC_GOP_30              30  // 30 frames in one GOP (for 30fps)
#define H265ENC_GOP_50              50  // 50 frames in one GOP
#define H265ENC_GOP_60              60  // 60 frames in one GOP
#define H265ENC_GOP_120             120 // 120 frames in one GOP
#define H265ENC_GOP_MAX             300 // Maximum frame number in one GOP
#define H265ENC_GOP_IPONLY          1   // Encode GOP type: I, P only
#define H265ENC_GOP_IPB_1ST13       2   // Encode GOP type: I, P, B, and 1st GOP is 13 frames
#define H265ENC_GOP_IPB_1ST15       3   // Encode GOP type: I, P, B, and 1st GOP is 15 frames (normal)
//#NT#2013/02/25#Calvin Chang#Support four interlaced B/B fields reorder case -begin
#define H265ENC_GOP_IPB_1ST30_4BB   4   // Encode GOP type: I, P, B, and 1st GOP is 30 frames for four interlaced B/B fields
//#NT#2013/02/25#Calvin Chang -end
//#define H265ENC_IMG_BUF_NUM         8   // Encode image buffer number: 8 buffers for video with B frame

#define H265DEC_GOP_10              10  // 10 frames in one GOP (for 10fps, I, P only)
#define H265DEC_GOP_12              12  // 12 frames in one GOP (for 12fps, 24fps and 48 fps)
#define H265DEC_GOP_15              15  // 15 frames in one GOP (for 15fps, 30fps)
#define H265DEC_GOP_20              20  // 20 frames in one GOP (for 20fps, I, P only)
#define H265DEC_GOP_30              30  // 30 frames in one GOP (for 30fps)
#define H265DEC_GOP_MAX             20  // Maximum frame number in one GOP
#define H265DEC_GOP_IPONLY          1   // Decode GOP type: I, P only
#define H265DEC_GOP_IPB_1ST13       2   // Decode GOP type: I, P, B, and 1st GOP is 13 frames
#define H265DEC_GOP_IPB_1ST15       3   // Decode GOP type: I, P, B, and 1st GOP is 15 frames (normal)
//#NT#2013/02/25#Calvin Chang#Support four interlaced B/B fields reorder case -begin
#define H265DEC_GOP_IPB_1ST30_4BB   4   // Decode GOP type: I, P, B, and 1st GOP is 30 frames for four interlaced B/B fields
//#NT#2013/02/25#Calvin Chang -end
//#define H265DEC_IMG_BUF_NUM         8   // Decode image buffer number: 8 buffers for video with B frame

#define H265DEC_FRAME_I             1   // Decode frame type: I
#define H265DEC_FRAME_P             2   // Decode frame type: P
#define H265DEC_FRAME_B             3   // Decode frame type: B

#define H265ENC_FAKE_B_ADDR         0x100001
#define H265DEC_FAKE_B_ADDR         0x100001

typedef struct {
	UINT32  uiYAddr;                ///< Image Y address
	UINT32  uiUVAddr;               ///< Image UV address

} H265_IMG_DATA, *PH265_IMG_DATA;

typedef struct {
	H265_IMG_DATA   ImgData;        ///< Decoded image address
	UINT32          uiFrameType;    ///< H.265 frame type (I, P, or B)
	UINT32          uiOrder;        ///< Image display order
	BOOL            bIsReady;       ///< Ready to be displayed or not

} H265DEC_IMG_QUEUE, *PH265DEC_IMG_QUEUE;

extern void     MP_H265Dec_SetGopNum(UINT32 uiGopNum);
extern UINT32   MP_H265Dec_GetGopNum(void);
extern void     MP_H265Dec_SetGopType(UINT32 uiGopType);
extern UINT32   MP_H265Dec_GetGopType(void);
extern void     MP_H265Dec_ImgQueueInit(UINT32 uiGopType);
//#NT#2012/05/08#Calvin Chang#support Fowrad/Backward mechanism -begin
extern UINT32   MP_H265Dec_AddImgToQueue(PH265_IMG_DATA pImgData, UINT32 uiFrameType, UINT32 uiFrameNum);
//UINT32 MP_H265Dec_AddImgToQueue(PH265_IMG_DATA pImgData, UINT32 uiFrameType)
//#NT#2012/05/08#Calvin Chang -end
extern UINT32   MP_H265Dec_FreeImgFromQueue(UINT32 uiOrder);
extern void     MP_H265Dec_GetReOrderImg(UINT32 uiFrameNum, PH265DEC_IMG_QUEUE pImgQueueData);
//#NT#2012/05/10#Calvin Chang#support Fowrad/Backward mechanism -begin
extern UINT32   MP_H265Dec_Flush_ImgQueue(UINT32 uiFrameNum);
extern void     MP_H265Dec_Reset_ImgQueue_Order(UINT32 uiGopType);
//#NT#2012/05/10#Calvin Chang -end


//@}

#endif

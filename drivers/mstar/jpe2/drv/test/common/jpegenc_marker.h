#ifndef _OMX_JPEGENC_MARKER_H_
#define _OMX_JPEGENC_MARKER_H_

#include <stdio.h>
#include <stdlib.h>
//#include <stdbool.h>

//#include <asm/types.h>
//#include <linux/kernel.h>

#include "sys_MsWrapper_cus_os_msg.h"

#define MAX_QUANT_TABLE         (2)
#define QUANT_TABLE_SIZE        (64)


//---------------------------------------------------------------------------
// Structure and enum.
//---------------------------------------------------------------------------

typedef enum
{
    JPE_IBUF_ROW_MODE   = 0x0,
    JPE_IBUF_FRAME_MODE = 0x1
} JpeInBufMode_e;

typedef enum
{
    JPE_COLOR_PLAN_LUMA     = 0,
    JPE_COLOR_PLAN_CHROMA   = 1,
    JPE_COLOR_PLAN_MAX      = 2
} JPE_COLOR_PLAN;

typedef enum
{
    JPE_RAW_YUYV = 0x0,
    JPE_RAW_YVYU = 0x1,
    JPE_RAW_NV12 = 0x3,
    JPE_RAW_NV21 = 0x4,
} JpeRawFormat_e;

typedef enum
{
    JPE_CODEC_JPEG          = 0x1,
    JPE_CODEC_H263I         = 0x2,
    JPE_CODEC_ENCODE_DCT    = 0x4,
} JpeCodecFormat_e;

typedef enum
{
    JPE_IDLE_STATE          = 0,
    JPE_BUSY_STATE          = 1,
    JPE_FRAME_DONE_STATE    = 2,
    JPE_OUTBUF_FULL_STATE   = 3,
    JPE_INBUF_FULL_STATE    = 4
} JpeState_e;

typedef enum
{
    JPE_IOC_RET_SUCCESS             = 0,
    JPE_IOC_RET_BAD_QTABLE          = 1,
    JPE_IOC_RET_BAD_QP              = 2,
    JPE_IOC_RET_BAD_BITSOFFSET      = 3,
    JPE_IOC_RET_BAD_BANKNUM         = 4,
    JPE_IOC_RET_BAD_INBUF           = 5,
    JPE_IOC_RET_BAD_OUTBUF          = 6,
    JPE_IOC_RET_BAD_NULLPTR         = 7,
    JPE_IOC_RET_BAD_BANKCNT         = 8,
    JPE_IOC_RET_BAD_LASTZZ          = 9,
    JPE_IOC_RET_UNKOWN_COMMAND      = 10,
    JPE_IOC_RET_BAD_VIRTUAL_MEM     = 11,
    JPE_IOC_RET_NEED_DRIVER_INIT    = 12,
    JPE_IOC_RET_FMT_NOT_SUPPORT     = 13,
    JPE_IOC_RET_HW_IS_RUNNING       = 14,
    JPE_IOC_RET_FAIL                = 15
} JPE_IOC_RET_STATUS_e;

typedef struct
{
    u32 u32JpeId;
    u32 nRefYLogAddrAlign[2];
    u32 nRefCLogAddrAlign[2];
    u32 nOutBufSLogAddrAlign;
    u8 nSclHandShakeSupport;
    u8 nCodecSupport;
    u8 nBufferModeSupport;

} JpeCaps_t, *pJpeCaps;

typedef struct
{
    unsigned long nAddr;
    unsigned long nOrigSize;
    unsigned long nOutputSize;
    JpeState_e eState;
} JpeBitstreamInfo_t, *pJpeBitstreamInfo;

typedef struct
{
    unsigned long nAddr;
    unsigned long nSize;
} JpeBufInfo_t, *pJpeBufInfo;

typedef struct
{
    JpeInBufMode_e   eInBufMode;
    JpeRawFormat_e   eRawFormat;
    JpeCodecFormat_e eCodecFormat;

    u32 nWidth;
    u32 nHeight;

    u16 YQTable[64];
    u16 CQTable[64];
    u16 nQScale;

    JpeBufInfo_t InBuf[JPE_COLOR_PLAN_MAX];

    JpeBufInfo_t OutBuf;

    u32 nJpeOutBitOffset;
} JpeCfg_t, *pJpeCfg;


#include "list.h"

typedef enum {
  JPEG_MARKER_SOF0  = 0xFFC0,
  JPEG_MARKER_SOF1  = 0xFFC1,
  JPEG_MARKER_SOF2  = 0xFFC2,
  JPEG_MARKER_SOF3  = 0xFFC3,

  JPEG_MARKER_SOF5  = 0xFFC5,
  JPEG_MARKER_SOF6  = 0xFFC6,
  JPEG_MARKER_SOF7  = 0xFFC7,

  JPEG_MARKER_JPG   = 0xFFC8,
  JPEG_MARKER_SOF9  = 0xFFC9,
  JPEG_MARKER_SOF10 = 0xFFCA,
  JPEG_MARKER_SOF11 = 0xFFCB,

  JPEG_MARKER_SOF13 = 0xFFCD,
  JPEG_MARKER_SOF14 = 0xFFCE,
  JPEG_MARKER_SOF15 = 0xFFCF,

  JPEG_MARKER_DHT   = 0xFFC4,

  JPEG_MARKER_DAC   = 0xFFCC,

  JPEG_MARKER_RST0  = 0xFFD0,
  JPEG_MARKER_RST1  = 0xFFD1,
  JPEG_MARKER_RST2  = 0xFFD2,
  JPEG_MARKER_RST3  = 0xFFD3,
  JPEG_MARKER_RST4  = 0xFFD4,
  JPEG_MARKER_RST5  = 0xFFD5,
  JPEG_MARKER_RST6  = 0xFFD6,
  JPEG_MARKER_RST7  = 0xFFD7,

  JPEG_MARKER_SOI   = 0xFFD8,
  JPEG_MARKER_EOI   = 0xFFD9,
  JPEG_MARKER_SOS   = 0xFFDA,
  JPEG_MARKER_DQT   = 0xFFDB,
  JPEG_MARKER_DNL   = 0xFFDC,
  JPEG_MARKER_DRI   = 0xFFDD,
  JPEG_MARKER_DHP   = 0xFFDE,
  JPEG_MARKER_EXP   = 0xFFDF,

  JPEG_MARKER_APP0  = 0xFFE0,
  JPEG_MARKER_APP1  = 0xFFE1,
  JPEG_MARKER_APP2  = 0xFFE2,
  JPEG_MARKER_APP3  = 0xFFE3,
  JPEG_MARKER_APP4  = 0xFFE4,
  JPEG_MARKER_APP5  = 0xFFE5,
  JPEG_MARKER_APP6  = 0xFFE6,
  JPEG_MARKER_APP7  = 0xFFE7,
  JPEG_MARKER_APP8  = 0xFFE8,
  JPEG_MARKER_APP9  = 0xFFE9,
  JPEG_MARKER_APP10 = 0xFFEA,
  JPEG_MARKER_APP11 = 0xFFEB,
  JPEG_MARKER_APP12 = 0xFFEC,
  JPEG_MARKER_APP13 = 0xFFED,
  JPEG_MARKER_APP14 = 0xFFEE,
  JPEG_MARKER_APP15 = 0xFFEF,

  JPEG_MARKER_JPG0  = 0xFFF0,
  JPEG_MARKER_JPG8  = 0xFFF8,
  JPEG_MARKER_JPG13 = 0xFFFD,
  JPEG_MARKER_COM   = 0xFFFE
} JPEG_MARKER_TYPE;

void omx_jpegenc_marker_init(struct list_head *head);
void omx_jpegenc_marker_release(struct list_head *head);
void* omx_jpegenc_marker_create_app_n(struct list_head *head, JPEG_MARKER_TYPE type, u16 size);
int omx_jpegenc_marker_size(struct list_head *head);
int omx_jpegenc_marker_dump(struct list_head *head, u8 *data);
bool omx_jpegenc_marker_create_baseline(struct list_head *head, bool yuv422, u16 width, u16 height, u16 *dqt_table_y, u16 *dqt_table_c, u16 dqt_scale);

#endif // _OMX_JPEGENC_MARKER_H_

#ifndef _USERJPG_INT_
#define _USERJPG_INT_

#if defined __UITRON || defined __ECOS
#include "JpgHeader.h"

#elif defined __KERNEL__
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/error_no.h"
#include "../../include/jpg_header.h"
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#define debug_msg        printk
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/error_no.h"

#include <string.h>
#include "../../include/jpg_header.h"
#endif


#if defined (__UITRON)
#include "DebugModule.h"
#else
//#include "kwrap/debug.h"
#include "../include/jpeg_dbg.h"
#endif

#define THIS_DBGLVL         NVT_DBG_WARN
#define __MODULE__          jpg_hal
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass


extern unsigned int jpg_hal_debug_level;
///////////////////////////////////////////////////////////////////////////////

#define UINT32_BYTE1(data)      ((UINT8)((((UINT32)(data))) & 0x000000FF))      ///< Get a byte data which is [7:0] of a 32-bit data.
#define UINT32_BYTE2(data)      ((UINT8)((((UINT32)(data))>>8) & 0x000000FF))   ///< Get a byte data which is [15:8] of a 32-bit data.
#define UINT32_BYTE3(data)      ((UINT8)((((UINT32)(data))>>16) & 0x000000FF))  ///< Get a byte data which is [23:16] of a 32-bit data.
#define UINT32_BYTE4(data)      ((UINT8)((((UINT32)(data))>>24) & 0x000000FF))  ///< Get a byte data which is [31:24] of a 32-bit data.
#define UINT32_LOWORD(data)     ((UINT16)((((UINT32)(data))) & 0x0000FFFF))     ///< Get a 16-bit data which is [15:0] of a 32-bit data.
#define UINT32_HIWORD(data)     ((UINT16)((((UINT32)(data))>>16) & 0x0000FFFF)) ///< Get a 16-bit data which is [31:16] of a 32-bit data.
#define UINT16_LOBYTE(data)     ((UINT8)((((UINT32)(data))) & 0x000000FF))      ///< Get a byte data which is [7:0] of a 16-bit data.
#define UINT16_HIBYTE(data)     ((UINT8)((((UINT32)(data))>>8) & 0x000000FF))   ///< Get a byte data which is [15:8] of a 16-bit data.
//#define HiWord(data)            UINT32_HIWORD(data)     ///< Refer to UINT32_HiWord().
//#define LoWord(data)            UINT32_LOWORD(data)     ///< Refer to UINT32_LoWord().
//#define HiByte(data)            UINT16_HIBYTE(data)     ///< Refer to UINT16_HiByte().
//#define LoByte(data)            UINT16_LOBYTE(data)     ///< Refer to UINT16_LoByte().

#define JPEG_MARKER_SOI    0xD8FF

//#NT#2008/11/11#Daniel Lu - begin

//For QV 5.0 AVI
typedef _PACKED_BEGIN struct {
	UINT8           tag_id[2];
	UINT8           tag_length[2];
	UINT8           Comment[14];
} _PACKED_END TAG_CMAVI;    // Start of Scan

/**
    Quicktime version 5 AVI header.
*/
typedef _PACKED_BEGIN struct {
	UINT16        soi;
	TAG_CMAVI     MarkCME0;
	TAG_DQT       mark_dqt;
	TAG_SOF       mark_sof;
	TAG_SOS       mark_sos;
} _PACKED_END QV50AVI_HEADER, *PQV50AVI_HEADER;

//#NT#2007/10/17#Meg Lin -begin
//add MovMjpg
#define  QV5MOVMJPGHDRSIZE    0x299
//FFD8+ FFE0+ FFFE+ FFE1+ FFDB+ FFC4 + FFC0 +FFDA
//0x02+ 0x12+ 0x0E+ 0x2C+ 0x86+ 0x1A4+ 0x13 +0x0E

typedef _PACKED_BEGIN struct {
	UINT8           tag_id[2];
	UINT8           tag_length[2];
	UINT8           Comment[10];
} _PACKED_END TAG_CMFE;    // Start of Scan
//For QV 5.0 AVI
typedef _PACKED_BEGIN struct {
	UINT8           tag_id[2];
	UINT8           tag_length[2];
	UINT8           Comment[8];
	UINT32           FieldSize;
	UINT32           PaddedSize;
	UINT32           Offset2Next;
	UINT8           Offset1[4];
	UINT8           Offset2[4];
	UINT8           Offset3[4];
	UINT8           Offset4[4];
	UINT8           Offset5[4];

} _PACKED_END TAG_CME1;    // Start of Scan


/**
    Quicktime version 5 AVI header.
*/
typedef _PACKED_BEGIN struct {
	UINT16        soi;
	TAG_CMAVI     MarkCME0;
	TAG_CMFE      MarkCMFE;
	TAG_CME1      MarkCME1;
	TAG_DQT       mark_dqt;
	TAG_DHT       mark_dht;
	TAG_SOF       mark_sof;
	TAG_SOS       mark_sos;
} _PACKED_END MOVMJPG_HEADER, *PMOVMJPG_HEADER;

extern const UINT16 gstd_enc_lum_actbl[162];
extern const UINT16 gstd_enc_chr_actbl[162];
extern const UINT16 gstd_enc_lum_dctbl[14];
extern const UINT16 gstd_enc_chr_dctbl[12];

extern const UINT8  grucStdLuxQ[64];
extern const UINT8  grucStdChrQ[64];
extern const UINT8  grucThumbQ[132];
extern JPG_HEADER   JPGHeader;
extern QV50AVI_HEADER QV5AVIHeader;
extern MOVMJPG_HEADER MOVMJPGHeader;

#endif//_USERJPG_INT_

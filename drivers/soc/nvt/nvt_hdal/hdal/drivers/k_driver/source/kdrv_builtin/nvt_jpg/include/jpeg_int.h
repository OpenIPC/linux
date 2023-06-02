/*
    Header file for JPEG internal driver API.

    This file is the header file that define JPEG internal driver API.

    @file       jpeg_int.h
    @ingroup    mIDrvCodec_JPEG
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/

#ifndef _JEPG_INT_H
#define _JEPG_INT_H
#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"
#endif
#include "kwrap/type.h"
#include "jpeg_dbg.h"
/*
    @addtogroup mIDrvCodec_JPEG
*/
//@{

// Macro to read / write JPEG register

//extern UINT32 IOADDR_JPEG_REG_BASE;

//#define JPEG_SETREG(ofs, value)  OUTW(IOADDR_JPEG_REG_BASE+(ofs), (value))
//#define JPEG_GETREG(ofs)        INW(IOADDR_JPEG_REG_BASE+(ofs))

// DISABLE  : Only support JPEG_YUV_FORMAT_100, JPEG_YUV_FORMAT_211 (2h11, 4:2:2), JPEG_YUV_FORMAT_411 (411, 4:2:0)
// ENABLE   : Support all formats
#define JPEG_SUPPORT_ALL_FORMAT     DISABLE

// Macro to get Horizontal or Vertical block number from format F
#define JPEG_GET_YHOR_BLKNUM(F)   (((F) >> 10) & 0x03)    // Get Y's horizontal block number of format F
#define JPEG_GET_YVER_BLKNUM(F)   (((F) >> 8) & 0x03)    // Get Y's vertical   block number of format F
#define JPEG_GET_UHOR_BLKNUM(F)   (((F) >> 6) & 0x03)    // Get U's horizontal block number of format F
#define JPEG_GET_UVER_BLKNUM(F)   (((F) >> 4) & 0x03)    // Get U's vertical   block number of format F
#define JPEG_GET_VHOR_BLKNUM(F)   (((F) >> 2) & 0x03)    // Get V's horizontal block number of format F
#define JPEG_GET_VVER_BLKNUM(F)   (((F)) & 0x03)  // Get V's vertical   block number of format F

// Maximum line offset
#define JPEG_MAX_LINEOFFSET     (131068)

// Minimum and maximum bit stream length
#define JPEG_MIN_BSLENGTH       (256)
#define JPEG_MAX_BSLENGTH       (32*1024*1024-1)

// Maximum width, height (Unit: pixel)
#define JPEG_MAX_W              (65528)     // Width  is 13 bits register in MCU unit. 65528 for MCU Y width  is 8.
#define JPEG_MAX_H              (65528)     // Height is 13 bits register in MCU unit. 65528 for MCU Y height is 8.

// Maximum Crop starting point X, Y (Unit: MCU)
#define JPEG_MAX_CROP_X         (8191)
#define JPEG_MAX_CROP_Y         (8191)

// Maximum Crop width, height (Unit: MCU)
#define JPEG_MAX_CROP_W         (8191)
#define JPEG_MAX_CROP_H         (8191)

// Minimum crop width for MCU100 (Unit: MCU)
#define JPEG_MIN_CROP_W_100     (2)

// Table access
#define JPEG_TBL_ACCESS_DIS     (0)     // Disable table access
#define JPEG_TBL_ACCESS_EN      (1)     // Enable table access

// Table index auto incremental (Write table only)
#define JPEG_TBL_AUTOI_DIS      (0)     // Disable table index auto incremental (By TBL_ADR)
#define JPEG_TBL_AUTOI_EN       (1)     // Enable table index auto incremental

// Register default value
#define JPEG_CTRL_REG_DEFAULT           0x00000000
#define JPEG_CONF_REG_DEFAULT           0x00014000
#define JPEG_RSTR_REG_DEFAULT           0x00000000
#define JPEG_LUTCTRL_REG_DEFAULT        0x00000000
#define JPEG_LUTDATA_REG_DEFAULT        0x00000000
#define JPEG_INTCTRL_REG_DEFAULT        0x00000000
#define JPEG_INTSTS_REG_DEFAULT         0x00000000
#define JPEG_IMGCTRL_REG_DEFAULT        0x00000001
#define JPEG_SLICECTRL_REG_DEFAULT      0x00000000
#define JPEG_IMGSIZE_REG_DEFAULT        0x00000000
#define JPEG_MCUCNT_REG_DEFAULT         0x00000000
#define JPEG_BSCTRL_REG_DEFAULT         0x00000020
#define JPEG_BSDMA_ADDR_REG_DEFAULT     0x00000000
#define JPEG_BSDMA_SIZE_REG_DEFAULT     0x00000000
#define JPEG_BSDMA_CURADDR_REG_DEFAULT  0x00000000
#define JPEG_BSDMA_LEN_REG_DEFAULT      0x00000000
#define JPEG_IMGY_ADDR_REG_DEFAULT      0x00000000
#define JPEG_IMGUV_ADDR_REG_DEFAULT     0x00000000
#define JPEG_IMGY_LOFS_REG_DEFAULT      0x00000000
#define JPEG_IMGUV_LOFS_REG_DEFAULT     0x00000000
#define JPEG_DCY_ADDR_REG_DEFAULT       0x00000000
#define JPEG_DCUV_ADDR_REG_DEFAULT      0x00000000
#define JPEG_DCY_LOFS_REG_DEFAULT       0x00000000
#define JPEG_DCUV_LOFS_REG_DEFAULT      0x00000000
#define JPEG_CROPXY_REG_DEFAULT         0x00000000
#define JPEG_CROPSIZE_REG_DEFAULT       0x00000000
#define JPEG_RHO18_REG_DEFAULT          0x00000000
#define JPEG_RHO14_REG_DEFAULT          0x00000000
#define JPEG_RHO12_REG_DEFAULT          0x00000000
#define JPEG_RHO_REG_DEFAULT            0x00000000
#define JPEG_RHO2_REG_DEFAULT           0x00000000
#define JPEG_RHO4_REG_DEFAULT           0x00000000
#define JPEG_RHO8_REG_DEFAULT           0x00000000
#define JPEG_CYCCNT_REG_DEFAULT         0x00000000
#define JPEG_ERRSTS_REG_DEFAULT         0x00000000
#define JPEG_IMGDMA_BLK_REG_DEFAULT     0x00000000
#define JPEG_BSSUM_REG_DEFAULT          0x00000000
#define JPEG_YUVSUM_REG_DEFAULT         0x00000000

/*
    JPEG table select

    JPEG table select.
*/
typedef enum {
	JPEG_TABSEL_NONE,           // table de-select
	JPEG_TABSEL_QUANT,          // Q-table select
	JPEG_TABSEL_MINCODE,        // mincode table select (decode only)
	JPEG_TABSEL_BASEIDX = 4,    // baseindex table select (decode only)
	JPEG_TABSEL_HUFFMAN = 8,    // huffman table select
	ENUM_DUMMY4WORD(JPEG_TABSEL_MODE)
} JPEG_TABSEL_MODE;

/*
    JPEG encode/decode mode

    JPEG encode/decode mode.
*/
typedef enum {
	JPEG_MODE_ENCODE  = 0,
	JPEG_MODE_DECODE  = 1,
	JPEG_MODE_UNKNOWN = 2,
	ENUM_DUMMY4WORD(JPEG_MODE)
} JPEG_MODE;

/*
    JPEG color componenet count

    JPEG color componenet count.
*/
typedef enum {
	JPEG_COLOR_COMP_1CH,         // Only 1 color component (Y only)
	JPEG_COLOR_COMP_3CH,         // 3 color components (Y/U/V)
	ENUM_DUMMY4WORD(JPEG_COLOR_COMP)
} JPEG_COLOR_COMP;

/*
    JPEG MCU UV format

    JPEG MCU UV format.
*/
typedef enum {
	JPEG_MCU_UV_FMT_1COMP,       // 1U, 1V in a MCU
	JPEG_MCU_UV_FMT_2COMP,       // 2U, 2V in a MCU
	ENUM_DUMMY4WORD(JPEG_MCU_UV_FMT)
} JPEG_MCU_UV_FMT;

/*
    JPEG MCU Y format

    JPEG MCU Y format.
*/
typedef enum {
	JPEG_MCU_Y_FMT_1COMP,        // 1Y in a MCU
	JPEG_MCU_Y_FMT_2COMP,        // 2Y in a MCU
	JPEG_MCU_Y_FMT_4COMP = 3,    // 4Y in a MCU
	ENUM_DUMMY4WORD(JPEG_MCU_Y_FMT)
} JPEG_MCU_Y_FMT;

/*
    JPEG MCU slope

    JPEG MCU slope.
*/
typedef enum {
	JPEG_MCU_SLOPE_HORIZONTAL,   //< Y/U/V in MCU are horizontal
	JPEG_MCU_SLOPE_VERTICAL,     //< Y/U/V in MCU are vertical
	ENUM_DUMMY4WORD(JPEG_MCU_SLOPE)
} JPEG_MCU_SLOPE;

/*
    JPEG image address

    JPEG image (Y,U,V) address.
*/
typedef struct {
	UINT32  yaddr;
	UINT32  uvaddr;
} JPEG_IMG_ADDR;

/*
    JPEG register default value

    JPEG register default value.
*/
typedef struct {
	UINT32  offset;
	UINT32  value;
	CHAR    *pname;
} JPEG_REG_DEFAULT;

typedef UINT32      JPEG_BS_ADDR;

extern void jpeg_reg_init(void);
extern void jpeg_setbaseidx_mincode(UINT8 *phufftab_lumac, UINT8 *phufftab_lumdc, UINT8 *phufftab_chrac, UINT8 *phufftab_chrdc);
extern void jpeg_buildbaseidx_mincode(UINT8 *phuff_tab, UINT8 *pmin_tab, UINT8 *pbase_tab);
//base address
extern void jpeg_set_baseaddr(UINT32 addr);

//@}

#endif

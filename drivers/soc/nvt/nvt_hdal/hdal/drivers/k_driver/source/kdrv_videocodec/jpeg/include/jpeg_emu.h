#ifndef __JPG_EMU_H__
#define __JPG_EMU_H__

#define JPEG_REPEAT_ENCODE_TIME		0

#define SUPPORT_JE_SDE			1
#define SUPPORT_JE_PAD			1
#define SUPPORT_JE_OSG			1
#define DMA_ABORT_INT			1

#define YCC_ERR_MODULE_RST		0
#define DUMP_BS_ERROR_PAT		1
#define DUMP_DEC_ERROR_PAT		1
#if SUPPORT_JE_SDE
	#define DUMP_YCC_ERROR_PAT		0
	#define SUPPORT_YCC_DEBUG		0
	#define TEST_YCC_DECODE_ERROR	0
#else
	#define DUMP_YCC_ERROR_PAT		0
	#define SUPPORT_YCC_DEBUG		0
	#define TEST_YCC_DECODE_ERROR	0
#endif
#if SUPPORT_JE_OSG
	#define SUPPORT_OSG_DEBUG	1
#else
	#define SUPPORT_OSG_DEBUG	0
#endif

// Buffer alignment, must be 2^n - 1
// Bit stream buffer
#define EMU_JPEG_BS_BUF_ALIGN       (0xFF)  // 256 bytes alignment, 0 ~ 255
// Source Y, UV buffer
#define EMU_JPEG_SRC_BUF_ALIGN      (0xFF)  // 256 bytes alignment, 0 ~ 255


// Must sync to IC's spec
#define EMU_JPEG_REG_LENGTH         (0x100)
#define EMU_JPEG_MIN_BS_LEN         (0x100)

// Check with Graphic's SA & DE, this is the limitation of Graphic Engine
#define EMU_JPEG_GRPH_MAX_HEIGHT    ((1<<14) - 1)
#define EMU_JPEG_GRPH_MAX_WIDTH     ((1<<14) - 1)
#define EMU_JPEG_GRPH_MAX_LINEOFF   ((1<<16) - 1)

// DC out mode
typedef enum {
	DC_DISABLE = 0,
	DC_2_2 = 1,     // 1/2 Width, 1/2 Height
	DC_2_4 = 2,     // 1/2 Width, 1/4 Height
	DC_2_8 = 3,     // 1/2 Width, 1/8 Height
	DC_4_2 = 4,     // 1/4 Width, 1/2 Height
	DC_4_4 = 5,     // 1/4 Width, 1/4 Height
	DC_4_8 = 6,     // 1/4 Width, 1/8 Height
	DC_8_2 = 7,     // 1/8 Width, 1/2 Height
	DC_8_4 = 8,     // 1/8 Width, 1/4 Height
	DC_8_8 = 9,     // 1/8 Width, 1/8 Height
	ENUM_DUMMY4WORD(EMU_JPEG_DC_MODE)
} EMU_JPEG_DC_MODE;


// Encode buffer
typedef struct {
	// Register file buffer
	UINT32      uiRegFileBuf;

	// Encoded bit stream buffer
	UINT32      uiBSBuf;

	// Reference bit stream buffer
	UINT32      uiRefBSBuf;

	// Source Y buffer
	UINT32      uiYBuf;

	// Source Y temp buffer for line offset != width
	UINT32      uiTempYBuf;

	// Source UV buffer
	UINT32      uiUVBuf;

	// DC out Y buffer
	UINT32      uiDCOutYBuf;

	// DC out UV buffer
	UINT32      uiDCOutUVBuf;

	// UV packed temp buffer (Temporary solution)
	// Patterns are UV planar, but 650 only support UV pbacked
	// In all supported formats, Y size always >= U+V size.
	// We use source Y buffer for U V planar buffer
	UINT32      uiUPlanarBuf;
	UINT32      uiVPlanarBuf;

	// Reference DC out Y buffer
	UINT32      uiRefDCOutYBuf;

	// Reference DC out UV buffer
	UINT32      uiRefDCOutUVBuf;

	// For reference DC otuput UV packed temp buffer (Temporary solution)
	// Patterns are UV planar, but 650 only support UV pbacked
	// In all supported formats, Y size always >= U+V size.
	// We use reference DC out Y buffer for U V planar buffer
	UINT32      uiRefDCOutUPlanarBuf;
	UINT32      uiRefDCOutVPlanarBuf;

#if SUPPORT_JE_OSG
	UINT32		uiOSGGraphBuf[10];
#endif

	// End of buffer
	UINT32      uiBufEnd;
} EMU_JPEG_ENC_BUFFER;

// JPEG encode register setting
typedef _PACKED_BEGIN struct _JPEG_ENC_REG_DATA {
	UINT8  DC_MOD;

	UINT8  RSTR_EN;
	UINT16 RSTR_NUM;

	UINT8  TPOS_EN;
	UINT8  TPOS_MOD;
	UINT8  FMT_TRAN_EN;

	UINT8  IMG_FMT_COMP;
	UINT8  IMG_FMT_UV;
	UINT8  IMG_FMT_Y;
	UINT8  IMG_FMT_MCU;
	UINT8  UV_PACK_EN;

	UINT16 SRC_XB;  // Unit: Pixel
	UINT16 SRC_YB;  // Unit: Pixel
	UINT16 IMG_XB;
	UINT16 IMG_YB;
	UINT16 IMG_YOFS;
	UINT16 IMG_COFS;

	UINT32 IMG_YSTA;
	UINT32 IMG_CSTA;
	UINT32 CROP_YSTA;
	UINT8  Y_RATIO;

	UINT32 BS_LEN;
	UINT32 FILE_LEN;

	UINT32 RHO_1_8;
	UINT32 RHO_1_4;
	UINT32 RHO_1_2;
	UINT32 RHO;
	UINT32 RHO_2;
	UINT32 RHO_4;
	UINT32 RHO_8;

	UINT32 YUV_CHECKSUM;
	UINT32 BS_CHECKSUM;

} _PACKED_END JPEG_ENC_REG_DATA, *PJPEG_ENC_REG_DATA;

// JPEG encode register file
typedef struct {
	UINT8               QP;
	UINT8               MCUMode;
	JPEG_ENC_REG_DATA   JPEGEncRegData;
} EMU_JPEG_ENC_REG, *PEMU_JPEG_ENC_REG;

#if SUPPORT_JE_SDE
typedef struct {
	UINT32		YccReg[46];
} EMU_JPEG_ENC_YCC_REG;
#endif

#if SUPPORT_JE_PAD
typedef struct {
	UINT32 PAD_X;
	UINT32 PAD_Y;
	UINT32 RESERVED[30];
} EMU_JPEG_ENC_EXTRA_REG;
#endif

#if SUPPORT_JE_OSG
typedef struct {
	UINT32		OsgGraphReg[10][9];	// 0x000 ~ 0x164
	UINT32		OsgCstReg[3];		// 0x488 ~ 0x490
	UINT32		OsgPalReg[16];		// 0x494 ~ 0x4D0
	UINT32		OsgChromaAlpha;		// 0x4D4
} EMU_JPEG_ENC_OSG_REG;
#endif

// Test mode
#define EMU_JPEG_ENCMODE_NORMAL         1       // Normal
#define EMU_JPEG_ENCMODE_RESTART        2       // Restart Marker
#define EMU_JPEG_ENCMODE_TRANSFORM      3       // Transform (Source: 411, Encoded: 2h11)
#define EMU_JPEG_ENCMODE_ROTATE_90      4       // Rotate 90
#define EMU_JPEG_ENCMODE_ROTATE_180     5       // Rotate 180
#define EMU_JPEG_ENCMODE_ROTATE_270     6       // Rotate 270
#define EMU_JPEG_ENCMODE_UNKNOWN        7       // Unknown
#define EMU_JPEG_ENCMODE_DIS            8       // Source Y address offset 0 ~ 7



// MCU format
typedef enum {
	EMU_JPEG_MCU_100 = 1,
	EMU_JPEG_MCU_111,
	EMU_JPEG_MCU_222h,
	EMU_JPEG_MCU_222v,
	EMU_JPEG_MCU_2h11,
	EMU_JPEG_MCU_2v11,
	EMU_JPEG_MCU_422h,
	EMU_JPEG_MCU_422v,
	EMU_JPEG_MCU_411,	// 9
	EMU_JPEG_MCU_411p,	// 10
	EMU_JPEG_MCU_SDE,	// 11
	EMU_JPEG_MCU_PAD,	// 12
	EMU_JPEG_MCU_SDE_PAD,	// 13
	EMU_JPEG_MCU_OSG_BASIC,		// 14
	EMU_JPEG_MCU_SDE_OSG,	// 15
	EMU_JPEG_MCU_OSG_CK = 20,
	EMU_JPEG_MCU_OSG_ALL,
	EMU_JPEG_MCU_OSG_TEST,
	EMU_JPEG_MCU_ERR_SDE = 99,
} EMU_JPEG_MCU;


//decode
// Decode buffer
typedef struct {
	// Register file buffer
	UINT32      uiRegFileBuf;

	// Source bit stream buffer
	UINT32      uiBSBuf;

	// Source header buffer
	UINT32      uiHeaderBuf;

	// Decoded Y buffer
	UINT32      uiYBuf;

	// Decoded UV buffer
	UINT32      uiUVBuf;

	// Reference Y buffer
	UINT32      uiRefYBuf;

	// Reference U buffer
	UINT32      uiRefUBuf;

	// Reference V buffer
	UINT32      uiRefVBuf;

	// End of buffer
	UINT32      uiBufEnd;
} EMU_JPEG_DEC_BUFFER;

// JPEG deode register setting
typedef _PACKED_BEGIN struct _JPEG_DEC_REG_DATA {
	UINT8  IMG_FMT_COMP;
	UINT8  IMG_FMT_UV;
	UINT8  IMG_FMT_Y;
	UINT8  IMG_FMT_MCU;

	UINT8  TPOS_EN;
	UINT8  TPOS_MOD;
	UINT8  RESERVED;

	UINT8  SCAL_EN;
	UINT8  SCAL_MOD;
	UINT16 SCAL_XB; // Scaling width,  Unit: Pixel
	UINT16 SCAL_YB; // Scaling height, Unit: Pixel

	UINT8  CROP_EN;
	UINT16 CROP_STX;// Unit: MCU
	UINT16 CROP_STY;// Unit: MCU
	UINT16 CROP_XB; // Cropping width,  Unit: MCU
	UINT16 CROP_YB; // Cropping height, Unit: MCU

	UINT16 IMG_XB;  // Image width,  Unit: Pixel
	UINT16 IMG_YB;  // Image height, Unit: Pixel
	UINT16 IMG_YOFS;
	UINT16 IMG_COFS;

	UINT32 BS_LEN;
	UINT32 FILE_LEN;
	UINT16 HEADER_LEN;

	UINT32 YUV_CHECKSUM;
	UINT32 BS_CHECKSUM;

} _PACKED_END JPEG_DEC_REG_DATA, *PJPEG_DEC_REG_DATA;

// JPEG deocde register file
typedef struct {
	UINT8               MCUMode;
	JPEG_DEC_REG_DATA   JPEGDecRegData;
} EMU_JPEG_DEC_REG, *PEMU_JPEG_DEC_REG;

// Test mode
#define EMU_JPEG_DECMODE_NORMAL         1       // Normal
#define EMU_JPEG_DECMODE_SCALE1D8       2       // Scale 1/8 (width + height)
#define EMU_JPEG_DECMODE_SCALE1D4       3       // Scale 1/4 (width + height)
#define EMU_JPEG_DECMODE_SCALE1D2       4       // Scale 1/2 (width + height)
#define EMU_JPEG_DECMODE_CROP           5       // Crop
#define EMU_JPEG_DECMODE_CROP_S1D8      6       // Crop + Scale 1/8 (width + height)
#define EMU_JPEG_DECMODE_CROP_S1D4      7       // Crop + Scale 1/4 (width + height)
#define EMU_JPEG_DECMODE_CROP_S1D2      8       // Crop + Scale 1/2 (width + height)
#define EMU_JPEG_DECMODE_ROTATE90       9       // Rotate 90
#define EMU_JPEG_DECMODE_ROTATE180      10      // Rotate 180
#define EMU_JPEG_DECMODE_ROTATE270      11      // Rotate 270
#define EMU_JPEG_DECMODE_ROTATE90_S1D8  12      // Rotate 90  + Scale 1/8 (width + height)
#define EMU_JPEG_DECMODE_ROTATE180_S1D4 13      // Rotate 180 + Scale 1/4 (width + height)
#define EMU_JPEG_DECMODE_ROTATE270_S1D2 14      // Rotate 270 + Scale 1/2 (width + height)
#define EMU_JPEG_DECMODE_SCALE1D2_W     15      // Scale 1/2 width only
#define EMU_JPEG_DECMODE_RESTART        16      // Restart Marker


// Scale mode in register file
#define EMU_JPEG_SCALE_1D8              0       // Scale 1/8
#define EMU_JPEG_SCALE_1D4              1       // Scale 1/4
#define EMU_JPEG_SCALE_1D2              2       // Scale 1/2
#define EMU_JPEG_SCALE_1D2_W            3       // Scale 1/2 width only

#endif

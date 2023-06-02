#ifndef _NMEDIARECIMGCAP_H
#define _NMEDIARECIMGCAP_H


#include "kwrap/type.h"
#define _SECTION(sec)


typedef enum {
	NMI_IMGCAP_4_3_W          	= 640, ///< set YUV info
	NMI_IMGCAP_4_3_H         	= 480, ///< set image capture object
	NMI_IMGCAP_16_9_W        	= NMI_IMGCAP_4_3_W, ///< set max image width
	NMI_IMGCAP_16_9_H        	= 368, ///< set max image height
	ENUM_DUMMY4WORD(NMI_IMGCAP_THUMB_SIZE)
} NMI_IMGCAP_THUMB_SIZE;

typedef enum {
	// set param
	NMI_IMGCAP_SET_YUV          = 0x00, ///< set YUV info
	NMI_IMGCAP_SET_OBJ          = 0x01, ///< set image capture object
	NMI_IMGCAP_SET_MAX_W        = 0x02, ///< set max image width
	NMI_IMGCAP_SET_MAX_H        = 0x03, ///< set max image height
	NMI_IMGCAP_SET_DIR_ENC      = 0x04, ///< set encoding JPEG from source image directly
	NMI_IMGCAP_SET_MEM_RANGE    = 0x05, ///< set memory range
	NMI_IMGCAP_SET_JPG_BUFNUM   = 0x06, ///< set JPEG buffer number
	// get param
	NMI_IMGCAP_GET_JPG_ADDR     = 0x10, ///< get JPEG encoded addr
	NMI_IMGCAP_GET_JPG_SIZE     = 0x11, ///< get JPEG encoded size
	NMI_IMGCAP_ALLOC_SIZE       = 0x12, ///< get allocate size
	// others
	NMI_IMGCAP_REG_CB           = 0x20, ///< register image capture callback
	NMI_IMGCAP_PARAM_MAX_MEM_INFO = 0x21, ///< set mem info and get max need mem size
	ENUM_DUMMY4WORD(NMI_IMGCAP_PARAM)
} NMI_IMGCAP_PARAM;

typedef enum {
	NMI_IMGCAP_JPG_ONLY         = 0x00, ///< do JPEG encode only
	NMI_IMGCAP_WRITE_FILE       = 0x01, ///< do JPEG encode and write file
	NMI_IMGCAP_MODE_MAX,
	ENUM_DUMMY4WORD(NMI_IMGCAP_MODE)
} NMI_IMGCAP_MODE;

typedef enum {
	NMI_IMGCAP_EVENT_ENC_OK     = 0x00, ///< JPEG encode OK callback
	NMI_IMGCAP_EVENT_ENC_NG     = 0x01, ///< JPEG encode NG callback
	NMI_IMGCAP_EVENT_WR         = 0x02, ///< write file callback
	NMI_IMGCAP_EVENT_IPL        = 0x03, ///< IPL trigger callback (for direct mode)
	NMI_IMGCAP_EVENT_STAMP      = 0x04, ///< data stamp callback
	NMI_IMGCAP_EVENT_MAX,
	ENUM_DUMMY4WORD(NMI_IMGCAP_CB_EVENT)
} NMI_IMGCAP_CB_EVENT;

typedef struct {
	UINT32 path_id;
	UINT32 jpg_w;
	UINT32 jpg_h;
	NMI_IMGCAP_MODE mode;
	UINT32 job_id;
	UINT32 thumb;						///< 1: thumbnail
} NMI_IMGCAP_JOB, *PNMI_IMGCAP_JOB;

typedef struct {
	UINT32 path_id;
	UINT32 y_addr;
	UINT32 u_addr;
	UINT32 v_addr;
	UINT32 y_loff;
	UINT32 u_loff;
	UINT32 v_loff;
	UINT32 img_w;
	UINT32 img_h;
	UINT32 fmt;
	UINT32 dar;
	UINT32 thumb;						///< 1: thumbnail
} NMI_IMGCAP_YUV, *PNMI_IMGCAP_YUV;

typedef struct {
	UINT32 path_id;
	UINT32 addr;
	UINT32 size;
	UINT32 job_id;
} NMI_IMGCAP_JPG;

typedef struct {
	UINT32 max_w;
	UINT32 max_h;
} NMI_IMGCAP_INIT;

typedef struct {
	UINT32 Addr;
	UINT32 Size;
} NMI_IMGCAP_MEM_RANGE, *PNMI_IMGCAP_MEM_RANGE;

typedef struct {
	UINT32 uiPxlFmt;    ///< image pixel format (e.g. YUV420)
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiNeedSize;  ///< needed buffer size
	BOOL   bRelease;    ///< release max buf
} NMI_IMGCAP_MAX_MEM_INFO, *PNMI_IMGCAP_MAX_MEM_INFO;

typedef void (NMI_ImgCap_CB)(UINT32 event, UINT32 param);

extern void NMR_ImgCap_AddUnit(void);

#endif //_NMEDIARECIMGCAP_H

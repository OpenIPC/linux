#ifndef _BUILTIN_IPP_DRAWSTR_H_
#define _BUILTIN_IPP_DRAWSTR_H_

#include "kdrv_type.h"

/******* font struct *******/
typedef struct FONT_INFO_ST {
	UINT32 ascii;  //font ascii code
	UINT32 width;  //font width
	UINT32 height;  //font height
	UINT32 bit_depth; //font bit depth
	UINT32 data[98];  //font data
} FONT_INFO;

typedef struct {
	void *ptr;
	UINT32 w;
	UINT32 h;
	UINT32 lofs;
} KDRV_IPP_BUILTIN_DRAW_BUF;

#endif //_BUILTIN_IPP_DRAWSTR_H_

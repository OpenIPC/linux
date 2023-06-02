/**
    Image operation module.

    This module include the image operation like fill image,copy image, scale image, rotate image ...

    @file       gximage.h
    @ingroup    imgtrans

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _GFX_OPEN_H_
#define _GFX_OPEN_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void* gfx_alloc(int size);

extern void  gfx_free(void* buf);

extern void  gfx_memset(void *buf, unsigned char val, int len);

extern void  gfx_memcpy(void *dst, void *src, int len);

extern int   gfx_copy_from_user(void *dst, void *src, int len);

extern int   gfx_copy_to_user(void *dst, void *src, int len);

extern int   gfx_seq_printf(void *m, const char *fmtstr, ...);

extern int   gfx_snprintf(char *buf, int size, const char *fmtstr, ...);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //_GFX_OPEN_H_

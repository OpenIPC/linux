/**
    Image operation module.

    This module include the image operation like fill image,copy image, scale image, rotate image ...

    @file       gximage.h
    @ingroup    imgtrans

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _GFX_INTERNAL_H_
#define _GFX_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int  nvt_gfx_init(void);

extern void nvt_gfx_exit(void);

extern int  nvt_gfx_ioctl(int f, unsigned int cmd, void *arg);

extern int  gfx_info_show(void *m, void *v);

extern int  gfx_cmd_showhelp(void *m, void *v);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //_GFX_INTERNAL_H_

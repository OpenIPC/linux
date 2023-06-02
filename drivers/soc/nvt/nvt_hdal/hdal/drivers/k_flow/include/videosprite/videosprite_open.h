/**
    Image operation module.

    This module supports rendering osds and masks on video frame.

    @file       videosprite.h
    @ingroup    mIAppVideosprite

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEO_SPRITE_OPEN_H_
#define _VIDEO_SPRITE_OPEN_H_

extern void* vds_alloc(int size);

extern void vds_free(void* buf);

extern void vds_memset(void *buf, unsigned char val, int len);

extern void vds_memcpy(void *dst, void *src, int len);

extern int  vds_copy_from_user(void *dst, void *src, int len);

extern int  vds_copy_to_user(void *dst, void *src, int len);

extern int  vds_seq_printf(void *m, const char *fmtstr, ...);

extern int  vds_save_image(char *filename, void *p_addr, int size);

extern int  vds_snprintf(char *buf, int size, const char *fmtstr, ...);

extern int  vds_sprintf(char *buf, const char *fmtstr, ...);

extern int  vds_sscanf(char *buf, const char *fmtstr, ...);

extern int  vds_strcmp(char *s1, char *s2);

#endif //_VIDEO_SPRITE_OPEN_H_

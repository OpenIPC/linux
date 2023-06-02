

#ifndef _AUDLIB_SRC_H__
#define _AUDLIB_SRC_H__

#include "kdrv_type.h"
/**
	Description : Get version number (Ex. 0x0001030c means version 1.3.c
*/
int audlib_src_get_version(void);

/**
	Description : retrun the size of required memory allocation.
	Return : integer.
*/
int audlib_src_pre_init(int ch, int in_count, int out_count, int one_frame_mode);


/**
	Description : initialize audlib_src_run library, mainly
		for memory allocation.
	Return : 0 (Success)
		others (Error)
*/
int audlib_src_init(int *handle, int ch, int in_count, int out_count, int one_frame_mode, short *out_mem);

/**
	Description : main program
	Return : 0 (Success)
		others (error)
*/
int audlib_src_run(int handle, void *p_buffer_in, void *p_buffer_out);

/**
	Description : Ends audlib_src_run library, mainly for
		releasing allocated memory space.
	Return :
*/
void audlib_src_destroy(int handle);


int kdrv_audlib_src_init(void);

#endif

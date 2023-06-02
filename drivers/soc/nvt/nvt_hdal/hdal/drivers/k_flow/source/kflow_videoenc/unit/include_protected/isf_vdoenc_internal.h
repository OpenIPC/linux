/**
    Header file of isf_vdoenc internal header

    @file       isf_vdoenc_int.h

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _ISF_VDOENC_INTERNAL_H
#define _ISF_VDOENC_INTERNAL_H

#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"

#ifndef __KERNEL__
#define __RTOS__TODO__  0
#else
#define __RTOS__TODO__  1
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include <malloc.h>
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)

#define debug_msg 			vk_printk

#include <string.h>
#define DELAY_M_SEC(x)              vos_task_delay_ms(x)
#define DELAY_U_SEC(x)              vos_task_delay_us(x)

#else
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "comm/nvtmem.h"
#else
#include "frammap/frammap_if.h"
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/printk.h>
//#define debug_msg 			vk_printk

#include <linux/delay.h>
#define DELAY_M_SEC(x)              msleep(x) //require delay.h
#define DELAY_U_SEC(x)              udelay(x) //require delay.h
#endif

#define ISF_VDOENC_IN_NUM  16
#define ISF_VDOENC_OUT_NUM 16

extern ISF_UNIT isf_vdoenc;

extern UINT32 g_vdoenc_path_max_count;    // dynamic setting for actual used path count, using PATH_MAX_COUNT to access this variable
#define PATH_MAX_COUNT g_vdoenc_path_max_count

extern UINT32 g_vdoenc_gdc_first;

//process data
extern void _isf_vdoenc_do_input_mask(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out, BOOL b_before_rotate, BOOL b_jpg_yuv_trans);
extern void _isf_vdoenc_do_input_osd(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out, BOOL b_before_rotate, BOOL b_jpg_yuv_trans);
extern void _isf_vdoenc_finish_input_mask(ISF_UNIT *p_thisunit, UINT32 iport);
extern void _isf_vdoenc_finish_input_osd(ISF_UNIT *p_thisunit, UINT32 iport, BOOL b_before_rotate, BOOL b_jpg_yuv_trans);


//sxcmd
extern int _isf_vdoenc_cmd_isfve(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdoenc_cmd_isfve_showhelp(void);

extern int _isf_vdoenc_cmd_vdoenc(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdoenc_cmd_vdoenc_showhelp(void);

extern int _isf_vdoenc_cmd_debug(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdoenc_cmd_trace(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdoenc_cmd_probe(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdoenc_cmd_perf(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdoenc_cmd_save(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdoenc_cmd_isfdbg_showhelp(void);

#endif//_ISF_VDOENC_INTERNAL_H

//@}

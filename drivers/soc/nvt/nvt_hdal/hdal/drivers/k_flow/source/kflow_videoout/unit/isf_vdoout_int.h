/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       isf_vdoout_int.h

    @brief      isf_vdoout internal header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2018/06/04
*/
#ifndef _ISF_VDOOUT_INT_H
#define _ISF_VDOOUT_INT_H
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_videoout/isf_vdoout.h"

#if defined (__LINUX)
#include <linux/module.h>
#include <linux/string.h>
#include <linux/printk.h>
#else
#include <string.h>
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#define msecs_to_jiffies(x)	0
#endif

#define debug_msg 			    vk_printk
#define DELAY_M_SEC(x)          vos_task_delay_ms(x)
#define DELAY_U_SEC(x)          vos_task_delay_us(x)


#define DEV_ID_0		0
#define DEV_ID_1		1
#if defined(_BSP_NA51000_)
#define DEV_ID_MAX          2
#else
#define DEV_ID_MAX          1

#endif


typedef struct _VDOOUT_CONTEXT {
	UINT32 dev;
	UINT32 open_tag;
	UINT32 state;
    UINT32 usr_func;
    UINT32 vnd_func;
    VDOOUT_INFO info;
    VDOOUT_DEV_CFG dev_cfg;
    VDO_FRAME vdo_desc_buf;
} VDOOUT_CONTEXT;

#endif

#define ISF_VDOOUT_INPUT_MAX          1
#define ISF_VDOOUT_OUTPUT_MAX         1
#define ISF_VDOOUT_PATH_NUM           1

extern ISF_RV isf_vdoout_bind_input(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport);
extern ISF_RV isf_vdoout_bind_output(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport);
extern ISF_RV isf_vdoout_do_setportparam(ISF_UNIT  *p_thisunit, UINT32 nport, UINT32 param, UINT32 value);
extern UINT32 isf_vdoout_do_getportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param);
extern ISF_RV isf_vdoout_do_setportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);
extern ISF_RV isf_vdoout_do_getportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);
extern ISF_RV isf_vdoout_inputport_do_push(UINT32 id,ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms);
extern ISF_RV isf_vdoout_do_update(UINT32 id,ISF_UNIT  *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd);
extern ISF_RV isf_vdopout_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2);

extern void _isf_vdoout_do_output_mask(ISF_UNIT *p_thisunit, UINT32 oport, void *in, void *out);
extern void _isf_vdoout_do_output_osd(ISF_UNIT *p_thisunit, UINT32 oport, void *in, void *out);

//debug
extern void isf_vdoout_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);

extern void isf_vdoout_install_id(void);
extern void isf_vdoout_uninstall_id(void);
extern SEM_HANDLE ISF_VDOOUT_PROC_SEM_ID;

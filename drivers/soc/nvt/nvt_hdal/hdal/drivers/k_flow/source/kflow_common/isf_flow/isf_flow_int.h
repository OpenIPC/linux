#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/task.h"
//#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "kwrap/util.h"
#endif
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_common/nvtmpp.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#define EXPORT_SYMBOL(a)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define debug_msg 			vk_printk
#define debug_vprintf 		vprintf
#define _ATOI(str,pint)  	do { *(int*)(pint) = atoi((char*)str); } while (0)
#define _ATOUL(str,puint32)  	do { *(int*)(puint32) = atoi((char*)str); } while (0)
#define _ATOU32(str,puint32)  	do { *(int*)(puint32) = atoi((char*)str); } while (0)
#define _ATOU16(str,puint16)  	do { *(int*)(puint16) = atoi((char*)str); } while (0)
#define _ATOU8(str,puint8)  	do { *(int*)(puint8) = atoi((char*)str); } while (0)
#else
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h> //for atoi()=simple_strtol()
#include <linux/printk.h>
#define debug_msg 			vk_printk
#define debug_vprintf 		vprintk
#define GX_IMAGE_DUMP_ALL		0
#define _ATOI(str,pint32)  	do { kstrtoint((char*)str, 16, (int*)(pint32)); } while (0)
#define _ATOUL(str,puint32)  	do { kstrtoul((char*)str, 16, (int*)(puint32)); } while (0)
#define _ATOU32(str,puint32)  	do { kstrtou32((char*)str, 16, (int*)(puint32)); } while (0)
#define _ATOU16(str,puint16)  	do { kstrtou16((char*)str, 16, (int*)(puint16)); } while (0)
#define _ATOU8(str,puint8)  	do { kstrtou8((char*)str, 16, (int*)(puint8)); } while (0)
#endif

// Output ANSI color
#define _ANSI_BN_ 	"\x1B[1;30m" 	//~N -- for Bright-Gray string      "30"
#define _ANSI_BR_ 	"\x1B[1;31m" 	//~R -- for Bright-Red string       "31"
#define _ANSI_BG_ 	"\x1B[1;32m" 	//~G -- for Bright-Green string     "32"
#define _ANSI_BY_ 	"\x1B[1;33m" 	//~Y -- for Bright-Yellow string    "33"
#define _ANSI_BB_ 	"\x1B[1;34m" 	//~B -- for Bright-Blue string      "34"
#define _ANSI_BM_ 	"\x1B[1;35m" 	//~M -- for Bright-Magenta string   "35"
#define _ANSI_BC_ 	"\x1B[1;36m" 	//~C -- for Bright-Cyan string      "36"
#define _ANSI_BW_ 	"\x1B[1;37m" 	//~W -- for Bright-White string     "37"
#define _ANSI_N_ 		"\x1B[0;30m" 	//^N -- for Gray (Normal) string    "30"
#define _ANSI_R_ 		"\x1B[0;31m" 	//^R -- for Red string              "31"
#define _ANSI_G_ 		"\x1B[0;32m" 	//^G -- for Green string            "32"
#define _ANSI_Y_ 		"\x1B[0;33m" 	//^Y -- for Yellow string           "33"
#define _ANSI_B_ 		"\x1B[0;34m" 	//^B -- for Blue string             "34"
#define _ANSI_M_ 		"\x1B[0;35m" 	//^M -- for Magenta string          "35"
#define _ANSI_C_ 		"\x1B[0;36m" 	//^C -- for Cyan string             "36"
#define _ANSI_W_ 		"\x1B[0;37m" 	//^W -- for White string            "37"
#define _ANSI_0_		"\x1B[0m" 	//(Reset)

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
int debug_log_init(void);
void debug_log_cb(UINT32 uid);
int debug_log_exit(void);
int debug_log_wait(void);
void debug_log_output(char *string);
void debug_log_sig(int uid);
#else
int debug_log_init(void);
void debug_log_cb(struct seq_file *sfile, UINT32 uid);
int debug_log_exit(void);
int debug_log_wait(void);
void debug_log_output(char *string);
void debug_log_sig(int uid);
#endif

void debug_dump_current_stack(void);


void debug_write_open(char *filename);
void debug_write_data(void* p_data, UINT32 datasize);
void debug_write_close(void);

void debug_read_open(char *filename);
void debug_read_data(void* p_data, UINT32 datasize);
void debug_read_close(void);

void debug_write_vdoframe(VDO_FRAME* p_vdoframe);
void debug_read_vdoframe(VDO_FRAME* p_vdoframe);
void debug_write_audframe(AUD_FRAME* p_audframe);
void debug_read_audframe(AUD_FRAME* p_audframe);
void debug_write_vdobs(VDO_BITSTREAM* p_vdobs);
void debug_read_vdobs(VDO_BITSTREAM* p_vdobs);
void debug_write_audbs(AUD_BITSTREAM* p_audbs);
void debug_read_audbs(AUD_BITSTREAM* p_audbs);

#define ISF_PORT_FLAG_PAUSE_PUSH       	0x00000001 ///< pause image data transfer by push
#define ISF_PORT_FLAG_PAUSE_PULL       	0x00000002 ///< pause image data transfer by pull
#define ISF_PORT_FLAG_OPEN       		0x00000010 ///< open
#define ISF_PORT_FLAG_START       		0x00000020 ///< start

#define _IPORT_TYPE(p_unit, iport)		((p_unit)->port_incaps[(iport) - ISF_IN_BASE]->connecttype_caps)
#define _OPORT_TYPE(p_unit, oport)		((p_unit)->port_outcaps[(oport) - ISF_OUT_BASE]->connecttype_caps)
#define _IPORT_IS_ROOT(p_unit, iport)	(_IPORT_TYPE(p_unit, iport) == ISF_CONNECT_NONE)
#define _OPORT_IS_LEAF(p_unit, oport)	(_OPORT_TYPE(p_unit, oport) == ISF_CONNECT_NONE)

#define DUMP_PERFORMANCE 				DISABLE

#if (DUMP_PERFORMANCE == ENABLE)
static UINT32 pt1, pt2;
#define PERFORMANCE_BEGIN() {pt1 = HwClock_GetCounter();} while (0)
#define PERFORMANCE_END()   {pt2 = HwClock_GetCounter();DBG_FUNC("                                        port dt=%lu ms\r\n", (pt2-pt1)>>10);} while (0)
#else
#define PERFORMANCE_BEGIN()
#define PERFORMANCE_END()
#endif

#if (DUMP_PERFORMANCE == ENABLE)
static UINT32 ut1, ut2;
#define UPDATE_BEGIN()      {ut1 = HwClock_GetCounter();} while (0)
#define UPDATE_END()        {ut2 = HwClock_GetCounter();DBG_FUNC("                                        strm dt=%lu ms\r\n", (ut2-ut1)>>10);} while (0)
#else
#define UPDATE_BEGIN()
#define UPDATE_END()
#endif

//unit
extern ISF_RV _isf_unit_verify(ISF_UNIT *p_unit, char *act);
extern void _isf_unit_check_return(ISF_RV r);

//state
extern ISF_RV _isf_unit_set_bind(ISF_UNIT *p_unit, UINT32 oport, ISF_UNIT *p_destunit, UINT32 iport);
extern ISF_RV _isf_unit_set_state(ISF_UNIT *p_unit, UINT32 oport, ISF_PORT_CMD cmd);
extern ISF_RV _isf_unit_clear_bind(ISF_UNIT *p_unit, UINT32 oport);
extern ISF_RV _isf_unit_get_bind(ISF_UNIT *p_unit, UINT32 nport, ISF_UNIT **pp_destunit, UINT32* p_nport);
extern ISF_RV _isf_unit_get_state(ISF_UNIT *p_unit, UINT32 oport, ISF_PORT_STATE* p_state);
extern ISF_RV _isf_unit_clear_state(ISF_UNIT *p_unit, UINT32 oport);

extern void _isf_unit_set_connecttype(UINT32 nport, UINT32 connecttype);
extern void _isf_unit_dump_port(ISF_UNIT *p_unit, UINT32 nport); //dump (1) state (2) imginfo
extern void _isf_unit_dump_pathinfo(ISF_UNIT *p_unit);
//extern ISF_RV isf_unit_cfg_userdump(ISF_UNIT* p_unit, void (*d)(void));
extern ISF_RV isf_unit_match_name(ISF_UNIT *p_unit, char *name);
extern ISF_RV isf_unit_match_outputname(ISF_UNIT *p_unit, char *name);
extern ISF_RV isf_unit_match_inputname(ISF_UNIT *p_unit, char *name);

//param
extern ISF_RV _isf_unit_clear_context(ISF_UNIT *p_unit, UINT32 oport);
extern void _isf_unit_dump_imginfo(ISF_PORT *p_port);
extern void _isf_unit_clear_imginfo(ISF_PORT *p_port);
extern void _isf_unit_default_get_imginfo(ISF_PORT *p_port, ISF_VDO_INFO *p_imginfo);
//extern void _isf_unit_default_get_audinfo(ISF_PORT *p_port, ISF_AUD_INFO *pAudInfo);

//data
extern void _isf_data_reguser(void);
extern ISF_BASE_UNIT _isf_unit_base;

//debug 
extern void _isf_stream_dump_port(char* unit_name, char* port_name);
extern void _isf_stream_dump_port3(char* unit_name, char* port_name);

//probe data
extern void _isf_debug(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 opclass, const char *fmtstr, ...);
extern void _isf_trace(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 opclass, const char *fmtstr, ...);
extern void _isf_probe(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 probe, ISF_RV result);
extern void _isf_unit_debug_new(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 addr, UINT32 size, UINT32 probe);
extern void _isf_unit_debug_add(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe, ISF_RV result);
extern void _isf_unit_debug_prepush(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data);
extern void _isf_unit_debug_postpush(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data);
extern void _isf_unit_debug_release(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe, ISF_RV result);
extern void _isf_flow_probe_port(char* unit_name, char* port_name, char* mask_name);
extern void _isf_flow_perf_port(char* unit_name, char* port_name);
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
//trace state & param
extern void _isf_unit_debug_trace2(UINT32 Class, ISF_UNIT *p_thisunit, UINT32 nport, const char *fmtstr, ...);
extern void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name);
extern void _isf_flow_trace_port(char* unit_name, char* port_name, char* mask_name);

//id
///////////////////////////////////////////////////////////////////////////////
extern SEM_HANDLE ISF_SEM_ID;
extern SEM_HANDLE ISF_SEM_CFG_ID;
///////////////////////////////////////////////////////////////////////////////



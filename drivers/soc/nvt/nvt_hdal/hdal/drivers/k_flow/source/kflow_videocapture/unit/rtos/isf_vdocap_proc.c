#include "../isf_vdocap_api.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdocap_dbg.h"
#include "../isf_vdocap_int.h"
#include "kwrap/semaphore.h"

//#ifdef DEBUG
unsigned int isf_vdocap_debug_level = NVT_DBG_WRN;
module_param_named(isf_vdocap_debug_level, isf_vdocap_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdocap_debug_level, "vdocap debug level");
//#endif
//============================================================================
// Function define
//============================================================================
static int isf_vdocap_seq_printf(const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);
	if (len > 0)
		DBG_DUMP(buf);
	return 0;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
extern void debug_log_cb(UINT32 uid);
BOOL _isf_vdocap_api_info(unsigned char argc, char **pargv)
{
	UINT32 dev, vdocap_max_num;

	if( nvt_get_chip_id() != CHIP_NA51055) {
		vdocap_max_num = VDOCAP_MAX_NUM;
	} else {
		vdocap_max_num = 3;
	}

	SEM_WAIT(ISF_VDOCAP_PROC_SEM_ID);
	//dump info of all devices
	debug_log_cb(1); //show hdal version
	for(dev = 0; dev < vdocap_max_num; dev++) {
		UINT32 uid = ISF_UNIT_VDOCAP + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(uid);
		//dump work status of 1 device
		isf_vdocap_dump_status(isf_vdocap_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_VDOCAP_PROC_SEM_ID);
	return 1;
}

BOOL _isf_vdocap_api_drv(unsigned char argc, char **pargv)
{
#if defined(__LINUX)
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap0.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap1.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap2.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap3.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap4.refdata);
#if defined(_BSP_NA51000_)
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap5.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap6.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap7.refdata);
#endif
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap0.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap1.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap2.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap3.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap4.refdata);
#if defined(_BSP_NA51000_)
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap5.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap6.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap7.refdata);
#endif

#endif
	return 1;
}

BOOL _isf_vdocap_api_reg(unsigned char argc, char **pargv)
{
	VDOCAP_CONTEXT *p_ctx;

	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap0.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE1]\r\n");
		debug_dumpmem(0xF0C00000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap1.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE2]\r\n");
		debug_dumpmem(0xF0D20000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap2.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE3]\r\n");
		debug_dumpmem(0xF0D30000, 0x8B4);
	}
	if( nvt_get_chip_id() != CHIP_NA51055)
	{
		p_ctx = (VDOCAP_CONTEXT *)isf_vdocap3.refdata;
		if (p_ctx && p_ctx->sie_hdl) {
			DBG_DUMP("[SIE4]\r\n");
			debug_dumpmem(0xF0D40000, 0x8B4);
		}
		p_ctx = (VDOCAP_CONTEXT *)isf_vdocap4.refdata;
		if (p_ctx && p_ctx->sie_hdl) {
			DBG_DUMP("[SIE5]\r\n");
			debug_dumpmem(0xF0D50000, 0x8B4);
		}
	}
#if defined(_BSP_NA51000_)
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap5.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE6]\r\n");
		debug_dumpmem(0xF0D60000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap6.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE7]\r\n");
		debug_dumpmem(0xF0D70000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap7.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE8]\r\n");
		debug_dumpmem(0xF0D80000, 0x8B4);
	}
#endif
	return 1;
}
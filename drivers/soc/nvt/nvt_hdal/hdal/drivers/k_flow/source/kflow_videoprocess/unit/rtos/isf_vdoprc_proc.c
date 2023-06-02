#include "../isf_vdoprc_api.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdoprc_dbg.h"
#include "../isf_vdoprc_int.h"
#include "kwrap/semaphore.h"

//============================================================================
// Function define
//============================================================================
static int isf_vdoprc_seq_printf(const char *fmtstr, ...)
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
extern BOOL _isf_vdoprc_api_info(unsigned char argc, char **pargv)
{
	UINT32 dev;
	SEM_WAIT(ISF_VDOPRC_PROC_SEM_ID);
	//dump info of all devices
	debug_log_cb(1); //show hdal version
	for(dev = 0; dev < 16; dev++) {
		UINT32 uid = ISF_UNIT_VDOPRC + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(uid);
		//dump work status of 1 device
		isf_vdoprc_dump_status(isf_vdoprc_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_VDOPRC_PROC_SEM_ID);
	return 1;
}


#include "kwrap/cmdsys.h"
#include "kwrap/debug.h"

//=============================================================================
// proc "flow" file operation functions
//=============================================================================

BOOL _isf_vdoprc_api_flow(unsigned char argc, char **pargv)
{
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	if (!_isf_vdoprc_is_init()) {
		DBG_ERR("[vdoprc] not init\r\n");
		return 1;
	}

	DBG_DUMP("[kflow ctl_ipp]\r\n");
	nvt_cmdsys_runcmd("ctl_ipp dump_hdl_all");
#endif
	return 1;
}

//=============================================================================
// proc "drv" file operation functions
//=============================================================================

BOOL _isf_vdoprc_api_drv(unsigned char argc, char **pargv)
{
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	if (!_isf_vdoprc_is_init()) {
		DBG_ERR("[vdoprc] not init\r\n");
		return 1;
	}

	DBG_DUMP("[kdrv ife]\r\n");
	nvt_cmdsys_runcmd("kdrv_ife info");
	DBG_DUMP("[kdrv ife2]\r\n");
	nvt_cmdsys_runcmd("kdrv_ife2 info");
	DBG_DUMP("[kdrv dce]\r\n");
	nvt_cmdsys_runcmd("kdrv_dce info");
	DBG_DUMP("[kdrv ipe]\r\n");
	nvt_cmdsys_runcmd("kdrv_ipe info");
	DBG_DUMP("[kdrv ime]\r\n");
	nvt_cmdsys_runcmd("kdrv_ime info");
#endif
	return 1;
}

//=============================================================================
// proc "reg" file operation functions
//=============================================================================
BOOL _isf_vdoprc_api_reg(unsigned char argc, char **pargv)
{
#if defined(_BSP_NA51000_)
	DBG_DUMP("[RHE]\r\n");
	debug_dumpmem(0xf0ce0000, 0x900);
	DBG_DUMP("[IFE]\r\n");
	debug_dumpmem(0xf0c70000, 0x200);
	DBG_DUMP("[IFE2]\r\n");
	debug_dumpmem(0xf0d00000, 0x100);
	DBG_DUMP("[DCE]\r\n");
	debug_dumpmem(0xf0c20000, 0x300);
	DBG_DUMP("[IPE]\r\n");
	debug_dumpmem(0xf0c30000, 0x800);
	DBG_DUMP("[IME]\r\n");
	debug_dumpmem(0xf0c40000, 0x1000);
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	DBG_DUMP("[IFE]\r\n");
	debug_dumpmem(0xf0c70000, 0x800);
	DBG_DUMP("[IFE2]\r\n");
	debug_dumpmem(0xf0d00000, 0x100);
	DBG_DUMP("[DCE]\r\n");
	debug_dumpmem(0xf0c20000, 0x700);
	DBG_DUMP("[IPE]\r\n");
	debug_dumpmem(0xf0c30000, 0x900);
	DBG_DUMP("[IME]\r\n");
	debug_dumpmem(0xf0c40000, 0xb00);
#endif
	return 1;
}


//=============================================================================
// proc "rate" file operation functions
//=============================================================================
BOOL _isf_vdoprc_api_rate2(unsigned char argc, char **pargv)
{

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	DBG_DUMP("[kflow ctl_ipp]\r\n");
	nvt_cmdsys_runcmd("ctl_ipp dump_t");
#endif
	return 1;
}



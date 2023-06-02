#ifndef _IQ_DBG_H_
#define _IQ_DBG_H_

#define THIS_DBGLVL         6  //NVT_DBG_MSG
#define __MODULE__          iq
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#include "kwrap/debug.h"

#include "iq_alg.h"

#define IQ_DBG_NONE             0x00000000
#define IQ_DBG_WRN_MSG          0x00000001
#define IQ_DBG_SYNC             0x00000002
#define IQ_DBG_CFG              0x00000004
#define IQ_DBG_DTS              0x00000008
#define IQ_DBG_VIG              0x00000010
#define IQ_DBG_GAMMA            0x00000020
#define IQ_DBG_YCURVE           0x00000040
#define IQ_DBG_TABLE            0x00000080
#define IQ_DBG_P_SIE            0x00000100
#define IQ_DBG_P_IFE            0x00000200
#define IQ_DBG_P_DCE            0x00000400
#define IQ_DBG_P_IPE            0x00000800
#define IQ_DBG_P_IFE2           0x00001000
#define IQ_DBG_P_IME            0x00002000
#define IQ_DBG_P_ENC            0x00004000
#define IQ_DBG_A_WDR            0x00010000
#define IQ_DBG_A_DEFOG          0x00020000
#define IQ_DBG_A_SHDR           0x00040000
#define IQ_DBG_A_RGBIR          0x00080000
#define IQ_DBG_O_ISO            0x00100000
#define IQ_DBG_O_EDGE           0x00200000
#define IQ_DBG_O_TMNR           0x00400000
#define IQ_DBG_O_ECS            0x00800000
#define IQ_DBG_CAPTURE          0x01000000
#define IQ_DBG_PERFORMANCE      0x10000000
#define IQ_DBG_BUFFER           0x20000000

#define PRINT_IQ(type, fmt, args...) {if (type) DBG_DUMP(fmt, ## args); }
#define PRINT_IQ_VAR(type, var)      {if (type) DBG_DUMP("%s = %d\r\n", #var, var); }
#define PRINT_IQ_ARR(type, arr, len) {       \
	if (type) {                              \
		do {                                 \
			UINT32 i;                        \
			DBG_DUMP("%s = { ", #arr);    \
			for (i = 0; i < len; i++)        \
				DBG_DUMP("%d, ", arr[i]);    \
			DBG_DUMP("}\r\n");               \
		} while (0);                         \
	};                                       \
}
#define PRINT_IQ_WRN(type, fmt, args...) {if (type) DBG_WRN(fmt, ## args); if (iq_dbg_check_wrn_msg(type)) DBG_WRN(fmt, ## args); }
#if defined(__KERNEL__)
#define PRINT_IQ_INFO(sfile, fmt, args...) {seq_printf(sfile, fmt, ## args); }
#else
#define PRINT_IQ_INFO(sfile, fmt, args...) {DBG_DUMP(fmt, ## args); }
#endif

extern UINT32 iq_dbg_get_dbg_mode(IQ_ID id);
extern void iq_dbg_set_dbg_mode(IQ_ID id, UINT32 cmd);
extern void iq_dbg_clr_dbg_mode(IQ_ID id, UINT32 cmd);
extern BOOL iq_dbg_check_wrn_msg(BOOL show_dbg_msg);
extern void iq_dbg_clr_wrn_msg(void);
extern UINT32 iq_dbg_get_wrn_msg(void);

#endif

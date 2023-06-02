#ifndef _AWB_DBG_H_
#define _AWB_DBG_H_

#define THIS_DBGLVL         6  //NVT_DBG_MSG
#define __MODULE__          awb
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#include "kwrap/debug.h"

#include "awb_alg.h"

//=============================================================================
// define & struct
//=============================================================================
#define AWB_DBG_NONE             0x00
#define AWB_DBG_SYNC             0x01
#define AWB_DBG_CFG              0x02
#define AWB_DBG_UART             0x10
#define AWB_DBG_FLOW             0x20
#define AWB_DBG_ALG              0x40
#define AWB_DBG_CA               0x80

#if defined(__FREERTOS)
#define PRINT_AWB(type, fmt, args...)  {if (type) DBG_DUMP(fmt, ## args); }
#define PRINT_AWB_INFO(sfile, fmt, args...) {DBG_DUMP(fmt, ## args); }
#else
#define PRINT_AWB(type, fmt, args...)  {if (type) printk(fmt, ## args); }
#define PRINT_AWB_INFO(sfile, fmt, args...) {seq_printf(sfile, fmt, ## args); }
#endif

extern UINT32 awb_get_dbg_mode(AWB_ID id);
extern UINT32 awb_get_dbg_freq(AWB_ID id);
extern void awb_set_dbg_mode(AWB_ID id, UINT32 level, UINT32 freq);

#endif

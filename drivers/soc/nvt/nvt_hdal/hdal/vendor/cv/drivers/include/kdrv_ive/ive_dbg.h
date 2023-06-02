#ifndef __IVE_DBG_H_
#define __IVE_DBG_H_

#if defined(__FREERTOS)
#include "kwrap/debug.h"
#else
#endif

#define IVE_SYS_VFY_DBG_EN DISABLE

#define NVT_DBG_FATAL     0
#define NVT_DBG_ERR       1
#define NVT_DBG_WRN       2
#define NVT_DBG_UNIT      3
#define NVT_DBG_FUNC      4
#define NVT_DBG_IND       5
#define NVT_DBG_MSG       6
#define NVT_DBG_VALUE     7
#define NVT_DBG_USER      8

#if defined(__FREERTOS)
extern unsigned int ive_drv_debug_level;

//#define DBG_FATAL(fmt, args...) nvt_dbg(FATAL, fmt, ##args)
//#define DBG_ERR(fmt, args...) nvt_dbg(ERR, fmt, ##args)
//#define DBG_WRN(fmt, args...) nvt_dbg(WRN, fmt, ##args)
//#define DBG_UNIT(fmt, args...) nvt_dbg(UNIT, fmt, ##args)
//#define DBG_FUNC(fmt, args...) nvt_dbg(FUNC, fmt, ##args)
//#define DBG_IND(fmt, args...) nvt_dbg(IND, fmt, ##args)
//#define DBG_MSG(fmt, args...) nvt_dbg(MSG, fmt, ##args)
//#define DBG_VALUE(fmt, args...) nvt_dbg(VALUE, fmt, ##args)
//#define DBG_USER(fmt, args...) nvt_dbg(USER, fmt, ##args)
#if (IVE_SYS_VFY_DBG_EN == ENABLE)
#define DBG_EMU(fmt, args...) nvt_dbg(ERR, fmt, ##args)
#else
#define DBG_EMU(fmt, args...) nvt_dbg(IND, fmt, ##args)
#endif
//#define DBG_DUMP(fmt, args...) pr_warn("%s:" fmt, __func__, ##args);
#else

extern unsigned int ive_drv_debug_level;
#define nvt_dbg(level, fmt, args...) \
    do { \
        if (unlikely(NVT_DBG_##level <= ive_drv_debug_level)) \
            pr_warn("%s:" fmt, __func__, ##args);   \
    } while (0)

#define DBG_FATAL(fmt, args...) nvt_dbg(FATAL, fmt, ##args)
#define DBG_ERR(fmt, args...) nvt_dbg(ERR, fmt, ##args)
#define DBG_WRN(fmt, args...) nvt_dbg(WRN, fmt, ##args)
#define DBG_UNIT(fmt, args...) nvt_dbg(UNIT, fmt, ##args)
#define DBG_FUNC(fmt, args...) nvt_dbg(FUNC, fmt, ##args)
#define DBG_IND(fmt, args...) nvt_dbg(IND, fmt, ##args)
#define DBG_MSG(fmt, args...) nvt_dbg(MSG, fmt, ##args)
#define DBG_VALUE(fmt, args...) nvt_dbg(VALUE, fmt, ##args)
#define DBG_USER(fmt, args...) nvt_dbg(USER, fmt, ##args)
#if (IVE_SYS_VFY_DBG_EN == ENABLE)
#define DBG_EMU(fmt, args...) nvt_dbg(ERR, fmt, ##args)
#else
#define DBG_EMU(fmt, args...) nvt_dbg(IND, fmt, ##args)
#endif
#define DBG_DUMP(fmt, args...) pr_warn("%s:" fmt, __func__, ##args);

#endif //#if defined(__FREERTOS)

#endif

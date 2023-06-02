#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_

#if 1   //for 313
//#include "kwrap/debug.h"
#define NVT_DBG_FATAL     0
#define NVT_DBG_ERR       1
#define NVT_DBG_WRN       2
#define NVT_DBG_UNIT      3
#define NVT_DBG_FUNC      4
#define NVT_DBG_IND       5
#define NVT_DBG_MSG       6
#define NVT_DBG_VALUE     7
#define NVT_DBG_USER      8


/*#define NVT_DBG_INFO                    0x00000001
#define NVT_DBG_WARN                    0x00000002
#define NVT_DBG_ERR                     0x00000004*/
#define DEBUG                           1

#define IND                     0x00000001
#define WRN                     0x00000002
#define ERR                     0x00000004

#ifdef DEBUG
extern unsigned int xxx_debug_level;

#define nvt_dbg(level, fmt, args...)                \
	do {                                               \
		printk("%s:" fmt, __func__, ##args);   \
	} while (0)

#define DBG_ERR(fmt, args...) nvt_dbg(ERR, fmt, ##args)
#define DBG_WRN(fmt, args...) nvt_dbg(WRN, fmt, ##args)
#define DBG_IND(fmt, args...) nvt_dbg(IND, fmt, ##args)

#define DBGD(x)   printk("\033[0;35m%s=%d\033[0m\r\n",#x,(int)(x))              ///< Show a color sting of variable name and variable deciaml value
#define DBGH(x)   printk("\033[0;35m%s=0x%08X\033[0m\r\n",#x,(unsigned int)(x)) ///< Show a color sting of variable name and variable hexadecimal value
#else
#define nvt_dbg(fmt, ...)
#endif

#else
#define NVT_DBG_FATAL     0
#define NVT_DBG_ERR       1
#define NVT_DBG_WRN       2
#define NVT_DBG_UNIT      3
#define NVT_DBG_FUNC      4
#define NVT_DBG_IND       5
#define NVT_DBG_MSG       6
#define NVT_DBG_VALUE     7
#define NVT_DBG_USER      8

#ifdef DEBUG

extern unsigned int ive_drv_debug_level;
#define nvt_dbg(level, fmt, args...)                \
	do {                                               \
		if (unlikely(NVT_DBG_##level <= ive_drv_debug_level))    \
			pr_warn("%s:" fmt, __func__, ##args);   \
	} while (0)
#else
#define nvt_dbg(fmt, ...)
#endif

#define DBG_FATAL(fmt, args...) nvt_dbg(FATAL, fmt, ##args)
#define DBG_ERR(fmt, args...) nvt_dbg(ERR, fmt, ##args)
#define DBG_WRN(fmt, args...) nvt_dbg(WRN, fmt, ##args)
#define DBG_UNIT(fmt, args...) nvt_dbg(UNIT, fmt, ##args)
#define DBG_FUNC(fmt, args...) nvt_dbg(FUNC, fmt, ##args)
#define DBG_IND(fmt, args...) nvt_dbg(IND, fmt, ##args)
#define DBG_MSG(fmt, args...) nvt_dbg(MSG, fmt, ##args)
#define DBG_VALUE(fmt, args...) nvt_dbg(VALUE, fmt, ##args)
#define DBG_USER(fmt, args...) nvt_dbg(USER, fmt, ##args)
#define DBG_DUMP(fmt, args...) pr_warn("%s:" fmt, __func__, ##args);
#endif

#endif

#ifndef __LVDS_DBG_H_
#define __LVDS_DBG_H_

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

extern unsigned int lvds_debug_level;
#define nvt_dbg(level, fmt, args...)                \
do {                                               \
if (unlikely(NVT_DBG_##level <= lvds_debug_level))    \
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


//#define LVDS_API_DBG
#ifdef LVDS_API_DBG
#define lvds_api  printk
#else
#define lvds_api(fmt, ...)
#endif


#endif




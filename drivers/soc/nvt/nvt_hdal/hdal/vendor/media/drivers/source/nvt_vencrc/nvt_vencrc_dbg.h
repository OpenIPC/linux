#ifndef __NVT_VENCRC_DBG_H_
#define __NVT_VENCRC_DBG_H_

#if defined(__LINUX)
#include <linux/kernel.h>
#define vk_pr_warn pr_warn
#elif defined(__FREERTOS)
#include <stdio.h>
#define vk_pr_warn printf

#ifndef unlikely
#define unlikely(x) (x)
#endif

#endif

#define NVT_DBG_FATAL     (1<<1)
#define NVT_DBG_ERR       (1<<2)
#define NVT_DBG_WRN       (1<<3)
#define NVT_DBG_WARN      (1<<4)
#define NVT_DBG_UNIT      (1<<5)
#define NVT_DBG_FUNC      (1<<6)
#define NVT_DBG_IND       (1<<7)
#define NVT_DBG_INFO      (1<<8)
#define NVT_DBG_MSG       (1<<9)
#define NVT_DBG_VALUE     (1<<10)
#define NVT_DBG_USER      (1<<11)

#ifdef DEBUG
extern unsigned int NVT_vencrc_debug_level;

#define nvt_dbg(level, fmt, args...)                \
do {                                               \
        if (unlikely(NVT_DBG_##level & kdrv_vdocdc_debug_level))    \
                vk_pr_warn("%s:" fmt, __func__, ##args);   \
} while (0)
#else
#define nvt_dbg(fmt, ...)
#endif
#define DBG_FATAL(fmt, args...)	nvt_dbg(FATAL, fmt, ##args)
#define DBG_ERR(fmt, args...)	nvt_dbg(ERR, fmt, ##args)
#define DBG_WRN(fmt, args...)	nvt_dbg(WRN, fmt, ##args)
#define DBG_WARN(fmt, args...)	nvt_dbg(WARN, fmt, ##args)
#define DBG_UNIT(fmt, args...)	nvt_dbg(UNIT, fmt, ##args)
#define DBG_FUNC(fmt, args...)	nvt_dbg(FUNC, fmt, ##args)
#define DBG_IND(fmt, args...)	nvt_dbg(IND, fmt, ##args)
#define DBG_INFO(fmt, args...)	nvt_dbg(INFO, fmt, ##args)
#define DBG_MSG(fmt, args...)	nvt_dbg(MSG, fmt, ##args)
#define DBG_VALUE(fmt, args...)	nvt_dbg(VALUE, fmt, ##args)
#define DBG_USER(fmt, args...)	nvt_dbg(USER, fmt, ##args)
#define DBG_DUMP(fmt, args...)	vk_pr_warn("%s:" fmt, __func__, ##args);

#endif


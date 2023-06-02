#ifndef _VOS_DEBUG_H_
#define _VOS_DEBUG_H_

#include <stdarg.h>
#include <kwrap/type.h>


#if defined(__LINUX) && defined(__KERNEL__)
#include <linux/printk.h>
#define vk_pr_warn pr_warn
#define vk_printk printk
#define vk_print_isr printk
#define vos_dump_stack dump_stack
#define __DBG_COLOR_MONO__

#elif defined(__FREERTOS)
#define vk_pr_warn debug_msg_isr
#define vk_printk debug_msg_isr
#define vk_print_isr debug_msg_isr

#ifndef unlikely
#define unlikely(x) (x)
#endif

#ifndef KERN_EMERG
#define KERN_EMERG ""
#endif

#else
#include <stdio.h>
#define vk_pr_warn printf
#define vk_printk printf
#define vk_print_isr printf

#ifndef unlikely
#define unlikely(x) (x)
#endif

#endif

#define NVT_DBG_FATAL     0
#define NVT_DBG_ERR       1
#define NVT_DBG_WRN       2
#define NVT_DBG_UNIT      3
#define NVT_DBG_FUNC      4
#define NVT_DBG_IND       5
#define NVT_DBG_MSG       6
#define NVT_DBG_VALUE     7
#define NVT_DBG_USER      8

#define _TOKEN_BIND(a, b)  a ## b
#define TOKEN_BIND(a, b)  _TOKEN_BIND(a, b)

//If __MODULE__ not defined, just give it a name to let other macros can be used
#ifndef __MODULE__
#define __MODULE__ __SHORT_FILE__
#endif

//If __DBGLVL__ not defined, use NVT_DBG_WRN as the default level
#ifndef __DBGLVL__
#define __DBGLVL__ NVT_DBG_WRN
#endif

/**
	If __DBGLVL__ equals NVT_DBG_USER, the debug level is controlled by users
	Otherwise, the debug level is fixed to __DBGLVL__

	e.g.
	1. Fixed debug level
	@code
    {
		#define __MODULE__ my_module
		#define __DBGLVL__ 2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
		#include <kwrap/debug.h>
	}
    2. Dynamic debug level (an extern variable is requred)
	@code
    {
		#define __MODULE__ my_module
		#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
		#include <kwrap/debug.h>

		unsigned int my_module_debug_level = NVT_DBG_WRN;
		//Note: It will cause linking error if you set __DBGLVL__ as 8 but the variable is not found
	}
*/
#if (__DBGLVL__ == NVT_DBG_USER)
#define _NVT_DBG_CURRENT_ TOKEN_BIND(__MODULE__, _debug_level)
extern unsigned int _NVT_DBG_CURRENT_;

#define nvt_dbg(level, fmt, args...) do { \
	if (unlikely(NVT_DBG_##level <= _NVT_DBG_CURRENT_)) { \
		vk_pr_warn(fmt, ##args); \
	} \
} while (0)

#else //#if (__DBGLVL__ == NVT_DBG_USER)
#define _NVT_DBG_CURRENT_ __DBGLVL__
#define nvt_dbg(level, fmt, args...) NVT_PRINT_##level(fmt, ##args)

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_FATAL)
#define NVT_PRINT_FATAL(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_FATAL(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_ERR)
#define NVT_PRINT_ERR(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_ERR(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_WRN)
#define NVT_PRINT_WRN(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_WRN(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_UNIT)
#define NVT_PRINT_UNIT(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_UNIT(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_FUNC)
#define NVT_PRINT_FUNC(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_FUNC(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_IND)
#define NVT_PRINT_IND(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_IND(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_MSG)
#define NVT_PRINT_MSG(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_MSG(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_VALUE)
#define NVT_PRINT_VALUE(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_VALUE(fmt, args...)
#endif

#if (_NVT_DBG_CURRENT_ >= NVT_DBG_USER)
#define NVT_PRINT_USER(fmt, args...) vk_pr_warn(fmt, ##args)
#else
#define NVT_PRINT_USER(fmt, args...)
#endif

#endif //#if (__DBGLVL__ == NVT_DBG_USER)

#ifdef __DBG_COLOR_MONO__
#define DBG_COLOR_GRAY ""
#define DBG_COLOR_RED ""
#define DBG_COLOR_YELLOW ""
#define DBG_COLOR_CYAN ""
#define DBG_COLOR_GREEN ""
#define DBG_COLOR_MAGENTA ""
#define DBG_COLOR_BLUE ""
#define DBG_COLOR_WHITE ""
#define DBG_COLOR_HI_GRAY ""
#define DBG_COLOR_HI_RED ""
#define DBG_COLOR_HI_YELLOW ""
#define DBG_COLOR_HI_CYAN ""
#define DBG_COLOR_HI_GREEN ""
#define DBG_COLOR_HI_MAGENTA ""
#define DBG_COLOR_HI_BLUE ""
#define DBG_COLOR_HI_WHITE ""
#define DBG_COLOR_FATAL ""
#define DBG_COLOR_ERR ""
#define DBG_COLOR_WRN ""
#define DBG_COLOR_UNIT ""
#define DBG_COLOR_FUNC ""
#define DBG_COLOR_IND ""
#define DBG_COLOR_MSG ""
#define DBG_COLOR_VALUE ""
#define DBG_COLOR_USER ""
#define DBG_COLOR_END ""
#else
#define DBG_COLOR_GRAY "\033[0;30m"
#define DBG_COLOR_RED "\033[0;31m"
#define DBG_COLOR_YELLOW "\033[0;33m"
#define DBG_COLOR_CYAN "\033[0;36m"
#define DBG_COLOR_GREEN "\033[0;32m"
#define DBG_COLOR_MAGENTA "\033[0;35m"
#define DBG_COLOR_BLUE "\033[0;34m"
#define DBG_COLOR_WHITE "\033[0;37m"
#define DBG_COLOR_HI_GRAY "\033[1;30m"
#define DBG_COLOR_HI_RED "\033[1;31m"
#define DBG_COLOR_HI_YELLOW "\033[1;33m"
#define DBG_COLOR_HI_CYAN "\033[1;36m"
#define DBG_COLOR_HI_GREEN "\033[1;32m"
#define DBG_COLOR_HI_MAGENTA "\033[1;35m"
#define DBG_COLOR_HI_BLUE "\033[1;34m"
#define DBG_COLOR_HI_WHITE "\033[1;37m"
#define DBG_COLOR_FATAL "\033[1;31m"
#define DBG_COLOR_ERR "\033[1;31m"
#define DBG_COLOR_WRN "\033[1;33m"
#define DBG_COLOR_UNIT "\033[0;32m"
#define DBG_COLOR_FUNC "\033[0;36m"
#define DBG_COLOR_IND ""
#define DBG_COLOR_MSG ""
#define DBG_COLOR_VALUE ""
#define DBG_COLOR_USER ""
#define DBG_COLOR_END "\033[0m"
#endif

#define DBG_FATAL(fmt, args...)         nvt_dbg(FATAL, DBG_COLOR_FATAL "FATAL:%s() " fmt DBG_COLOR_END, __func__, ##args)
#define DBG_ERR(fmt, args...)           nvt_dbg(ERR,   DBG_COLOR_ERR   "ERR:%s() "   fmt DBG_COLOR_END, __func__, ##args)
#define DBG_WRN(fmt, args...)           nvt_dbg(WRN,   DBG_COLOR_WRN   "WRN:%s() "   fmt DBG_COLOR_END, __func__, ##args)
#define DBG_UNIT(fmt, args...)          nvt_dbg(UNIT,  DBG_COLOR_UNIT  "UNIT:%s() "  fmt DBG_COLOR_END, __func__, ##args)
#define DBG_FUNC_BEGIN(fmt, args...)    nvt_dbg(FUNC,  DBG_COLOR_FUNC  "%s():begin " fmt DBG_COLOR_END, __func__, ##args)
#define DBG_FUNC(fmt, args...)          nvt_dbg(FUNC,  DBG_COLOR_FUNC  "%s(): "      fmt DBG_COLOR_END, __func__, ##args)
#define DBG_FUNC_END(fmt, args...)      nvt_dbg(FUNC,  DBG_COLOR_FUNC  "%s():end "   fmt DBG_COLOR_END, __func__, ##args)
#define DBG_IND(fmt, args...)           nvt_dbg(IND,   DBG_COLOR_IND   "%s() "       fmt DBG_COLOR_END, __func__, ##args)
#define DBG_MSG(fmt, args...)           nvt_dbg(MSG,   DBG_COLOR_MSG   fmt DBG_COLOR_END, ##args)
#define DBG_VALUE(fmt, args...)         nvt_dbg(VALUE, DBG_COLOR_VALUE fmt DBG_COLOR_END, ##args)
#define DBG_USER(fmt, args...)          nvt_dbg(USER,  DBG_COLOR_USER  fmt DBG_COLOR_END, ##args)
#define DBG_DUMP(fmt, args...)          vk_pr_warn(fmt, ##args)

// only used in ISR. max string len is limited
#define DBG_ISR(fmt, args...)           vk_print_isr(fmt, ##args)

#define CHKPNT    vk_pr_warn(DBG_COLOR_WHITE "CHK: %d, %s\r\n" DBG_COLOR_END, __LINE__, __func__)
#define DBGD(x)   vk_pr_warn(DBG_COLOR_MAGENTA "%s=%d\r\n" DBG_COLOR_END, #x, x)
#define DBGH(x)   vk_pr_warn(DBG_COLOR_MAGENTA "%s=0x%08X\r\n" DBG_COLOR_END, #x, x)

#if defined(__FREERTOS)
extern void profile_task_busy(void);
extern void profile_task_idle(void);
#define PROFILE_TASK_BUSY() profile_task_busy()
#define PROFILE_TASK_IDLE() profile_task_idle()
#else
#define PROFILE_TASK_BUSY()
#define PROFILE_TASK_IDLE()
#endif

#define ASSERT(x)                                                       \
	do {    if (x) break;                                               \
		vk_printk(KERN_EMERG "### ASSERTION FAILED %s: %s: %d: %s\n",      \
			   __FILE__, __func__, __LINE__, #x); vos_dump_stack(); vos_debug_halt();  \
	} while (0)


#if defined(__FREERTOS)
int debug_msg_isr(const char *fmtstr, ...);
#endif

#ifndef exam_msg
// exam_msg is used by auto-test. we directly define exam_msg as vk_printk to reserve for future modifications.
#define exam_msg vk_printk
#endif

void vos_debug_halt(void);

void debug_dumpmem(ULONG addr, ULONG length); ///< dump memory region to console output
void debug_dumpmem2file(ULONG addr, ULONG length, CHAR *filename);
void debug_kmsg(char *fmtstr, ...); ///< debug message entry
void debug_kmsg_register(void (*fp)(const char *fmtstr, ...)); ///< hook debug msg

#endif //_VOS_DEBUG_H_



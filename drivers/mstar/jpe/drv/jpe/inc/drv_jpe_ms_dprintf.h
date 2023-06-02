#ifndef DRV_JPE_MS_DPRINTF_H
#define DRV_JPE_MS_DPRINTF_H

#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL 0
#endif

#ifdef _MSC_VER

#if DEBUG_LEVEL == 0

//static __inline void ms_dprintf(int level, const char *fmt, ...) {}

#include <stdio.h>
#include <stdarg.h>
//#include <windows.h>

static __inline void ms_dprintf(int level, const char *fmt, ...)
{
    va_list ap;
    char msg[2048];

    if(level <= 1) return;

    va_start(ap, fmt);
    vsprintf(msg, fmt, ap);
    fprintf(stdout, "%s", msg);
    va_end(ap);
}

#else /* DEBUG_LEVEL */

#include <stdio.h>
#include <stdarg.h>
//#include <windows.h>

static __inline void ms_dprintf(int level, const char *fmt, ...)
{
    va_list ap;
    char msg[2048];

    if (level > DEBUG_LEVEL)
        return;

    va_start(ap, fmt);
    vsprintf(msg, fmt, ap);
#ifdef WINCE
    fprintf(stderr, "%s", msg);
#else
    OutputDebugStringA(msg);
#endif
    va_end(ap);
}

#endif /* DEBUG_LEVEL */

#else /* _MSC_VER */

#if DEBUG_LEVEL == 0

#define ms_dprintf(arg...) ((void) 0)

#else /* DEBUG_LEVEL */

#ifdef MIPSDEV

#include <cyg/infra/diag.h>

#define ms_dprintf(level, fmt, arg...) \
    do { \
        if (level <= DEBUG_LEVEL) diag_printf(fmt, ##arg); \
    } while (0)
#elif defined __I_SW__
#include <stdio.h>
#include <stdarg.h>
#include "sys_sys_isw_uart.h" // send_trace
static __inline void ms_dprintf(int level, const char *fmt, ...)
{
    va_list ap;
    char msg[2048];

    if (level > DEBUG_LEVEL)
        return;

    va_start(ap, fmt);
    vsprintf(msg, fmt, ap);
    send_msg(msg);
    va_end(ap);
}


#else /* MIPSDEV */

#if 1
#define ms_dprintf(level, fmt, arg...)
#else
#define ms_dprintf(level, fmt, arg...) \
    do { \
        if (level <= DEBUG_LEVEL) printf(fmt, ##arg); \
    } while (0)
#endif

#endif /* MIPSDEV */

#endif /* DEBUG_LEVEL */

#endif /* _MSC_VER */

#endif /* MS_DPRINTF_H */

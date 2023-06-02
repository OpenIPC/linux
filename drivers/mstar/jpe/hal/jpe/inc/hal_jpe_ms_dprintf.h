#ifndef DRV_JPE_MS_DPRINTF_H
#define DRV_JPE_MS_DPRINTF_H

#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL 0
#endif

////////////////////////////////////// Define for Microsoft platform ///////////////////////////////
#if defined(_MSC_VER)
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


////////////////////////////////////// Define for MISP platform ///////////////////////////////
#elif defined(MIPSDEV)
#include <cyg/infra/diag.h>

#define ms_dprintf(level, fmt, arg...) \
    do { \
        if (level <= DEBUG_LEVEL) diag_printf(fmt, ##arg); \
    } while (0)


////////////////////////////////////// Define for ?? platform ///////////////////////////////
#elif defined(__I_SW__)
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


////////////////////////////////////// Define for Linux pletform ///////////////////////////////
#elif defined(__linux__)
#include "mdrv_jpe.h"
#define ms_dprintf JPE_MSG


////////////////////////////////////// Define for ??? ///////////////////////////////
#else
#define ms_dprintf(level, fmt, arg...)
#endif /* _MSC_VER */

#endif /* MS_DPRINTF_H */

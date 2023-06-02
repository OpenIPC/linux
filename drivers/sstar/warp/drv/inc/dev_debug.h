#include <linux/kernel.h>

#ifndef _DEV_DEBUG_H_
#define _DEV_DEBUG_H_

//for debug
#define LOG_WARP_TIMING (0)

// Defines reference kern levels of printfk
#define DEV_MSG_ERR     3
#define DEV_MSG_WRN     4
#define DEV_MSG_DBG     5

#define DEV_MSG_LEVL    DEV_MSG_WRN

///////////////////////////////////////////////////////////////////////////////////////////////////
#define DEV_MSG_ENABLE

#if defined(DEV_MSG_ENABLE)
#define DEV_MSG_FUNC_ENABLE

#define DEV_STRINGIFY(x) #x
#define DEV_TOSTRING(x) DEV_STRINGIFY(x)

#if defined(DEV_MSG_FUNC_ENABLE)
#define DEV_MSG_TITLE   "[DEV, %s] "
#define DEV_MSG_FUNC    __func__
#else   // NOT defined(DEV_MSG_FUNC_ENABLE)
#define DEV_MSG_TITLE   "[DEV] %s"
#define DEV_MSG_FUNC    ""
#endif  // NOT defined(DEV_MSG_FUNC_ENABLE)

#define DEV_MSG(dbglv, _fmt, _args...)                          \
    do if(dbglv <= DEV_MSG_LEVL) {                              \
        printk(KERN_SOH DEV_TOSTRING(dbglv) DEV_MSG_TITLE  _fmt, DEV_MSG_FUNC, ## _args);   \
    } while(0)

#else   // NOT defined(DEV_MSG_ENABLE)
#define     DEV_MSG(dbglv, _fmt, _args...)
#endif  // NOT defined(DEV_MSG_ENABLE)

#endif // _DEV_DEBUG_H_
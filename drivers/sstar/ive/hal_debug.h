#ifndef __HAL_DEBUG_H__
#define __HAL_DEBUG_H__

#include <linux/kernel.h>

// Defines debug message levels of HAL_MSG
#define HAL_MSG_ERR     3
#define HAL_MSG_WRN     4
#define HAL_MSG_DBG     5

#define HAL_MSG_LEVL    HAL_MSG_WRN

///////////////////////////////////////////////////////////////////////////////////////////////////
#define HAL_MSG_ENABLE          // enable/disable message
#define HAL_MSG_FUNC_ENABLE     // enable/disable function name dump

#if defined(HAL_MSG_ENABLE)

#define HAL_STRINGIFY(x) #x
#define HAL_TOSTRING(x) HAL_STRINGIFY(x)

#if defined(HAL_MSG_FUNC_ENABLE)
#define HAL_MSG_TITLE   "[HAL, %s] "
#define HAL_MSG_FUNC    __func__
#else   // NOT defined(HAL_MSG_FUNC_ENABLE)
#define HAL_MSG_TITLE   "[HAL] %s"
#define HAL_MSG_FUNC    ""
#endif  // NOT defined(HAL_MSG_FUNC_ENABLE)

#define HAL_MSG(dbglv, _fmt, _args...)                                                      \
    do if(dbglv <= HAL_MSG_LEVL) {                                                          \
        printk(KERN_SOH HAL_TOSTRING(dbglv) HAL_MSG_TITLE  _fmt, HAL_MSG_FUNC, ## _args);   \
    } while(0)
#else   // NOT defined(HAL_MSG_ENABLE)
#define     HAL_MSG(dbglv, _fmt, _args...)
#endif  // NOT defined(HAL_MSG_ENABLE)

#endif // __HAL_DEBUG_H__

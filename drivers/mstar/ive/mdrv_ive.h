#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#include "mdrv_ive_data.h"
#include "hal_ive.h"
#include "mdrv_ive_io_st.h"

#ifndef _MDRV_IVE_H_
#define _MDRV_IVE_H_

// SW simulation for driver debug without real HW
// #define IVE_SW_SIMULATE

// Defines reference kern levels of printfk
#define IVE_MSG_ERR     3
#define IVE_MSG_WRN     4
#define IVE_MSG_DBG     5

#define IVE_MSG_LEVL    IVE_MSG_WRN


#define IVE_MSG_ENABLE

#if defined(IVE_MSG_ENABLE)
#define IVE_MSG_FUNC_ENABLE

#define IVE_STRINGIFY(x) #x
#define IVE_TOSTRING(x) IVE_STRINGIFY(x)

#if defined(IVE_MSG_FUNC_ENABLE)
#define IVE_MSG_TITLE   "[IVE, %s] "
#define IVE_MSG_FUNC    __func__
#else   // NOT defined(IVE_MSG_FUNC_ENABLE)
#define IVE_MSG_TITLE   "[IVE] %s"
#define IVE_MSG_FUNC    ""
#endif  // NOT defined(IVE_MSG_FUNC_ENABLE)

#define IVE_ASSERT(_con)   \
    do {\
        if (!(_con)) {\
            printk(KERN_CRIT "BUG at %s:%d assert(%s)\n",\
                    __FILE__, __LINE__, #_con);\
            BUG();\
        }\
    } while (0)

#define IVE_MSG(dbglv, _fmt, _args...)                          \
    do if(dbglv <= IVE_MSG_LEVL) {                              \
        printk(KERN_SOH IVE_TOSTRING(dbglv) IVE_MSG_TITLE  _fmt, IVE_MSG_FUNC, ## _args);   \
    } while(0)

#else   // NOT defined(IVE_MSG_ENABLE)
#define     IVE_ASSERT(arg)
#define     IVE_MSG(dbglv, _fmt, _args...)
#endif  // NOT defined(IVE_MSG_ENABLE)

#endif //_MDRV_IVE_H_

#include <linux/kernel.h>

// Defines reference kern levels of printfk
#define JPE_MSG_ERR     3
#define JPE_MSG_WARNING 4
#define JPE_MSG_DEBUG   5

#define JPE_MSG_LEVEL    JPE_MSG_WARNING


#define JPE_MSG_ENABLE

#ifdef  JPE_MSG_ENABLE
#define JPE_MSG_FUNC_ENABLE

#define JPE_STRINGIFY(x) #x
#define JPE_TOSTRING(x) JPE_STRINGIFY(x)

#ifdef JPE_MSG_FUNC_ENABLE
#define JPE_MSG_TITLE   "[JPE, %s] "
#define JPE_MSG_FUNC    __func__
#else
#define JPE_MSG_TITLE   "[JPE] %s"
#define JPE_MSG_FUNC    ""
#endif

#define JPE_ASSERT(_con)   \
    do {\
        if (!(_con)) {\
            printk(KERN_CRIT "BUG at %s:%d assert(%s)\n",\
                    __FILE__, __LINE__, #_con);\
            BUG();\
        }\
    } while (0)

#define JPE_MSG(dbglv, _fmt, _args...)                          \
    do if(dbglv <= JPE_MSG_LEVEL) {                              \
        printk(KERN_SOH JPE_TOSTRING(dbglv) JPE_MSG_TITLE  _fmt, JPE_MSG_FUNC, ## _args);   \
    } while(0)

#else
#define     JPE_ASSERT(arg)
#define     JPE_MSG(dbglv, _fmt, _args...)

#endif







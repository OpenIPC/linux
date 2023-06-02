#if defined(__KERNEL__)
#include <linux/module.h>
#include "audcap_builtin.h"

EXPORT_SYMBOL(audcap_builtin_get_param);
EXPORT_SYMBOL(audcap_builtin_set_param);
#endif

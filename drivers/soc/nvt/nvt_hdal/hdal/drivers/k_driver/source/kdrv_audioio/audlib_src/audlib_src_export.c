#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"


EXPORT_SYMBOL(audlib_src_init);
EXPORT_SYMBOL(audlib_src_pre_init);
EXPORT_SYMBOL(audlib_src_destroy);
EXPORT_SYMBOL(audlib_src_run);
EXPORT_SYMBOL(audlib_src_get_version);



#endif

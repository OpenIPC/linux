#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"


EXPORT_SYMBOL(audlib_agc_open);
EXPORT_SYMBOL(audlib_agc_is_opened);
EXPORT_SYMBOL(audlib_agc_close);
EXPORT_SYMBOL(audlib_agc_set_config);
EXPORT_SYMBOL(audlib_agc_init);
EXPORT_SYMBOL(audlib_agc_run);


#endif

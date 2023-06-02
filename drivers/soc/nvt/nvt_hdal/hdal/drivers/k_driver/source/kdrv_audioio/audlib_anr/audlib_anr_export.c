#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"


EXPORT_SYMBOL(audlib_anr_get_version);
EXPORT_SYMBOL(audlib_anr_pre_init);
EXPORT_SYMBOL(audlib_anr_init);
EXPORT_SYMBOL(audlib_anr_set_snri);
EXPORT_SYMBOL(audlib_anr_get_snri);
EXPORT_SYMBOL(audlib_anr_detect_reset);
EXPORT_SYMBOL(audlib_anr_detect);
EXPORT_SYMBOL(audlib_anr_run);
EXPORT_SYMBOL(audlib_anr_destroy);

#endif

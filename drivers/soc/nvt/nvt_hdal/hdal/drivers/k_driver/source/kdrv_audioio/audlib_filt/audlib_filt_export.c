#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"


EXPORT_SYMBOL(audlib_filt_open);
EXPORT_SYMBOL(audlib_filt_is_opened);
EXPORT_SYMBOL(audlib_filt_close);
EXPORT_SYMBOL(audlib_filt_set_config);
EXPORT_SYMBOL(audlib_filt_enable_filt);
EXPORT_SYMBOL(audlib_filt_enable_eq);
EXPORT_SYMBOL(audlib_filt_run);
EXPORT_SYMBOL(audlib_filt_init);
EXPORT_SYMBOL(audlib_filt_design_filt);
EXPORT_SYMBOL(audlib_filt_design_eq);


#endif

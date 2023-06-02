#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"



EXPORT_SYMBOL(audlib_aec_open);
EXPORT_SYMBOL(audlib_aec_is_opened);
EXPORT_SYMBOL(audlib_aec_close);
EXPORT_SYMBOL(audlib_aec_get_config);
EXPORT_SYMBOL(audlib_aec_set_config);
EXPORT_SYMBOL(audlib_aec_get_required_buffer_size);
EXPORT_SYMBOL(audlib_aec_init);
EXPORT_SYMBOL(audlib_aec_run);

EXPORT_SYMBOL(audlib_ns_open);
EXPORT_SYMBOL(audlib_ns_is_opened);
EXPORT_SYMBOL(audlib_ns_close);
EXPORT_SYMBOL(audlib_ns_set_config);
EXPORT_SYMBOL(audlib_ns_get_required_buffer_size);
EXPORT_SYMBOL(audlib_ns_init);
EXPORT_SYMBOL(audlib_ns_run);



#endif

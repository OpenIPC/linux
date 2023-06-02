#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"


EXPORT_SYMBOL(kdrv_audioio_open);
EXPORT_SYMBOL(kdrv_audioio_close);
EXPORT_SYMBOL(__kdrv_audioio_set);
EXPORT_SYMBOL(__kdrv_audioio_get);
EXPORT_SYMBOL(kdrv_audioio_trigger);
EXPORT_SYMBOL(kdrv_audioio_trigger_not_start);
EXPORT_SYMBOL(kdrv_audioio_abort);
EXPORT_SYMBOL(kdrv_audioio_get_audiolib);
EXPORT_SYMBOL(kdrv_audioio_reg_audiolib);


#endif

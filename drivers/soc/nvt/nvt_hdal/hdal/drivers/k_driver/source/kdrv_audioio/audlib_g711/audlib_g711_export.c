#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"


EXPORT_SYMBOL(g711_ulaw_encode);
EXPORT_SYMBOL(g711_ulaw_decode);
EXPORT_SYMBOL(g711_alaw_encode);
EXPORT_SYMBOL(g711_alaw_decode);


#endif

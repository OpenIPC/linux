#if defined(__KERNEL__)
#include<linux/module.h>
#include "kdrv_audioio/kdrv_audioio.h"



EXPORT_SYMBOL(audlib_aac_decode_init);
EXPORT_SYMBOL(audlib_aac_decode_one_frame);
EXPORT_SYMBOL(audlib_aac_encode_init);
EXPORT_SYMBOL(audlib_aac_encode_one_frame);





#endif

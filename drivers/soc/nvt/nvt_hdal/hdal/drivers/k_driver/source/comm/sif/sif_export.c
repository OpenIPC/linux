#if defined(__KERNEL__)
#include<linux/module.h>
#include "kwrap/type.h"
#include "comm/sif_api.h"




EXPORT_SYMBOL(sif_open);
EXPORT_SYMBOL(sif_close);
EXPORT_SYMBOL(sif_isOpened);
EXPORT_SYMBOL(sif_setConfig);
EXPORT_SYMBOL(sif_getConfig);
EXPORT_SYMBOL(sif_send);
EXPORT_SYMBOL(sif_waitTransferEnd);



#endif

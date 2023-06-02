#include "linux/module.h"
#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videodec/kdrv_videodec.h"

#if defined(__LINUX)
EXPORT_SYMBOL(kdrv_videoenc_open);
EXPORT_SYMBOL(kdrv_videoenc_close);
EXPORT_SYMBOL(kdrv_videoenc_trigger);
EXPORT_SYMBOL(kdrv_videoenc_get);
EXPORT_SYMBOL(kdrv_videoenc_set);
//EXPORT_SYMBOL(rc_cb_init);

EXPORT_SYMBOL(kdrv_videodec_open);
EXPORT_SYMBOL(kdrv_videodec_close);
EXPORT_SYMBOL(kdrv_videodec_trigger);
EXPORT_SYMBOL(kdrv_videodec_get);
EXPORT_SYMBOL(kdrv_videodec_set);
#endif


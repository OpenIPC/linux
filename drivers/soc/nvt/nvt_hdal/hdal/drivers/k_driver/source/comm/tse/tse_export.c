#include<linux/module.h>
#include "comm/tse.h"

EXPORT_SYMBOL(tse_open);
EXPORT_SYMBOL(tse_close);
EXPORT_SYMBOL(tse_isOpened);
EXPORT_SYMBOL(tse_start);
EXPORT_SYMBOL(tse_waitDone);
EXPORT_SYMBOL(tse_setConfig);
EXPORT_SYMBOL(tse_getConfig);
EXPORT_SYMBOL(tse_getIntStatus);


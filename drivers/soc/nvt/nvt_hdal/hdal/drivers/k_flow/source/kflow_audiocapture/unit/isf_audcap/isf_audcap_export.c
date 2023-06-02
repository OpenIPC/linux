#include <linux/module.h>
#include "isf_audcap_int.h"

EXPORT_SYMBOL(ctl_aud_reg_auddrv);
EXPORT_SYMBOL(ctl_aud_unreg_auddrv);
EXPORT_SYMBOL(ctl_aud_module_init_cfg);

EXPORT_SYMBOL(wavstudio_push_buf_to_queue);
EXPORT_SYMBOL(wavstudio_get_remain_buf);
EXPORT_SYMBOL(wavstudio_push_buf_to_queue2);
EXPORT_SYMBOL(wavstudio_get_remain_buf2);
EXPORT_SYMBOL(wavstudio_open);
EXPORT_SYMBOL(wavstudio_close);
EXPORT_SYMBOL(wavstudio_start);
EXPORT_SYMBOL(wavstudio_stop);
EXPORT_SYMBOL(wavstudio_pause);
EXPORT_SYMBOL(wavstudio_resume);
EXPORT_SYMBOL(wavstudio_set_config);
EXPORT_SYMBOL(wavstudio_get_config);
EXPORT_SYMBOL(wavstudio_wait_start);
EXPORT_SYMBOL(wavstudio_wait_stop);
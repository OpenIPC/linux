#if defined(__KERNEL__)
#include<linux/module.h>
#include "eac.h"

EXPORT_SYMBOL(eac_enableclk);
EXPORT_SYMBOL(eac_enableadcclk);
EXPORT_SYMBOL(eac_enabledacclk);
EXPORT_SYMBOL(eac_setdacclkrate);
EXPORT_SYMBOL(eac_init);
EXPORT_SYMBOL(eac_set_ad_enable);
EXPORT_SYMBOL(eac_set_da_enable);
EXPORT_SYMBOL(eac_get_ad_enable);
EXPORT_SYMBOL(eac_get_da_enable);
EXPORT_SYMBOL(eac_set_load);
EXPORT_SYMBOL(eac_set_ad_config);
EXPORT_SYMBOL(eac_get_ad_config);
EXPORT_SYMBOL(eac_set_iir_coef);
EXPORT_SYMBOL(eac_set_da_config);
EXPORT_SYMBOL(eac_get_da_config);
EXPORT_SYMBOL(eac_is_dac_enable);
EXPORT_SYMBOL(eac_set_dac_output);
EXPORT_SYMBOL(eac_get_dac_output);
EXPORT_SYMBOL(eac_get_cur_pgagain);
EXPORT_SYMBOL(eac_set_phypower);
EXPORT_SYMBOL(eac_set_phydacpower);



#endif

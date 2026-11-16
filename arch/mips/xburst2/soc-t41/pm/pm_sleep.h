#ifndef __PM_SLEEP_H__
#define __PM_SLEEP_H__



void load_func_to_sram(void);
int soc_pm_idle_pd_config(void);
int soc_pm_idle_config(void);
int soc_pm_sleep_config(void);
int soc_pm_wakeup_idle_sleep(void);
void soc_set_reset_entry(void);



#endif


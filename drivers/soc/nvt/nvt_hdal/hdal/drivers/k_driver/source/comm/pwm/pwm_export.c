#include<linux/module.h>
#include "comm/pwm.h"

EXPORT_SYMBOL(pwm_open);
EXPORT_SYMBOL(pwm_open_set);
EXPORT_SYMBOL(pwm_pwm_get_cycle_number);
EXPORT_SYMBOL(pwm_close);
EXPORT_SYMBOL(pwm_close_set);
EXPORT_SYMBOL(pwm_wait);
EXPORT_SYMBOL(pwm_wait_set);
EXPORT_SYMBOL(pwm_pwm_reload);
EXPORT_SYMBOL(pwm_pwm_enable);
EXPORT_SYMBOL(pwm_pwm_disable);
EXPORT_SYMBOL(pwm_pwm_enable_set);
EXPORT_SYMBOL(pwm_pwm_disable_set);
EXPORT_SYMBOL(pwm_pwm_config);
EXPORT_SYMBOL(pwm_pwm_config_clock_div);
EXPORT_SYMBOL(pwm_pwm_reload_config);
EXPORT_SYMBOL(pwm_mstep_enable);
EXPORT_SYMBOL(pwm_mstep_disable);
EXPORT_SYMBOL(pwm_mstep_enable_set);
EXPORT_SYMBOL(pwm_mstep_disable_set);
EXPORT_SYMBOL(pwm_mstep_clock_div_reload);
EXPORT_SYMBOL(pwm_mstep_config);
EXPORT_SYMBOL(pwm_mstep_target_count_wait_done);
EXPORT_SYMBOL(pwm_mstep_clk_div_wait_load_done);
EXPORT_SYMBOL(pwm_mstep_config_clock_div);
EXPORT_SYMBOL(pwm_mstep_config_target_count_enable);
EXPORT_SYMBOL(pwm_mstep_config_set);
EXPORT_SYMBOL(pwm_ccnt_enable);
EXPORT_SYMBOL(pwm_ccnt_reload);
EXPORT_SYMBOL(pwm_ccnt_get_current_val);
EXPORT_SYMBOL(pwm_ccnt_get_start_value);
EXPORT_SYMBOL(pwm_ccnt_get_edge_interval);
EXPORT_SYMBOL(pwm_ccnt_config);
EXPORT_SYMBOL(pwm_set_config);
EXPORT_SYMBOL(pwm_ccnt_config_timeout_enable);



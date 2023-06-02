#ifndef _PWM_FULLHAN_COMMON_H_
#define _PWM_FULLHAN_COMMON_H_

#if defined(CONFIG_ARCH_FH885xV200) || \
	defined(CONFIG_ARCH_FH865x)     || \
	defined(CONFIG_ARCH_FH8636)     || \
	defined(CONFIG_ARCH_FH8852V101)
#define PWM_REG_INTERVAL       (0x80)
#else
#define PWM_REG_INTERVAL       (0x100)
#endif
unsigned int _pwm_set_stoptime_bit(int chn);
void _pwm_interrupt_finishall_enable(void *base, int chn);
void _pwm_interrupt_finishall_disable(void *base, int chn);
void _pwm_interrupt_finishonce_enable(void *base, int chn);
void _pwm_interrupt_finishonce_disable(void *base, int chn);
void _pwm_interrupt_finishall_clear(void *base, int chn);
void _pwm_interrupt_finishonce_clear(void *base, int chn);
int _pwm_need_inverse(void);
void _pwm_interrupt_process(unsigned int status, int *chn_once, int *chn_all);

#endif /* _PWM_FULLHAN_COMMON_H_ */

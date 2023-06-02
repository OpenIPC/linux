#ifndef _PWM_FULLHAN_H_
#define _PWM_FULLHAN_H_

#include <linux/ioctl.h>

#define DEVICE_NAME					"fh_pwm"
#define FH_PWM_PROC_FILE			"driver/pwm"

#define PWM_IOCTL_MAGIC				'p'
#define ENABLE_PWM					_IOWR(PWM_IOCTL_MAGIC, 0, __u32)
#define DISABLE_PWM					_IOWR(PWM_IOCTL_MAGIC, 1, __u32)

#define SET_PWM_DUTY_CYCLE			_IOWR(PWM_IOCTL_MAGIC, 2, __u32)
#define GET_PWM_DUTY_CYCLE			_IOWR(PWM_IOCTL_MAGIC, 3, __u32)
#define SET_PWM_DUTY_CYCLE_PERCENT	_IOWR(PWM_IOCTL_MAGIC, 4, __u32)
#define SET_PWM_ENABLE				_IOWR(PWM_IOCTL_MAGIC, 5, __u32)
#define ENABLE_MUL_PWM				_IOWR(PWM_IOCTL_MAGIC, 6, __u32)
#define ENABLE_FINSHALL_INTR		_IOWR(PWM_IOCTL_MAGIC, 7, __u32)
#define ENABLE_FINSHONCE_INTR		_IOWR(PWM_IOCTL_MAGIC, 8, __u32)
#define DISABLE_FINSHALL_INTR		_IOWR(PWM_IOCTL_MAGIC, 9, __u32)
#define DISABLE_FINSHONCE_INTR		_IOWR(PWM_IOCTL_MAGIC, 10, __u32)
#define WAIT_PWM_FINSHALL			_IOWR(PWM_IOCTL_MAGIC, 12, __u32)

#define PWM_IOCTL_MAXNR				16

struct fh_pwm_config
{
	unsigned int period_ns;
	unsigned int duty_ns;
	unsigned int pulses;
#define FH_PWM_STOPLVL_LOW		(0x0)
#define FH_PWM_STOPLVL_HIGH		(0x3)
#define FH_PWM_STOPLVL_KEEP		(0x1)

#define FH_PWM_STOPCTRL_ATONCE		(0x10)
#define FH_PWM_STOPCTRL_AFTERFINISH	(0x00)
	unsigned int stop;
	unsigned int delay_ns;
	unsigned int phase_ns;
	unsigned int percent;
	unsigned int finish_once;
	unsigned int finish_all;
};

struct fh_pwm_status {
	unsigned int done_cnt;
	unsigned int total_cnt;
	unsigned int busy;
	unsigned int error;
};

struct fh_pwm_chip_data
{
	int id;
	struct fh_pwm_config config;
	struct fh_pwm_status status;
};

#endif /* _PWM_FULLHAN_H_ */

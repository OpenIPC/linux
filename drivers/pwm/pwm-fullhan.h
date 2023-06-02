#ifndef FH_PMU_H_
#define FH_PMU_H_

#include <linux/slab.h>
#include <linux/ioctl.h>

#define DEVICE_NAME					"fh_pwm"
#define FH_PWM_PROC_FILE            "driver/pwm"

#define REG_PWM_CTRL				(0x00)
#define REG_PWM_CMD(n)          	(((n) * 4) + REG_PWM_CTRL + 4)

#define PWM_IOCTL_MAGIC 			'p'
#define ENABLE_PWM	 				_IOWR(PWM_IOCTL_MAGIC, 0, __u32)
#define DISABLE_PWM		 			_IOWR(PWM_IOCTL_MAGIC, 1, __u32)

#define SET_PWM_DUTY_CYCLE			_IOWR(PWM_IOCTL_MAGIC, 2, __u32)
#define GET_PWM_DUTY_CYCLE 		    _IOWR(PWM_IOCTL_MAGIC, 3, __u32)
#define SET_PWM_DUTY_CYCLE_PERCENT  _IOWR(PWM_IOCTL_MAGIC, 4, __u32)
#define PWM_IOCTL_MAXNR 			8

struct fh_pwm_chip_data
{
	int id;
	int working;
	u32 period_ns;
	u32 counter_ns;
	int percent;
};

#endif /* FH_PMU_H_ */

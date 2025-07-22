#ifndef _LINUX_TCU_H
#define _LINUX_TCU_H

#include <linux/interrupt.h>
enum tcu_mode {
	TCU1_MODE = 1,
	TCU2_MODE = 2,
};

enum tcu_clk_mode {
	PCLK_EN = 0, /*timer input clock is PCLK*/
	RTC_EN = 1,/*timer input clock is RTC*/
	EXT_EN = 2,/*timer input clock is EXT*/
	CLK_MASK = 3,
};
enum tcu_irq_mode {
	FULL_IRQ_MODE = 1,
	HALF_IRQ_MODE = 2,
	FULL_HALF_IRQ_MODE = 3,
	NULL_IRQ_MODE = 4,
};

struct tcu_device {
	int id;
	short using;
	short pwm_flag;

	enum tcu_irq_mode irq_type;
	enum tcu_clk_mode clock;
	enum tcu_mode tcumode;
	int half_num;
	int full_num;
	int count_value;
	unsigned int init_level; /*used in pwm output mode*/
	unsigned int divi_ratio;  /*  0/1/2/3/4/5/something else------>1/4/16/64/256/1024/mask  */
	unsigned int pwm_shutdown; /*0-->graceful shutdown   1-->abrupt shutdown only use in TCU1_MODE*/
	struct tasklet_struct	tasklet;
};

struct tcu_device *tcu_request(int channel_num,void (*channel_handler)(unsigned long));
void tcu_free(struct tcu_device *tcu);
int tcu_as_timer_config(struct tcu_device *tcu);
int tcu_as_counter_config(struct tcu_device *tcu);
int tcu_enable(struct tcu_device *tcu);
void tcu_disable(struct tcu_device *tcu);
#endif

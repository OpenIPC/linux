
#ifndef FH_STEPMOTOR_HW_H_
#define FH_STEPMOTOR_HW_H_

#include <linux/ioctl.h>





#define MAX_FHSM_NR (2)



#define MOTOR_CTRL     (0x0)
#define MOTOR_RESET    (0x4)
#define MOTOR_MODE     (0x8)
#define MOTOR_TIMING0     (0xc)
#define MOTOR_TIMING1     (0x10)
#define MOTOR_MANUAL_CONFIG0     (0x14)
#define MOTOR_MANUAL_CONFIG1     (0x18)
#define MOTOR_INT_EN     (0x1c)
#define MOTOR_INT_STATUS     (0x20)
#define MOTOR_STATUS0     (0x24)

#define MOTOR_MEM     (0x100)



enum fh_sm_mode {
	fh_sm_constant = 0,
	fh_sm_high = 1,
	fh_sm_manual_4 = 2,
	fh_sm_manual_8 = 3,
};
struct fh_sm_timingparam {
	int period;
	int counter;
	int copy;
	int microstep;
};


struct fh_sm_param
{
	 enum fh_sm_mode mode;
	int direction;
	int output_invert_A;
	int output_invert_B;
	unsigned int manual_pwm_choosenA;// please lookup doc to set
	unsigned int manual_pwm_choosenB;// please lookup doc to set
	struct fh_sm_timingparam timingparam;
};

struct fh_sm_lut
{
	int lutsize;
	int *lut;
};

struct fh_sm_obj_t{
	int irq_no;
	void* regs;
	struct clk *clk;
	struct completion	run_complete;
	char isrname[24];
};


irqreturn_t fh_stepmotor_isr(int irq, void *dev_id);
int fh_stepmotor_is_busy(struct fh_sm_obj_t *obj);
void fh_stepmotor_set_hw_param(struct fh_sm_obj_t *obj,
	struct fh_sm_param *param);
void fh_stepmotor_get_hw_param(struct fh_sm_obj_t *obj,
	struct fh_sm_param *param);
int fh_stepmotor_start_sync(struct fh_sm_obj_t *obj,
	int cycles);
void fh_stepmotor_start_async(struct fh_sm_obj_t *obj,
	int cycles);
int fh_stepmotor_stop(struct fh_sm_obj_t *obj);
int fh_stepmotor_get_current_cycle(struct fh_sm_obj_t *obj);
int fh_stepmotor_set_lut(struct fh_sm_obj_t *obj,struct fh_sm_lut *lut);
int fh_stepmotor_get_lutsize(struct fh_sm_obj_t *obj);
int fh_stepmotor_get_lut(struct fh_sm_obj_t* obj,struct fh_sm_lut *lut);
void fh_stepmotor_init_hw_param(struct fh_sm_obj_t *obj);
#endif /* FH_STEPMOTOR_HW_H_ */

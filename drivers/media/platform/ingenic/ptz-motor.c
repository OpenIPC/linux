// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 PTZ stepper motor driver using hrtimer pulse generator
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <media/media-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>

#define MOTOR_STOP_DECEL_THRESH_X	31
#define MOTOR_STOP_DECEL_TARGET_X	29
#define MOTOR_STOP_DECEL_THRESH_Y	8
#define MOTOR_STOP_DECEL_TARGET_Y	6

#define MOTOR_PHASE_ST1			BIT(3)
#define MOTOR_PHASE_ST2			BIT(2)
#define MOTOR_PHASE_ST3			BIT(1)
#define MOTOR_PHASE_ST4			BIT(0)

static const unsigned char step_8[8] = {
	0x08, 0x0c, 0x04, 0x06, 0x02, 0x03, 0x01, 0x09
};

static const unsigned char pan_ramp_div[16] = {
	0x02, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x00,
	0x04, 0x03, 0x02, 0x00, 0x04, 0x03, 0x02, 0x01
};

enum motor_channel_state {
	MOTOR_STATE_MOVE = 0,
	MOTOR_STATE_CRUISE = 1,
	MOTOR_STATE_RESET = 2,
	MOTOR_STATE_STOP = 3,
};

struct motor_coils {
	struct gpio_desc *coil[4];
};

struct motor_channel {
	struct motor_coils *coils;
	int max_steps;
	int cur_steps;
	int reset_steps;
	int direction;
	enum motor_channel_state state;
	wait_queue_head_t wait;
};

struct motor_device {
	struct kref ref;
	struct device *dev;
	struct media_device mdev;
	struct v4l2_device v4l2_dev;
	struct v4l2_subdev sd;
	atomic_t subdev_kref;
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *pan_rel;
	struct v4l2_ctrl *tilt_rel;
	struct v4l2_ctrl *pan_abs;
	struct v4l2_ctrl *tilt_abs;
	struct v4l2_ctrl *pan_reset;
	struct v4l2_ctrl *tilt_reset;
	struct v4l2_ctrl *pan_speed_ctrl;
	struct v4l2_ctrl *tilt_speed_ctrl;
	struct motor_coils coils[2];
	struct gpio_desc *switch_gpiod;
	struct motor_channel motors[2];
	int pan_max_steps;
	int tilt_max_steps;
	u32 pan_max_degrees;
	u32 tilt_max_degrees;
	int pan_reset_steps;
	int tilt_reset_steps;
	int tilt_step_div;
	const unsigned char *ramp_div;
	unsigned int irq_count;
	struct completion completion;
	bool wait_flag;
	struct hrtimer timer;
	ktime_t step_interval;
	int pan_speed;
	int tilt_speed;
	struct mutex dev_mutex;
	spinlock_t lock;
	enum motor_channel_state status;
	int switch_flag;
	int target_step_x;
	int target_step_y;
	int steps_x;
	int steps_y;
};

static s32 motor_steps_to_arcsec(int steps, int max_steps, u32 max_degrees)
{
	u32 total_arcsec = max_degrees * 3600U;
	u64 num;

	if (max_steps <= 0)
		return 0;
	if (steps >= 0) {
		num = (u64)steps * total_arcsec;
		return (s32)DIV_ROUND_CLOSEST_ULL(num, (u32)max_steps);
	}
	num = (u64)(-(s64)steps) * total_arcsec;
	return -(s32)DIV_ROUND_CLOSEST_ULL(num, (u32)max_steps);
}

static int motor_arcsec_to_steps(s32 arcsec, int max_steps, u32 max_degrees)
{
	u32 total_arcsec = max_degrees * 3600U;
	u64 num;

	if (!total_arcsec)
		return 0;
	if (arcsec >= 0) {
		num = (u64)arcsec * (u32)max_steps;
		return (int)DIV_ROUND_CLOSEST_ULL(num, total_arcsec);
	}
	num = (u64)(-(s64)arcsec) * (u32)max_steps;
	return -(int)DIV_ROUND_CLOSEST_ULL(num, total_arcsec);
}

static void motor_timer_start(struct motor_device *mdev, int speed_val)
{
	unsigned int speed = clamp_t(int, speed_val, 100, 900);

	mdev->step_interval = ktime_set(0, div_u64(1000000000ULL, speed));
	hrtimer_start(&mdev->timer, mdev->step_interval, HRTIMER_MODE_REL);
}

static void motor_move_step(struct motor_device *mdev, int motor_idx)
{
	struct motor_channel *chan = &mdev->motors[motor_idx];
	struct motor_coils *coils = chan->coils;
	unsigned char phase = 0;
	int step;

	if (chan->state != MOTOR_STATE_STOP) {
		step = chan->cur_steps % 8;
		if (step < 0)
			step += 8;
		phase = step_8[step];
	}

	if (coils->coil[0])
		gpiod_set_value(coils->coil[0], !!(phase & MOTOR_PHASE_ST1));
	if (coils->coil[1])
		gpiod_set_value(coils->coil[1], !!(phase & MOTOR_PHASE_ST2));
	if (coils->coil[2])
		gpiod_set_value(coils->coil[2], !!(phase & MOTOR_PHASE_ST3));
	if (coils->coil[3])
		gpiod_set_value(coils->coil[3], !!(phase & MOTOR_PHASE_ST4));

	if (chan->state == MOTOR_STATE_RESET)
		chan->reset_steps++;
}

static bool motor_step_tick(struct motor_device *mdev)
{
	struct motor_channel *m0 = &mdev->motors[0];
	struct motor_channel *m1 = &mdev->motors[1];
	unsigned int count, skip;

	if (m0->state == MOTOR_STATE_STOP && m1->state == MOTOR_STATE_STOP) {
		mdev->status = MOTOR_STATE_STOP;
		motor_move_step(mdev, 0);
		motor_move_step(mdev, 1);
		if (mdev->wait_flag) {
			mdev->wait_flag = false;
			complete(&mdev->completion);
		}
		return false;
	}

	if (m0->cur_steps < 1) {
		if (m0->state == MOTOR_STATE_MOVE && m0->direction == -1)
			m0->state = MOTOR_STATE_STOP;
		else if (m0->state == MOTOR_STATE_CRUISE)
			m0->direction = 1;
		m0->cur_steps = 0;
	}

	if (m0->cur_steps >= m0->max_steps) {
		if (m0->state == MOTOR_STATE_RESET) {
			m0->state = MOTOR_STATE_STOP;
			m0->max_steps = mdev->pan_max_steps;
			wake_up(&m0->wait);
			m0->direction = -1;
		} else if (m0->state == MOTOR_STATE_MOVE) {
			if (m0->direction == 1)
				m0->state = MOTOR_STATE_STOP;
		} else {
			m0->direction = -1;
		}
		m0->cur_steps = m0->max_steps;
	}

	if (m1->cur_steps < 1) {
		if (m1->state == MOTOR_STATE_MOVE && m1->direction == -1)
			m1->state = MOTOR_STATE_STOP;
		else if (m1->state == MOTOR_STATE_CRUISE)
			m1->direction = 1;
		m1->cur_steps = 0;
	}

	if (m1->cur_steps >= m1->max_steps) {
		if (m1->state == MOTOR_STATE_RESET) {
			m1->state = MOTOR_STATE_STOP;
			m1->max_steps = mdev->tilt_max_steps;
			wake_up(&m1->wait);
			m1->direction = -1;
		} else if (m1->state == MOTOR_STATE_MOVE) {
			if (m1->direction == 1)
				m1->state = MOTOR_STATE_STOP;
		} else {
			m1->direction = -1;
		}
		m1->cur_steps = m1->max_steps;
	}

	if (mdev->status == MOTOR_STATE_CRUISE) {
		m0->cur_steps += m0->direction;
		count = ++mdev->irq_count;
		if (!(count % mdev->tilt_step_div))
			m1->cur_steps += m1->direction;
		motor_move_step(mdev, 0);
		motor_move_step(mdev, 1);
		return true;
	}

	if (mdev->status == MOTOR_STATE_RESET) {
		if (m0->state != MOTOR_STATE_STOP) {
			m0->cur_steps += m0->direction;
			motor_move_step(mdev, 0);
			mdev->steps_x++;
		}
		if (m1->state != MOTOR_STATE_STOP) {
			m1->cur_steps += m1->direction;
			motor_move_step(mdev, 1);
			mdev->steps_y++;
		}
		return true;
	}

	count = ++mdev->irq_count;
	if (mdev->steps_x < mdev->target_step_x && m0->state != MOTOR_STATE_STOP) {
		skip = min_t(unsigned int,
			     (mdev->target_step_x - mdev->steps_x) / 10, 3);
		skip = mdev->ramp_div[skip];
		if (skip && !(count % skip)) {
			m0->cur_steps += m0->direction;
			motor_move_step(mdev, 0);
			mdev->steps_x++;
		}
	} else {
		m0->state = MOTOR_STATE_STOP;
		motor_move_step(mdev, 0);
	}

	if (mdev->steps_y < mdev->target_step_y && m1->state != MOTOR_STATE_STOP) {
		if (count % mdev->tilt_step_div)
			return true;
		m1->cur_steps += m1->direction;
		mdev->steps_y++;
	} else {
		m1->state = MOTOR_STATE_STOP;
	}
	motor_move_step(mdev, 1);
	return true;
}

static enum hrtimer_restart motor_timer_callback(struct hrtimer *timer)
{
	struct motor_device *mdev = container_of(timer, struct motor_device, timer);
	unsigned long flags;
	ktime_t interval;
	bool running;

	spin_lock_irqsave(&mdev->lock, flags);
	running = motor_step_tick(mdev);
	interval = mdev->step_interval;
	spin_unlock_irqrestore(&mdev->lock, flags);

	if (running) {
		hrtimer_forward_now(timer, interval);
		return HRTIMER_RESTART;
	}

	return HRTIMER_NORESTART;
}

static void motor_set_default(struct motor_device *mdev)
{
	int i;

	mdev->status = MOTOR_STATE_STOP;
	for (i = 0; i < 2; i++) {
		mdev->motors[i].state = MOTOR_STATE_STOP;
		motor_move_step(mdev, i);
	}
}

static void motor_get_message(struct motor_device *mdev, unsigned int *msg)
{
	unsigned long flags;

	mutex_lock(&mdev->dev_mutex);
	spin_lock_irqsave(&mdev->lock, flags);
	msg[0] = mdev->motors[0].cur_steps;
	msg[1] = mdev->motors[1].cur_steps;

	if (mdev->switch_flag == 1 &&
	    mdev->motors[1].max_steps != mdev->tilt_max_steps + 30) {
		msg[0] = mdev->motors[1].cur_steps;
		msg[1] = mdev->motors[0].cur_steps;
	}

	msg[2] = mdev->status == MOTOR_STATE_STOP ? 0 : 1;
	spin_unlock_irqrestore(&mdev->lock, flags);
	mutex_unlock(&mdev->dev_mutex);
}

static int motor_ops_move(struct motor_device *mdev, int hstep, int vstep)
{
	unsigned long flags;
	int step_val;

	if (hstep == INT_MIN || vstep == INT_MIN)
		return -ERANGE;
	if (!hstep && !vstep)
		return 0;

	mutex_lock(&mdev->dev_mutex);
	spin_lock_irqsave(&mdev->lock, flags);

	if (!mdev->switch_gpiod) {
		if ((hstep > 0 &&
		     mdev->motors[0].cur_steps >= mdev->motors[0].max_steps) ||
		    (hstep < 0 && mdev->motors[0].cur_steps <= 0))
			hstep = 0;
		if ((vstep > 0 &&
		     mdev->motors[1].cur_steps >= mdev->motors[1].max_steps) ||
		    (vstep < 0 && mdev->motors[1].cur_steps <= 0))
			vstep = 0;
		if (!hstep && !vstep)
			goto unlock;

		mdev->target_step_x = abs(hstep);
		mdev->target_step_y = abs(vstep);
		mdev->ramp_div = &pan_ramp_div[min_t(unsigned int,
						     mdev->target_step_x / 10, 3) * 4];
		mdev->irq_count = 0;
		mdev->status = MOTOR_STATE_MOVE;
		mdev->steps_x = 0;
		mdev->steps_y = 0;
		mdev->motors[0].state = MOTOR_STATE_MOVE;
		mdev->motors[0].direction = (hstep > 0) ? 1 : -1;
		mdev->motors[1].state = MOTOR_STATE_MOVE;
		mdev->motors[1].direction = (vstep > 0) ? 1 : -1;
		motor_timer_start(mdev,
			(hstep && vstep) ? max(mdev->pan_speed, mdev->tilt_speed) :
			(hstep ? mdev->pan_speed : mdev->tilt_speed));
		goto unlock;
	}

	if (hstep == 0) {
		if (mdev->switch_flag == 1) {
			if (mdev->motors[1].max_steps != mdev->tilt_max_steps + 30) {
				int tmp = mdev->motors[1].cur_steps;

				mdev->motors[1].max_steps = mdev->tilt_max_steps + 30;
				mdev->motors[1].cur_steps = mdev->motors[0].cur_steps;
				mdev->motors[0].cur_steps = tmp;
			}
			mdev->switch_flag = 0;
		}
		gpiod_set_value(mdev->switch_gpiod, 1);
		step_val = vstep;
	} else {
		gpiod_set_value(mdev->switch_gpiod, 0);
		if (mdev->motors[1].max_steps != mdev->pan_max_steps + 100) {
			int tmp = mdev->motors[0].cur_steps;

			mdev->motors[1].max_steps = mdev->pan_max_steps + 100;
			mdev->motors[0].cur_steps = mdev->motors[1].cur_steps;
			mdev->motors[1].cur_steps = tmp;
		}
		mdev->switch_flag = 1;
		step_val = hstep;
	}

	if ((step_val > 0 &&
	     mdev->motors[1].cur_steps >= mdev->motors[1].max_steps) ||
	    (step_val < 0 && mdev->motors[1].cur_steps <= 0))
		goto unlock;

	mdev->ramp_div = pan_ramp_div;
	mdev->irq_count = 0;
	mdev->status = MOTOR_STATE_MOVE;
	mdev->target_step_x = 0;
	mdev->target_step_y = abs(step_val);
	mdev->steps_x = 0;
	mdev->steps_y = 0;
	mdev->motors[0].state = MOTOR_STATE_MOVE;
	mdev->motors[0].direction = -1;
	mdev->motors[1].state = MOTOR_STATE_MOVE;
	mdev->motors[1].direction = (step_val > 0) ? 1 : -1;

	motor_timer_start(mdev, hstep ? mdev->pan_speed : mdev->tilt_speed);

unlock:
	spin_unlock_irqrestore(&mdev->lock, flags);
	mutex_unlock(&mdev->dev_mutex);
	return 0;
}

static int motor_ops_stop(struct motor_device *mdev)
{
	unsigned long flags;
	long ret = 1;

	mutex_lock(&mdev->dev_mutex);
	spin_lock_irqsave(&mdev->lock, flags);
	if (mdev->status != MOTOR_STATE_STOP) {
		if (mdev->status == MOTOR_STATE_MOVE) {
			if ((u32)mdev->target_step_x -
			    (u32)mdev->steps_x >= MOTOR_STOP_DECEL_THRESH_X) {
				mdev->target_step_x = MOTOR_STOP_DECEL_TARGET_X;
				mdev->steps_x = 0;
				mdev->ramp_div = &pan_ramp_div[min_t(unsigned int,
								     mdev->target_step_x / 10, 3) * 4];
			}
			if ((u32)mdev->target_step_y -
			    (u32)mdev->steps_y > MOTOR_STOP_DECEL_THRESH_Y) {
				mdev->target_step_y = MOTOR_STOP_DECEL_TARGET_Y;
				mdev->steps_y = 0;
			}
		} else if (mdev->status == MOTOR_STATE_CRUISE) {
			mdev->status = MOTOR_STATE_MOVE;
			mdev->motors[0].state = MOTOR_STATE_MOVE;
			mdev->motors[1].state = MOTOR_STATE_MOVE;
			mdev->target_step_x = 0;
			mdev->target_step_y = 0;
			mdev->steps_x = 0;
			mdev->steps_y = 0;
		}
		mdev->irq_count = 0;
		reinit_completion(&mdev->completion);
		mdev->wait_flag = true;
		spin_unlock_irqrestore(&mdev->lock, flags);

		ret = wait_for_completion_interruptible_timeout(&mdev->completion,
								msecs_to_jiffies(15000));

		spin_lock_irqsave(&mdev->lock, flags);
	}
	mdev->status = MOTOR_STATE_STOP;
	mdev->motors[0].state = MOTOR_STATE_STOP;
	mdev->motors[1].state = MOTOR_STATE_STOP;
	spin_unlock_irqrestore(&mdev->lock, flags);

	hrtimer_cancel(&mdev->timer);

	spin_lock_irqsave(&mdev->lock, flags);
	if (mdev->switch_flag == 1) {
		if (mdev->motors[1].max_steps != mdev->tilt_max_steps + 30) {
			int tmp = mdev->motors[1].cur_steps;

			mdev->motors[1].max_steps = mdev->tilt_max_steps + 30;
			mdev->motors[1].cur_steps = mdev->motors[0].cur_steps;
			mdev->motors[0].cur_steps = tmp;
		}
		mdev->switch_flag = 0;
	}
	mdev->wait_flag = false;
	motor_set_default(mdev);
	spin_unlock_irqrestore(&mdev->lock, flags);
	mutex_unlock(&mdev->dev_mutex);

	if (ret < 0)
		return ret;
	if (!ret)
		return -ETIMEDOUT;
	return 0;
}

static int motor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct motor_device *mdev = container_of(ctrl->handler, struct motor_device, ctrls);
	unsigned int msg[3];

	switch (ctrl->id) {
	case V4L2_CID_PAN_RELATIVE:
		return motor_ops_move(mdev,
			motor_arcsec_to_steps(ctrl->val, mdev->pan_max_steps,
					      mdev->pan_max_degrees),
			0);
	case V4L2_CID_TILT_RELATIVE:
		return motor_ops_move(mdev, 0,
			motor_arcsec_to_steps(ctrl->val, mdev->tilt_max_steps,
					      mdev->tilt_max_degrees));
	case V4L2_CID_PAN_ABSOLUTE: {
		int target = motor_arcsec_to_steps(ctrl->val, mdev->pan_max_steps,
						   mdev->pan_max_degrees);

		motor_get_message(mdev, msg);
		return motor_ops_move(mdev, target - (int)msg[0], 0);
	}
	case V4L2_CID_TILT_ABSOLUTE: {
		int target = motor_arcsec_to_steps(ctrl->val, mdev->tilt_max_steps,
						   mdev->tilt_max_degrees);

		motor_get_message(mdev, msg);
		return motor_ops_move(mdev, 0, target - (int)msg[1]);
	}
	case V4L2_CID_PAN_RESET:
		motor_get_message(mdev, msg);
		return motor_ops_move(mdev, mdev->pan_reset_steps - (int)msg[0], 0);
	case V4L2_CID_TILT_RESET:
		motor_get_message(mdev, msg);
		return motor_ops_move(mdev, 0, mdev->tilt_reset_steps - (int)msg[1]);
	case V4L2_CID_PAN_SPEED:
	case V4L2_CID_TILT_SPEED: {
		unsigned long flags;

		if (ctrl->val < 0 || ctrl->val > 900)
			return -EINVAL;
		mutex_lock(&mdev->dev_mutex);
		spin_lock_irqsave(&mdev->lock, flags);
		if (ctrl->id == V4L2_CID_PAN_SPEED)
			mdev->pan_speed = ctrl->val;
		else
			mdev->tilt_speed = ctrl->val;
		spin_unlock_irqrestore(&mdev->lock, flags);
		mutex_unlock(&mdev->dev_mutex);
		if (!ctrl->val)
			return motor_ops_stop(mdev);
		return 0;
	}
	default:
		return -EINVAL;
	}
}

static int motor_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct motor_device *mdev = container_of(ctrl->handler, struct motor_device, ctrls);
	unsigned int msg[3];

	motor_get_message(mdev, msg);
	switch (ctrl->id) {
	case V4L2_CID_PAN_ABSOLUTE:
		ctrl->val = motor_steps_to_arcsec(msg[0], mdev->pan_max_steps,
						  mdev->pan_max_degrees);
		return 0;
	case V4L2_CID_TILT_ABSOLUTE:
		ctrl->val = motor_steps_to_arcsec(msg[1], mdev->tilt_max_steps,
						  mdev->tilt_max_degrees);
		return 0;
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops motor_ctrl_ops = {
	.s_ctrl = motor_s_ctrl,
	.g_volatile_ctrl = motor_g_volatile_ctrl,
};

static const struct v4l2_subdev_core_ops motor_subdev_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops motor_subdev_ops = {
	.core = &motor_subdev_core_ops,
};

static void motor_free(struct kref *ref)
{
	struct motor_device *mdev = container_of(ref, struct motor_device, ref);

	v4l2_ctrl_handler_free(&mdev->ctrls);
	kfree(mdev);
}

static void motor_put(void *data)
{
	struct motor_device *mdev = data;

	kref_put(&mdev->ref, motor_free);
}

static void motor_subdev_release(struct v4l2_subdev *sd)
{
	struct motor_device *mdev = v4l2_get_subdevdata(sd);

	if (mdev && atomic_xchg(&mdev->subdev_kref, 0))
		motor_put(mdev);
}

static const struct v4l2_subdev_internal_ops motor_subdev_internal_ops = {
	.release = motor_subdev_release,
};

static int motor_get_coils(struct device *dev, const char *con_id,
			   struct motor_coils *out)
{
	int i;

	for (i = 0; i < 4; i++) {
		struct gpio_desc *desc;

		desc = devm_gpiod_get_index_optional(dev, con_id, i,
						     GPIOD_OUT_LOW);
		if (IS_ERR(desc))
			return PTR_ERR(desc);
		if (!desc) {
			if (i == 0)
				return 1;
			return -EINVAL;
		}
		if (gpiod_cansleep(desc))
			return -EINVAL;
		out->coil[i] = desc;
	}
	return 0;
}

static int motor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct motor_device *mdev;
	s32 pan_max_arcsec, tilt_max_arcsec, pan_def_arcsec, tilt_def_arcsec;
	u32 val;
	int ret, i;

	mdev = kzalloc(sizeof(*mdev), GFP_KERNEL);
	if (!mdev)
		return -ENOMEM;

	kref_init(&mdev->ref);
	v4l2_ctrl_handler_init(&mdev->ctrls, 8);
	ret = devm_add_action_or_reset(dev, motor_put, mdev);
	if (ret)
		return ret;

	mdev->dev = dev;
	mdev->pan_max_steps = 3800;
	mdev->tilt_max_steps = 1200;
	mdev->pan_max_degrees = 355;
	mdev->tilt_max_degrees = 90;
	mdev->pan_reset_steps = mdev->pan_max_steps / 2;
	mdev->tilt_reset_steps = 600;
	mdev->tilt_step_div = 1;
	mdev->pan_speed = 900;
	mdev->tilt_speed = 900;

	if (np) {
		if (!of_property_read_u32(np, "ingenic,pan-max-steps", &val)) {
			if (!val || val > INT_MAX - 100)
				return -EINVAL;
			mdev->pan_max_steps = val;
			mdev->pan_reset_steps = val / 2;
		}
		if (!of_property_read_u32(np, "ingenic,tilt-max-steps", &val)) {
			if (!val || val > INT_MAX - 30)
				return -EINVAL;
			mdev->tilt_max_steps = val;
			mdev->tilt_reset_steps = min(mdev->tilt_reset_steps,
						     (int)val);
		}
		if (!of_property_read_u32(np, "ingenic,pan-max-degrees", &val)) {
			if (!val || val > 360)
				return -EINVAL;
			mdev->pan_max_degrees = val;
		}
		if (!of_property_read_u32(np, "ingenic,tilt-max-degrees", &val)) {
			if (!val || val > 360)
				return -EINVAL;
			mdev->tilt_max_degrees = val;
		}
		if (!of_property_read_u32(np, "ingenic,pan-reset-steps", &val)) {
			if (val > mdev->pan_max_steps)
				return -EINVAL;
			mdev->pan_reset_steps = val;
		}
		if (!of_property_read_u32(np, "ingenic,tilt-reset-steps", &val)) {
			if (val > mdev->tilt_max_steps)
				return -EINVAL;
			mdev->tilt_reset_steps = val;
		}
		if (!of_property_read_u32(np, "ingenic,tilt-step-div", &val)) {
			if (!val || val > 64)
				return -EINVAL;
			mdev->tilt_step_div = val;
		}
	}

	mdev->switch_gpiod = devm_gpiod_get_optional(dev, "switch",
						     GPIOD_OUT_HIGH);
	if (IS_ERR(mdev->switch_gpiod))
		return PTR_ERR(mdev->switch_gpiod);
	if (mdev->switch_gpiod && gpiod_cansleep(mdev->switch_gpiod))
		return -EINVAL;

	ret = motor_get_coils(dev, "pan", &mdev->coils[0]);
	if (ret < 0)
		return ret;
	ret = motor_get_coils(dev, "tilt", &mdev->coils[1]);
	if (ret < 0)
		return ret;
	if (ret == 1)
		mdev->coils[1] = mdev->coils[0];

	mutex_init(&mdev->dev_mutex);
	spin_lock_init(&mdev->lock);
	init_completion(&mdev->completion);

	hrtimer_init(&mdev->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mdev->timer.function = motor_timer_callback;
	mdev->step_interval = ktime_set(0, div_u64(1000000000ULL, mdev->pan_speed));

	for (i = 0; i < 2; i++) {
		mdev->motors[i].coils = &mdev->coils[i];
		init_waitqueue_head(&mdev->motors[i].wait);
	}
	mdev->motors[0].max_steps = mdev->pan_max_steps + 100;
	mdev->motors[1].max_steps = mdev->tilt_max_steps + 30;

	motor_set_default(mdev);

	mdev->mdev.dev = dev;
	strscpy(mdev->mdev.model, "Ingenic T23 PTZ Motor", sizeof(mdev->mdev.model));
	media_device_init(&mdev->mdev);

	mdev->v4l2_dev.mdev = &mdev->mdev;
	ret = v4l2_device_register(dev, &mdev->v4l2_dev);
	if (ret)
		goto err_media;

	pan_max_arcsec = mdev->pan_max_degrees * 3600U;
	tilt_max_arcsec = mdev->tilt_max_degrees * 3600U;
	pan_def_arcsec = motor_steps_to_arcsec(mdev->pan_reset_steps,
					       mdev->pan_max_steps,
					       mdev->pan_max_degrees);
	tilt_def_arcsec = motor_steps_to_arcsec(mdev->tilt_reset_steps,
						mdev->tilt_max_steps,
						mdev->tilt_max_degrees);

	mdev->pan_rel = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
					  V4L2_CID_PAN_RELATIVE,
					  -pan_max_arcsec, pan_max_arcsec,
					  1, 0);
	mdev->tilt_rel = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
					   V4L2_CID_TILT_RELATIVE,
					   -tilt_max_arcsec, tilt_max_arcsec,
					   1, 0);
	mdev->pan_abs = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
					  V4L2_CID_PAN_ABSOLUTE, 0,
					  pan_max_arcsec, 1, pan_def_arcsec);
	if (mdev->pan_abs)
		mdev->pan_abs->flags |= V4L2_CTRL_FLAG_VOLATILE |
					V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;
	mdev->tilt_abs = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
					   V4L2_CID_TILT_ABSOLUTE, 0,
					   tilt_max_arcsec, 1, tilt_def_arcsec);
	if (mdev->tilt_abs)
		mdev->tilt_abs->flags |= V4L2_CTRL_FLAG_VOLATILE |
					 V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;
	mdev->pan_reset = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
					    V4L2_CID_PAN_RESET, 0, 0, 0, 0);
	mdev->tilt_reset = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
					     V4L2_CID_TILT_RESET, 0, 0, 0, 0);
	mdev->pan_speed_ctrl = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
						 V4L2_CID_PAN_SPEED,
						 0, 900, 1, 900);
	mdev->tilt_speed_ctrl = v4l2_ctrl_new_std(&mdev->ctrls, &motor_ctrl_ops,
						  V4L2_CID_TILT_SPEED,
						  0, 900, 1, 900);
	if (mdev->ctrls.error) {
		ret = mdev->ctrls.error;
		goto err_v4l2;
	}
	mdev->v4l2_dev.ctrl_handler = &mdev->ctrls;

	v4l2_subdev_init(&mdev->sd, &motor_subdev_ops);
	mdev->sd.owner = THIS_MODULE;
	mdev->sd.dev = dev;
	mdev->sd.internal_ops = &motor_subdev_internal_ops;
	mdev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	mdev->sd.entity.function = MEDIA_ENT_F_LENS;
	mdev->sd.ctrl_handler = &mdev->ctrls;
	strscpy(mdev->sd.name, "t23-motor", sizeof(mdev->sd.name));
	v4l2_set_subdevdata(&mdev->sd, mdev);

	ret = v4l2_device_register_subdev(&mdev->v4l2_dev, &mdev->sd);
	if (ret)
		goto err_v4l2;

	kref_get(&mdev->ref);
	atomic_set(&mdev->subdev_kref, 1);
	ret = v4l2_device_register_subdev_nodes(&mdev->v4l2_dev);
	if (ret) {
		if (atomic_xchg(&mdev->subdev_kref, 0))
			motor_put(mdev);
		goto err_v4l2;
	}

	ret = media_device_register(&mdev->mdev);
	if (ret)
		goto err_v4l2;

	platform_set_drvdata(pdev, mdev);
	return 0;

err_v4l2:
	v4l2_device_unregister(&mdev->v4l2_dev);
err_media:
	media_device_cleanup(&mdev->mdev);
	return ret;
}

static void motor_remove(struct platform_device *pdev)
{
	struct motor_device *mdev = platform_get_drvdata(pdev);

	if (!mdev)
		return;

	v4l2_device_unregister(&mdev->v4l2_dev);
	media_device_unregister(&mdev->mdev);
	media_device_cleanup(&mdev->mdev);
	motor_ops_stop(mdev);
	hrtimer_cancel(&mdev->timer);
	motor_set_default(mdev);
	if (mdev->switch_gpiod)
		gpiod_set_value(mdev->switch_gpiod, 0);
}

static const struct of_device_id motor_of_match[] = {
	{ .compatible = "ingenic,t23-motor" },
	{ }
};
MODULE_DEVICE_TABLE(of, motor_of_match);

static struct platform_driver motor_driver = {
	.probe  = motor_probe,
	.remove = motor_remove,
	.driver = {
		.name  = "ptz-motor",
		.of_match_table = motor_of_match,
	},
};
module_platform_driver(motor_driver);

MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Ingenic T23 PTZ Stepper Motor Driver");
MODULE_LICENSE("GPL");

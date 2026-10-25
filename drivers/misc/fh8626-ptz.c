// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626 GPIO pan/tilt stepper motor driver
 *
 * Drives the pan and tilt stepper motors from an hrtimer through V4L2
 * controls and sysfs. There is no position feedback: positions are software
 * estimates in four-phase cycles over [0, range], with 0 the reverse stop.
 * Probe never homes or sweeps the mechanism.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>

#define PTZ_NUM_COILS		4
#define PTZ_DEFAULT_MAX_PPS	500
#define PTZ_MIN_PPS		20
#define PTZ_DEFAULT_H_RANGE	496
#define PTZ_DEFAULT_V_RANGE	169
#define PTZ_MAX_SPEED		5

/* Dual-phase excitation: two adjacent coils are on at a time */
static const u8 ptz_step_table_fwd[4] = { 0x03, 0x06, 0x0c, 0x09 };

/* Per-speed pulse-skipping pattern, consumed over six timer ticks */
static const u8 ptz_speed_mask[6] = { 0x80, 0x90, 0xa8, 0xd8, 0xf8, 0xfc };

enum ptz_motor_state {
	PTZ_STATE_STOP = 1,
	PTZ_STATE_FWD = 2,
	PTZ_STATE_REV = 3,
	PTZ_STATE_LOCATE = 4,
};

struct ptz_axis {
	struct gpio_desc	*coils[PTZ_NUM_COILS];
	bool			invert_dir;
	int			range;		/* Travel limit in cycles */
	int			range_limit;	/* Board-configured ceiling */
	int			position;	/* Estimated whole cycles */
	int			target_pos;
	enum ptz_motor_state	state;
	u8			speed_idx;
	u8			tick_counter;	/* Position in the speed mask */
	u8			phase_idx;
	s8			step_offset;	/* Partial cycle, -3..3 */
	bool			phase_valid;
	bool			is_homing;
	int			homing_steps_left;
	u32			steps_moved;
};

struct fh8626_ptz {
	struct hrtimer		timer;
	ktime_t			timer_interval;
	spinlock_t		lock;
	struct ptz_axis		h_axis;
	struct ptz_axis		v_axis;
	u32			max_pps;
	u32			pps_limit;
	bool			sequential_axes; /* V waits for H */
	bool			timer_running;
	bool			shutting_down;
	bool			suspended;

	struct v4l2_device	v4l2_dev;
	struct v4l2_subdev	sd;
	struct v4l2_ctrl_handler ctrls;
	struct mutex		ctrl_lock;
	struct v4l2_ctrl	*ctrl_pan_abs;
	struct v4l2_ctrl	*ctrl_tilt_abs;
	struct v4l2_ctrl	*ctrl_pan_rel;
	struct v4l2_ctrl	*ctrl_tilt_rel;
};

static void ptz_coils_power_off(struct ptz_axis *axis)
{
	int i;

	for (i = 0; i < PTZ_NUM_COILS; i++) {
		if (axis->coils[i])
			gpiod_set_value(axis->coils[i], 0);
	}
}

static void ptz_coils_apply_phase(struct ptz_axis *axis, u8 phase_byte)
{
	int i;

	/* Drop outgoing phases before energizing incoming ones */
	for (i = 0; i < PTZ_NUM_COILS; i++)
		if (!(phase_byte & BIT(i)))
			gpiod_set_value(axis->coils[i], 0);
	for (i = 0; i < PTZ_NUM_COILS; i++)
		if (phase_byte & BIT(i))
			gpiod_set_value(axis->coils[i], 1);
}

/* Called with ptz->lock held; partial travel is kept on cancellation */
static void ptz_axis_stop(struct ptz_axis *axis)
{
	axis->state = PTZ_STATE_STOP;
	axis->is_homing = false;
	axis->steps_moved = 0;
	ptz_coils_power_off(axis);
}

static bool ptz_axis_at_target(struct ptz_axis *axis)
{
	return axis->position == axis->target_pos && !axis->step_offset;
}

static void ptz_axis_set_target(struct ptz_axis *axis, int target)
{
	axis->target_pos = target;
	axis->is_homing = false;
	if (ptz_axis_at_target(axis)) {
		ptz_axis_stop(axis);
	} else {
		axis->steps_moved = 0;
		axis->state = PTZ_STATE_LOCATE;
	}
}

static void ptz_axis_start_homing(struct ptz_axis *axis)
{
	axis->is_homing = true;
	axis->homing_steps_left = axis->range;
	axis->steps_moved = 0;
	axis->state = PTZ_STATE_REV;
}

/*
 * Advance one axis by one timer tick; returns false once the axis is idle.
 * Called with ptz->lock held.
 */
static bool ptz_axis_step(struct ptz_axis *axis)
{
	u8 mask;
	u8 phase_byte;
	bool fwd;
	bool electrical_fwd;
	u8 effective_speed;

	if (axis->state == PTZ_STATE_STOP)
		return false;

	/* Ramp up from speed 0 at the start and down near the target */
	effective_speed = axis->speed_idx;
	if (axis->speed_idx > 0) {
		u8 ramp_up = min_t(u8, axis->speed_idx, axis->steps_moved);

		if (axis->state == PTZ_STATE_LOCATE && !axis->is_homing) {
			int dist = abs(axis->target_pos - axis->position);
			u8 ramp_down = min_t(u8, axis->speed_idx, dist);

			effective_speed = min(ramp_up, ramp_down);
		} else {
			effective_speed = ramp_up;
		}
	}

	mask = ptz_speed_mask[effective_speed];
	if (!(mask & (0x80 >> (axis->tick_counter & 7)))) {
		axis->tick_counter = (axis->tick_counter + 1) % 6;
		return true;
	}
	axis->tick_counter = (axis->tick_counter + 1) % 6;

	if (axis->is_homing) {
		fwd = false;
	} else if (axis->state == PTZ_STATE_LOCATE) {
		if (ptz_axis_at_target(axis)) {
			ptz_axis_stop(axis);
			return false;
		}
		fwd = axis->position < axis->target_pos ||
		      (axis->position == axis->target_pos &&
		       axis->step_offset < 0);
	} else if (axis->state == PTZ_STATE_FWD) {
		fwd = true;
	} else {
		fwd = false;
	}

	if (!axis->is_homing) {
		if (fwd && axis->position >= axis->range &&
		    axis->step_offset >= 0) {
			ptz_axis_stop(axis);
			return false;
		} else if (!fwd && axis->position <= 0 &&
			   axis->step_offset <= 0) {
			ptz_axis_stop(axis);
			return false;
		}
	}

	electrical_fwd = fwd != axis->invert_dir;
	if (!axis->phase_valid) {
		axis->phase_idx = electrical_fwd ? 0 : 3;
		axis->phase_valid = true;
	} else {
		axis->phase_idx = (axis->phase_idx +
				   (electrical_fwd ? 1 : 3)) & 3;
	}
	phase_byte = ptz_step_table_fwd[axis->phase_idx];
	ptz_coils_apply_phase(axis, phase_byte);

	axis->step_offset += fwd ? 1 : -1;
	if (axis->step_offset == 4) {
		axis->step_offset = 0;
		axis->position++;
		axis->steps_moved++;
	} else if (axis->step_offset == -4) {
		axis->step_offset = 0;
		axis->position--;
		axis->steps_moved++;
		if (axis->is_homing) {
			axis->homing_steps_left--;
			if (axis->homing_steps_left <= 0) {
				axis->is_homing = false;
				axis->position = 0;
				axis->target_pos = 0;
				axis->step_offset = 0;
				ptz_axis_stop(axis);
				return false;
			}
		}
	}

	if (!axis->is_homing && axis->state == PTZ_STATE_LOCATE &&
	    ptz_axis_at_target(axis)) {
		ptz_axis_stop(axis);
		return false;
	}

	return true;
}

static enum hrtimer_restart fh8626_ptz_timer_func(struct hrtimer *timer)
{
	struct fh8626_ptz *ptz = container_of(timer, struct fh8626_ptz, timer);
	unsigned long flags;
	bool h_active, v_active;

	spin_lock_irqsave(&ptz->lock, flags);

	h_active = ptz_axis_step(&ptz->h_axis);

	if (ptz->sequential_axes && h_active) {
		v_active = (ptz->v_axis.state != PTZ_STATE_STOP);
		if (v_active)
			ptz_coils_power_off(&ptz->v_axis);
	} else {
		v_active = ptz_axis_step(&ptz->v_axis);
	}

	if (h_active || v_active) {
		hrtimer_forward_now(timer, ptz->timer_interval);
		spin_unlock_irqrestore(&ptz->lock, flags);
		return HRTIMER_RESTART;
	}

	ptz->timer_running = false;
	spin_unlock_irqrestore(&ptz->lock, flags);
	return HRTIMER_NORESTART;
}

static void ptz_start_timer_locked(struct fh8626_ptz *ptz)
{
	if (!ptz->timer_running) {
		ptz->timer_running = true;
		hrtimer_start(&ptz->timer, ptz->timer_interval,
			      HRTIMER_MODE_REL);
	}
}

static ssize_t position_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	int h_pos, v_pos;

	spin_lock_irqsave(&ptz->lock, flags);
	h_pos = ptz->h_axis.position;
	v_pos = ptz->v_axis.position;
	spin_unlock_irqrestore(&ptz->lock, flags);

	return scnprintf(buf, PAGE_SIZE, "H:%d V:%d\n", h_pos, v_pos);
}

static ssize_t position_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	int h_target, v_target;

	if (sscanf(buf, "%d %d", &h_target, &v_target) != 2)
		return -EINVAL;

	spin_lock_irqsave(&ptz->lock, flags);

	if (ptz->shutting_down || ptz->suspended) {
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -ENODEV;
	}

	h_target = clamp(h_target, 0, ptz->h_axis.range);
	v_target = clamp(v_target, 0, ptz->v_axis.range);

	ptz_axis_set_target(&ptz->h_axis, h_target);
	ptz_axis_set_target(&ptz->v_axis, v_target);

	ptz_start_timer_locked(ptz);
	spin_unlock_irqrestore(&ptz->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(position);

static ssize_t relative_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	int dh, dv;
	int h_target, v_target;

	if (sscanf(buf, "%d %d", &dh, &dv) != 2)
		return -EINVAL;

	spin_lock_irqsave(&ptz->lock, flags);
	if (ptz->shutting_down || ptz->suspended) {
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -ENODEV;
	}
	/* Add in 64 bits so user-supplied displacements cannot overflow */
	h_target = clamp_t(s64, (s64)ptz->h_axis.position + dh,
			   0, ptz->h_axis.range);
	v_target = clamp_t(s64, (s64)ptz->v_axis.position + dv,
			   0, ptz->v_axis.range);

	ptz_axis_set_target(&ptz->h_axis, h_target);
	ptz_axis_set_target(&ptz->v_axis, v_target);

	ptz_start_timer_locked(ptz);
	spin_unlock_irqrestore(&ptz->lock, flags);

	return count;
}
static DEVICE_ATTR_WO(relative);

static ssize_t speed_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	u8 h_spd, v_spd;
	u32 pps;

	spin_lock_irqsave(&ptz->lock, flags);
	h_spd = ptz->h_axis.speed_idx;
	v_spd = ptz->v_axis.speed_idx;
	pps = ptz->max_pps;
	spin_unlock_irqrestore(&ptz->lock, flags);

	return scnprintf(buf, PAGE_SIZE, "H_speed:%u V_speed:%u max_pps:%u\n",
			 h_spd, v_spd, pps);
}

static ssize_t speed_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	unsigned int h_spd, v_spd;
	unsigned int pps = 0;
	int ret;

	ret = sscanf(buf, "%u %u %u", &h_spd, &v_spd, &pps);
	if (ret < 2)
		return -EINVAL;

	if (h_spd > PTZ_MAX_SPEED || v_spd > PTZ_MAX_SPEED)
		return -EINVAL;
	if (ret >= 3 && (pps < PTZ_MIN_PPS || pps > ptz->pps_limit))
		return -EINVAL;

	spin_lock_irqsave(&ptz->lock, flags);
	ptz->h_axis.speed_idx = h_spd;
	ptz->v_axis.speed_idx = v_spd;
	if (ret >= 3) {
		ptz->max_pps = pps;
		ptz->timer_interval = ktime_set(0, 1000000000 / pps);
	}
	spin_unlock_irqrestore(&ptz->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(speed);

static ssize_t limits_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	int h_range, v_range;

	spin_lock_irqsave(&ptz->lock, flags);
	h_range = ptz->h_axis.range;
	v_range = ptz->v_axis.range;
	spin_unlock_irqrestore(&ptz->lock, flags);

	return scnprintf(buf, PAGE_SIZE, "H_max:%d V_max:%d\n",
			 h_range, v_range);
}

static ssize_t limits_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	int h_range, v_range;

	if (sscanf(buf, "%d %d", &h_range, &v_range) != 2)
		return -EINVAL;

	if (h_range <= 0 || h_range > ptz->h_axis.range_limit ||
	    v_range <= 0 || v_range > ptz->v_axis.range_limit)
		return -EINVAL;

	spin_lock_irqsave(&ptz->lock, flags);
	/* Changing the limits cancels travel so no target lies beyond them */
	if (ptz->shutting_down || ptz->suspended) {
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -ENODEV;
	}
	ptz_axis_stop(&ptz->h_axis);
	ptz_axis_stop(&ptz->v_axis);
	ptz->h_axis.range = h_range;
	ptz->v_axis.range = v_range;
	spin_unlock_irqrestore(&ptz->lock, flags);

	if (ptz->ctrl_pan_abs)
		v4l2_ctrl_modify_range(ptz->ctrl_pan_abs, 0, h_range, 1, 0);
	if (ptz->ctrl_pan_rel)
		v4l2_ctrl_modify_range(ptz->ctrl_pan_rel, -h_range, h_range,
				       1, 0);
	if (ptz->ctrl_tilt_abs)
		v4l2_ctrl_modify_range(ptz->ctrl_tilt_abs, 0, v_range, 1, 0);
	if (ptz->ctrl_tilt_rel)
		v4l2_ctrl_modify_range(ptz->ctrl_tilt_rel, -v_range, v_range,
				       1, 0);

	return count;
}
static DEVICE_ATTR_RW(limits);

static ssize_t stop_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&ptz->lock, flags);
	if (ptz->shutting_down || ptz->suspended) {
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -ENODEV;
	}
	ptz_axis_stop(&ptz->h_axis);
	ptz_axis_stop(&ptz->v_axis);
	spin_unlock_irqrestore(&ptz->lock, flags);

	/*
	 * The timer callback retires itself under ptz->lock. Cancelling the
	 * timer here could strand a following command.
	 */
	return count;
}
static DEVICE_ATTR_WO(stop);

static ssize_t status_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;
	int h_state, v_state, h_pos, v_pos;
	bool running;

	spin_lock_irqsave(&ptz->lock, flags);
	h_state = ptz->h_axis.state;
	v_state = ptz->v_axis.state;
	h_pos = ptz->h_axis.position;
	v_pos = ptz->v_axis.position;
	running = ptz->timer_running;
	spin_unlock_irqrestore(&ptz->lock, flags);

	return scnprintf(buf, PAGE_SIZE,
			 "H_state:%d H_pos:%d V_state:%d V_pos:%d running:%d\n",
			 h_state, h_pos, v_state, v_pos, running ? 1 : 0);
}
static DEVICE_ATTR_RO(status);

static ssize_t calibrate_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&ptz->lock, flags);
	/* Only rebases the position estimate; nothing moves */
	if (ptz->shutting_down || ptz->suspended) {
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -ENODEV;
	}
	ptz->h_axis.position = 0;
	ptz->v_axis.position = 0;
	ptz->h_axis.step_offset = 0;
	ptz->v_axis.step_offset = 0;
	ptz->h_axis.target_pos = 0;
	ptz->v_axis.target_pos = 0;
	ptz_axis_stop(&ptz->h_axis);
	ptz_axis_stop(&ptz->v_axis);
	spin_unlock_irqrestore(&ptz->lock, flags);

	return count;
}
static DEVICE_ATTR_WO(calibrate);

static struct attribute *fh8626_ptz_attrs[] = {
	&dev_attr_position.attr,
	&dev_attr_relative.attr,
	&dev_attr_speed.attr,
	&dev_attr_limits.attr,
	&dev_attr_stop.attr,
	&dev_attr_status.attr,
	&dev_attr_calibrate.attr,
	NULL,
};
ATTRIBUTE_GROUPS(fh8626_ptz);

static int fh8626_ptz_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_ptz *ptz = container_of(ctrl->handler,
					      struct fh8626_ptz, ctrls);
	unsigned long flags;
	int target;

	spin_lock_irqsave(&ptz->lock, flags);
	if (ptz->shutting_down || ptz->suspended) {
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -ENODEV;
	}

	switch (ctrl->id) {
	case V4L2_CID_PAN_RELATIVE:
		target = clamp_t(s64, (s64)ptz->h_axis.position + ctrl->val,
				 0, ptz->h_axis.range);
		ptz_axis_set_target(&ptz->h_axis, target);
		ptz_start_timer_locked(ptz);
		break;

	case V4L2_CID_TILT_RELATIVE:
		target = clamp_t(s64, (s64)ptz->v_axis.position + ctrl->val,
				 0, ptz->v_axis.range);
		ptz_axis_set_target(&ptz->v_axis, target);
		ptz_start_timer_locked(ptz);
		break;

	case V4L2_CID_PAN_ABSOLUTE:
		target = clamp_val(ctrl->val, 0, ptz->h_axis.range);
		ptz_axis_set_target(&ptz->h_axis, target);
		ptz_start_timer_locked(ptz);
		break;

	case V4L2_CID_TILT_ABSOLUTE:
		target = clamp_val(ctrl->val, 0, ptz->v_axis.range);
		ptz_axis_set_target(&ptz->v_axis, target);
		ptz_start_timer_locked(ptz);
		break;

	case V4L2_CID_PAN_SPEED:
		ptz->h_axis.speed_idx = clamp_val(ctrl->val, 0, PTZ_MAX_SPEED);
		break;

	case V4L2_CID_TILT_SPEED:
		ptz->v_axis.speed_idx = clamp_val(ctrl->val, 0, PTZ_MAX_SPEED);
		break;

	case V4L2_CID_PAN_RESET:
		ptz_axis_start_homing(&ptz->h_axis);
		ptz_start_timer_locked(ptz);
		break;

	case V4L2_CID_TILT_RESET:
		ptz_axis_start_homing(&ptz->v_axis);
		ptz_start_timer_locked(ptz);
		break;

	default:
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -EINVAL;
	}

	spin_unlock_irqrestore(&ptz->lock, flags);
	return 0;
}

static int fh8626_ptz_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_ptz *ptz = container_of(ctrl->handler,
					      struct fh8626_ptz, ctrls);
	unsigned long flags;

	spin_lock_irqsave(&ptz->lock, flags);
	switch (ctrl->id) {
	case V4L2_CID_PAN_ABSOLUTE:
		ctrl->val = ptz->h_axis.position;
		break;
	case V4L2_CID_TILT_ABSOLUTE:
		ctrl->val = ptz->v_axis.position;
		break;
	case V4L2_CID_PAN_SPEED:
		ctrl->val = ptz->h_axis.speed_idx;
		break;
	case V4L2_CID_TILT_SPEED:
		ctrl->val = ptz->v_axis.speed_idx;
		break;
	default:
		spin_unlock_irqrestore(&ptz->lock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&ptz->lock, flags);
	return 0;
}

static const struct v4l2_ctrl_ops fh8626_ptz_ctrl_ops = {
	.s_ctrl = fh8626_ptz_s_ctrl,
	.g_volatile_ctrl = fh8626_ptz_g_volatile_ctrl,
};

static const struct v4l2_subdev_core_ops fh8626_ptz_core_ops = {
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops fh8626_ptz_subdev_ops = {
	.core = &fh8626_ptz_core_ops,
};

static int fh8626_ptz_probe_axis(struct device *dev, struct ptz_axis *axis,
				 const char *con_id, int default_range)
{
	int i;

	if (gpiod_count(dev, con_id) != PTZ_NUM_COILS)
		return -EINVAL;

	axis->range = default_range;
	axis->state = PTZ_STATE_STOP;
	axis->speed_idx = PTZ_MAX_SPEED;

	for (i = 0; i < PTZ_NUM_COILS; i++) {
		axis->coils[i] = devm_gpiod_get_index(dev, con_id, i,
						     GPIOD_OUT_LOW);
		if (IS_ERR(axis->coils[i])) {
			dev_err(dev, "failed to get coil GPIO %s[%d]: %ld\n",
				con_id, i, PTR_ERR(axis->coils[i]));
			return PTR_ERR(axis->coils[i]);
		}
		/* The coils are driven from the timer callback */
		if (gpiod_cansleep(axis->coils[i])) {
			dev_err(dev, "coil GPIO %s[%d] must not sleep\n",
				con_id, i);
			return -EINVAL;
		}
	}

	ptz_coils_power_off(axis);
	return 0;
}

static void fh8626_ptz_shutdown(struct platform_device *pdev);

static int fh8626_ptz_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct fh8626_ptz *ptz;
	struct v4l2_ctrl *ctrl;
	u32 val;
	int ret;

	if (!np)
		return -ENODEV;

	ptz = devm_kzalloc(dev, sizeof(*ptz), GFP_KERNEL);
	if (!ptz)
		return -ENOMEM;

	spin_lock_init(&ptz->lock);

	ptz->max_pps = PTZ_DEFAULT_MAX_PPS;
	if (!of_property_read_u32(np, "max-pps", &val)) {
		if (val < PTZ_MIN_PPS || val > PTZ_DEFAULT_MAX_PPS)
			return -EINVAL;
		ptz->max_pps = val;
	}
	ptz->timer_interval = ktime_set(0, 1000000000 / ptz->max_pps);
	ptz->pps_limit = ptz->max_pps;

	ptz->sequential_axes = of_property_read_bool(np, "sequential-axes");

	ret = fh8626_ptz_probe_axis(dev, &ptz->h_axis, "h",
				    PTZ_DEFAULT_H_RANGE);
	if (ret)
		return ret;

	if (!of_property_read_u32(np, "h-upbound", &val)) {
		if (!val || val > PTZ_DEFAULT_H_RANGE)
			return -EINVAL;
		ptz->h_axis.range = val;
	}

	ptz->h_axis.invert_dir = of_property_read_bool(np, "h-invert-dir");
	ptz->h_axis.range_limit = ptz->h_axis.range;

	ret = fh8626_ptz_probe_axis(dev, &ptz->v_axis, "v",
				    PTZ_DEFAULT_V_RANGE);
	if (ret)
		return ret;

	if (!of_property_read_u32(np, "v-upbound", &val)) {
		if (!val || val > PTZ_DEFAULT_V_RANGE)
			return -EINVAL;
		ptz->v_axis.range = val;
	}

	ptz->v_axis.invert_dir = of_property_read_bool(np, "v-invert-dir");
	ptz->v_axis.range_limit = ptz->v_axis.range;

	hrtimer_init(&ptz->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ptz->timer.function = fh8626_ptz_timer_func;

	platform_set_drvdata(pdev, ptz);
	ret = sysfs_create_groups(&dev->kobj, fh8626_ptz_groups);
	if (ret) {
		dev_err(dev, "failed to create sysfs attributes: %d\n", ret);
		/* A partly created group may already have taken a command */
		fh8626_ptz_shutdown(pdev);
		return ret;
	}

	ret = v4l2_device_register(dev, &ptz->v4l2_dev);
	if (ret) {
		dev_err(dev, "failed to register V4L2 device: %d\n", ret);
		goto err_sysfs;
	}

	mutex_init(&ptz->ctrl_lock);
	ret = v4l2_ctrl_handler_init(&ptz->ctrls, 8);
	if (ret) {
		dev_err(dev, "failed to init V4L2 ctrl handler: %d\n", ret);
		goto err_v4l2_dev;
	}
	ptz->ctrls.lock = &ptz->ctrl_lock;

	ptz->ctrl_pan_abs = v4l2_ctrl_new_std(&ptz->ctrls, &fh8626_ptz_ctrl_ops,
					      V4L2_CID_PAN_ABSOLUTE,
					      0, ptz->h_axis.range, 1, 0);
	if (ptz->ctrl_pan_abs)
		ptz->ctrl_pan_abs->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ptz->ctrl_tilt_abs = v4l2_ctrl_new_std(&ptz->ctrls,
					       &fh8626_ptz_ctrl_ops,
					       V4L2_CID_TILT_ABSOLUTE,
					       0, ptz->v_axis.range, 1, 0);
	if (ptz->ctrl_tilt_abs)
		ptz->ctrl_tilt_abs->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ptz->ctrl_pan_rel = v4l2_ctrl_new_std(&ptz->ctrls, &fh8626_ptz_ctrl_ops,
					      V4L2_CID_PAN_RELATIVE,
					      -ptz->h_axis.range,
					      ptz->h_axis.range, 1, 0);

	ptz->ctrl_tilt_rel = v4l2_ctrl_new_std(&ptz->ctrls,
					       &fh8626_ptz_ctrl_ops,
					       V4L2_CID_TILT_RELATIVE,
					       -ptz->v_axis.range,
					       ptz->v_axis.range, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ptz->ctrls, &fh8626_ptz_ctrl_ops,
				 V4L2_CID_PAN_SPEED, 0, PTZ_MAX_SPEED, 1,
				 ptz->h_axis.speed_idx);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(&ptz->ctrls, &fh8626_ptz_ctrl_ops,
				 V4L2_CID_TILT_SPEED, 0, PTZ_MAX_SPEED, 1,
				 ptz->v_axis.speed_idx);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	v4l2_ctrl_new_std(&ptz->ctrls, &fh8626_ptz_ctrl_ops,
			  V4L2_CID_PAN_RESET, 0, 0, 0, 0);
	v4l2_ctrl_new_std(&ptz->ctrls, &fh8626_ptz_ctrl_ops,
			  V4L2_CID_TILT_RESET, 0, 0, 0, 0);

	if (ptz->ctrls.error) {
		ret = ptz->ctrls.error;
		dev_err(dev, "failed to create V4L2 controls: %d\n", ret);
		goto err_ctrls;
	}

	v4l2_subdev_init(&ptz->sd, &fh8626_ptz_subdev_ops);
	ptz->sd.owner = THIS_MODULE;
	ptz->sd.dev = dev;
	strlcpy(ptz->sd.name, "fh8626-ptz", sizeof(ptz->sd.name));
	ptz->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	ptz->sd.ctrl_handler = &ptz->ctrls;

	ptz->sd.entity.function = MEDIA_ENT_F_LENS;
	ret = media_entity_pads_init(&ptz->sd.entity, 0, NULL);
	if (ret) {
		dev_err(dev, "failed to init media entity: %d\n", ret);
		goto err_ctrls;
	}

	ret = v4l2_device_register_subdev(&ptz->v4l2_dev, &ptz->sd);
	if (ret) {
		dev_err(dev, "failed to register V4L2 subdev: %d\n", ret);
		goto err_entity;
	}

	ret = v4l2_device_register_subdev_nodes(&ptz->v4l2_dev);
	if (ret) {
		dev_err(dev, "failed to register subdev devnodes: %d\n", ret);
		goto err_subdev;
	}

	return 0;

err_subdev:
	v4l2_device_unregister_subdev(&ptz->sd);
err_entity:
	media_entity_cleanup(&ptz->sd.entity);
err_ctrls:
	v4l2_ctrl_handler_free(&ptz->ctrls);
	mutex_destroy(&ptz->ctrl_lock);
err_v4l2_dev:
	v4l2_device_unregister(&ptz->v4l2_dev);
err_sysfs:
	sysfs_remove_groups(&dev->kobj, fh8626_ptz_groups);
	fh8626_ptz_shutdown(pdev);
	return ret;
}

static void fh8626_ptz_shutdown(struct platform_device *pdev)
{
	struct fh8626_ptz *ptz = platform_get_drvdata(pdev);
	unsigned long flags;

	spin_lock_irqsave(&ptz->lock, flags);
	ptz->shutting_down = true;
	ptz_axis_stop(&ptz->h_axis);
	ptz_axis_stop(&ptz->v_axis);
	spin_unlock_irqrestore(&ptz->lock, flags);
	hrtimer_cancel(&ptz->timer);
	spin_lock_irqsave(&ptz->lock, flags);
	ptz->timer_running = false;
	spin_unlock_irqrestore(&ptz->lock, flags);
}

static int fh8626_ptz_remove(struct platform_device *pdev)
{
	struct fh8626_ptz *ptz = platform_get_drvdata(pdev);

	sysfs_remove_groups(&pdev->dev.kobj, fh8626_ptz_groups);
	fh8626_ptz_shutdown(pdev);
	v4l2_device_unregister_subdev(&ptz->sd);
	media_entity_cleanup(&ptz->sd.entity);
	v4l2_ctrl_handler_free(&ptz->ctrls);
	mutex_destroy(&ptz->ctrl_lock);
	v4l2_device_unregister(&ptz->v4l2_dev);
	return 0;
}

static int fh8626_ptz_suspend(struct device *dev)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&ptz->lock, flags);
	ptz->suspended = true;
	ptz_axis_stop(&ptz->h_axis);
	ptz_axis_stop(&ptz->v_axis);
	spin_unlock_irqrestore(&ptz->lock, flags);
	hrtimer_cancel(&ptz->timer);
	spin_lock_irqsave(&ptz->lock, flags);
	ptz->timer_running = false;
	spin_unlock_irqrestore(&ptz->lock, flags);
	return 0;
}

static int fh8626_ptz_resume(struct device *dev)
{
	struct fh8626_ptz *ptz = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&ptz->lock, flags);
	/* Travel after suspend requires a new command. */
	ptz->suspended = false;
	spin_unlock_irqrestore(&ptz->lock, flags);
	return 0;
}

static const struct dev_pm_ops fh8626_ptz_pm_ops = {
	.suspend = fh8626_ptz_suspend,
	.resume = fh8626_ptz_resume,
	.freeze = fh8626_ptz_suspend,
	.thaw = fh8626_ptz_resume,
	.poweroff = fh8626_ptz_suspend,
	.restore = fh8626_ptz_resume,
};

static const struct of_device_id fh8626_ptz_of_match[] = {
	{ .compatible = "fullhan,fh8626-ptz" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fh8626_ptz_of_match);

static struct platform_driver fh8626_ptz_driver = {
	.probe		= fh8626_ptz_probe,
	.remove		= fh8626_ptz_remove,
	.shutdown	= fh8626_ptz_shutdown,
	.driver		= {
		.name		= "fh8626-ptz",
		.of_match_table	= fh8626_ptz_of_match,
		.pm		= &fh8626_ptz_pm_ops,
	},
};

module_platform_driver(fh8626_ptz_driver);

MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Fullhan FH8626 GPIO pan/tilt stepper motor driver");
MODULE_LICENSE("GPL v2");

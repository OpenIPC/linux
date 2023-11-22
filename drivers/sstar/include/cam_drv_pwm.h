/*
 * cam_drv_pwm.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _CAM_DRV_PWM_H_
#define _CAM_DRV_PWM_H_

#include <linux/pwm.h>

/*
 * struct CamPwmState - state of a PWM channel
 * @period: PWM period (in nanoseconds or HZ)
 * @duty_cycle: PWM duty cycle (in nanoseconds or percent)
 * @polarity: PWM polarity
 * @enabled: PWM enabled status
 */
struct CamPwmState
{
    u64  duty;
    u64  period;
    bool enabled;
    u8   polarity;
};

/**
 * enum CamPWMArgs - PWM parameter config
 * @CAM_PWM_PERIOD: update period
 * @CAM_PWM_DUTY: update duty cycle
 * @CAM_PWM_POLAR: update polarity,
 * if set 0, a high signal for the duration of the duty-cycle,
 * followed by a low signal for the remainder of the pulse period,
 * if set 1, a low signal for the duration of the duty-cycle,
 * followed by a high signal for the remainder of the pulse period.
 * @CAM_PWM_ONOFF: update enable status
 * @CAM_PWM_ALL: update all parameter
 */
enum CamPWMArgs
{
    CAM_PWM_PERIOD,
    CAM_PWM_DUTY,
    CAM_PWM_POLAR,
    CAM_PWM_ONOFF,
    CAM_PWM_ALL,
};

/**
 * CamPwmConfig() - atomically apply a new state to a PWM channel
 * @pwm_dev: PWM device
 * @args: enum CamPWMArgs
 * @CamPwmState: new state to apply
 */
int CamPwmConfig(struct pwm_device *pwm_dev, enum CamPWMArgs args, struct CamPwmState *cam_state);

/**
 * CamPwmFree() - release a PWM device
 * @pwm: PWM device
 */
void CamPwmFree(struct pwm_device *pwm_dev);

/**
 * CamPwmRequest() - request a PWM device
 * @channel: PWM channel
 * @label: PWM device label
 */
struct pwm_device *CamPwmRequest(int channel, const char *label);

#endif // _CAM_DRV_PWM_H_

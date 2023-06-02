/**
    nvt power IC header file

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _NVT_POWER_H
#define _NVT_POWER_H

#define POWER_DECAY_LEVEL_00   0
#define POWER_DECAY_LEVEL_10   10
#define POWER_DECAY_LEVEL_20   20
#define POWER_DECAY_LEVEL_30   30
#define POWER_DECAY_LEVEL_40   40
#define POWER_DECAY_LEVEL_50   50
#define POWER_DECAY_LEVEL_60   60
#define POWER_DECAY_LEVEL_70   70
#define POWER_DECAY_LEVEL_80   80
#define POWER_DECAY_LEVEL_90   90
#define POWER_DECAY_LEVEL_100  100

#ifdef CONFIG_POWER_IC_CONTROL
void nvt_pwr_control(int level);
void nvt_pwr_disable(void);
void nvt_pwr_control_ep(int level);
void nvt_pwr_disable_ep(void);
#else
#ifdef CONFIG_PWM_NA51068
inline void nvt_pwr_control(int level) {}
inline void nvt_pwr_disable(void) {}
inline void nvt_pwr_control_ep(int level) {}
inline void nvt_pwr_disable_ep(void) {}
#endif
#endif

#endif /* _NVT_POWER_H */

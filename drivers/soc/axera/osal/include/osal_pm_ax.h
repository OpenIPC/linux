/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_POWER_AX__H__
#define __OSAL_POWER_AX__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_type_ax.h"

#define AX_OSAL_PM_STATE_RUNNING        0
#define AX_OSAL_PM_STATE_STANDBY        1
#define AX_OSAL_PM_STATE_LIGHTSLEEP     2
#define AX_OSAL_PM_STATE_DEEPSLEEP      3

#define AX_OSAL_PM_SYS_NPU              1
#define AX_OSAL_PM_SYS_VPU              2
#define AX_OSAL_PM_SYS_ISP              3
#define AX_OSAL_PM_SYS_MM               4

#define AX_OSAL_PM_SYS_WAKEUP2          2
#define AX_OSAL_PM_SYS_WAKEUP5          5

int AX_OSAL_PM_WakeupLock(char *lock_name);
int AX_OSAL_PM_WakeupUnlock(char *lock_name);
int AX_OSAL_PM_SetLevel(int pm_level);
int AX_OSAL_PM_GetLevel(int *pm_level);
int AX_OSAL_PM_SetSysMode(int sys,int mode);
int AX_OSAL_PM_SetWakePin(int wake_pin);

#ifdef __cplusplus
}
#endif

#endif /*__OSAL_POWER_AX__H__*/

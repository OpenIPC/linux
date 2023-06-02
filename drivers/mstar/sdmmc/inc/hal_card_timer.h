#ifndef __HAL_CARD_TIMER_H
#define __HAL_CARD_TIMER_H

#include "hal_card_regs.h"

//###########################################################################################################
#if (D_OS == D_OS__LINUX)
//###########################################################################################################
#include <linux/delay.h>
#include <linux/time.h>
//###########################################################################################################
#endif

U32_T Hal_Timer_mDelay(U32_T u32_msec);
U32_T Hal_Timer_uDelay(U32_T u32_usec);
U32_T Hal_Timer_mSleep(U32_T u32_msec);



#endif //End of __HAL_CARD_TIMER_H

#ifndef _CSI_PLATFORM_H_
#define _CSI_PLATFORM_H_
#include "../../csi.h"


extern void csi_platform_clk_enable(CSI_ID id) ;
extern void csi_platform_clk_disable(CSI_ID id);
extern void csi_platform_clk_get_freq(CSI_ID id, unsigned long *p_clk);
#ifdef __KERNEL__
extern void csi_platform_init_completion(CSI_ID id);
extern void csi_platform_reinit_completion(CSI_ID id);
extern void csi_platform_wait_completion(CSI_ID id);
extern unsigned long csi_platform_wait_completion_timeout(CSI_ID id, unsigned long timeout);
extern void csi_platform_complete(CSI_ID id);
#endif
#endif

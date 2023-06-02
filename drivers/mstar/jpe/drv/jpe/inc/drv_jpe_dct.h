#ifndef DRV_JPE_DCT_H
#define DRV_JPE_DCT_H

#include "drv_jpe.h"


#if defined(_FPGA_) || defined(_UDMA_) || defined(_HIF_)
  #include "fpgaapi.h"
#endif
#if defined(MSTAR_JPD_IDCT)
#include "udma_share.h"
JPE_IOC_RET_STATUS_e JpeDctInit(void);  // Initialize DCT-reuse mode reg settings.
JPE_IOC_RET_STATUS_e JpeDctFire(MEMMAP_t* pMemMap);  // Fire JPE.
JPE_IOC_RET_STATUS_e JpeDctDoneCheck(MEMMAP_t* pMemMap);    // Check JPE frame down, and then clean int flag.
JPE_IOC_RET_STATUS_e JpeDctSwitchToJPD(void);   // Switch to JPD mode for later JPD IDCT-reuse phase.
#endif
#endif // DRV_JPE_dCT_H

#ifndef _JPE_DRV_H_
#define _JPE_DRV_H_

#include "hal_jpe.h"
#include "mdrv_jpe_io_st.h"

#define MAX_BANK_NUM 4
#define BistreamInfo_t JpeBitstreamInfo_t
#define pBistreamInfo pJpeBitstreamInfo

typedef enum
{
    JPE_DEV_INIT = 0,
    JPE_DEV_BUSY,
    JPE_DEV_ENC_DONE,
    JPE_DEV_OUTBUF_FULL,
    JPE_DEV_INBUF_FULL
} JpeDevStatus_e;

typedef struct
{
    JpeHalHandle_t jpeHalHandle;
    JpeCfg_t       jpeCfg;
    JpeDevStatus_e ejpeDevStatus;
    u32            nEncodeSize;
} JpeHandle_t, *pJpeHandle;

void  JpeIsrHandler(int eIntNum, JpeHandle_t *jpeHandle);

void DrvJpeReset(JpeHandle_t *jpeHandle);
JPE_IOC_RET_STATUS_e DrvJpeInit(JpeHandle_t *jpeHandle, pJpeCfg pJpeCfg);
JPE_IOC_RET_STATUS_e DrvJpeEncodeOneFrame(JpeHandle_t *jpeHandle,JpeHalOutBufCfg_t* jpeHalBuf);
JPE_IOC_RET_STATUS_e DrvJpeSetOutBuf(JpeHandle_t *jpeHandle, JpeBufInfo_t *jpeBuf);
JPE_IOC_RET_STATUS_e DrvJpeGetCaps(JpeHandle_t *jpeHandle, JpeCaps_t *pCaps);

#endif

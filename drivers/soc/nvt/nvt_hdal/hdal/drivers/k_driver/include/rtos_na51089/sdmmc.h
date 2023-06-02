/**
    @file       Sdmmc.h
    @ingroup    mIDrvStg_SDIO

    @brief      SD/MMC Card driver header

    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

*/

#ifndef _SDMMC_H
#define _SDMMC_H

#include "strg_def.h"
#include "sdmmc_desc.h"

/**
    @addtogroup mIDrvStg_SDIO
*/
//@{

typedef BOOL (*SDMMC_CARD_DETECT_CB)(void);

//------------------------------------------------------------
// The general api for the SDMMC device driver
//------------------------------------------------------------
extern PSTRG_TAB    sdmmc_getStorageObject(void);

// -- Card detect functions --
extern UINT32   sdmmc_getCardExist(void);
extern UINT32   sdmmc_getCardWriteProt(void);
extern void     sdmmc_setDetectCardExistHdl(SDMMC_CARD_DETECT_CB pHdl);
extern void     sdmmc_setDetectCardProtectHdl(SDMMC_CARD_DETECT_CB pHdl);

// -- Get card register functions --
extern UINT32               sdmmc_getOCRRegister(void);
extern PSD_CID_STRUCT       sdmmc_getCIDRegister(void);
extern PMMC_CID_STRUCT      sdmmc_getCIDRegisterMMC(void);
extern PSDMMC_CSD_STRUCT    sdmmc_getCSDRegister(void);
extern PSD_SCR_STRUCT       sdmmc_getSCRRegister(void);
extern PSD_STATUS_STRUCT    sdmmc_getSSRRegister(void);

// -- Get card info functions --
extern PSDMMC_INFO_STRUCT   sdmmc_getCardInformation(void);
extern void sdmmc_printCardInformation(void);

//@}

#endif

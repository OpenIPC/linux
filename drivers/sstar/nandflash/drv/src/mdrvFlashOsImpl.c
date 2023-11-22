/*
 * mdrvFlashOsImpl.c- Sigmastar
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

#include <linux/types.h>
#include <mdrvFlashOsImpl.h>
#include <ms_platform.h>
#include <cam_os_wrapper.h>

#if defined(CONFIG_FLASH_BDMA)
#include <hal_bdma.h>

struct bdma_alloc_dmem
{
    dma_addr_t  bdma_phy_addr;
    const char *DMEM_BDMA_INPUT;
    u8 *        bdma_vir_addr;
};

extern struct bdma_alloc_dmem _BDMA_ALLOC_DMEM;

u8 FLASH_IMPL_BdmaTransfer(u8 ePath, u8 *pSrcAddr, u8 *nDstAddr, u32 nLength)
{
    u64            u64_SrcAddr;
    u64            u64_DstAddr;
    HalBdmaParam_t tBdmaParam;

    memset(&tBdmaParam, 0, sizeof(HalBdmaParam_t));
    tBdmaParam.bIntMode = 0; // 0:use polling mode

    if (ePath == BDMA_SPI2MIU)
    {
        u64_DstAddr =
            _BDMA_ALLOC_DMEM.bdma_phy_addr + ((unsigned long)nDstAddr - (unsigned long)_BDMA_ALLOC_DMEM.bdma_vir_addr);
        tBdmaParam.ePathSel      = HAL_BDMA_SPI_TO_MIU0;
        tBdmaParam.eSrcDataWidth = HAL_BDMA_DATA_BYTE_8;
        tBdmaParam.eDstDataWidth = HAL_BDMA_DATA_BYTE_16;
        tBdmaParam.pSrcAddr      = (phys_addr_t)(unsigned long)pSrcAddr;
        tBdmaParam.pDstAddr      = (phys_addr_t)Chip_Phys_to_MIU((ss_phys_addr_t)u64_DstAddr);
    }
    else if (ePath == BDMA_MIU2SPI)
    {
        u64_SrcAddr =
            _BDMA_ALLOC_DMEM.bdma_phy_addr + ((unsigned long)pSrcAddr - (unsigned long)_BDMA_ALLOC_DMEM.bdma_vir_addr);
        tBdmaParam.ePathSel      = HAL_BDMA_MIU0_TO_SPI;
        tBdmaParam.eSrcDataWidth = HAL_BDMA_DATA_BYTE_16;
        tBdmaParam.eDstDataWidth = HAL_BDMA_DATA_BYTE_8;
        tBdmaParam.pSrcAddr      = (phys_addr_t)Chip_Phys_to_MIU((ss_phys_addr_t)u64_SrcAddr);
        tBdmaParam.pDstAddr      = 0;
    }
    tBdmaParam.eDstAddrMode = HAL_BDMA_ADDR_INC; // address increase
    tBdmaParam.u32TxCount   = nLength;
    tBdmaParam.u32Pattern   = 0;
    tBdmaParam.pfTxCbFunc   = NULL;
    if (HAL_BDMA_PROC_DONE != HalBdma_Transfer(HAL_BDMA_CH0, &tBdmaParam))
    {
        FLASH_IMPL_PRINTF("[SPINOR] bdma fail\r\n");
        return 1;
    }
    return DRV_BDMA_PROC_DONE;
}
#endif

void FLASH_IMPL_PRINTF_HEX(const char *pu8_data, u32 u32_value, const char *pu8_data2)
{
    if (pu8_data && pu8_data2)
        FLASH_IMPL_PRINTF("%s%x%s", pu8_data, u32_value, pu8_data2);
    else if (pu8_data)
        FLASH_IMPL_PRINTF("%s%x", pu8_data, u32_value);
    else if (pu8_data2)
        FLASH_IMPL_PRINTF("%x%s", u32_value, pu8_data2);
    else
        FLASH_IMPL_PRINTF("%x", u32_value);
}

void FLASH_IMPL_SHOW_ID(u8 *pu8_data, u8 u8_cnt)
{
    FLASH_IMPL_PRINTF("[FLASH] Device id is 0x%02x 0x%02x 0x%02x\n", pu8_data[0], pu8_data[1], pu8_data[2]);
}

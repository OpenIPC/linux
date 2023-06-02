#include <linux/kernel.h>
#include <asm/uaccess.h>  /* for get_fs*/
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>      /* for dma_alloc_coherent */
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/compaction.h> /*  for sysctl_compaction_handler*/
#include <asm/cacheflush.h>

#include "registers.h"
#include "ms_platform.h"
#include "mdrv_msys_io_st.h"
#include "mdrv_msys_io.h"
#include "platform_msys.h"
#include "hal_bdma.h"
#include "hal_movedma.h"
#include "cam_os_wrapper.h"

extern struct miscdevice  sys_dev;

#if defined(CONFIG_MS_MOVE_DMA)
void msys_mdma_done(void *Parm)
{
    CamOsTsem_t *ptMdmaDoneSem = (CamOsTsem_t *)Parm;
    CamOsTsemUp(ptMdmaDoneSem);
}


int msys_dma_blit(MSYS_DMA_BLIT *pstMdmaCfg)
{
    HalMoveDmaParam_t       tMoveDmaParam;
    HalMoveDmaLineOfst_t    tMoveDmaLineOfst;
    CamOsTsem_t             tMovedmaDoneSem;

    memset(&tMovedmaDoneSem, 0, sizeof(CamOsTsem_t));
    CamOsTsemInit(&tMovedmaDoneSem, 0);
    memset(&tMoveDmaParam, 0, sizeof(HalMoveDmaParam_t));
    tMoveDmaParam.u32SrcAddr    = pstMdmaCfg->phyaddr_src;
    tMoveDmaParam.u32SrcMiuSel  = (pstMdmaCfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (0) : (1);
    tMoveDmaParam.u32DstAddr    = pstMdmaCfg->phyaddr_dst;
    tMoveDmaParam.u32DstMiuSel  = (pstMdmaCfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? (0) : (1);
    tMoveDmaParam.u32Count      = pstMdmaCfg->length;
    tMoveDmaParam.CallBackFunc  = msys_mdma_done;
    tMoveDmaParam.CallBackParm   = (void *)&tMovedmaDoneSem;


    if (pstMdmaCfg->lineofst_src && pstMdmaCfg->lineofst_dst)
    {
        if((pstMdmaCfg->lineofst_src<pstMdmaCfg->width_src)
            || (pstMdmaCfg->lineofst_dst<pstMdmaCfg->width_dst) )
        {
            printk("[MDMA] CAUTION: line offset is less than width\n"
                "width_src=0x%x  lineofst_src=0x%x, DstWidth=0x%x  lineofst_dst=0x%x\n",
                pstMdmaCfg->width_src, pstMdmaCfg->lineofst_src, pstMdmaCfg->width_dst, pstMdmaCfg->lineofst_dst);
            dump_stack();
            return -1;
        }

        tMoveDmaLineOfst.u32SrcWidth    = pstMdmaCfg->width_src;
        tMoveDmaLineOfst.u32SrcOffset   = pstMdmaCfg->lineofst_src;
        tMoveDmaLineOfst.u32DstWidth    = pstMdmaCfg->width_dst;
        tMoveDmaLineOfst.u32DstOffset   = pstMdmaCfg->lineofst_dst;

        tMoveDmaParam.u32Mode           = HAL_MOVEDMA_LINE_OFFSET;
        tMoveDmaParam.pstLineOfst       = &tMoveDmaLineOfst;
    }
    else {
        tMoveDmaParam.u32Mode           = HAL_MOVEDMA_LINEAR;
        tMoveDmaParam.pstLineOfst       = NULL;
    }

    if (HAL_MOVEDMA_NO_ERR != HalMoveDma_MoveData(&tMoveDmaParam)) {
        return -1;
    }

    CamOsTsemDown(&tMovedmaDoneSem);
    CamOsTsemDeinit(&tMovedmaDoneSem);

    return 0;
}
EXPORT_SYMBOL(msys_dma_blit);
#endif
#if defined(CONFIG_MS_BDMA)
static void msys_bdma_done(void *Parm)
{
    CamOsTsem_t *ptBdmaDoneSem = (CamOsTsem_t *)Parm;
    CamOsTsemUp(ptBdmaDoneSem);
}

int msys_dma_fill(MSYS_DMA_FILL *pstDmaCfg)
{
    HalBdmaParam_t  tBdmaParam;
    u8              u8DmaCh = HAL_BDMA_CH1;
    CamOsTsem_t     tBdmaDoneSem;

    memset(&tBdmaDoneSem, 0, sizeof(CamOsTsem_t));
    CamOsTsemInit(&tBdmaDoneSem, 0);
    memset(&tBdmaParam, 0, sizeof(HalBdmaParam_t));
    tBdmaParam.ePathSel     = (pstDmaCfg->phyaddr < ARM_MIU1_BASE_ADDR) ? (HAL_BDMA_MEM_TO_MIU0) : (HAL_BDMA_MEM_TO_MIU1);
    tBdmaParam.bIntMode     = 1;
    tBdmaParam.eDstAddrMode = HAL_BDMA_ADDR_INC;
    tBdmaParam.u32TxCount   = pstDmaCfg->length;
    tBdmaParam.pSrcAddr     = (void*)0;
    tBdmaParam.pDstAddr     = (pstDmaCfg->phyaddr < ARM_MIU1_BASE_ADDR) ? (void *)((U32)pstDmaCfg->phyaddr) : (void *)((U32)pstDmaCfg->phyaddr - ARM_MIU1_BASE_ADDR);
    tBdmaParam.pfTxCbFunc   = msys_bdma_done;
    tBdmaParam.pTxCbParm    = (void *)&tBdmaDoneSem;
    tBdmaParam.u32Pattern   = pstDmaCfg->pattern;

    if (HAL_BDMA_PROC_DONE != HalBdma_Transfer(u8DmaCh, &tBdmaParam)) {
        return -1;
    }

    if (tBdmaParam.bIntMode) {
        CamOsTsemDown(&tBdmaDoneSem);
    }
    CamOsTsemDeinit(&tBdmaDoneSem);

    return 0;
}
EXPORT_SYMBOL(msys_dma_fill);

int  msys_dma_copy(MSYS_DMA_COPY *cfg)
{
    HalBdmaParam_t  tBdmaParam;
#if defined(CONFIG_ARCH_INFINITY2)
    u8              u8DmaCh = HAL_BDMA_CH1;
#else
    u8              u8DmaCh = HAL_BDMA_CH2;
#endif
    CamOsTsem_t     tBdmaDoneSem;

    memset(&tBdmaDoneSem, 0, sizeof(CamOsTsem_t));
    CamOsTsemInit(&tBdmaDoneSem, 0);
    memset(&tBdmaParam, 0, sizeof(HalBdmaParam_t));
    tBdmaParam.ePathSel     = ((U32)cfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (HAL_BDMA_MIU0_TO_MIU0) : (HAL_BDMA_MIU1_TO_MIU0);
    tBdmaParam.ePathSel     = ((U32)cfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? tBdmaParam.ePathSel : tBdmaParam.ePathSel+1;
    tBdmaParam.pSrcAddr     = ((U32)cfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (void *)((U32)cfg->phyaddr_src) : (void *)((U32)cfg->phyaddr_src - ARM_MIU1_BASE_ADDR);
    tBdmaParam.pDstAddr     = ((U32)cfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? (void *)((U32)cfg->phyaddr_dst) : (void *)((U32)cfg->phyaddr_dst - ARM_MIU1_BASE_ADDR);
    tBdmaParam.bIntMode     = 1;
    tBdmaParam.eDstAddrMode = HAL_BDMA_ADDR_INC;
    tBdmaParam.u32TxCount   = cfg->length;
    tBdmaParam.pfTxCbFunc   = msys_bdma_done;
    tBdmaParam.pTxCbParm    = (void *)&tBdmaDoneSem;
    tBdmaParam.u32Pattern   = 0;

    if (HAL_BDMA_PROC_DONE != HalBdma_Transfer(u8DmaCh, &tBdmaParam)) {
        return -1;
    }

    if (tBdmaParam.bIntMode) {
        CamOsTsemDown(&tBdmaDoneSem);
    }
    CamOsTsemDeinit(&tBdmaDoneSem);

    return 0;
}
EXPORT_SYMBOL(msys_dma_copy);

#if defined(CONFIG_MS_BDMA_LINE_OFFSET_ON)
int msys_dma_fill_lineoffset(MSYS_DMA_FILL_BILT *pstDmaCfg)
{
    HalBdmaParam_t  tBdmaParam;
    HalBdmaLineOfst_t tBdmaLineOfst;
    u8              u8DmaCh = HAL_BDMA_CH1;
    CamOsTsem_t     tBdmaDoneSem;

    memset(&tBdmaDoneSem, 0, sizeof(CamOsTsem_t));
    CamOsTsemInit(&tBdmaDoneSem, 0);
    memset(&tBdmaParam, 0, sizeof(HalBdmaParam_t));
    memset(&tBdmaLineOfst, 0, sizeof(HalBdmaLineOfst_t));
    tBdmaParam.ePathSel     = (pstDmaCfg->phyaddr < ARM_MIU1_BASE_ADDR) ? (HAL_BDMA_MEM_TO_MIU0) : (HAL_BDMA_MEM_TO_MIU1);
    tBdmaParam.bIntMode     = 1;
    tBdmaParam.eDstAddrMode = HAL_BDMA_ADDR_INC;
    tBdmaParam.u32TxCount   = pstDmaCfg->length;
    tBdmaParam.pSrcAddr     = (void*)0;
    tBdmaParam.pDstAddr     = (pstDmaCfg->phyaddr < ARM_MIU1_BASE_ADDR) ? (void *)((U32)pstDmaCfg->phyaddr) : (void *)((U32)pstDmaCfg->phyaddr - ARM_MIU1_BASE_ADDR);
    tBdmaParam.pfTxCbFunc   = msys_bdma_done;
    tBdmaParam.pTxCbParm    = (void *)&tBdmaDoneSem;
    tBdmaParam.u32Pattern   = pstDmaCfg->pattern;

    if (pstDmaCfg->lineofst_dst)
    {
        if (pstDmaCfg->lineofst_dst < pstDmaCfg->width_dst)
        {
            printk("[BDMA] CAUTION: line offset is less than width\n"
            "DstWidth=0x%x  lineofst_dst=0x%x\n",
            pstDmaCfg->width_dst, pstDmaCfg->lineofst_dst);
            dump_stack();
            return -1;
        }

        tBdmaParam.pstLineOfst = &tBdmaLineOfst;
        tBdmaParam.pstLineOfst->u32SrcWidth  = pstDmaCfg->width_dst;
        tBdmaParam.pstLineOfst->u32SrcOffset = pstDmaCfg->lineofst_dst;
        tBdmaParam.pstLineOfst->u32DstWidth  = pstDmaCfg->width_dst;
        tBdmaParam.pstLineOfst->u32DstOffset = pstDmaCfg->lineofst_dst;

        tBdmaParam.bEnLineOfst = 1;
    }
    else {
        tBdmaParam.bEnLineOfst = 0;
        tBdmaParam.pstLineOfst = NULL;
    }

    if (HAL_BDMA_PROC_DONE != HalBdma_Transfer_LineOffset(u8DmaCh, &tBdmaParam)) {
        return -1;
    }

    if (tBdmaParam.bIntMode) {
        CamOsTsemDown(&tBdmaDoneSem);
    }
    CamOsTsemDeinit(&tBdmaDoneSem);

    return 0;
}
EXPORT_SYMBOL(msys_dma_fill_lineoffset);

int msys_dma_copy_lineoffset(MSYS_DMA_BLIT *cfg)
{
    HalBdmaParam_t    tBdmaParam;
    HalBdmaLineOfst_t tBdmaLineOfst;
#if defined(CONFIG_ARCH_INFINITY2)
    u8              u8DmaCh = HAL_BDMA_CH1;
#else
    u8              u8DmaCh = HAL_BDMA_CH2;
#endif
    CamOsTsem_t     tBdmaDoneSem;

    memset(&tBdmaDoneSem, 0, sizeof(CamOsTsem_t));
    CamOsTsemInit(&tBdmaDoneSem, 0);
    memset(&tBdmaParam, 0, sizeof(HalBdmaParam_t));
    memset(&tBdmaLineOfst, 0, sizeof(HalBdmaLineOfst_t));
    tBdmaParam.ePathSel     = ((U32)cfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (HAL_BDMA_MIU0_TO_MIU0) : (HAL_BDMA_MIU1_TO_MIU0);
    tBdmaParam.ePathSel     = ((U32)cfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? tBdmaParam.ePathSel : tBdmaParam.ePathSel+1;
    tBdmaParam.pSrcAddr     = ((U32)cfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (void *)((U32)cfg->phyaddr_src) : (void *)((U32)cfg->phyaddr_src - ARM_MIU1_BASE_ADDR);
    tBdmaParam.pDstAddr     = ((U32)cfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? (void *)((U32)cfg->phyaddr_dst) : (void *)((U32)cfg->phyaddr_dst - ARM_MIU1_BASE_ADDR);
    tBdmaParam.bIntMode     = 1;
    tBdmaParam.eDstAddrMode = HAL_BDMA_ADDR_INC;
    tBdmaParam.u32TxCount   = cfg->length;
    tBdmaParam.pfTxCbFunc   = msys_bdma_done;
    tBdmaParam.pTxCbParm    = (void *)&tBdmaDoneSem;
    tBdmaParam.u32Pattern   = 0;

    if (cfg->lineofst_src && cfg->lineofst_dst)
    {
        if((cfg->lineofst_src < cfg->width_src)
        || (cfg->lineofst_dst < cfg->width_dst) )
        {
            printk("[BDMA] CAUTION: line offset is less than width\n"
            "width_src=0x%x  lineofst_src=0x%x, DstWidth=0x%x  lineofst_dst=0x%x\n",
            cfg->width_src, cfg->lineofst_src, cfg->width_dst, cfg->lineofst_dst);
            dump_stack();
            return -1;
        }

        tBdmaParam.pstLineOfst = &tBdmaLineOfst;
        tBdmaParam.pstLineOfst->u32SrcWidth  = cfg->width_src;
        tBdmaParam.pstLineOfst->u32SrcOffset = cfg->lineofst_src;
        tBdmaParam.pstLineOfst->u32DstWidth  = cfg->width_dst;
        tBdmaParam.pstLineOfst->u32DstOffset = cfg->lineofst_dst;

        tBdmaParam.bEnLineOfst = 1;
    }
    else {
        tBdmaParam.bEnLineOfst = 0;
        tBdmaParam.pstLineOfst = NULL;
    }

    if (HAL_BDMA_PROC_DONE != HalBdma_Transfer_LineOffset(u8DmaCh, &tBdmaParam)) {
        return -1;
    }

    if (tBdmaParam.bIntMode) {
        CamOsTsemDown(&tBdmaDoneSem);
    }
    CamOsTsemDeinit(&tBdmaDoneSem);

    return 0;
}
EXPORT_SYMBOL(msys_dma_copy_lineoffset);

#endif

#endif

static int __init ms_msys_dma_wrapper_init(void)
{
#if defined(CONFIG_MS_MOVE_DMA)

    HalMoveDma_Initialize();
#endif

#if defined(CONFIG_MS_BDMA)

    //HalBdma_Initialize(0);
    HalBdma_Initialize(1);
    HalBdma_Initialize(2);
    HalBdma_Initialize(3);
#endif
    return 0;
}
subsys_initcall(ms_msys_dma_wrapper_init)

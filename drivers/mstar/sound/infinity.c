// $Change: 548282 $
//
// bach.cpp
//
// defines for the registers in the iNfinity3 BACH chip
//
//
#if defined(__linux__)
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <linux/delay.h>
#include <linux/spinlock.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <asm/io.h>
#include "infinity.h"


#define DELAY(x) mdelay(x)
#define SLEEP(x) schedule_timeout(msecs_to_jiffies(x * 1000))
#define UDELAY(x) udelay(x)		//CAPDELAY(1000*us)

#endif


const U16 g_nInfinityDmaIntReg[BACH_DMA_NUM][BACH_DMA_INT_NUM] =
{
    {REG_WR_UNDERRUN_INT_EN, REG_WR_OVERRUN_INT_EN, 0, REG_WR_FULL_INT_EN},
    {REG_RD_UNDERRUN_INT_EN, REG_RD_OVERRUN_INT_EN, REG_RD_EMPTY_INT_EN, 0}
};

static S8  m_nInfinityDpgaGain[4] = {0, 0, 0, 0};


//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------


static DMACHANNEL m_infinitydmachannel[2]; // info about dma channel states

static U32  m_pInfinityBaseRegAddr;
static U32  m_pInfinityAudBank1RegAddr;
static U32  m_pInfinityAudBank2RegAddr;
static U32  m_pInfinityAudBank3RegAddr;

static BOOL m_bADCActive;
static BOOL m_bDACActive;

static U16 m_nMicGain = 0x0;
static U16 m_nMicInGain = 0x011;
static U16 m_nLineInGain = 0x000;

static BOOL m_bInfinityAtopStatus[BACH_ATOP_NUM];




//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
U16 InfinityGetMaskReg(BachRegBank_e nBank, U8 nAddr)
{
    return InfinityReadReg(nBank, nAddr);
}

void InfinitySetBank1BaseAddr(U32 addr)
{
    m_pInfinityAudBank1RegAddr = addr;
}

void InfinitySetBank2BaseAddr(U32 addr)
{
    m_pInfinityAudBank2RegAddr = addr;
}

void InfinitySetBank3BaseAddr(U32 addr)
{
    m_pInfinityAudBank3RegAddr = addr;
}




void InfinitySetBankBaseAddr(U32 addr)
{
    m_pInfinityBaseRegAddr = addr;
    m_pInfinityAudBank1RegAddr = addr + (U32)(BACH_REG_BANK_1 << 1);
    m_pInfinityAudBank2RegAddr = addr + (U32)(BACH_REG_BANK_2 << 1);
    m_pInfinityAudBank3RegAddr = addr + (U32)(BACH_REG_BANK_3 << 1);
}
EXPORT_SYMBOL_GPL(InfinitySetBankBaseAddr);

void InfinityWriteReg2Byte(U32 nAddr, U16 nValue)
{
    WRITE_WORD(m_pInfinityBaseRegAddr + ((nAddr) << 1), nValue);
}

void InfinityWriteRegByte(U32 nAddr, U8 nValue)
{
    WRITE_BYTE(m_pInfinityBaseRegAddr + ((nAddr) << 1) - ((nAddr) & 1), nValue);
}

U16 InfinityReadReg2Byte(U32 nAddr)
{
    return READ_WORD(m_pInfinityBaseRegAddr + ((nAddr) << 1));
}
EXPORT_SYMBOL_GPL(InfinityReadReg2Byte);

U8 InfinityReadRegByte(U32 nAddr)
{
    return READ_BYTE(m_pInfinityBaseRegAddr + ((nAddr) << 1) - ((nAddr) & 1));
}

void InfinityWriteReg(BachRegBank_e nBank, U8 nAddr, U16 regMsk, U16 nValue)
{
    U16 nConfigValue;

    switch(nBank)
    {
    case BACH_REG_BANK1:
        nConfigValue = READ_WORD(m_pInfinityAudBank1RegAddr + ((nAddr) << 1));
        nConfigValue &= ~regMsk;
        nConfigValue |= (nValue & regMsk);
        WRITE_WORD(m_pInfinityAudBank1RegAddr + ((nAddr) << 1), nConfigValue);
        break;
    case BACH_REG_BANK2:
        nConfigValue = READ_WORD(m_pInfinityAudBank2RegAddr + ((nAddr) << 1));
        nConfigValue &= ~regMsk;
        nConfigValue |= (nValue & regMsk);
        WRITE_WORD(m_pInfinityAudBank2RegAddr + ((nAddr) << 1), nConfigValue);
        break;
    case BACH_REG_BANK3:
        nConfigValue = READ_WORD(m_pInfinityAudBank3RegAddr + ((nAddr) << 1));
        nConfigValue &= ~regMsk;
        nConfigValue |= (nValue & regMsk);
        WRITE_WORD(m_pInfinityAudBank3RegAddr + ((nAddr) << 1), nConfigValue);
        break;
    default:
        ERRMSG("WAVEDEV.DLL: InfinityWriteReg - ERROR bank default case!\n");
        break;
    }
}


U16 InfinityReadReg(BachRegBank_e nBank, U8 nAddr)
{
    switch(nBank)
    {
    case BACH_REG_BANK1:
        return READ_WORD(m_pInfinityAudBank1RegAddr + ((nAddr) << 1));
    case BACH_REG_BANK2:
        return READ_WORD(m_pInfinityAudBank2RegAddr + ((nAddr) << 1));
    case BACH_REG_BANK3:
        return READ_WORD(m_pInfinityAudBank3RegAddr + ((nAddr) << 1));
    default:
        ERRMSG("WAVEDEV.DLL: InfinityReadReg - ERROR bank default case!\n");
        return 0;
    }
}

//------------------------------------------------------------------------------
//
//  Function:   BachC3::MapHardware
//
//  Description
//      Maps port address, assigns SysIntr.
//
//  Parameters
//      u32RegAddr:     [in] Physical address of audio hardware register.
//      u32RegLength:   [in] Length of the address space of audio hardware register.
//
//  Return Value
//      Returns TRUE if device was mapped properly
//      Return FALSE if device could not be mapped
//------------------------------------------------------------------------------
void InfinityDmaInitChannel( U32 nChannelIndex,
                             U32 nPhysDMAAddr,
                             U32 nBufferSize,
                             U32 nChannels,
                             U32 nSampleSize,
                             U32 nSampleRate,
                             U32 nOverrunTh,
                             U32 nUnderrunTh
                           )
{
    //U16 nConfigValue;

    // save off the info for power managment
    m_infinitydmachannel[nChannelIndex].nPhysDMAAddr = nPhysDMAAddr;
    m_infinitydmachannel[nChannelIndex].nBufferSize  = nBufferSize;
    m_infinitydmachannel[nChannelIndex].nChannels   = nChannels;
    m_infinitydmachannel[nChannelIndex].nSampleSize = nSampleSize;
    m_infinitydmachannel[nChannelIndex].nSampleRate = nSampleRate;


    // Set up the physical DMA buffer address
    InfinityDmaSetPhyAddr((BachDmaChannel_e)nChannelIndex, nPhysDMAAddr, nBufferSize);


    // Set up the underrun and overrun
    //DmaSetThreshold((BachDmaChannel_e)nChannelIndex, nBufferSize+MIU_WORD_BYTE_SIZE, (nBufferSize/4)*3);
    InfinityDmaSetThreshold((BachDmaChannel_e)nChannelIndex, nOverrunTh, nUnderrunTh);

    // Set up channel mode
    InfinityDmaSetChMode((BachDmaChannel_e)nChannelIndex, (nChannels==1 ? TRUE:FALSE));

    return ;
}
EXPORT_SYMBOL_GPL(InfinityDmaInitChannel);

void InfinityDmaReset(void)
{
    //rest DMA1 interal register

    InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_4, 0xFFFF, 0);		//reset DMA 1 read size
    InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_12, 0xFFFF, 0);		//reset DMA 1 write size

    InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_SW_RST_DMA, REG_SW_RST_DMA);		//DMA 1 software reset
    InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_SW_RST_DMA, 0);


    InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, (REG_PRIORITY_KEEP_HIGH | REG_RD_LEVEL_CNT_LIVE_MASK),
                     (REG_PRIORITY_KEEP_HIGH | REG_RD_LEVEL_CNT_LIVE_MASK));

    //enable DMA interrupt
    InfinityWriteReg(BACH_REG_BANK2, BACH_INT_EN, REG_DMA_INT_EN, REG_DMA_INT_EN);
}
EXPORT_SYMBOL_GPL(InfinityDmaReset);

void InfinityDmaReInit(BachDmaChannel_e eDmaChannel)
{
    switch ( eDmaChannel )
    {
    case BACH_DMA_READER1:
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_TRIG, 0); // prevent from triggering levelcount at toggling init step
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_INIT, REG_RD_INIT);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_INIT, 0);
        break;

    case BACH_DMA_WRITER1:
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_TRIG, 0);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_INIT, REG_WR_INIT);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_INIT, 0);
        break;

    default:
        ERRMSG("InfinityDmaReInit - ERROR bank default case!\n");
        break;
    }

}

void InfinityDmaEnable(BachDmaChannel_e eDmaChannel, BOOL bEnable)
{
    switch ( eDmaChannel )
    {
    case BACH_DMA_READER1:

        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0,
                         (REG_RD_EMPTY_INT_EN | REG_RD_UNDERRUN_INT_EN),
                         (bEnable ? (REG_RD_EMPTY_INT_EN | REG_RD_UNDERRUN_INT_EN) : 0));

        if(bEnable)
        {
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_ENABLE, REG_ENABLE); //reader prefetch enable, it should be enabled before reader enable
            udelay(10);
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_ENABLE, REG_RD_ENABLE);
        }
        else
        {
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_ENABLE, 0);
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_ENABLE, 0); //reader prefetch enable, it has to be disabled before dma init
        }

        break;

    case BACH_DMA_WRITER1:

        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0,
                         (REG_WR_FULL_INT_EN | REG_WR_OVERRUN_INT_EN),
                         (bEnable ? (REG_WR_FULL_INT_EN | REG_WR_OVERRUN_INT_EN) : 0));

        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_ENABLE, (bEnable ? REG_WR_ENABLE : 0));

        break;

    default:
        ERRMSG("InfinityDmaEnable - ERROR bank default case!\n");
        break;

    }

}

void InfinityDmaStartChannel(BachDmaChannel_e eDmaChannel)
{
    InfinityDmaClearInt(eDmaChannel);
    InfinityDmaEnable(eDmaChannel, TRUE);
    m_infinitydmachannel[eDmaChannel].nDMAChannelState = DMA_RUNNING;  // save the state
    return;
}
EXPORT_SYMBOL_GPL(InfinityDmaStartChannel);

void InfinityDmaStopChannel(BachDmaChannel_e eDmaChannel)
{
    InfinityDmaEnable(eDmaChannel, FALSE);
    InfinityDmaReInit(eDmaChannel);

    //Sleep(100);
    // save the state
    m_infinitydmachannel[eDmaChannel].nDMAChannelState = DMA_STOPPED;
    return;
}
EXPORT_SYMBOL_GPL(InfinityDmaStopChannel);

/**
 * \brief clear DMA2 interrupt
 */
void InfinityDmaClearInt(BachDmaChannel_e eDmaChannel)
{
    switch(eDmaChannel)
    {

    case BACH_DMA_WRITER1:
        //DMA writer full flag clear / DMA writer local buffer full flag clear
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_WR_FULL_FLAG_CLR, REG_WR_FULL_FLAG_CLR);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_WR_FULL_FLAG_CLR, 0);
        break;

    case BACH_DMA_READER1:
        //DMA reader empty flag clear / DMA reader local buffer empty flag clear
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_RD_EMPTY_FLAG_CLR, REG_RD_EMPTY_FLAG_CLR);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0, REG_RD_EMPTY_FLAG_CLR, 0);

        break;

    default:
        ERRMSG("InfinityDmaClearInt - ERROR bank default case!\n");
        break;
    }

    return;
}
EXPORT_SYMBOL_GPL(InfinityDmaClearInt);

U32 InfinityDmaGetLevelCnt(BachDmaChannel_e eDmaChannel)
{
    U16 nConfigValue = 0;
    U32 nByteSize = 0;

    switch(eDmaChannel)
    {
    case BACH_DMA_WRITER1:
    {

        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_LEVEL_CNT_MASK, REG_WR_LEVEL_CNT_MASK);
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_15);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_LEVEL_CNT_MASK, 0);
        nConfigValue = ((nConfigValue>8)? (nConfigValue-8):0); //level count contains the local buffer data size
        break;
    }

    case BACH_DMA_READER1:
    {

        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_LEVEL_CNT_MASK, REG_RD_LEVEL_CNT_MASK);
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_7);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_LEVEL_CNT_MASK, 0);
        break;
    }

    default:
        ERRMSG("InfinityDmaGetLevelCnt - ERROR bank default case!\n");
        return 0;

    }

    nByteSize = nConfigValue * MIU_WORD_BYTE_SIZE;

    return nByteSize;
}
EXPORT_SYMBOL_GPL(InfinityDmaGetLevelCnt);


void InfinityDmaSetThreshold(BachDmaChannel_e eDmaChannel, U32 nOverrunTh, U32 nUnderrunTh)
{
    U16 nMiuOverrunTh, nMiuUnderrunTh;

    switch(eDmaChannel)
    {
    case BACH_DMA_WRITER1:
        nMiuOverrunTh = (U16)((nOverrunTh / MIU_WORD_BYTE_SIZE) & REG_WR_OVERRUN_TH_MSK);
        nMiuUnderrunTh = (U16)((nUnderrunTh / MIU_WORD_BYTE_SIZE) & REG_WR_UNDERRUN_TH_MSK);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_13, 0xFFFF, nMiuOverrunTh);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_14, 0xFFFF, nMiuUnderrunTh);
        break;

    case BACH_DMA_READER1:

        nMiuOverrunTh = (U16)((nOverrunTh / MIU_WORD_BYTE_SIZE) & REG_RD_OVERRUN_TH_MSK);
        nMiuUnderrunTh = (U16)((nUnderrunTh / MIU_WORD_BYTE_SIZE) & REG_RD_UNDERRUN_TH_MSK);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_5, 0xFFFF, nMiuOverrunTh);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_6, 0xFFFF, nMiuUnderrunTh);
        break;

    default:
        ERRMSG("InfinityDmaSetThreshold - ERROR bank default case!\n");
        break;
    }

}
EXPORT_SYMBOL_GPL(InfinityDmaSetThreshold);

/**
 * \brief DMA set MIU address
 */
void InfinityDmaSetPhyAddr(BachDmaChannel_e eDmaChannel, U32 nBufAddrOffset, U32 nBufSize)
{
    U16 nMiuAddrLo, nMiuAddrHi, nMiuSize;

    switch(eDmaChannel)
    {
    case BACH_DMA_WRITER1:
        nMiuAddrLo = (U16)((nBufAddrOffset / MIU_WORD_BYTE_SIZE) & REG_WR_BASE_ADDR_LO_MSK);
        nMiuAddrHi = (U16)(((nBufAddrOffset / MIU_WORD_BYTE_SIZE) >> REG_WR_BASE_ADDR_HI_OFFSET) & REG_WR_BASE_ADDR_HI_MSK);
        nMiuSize = (U16)((nBufSize / MIU_WORD_BYTE_SIZE) & REG_WR_BUFF_SIZE_MSK);


        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_BASE_ADDR_LO_MSK, nMiuAddrLo);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_10, REG_WR_BASE_ADDR_HI_MSK, nMiuAddrHi);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_11, 0xFFFF, nMiuSize);

        break;

    case BACH_DMA_READER1:

        nMiuAddrLo = (U16)((nBufAddrOffset / MIU_WORD_BYTE_SIZE) & REG_RD_BASE_ADDR_LO_MSK);
        nMiuAddrHi = (U16)(((nBufAddrOffset / MIU_WORD_BYTE_SIZE) >> REG_RD_BASE_ADDR_HI_OFFSET) & REG_RD_BASE_ADDR_HI_MSK);
        nMiuSize = (U16)((nBufSize / MIU_WORD_BYTE_SIZE) & REG_RD_BUFF_SIZE_MSK);

        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_BASE_ADDR_LO_MSK, nMiuAddrLo);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_2, REG_RD_BASE_ADDR_HI_MSK, nMiuAddrHi);
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_3, 0xFFFF, nMiuSize);

        break;

    default:
        ERRMSG("InfinityDmaSetPhyAddr - ERROR bank default case!\n");
        break;

    }

}

BOOL InfinityDmaMaskInt(BachDmaChannel_e eDmaChan, BachDmaInterrupt_e eDmaInt, BOOL bMask)
{
    switch(eDmaChan)
    {
    case BACH_DMA_READER1:
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0,
                         g_nInfinityDmaIntReg[BACH_DMA_READER1][eDmaInt], (bMask ? 0 : g_nInfinityDmaIntReg[BACH_DMA_READER1][eDmaInt]));
        break;

    case BACH_DMA_WRITER1:
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0,
                         g_nInfinityDmaIntReg[BACH_DMA_WRITER1][eDmaInt], (bMask ? 0 : g_nInfinityDmaIntReg[BACH_DMA_WRITER1][eDmaInt]));
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
EXPORT_SYMBOL_GPL(InfinityDmaMaskInt);

BOOL InfinityDmaIsFull(BachDmaChannel_e eDmaChannel)
{
    U16 nConfigValue;

    switch(eDmaChannel)
    {
    case BACH_DMA_WRITER1:
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8);
        return (nConfigValue & REG_WR_FULL_FLAG) ? TRUE : FALSE;

    default:
        return FALSE;
    }
}
EXPORT_SYMBOL_GPL(InfinityDmaIsFull);

BOOL InfinityDmaIsEmpty(BachDmaChannel_e eDmaChannel)
{
    U16 nConfigValue;

    switch(eDmaChannel)
    {
    case BACH_DMA_READER1:
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8);
        return (nConfigValue & REG_RD_EMPTY_FLAG) ? TRUE : FALSE;

    default:
        return FALSE;
    }
}
EXPORT_SYMBOL_GPL(InfinityDmaIsEmpty);

BOOL InfinityDmaIsLocalEmpty(BachDmaChannel_e eDmaChannel)
{
    U16 nConfigValue;

    switch(eDmaChannel)
    {
    case BACH_DMA_READER1:
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8);
        return (nConfigValue & REG_RD_LOCALBUF_EMPTY) ? TRUE : FALSE;

    default:
        return FALSE;
    }
}


BOOL InfinityDmaIsUnderrun(BachDmaChannel_e eDmaChannel)
{
    U16 nConfigValue;

    switch(eDmaChannel)
    {
    case BACH_DMA_WRITER1:
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8);
        return (nConfigValue & REG_WR_UNDERRUN_FLAG) ? TRUE : FALSE;

    case BACH_DMA_READER1:
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8);
        return (nConfigValue & REG_RD_UNDERRUN_FLAG) ? TRUE : FALSE;

    default:
        ERRMSG("InfinityDmaIsUnderrun - ERROR default case!\n");
        return FALSE;
    }

    return FALSE;
}
EXPORT_SYMBOL_GPL(InfinityDmaIsUnderrun);

BOOL InfinityDmaIsOverrun(BachDmaChannel_e eDmaChannel)
{
    U16 nConfigValue;

    switch(eDmaChannel)
    {
    case BACH_DMA_WRITER1:
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8);
        return (nConfigValue & REG_WR_OVERRUN_FLAG) ? TRUE : FALSE;

    case BACH_DMA_READER1:
        nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8);
        return (nConfigValue & REG_RD_OVERRUN_FLAG) ? TRUE : FALSE;

    default:
        return FALSE;
    }

    return FALSE;
}
EXPORT_SYMBOL_GPL(InfinityDmaIsOverrun);

U32 InfinityDmaTrigLevelCnt(BachDmaChannel_e eDmaChannel, U32 nDataSize)
{
    U16 nConfigValue = 0;

    nConfigValue = (U16)((nDataSize / MIU_WORD_BYTE_SIZE) & REG_WR_SIZE_MSK);
    nDataSize = nConfigValue * MIU_WORD_BYTE_SIZE;

    if (nConfigValue > 0)
    {
        switch(eDmaChannel)
        {
        case BACH_DMA_WRITER1:
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_12, 0xFFFF, nConfigValue);
            nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9);
            if(nConfigValue & REG_WR_TRIG)
            {
                InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_TRIG, 0);
            }
            else
            {
                InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_9, REG_WR_TRIG, REG_WR_TRIG);
            }

            break;

        case BACH_DMA_READER1:
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_4, 0xFFFF, nConfigValue);
            nConfigValue = InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1);
            if(nConfigValue & REG_RD_TRIG)
            {
                InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_TRIG, 0);
            }
            else
            {
                InfinityWriteReg(BACH_REG_BANK1, BACH_DMA1_CTRL_1, REG_RD_TRIG, REG_RD_TRIG);
            }
            break;

        default:
            ERRMSG("InfinityDmaTrigLevelCnt - ERROR bank default case!\n");
            return 0;
        }

        return nDataSize;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(InfinityDmaTrigLevelCnt);

U32 InfinityRateToU32(BachRate_e eRate)
{
    switch(eRate)
    {
    case BACH_RATE_8K:
        return 8000;
    case BACH_RATE_11K:
        return 11025;
    case BACH_RATE_12K:
        return 12000;
    case BACH_RATE_16K:
        return 16000;
    case BACH_RATE_22K:
        return 22050;
    case BACH_RATE_24K:
        return 24000;
    case BACH_RATE_32K:
        return 32000;
    case BACH_RATE_44K:
        return 44100;
    case BACH_RATE_48K:
        return 48000;
    default:
        return 0;
    }
}

BachRate_e InfinityRateFromU32(U32 nRate)
{
    switch(nRate)
    {
    case 8000:
        return BACH_RATE_8K;
    case 11025:
        return BACH_RATE_11K;
    case 12000:
        return BACH_RATE_12K;
    case 16000:
        return BACH_RATE_16K;
    case 22050:
        return BACH_RATE_22K;
    case 24000:
        return BACH_RATE_24K;
    case 32000:
        return BACH_RATE_32K;
    case 44100:
        return BACH_RATE_44K;
    case 48000:
        return BACH_RATE_48K;
    default:
        return BACH_RATE_NULL;
    }
}
EXPORT_SYMBOL_GPL(InfinityRateFromU32);

BOOL InfinityDmaSetRate(BachDmaChannel_e eDmaChannel, BachRate_e eRate)
{

    //TODO:
    switch(eDmaChannel)
    {
        //ADC rate should be set according to the DMA writer rate
    case BACH_DMA_WRITER1:
        switch(eRate)
        {
        case BACH_RATE_8K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_WRITER_SEL_MSK, 0<<REG_WRITER_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_CIC_3_SEL_MSK, 0<<REG_CIC_3_SEL_POS);
            break;
        case BACH_RATE_16K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_WRITER_SEL_MSK, 1<<REG_WRITER_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_CIC_3_SEL_MSK, 1<<REG_CIC_3_SEL_POS);
            break;
        case BACH_RATE_32K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_WRITER_SEL_MSK, 2<<REG_WRITER_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_CIC_3_SEL_MSK, 2<<REG_CIC_3_SEL_POS);
            break;
        case BACH_RATE_48K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_WRITER_SEL_MSK, 3<<REG_WRITER_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_CIC_3_SEL_MSK, 3<<REG_CIC_3_SEL_POS);
            break;
        default:
            return FALSE;
        }
        break;

    case BACH_DMA_READER1:
        switch(eRate)
        {
        case BACH_RATE_8K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 0<<REG_SRC1_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK2, BACH_AU_SYS_CTRL1, REG_CODEC_SEL_MSK, 0<<REG_CODEC_SEL_POS);
            break;
        case BACH_RATE_11K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 1<<REG_SRC1_SEL_POS);
            break;
        case BACH_RATE_12K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 2<<REG_SRC1_SEL_POS);
            break;
        case BACH_RATE_16K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 3<<REG_SRC1_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK2, BACH_AU_SYS_CTRL1, REG_CODEC_SEL_MSK, 1<<REG_CODEC_SEL_POS);
            break;
        case BACH_RATE_22K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 4<<REG_SRC1_SEL_POS);
            break;
        case BACH_RATE_24K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 5<<REG_SRC1_SEL_POS);
            break;
        case BACH_RATE_32K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 6<<REG_SRC1_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK2, BACH_AU_SYS_CTRL1, REG_CODEC_SEL_MSK, 2<<REG_CODEC_SEL_POS);
            break;
        case BACH_RATE_44K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 7<<REG_SRC1_SEL_POS);
            break;
        case BACH_RATE_48K:
            InfinityWriteReg(BACH_REG_BANK1, BACH_SR0_SEL, REG_SRC1_SEL_MSK, 8<<REG_SRC1_SEL_POS);
            InfinityWriteReg(BACH_REG_BANK2, BACH_AU_SYS_CTRL1, REG_CODEC_SEL_MSK, 3<<REG_CODEC_SEL_POS);
            break;
        default:
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}
EXPORT_SYMBOL_GPL(InfinityDmaSetRate);

U32 InfinityDmaGetRate(BachDmaChannel_e eDmaChannel)
{
    return m_infinitydmachannel[eDmaChannel].nSampleRate;
}
EXPORT_SYMBOL_GPL(InfinityDmaGetRate);

void InfinityDmaSetChMode(BachDmaChannel_e eDma, BOOL bMono)
{
    switch(eDma)
    {
    case BACH_DMA_WRITER1:
        if(bMono)
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA_TEST_CTRL7, REG_DMA1_WR_MONO, REG_DMA1_WR_MONO);
        else
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA_TEST_CTRL7, REG_DMA1_WR_MONO, 0);
        return;
    case BACH_DMA_READER1:
        if(bMono)
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA_TEST_CTRL7, REG_DMA1_RD_MONO | REG_DMA1_RD_MONO_COPY, REG_DMA1_RD_MONO | REG_DMA1_RD_MONO_COPY);
        else
            InfinityWriteReg(BACH_REG_BANK1, BACH_DMA_TEST_CTRL7, REG_DMA1_RD_MONO | REG_DMA1_RD_MONO_COPY, 0);
        return;
    default:
        ERRMSG("InfinityDmaSetChMode - unexpected DMA channel\n");
        return;
    }
}

BOOL InfinityDmaIsWork(BachDmaChannel_e eDmaChannel)
{
    return ((m_infinitydmachannel[eDmaChannel].nDMAChannelState==DMA_RUNNING)? TRUE : FALSE);
}
EXPORT_SYMBOL_GPL(InfinityDmaIsWork);

void InfinityDpgaCtrl(BachDpga_e eDpga, BOOL bEnable, BOOL bMute, BOOL bFade)
{
    U8 nAddr = 0;
    U16 nConfigValue;

    switch(eDpga)
    {
    case BACH_DPGA_MMC1:
        nAddr = BACH_MMC1_DPGA_CFG1;
        break;
    case BACH_DPGA_ADC:
        nAddr = BACH_ADC_DPGA_CFG1;
        break;
    case BACH_DPGA_AEC1:
        nAddr = BACH_AEC1_DPGA_CFG1;
        break;
    case BACH_DPGA_DEC1:
        nAddr = BACH_MMCDEC1_DPGA_CFG1;
        break;
    default:
        ERRMSG("InfinityDpgaCtrl - ERROR default case!\n");
        return;
    }

    nConfigValue = 0;
    if(bEnable)
        nConfigValue |= DPGA_EN;
    if(bMute)
        nConfigValue |= MUTE_2_ZERO;
    if(bFade)
        nConfigValue |= FADING_EN;

    InfinityWriteReg(BACH_REG_BANK1, nAddr, (DPGA_EN | MUTE_2_ZERO | FADING_EN), nConfigValue);
}

void InfinityDpgaCalGain(S8 s8Gain, U8 *pu8GainIdx)//ori step:0.5dB,new step 1dB
{
    if(s8Gain > BACH_DPGA_GAIN_MAX_DB)
        s8Gain = BACH_DPGA_GAIN_MAX_DB;
    else if(s8Gain < BACH_DPGA_GAIN_MIN_DB)
        s8Gain = BACH_DPGA_GAIN_MIN_DB;

    if(s8Gain == BACH_DPGA_GAIN_MIN_DB)
        *pu8GainIdx = BACH_DPGA_GAIN_MIN_IDX;
    else
        *pu8GainIdx = (U8)(-2 * s8Gain); //index = -2 * (gain) ,because step = -0.5dB
}

S8 InfinityDpgaGetGain(BachDpga_e eDpga)
{
    U16 nConfigValue;
    U8	nGainIdx, nAddr;
    S8	nGain;

    switch(eDpga)
    {
    case BACH_DPGA_MMC1:
        nAddr = BACH_MMC1_DPGA_CFG2;
        break;
    case BACH_DPGA_ADC:
        nAddr = BACH_ADC_DPGA_CFG2;
        break;
    case BACH_DPGA_AEC1:
        nAddr = BACH_AEC1_DPGA_CFG2;
        break;
    case BACH_DPGA_DEC1:
        nAddr = BACH_MMCDEC1_DPGA_CFG2;
        break;
    default:
        ERRMSG("InfinityDpgaGetGain - ERROR default case!\n");
        return 0;
    }

    nConfigValue = InfinityReadReg(BACH_REG_BANK1, nAddr);
    nGainIdx = (U8)(nConfigValue & REG_GAIN_L_MSK);
    if(nGainIdx == BACH_DPGA_GAIN_MIN_IDX)
        nGain = BACH_DPGA_GAIN_MIN_DB;
    else
        nGain = -((S8)nGainIdx) / 2;
    return nGain;
}

void InfinityDpgaSetGain(BachDpga_e eDpga, S8 s8Gain)
{
    U8 nAddr;
    U8 nGainIdx,nLGain,nRGain;

    InfinityDpgaCalGain(s8Gain, &nGainIdx);

    switch(eDpga)
    {
    case BACH_DPGA_MMC1:
        nAddr = BACH_MMC1_DPGA_CFG2;
        nLGain = nGainIdx;
        nRGain = nGainIdx;
        break;
    case BACH_DPGA_ADC:
        nAddr = BACH_ADC_DPGA_CFG2;
        nLGain = nGainIdx;
        nRGain = 0;
        break;
    case BACH_DPGA_AEC1:
        nAddr = BACH_AEC1_DPGA_CFG2;
        nLGain = nGainIdx;
        nRGain = 0;
        break;
    case BACH_DPGA_DEC1:
        nAddr = BACH_MMCDEC1_DPGA_CFG2;
        nLGain = nGainIdx;
        nRGain = nGainIdx;
        break;
    default:
        ERRMSG("InfinityDpgaSetGain - ERROR default case!\n");
        return;
    }

    //set gain
    InfinityWriteReg(BACH_REG_BANK1, nAddr, REG_GAIN_R_MSK | REG_GAIN_L_MSK, (nRGain<<REG_GAIN_R_POS) | (nLGain<<REG_GAIN_L_POS));
}

void InfinitySetPathOnOff(BachPath_e ePath, BOOL bOn)
{
    switch(ePath)
    {
    case BACH_PATH_PLAYBACK:
        if(bOn)
        {
            InfinityDpgaSetGain(BACH_DPGA_MMC1, m_nInfinityDpgaGain[BACH_DPGA_MMC1]);
        }
        else
        {
            InfinityDpgaSetGain(BACH_DPGA_MMC1, BACH_DPGA_GAIN_MIN_DB);
        }
        break;
    case BACH_PATH_CAPTURE:
        if(bOn)
        {
            InfinityDpgaSetGain(BACH_DPGA_ADC, m_nInfinityDpgaGain[BACH_DPGA_ADC]);
            InfinityDpgaSetGain(BACH_DPGA_AEC1, m_nInfinityDpgaGain[BACH_DPGA_AEC1]);
        }
        else
        {
            InfinityDpgaSetGain(BACH_DPGA_ADC, BACH_DPGA_GAIN_MIN_DB);
            InfinityDpgaSetGain(BACH_DPGA_AEC1, BACH_DPGA_GAIN_MIN_DB);
        }
        break;
    default:
        ERRMSG("InfinitySetPathOnOff - default case!\n");
        break;

    }
}
EXPORT_SYMBOL_GPL(InfinitySetPathOnOff);

void InfinitySetPathGain(BachPath_e ePath, S8 s8Gain)
{
    switch(ePath)
    {
    case BACH_PATH_PLAYBACK:
        InfinityDpgaSetGain(BACH_DPGA_MMC1, s8Gain);
        m_nInfinityDpgaGain[BACH_DPGA_MMC1] = InfinityDpgaGetGain(BACH_DPGA_MMC1);
        break;
    case BACH_PATH_CAPTURE:

        InfinityDpgaSetGain(BACH_DPGA_ADC, s8Gain);
        m_nInfinityDpgaGain[BACH_DPGA_ADC] = InfinityDpgaGetGain(BACH_DPGA_ADC);

        InfinityDpgaSetGain(BACH_DPGA_AEC1, s8Gain);
        m_nInfinityDpgaGain[BACH_DPGA_AEC1] = InfinityDpgaGetGain(BACH_DPGA_AEC1);
        break;
    default:
        ERRMSG("InfinitySetPathGain - default case!\n");
        break;

    }
}
EXPORT_SYMBOL_GPL(InfinitySetPathGain);


void InfinitySysInit(void)
{
    U16 nConfigValue;

    InfinityAtopInit();

    InfinityWriteRegByte(0x00150200, 0x00);
    InfinityWriteRegByte(0x00150201, 0x40);
    InfinityWriteRegByte(0x00150200, 0xff);
    //InfinityWriteRegByte(0x00150201, 0x8d);
    InfinityWriteRegByte(0x00150201, 0x89);
    InfinityWriteRegByte(0x00150202, 0x88);
    InfinityWriteRegByte(0x00150203, 0xff);
    InfinityWriteRegByte(0x00150204, 0x03);
    InfinityWriteRegByte(0x00150205, 0x00);
    InfinityWriteRegByte(0x00150206, 0xB4);
    InfinityWriteRegByte(0x00150207, 0x19);
    InfinityWriteRegByte(0x00150208, 0x00);
    InfinityWriteRegByte(0x00150209, 0xf0);
    InfinityWriteRegByte(0x0015020a, 0x00);
    InfinityWriteRegByte(0x0015020b, 0x80);
    InfinityWriteRegByte(0x0015020c, 0x9a);
    InfinityWriteRegByte(0x0015020d, 0xc0);
    InfinityWriteRegByte(0x0015020e, 0x5a);
    InfinityWriteRegByte(0x0015020f, 0x55);
    InfinityWriteRegByte(0x00150212, 0x05);
    InfinityWriteRegByte(0x00150213, 0x02);
    InfinityWriteRegByte(0x00150214, 0x00);
    InfinityWriteRegByte(0x00150215, 0x00);
    InfinityWriteRegByte(0x00150216, 0x7d);
    InfinityWriteRegByte(0x00150217, 0x00);
    InfinityWriteRegByte(0x0015023a, 0x1d);
    InfinityWriteRegByte(0x0015023b, 0x02);
    InfinityWriteRegByte(0x0015023a, 0x00);
    InfinityWriteRegByte(0x0015023b, 0x00);
    InfinityWriteRegByte(0x0015031c, 0x03);
    InfinityWriteRegByte(0x0015031d, 0x00);
    InfinityWriteRegByte(0x0015032c, 0x03);
    InfinityWriteRegByte(0x0015031d, 0x00);
    InfinityWriteRegByte(0x00150226, 0x00);
    InfinityWriteRegByte(0x00150227, 0xd4);

    //correct IC default value
    InfinityWriteRegByte(0x00150248, 0x07);
    InfinityWriteRegByte(0x00150249, 0x00);
    InfinityWriteRegByte(0x00150250, 0x07);

    //digital mic settings(32kHz,4M,CLK_INV)
#ifdef DIGMIC_EN
    InfinityWriteRegByte(0x0015033a, 0x02);
    InfinityWriteRegByte(0x0015033b, 0x40);
    InfinityWriteRegByte(0x0015033c, 0x04);
    InfinityWriteRegByte(0x0015033d, 0x81);//[15] CIC selection : Digital Mic
#endif

    //set I2s pad mux
    nConfigValue = InfinityReadReg2Byte(0x101e1e);
    //nConfigValue |= REG_I2S_MODE;
#ifdef DIGMIC_EN
    nConfigValue |= (1<<8);
#endif
    InfinityWriteReg2Byte(0x101e1e, nConfigValue);

    //PM GPIO01,enable for line-out
   /* nConfigValue = InfinityReadReg2Byte(0x0f02);
    nConfigValue &= ~(1<<0);
    nConfigValue |= (1<<1);
    InfinityWriteReg2Byte(0x0f02, nConfigValue);*/

    //init dma sample rate
    m_infinitydmachannel[BACH_DMA_READER1].nSampleRate = 48000;
    m_infinitydmachannel[BACH_DMA_WRITER1].nSampleRate = 48000;

    // nConfigValue = InfinityReadReg2Byte(0x10340A);
    // AUD_PRINTF(ERROR_LEVEL, "!!!!!!!!!!!!%s: 0x10340A=0x%x\n", __FUNCTION__, nConfigValue);

}
EXPORT_SYMBOL_GPL(InfinitySysInit);


void InfinitySetMux2(BachMux2_e eMux, U8 u8Choice)
{
    switch(eMux)
    {
    case BACH_MUX2_MMC1:
        InfinityWriteReg(BACH_REG_BANK1, BACH_MUX0_SEL, REG_MMC1_SRC_SEL, (u8Choice?REG_MMC1_SRC_SEL:0));;
        break;
    case BACH_MUX2_DMAWR1:
        InfinityWriteReg(BACH_REG_BANK2, BACH_MUX3_SEL, MUX_ASRC_ADC_SEL, (u8Choice?MUX_ASRC_ADC_SEL:0));;
        break;
    default:
        ERRMSG("InfinitySetMux2 - ERROR MUX2 default case!\n");
        return;

    }
}
EXPORT_SYMBOL_GPL(InfinitySetMux2);

void InfinityAtopInit(void)
{
    int i;
    InfinityWriteRegByte(0x00103400, 0x14);
    //InfinityWriteRegByte(0x00103401, 0x02);
    InfinityWriteRegByte(0x00103401, 0x0a);//enable MSP,speed up charge VREF
    InfinityWriteRegByte(0x00103402, 0x30);
    InfinityWriteRegByte(0x00103403, 0x00);
    InfinityWriteRegByte(0x00103404, 0x80);
    InfinityWriteRegByte(0x00103405, 0x00);
    //InfinityWriteRegByte(0x00103406, 0x00);
    //InfinityWriteRegByte(0x00103407, 0x00);
    InfinityWriteRegByte(0x00103406, 0xff);
    InfinityWriteRegByte(0x00103407, 0x1f);
    InfinityWriteRegByte(0x00103408, 0x00);
    InfinityWriteRegByte(0x00103409, 0x00);
    InfinityWriteRegByte(0x0010340a, 0x77);
    InfinityWriteRegByte(0x0010340b, 0x00);
    InfinityWriteRegByte(0x0010340c, 0x33);
    InfinityWriteRegByte(0x0010340d, 0x00);
    InfinityWriteRegByte(0x0010340e, 0x00);
    InfinityWriteRegByte(0x0010340f, 0x00);
   // InfinityWriteRegByte(0x00103410, 0x14);
    InfinityWriteRegByte(0x00103410, 0x00);
    InfinityWriteRegByte(0x00103411, 0x00);
    InfinityWriteRegByte(0x00103424, 0x02);
    InfinityWriteRegByte(0x00103425, 0x00);


    //status init
    m_bADCActive = FALSE;
    m_bDACActive = FALSE;

    for(i = 0; i < BACH_ATOP_NUM; i++)
        m_bInfinityAtopStatus[i] = FALSE;
}

void InfinityAtopEnableRef(BOOL bEnable)
{
    U16 nMask;
    nMask = (REG_PD_VI | REG_PD_VREF);
    InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL03, nMask, (bEnable? 0:nMask));
}

void InfinityAtopDac(BOOL bEnable)
{
    U16 nMask;
    nMask = (REG_PD_BIAS_DAC | REG_PD_L0_DAC | REG_PD_LDO_DAC | REG_PD_R0_DAC | REG_PD_REF_DAC);
    InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL03, nMask, (bEnable? 0:nMask));
    m_bDACActive = bEnable;
	m_bInfinityAtopStatus[BACH_ATOP_LINEOUT]=bEnable;
}

void InfinityAtopAdc(BOOL bEnable)
{
    U16 nMask;
    nMask = (REG_PD_ADC0 | REG_PD_INMUX_MSK | REG_PD_LDO_ADC );
    InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL03, nMask, (bEnable? 0:((REG_PD_ADC0 | REG_PD_INMUX_MSK | REG_PD_LDO_ADC ))));
    m_bADCActive = bEnable;
}

void InfinityAtopMic(BOOL bEnable)
{
    if(bEnable)
    {
        InfinityAtopAdc(TRUE);
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL03, REG_PD_MIC_STG1_L | REG_PD_MIC_STG1_R, 0);
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL08, REG_SEL_MICGAIN_STG1_L_MSK | REG_SEL_MICGAIN_STG1_R_MSK, m_nMicGain<<REG_SEL_MICGAIN_STG1_L_POS | m_nMicGain<<REG_SEL_MICGAIN_STG1_R_POS);
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL06, REG_SEL_GAIN_INMUX0_MSK | REG_SEL_GAIN_INMUX1_MSK, m_nMicInGain<<REG_SEL_GAIN_INMUX0_POS | m_nMicInGain<<REG_SEL_GAIN_INMUX1_POS);
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL05, REG_SEL_CH_INMUX0_MSK | REG_SEL_CH_INMUX1_MSK, 0x7<<REG_SEL_CH_INMUX0_POS | 0x7<<REG_SEL_CH_INMUX1_POS);
        m_bInfinityAtopStatus[BACH_ATOP_MIC]=TRUE;

    }
    else
    {
        InfinityAtopAdc(FALSE);
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL03, REG_PD_MIC_STG1_L | REG_PD_MIC_STG1_R, REG_PD_MIC_STG1_L | REG_PD_MIC_STG1_R);
        m_bInfinityAtopStatus[BACH_ATOP_MIC]=FALSE;
    }
}

void InfinityAtopLineIn(BOOL bEnable)
{
    if(bEnable)
    {
        InfinityAtopAdc(TRUE);
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL06, REG_SEL_GAIN_INMUX0_MSK | REG_SEL_GAIN_INMUX1_MSK, m_nLineInGain<<REG_SEL_GAIN_INMUX0_POS | m_nLineInGain<<REG_SEL_GAIN_INMUX1_POS);
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL05, REG_SEL_CH_INMUX0_MSK | REG_SEL_CH_INMUX1_MSK, 0x0<<REG_SEL_CH_INMUX0_POS | 0x0<<REG_SEL_CH_INMUX1_POS);
        m_bInfinityAtopStatus[BACH_ATOP_LINEIN]=TRUE;
    }
    else
    {
        InfinityAtopAdc(FALSE);
        m_bInfinityAtopStatus[BACH_ATOP_LINEIN]=FALSE;
    }

}


BOOL InfinityOpenAtop(BachAtopPath_e ePath)
{
    if(ePath > BACH_ATOP_NUM)
        return FALSE;
    else
    {
        if(!(m_bADCActive||m_bDACActive))
            InfinityAtopEnableRef(TRUE);
        if(ePath<2)
        {
            if(ePath==BACH_ATOP_LINEIN)
            {
                if(m_bInfinityAtopStatus[BACH_ATOP_MIC])
                    return FALSE;
                else if(!m_bInfinityAtopStatus[BACH_ATOP_LINEIN])
                    InfinityAtopLineIn(TRUE);
            }
            else
            {
                if(m_bInfinityAtopStatus[BACH_ATOP_LINEIN])
                    return FALSE;
                else if(!m_bInfinityAtopStatus[BACH_ATOP_MIC])
                    InfinityAtopMic(TRUE);
            }
        }
        else
        {
            if(!m_bDACActive)
                InfinityAtopDac(TRUE);
        }
        return TRUE;
    }
}
EXPORT_SYMBOL_GPL(InfinityOpenAtop);

BOOL InfinityCloseAtop(BachAtopPath_e ePath)
{
    switch(ePath)
    {
    case BACH_ATOP_LINEIN:
        if(m_bInfinityAtopStatus[BACH_ATOP_LINEIN])
            InfinityAtopLineIn(FALSE);
        break;
    case BACH_ATOP_MIC:
        if(m_bInfinityAtopStatus[BACH_ATOP_MIC])
            InfinityAtopMic(FALSE);
        break;
    case BACH_ATOP_LINEOUT:
        if(m_bInfinityAtopStatus[BACH_ATOP_LINEOUT])
            InfinityAtopDac(FALSE);
        break;
    default:
        return FALSE;
    }

    if(!(m_bADCActive || m_bDACActive))
        InfinityAtopEnableRef(FALSE);
    return TRUE;
}
EXPORT_SYMBOL_GPL(InfinityCloseAtop);

/*BOOL InfinityAtopMicGain(U16 nLevel)
{
    if(nLevel>7)
    {
        ERRMSG("BachAtopMicGain - ERROR!! not Support.\n");
        return FALSE;
    }

	if(nLevel==2)
        m_nMicInGain = 0x0;
    else if(nLevel<2)
        m_nMicInGain = 0x1 + nLevel;
    else if(nLevel)
        m_nMicInGain = nLevel;

    if(m_bInfinityAtopStatus[BACH_ATOP_MIC])
    {
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL06, REG_SEL_GAIN_INMUX0_MSK | REG_SEL_GAIN_INMUX1_MSK, m_nMicInGain<<REG_SEL_GAIN_INMUX0_POS | m_nMicInGain<<REG_SEL_GAIN_INMUX1_POS);
    }


    return TRUE;
}*/

BOOL InfinityAtopMicPreGain(U16 nLevel)
{
    if(nLevel>3)
    {
        ERRMSG("BachAtopMicPreGain - ERROR!! not Support.\n");
        return FALSE;
    }

    m_nMicGain = nLevel;

    if(m_bInfinityAtopStatus[BACH_ATOP_MIC])
    {
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL08, REG_SEL_MICGAIN_STG1_L_MSK | REG_SEL_MICGAIN_STG1_R_MSK, m_nMicGain<<REG_SEL_MICGAIN_STG1_L_POS | m_nMicGain<<REG_SEL_MICGAIN_STG1_R_POS);
    }


    return TRUE;
}
EXPORT_SYMBOL_GPL(InfinityAtopMicPreGain);

/*BOOL InfinityAtopLineInGain(U16 nLevel)
{
    if(nLevel>7)
    {
        ERRMSG("BachAtopLineInGain - ERROR!! Level too large .\n");
        return FALSE;
    }

    if(nLevel==2)
        m_nLineInGain = 0x0;
    else if(nLevel<2)
        m_nLineInGain = 0x1 + nLevel;
    else if(nLevel)
        m_nLineInGain = nLevel;

    if(m_bInfinityAtopStatus[BACH_ATOP_LINEIN])
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL06, REG_SEL_GAIN_INMUX0_MSK | REG_SEL_GAIN_INMUX1_MSK, m_nLineInGain<<REG_SEL_GAIN_INMUX0_POS | m_nLineInGain<<REG_SEL_GAIN_INMUX1_POS);

    return TRUE;

}*/

BOOL InfinityAtopAdcGain(U16 nLevel, BachAtopPath_e eAtop)
{
	U16 nGain;
    if(nLevel>7)
    {
        ERRMSG("InfinityAtopAdcGain - ERROR!! Level too large .\n");
        return FALSE;
    }

    if(nLevel==2)
        nGain = 0x0;
    else if(nLevel<2)
        nGain = 0x1 + nLevel;
    else if(nLevel)
        nGain = nLevel;

	if(eAtop == BACH_ATOP_LINEIN)
		m_nLineInGain = nGain;
	else if(eAtop == BACH_ATOP_MIC)
		m_nMicInGain = nGain;
	else
	{
        ERRMSG("InfinityAtopAdcGain - ERROR!! ADC source error.\n");
        return FALSE;
    }

    if(m_bInfinityAtopStatus[eAtop])
        InfinityWriteReg(BACH_REG_BANK3, BACH_ANALOG_CTRL06, REG_SEL_GAIN_INMUX0_MSK | REG_SEL_GAIN_INMUX1_MSK, nGain<<REG_SEL_GAIN_INMUX0_POS | nGain<<REG_SEL_GAIN_INMUX1_POS);

    return TRUE;

}
EXPORT_SYMBOL_GPL(InfinityAtopAdcGain);


BOOL InfinityDigMicSetRate(BachRate_e eRate)
{
    U16 nConfigValue;
    nConfigValue = InfinityReadReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL0);
    if(nConfigValue & REG_DIGMIC_CLK_MODE)// 0:4M, 1:2M
    {
        switch(eRate)
        {
        case BACH_RATE_8K:
            InfinityWriteReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL0, REG_DIGMIC_SEL_MSK, 1<<REG_DIGMIC_SEL_POS);
            break;
        case BACH_RATE_16K:
            InfinityWriteReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL0, REG_DIGMIC_SEL_MSK, 2<<REG_DIGMIC_SEL_POS);
            break;
        default:
            return FALSE;
        }
    }
    else
    {
        switch(eRate)
        {
        case BACH_RATE_8K:
            InfinityWriteReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL0, REG_DIGMIC_SEL_MSK, 0<<REG_DIGMIC_SEL_POS);
            break;
        case BACH_RATE_16K:
            InfinityWriteReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL0, REG_DIGMIC_SEL_MSK, 1<<REG_DIGMIC_SEL_POS);
            break;
        case BACH_RATE_32K:
            InfinityWriteReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL0, REG_DIGMIC_SEL_MSK, 2<<REG_DIGMIC_SEL_POS);
            break;
        default:
            return FALSE;
        }
    }
    return TRUE;

}


BOOL InfinityDigMicEnable(BOOL bEn)
{
    U16 nConfigValue;
    nConfigValue = InfinityReadReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL1);
    if(nConfigValue & REG_CIC_SEL)
    {
        InfinityWriteReg(BACH_REG_BANK2, BACH_DIG_MIC_CTRL0, REG_DIGMIC_EN, (bEn ? REG_DIGMIC_EN :0) );
        return TRUE;
    }
    else
        return FALSE;
}
EXPORT_SYMBOL_GPL(InfinityDigMicEnable);

//step:-6dB
BOOL InfinitySineGenGain(U16 nGain)
{
    if(nGain<=4)
    {
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA_TEST_CTRL5, REG_SINE_GEN_GAIN_MSK, nGain<<REG_SINE_GEN_GAIN_POS );
        return TRUE;
    }
    else
        return FALSE;
}
EXPORT_SYMBOL_GPL(InfinitySineGenGain);

BOOL InfinitySineGenRate(U16 nRate)
{
    if(nRate<BACH_SINERATE_NUM)
    {
        InfinityWriteReg(BACH_REG_BANK1, BACH_DMA_TEST_CTRL5, REG_SINE_GEN_FREQ_MSK, nRate<<REG_SINE_GEN_FREQ_POS );
        return TRUE;
    }
    else
        return FALSE;
}
EXPORT_SYMBOL_GPL(InfinitySineGenRate);

void InfinitySineGenEnable(BOOL bEnable)
{
    U16 nConfigValue;
    nConfigValue = (bEnable? REG_SINE_GEN_EN | REG_SINE_GEN_L | REG_SINE_GEN_R : 0);
    InfinityWriteReg(BACH_REG_BANK1, BACH_DMA_TEST_CTRL5, REG_SINE_GEN_EN | REG_SINE_GEN_L | REG_SINE_GEN_R ,nConfigValue);
}
EXPORT_SYMBOL_GPL(InfinitySineGenEnable);


MODULE_LICENSE("GPL");


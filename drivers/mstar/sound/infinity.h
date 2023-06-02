// $Change: 548282 $
//
// infinity.h
//
// defines for the registers in the iNfinity3 BACH chip
//
//
#ifndef _INFINITY_H
#define _INFINITY_H

#include "infinity_reg.h"
#if defined(__linux__)
#include <linux/types.h>
#include <linux/kernel.h>
typedef unsigned int    U32;
typedef int             S32;
typedef bool            BOOL;

#define TRUE 1
#define FALSE 0

//#define ERRMSG printk
#define ERRMSG(fmt, arg...)		printk(KERN_ERR "<Audio Error>" fmt, ##arg);
#define TRACE(fmt, arg...)		printk(KERN_DEBUG "<Audio Trace>" fmt, ##arg);
#else
#include <wtypes.h>
#include <ceddk.h>

typedef unsigned long    U32;
typedef long             S32;

#endif

typedef unsigned char   U8;
typedef signed char     S8;
typedef unsigned short  U16;
typedef short           S16;


#define TYPE_CAST(OldType, NewType, Var)    ((NewType)(Var))

#define BACH_RIU_BASE_ADDR        0x1f000000
#define BACH_REG_BANK_1           0x150200//0x112a00
#define BACH_REG_BANK_2           0x150300//0x112b00
#define BACH_REG_BANK_3           0x103400//0x112c00

#define MIU0_OFFSET 0x20000000

#define MIU_WORD_BYTE_SIZE          (8)
#define BACH_ARM_CACHE_LINE_ALIGN   (32)

/* DMA Channel State defines */
#define DMA_UNINIT          0
#define DMA_INIT            1
#define DMA_STOPPED         2
#define DMA_RUNNING         3

#define BACH_DPGA_GAIN_MAX_DB 30
#define BACH_DPGA_GAIN_MIN_DB -64 //actually -63.5 dB
#define BACH_DPGA_GAIN_MIN_IDX 0x7F

#define WRITE_BYTE(_reg, _val)      (*((volatile U8*)(_reg)))  = (U8)(_val)
#define WRITE_WORD(_reg, _val)      (*((volatile U16*)(_reg))) = (U16)(_val)
#define WRITE_LONG(_reg, _val)      (*((volatile U32*)(_reg))) = (U32)(_val)
#define READ_BYTE(_reg)             (*(volatile U8*)(_reg))
#define READ_WORD(_reg)             (*(volatile U16*)(_reg))
#define READ_LONG(_reg)             (*(volatile U32*)(_reg))

typedef struct DMAChannelTag
{
    U32 nDMAChannelState;

    // system things
    U32 nPhysDMAAddr;         // physical RAM address of the buffer
    U32 nBufferSize;          // lenght of contiguous physical RAM

    // internal things
    U32 nChannels;            // number of channels (1 or 2)
    U32 nSampleSize;          // sample word size
    U32 nSampleRate;          // sample rate in samples/second
    U32 nBytesPerInt;       // number of samples to play before interrupting

} DMACHANNEL;


/**
 * \brief Audio DMA
 */
typedef enum
{
    BACH_DMA_WRITER1 = 0,
   // BACH_DMA_WRITER2,
    BACH_DMA_READER1,
  //  BACH_DMA_READER2,
    BACH_DMA_NUM,
    BACH_DMA_NULL = 0xff,

} BachDmaChannel_e;

typedef enum
{//
    BACH_DMA_INT_UNDERRUN = 0,
    BACH_DMA_INT_OVERRUN,
    BACH_DMA_INT_EMPTY,
    BACH_DMA_INT_FULL,
    BACH_DMA_INT_NUM

} BachDmaInterrupt_e;

typedef enum
{
    BACH_REG_BANK1,
    BACH_REG_BANK2,
    BACH_REG_BANK3,
  //  BACH_REG_BANK4
} BachRegBank_e;

typedef enum
{
    BACH_RATE_SLAVE,
    BACH_RATE_8K,
    BACH_RATE_11K,
    BACH_RATE_12K,
    BACH_RATE_16K,
    BACH_RATE_22K,
    BACH_RATE_24K,
    BACH_RATE_32K,
    BACH_RATE_44K,
    BACH_RATE_48K,
    BACH_RATE_NUM,
    BACH_RATE_NULL = 0xff,
} BachRate_e;

typedef enum
{
    BACH_DPGA_MMC1,
    BACH_DPGA_ADC,
    BACH_DPGA_AEC1,
    BACH_DPGA_DEC1,
    BACH_DPGA_NUM,
    BACH_DPGA_NULL = 0xff,
} BachDpga_e;

typedef enum
{
    BACH_PATH_PLAYBACK,
    BACH_PATH_CAPTURE,
    BACH_PATH_NUM
} BachPath_e;

typedef enum
{
    BACH_MUX2_MMC1,
    BACH_MUX2_DMAWR1,
    BACH_MUX2_NULL = 0xff
} BachMux2_e;

typedef enum
{
    BACH_ATOP_LINEIN,
    BACH_ATOP_MIC,
    BACH_ATOP_LINEOUT,
    BACH_ATOP_NUM
} BachAtopPath_e;

typedef enum
{
    BACH_SINERATE_250,
    BACH_SINERATE_500,
    BACH_SINERATE_1000,
    BACH_SINERATE_1500,
    BACH_SINERATE_2K,
    BACH_SINERATE_3K,
    BACH_SINERATE_4K,
    BACH_SINERATE_6K,
    BACH_SINERATE_8K,
    BACH_SINERATE_12K,
    BACH_SINERATE_16K,
    BACH_SINERATE_NUM
} BachSineRate_e;


#ifdef __cplusplus
extern "C" {
#endif


    U16 InfinityGetMaskReg(BachRegBank_e nBank, U8 nAddr);
    // DMA
    void InfinityDmaReset(void);
    void InfinityDmaInitChannel( U32 nChannelIndex,
                             U32 nPhysDMAAddr,
                             U32 nBufferSize,
                             U32 nChannels,
                             U32 nSampleSize,
                             U32 nSampleRate,
                             U32 nOverrunTh,
                             U32 nUnderrunTh
                           );

    U32  InfinityDmaGetLevelCnt(BachDmaChannel_e eDmaChannel);
    BOOL InfinityDmaMaskInt(BachDmaChannel_e eDmaChan, BachDmaInterrupt_e eDmaInt, BOOL bEnable);
    void InfinityDmaClearInt(BachDmaChannel_e eDmaChannel);
    void InfinityDmaReInit(BachDmaChannel_e eDmaChannel);
    void InfinityDmaEnable(BachDmaChannel_e eDmaChannel, BOOL bEnable);
    void InfinityDmaStartChannel(BachDmaChannel_e eDmaChannel);
    void InfinityDmaStopChannel(BachDmaChannel_e eDmaChannel);
    void InfinityDmaSetThreshold(BachDmaChannel_e eDmaChannel, U32 nOverrunTh, U32 nUnderrunTh);
    void InfinityDmaSetPhyAddr(BachDmaChannel_e eDmaChannel, U32 nBufAddr, U32 nBufSize);
    void InfinityDmaSetChMode(BachDmaChannel_e eDma, BOOL bMono);

    BOOL InfinityDmaSetRate(BachDmaChannel_e eDmaChannel, BachRate_e eRate);
    U32 InfinityDmaGetRate(BachDmaChannel_e eDmaChannel);

    BOOL InfinityDmaIsFull(BachDmaChannel_e eDmaChannel);
    BOOL InfinityDmaIsEmpty(BachDmaChannel_e eDmaChannel);
    BOOL InfinityDmaIsLocalEmpty(BachDmaChannel_e eDmaChannel);
    BOOL InfinityDmaIsUnderrun(BachDmaChannel_e eDmaChannel);
    BOOL InfinityDmaIsOverrun(BachDmaChannel_e eDmaChannel);
    U32  InfinityDmaTrigLevelCnt(BachDmaChannel_e eDmaChannel, U32 nDataSize);
    BOOL InfinityDmaIsWork(BachDmaChannel_e eDmaChannel);

    //DIGMIC
    BOOL InfinityDigMicSetRate(BachRate_e eRate);
    BOOL InfinityDigMicEnable(BOOL bEn);

    //DPGA
    void InfinityDpgaCtrl(BachDpga_e eDpga, BOOL bEnable, BOOL bMute, BOOL bFade);
    void InfinityDpgaCalGain(S8 s8Gain, U8 *pu8GainIdx);
    S8 InfinityDpgaGetGain(BachDpga_e eDpga);
    void InfinityDpgaSetGain(BachDpga_e eDpga, S8 s8Gain);
    void InfinitySetPathOnOff(BachPath_e ePath, BOOL bOn);
    void InfinitySetPathGain(BachPath_e ePath, S8 s8Gain);

    //ATOP
    void InfinityAtopInit(void);
    void InfinityAtopEnableRef(BOOL bEnable);
    void InfinityAtopDac(BOOL bEnable);
    void InfinityAtopAdc(BOOL bEnable);
    void InfinityAtopMic(BOOL bEnable);
    void InfinityAtopLineIn(BOOL bEnable);
    BOOL InfinityOpenAtop(BachAtopPath_e ePath);
    BOOL InfinityCloseAtop(BachAtopPath_e ePath);
   // BOOL InfinityAtopMicGain(U16 nLevel);
    BOOL InfinityAtopMicPreGain(U16 nLevel);
   // BOOL InfinityAtopLineInGain(U16 nLevel);
	BOOL InfinityAtopAdcGain(U16 nLevel, BachAtopPath_e eAtop);

    //sinegen
    BOOL InfinitySineGenGain(U16 nGain);
    BOOL InfinitySineGenRate(U16 nRate);
    void InfinitySineGenEnable(BOOL bEnable);

    void InfinitySetMux2(BachMux2_e eMux, U8 u8Choice);

    void InfinitySysInit(void);

    void InfinitySetBankBaseAddr(U32 addr);
    void InfinitySetBank1BaseAddr(U32 addr);
    void InfinitySetBank2BaseAddr(U32 addr);
    void InfinitySetBank3BaseAddr(U32 addr);

    BachRate_e InfinityRateFromU32(U32 nRate);
    U32 InfinityRateToU32(BachRate_e eRate);

    U16 InfinityReadReg(BachRegBank_e nBank, U8 nAddr);
    void InfinityWriteReg(BachRegBank_e nBank, U8 nAddr, U16 regMsk, U16 nValue);
    void InfinityWriteReg2Byte(U32 nAddr, U16 nValue);
    void InfinityWriteRegByte(U32 nAddr, U8 nValue);
    U16 InfinityReadReg2Byte(U32 nAddr);
    U8 InfinityReadRegByte(U32 nAddr);



#ifdef __cplusplus
}
#endif


#endif

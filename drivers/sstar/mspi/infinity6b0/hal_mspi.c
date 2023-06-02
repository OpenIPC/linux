/*
* hal_mspi.c- Sigmastar
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

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/io.h>
#include "gpio.h"
#include <hal_movedma.h>
#include <registers.h>
#include <ms_platform.h>
#include <cam_os_wrapper.h>

#include "hal_mspi.h"
#include "hal_mspireg.h"

#if defined(CONFIG_MS_PADMUX)
#include "mdrv_padmux.h"
#endif

#define SSTAR_MSPI_HAL_MUX                  0

#define mspi_dbg                            1
#if mspi_dbg == 1
    #define mspi_dbgmsg(args...)            printk("[MSPI] : " args)
#else
    #define mspi_dbgmsg(args...)            do{}while(0)
#endif
#define mspi_errmsg(fmt, ...)               printk(KERN_ERR "[MSPI] error : " fmt, ##__VA_ARGS__)
#define mspi_warnmsg(fmt, ...)              printk(KERN_ERR "[MSPI] warning : " fmt, ##__VA_ARGS__)

#define READ_WORD(_reg)                     (*(volatile u16*)(_reg))
#define WRITE_WORD(_reg, _val)              (*((volatile u16*)(_reg))) = (u16)(_val)

#define MSPI_READ(_reg_)                    READ_WORD(mspi->mspi_base + ((_reg_)<<2))
#define MSPI_WRITE(_reg_, _val_)            WRITE_WORD(mspi->mspi_base + ((_reg_)<<2), (_val_))
#define MSPI_WRITE_MASK(_reg_, _val_, mask) WRITE_WORD_MASK(mspi->mspi_base + ((_reg_)<<2), (_val_), (mask))

#define MSPI_READ_INDEX                     0x0
#define MSPI_WRITE_INDEX                    0x1

#define MSTAR_SPI_TIMEOUT_MS                30000

#define MSPI_MIU0_BUS_BASE                  ARM_MIU0_BUS_BASE
#define MSPI_MIU1_BUS_BASE                  ARM_MIU1_BUS_BASE

#define MSPI_DMA_MODE_MAX                   1

typedef enum
{
     E_MSPI_OK = 0,
     E_MSPI_INIT_FLOW_ERROR,
     E_MSPI_DCCONFIG_ERROR,
     E_MSPI_CLKCONFIG_ERROR,
     E_MSPI_FRAMECONFIG_ERROR,
     E_MSPI_OPERATION_ERROR,
     E_MSPI_PARAM_OVERFLOW,
     E_MSPI_MMIO_ERROR,
     E_MSPI_TIMEOUT,
     E_MSPI_HW_NOT_SUPPORT,
     E_MSPI_NOMEM,
     E_MSPI_NULL,
     E_MSPI_ERR,
} MSPI_ErrorNo;

typedef enum
{
    E_MSPI_OFF = 0,
    E_MSPI_ON,
} MSPI_CsState;

typedef enum
{
    E_MSPI_CS0 = 0,
    E_MSPI_CS1,
    E_MSPI_CS2,
} MSPI_CsChannel;

typedef struct
{
    u8 u8TrStart;      //time from trigger to first SPI clock
    u8 u8TrEnd;        //time from last SPI clock to transferred done
    u8 u8TB;           //time between byte to byte transfer
    u8 u8TRW;          //time between last write and first read
} MSPI_DCConfig;

typedef enum _HAL_DC_Config
{
    E_MSPI_TRSTART,
    E_MSPI_TREND,
    E_MSPI_TB,
    E_MSPI_TRW
}eDC_config;

typedef enum _HAL_CLK_Config
{
    E_MSPI_POL,
    E_MSPI_PHA,
    E_MSPI_CLK
}eCLK_config;

typedef struct
{
    u8 u8WBitConfig[8];      //bits will be transferred in write buffer
    u8 u8RBitConfig[8];      //bits Will be transferred in read buffer
} MSPI_FrameConfig;

static bool            gbInitFlag = false;

#if SSTAR_MSPI_HAL_MUX
static CamOsMutex_t    hal_mspi_lock;
#define HAL_MSPI_Lock()       CamOsMutexLock(&hal_mspi_lock);
#define HAL_MSPI_Unlock()     CamOsMutexUnlock(&hal_mspi_lock);
#else
#define HAL_MSPI_Lock()
#define HAL_MSPI_Unlock()
#endif

static void HAL_MSPI_Enable(struct mspi_hal *mspi, bool bEnable)
{
    HAL_MSPI_Lock();
    if(bEnable) {
        MSPI_WRITE(MSPI_CTRL_OFFSET,MSPI_READ(MSPI_CTRL_OFFSET)|MSPI_INT_ENABLE);
    } else {
        MSPI_WRITE(MSPI_CTRL_OFFSET,MSPI_READ(MSPI_CTRL_OFFSET)&(~MSPI_INT_ENABLE));
    }
    HAL_MSPI_Unlock();
}

void HAL_MSPI_select_pad( int pad_ctrl, u8 padmux, u8 pad_mode)
{
    switch(padmux)
    {
        case MUX_SPI0:
            OUTREGMSK16(BASE_REG_CHIPTOP_PA+(MSPI0_MODE<<2), pad_mode << 0, 0x7 << 0);  //reg_spi0_mode[2:0]
            break;
        case MUX_SPI1:
            OUTREGMSK16(BASE_REG_CHIPTOP_PA+(MSPI1_MODE<<2), pad_mode << 4, 0x7 << 4);  //reg_spi1_mode[6:4]
            break;

        default:
            break;
    }

    if(pad_ctrl==PAD_SPI0_CZ)
        OUTREGMSK16(BASE_REG_CHIPTOP_PA+(0x0F<<2),  0, 0x02 << 0); //Disable jtag mode   // IO PAD conflict turn off jtag
}

int HAL_MSPI_get_padmux(int pad_ctrl, u8 *padmux, u8 *pad_mode)
{
    int ret = 0;

    switch(pad_ctrl)
    {
        case PAD_SPI0_CZ:
            *padmux=MUX_SPI0;
            *pad_mode=0x1;
            break;

        case PAD_SPI1_CZ:
            *padmux=MUX_SPI1;
            *pad_mode=0x1;
            break;

        case PAD_GPIO0:
            *padmux=MUX_SPI1;
            *pad_mode=0x4;
            break;

        default:
            ret = -1;
            break;
    }

    return ret;
}

static void HAL_MSPI_PadSet(struct mspi_hal *mspi)
{
    u8 padmux;
    u8 pad_mode;

    HAL_MSPI_get_padmux(mspi->pad_ctrl,&padmux,&pad_mode);
    HAL_MSPI_select_pad(mspi->pad_ctrl,padmux,pad_mode);
}

static void HAL_MSPI_Init(struct mspi_hal *mspi)
{
    HAL_MSPI_Lock();
    MSPI_WRITE(MSPI_CTRL_OFFSET,MSPI_READ(MSPI_CTRL_OFFSET)|(MSPI_RESET|MSPI_ENABLE));

#if defined(CONFIG_MS_PADMUX)
    if (0 == mdrv_padmux_active())
#endif
    {
        HAL_MSPI_PadSet(mspi);
    }

    HAL_MSPI_Unlock();
    HAL_MSPI_Enable(mspi,true);
    gbInitFlag = true;
}

static void HAL_MSPI_Reset_DCConfig(struct mspi_hal *mspi)
{
    HAL_MSPI_Lock();
    //DC reset
    MSPI_WRITE(MSPI_DC_TR_START_OFFSET, 0x00);
    MSPI_WRITE(MSPI_DC_TB_OFFSET, 0x00);
    HAL_MSPI_Unlock();
}

static u8 HAL_MSPI_DCConfigMax(eDC_config eDCField)
{
    switch(eDCField)
    {
        case E_MSPI_TRSTART:
            return MSPI_DC_TRSTART_MAX;
        case E_MSPI_TREND:
            return MSPI_DC_TREND_MAX;
        case E_MSPI_TB:
            return MSPI_DC_TB_MAX;
        case E_MSPI_TRW:
            return MSPI_DC_TRW_MAX;
        default:
            return 0;
    }
}

void HAL_MSPI_SetDcTiming (struct mspi_hal *mspi, eDC_config eDCField, u8 u8DCtiming)
{
    u16 u16TempBuf = 0;
    HAL_MSPI_Lock();
    switch(eDCField)
    {
        case E_MSPI_TRSTART:
            u16TempBuf = MSPI_READ(MSPI_DC_TR_START_OFFSET);
            u16TempBuf &= (~MSPI_DC_MASK);
            u16TempBuf |= u8DCtiming;
            MSPI_WRITE(MSPI_DC_TR_START_OFFSET, u16TempBuf);
            break;
        case E_MSPI_TREND:
            u16TempBuf = MSPI_READ(MSPI_DC_TR_END_OFFSET);
            u16TempBuf &= MSPI_DC_MASK;
            u16TempBuf |= u8DCtiming << MSPI_DC_BIT_OFFSET;
            MSPI_WRITE(MSPI_DC_TR_END_OFFSET, u16TempBuf);
            break;
        case E_MSPI_TB:
            u16TempBuf = MSPI_READ(MSPI_DC_TB_OFFSET);
            u16TempBuf &= (~MSPI_DC_MASK);
            u16TempBuf |= u8DCtiming;
            MSPI_WRITE(MSPI_DC_TB_OFFSET, u16TempBuf);
            break;
        case E_MSPI_TRW:
            u16TempBuf = MSPI_READ(MSPI_DC_TRW_OFFSET);
            u16TempBuf &= MSPI_DC_MASK;
            u16TempBuf |= u8DCtiming << MSPI_DC_BIT_OFFSET;
            MSPI_WRITE(MSPI_DC_TRW_OFFSET, u16TempBuf);
            break;
    }

    HAL_MSPI_Unlock();
}


static u8 HAL_DC_TrStartSetting(struct mspi_hal *mspi, u8 TrStart)
{
    u8 u8TrStartMax;
    u8 errnum = E_MSPI_OK;

    u8TrStartMax = HAL_MSPI_DCConfigMax(E_MSPI_TRSTART);
    if(TrStart > u8TrStartMax)
        errnum = E_MSPI_PARAM_OVERFLOW;
    else
        HAL_MSPI_SetDcTiming(mspi,E_MSPI_TRSTART ,TrStart);
    return errnum;
}



static u8 HAL_DC_TrEndSetting(struct mspi_hal *mspi,u8 TrEnd)
{
    u8 u8TrEndMax;
    u8 errnum = E_MSPI_OK;

    u8TrEndMax = HAL_MSPI_DCConfigMax(E_MSPI_TREND);
    if(TrEnd > u8TrEndMax)
        errnum = E_MSPI_PARAM_OVERFLOW;
    else
        HAL_MSPI_SetDcTiming(mspi,E_MSPI_TREND ,TrEnd);
    return errnum;
}

static u8 HAL_DC_TBSetting(struct mspi_hal *mspi, u8 TB)
{
    u8 u8TBMax;
    u8 errnum = E_MSPI_OK;

    u8TBMax = HAL_MSPI_DCConfigMax(E_MSPI_TB);
    if(TB > u8TBMax)
        errnum = E_MSPI_PARAM_OVERFLOW;
    else
        HAL_MSPI_SetDcTiming(mspi,E_MSPI_TB ,TB);
    return errnum;
}

static u8 HAL_DC_TRWSetting(struct mspi_hal *mspi, u8 TRW)
{
    u8 u8TRWMax;
    u8 errnum = E_MSPI_OK;

    u8TRWMax = HAL_MSPI_DCConfigMax(E_MSPI_TRW);
    if(TRW > u8TRWMax)
        errnum = E_MSPI_PARAM_OVERFLOW;
    else
        HAL_MSPI_SetDcTiming(mspi,E_MSPI_TRW ,TRW);
    return errnum;
}

static u8 HAL_MSPI_DCConfig(struct mspi_hal *mspi, MSPI_DCConfig *ptDCConfig)
{
    u8 errnum = E_MSPI_OK;

    //check init
    if(!gbInitFlag)
        return E_MSPI_ERR;

    if(ptDCConfig == NULL)
    {
        HAL_MSPI_Reset_DCConfig(mspi);
        return E_MSPI_OK;
    }
    errnum = HAL_DC_TrStartSetting(mspi,ptDCConfig->u8TrStart);
    if(errnum != E_MSPI_OK)
        goto ERROR_HANDLE;
    errnum = HAL_DC_TrEndSetting(mspi,ptDCConfig->u8TrEnd);
    if(errnum != E_MSPI_OK)
        goto ERROR_HANDLE;
    errnum = HAL_DC_TBSetting(mspi,ptDCConfig->u8TB);
    if(errnum != E_MSPI_OK)
        goto ERROR_HANDLE;
    errnum = HAL_DC_TRWSetting(mspi,ptDCConfig->u8TRW);
    if(errnum != E_MSPI_OK)
        goto ERROR_HANDLE;
    return E_MSPI_OK;

ERROR_HANDLE:
    errnum |= E_MSPI_DCCONFIG_ERROR;
    return errnum;
}

static void HAL_MSPI_SetCLKTiming(struct mspi_hal *mspi, eCLK_config eCLKField, u8 u8CLKVal)
{
    u16 u16TempBuf = 0;
    HAL_MSPI_Lock();
    switch(eCLKField)
    {
        case E_MSPI_POL:
            u16TempBuf = MSPI_READ(MSPI_CLK_CLOCK_OFFSET);
            u16TempBuf &= ~(MSPI_CLK_POLARITY_MASK);
            u16TempBuf |= u8CLKVal << MSPI_CLK_POLARITY_BIT_OFFSET;
            break;
        case E_MSPI_PHA:
            u16TempBuf = MSPI_READ(MSPI_CLK_CLOCK_OFFSET);
            u16TempBuf &= ~(MSPI_CLK_PHASE_MASK);
            u16TempBuf |= u8CLKVal << MSPI_CLK_PHASE_BIT_OFFSET;
            break;
        case E_MSPI_CLK:
            u16TempBuf = MSPI_READ(MSPI_CLK_CLOCK_OFFSET);
            u16TempBuf &= MSPI_CLK_CLOCK_MASK;
            u16TempBuf |= u8CLKVal << MSPI_CLK_CLOCK_BIT_OFFSET;
            break;
    }
    MSPI_WRITE(MSPI_CLK_CLOCK_OFFSET, u16TempBuf);

    HAL_MSPI_Unlock();
}

static void HAL_MSPI_Reset_CLKConfig(struct mspi_hal *mspi)
{
    u16 Tempbuf;
    HAL_MSPI_Lock();
    //reset clock
    Tempbuf = MSPI_READ(MSPI_CTRL_OFFSET);
    Tempbuf &= 0x3F;
    MSPI_WRITE(MSPI_CTRL_OFFSET, Tempbuf);
    HAL_MSPI_Unlock();
}

u8  HAL_MSPI_SetMode(struct mspi_hal *mspi, MSPI_Mode_Config_e eMode)
{
    if (eMode >= E_MSPI_MODE_MAX)
    {
        return E_MSPI_PARAM_OVERFLOW;
    }

    switch (eMode)
    {
        case E_MSPI_MODE0:
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_POL ,false);
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_PHA ,false);

            break;
        case E_MSPI_MODE1:
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_POL ,false);
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_PHA ,true);
            break;
        case E_MSPI_MODE2:
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_POL ,true);
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_PHA ,false);
            break;
        case E_MSPI_MODE3:
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_POL ,true);
            HAL_MSPI_SetCLKTiming(mspi, E_MSPI_PHA ,true);
            break;
        default:
            HAL_MSPI_Reset_CLKConfig(mspi);
            return E_MSPI_OPERATION_ERROR;
    }

    return E_MSPI_OK;
}

static void HAL_MSPI_Reset_FrameConfig(struct mspi_hal *mspi)
{
    HAL_MSPI_Lock();
    // Frame reset
    MSPI_WRITE(MSPI_FRAME_WBIT_OFFSET, 0xFFF);
    MSPI_WRITE(MSPI_FRAME_WBIT_OFFSET+1, 0xFFF);
    MSPI_WRITE(MSPI_FRAME_RBIT_OFFSET, 0xFFF);
    MSPI_WRITE(MSPI_FRAME_RBIT_OFFSET+1, 0xFFF);
    HAL_MSPI_Unlock();
}

static void HAL_MSPI_SetPerFrameSize(struct mspi_hal *mspi, u8 bDirect, u8 u8BufOffset, u8 u8PerFrameSize)
{
    u8 u8Index = 0;
    u16 u16TempBuf = 0;
    u8 u8BitOffset = 0;
    u16 u16regIndex = 0;

    HAL_MSPI_Lock();
    if(bDirect == MSPI_READ_INDEX)
    {
        u16regIndex = MSPI_FRAME_RBIT_OFFSET;
    }
    else
    {
        u16regIndex = MSPI_FRAME_WBIT_OFFSET;
    }
    if(u8BufOffset >=4) {
        u8Index++;
        u8BufOffset -= 4;
    }
    u8BitOffset = u8BufOffset * MSPI_FRAME_BIT_FIELD;
    u16TempBuf = MSPI_READ(u16regIndex+ u8Index);
    u16TempBuf &= ~(MSPI_FRAME_BIT_MASK << u8BitOffset);
    u16TempBuf |= u8PerFrameSize << u8BitOffset;
    MSPI_WRITE((u16regIndex + u8Index), u16TempBuf);
    HAL_MSPI_Unlock();
}

static u8 HAL_MSPI_FRAMEConfig(struct mspi_hal *mspi, MSPI_FrameConfig  *ptFrameConfig)
{
    u8 errnum = E_MSPI_OK;
    u8 u8Index = 0;

    if(ptFrameConfig == NULL)
    {
        HAL_MSPI_Reset_FrameConfig(mspi);
        return E_MSPI_OK;
    }
    // read buffer bit config
    for(u8Index = 0; u8Index < MAX_READ_BUF_SIZE; u8Index++)
    {
        if(ptFrameConfig->u8RBitConfig[u8Index] > MSPI_FRAME_BIT_MAX)
        {
            errnum = E_MSPI_PARAM_OVERFLOW;
        }
        else
        {
            HAL_MSPI_SetPerFrameSize(mspi, MSPI_READ_INDEX,  u8Index, ptFrameConfig->u8RBitConfig[u8Index]);
        }
    }
    //write buffer bit config
    for(u8Index = 0; u8Index < MAX_WRITE_BUF_SIZE; u8Index++)
    {
        if(ptFrameConfig->u8WBitConfig[u8Index] > MSPI_FRAME_BIT_MAX)
        {
            errnum = E_MSPI_PARAM_OVERFLOW;
        }
        else
        {
            HAL_MSPI_SetPerFrameSize(mspi, MSPI_WRITE_INDEX,  u8Index, ptFrameConfig->u8WBitConfig[u8Index]);
        }
    }
    return errnum;
}

void HAL_MSPI_ChipSelect(struct mspi_hal *mspi, u8 Enable, u8 eSelect)
{
    u16 regdata = 0;
    u8 bitmask = 0;
    HAL_MSPI_Lock();
    regdata = MSPI_READ(MSPI_CHIP_SELECT_OFFSET);
    if(Enable)
    {
        bitmask = ~(1 << eSelect);
        regdata &= bitmask;
    }
    else
    {
        bitmask = (1 << eSelect);
        regdata |= bitmask;
    }
    MSPI_WRITE(MSPI_CHIP_SELECT_OFFSET, regdata);
    HAL_MSPI_Unlock();
}

u8 HAL_MSPI_SetLSB(struct mspi_hal *mspi, u8 enable)
{
    HAL_MSPI_Lock();

    MSPI_WRITE(MSPI_LSB_FIRST_OFFSET, enable);

    HAL_MSPI_Unlock();
    return E_MSPI_OK;
}

u8 HAL_MSPI_Config(struct mspi_hal *mspi)
{
    u8 errnum = E_MSPI_OK;

    MSPI_DCConfig stDCConfig;
    MSPI_FrameConfig stFrameConfig;
    MSPI_Mode_Config_e mspimode = E_MSPI_MODE0;

    stDCConfig.u8TB = 0;
    stDCConfig.u8TrEnd = 0x0;
    stDCConfig.u8TrStart = 0x0;
    stDCConfig.u8TRW = 0;

#if SSTAR_MSPI_HAL_MUX
    CamOsMutexInit(&hal_mspi_lock);
#endif

    memset(&stFrameConfig,0x07,sizeof(MSPI_FrameConfig));
    HAL_MSPI_Init(mspi);

    errnum = HAL_MSPI_DCConfig(mspi, &stDCConfig);
    if(errnum)
    {
        return errnum;
    }

    errnum = HAL_MSPI_SetMode(mspi, mspimode);
    if(errnum)
    {
        return errnum;
    }

    errnum = HAL_MSPI_FRAMEConfig(mspi, &stFrameConfig);
    if(errnum)
    {
        return errnum;
    }
    //MDrv_MSPI_SetCLK(bs,u8Channel,54000000); //200000 CLK
    HAL_MSPI_ChipSelect(mspi,E_MSPI_OFF,E_MSPI_CS0);
    errnum = HAL_MSPI_SetLSB(mspi, 0);
    if(errnum)
    {
        return errnum;
    }
    return errnum;
}

static void HAL_MSPI_RWBUFSize(struct mspi_hal *mspi,u8 Direct, u8 Size)
{
    u16 u16Data = 0;
    u16Data = MSPI_READ(MSPI_RBF_SIZE_OFFSET);
    //printk("===RWBUFSize:%d\n",Size);
    if(Direct == MSPI_READ_INDEX)
    {
        u16Data &= MSPI_RWSIZE_MASK;
        u16Data |= Size << MSPI_RSIZE_BIT_OFFSET;
    }
    else
    {
        u16Data &= ~MSPI_RWSIZE_MASK;
        u16Data |= Size;
    }
    MSPI_WRITE(MSPI_RBF_SIZE_OFFSET, u16Data);
}

#ifndef CONFIG_SPI_INT_CALL
u8 HAL_MSPI_Trigger(struct mspi_hal *mspi)
{
    unsigned int timeout;
    // trigger operation
    reinit_completion(&mspi->done);
    MSPI_WRITE(MSPI_TRIGGER_OFFSET,MSPI_TRIGGER);
    timeout = wait_for_completion_timeout(&mspi->done,
    msecs_to_jiffies(MSTAR_SPI_TIMEOUT_MS));

    MSPI_WRITE(MSPI_RBF_SIZE_OFFSET,0x0);

    if (!timeout)
    {
        mspi_errmsg("mspi timeout\n");
        return E_MSPI_TIMEOUT;
    }

    return E_MSPI_OK;
}
#else
u8 HAL_MSPI_Trigger(struct mspi_hal *mspi)
{
    unsigned int timeout = 30000000;
    u16 done = 0;
    MSPI_WRITE(MSPI_TRIGGER_OFFSET,MSPI_TRIGGER);

    while(timeout != 0 && !done)
    {
        done = MSPI_READ(MSPI_DONE_OFFSET);
        //udelay(1);
        timeout--;
    }

    MSPI_WRITE(MSPI_RBF_SIZE_OFFSET,0x0);
    MSPI_WRITE(MSPI_DONE_CLEAR_OFFSET,MSPI_CLEAR_DONE);

    if(!timeout)
    {
        printk("mspi timeout\n");
        return E_MSPI_TIMEOUT;
    }

    return E_MSPI_OK;
}
#endif
static u8 HAL_MSPI_FullDuplexBuf(struct mspi_hal *mspi, u8 *rx_buff, u8 *tx_buff, u16 u16Size)
{
    u8  u8Index = 0;
    u16 u16TempBuf = 0;
    u8 errnum = E_MSPI_OK;

#ifndef CONFIG_SPI_INT_CALL
    HAL_MSPI_Lock();
#endif

    for(u8Index = 0; u8Index < u16Size; u8Index++)
    {
        if(u8Index & 1)
        {
            u16TempBuf = (tx_buff[u8Index] << 8) | tx_buff[u8Index-1];
            MSPI_WRITE((MSPI_WRITE_BUF_OFFSET + (u8Index >> 1)),u16TempBuf);
        }
        else if(u8Index == (u16Size -1))
        {
            MSPI_WRITE((MSPI_WRITE_BUF_OFFSET + (u8Index >> 1)),tx_buff[u8Index]);
        }
    }

    HAL_MSPI_RWBUFSize(mspi,MSPI_WRITE_INDEX, u16Size);
    errnum = HAL_MSPI_Trigger(mspi);
    if(errnum)
    {
        goto err_out;
    }

    for(u8Index = 0; u8Index < u16Size; u8Index++)
    {
        if(u8Index & 1)
        {
            u16TempBuf = MSPI_READ((MSPI_FULL_DEPLUX_OFFSET + (u8Index >> 1)));
            rx_buff[u8Index] = u16TempBuf >> 8;
            rx_buff[u8Index-1] = u16TempBuf & 0xFF;
        }
        else if(u8Index == (u16Size -1))
        {
            u16TempBuf = MSPI_READ((MSPI_FULL_DEPLUX_OFFSET + (u8Index >> 1)));
            rx_buff[u8Index] = u16TempBuf & 0xFF;
        }
    }

err_out:
#ifndef CONFIG_SPI_INT_CALL
    HAL_MSPI_Unlock();
#endif
    return errnum;
}

static u8 HAL_MSPI_ReadBuf(struct mspi_hal *mspi, u8 *pData, u16 u16Size)
{
    u8  u8Index = 0;
    u16  u16TempBuf = 0;
    u16 i =0, j = 0;
    u8 errnum = E_MSPI_OK;

#ifndef CONFIG_SPI_INT_CALL
    HAL_MSPI_Lock();
#endif

    for(i = 0; i < u16Size; i+= MAX_READ_BUF_SIZE)
    {
        u16TempBuf = u16Size - i;
        if(u16TempBuf > MAX_READ_BUF_SIZE)
        {
            j = MAX_READ_BUF_SIZE;
        }
        else
        {
            j = u16TempBuf;
        }
        HAL_MSPI_RWBUFSize(mspi,MSPI_READ_INDEX, j);

        errnum = HAL_MSPI_Trigger(mspi);
        if(errnum)
        {
            goto err_out;
        }

        for(u8Index = 0; u8Index < j; u8Index++)
        {
            if(u8Index & 1)
            {
                u16TempBuf = MSPI_READ((MSPI_READ_BUF_OFFSET + (u8Index >> 1)));
                pData[u8Index] = u16TempBuf >> 8;
                pData[u8Index-1] = u16TempBuf & 0xFF;
            }
            else if(u8Index == (j -1))
            {
                u16TempBuf = MSPI_READ((MSPI_READ_BUF_OFFSET + (u8Index >> 1)));
                pData[u8Index] = u16TempBuf & 0xFF;
            }
        }
        pData+= j;
    }

err_out:
#ifndef CONFIG_SPI_INT_CALL
    HAL_MSPI_Unlock();
#endif
    return errnum;
}

static u8 HAL_MSPI_WriteBuf(struct mspi_hal *mspi, u8 *pData, u16 u16Size)
{
    u8  u8Index = 0;
    u16 u16TempBuf = 0;
    u8 errnum = E_MSPI_OK;

#ifndef CONFIG_SPI_INT_CALL
    HAL_MSPI_Lock();
#endif

    for(u8Index = 0; u8Index < u16Size; u8Index++)
    {
        if(u8Index & 1)
        {
            u16TempBuf = (pData[u8Index] << 8) | pData[u8Index-1];
            MSPI_WRITE((MSPI_WRITE_BUF_OFFSET + (u8Index >> 1)),u16TempBuf);
        }
        else if(u8Index == (u16Size -1))
        {
            MSPI_WRITE((MSPI_WRITE_BUF_OFFSET + (u8Index >> 1)),pData[u8Index]);
        }
    }

    HAL_MSPI_RWBUFSize(mspi,MSPI_WRITE_INDEX, u16Size);
    errnum = HAL_MSPI_Trigger(mspi);
    if(errnum)
    {
        goto err_out;
    }
    // set write data size
err_out:
#ifndef CONFIG_SPI_INT_CALL
    HAL_MSPI_Unlock();
#endif
    return errnum;
}

u8 HAL_MSPI_FullDuplex(u8 u8Channel ,struct mspi_hal * mspi, u8 * rx_buff, u8 * tx_buff, u16 u16Size)
{
    u16 u16Index = 0;
    u16 u16TempFrameCnt=0;
    u16 u16TempLastFrameCnt=0;
    u8  ret = E_MSPI_OK;

    if(rx_buff == NULL || tx_buff == NULL)
    {
        return E_MSPI_NULL;
    }

    u16TempFrameCnt = u16Size/MAX_WRITE_BUF_SIZE; //Cut data to frame by max frame size
    u16TempLastFrameCnt = u16Size%MAX_WRITE_BUF_SIZE; //Last data less than a MAX_WRITE_BUF_SIZE fame

    for (u16Index = 0; u16Index < u16TempFrameCnt; u16Index++)
    {
        ret = HAL_MSPI_FullDuplexBuf(mspi,rx_buff+u16Index*MAX_READ_BUF_SIZE,tx_buff+u16Index*MAX_WRITE_BUF_SIZE,MAX_WRITE_BUF_SIZE);
        if (ret)
        {
            return E_MSPI_OPERATION_ERROR;
        }
    }

    if(u16TempLastFrameCnt)
    {
        ret = HAL_MSPI_FullDuplexBuf(mspi,rx_buff+u16TempFrameCnt*MAX_READ_BUF_SIZE,tx_buff+u16TempFrameCnt*MAX_WRITE_BUF_SIZE,u16TempLastFrameCnt);
    }

    if (ret) {
        return E_MSPI_OPERATION_ERROR;
    }
    return E_MSPI_OK;
}

u8 HAL_MSPI_Write(u8 u8Channel ,struct mspi_hal *mspi, u8 *pData, u16 u16Size)
{
    u16 u16Index = 0;
    u16 u16TempFrameCnt=0;
    u16 u16TempLastFrameCnt=0;
    u8  ret = E_MSPI_OK;

    if(pData == NULL)
    {
        return E_MSPI_NULL;
    }

    u16TempFrameCnt = u16Size/MAX_WRITE_BUF_SIZE; //Cut data to frame by max frame size
    u16TempLastFrameCnt = u16Size%MAX_WRITE_BUF_SIZE; //Last data less than a MAX_WRITE_BUF_SIZE fame

    for (u16Index = 0; u16Index < u16TempFrameCnt; u16Index++)
    {
        ret = HAL_MSPI_WriteBuf(mspi,pData+u16Index*MAX_WRITE_BUF_SIZE,MAX_WRITE_BUF_SIZE);
        if (ret)
        {
            return E_MSPI_OPERATION_ERROR;
        }
    }

    if(u16TempLastFrameCnt)
    {
        ret = HAL_MSPI_WriteBuf(mspi,pData+u16TempFrameCnt*MAX_WRITE_BUF_SIZE,u16TempLastFrameCnt);
    }

    if (ret) {
        return E_MSPI_OPERATION_ERROR;
    }
    return E_MSPI_OK;
}

u8 HAL_MSPI_Read(u8 u8Channel, struct mspi_hal *mspi, u8 *pData, u16 u16Size)
{
    //MSPI_ErrorNo errorno = E_MSPI_OK;

    u16 u16Index = 0;
    u16 u16TempFrameCnt=0;
    u16 u16TempLastFrameCnt=0;
    u8  ret = E_MSPI_OK;

    if(pData == NULL)
    {
        return E_MSPI_NULL;
    }

    u16TempFrameCnt = u16Size/MAX_WRITE_BUF_SIZE;        //Cut data to frame by max frame size
    u16TempLastFrameCnt = u16Size%MAX_WRITE_BUF_SIZE;    //Last data less than a MAX_WRITE_BUF_SIZE fame
    for (u16Index = 0; u16Index < u16TempFrameCnt; u16Index++)
    {
        ret = HAL_MSPI_ReadBuf(mspi,pData+u16Index*MAX_WRITE_BUF_SIZE,MAX_WRITE_BUF_SIZE);
        if (ret)
        {
            return E_MSPI_OPERATION_ERROR;
        }
    }
    if(u16TempLastFrameCnt)
    {
        ret = HAL_MSPI_ReadBuf(mspi,pData+u16TempFrameCnt*MAX_WRITE_BUF_SIZE,u16TempLastFrameCnt);
    }
    if (ret)
    {
        return E_MSPI_OPERATION_ERROR;
    }
    return E_MSPI_OK;
}

u8 HAL_MSPI_DMA_Write(u8      u8Channel, struct mspi_hal *mspi, u8 *pData, u16 u16Size)
{
    u8                errnum;
    u32               data_addr;
    HalMoveDmaMspi_t  tMoveDmaMspi;
    HalMoveDmaParam_t tMoveDmaParam;

    if(pData == NULL)
    {
        return E_MSPI_NULL;
    }

    data_addr = virt_to_phys(pData);
    if(data_addr > MSPI_MIU1_BUS_BASE)
        data_addr -= MSPI_MIU1_BUS_BASE;
    else
        data_addr -= MSPI_MIU0_BUS_BASE;

    tMoveDmaParam.u32Mode        = HAL_MOVEDMA_MSPI;
    tMoveDmaParam.u32SrcAddr     = data_addr;
    tMoveDmaParam.u32SrcMiuSel   = (data_addr < ARM_MIU1_BASE_ADDR) ? (0) : (1);
    tMoveDmaParam.u32DstAddr     = 0;
    tMoveDmaParam.u32DstMiuSel   = 0;
    tMoveDmaParam.u32Count       = u16Size;
    tMoveDmaParam.CallBackFunc   = NULL;
    tMoveDmaParam.CallBackArg    = 0;

    tMoveDmaMspi.u32Direction    = HAL_MOVEDMA_WR;
    tMoveDmaMspi.u32DeviceSelect = u8Channel;
    tMoveDmaParam.pstMspist      = &tMoveDmaMspi;

    Chip_Flush_Cache_Range((u32)pData, u16Size);

    if (HAL_MOVEDMA_NO_ERR != HalMoveDma_MoveData(&tMoveDmaParam))
    {
        mspi_errmsg("move dma set fail wr\n");
        return E_MSPI_ERR;
    }

    HAL_MSPI_Lock();

    MSPI_WRITE(MSPI_DMA_ENABLE, 0x01);
    MSPI_WRITE(MSPI_DMA_RW_MODE, MSPI_DMA_WRITE);

    MSPI_WRITE(MSPI_DMA_DATA_LENGTH_L, u16Size & 0xFFFF );
    MSPI_WRITE(MSPI_DMA_DATA_LENGTH_H, u16Size>>16 );

    HAL_MSPI_RWBUFSize(mspi,MSPI_WRITE_INDEX, 0);
    errnum = HAL_MSPI_Trigger(mspi);
    if(errnum)
    {
        goto err_out;
    }

err_out:
    MSPI_WRITE(MSPI_DMA_ENABLE, 0x00);
    HAL_MSPI_Unlock();
    return errnum;

}

u8 HAL_MSPI_DMA_Read(u8 u8Channel, struct mspi_hal *mspi, u8 *pData, u16 u16Size)
{
    u8                errnum;
    u32               data_addr;
    HalMoveDmaMspi_t  tMoveDmaMspi;
    HalMoveDmaParam_t tMoveDmaParam;

    if(pData == NULL)
    {
        return E_MSPI_NULL;
    }

    data_addr = virt_to_phys(pData);
    if(data_addr > MSPI_MIU1_BUS_BASE)
        data_addr -= MSPI_MIU1_BUS_BASE;
    else
        data_addr -= MSPI_MIU0_BUS_BASE;

    tMoveDmaParam.u32Mode        = HAL_MOVEDMA_MSPI;
    tMoveDmaParam.u32SrcAddr     = 0;
    tMoveDmaParam.u32SrcMiuSel   = 0;
    tMoveDmaParam.u32DstAddr     = data_addr;
    tMoveDmaParam.u32DstMiuSel   = (data_addr < ARM_MIU1_BASE_ADDR) ? (0) : (1);
    tMoveDmaParam.u32Count       = u16Size;
    tMoveDmaParam.CallBackFunc   = NULL;
    tMoveDmaParam.CallBackArg    = 0;

    tMoveDmaMspi.u32Direction    = HAL_MOVEDMA_RD;
    tMoveDmaMspi.u32DeviceSelect = u8Channel;
    tMoveDmaParam.pstMspist      = &tMoveDmaMspi;

    if (HAL_MOVEDMA_NO_ERR != HalMoveDma_MoveData(&tMoveDmaParam))
    {
        mspi_errmsg("move dma set fail rd\n");
        return E_MSPI_ERR;
    }

    HAL_MSPI_Lock();

    MSPI_WRITE(MSPI_DMA_ENABLE, 0x01);
    MSPI_WRITE(MSPI_DMA_RW_MODE, MSPI_DMA_READ);

    MSPI_WRITE(MSPI_DMA_DATA_LENGTH_L, u16Size & 0xFFFF );
    MSPI_WRITE(MSPI_DMA_DATA_LENGTH_H, (u16Size>>16)& 0x00FF);  // 24bit

    HAL_MSPI_RWBUFSize(mspi,MSPI_READ_INDEX, 0);                //spi length = 0
    Chip_Inv_Cache_Range((u32)pData, u16Size);
    errnum = HAL_MSPI_Trigger(mspi);
    if(errnum)
    {
        goto err_out;
    }

    Chip_Inv_Cache_Range((u32)pData, u16Size);

err_out:
    MSPI_WRITE(MSPI_DMA_ENABLE, 0x00);
    HAL_MSPI_Unlock();
    return errnum;
}

u16 HAL_MSPI_CheckDone(struct mspi_hal *mspi)
{
    return MSPI_READ(MSPI_DONE_OFFSET);
}

void HAL_MSPI_ClearDone(struct mspi_hal *mspi)
{
    MSPI_WRITE(MSPI_DONE_CLEAR_OFFSET,MSPI_CLEAR_DONE);
}

u8 HAL_MSPI_SetDivClk(struct mspi_hal *mspi, u8 div)
{
    u16 TempData = 0;

    TempData = MSPI_READ(MSPI_CLK_CLOCK_OFFSET);
    TempData &= MSPI_CLK_CLOCK_MASK;
    TempData |= div << MSPI_CLK_CLOCK_BIT_OFFSET;
    MSPI_WRITE(MSPI_CLK_CLOCK_OFFSET, TempData);

    return E_MSPI_OK;
}

u8 HAL_MSPI_CheckDmaMode(u8 u8Channel)
{
    if(u8Channel > MSPI_DMA_MODE_MAX)
    {
        return E_MSPI_HW_NOT_SUPPORT;
    }
    else
    {
        return E_MSPI_OK;
    }
}


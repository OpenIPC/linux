/*
*	Copyright (c) 2010 Shanghai Fullhan Microelectronics Co., Ltd.
*				All Rights Reserved. Confidential.
*
*This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef __ASM_ARCH_I2C_H
#define __ASM_ARCH_I2C_H
#include "fh_predefined.h"

//I2C
#define     REG_I2C_CON             (0x0000)
#define     REG_I2C_TAR             (0x0004)
#define     REG_I2C_SAR             (0x0008)
#define     REG_I2C_HS_MADDR        (0x000C)
#define     REG_I2C_DATA_CMD        (0x0010)
#define     REG_I2C_SS_SCL_HCNT     (0x0014)
#define     REG_I2C_SS_SCL_LCNT     (0x0018)
#define     REG_I2C_FS_SCL_HCNT     (0x001C)
#define     REG_I2C_FS_SCL_LCNT     (0x0020)
#define     REG_I2C_HS_SCL_HCNT     (0x0024)
#define     REG_I2C_HS_SCL_LCNT     (0x0028)
#define     REG_I2C_INTR_STAT       (0x002c)
#define     REG_I2C_INTR_MASK       (0x0030)
#define     REG_I2C_RAW_INTR_STAT   (0x0034)
#define     REG_I2C_RX_TL           (0x0038)
#define     REG_I2C_TX_TL           (0x003c)
#define     REG_I2C_CLR_INTR        (0x0040)
#define     REG_I2C_ENABLE          (0x006c)
#define     REG_I2C_STATUS          (0x0070)
#define     REG_I2C_TXFLR           (0x0074)
#define     REG_I2C_RXFLR           (0x0078)
#define     REG_I2C_DMA_CR          (0x0088)
#define     REG_I2C_DMA_TDLR        (0x008c)
#define     REG_I2C_DMA_RDLR        (0x0090)

#define DW_IC_INTR_NONE         0x0


enum BUS_STATUS {
	I2C_BUSY,
	I2C_IDLE
};
enum RESULT {
	SUCCESS,
	FAILURE
};
enum ENABLE_SET {
	DISABLE,
	ENABLE
};
enum SPEED_MODE {
	SSPEED = 1,
	FSPEED = 2,
	HSPEED = 3,
};

UINT32 I2c_Disable(UINT32 base_addr);

void I2c_SetSpeed(UINT32 base_addr, UINT8 model);
void I2c_SetDeviceId(UINT32 base_addr, UINT32 deviceID);
void I2c_Enable(UINT32 enable);
UINT32 I2c_GetStatus(UINT32 base_addr);
void I2c_SetIr(UINT32 base_addr, UINT16 mask);
UINT32 I2c_Disable(UINT32 base_addr);

void I2c_Init(UINT32 base_addr, UINT16 slave_addr, enum SPEED_MODE speed,
	      int txtl, int rxtl);

/* function Macro */

/*************************************************************************
* Function Name  : I2C_GetTransmitFifoLevel
* Description    : get tx fifo level
* Input          : base addr
* Output         : None
* Return         : None
*
 ***********************************************************************/
#define I2C_GetTransmitFifoLevel(base_addr)   (GET_REG(base_addr + \
		REG_I2C_TXFLR))

/*******************************************************************************
* Function Name  : I2c_GetTxFifoDepth
* Description    : get tx fifo depth
* Input          : base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define I2c_GetTxFifoDepth( base_addr)   (((GET_REG(base_addr + \
		DW_IC_COMP_PARAM_1)>> 16) & 0xff) + 1)

/*******************************************************************************
* Function Name  : I2c_GetRxFifoDepth
* Description    : get rx fifo depth
* Input          : base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define I2c_GetRxFifoDepth( base_addr)   (((GET_REG(base_addr + \
		DW_IC_COMP_PARAM_1)>> 8) & 0xff) + 1)
/*******************************************************************************
* Function Name  : I2c_SetDeviceId
* Description    : set the slave addr
* Input          : deviceID:slave addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define I2c_SetDeviceId( base_addr,deviceID)   SET_REG(base_addr + REG_I2C_TAR,deviceID)  //set IIC  slave address

/*******************************************************************************
* Function Name  : I2c_Read
* Description    : read data from I2C bus
* Input          : None
* Output         : None
* Return         : data:I2C  data
*
 *******************************************************************************/

#define I2c_Read(base_addr )   (GET_REG(base_addr + REG_I2C_DATA_CMD)&0xff)  //DW_I2C_DATA_CMD
/*******************************************************************************
* Function Name  : I2c_SetSsSclHcnt
* Description    : set i2c ss scl hcnt
* Input          : hcnt
* Output         : None
* Return         : data:I2C  data
*
 *******************************************************************************/

#define I2c_SetSsHcnt(base_addr, hcnt)  SET_REG(base_addr + DW_IC_SS_SCL_HCNT,hcnt)

/*******************************************************************************
* Function Name  : I2c_SetSsSclLcnt
* Description    : set i2c ss scl lcnt
* Input          : lcnt
* Output         : None
* Return         : data:I2C  data
*
 *******************************************************************************/

#define I2c_SetSsLcnt(base_addr, lcnt)  SET_REG(base_addr + DW_IC_SS_SCL_LCNT,lcnt)
/*******************************************************************************
* Function Name  : I2c_SetFsSclHcnt
* Description    : set i2c fs scl hcnt
* Input          : hcnt
* Output         : None
* Return         : data:I2C  data
*
 *******************************************************************************/

#define I2c_SetFsHcnt(base_addr, hcnt)  SET_REG(base_addr + DW_IC_FS_SCL_HCNT,hcnt)

/*******************************************************************************
* Function Name  : I2c_SetFsSclLcnt
* Description    : set i2c fs scl lcnt
* Input          : lcnt
* Output         : None
* Return         : data:I2C  data
*
 *******************************************************************************/

#define I2c_SetFsLcnt(base_addr, lcnt)  SET_REG(base_addr + DW_IC_FS_SCL_LCNT,lcnt)
/*******************************************************************************
* Function Name  : I2c_Disable
* Description    : disable I2C bus
* Input          : None
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define I2c_DisEnable(base_addr)  SET_REG(base_addr + REG_I2C_ENABLE,DISABLE);
/*******************************************************************************
* Function Name  : I2c_Enable
* Description    : set the I2C bus enable
* Input          : enable
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define I2c_Enable(base_addr)  SET_REG(base_addr + REG_I2C_ENABLE,ENABLE);
/*******************************************************************************
* Function Name  : I2c_Write
* Description    : Write data to I2C bus
* Input          : data:wirte out data
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define I2c_Write(base_addr, data)  SET_REG(base_addr + REG_I2C_DATA_CMD,data)

/*******************************************************************************
* Function Name  : I2c_GetTxTl
* Description    : Get TX_TL
* Input          : TX_TL
* Return         : None
 *******************************************************************************/
#define I2c_GetTxTl(base_addr )     (GET_REG(base_addr + REG_I2C_TX_TL)&0xff);
/*******************************************************************************
* Function Name  : I2c_GetRxTl
* Description    : Get RX_TL
* Input          : RX_TL
* Return         : None
 *******************************************************************************/
#define I2c_GetRxTl(base_addr )     (GET_REG(base_addr + REG_I2C_RX_TL)&0xff);
/*******************************************************************************
* Function Name  : I2c_GetRxFLR
* Description    : Get RX_FLR
* Input          : base_addr
* Return         : None
 *******************************************************************************/
#define I2c_GetRxFLR(base_addr)     (GET_REG(base_addr + DW_IC_RXFLR)&0xff);
/*******************************************************************************
* Function Name  : I2c_GetTxFLR
* Description    : Get TX_FLR
* Input          : base_addr
* Return         : None
 *******************************************************************************/
#define I2c_GetTxFLR(base_addr)     (GET_REG(base_addr + DW_IC_TXFLR)&0xff);
/*******************************************************************************
* Function Name  : I2c_SetTxRxTl
* Description    : set TX_TL RX_TL
* Input          : TX_TL, RX_TL
* Return         : None
 *******************************************************************************/
#define I2c_SetTxRxTl(base_addr ,txtl,rxtl)    \
    SET_REG(base_addr + REG_I2C_TX_TL, txtl);   \
    SET_REG(base_addr + REG_I2C_RX_TL, rxtl)

/*******************************************************************************
* Function Name  : I2c_IsActiveMst
* Description    : if master mode is active, return 1
* Input          : none
* Return         : MST_ACTIVITY (IC_STATUS[5])
 *******************************************************************************/
#define I2c_IsActiveMst(base_addr)   (GET_REG(base_addr + REG_I2C_STATUS)>>5 & 1)

/*******************************************************************************
* Function Name  : I2c_SetCon
* Description    : set config
* Input          : config
* Return         : None
 *******************************************************************************/
#define I2c_SetCon(base_addr,config)      SET_REG(base_addr + REG_I2C_CON,config)
/*******************************************************************************
* Function Name  : I2c_GetCon
* Description    : get config
* Input          : config
* Return         : None
 *******************************************************************************/
#define I2c_GetCon(base_addr)      GET_REG(base_addr + REG_I2C_CON)

/*******************************************************************************
* Function Name  : I2c_Status
* Description    : get i2c status
* Input          : None
* Return         : None
 *******************************************************************************/
#define I2c_Status(base_addr)     GET_REG(base_addr + REG_I2C_STATUS)

/*******************************************************************************
* Function Name  : I2c_SetTar
* Description    : set target address
* Input          : id
* Return         : None
 *******************************************************************************/
#define I2c_SetTar(base_addr, id)      SET_REG(base_addr + REG_I2C_TAR,id)

/*******************************************************************************
* Function Name  : I2c_SetIntrMask
* Description    : set interrupt mask
* Input          : mask
* Return         : None
 *******************************************************************************/
#define I2c_SetIntrMask(base_addr,mask)   SET_REG(base_addr + REG_I2C_INTR_MASK,mask)

/*******************************************************************************
* Function Name  : I2c_ClrIntr
* Description    : clear interrupt
* Input          : mask
* Return         : None
 *******************************************************************************/
#define I2c_ClrIntr(base_addr,mask)   GET_REG(base_addr + mask)
/*******************************************************************************
* Function Name  : I2c_REG_STATUS
* Description    : clear interrupt
* Input          : mask
* Return         : None
 *******************************************************************************/
#define I2c_GetTxAbrtSource(base_addr)   GET_REG(base_addr + DW_IC_TX_ABRT_SOURCE)

/*******************************************************************************
* Function Name  : I2c_TxEmpty
* Description    : TX_EMPTY interrupt assert
* Input          : none
* Return         : TX_EMPTY
 *******************************************************************************/
#define I2c_TxEmpty(base_addr)   (GET_REG(base_addr + REG_I2C_RAW_INTR_STAT) & M_TX_EMPTY)

/*******************************************************************************
* Function Name  : I2c_RxFull
* Description    : RX_FULL interrupt assert
* Input          : none
* Return         : RX_FULL
 *******************************************************************************/
#define I2c_RxFull(base_addr)    (GET_REG(base_addr + REG_I2C_RAW_INTR_STAT) & M_RX_FULL)
/*******************************************************************************
* Function Name  : I2c_RxEmpty
* Description    : RX_EMPTY interrupt assert
* Input          : none
* Return         : RX_EMPTY
 *******************************************************************************/
#define I2c_RxEmpty(base_addr)    (GET_REG(base_addr + REG_I2C_RAW_INTR_STAT) & M_RX_OVER)

/* register define */
typedef union {
	struct {
		UINT32 MASTER_MODE      	: 1;
		UINT32 SPEED            	: 2;
		UINT32 IC_10BITADDR_SLAVE   : 1;
		UINT32 IC_10BITADDR_MASTER  : 1;
		UINT32 IC_RESTART_EN        : 1;
		UINT32 IC_SLAVE_DISABLE     : 1;
		UINT32 reserved_31_7        : 25;
	} x;
	UINT32 dw;
} Reg_I2c_Con;

#endif /* __ASM_ARCH_I2C_H */

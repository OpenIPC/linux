
/*******************************************
 * 
 * new drive add by xuww
 * 
 * 
 * **********************************/
/**
* @file			
* @brief  
* @version 
* @author		xuww
* @date			
* @note
*      
*      
* @copy
*
* �˴���Ϊ�Ϻ����΢�������޹�˾��Ŀ���룬�κ��˼���˾δ����ɲ��ø��ƴ�����������
* ����˾�������Ŀ����˾����һ��׷��Ȩ����
*
* <h1><center>&copy; COPYRIGHT 2013 fullhan</center></h1>
*/
/* Includes ------------------------------------------------------------------*/
#include "dma.h"
#include <mach/fh_predefined.h>

#define		MAX_DMA_CHANS				(4)

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : Dma_GetChanStatus
* Description    : get the channel status 
* Input          : nChanID :channel ID 
* Output         : None
* Return         : None
* 		                                      
 *******************************************************************************/
int Dma_GetChanStatus( int nChanID )
{
	return (int)(GET_REG( REG_DMAC_CHAN_EN ) & ( 1 << nChanID ));
}
/*******************************************************************************
* Function Name  : Dma_EnableChan
* Description    : enable channel
* Input          : nChanID   :channel ID 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_EnableChan( int nChanID )
{
	int nMask = (1 << (nChanID + 8)) | ( 1 << nChanID );

	SET_REG_M( REG_DMAC_CHAN_EN, nMask, nMask );
}
/*******************************************************************************
* Function Name  : Dma_DisableChan
* Description    : disable channel  
* Input          : nChanID :channel ID 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_DisableChan( int nChanID )
{
	int nMask = ( 1 << ( nChanID + 8 ) );

	SET_REG_M( REG_DMAC_CHAN_EN, nMask, (nMask + (1 << nChanID)) );
	while( GET_REG( REG_DMAC_CHAN_EN) & (1 << nChanID) );
}
/*******************************************************************************
* Function Name  : Dma_ClearIsrBit
* Description    : clear the  interruput   bit 
* Input          : iChan :channel ID    nMask :unchange bit
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_ClearIsrBit( int iChan, int nMask )
{
	if( nMask & DMA_INTT_TXR)
		SET_REG( REG_DMAC_INTCLR_TFR, 1 << iChan );
	if( nMask & DMA_INTT_BLOCK)
		SET_REG( REG_DMAC_INTCLR_BLK, 1 << iChan );
	if( nMask & DMA_INTT_SOURCE )
		SET_REG( REG_DMAC_INTCLR_SRCTXR, 1 << iChan );
	if( nMask & DMA_INTT_DEST )
		SET_REG( REG_DMAC_INTCLR_DSTTXR, 1 << iChan );
	if( nMask & DMA_INTT_ERR )
		SET_REG( REG_DMAC_INTCLR_ERR, 1 << iChan );
}
/*******************************************************************************
* Function Name  : Dma_EnableIsrBit
* Description    : enable intruput bit  
* Input          : iChan :channel ID    nMask:unchange bit 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_EnableIsrBit( int iChan, int nMask )
{
	int nEnable = (1 << (iChan + 8)) | (1 << iChan);
	
	if( nMask & DMA_INTT_TXR) 
		SET_REG_M( REG_DMAC_INTMSK_TFR, nEnable, nEnable );
	if( nMask & DMA_INTT_BLOCK)
		SET_REG_M( REG_DMAC_INTMSK_BLK, nEnable, nEnable );
	if( nMask & DMA_INTT_SOURCE)
		SET_REG_M( REG_DMAC_INTMSK_SRCTXR, nEnable, nEnable );
	if( nMask & DMA_INTT_DEST)
		SET_REG_M( REG_DMAC_INTMSK_DSTTXR, nEnable, nEnable );
	if( nMask & DMA_INTT_ERR)
		SET_REG_M( REG_DMAC_INTMSK_ERR, nEnable, nEnable );
	
	SET_REG_M( REG_DMAC_CTXi(iChan), 1, 1 );		// Enable isr.
}
/*******************************************************************************
* Function Name  : Dma_DisableIsrBit
* Description    : disbale interruput  
* Input          : iChan:channel ID    nMask:unchange bit
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/

void Dma_DisableIsrBit( int iChan, int nMask )
{
	int nEnable = (1 << (iChan + 8)) | (1 << iChan);
	
	if( nMask & DMA_INTT_TXR) 
		SET_REG_M( REG_DMAC_INTCLR_TFR, 0, nEnable );
	if( nMask & DMA_INTT_BLOCK)
		SET_REG_M( REG_DMAC_INTCLR_BLK, 0, nEnable );
	if( nMask & DMA_INTT_SOURCE)
		SET_REG_M( REG_DMAC_INTCLR_SRCTXR, 0, nEnable );
	if( nMask & DMA_INTT_DEST)
		SET_REG_M( REG_DMAC_INTCLR_DSTTXR, 0, nEnable );
	if( nMask & DMA_INTT_ERR)
		SET_REG_M( REG_DMAC_INTCLR_ERR, 0, nEnable );
}
/*******************************************************************************
* Function Name  : Dma_QueryISRStatus
* Description    : not use 
* Input          : None
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
int Dma_QueryISRStatus( int iChan )
{
	return 0;
}
/*******************************************************************************
* Function Name  : Dma_ClearTfrDone
* Description    : clear tfr Done bit
* Input          : iChan:channel ID 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_ClearTfrDone(int iChan )
{
	SET_REG_M( REG_DMAC_CTXi(iChan) + 4, 0, 1 << 12 );
}
/*******************************************************************************
* Function Name  : Dma_SetTxrSize
* Description    : set txr size 
* Input          : iChan:channel ID   nByes:size 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetTxrSize( int iChan, int nBytes )
{
	if( nBytes > 4095 )
		nBytes = 4095;
	SET_REG_M( REG_DMAC_CTXi(iChan) + 4, (unsigned long long)nBytes, 0xfff );
}
/*******************************************************************************
* Function Name  : Dma_SetSrcWidth
* Description    : set source width 
* Input          : iChan:channel ID   nWidth :fifo width
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcWidth( int iChan, int nWidth )
{
	SET_REG_M( REG_DMAC_CTXi(iChan), nWidth << 4, 0x70 );
}
/*******************************************************************************
* Function Name  : Dma_SetDstWidth
* Description    : set destination  
* Input          : iChan:channel ID   nWidth :fifo width
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstWidth( int iChan, int nWidth )
{
	SET_REG_M( REG_DMAC_CTXi(iChan), nWidth << 1, 0xe );
}
/*******************************************************************************
* Function Name  : Dma_SetSrcSize
* Description    : set source size  
* Input          : iChan :channel ID  nSize : fifo depth
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcSize( int iChan, int nSize )		// burst size
{
	SET_REG_M( REG_DMAC_CTXi(iChan), nSize << 14, 0x1c000 );
}
/*******************************************************************************
* Function Name  : Dma_SetDstSize
* Description    : set  destination size 
* Input          : iChan :channel ID  nSize : fifo depth
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstSize( int iChan, int nSize )
{
	SET_REG_M( REG_DMAC_CTXi(iChan), nSize << 11, 0x3800 );
}
/*******************************************************************************
* Function Name  : Dma_EnableSrcBlkChain
* Description    : enable source  block chain 
* Input          : iChan:channel ID 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_EnableSrcBlkChain(int iChan )
{
	SET_REG_M( REG_DMAC_CTXi(iChan), 1 << 28, 1 << 28 );
}
/*******************************************************************************
* Function Name  : Dma_EnableDstBlkChain
* Description    : enable destinationg block chain 
* Input          : iChan:channel ID    
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_EnableDstBlkChain(int iChan )
{
	SET_REG_M( REG_DMAC_CTXi(iChan), 1 << 27, 1 << 27 );
}
/*******************************************************************************
* Function Name  : Dma_SetTxrType
* Description    : set   txr  mode 
* Input          : iChan:channel ID    nMode :transation mode
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetTxrType( int iChan, int nMode )
{
	if( nMode >= 0 && nMode < DMA_TTFC_INVALID )
	{
		SET_REG_M( REG_DMAC_CTXi(iChan), nMode << 20, 0x7 << 20 );	
	}
}
/*******************************************************************************
* Function Name  : Dma_SetDstIncDirection
* Description    : set source address increment decrement   or not change 
* Input          : iChan:channel ID    nDir :0 :increment  1:decrement   other :not   change
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcIncDirection( int iChan, int nDir )
{
	if( nDir == DMA_DIR_INC )
		SET_REG_M( REG_DMAC_CTXi(iChan), 0, 0x600 );
	else if( nDir == DMA_DIR_DEC )
		SET_REG_M( REG_DMAC_CTXi(iChan), 0x200, 0x200 );
	else
		SET_REG_M( REG_DMAC_CTXi(iChan), 0x400, 0x400 );
}
/*******************************************************************************
* Function Name  : Dma_SetDstIncDirection
* Description    : set destination address increment decrement   or not change
* Input          : iChan:channel ID    nDir :0 :increment  1:decrement   other :not   change
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstIncDirection( int iChan, int nDir )
{
	if( nDir == DMA_DIR_INC )
		SET_REG_M( REG_DMAC_CTXi(iChan), 0, 0x180 );
	else if( nDir == DMA_DIR_DEC )
		SET_REG_M( REG_DMAC_CTXi(iChan), 0x80, 0x080 );
	else
		SET_REG_M( REG_DMAC_CTXi(iChan), 0x100, 0x100 );
}
/*******************************************************************************
* Function Name  : Dma_EnableGather
* Description    : set   enable   gather
* Input          : iChan :channel ID    bEnable :0 disable   1:enable
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_EnableGather(int iChan, int bEnable )
{
	int v;
	if( bEnable ) v = 1;
	else v = 0;
	SET_REG_M( REG_DMAC_CTXi(iChan), v << 17, 1 << 17 );	
}
/*******************************************************************************
* Function Name  : Dma_EnableScatter
* Description    : set   enable   scatter  
* Input          : iChan :channel ID    bEnable :0 disable   1:enable 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_EnableScatter(int iChan, int bEnable )
{
	int v = 0;
	if( bEnable ) v = 1;
	SET_REG_M( REG_DMAC_CTXi(iChan), v << 18, 1 << 18 );	
}
/*******************************************************************************
* Function Name  : Dma_SetSrcHsMode
* Description    : set the source handshaking  mode  
* Input          : iChan:channe ID    nMode:0 hardware  1:software
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcHsMode( int iChan, int nMode )
{
	nMode &= 0x1;
	SET_REG_M( REG_DMAC_CFGi(iChan), nMode << 11, 1 << 11 );
}
/*******************************************************************************
* Function Name  : Dma_SetDstHsMode
* Description    : set the destination handshaking  mode  
* Input          : iChan:channe ID    nMode:0 hardware  1:software
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstHsMode( int iChan, int nMode )
{
	nMode &= 1;
	SET_REG_M( REG_DMAC_CFGi(iChan), nMode << 10, 1 << 10 );
}
/*******************************************************************************
* Function Name  : Dma_SetFifoMode
* Description    : set fifo request transation mode 
* Input          : iChan :channel   nMode  :1:half  fifo  or 0:enough one burst
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetFifoMode( int iChan, int nMode )
{
	nMode &= 1;
	SET_REG_M( REG_DMAC_CFGi(iChan) + 4, nMode << 1, 1 << 1 );
}
/*******************************************************************************
* Function Name  : Dma_SetFlowCtrl
* Description    : set dam flow control :source or  destionation 
* Input          : iChan :channel ID      ctrl:0: source   1:destinationg
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetFlowCtrl( int iChan, int ctrl )
{
	ctrl &= 1;
	SET_REG_M( REG_DMAC_CFGi(iChan) + 4, ctrl, 1 );
}
/*******************************************************************************
* Function Name  : Dma_SetSrcAutoload
* Description    : set  destination auto load  the   init address
* Input          : iChan :channel ID    bEnable :enable or disable
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcAutoload(int iChan, int bEnable )
{
	int v = bEnable ? 1 : 0;
	SET_REG_M( REG_DMAC_CFGi(iChan), v << 30, 1 << 30 );
}
/*******************************************************************************
* Function Name  : Dma_SetDstAutoload
* Description    : set  destination auto load  the   init address
* Input          : iChan :channel ID    bEnable :enable or disable 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstAutoload(int iChan, int bEnable )
{
	int v = bEnable ? 1 : 0;
	SET_REG_M( REG_DMAC_CFGi(iChan), v << 31, 1 << 31 );
}
/*******************************************************************************
* Function Name  : Write_Regm
* Description    : write the   reg   mask the unchange bit  
* Input          : addr: reg address   value :reg value    mask :unchange bit 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
static void Write_Regm( unsigned int addr, unsigned int value, unsigned int mask )
{
	unsigned int tmp = GET_REG(addr);
	tmp &= ~mask;
	value &= mask;
	tmp |= value;
	SET_REG(addr, tmp);
}
/*******************************************************************************
* Function Name  : Dma_SetMaxBurst
* Description    : set the max burst size
* Input          : iChan:channel ID     nSize :  burst size
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetMaxBurst( int iChan, int nSize )
{
	if( nSize > 1023 )
		nSize = 1023; 
	Write_Regm( REG_DMAC_CFGi(iChan), (nSize << 20), 0x3ff00000 );
}
/*******************************************************************************
* Function Name  : Dma_SetSrcHsPol
* Description    : set the  source handshaking polatity
* Input          : iChan:channel ID      nPol: polarity  high or low
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcHsPol( int iChan, int nPol )
{
	nPol &= 1;
	SET_REG_M( REG_DMAC_CFGi(iChan), nPol << 19, 1 << 19 );
}
/*******************************************************************************
* Function Name  : Dma_SetDstHsPol
* Description    : set the  destination handshaking polatity
* Input          : iChan:channel ID      nPol: polarity  high or low
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstHsPol( int iChan, int nPol )
{
	nPol &= 1;
	SET_REG_M( REG_DMAC_CFGi(iChan), nPol << 18, 1 << 18 );
}
/*******************************************************************************
* Function Name  : Dma_SetLinkEntry
* Description    : enbale the  link list 
* Input          : iChan:channel ID    nAddr :link list address
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetLinkEntry( int iChan, unsigned int nAddr )
{
	nAddr &= 0xfffffffc;
	// force to use AHB Master 0, for this is the only AHB Master.
	SET_REG_M( REG_DMAC_LLPi(iChan), nAddr, 0xffffffff );
}
/*******************************************************************************
* Function Name  : Dma_SetSrcAddress
* Description    : set source address 
* Input          : iChan:channel ID     nAddr:destination address
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcAddress( int iChan, unsigned int nAddr )
{
	SET_REG_M( REG_DMAC_SARi(iChan), nAddr, 0xffffffff );
}
/*******************************************************************************
* Function Name  : Dma_SetDstAddress
* Description    : set destination address 
* Input          : iChan:channel ID     nAddr:destination address
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstAddress( int iChan, unsigned int nAddr )
{
	SET_REG_M( REG_DMAC_DARi(iChan), nAddr, 0xffffffff );
}
/*******************************************************************************
* Function Name  : Dma_SetSrcPe
* Description    : select the hardshaking   interface
* Input          : iChan:channel ID   nPer:handshaking interface
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetSrcPer( int iChan, unsigned int nPer )
{
	if( nPer < DMA_HSP_INVALID ) 
	{
		SET_REG_M( REG_DMAC_CFGi(iChan) + 4, nPer << 7, 0xf << 7 );
	}
}
/*******************************************************************************
* Function Name  : Dma_SetDstPer
* Description    : select the hardshaking   interface
* Input          : iChan:channel ID   nPer:handshaking interface
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_SetDstPer( int iChan, unsigned int nPer )
{
	if( nPer < DMA_HSP_INVALID )
	{
		SET_REG_M( REG_DMAC_CFGi(iChan) + 4, nPer << 11, 0xf << 11 );
	}
}
/*******************************************************************************
* Function Name  : Dma_GetIsrChan
* Description    : get the status of dam interruput  
* Input          : iChan :channel ID 
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
unsigned int Dma_GetIsrChan(unsigned int nMask)
{
	if( nMask & DMA_INTT_TXR )
		return GET_REG(REG_DMAC_INTSTAT_TFR);
	if( nMask & DMA_INTT_BLOCK )
		return GET_REG(REG_DMAC_INTSTAT_BLK);
	if( nMask & DMA_INTT_SOURCE )
		return GET_REG(REG_DMAC_INTSTAT_SRCTXR);
	if( nMask & DMA_INTT_DEST )
		return GET_REG(REG_DMAC_INTSTAT_DSTTXR);
	if( nMask & DMA_INTT_ERR )
		return GET_REG(REG_DMAC_INTSTAT_ERR);
	
	return 0;
}
/*******************************************************************************
* Function Name  : Dma_StartSrctfr
* Description    : dma source start transaction 
* Input          : iChan:channel
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_StartSrctfr( int iChan )
{
	int nMask = 0x101 << (iChan);

	SET_REG( REG_DMAC_REQSRC, 0x101 );
	SET_REG( REG_DMAC_SGLREQSRC, 0x101 );
}
/*******************************************************************************
* Function Name  : Dma_StartDsttfr
* Description    : destination transaction  request
* Input          : iChan: channel ID
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_StartDsttfr( int iChan )
{
	int nMask = 0x101 << (iChan);
	SET_REG( REG_DMAC_REQDST, nMask );
	SET_REG( REG_DMAC_SGLREQDST, nMask );
}
/*******************************************************************************
* Function Name  : fh_dma_init
* Description    : dma init
* Input          : None
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void fh_dma_init()
{
		Dma_Init();
}
/*******************************************************************************
* Function Name  : Dma_Init
* Description    : dma init 
* Input          : None
* Output         : None
* Return         : None   	
* 		                                      
 *******************************************************************************/
void Dma_Init()
{
	int i;
	SET_REG( REG_DMAC_CFG_REG, 1 );
	
	for( i = 0; i < MAX_DMA_CHANS; i ++ )
	{
		Dma_DisableChan(i);
		Dma_ClearIsrBit( i, DMA_INTT_TXR | DMA_INTT_BLOCK | DMA_INTT_SOURCE | DMA_INTT_DEST | DMA_INTT_ERR );
		Dma_ClearTfrDone(i);
		Dma_SetTxrType(i, DMA_TTFC_M2P_DMAC);
		Dma_SetSrcWidth( i, DMA_TXR_32BITS );
		Dma_SetSrcSize( i, DMA_BURST_8 );	
		Dma_SetDstWidth( i, DMA_TXR_8BITS );
		Dma_SetDstSize( i, DMA_BURST_8 );
		Dma_SetSrcHsPol( i, DMA_HSPOL_ACTHIGH );  
		Dma_SetDstHsPol( i, DMA_HSPOL_ACTHIGH );	
		Dma_SetSrcIncDirection( i, DMA_DIR_INC );
		Dma_SetDstIncDirection( i, DMA_DIR_UNCHG );
		Dma_SetSrcHsMode( i, DMA_HSMODE_SOFTWARE );
		Dma_SetDstHsMode( i, DMA_HSMODE_SOFTWARE );
		Dma_SetMaxBurst( i, 0 );
		Dma_SetFifoMode( i, 0 );
		Dma_SetLinkEntry( i, 0 );
		Dma_EnableGather( i, 0 );
		Dma_EnableScatter( i, 0 );
	}
}

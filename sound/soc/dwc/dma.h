
/*********************************
 * 
 * new drive add by xuww
 * 
 * ****************************/
#ifndef DMA__H
#define	DMA__H


#if(1)
enum
{
	DMA_INTT_TXR = 1,
	DMA_INTT_BLOCK = 2,
	DMA_INTT_SOURCE = 4,
	DMA_INTT_DEST = 8,
	DMA_INTT_ERR = 16
};

enum
{
	DMA_TXR_8BITS = 0,
	DMA_TXR_16BITS = 1,
	DMA_TXR_32BITS = 2,
	DMA_TXR_64BITS = 3,
	DMA_TXR_128BITS = 4,
	DMA_TXR_256BITS = 5,
	DMA_TXR_INVALID = 6
};

enum
{
	DMA_BURST_1 = 0,
	DMA_BURST_4 = 1,
	DMA_BURST_8 = 2,
	DMA_BURST_16 = 3,
	DMA_BURST_32 = 4,
	DMA_BURST_64 = 5,
	DMA_BURST_128 = 6,
	DMA_BURST_256 = 7,
	DMA_BURST_INVALID = 8
};

enum
{
	DMA_TTFC_M2M_DMAC,
	DMA_TTFC_M2P_DMAC,
	DMA_TTFC_P2M_DMAC,
	DMA_TTFC_P2P_DMAC,
	DMA_TTFC_P2M_PFR,
	DMA_TTFC_P2P_PSRC,
	DMA_TTFC_M2P_PFR,
	DMA_TTFC_P2P_PDST,
	DMA_TTFC_INVALID
};

enum
{
	DMA_DIR_INC,
	DMA_DIR_DEC,
	DMA_DIR_UNCHG,
	DMA_DIR_INVALID
};

enum
{
	DMA_HSPOL_ACTHIGH,
	DMA_HSPOL_ACTLOW
};

enum
{
	DMA_HSMODE_HARDWARE = 0,
	DMA_HSMODE_SOFTWARE = 1
};

enum
{
	DMA_HSP_SDC,
	DMA_HSP_AIFRX,
	DMA_HSP_AIFTX,
	DMA_HSP_TAE,
	DMA_HSP_I2SRX,
	DMA_HSP_I2STX,
	DMA_HSP_SPI0RX,
	DMA_HSP_SPI0TX,
	DMA_HSP_SPI1RX,
	DMA_HSP_SPI1TX,
	DMA_HSP_UART0RX,
	DMA_HSP_UART0TX,
	DMA_HSP_UART1RX,
	DMA_HSP_UART1TX,
	DMA_HSP_SPI2RX,
	DMA_HSP_SPI2TX,
	DMA_HSP_INVALID
};
#endif
#define		DMAC_REG_BASE		(0xfe600000)
#define     REG_DMAC_SAR_OFFSET     (0x0)
#define     REG_DMAC_DAR_OFFSET     (0x8)
#define     REG_DMAC_LLP_OFFSET     (0x10)
#define     REG_DMAC_CTX_OFFSET     (0x18)
#define     REG_DMAC_SSTAT_OFFSET       (0x20)
#define     REG_DMAC_DSTAT_OFFSET       (0x28)
#define     REG_DMAC_SSTATAR_OFFSET (0x30)
#define     REG_DMAC_DSTATAR_OFFSET (0x38)
#define     REG_DMAC_CFG_OFFSET     (0x40)
#define     REG_DMAC_SGR_OFFSET     (0x48)
#define     REG_DMAC_DSR_OFFSET     (0x50)
#define     REG_DMAC_SARi(n)            (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_SAR_OFFSET)
#define     REG_DMAC_DARi(n)            (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_DAR_OFFSET)
#define     REG_DMAC_LLPi(n)            (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_LLP_OFFSET)
#define     REG_DMAC_CTXi(n)            (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_CTX_OFFSET)
#define     REG_DMAC_SSTATi(n)          (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_SSTAT_OFFSET)
#define     REG_DMAC_DSTATi(n)          (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_DSTAT_OFFSET)
#define     REG_DMAC_SSTATARi(n)        (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_SSTATAR_OFFSET)
#define     REG_DMAC_DSTATARi(n)        (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_DSTATAR_OFFSET)
#define     REG_DMAC_CFGi(n)            (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_CFG_OFFSET)
#define     REG_DMAC_SGRi(n)            (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_SGR_OFFSET)
#define     REG_DMAC_DSRi(n)            (DMAC_REG_BASE + (n) * 0x58 + REG_DMAC_DSR_OFFSETR)

#define     REG_DMAC_INTRAWTFR          (DMAC_REG_BASE + 0x2c0)
#define     REG_DMAC_INTRAWBLK          (DMAC_REG_BASE + 0x2c8)
#define     REG_DMAC_INTRAWSRCTXR       (DMAC_REG_BASE + 0x2d0)
#define     REG_DMAC_INTRAWDSTTXR       (DMAC_REG_BASE + 0x2d8)
#define     REG_DMAC_INTRAWERR          (DMAC_REG_BASE + 0x2e0)
#define     REG_DMAC_INTSTAT_TFR        (DMAC_REG_BASE + 0x2e8)
#define     REG_DMAC_INTSTAT_BLK        (DMAC_REG_BASE + 0x2f0)
#define     REG_DMAC_INTSTAT_SRCTXR     (DMAC_REG_BASE + 0x2f8)
#define     REG_DMAC_INTSTAT_DSTTXR     (DMAC_REG_BASE + 0x300)
#define     REG_DMAC_INTSTAT_ERR        (DMAC_REG_BASE + 0x308)
#define     REG_DMAC_INTMSK_TFR         (DMAC_REG_BASE + 0x310)
#define     REG_DMAC_INTMSK_BLK         (DMAC_REG_BASE + 0x318)
#define     REG_DMAC_INTMSK_SRCTXR      (DMAC_REG_BASE + 0x320)
#define     REG_DMAC_INTMSK_DSTTXR      (DMAC_REG_BASE + 0x328)
#define     REG_DMAC_INTMSK_ERR         (DMAC_REG_BASE + 0x330)
#define     REG_DMAC_INTCLR_TFR         (DMAC_REG_BASE + 0x338)
#define     REG_DMAC_INTCLR_BLK         (DMAC_REG_BASE + 0x340)
#define     REG_DMAC_INTCLR_SRCTXR      (DMAC_REG_BASE + 0x348)
#define     REG_DMAC_INTCLR_DSTTXR      (DMAC_REG_BASE + 0x350)
#define     REG_DMAC_INTCLR_ERR         (DMAC_REG_BASE + 0x358)
#define     REG_DMAC_INT_STATUS_ALL     (DMAC_REG_BASE + 0x360)

#define     REG_DMAC_REQSRC             (DMAC_REG_BASE + 0x368)
#define     REG_DMAC_REQDST             (DMAC_REG_BASE + 0x370)
#define     REG_DMAC_SGLREQSRC          (DMAC_REG_BASE + 0x378)
#define     REG_DMAC_SGLREQDST          (DMAC_REG_BASE + 0x380)
#define     REG_DMAC_LSTSRC             (DMAC_REG_BASE + 0x388)
#define     REG_DMAC_LSTDST             (DMAC_REG_BASE + 0x390)
#define     REG_DMAC_CFG_REG            (DMAC_REG_BASE + 0x398)
#define     REG_DMAC_CHAN_EN            (DMAC_REG_BASE + 0x3a0)
#define     REG_DMAC_IDREG              (DMAC_REG_BASE + 0x3a8)
#define     REG_DMAC_TESTREG            (DMAC_REG_BASE + 0x3b0)
#define     REG_DMAC_COMPARAMS_6        (DMAC_REG_BASE + 0x3c8)
#define     REG_DMAC_COMPARAMS_5        (DMAC_REG_BASE + 0x3d0)
#define     REG_DMAC_COMPARAMS_4        (DMAC_REG_BASE + 0x3d8)
#define     REG_DMAC_COMPARAMS_3        (DMAC_REG_BASE + 0x3e0)
#define     REG_DMAC_COMPARAMS_2        (DMAC_REG_BASE + 0x3e8)
#define     REG_DMAC_COMPARAMS_1        (DMAC_REG_BASE + 0x3f0)
#define     REG_DMAC_COMP_IDREG         (DMAC_REG_BASE + 0x3f8)
int 	Dma_GetChanStatus( int nChanID );
void 	Dma_EnableChan( int nChanID );
void 	Dma_DisableChan( int nChanID );

void 	Dma_ClearIsrBit( int iChan, int nMask );
void 	Dma_EnableIsrBit( int iChan, int nMask );
void 	Dma_DisableIsrBit( int iChan, int nMask );
unsigned int Dma_GetIsrChan(unsigned int nMask);
int 	Dma_QueryIsrStatus( );

void 	Dma_ClearTfrDone(int iChan);
void 	Dma_SetTxrSize( int iChan, int nBytes );
void 	Dma_SetSrcWidth( int iChan, int nWidth );
void	Dma_SetDstWidth( int iChan, int nWidth );
void 	Dma_SetSrcSize( int iChan, int nSize );		// burst size
void	Dma_SetDstSize( int iChan, int nSize );
void 	Dma_EnableSrcBlkChain(int iChan);
void	Dma_EnableDstBlkChain(int iChan);

void 	Dma_SetTxrType( int iChan, int nMode );
void	Dma_SetSrcIncDirection( int iChan, int nDir );
void	Dma_SetDstIncDirection( int iChan, int nDir );

void 	Dma_EnableGather( int iChan, int bEnable );
void 	Dma_EnableScatter( int iChan, int bEnable);

void 	Dma_SetSrcHsMode( int iChan, int nMode );
void 	Dma_SetDstHsMode( int iChan, int nMode );

void	Dma_SetFifoMode( int iChan, int nMode );
void 	Dma_SetFlowCtrl( int iChan, int ctrl );
void 	Dma_SetSrcAutoload(int iChan, int bEnable );
void	Dma_SetDstAutoload(int iChan, int bEnable );
void	Dma_SetMaxBurst( int iChan, int nSize );

void 	Dma_SetSrcHsPol( int iChan, int nPol );
void 	Dma_SetDstHsPol( int iChan, int nPol );

void 	Dma_SetLinkEntry( int iChan, unsigned int nAddr );
void 	Dma_SetSrcAddress( int iChan, unsigned int nAddr );
void 	Dma_SetDstAddress( int iChan, unsigned int nAddr );
void	Dma_SetSrcPer( int iChan, unsigned int nPer );
void 	Dma_SetDstPer( int iChan, unsigned int nPer );

void 	Dma_StartSrctfr( int iChan );
void 	Dma_StartDsttfr( int iChan );

void Dma_Init();

#endif


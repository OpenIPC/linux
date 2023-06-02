/*------------------------------------------------------------------------------
	Copyright (c) 2009 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
    PROJECT: MSB250x Linux BSP
    DESCRIPTION:
        DMA driver of MSB250x dual role USB device controllers

    HISTORY:
        6/11/2008     Winder Sung    First Created

    NOTE:
        This driver is from other project in MStar Co,.
-------------------------------------------------------------------------------*/

/******************************************************************************
 * Include Files
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
//#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/cacheflush.h>
#include <linux/jiffies.h>

//#include <kernel.h>

//#include <cache_ops.h>
#if defined( CONFIG_ARM )
//#include <mach/irqs.h>
#endif

#if defined(CONFIG_USB_MSB250X_MODULE)
#define CONFIG_USB_MSB250X 1
#endif

#if defined(CONFIG_USB_MSB250X_DMA_MODULE)
#define CONFIG_USB_MSB250X_DMA 1
#endif

//#include "hal_timer.h"
#include "ms_dma.h"
#include "ms_config.h"
#include "ms_otg.h"
#include "ms_drc.h"
#include <asm/outercache.h>
#define HalUtilPHY2MIUAddr(addr)        addr
#define HalUtilMIU2PHYAddr(addr)        addr

#if defined(CONFIG_ARCH_MSW8533X)
#include <hal_drv_util.h>

#define SYSHAL_DCACHE_LINE_SIZE 32

#define HAL_DCACHE_START_ADDRESS(_addr_) \
    (((u32)(_addr_)) & ~(SYSHAL_DCACHE_LINE_SIZE-1))

#define HAL_DCACHE_END_ADDRESS(_addr_, _asize_) \
    (((u32)((_addr_) + (_asize_) + (SYSHAL_DCACHE_LINE_SIZE-1) )) & \
     ~(SYSHAL_DCACHE_LINE_SIZE-1))

void _hal_dcache_flush(void *base , u32 asize)
{
    register u32 _addr_ = HAL_DCACHE_START_ADDRESS((u32)base);
    register u32 _eaddr_ = HAL_DCACHE_END_ADDRESS((u32)(base), asize);

    for( ; _addr_ < _eaddr_; _addr_ += SYSHAL_DCACHE_LINE_SIZE )
        __asm__ __volatile__ ("MCR p15, 0, %0, c7, c14, 1" : : "r" (_addr_));

    /* Drain write buffer */
    _addr_ = 0x00UL;
    __asm__ __volatile__ ("MCR p15, 0, %0, c7, c10, 4" : : "r" (_addr_));
}

#define msb250x_dma_dcache_flush(addr, size) _hal_dcache_flush(addr, size)
#define msb250x_dma_dcache_invalidate(addr, size)
#define msb250x_dma_dcache_flush_invalidate(addr, size) _hal_dcache_flush((void *)addr, size)
#elif defined(CONFIG_MSTAR_MSW8X68) || defined(CONFIG_MSTAR_MSW8X68T)
#ifdef CONFIG_OUTER_CACHE
#include <asm/outercache.h>
#endif
#define HalUtilPHY2MIUAddr(addr)        addr
#define HalUtilMIU2PHYAddr(addr)        addr

#define SYSHAL_DCACHE_LINE_SIZE 32

#define HAL_DCACHE_START_ADDRESS(_addr_) \
    (((u32)(_addr_)) & ~(SYSHAL_DCACHE_LINE_SIZE-1))

#define HAL_DCACHE_END_ADDRESS(_addr_, _asize_) \
    (((u32)((_addr_) + (_asize_) + (SYSHAL_DCACHE_LINE_SIZE-1) )) & \
     ~(SYSHAL_DCACHE_LINE_SIZE-1))

void _hal_dcache_flush(void *base , u32 asize)
{
    register u32 _addr_ = HAL_DCACHE_START_ADDRESS((u32)base);
    register u32 _eaddr_ = HAL_DCACHE_END_ADDRESS((u32)(base), asize);

    for( ; _addr_ < _eaddr_; _addr_ += SYSHAL_DCACHE_LINE_SIZE )
        __asm__ __volatile__ ("MCR p15, 0, %0, c7, c14, 1" : : "r" (_addr_));

    /* Drain write buffer */
    _addr_ = 0x00UL;
    __asm__ __volatile__ ("MCR p15, 0, %0, c7, c10, 4" : : "r" (_addr_));
}

#define msb250x_dma_dcache_flush(addr, size) _hal_dcache_flush(addr, size)
#define msb250x_dma_dcache_invalidate(addr, size)
#ifdef CONFIG_OUTER_CACHE
#define msb250x_dma_dcache_flush_invalidate(addr, size) \
    do{ \
        _hal_dcache_flush((void *)addr, size); \
        outer_flush_range(__pa(addr),__pa(addr) + size); \
        outer_inv_range(__pa(addr),__pa(addr) + size); \
    }while(0)
#else
#define msb250x_dma_dcache_flush_invalidate(addr, size) _hal_dcache_flush((void *)addr, size)
#endif    
#else
#define HalUtilPHY2MIUAddr(addr)        addr
#define HalUtilMIU2PHYAddr(addr)        addr

#define msb250x_dma_dcache_flush(addr, size) _dma_cache_wback(addr, size)
#define msb250x_dma_dcache_invalidate(addr, size) _dma_cache_inv(addr, size)
#define msb250x_dma_dcache_flush_invalidate(addr, size) _dma_cache_wback_inv(addr, size)
#endif

#ifdef CONFIG_USB_MSB250X_DMA

#ifdef CONFIG_USB_MSB250X_DEBUG
#define DBG_MSG(x...) printk(KERN_INFO x)
#else
#define DBG_MSG(x...)
#endif

#ifndef SUCCESS
#define SUCCESS 1
#define FAILURE -1
#endif

extern int msb250x_udc_fifo_count(void);
extern int msb250x_udc_fifo_ctl_count(void);
extern int msb250x_udc_fifo_count_ep1(void);
extern int msb250x_udc_fifo_count_ep2(void);
extern int msb250x_udc_fifo_count_ep3(void);
extern int msb250x_udc_fifo_count_ep4(void);
extern int msb250x_udc_fifo_count_ep5(void);
extern int msb250x_udc_fifo_count_ep6(void);
extern void putb(char* string,int a,int b);//,int c,int d,int e,int f,int g,int h,int i,int j,int k,int l,int m);
/* Winder, we should think to reuse this inline functions with msb250x_udc.c */
/* --------------------- container_of ops ----------------------------------*/
static inline struct msb250x_ep *to_msb250x_ep(struct usb_ep *ep)
{
    return container_of(ep, struct msb250x_ep, ep);
}

static inline struct msb250x_udc *to_msb250x_udc(struct usb_gadget *gadget)
{
    return container_of(gadget, struct msb250x_udc, gadget);
}

static inline struct msb250x_request *to_msb250x_req(struct usb_request *req)
{
    return container_of(req, struct msb250x_request, req);
}
/* --------------------------------------------------------------------------*/

s8 free_dma_channels = 0x7f;

//u8 rx_dma_busy = DMA_NOT_BUSY;
//u8 tx_dma_busy = DMA_NOT_BUSY;

//u8 DmaRxMode1=0;
//u8 DmaTxMode1=0;
static void Set32BitsReg(u32 volatile* Reg,u32 val)
{
    udc_write16((u16)(val & 0xffff), (u32)Reg);
    udc_write16((u16)(val>>16), (u32)(Reg+1));
    DBG_MSG( "write val:%x ",(u16)(val >> 16));
    DBG_MSG( "  *(Reg+4):%x\n",*(Reg+4));
}

u32 Get32BitsReg(u32 volatile* Reg)
{
     return (udc_read16((u32)Reg)&0xffff)|(udc_read16((u32)(Reg+1))<<16);
}

//extern s8 dma_busy;
static inline s8 Get_DMA_Channel(void)
{
    s8 i, bit;
//    unsigned long        flags;

    /* cyg_semaphore_wait(&ChannelDmaSem); */
    for(i = 0, bit = 1; i <  MAX_USB_DMA_CHANNEL; i++, bit <<= 1)
    {
        if(free_dma_channels & bit)
        {
            free_dma_channels &= ~bit;
            DBG_MSG( "Get Channel:%x ",i+1);
            /* cyg_semaphore_post(&ChannelDmaSem); */
			//printk("set DMA busy\n");
			
		//	local_irq_save (flags);
            //dma_busy = DMA_BUSY;
			//ocal_irq_restore(flags);

			return i+1;
        }
    }

    DBG_MSG( "no channel");
    /* cyg_semaphore_post(&ChannelDmaSem); */
    return -EBUSY;
}
#if 1
void Release_DMA_Channel(s8 channel)
{
//    unsigned long        flags;

    DBG_MSG( "release channel:%x \n",channel);

    /* cyg_semaphore_wait(&ChannelDmaSem); */
    free_dma_channels |= (1 << (channel - 1));

    /* cyg_semaphore_post(&ChannelDmaSem); */
//	local_irq_save (flags);	
   // dma_busy = DMA_NOT_BUSY;
//	local_irq_restore(flags);
	
}
#endif

s8 check_dma_busy(void)
{
    s8 i, bit;

    for(i = 0, bit = 1; i <  MAX_USB_DMA_CHANNEL; i++, bit <<= 1)
    {
        if(free_dma_channels & bit)
        {
          

	   return DMA_NOT_BUSY;
        }
    }
	return DMA_BUSY;

}
//s8 check_tx_dma_busy(void)
//{
//	return tx_dma_busy;
//}
//s8 check_rx_dma_busy(void)
//{
  //  return rx_dma_busy;
//}
u8 *buf=NULL;
u8 *ppp=NULL;
u32 buff_ttt[20000];
int size_ttt;
u32 add_ttt;
int ii=0;
//unsigned long	val=0,val2=0,ms=0;
extern int xx;
extern void Chip_Flush_Memory(void);
extern void Chip_Flush_Cache_Range_VA_PA(unsigned long u32VAddr,unsigned long u32PAddr,unsigned long u32Size);
extern void Chip_Inv_Cache_Range_VA_PA(unsigned long u32VAddr,unsigned long u32PAddr,unsigned long u32Size);
extern void Chip_Inv_Cache_Range(unsigned long u32Addr, unsigned long u32Size);
extern void Chip_Clean_Cache_Range_VA_PA(unsigned long u32VAddr,unsigned long u32PAddr,unsigned long u32Size);
s8 USB_Set_DMA(struct usb_ep *_ep, struct msb250x_request *req, u32 count, u32 mode)
{
    s8 ch;
    u32 /*index,*/ csr2=0;
#ifdef 	RX_modify_mode1
	u16 packnum;
#endif
    u16 control;
    u32 address;
    u32 fiforemain=0;
    s8  idx;
	//u8	*test_buf;
    struct msb250x_ep *ep = to_msb250x_ep(_ep);
	
    address = (u32)(req->req.buf) + (u32)(req->req.actual);
    idx = ep->bEndpointAddress & 0x7F;

    DBG_MSG("ep %d count %x  dma req.actual %x \n", idx, count, req->req.actual);
#ifdef	RX_mode1_log
	printk("ep %d count %x dma actual %x \n", idx, count, req->req.actual);
#endif

	//printk("ep %d count %x dma actual %x \n", idx, count, req->req.actual);
    if(count < MIN_DMA_TRANSFER_BYTES)
    {
    	printk("count < MIN_DMA_TRANSFER_BYTES\n");
        /* packet is too small just use FIFO mode */
        return -EOPNOTSUPP;
    }

    /* save and set index register */
    //index = udc_read8(MSB250X_UDC_INDEX_REG);
    //udc_write8(idx, MSB250X_UDC_INDEX_REG);

   //if (mode & DMA_TX)
   	ch=Get_DMA_Channel();
   //else
   	//ch=Get_RX_DMA_Channel();
   
    if(ch < 0)       /* no free channel */
    {
        printk( "Get DMA channel fail %d\n", free_dma_channels);
        return -EBUSY;
    }


    /* for multiple Bulk packets, set Mode 1 */
    if (count > _ep->maxpacket)
    {
        mode |= DMA_MODE_ONE;
    }
    else /* mode 0 */
    {
        if(mode & DMA_TX)
        {
            u16 ep_maxpacket;

            ep_maxpacket = _ep->maxpacket;

            count = min((u16)ep_maxpacket, (u16)count);
        }
        else
        {
            //if(idx)
            //    fiforemain = msb250x_udc_fifo_count();
            if(idx==1)
				fiforemain = msb250x_udc_fifo_count_ep1();
			else if(idx==2)
				fiforemain = msb250x_udc_fifo_count_ep2();
			else if(idx==3)
				fiforemain = msb250x_udc_fifo_count_ep3();

            count = min(fiforemain, count);
        }
		#if 0
    	while((udc_read8(MSB250X_UDC_TXCSR1_REG)&(MSB250X_UDC_TXCSR1_TXPKTRDY|MSB250X_UDC_TXCSR1_FIFONOEMPTY))!=0)
    	{
    		printk("@@@@\n");
    	}
		#endif
        mode &= ~DMA_MODE_ONE;
        DBG_MSG("count less than maxpacket.\n");
    }
    /* flush and invalidate data cache */


    /* prepare DMA control register */
    control = DMA_ENABLE_BIT | mode | (idx << DMA_ENDPOINT_SHIFT) | (DMA_BurstMode<<9);

	size_ttt=count;

	Chip_Flush_Cache_Range_VA_PA(address,HalUtilPHY2MIUAddr(virt_to_phys((void *)address)),count);
	//Chip_Inv_Cache_Range(address,count);
	//Chip_Inv_Cache_Range_VA_PA(address,HalUtilPHY2MIUAddr(virt_to_phys((void *)address)),count);	
#if 0	
	if(idx==2)
	{
		int a=0;
		test_buf=address;
		
		for(a=0;a<13;a++)
		{
			test_buf[a]=0x55;
			//printk("%x ",test_buf[a]);
		}
		Chip_Clean_Cache_Range_VA_PA(address,__pa(address), count);
		Chip_Inv_Cache_Range(address,count);//Chip_Inv_Cache_Range_VA_PA(address,__pa(address),size_ttt);
	}
	//printk("\n");
#endif	
	//if(mode & DMA_TX)
		Chip_Clean_Cache_Range_VA_PA(address,__pa(address), count);
	//else
		Chip_Inv_Cache_Range(address,count);//Chip_Inv_Cache_Range_VA_PA(address,__pa(address),size_ttt);

	Set32BitsReg((u32 volatile*)DMA_ADDR_REGISTER(ch), HalUtilPHY2MIUAddr(virt_to_phys((void *)address)));
    Set32BitsReg((u32 volatile*)DMA_COUNT_REGISTER(ch), count);
    /* program DRC registers */

	//putb("set dma",ep->bEndpointAddress,0);
    switch(mode & DMA_MODE_MASK)
    {
        case DMA_RX | DMA_MODE_ZERO:
			if(idx==2)
				udc_write8(udc_read8((MSB250X_USBCREG(0x126))+1) & ~0x20,(MSB250X_USBCREG(0x126))+1);
			else if(idx==4)
				udc_write8(udc_read8((MSB250X_USBCREG(0x146))+1) & ~0x20,(MSB250X_USBCREG(0x146))+1);
			//udc_write8(udc_read8(MSB250X_UDC_RXCSR2_REG) & ~0x20,MSB250X_UDC_RXCSR2_REG);
            DBG_MSG( "1_ Rx_0 ep: %x, count = %x, Request = %x, control = %x\n",idx,count,req->req.length,control);
            //printk( " Rx_0 ep: %x, count = %x, Request = %x, control = %x\n",idx,count,req->req.length,control);
			
            //csr2 = udc_read8(MSB250X_UDC_RXCSR2_REG);
            //Enable_RX_EP_Interrupt(idx);
            //udc_write8((csr2 & ~RXCSR2_MODE1), MSB250X_UDC_RXCSR2_REG);
            if(idx==2)
            {
            	csr2 = udc_read8((MSB250X_USBCREG(0x126))+1);
            	Enable_RX_EP_Interrupt(idx);
            	udc_write8((csr2 & ~RXCSR2_MODE1), (MSB250X_USBCREG(0x126))+1);            
            }
			else if(idx==4)
			{
            	csr2 = udc_read8((MSB250X_USBCREG(0x146))+1);
            	Enable_RX_EP_Interrupt(idx);
            	udc_write8((csr2 & ~RXCSR2_MODE1), (MSB250X_USBCREG(0x146))+1);      			
			}
            break;

        case DMA_TX | DMA_MODE_ZERO:
    	#if 0
    	//tick_start = HalTimerRead(TIMER_FREERUN_XTAL);
    	while((udc_read8(MSB250X_UDC_TXCSR1_REG)&(MSB250X_UDC_TXCSR1_TXPKTRDY|MSB250X_UDC_TXCSR1_FIFONOEMPTY))!=0)
    	{
    		//tick_end = HalTimerRead(TIMER_FREERUN_XTAL);
			//timeout=tick_start-tick_end;
			//if(timeout > (100*1000)/26)			//100 us , 1 tick=26ns
			{
				//printk("[USB]Polling timeout\n");
				//break;
			}
    		if (g_ptKePmu->reg_chrg_pgin==0)
    		{
    			printk("[USB]USB disconnect\n");
				break;
    		}
    	}
     	//csr2 = udc_read8(MSB250X_UDC_TXCSR2_REG);
        //udc_write8((csr2 & ~TXCSR2_MODE1), MSB250X_UDC_TXCSR2_REG);
		#endif			
		
            DBG_MSG( "2_ Tx_0 ep: %x, buff = %x, count = %x, Request = %x, control = %x\n",idx, address,count,req->req.length,control);
			//printk( "Tx_0 count = %x, Request = %x, control = %x\n",count,req->req.length,control);
			Enable_TX_EP_Interrupt(idx);
            //csr2 = udc_read8(MSB250X_UDC_TXCSR2_REG);
            //udc_write8((csr2 & ~TXCSR2_MODE1), MSB250X_UDC_TXCSR2_REG);
            if(idx==1)
            {
            	csr2 = udc_read8((MSB250X_USBCREG(0x112))+1);
            	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x112))+1);            
            }
			else if(idx==3)
			{
            	csr2 = udc_read8((MSB250X_USBCREG(0x132))+1);
            	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x132))+1);    			
			}
			else if(idx==5)
			{
            	csr2 = udc_read8((MSB250X_USBCREG(0x152))+1);
            	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x152))+1);    			
			}
			else if(idx==6)
			{
            	csr2 = udc_read8((MSB250X_USBCREG(0x162))+1);
            	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x162))+1);    			
			}
            break;

        case DMA_RX | DMA_MODE_ONE:
			//udc_write8(udc_read8(MSB250X_UDC_RXCSR2_REG) & ~0x20,MSB250X_UDC_RXCSR2_REG);
			if(idx==2)
			{		
				udc_write8(udc_read8((MSB250X_USBCREG(0x126))+1) & ~0x20,(MSB250X_USBCREG(0x126))+1);
				csr2 = udc_read8((MSB250X_USBCREG(0x126))+1);
			}
			else if(idx==4)
			{
				udc_write8(udc_read8((MSB250X_USBCREG(0x146))+1) & ~0x20,(MSB250X_USBCREG(0x146))+1);
				csr2 = udc_read8((MSB250X_USBCREG(0x146))+1);
			}
            DBG_MSG( "3_ Rx_1 ep: %x, count = %x, Request = %x, ch = %x\n",idx,count,req->req.length,ch);
            //csr2 = udc_read8(MSB250X_UDC_RXCSR2_REG);
            udc_write16(control, (u32)DMA_CNTL_REGISTER(ch));
            Enable_RX_EP_Interrupt(idx);

            //udc_write8((csr2 | RXCSR2_MODE1| MSB250X_UDC_RXCSR2_DISNYET), MSB250X_UDC_RXCSR2_REG);
            if(idx==2)
				udc_write8((csr2 | RXCSR2_MODE1| MSB250X_UDC_RXCSR2_DISNYET), (MSB250X_USBCREG(0x126))+1);
			else if(idx==4)
				udc_write8((csr2 | RXCSR2_MODE1| MSB250X_UDC_RXCSR2_DISNYET), (MSB250X_USBCREG(0x146))+1);
            //udc_write8(0, MSB250X_UDC_RXCSR1_REG);//enable RX

            DBG_MSG("%s: DMA TX MODE1 Set DMA CTL\n",__FUNCTION__);
#ifdef RX_modify_mode1
			packnum=count/(_ep->maxpacket);
			if (count % _ep->maxpacket)
				packnum+=1;
			if(((ep->bEndpointAddress)&0x0f)==2){
			  if (ep->RxShort==1)
		      {
			  udelay(125);	
			  ep->RxShort=0;
			}
              udc_write16((M_Mode1_P_OK2Rcv|M_Mode1_P_NAK_Enable|packnum), MSB250X_UDC_DMA_MODE_CTL);
			  //putb("rx1 allow ack",udc_read8(MSB250X_UDC_DMA_MODE_CTL),udc_read8(MSB250X_UDC_USB_CFG5_H));
			  //putb("epnum",udc_read8(MSB250X_UDC_EP_BULKOUT),packnum);
			  //putb("RXCSR",udc_read8(MSB250X_UDC_RXCSR1_REG),0);
			}
			
			else if(((ep->bEndpointAddress)&0x0f)==4)
			{
        			if (ep->RxShort==1)
	        	      {
		       	  		udelay(125);	
			         ep->RxShort=0;
			       }

				udc_write16((packnum), MSB250X_UDC_USB_CFG1_L);
				udc_write8((M_Mode1_P_OK2Rcv_1|M_Mode1_P_NAK_Enable_1|M_Mode1_P_BulkOut_EP_4), MSB250X_UDC_DMA_MODE_CTL1);
			   //udc_write16((M_Mode1_P_OK2Rcv_1|M_Mode1_P_NAK_Enable_1|packnum), MSB250X_UDC_DMA_MODE_CTL1);
			}
            /* disable set_ok2rcv[15]&ECO4NAK_en[14],wayne added */
            //udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|((count/(_ep->maxpacket)))), MSB250X_UDC_DMA_MODE_CTL);
            /* enable set_ok2rcv[15]&ECO4NAK_en[14],wayne added */

			//printk("[USB]packet:%x\n",(count/(_ep->maxpacket)));
           // udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_OK2Rcv), MSB250X_UDC_DMA_MODE_CTL);
#endif
#ifdef RX_modify_mode1
#ifdef RX_mode1_log	
			printk( "[USB]Set RX1,count = %x, Request = %x, ch = %x\n",count,req->req.length,ch);
#endif
			//putb("SET_RX1",count,req->req.length);
			ep->DmaRxMode1=1;
			ep->ch=ch;
			//printk("[USB]MSB250X_UDC_DMA_MODE_CTL%x\n",udc_read16(MSB250X_UDC_DMA_MODE_CTL));
            //printk("C:%x\n",count);

#endif
            /* restore previous index value */
            //udc_write8(index, MSB250X_UDC_INDEX_REG);
            return SUCCESS;

        case DMA_TX | DMA_MODE_ONE:
            DBG_MSG( "4_ Tx_1 ep: %x, count = %x, Request = %x, control = %x\n",idx,count,req->req.length,control);
#ifdef TX_log
            printk( "DMA Tx_1 count = %x, Request = %x, ch = %x\n",count,req->req.length,ch);
#endif
			//putb("TX1",0,0);
            Enable_TX_EP_Interrupt(idx);
            //csr2 = udc_read8(MSB250X_UDC_TXCSR2_REG);
            //udc_write8((csr2 | TXCSR2_MODE1), MSB250X_UDC_TXCSR2_REG);
            if(idx==1)
            {
            	csr2 = udc_read8((MSB250X_USBCREG(0x112))+1);
            	udc_write8((csr2 | TXCSR2_MODE1), (MSB250X_USBCREG(0x112))+1);            
            }
			else if(idx==3)
			{
            	csr2 = udc_read8((MSB250X_USBCREG(0x132))+1);
            	udc_write8((csr2 | TXCSR2_MODE1), (MSB250X_USBCREG(0x132))+1);   			
			}
#if 0
if(count>=200)
{
int ii=0;
for(ii=0;ii<=12;ii++)
	printk("%x ",buf[ii]);
printk("\n");
}	
#endif
            break;
    }

    /* restore previous index value */
    //udc_write8(index, MSB250X_UDC_INDEX_REG);
    udc_write16(control, (u32)DMA_CNTL_REGISTER(ch));
	
    return SUCCESS;
}
extern int xx;
//extern int tmp_buff;
int test_start=0;

/*static */extern struct msb250x_request * msb250x_udc_do_request(struct msb250x_ep *ep, struct msb250x_request *req);
void USB_DMA_IRQ_Handler(u8 ch, struct msb250x_udc *dev)
{
    u8  /*index,*/ endpoint, direction;
    u32 csr, mode, bytesleft, addr, bytesdone, control;
    u8  csr2,csr2_tmp=0;
    u32 lastpacket = 0;
    u32 fiforemain;
	//u32 *prt;
    struct msb250x_ep *ep;
    struct msb250x_request	*req = NULL;
	//unsigned int tick_start,tick_end,timeout;
	u32 address;
    /* get DMA Mode, address and byte counts from DMA registers */
    control = udc_read16((u32)(DMA_CNTL_REGISTER(ch)));
    mode = control & 0xf;

    addr = Get32BitsReg((u32 volatile*)DMA_ADDR_REGISTER(ch));
    bytesleft =Get32BitsReg((u32 volatile*)DMA_COUNT_REGISTER(ch));
#ifdef RX_mode1_log
	printk("bytesleft:%x\n",bytesleft);
#endif
    /* get endpoint, URB pointer */
    endpoint = (udc_read16((u32)(DMA_CNTL_REGISTER(ch))) & 0xf0) >> DMA_ENDPOINT_SHIFT;
    direction = (mode & DMA_TX) ? 0 : 1;

    ep = &dev->ep[endpoint];

    if (likely (!list_empty(&ep->queue)))
        req = list_entry(ep->queue.next, struct msb250x_request, queue);
    else
        req = NULL;

    if (!req)
    {
    	//prt=(u32*)list_entry(ep->queue.next, struct msb250x_request, queue);
        printk("no request but DMA done?!\n");
		//printk("flag:%x\n",xx);
		printk("ep %x  left %x \n", endpoint, bytesleft);
        return;
    }
#if 0	
	////////test///////////
	if((ep->bEndpointAddress & 0x7F)==2)
	{
		//putb("DMA_RX_INT",0,0);
		test_start = HalTimerRead(TIMER_FREERUN_XTAL);
	}
	////////test///////////
#endif	
    DBG_MSG("DMA done__ ep %d\n", endpoint);\
	//buf=(u32)(req->req.buf + req->req.actual);
	address = (u32)(req->req.buf) + (u32)(req->req.actual);
	//ppp=virt_to_phys((u32)(req->req.buf + req->req.actual));
	//putb("done",ep->bEndpointAddress,0);
	//putb("buf",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[20]);

	Chip_Inv_Cache_Range(address,size_ttt);//Chip_Inv_Cache_Range_VA_PA(address,__pa(address),size_ttt);
	#if 0
	if((ep->bEndpointAddress & 0x7F)==2)
	{		
		if(buf[0]==0x55)
		{int kk=0;	
			//Chip_Inv_Cache_Range_VA_PA(buf,__pa(buf),size_ttt);
			printk("buf:\n");
			for(kk=0;kk<=31;kk++)
			{	
				printk("%x",buf[kk]);
			}
			printk("\n");
			//printk("ppp:\n");
			//for(kk=0;kk<=31;kk++)
			//{	
			//	printk("%x",ppp[kk]);
			//}
			//printk("\n");		
			while(1);
		}
	}
	#endif
    /* how many bytes were processed ? */
    bytesdone = (u32) phys_to_virt((u32)HalUtilMIU2PHYAddr(addr) - (u32)(req->req.buf + req->req.actual));
#ifdef RX_mode1_log	
	printk("@@@bytesdone:%x\n",bytesdone);
#endif
    DBG_MSG("irq-- data: %02x %02x %02x \n", *((u8 *)req->req.buf), *((u8 *)req->req.buf + 1), *((u8 *)req->req.buf + 2));

    req->req.actual += bytesdone;

    DBG_MSG("ep %d ac %x lt %x \n", endpoint, bytesdone, bytesleft);

    /* save and set index register */
    //index = udc_read8(MSB250X_UDC_INDEX_REG);
    //udc_write8((ep->bEndpointAddress & 0x7F), MSB250X_UDC_INDEX_REG);

    /* release DMA channel */
    Release_DMA_Channel(ch);

    /* clean DMA setup in CSR  */
    if (mode & DMA_TX)
    {
    	#if 1
		if((ep->bEndpointAddress & 0x7F)==1)
			csr2_tmp=udc_read8(MSB250X_USBCREG(0x112));
		else if((ep->bEndpointAddress & 0x7F)==3)
			csr2_tmp=udc_read8(MSB250X_USBCREG(0x132));
		else
			printk("ERROR\n");
		
    	//tick_start = HalTimerRead(TIMER_FREERUN_XTAL);
    	//while((udc_read8(MSB250X_UDC_TXCSR1_REG)&(MSB250X_UDC_TXCSR1_TXPKTRDY|MSB250X_UDC_TXCSR1_FIFONOEMPTY))!=0)
#if 1
		while((csr2_tmp&(MSB250X_UDC_TXCSR1_TXPKTRDY|MSB250X_UDC_TXCSR1_FIFONOEMPTY))!=0)
    	{
    		//tick_end = HalTimerRead(TIMER_FREERUN_XTAL);
			//timeout=tick_start-tick_end;
			//if(timeout > (300*1000)/26)			//300 us , 1 tick=26ns
			//{
				//printk("[USB]Polling timeout:%x\n",timeout);
				//break;
			//}
    		//if (g_ptKePmu->reg_chrg_pgin==0)
    		//{
    		//	printk("[USB]USB disconnect\n");
			//	break;
    		//}
			if((ep->bEndpointAddress & 0x7F)==1)
				csr2_tmp=udc_read8(MSB250X_USBCREG(0x112));
			else if((ep->bEndpointAddress & 0x7F)==3)
				csr2_tmp=udc_read8(MSB250X_USBCREG(0x132));
			else
				printk("error\n");
    	}
#endif	
		if((ep->bEndpointAddress & 0x7F)==1)
		{
     		csr2 = udc_read8((MSB250X_USBCREG(0x112))+1);
        	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x112))+1);		
		}
		else if((ep->bEndpointAddress & 0x7F)==3)
		{
     		csr2 = udc_read8((MSB250X_USBCREG(0x132))+1);
        	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x132))+1);			
		}
		else if((ep->bEndpointAddress & 0x7F)==5)
		{
     		csr2 = udc_read8((MSB250X_USBCREG(0x152))+1);
        	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x152))+1);			
		}
		else if((ep->bEndpointAddress & 0x7F)==6)
		{
     		csr2 = udc_read8((MSB250X_USBCREG(0x162))+1);
        	udc_write8((csr2 & ~TXCSR2_MODE1), (MSB250X_USBCREG(0x162))+1);			
		}
     	//csr2 = udc_read8(MSB250X_UDC_TXCSR2_REG);
        //udc_write8((csr2 & ~TXCSR2_MODE1), MSB250X_UDC_TXCSR2_REG);
		#endif
    }
    else /* DMA RX */
    {
    	if((ep->bEndpointAddress & 0x7F)==2)
    	{
        	csr2 = udc_read8((MSB250X_USBCREG(0x126))+1);
        	udc_write8((csr2 & ~RXCSR2_MODE1), (MSB250X_USBCREG(0x126))+1);    	
    	}
		else if((ep->bEndpointAddress & 0x7F)==4)
		{
        	csr2 = udc_read8((MSB250X_USBCREG(0x146))+1);
        	udc_write8((csr2 & ~RXCSR2_MODE1), (MSB250X_USBCREG(0x146))+1);		
		}
        //csr2 = udc_read8(MSB250X_UDC_RXCSR2_REG);
        //udc_write8((csr2 & ~RXCSR2_MODE1), MSB250X_UDC_RXCSR2_REG);
    }

    /* Bus Error */
    if (control & DMA_BUSERROR_BIT)
    {
        printk(KERN_ERR "DMA Bus ERR\n");

        ep->halted = 1; /* Winder */

        return;
    }

    if (mode & DMA_TX)
    {
		//DmaTxMode1=0;
        if (req->req.actual == req->req.length)
        {
            if ((req->req.actual % ep->ep.maxpacket) || ((mode & DMA_MODE_ONE)==0)) /* short packet || TX DMA mode0 */
            {
               if (mode & DMA_MODE_ONE)
               	{  
#ifdef sean_modify_log
					printk("DMA_TX mode1 short packet\n");
#endif
               	}
                lastpacket = 1; /* need to set TXPKTRDY manually */
            }
            else  /* the last packet size is equal to MaxEPSize */
            {	

                msb250x_udc_done(ep, req, 0);
				//xx=0;
                /* if DMA busy, it will use FIFO mode, do no one will catually wait DMA */
                /* msb250x_udc_schedule_DMA(ep); */

                /*
                 * Because there is no TX interrtup follow TX mode1 & big packet so we
                 * have to schedule next TX Req there
                 */
                if(mode & DMA_MODE_ONE)
                    msb250x_udc_schedule_done(ep);
//udelay(150);
                return;
            }
        }
    }
    else /* DMA RX */
    {
#ifdef RX_mode1_log	            
        printk("[USB]DMA_IRQ_Handler_RX\n");
#endif	    
        Enable_RX_EP_Interrupt(endpoint);		//????
        fiforemain = bytesleft;
#if 0		
{ 
memcpy(add_ttt,buff_ttt,size_ttt);
//ttt = buff; 
} 
#endif		
        if (fiforemain == 0)
        {
            if (req->req.actual % ep->ep.maxpacket) /* short packet */
            {
                lastpacket = 1;
            }
            else  /* the last packet size is equal to MaxEPSize */
            {
                if (mode & DMA_MODE_ONE)
                {
#ifdef NAK_MODIFY
					//udc_write8((udc_read8(MSB250X_UDC_USB_CFG6_H)|0x20),MSB250X_UDC_USB_CFG6_H);
					//printk("[USB]revpkt:%x\n",udc_read16(MSB250X_UDC_DMA_MODE_CTL));

					//udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)&~M_Mode1_P_OK2Rcv), MSB250X_UDC_DMA_MODE_CTL);
					if(((ep->bEndpointAddress)&0x0f)==2)
						udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_NAK_Enable), MSB250X_UDC_DMA_MODE_CTL);
					else if(((ep->bEndpointAddress)&0x0f)==4)
					 	udc_write8((udc_read8(MSB250X_UDC_DMA_MODE_CTL1)|M_Mode1_P_NAK_Enable_1), MSB250X_UDC_DMA_MODE_CTL1);
					//udc_write8((udc_read8(MSB250X_UDC_USB_CFG6_H)&~0x20),MSB250X_UDC_USB_CFG6_H);				
					//printk("IRQ DMA Mode:%x\n",udc_read16(MSB250X_UDC_DMA_MODE_CTL));
					//printk("bytesleft:%x\n",bytesleft);
					
					//tick_start = HalTimerRead(TIMER_FREERUN_XTAL);
					while((udc_read8(MSB250X_UDC_USB_CFG7_H)&0x80)==0)	//last done bit
					{
						//tick_end = HalTimerRead(TIMER_FREERUN_XTAL);
						//timeout=tick_start-tick_end;
						//if(timeout > (100*1000)/26)			//100 us , 1 tick=26ns
						//{
						//	printk("Polling last done bit timeout:%x\n",timeout);
							//break;
						//}
						//if (g_ptKePmu->reg_chrg_pgin==0)
						//{
						//	printk("USB disconnect\n");
						//	break;
						//}
						printk("[USB]Last done bit\n");
					}
#endif      

#ifdef RX_modify_mode1
					ep->DmaRxMode1=0;
#endif
                    msb250x_udc_done(ep, req, 0);

                    /* if DMA busy, it will use FIFO mode, no one will catually wait DMA */
                    /* msb250x_udc_schedule_DMA(ep); */

                    /*
                     * Because there is no RX interrtup follow RX mode1 & big packet so we
                     * have to schedule next RX Req there
                      */
                    msb250x_udc_schedule_done(ep);
					//val2=jiffies;
					//ms=val2 - val;
					//ms=jiffies_to_milliseconds(val2 - val);
					//ms=ms*1000*1000/HZ;
					//ms= jiffies_to_usecs(val2 - val);
					//printk("[USB]tick:%ld\n",ms);			
//udelay(150); 
                    return;
                }
                else
                {
                     DBG_MSG("DMA rx mode 0 partial done \n");
#ifdef RX_mode1_log					 
					 printk("DMA rx mode 0 partial done \n");
#endif
					 if((ep->bEndpointAddress & 0x7F)==2)
					 {
                     	csr = udc_read8(MSB250X_USBCREG(0x126));
                     	udc_write8((csr & ~MSB250X_UDC_RXCSR1_RXPKTRDY), MSB250X_USBCREG(0x126));					 
					 }

                     //csr = udc_read8(MSB250X_UDC_RXCSR1_REG);
                     //udc_write8((csr & ~MSB250X_UDC_RXCSR1_RXPKTRDY), MSB250X_UDC_RXCSR1_REG);

                     /* restore the index */
                     //udc_write8(index, MSB250X_UDC_INDEX_REG);

                     /* Buffer Full */
					 

                     if(req->req.actual == req->req.length)			//finished
                     {
                         DBG_MSG("buff full len %x \n", req->req.actual);
                         msb250x_udc_done(ep, req, 0);
						 //printk("mode 0 done->do request\n");
						 msb250x_udc_schedule_done(ep);
						 
                     }
#ifdef RX_modify_mode1
					 else
				 	 {
#ifdef RX_mode1_log	            
						printk("[USB]do_request for following mode1 packet\n");
#endif		 	 
				 	 	//do_request
				 	 	msb250x_udc_do_request(ep, req);
			      	 }
#endif					 
                     /* Do not call this becasue if DMA busy, it will use FIFO mode, do no one will catually wait DMA */
                     /* msb250x_udc_schedule_DMA(ep); */
//udelay(150);
                     return;
                }
            }
        }
    }

    /*  for short packet, CPU needs to handle TXPKTRDY/RXPKTRDY bit  */
    if (lastpacket)
    {
#ifdef RX_mode1_log	            
        printk("[USB]DMA_IRQ_Handler_shortpacket\n");
#endif    
        if (mode & DMA_TX)
        {
            //udc_write8(MSB250X_UDC_TXCSR1_TXPKTRDY, MSB250X_UDC_TXCSR1_REG);
            if((ep->bEndpointAddress & 0x7F)==1)
				udc_write8(MSB250X_UDC_TXCSR1_TXPKTRDY, MSB250X_USBCREG(0x112));
			else if((ep->bEndpointAddress & 0x7F)==3)
				udc_write8(MSB250X_UDC_TXCSR1_TXPKTRDY, MSB250X_USBCREG(0x132));
			else if((ep->bEndpointAddress & 0x7F)==5)
				udc_write8(MSB250X_UDC_TXCSR1_TXPKTRDY, MSB250X_USBCREG(0x152));
			else if((ep->bEndpointAddress & 0x7F)==6)
				udc_write8(MSB250X_UDC_TXCSR1_TXPKTRDY, MSB250X_USBCREG(0x162));
            DBG_MSG( "DMA TXONE2\n");
#ifdef TX_log				
			printk("DMA_TX TX_PACKET_READY\n");
#endif
        }
        else
        {
            //csr = udc_read8(MSB250X_UDC_RXCSR1_REG);
            DBG_MSG( "DMARXCSR : %x \n",csr);
            //udc_write8((csr & ~MSB250X_UDC_RXCSR1_RXPKTRDY), MSB250X_UDC_RXCSR1_REG);
            if((ep->bEndpointAddress & 0x7F)==2)
            {
            	csr = udc_read8(MSB250X_USBCREG(0x126));
				udc_write8((csr & ~MSB250X_UDC_RXCSR1_RXPKTRDY), MSB250X_USBCREG(0x126));
            }
			else if((ep->bEndpointAddress & 0x7F)==4)
			{
            	csr = udc_read8(MSB250X_USBCREG(0x146));
				udc_write8((csr & ~MSB250X_UDC_RXCSR1_RXPKTRDY), MSB250X_USBCREG(0x146));			
			}
#ifdef TX_modify		
	 	//	msb250x_udc_done(ep, req, 0);	//rx need to do done, tx has interupt to notify
#endif
        }
#ifndef TX_modify
        msb250x_udc_done(ep, req, 0);
#endif
        /* Do not call this becasue if DMA busy, it will use FIFO mode, do no one will catually wait DMA */
        /* msb250x_udc_schedule_DMA(ep); */
    }

    /* restore the index */
    //udc_write8(index, MSB250X_UDC_INDEX_REG);
//udelay(150);
    return;
}

void USB_DisableDMAChannel(s8 channel)
{
    u16 control;

    control = udc_read16((u32)(DMA_CNTL_REGISTER(channel)));
    control &= (u16)~DMA_ENABLE_BIT;
    udc_write16(control, (u32)DMA_CNTL_REGISTER(channel));
}

void USB_DisableDMAMode1(void)
{	
    udc_write16(udc_read16(MSB250X_UDC_DMA_MODE_CTL)&~M_Mode1_P_Enable, MSB250X_UDC_DMA_MODE_CTL);
}

void USB_ResetDMAMode(void)
{
    udc_write16(0, MSB250X_UDC_DMA_MODE_CTL); //disable set_ok2rcv[15]&ECO4NAK_en[14],wayne added
}

void USB_EnableDMA(void)
{
    USB_DisableDMAChannel(MAX_USB_DMA_CHANNEL);
    USB_ResetDMAMode();
    udc_write16((udc_read16(MSB250X_UDC_EP_BULKOUT)|M_Mode1_P_BulkOut_EP), MSB250X_UDC_EP_BULKOUT);
    udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_Enable|M_Mode1_P_AllowAck), MSB250X_UDC_DMA_MODE_CTL); // Allow ACK
}

void USB_DisableDMA(void)
{
    USB_ResetDMAMode();
    USB_DisableDMAChannel(MAX_USB_DMA_CHANNEL);
}

void Control_EP_Interrupt(s8 ep, u32 mode)
{
    u32 reg, current_reg, bit;
    u8 endpoint;

    endpoint = ep;

    if(mode & EP_IRQ_TX)
        reg = (endpoint < 8) ? MSB250X_UDC_INTRTX1E_REG : MSB250X_UDC_INTRTX2E_REG;
    else
        reg = (endpoint < 8) ? MSB250X_UDC_INTRRX1E_REG : MSB250X_UDC_INTRRX2E_REG;


    current_reg = udc_read8(reg);

    bit = 1 << (endpoint % 8);

    if(mode & EP_IRQ_ENABLE)
        udc_write8((current_reg | bit), reg);
    else
        udc_write8((current_reg & ~bit), reg);

}

void USB_Set_ClrRXMode1(void)
{printk("[USB]999\n");
    DBG_MSG("USB_Set_ClrRXMode1\n");
    //udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)&~M_Mode1_P_OK2Rcv), MSB250X_UDC_DMA_MODE_CTL); //disable set_ok2rcv[15]&ECO4NAK_en[14],wayne added
	udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_OK2Rcv), MSB250X_UDC_DMA_MODE_CTL);
	udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_AllowAck), MSB250X_UDC_DMA_MODE_CTL); //enable Allow ok,wayne added
}

u16 USB_Read_DMA_Control(s8 nChannel)
{
    return *((DMA_CNTL_REGISTER(nChannel)));
}

enum DMA_RX_MODE_TYPE msb250_udc_set_dma_rx_by_name(const char * dev_name)
{
    if(!strcmp(dev_name,"g_ether"))
    {
        DBG_MSG("DMA_CONFIG: %s with dma_rx_mode0 \n", dev_name);
        return DMA_RX_MODE0;
    }

    if(!strcmp(dev_name,"g_file_storage"))
    {
        DBG_MSG("DMA_CONFIG: %s with dma_rx_mode1 \n", dev_name);
        return DMA_RX_MODE1;
    }

    DBG_MSG("DMA_CONFIG: %s with NO RX DMA \n", dev_name);
    return DMA_RX_MODE1;//DMA_RX_MODE_NULL;
}

EXPORT_SYMBOL_GPL(USB_Set_ClrRXMode1);
#endif /* CONFIG_USB_MSB250X_DMA */

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("ms_dma");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

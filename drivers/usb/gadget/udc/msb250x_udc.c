/*------------------------------------------------------------------------------
    Copyright (c) 2010 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
    PROJECT: MSB250x Linux BSP
    DESCRIPTION:
          MSB250x dual role USB device controllers


    HISTORY:
         6/11/2010     Calvin Hung    First Revision

-------------------------------------------------------------------------------*/


/******************************************************************************
 * Include Files
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
//#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h> // Mstar OTG operation
#include <linux/cdev.h>
#include <asm/uaccess.h>        /* copy_*_user */

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
//#include <asm/system.h>
#include <asm/unaligned.h>
//#include <mach/irqs.h>

//#include <mach/hardware.h>
#include <asm/mach-types.h>

#if defined(CONFIG_USB_MSB250X_MODULE)
#define CONFIG_USB_MSB250X 1
#endif
#if defined(CONFIG_USB_MSB250X_DMA_MODULE)
#define CONFIG_USB_MSB250X_DMA 1
#endif
#if defined(CONFIG_USB_MS_OTG_MODULE)
#define CONFIG_USB_MS_OTG 1
#endif
#if defined(CONFIG_USB_GADGET_MSB250X_MODULE)
#define CONFIG_USB_GADGET_MSB250X 1
#endif


#include "msb250x_udc.h"
#include "msb250x_udc_reg.h"
#include "msb250x_udc_ioctl.h"
#include "ms_usbmain.h"
#include "ms_otg.h"
#include "ms_gvar.h"
#ifdef CONFIG_USB_MSB250X_DMA
#include "ms_dma.h"
#include <linux/mm.h>
#endif
//#include "mach/irqs.h"
/******************************************************************************
 * Constants
 ******************************************************************************/
//#define MSB250X_UDC_DEBUG 0
#if 1//def MSB250X_UDC_DEBUG
//static const char *ep0states[]=
//{
//    "EP0_IDLE",
//    "EP0_IN_DATA_PHASE",
//    "EP0_OUT_DATA_PHASE",
//    "EP0_END_XFER",
//    "EP0_STALL",
//};
#define DBG(x...) 
#else
#define DBG(x...)	printk(KERN_INFO x)
#endif

#define UDC_DEBUG_TX_RX 0
#if UDC_DEBUG_TX_RX
#define DBG_TR(x...) printk(KERN_INFO x)
#else
#define DBG_TR(x...)
#endif
#if defined(CONFIG_ARCH_CEDRIC)
/* the OTG IRQ number */
#define INT_MS_OTG (2048 + 64 + 5)//INT_IRQ_OTG
#elif defined(CONFIG_ARCH_INFINITY)
#define INT_MS_OTG (96)//INT_IRQ_OTG
#elif defined(CONFIG_ARCH_INFINITY3)
#define INT_MS_OTG (96)//INT_IRQ_OTG
#endif
/* the module parameter */
#define DRIVER_DESC "MSB250x USB Device Controller Gadget"
#define DRIVER_VERSION "5 June 2010"
#define DRIVER_AUTHOR "mstarsemi.com"

int idx=0;
char array[900][32];
int array1[900];
int array2[900];
int array3[900];
int array4[900];
int array5[900];
int array6[900];
int array7[900];
int array8[900];
int array9[900];
int array10[900];
int array11[900];
int array12[900];
int array13[900];

#if 0
strcpy(array,"an apple");
printk("%s\n",array);
#endif
void putb(char* string,int a,int b)//,int c,int d,int e,int f,int g,int h,int i,int j,int k,int l,int m)
{
	strcpy(array[idx],string);
	array1[idx]=a;
	array2[idx]=b;
#if 0	
	array3[idx]=c;
	array4[idx]=d;
	array5[idx]=e;
	array6[idx]=f;
	array7[idx]=g;
	array8[idx]=h;
	array9[idx]=i;
	array10[idx]=j;
	array11[idx]=k;
	array12[idx]=l;
	array13[idx]=m;
#endif	
	idx++;
	if(idx==900)
		idx=0;
}
void printb(void)
{	
	int tmp=0,i;
	if((idx+1)==900)
		tmp=0;
	else 
		tmp=idx+1;
	for(i=tmp;i<900;i++)
		printk("[idx:%d]%s,%x,%x\n",i,array[i],array1[i],array2[i]);
	for(i=0;i<idx;i++)
		printk("[idx:%d]%s,%x,%x\n",i,array[i],array1[i],array2[i]);
	
}
//int tmp_buff;
/******************************************************************************
 * Variables
 ******************************************************************************/
static const char sg_gadget_name[] = "msb250x_udc";
struct msb250x_udc *sg_udc_controller=NULL;

#ifdef CONFIG_USB_MSB250X_DMA
static int using_dma = 0;
#endif

static int msb250x_udc_major =   MSB250X_UDC_MAJOR;
static int msb250x_udc_minor =   0;
static struct class *msb250x_udc_class;
static u16 old_linestate;
static u8 old_soft_conn;

//extern u32 g_charger_flag; // 0: USB cable; 1: Adapter

void msb250x_udc_done(struct msb250x_ep *ep,
        struct msb250x_request *req, int status);

module_param(msb250x_udc_major, int, S_IRUGO);
module_param(msb250x_udc_minor, int, S_IRUGO);


#define init_MUTEX(sem)		sema_init(sem, 1)
#define init_MUTEX_LOCKED(sem)	sema_init(sem, 0)

extern void msw8533x_clear_irq(int irq);
extern u32 Get32BitsReg(u32 volatile* Reg);

static struct work_struct usb_bh;
#define msb250x_udc_kick_intr_bh() schedule_work(&usb_bh)
static DEFINE_SPINLOCK(dev_lock);
#if 0
static inline void DMA_DUMP(void)
{
	u32 val1,val2,val3;
//	int i=0;	

	val1=Get32BitsReg((u32 *)DMA_COUNT_REGISTER(1));
	val2=Get32BitsReg((u32 *)DMA_ADDR_REGISTER(1));
	val3=Get32BitsReg((u32 *)DMA_CNTL_REGISTER(1));
	printk("ch_1:\n");
	printk("DMA_COUNT_REGISTER:%x,DMA_ADDR_REGISTER:%x,DMA_CNTL_REGISTER:%x\n",val1,val2,val3);
	printk("ch_2:\n");
	val1=0;val2=0;val3=0;
	val1=Get32BitsReg((u32 *)DMA_COUNT_REGISTER(2));
	val2=Get32BitsReg((u32 *)DMA_ADDR_REGISTER(2));
	val3=Get32BitsReg((u32 *)DMA_CNTL_REGISTER(2));	
	printk("DMA_COUNT_REGISTER:%x,DMA_ADDR_REGISTER:%x,DMA_CNTL_REGISTER:%x\n",val1,val2,val3);

	printk("CFG0:%x\n",udc_read16(MSB250X_UDC_USB_CFG0_L));
	printk("CFG5:%x\n",udc_read16(MSB250X_UDC_DMA_MODE_CTL));
	udc_write8(1, MSB250X_UDC_INDEX_REG);
	printk("TXCSR1:%x\n",udc_read8(MSB250X_UDC_TXCSR1_REG));
	printk("TXCSR2:%x\n",udc_read8(MSB250X_UDC_TXCSR2_REG));
	udc_write8(2, MSB250X_UDC_INDEX_REG);
	printk("RXCSR1:%x\n",udc_read8(MSB250X_UDC_RXCSR1_REG));
	printk("RXCSR2:%x\n",udc_read8(MSB250X_UDC_RXCSR2_REG));		
	printk("RXCOUNT_L:%x\n",udc_read8(MSB250X_UDC_RXCOUNT_L_REG));
	printk("RXCOUNT_H:%x\n",udc_read8(MSB250X_UDC_RXCOUNT_H_REG));		
	printk("[80]%x\n",udc_read16(MSB250X_USBCREG(0x80)));
	//printk("%x\n",udc_read16(MSB250X_USBCREG(0x40)+1));
	printk("[82]%x\n",udc_read16(MSB250X_USBCREG(0x82)));
	//printk("%x\n",udc_read16(MSB250X_USBCREG(0x42)+1));
	printk("[84]%x\n",udc_read16(MSB250X_USBCREG(0x84)));
	//printk("%x\n",udc_read16(MSB250X_USBCREG(0x44)+1));
	printk("[86]%x\n",udc_read16(MSB250X_USBCREG(0x86)));
	//printk("%x\n",udc_read16(MSB250X_USBCREG(0x46)+1));
	printk("[88]%x\n",udc_read16(MSB250X_USBCREG(0x88)));
	//printk("%x\n",udc_read16(MSB250X_USBCREG(0x48)+1));
	printk("[8a]%x\n",udc_read16(MSB250X_USBCREG(0x8a)));
	//printk("%x\n",udc_read16(MSB250X_USBCREG(0x4a)+1));
	printk("[8c]%x\n",udc_read16(MSB250X_USBCREG(0x8c)));
	printk("[8e]%x\n",udc_read16(MSB250X_USBCREG(0x8e)));
	printk("@@@@:%x\n",USBC_REG_READ16(0x00));	
}
#endif
/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_clear_ep0_opr
+------------------------------------------------------------------------------
| DESCRIPTION : to clear the RxPktRdy bit
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/

static inline void msb250x_udc_clear_ep0_opr(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8(MSB250X_UDC_CSR0_SRXPKTRDY, MSB250X_UDC_CSR0_REG);
    udc_write8(MSB250X_UDC_CSR0_SRXPKTRDY,MSB250X_USBCREG(0x102));//remove index
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_clear_ep0_sst
+------------------------------------------------------------------------------
| DESCRIPTION : to clear SENT_STALL
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_clear_ep0_sst(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8(0x00, MSB250X_UDC_CSR0_REG);
    udc_write8(0x00,MSB250X_USBCREG(0x102));//remove index
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_clear_ep0_se
+------------------------------------------------------------------------------
| DESCRIPTION : to clear the SetupEnd bit
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_clear_ep0_se(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8(MSB250X_UDC_CSR0_SSETUPEND, MSB250X_UDC_CSR0_REG);
    udc_write8(MSB250X_UDC_CSR0_SSETUPEND,MSB250X_USBCREG(0x102));//remove index
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_ep0_ipr
+------------------------------------------------------------------------------
| DESCRIPTION : to set the TxPktRdy bit affer loading a data packet into the FIFO
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_set_ep0_ipr(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8(MSB250X_UDC_CSR0_TXPKTRDY, MSB250X_UDC_CSR0_REG);
    udc_write8(MSB250X_UDC_CSR0_TXPKTRDY,MSB250X_USBCREG(0x102));//remove index
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_ep0_de
+------------------------------------------------------------------------------
| DESCRIPTION : to set the DataEnd bit
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_set_ep0_de(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8(MSB250X_UDC_CSR0_DATAEND, MSB250X_UDC_CSR0_REG);
    udc_write8(MSB250X_UDC_CSR0_DATAEND,MSB250X_USBCREG(0x102));//remove index
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_ep0_ss
+------------------------------------------------------------------------------
| DESCRIPTION : to set the SendStall bit to terminate the current transaction
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_set_ep0_ss(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8(MSB250X_UDC_CSR0_SENDSTALL, MSB250X_UDC_CSR0_REG);
    udc_write8(MSB250X_UDC_CSR0_SENDSTALL,MSB250X_USBCREG(0x102));//remove index
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_ep0_de_out
+------------------------------------------------------------------------------
| DESCRIPTION : to clear the ServiceRxPktRdy bit and set the DataEnd bit
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_set_ep0_de_out(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8((MSB250X_UDC_CSR0_SRXPKTRDY | MSB250X_UDC_CSR0_DATAEND),
    //        MSB250X_UDC_CSR0_REG);
    udc_write8((MSB250X_UDC_CSR0_SRXPKTRDY | MSB250X_UDC_CSR0_DATAEND),
            MSB250X_USBCREG(0x102));    //remove index    
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_ep0_sse_out
+------------------------------------------------------------------------------
| DESCRIPTION : to clear the ServiceRxPktRdy bit and clear the ServiceSetupEnd bit
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_set_ep0_sse_out(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8((MSB250X_UDC_CSR0_SRXPKTRDY | MSB250X_UDC_CSR0_SSETUPEND),
    //        MSB250X_UDC_CSR0_REG);
    udc_write8((MSB250X_UDC_CSR0_SRXPKTRDY | MSB250X_UDC_CSR0_SSETUPEND),
            MSB250X_USBCREG(0x102));    //remove index        
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_ep0_de_in
+------------------------------------------------------------------------------
| DESCRIPTION : to set the TxPktRdy bit and set the DataEnd bit
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static inline void msb250x_udc_set_ep0_de_in(void)
{
    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //udc_write8((MSB250X_UDC_CSR0_TXPKTRDY | MSB250X_UDC_CSR0_DATAEND),
    //        MSB250X_UDC_CSR0_REG);
    udc_write8((MSB250X_UDC_CSR0_TXPKTRDY | MSB250X_UDC_CSR0_DATAEND),
            MSB250X_USBCREG(0x102));    //remove index          
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_nuke
+------------------------------------------------------------------------------
| DESCRIPTION : dequeue ALL requests
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| ep                        | x  |      | msb250x_ep struct point
|--------------------+---+---+-------------------------------------------------
| req                       | x  |      | msb250x_request struct point
|--------------------+---+---+-------------------------------------------------
| status                   | x  |      | reports completion code, zero or a negative errno
+--------------------+---+---+-------------------------------------------------
*/
static void msb250x_udc_nuke(struct msb250x_udc *udc,
        struct msb250x_ep *ep, int status)
{
    /* Sanity check */
    if (&ep->queue == NULL)
        return;
	
    while (!list_empty (&ep->queue))
    {
        struct msb250x_request *req;
         req = list_entry (ep->queue.next, struct msb250x_request, queue);
        msb250x_udc_done(ep, req, status);
    }
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_fifo_ctl_count
+------------------------------------------------------------------------------
| DESCRIPTION : get the endpoint 0 RX FIFO count
|
| RETURN      : count number
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                       |  |   |
+--------------------+---+---+-------------------------------------------------
*/
int msb250x_udc_fifo_ctl_count(void)
{
    int tmp = 0;
    tmp = udc_read8(MSB250X_UDC_COUNT0_REG);
    return tmp;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_fifo_ctl_count
+------------------------------------------------------------------------------
| DESCRIPTION : get the endpoint RX FIFO count except the endpoint 0
|
| RETURN      : count number
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                       |  |   |
+--------------------+---+---+-------------------------------------------------
*/
int msb250x_udc_fifo_count(void)
{
    int tmp = 0;
    tmp = udc_read16(MSB250X_UDC_RXCOUNT_L_REG);
    return tmp;
}

int msb250x_udc_fifo_count_ep1(void)
{
    int tmp = 0;
    tmp = udc_read16(MSB250X_USBCREG(0x118));
    return tmp;
}

int msb250x_udc_fifo_count_ep2(void)
{
    int tmp = 0;
    tmp = udc_read16(MSB250X_USBCREG(0x128));
    return tmp;
}

int msb250x_udc_fifo_count_ep3(void)
{
    int tmp = 0;
    tmp = udc_read16(MSB250X_USBCREG(0x138));
    return tmp;
}

int msb250x_udc_fifo_count_ep4(void)
{
    int tmp = 0;
    tmp = udc_read16(MSB250X_USBCREG(0x148));
    return tmp;
}

int msb250x_udc_fifo_count_ep5(void)
{
    int tmp = 0;
    tmp = udc_read16(MSB250X_USBCREG(0x158));
    return tmp;
}

int msb250x_udc_fifo_count_ep6(void)
{
    int tmp = 0;
    tmp = udc_read16(MSB250X_USBCREG(0x168));
    return tmp;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_fifo_ctl_count
+------------------------------------------------------------------------------
| DESCRIPTION : get the usb control request info
|
| RETURN      : length
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| crq                 |x  |      | usb_ctrlrequest struct point
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_read_fifo_ctl_req(struct usb_ctrlrequest *crq)
{
    unsigned char *outbuf = (unsigned char*)crq;
    int bytes_read = 0;
    int fifo = MSB250X_UDC_EP0_FIFO_ACCESS_L;

    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);

    bytes_read = msb250x_udc_fifo_ctl_count();
    if (bytes_read > sizeof(struct usb_ctrlrequest))
        bytes_read = sizeof(struct usb_ctrlrequest);

    readsb((void *)/*IO_ADDRESS*/(fifo), outbuf, bytes_read);

    DBG("%s: len=%d %02x:%02x {%x,%x,%x}\n", __FUNCTION__,
        bytes_read, crq->bRequest, crq->bRequestType,
        crq->wValue, crq->wIndex, crq->wLength);

    return bytes_read;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_write_packet
+------------------------------------------------------------------------------
| DESCRIPTION : write the usb request info
|
| RETURN      : length
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| fifo                 |x  |      | the USB FIFO address register
|--------------------+---+---+-------------------------------------------------
| req                 |x  |      | msb250x_request struct point
|--------------------+---+---+-------------------------------------------------
| max                 |x  |      | max size
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_write_packet(int fifo,
        struct msb250x_request *req, unsigned max)
{
    unsigned int len = min(req->req.length - req->req.actual, max);
    u8 *buf = req->req.buf + req->req.actual;

    prefetch(buf);

    DBG("%s %d %d %d %d\n", __FUNCTION__,
        req->req.actual, req->req.length, len, req->req.actual + len);

    req->req.actual += len;
#ifdef TX_log	
	printk("[USB]fifo len:%x\n",len);
#endif	

    writesb((void *)/*IO_ADDRESS*/(fifo), buf, len);
#if 0	
printk("write\n");
if(len<=50)
{	int i=0;
	for(i=0;i<len;i++)
		{
			printk("%x ",buf[i]);
		}
	printk("\n");
}
#endif
//putb("w_f",tmp_buff,buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12]);
    return len;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_write_fifo
+------------------------------------------------------------------------------
| DESCRIPTION : write the usb request info
|
| RETURN      : length
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| ep                      |x  |      | msb250x_ep struct point
|--------------------+---+---+-------------------------------------------------
| req                 |x  |      | msb250x_request struct point
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_write_fifo(struct msb250x_ep *ep,
        struct msb250x_request *req)
{
    unsigned int count = 0;
    int is_last = 0;
    u32 idx = 0;
    int fifo_reg = 0;
    u32 ep_csr = 0;

    idx = ep->bEndpointAddress & 0x7F;
    switch (idx)
    {
        default:
         idx = 0;
         /* Fall Through */
     case 0:
         fifo_reg = MSB250X_UDC_EP0_FIFO_ACCESS_L;
         break;
     case 1:
        fifo_reg = MSB250X_UDC_EP1_FIFO_ACCESS_L;
        break;
     case 2:
         fifo_reg = MSB250X_UDC_EP2_FIFO_ACCESS_L;
         break;
     case 3:
         fifo_reg = MSB250X_UDC_EP3_FIFO_ACCESS_L;
         break;
    }

    count = msb250x_udc_write_packet(fifo_reg, req, ep->ep.maxpacket);

    /* last packet is often short (sometimes a zlp) */
    if (count != ep->ep.maxpacket)
        is_last = 1;
    else if (req->req.length != req->req.actual || req->req.zero)
     is_last = 0;
    else
        is_last = 2;

    /* Only ep0 debug messages are interesting */
    if (idx == 0)
        DBG("Written ep%d %d.%d of %d b [last %d,z %d]\n",
            idx, count, req->req.actual, req->req.length,
            is_last, req->req.zero);

    if (is_last)
    {
        /* The order is important. It prevents sending 2 packets at the same time */
        if (idx == 0)
        {
            /* Reset signal => no need to say 'data sent' */
            if (! (udc_read8(MSB250X_UDC_INTRUSB_REG) & MSB250X_UDC_INTRUSB_RESET))
                msb250x_udc_set_ep0_de_in();

            ep->dev->ep0state=EP0_IDLE;
        }
        else
        {
#if 0        
            udc_write8(idx, MSB250X_UDC_INDEX_REG);
            ep_csr = udc_read8(MSB250X_UDC_TXCSR1_REG);
            udc_write8(idx, MSB250X_UDC_INDEX_REG);
            udc_write8(ep_csr | MSB250X_UDC_TXCSR1_TXPKTRDY,
                       MSB250X_UDC_TXCSR1_REG);
#endif
			if(idx==1)
			{
				ep_csr = udc_read8(MSB250X_USBCREG(0x112));
				udc_write8(ep_csr|MSB250X_UDC_TXCSR1_TXPKTRDY,MSB250X_USBCREG(0x112));
				//udc_write8((ep_csr|MSB250X_UDC_TXCSR1_TXPKTRDY),udc_read8(MSB250X_USBCREG(0x112)));
			}
			else if(idx==3)
			{
				ep_csr = udc_read8(MSB250X_USBCREG(0x132));
				udc_write8(ep_csr|MSB250X_UDC_TXCSR1_TXPKTRDY,MSB250X_USBCREG(0x132));
			}			
        }
#ifndef TX_modify
        msb250x_udc_done(ep, req, 0);
#endif
        is_last = 1;
    }
    else
    {
        if (idx == 0)
        {
            /* Reset signal => no need to say 'data sent' */
            if (! (udc_read8(MSB250X_UDC_INTRUSB_REG) & MSB250X_UDC_INTRUSB_RESET))
                msb250x_udc_set_ep0_ipr();
        }
        else
        {
#if 0        
            udc_write8(idx, MSB250X_UDC_INDEX_REG);
            ep_csr = udc_read8(MSB250X_UDC_TXCSR1_REG);
            udc_write8(idx, MSB250X_UDC_INDEX_REG);
            udc_write8(ep_csr | MSB250X_UDC_TXCSR1_TXPKTRDY,
                                       MSB250X_UDC_TXCSR1_REG);
#endif
			if(idx==1)
			{
				ep_csr = udc_read8(MSB250X_USBCREG(0x112));
				udc_write8(ep_csr|MSB250X_UDC_TXCSR1_TXPKTRDY,MSB250X_USBCREG(0x112));
				//udc_write8((ep_csr|MSB250X_UDC_TXCSR1_TXPKTRDY),udc_read8(MSB250X_USBCREG(0x112)));
			}
			else if(idx==3)
			{
				ep_csr = udc_read8(MSB250X_USBCREG(0x132));
				udc_write8(ep_csr|MSB250X_UDC_TXCSR1_TXPKTRDY,MSB250X_USBCREG(0x132));
			}					
         }
     }

     return is_last;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_read_packet
+------------------------------------------------------------------------------
| DESCRIPTION : read the usb request info
|
| RETURN      : length
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| fifo                 |x  |      | the USB FIFO address register
|--------------------+---+---+-------------------------------------------------
| buf                 |x  |      | buf point
|--------------------+---+---+-------------------------------------------------
| req                 |x  |      | msb250x_request struct point
|--------------------+---+---+-------------------------------------------------
| avail                 |x  |      | available
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_read_packet(int fifo, u8 *buf,
        struct msb250x_request *req, unsigned avail)
{
    unsigned int len = 0;

    /* Sanity checks */
    if(!buf)
    {
        DBG("%s buff null \n", __FUNCTION__);
        return 0;
    }

    len = min(req->req.length - req->req.actual, avail);
    req->req.actual += len;

    readsb((void *)/*IO_ADDRESS*/(fifo), buf, len);
#if 0
printk("read:%x\n",len);
if(len<=50)
{	int i=0;
	for(i=0;i<len;i++)
		{
			printk("%x ",buf[i]);
		}
	printk("\n");
}
#endif
//putb("r_f",tmp_buff,buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12]);
    return len;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_read_fifo
+------------------------------------------------------------------------------
| DESCRIPTION : read the usb request info
|
| RETURN      : length
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| ep                      |x  |      | msb250x_ep struct point
|--------------------+---+---+-------------------------------------------------
| req                 |x  |      | msb250x_request struct point
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_read_fifo(struct msb250x_ep *ep,
                 struct msb250x_request *req)
{
    u8 *buf = NULL;
    //u32 ep_csr = 0;
    unsigned bufferspace = 0;
    int is_last = 1;
    unsigned avail = 0;
    int fifo_count = 0;
    u32 idx = 0;
    int fifo_reg = 0;

    idx = ep->bEndpointAddress & 0x7F;

    switch (idx)
    {
        default:
            idx = 0;
            /* Fall Through */
        case 0:
            fifo_reg = MSB250X_UDC_EP0_FIFO_ACCESS_L;
            break;
        case 1:
            fifo_reg = MSB250X_UDC_EP1_FIFO_ACCESS_L;
            break;
        case 2:
            fifo_reg = MSB250X_UDC_EP2_FIFO_ACCESS_L;
            break;
        case 3:
            fifo_reg = MSB250X_UDC_EP3_FIFO_ACCESS_L;
            break;
    }

    if (!req->req.length)
        return 1;

    buf = req->req.buf + req->req.actual;
    bufferspace = req->req.length - req->req.actual;
	//DBG("len %x -- buf %x -- actual %x\n", req->req.length, req->req.buf, req->req.actual);
	
    if (!bufferspace)
    {
        printk(KERN_ERR "%s: buffer full!\n", __FUNCTION__);
        return -1;
    }

    //udc_write8(idx, MSB250X_UDC_INDEX_REG);

    //if(idx)
    //    fifo_count = msb250x_udc_fifo_count();
	if(idx==1)
		fifo_count = msb250x_udc_fifo_count_ep1();
	else if(idx==2)
		fifo_count = msb250x_udc_fifo_count_ep2();
	else if(idx==3)
		fifo_count = msb250x_udc_fifo_count_ep3();    
    else	//ep0
        fifo_count = msb250x_udc_fifo_ctl_count();

    DBG("%s fifo count : %d\n", __FUNCTION__, fifo_count);

    if (fifo_count > ep->ep.maxpacket)
        avail = ep->ep.maxpacket;
    else
        avail = fifo_count;
#ifdef RX_mode1_log	  
	printk("read fifo-> len %x -- buf %p -- avail %x\n", req->req.length, buf, avail);
#endif
    fifo_count = msb250x_udc_read_packet(fifo_reg, buf, req, avail);

    /* checking this with ep0 is not accurate as we already
     * read a control request
    **/
    if (idx != 0 && fifo_count < ep->ep.maxpacket)
    {
        is_last = 1;
        /* overflowed this request?  flush extra data */
        if (fifo_count != avail)
            req->req.status = -EOVERFLOW;
    }
    else
    {
        is_last = (req->req.length <= req->req.actual) ? 1 : 0;
    }

    /* Only ep0 debug messages are interesting */
    if (idx == 0)
        DBG("%s fifo count : %d [last %d]\n",__FUNCTION__, fifo_count,is_last);

    if (is_last)
    {
        if (idx == 0)
        {
            msb250x_udc_set_ep0_de_out();
            ep->dev->ep0state = EP0_IDLE;
        }
        else
        {
            //udc_write8(idx, MSB250X_UDC_INDEX_REG);
            //ep_csr = udc_read8(MSB250X_UDC_RXCSR1_REG);
            //udc_write8(idx, MSB250X_UDC_INDEX_REG);
            //udc_write8(ep_csr & ~MSB250X_UDC_RXCSR1_RXPKTRDY,
            //                            MSB250X_UDC_RXCSR1_REG);
            if(idx==2)
            	udc_write8(0,MSB250X_USBCREG(0x126));          
        }

        msb250x_udc_done(ep, req, 0);
#ifndef RX_modify_mode1		
#ifdef CONFIG_USB_MSB250X_DMA
		/* if rx dma mode1 we have to check to set allow ack for net rx req early for net req */
		if(using_dma && ep->dev->DmaRxMode == DMA_RX_MODE1)
			msb250x_udc_schedule_done(ep);
#endif
#endif
    }
    else
    {
        if (idx == 0)
        {
            msb250x_udc_clear_ep0_opr();
        }
        else
        {
            //udc_write8(idx, MSB250X_UDC_INDEX_REG);
            //ep_csr = udc_read8(MSB250X_UDC_RXCSR1_REG);
            //udc_write8(idx, MSB250X_UDC_INDEX_REG);
            //udc_write8(ep_csr & ~MSB250X_UDC_RXCSR1_RXPKTRDY,
            //                            MSB250X_UDC_RXCSR1_REG);
            if(idx==2)
            	udc_write8(0,MSB250X_USBCREG(0x126));	            
        }
    }

    return is_last;
}

#ifdef CB2
/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_do_request
+------------------------------------------------------------------------------
| DESCRIPTION : dispatch request to use DMA or FIFO
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                | x |   | usb_ep struct point
|--------------------+---+---+-------------------------------------------------
| _req               | x |   | usb_request struct point
+--------------------+---+---+-------------------------------------------------
*/
extern void Release_DMA_Channel(s8 channel);
extern u8 DmaRxMode1;
//extern u8 DmaTxMode1;
extern s8 free_dma_channels;

#ifdef RX_modify_mode1
#ifdef CONFIG_USB_MSB250X_DMA
void RxHandler(struct msb250x_ep *ep, struct msb250x_request *req)
{
    u32 ep_csr = 0;
	
    int fifo_count = 0;
	//u8 save_idx = udc_read8(MSB250X_UDC_INDEX_REG);
#ifdef RX_modify_mode1	
		u32 bytesleft,addr,bytesdone;
#endif
		   //udc_write8((ep->bEndpointAddress & 0x7F), MSB250X_UDC_INDEX_REG);
		   //ep_csr = udc_read8((ep->bEndpointAddress & USB_DIR_IN)
		   //					 ? MSB250X_UDC_TXCSR1_REG
		   //						 : MSB250X_UDC_RXCSR1_REG);
		   //fifo_count = msb250x_udc_fifo_count();
			if((ep->bEndpointAddress & 0x7F)==2)
			{
				ep_csr = udc_read8(MSB250X_USBCREG(0x126));
				fifo_count = msb250x_udc_fifo_count_ep2();
			}
			
	    	if (ep->DmaRxMode1)
					{
					bytesleft =Get32BitsReg((u32 volatile*)DMA_COUNT_REGISTER(ep->ch));
#ifdef RX_mode1_log	 
					printk("bytesleft:%x\n",bytesleft);
#endif
                	//if(fifo_count==bytesleft)
						addr = Get32BitsReg((u32 volatile*)DMA_ADDR_REGISTER(ep->ch));
#ifdef RX_mode1_log	 						            	
			printk("short pack stop, addr:%x\n",addr);
#endif           	            		
						bytesdone = (u32) phys_to_virt((u32)(addr) - (u32)(req->req.buf + req->req.actual));
                	    req->req.actual+=bytesdone;
#ifdef RX_mode1_log	 						
						printk("[USB]RX_handler actual:%x,bytesdone:%x\n",req->req.actual,bytesdone);
#endif
						//putb("RX_handler actual,bytesdone",req->req.actual,bytesdone);
						//printk("read fifo xx:%x\n",fifo_count);
						msb250x_udc_read_fifo(ep, req);
						Release_DMA_Channel(ep->ch);
						//DmaRxMode1=FALSE;
						ep->DmaRxMode1=0;
						ep->RxShort=1;
						 req = NULL;
						//allow ACK
					
				   		 //putb("RXHandler-sch_done",0,0);
					     msb250x_udc_schedule_done(ep);
				   
				
					}	   
					else if (check_dma_busy()==DMA_NOT_BUSY)
					{
	                	if(fifo_count >= ep->ep.maxpacket)
                    	{
                    		//putb("DMA NO BUSY",0,0);
                        	if((req->req.actual + fifo_count) > req->req.length)
                            	printk(KERN_ERR "usb req buffer is too small\n");
                        	if(USB_Set_DMA(&ep->ep, req, fifo_count,DMA_RX_ZERO_IRQ) != SUCCESS)
                        	{
                            	DBG("USB_CLASS_COMM: DMA fail use FIFO\n");
                            	if(msb250x_udc_read_fifo(ep, req))
                                	req = NULL;
                        	}
                    	}
                    	else
                    	{
                    		//putb("short data",0,0);
#ifdef RX_mode1_log	                        
                    		printk("[USB]rx short data read fifo count:%x\n",fifo_count);
#endif
							//putb("rx short read fifo",0,0);
                        	if(msb250x_udc_read_fifo(ep, req))
                            	req = NULL;
							
							msb250x_udc_schedule_done(ep);
							//receive next out packet

                    	}				
					}
					else if(check_dma_busy()==DMA_BUSY)
					{
#ifdef RX_mode1_log	                        
		                printk("[USB]RxHandler>>rx dma busy:%x\n",fifo_count);
#endif					
						if(fifo_count >= ep->ep.maxpacket)
						{
		                    if((req->req.actual + fifo_count) > req->req.length)
		                       	printk(KERN_ERR "usb req buffer is too small\n");
							
							if(msb250x_udc_read_fifo(ep, req))
								req = NULL;
						}
		                else
		                {
#ifdef RX_mode1_log	                        
		                 	printk("[USB]rx__ short data read fifo count:%x\n",fifo_count);
#endif
		                    if(msb250x_udc_read_fifo(ep, req))
		                       	req = NULL;
									
							msb250x_udc_schedule_done(ep);
							//receive next out packet
		                }				
					}					
	//udc_write8(save_idx, MSB250X_UDC_INDEX_REG);
}
#endif
#endif

struct msb250x_request * msb250x_udc_do_request(struct msb250x_ep *ep, struct msb250x_request *req)
{
    u32 ep_csr = 0;
#ifdef CONFIG_USB_MSB250X_DMA
#ifdef RX_modify_mode1	
//	u32 bytesleft,addr,bytesdone;
#endif
#endif
    int fifo_count = 0;
#if 0
if((ep->bEndpointAddress)!=0)
{
	int yy=0,i;

	//ep->buf[ep->work_index]=_req->buf;
	for(yy=0;yy<2;yy++)
	{
		    if(e_buf[yy]==req->req.buf) 
		    	{
		    	  //workidx=yy;
				  if(e_working[yy]==1)
				  	{
						printk("buffer conflict WARN  ...\n");
						while(1);
				  	}
				  else
						e_working[yy]=1;
				  break;
		    	}
			
	}
	if (yy==2)
	{
		for (i=0; i < 2 ; i++)
			if (e_buf[i]==0)
			{
				 e_buf[i]=req->req.buf;
				 e_working[i]=1;
				 //printk("buf:%x\n",e_buf[i]);
				 break;
			}
	}
	//printk("buf:%x,req:%x,index:%x\n",ep->buf[ep->work_index],_req->buf,ep->work_index);
}
#endif		
    //u8 save_idx = udc_read8(MSB250X_UDC_INDEX_REG);
#ifdef TX_log		
	printk("[USB]msb250x_udc_do_request\n");
#endif
    if (ep->bEndpointAddress)
    {
        //udc_write8((ep->bEndpointAddress & 0x7F), MSB250X_UDC_INDEX_REG);
        //ep_csr = udc_read8((ep->bEndpointAddress & USB_DIR_IN)
        //                      ? MSB250X_UDC_TXCSR1_REG
        //                      : MSB250X_UDC_RXCSR1_REG);
        //fifo_count = msb250x_udc_fifo_count();
        if((ep->bEndpointAddress & 0x7F)==1)
        {
        	ep_csr = udc_read8(MSB250X_USBCREG(0x112));
			fifo_count = msb250x_udc_fifo_count_ep1();
        }
		else if((ep->bEndpointAddress & 0x7F)==2)
		{
			ep_csr = udc_read8(MSB250X_USBCREG(0x126));
			fifo_count = msb250x_udc_fifo_count_ep2();
		}
		else if((ep->bEndpointAddress & 0x7F)==3)
		{
			ep_csr = udc_read8(MSB250X_USBCREG(0x132));
			fifo_count = msb250x_udc_fifo_count_ep3();
		}
    }
    else
    {
        //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
        //ep_csr = udc_read8(MSB250X_UDC_CSR0_REG);
        ep_csr = udc_read8(MSB250X_USBCREG(0x102));
        fifo_count = msb250x_udc_fifo_ctl_count();
    }

    DBG("ep %d fifo_count: %x req.len %x \n", ep->bEndpointAddress & 0x7F, fifo_count, req->req.length);
#ifdef RX_mode1_log		
    printk("ep %d fifo_count: %x req.len %x \n", ep->bEndpointAddress & 0x7F, fifo_count, req->req.length);
#endif
    if(!ep->halted)
    {
        if (ep->bEndpointAddress == MSB250X_UDC_INDEX_EP0 )
        {
            switch (ep->dev->ep0state)
            {
                case EP0_IN_DATA_PHASE:
                    if (!(ep_csr & MSB250X_UDC_CSR0_TXPKTRDY)
                           && msb250x_udc_write_fifo(ep, req))
                    {
                        ep->dev->ep0state = EP0_IDLE;
#ifndef TX_modify
                        req = NULL;
#endif
                    }
                    break;

                case EP0_OUT_DATA_PHASE:
                    if ((!req->req.length)
                           || ((ep_csr & MSB250X_UDC_CSR0_RXPKTRDY)
                           && msb250x_udc_read_fifo(ep, req)))
                    {
                        ep->dev->ep0state = EP0_IDLE;
                        req = NULL;
                    }
                    break;

                default:
                    printk(KERN_ERR " EP0 Request Error !!\n");
                    return req;
            }
        }
#ifdef CONFIG_USB_MSB250X_DMA
        else if (using_dma && (ep->bEndpointAddress & USB_DIR_IN))
        {
        	//putb("do_req_TX",0,0);
        /* DMA TX: */
           // if(!(ep_csr & MSB250X_UDC_TXCSR1_TXPKTRDY))
            {
#ifndef xxxx
                if(check_dma_busy() == DMA_BUSY)
                {
                    /* Double check to insure this EP is not DMAing */
                    u8 endpoint = (udc_read16((u32)(DMA_CNTL_REGISTER(1))) & 0xf0) >> DMA_ENDPOINT_SHIFT;
					//putb("TX_FIFO",0,0);
                    if(endpoint != (ep->bEndpointAddress &0x7f) && msb250x_udc_write_fifo(ep, req)){
#ifndef TX_modify
                        req = NULL;
#endif
                   }
                }
                else
#endif			
#ifdef xxxx
				//printk("dma_busy:%x\n",check_dma_busy());
				if (check_dma_busy()==DMA_NOT_BUSY)
#endif
                {
                           
						
                   if ((req->req.length >= ep->ep.maxpacket) && (req->req.length >= MIN_DMA_TRANSFER_BYTES))
                    {
                        u32 tx_dma_count = (u32)(req->req.length) - (u32)(req->req.actual);
						//putb("TX_DMA",0,0);
                        /* If req len equal to maxpacket, it will use DMA TX mode0 automaticly */
                        if(USB_Set_DMA(&ep->ep, req, tx_dma_count, DMA_TX_ONE_IRQ) != SUCCESS)
                        {
                            DBG("Use fifo write mode\n");
							//putb("Use fifo write mode",0,0);
                            if(msb250x_udc_write_fifo(ep, req)){
 #ifndef TX_modify
                                req = NULL;
 #endif
                            }
                        }
                    }
                    else
                    {
#ifdef TX_log	                    
                    	printk("[USB]do_request--write_fifo\n");
#endif
						//putb("do_req--write_fifo",0,0);
                        if(msb250x_udc_write_fifo(ep, req)){
#ifndef TX_modify							
                            req = NULL;
#endif
                        }
                    }
                }
            }
        }
        else if (using_dma && !(ep->bEndpointAddress & USB_DIR_IN) && (ep->dev->DmaRxMode != DMA_RX_MODE_NULL))
        {	
#ifdef RX_mode1_log	        
        	printk("[USB]DMA_RX\n");
#endif
			/* DMA RX: */
			if(ep->dev->DmaRxMode == DMA_RX_MODE0)
            {
            /* DMA mode 0 */
                if(ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY)
                {
#ifndef RX_modify_mode1              
                    if(check_dma_busy() == DMA_BUSY)
                    {
                        /* Double check to insure this EP is not DMAing */
                        u8 endpoint = (udc_read16((u32)(DMA_CNTL_REGISTER(1))) & 0xf0) >> DMA_ENDPOINT_SHIFT;						
                        if(endpoint != (ep->bEndpointAddress &0x7f) && msb250x_udc_read_fifo(ep, req))
                            req = NULL;
                    }
                    else
#endif						
#ifdef RX_modify_mode1
					if (check_dma_busy()==DMA_NOT_BUSY)
#endif
                    {
#ifdef RX_mode1_log	                    
                    	printk("[USB]SET_DMA_RX\n");
#endif
                        if(fifo_count >= ep->ep.maxpacket)
                        {
                            if((req->req.actual + fifo_count) > req->req.length)
                                printk(KERN_ERR "usb req buffer is too small\n");

                          if(USB_Set_DMA(&ep->ep, req, fifo_count,DMA_RX_ZERO_IRQ) != SUCCESS)
                            {
                                DBG("USB_CLASS_COMM: DMA fail use FIFO\n");
                                if(msb250x_udc_read_fifo(ep, req))
                                    req = NULL;
                            }
                        }
                        else
                        {
#ifdef RX_mode1_log	                        
                        	printk("[USB]2FIFO_COUNT:%x\n",fifo_count);
#endif
                            if(msb250x_udc_read_fifo(ep, req))
                                req = NULL;
                        }
                    }					
                }
            }
            else
            {
            /* DMA mode 1*/
#ifdef RX_modify_mode1			
				//set auto NAK
				//udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Model_P_NAK), MSB250X_UDC_DMA_MODE_CTL);				
#ifdef RX_mode1_log	            
				//printk("[USB]Set auto NAK\n");
#endif
				{
#ifdef RX_mode1_log	            
            		printk("[USB]do mode 1 for all left \n");
#endif				
					//putb("do mode 1 for all",0,0);
					// do mode 1 for all left , enable ACK, left=length-actual
					if( (req->req.length-req->req.actual)>  ep->ep.maxpacket)
					{
#ifdef RX_mode1_log	            
            			printk("[USB]length:%x,actual:%x\n",req->req.length,req->req.actual);
#endif					
						if(check_dma_busy() != DMA_BUSY)
						{
							u32 rx_dma_count = (u32)(req->req.length) - (u32)(req->req.actual);
#ifdef RX_mode1_log	            
            				printk("[USB]set DMA mode1\n");
#endif						
							//putb("set DMA mode1",0,0);
							//printk("[USB]TOTAL:%x,RXCSR:%x\n",req->req.length,udc_read8(MSB250X_UDC_RXCSR1_REG));
							if(USB_Set_DMA(&ep->ep, req, rx_dma_count, DMA_RX_ONE_IRQ) != SUCCESS){
								DBG(KERN_ERR "USB_CLASS_MASS_STORAGE: Set DMA fail FIFO\n");
								}
							//enable Allow ok,
							//udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)&~M_Model_P_NAK), MSB250X_UDC_DMA_MODE_CTL);
#ifdef RX_mode1_log	            
							printk("[USB]allow ack\n");
#endif	

						}
						else{
                        	DBG(KERN_ERR "DMA busy.. just queue req \n");
                    	}
					}
					else
					{
#if 0					
#ifdef RX_mode1_log	            
            			printk("[USB]fifo read\n");
#endif					
                    	if(ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY)
                    	{
#ifdef RX_mode1_log	            
      	      				printk("[USB]fifo_read_RXPKTRDY\n");
#endif                    	
                       		if(msb250x_udc_read_fifo(ep, req))
                           		req = NULL;
                    	}
                    	else
#endif							
							{
#ifdef RX_mode1_log							
							printk("[USB]USB_Set_ClrRXMode1\n");							
#endif
							//putb("allowack",ep->bEndpointAddress,0);
							if(((ep->bEndpointAddress)&0x0f)==2)
								udc_write16(udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_AllowAck|M_Mode1_P_NAK_Enable, MSB250X_UDC_DMA_MODE_CTL);
							else if(((ep->bEndpointAddress)&0x0f)==4)
							{
								udc_write8(udc_read8(MSB250X_UDC_DMA_MODE_CTL1)|M_Mode1_P_AllowAck_1|M_Mode1_P_NAK_Enable_1, MSB250X_UDC_DMA_MODE_CTL1);
							}
                       		//USB_Set_ClrRXMode1();
                    	}
					}				
				}
#endif			
#ifndef RX_modify_mode1
                if( req->req.length > ep->ep.maxpacket)
                {
                    if(check_dma_busy() != DMA_BUSY)
                    {
                        u32 rx_dma_count = (u32)(req->req.length) - (u32)(req->req.actual);
                        if(USB_Set_DMA(&ep->ep, req, rx_dma_count, DMA_RX_ONE_IRQ) != SUCCESS){
                            DBG(KERN_ERR "USB_CLASS_MASS_STORAGE: Set DMA fail FIFO\n");
							}
                    }
                    else{
                        DBG(KERN_ERR "DMA busy.. just queue req \n");
                    	}
                }
                else
                {
                    if(ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY)
                    {
                       if(msb250x_udc_read_fifo(ep, req))
                           req = NULL;
                    }
                    else
                       USB_Set_ClrRXMode1();
                }				
#endif				
            }
        }
#endif    /* CONFIG_USB_MSB250X_DMA */
        else if ((ep->bEndpointAddress & USB_DIR_IN) != 0 && (!(ep_csr & MSB250X_UDC_TXCSR1_TXPKTRDY))
                    && msb250x_udc_write_fifo(ep, req))    /* IN token packet */
        {
#ifdef TX_log	        
        	printk("[USB]TXPACKET_NOT_READY_FIFO_fifo_write\n");
#endif
#ifndef TX_modify
            req = NULL;
#endif
        }
        else if (!(ep->bEndpointAddress & USB_DIR_IN))    /* OUT token packet */
        {
#ifdef RX_mode1_log	        
        	printk("[USB]OUT token packet\n");
#endif
            if ((ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY) &&
                   //fifo_count &&    // Kate.Liu: mark this condition to handle 0-lentgh packet.
                   (msb250x_udc_read_fifo(ep, req)))
            {
#ifdef RX_mode1_log	            
            		printk("[USB]MSB250X_UDC_RXCSR1_RXPKTRDY_fifo_read\n");
#endif
                    req = NULL;
            }
        }
    }
#ifdef RX_mode1_log
else
{
	printk("[USB]halted, return\n");
}
#endif
    //udc_write8(save_idx, MSB250X_UDC_INDEX_REG);

    return req;
}
#if 0
static struct msb250x_request * xx_msb250x_udc_do_request(struct msb250x_ep *ep, struct msb250x_request *req)
{
	u32 ep_csr = 0;
	int fifo_count = 0;

	u8 save_idx = udc_read8(MSB250X_UDC_INDEX_REG);
	
	if (ep->bEndpointAddress)
	{
		udc_write8((ep->bEndpointAddress & 0x7F), MSB250X_UDC_INDEX_REG);
		ep_csr = udc_read8((ep->bEndpointAddress & USB_DIR_IN)
							  ? MSB250X_UDC_TXCSR1_REG
							  : MSB250X_UDC_RXCSR1_REG);
		fifo_count = msb250x_udc_fifo_count();
	}
	else
	{
		udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
		ep_csr = udc_read8(MSB250X_UDC_CSR0_REG);
		fifo_count = msb250x_udc_fifo_ctl_count();
	}
	
	DBG("ep %d fifo_count: %d req.len %d \n", ep->bEndpointAddress & 0x7F, fifo_count, req->req.length);

	if(!ep->halted)
	{ 
		if (ep->bEndpointAddress == MSB250X_UDC_INDEX_EP0 )
		{
			switch (ep->dev->ep0state)
			{
				case EP0_IN_DATA_PHASE:
					if (!(ep_csr & MSB250X_UDC_CSR0_TXPKTRDY)
						   && msb250x_udc_write_fifo(ep, req))
					{
						ep->dev->ep0state = EP0_IDLE;
						req = NULL;
					}
					break;
	
				case EP0_OUT_DATA_PHASE:
					if ((!req->req.length)
						   || ((ep_csr & MSB250X_UDC_CSR0_RXPKTRDY)
						   && msb250x_udc_read_fifo(ep, req)))
					{
						ep->dev->ep0state = EP0_IDLE;
						req = NULL;
					}
					break;
	
				default:
					printk(KERN_ERR " EP0 Request Error !!\n");
					return req;
			}
		}
#ifdef CONFIG_USB_MSB250X_DMA
		else if (using_dma && (ep->bEndpointAddress & USB_DIR_IN))
		{
			/* DMA TX: */
			if(!(ep_csr & MSB250X_UDC_TXCSR1_TXPKTRDY))
			{
				if(check_dma_busy() == DMA_BUSY)
				{
					/* Double check to insure this EP is not DMAing */
					u8 endpoint = (udc_read16((u32)(DMA_CNTL_REGISTER(1))) & 0xf0) >> DMA_ENDPOINT_SHIFT;

					if(endpoint != (ep->bEndpointAddress &0x7f) && msb250x_udc_write_fifo(ep, req))
						req = NULL;
				}
				else
				{
					if (req->req.length >= ep->ep.maxpacket)
					{
						u32 tx_dma_count = (u32)(req->req.length) - (u32)(req->req.actual);

						/* If req len equal to maxpacket, it will use DMA TX mode0 automaticly */
						if(USB_Set_DMA(&ep->ep, req, tx_dma_count, DMA_TX_ONE_IRQ) != SUCCESS)
						{
							DBG("Use fifo write mode\n");
							if(msb250x_udc_write_fifo(ep, req))
								req = NULL;
						}
					}
					else
					{
						if(msb250x_udc_write_fifo(ep, req))
							req = NULL;
					}
				}
			}
		}
		else if (using_dma && !(ep->bEndpointAddress & USB_DIR_IN) && (ep->dev->DmaRxMode != DMA_RX_MODE_NULL))
		{
			/* DMA RX: */
			if(ep->dev->DmaRxMode == DMA_RX_MODE0)
			{
				/* DMA mode 0 */
				if(ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY)
				{
					if(check_dma_busy() == DMA_BUSY)
					{
						/* Double check to insure this EP is not DMAing */
						u8 endpoint = (udc_read16((u32)(DMA_CNTL_REGISTER(1))) & 0xf0) >> DMA_ENDPOINT_SHIFT;
	
						if(endpoint != (ep->bEndpointAddress &0x7f) && msb250x_udc_read_fifo(ep, req))
							req = NULL;
					}
					else
					{
						if(fifo_count >= ep->ep.maxpacket)
						{
							if((req->req.actual + fifo_count) > req->req.length)
								printk(KERN_ERR "usb req buffer is too small\n");
  
							if(USB_Set_DMA(&ep->ep, req, fifo_count,DMA_RX_ZERO_IRQ) != SUCCESS)
							{
								DBG("USB_CLASS_COMM: DMA fail use FIFO\n");
								if(msb250x_udc_read_fifo(ep, req))
									req = NULL;
							}
						}
						else
						{
							if(msb250x_udc_read_fifo(ep, req))
								req = NULL;
						}
					}
				}
			}
			else
			{
				/* DMA mode 1*/
				if( req->req.length > ep->ep.maxpacket)
				{
					if(check_dma_busy() != DMA_BUSY)
					{
						u32 rx_dma_count = (u32)(req->req.length) - (u32)(req->req.actual);

						if(USB_Set_DMA(&ep->ep, req, rx_dma_count, DMA_RX_ONE_IRQ) != SUCCESS)
							DBG(KERN_ERR "USB_CLASS_MASS_STORAGE: Set DMA fail FIFO\n");
					}
					else
						DBG(KERN_ERR "DMA busy.. just queue req \n");
				}
				else
				{
					if(ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY)
					{
					   if(msb250x_udc_read_fifo(ep, req))
						   req = NULL;
					}
					else
					   USB_Set_ClrRXMode1();
				}
			}
		}
#endif    /* CONFIG_USB_MSB250X_DMA */
		else if ((ep->bEndpointAddress & USB_DIR_IN) != 0 && (!(ep_csr & MSB250X_UDC_TXCSR1_TXPKTRDY))
					&& msb250x_udc_write_fifo(ep, req))    /* IN token packet */
		{
			req = NULL;
		}
		else if (!(ep->bEndpointAddress & USB_DIR_IN))	  /* OUT token packet */
		{
			if ((ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY) &&
				   //fifo_count &&	  // Kate.Liu: mark this condition to handle 0-lentgh packet.
				   (msb250x_udc_read_fifo(ep, req)))
			{
					req = NULL;
			}
		}
	}
	
	udc_write8(save_idx, MSB250X_UDC_INDEX_REG);
	
	return req;
}
#endif
#ifdef CONFIG_USB_MSB250X_DMA
/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_schedule_DMA
+------------------------------------------------------------------------------
| DESCRIPTION : Schedule the next DMA EP
|
| RETURN      :
+------------------------------------------------------------------------------
| Variable Name  |IN |OUT|                   Usage
|----------------+---+---+-----------------------------------------------------
| ep             | x |   | msb250x_ep struct point
+-------------- -+---+---+-----------------------------------------------------
*/
s8 msb250x_udc_schedule_DMA(struct msb250x_ep *ep)
{
    struct msb250x_request *req = NULL;
    u8 ep_idx = 0;
    struct msb250x_ep *cur_ep;
    static u8 token = 0;
    u8 i;

    for(i = 1; i < MSB250X_ENDPOINTS - 1; i++)
    {
        token = (token + 1)%(MSB250X_ENDPOINTS-1);

        ep_idx = token + 1;

        cur_ep = &(ep->dev->ep[ep_idx]);

        if (req == NULL && likely (!list_empty(&cur_ep->queue)))
        {
            req = list_entry(cur_ep->queue.next, struct msb250x_request, queue);

            if(req && req->req.length >= ep->ep.maxpacket)
            {
                if(msb250x_udc_do_request(cur_ep, req) != NULL)
                    break;
            }
        }
    }

    return 0;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_schedule_done
+------------------------------------------------------------------------------
| DESCRIPTION : Schedule the next packet for this EP
|
| RETURN      :
+------------------------------------------------------------------------------
| Variable Name  |IN |OUT|                   Usage
|----------------+---+---+-----------------------------------------------------
| ep             | x |   | msb250x_ep struct point
+-------------- -+---+---+-----------------------------------------------------
*/
s8 msb250x_udc_schedule_done(struct msb250x_ep *ep)
{
    if (likely (!list_empty(&ep->queue)))
    {
        struct msb250x_request *req = NULL;
		//printk("[USB]CHECK QUEUE\n");
        req = list_entry(ep->queue.next, struct msb250x_request, queue);
        msb250x_udc_do_request(ep, req);
    }

    return 0;
}
#endif
#endif
/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_done
+------------------------------------------------------------------------------
| DESCRIPTION : complete the usb request
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| ep                        | x  |      | msb250x_ep struct point
|--------------------+---+---+-------------------------------------------------
| req                       | x  |      | msb250x_request struct point
|--------------------+---+---+-------------------------------------------------
| status                   | x  |      | reports completion code, zero or a negative errno
+--------------------+---+---+-------------------------------------------------
*/
#ifdef CB2
void msb250x_udc_done(struct msb250x_ep *ep,
        struct msb250x_request *req, int status)
	{
		unsigned halted = ep->halted;
		//u32 *prt;
		DBG("done %d actral %d length %x\n", ep->num, req->req.actual, req->req.length);
		//putb("done ep's_req",ep->num,0);
#ifdef TX_modify
		if(req==NULL)
		{
			printk("[USB]REQ NULL\n");
			return;
		}
#endif	
		if(req==NULL)
		{
			printk("[USB]REQ NULL\n");
			return;
		}
		//req = list_entry(ep->queue.next, struct msb250x_request, queue);
		//prt=(u32*)req;
		//printk("D_req:%x\n",(u32)prt);
	
		if (ep->num)
			DBG_TR("complete %u\n", ep->num);
	
		list_del_init(&req->queue);
		if(!(list_empty(&req->queue)))
		{
			printk("[USB]queue not empty!!\n");
		}
	
		if (likely (req->req.status == -EINPROGRESS))
			req->req.status = status;
		else
			status = req->req.status;
	
		ep->halted = 1;
		req->req.complete(&ep->ep, &req->req);
		ep->halted = halted;
	
		return;
}

#else
void msb250x_udc_done(struct msb250x_ep *ep,
        struct msb250x_request *req, int status)
{
    unsigned halted = ep->halted;
#ifdef CONFIG_USB_MSB250X_DMA
    u32 ep_csr = 0;
    u8 idx, saved_idx;
    u8 inreg;
#endif /* CONFIG_USB_MSB250X_DMA */

    // DBG("done %d\n", ep->num);
    if (ep->num)
        DBG_TR("complete %u\n", ep->num);

    list_del_init(&req->queue);

    if (likely (req->req.status == -EINPROGRESS))
        req->req.status = status;
    else
        status = req->req.status;

    ep->halted = 1;
    req->req.complete(&ep->ep, &req->req);
    ep->halted = halted;

#ifdef CONFIG_USB_MSB250X_DMA
    if (likely (!list_empty(&ep->queue)))
    {
    	//putb("find queue",ep->bEndpointAddress,0);
        req = list_entry(ep->queue.next, struct msb250x_request, queue);
    }
    else
    {
    	//putb("queue null",ep->bEndpointAddress,0);
        req = NULL;
    }

    if (req)
    {
        idx = ep->bEndpointAddress & 0x7f;

        if (idx != 0)
        {
            /* Save index */
            saved_idx = udc_read8(MSB250X_UDC_INDEX_REG);
            udc_write8(idx, MSB250X_UDC_INDEX_REG);

            ep_csr = udc_read8((ep->bEndpointAddress & USB_DIR_IN)
                                ? MSB250X_UDC_TXCSR1_REG
                                : MSB250X_UDC_RXCSR1_REG);
            if (check_dma_busy() != DMA_BUSY)
            {
                if (using_dma && (req->req.length > (ep->ep.maxpacket)))
                {
                    if (!(ep->bEndpointAddress & USB_DIR_IN)) /* OUT token packet */
                    {
                        if (unlikely(USB_Set_DMA(&ep->ep, req, DMA_RX_ONE_IRQ)<0))
                        {
                            printk("DMA Rx Error!!\n");
                        }
                    }
                    else /* IN token packet */
                    {
                        inreg = udc_read16(MSB250X_UDC_TXCSR1_REG);
                        if(!(inreg & MSB250X_UDC_TXCSR1_FIFONOEMPTY)||!(inreg & MSB250X_UDC_TXCSR1_SENTSTALL))
                        {
                            if (unlikely(USB_Set_DMA(&ep->ep, req, DMA_TX_ONE_IRQ)<0))
                            {
                                printk("DMA Tx Error");
                            }
                        }
                        // else
                        // {
                        //     printk("TX FIFO not EMPTY!\n");
                        // }
                    }
                }
                else if ((ep->bEndpointAddress & USB_DIR_IN) != 0 /* IN token packet */
                                    && (!(ep_csr & MSB250X_UDC_TXCSR1_TXPKTRDY))
                                    && msb250x_udc_write_fifo(ep, req))
                {
                    req = NULL;
                }
                else if (!(ep->bEndpointAddress & USB_DIR_IN)) /* OUT token packet */
                {
                    int fifo_count;

                    USB_Set_ClrRXMode1(); // winder

                    fifo_count = msb250x_udc_fifo_count();

                    if ((fifo_count != 0) && (ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY))
                    {
                        msb250x_udc_read_fifo(ep, req);
                    }
                }
            }
            /* restore previous index value */
            udc_write8(saved_idx, MSB250X_UDC_INDEX_REG);
        }
    }
#endif /* CONFIG_USB_MSB250X_DMA */

    return;
}
#endif
/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_get_status
+------------------------------------------------------------------------------
| DESCRIPTION :get the USB device status
|
| RETURN      :0 when success, error code in other case.
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                      |x  |      | msb250x_udc struct point
|--------------------+---+---+-------------------------------------------------
| crq                        |x  |      | usb_ctrlrequest struct point
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_get_status(struct msb250x_udc *dev,
        struct usb_ctrlrequest *crq)
{
    u16 status = 0;
    u8 ep_num = crq->wIndex & 0x7F;
    u8 is_in = crq->wIndex & USB_DIR_IN;

    switch (crq->bRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_INTERFACE:
             break;

        case USB_RECIP_DEVICE:
            status = dev->devstatus;
            break;

        case USB_RECIP_ENDPOINT:
            if (ep_num > 3 || crq->wLength > 2)
                return 1;

            if (ep_num == 0)
            {
                //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
                //status = udc_read8(MSB250X_UDC_CSR0_REG);
                status = udc_read8(MSB250X_USBCREG(0x102));
                status = status & MSB250X_UDC_CSR0_SENDSTALL;
            }
            else
            {
                //udc_write8(ep_num, MSB250X_UDC_INDEX_REG);

                if (is_in)
                {
                    //status = udc_read8(MSB250X_UDC_TXCSR1_REG);
                    if(ep_num==1)
						status = udc_read8(MSB250X_USBCREG(0x112));
					else if(ep_num==3)
						status = udc_read8(MSB250X_USBCREG(0x132));
					
                    status = status & MSB250X_UDC_TXCSR1_SENDSTALL;
                }
                else
                {
                    //status = udc_read8(MSB250X_UDC_RXCSR1_REG);
					if(ep_num==2)
						status = udc_read8(MSB250X_USBCREG(0x126));
					
                    status = status & MSB250X_UDC_RXCSR1_SENDSTALL;
                }
            }

            status = status ? 1 : 0;
            break;

        default:
            return 1;
    }

    /* Seems to be needed to get it working. ouch :( */
    udelay(5);
    udc_write8(status & 0xFF, MSB250X_UDC_EP0_FIFO_ACCESS_L);
    udc_write8(status >> 8, MSB250X_UDC_EP0_FIFO_ACCESS_L);
    msb250x_udc_set_ep0_de_in();

    return 0;
}

/*----------- msb250x_udc_set_halt prototype ------------*/
static int msb250x_udc_set_halt(struct usb_ep *_ep, int value);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_handle_ep0_idle
+------------------------------------------------------------------------------
| DESCRIPTION :handle the endpoint 0 when endpoint 0 is idle
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                 |x  |      | msb250x_udc struct point
|--------------------+---+---+-------------------------------------------------
| ep                      |x  |      | msb250x_ep struct point
|--------------------+---+---+-------------------------------------------------
| crq                 |x  |      | usb_ctrlrequest struct point
|--------------------+---+---+-------------------------------------------------
| ep0csr                 |x  |      | the csr0 register value
+--------------------+---+---+-------------------------------------------------
*/
static void msb250x_udc_handle_ep0_idle(struct msb250x_udc *dev,
                    struct msb250x_ep *ep,
                    struct usb_ctrlrequest *crq,
                    u32 ep0csr)
{
    int len = 0, ret = 0 , tmp = 0;

    /* start control request? */
    if (!(ep0csr & MSB250X_UDC_CSR0_RXPKTRDY))
        return;

    msb250x_udc_nuke(dev, ep, -EPROTO);

    len = msb250x_udc_read_fifo_ctl_req(crq);
    if (len != sizeof(*crq))
    {
    	//putb("fifo READ ERROR",0,0);
    	printk("len:%x,crq:%x\n",len,sizeof(*crq));
        printk(KERN_ERR "setup begin: fifo READ ERROR"
             " wanted %d bytes got %d. Stalling out...\n",
          sizeof(*crq), len);

        msb250x_udc_set_ep0_ss();
            return;
    }

    DBG("bRequest = %x bRequestType %x wLength = %d\n",
            crq->bRequest, crq->bRequestType, crq->wLength);

    /* cope with automagic for some standard requests. */
    dev->req_std = (crq->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD;
    dev->req_config = 0;
    dev->req_pending = 1;

    if (dev->req_std)
    {
        switch (crq->bRequest)
        {
            case USB_REQ_SET_CONFIGURATION:
                DBG("USB_REQ_SET_CONFIGURATION ... \n");

                if (crq->bRequestType == USB_RECIP_DEVICE)
                {
                    dev->req_config = 1;
                    msb250x_udc_set_ep0_de_out();
                }
                break;

            case USB_REQ_SET_INTERFACE:
                DBG("USB_REQ_SET_INTERFACE ... \n");

                if (crq->bRequestType == USB_RECIP_INTERFACE)
                {
                    dev->req_config = 1;
                    msb250x_udc_set_ep0_de_out();
                }
                break;

            case USB_REQ_SET_ADDRESS:
                DBG("USB_REQ_SET_ADDRESS ... \n");

                if (crq->bRequestType == USB_RECIP_DEVICE)
                {
                    tmp = crq->wValue & 0x7F;
                    dev->address = tmp;
                    udc_write8(tmp, MSB250X_UDC_FADDR_REG);
                    msb250x_udc_set_ep0_de_out();
                    return;
                }
                break;

            case USB_REQ_GET_STATUS:
                DBG("USB_REQ_GET_STATUS ... \n");

                msb250x_udc_clear_ep0_opr();
                if (dev->req_std)
                {
                    if (!msb250x_udc_get_status(dev, crq))
                    {
                        return;
                    }
                }
                break;

            case USB_REQ_CLEAR_FEATURE:
				printk("USB_REQ_CLEAR_FEATURE\n");
                DBG("USB_REQ_CLEAR_FEATURE ... \n");
                msb250x_udc_clear_ep0_opr();
                if (crq->bRequestType != USB_RECIP_ENDPOINT)
                    break;

                if (crq->wValue != USB_ENDPOINT_HALT || crq->wLength != 0)
                    break;
				printk("endpoint_halt\n");
                msb250x_udc_set_halt(&dev->ep[crq->wIndex & 0x7f].ep, 0);
                msb250x_udc_set_ep0_de_out();
                return;

            case USB_REQ_SET_FEATURE:
                DBG("USB_REQ_SET_FEATURE ... \n");

                msb250x_udc_clear_ep0_opr();

				if(crq->bRequestType == USB_RECIP_DEVICE)
				{
					if(crq->wValue==0x02)//USB20_TEST_MODE
					{
						//nUsb20TestMode=crq->wIndex;
					}
				}
                if (crq->bRequestType != USB_RECIP_ENDPOINT)
                    break;

                if (crq->wValue != USB_ENDPOINT_HALT || crq->wLength != 0)
                    break;

                msb250x_udc_set_halt(&dev->ep[crq->wIndex & 0x7f].ep, 1);
                msb250x_udc_set_ep0_de_out();
                return;

            default:
                msb250x_udc_clear_ep0_opr();
                break;
        }
    }
    else
    {
        msb250x_udc_clear_ep0_opr();
    }

    if (crq->bRequestType & USB_DIR_IN){
#ifdef test    
    		printk("EP0_IN_DATA_PHASE\n");
#endif    		
        dev->ep0state = EP0_IN_DATA_PHASE;
        }
    else{
#ifdef test    
    		printk("EP0_OUT_DATA_PHASE\n");
#endif    		
        dev->ep0state = EP0_OUT_DATA_PHASE;
				}
    if (dev->driver && dev->driver->setup)
        ret = dev->driver->setup(&dev->gadget, crq);
    else
        ret = -EINVAL;
    if (ret < 0)
    {
        if (dev->req_config)
        {
            DBG("config change %02x fail %d?\n",
                        crq->bRequest, ret);
            return;
        }

        if (ret == -EOPNOTSUPP)
            DBG("Operation not supported\n");
        else
            DBG("dev->driver->setup failed. (%d)\n", ret);

        udelay(5);
        msb250x_udc_set_ep0_ss();
        msb250x_udc_set_ep0_de_out();

        dev->ep0state = EP0_IDLE;

        /* deferred i/o == no response yet */
    }
    else if (dev->req_pending)
    {
        DBG("dev->req_pending... what now?\n");
        dev->req_pending = 0;
    }

    DBG("ep0state %s\n", ep0states[dev->ep0state]);
}
#if 0
static void xx_msb250x_udc_handle_ep0_idle(struct msb250x_udc *dev,
                    struct msb250x_ep *ep,
                    struct usb_ctrlrequest *crq,
                    u32 ep0csr)
{
    int len = 0, ret = 0 , tmp = 0;

    /* start control request? */
    if (!(ep0csr & MSB250X_UDC_CSR0_RXPKTRDY))
        return;
	
    msb250x_udc_nuke(dev, ep, -EPROTO);

    len = msb250x_udc_read_fifo_ctl_req(crq);
    if (len != sizeof(*crq))
    {
        printk(KERN_ERR "setup begin: fifo READ ERROR"
             " wanted %d bytes got %d. Stalling out...\n",
          sizeof(*crq), len);

        msb250x_udc_set_ep0_ss();
            return;
    }

    DBG("bRequest = %x bRequestType %x wLength = %d\n",
            crq->bRequest, crq->bRequestType, crq->wLength);

    /* cope with automagic for some standard requests. */
    dev->req_std = (crq->bRequestType & USB_TYPE_MASK)    == USB_TYPE_STANDARD;
    dev->req_config = 0;
    dev->req_pending = 1;

    switch (crq->bRequest)
    {
        case USB_REQ_SET_CONFIGURATION:
            DBG("USB_REQ_SET_CONFIGURATION ... \n");

            if (crq->bRequestType == USB_RECIP_DEVICE)
            {
                dev->req_config = 1;
                msb250x_udc_set_ep0_de_out();
            }
            break;

        case USB_REQ_SET_INTERFACE:
            DBG("USB_REQ_SET_INTERFACE ... \n");

            if (crq->bRequestType == USB_RECIP_INTERFACE)
            {
                dev->req_config = 1;
                msb250x_udc_set_ep0_de_out();
            }
            break;

        case USB_REQ_SET_ADDRESS:
            DBG("USB_REQ_SET_ADDRESS ... \n");
            //g_charger_flag = 0;

            if (crq->bRequestType == USB_RECIP_DEVICE)
            {
                tmp = crq->wValue & 0x7F;
                dev->address = tmp;
                udc_write8(tmp, MSB250X_UDC_FADDR_REG);
                msb250x_udc_set_ep0_de_out();
                return;
            }
            break;

        case USB_REQ_GET_STATUS:
            DBG("USB_REQ_GET_STATUS ... \n");

            msb250x_udc_clear_ep0_opr();
            if (dev->req_std)
            {
                if (!msb250x_udc_get_status(dev, crq))
                {
                    return;
                }
            }
            break;

        case USB_REQ_CLEAR_FEATURE:
            DBG("USB_REQ_CLEAR_FEATURE ... \n");

            msb250x_udc_clear_ep0_opr();
            if (crq->bRequestType != USB_RECIP_ENDPOINT)
                break;

            if (crq->wValue != USB_ENDPOINT_HALT || crq->wLength != 0)
                break;

            msb250x_udc_set_halt(&dev->ep[crq->wIndex & 0x7f].ep, 0);
            msb250x_udc_set_ep0_de_out();
            return;

        case USB_REQ_SET_FEATURE:
            DBG("USB_REQ_SET_FEATURE ... \n");

            msb250x_udc_clear_ep0_opr();
            if (crq->bRequestType != USB_RECIP_ENDPOINT)
                break;

            if (crq->wValue != USB_ENDPOINT_HALT || crq->wLength != 0)
                break;

            msb250x_udc_set_halt(&dev->ep[crq->wIndex & 0x7f].ep, 1);
            msb250x_udc_set_ep0_de_out();
            return;

        default:
            msb250x_udc_clear_ep0_opr();
            break;
    }

    if (crq->bRequestType & USB_DIR_IN)
        dev->ep0state = EP0_IN_DATA_PHASE;
    else
        dev->ep0state = EP0_OUT_DATA_PHASE;

    ret = dev->driver->setup(&dev->gadget, crq);
    if (ret < 0)
    {
        if (dev->req_config)
        {
            DBG("config change %02x fail %d?\n",
                        crq->bRequest, ret);
            return;
        }

        if (ret == -EOPNOTSUPP)
            DBG("Operation not supported\n");
        else
            DBG("dev->driver->setup failed. (%d)\n", ret);

        udelay(5);
        msb250x_udc_set_ep0_ss();
        msb250x_udc_set_ep0_de_out();

        dev->ep0state = EP0_IDLE;

        /* deferred i/o == no response yet */
    }
    else if (dev->req_pending)
    {
        DBG("dev->req_pending... what now?\n");
        dev->req_pending=0;
    }

    DBG("ep0state %s\n", ep0states[dev->ep0state]);
}
#endif
/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_handle_ep0
+------------------------------------------------------------------------------
| DESCRIPTION :handle the endpoint 0
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                 |x  |      | msb250x_udc struct point
+--------------------+---+---+-------------------------------------------------
*/
static void msb250x_udc_handle_ep0(struct msb250x_udc *dev)
{
    u32 ep0csr = 0;
#ifdef CB2	
	u32 rxcnt = 0;
#endif
    struct msb250x_ep    *ep = &dev->ep[0];
    struct msb250x_request    *req = NULL;
    struct usb_ctrlrequest    crq;

    if (list_empty(&ep->queue))
        req = NULL;
    else
        req = list_entry(ep->queue.next, struct msb250x_request, queue);

    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //ep0csr = udc_read8(MSB250X_UDC_CSR0_REG);
    ep0csr = udc_read8(MSB250X_USBCREG(0x102));
    rxcnt =	udc_read8(MSB250X_USBCREG(0x108));
	
    /* clear stall status */
    if (ep0csr & MSB250X_UDC_CSR0_SENTSTALL)
    {
        DBG("... clear SENT_STALL ...\n");
        msb250x_udc_nuke(dev, ep, -EPIPE);
        msb250x_udc_clear_ep0_sst();
        dev->ep0state = EP0_IDLE;
        return;
    }

    /* clear setup end */
    if (ep0csr & MSB250X_UDC_CSR0_SETUPEND)
    {
        DBG("... serviced SETUP_END ...\n");
        msb250x_udc_nuke(dev, ep, 0);
        msb250x_udc_clear_ep0_se();
        dev->ep0state = EP0_IDLE;
    }

    switch (dev->ep0state)
    {
        case EP0_IDLE:
            msb250x_udc_handle_ep0_idle(dev, ep, &crq, ep0csr);
            break;

        case EP0_IN_DATA_PHASE:     /* GET_DESCRIPTOR etc */
            DBG("EP0_IN_DATA_PHASE ... what now?\n");
            if (!(ep0csr & MSB250X_UDC_CSR0_TXPKTRDY) && req)
            {
                msb250x_udc_write_fifo(ep, req);
            }
            break;

        case EP0_OUT_DATA_PHASE:    /* SET_DESCRIPTOR etc */
            DBG("EP0_OUT_DATA_PHASE ... what now?\n");
            if ((ep0csr & MSB250X_UDC_CSR0_RXPKTRDY) && req )
            {
                msb250x_udc_read_fifo(ep,req);
            }
            break;

        case EP0_END_XFER:
            DBG("EP0_END_XFER ... what now?\n");
            dev->ep0state = EP0_IDLE;
            break;

        case EP0_STALL:
            DBG("EP0_STALL ... what now?\n");
            dev->ep0state = EP0_IDLE;
            break;

        default:
            DBG("EP0 status ... what now?\n");
            break;
    }

}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_handle_ep
+------------------------------------------------------------------------------
| DESCRIPTION :handle the endpoint except endpoint 0
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| ep                        |x  |      | msb250x_ep struct point
+--------------------+---+---+-------------------------------------------------
*/
int i=0;
extern s8 dma_busy;
static void msb250x_udc_handle_ep(struct msb250x_ep *ep)
{
    struct msb250x_request    *req = NULL;
    int is_in = 0;
    u32 ep_csr1 = 0;
    u32 idx = 0;

    if (likely (!list_empty(&ep->queue)))
    {
#ifdef TX_log
		printk("find ep%x req\n",ep->bEndpointAddress);
#endif
		//putb("find ep req",ep->bEndpointAddress,0);
        req = list_entry(ep->queue.next, struct msb250x_request, queue);
    }
    else
    {
#ifdef TX_log    
    	printk("no find ep%x req in queue\n",ep->bEndpointAddress);
#endif
		//putb("no find ep req",ep->bEndpointAddress,0);
        req = NULL;
    }
    idx = ep->bEndpointAddress & 0x7F;
    is_in = ep->bEndpointAddress & USB_DIR_IN;
	
    if (is_in)
    { 
    	#if 0
    	//if(dma_busy==1)
			//printk("[USB]DMA BUSY!!!!\n");
		//if (idx==(DmaTxMode1 >>4))		//	ep is correct
		if(test ==1)
		{
		  if (((DmaTxMode1))==1)
		  {
			  udc_write8(idx, MSB250X_UDC_INDEX_REG);
			  ep_csr1 = udc_read8(MSB250X_UDC_TXCSR1_REG);
			  //ep_csr2 = udc_read8(MSB250X_UDC_TXCSR2_REG);
			  //printk("txcsr1:%x,txcsr2:%x,ep:%x\n",ep_csr1,ep_csr2,ep->bEndpointAddress);
			  printk("DMA->TXINT:%x\n",Get32BitsReg((u32 volatile*)DMA_COUNT_REGISTER(2)));
			  return;
		  }
		 
		}	
		#endif
#ifdef TX_log    
    	printk("[USB]TX_INTTRUPT\n");
#endif
#ifdef TX_modify	
#ifdef TX_log    
		printk("msb250x_udc_handle_ep--DONE\n");
#endif
		 //putb("TX_INTTRUPT",0,0);
         if (req==NULL){
		 		printk("[USB]TX_REQ NULL\n");
				//putb("TX_REQ NULL",0,0);
         }
		 msb250x_udc_done(ep, req, 0);
#endif
		//remove index
        //udc_write8(idx, MSB250X_UDC_INDEX_REG);
        //ep_csr1 = udc_read8(MSB250X_UDC_TXCSR1_REG);
		if(idx==1)
			ep_csr1 = udc_read8(MSB250X_USBCREG(0x112));
		else if(idx==3)
			ep_csr1 = udc_read8(MSB250X_USBCREG(0x132));

		
        DBG("ep%01d write csr:%02x %d\n", idx, ep_csr1, req ? 1 : 0);

        if (ep_csr1 & MSB250X_UDC_TXCSR1_SENTSTALL)
        {
            DBG("tx st\n");
			printk("tx st\n");
			//remove index
            //udc_write8(idx, MSB250X_UDC_INDEX_REG);
            if(idx==1)
				udc_write8(ep_csr1 & ~MSB250X_UDC_TXCSR1_SENTSTALL, MSB250X_USBCREG(0x112));
			else if(idx==3)
				udc_write8(ep_csr1 & ~MSB250X_UDC_TXCSR1_SENTSTALL, MSB250X_USBCREG(0x132));			
            //udc_write8(ep_csr1 & ~MSB250X_UDC_TXCSR1_SENTSTALL, MSB250X_UDC_TXCSR1_REG);
            return;
        }
#ifdef TX_modify
#ifdef CONFIG_USB_MSB250X_DMA
		  msb250x_udc_schedule_done(ep);
#endif
#endif
#ifndef TX_modify
        if(req != NULL)
        {
            msb250x_udc_do_request(ep, req);
        }
#endif		
    }
    else
    {
#ifdef RX_mode1_log        
    	printk("[USB]RX_INTTRUPT:%x\n",ep->bEndpointAddress);
#endif
#ifdef NAK_MODIFY
	//putb("RX_INTTRUPT",0,0);
	//auto nak	, not allow any data out ACK
	   //printk("[USB handle_ep]MSB250X_UDC_DMA_MODE_CTL:%x\n",udc_read16(MSB250X_UDC_DMA_MODE_CTL));   	
	   if(((ep->bEndpointAddress)&0x0f)==2){	   
	   	//putb("rx_int",udc_read8(MSB250X_UDC_DMA_MODE_CTL),udc_read8(MSB250X_UDC_USB_CFG5_H));
         udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|(M_Mode1_P_NAK_Enable)), MSB250X_UDC_DMA_MODE_CTL);
	   	}
	   else if (((ep->bEndpointAddress)&0x0f)==4)
	   	  udc_write8((udc_read8(MSB250X_UDC_DMA_MODE_CTL1)|(M_Mode1_P_NAK_Enable_1)), MSB250X_UDC_DMA_MODE_CTL1);
#endif
		//remove index
        //udc_write8(idx, MSB250X_UDC_INDEX_REG);
        //ep_csr1 = udc_read8(MSB250X_UDC_RXCSR1_REG);
		if(idx==2)
			ep_csr1 = udc_read8(MSB250X_USBCREG(0x126));

        DBG("ep%01d rd csr:%02x\n", idx, ep_csr1);

        if (ep_csr1 & MSB250X_UDC_RXCSR1_SENTSTALL)
        {
            DBG("rx st\n");
			//remove index
            //udc_write8(idx, MSB250X_UDC_INDEX_REG);
            if(idx==2)
				udc_write8(ep_csr1 & ~MSB250X_UDC_RXCSR1_SENTSTALL, MSB250X_USBCREG(0x126));	
            //udc_write8(ep_csr1 & ~MSB250X_UDC_RXCSR1_SENTSTALL, MSB250X_UDC_RXCSR1_REG);
            return;
        }
#ifdef RX_modify_mode1		
#ifdef CONFIG_USB_MSB250X_DMA
	 if(req != NULL)
		RxHandler(ep,req);
	 else
	 {
#ifdef RX_mode1_log
	 	printk("req is null\n");
#endif
		//putb("req is null",0,0);
	 }
#endif	 
#else	 
        if(req != NULL){
			
			//printk("[INT]ep%x len %d\n",
			//				 ep->bEndpointAddress, req->req.length);	
			//printk("[USB]check_dma:%x\n",check_dma_busy());
            msb250x_udc_do_request(ep, req);
						//udc_write16(M_Mode1_P_NAK_Enable, MSB250X_UDC_DMA_MODE_CTL); //allow ack,wayne added
						//udc_write16(M_Mode1_P_NAK_Enable|M_Mode1_P_AllowAck , MSB250X_UDC_DMA_MODE_CTL);		
						//udc_write16((M_Mode1_P_OK2Rcv|M_Mode1_P_NAK_Enable|1), MSB250X_UDC_DMA_MODE_CTL);
        	}
		else
			printk("req is null\n");
#endif		
    }

}
#if 0
static void xx_msb250x_udc_handle_ep(struct msb250x_ep *ep)
{
    struct msb250x_request    *req = NULL;
    int is_in = 0;
    u32 ep_csr1 = 0;
    u32 idx = 0;

    if (likely (!list_empty(&ep->queue)))
        req = list_entry(ep->queue.next, struct msb250x_request, queue);
    else
        req = NULL;

    idx = ep->bEndpointAddress & 0x7F;
    is_in = ep->bEndpointAddress & USB_DIR_IN;

    if (is_in)
    {
        udc_write8(idx, MSB250X_UDC_INDEX_REG);
        ep_csr1 = udc_read8(MSB250X_UDC_TXCSR1_REG);

        DBG("ep%01d write csr:%02x %d\n", idx, ep_csr1, req ? 1 : 0);

        if (ep_csr1 & MSB250X_UDC_TXCSR1_SENTSTALL)
        {
            DBG("tx st\n");
            udc_write8(idx, MSB250X_UDC_INDEX_REG);
            udc_write8(ep_csr1 & ~MSB250X_UDC_TXCSR1_SENTSTALL, MSB250X_UDC_TXCSR1_REG);
            return;
        }
#ifdef CB2
		if(req != NULL)
			 msb250x_udc_do_request(ep, req);
#else
        if (!(ep_csr1 & MSB250X_UDC_TXCSR1_TXPKTRDY) && req)
        {
            msb250x_udc_write_fifo(ep,req);
        }
#endif
    }
    else
    {
        udc_write8(idx, MSB250X_UDC_INDEX_REG);
        ep_csr1 = udc_read8(MSB250X_UDC_RXCSR1_REG);

        DBG("ep%01d rd csr:%02x\n", idx, ep_csr1);

        if (ep_csr1 & MSB250X_UDC_RXCSR1_SENTSTALL)
        {
            DBG("rx st\n");
            udc_write8(idx, MSB250X_UDC_INDEX_REG);
            udc_write8(ep_csr1 & ~MSB250X_UDC_RXCSR1_SENTSTALL, MSB250X_UDC_RXCSR1_REG);
            return;
        }
#ifdef CB2
        if(req != NULL)
            msb250x_udc_do_request(ep, req);
#else
        if ((ep_csr1 & MSB250X_UDC_RXCSR1_RXPKTRDY) && req)
        {
            msb250x_udc_read_fifo(ep,req);
        }
#endif		
    }

}
#endif
static void wakeup_connection_change_event(struct msb250x_udc *dev)
{
    u16 new_linestate;
    u8 new_soft_conn;

    mdelay(100);

    new_linestate = (udc_read8(UTMI_SIGNAL_STATUS)>>6) & 0x3;
    new_soft_conn = udc_read8(MSB250X_UDC_PWR_REG)&(MSB250X_UDC_PWR_SOFT_CONN);
    if ((old_linestate != new_linestate) || (old_soft_conn != new_soft_conn))
    {
        /* wake up event queue. */
        dev->conn_chg = 1;
        wake_up_interruptible(&dev->event_q);
        // printk("old linestate 0x%04x new linestate 0x%04x\n", old_linestate, new_linestate);
        // printk("old soft_conn 0x%04x new soft_conn 0x%04x\n", old_soft_conn, new_soft_conn);
    }
    else
    {
        dev->conn_chg = 0;
        // printk("old linestate 0x%04x new linestate 0x%04x\n", old_linestate, new_linestate);
        // printk("old soft_conn 0x%04x new soft_conn 0x%04x\n", old_soft_conn, new_soft_conn);
    }

    old_linestate = new_linestate;
    old_soft_conn = new_soft_conn;

    return;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_irq_init
+------------------------------------------------------------------------------
| DESCRIPTION :initial the PIC's USB interrupt
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                         |    |      |
+--------------------+---+---+-------------------------------------------------
*/
#if 0
static void msb250x_udc_irq_init(void)
{
    unsigned short int tmp=0;

    /* set usb otg interrupt to high trigger. write 0 */
    tmp = udc_read16(0xa0005650);
    tmp &= ~(0x01 << INT_OTG);
    udc_write16(tmp, 0xa0005650);

    /* clear usb otg interrupt mask */
    tmp = udc_read16(0xa0005630);
    tmp &= ~(0x01 << INT_OTG);
    udc_write16(tmp, 0xa0005630);
}
#endif
/*
+------------------------------------------------------------------------------
| FUNCTION    : USB_ReSet_ClrRXMode1
+------------------------------------------------------------------------------
| DESCRIPTION : Reset DMA control engine 
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
void USB_ReSet_ClrRXMode1(void)
{
      udc_write16(0, MSB250X_UDC_DMA_MODE_CTL);
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_irq
+------------------------------------------------------------------------------
| DESCRIPTION :the USB interrupt service routine
|
| RETURN      : non-zero when the irq be handled
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| irq                        |x  |      | irq number
|--------------------+---+---+-------------------------------------------------
| _dev                 |x  |      | irq argument
+--------------------+---+---+-------------------------------------------------
*/
#if 0
static irqreturn_t xx_msb250x_udc_irq(int irq, void *_dev)
{
    struct msb250x_udc *dev = _dev;
    int usb_status = 0;
    int usb_intrx_status = 0, usb_inttx_status = 0;
    int pwr_reg = 0;
    int i = 0;
    //u32 idx = 0;
    unsigned long flags;

#if 0
/* FFixMEE */
    Mstar_Chip_DisableInterrupt(dev->irq);
#endif
    spin_lock_irqsave(&dev->lock, flags);

    /* Save index */
    //idx = udc_read8(MSB250X_UDC_INDEX_REG);

    /*
     * Read status registers, note also that all active interrupts are cleared
     * when this register is read.
     */
    usb_status = udc_read8(MSB250X_UDC_INTRUSB_REG);
    udc_write8(usb_status, MSB250X_UDC_INTRUSB_REG);

    usb_intrx_status = udc_read16(MSB250X_UDC_INTRRX_REG);
    usb_inttx_status = udc_read16(MSB250X_UDC_INTRTX_REG);
    udc_write16(usb_intrx_status, MSB250X_UDC_INTRRX_REG);
    udc_write16(usb_inttx_status, MSB250X_UDC_INTRTX_REG);

#if defined(CONFIG_ARCH_MSW8533X)
    HalIntcAck(irq);
    HalIntcClear(irq);
#endif
    pwr_reg = udc_read8(MSB250X_UDC_PWR_REG);

    DBG("usbs=%02x, usb_intrxs=%02x, usb_inttxs=%02x pwr=%02x\n",
        usb_status, usb_intrx_status, usb_inttx_status, pwr_reg);

#ifdef CONFIG_USB_MSB250X_DMA
{
    u8 dma_intr;
    dma_intr = udc_read8(M_REG_DMA_INTR);
    udc_write8(dma_intr, M_REG_DMA_INTR);

    if (dma_intr)
    {
        DBG("dma_intr: %x \n",dma_intr);
        for (i = 0; i < MAX_USB_DMA_CHANNEL; i++)
        {
            if (dma_intr & (1 << i))
            {
                u8 ch;

                ch = i + 1;
                USB_DMA_IRQ_Handler(ch, dev);
            }
        }
    }
}
#endif /* CONFIG_USB_MSB250X_DMA */
    /*
     * Now, handle interrupts. There's two types :
     * - Reset, Resume, Suspend coming -> usb_int_reg
     * - EP -> ep_int_reg
     */

    /* RESET */
    if (usb_status & MSB250X_UDC_INTRUSB_RESET)
    {
        /* two kind of reset :
         * - reset start -> pwr reg = 8
         * - reset end   -> pwr reg = 0
         **/
        DBG("USB reset pwr %x\n", pwr_reg);

		UTMI_REG_WRITE8(0x58,0x10); //TX-current adjust to 105%=> bit <4> set 1
		UTMI_REG_WRITE8(0x5A,0x02); // Pre-emphasis enable=> bit <1> set 1
		UTMI_REG_WRITE8(0x5E,0x01);	//HS_TX common mode current enable (100mV)=> bit <7> set 1
		//    printk("usbs=%02x, usb_intrxs=%02x, usb_inttxs=%02x pwr=%02x\n",
        //usb_status, usb_intrx_status, usb_inttx_status, pwr_reg);							//Pre-emphasis enable (10%)=> bit <0> set 1
        if (dev->driver && dev->driver->disconnect)
            dev->driver->disconnect(&dev->gadget);

        // Re-enable power
        //UTMI_REG_WRITE16(0, UTMI_REG_IREF_PDN);

        // clear function addr
        udc_write8(0, MSB250X_UDC_FADDR_REG);

        dev->address = 0;
        dev->ep0state = EP0_IDLE;
        if (udc_read8(MSB250X_UDC_PWR_REG)&MSB250X_UDC_PWR_HS_MODE)
        {
            dev->gadget.speed = USB_SPEED_HIGH;
            UTMI_REG_WRITE16(0x58, 0x0230); //B2 analog parameter
        }
        else
        {
            dev->gadget.speed = USB_SPEED_FULL;
            UTMI_REG_WRITE16(0x58, 0x0030); //B2 analog parameter
        }
        /* Restore old index */
        //udc_write8(idx, MSB250X_UDC_INDEX_REG);

#if defined(CONFIG_USB_MSB250X_DMA)
        if(using_dma && dev->DmaRxMode == DMA_RX_MODE1)
        {
            DBG("DMA RX Mode1 ReSetting \n");
            *(DMA_CNTL_REGISTER(MAX_USB_DMA_CHANNEL))=USB_Read_DMA_Control(MAX_USB_DMA_CHANNEL)&0xfe;
            USB_ReSet_ClrRXMode1();
			udc_write8((udc_read8(MSB250X_UDC_USB_CFG6_H)|0x20), MSB250X_UDC_USB_CFG6_H);//short_mode
            //udc_write16((udc_read16(MSB250X_UDC_EP_BULKOUT)&0xfff0), MSB250X_UDC_EP_BULKOUT); //wayne added
            //udc_write16((udc_read16(MSB250X_UDC_EP_BULKOUT)|M_Mode1_P_BulkOut_EP), MSB250X_UDC_EP_BULKOUT); //wayne added

			/*  set autonak for ep2  */
            udc_write16(M_Mode1_P_BulkOut_EP, MSB250X_UDC_EP_BULKOUT);
            udc_write16(udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_NAK_Enable, MSB250X_UDC_DMA_MODE_CTL); //allow ack,wayne added
            //printk("[USB BUS_RESET]MSB250X_UDC_DMA_MODE_CTL:%x\n",udc_read16(MSB250X_UDC_DMA_MODE_CTL));
            //udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|(64*1024)/512), MSB250X_UDC_DMA_MODE_CTL); //allow ack,wayne added

			/*  set autonak for ep4  */
            //udc_write16(M_Mode1_P_BulkOut_EP_4, MSB250X_UDC_EP_BULKOUT1);
			udc_write8(M_Mode1_P_NAK_Enable_1|M_Mode1_P_BulkOut_EP_4, MSB250X_UDC_DMA_MODE_CTL1); //allow ack,wayne added
        }
#else
			//printk("set allow ack for fifo\n");
			//udc_write16(M_Mode1_P_BulkOut_EP, MSB250X_UDC_EP_BULKOUT);
			//udc_write16(M_Mode1_P_NAK_Enable, MSB250X_UDC_DMA_MODE_CTL); //allow ack,wayne added
			//udc_write16(M_Mode1_P_NAK_Enable|M_Mode1_P_AllowAck , MSB250X_UDC_DMA_MODE_CTL);
			//udc_write16((M_Mode1_P_OK2Rcv|M_Mode1_P_NAK_Enable|1), MSB250X_UDC_DMA_MODE_CTL);
#endif

        //wakeup_connection_change_event(dev);
        msb250x_udc_kick_intr_bh();

        DBG("irq: %d msb250x_udc_done reset: %x\n", irq, usb_status);

    }

    /* RESUME */
    if (usb_status & MSB250X_UDC_INTRUSB_RESUME)
    {
        DBG("USB resume\n");

        if (dev->gadget.speed != USB_SPEED_UNKNOWN
                && dev->driver
                && dev->driver->resume)
            dev->driver->resume(&dev->gadget);

        //wakeup_connection_change_event(dev);
        msb250x_udc_kick_intr_bh();

    }

    /* SUSPEND */
    if (usb_status & MSB250X_UDC_INTRUSB_SUSPEND)
    {
        DBG("USB suspend\n");

#if defined( CONFIG_USB_CHARGER_DETECT )
        if(plugged == USB_PLUG)
        {
            printk(KERN_INFO "wake up kplugd\n");
            wake_up_interruptible(&kplugd_wait);
        }
        else if(plugged == USB_HOST)
        {
#endif
            if (dev->driver && dev->driver->disconnect)
                dev->driver->disconnect(&dev->gadget);

            if (dev->gadget.speed != USB_SPEED_UNKNOWN
                    && dev->driver
                    && dev->driver->suspend)
                dev->driver->suspend(&dev->gadget);

            dev->ep0state = EP0_IDLE;

            //wakeup_connection_change_event(dev);
            msb250x_udc_kick_intr_bh();

            //UTMI_REG_WRITE16(0, UTMI_REG_SUSPEND_PDN);
#if defined( CONFIG_USB_CHARGER_DETECT )
        }
#endif
    }

    /* EP */
    /* control traffic */
    /*
     * check on ep0csr != 0 is not a good idea as clearing in_pkt_ready
     * generate an interrupt
     */
    if (usb_inttx_status & MSB250X_UDC_INTRTX_EP0)
    {
        DBG("USB ep0 irq\n");
        msb250x_udc_handle_ep0(dev);
    }

    for (i = 1; i < MSB250X_ENDPOINTS; i++)
    {
        u32 tmp = 1 << i;
        if ((usb_inttx_status & tmp) || (usb_intrx_status & tmp))
        {
            DBG("USB ep%d irq\n", i);
            msb250x_udc_handle_ep(&dev->ep[i]);
        }
    }

    DBG("irq: %d msb250x_udc_done %x.\n", irq, usb_intrx_status);

    /* Restore old index */
    //udc_write8(idx, MSB250X_UDC_INDEX_REG);
    spin_unlock_irqrestore(&dev->lock, flags);

#if 0
/* FFixMEE */
    Mstar_Chip_EnableInterrupt(dev->irq);
#endif

    return IRQ_HANDLED;
}
#endif
static irqreturn_t msb250x_udc_irq(int irq, void *_dev)
{
    struct msb250x_udc *dev = _dev;
    int usb_status = 0;
    int usb_intrx_status = 0, usb_inttx_status = 0;
    int pwr_reg = 0;
    //int ep0csr = 0;
    int i = 0;
    //u32 idx = 0;
    unsigned long flags,sets;
	
    spin_lock_irqsave(&dev->lock, flags);

    /* Save index */
    //idx = udc_read8(MSB250X_UDC_INDEX_REG);	
	//udc_write8(usb_status, MSB250X_UDC_INTRUSB_REG);
	
    /* Read status registers, note also that all active interrupts are cleared
    when this register is read. */
    usb_status = udc_read8(MSB250X_UDC_INTRUSB_REG);
	udc_write8(usb_status, MSB250X_UDC_INTRUSB_REG);
	
    usb_intrx_status = udc_read16(MSB250X_UDC_INTRRX_REG);
    usb_inttx_status = udc_read16(MSB250X_UDC_INTRTX_REG);
    udc_write16(usb_intrx_status, MSB250X_UDC_INTRRX_REG);
    udc_write16(usb_inttx_status, MSB250X_UDC_INTRTX_REG);

    pwr_reg = udc_read8(MSB250X_UDC_PWR_REG);

    //udc_write8(MSB250X_UDC_INDEX_EP0, MSB250X_UDC_INDEX_REG);
    //ep0csr = udc_read8(MSB250X_UDC_CSR0_REG);

    DBG("usbs=%02x, usb_intrxs=%02x, usb_inttxs=%02x pwr=%02x ep0csr=%02x\n",
        usb_status, usb_intrx_status, usb_inttx_status, pwr_reg, ep0csr);
    // udelay(500);
#ifdef CONFIG_USB_MSB250X_DMA
{
    u8 dma_intr;

    dma_intr = udc_read8(M_REG_DMA_INTR);
	udc_write8(dma_intr, M_REG_DMA_INTR);
    if (dma_intr)
    {
        //DBG("########## dma_intr: %x #########\n",dma_intr);
        for (i = 0; i < MAX_DMA_CHANNEL; i++)
        {
            if (dma_intr & (1 << i))
            {
                u8 ch;

                ch = i + 1;
                USB_DMA_IRQ_Handler(ch, dev);
            }
        }
    }
}
#endif /* CONFIG_USB_MSB250X_DMA */
    /*
     * Now, handle interrupts. There's two types :
     * - Reset, Resume, Suspend coming -> usb_int_reg
     * - EP -> ep_int_reg
     */

    /* RESET */
    if (usb_status & MSB250X_UDC_INTRUSB_RESET)
    {
        /* two kind of reset :
         * - reset start -> pwr reg = 8
         * - reset end   -> pwr reg = 0
         **/
	printk("#######======>>>>>hello_bus_reset\n");
        //DBG("USB reset csr %x pwr %x\n", ep0csr, pwr_reg);
		UTMI_REG_WRITE8(0x58,0x10); //TX-current adjust to 105%=> bit <4> set 1
		UTMI_REG_WRITE8(0x5A,0x02); // Pre-emphasis enable=> bit <1> set 1
		UTMI_REG_WRITE8(0x5E,0x01);	//HS_TX common mode current enable (100mV)=> bit <7> set 1
        if (dev->driver && dev->driver->disconnect)
        {
            dev->driver->disconnect(&dev->gadget);
        }

        // Re-enable power
        //UTMI_REG_WRITE16(0, UTMI_REG_IREF_PDN);

        // clear function addr
        udc_write8(0, MSB250X_UDC_FADDR_REG);

        dev->address = 0;
        dev->ep0state = EP0_IDLE;
        if (udc_read8(MSB250X_UDC_PWR_REG)&MSB250X_UDC_PWR_HS_MODE)
        {
        	dev->gadget.speed = USB_SPEED_HIGH;
			UTMI_REG_WRITE16(0x58, 0x0230); //B2 analog parameter
        }
        else
        {
        	dev->gadget.speed = USB_SPEED_FULL;
			UTMI_REG_WRITE16(0x58, 0x0030); //B2 analog parameter
        }
        /* Restore old index */
        //udc_write8(idx, MSB250X_UDC_INDEX_REG);

#if defined(CONFIG_USB_MSB250X_DMA)
        if(using_dma && dev->DmaRxMode == DMA_RX_MODE1)
        {
            DBG("DMA RX Mode1 ReSetting \n");
            *(DMA_CNTL_REGISTER(MAX_USB_DMA_CHANNEL))=USB_Read_DMA_Control(MAX_USB_DMA_CHANNEL)&0xfe;
            USB_ReSet_ClrRXMode1();
			udc_write8((udc_read8(MSB250X_UDC_USB_CFG6_H)|0x20), MSB250X_UDC_USB_CFG6_H);//short_mode
            //udc_write16((udc_read16(MSB250X_UDC_EP_BULKOUT)&0xfff0), MSB250X_UDC_EP_BULKOUT); //wayne added
            //udc_write16((udc_read16(MSB250X_UDC_EP_BULKOUT)|M_Mode1_P_BulkOut_EP), MSB250X_UDC_EP_BULKOUT); //wayne added

			/*  set autonak for ep2  */
            udc_write16(M_Mode1_P_BulkOut_EP, MSB250X_UDC_EP_BULKOUT);
            udc_write16(udc_read16(MSB250X_UDC_DMA_MODE_CTL)|M_Mode1_P_NAK_Enable, MSB250X_UDC_DMA_MODE_CTL); //allow ack,wayne added
            //printk("[USB BUS_RESET]MSB250X_UDC_DMA_MODE_CTL:%x\n",udc_read16(MSB250X_UDC_DMA_MODE_CTL));
            //udc_write16((udc_read16(MSB250X_UDC_DMA_MODE_CTL)|(64*1024)/512), MSB250X_UDC_DMA_MODE_CTL); //allow ack,wayne added

			/*  set autonak for ep4  */
            //udc_write16(M_Mode1_P_BulkOut_EP_4, MSB250X_UDC_EP_BULKOUT1);
			udc_write8(M_Mode1_P_NAK_Enable_1|M_Mode1_P_BulkOut_EP_4, MSB250X_UDC_DMA_MODE_CTL1); //allow ack,wayne added
        }
#else
			//printk("set allow ack for fifo\n");
			//udc_write16(M_Mode1_P_BulkOut_EP, MSB250X_UDC_EP_BULKOUT);
			//udc_write16(M_Mode1_P_NAK_Enable, MSB250X_UDC_DMA_MODE_CTL); //allow ack,wayne added
			//udc_write16(M_Mode1_P_NAK_Enable|M_Mode1_P_AllowAck , MSB250X_UDC_DMA_MODE_CTL);
			//udc_write16((M_Mode1_P_OK2Rcv|M_Mode1_P_NAK_Enable|1), MSB250X_UDC_DMA_MODE_CTL);
#endif
        //wakeup_connection_change_event(dev);
        msb250x_udc_kick_intr_bh();
    }

    /* RESUME */
    if (usb_status & MSB250X_UDC_INTRUSB_RESUME)
    {
        DBG("USB resume\n");

        if (dev->gadget.speed != USB_SPEED_UNKNOWN
                && dev->driver
                && dev->driver->resume)
            dev->driver->resume(&dev->gadget);

        wakeup_connection_change_event(dev);
    }

    /* SUSPEND */
    if (usb_status & MSB250X_UDC_INTRUSB_SUSPEND)
    {
        DBG("USB suspend\n");

        if (dev->driver && dev->driver->disconnect) {
            DBG("call gadget->disconnect\n");
            dev->driver->disconnect(&dev->gadget);
        }
        if (dev->gadget.speed != USB_SPEED_UNKNOWN
                && dev->driver
                && dev->driver->suspend) {
            DBG("call gadget->suspend\n");
            dev->driver->suspend(&dev->gadget);
        }
        dev->ep0state = EP0_IDLE;

        wakeup_connection_change_event(dev);

        //UTMI_REG_WRITE16(0, UTMI_REG_SUSPEND_PDN);
    }

    /* EP */
    /* control traffic */
    /* check on ep0csr != 0 is not a good idea as clearing in_pkt_ready
     * generate an interrupt
     */
    if (usb_inttx_status & MSB250X_UDC_INTRTX_EP0)
    {
        DBG("USB ep0 irq\n");
        msb250x_udc_handle_ep0(dev);
    }
    /* endpoint data transfers */
    for (i = 1; i < MSB250X_ENDPOINTS; i++)
    {
        u32 tmp = 1 << i;
        if ((usb_inttx_status & tmp) || (usb_intrx_status & tmp))
        {
        	DBG("USB ep%d irq\n", i);
			spin_lock_irqsave(&dev_lock,sets);//local_irq_save (flags);
            msb250x_udc_handle_ep(&dev->ep[i]);
			spin_unlock_irqrestore(&dev_lock,sets);
			
        }
    }

    DBG("irq: %d msb250x_udc_done %x.\n", irq, usb_intrx_status);

    /* Restore old index */
    //udc_write8(idx, MSB250X_UDC_INDEX_REG);

    spin_unlock_irqrestore(&dev->lock, flags);

    return IRQ_HANDLED;
}

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


/*------------------------- msb250x_ep_ops ----------------------------------*/

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_ep_enable
+------------------------------------------------------------------------------
| DESCRIPTION : configure endpoint, making it usable
|
| RETURN      : zero, or a negative error code.
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                 |x  |       | usb_ep struct point
|--------------------+---+---+-------------------------------------------------
| desc                 |x  |       | usb_endpoint_descriptor struct point
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_ep_enable(struct usb_ep *_ep,
                 const struct usb_endpoint_descriptor *desc)
{
    struct msb250x_udc *dev = NULL;
    struct msb250x_ep *ep = NULL;
    u32 max = 0, tmp = 0;
    u32 csr1 = 0 ,csr2 = 0;
    u32 int_en_reg = 0;
    unsigned long flags;

    ep = to_msb250x_ep(_ep);

    if (!_ep || !desc || ep->desc
            || _ep->name == ep0name
            || desc->bDescriptorType != USB_DT_ENDPOINT)
        return -EINVAL;

    dev = ep->dev;
    if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
        return -ESHUTDOWN;

    max = le16_to_cpu(desc->wMaxPacketSize) & 0x1fff;

    spin_lock_irqsave(&dev_lock,flags);//local_irq_save (flags);
    _ep->maxpacket = max & 0x7ff;
    ep->desc = desc;
    ep->halted = 0;
    ep->bEndpointAddress = desc->bEndpointAddress;

    /* set type, direction, address; reset fifo counters */
    if (desc->bEndpointAddress & USB_DIR_IN)
    {
        csr1 = MSB250X_UDC_TXCSR1_FLUSHFIFO|MSB250X_UDC_TXCSR1_CLRDATAOTG;
        csr2 = MSB250X_UDC_TXCSR2_MODE;
#if 0
        /* set max packet */
        udc_write8(ep->num, MSB250X_UDC_INDEX_REG);
        udc_write16(_ep->maxpacket , MSB250X_UDC_TXMAP_L_REG);

        udc_write8(ep->num, MSB250X_UDC_INDEX_REG);
        udc_write8(csr1, MSB250X_UDC_TXCSR1_REG);
        udc_write8(ep->num, MSB250X_UDC_INDEX_REG);
        udc_write8(csr2, MSB250X_UDC_TXCSR2_REG);
#endif
		if(ep->num==1)
		{
			udc_write16(_ep->maxpacket , MSB250X_USBCREG(0x110));
			udc_write8(csr1, MSB250X_USBCREG(0x112));
			udc_write8(csr2, (MSB250X_USBCREG(0x112))+1);
		}
		else if(ep->num==3)
		{
			udc_write16(_ep->maxpacket , MSB250X_USBCREG(0x130));
			udc_write8(csr1, MSB250X_USBCREG(0x132));
			udc_write8(csr2, (MSB250X_USBCREG(0x132))+1);			
		}

        /* enable irqs */
        int_en_reg = udc_read16(MSB250X_UDC_INTRTXE_REG);
        udc_write16(int_en_reg | (1 << ep->num), MSB250X_UDC_INTRTXE_REG);
    }
    else
    {
        /* enable the enpoint direction as Rx */
#if 0		
        udc_write8(ep->num, MSB250X_UDC_INDEX_REG);
        csr2 = 0;
        udc_write8(csr2, MSB250X_UDC_TXCSR2_REG);
#endif
		csr2 = 0;
		if(ep->num==1)
		{
			udc_write8(csr2, (MSB250X_USBCREG(0x112))+1);
		}
		else if(ep->num==3)
		{
			udc_write8(csr2, (MSB250X_USBCREG(0x132))+1);
		}

         csr1 = MSB250X_UDC_RXCSR1_FLUSHFIFO | MSB250X_UDC_RXCSR1_CLRDATATOG;
         csr2 = 0;
#if 0
        /* set max packet */
        udc_write8(ep->num, MSB250X_UDC_INDEX_REG);
        udc_write16(_ep->maxpacket, MSB250X_UDC_RXMAP_L_REG);

        udc_write8(ep->num, MSB250X_UDC_INDEX_REG);
        udc_write8(csr1, MSB250X_UDC_RXCSR1_REG);
        udc_write8(ep->num, MSB250X_UDC_INDEX_REG);
        udc_write8(csr2, MSB250X_UDC_RXCSR2_REG);
#endif
		if(ep->num==2)
		{
			udc_write16(_ep->maxpacket , MSB250X_USBCREG(0x124));
			udc_write8(csr1, MSB250X_USBCREG(0x126));
			udc_write8(csr2, (MSB250X_USBCREG(0x126))+1);			
			
		}
        /* enable irqs */
        int_en_reg = udc_read16(MSB250X_UDC_INTRRXE_REG);
        udc_write16(int_en_reg | (1 << ep->num), MSB250X_UDC_INTRRXE_REG);
    }

    /* print some debug message */
    tmp = desc->bEndpointAddress;
    DBG ( "enable %s(%d) ep%x%s-blk max %02x\n",
             _ep->name,ep->num, tmp,
            desc->bEndpointAddress & USB_DIR_IN ? "in" : "out", max);

    spin_unlock_irqrestore(&dev_lock,flags);//local_irq_restore (flags);
    msb250x_udc_set_halt(_ep, 0);

    return 0;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_ep_disable
+------------------------------------------------------------------------------
| DESCRIPTION : endpoint is no longer usable
|
| RETURN      : zero, or a negative error code.
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                 |x  |       | usb_ep struct point
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_ep_disable(struct usb_ep *_ep)
{
    struct msb250x_ep *ep = to_msb250x_ep(_ep);
    u32 int_en_reg = 0;
    unsigned long flags;

    if (!_ep || !ep->desc)
    {
        DBG("%s not enabled\n",_ep ? ep->ep.name : NULL);
        return -EINVAL;
    }

    DBG("Entered %s\n", __FUNCTION__);

    spin_lock_irqsave(&dev_lock,flags);//local_irq_save(flags);

    DBG("ep_disable: %s\n", _ep->name);

    ep->desc = NULL;
    ep->halted = 1;

    msb250x_udc_nuke (ep->dev, ep, -ESHUTDOWN);

#ifdef CONFIG_USB_MSB250X_DMA	
	ep->DmaRxMode1=0;
	ep->RxShort=0;
	ep->ch=0;
	free_dma_channels=0x7f;
#endif

    /* disable irqs */
    if(ep->bEndpointAddress & USB_DIR_IN)
    {
        int_en_reg = udc_read16(MSB250X_UDC_INTRTXE_REG);
        udc_write16(int_en_reg & ~(1<<ep->num), MSB250X_UDC_INTRTXE_REG);
    }
    else
    {
        int_en_reg = udc_read16(MSB250X_UDC_INTRRXE_REG);
        udc_write16(int_en_reg & ~(1<<ep->num), MSB250X_UDC_INTRRXE_REG);
    }

    spin_unlock_irqrestore(&dev_lock,flags);//local_irq_restore(flags);

#if 0
	DMA_DUMP();	
#endif

    DBG("%s disabled\n", _ep->name);
//printb();
    return 0;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_alloc_request
+------------------------------------------------------------------------------
| DESCRIPTION : allocate a request object to use with this endpoint
|
| RETURN      : the request, or null if one could not be allocated.
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                 |x  |       | usb_ep struct point
|--------------------+---+---+-------------------------------------------------
| mem_flags          |x  |       | GFP_* flags to use
+--------------------+---+---+-------------------------------------------------
*/
static struct usb_request *
msb250x_udc_alloc_request(struct usb_ep *_ep, gfp_t mem_flags)
{
    struct msb250x_request *req = NULL;

    DBG("Entered %s(%p,%d)\n", __FUNCTION__, _ep, mem_flags);

    if (!_ep)
        return NULL;

    req = kzalloc (sizeof(struct msb250x_request), mem_flags);
    if (!req)
        return NULL;

    INIT_LIST_HEAD (&req->queue);
    return &req->req;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_free_request
+------------------------------------------------------------------------------
| DESCRIPTION : frees a request object
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                 |x  |       | usb_ep struct point
|--------------------+---+---+-------------------------------------------------
| _req                 |x  |       |usb_request struct point
+--------------------+---+---+-------------------------------------------------
*/
static void
msb250x_udc_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
    struct msb250x_ep *ep = to_msb250x_ep(_ep);
    struct msb250x_request    *req = to_msb250x_req(_req);

    DBG("Entered %s(%p,%p)\n", __FUNCTION__, _ep, _req);

    if (!ep || !_req || (!ep->desc && _ep->name != ep0name))
        return;

    WARN_ON (!list_empty (&req->queue));
    kfree(req);
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_queue
+------------------------------------------------------------------------------
| DESCRIPTION : queues (submits) an I/O request to an endpoint
|
| RETURN      : zero, or a negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                 |x  |       | usb_ep struct point
|--------------------+---+---+-------------------------------------------------
| _req                 |x  |       |usb_request struct point
|--------------------+---+---+-------------------------------------------------
| gfp_flags          |x  |       | GFP_* flags to use
+--------------------+---+---+-------------------------------------------------
*/
#if 0
static int xx_msb250x_udc_queue(struct usb_ep *_ep, struct usb_request *_req,
        gfp_t gfp_flags)
{
    struct msb250x_request    *req = to_msb250x_req(_req);
    struct msb250x_ep *ep = to_msb250x_ep(_ep);
    struct msb250x_udc     *dev = NULL;
    unsigned long        flags;

    DBG("Entered %s\n", __FUNCTION__);
#ifdef TX_log		
	printk("[USB]msb250x_udc_queue\n");
#endif
    if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name)))
    {
        DBG("%s: invalid args\n", __FUNCTION__);
        return -EINVAL;
    }

    dev = ep->dev;
    if (unlikely (!dev->driver
            || dev->gadget.speed == USB_SPEED_UNKNOWN))
    {
        return -ESHUTDOWN;
    }

#if 0
/* FFixMEE */
    Mstar_Chip_DisableInterrupt(dev->irq);
#endif

    local_irq_save (flags);

    if (unlikely(!_req || !_req->complete || !_req->buf || !list_empty(&req->queue)))
    {
        if (!_req)
            DBG("%s: 1 X X X\n", __FUNCTION__);
        else
        {
            DBG("%s: 0 %01d %01d %01d\n",
                        __FUNCTION__, !_req->complete,!_req->buf,
                        !list_empty(&req->queue));
        }

        local_irq_restore(flags);
        return -EINVAL;
    }

    _req->status = -EINPROGRESS;
    _req->actual = 0;

    DBG("%s: ep%x len %d\n",
                     __FUNCTION__, ep->bEndpointAddress, _req->length);
//#ifdef TX_modify
//	if(dma_busy==DMA_NOT_BUSY){
//#endif	
//printk(".");
#ifdef	RX_mode1_log
	printk("QUEUE ep%x len %x buf:%p\n",
					 ep->bEndpointAddress, _req->length,_req->buf);
#endif
	//printk("Q%x len %x\n",
	//				 ep->bEndpointAddress, _req->length);
#if 0
	arrary1[idx]=_req->length;
	arrary2[idx]=ep->bEndpointAddress;
	arrary3[idx]=idx;
	idx++;
	if(idx==50)
		idx=0;
#endif	
//	save_tmp++;

    if(list_empty(&ep->queue))
    {

    	req = msb250x_udc_do_request(ep, req);
    }
//#ifdef TX_modify	
//	}
//#endif	

    /* pio or dma irq handler will advance the queue. */
    if (req != NULL){
#ifdef TX_log	
		printk("[USB]add_QUEUE:%x\n",ep->bEndpointAddress);
#endif
        list_add_tail(&req->queue, &ep->queue);
	}

    local_irq_restore(flags);
//	save_tmp--;
#if 0
/* FFixMEE */
    Mstar_Chip_EnableInterrupt(dev->irq);
#endif

    DBG("%s ok and dev->ep0state=%d \n", __FUNCTION__, dev->ep0state);
    return 0;

}
#endif
static int msb250x_udc_queue(struct usb_ep *_ep, struct usb_request *_req,
        gfp_t gfp_flags)
{
    struct msb250x_request    *req = to_msb250x_req(_req);
    struct msb250x_ep *ep = to_msb250x_ep(_ep);
    struct msb250x_udc     *dev = NULL;
    unsigned long        flags;

    DBG("Entered %s\n", __FUNCTION__);
#ifdef TX_log		
	printk("[USB]msb250x_udc_queue\n");
#endif
    if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name)))
    {
        DBG("%s: invalid args\n", __FUNCTION__);
        return -EINVAL;
    }

    dev = ep->dev;
    if (unlikely (!dev->driver
            || dev->gadget.speed == USB_SPEED_UNKNOWN))
    {
        return -ESHUTDOWN;
    }

    spin_lock_irqsave(&dev_lock,flags);//local_irq_save (flags);

    if (unlikely(!_req || !_req->complete
            || !_req->buf || !list_empty(&req->queue)))
    {
        if (!_req)
            DBG("%s: 1 X X X\n", __FUNCTION__);
        else
        {
            DBG("%s: 0 %01d %01d %01d\n",
                        __FUNCTION__, !_req->complete,!_req->buf,
                        !list_empty(&req->queue));
        }

        spin_unlock_irqrestore(&dev_lock,flags);//local_irq_restore(flags);
        return -EINVAL;
    }

    _req->status = -EINPROGRESS;
    _req->actual = 0;

    DBG("%s: ep%x len %d\n",
                     __FUNCTION__, ep->bEndpointAddress, _req->length);

#ifdef	RX_mode1_log
	printk("QUEUE ep%x len %x buf:%p\n",
					 ep->bEndpointAddress, _req->length,_req->buf);
#endif

	//putb("ep len buf",ep->bEndpointAddress, _req->length);
	//tmp_buff=req->req.buf;

	if(list_empty(&ep->queue)) 
	{	//putb("recv ep len",ep->bEndpointAddress, _req->length);
		req = msb250x_udc_do_request(ep, req);
	}
	 /* pio or dma irq handler will advance the queue. */
	if (req != NULL){		
		 list_add_tail(&req->queue, &ep->queue);
#ifdef TX_log			
		 printk("[USB]add_QUEUE:%x\n",ep->bEndpointAddress);
#endif		 
		//putb("add queue ep,len",ep->bEndpointAddress, _req->length);
	}
    spin_unlock_irqrestore(&dev_lock,flags);//local_irq_restore(flags);

    DBG("%s ok and dev->ep0state=%d \n", __FUNCTION__, dev->ep0state);
    return 0;

}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_dequeue
+------------------------------------------------------------------------------
| DESCRIPTION : dequeues (cancels, unlinks) an I/O request from an endpoint
|
| RETURN      : zero, or a negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                |x  |   | usb_ep struct point
|--------------------+---+---+-------------------------------------------------
| _req               |x  |   | usb_request struct point
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
    struct msb250x_ep *ep = to_msb250x_ep(_ep);
    //struct msb250x_udc  *udc = NULL;
    struct msb250x_request    *req = NULL;
    int retval = -EINVAL;
    unsigned long flags;

    DBG("Entered %s(%p,%p)\n", __FUNCTION__, _ep, _req);

    if (!sg_udc_controller->driver)
        return -ESHUTDOWN;

    if (!_ep || !_req)
        return retval;

    // udc = to_msb250x_udc(ep->gadget);

    spin_lock_irqsave(&dev_lock,flags);//local_irq_save (flags);

    list_for_each_entry (req, &ep->queue, queue)
    {
        if (&req->req == _req)
        {
            list_del_init (&req->queue);
            _req->status = -ECONNRESET;
            retval = 0;
            break;
        }
    }

    if (retval == 0)
    {
        DBG("dequeued req %p from %s, len %d buf %p\n",
                    req, _ep->name, _req->length, _req->buf);

        msb250x_udc_done(ep, req, -ECONNRESET);
    }

    spin_unlock_irqrestore(&dev_lock,flags);//local_irq_restore (flags);
    return retval;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_halt
+------------------------------------------------------------------------------
| DESCRIPTION : sets the endpoint halt feature.
|
| RETURN      : zero, or a negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _ep                 |x  |       | usb_ep struct point
|--------------------+---+---+-------------------------------------------------
| value                 |x  |       |set halt or not
+--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_set_halt(struct usb_ep *_ep, int value)
{
    struct msb250x_ep *ep = to_msb250x_ep(_ep);
    u32 ep_csr = 0;
    u32 idx = 0;
    unsigned long flags;
    struct msb250x_request    *preq = NULL;

    if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name)))
    {
        DBG("%s: inval 2\n", __FUNCTION__);
        return -EINVAL;
    }

    if ((!list_empty(&ep->queue)))
        preq = list_entry(ep->queue.next, struct msb250x_request, queue);
    else
        preq = NULL;

    DBG("Entered %s\n", __FUNCTION__);

    spin_lock_irqsave(&dev_lock,flags);//local_irq_save (flags);

    idx = ep->bEndpointAddress & 0x7F;

    ep->halted = value ? 1 : 0;

    if (idx == 0)
    {
        msb250x_udc_set_ep0_ss();
        msb250x_udc_set_ep0_de_out();
    }
    else
    {
#if 0    
        udc_write8(idx, MSB250X_UDC_INDEX_REG);
        ep_csr = udc_read8((ep->bEndpointAddress & USB_DIR_IN)
                              ? MSB250X_UDC_TXCSR1_REG
                              : MSB250X_UDC_RXCSR1_REG);
#endif
		if(idx==1)
			ep_csr = udc_read8(MSB250X_USBCREG(0x112));
		else if(idx==2)
			ep_csr = udc_read8(MSB250X_USBCREG(0x126));
		else if(idx==3)
			ep_csr = udc_read8(MSB250X_USBCREG(0x132));
		
        if ((ep->bEndpointAddress & USB_DIR_IN) != 0)
        {
            if (value)
            {
                if (ep_csr & MSB250X_UDC_TXCSR1_FIFONOEMPTY)
                {
                     DBG("%s fifo busy, cannot halt\n", _ep->name);
                    ep->halted = 0;
                    spin_unlock_irqrestore(&dev_lock,flags);//local_irq_restore (flags);
                    return -EAGAIN;
                }
                 DBG("%s stall\n", _ep->name);
                //udc_write8(ep_csr | MSB250X_UDC_TXCSR1_SENDSTALL,
                //                        MSB250X_UDC_TXCSR1_REG);
                if(idx==1)
					udc_write8(ep_csr | MSB250X_UDC_TXCSR1_SENDSTALL,MSB250X_USBCREG(0x112));
				else if(idx==3)
					udc_write8(ep_csr | MSB250X_UDC_TXCSR1_SENDSTALL,MSB250X_USBCREG(0x132));                
            }
            else
            {
            	printk("[USB]clear stall\n");
#if 0				
                ep_csr &= ~MSB250X_UDC_TXCSR1_SENDSTALL;
                udc_write8(ep_csr, MSB250X_UDC_TXCSR1_REG);
                ep_csr |= MSB250X_UDC_TXCSR1_CLRDATAOTG;
                udc_write8(ep_csr, MSB250X_UDC_TXCSR1_REG);
#endif
                if(idx==1)
                {
                	udc_write8(MSB250X_UDC_TXCSR1_CLRDATAOTG, MSB250X_USBCREG(0x112));
					udc_write8(0,MSB250X_USBCREG(0x112));
                }
				else if(idx==3)
				{
                	udc_write8(MSB250X_UDC_TXCSR1_CLRDATAOTG, MSB250X_USBCREG(0x132));
					udc_write8(0,MSB250X_USBCREG(0x132));
				}
				
                if (!(ep_csr & MSB250X_UDC_TXCSR1_TXPKTRDY) && preq)
                {
                    msb250x_udc_write_fifo(ep, preq);
                }
            }
        }
        else /* out token */
        {
            if (value)
            {
                //udc_write8(ep_csr | MSB250X_UDC_RXCSR1_SENDSTALL,
                //                        MSB250X_UDC_RXCSR1_REG);
                if(idx==2)
					udc_write8(ep_csr | MSB250X_UDC_RXCSR1_SENDSTALL,MSB250X_USBCREG(0x126));                
            }
            else
            {
                //ep_csr &= ~MSB250X_UDC_RXCSR1_SENDSTALL;
                //udc_write8(ep_csr, MSB250X_UDC_RXCSR1_REG);
                //ep_csr |= MSB250X_UDC_RXCSR1_CLRDATATOG;
                //udc_write8(ep_csr, MSB250X_UDC_RXCSR1_REG);
                if(idx==2)
                {
                	ep_csr &= ~MSB250X_UDC_RXCSR1_SENDSTALL;
                	udc_write8(ep_csr, MSB250X_USBCREG(0x126));
                	ep_csr |= MSB250X_UDC_RXCSR1_CLRDATATOG;
                	udc_write8(ep_csr, MSB250X_USBCREG(0x126));
                }
				
                if ((ep_csr & MSB250X_UDC_RXCSR1_RXPKTRDY) && preq)
                {
                    msb250x_udc_read_fifo(ep, preq);
                }
            }
        }
    }

    spin_unlock_irqrestore(&dev_lock,flags);//local_irq_restore (flags);

    return 0;
}

/* endpoint-specific parts of the api to the usb controller hardware */
static const struct usb_ep_ops sg_msb250x_ep_ops =
{
    .enable = msb250x_udc_ep_enable,
    .disable = msb250x_udc_ep_disable,

    .alloc_request = msb250x_udc_alloc_request,
    .free_request = msb250x_udc_free_request,

    .queue = msb250x_udc_queue,
    .dequeue = msb250x_udc_dequeue,

    .set_halt = msb250x_udc_set_halt,
};

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_pullup_i
+------------------------------------------------------------------------------
| DESCRIPTION : internal software connection function
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| is_on                 |x  |       | enable the software connection or not
|--------------------+---+---+-------------------------------------------------
*/
static void msb250x_udc_pullup_i(int is_on)
{
    u32 pwr_reg = 0;
    pwr_reg = udc_read8(MSB250X_UDC_PWR_REG);
    if(is_on)
        udc_write8((pwr_reg |= MSB250X_UDC_PWR_SOFT_CONN), MSB250X_UDC_PWR_REG);
    else
        udc_write8((pwr_reg &= ~MSB250X_UDC_PWR_SOFT_CONN), MSB250X_UDC_PWR_REG);
}

/*------------------------- usb_gadget_ops ----------------------------------*/

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_get_frame
+------------------------------------------------------------------------------
| DESCRIPTION : get frame count
|
| RETURN      : the current frame number
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _gadget          |x  |       | usb_gadget struct point
|--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_get_frame(struct usb_gadget *_gadget)
{
    int tmp = 0;

    DBG("Entered %s\n", __FUNCTION__);

    tmp = udc_read16(MSB250X_UDC_FRAME_L_REG);
    return tmp;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_wakeup
+------------------------------------------------------------------------------
| DESCRIPTION : tries to wake up the host connected to this gadget
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _gadget          |x  |       | usb_gadget struct point
|--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_wakeup(struct usb_gadget *_gadget)
{
    DBG("Entered %s\n", __FUNCTION__);
    return 0;
}


/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_set_selfpowered
+------------------------------------------------------------------------------
| DESCRIPTION : sets the device selfpowered feature
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| gadget                 |x  |       | usb_gadget struct point
|--------------------+---+---+-------------------------------------------------
| value                 |x  |       | set this feature or not
|--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_set_selfpowered(struct usb_gadget *gadget, int value)
{
    struct msb250x_udc *udc = to_msb250x_udc(gadget);

    DBG("Entered %s\n", __FUNCTION__);

    if (value)
        udc->devstatus |= (1 << USB_DEVICE_SELF_POWERED);
    else
        udc->devstatus &= ~(1 << USB_DEVICE_SELF_POWERED);

    return 0;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_pullup
+------------------------------------------------------------------------------
| DESCRIPTION : software-controlled connect to USB host
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| gadget                 |x  |       | usb_gadget struct point
|--------------------+---+---+-------------------------------------------------
| is_on                 |x  |       | set software-controlled connect to USB host or not
|--------------------+---+---+-------------------------------------------------
*/
void do_soft_connect(void);
static int msb250x_udc_pullup(struct usb_gadget *gadget, int is_on)
{
    struct msb250x_udc *udc = to_msb250x_udc(gadget);

    //DBG("Entered %s\n", __FUNCTION__);
    //printk("#####====>msb250x_udc_pullup\n");
    //do_soft_connect();

    if (udc->driver && udc->driver->disconnect)
        udc->driver->disconnect(&udc->gadget);

    msb250x_udc_pullup_i(is_on);

    return 0;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_vbus_session
+------------------------------------------------------------------------------
| DESCRIPTION : establish the USB session
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| gadget                 |x  |       | usb_gadget struct point
|--------------------+---+---+-------------------------------------------------
| is_active          |x  |       | establish the session or not
|--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_vbus_session(struct usb_gadget *gadget, int is_active)
{
    DBG("Entered %s\n", __FUNCTION__);
    return 0;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_vbus_draw
+------------------------------------------------------------------------------
| DESCRIPTION : constrain controller's VBUS power usage
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| _gadget          |x  |       | usb_gadget struct point
|--------------------+---+---+-------------------------------------------------
| ma                 |x  |       | milliAmperes
|--------------------+---+---+-------------------------------------------------
*/
static int msb250x_vbus_draw(struct usb_gadget *_gadget, unsigned ma)
{
    DBG("Entered %s\n", __FUNCTION__);
    return 0;
}

static int msb250x_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	//usb_probe_driver();
	return 0;
}

static int msb250x_udc_stop(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	return 0;
}



/* the rest of the api to the controller hardware: device operations,
 * which don't involve endpoints (or i/o).
 */
static const struct usb_gadget_ops sg_msb250x_gadget_ops =
{
    .get_frame = msb250x_udc_get_frame,
    .wakeup = msb250x_udc_wakeup,
    .set_selfpowered = msb250x_udc_set_selfpowered,
    .pullup = msb250x_udc_pullup,
    .vbus_session = msb250x_udc_vbus_session,
    .vbus_draw    = msb250x_vbus_draw,
	.udc_start = msb250x_udc_start,
	.udc_stop = msb250x_udc_stop,    
};

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_disable
+------------------------------------------------------------------------------
| DESCRIPTION : disable udc
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                 |x  |       | msb250x_udc struct point
|--------------------+---+---+-------------------------------------------------
*/
void msb250x_udc_disable(struct msb250x_udc *dev)
{
#ifndef CB2
    u32 tmp=0;
#endif
    DBG("Entered %s\n", __FUNCTION__);
    /* Disable all interrupts */
    udc_write8(0x00, MSB250X_UDC_INTRUSBE_REG);
    udc_write16(0x00, MSB250X_UDC_INTRTXE_REG);
    udc_write16(0x00, MSB250X_UDC_INTRRXE_REG);

    /* Clear the interrupt registers */
    /* All active interrupts will be cleared when this register is read */
#ifdef CB2
	udc_write8(udc_read8(MSB250X_UDC_INTRUSB_REG), MSB250X_UDC_INTRUSB_REG);
	udc_write16(udc_read16(MSB250X_UDC_INTRTX_REG), MSB250X_UDC_INTRTX_REG);
	udc_write16(udc_read16(MSB250X_UDC_INTRRX_REG), MSB250X_UDC_INTRRX_REG);
#else
	tmp = udc_read8(MSB250X_UDC_INTRUSB_REG);
    tmp = udc_read16(MSB250X_UDC_INTRTX_REG);
    tmp = udc_read16(MSB250X_UDC_INTRRX_REG);
#endif
    /* Good bye, cruel world */
    msb250x_udc_pullup_i(0);

#if !defined(QC_BOARD)
    /* USB device reset, write 0 to reset OTG IP, active low*/
    //udc_write16(0, (OTG0_BASE_ADDR + 0x100));
    //udc_write16(1, (OTG0_BASE_ADDR + 0x100));
#endif

    /* Set speed to unknown */
    dev->gadget.speed = USB_SPEED_UNKNOWN;

#ifdef CONFIG_USB_MSB250X_DMA	
	//DmaRxMode1=0;
	free_dma_channels=0x7f;
#endif	
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_reinit
+------------------------------------------------------------------------------
| DESCRIPTION : reinit the ep list
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                 |x  |       | msb250x_udc struct point
|--------------------+---+---+-------------------------------------------------
*/
static void msb250x_udc_reinit(struct msb250x_udc *dev)
{
    u32 i = 0;

    DBG("Entered %s\n", __FUNCTION__);

    /* device/ep0 records init */
    INIT_LIST_HEAD (&dev->gadget.ep_list);
    INIT_LIST_HEAD (&dev->gadget.ep0->ep_list);
    dev->ep0state = EP0_IDLE;

    for (i = 0; i < MSB250X_ENDPOINTS; i++)
    {
        struct msb250x_ep *ep = &dev->ep[i];

        if (i != 0)
            list_add_tail (&ep->ep.ep_list, &dev->gadget.ep_list);

        ep->dev = dev;
        ep->desc = NULL;
        ep->halted = 0;
        INIT_LIST_HEAD (&ep->queue);
    }
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_enable
+------------------------------------------------------------------------------
| DESCRIPTION : enable udc
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| dev                 |x  |       | msb250x_udc struct point
|--------------------+---+---+-------------------------------------------------
*/
USB_INFO_st pg_USBInfo;
void InitUSBVar(USB_INFO_st *pUsbInfo)
{
    unsigned char u8EpNum = 0;

    if (NULL == pUsbInfo)
        return;

    pUsbInfo->otgTestMode = 0;
    pUsbInfo->otgSetFaddr = 0;
    pUsbInfo->otgClearedEP = 0;
    pUsbInfo->otgGetStatusResponse = 0;
    pUsbInfo->otgMyDeviceHNPSupport = 1;
    pUsbInfo->free_dma_channels = 0x7f;
    pUsbInfo->otgCSW_Addr = 0;
    pUsbInfo->otgSetupCmdBuf_Addr = 0;
    pUsbInfo->otgCBWCB_Addr = 0;
    pUsbInfo->otgRegPower = 0;
    pUsbInfo->otgIntStatus = 0;
    pUsbInfo->otgDMARXIntStatus = 0;
    pUsbInfo->otgDMATXIntStatus = 0;
    pUsbInfo->otgDataPhaseDir = 0;
    pUsbInfo->otgMassCmdRevFlag = 0;
    pUsbInfo->otgMassRxDataReceived = 0;
    pUsbInfo->otgReqOTGState = 0;
    pUsbInfo->otgCurOTGState = 0;
    pUsbInfo->otgSuspended = 0;
    pUsbInfo->otgRemoteWakeup = 0;
    pUsbInfo->otgHNPEnabled = 0;
    pUsbInfo->otgHNPSupport = 0;
    pUsbInfo->otgSelfPower = 1;
    pUsbInfo->otgConfig = 0;
    pUsbInfo->otgInterface = 0;
    pUsbInfo->otgUSBState = 0;
    pUsbInfo->otgcid = 0;
    pUsbInfo->otgFaddr = 0;
    pUsbInfo->otgRegDevCtl = 0;
    pUsbInfo->otgSpeed = 0;
    pUsbInfo->otgResetComplete = 0;
    pUsbInfo->otgSOF_1msCount = 0;
    pUsbInfo->otgIsNonOSmodeEnable = 0;
    pUsbInfo->otgUDPAddress = 0;
    pUsbInfo->otgUDPTxPacketCount = 0;
    pUsbInfo->otgUDPRxPacketCount = 0;
    pUsbInfo->bDownloadCode = 0;
    pUsbInfo->u8USBDeviceMode = 0;
    pUsbInfo->DeviceConnect = 0;
    pUsbInfo->u8USBDevMode = 0;
    pUsbInfo->gu16UplinkStart = 0;
    pUsbInfo->gu32UplinkSize = 0;
    pUsbInfo->otgSelectROMRAM = 0;
    pUsbInfo->PPB_One_CB= 0;
    pUsbInfo->PPB_Two_CB= 0;
    pUsbInfo->UploadResume = 1;
    pUsbInfo->gu16BBErrorCode = 0;
    pUsbInfo->nTransferLength=0;
    pUsbInfo->NonOS_UsbDeviceDataBuf_CB=0;
    pUsbInfo->bHIF_GetUplinkDataStatus=0;
    pUsbInfo->SizeofUSB_Msdfn_Dscr=0;
    pUsbInfo->otgEP0Setup.bmRequestType = 0;
    pUsbInfo->otgEP0Setup.bRequest = 0;
    pUsbInfo->otgEP0Setup.wValue = 0;
    pUsbInfo->otgEP0Setup.wIndex = 0;
    pUsbInfo->otgEP0Setup.wLength = 0;
    pUsbInfo->otgFSenseKey= 0;
    pUsbInfo->otgFASC= 0;
    pUsbInfo->otgFASCQ= 0;
    pUsbInfo->otgfun_residue= 0;
    pUsbInfo->otgactualXfer_len= 0;
    pUsbInfo->otgdataXfer_dir= 0;
    pUsbInfo->USB_CREATEPORT_COUNTER= 0;
    pUsbInfo->USB_PB_CONNECTED= 0;


    for (u8EpNum = 0; u8EpNum<3; u8EpNum++)
    {
        pUsbInfo->otgUSB_EP[u8EpNum].FIFOSize = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].MaxEPSize = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].BytesRequested = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].BytesProcessed = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].DRCInterval = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].intr_flag = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].pipe = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].BltEP = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].DRCDir = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].LastPacket = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].IOState = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].Halted = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].transfer_buffer = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].transfer_buffer_length = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].FifoRemain = 0;
        pUsbInfo->otgUSB_EP[u8EpNum].Infnum = 0;
    }
}

void do_soft_connect(void)
{
        USB_REG_WRITE8(M_REG_POWER, (USB_REG_READ8(M_REG_POWER) | M_POWER_SOFTCONN));	
}
#if 0//defined(CONFIG_ARCH_CHICAGO)
void otg_usb_bc_enable(u32 utmi_base, u32 bc_base, u8 enable)
{
	if (enable) {
		//printf("BC enable \n");
		writeb(readb((void *)(utmi_base+(0x1*2-1))) | 0x40, (void *)(utmi_base+(0x1*2-1)));  //IREF_PDN=1��b1. (utmi+0x01[6] )
		writeb(readb((void *)(bc_base+(0x3*2-1))) | 0x40, (void *)(bc_base+(0x3*2-1)));  // [6]= reg_host_bc_en
		writeb(readb((void *)(bc_base+(0xc*2))) | 0x40, (void *)(bc_base+(0xc*2)));  // [6]= reg_into_host_bc_sw_tri
		writew(0x0000, (void *)(bc_base));  // [15:0] = bc_ctl_ov_en
		writeb(readb((void *)(bc_base+(0xa*2))) | 0x80, (void *)(bc_base+(0xa*2)));  // [7]=reg_bc_switch_en
	}
	else {
		// disable BC
		//printf("BC disable \n");
		writeb(readb((void *)(bc_base+(0xc*2))) & (~0x40), (void *)(bc_base+(0xc*2)));  // [6]= reg_into_host_bc_sw_tri
		writeb(readb((void *)(bc_base+(0x3*2-1))) & (~0x40), (void *)(bc_base+(0x3*2-1)));  // [6]= reg_host_bc_en
		writeb(readb((void *)(utmi_base+(0x1*2-1))) & (~0x40), (void *)(utmi_base+(0x1*2-1)));  //IREF_PDN=1��b1. (utmi+0x01[6] )
	}
}
#endif
void msb250x_udc_enable(struct msb250x_udc *dev)
{
#if 1//def CONFIG_USB_GADGET_DUALSPEED
    //USBInit(E_Class_Serial, HIGH_SPEED);    
{
    printk("+USBInit\r\n");
	InitUSBVar(&pg_USBInfo);

    pg_USBInfo.u8DeviceClass = E_Class_Serial;
    pg_USBInfo.u8DeviceCap = HIGH_SPEED;
    pg_USBInfo.u8USBDevMode = E_USB_VirtCOM;

    // Disable UHC and OTG controllers
    //USBC_REG_WRITE8(0x4, USBC_REG_READ8(0x4)& (~0x3));
#if !defined(CONFIG_USB_MS_OTG)
    // Enable OTG controller
    USBC_REG_WRITE8(0x02*2, (USBC_REG_READ8(0x02*2)& ~(OTG_BIT0|OTG_BIT1)) | (OTG_BIT1));
#endif    

#if 0//defined(CONFIG_ARCH_CHICAGO)
	otg_usb_bc_enable(UTMI_BASE_ADDR, BC_BASE_ADDR, false);
#endif

#ifdef USB_ENABLE_UPLL
    //UTMI_REG_WRITE16(0, 0x4000);
    UTMI_REG_WRITE16(0, 0x6BC3); // Turn on UPLL, reg_pdn: bit<9> reg_pdn: bit<15>, bit <2> ref_pdn
    mdelay(1);
    UTMI_REG_WRITE8(1, 0x69);      // Turn on UPLL, reg_pdn: bit<9>
    mdelay(2);
    UTMI_REG_WRITE16(0, 0x0001); //Turn all (including hs_current) use override mode
                                 // Turn on UPLL, reg_pdn: bit<9>
    mdelay(3);
#endif 
	UTMI_REG_WRITE8(0x3C*2, UTMI_REG_READ8(0x3C*2) | 0x1); // set CA_START as 1
	mdelay(10);
	UTMI_REG_WRITE8(0x3C*2, UTMI_REG_READ8(0x3C*2) & ~0x01); // release CA_START
    while ((UTMI_REG_READ8(0x3C*2) & 0x02) == 0);        // polling bit <1> (CA_END)

	// Reset OTG controllers
	//USBC_REG_WRITE8(0, 0xC);
	USBC_REG_WRITE8(0, USBC_REG_READ8(0)|(OTG_BIT3|OTG_BIT2));

	// Unlock Register R/W functions  (RST_CTRL[6] = 1)
	// Enter suspend  (RST_CTRL[3] = 1)
	//USBC_REG_WRITE8(0, 0x48);
	USBC_REG_WRITE8(0, (USBC_REG_READ8(0)&~(OTG_BIT2))|OTG_BIT6);

	printk("+UTMI\n");
    UTMI_REG_WRITE8(0x06*2, (UTMI_REG_READ8(0x06*2) & 0x9F) | 0x40); //reg_tx_force_hs_current_enable
    UTMI_REG_WRITE8(0x03*2-1, UTMI_REG_READ8(0x03*2-1) | 0x28); //Disconnect window select
    UTMI_REG_WRITE8(0x03*2-1, UTMI_REG_READ8(0x03*2-1) & 0xef); //Disconnect window select
    UTMI_REG_WRITE8(0x07*2-1, UTMI_REG_READ8(0x07*2-1) & 0xfd); //Disable improved CDR
    UTMI_REG_WRITE8(0x09*2-1, UTMI_REG_READ8(0x09*2-1) |0x81);  // UTMI RX anti-dead-loc, ISI effect improvement
    UTMI_REG_WRITE8(0x15*2-1, UTMI_REG_READ8(0x15*2-1) |0x20);  // Chirp signal source select
    UTMI_REG_WRITE8(0x0b*2-1, UTMI_REG_READ8(0x0b*2-1) |0x80);  // set reg_ck_inv_reserved[6] to solve timing problem 
#if 1//defined(CONFIG_MSTAR_CEDRIC)
    UTMI_REG_WRITE8(0x2c*2,   0x10);
    UTMI_REG_WRITE8(0x2d*2-1, 0x02);
    UTMI_REG_WRITE8(0x2f*2-1, 0x81);
#else
    UTMI_REG_WRITE8(0x2c*2,   UTMI_REG_READ8(0x2c*2) |0x98);
    UTMI_REG_WRITE8(0x2d*2-1, UTMI_REG_READ8(0x2d*2-1) |0x02);
    UTMI_REG_WRITE8(0x2e*2,   UTMI_REG_READ8(0x2e*2) |0x10);    
    UTMI_REG_WRITE8(0x2f*2-1, UTMI_REG_READ8(0x2f*2-1) |0x01);
#endif	
	printk("-UTMI\n");
    // 2'b10: OTG enable
    //USBC_REG_WRITE8(0x4, (USBC_REG_READ8(0x4)&~0x1) | 0x02);
	//USBC_REG_WRITE8(0, USBC_REG_READ8(0)|OTG_BIT6);	

	//USBC_REG_WRITE8(0x00, 0x68);	//enable OTG and UHC XIU
	//USBC_REG_WRITE8(0x02*2, 0x02);	//Enable OTG MAC
	//USBC_REG_WRITE8(0x02*2, USBC_REG_READ8(0x02*2) | 0x02);	//Enable OTG MAC
#if !defined(CONFIG_USB_MS_OTG)
    // 2'b10: OTG enable
    USBC_REG_WRITE8(0x02*2, (USBC_REG_READ8(0x02*2)& ~(OTG_BIT0|OTG_BIT1)) | (OTG_BIT1));
#endif	
		
	USB_REG_WRITE8(0x100, USB_REG_READ8(0x100)&0xFE); // Reset OTG
	USB_REG_WRITE8(0x100, USB_REG_READ8(0x100)|0x01);
	USB_REG_WRITE16(0x100, USB_REG_READ16(0x100)|0x8000); /* Disable DM pull-down */
	
    // Set FAddr to 0
    USB_REG_WRITE8(M_REG_FADDR, 0);
	// Set Index to 0
	USB_REG_WRITE8(M_REG_INDEX, 0);
	USB_REG_WRITE8(M_REG_CFG6_H, USB_REG_READ8(M_REG_CFG6_H) | 0x08); 
	USB_REG_WRITE8(M_REG_CFG6_H, USB_REG_READ8(M_REG_CFG6_H) | 0x40);
	//while(0x18 != (USB_REG_READ8(M_REG_DEVCTL) & 0x18));
	
	//Pull up D+
    //if (u8DeviceCap==FULL_SPEED)
    //{
    //	printk("FULL SPEED\n");
    //    USB_REG_WRITE8(M_REG_POWER, M_POWER_SOFTCONN);
    //}
	//else if (u8DeviceCap==HIGH_SPEED)
	{
		printk("HIGH SPEED\n");
        USB_REG_WRITE8(M_REG_POWER, (USB_REG_READ8(M_REG_POWER) & ~M_POWER_ENSUSPEND) | M_POWER_SOFTCONN | M_POWER_HSENAB);	
	}

	USB_REG_WRITE8(M_REG_DEVCTL,0);
	
    // Flush the next packet to be transmitted/ read from the endpoint 0 FIFO
    USB_REG_WRITE16(M_REG_CSR0, USB_REG_READ16(M_REG_CSR0) | M_CSR0_FLUSHFIFO);

	// Flush the latest packet from the endpoint Tx FIFO
	USB_REG_WRITE8(M_REG_INDEX, 1);
	USB_REG_WRITE16(M_REG_TXCSR, USB_REG_READ16(M_REG_TXCSR) | M_TXCSR_FLUSHFIFO);

	// Flush the next packet to be read from the endpoint Rx FIFO
	USB_REG_WRITE8(M_REG_INDEX, 2);
	USB_REG_WRITE16(M_REG_RXCSR, USB_REG_READ16(M_REG_RXCSR) | M_RXCSR_FLUSHFIFO);

	USB_REG_WRITE8(M_REG_INDEX, 0);

	// Clear all control/status registers
    USB_REG_WRITE16(M_REG_CSR0, 0);
    USB_REG_WRITE8(M_REG_INDEX, 1);
    USB_REG_WRITE16(M_REG_TXCSR, 0);
    USB_REG_WRITE8(M_REG_INDEX, 2);
    USB_REG_WRITE16(M_REG_RXCSR, 0);

    USB_REG_WRITE8(M_REG_INDEX, 0);

 	// Enable all endpoint interrupts

    USB_REG_WRITE8(M_REG_INTRUSBE, 0xf7);

    USB_REG_WRITE16(M_REG_INTRTXE, 0xff);

    USB_REG_WRITE16(M_REG_INTRRXE, 0xff);

    USB_REG_READ8(M_REG_INTRUSB);

    USB_REG_READ16(M_REG_INTRTX);

    USB_REG_READ16(M_REG_INTRRX);

	//return &pg_USBInfo;	
}    
#else
    //USBInit(E_Class_Serial, FULL_SPEED);
{
    printk("+USBInit\r\n");
	InitUSBVar(&pg_USBInfo);

    pg_USBInfo.u8DeviceClass = E_Class_Serial;
    pg_USBInfo.u8DeviceCap = HIGH_SPEED;
    pg_USBInfo.u8USBDevMode = E_USB_VirtCOM;

    // Disable UHC and OTG controllers
    //USBC_REG_WRITE8(0x4, USBC_REG_READ8(0x4)& (~0x3));
#if !defined(CONFIG_USB_MS_OTG)
    // Enable OTG controller
    USBC_REG_WRITE8(0x02*2, (USBC_REG_READ8(0x02*2)& ~(OTG_BIT0|OTG_BIT1)) | (OTG_BIT1));
#endif        

#if 0//defined(CONFIG_ARCH_CHICAGO)
	otg_usb_bc_enable(UTMI_BASE_ADDR, BC_BASE_ADDR, false);
#endif
	
#ifdef USB_ENABLE_UPLL
    //UTMI_REG_WRITE16(0, 0x4000);
    UTMI_REG_WRITE16(0, 0x6BC3); // Turn on UPLL, reg_pdn: bit<9> reg_pdn: bit<15>, bit <2> ref_pdn
    mdelay(1);
    UTMI_REG_WRITE8(1, 0x69);      // Turn on UPLL, reg_pdn: bit<9>
    mdelay(2);
    UTMI_REG_WRITE16(0, 0x0001); //Turn all (including hs_current) use override mode
                                 // Turn on UPLL, reg_pdn: bit<9>
    mdelay(3);
#endif 
	UTMI_REG_WRITE8(0x3C*2, UTMI_REG_READ8(0x3C*2) | 0x1); // set CA_START as 1
	mdelay(10);
	UTMI_REG_WRITE8(0x3C*2, UTMI_REG_READ8(0x3C*2) & ~0x01); // release CA_START
    while ((UTMI_REG_READ8(0x3C*2) & 0x02) == 0);        // polling bit <1> (CA_END)

	// Reset OTG controllers
	//USBC_REG_WRITE8(0, 0xC);
	USBC_REG_WRITE8(0, USBC_REG_READ8(0)|(OTG_BIT3|OTG_BIT2));

	// Unlock Register R/W functions  (RST_CTRL[6] = 1)
	// Enter suspend  (RST_CTRL[3] = 1)
	//USBC_REG_WRITE8(0, 0x48);
	USBC_REG_WRITE8(0, (USBC_REG_READ8(0)&~(OTG_BIT2))|OTG_BIT6);

	printk("+UTMI\n");
    UTMI_REG_WRITE8(0x06*2, (UTMI_REG_READ8(0x06*2) & 0x9F) | 0x40); //reg_tx_force_hs_current_enable
    UTMI_REG_WRITE8(0x03*2-1, UTMI_REG_READ8(0x03*2-1) | 0x28); //Disconnect window select
    UTMI_REG_WRITE8(0x03*2-1, UTMI_REG_READ8(0x03*2-1) & 0xef); //Disconnect window select
    UTMI_REG_WRITE8(0x07*2-1, UTMI_REG_READ8(0x07*2-1) & 0xfd); //Disable improved CDR
    UTMI_REG_WRITE8(0x09*2-1, UTMI_REG_READ8(0x09*2-1) |0x81);  // UTMI RX anti-dead-loc, ISI effect improvement
    UTMI_REG_WRITE8(0x15*2-1, UTMI_REG_READ8(0x15*2-1) |0x20);  // Chirp signal source select
    UTMI_REG_WRITE8(0x0b*2-1, UTMI_REG_READ8(0x0b*2-1) |0x80);  // set reg_ck_inv_reserved[6] to solve timing problem 
#if 1//defined(CONFIG_MSTAR_CEDRIC)
    UTMI_REG_WRITE8(0x2c*2,   0x10);
    UTMI_REG_WRITE8(0x2d*2-1, 0x02);
    UTMI_REG_WRITE8(0x2f*2-1, 0x81);
#else
    UTMI_REG_WRITE8(0x2c*2,   UTMI_REG_READ8(0x2c*2) |0x98);
    UTMI_REG_WRITE8(0x2d*2-1, UTMI_REG_READ8(0x2d*2-1) |0x02);
    UTMI_REG_WRITE8(0x2e*2,   UTMI_REG_READ8(0x2e*2) |0x10);    
    UTMI_REG_WRITE8(0x2f*2-1, UTMI_REG_READ8(0x2f*2-1) |0x01);
#endif	
	printk("-UTMI\n");
    // 2'b10: OTG enable
    //USBC_REG_WRITE8(0x4, (USBC_REG_READ8(0x4)&~0x1) | 0x02);
	//USBC_REG_WRITE8(0, USBC_REG_READ8(0)|OTG_BIT6);	

	//USBC_REG_WRITE8(0x00, 0x68);	//enable OTG and UHC XIU
	//USBC_REG_WRITE8(0x02*2, 0x02);	//Enable OTG MAC
	//USBC_REG_WRITE8(0x02*2, USBC_REG_READ8(0x02*2) | 0x02);	//Enable OTG MAC
#if !defined(CONFIG_USB_MS_OTG)
    // 2'b10: OTG enable
    USBC_REG_WRITE8(0x02*2, (USBC_REG_READ8(0x02*2)& ~(OTG_BIT0|OTG_BIT1)) | (OTG_BIT1));
#endif		
		
	USB_REG_WRITE8(0x100, USB_REG_READ8(0x100)&0xFE); // Reset OTG
	USB_REG_WRITE8(0x100, USB_REG_READ8(0x100)|0x01);
	USB_REG_WRITE16(0x100, USB_REG_READ16(0x100)|0x8000); /* Disable DM pull-down */
	
    // Set FAddr to 0
    USB_REG_WRITE8(M_REG_FADDR, 0);
	// Set Index to 0
	USB_REG_WRITE8(M_REG_INDEX, 0);
	USB_REG_WRITE8(M_REG_CFG6_H, USB_REG_READ8(M_REG_CFG6_H) | 0x08); 
	USB_REG_WRITE8(M_REG_CFG6_H, USB_REG_READ8(M_REG_CFG6_H) | 0x40);
	//while(0x18 != (USB_REG_READ8(M_REG_DEVCTL) & 0x18));
	
	//Pull up D+
    //if (u8DeviceCap==FULL_SPEED)
    //{
    	printk("FULL SPEED\n");
        USB_REG_WRITE8(M_REG_POWER, M_POWER_SOFTCONN);
    //}
	//else if (u8DeviceCap==HIGH_SPEED)
	//{
	//	printk("HIGH SPEED\n");
    //    USB_REG_WRITE8(M_REG_POWER, (USB_REG_READ8(M_REG_POWER) & ~M_POWER_ENSUSPEND) | M_POWER_SOFTCONN | M_POWER_HSENAB);	
	//}

	USB_REG_WRITE8(M_REG_DEVCTL,0);
	
    // Flush the next packet to be transmitted/ read from the endpoint 0 FIFO
    USB_REG_WRITE16(M_REG_CSR0, USB_REG_READ16(M_REG_CSR0) | M_CSR0_FLUSHFIFO);

	// Flush the latest packet from the endpoint Tx FIFO
	USB_REG_WRITE8(M_REG_INDEX, 1);
	USB_REG_WRITE16(M_REG_TXCSR, USB_REG_READ16(M_REG_TXCSR) | M_TXCSR_FLUSHFIFO);

	// Flush the next packet to be read from the endpoint Rx FIFO
	USB_REG_WRITE8(M_REG_INDEX, 2);
	USB_REG_WRITE16(M_REG_RXCSR, USB_REG_READ16(M_REG_RXCSR) | M_RXCSR_FLUSHFIFO);

	USB_REG_WRITE8(M_REG_INDEX, 0);

	// Clear all control/status registers
    USB_REG_WRITE16(M_REG_CSR0, 0);
    USB_REG_WRITE8(M_REG_INDEX, 1);
    USB_REG_WRITE16(M_REG_TXCSR, 0);
    USB_REG_WRITE8(M_REG_INDEX, 2);
    USB_REG_WRITE16(M_REG_RXCSR, 0);

    USB_REG_WRITE8(M_REG_INDEX, 0);

 	// Enable all endpoint interrupts

    USB_REG_WRITE8(M_REG_INTRUSBE, 0xf7);

    USB_REG_WRITE16(M_REG_INTRTXE, 0xff);

    USB_REG_WRITE16(M_REG_INTRRXE, 0xff);

    USB_REG_READ8(M_REG_INTRUSB);

    USB_REG_READ16(M_REG_INTRTX);

    USB_REG_READ16(M_REG_INTRRX);

	//return &pg_USBInfo;	
}    
#endif	
    if (udc_read8(MSB250X_UDC_PWR_REG)&MSB250X_UDC_PWR_HS_MODE)
    {
        dev->gadget.speed = USB_SPEED_HIGH;
    }
    else
    {
        dev->gadget.speed = USB_SPEED_FULL;
    }
}

void usb_probe_driver(struct usb_gadget_driver *driver)
{
	struct msb250x_udc *udc = sg_udc_controller;

	printk("msb250x_udc--3.10v\n");
    udc->driver = driver;
    udc->gadget.dev.driver = &driver->driver;
	
	old_linestate = 0;
	old_soft_conn = 0;
	udc->conn_chg = 0;
	//init_waitqueue_head(&(udc->event_q));
#ifdef CONFIG_USB_MSB250X_DMA
	udc->DmaRxMode = DMA_RX_MODE1;
	using_dma = 1;
#endif
#if !defined( CONFIG_USB_CHARGER_DETECT )
	msb250x_udc_enable(udc);
#endif
	 mdelay(1);
	
#if !defined( CONFIG_USB_CHARGER_DETECT )
	old_linestate = (udc_read8(UTMI_SIGNAL_STATUS)>>6) & 0x3;
	old_soft_conn = udc_read8(MSB250X_UDC_PWR_REG)&(MSB250X_UDC_PWR_SOFT_CONN);
#endif
	printk("end probe_driver\n");

}
EXPORT_SYMBOL(usb_probe_driver);

void usb_unprobe_driver(void)
{
	struct msb250x_udc *udc = sg_udc_controller;

#ifdef CB2
	/* Disable udc */
	msb250x_udc_disable(udc);
#endif	
}
EXPORT_SYMBOL(usb_unprobe_driver);

#if 0
void xxx_msb250x_udc_enable(struct msb250x_udc *dev)
{
    u32 temp;

    DBG("Entered %s\n", __FUNCTION__);

    /* Set addr */
    udc_write8(0, MSB250X_UDC_FADDR_REG);

    /* Set device */
    udc_write8(0, MSB250X_UDC_DEVCTL_REG);

    /* Set default power state */
    udc_write8(DEFAULT_POWER_STATE, MSB250X_UDC_PWR_REG);

    /* Enable reset and suspend interrupt interrupts */
    temp = udc_read8(MSB250X_UDC_INTRUSBE_REG);
    udc_write8(temp | MSB250X_UDC_INTRUSBE_RESET | MSB250X_UDC_INTRUSB_RESUME | MSB250X_UDC_INTRUSBE_SUSPEND,
                            MSB250X_UDC_INTRUSBE_REG);

    /* Enable ep0 interrupt */
    udc_write16(MSB250X_UDC_INTRTXE_EP0, MSB250X_UDC_INTRTXE_REG);

    /* time to say "hello, world" */
    // msb250x_udc_pullup_i(1); /* Winder, let user space program to decide connect or not. */
   
#ifdef CB2
#ifdef CONFIG_USB_GADGET_DUALSPEED
    USBInit(E_Class_Serial, HIGH_SPEED);
#else
    USBInit(E_Class_Serial, FULL_SPEED);
#endif
#else
    USBInit(E_Class_Serial, HIGH_SPEED);
#endif
    if (udc_read8(MSB250X_UDC_PWR_REG)&MSB250X_UDC_PWR_HS_MODE)
    {
        dev->gadget.speed = USB_SPEED_HIGH;
    }
    else
    {
        dev->gadget.speed = USB_SPEED_FULL;
    }
}
#endif
/*------------------------- gadget driver handling---------------------------*/
#if 0
int usb_gadget_probe_driver(struct usb_gadget_driver *driver,
		int (*bind)(struct usb_gadget *))
{
	struct msb250x_udc *udc = sg_udc_controller;
	int        retval = 0;

	DBG("Entered %s driver.name=%s \n", __FUNCTION__, driver->driver.name);

	/* Sanity checks */
	if (!udc)
		return -ENODEV;
	if (udc->driver)
		return -EBUSY;

    if (!driver
			//|| driver->speed < USB_SPEED_FULL
			|| !bind
			|| !driver->setup) {
		DBG("bad parameter.\n");
		return -EINVAL;
	}

#if defined(MODULE)
    if (!driver->unbind)
    {
        DBG("Invalid driver: no unbind method\n");
        return -EINVAL;
    }
#endif
    /* Hook the driver */
    udc->driver = driver;
    udc->gadget.dev.driver = &driver->driver;

   /* Bind the driver */
    if ((retval = device_add(&udc->gadget.dev)) != 0)
    {
        DBG("Error in device_add() : %d\n",retval);
        goto register_error;
    }

    DBG("binding gadget driver '%s'\n", driver->driver.name);
    if ((retval = bind (&udc->gadget)) != 0)
    {
        device_del(&udc->gadget.dev);
        goto register_error;
    }
    /* init event queue. */
    old_linestate = 0;
    old_soft_conn = 0;
    udc->conn_chg = 0;
    //init_waitqueue_head(&(udc->event_q));
#ifdef CONFIG_USB_MSB250X_DMA
    udc->DmaRxMode = msb250_udc_set_dma_rx_by_name( driver->driver.name);
    using_dma = 1;
#endif
#if !defined( CONFIG_USB_CHARGER_DETECT )
    msb250x_udc_enable(udc);
#endif
	 mdelay(1);

#if !defined( CONFIG_USB_CHARGER_DETECT )
    old_linestate = (udc_read8(UTMI_SIGNAL_STATUS)>>6) & 0x3;
    old_soft_conn = udc_read8(MSB250X_UDC_PWR_REG)&(MSB250X_UDC_PWR_SOFT_CONN);
#endif
	printk("end probe_driver\n");
    return 0;

register_error:
	printk("ERROR:register error\n");
    udc->driver = NULL;
    udc->gadget.dev.driver = NULL;
    return retval;
}
EXPORT_SYMBOL_GPL(usb_gadget_probe_driver);
#endif
/*
+------------------------------------------------------------------------------
| FUNCTION    : usb_gadget_register_driver
+------------------------------------------------------------------------------
| DESCRIPTION : register a gadget driver
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| driver                 |x  |       | usb_gadget_driver struct point
|--------------------+---+---+-------------------------------------------------
*/
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
    struct msb250x_udc *udc = sg_udc_controller;
    int        retval = 0;

    DBG("Entered %s driver.name=%s \n", __FUNCTION__, driver->driver.name);

    /* Sanity checks */
    if (!udc)
        return -ENODEV;

    if (udc->driver)
        return -EBUSY;

    //if (!driver->bind || !driver->setup
    //{
    //    DBG("Invalid driver: bind %p setup %p speed %d\n",
    //                    driver->bind, driver->setup, driver->speed);
    //    return -EINVAL;
    //}

#if defined(MODULE)
    if (!driver->unbind)
    {
        DBG("Invalid driver: no unbind method\n");
        return -EINVAL;
    }
#endif

    /* Hook the driver */
    udc->driver = driver;
    udc->gadget.dev.driver = &driver->driver;

    /* Bind the driver */
    if ((retval = device_add(&udc->gadget.dev)) != 0)
    {
        DBG("Error in device_add() : %d\n",retval);
        goto register_error;
    }

    DBG("binding gadget driver '%s'\n", driver->driver.name);

    //if ((retval = driver->bind (&udc->gadget)) != 0)
    //{
    //    device_del(&udc->gadget.dev);
    //    goto register_error;
    //}

    /* init event queue. */
    old_linestate = 0;
    old_soft_conn = 0;
    udc->conn_chg = 0;
    init_waitqueue_head(&(udc->event_q));
#ifdef CB2
#ifdef CONFIG_USB_MSB250X_DMA
		udc->DmaRxMode = msb250_udc_set_dma_rx_by_name( driver->driver.name);
		using_dma = 1;
#endif
#endif
    /* Enable udc */
    msb250x_udc_enable(udc);

    mdelay(1);

    old_linestate = (udc_read8(UTMI_SIGNAL_STATUS)>>6) & 0x3;
    old_soft_conn = udc_read8(MSB250X_UDC_PWR_REG)&(MSB250X_UDC_PWR_SOFT_CONN);


    return 0;

register_error:
    udc->driver = NULL;
    udc->gadget.dev.driver = NULL;
    return retval;

	return 0;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : usb_gadget_unregister_driver
+------------------------------------------------------------------------------
| DESCRIPTION : unregister a gadget driver
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| driver                 |x  |       | usb_gadget_driver struct point
|--------------------+---+---+-------------------------------------------------
*/
#if 0
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
    struct msb250x_udc *udc = sg_udc_controller;

    if (!udc)
        return -ENODEV;

    if (!driver || driver != udc->driver || !driver->unbind)
        return -EINVAL;


    DBG("Entered %s driver.name=%s \n", __FUNCTION__, driver->driver.name);

    if (driver->disconnect)
        driver->disconnect(&udc->gadget);

#ifdef CB2
	/* Disable udc */
	msb250x_udc_disable(udc);
#endif

    if (driver->unbind)
        driver->unbind(&udc->gadget);

    device_del(&udc->gadget.dev);
    udc->driver = NULL;
#ifndef CB2	
    /* Disable udc */
    msb250x_udc_disable(udc);
#endif
    return 0;
}
EXPORT_SYMBOL_GPL(usb_gadget_unregister_driver);
#endif

/*---------------------platform_driver  msb250x_udc_driver -------------------------------*/

/* set the gadget and endpoint parameter */
static struct msb250x_udc sg_udc_config =
{
    .gadget =
    {
        .ops = &sg_msb250x_gadget_ops,
        .ep0 = &sg_udc_config.ep[0].ep,
        .name = sg_gadget_name,
        .dev =
        {
            //.bus_id = "gadget",
            .init_name = "gadget",
            //.release	= nop_release,
        },
    },

    /* control endpoint */
    .ep[0] =
    {
        .num = 0,
        .ep =
        {
            .name = ep0name,
            .ops = &sg_msb250x_ep_ops,
            .maxpacket = EP0_FIFO_SIZE,
        },
        .dev = &sg_udc_config,
    },

    /* first group of endpoints */
    .ep[1] =
    {
        .num = 1,
        .ep =
        {
            .name = "ep1in-bulk",
            .ops = &sg_msb250x_ep_ops,
            .maxpacket = EP_FIFO_SIZE,
        },
        .dev= &sg_udc_config,
        .fifo_size    = EP_FIFO_SIZE,
        .bEndpointAddress = USB_DIR_IN | 1,
        .bmAttributes    = USB_ENDPOINT_XFER_BULK,
    },
    .ep[2] =
    {
        .num = 2,
        .ep =
        {
            .name = "ep2out-bulk",
            .ops = &sg_msb250x_ep_ops,
            .maxpacket = EP_FIFO_SIZE,
        },
        .dev = &sg_udc_config,
        .fifo_size    = EP_FIFO_SIZE,
        .bEndpointAddress = USB_DIR_OUT | 2,
        .bmAttributes    = USB_ENDPOINT_XFER_BULK,
    },
    .ep[3] =
    {
        .num = 3,
        .ep =
        {
            .name = "ep3in-int",
            .ops = &sg_msb250x_ep_ops,
            .maxpacket = EP_FIFO_SIZE,
        },
        .dev = &sg_udc_config,
        .fifo_size    = EP_FIFO_SIZE,
        .bEndpointAddress = USB_DIR_IN | 3,
        .bmAttributes    = USB_ENDPOINT_XFER_INT,
    },
    .got_irq = 0,
};

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_utmi_init
+------------------------------------------------------------------------------
| DESCRIPTION : initial the UTMI interface
|
| RETURN      : NULL
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
*/
#if 0
static void msb250x_utmi_init(void)
{
    u32 temp;

    temp=udc_read16(UTMI_BASE_ADDR + 0x0C) | 0x0040;
    temp &= 0xDBFD;
    udc_write16(temp, UTMI_BASE_ADDR + 0x0C);

    temp = udc_read8(UTMI_BASE_ADDR + 0x11) | 0x62;
    udc_write8(temp, UTMI_BASE_ADDR + 0x11);

    temp = udc_read8(UTMI_BASE_ADDR + 0x4D) | 0x08;
    temp &= 0xFB;
    udc_write8(temp, UTMI_BASE_ADDR + 0x4D);

    temp = udc_read8(UTMI_BASE_ADDR + 0x51) | 0x08;
    udc_write8(temp, UTMI_BASE_ADDR + 0x51);

    temp = udc_read8(UTMI_BASE_ADDR + 0x54) & 0xF0;
    udc_write8(temp, UTMI_BASE_ADDR + 0x54);

    temp=udc_read16(UTMI_BASE_ADDR + 0x58) | 0x0791;
    temp &= 0xFFCD;
    udc_write16(temp, UTMI_BASE_ADDR + 0x58);

    temp = udc_read8(UTMI_BASE_ADDR + 0x5D) | 0x0E;
    udc_write8(temp, UTMI_BASE_ADDR + 0x5D);

    while((udc_read8(UTMI_BASE_ADDR + 0x60) & 0x0001) == 0);

    temp = udc_read8(USBC_BASE_ADDR + 0x04) | 0x02;
    udc_write8(temp, USBC_BASE_ADDR + 0x04);

    temp = udc_read8(OTG0_BASE_ADDR + 0x100) & 0xFE; /* Reset OTG */
    udc_write8(temp, OTG0_BASE_ADDR + 0x100);

    temp = udc_read8(OTG0_BASE_ADDR + 0x100) | 0x01;
    udc_write8(temp, OTG0_BASE_ADDR + 0x100);

    temp = udc_read8(OTG0_BASE_ADDR + 0x118) | 0x01;
    udc_write8(temp, OTG0_BASE_ADDR + 0x118);
}
#endif

static void msb250x_udc_intr_bh(struct work_struct* work)
{
    struct msb250x_udc *udc = &sg_udc_config;
    wakeup_connection_change_event(udc);
}

/*
 * Open and close
 */

int msb250x_udc_open(struct inode *inode, struct file *filp)
{
    struct msb250x_udc *dev; /* device information */

    dev = container_of(inode->i_cdev, struct msb250x_udc, cdev);
    filp->private_data = dev; /* for other methods */

    if (!dev->driver)
    {
        printk(KERN_ERR "Driver not registered yet!\n");

        return -EFAULT;
    }
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    DBG("msb250x_udc opened.\n");

    up(&dev->sem);

    return 0;          /* success */
}

int msb250x_udc_release(struct inode *inode, struct file *filp)
{
    DBG("release msb250x udc.\n");

    return 0;
}


/*
 * The ioctl() implementation
 */

int msb250x_udc_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    int err = 0;
    struct msb250x_udc *dev = (struct msb250x_udc *)filp->private_data;

    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != MSB250X_UDC_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > MSB250X_UDC_IOC_MAXNR)
        return -ENOTTY;

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
            err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
            err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    switch(cmd)
    {
        case MSB250X_UDC_CONN_CHG:
        {
            int conn;
            u8 linestate;
            u8 soft_conn;

            if (wait_event_interruptible(dev->event_q, (dev->conn_chg != 0)))
                 return -ERESTARTSYS; /* signal: tell the fs layer to handle it */

            dev->conn_chg = 0;

            linestate = (udc_read8(UTMI_SIGNAL_STATUS)>>6) & 0x3;
            soft_conn = udc_read8(MSB250X_UDC_PWR_REG)&(MSB250X_UDC_PWR_SOFT_CONN);

            if (linestate) /* if line state is not zero, cable is not connected. */
            {
                conn = 0;
            }
            else
            {
                if (soft_conn) /* if cable is connected and soft_conn is enable, */
                {              /* then device is connected. */
                    conn = 1;
                }
                else /* if cable is connected, but soft_conn is disable, */
                {    /* then device is also considered disconnected. */
                    conn = 0;
                }
            }

            retval = __put_user(conn, (int __user *)arg);

            break;
        }

        case MSB250X_UDC_SET_CONN:
        {
            int set_conn;
            u8 tmp;

            if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

            retval = __get_user(set_conn, (int __user *)arg);
            if (retval == 0)
            {
                tmp = udc_read8(MSB250X_UDC_PWR_REG);

                if (set_conn)
                {
                    tmp |= MSB250X_UDC_PWR_SOFT_CONN;
                    udc_write8(tmp, MSB250X_UDC_PWR_REG);
                }
                else
                {
                    tmp &= (~(MSB250X_UDC_PWR_SOFT_CONN));
                    udc_write8(tmp, MSB250X_UDC_PWR_REG);
                }
            }

            up(&dev->sem);

            break;
        }

        case MSB250X_UDC_GET_LINESTAT:
        {
            int linestate;

            if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

            linestate = (int)(udc_read8(UTMI_SIGNAL_STATUS)>>6) & 0x3;

            retval = __put_user(linestate, (int __user *)arg);

            up(&dev->sem);
            break;
        }

        case MSB250X_UDC_GET_CONN:
        {
            int soft_conn;

            if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

            soft_conn = (int)(udc_read8(MSB250X_UDC_PWR_REG)>>6) & 0x1;

            retval = __put_user(soft_conn, (int __user *)arg);

            up(&dev->sem);
            break;
        }

        default:  /* redundant, as cmd was checked against MAXNR */

            return -ENOTTY;
    }

    return retval;

}

struct file_operations msb250x_udc_fops = {
    .owner =    THIS_MODULE,
    //.ioctl =    msb250x_udc_ioctl,
    .open =     msb250x_udc_open,
    .release =  msb250x_udc_release,
};

/*
 * Set up the char_dev structure for this device.
 */
static int msb250x_udc_setup_cdev(struct msb250x_udc *dev)
{
    int result = 0;
    dev_t devno;
    int i;

    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    if (msb250x_udc_major)
    {
        devno = MKDEV(msb250x_udc_major, msb250x_udc_minor);
        result = register_chrdev_region(devno, MSB250X_UDC_NR_DEVS, "msb250x_udc");
    } else {
        result = alloc_chrdev_region(&devno, msb250x_udc_minor, MSB250X_UDC_NR_DEVS,
                "msb250x_udc");
        msb250x_udc_major = MAJOR(devno);
    }

    if (result < 0)
    {
        printk(KERN_WARNING "scull: can't get major %d\n", msb250x_udc_major);
        return result;
    }

    init_MUTEX(&dev->sem);

    cdev_init(&dev->cdev, &msb250x_udc_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &msb250x_udc_fops;
    result = cdev_add (&dev->cdev, devno, 1);
    /* Fail gracefully if need be */
    if (result)
        printk(KERN_NOTICE "Error %d adding msb250x_udc0", result);

    msb250x_udc_class = class_create(THIS_MODULE, "msb250x_udc");
    if (IS_ERR(msb250x_udc_class))
    {
        cdev_del(&dev->cdev);
        /* cleanup_module is never called if registering failed */
        unregister_chrdev_region(devno, MSB250X_UDC_NR_DEVS);

        return PTR_ERR(msb250x_udc_class);
    }

    for (i=0;i<MSB250X_UDC_NR_DEVS;i++)
    {
        device_create(msb250x_udc_class, NULL, devno,
                      NULL, "msb250x_udc%d", i);
    }

    return result;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_probe
+------------------------------------------------------------------------------
| DESCRIPTION : The generic driver interface function which called for initial udc
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| pdev                 |x  |       | platform_device struct point
|--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_probe(struct platform_device *pdev)
{
    struct msb250x_udc *udc = &sg_udc_config;
    int retval = 0;
	int ret;
	
    DBG("Entered %s\n", __FUNCTION__);
    spin_lock_init (&udc->lock);

	INIT_WORK(&usb_bh, msb250x_udc_intr_bh);

    /* since event_q is used in irq, we need to init it before request_irq() */
    init_waitqueue_head(&(udc->event_q));
    //device_initialize(&udc->gadget.dev);
    udc->gadget.dev.parent = &pdev->dev;
    udc->gadget.dev.dma_mask = pdev->dev.dma_mask;
	udc->pdev = pdev;
	udc->enabled = 0;

    platform_set_drvdata(pdev, udc);

    //CLRREG32(REG_CLK_OTG20_EN, 0x3F);
    //CLRREG32(CHIPTOP_BASE_ADDR+(0x2E<<2), BIT_3);
    //USBInit(E_Class_Serial, HIGH_SPEED);
    msb250x_udc_disable(udc);
	
    msb250x_udc_reinit(udc);

    //msb250x_udc_irq_init();
    //udc->got_irq = 1;
    udc->active_suspend = 0;

#ifdef CONFIG_USB_MSB250X_DMA
    using_dma = 1;
    //udc_write16(udc_read16(REG_CLK_OTG20_EN)&(~0x4000), REG_CLK_OTG20_EN);
#endif /* CONFIG_USB_MSB250X_DMA */

    /* Setup char device */
    retval = msb250x_udc_setup_cdev(udc);

    /* irq setup after old hardware state is cleaned up */
    retval = request_irq(INT_MS_OTG/*INT_MSB250X_OTG*/, msb250x_udc_irq,
                                                IRQF_DISABLED, sg_gadget_name, udc);
    if (retval != 0)
    {
        printk("cannot get irq, err %d\n", retval);
        return -EBUSY;
    }
	udc->got_irq = 1;

#ifdef ANDROID_WAKELOCK
    printk(KERN_INFO "init usb_connect_lock\n");
    wake_lock_init(&usb_connect_wake_lock, WAKE_LOCK_SUSPEND, "usb_connect_lock");
#endif

	sg_udc_controller = udc;

	ret = usb_add_gadget_udc(&pdev->dev, &udc->gadget);
	if(ret)
	{
		printk("error probe\n");
		while(1);
	}

	//msb250x_udc_enable(udc);
	printk("end porbe\n");
    return retval;
}

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_remove
+------------------------------------------------------------------------------
| DESCRIPTION : The generic driver interface function which called for disable udc
|
| RETURN      : zero on success, else negative error code
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
| pdev                 |x  |       | platform_device struct point
|--------------------+---+---+-------------------------------------------------
*/
static int msb250x_udc_remove(struct platform_device *pdev)
{
    struct msb250x_udc *udc = platform_get_drvdata(pdev);
    dev_t devno = MKDEV(msb250x_udc_major, msb250x_udc_minor);
    int i;

    DBG("Entered %s\n", __FUNCTION__);

    if (udc->driver)
        return -EBUSY;

    if(udc->got_irq)
    {
    	//DBG("free irq: %d \n", udc->irq);
        free_irq(INT_MS_OTG/*INT_MSB250X_OTG*/, udc);
        udc->got_irq = 0;
    }

    platform_set_drvdata(pdev, NULL);

    DBG("%s: remove ok\n", __FUNCTION__);

    cdev_del(&udc->cdev);
    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno, MSB250X_UDC_NR_DEVS);
    class_destroy(msb250x_udc_class);
    for (i=0;i<MSB250X_UDC_NR_DEVS;i++)
    {
        device_destroy(msb250x_udc_class, devno);
    }

    return 0;
}

#ifdef CONFIG_PM
static int msb250x_udc_suspend(struct platform_device *pdev, pm_message_t message)
{
    struct msb250x_udc *udc = platform_get_drvdata(pdev);

    DBG("Entered %s\n", __FUNCTION__);

    // disable udc
    msb250x_udc_disable(udc);

    // disable power
    USB_REG_WRITE8(M_REG_POWER, 0);

    // mark suspend state
    udc->active_suspend = 1;

    return 0;
}

static int msb250x_udc_resume(struct platform_device *pdev)
{
    struct msb250x_udc *udc = platform_get_drvdata(pdev);
    DBG("Entered %s\n", __FUNCTION__);
    if (udc->active_suspend)
    {
        if (udc->driver)
        {
            // enable udc
            msb250x_udc_enable(udc);
            mdelay(1);
        }
        udc->active_suspend = 0;
    }
    return 0;
}
#else
#define msb250x_udc_suspend NULL
#define msb250x_udc_resume NULL
#endif

static struct resource ms_udc_device_resource[] =
{
    [0] = {
        .start = 0xA0003E00,
        .end   = 0xA0004C00 - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = INT_MS_OTG,
        .end   = INT_MS_OTG,
        .flags = IORESOURCE_IRQ,
    }
};


static const struct of_device_id udc_of_match[] = {
	{
		.name = "usb",
		.compatible = "udc-bigendian",
	},
	{
		.name = "usb",
		.compatible = "udc-be",
	},
	{},
};


static struct platform_driver msb250x_udc_driver =
{
    .probe    = msb250x_udc_probe,
    .remove = msb250x_udc_remove,
    .suspend = msb250x_udc_suspend,
    .resume = msb250x_udc_resume,
    .driver =
    {
        .name = "msb250x_udc",
        .owner = THIS_MODULE,
        .of_match_table = udc_of_match,
    },
};


struct platform_device ms_udc_device =

{
    .name             = "msb250x_udc",
    .id               = -1,
    .num_resources    = ARRAY_SIZE(ms_udc_device_resource),
    .resource         = ms_udc_device_resource,
};


/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_init
+------------------------------------------------------------------------------
| DESCRIPTION : The generic driver interface function for register this driver
|               to Linux Kernel.
|
| RETURN      : 0 when success, error code in other case.
|
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
//#ifndef CONFIG_USB_EHCI_HCD
extern void (*ms_udc_probe_usb_driver)(struct usb_gadget_driver *driver);
static int __init msb250x_udc_init(void)
{
    DBG("Entered %s: gadget_name=%s version=%s\n", __FUNCTION__, sg_gadget_name, DRIVER_VERSION);
	printk("MSTAR UDC INIT\n");
	ms_udc_probe_usb_driver=usb_probe_driver;
	platform_device_register(&ms_udc_device);
    return platform_driver_register(&msb250x_udc_driver);
}
//#endif
/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_udc_exit
+------------------------------------------------------------------------------
| DESCRIPTION : The generic driver interface function for unregister this driver
|               from Linux Kernel.
|
| RETURN      : none.
|
+------------------------------------------------------------------------------
| Variable Name      |IN |OUT|                   Usage
|--------------------+---+---+-------------------------------------------------
|                    |   |   |
+--------------------+---+---+-------------------------------------------------
*/
static void __exit msb250x_udc_exit(void)
{
    DBG("Entered %s \n", __FUNCTION__);
    platform_driver_unregister(&msb250x_udc_driver);
}
#if 0
static int __init ms_udc_device_init(void)
{
	printk("device_init@@@@@\n");
	DBG("Entered %s: gadget_name=%s version=%s\n", __FUNCTION__, sg_gadget_name, DRIVER_VERSION);
    return platform_device_register(&ms_udc_device);
}
#endif

//EXPORT_SYMBOL(usb_gadget_unregister_driver);
EXPORT_SYMBOL(usb_gadget_register_driver);
EXPORT_SYMBOL(msb250x_udc_do_request);
EXPORT_SYMBOL(msb250x_udc_schedule_done);
EXPORT_SYMBOL(msb250x_udc_done);
EXPORT_SYMBOL(msb250x_udc_fifo_count_ep1);
EXPORT_SYMBOL(msb250x_udc_fifo_count_ep2);
EXPORT_SYMBOL(msb250x_udc_fifo_count_ep3);


//#ifndef CONFIG_USB_EHCI_HCD
module_init(msb250x_udc_init);
module_exit(msb250x_udc_exit);
//#endif

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");


//arch_initcall(ms_udc_device_init);
//fs_initcall(msb250x_udc_init); //use fs_initcall due to this should be earlier than ADB module_init
//module_exit(msb250x_udc_exit);

//MODULE_ALIAS(DRIVER_NAME);
//MODULE_LICENSE("GPL");
//MODULE_DESCRIPTION(DRIVER_DESC);



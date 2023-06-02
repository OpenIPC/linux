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
#ifndef _MSB250X_UDC_H
#define _MSB250X_UDC_H

/******************************************************************************
 * Include Files
 ******************************************************************************/
#include <linux/kernel.h>
#include <linux/cdev.h>
#include "ms_gvar.h"
/******************************************************************************
 * Constants
 ******************************************************************************/
#define USB_HIGH_SPEED 1
#define QC_BOARD 1
#define EP0_FIFO_SIZE 64
#ifdef CB2
#ifdef CONFIG_USB_GADGET_DUALSPEED
#define EP_FIFO_SIZE 512 
#else
#define EP_FIFO_SIZE 64
#endif

#else
#define EP_FIFO_SIZE 2048
#endif

#if defined(USB_HIGH_SPEED)
#define DEFAULT_POWER_STATE MSB250X_UDC_PWR_HS_EN
#else
#define DEFAULT_POWER_STATE 0x00
#endif

#ifndef MSB250X_UDC_MAJOR
#define MSB250X_UDC_MAJOR 0   /* dynamic major by default */
#endif

#ifndef MSB250X_UDC_NR_DEVS
#define MSB250X_UDC_NR_DEVS 1    /* msb250x_udc0 */
#endif

static const char ep0name [] = "ep0";
static const char *const ep_name[] = {
    ep0name,                                /* everyone has ep0 */
    /* msb250x four bidirectional bulk endpoints */
    "ep1in-bulk", "ep2out-bulk", "ep3in-int",
};
#define MSB250X_ENDPOINTS ARRAY_SIZE(ep_name)

/******************************************************************************
 * Variables
 ******************************************************************************/
enum ep0_state
{
    EP0_IDLE,
    EP0_IN_DATA_PHASE,
    EP0_OUT_DATA_PHASE,
    EP0_END_XFER,
    EP0_STALL,
};

struct msb250x_ep
{
    struct list_head queue;
    unsigned long last_io;	/* jiffies timestamp */
    struct usb_gadget *gadget;
    struct msb250x_udc *dev;
    const struct usb_endpoint_descriptor *desc;
    struct usb_ep ep;
    u8 num;

    unsigned short fifo_size;
    u8 bEndpointAddress;
    u8 bmAttributes;
    u8 DmaRxMode1;
    u8 RxShort;
    u8 ch;
	
    unsigned halted : 1;
    unsigned already_seen : 1;
    unsigned setup_stage : 1;


};

struct msb250x_request
{
    struct list_head		queue;		/* ep's requests */
    struct usb_request		req;
};

struct msb250x_udc
{
    spinlock_t lock;

    struct msb250x_ep ep[MSB250X_ENDPOINTS];
    struct usb_gadget gadget;
    struct usb_gadget_driver	*driver;
    struct msb250x_request fifo_req;
	struct platform_device		*pdev;
    u8 fifo_buf[EP_FIFO_SIZE];
    u16 devstatus;
    int address;
    int ep0state;
    unsigned got_irq : 1;
    unsigned req_std : 1;
    unsigned req_config : 1;
    unsigned req_pending : 1;
	unsigned enabled:1;
    u8 vbus;
	u8 DmaRxMode;
    struct semaphore sem;     /* Mutual exclusion semaphore     */
    struct cdev cdev;	      /* Char device structure */
    wait_queue_head_t event_q; /* Wait event queue. Now, only connection status change event. */
    int conn_chg; /* flag for connect status change event. */
    unsigned active_suspend : 1;


struct device			dev;
struct list_head		list;
unsigned int	irq;			/* irq number */
	
};

/******************************************************************************
 * Function definition
 ******************************************************************************/
static inline u32 udc_read8(u32 reg)
{
    //return INREG8(reg);
    return readb((void *)reg);
}

static inline void udc_write8(u32 value, u32 reg)
{
    //OUTREG8(reg, value);
    writeb(value, (void *)reg);
}

static inline u32 udc_read16(u32 reg)
{
    //return INREG16(reg);
    return readw((void *)reg);
}

static inline void udc_write16(u32 value, u32 reg)
{
    //OUTREG16(reg, value);
    writew(value, (void *)reg);
}

#ifdef CB2
s8 msb250x_udc_schedule_done(struct msb250x_ep *);
#endif

#endif /* _MSB250X_UDC_H */

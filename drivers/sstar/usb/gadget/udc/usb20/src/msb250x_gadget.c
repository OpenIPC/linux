/*
 * msb250x_gadget.c- Sigmastar
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

/*------------------------- usb_gadget_ops ----------------------------------*/
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include "msb250x_udc_reg.h"
#include "msb250x_udc.h"

#ifdef CONFIG_USB_FPGA_VERIFICATION
unsigned int epnum;
unsigned int ep_number[6] = {1, 2, 1, 2, 3, 6};
module_param_array(ep_number, uint, &epnum, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ep_number, "1-6");
#endif

extern void Chip_Inv_Cache_Range(unsigned long u32Addr, unsigned long u32Size);
extern void Chip_Flush_Cache_Range(unsigned long u32Addr, unsigned long u32Size);
extern void Chip_Flush_MIU_Pipe(void);

void msb250x_gadget_sync_request(struct usb_gadget *gadget, struct usb_request *req, int offset, int size)
{
    Chip_Inv_Cache_Range((unsigned long)req->buf + offset, size);
}
EXPORT_SYMBOL(msb250x_gadget_sync_request);

int msb250x_gadget_map_request(struct usb_gadget *gadget, struct usb_request *req, int is_in)
{
    int        ret   = usb_gadget_map_request(gadget, req, is_in);
    dma_addr_t __dma = req->dma;

    if (is_in)
    {
        Chip_Flush_Cache_Range((unsigned long)req->buf, req->length);
        Chip_Flush_MIU_Pipe();
    }

#ifndef __arch_pfn_to_dma
    __dma    = MSB250X_PA2BUS(__dma);
    req->dma = __dma;
#endif

    return ret;
}
EXPORT_SYMBOL(msb250x_gadget_map_request);

void msb250x_gadget_unmap_request(struct usb_gadget *gadget, struct usb_request *req, int is_in)
{
#ifndef __arch_pfn_to_dma
    dma_addr_t __dma = req->dma;

    __dma    = MSB250X_BUS2PA(__dma);
    req->dma = __dma;
#endif

    usb_gadget_unmap_request(gadget, req, is_in);
}
EXPORT_SYMBOL(msb250x_gadget_unmap_request);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_gadget_pullup_i
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
void msb250x_gadget_pullup_i(int is_on)
{
    u8                     power     = 0;
    struct otg0_usb_power *pst_power = (struct otg0_usb_power *)&power;

    power                = ms_readb(MSB250X_OTG0_PWR_REG);
    pst_power->bSoftConn = is_on;

    ms_writeb(power, MSB250X_OTG0_PWR_REG);

    printk(KERN_INFO "<USB>[GADGET] PULL_UP(%s)\n", (0 == is_on) ? "OFF" : "ON");
}

EXPORT_SYMBOL(msb250x_gadget_pullup_i);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_gadget_get_frame
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
int msb250x_gadget_get_frame(struct usb_gadget *g)
{
    return ms_readw(MSB250X_OTG0_FRAME_L_REG);
}
EXPORT_SYMBOL(msb250x_gadget_get_frame);

struct usb_ep *msb250x_gadget_match_ep(struct usb_gadget *g, struct usb_endpoint_descriptor *desc,
                                       struct usb_ss_ep_comp_descriptor *ep_comp)
{
    struct msb250x_udc *dev = to_msb250x_udc(g);
    struct usb_ep *     ep  = NULL;

#ifdef CONFIG_USB_FPGA_VERIFICATION
    unsigned int ep_num;

    switch (usb_endpoint_type(desc))
    {
        case USB_ENDPOINT_XFER_ISOC:
            if (usb_endpoint_dir_in(desc))
            {
                ep_num = ep_number[0];
                MSB250X_PARAM_EP(dev, ep_num, ep);
                printk("<USB>[GADGET] match isoc in ep\n");
                break;
            }
            else
            {
                ep_num = ep_number[1];
                MSB250X_PARAM_EP(dev, ep_num, ep);
                printk("<USB>[GADGET] match isoc out ep\n");
                break;
            }

        case USB_ENDPOINT_XFER_BULK:
            if (usb_endpoint_dir_in(desc))
            {
                ep_num = ep_number[2];
                MSB250X_PARAM_EP(dev, ep_num, ep);
                printk("<USB>[GADGET] match bulk in ep\n");
                break;
            }
            else
            {
                ep_num = ep_number[3];
                MSB250X_PARAM_EP(dev, ep_num, ep);
                printk("<USB>[GADGET] match bulk out ep\n");
                break;
            }

        case USB_ENDPOINT_XFER_INT:
            if (usb_endpoint_dir_in(desc))
            {
                ep_num = ep_number[4];
                MSB250X_PARAM_EP(dev, ep_num, ep);
                printk("<USB>[GADGET] match intr in ep\n");
                break;
            }
            else
            {
                ep_num = ep_number[5];
                MSB250X_PARAM_EP(dev, ep_num, ep);
                printk("<USB>[GADGET] match intr out ep\n");
                break;
            }
        default:
            /* nothing */;
    }
#else
#ifdef CONFIG_ANDROID
    /* This is for Android functionfs ep match. Because in functionfs,
     * EP is matched by full speed descriptor(bulk ep maxpacket size is 64bytes).
     * These eps matched can not be used for adb function.
     */
    switch (usb_endpoint_type(desc))
    {
        case USB_ENDPOINT_XFER_ISOC:
            if (1 < usb_endpoint_maxp_mult(desc))
            {
                MSB250X_HIGH_BANDWIDTH_EP(dev, ep);
            }
            break;
        case USB_ENDPOINT_XFER_INT:
            MSB250X_INTR_EP(dev, ep);
            break;

        case USB_ENDPOINT_XFER_BULK:
            if (usb_endpoint_dir_in(desc))
                MSB250X_HIGH_IN_EP(dev, ep);
            else
                MSB250X_HIGH_OUT_EP(dev, ep);
            break;
        default:
            /* nothing */
            break;
    }
#else
    int maxpacket = usb_endpoint_maxp(desc) & 0x07FF;
    int mult      = usb_endpoint_maxp_mult(desc);
    int index, select;
    if (!maxpacket)
    {
        switch (usb_endpoint_type(desc))
        {
            case USB_ENDPOINT_XFER_ISOC:
                maxpacket = 1024;
                break;
            case USB_ENDPOINT_XFER_BULK:
                maxpacket = 512;
                break;
            default:
                maxpacket = 64;
                break;
        }
    }
    for (index = 1, select = 1; index < MSB250X_MAX_ENDPOINTS; index++)
    {
        if ((maxpacket > dev->ep[index].ep.maxpacket_limit) || (maxpacket * mult > dev->ep[index].fifo_size)
            || dev->ep[index].ep.claimed)
            continue;
        if (dev->ep[select].ep.maxpacket_limit >= dev->ep[index].ep.maxpacket_limit)
        {
            select = index;
        }
    }
    if ((maxpacket <= dev->ep[select].ep.maxpacket_limit) && (maxpacket * mult <= dev->ep[select].fifo_size)
        && !dev->ep[select].ep.claimed)
    {
        ep = &dev->ep[select].ep;
        printk(KERN_DEBUG "<USB>[GADGET] Match EP%d(%d x %d)", select, mult, maxpacket);
    }
    else
    {
        printk("<USB_ERR>[GADGET] No Match EP(%d x %d)\n", mult, maxpacket);
    }
#endif
#endif

    return ep;
}
EXPORT_SYMBOL(msb250x_gadget_match_ep);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_gadget_wakeup
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
int msb250x_gadget_wakeup(struct usb_gadget *g)
{
    printk("Entered %s\n", __FUNCTION__);
    return 0;
}
EXPORT_SYMBOL(msb250x_gadget_wakeup);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_gadget_set_selfpowered
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
int msb250x_gadget_set_selfpowered(struct usb_gadget *g, int value)
{
    struct msb250x_udc *udc = to_msb250x_udc(g);

    if (value)
        udc->devstatus |= (1 << USB_DEVICE_SELF_POWERED);
    else
        udc->devstatus &= ~(1 << USB_DEVICE_SELF_POWERED);

    return 0;
}
EXPORT_SYMBOL(msb250x_gadget_set_selfpowered);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_gadget_pullup
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
int msb250x_gadget_pullup(struct usb_gadget *g, int is_on)
{
    struct msb250x_udc *dev = to_msb250x_udc(g);

    u8                     power     = ms_readb(MSB250X_OTG0_PWR_REG);
    struct otg0_usb_power *pst_power = (struct otg0_usb_power *)&power;

    dev->soft_conn = pst_power->bSoftConn;

    if (1 == is_on)
    {
        if (0 == dev->soft_conn)
        {
            msb250x_gadget_pullup_i(is_on);
        }
    }
    else
    {
        if (1 == dev->soft_conn)
        {
            msb250x_gadget_pullup_i(is_on);
        }
    }

    dev->soft_conn = is_on;

    return 0;
}
EXPORT_SYMBOL(msb250x_gadget_pullup);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_gadget_vbus_session
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

int msb250x_gadget_vbus_session(struct usb_gadget *g, int is_active)
{
    printk("Entered %s\n", __FUNCTION__);
    return 0;
}
EXPORT_SYMBOL(msb250x_gadget_vbus_session);

/*
+------------------------------------------------------------------------------
| FUNCTION    : msb250x_gadget_vbus_draw
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
int msb250x_gadget_vbus_draw(struct usb_gadget *g, unsigned ma)
{
    printk("Entered %s\n", __FUNCTION__);
    return 0;
}
EXPORT_SYMBOL(msb250x_gadget_vbus_draw);

int msb250x_gadget_udc_start(struct usb_gadget *g, struct usb_gadget_driver *driver)
{
    struct msb250x_udc *dev = NULL;

    if (!g)
    {
        printk("<USB_ERR>[GADGET] ENODEV!\n");
        return -ENODEV;
    }

    dev = to_msb250x_udc(g);

    dev->driver            = driver;
    dev->gadget.dev.driver = &driver->driver;
#ifdef CONFIG_USB_ULPI_BUS
    ms_writew((ms_readw(GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)) & 0x7fff),
              GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)); // bit15 = 0 pull low reset
    ms_writew((ms_readw(GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)) & 0xffef),
              GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)); // bit4 = 0 pull low stp
    ms_writew((ms_readw(GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)) & 0xefff),
              GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73));         // bit12 = 0 enable data bus for riu
    ms_writew(0x6184, GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x74)); // command softreset
    ms_writew((ms_readw(GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)) & 0xfffe),
              GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)); // trigger command
    ms_writew((ms_readw(GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)) | ~(0xefff)),
              GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)); // bit12 = 1 disable data bus for riu
    ms_writew((ms_readw(GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)) & 0xfeff),
              GET_REG16_ADDR(REG_ADDR_BASE_CHIPTOP, 0x73)); // bit8 = 0 enable data bus for mac
#endif

    msb250x_udc_init_utmi();
    msb250x_udc_init_usb_ctrl();
    msb250x_udc_reset_otg();
    msb250x_udc_init_otg(dev);

    mdelay(1);

    printk("<USB>[GADGET] UDC start\n");
    return 0;
}
EXPORT_SYMBOL(msb250x_gadget_udc_start);

int msb250x_gadget_udc_stop(struct usb_gadget *g)
{
    struct msb250x_udc *dev = NULL;
    int                 i;

    if (!g)
    {
        printk("<USB_ERR>[GADGET] ENODEV!\n");
        return -ENODEV;
    }

    dev = to_msb250x_udc(g);
    msb250x_udc_disable(dev);
    msb250x_udc_deinit_utmi();

    for (i = 0; i < MSB250X_MAX_ENDPOINTS; i++)
    {
        dev->ep[i].ep.desc = NULL;
    }
    dev->driver            = NULL;
    dev->gadget.dev.driver = NULL;

    // msb250x_udc_init_usb_ctrl();
    return 0;
}
EXPORT_SYMBOL(msb250x_gadget_udc_stop);

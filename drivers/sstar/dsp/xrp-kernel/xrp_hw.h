/*
 * xrp_hw.h- Sigmastar
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

#ifndef _XRP_HW_H_
#define _XRP_HW_H_

#include <linux/irqreturn.h>
#include <linux/platform_device.h>
#include <linux/types.h>

struct xvp;

/*!
 * Hardware-specific operation entry points.
 * Hardware-specific driver passes a pointer to this structure to xrp_init
 * at initialization time.
 */
struct xrp_hw_ops
{
    /*!
     * Enable power/clock, but keep the core stalled.
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     */
    int (*enable)(void *hw_arg);
    /*!
     * Diable power/clock.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     */
    void (*disable)(void *hw_arg);
    /*!
     * Reset the core.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     */
    void (*reset)(void *hw_arg);
    /*!
     * Unstall the core.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     */
    void (*release)(void *hw_arg);
    /*!
     * Stall the core.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     */
    void (*halt)(void *hw_arg);

    /*! Get HW-specific data to pass to the DSP on synchronization
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     * \param sz: return size of sync data here
     * \return a buffer allocated with kmalloc that the caller will free
     */
    void *(*get_hw_sync_data)(void *hw_arg, size_t *sz);

    /*!
     * Send IRQ to the core.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     */
    void (*send_irq)(void *hw_arg);

    /*!
     * Check whether region of physical memory may be handled by
     * dma_sync_* operations
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     */
    bool (*cacheable)(void *hw_arg, unsigned long pfn, unsigned long n_pages);
    /*!
     * Synchronize region of memory for DSP access.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     * \param flags: XRP_FLAG_{READ,WRITE,READWRITE}
     */
    void (*dma_sync_for_device)(void *hw_arg, void *vaddr, phys_addr_t paddr, unsigned long sz, unsigned flags);
    /*!
     * Synchronize region of memory for host access.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     * \param flags: XRP_FLAG_{READ,WRITE,READWRITE}
     */
    void (*dma_sync_for_cpu)(void *hw_arg, void *vaddr, phys_addr_t paddr, unsigned long sz, unsigned flags);

    /*!
     * memcpy data/code to device-specific memory.
     */
    void (*memcpy_tohw)(void __iomem *dst, const void *src, size_t sz);
    /*!
     * memset device-specific memory.
     */
    void (*memset_hw)(void __iomem *dst, int c, size_t sz);

    /*!
     * Check DSP status.
     *
     * \param hw_arg: opaque parameter passed to xrp_init at initialization
     *                time
     * \return whether the core has crashed and needs to be restarted
     */
    bool (*panic_check)(void *hw_arg);
};

enum xrp_init_flags
{
    /*! Use interrupts in DSP->host communication */
    XRP_INIT_USE_HOST_IRQ = 0x1,
};

/*!
 * Initialize generic XRP kernel driver from cdns,xrp-compatible device
 * tree node.
 *
 * \param pdev: pointer to platform device associated with the XRP device
 *              instance
 * \param flags: initialization flags
 * \param hw: pointer to xrp_hw_ops structeure for this device
 * \param hw_arg: opaque pointer passed back to hw-specific functions
 * \return error code or pointer to struct xvp, use IS_ERR_VALUE and ERR_PTR
 */
long xrp_init(struct platform_device *pdev, enum xrp_init_flags flags, const struct xrp_hw_ops *hw, void *hw_arg);

/*!
 * Initialize generic XRP kernel driver from cdns,xrp,v1-compatible device
 * tree node.
 *
 * \param pdev: pointer to platform device associated with the XRP device
 *              instance
 * \param flags: initialization flags
 * \param hw: pointer to xrp_hw_ops structeure for this device
 * \param hw_arg: opaque pointer passed back to hw-specific functions
 * \return error code or pointer to struct xvp, use IS_ERR_VALUE and ERR_PTR
 */
long xrp_init_v1(struct platform_device *pdev, enum xrp_init_flags flags, const struct xrp_hw_ops *hw, void *hw_arg);

/*!
 * Initialize generic XRP kernel driver from cdns,xrp,cma-compatible device
 * tree node.
 *
 * \param pdev: pointer to platform device associated with the XRP device
 *              instance
 * \param flags: initialization flags
 * \param hw: pointer to xrp_hw_ops structeure for this device
 * \param hw_arg: opaque pointer passed back to hw-specific functions
 * \return error code or pointer to struct xvp, use IS_ERR_VALUE and ERR_PTR
 */
long xrp_init_cma(struct platform_device *pdev, enum xrp_init_flags flags, const struct xrp_hw_ops *hw, void *hw_arg);

/*!
 * Deinitialize generic XRP kernel driver.
 *
 * \param pdev: pointer to platform device associated with the XRP device
 *              instance
 * \return 0 on success, negative error code otherwise
 */
int xrp_deinit(struct platform_device *pdev);

/*!
 * Deinitialize generic XRP kernel driver.
 *
 * \param pdev: pointer to platform device associated with the XRP device
 *              instance
 * \param hw_arg: optional pointer to opaque pointer where generic XRP driver
 *                returns hw_arg that was associated with the pdev at xrp_init
 *                time
 * \return 0 on success, negative error code otherwise
 */
int xrp_deinit_hw(struct platform_device *pdev, void **hw_arg);

/*!
 * Notify generic XRP driver of possible IRQ from the DSP.
 *
 * \param irq: IRQ number
 * \param xvp: pointer to struct xvp returned from xrp_init* call
 * \return whether IRQ was recognized and handled
 */
irqreturn_t xrp_irq_handler(int irq, struct xvp *xvp);

/*!
 * Resume generic XRP operation of the device dev.
 *
 * \param dev: device which operation shall be resumed
 * \return 0 on success, negative error code otherwise
 */
int xrp_runtime_resume(struct device *dev);

/*!
 * Suspend generic XRP operation of the device dev.
 *
 * \param dev: device which operation shall be suspended
 * \return 0 on success, negative error code otherwise
 */
int xrp_runtime_suspend(struct device *dev);

#endif /*_XRP_HW_H_*/

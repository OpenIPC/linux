#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#include "mhal_warp.h"

#ifndef _MDRV_WARP_H_
#define _MDRV_WARP_H_

// Device data
typedef struct
{
    struct platform_device  *pdev;          // platform device data
    struct cdev             cdev;           // character device
    struct clk              **clk;          // clock
    s32                     clk_num;        // clock number
    u32                     irq;            // IRQ number
    MHAL_WARP_DEV_HANDLE    dev_handle;     // MHAL device handle
    struct mutex            mutex;          // for critical section
} warp_dev_data;


// File private data
typedef struct
{
    warp_dev_data            *dev_data;      // Device data
    MHAL_WARP_INST_HANDLE   instance;       // MHAL instance handle
    wait_queue_head_t       wait_queue;     // Wait queue for polling operation
} warp_file_data;

#endif // _MDRV_WARP_H_
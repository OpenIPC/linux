#include "hal_ive.h"
#include "mdrv_ive_io_st.h"

#ifndef _MDRV_IVE_DATA_H_
#define _MDRV_IVE_DATA_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
// Driver Data Structure
//
//   Enum and structure fo IVE dirver
//   We declare enum & structure here because enum and structures are
//   shared in all three layers (mdrv, drv, and hal_simulation)
//
//   It is not good way for modulization, but most members of structure are not pointer
//   because it can simplify the memory managent
//   and container_of() is a key funtion to access data strucure in operators
//
///////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * There are 2 state enumeration
 *   IVE_DRV_STATE: indicate the state of HW
 *   IVE_FILE_STATE: indicate the state of File
 *
 *   File and HW has different state because it support multi instance
 *   and file request may be queued if HW is busy
 */
typedef enum
{
    IVE_DRV_STATE_READY         = 0,
    IVE_DRV_STATE_PROCESSING    = 1,
    IVE_DRV_STATE_DONE          = 2
} IVE_DRV_STATE;

typedef enum
{
    IVE_FILE_STATE_READY        = 0,
    IVE_FILE_STATE_PROCESSING   = 1,
    IVE_FILE_STATE_DONE         = 2,
    IVE_FILE_STATE_IN_QUEUE     = 3
} IVE_FILE_STATE;

typedef union
{
    ive_ioc_coeff_ncc       ncc_buffer;
    __u8                    map_buffer[256];
} ive_work_buffer;

// Data structure for driver
typedef struct
{
    struct platform_device  *pdev;          // platform device data
    ive_hal_handle          hal_handle;     // HAL handle for real HW configuration
    IVE_DRV_STATE           dev_state;      // HW state
    struct list_head        request_list;   // request list to queue waiting requst
    ive_work_buffer         *work_buffer;   // working buffer
    dma_addr_t              work_buffer_dma;// working buffer pysical address
} ive_drv_handle;

// Device data
typedef struct {
    struct platform_device  *pdev;          // platform device data
    struct cdev             cdev;           // character device
    struct clk              **clk;          // clock
    int                     clk_num;        // clock number
    unsigned int            irq;            // IRQ number
    ive_drv_handle          drv_handle;     // device handle
    struct work_struct      work_queue;     // work queue for post process after ISR
    struct mutex            mutex;          // for critical section
} ive_dev_data;

// File private data
typedef struct{
    ive_dev_data            *dev_data;      // Device data
    ive_ioc_config          ioc_config;     // IO configuation, i.e. one device file can service one request at the same time
    IVE_FILE_STATE          state;          // File state
    wait_queue_head_t       wait_queue;     // Wait queue for polling operation
    ive_ioc_config          *user_io_config;// IO configuation pointer from user space
    ive_work_buffer         work_buffer;
} ive_file_data;

#endif //_MDRV_IVE_DATA_H_
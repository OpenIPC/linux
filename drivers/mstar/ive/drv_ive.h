#ifndef _IVE_DRV_H_
#define _IVE_DRV_H_

#include <linux/kernel.h>
#include "mdrv_ive_data.h"
#include "hal_ive.h"
#include "mdrv_ive_io_st.h"

IVE_DRV_STATE  ive_drv_isr_handler(int irq, ive_drv_handle *handle);

int ive_drv_init(ive_drv_handle *handle, struct platform_device *pdev, phys_addr_t base_addr0, phys_addr_t base_addr1);
void ive_drv_release(ive_drv_handle *handle);
IVE_IOC_ERROR ive_drv_process(ive_drv_handle *handle, ive_file_data *file_data);
ive_file_data* ive_drv_post_process(ive_drv_handle *handle);
void ive_drv_copy_work_buffer(ive_drv_handle *handle, ive_work_buffer *work_buffer);
#endif

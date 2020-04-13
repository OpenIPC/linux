#ifndef _HAL_CMP_H_
#define _HAL_CMP_H_

#include <linux/kernel.h>

typedef struct
{
    phys_addr_t base_addr;
} cpm_hal_handle;

void cmp_hal_init(cpm_hal_handle *handle, phys_addr_t base_addr);
void cmp_hal_mcci_write(cpm_hal_handle *handle, u32 index, u32 value);
u16 cmp_hal_mcci_read(cpm_hal_handle *handle, u32 index);

#endif // _HAL_CMP_H_

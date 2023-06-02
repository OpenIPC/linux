#ifndef _KDRV_IFE_INT_INIT_H_
#define _KDRV_IFE_INT_INIT_H_

#include "kwrap/type.h"

#define NVT_KDRV_IFE_CHIP_COUNT		(1)
#define NVT_KDRV_IFE_MINOR_COUNT	(1)
#define NVT_KDRV_IFE_TOTAL_CH_COUNT	(NVT_KDRV_IFE_CHIP_COUNT * NVT_KDRV_IFE_MINOR_COUNT)
#define NVT_KDRV_IFE_NAME          "kdrv_ife"

typedef struct {
	void *p_mclk;		/* clk handle for linux */
	void *p_io_addr;	/* virtual register address */
	UINT32 phy_io_base;	/* physical register address */
	UINT32 phy_io_size;	/* physical register size */
	INT32 irq_id;		/* interrupt id */
} NVT_KDRV_IFE_RESOURCE;

INT32 kdrv_ife_resource_init(void);
INT32 kdrv_ife_resource_uninit(void);
INT32 kdrv_ife_resource_set(NVT_KDRV_IFE_RESOURCE *p_resource, UINT32 idx);
NVT_KDRV_IFE_RESOURCE* kdrv_ife_resource_get(UINT32 idx);

#endif
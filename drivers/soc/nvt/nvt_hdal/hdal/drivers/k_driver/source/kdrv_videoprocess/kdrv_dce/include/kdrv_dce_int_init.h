#ifndef _KDRV_DCE_INT_INIT_H_
#define _KDRV_DCE_INT_INIT_H_

#include "kwrap/type.h"

#define NVT_KDRV_DCE_CHIP_COUNT		(1)
#define NVT_KDRV_DCE_MINOR_COUNT	(1)
#define NVT_KDRV_DCE_TOTAL_CH_COUNT	(NVT_KDRV_DCE_CHIP_COUNT * NVT_KDRV_DCE_MINOR_COUNT)
#define NVT_KDRV_DCE_NAME          "kdrv_dce"

typedef struct {
	void *p_mclk;		/* clk handle for linux */
	void *p_io_addr;	/* virtual register address */
	UINT32 phy_io_base;	/* physical register address */
	UINT32 phy_io_size;	/* physical register size */
	INT32 irq_id;		/* interrupt id */
} NVT_KDRV_DCE_RESOURCE;

INT32 kdrv_dce_resource_init(void);
INT32 kdrv_dce_resource_uninit(void);
INT32 kdrv_dce_resource_set(NVT_KDRV_DCE_RESOURCE *p_resource, UINT32 idx);
NVT_KDRV_DCE_RESOURCE* kdrv_dce_resource_get(UINT32 idx);

#endif

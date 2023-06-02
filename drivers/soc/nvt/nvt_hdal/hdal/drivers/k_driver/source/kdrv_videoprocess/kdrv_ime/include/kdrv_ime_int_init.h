#ifndef _KDRV_IME_INT_INIT_H_
#define _KDRV_IME_INT_INIT_H_

#include "kwrap/type.h"

#define NVT_KDRV_IME_CHIP_COUNT		(1)
#define NVT_KDRV_IME_MINOR_COUNT	(1)
#define NVT_KDRV_IME_TOTAL_CH_COUNT	(NVT_KDRV_IME_CHIP_COUNT * NVT_KDRV_IME_MINOR_COUNT)
#define NVT_KDRV_IME_NAME          "kdrv_ime"

/* chip_id, eng_id to idx mapping */
#define KDRV_IME_CHIP_IDX(chip)				(chip - KDRV_CHIP0)
#define KDRV_IME_ENG_IDX(eng)				(eng - KDRV_VIDEOPROCS_IME_ENGINE0)
#define KDRV_IME_CONV2_HDL_IDX(chip, eng)	((KDRV_IME_CHIP_IDX(chip) * NVT_KDRV_IME_MINOR_COUNT) + KDRV_IME_ENG_IDX(eng))

typedef struct {
	void *p_mclk;		/* clk handle for linux */
	void *p_io_addr;	/* virtual register address */
	UINT32 phy_io_base;	/* physical register address */
	UINT32 phy_io_size;	/* physical register size */
	INT32 irq_id;		/* interrupt id */
} NVT_KDRV_IME_RESOURCE;

INT32 kdrv_ime_resource_init(void);
INT32 kdrv_ime_resource_uninit(void);
INT32 kdrv_ime_resource_set(NVT_KDRV_IME_RESOURCE *p_resource, UINT32 idx);
NVT_KDRV_IME_RESOURCE* kdrv_ime_resource_get(UINT32 idx);

#endif
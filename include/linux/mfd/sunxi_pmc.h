/*
 * Functions and registers to access sunxi_pmc power management chip.
 *
 * Copyright (C) 2013, Carlo Caione <carlo@caione.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_MFD_SUNXI_PMC_H
#define __LINUX_MFD_SUNXI_PMC_H

#include <linux/regmap.h>

/*
 * PMC_V1_ID SUPPORT:sun8iw21p1,
 *
 * */

enum {
	PMC_V1_ID = 0,
	NR_PMC_VARIANTS,
};

extern int pmc_debug_mask;



/* For pmc */
#define PAD_CTRL_REG			(0x08)
#define PMC_CTRL_EN_REG			(0x10)
#define PMC_DLY_CTRL_REG		(0x14)
#define SW_CFG_REG			(0x18)
#define	PWRON_INT_EN_REG		(0x1C)
#define PMC_STATUS_REG			(0x20)
#define PWR_EN_CFG_REG			(0x30)
#define RST_DLY_SEL_REG			(0x34)
#define PMC_BYP_ST_REG			(0x50)
#define PMC_ADDR_EXT			(0x100)

#define PMC_VBUS_STATUS			BIT(8)

/* Regulators IDs */
enum {
	PMC_DCDC1 = 0,
	PMC_DCDC2,
	PMC_DCDC3,
	PMC_LDOA,
	PMC_ALDO,
	PMC_RTCLDO,
	PMC_LDO33,
	PMC_LDO09,
	PMC_REG_ID_MAX,
};

/* IRQs */
enum pmc_irqs {
	PMC_IRQ_PWRON_INT_NEG,
	PMC_IRQ_PWRON_INT_POS,
	PMC_IRQ_PWRON_INT_SLVL,
	PMC_IRQ_PWRON_INT_LLVL,
	PMC_IRQ_PWRON_INT_SUPER_KEY,
	PMC_IRQ_IRQ_INT_POS,
	PMC_IRQ_IRQ_INT_NEG,
	PMC_IRQ_PWR_VBUS_IN_INT,
	PMC_IRQ_PWR_VBUS_OUT_INT,
};

struct sunxi_pmc_dev {
	struct device			*dev;
	int				irq;
	struct regmap			*regmap;
	struct regmap_irq_chip_data	*regmap_irqc;
	long				variant;
	int                             nr_cells;
	struct mfd_cell                 *cells;
	const struct regmap_config	*regmap_cfg;
	const struct regmap_irq_chip	*regmap_irq_chip;
	void (*dts_parse)(struct sunxi_pmc_dev *);
};


/**
 * sunxi_pmc_match_device(): Setup sunxi_pmc variant related fields
 *
 * @sunxi_pmc: sunxi_pmc device to setup (.dev field must be set)
 * @dev: device associated with this sunxi_pmc device
 *
 * This lets the sunxi_pmc core configure the mfd cells and register maps
 * for later use.
 */
int sunxi_pmc_match_device(struct sunxi_pmc_dev *sunxi_pmc);

/**
 * sunxi_pmc_device_probe(): Probe a configured sunxi_pmc device
 *
 * @sunxi_pmc: sunxi_pmc device to probe (must be configured)
 *
 * This function lets the sunxi_pmc core register the sunxi_pmc mfd devices
 * and irqchip. The sunxi_pmc device passed in must be fully configured
 * with sunxi_pmc_match_device, its irq set, and regmap created.
 */
int sunxi_pmc_device_probe(struct sunxi_pmc_dev *sunxi_pmc);

/**
 * sunxi_pmc_device_probe(): Remove a sunxi_pmc device
 *
 * @sunxi_pmc: sunxi_pmc device to remove
 *
 * This tells the sunxi_pmc core to remove the associated mfd devices
 */
int sunxi_pmc_device_remove(struct sunxi_pmc_dev *sunxi_pmc);

#endif /* __LINUX_MFD_SUNXI_PMC_H */

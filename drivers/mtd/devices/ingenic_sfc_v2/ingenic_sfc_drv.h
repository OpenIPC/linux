#ifndef __INGENIC_SFC_H
#define __INGENIC_SFC_H

#include "sfc_flash.h"

enum flash_type {
	NOR,
	NAND,
};

struct sfc_data {
	int (*flash_type_auto_detect)(struct platform_device *pdev);
};

#ifdef CONFIG_MTD_INGENIC_SFC_V2_NORFLASH
int ingenic_sfc_nor_probe(struct sfc_flash *flash);
int ingenic_sfc_nor_suspend(struct sfc_flash *flash);
int ingenic_sfc_nor_resume(struct sfc_flash *flash);
#endif

#ifdef CONFIG_MTD_INGENIC_SFC_V2_NANDFLASH
int ingenic_sfc_nand_probe(struct sfc_flash *flash);
int ingenic_sfc_nand_suspend(struct sfc_flash *flash);
int ingenic_sfc_nand_resume(struct sfc_flash *flash);
#endif

#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mtd/partitions.h>
#include "../spinand.h"
#include "../ingenic_sfc_common.h"
#include "nand_common.h"

#define AS_DEVICES_NUM         1
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		30
#define	TSHSL_W		30

#define TRD		25
#define TPP		500
#define TBE		3

static struct ingenic_sfcnand_device *as_nand;

static struct ingenic_sfcnand_base_param as_param[AS_DEVICES_NUM] = {
	[0]= {
		/* AS5F38G04SND-08LIN */
		.pagesize = 4 * 1024,
		.oobsize = 256,
		.blocksize = 4 * 1024 * 64,
		.flashsize = 4 * 1024 * 64 * 4096,

		.tSETUP = TSETUP,
		.tHOLD  = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD_MODE
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
};

static struct device_id_struct device_id[AS_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0x2D, "AS5F38G04SND", &as_param[0]),
};


static cdt_params_t *as_get_cdt_params(struct sfc_flash *flash, uint8_t device_id)
{
	CDT_PARAMS_INIT(as_nand->cdt_params);

	switch(device_id) {
		case 0x2D:
		    break;
		default:
		    dev_err(flash->dev, "device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
		    return NULL;
	}

	return &as_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint8_t device_id, uint8_t ecc_status)
{
	int ret = 0;
	switch(device_id) {
		case 0x2D:
			switch((ecc_status >> 4) & 0x3) {
				case 0x2:
					dev_err(flash->dev, "SFC ECC:Bit errors greater than %d bits detected and not corrected.\n", as_param[0].ecc_max);
					ret = -EBADMSG;
					break;
				default:
					ret = 0;
			}
			return 0;
		default:
			dev_err(flash->dev, "device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
			return -EIO;   //notice!!!
	}
	return ret;
}


static int __init as_nand_init(void) {

	as_nand = kzalloc(sizeof(*as_nand), GFP_KERNEL);
	if(!as_nand) {
		pr_err("alloc as_nand struct fail\n");
		return -ENOMEM;
	}

	as_nand->id_manufactory = 0x52;
	as_nand->id_device_list = device_id;
	as_nand->id_device_count = AS_DEVICES_NUM;

	as_nand->ops.get_cdt_params = as_get_cdt_params;
	as_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	as_nand->ops.get_feature = NULL;

	return ingenic_sfcnand_register(as_nand);
}

fs_initcall(as_nand_init);

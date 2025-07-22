#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_nand.h"
#include "nand_common.h"

#define GD_DEVICES_NUM          10
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		20

#define TRD		80
#define TRD_4G		120
#define TPP		700
#define TBE		5

static struct jz_nand_base_param gd_param[GD_DEVICES_NUM] = {

	[0] = {
		/*GD5F1GQ4UB*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_QUAD
		.need_quad = 1,
#else
		.need_quad = 0,
#endif
	},
	[1] = {
		 /*GD5F2GQ4UB*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},
	[2] = {
		/*GD5F4GQ4UB*/
		.pagesize = 4 * 1024,
		.blocksize = 4 * 1024 * 64,
		.oobsize = 256,
		.flashsize = 4 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD_4G,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},
	[3] = {
		/*GD5F1GQ4UC*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif

	},
	[4] = {
		/*GD5F2GQ4UC*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},
	[5] = {
		/*GD5F4GQ4UC*/
		.pagesize = 4 * 1024,
		.blocksize = 4 * 1024 * 64,
		.oobsize = 256,
		.flashsize = 4 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD_4G,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x3,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},
	[6] = {
		/*GD5F1GQ5UE*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD_4G,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},
	[7] = {
		/*GD5F2GQ5UE*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = 40,
		.tHOLD   = 40,
		.tSHSL_R = 20,
		.tSHSL_W = 50,

		.tRD = TRD_4G,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},
	[8] = {
		/*IS37SML01G1*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = 5,
		.tHOLD   = 5,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 50,
		.tPP = 500,
		.tBE = 1,

		.ecc_max = 0x2,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},
	[9] = {
		/*GD5F1GM7UE*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = 5,
		.tHOLD   = 5,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 50,
		.tPP = 500,
		.tBE = 1,

		.ecc_max = 0x2,
#ifdef CONFIG_SPI_QUAD
	.need_quad = 1,
#else
	.need_quad = 0,
#endif
	},


};


static struct device_id_struct device_id[GD_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0xD1, "GD5F1GQ4UB",&gd_param[0]),
	DEVICE_ID_STRUCT(0xD2, "GD5F2GQ4UB",&gd_param[1]),
	DEVICE_ID_STRUCT(0xD4, "GD5F4GQ4UB",&gd_param[2]),
	DEVICE_ID_STRUCT(0xB1, "GD5F1GQ4UC",&gd_param[3]),
	DEVICE_ID_STRUCT(0xB2, "GD5F2GQ4UC",&gd_param[4]),
	DEVICE_ID_STRUCT(0xB4, "GD5F4GQ4UC",&gd_param[5]),
	DEVICE_ID_STRUCT(0x51, "GD5F1GQ5UE",&gd_param[6]),
	DEVICE_ID_STRUCT(0x52, "GD5F2GQ5UE",&gd_param[7]),
	DEVICE_ID_STRUCT(0x21, "IS37SML01G1",&gd_param[8]),
	DEVICE_ID_STRUCT(0x91, "GD5F1GM7UE",&gd_param[9]),
};

static void gd_single_read(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	uint8_t device_id = nand_desc->id_device;
	uint8_t addr_len = 0;
	switch(device_id) {
	    case 0xB1 ... 0xB4:
		    addr_len = 3;
		    break;
	    case 0xD1 ... 0xD4:
		    addr_len = 2;
		    break;
		case 0x51 ... 0x52:
			addr_len = 2;
			break;
		case 0x21:
		case 0x91:
			addr_len = 2;
			break;
	    default:
		    pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
		    addr_len = 2;
		    break;
	}

	nand_single_read(transfer, cmd, op_info, addr_len);
	return;
}

static void gd_quad_read(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	uint8_t device_id = nand_desc->id_device;
	uint8_t addr_len = 0;
	switch(device_id) {
	    case 0xB1 ... 0xB4:
		    addr_len = 3;
		    break;
	    case 0xD1 ... 0xD4:
		    addr_len = 2;
		    break;
		case 0x51 ... 0x52:
		case 0x21:
		case 0x91:
			addr_len = 2;
			break;
	    default:
		    pr_err("device_id err, please check your device id: device_id = 0x%02x\n", device_id);
		    addr_len = 2;
		    break;
	}
	nand_quad_read(transfer, cmd, op_info, addr_len);
	return;
}

static int32_t gd_get_f0_register_value(struct sfc_flash *flash) {

	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	uint32_t buf = 0;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	cmd.cmd = SPINAND_CMD_GET_FEATURE;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = 0xf0;
	transfer.addr_len = 1;

	cmd.dataen = ENABLE;
	transfer.len = 1;
	transfer.data = (u_char *)&buf;
	transfer.direction = GLB_TRAN_DIR_READ;

	transfer.data_dummy_bits = 0;
	transfer.cmd_info = &cmd;
	transfer.ops_mode = CPU_OPS;

	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
	        dev_err(flash->dev,"sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}
	return buf;
}

static int32_t gd_get_read_feature(struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	uint8_t device_id = nand_desc->id_device;
	uint8_t ecc_status = 0;
	int32_t ret = 0;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	cmd.cmd = SPINAND_CMD_GET_FEATURE;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = SPINAND_ADDR_STATUS;
	transfer.addr_len = 1;

	cmd.dataen = DISABLE;
	transfer.len = 0;

	transfer.data_dummy_bits = 0;
	cmd.sta_exp = (0 << 0);
	cmd.sta_msk = SPINAND_IS_BUSY;
	transfer.cmd_info = &cmd;
	transfer.ops_mode = CPU_OPS;

	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
	        dev_err(flash->dev,"sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}
	ecc_status = sfc_get_sta_rt(flash->sfc);

	switch(device_id) {
		case 0xB1 ... 0xB4:
			switch((ecc_status >> 4) & 0x7) {
				case 0x7:
					dev_err(flash->dev, "SFC ECC:Bit errors greater than %d bits detected and not corrected.\n", gd_param[3].ecc_max);
					ret = -EBADMSG;
					break;
				default:
					ret = 0;
					break;
			}
			break;
		case 0xD1 ... 0xD4:
			switch((ecc_status >> 4) & 0x3) {
				case 0x2:
					dev_err(flash->dev, "SFC ECC:Bit errors greater than %d bits detected and not corrected.\n", gd_param[0].ecc_max);
					ret = -EBADMSG;
					break;
				default:
					ret = 0;
					break;
			}
			break;
		case 0x51 ... 0x52:
			switch((ecc_status >> 4) & 0x3) {
				case 0x2:
					dev_err(flash->dev, "SFC ECC:Bit errors greater than %d bits detected and not corrected.\n", gd_param[7].ecc_max);
					ret = -EBADMSG;
					break;
				default:
					ret = 0;
			}
			break;
		case 0x21:
			switch((ecc_status >> 4) & 0x3) {
				case 0x2:
					dev_err(flash->dev, "SFC ECC:Bit errors greater than %d bits detected and not corrected.\n", gd_param[8].ecc_max);
					ret = -EBADMSG;
				default:
					ret = 0;
			}
			break;
		case 0x91:
			switch((ecc_status >> 4) & 0x3) {
				case 0x2:
					dev_err(flash->dev, "SFC ECC:Bit errors greater than %d bits detected and not corrected.\n", gd_param[9].ecc_max);
					ret = -EBADMSG;
				default:
					ret = 0;
			}
			break;

		default:
			pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
			ret = -EIO;   //notice!!!
	}

	return ret;
}

int __init gd_nand_init(void) {
	struct jz_nand_device *gd_nand;
	gd_nand = kzalloc(sizeof(*gd_nand), GFP_KERNEL);
	if(!gd_nand) {
		pr_err("alloc gd_nand struct fail\n");
		return -ENOMEM;
	}

	gd_nand->id_manufactory = 0xC8;
	gd_nand->id_device_list = device_id;
	gd_nand->id_device_count = GD_DEVICES_NUM;

	gd_nand->ops.nand_read_ops.get_feature = gd_get_read_feature;
	gd_nand->ops.nand_read_ops.single_read = gd_single_read;
	gd_nand->ops.nand_read_ops.quad_read = gd_quad_read;

	return jz_nand_register(gd_nand);
}
module_init(gd_nand_init);

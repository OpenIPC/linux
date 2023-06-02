/*
 * fh_efuse.c
 *
 *  Created on: Mar 13, 2015
 *      Author: duobao
 */
#define pr_fmt(fmt) "%s %d: " fmt, __func__, __LINE__

#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include "fh_efuse.h"
#include <mach/fh_efuse_plat.h>
#include <linux/io.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <tee_client_api.h>

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/

#define FH_EFUSE_PLAT_DEVICE_NAME			"fh_efuse"
#define FH_EFUSE_MISC_DEVICE_NAME			"fh_efuse"
#define FH_EFUSE_MISC_DEVICE_NODE_NAME		"fh_efuse_node"

#define EFUSE_MAX_ENTRY			60


/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/

/*****************************************************************************

 *  static fun;
 *****************************************************************************/
static int fh_efuse_open(struct inode *inode, struct file *file);
static int fh_efuse_release(struct inode *inode, struct file *filp);
static long fh_efuse_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

static const struct file_operations fh_efuse_fops = {
		.owner = THIS_MODULE,
		.open = fh_efuse_open,
		.release = fh_efuse_release,
		.unlocked_ioctl = fh_efuse_ioctl,
};

static struct miscdevice fh_efuse_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = FH_EFUSE_MISC_DEVICE_NAME,
	/*.nodename = FH_EFUSE_MISC_DEVICE_NODE_NAME,*/
	.fops = &fh_efuse_fops,
};

static TEEC_UUID TA_FH_EFUSE_UUID = \
    { 0x796ad1ff, 0x5565, 0x4141,\
    { 0x9a, 0x1c, 0x29, 0x00, 0x20, 0xaa, 0x9f, 0x16} };

static TEEC_Context ctx;
static TEEC_Session sess;

static int fh_efuse_open(struct inode *inode, struct file *file)
{
	if (tee_open_session(&ctx, &sess, &TA_FH_EFUSE_UUID))
		return -ENOENT;

	return 0;
}

static int fh_efuse_release(struct inode *inode, struct file *filp)
{
	if (tee_close_session(&ctx, &sess))
		return -EFAULT;

	return 0;
}

#ifdef CONFIG_CRYPTO_DEV_FH_AES
#warning efuse funcs cannot be used in aes, please use FH_AES driver for tee!!!
struct wrap_efuse_obj s_efuse_obj = {0};

void efuse_trans_key(struct wrap_efuse_obj *obj, u32 start_no,
u32 size, struct af_alg_usr_def *p_alg)
{
	pr_err("not supported, must use FH_AES driver for tee!!!");
}
#endif

static void aes_biglittle_swap(u8 *buf)
{
	u8 tmp, tmp1;

	tmp = buf[0];
	tmp1 = buf[1];
	buf[0] = buf[3];
	buf[1] = buf[2];
	buf[2] = tmp1;
	buf[3] = tmp;
}

long fh_efuse_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	int ret;
	void *key_buff = NULL;
	EFUSE_INFO efuse_info = {0};
	EFUSE_INFO *efuse_user_info = (EFUSE_INFO *)arg;
	u32 temp_swap_data[32] = {0};
	int paramTypes = 0;
	TEEC_Parameter params[TEEC_CONFIG_PAYLOAD_REF_COUNT];
	int i;

	memset(params, 0, sizeof(params));

	if (!efuse_user_info) {
		return -EINVAL;
	}

	if (copy_from_user(&efuse_info, efuse_user_info, sizeof(EFUSE_INFO))) {
		return -EFAULT;
	}

	switch (cmd) {
	case IOCTL_EFUSE_WRITE_KEY:
	case IOCTL_EFUSE_READ_KEY:
	case IOCTL_EFUSE_WRITE_ENTRY:
	case IOCTL_EFUSE_READ_ENTRY:
		if (!efuse_info.key_buff || !efuse_info.key_size)
			return -EINVAL;
		if (copy_from_user(temp_swap_data, efuse_info.key_buff,
			efuse_info.key_size)) {
			return -EFAULT;
		}

		if (cmd == IOCTL_EFUSE_WRITE_KEY) {
			for (i = 0; i < efuse_info.key_size / 4; i++)
				aes_biglittle_swap((u8 *) (temp_swap_data + i));
		}
		key_buff = (void *)temp_swap_data;
		paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
							TEEC_MEMREF_TEMP_INOUT,
							TEEC_NONE, TEEC_NONE);
		params[1].tmpref.buffer = key_buff;
		params[1].tmpref.size = efuse_info.key_size;
		break;
	default:
		paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_NONE,
							TEEC_NONE, TEEC_NONE);
		break;
	}

	params[0].tmpref.buffer = &efuse_info;
	params[0].tmpref.size = sizeof(efuse_info);

	ret = tee_invoke_cmd(&sess, cmd, paramTypes, params);
	if (ret)
		return ret;

	if (copy_to_user(efuse_user_info, &efuse_info, sizeof(EFUSE_INFO)))
		return -EFAULT;

	if (cmd == IOCTL_EFUSE_READ_KEY) {
		for (i = 0; i < efuse_info.key_size / 4; i++)
			aes_biglittle_swap((u8 *) (temp_swap_data + i));
	}

	if (key_buff && copy_to_user(efuse_info.key_buff, temp_swap_data,
			efuse_info.key_size)) {
		return -EFAULT;
	}

	return 0;
}

/*****************************************************************************
 *
 *
 *		function body
 *
 *
 *****************************************************************************/


static int fh_efuse_probe(struct platform_device *pdev)
{
	int err = -ENODEV;
	u32 uuid_array[11];
	TEEC_UUID uuid;

#ifdef CONFIG_USE_OF
	struct device_node *np = pdev->dev.of_node;

	err = of_property_read_u32_array(np, "optee-uuid", uuid_array, 11);

	if (err) {
		dev_err(&pdev->dev, "error when parse optee-uuid %s", np->full_name);
		return err;
	} else {
#else
	struct fh_efuse_platform_data *plat_data =
		(struct fh_efuse_platform_data *)dev_get_platdata(&pdev->dev);

	if (!plat_data || !plat_data->optee_uuid) {
		dev_err(&pdev->dev, "cannot get optee-uuid from plat_data");
	} else {
		memcpy(uuid_array, plat_data->optee_uuid, sizeof(uuid_array));
#endif
		uuid = get_uuid_from_array(uuid_array);
		dev_info(&pdev->dev,
			"efuse-uuid: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			uuid.timeLow, uuid.timeMid, uuid.timeHiAndVersion,
			uuid.clockSeqAndNode[0], uuid.clockSeqAndNode[1],
			uuid.clockSeqAndNode[2], uuid.clockSeqAndNode[3],
			uuid.clockSeqAndNode[4], uuid.clockSeqAndNode[5],
			uuid.clockSeqAndNode[6], uuid.clockSeqAndNode[7]);
	}

	TA_FH_EFUSE_UUID = uuid;


	err = misc_register(&fh_efuse_misc);
	if (err != 0) {
		dev_err(&pdev->dev, "efuse register misc error\n");
		return err;
	}

	platform_set_drvdata(pdev, &fh_efuse_misc);

	return 0;
}

static int fh_efuse_remove(struct platform_device *pdev)
{
	struct miscdevice *misc;
	misc = (struct miscdevice *)platform_get_drvdata(pdev);
	misc_deregister(misc);
	return 0;
}


static const struct of_device_id fh_efuse_of_match[] = {
	{.compatible = "fh,fh-efuse",},
	{},
};

MODULE_DEVICE_TABLE(of, fh_efuse_of_match);

static struct platform_driver fh_efuse_driver = {
	.driver = {
		.name = "fh_efuse",
		.of_match_table = fh_efuse_of_match,
	},
	.probe = fh_efuse_probe,
	.remove = fh_efuse_remove,
};

module_platform_driver(fh_efuse_driver);
MODULE_DESCRIPTION("fh efuse driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FULLHAN");
MODULE_ALIAS("platform:FH_efuse");

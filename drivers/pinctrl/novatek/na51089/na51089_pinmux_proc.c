/*
 * Proc interface for the Novatek pinmux
 *
 * Copyright (c) 2018, NOVATEK MICROELECTRONIC CORPORATION.  All rights reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <plat/efuse_protected.h>
#include <plat/dma_protected.h>
#include "na51089_pinmux.h"
#include <linux/scatterlist.h>
#include <linux/crypto.h>

#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6

extern u32 top_reg_addr;
extern UINT32 efuse_getPkgVersion(void);

typedef struct proc_pinmux {
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
	struct proc_dir_entry *pproc_summary_entry;
	struct proc_dir_entry *pproc_gpio_entry;
	struct proc_dir_entry *pproc_efuse_entry;
	struct proc_dir_entry *pproc_chipid_entry;
	struct proc_dir_entry *pproc_packageid_entry;
	struct proc_dir_entry *pproc_version_entry;
} proc_pinmux_t;
proc_pinmux_t proc_pinmux;

static char *pinmux_table[] = {
	"sdio",
	"sdio2",
	"sdio3",
	"nand",
	"sensor",
	"sensor2",
	"mipi_lvds",
	"i2c",
	"sif",
	"uart",
	"spi",
	"sdp",
	"remote",
	"pwm",
	"pwm2",
	"ccnt",
	"audio",
	"lcd",
	"tv",
	"eth",
	"misc"
};

struct tcrypt_result {
	struct completion completion;
	int err;
};

/* tie all data structures together */

struct ablkcipher_def {
	struct scatterlist sg;
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	struct tcrypt_result result;
};

static u8 key_2nd[16] = {0x01, 0x02, 0x3, 0x4, 0x05, 0x06, 0x7, 0x8, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

/* Callback function */
static void nvt_ablkcipher_cb(struct crypto_async_request *req, int error)
{
	struct tcrypt_result *result = (struct tcrypt_result *)req->data;

	if (error == -EINPROGRESS) {
		return;
	}
	result->err = error;
	complete(&result->completion);
	pr_info("Encryption finished successfully\n");
}

/* Perform cipher operation */
static unsigned int nvt_ablkcipher_encdec(struct ablkcipher_def *ablk, int enc)
{
	int rc = 0;

	pr_info("nvt_ablkcipher_encdec %d\n", enc);
	if (enc) {
		rc = crypto_ablkcipher_encrypt(ablk->req);
	} else {
		rc = crypto_ablkcipher_decrypt(ablk->req);
	}

	switch (rc) {
	case 0:
		break;
	case -EINPROGRESS:
	case -EBUSY:
		rc = wait_for_completion_interruptible(
				 &ablk->result.completion);
		if (!rc && !ablk->result.err) {
			reinit_completion(&ablk->result.completion);
			break;
		}
	default:
		pr_info("ablkcipher encrypt returned with %d result %d\n",
				rc, ablk->result.err);
		break;
	}
	init_completion(&ablk->result.completion);

	return rc;
}

/*=============================================================================
 * proc "get" file operation functions
 *=============================================================================
 */
static int nvt_pinmux_proc_gpio_show(struct seq_file *sfile, void *v)
{
	struct nvt_pinctrl_info *info;

	info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	if (top_reg_addr) {
		info->top_base = (void *) top_reg_addr;
		pinmux_gpio_parsing(info);
	} else {
		pr_err("invalid pinmux address\n");
		kfree(info);
		return -ENOMEM;
	}

	kfree(info);

	return 0;
}


static int nvt_pinmux_proc_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_gpio_show, NULL);
}

static struct file_operations proc_gpio_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_pinmux_proc_gpio_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

/*=============================================================================
 * proc "efuse" file operation functions
 *=============================================================================
 */

static ssize_t nvt_pinmux_proc_efuse_otp_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
{

	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	UINT32      key_set = EFUSE_OTP_2ND_KEY_SET_FIELD;
	char *scratchpad = NULL;
	int result;
	UINT32  usage;
	struct ablkcipher_def ablk;
	struct ablkcipher_request *req = NULL;
	struct crypto_ablkcipher *tfm = NULL;

	/*check command length*/
	if ((!len) || (len > (MAX_CMD_LENGTH - 1))) {
		pr_err("Command length is too long or 0!\n");
		goto ERR_OUT;
	}

	/*copy command string from user space*/
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	printk("CMD:%s\n", cmd_line);

	/*parse command string*/
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	if (ucargc < 1) {
		pr_err("NULL command error\n");
		goto ERR_OUT;
	}

	if (!strcmp(argv[0], "trim")) {
		//UINT16  data;
#if 0
		if (efuse_read_param_ops(EFUSE_HDMI_TRIM_DATA, &data) != E_OK) {
			pr_err("Read HDMI trim data err\n");
		} else {
			pr_info("  HDMI trim data = 0x%08x\r\n", data);
		}

		if (efuse_read_param_ops(EFUSE_USBC_TRIM_DATA, &data) != E_OK) {
			pr_err("Read USB trim data err\n");
		} else {
			pr_info("   USB trim data = 0x%08x\r\n", data);
		}

		if (efuse_read_param_ops(EFUSE_TVDC_TRIM_DATA, &data) != E_OK) {
			pr_err("Read TVDAC trim data err\n");
		} else {
			pr_info(" TVDAC trim data = 0x%08x\r\n", data);
		}

		if (efuse_read_param_ops(EFUSE_SLVSEC_TRIM_DATA, &data) != E_OK) {
			pr_err("Read SLVS_EC trim data err\n");
		} else {
			pr_info("SLVSEC trim data = 0x%08x\r\n", data);
		}
#endif
		return size;
	} else if (!strcmp(argv[0], "keyset")) {

		//tfm = cryptodev_crypto_alloc_blkcipher("ecb(aes)-efuse", 0, 0);

		if (!strcmp(argv[1], "0")) {
			pr_info("key set 0\r\n");
			key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
		} else if (!strcmp(argv[1], "1")) {
			pr_info("key set 1\r\n");
			key_set = EFUSE_OTP_2ND_KEY_SET_FIELD;
		} else if (!strcmp(argv[1], "2")) {
			pr_info("key set 2\r\n");
			key_set = EFUSE_OTP_3RD_KEY_SET_FIELD;
		} else if (!strcmp(argv[1], "3")) {
			pr_info("key set 3\r\n");
			key_set = EFUSE_OTP_4TH_KEY_SET_FIELD;
		}

		if (IS_ERR(tfm)) {
			result = PTR_ERR(tfm);
			printk(KERN_ERR "E: can't load otp(keymanager): %d\n", (int)result);
			goto ERR_OUT;
		} else {
			printk("otp(keymanager)=> cra_name:%s cra_driver_name:%s\n",
				   crypto_tfm_alg_name(crypto_ablkcipher_tfm(tfm)),
				   crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm)));

			req = ablkcipher_request_alloc(tfm, GFP_KERNEL);

			if (IS_ERR(req)) {
				pr_info("could not allocate request queue\n");
				result = PTR_ERR((void *)req);
				goto ERR_OUT;
			} else {
				printk("ablkcipher_request_alloc success\n");
			}


			ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
											nvt_ablkcipher_cb,
											&ablk.result);

			printk("ablkcipher_request_set_callback success\n");

			crypto_ablkcipher_setkey(tfm, NULL, key_set);

			printk("ablkcipher_request_setkey success\n");

			/* Input data will be random */
			scratchpad = kmalloc(16, GFP_KERNEL);
			if (!scratchpad) {
				pr_info("could not allocate scratchpad\n");
				goto ERR_OUT;
			}

			memcpy((void *)scratchpad, (void *)key_2nd, (UINT32)16);

			ablk.tfm = tfm;
			ablk.req = req;

			printk("scratchpad 0x%08x 0x%08x 0x%08x 0x%08x\n", *(UINT32 *)(scratchpad), *(UINT32 *)(scratchpad + 4), *(UINT32 *)(scratchpad + 8), *(UINT32 *)(scratchpad + 12));
			printk("scratchpad 0x%08x \n", (UINT32)scratchpad);

			/* We encrypt one block */
			sg_init_one(&ablk.sg, scratchpad, 16);

			printk("scratchpad 0x%08x 0x%08x 0x%08x 0x%08x\n", *(UINT32 *)(scratchpad), *(UINT32 *)(scratchpad + 4), *(UINT32 *)(scratchpad + 8), *(UINT32 *)(scratchpad + 12));

			printk("page_link 0x%08x 0x%08x 0x%08x 0x%08x\n", *(UINT32 *)(sg_virt(&ablk.sg)), *(UINT32 *)(sg_virt(&ablk.sg) + 4), *(UINT32 *)(sg_virt(&ablk.sg) + 8), *(UINT32 *)(sg_virt(&ablk.sg) + 12));

			ablkcipher_request_set_crypt(req, &ablk.sg, &ablk.sg, 16, NULL);

			printk("ablkcipher_request_set_crypt\n");

			ablkcipher_request_set_tfm(req, tfm);

			printk("ablkcipher_request_set_tfm\n");

			init_completion(&ablk.result.completion);

			/* encrypt data */
			result = nvt_ablkcipher_encdec(&ablk, 1);

			printk("nvt_ablkcipher_encdec done result = %d\n", result);
			if (result) {
				goto ERR_OUT;
			} else {
				pr_info("Encryption triggered successfully\n");
				pr_info("Encrypt => 0x%08x 0x%08x 0x%08x 0x%08x\n", *(UINT32 *)(sg_virt(&ablk.sg)), *(UINT32 *)(sg_virt(&ablk.sg) + 4), *(UINT32 *)(sg_virt(&ablk.sg) + 8), *(UINT32 *)(sg_virt(&ablk.sg) + 12));
			}

			/* decrypt data */
			result = nvt_ablkcipher_encdec(&ablk, 0);

			printk("nvt_ablkcipher_encdec done result = %d\n", result);
			if (result) {
				goto ERR_OUT;
			} else {
				pr_info("Decryption triggered successfully\n");
				pr_info("Decrypt => 0x%08x 0x%08x 0x%08x 0x%08x\n", *(UINT32 *)(sg_virt(&ablk.sg)), *(UINT32 *)(sg_virt(&ablk.sg) + 4), *(UINT32 *)(sg_virt(&ablk.sg) + 8), *(UINT32 *)(sg_virt(&ablk.sg) + 12));
			}
ERR_OUT:
			if (tfm) {
				crypto_free_ablkcipher(tfm);
			}
			if (req) {
				ablkcipher_request_free(req);
			}
			if (scratchpad) {
				kfree(scratchpad);
			}
		}
		pr_err("crypto_alloc_cipher set key proc done\r\n");
		return size;
	} else if (!strcmp(argv[0], "dmausage")) {
		//usage = dma_get_utilization(DMA_ID_1);

		pr_info("dma usage = %02d\n", (int)usage);

	}

	return size;
}

static int nvt_pinmux_proc_efuse_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "\nUsage\n");
	seq_printf(sfile, "\necho [command] > efuse_op\n\n");
	seq_printf(sfile, "[command]    =>\n");
	seq_printf(sfile, "             => trim (driver's trim data)\n");
	seq_printf(sfile, "             => keyset X (X=0~3)\n");
	return 0;
}

static int nvt_pinmux_proc_efuse_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_efuse_help_show, NULL);
}

static struct file_operations proc_efuse_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_pinmux_proc_efuse_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
	.write  = nvt_pinmux_proc_efuse_otp_write,
};

/*=============================================================================
 * proc "Custom Command" file operation functions
 *=============================================================================
 */
static int nvt_pinmux_proc_cmd_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int nvt_pinmux_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_cmd_show, NULL);
}


static int nvt_pinmux_parse(unsigned char argc, char **pargv)
{
	unsigned long config = 0x0;
	char module[MAX_MODULE_NAME];
	PIN_GROUP_CONFIG pinmux_config[1] = {0};
	u8 count;

	if (argc != 2) {
		pr_err("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (strlen(pargv[0]) <= MAX_MODULE_NAME) {
		strcpy(module, pargv[0]);
	} else {
		pr_err("invalid length of module name:%d\n", strlen(pargv[0]));
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &config)) {
		pr_err("invalid config:%s\n", pargv[1]);
		return -EINVAL;
	}

	/*Find the module name and replace the configuration*/
	for (count = 0; count < PIN_FUNC_MAX; count++) {
		if (!strcmp(module, pinmux_table[count])) {
			pinmux_config->pin_function = count;
			pinmux_config->config = config;
			break;
		}
	}

	if (count == PIN_FUNC_MAX) {
		pr_err("module name invalid %s\n", module);
		return -EINVAL;
	}

	return nvt_pinmux_update(pinmux_config, 1);
}


static ssize_t nvt_pinmux_proc_cmd_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	/*check command length*/
	if ((!len) || (len > (MAX_CMD_LENGTH - 1))) {
		pr_err("Command length is too long or 0!\n");
		goto ERR_OUT;
	}

	/*copy command string from user space*/
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	printk("CMD:%s\n", cmd_line);

	/*parse command string*/
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	if (nvt_pinmux_parse(ucargc, &argv[0])) {
		goto ERR_OUT;
	} else {
		return size;
	}

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_pinmux_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_pinmux_proc_cmd_write
};

/*=============================================================================
 * proc "get" file operation functions
 *=============================================================================
 */
static int nvt_pinmux_proc_summary_show(struct seq_file *sfile, void *v)
{
	struct nvt_pinctrl_info *info;
	u8 count;

	info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	if (top_reg_addr) {
		info->top_base = (void *) top_reg_addr;
		pinmux_parsing(info);

		seq_printf(sfile, "\n&top {\n");
		for (count = 0; count < PIN_FUNC_MAX; count++) {
			seq_printf(sfile, "%s{pinmux=<0x%x>;};\n", pinmux_table[count], info->top_pinmux[count].config);
		}

		seq_printf(sfile, "};\n");
	} else {
		pr_err("invalid pinmux address\n");
		kfree(info);
		return -ENOMEM;
	}

	kfree(info);

	return 0;
}

static int nvt_pinmux_proc_summary_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_summary_show, NULL);
}

static struct file_operations proc_summary_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_pinmux_proc_summary_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


/*=============================================================================
 * proc "help" file operation functions
 *=============================================================================
 */
static int nvt_pinmux_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "\nUsage\n");
	seq_printf(sfile, "\necho module pinmux_config > pinmux_set\n\n");
	seq_printf(sfile, "module           : name of modules\n");
	seq_printf(sfile, "                   example: sdio, nand, spi\n");
	seq_printf(sfile, "pinmux_config    : pinmux configuration\n");
	seq_printf(sfile, "                   example: 0x25, please refer modelext table for value\n");
	seq_printf(sfile, "Example          : echo spi 0x370 > /proc/nvt_info/nvt_pinmux/pinmux_set\n");
	return 0;
}

static int nvt_pinmux_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_pinmux_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

/*=============================================================================
 * proc "package_id" file operation functions
 *=============================================================================
 */
static int nvt_pinmux_proc_packageid_show(struct seq_file *sfile, void *v)
{
	// seq_printf(sfile, "0x%x\n", efuse_getPkgVersion());
	return 0;
}

static int nvt_pinmux_proc_packageid_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_packageid_show, NULL);
}

static struct file_operations proc_package_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_pinmux_proc_packageid_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


/*=============================================================================
 * proc "chip_id" file operation functions
 *=============================================================================
 */
static int nvt_pinmux_proc_chipid_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "0x%x\n", nvt_get_chip_id());
	return 0;
}

static int nvt_pinmux_proc_chipid_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_chipid_show, NULL);
}

static struct file_operations proc_chipid_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_pinmux_proc_chipid_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

/*=============================================================================
 * proc "version" file operation functions
 *=============================================================================
 */
static int nvt_pinmux_proc_version_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "%s\n", DRV_VERSION);
	return 0;
}

static int nvt_pinmux_proc_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_pinmux_proc_version_show, NULL);
}

static struct file_operations proc_version_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_pinmux_proc_version_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_pinmux_proc_init(void)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("nvt_info/nvt_pinmux", NULL);
	if (pmodule_root == NULL) {
		pr_err("failed to create Module root\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_module_root = pmodule_root;

	pentry = proc_create("pinmux_set", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_cmd_entry = pentry;

	pentry = proc_create("pinmux_summary", S_IRUGO | S_IXUGO, pmodule_root, &proc_summary_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_summary_entry = pentry;

	pentry = proc_create("gpio_summary", S_IRUGO | S_IXUGO, pmodule_root, &proc_gpio_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc gpio!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_gpio_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &proc_help_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_help_entry = pentry;

	pentry = proc_create("efuse", S_IRUGO | S_IXUGO, pmodule_root, &proc_efuse_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc efuse!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_efuse_entry = pentry;

	pentry = proc_create("chip_id", S_IRUGO | S_IXUGO, pmodule_root, &proc_chipid_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc efuse!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_chipid_entry = pentry;

	pentry = proc_create("package_id", S_IRUGO | S_IXUGO, pmodule_root, &proc_package_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc efuse!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_packageid_entry = pentry;

	pentry = proc_create("version", S_IRUGO | S_IXUGO, pmodule_root, &proc_version_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc efuse!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_pinmux.pproc_version_entry = pentry;

remove_proc:
	return ret;
}

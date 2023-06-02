#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <kwrap/file.h>
#include <comm/libfdt.h>
#include "fastboot_main.h"
#include "fastboot_proc.h"
#include "fastboot_fdt.h"
#include <kwrap/debug.h>
#include <mach/nvt_ivot_emmc.h>

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH  64
#define MAX_ARG_NUM     20
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)
//============================================================================
// Declaration
//============================================================================
typedef struct _PROC_CMD {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(FASTBOOT_DRV_INFO *pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

typedef enum _STRG_TYPE {
	STRG_TYPE_UNKNOWN,
	STRG_TYPE_NAND,
	STRG_TYPE_NOR,
	STRG_TYPE_EMMC,
} STRG_TYPE;

typedef struct _STRG_DEV {
	STRG_TYPE type;
	unsigned int partition_size;
	struct mtd_info *mtd;
	struct nvt_ivot_emmc_partition	*card;
} STRG_DEV;

//============================================================================
// Global variable
//============================================================================
static FASTBOOT_DRV_INFO *pdrv_info_data;
//============================================================================
// Function define
//============================================================================
static struct mtd_info* find_fdt_app_mtd(void)
{
#if defined(CONFIG_MTD)
	int i;
	for (i = 0; i < 64; i++) {
		struct mtd_info *mtd = get_mtd_device(NULL, i);
		if (mtd==NULL || IS_ERR(mtd)) {
			continue;
		}
		if (strncmp(mtd->name, "fdt.app", 8) == 0) {
			return mtd;
		}
	}
#endif
	return NULL;
}

static struct nvt_ivot_emmc_partition* find_fdt_app_emmc(void)
{
#if defined(CONFIG_MMC_NVT_EMMC_INFO)
	extern struct nvt_ivot_emmc_total_partitions* nvt_ivot_emmc_parts;
	int i = 0;

	for (i = 0; i < nvt_ivot_emmc_parts->nr_parts; i++)
	{
		if(strncmp(nvt_ivot_emmc_parts->part_info[i].name, "fdt.app", 8) == 0) {
			return &nvt_ivot_emmc_parts->part_info[i];
		}
	}
#endif
	return NULL;
}

static int get_strg(STRG_DEV *p_strg)
{
	int i;
	struct device_node* of_node;
	unsigned int reg[4] = {0};
	const char *path[3] = {
		"/mmc@f0510000/partition_fdt.app",
		"/nor@f0400000/partition_fdt.app",
		"/nand@f0400000/partition_fdt.app",
	};
	const STRG_TYPE types[3] = {
		STRG_TYPE_EMMC,
		STRG_TYPE_NOR,
		STRG_TYPE_NAND,
	};

	memset(p_strg, 0, sizeof(STRG_DEV));
	for (i = 0; i < 3; i++) {
		of_node = of_find_node_by_path(path[i]);
		if (of_node) {
			p_strg->type = types[i];
			break;
		}
	}

	if (p_strg->type == STRG_TYPE_UNKNOWN) {
		DBG_ERR("unable to find partition_fdt.app in fdt.");
		return -1;
	}

	// read partition size from fdt
	if (of_property_read_u32_array(of_node, "reg", reg, 4) != 0) {
		DBG_ERR("unable to read reg of partition_fdt.app in fdt.");
		return -1;
	}

	p_strg->partition_size = reg[3];

	switch (p_strg->type)
	{
	case STRG_TYPE_NAND:
	case STRG_TYPE_NOR:
		p_strg->mtd = find_fdt_app_mtd();
		break;
	case STRG_TYPE_EMMC:
		p_strg->card = find_fdt_app_emmc();
		break;
	default:
		return -1;
	}
	return 0;
}

static int read_fdt(STRG_DEV *p_strg, unsigned char *mem, unsigned int mem_size)
{
	int err = -1;
	int fdt_size;
	struct fdt_header fdt_hdr = { 0 };
#if defined(CONFIG_MTD)
	size_t read;
#endif

	if (p_strg->mtd) {
#if defined(CONFIG_MTD)
		err = mtd_read(p_strg->mtd, 0, sizeof(fdt_hdr), &read, (void *)&fdt_hdr);
		/* Ignore corrected ECC errors */
		if (mtd_is_bitflip(err))
			err = 0;
		if (!err && read != sizeof(fdt_hdr))
			err = -EIO;
		if (err) {
			DBG_ERR("error: read failed at 0\n");
			return err;
		}
#endif
	} else if (p_strg->card) {
		int fd, len;
		char mmc_path[64] = {0};
		snprintf(mmc_path, sizeof(mmc_path)-1, "/dev/mmcblk2p%d", p_strg->card->partno);

		fd = vos_file_open(mmc_path, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			DBG_ERR("failed to open:%s\n", mmc_path);
			return -1;
		}

		len = vos_file_read(fd, (void *)&fdt_hdr, sizeof(fdt_hdr));
		if (len < sizeof(fdt_hdr)) {
			DBG_ERR("failed to read fdt header\n");
			vos_file_close(fd);
			return -1;
		}

		vos_file_close(fd);
	}

	fdt_size = fdt_totalsize(&fdt_hdr);
	if (fdt_size < 0) {
		//fdt is invalid or empty, just create new
		DBG_WRN("fdt.app is corrupted, create new one\n");
		if ((err = fdt_create_empty_tree(mem, mem_size)) < 0) {
			DBG_ERR("error: create new fdt.\n");
			return err;
		}
		return 0;
	}
	if (mem_size < fdt_size) {
		DBG_ERR("mem size too small, require = %d bytes\n", fdt_size);
		return -1;
	}

	if (p_strg->mtd) {
#if defined(CONFIG_MTD)
		err = mtd_read(p_strg->mtd, 0, fdt_size, &read, (void *)mem);
		/* Ignore corrected ECC errors */
		if (mtd_is_bitflip(err))
			err = 0;
		if (!err && read != fdt_size)
			err = -EIO;
		if (err) {
			DBG_ERR("error: read failed at 0\n");
			return err;
		}
#endif
	} else if (p_strg->card) {
		int fd, len;
		char mmc_path[64] = {0};
		snprintf(mmc_path, sizeof(mmc_path)-1, "/dev/mmcblk2p%d", p_strg->card->partno);

		fd = vos_file_open(mmc_path, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			DBG_ERR("failed to open:%s\n", mmc_path);
			return -1;
		}

		len = vos_file_read(fd, (void *)mem, fdt_size);
		if (len < fdt_size) {
			DBG_ERR("failed to read fdt.\n");
			vos_file_close(fd);
			return -1;
		}
		vos_file_close(fd);
		err = 0;
	}

	// enlarge the size to write more nodes
	fdt_set_totalsize(mem, mem_size);
	return err;
}

static int write_fdt(STRG_DEV *p_strg, unsigned char *fdt)
{
	if (fdt == NULL) {
		return -1;
	}
	if (p_strg->mtd) {
#if defined(CONFIG_MTD)
		int err;
		size_t written;
		struct mtd_info *mtd = p_strg->mtd;
		size_t size = ALIGN_CEIL(fdt_totalsize(fdt), mtd->erasesize);
		struct erase_info ei = {0};
		if (size > mtd->size) {
			DBG_ERR("fdt size larger than mtd->size\n");
			return -1;
		}
		ei.len  = mtd->size;
		err = mtd_erase(mtd, &ei);
		if (err) {
			DBG_ERR("error %d while erasing flash.\n", err);
			return err;
		}

		err = mtd_write(mtd, 0, size, &written, fdt);
		if (!err && written != size) {
			err = -EIO;
		}
		if (err) {
			DBG_ERR("error: write failed\n");
			return err;
		}
		return 0;
#endif
	} else if (p_strg->card) {
		int fd, len, fdt_size;
		char mmc_path[64] = {0};
		snprintf(mmc_path, sizeof(mmc_path)-1, "/dev/mmcblk2p%d", p_strg->card->partno);

		fd = vos_file_open(mmc_path, O_CREAT|O_WRONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			DBG_ERR("failed to open:%s\n", mmc_path);
			return -1;
		}

		fdt_size = fdt_totalsize(fdt);
		if (fdt_size < sizeof(struct fdt_header) || fdt_size > 0x100000) {
			DBG_WRN("fdt.app is corrupted\n");
			vos_file_close(fd);
			return -1;
		}

		len = vos_file_write(fd, (void *)fdt, fdt_size);
		if (len < fdt_size) {
			DBG_ERR("failed to write fdt.\n");
			vos_file_close(fd);
			return -1;
		}
		vos_file_close(fd);
		return 0;
	}
	return -1;
}

int fastboot_cmd_save(FASTBOOT_DRV_INFO *p_drv, unsigned char argc, char **argv)
{
	STRG_DEV strg = { 0 };
	unsigned char *fdt = NULL;

	if (get_strg(&strg) != 0) {
		DBG_ERR("get_strg failed.");
		return -1;
	}

	fdt = kzalloc(strg.partition_size, GFP_KERNEL);
	if (fdt == NULL) {
		DBG_DUMP("failed to alloc mem for fdt.app.\n");
		return -1;
	}

	if (read_fdt(&strg, fdt, strg.partition_size) != 0) {
		kfree(fdt);
		return -1;
	}

	if (fastboot_fdt_update(fdt) != 0) {
		kfree(fdt);
		return -1;
	}

	//shrink fdt is required, before write fdt into flash.
	fdt_pack(fdt);
	if (write_fdt(&strg, fdt) != 0) {
		kfree(fdt);
		return -1;
	}

	kfree(fdt);
	return 0;
}

int fastboot_cmd_dump(FASTBOOT_DRV_INFO *p_drv, unsigned char argc, char **argv)
{
	int fd;
	STRG_DEV strg = { 0 };
	int fdt_size = 0;
	unsigned char *fdt = NULL;

	if (argc < 2) {
		DBG_DUMP("target path is required\n");
		return -1;
	}

	if (get_strg(&strg) != 0) {
		DBG_ERR("get_strg failed.");
		return -1;
	}

	if(strg.partition_size < sizeof(struct fdt_header)) {
		DBG_ERR("fdt.app partition size is too small.\n");
		return -1;
	}

	fdt = kzalloc(strg.partition_size, GFP_KERNEL);
	if (fdt == NULL) {
		DBG_DUMP("failed to alloc mem for fdt.app.\n");
		return -1;
	}

	if (read_fdt(&strg, fdt, strg.partition_size) != 0) {
		kfree(fdt);
		return -1;
	}
	//make it real size
	fdt_pack(fdt);
	fd = vos_file_open(argv[1], O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		DBG_ERR("failed in file open:%s\r\n", argv[1]);
		kfree(fdt);
		return -1;
	}
	fdt_size = fdt_totalsize(fdt);
	if (fdt_size < 0 || fdt_size > strg.partition_size) {
		vos_file_close(fd);
		kfree(fdt);
		return -1;
	}
	vos_file_write(fd, (void *)fdt, fdt_size);
	vos_file_close(fd);
	kfree(fdt);
	return 0;
}

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_list[] = {
	// keyword      function name
	{ "save",       fastboot_cmd_save},
	{ "dump",       fastboot_cmd_dump},
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))


//=============================================================================
// proc "test1" file operation functions
//=============================================================================
static ssize_t fastboot_cmd_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	unsigned char argc = 0;
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char loop;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}
	if (len == 0) {
		nvt_dbg(ERR, "Command length is 0!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	for (argc = 0; argc < MAX_ARG_NUM; argc++) {
		argv[argc] = strsep(&cmdstr, delimiters);
		if (argv[argc] == NULL) {
			break;
		}
	}

	// dispatch command handler
	for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
		if (strncmp(argv[0], cmd_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
			ret = cmd_list[loop].execute(pdrv_info_data, argc, &argv[0]);
			break;
		}
	}
	if (loop >= NUM_OF_CMD) {
		goto ERR_INVALID_CMD;
	}
	return size;

ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD !!\r\n");
ERR_OUT:
	return -1;
}

static int fastboot_cmd_proc_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int fastboot_cmd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fastboot_cmd_proc_show, PDE_DATA(inode));
}

// plug file-system
static const struct file_operations fastboot_cmd_proc_fops = {
	.owner = THIS_MODULE,
	.open  = fastboot_cmd_proc_open,
	.read  = seq_read,
	.write   = fastboot_cmd_proc_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

int fastboot_proc_init(PFASTBOOT_DRV_INFO pdrv_info)
{
	int ret = 0;
	//struct proc_dir_entry *phdal_root = NULL;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir(MODULE_NAME, NULL);
	if (pmodule_root == NULL) {
		DBG_ERR("failed to create Module root\n");
		ret = -EINVAL;
		return ret;
	}
	pdrv_info->pproc_module_root = pmodule_root;

	// cmd
	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &fastboot_cmd_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc cmd!\r\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_cmd_entry = pentry;
	pdrv_info_data = pdrv_info;
	return ret;

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

void fastboot_proc_exit(PFASTBOOT_DRV_INFO pdrv_info)
{
	if (pdrv_info_data) {
		proc_remove(pdrv_info->pproc_cmd_entry);
		pdrv_info_data = NULL;
	}
}



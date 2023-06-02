#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/of_device.h>
#include <mach/drvdump.h>
#include "comm/drvdump.h"
#include <plat/efuse_protected.h>
#include <kwrap/verinfo.h>
/*============================================================================
Define
============================================================================*/
#define MAX_CMD_LENGTH 30
#define MAX_LIB_LENGTH 50
#define MAX_ARG_NUM     6

static char library_name[MAX_LIB_LENGTH];

typedef struct proc_drvdump {
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_summary_entry;
	struct proc_dir_entry *pproc_check_available_entry;
} proc_drvdump_t;
proc_drvdump_t proc_drvdump;

static int nvt_drvdump_proc_summary_show(struct seq_file *sfile, void *v)
{
	// pr_info("####### NAND ######\n");
	// nand_drvdump();
	// pr_info("####### PWM ######\n");
	// pwm_dump_info();
	// pr_info("####### IDE1 ######\n");
	// idec_dump_info(0);
	// pr_info("####### IDE2 ######\n");
	// idec_dump_info(1);
	// pr_info("####### EAC ######\n");
	// eac_dump_info();
	// pr_info("####### SSENIF ######\n");
	// ssenif_debug_dump(NULL);
	// pr_info("####### TV ######\n");
	// idetv_dump_info();
	// pr_info("####### DSI ######\n");
	// dsi_dump_info();
	pr_info("####### SDIO ######\n");
	sdio_drvdump();
	return 0;
}

static int nvt_drvdump_proc_summary_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_drvdump_proc_summary_show, NULL);
}

static struct file_operations proc_summary_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_drvdump_proc_summary_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


static int nvt_check_available_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "\nUsage\n");
	seq_printf(sfile, "\necho avl [library name] > efuse_check_available\n\n");
	pr_info("####### efuse_check_available ######\n");
	return 0;
}

static ssize_t nvt_check_available_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	BOOL    avl = FALSE;

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

//	printk("CMD:%s\n", cmd_line);

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

	if (!strcmp(argv[0], "avl")) {
		//cmd_efuse_show_avl();
		memset((void *)library_name, 0x0, sizeof(library_name));

		if (strlen(argv[1])) {
			if (strlen(argv[1])> MAX_LIB_LENGTH) {
				pr_err("invalide length %d\n", strlen(argv[1]));
				goto ERR_OUT;
			}
			strcpy((void *)library_name, argv[1]);
//			pr_info("linrary nane [%s]\r\n", argv[1]);
			avl = efuse_check_available(argv[1]);
//			pr_info("efuse_check_available = %d\r\n", avl);
		}

		return size;
	}
ERR_OUT:
	return size;
}

static ssize_t nvt_check_available_proc_cmd_read(struct file *fp, char __user *ubuf, size_t cnt, loff_t *ppos)
{
	int ret = 0;
	int len;
	char kbuf[32];
	BOOL    avl = FALSE;

	//pr_info("nvt_check_available_proc_cmd_read cnt = [%d]\r\n", cnt);

	if (strlen(library_name)&& !*ppos) {
		avl = efuse_check_available(library_name);

		sprintf(kbuf, "%d\n", avl);
		len = strlen(kbuf);
		if (clear_user((void *)ubuf, cnt)) {
			printk(KERN_ERR "clear error\n");
			return -EIO;
		}

		ret = copy_to_user(ubuf, kbuf, len);
		if (ret) {
			printk(KERN_ERR "copy_to_user error\n");
			return -EIO;
		}

		*ppos += len;
		return len;
	}

	return ret;
}


static int nvt_check_available_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_check_available_proc_show, NULL);
}

static struct file_operations proc_efuse_check_available_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_check_available_proc_open,
	.release = single_release,
	.read   = nvt_check_available_proc_cmd_read,
	.llseek = seq_lseek,
	.write  = nvt_check_available_proc_cmd_write
};



static int nvt_drvdump_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("nvt_info/drvdump", NULL);

	if (pmodule_root == NULL) {
		pr_err("failed to create Module root\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_drvdump.pproc_module_root = pmodule_root;

	pentry = proc_create("info", S_IRUGO | S_IXUGO, pmodule_root, &proc_summary_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc drvdump!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_drvdump.pproc_summary_entry = pentry;


	pentry = proc_create("available", S_IRUGO | S_IXUGO, pmodule_root, &proc_efuse_check_available_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc check available!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_drvdump.pproc_check_available_entry = pentry;


remove_proc:
	return ret;

	return 0;
}

static int nvt_drvdump_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id drvdump_match_table[] = {
	{   .compatible = "nvt,nvt_drvdump"},
	{}
};

static struct platform_driver nvt_drvdump_driver = {
	.driver = {
		.name   = "nvt_drvdump",
		.owner = THIS_MODULE,
		.of_match_table = drvdump_match_table,
	},
	.probe      = nvt_drvdump_probe,
	.remove     = nvt_drvdump_remove,
};

int __init nvt_drvdump_init(void)
{
	int ret;

	ret = platform_driver_register(&nvt_drvdump_driver);

	return 0;
}

void __exit nvt_drvdump_exit(void)
{
	platform_driver_unregister(&nvt_drvdump_driver);
}

#if defined(_GROUP_KO_)
#undef __init
#undef __exit
#undef module_init
#undef module_exit
#define __init
#define __exit
#define module_init(x)
#define module_exit(x)
#ifndef MODULE // in built-in case
#undef MODULE_VERSION
#undef VOS_MODULE_VERSION
#define MODULE_VERSION(x)
#define VOS_MODULE_VERSION(a1, a2, a3, a4, a5)
#endif
#endif

VOS_MODULE_VERSION(nvt_drvdump, 1, 00, 000, 00);

module_init(nvt_drvdump_init);
module_exit(nvt_drvdump_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("drvdump driver");
MODULE_LICENSE("GPL");
#else
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "kwrap/debug.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "dma.h"

#define _Y_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_YELLOW fmt DBG_COLOR_END, ##args)
#define _R_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_RED fmt DBG_COLOR_END, ##args)
#define _M_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_MAGENTA fmt DBG_COLOR_END, ##args)
#define _G_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_GREEN fmt DBG_COLOR_END, ##args)
#define _W_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_WHITE fmt DBG_COLOR_END, ##args)
#define _X_LOG(fmt, args...)         	DBG_DUMP(DBG_COLOR_HI_GRAY fmt DBG_COLOR_END, ##args)

extern UINT32 _ttb[];                       // symbol from lds file

/**
    Dump MMU TLB setting

    This API will dump all TLB mapping information.

    @return void
*/
static void drv_dump_mmu_info(void)
{
    UINT32 i;
    printf("tlb base addr 0x%lx\r\n", (unsigned long)_ttb);

    for (i=0; i<4096; i++) {
        printf("0x%4lx:\t0x%lx\t",
            (unsigned long)i, (unsigned long)_ttb[i]);
        if (_ttb[i] & 0x01) {
            // L2 descriptor
            UINT32 uiL2Addr;
            UINT32 j;
            UINT32 *pL2Desc;

            uiL2Addr = (_ttb[i] >> 10) * 1024;
            pL2Desc = (UINT32*)uiL2Addr;
            _G_LOG("L2 desc addr:0x%lx\t\r\n",
                    (unsigned long)uiL2Addr);
            if (uiL2Addr > dma_getDramCapacity(DMA_ID_1)) {
                _R_LOG("\t\t\t\tL2 desc out range\r\n");
                continue;
            }
            for (j=0; j<256; j++) {
                printf("\t\t\t\t0x%4lx:\t0x%lx\t", (unsigned long)j, (unsigned long)pL2Desc[j]);
                if (pL2Desc[j] & 0x01) {
                    // large page
                    printf("VA:0x%lx\t",
                        (unsigned long)i*1024*1024 + (unsigned long)j*4096);
                    printf("PA:0x%lx\t\r\n", (unsigned long)(pL2Desc[j]>>16)*64*1024);
                } else if (pL2Desc[j] & 0x02) {
                    // small page
                    printf("VA:0x%lx\t",
                        (unsigned long)i*1024*1024 + j*4096);
                    printf("PA:0x%lx\t\r\n", (unsigned long)(pL2Desc[j]>>12)*4*1024);
                } else {
                    // ignored
                    printf("IGNORED\r\n");
                }
            }
        } else if (_ttb[i] & 0x02) {
            // valid L1 descriptor
            if (! (_ttb[i] & (1<<18))) {
                // normal section (unit: 1MB)
                printf("VA:0x%lx\t",
                    (unsigned long)i*1024*1024);
                printf("PA:0x%lx\t", (unsigned long)(_ttb[i]>>20)*1024*1024);
                if ((_ttb[i]&0xFFFFF) == 0x15DE6) {
                    printf("VALID\r\n");
                } else if ((_ttb[i]&0xFFFFF) == 0x11DE2) {
                    printf("VALID\r\n");
                } else if ((_ttb[i]&0xFFFFF) == 0x00DE6) {
                    printf("VALID\r\n");
                } else if ((_ttb[i]) == 0x0000) {
                    printf("VALID\r\n");
                } else {
                    _R_LOG("INVALID\r\n");
                }
            } else {
                // super section (unit: 16MB)
                printf("VA:0x%lx\t",
                    (unsigned long)i*1024*1024);
                printf("PA:0x%lx\t", (unsigned long)(_ttb[i]>>24)*16*1024*1024);
                _R_LOG("INVALID\r\n");
            }
        } else {
            // ingored descriptor
            _G_LOG("IGNORED\r\n");
        }
    }
}

MAINFUNC_ENTRY(drvdump, argc, argv)
{
	drv_dump_mmu_info();
	// DBG_DUMP("####### NAND ######\n");
	// nand_drvdump();
	// DBG_DUMP("####### PWM ######\n");
	// pwm_dump_info();
	// DBG_DUMP("####### IDE1 ######\n");
	// idec_dump_info(0);
	// DBG_DUMP("####### IDE2 ######\n");
	// idec_dump_info(1);
	// DBG_DUMP("####### EAC ######\n");
	// eac_dump_info();
	// DBG_DUMP("####### SSENIF ######\n");
	// ssenif_debug_dump(NULL);
	// DBG_DUMP("####### TV ######\n");
	// idetv_dump_info();
	// DBG_DUMP("####### DSI ######\n");
	// dsi_dump_info();
	// DBG_DUMP("####### SDIO ######\n");
	// sdio_drvdump();
	return 0;
}
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/virtio.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "ax_hwinfo.h"
#include <linux/version.h>

static struct proc_dir_entry *uid_file;
static struct proc_dir_entry *board_id_file;
static struct proc_dir_entry *chip_type_file;
static misc_info_t misc_info;
static u32 ax_uid_l = 0;
static u32 ax_uid_h = 0;
static u32 board_id = 0;
static u32 chip_type = 0;
static u32 phy_board_id = 0;

static const char * chip_type_name[AX620E_CHIP_MAX] = {
	[AX620Q_CHIP] = "AX620Q_CHIP",
	[AX620QX_CHIP] = "AX620QX_CHIP",
	[AX630C_CHIP] = "AX630C_CHIP",
	[AX631_CHIP] = "AX631_CHIP",
	[AX620QZ_CHIP] = "AX620QZ_CHIP",
	[AX620QP_CHIP] = "AX620QP_CHIP",
	[AX620QE_CHIP] = "AX620QE_CHIP",
	[AX620QF_CHIP] = "AX620QF_CHIP",
};

// ### SIPEED EDIT ###
static char * board_name[AX620E_CHIP_MAX][17] = {
// ### SIPEED EDIT END ###

	/*AX620Q_CHIP board name*/
	[AX620Q_CHIP][PHY_AX620Q_LP4_EVB_V1_0] = "AX620Q_LP4_EVB_V1_0",
	[AX620Q_CHIP][PHY_AX620Q_LP4_DEMO_V1_0] = "AX620Q_LP4_DEMO_V1_0",
	[AX620Q_CHIP][PHY_AX620Q_LP4_SLT_V1_0] = "AX620Q_LP4_SLT_V1_0",
	[AX620Q_CHIP][PHY_AX620Q_LP4_DEMO_V1_1] = "AX620Q_LP4_DEMO_V1_1",
	[AX620Q_CHIP][PHY_AX620Q_LP4_38BOARD_V1_0] = "AX620Q_LP4_38BOARD_V1_0",
	[AX620Q_CHIP][PHY_AX620Q_LP4_MINION_BOARD] = "AX620Q_LP4_MINION_BOARD",
	// ### SIPEED EDIT ###
	[AX620Q_CHIP][PHY_AX620Q_LP4_NANOAGENT_256M] = "AX620Q_LP4_NANOAGENT_256M",
	// ### SIPEED EDIT END ###
	/*AX620QX_CHIP board name*/
	[AX620QX_CHIP][PHY_AX620Q_LP4_EVB_V1_0] = "AX620QX_LP4_EVB_V1_0",
	[AX620QX_CHIP][PHY_AX620Q_LP4_DEMO_V1_0] = "AX620QX_LP4_DEMO_V1_0",
	[AX620QX_CHIP][PHY_AX620Q_LP4_SLT_V1_0] = "AX620QX_LP4_SLT_V1_0",
	[AX620QX_CHIP][PHY_AX620Q_LP4_DEMO_V1_1] = "AX620QX_LP4_DEMO_V1_1",
	[AX620QX_CHIP][PHY_AX620Q_LP4_38BOARD_V1_0] = "AX620QX_LP4_38BOARD_V1_0",
	[AX620QX_CHIP][PHY_AX620Q_LP4_MINION_BOARD] = "AX620QX_LP4_MINION_BOARD",
	/*AX630C_CHIP board name*/
	[AX630C_CHIP][PHY_AX630C_EVB_V1_0] = "AX630C_EVB_V1_0",
	[AX630C_CHIP][PHY_AX630C_DEMO_V1_0] = "AX630C_DEMO_V1_0",
	[AX630C_CHIP][PHY_AX630C_SLT_V1_0] = "AX630C_SLT_V1_0",
	[AX630C_CHIP][PHY_AX630C_DEMO_V1_1] = "AX630C_DEMO_V1_1",
	[AX630C_CHIP][PHY_AX630C_DEMO_LP4_V1_0] = "AX630C_DEMO_LP4_V1_0",
	// ### SIPEED EDIT ###
	[AX630C_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G] = "AX630C_MAIXCAM2_SOM_0_5G",
	[AX630C_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_1G] = "AX630C_MAIXCAM2_SOM_1G",
	[AX630C_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_2G] = "AX630C_MAIXCAM2_SOM_2G",
	[AX630C_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_4G] = "AX630C_MAIXCAM2_SOM_4G",
	// ### SIPEED EDIT END ###
	/*AX631_CHIP board name*/
	[AX631_CHIP][PHY_AX630C_EVB_V1_0] = "AX631_EVB_V1_0",
	[AX631_CHIP][PHY_AX630C_DEMO_V1_0] = "AX631_DEMO_V1_0",
	[AX631_CHIP][PHY_AX630C_SLT_V1_0] = "AX631_SLT_V1_0",
	[AX631_CHIP][PHY_AX630C_DEMO_V1_1] = "AX631_DEMO_V1_1",
	[AX631_CHIP][PHY_AX630C_DEMO_LP4_V1_0] = "AX631_DEMO_LP4_V1_0",
	// ### SIPEED EDIT ###
	[AX631_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G] = "AX631_MAIXCAM2_SOM_0_5G",
	[AX631_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_1G] = "AX631_MAIXCAM2_SOM_1G",
	[AX631_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_2G] = "AX631_MAIXCAM2_SOM_2G",
	[AX631_CHIP][PHY_AX630C_AX631_MAIXCAM2_SOM_4G] = "AX631_MAIXCAM2_SOM_1G",
	// ### SIPEED EDIT END ###
	/*AX620QZ_CHIP board name*/
	[AX620QZ_CHIP][PHY_AX620QZ_LP4_DEMO_V1_0] = "AX620QZ_LP4_DEMO_V1_0",
	[AX620QZ_CHIP][PHY_AX620QZ_LP4_EVB_V1_0] = "AX620QZ_LP4_EVB_V1_0",
	/*AX620QP_CHIP board name*/
	[AX620QP_CHIP][PHY_AX620Q_LP4_EVB_V1_0] = "AX620QP_LP4_EVB_V1_0",
	[AX620QP_CHIP][PHY_AX620Q_LP4_DEMO_V1_0] = "AX620QP_LP4_DEMO_V1_0",
	[AX620QP_CHIP][PHY_AX620Q_LP4_SLT_V1_0] = "AX620QP_LP4_SLT_V1_0",
	[AX620QP_CHIP][PHY_AX620Q_LP4_DEMO_V1_1] = "AX620QP_LP4_DEMO_V1_1",
	[AX620QP_CHIP][PHY_AX620Q_LP4_38BOARD_V1_0] = "AX620QP_LP4_38BOARD_V1_0",
	[AX620QP_CHIP][PHY_AX620Q_LP4_MINION_BOARD] = "AX620QP_LP4_MINION_BOARD",
	/*AX620QE/F_CHIP board name*/
	[AX620QE_CHIP][PHY_AX620QE_F_DEMO_V1_0] = "AX620QE_DEMO_V1_0",
	// ### SIPEED EDIT ###
	[AX620QE_CHIP][PHY_AX620QE_LP4_NANOAGENT_512M] = "AX620QE_LP4_NANOAGENT_512M",
	[AX620QF_CHIP][PHY_AX620QF_LP4_NANOAGENT_256M] = "AX620QF_LP4_NANOAGENT_256M",
	// ### SIPEED EDIT END ###
	[AX620QF_CHIP][PHY_AX620QE_F_DEMO_V1_0] = "AX620QF_DEMO_V1_0",
};

static int ax_get_uid(void)
{
	ax_uid_l = misc_info.uid_l;
	ax_uid_h = misc_info.uid_h;

	return 0;
}

u32 ax_info_get_board_id(void)
{
	board_id = misc_info.board_id;
	phy_board_id = misc_info.phy_board_id;

	return board_id;
}
EXPORT_SYMBOL(ax_info_get_board_id);

u32 ax_get_chip_type(void)
{
	chip_type = misc_info.chip_type;

	return chip_type;
}
EXPORT_SYMBOL(ax_get_chip_type);

static int ax_uid_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "ax_uid: 0x%08x%08x\n", ax_uid_h, ax_uid_l);
	return 0;
}
static int ax_uid_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_uid_proc_show, NULL);
}

static struct file_operations ax_uid_fops = {
	.owner  = THIS_MODULE,
	.open   = ax_uid_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};
static int ax_uid_create_proc_file(void)
{
	uid_file = proc_create_data(AX_UID_NODE_NAME, 0644, NULL, &ax_uid_fops, NULL);
	if (!uid_file) {
		printk("uid proc_create_data failed\n");
		return -ENOMEM;
	}
	return 0;
}

static int ax_board_id_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", board_name[chip_type][phy_board_id]);
	return 0;
}

static int ax_board_id_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_board_id_proc_show, NULL);
}

static struct file_operations ax_board_id_fops = {
	.owner  = THIS_MODULE,
	.open   = ax_board_id_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};
static int ax_board_id_create_proc_file(void)
{
	board_id_file = proc_create_data(AX_BOARD_ID_NODE_NAME, 0644, NULL, &ax_board_id_fops, NULL);
	if (!board_id_file) {
		printk("board_id proc_create_data failed\n");
		return -ENOMEM;
	}
	return 0;
}

static int ax_chip_type_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", chip_type_name[chip_type]);
	return 0;
}
static int ax_chip_type_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_chip_type_proc_show, NULL);
}

static struct file_operations ax_chip_type_fops = {
	.owner  = THIS_MODULE,
	.open   = ax_chip_type_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};
static int ax_chip_type_create_proc_file(void)
{
	chip_type_file = proc_create_data(AX_CHIP_TYPE_NODE_NAME, 0644, NULL, &ax_chip_type_fops, NULL);
	if (!chip_type_file) {
		printk("chip_type proc_create_data failed\n");
		return -ENOMEM;
	}
	return 0;
}

static int ax_hwinfo_probe(struct platform_device *pdev)
{
	void __iomem *regs;
	regs = ioremap(MISC_INFO_ADDR, sizeof(misc_info_t));
	memcpy(&misc_info, regs, sizeof(misc_info_t));
	ax_board_id_create_proc_file();
	ax_chip_type_create_proc_file();
	ax_uid_create_proc_file();
	ax_get_uid();
	ax_get_chip_type();
	ax_info_get_board_id();
	iounmap(regs);
	return 0;
}
static int ax_hwinfo_remove(struct platform_device *pdev)
{
	remove_proc_entry(AX_UID_NODE_NAME, NULL);
	remove_proc_entry(AX_CHIP_TYPE_NODE_NAME, NULL);
	remove_proc_entry(AX_BOARD_ID_NODE_NAME, NULL);
	return 0;
}
static const struct of_device_id ax_hwinfo_of_match[] = {
	{.compatible = "ax,ax_hwinfo" },
	{ }
};
static struct platform_driver ax_hwinfo_driver = {
	.probe = ax_hwinfo_probe,
	.remove = ax_hwinfo_remove,
	.driver = {
		.name  = KBUILD_MODNAME,
		.of_match_table = ax_hwinfo_of_match,
	},
};
static int __init ax_hwinfo_init(void)
{
	platform_driver_register(&ax_hwinfo_driver);
	return 0;
}
static void ax_hwinfo_exit(void)
{
	platform_driver_unregister(&ax_hwinfo_driver);
}
module_init(ax_hwinfo_init);
module_exit(ax_hwinfo_exit);
MODULE_DESCRIPTION("axera ax_hwinfo driver");
MODULE_AUTHOR("Axera Inc.");
MODULE_LICENSE("GPL v2");

/*
 * fw_loader.c
 *
 *  Created on: Aug 9, 2016
 *      Author: duobao
 */

#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <mach/fh8830.h>
#include <mach/pmu.h>
#include <asm/delay.h>
#include <asm/uaccess.h>

#define DEVICE_NAME   "fw_loader"

struct fwl_parameter
{
    unsigned int addr;
    unsigned int size;
};

struct fwl_description
{
    char* name;
    void *virt_addr;
    unsigned int phy_addr;
    const struct firmware *fw_entry;
    struct fwl_parameter fwl_param;
};

//#define  FW_LOADER_DEBUG
#ifdef FW_LOADER_DEBUG
#define PRINT_DBG(fmt,args...)  printk(fmt,##args)
#else
#define PRINT_DBG(fmt,args...)  do{} while(0)
#endif


#define FWL_LOADER_IOCTL_MAGIC             'f'

#define FWL_LOAD_FW          _IOWR(FWL_LOADER_IOCTL_MAGIC, 0, unsigned int)
#define FWL_SET_PARAM        _IOWR(FWL_LOADER_IOCTL_MAGIC, 1, struct fwl_parameter)
#define FWL_START_FW         _IOWR(FWL_LOADER_IOCTL_MAGIC, 2, unsigned int)

#define FWL_LOADER_IOCTL_MAXNR             8


/* Module parameters */
#define FIRMWARE_NAME   "rtthread_arc.bin"
static char* fw_name = FIRMWARE_NAME;
module_param(fw_name, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fw_name, "firmware name");

#define FIRMWARE_PHY_ADDRESS   0xa7800000
static unsigned int fw_phy_addr = FIRMWARE_PHY_ADDRESS;
module_param(fw_phy_addr, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fw_phy_addr, "firmware address");

static struct miscdevice fwl_loader_device;
static struct fwl_description *fwl_desc;

static int fwl_load_firmware(void)
{
    int ret = 0;

    if(fwl_desc->fw_entry)
        fwl_desc->virt_addr = ioremap(fwl_desc->phy_addr, fwl_desc->fw_entry->size);
    else
        pr_err("ERROR: SET_FW_ADDRESS, fw_entry is NULL\n");

    if(!fwl_desc->virt_addr)
        pr_err("ERROR: SET_FW_ADDRESS, ioremap failed\n");

    memcpy(fwl_desc->virt_addr, fwl_desc->fw_entry->data, fwl_desc->fw_entry->size);

    writel(fwl_desc->fwl_param.addr, fwl_desc->virt_addr + fwl_desc->fw_entry->size - 8);
    writel(fwl_desc->fwl_param.size, fwl_desc->virt_addr + fwl_desc->fw_entry->size - 4);


    iounmap(fwl_desc->virt_addr);
    fwl_desc->virt_addr = NULL;

    printk("firmware: %s loaded\n", fwl_desc->name);

    return ret;
}

static int fwl_start_firmware(void)
{
    unsigned int arc_addr;
    unsigned int reg;

    arc_addr = ((fwl_desc->phy_addr & 0xffff) << 16) | (fwl_desc->phy_addr >> 16);

    // ARC Reset
    fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, 0xFFBFFFFF);

    fh_pmu_set_reg(REG_PMU_A625BOOT0 , 0x7940266B);
    fh_pmu_set_reg(REG_PMU_A625BOOT1 , arc_addr);  // Configure ARC Bootcode start address
    fh_pmu_set_reg(REG_PMU_A625BOOT2 , 0x0F802020);
    fh_pmu_set_reg(REG_PMU_A625BOOT3 , arc_addr);

    fh_pmu_set_reg(REG_PMU_REMAP , 0 );  // Disable A2X BUS Remap and Resize

    // ARC reset released
    fh_pmu_set_reg( REG_PMU_SWRSTN_NSR, 0xFFFFFFFF);

    // wait ramloader done, about 1024 ARC CPU cycle
    udelay(100);

    // start ARC625
    reg = fh_pmu_get_reg(REG_PMU_A625_START_CTRL);
    reg &= ~(0xff);
    reg |= 0x10;
    fh_pmu_set_reg(REG_PMU_A625_START_CTRL, reg);

    printk("firmware: %s started\n", fwl_desc->name);

    return 0;
}


static long fwl_loader_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    if (unlikely(_IOC_TYPE(cmd) != FWL_LOADER_IOCTL_MAGIC))
    {
        pr_err("%s: ERROR: incorrect magic num %d (error: %d)\n",
               __func__, _IOC_TYPE(cmd), -ENOTTY);
        return -ENOTTY;
    }

    if (unlikely(_IOC_NR(cmd) > FWL_LOADER_IOCTL_MAXNR))
    {
        pr_err("%s: ERROR: incorrect cmd num %d (error: %d)\n",
               __func__, _IOC_NR(cmd), -ENOTTY);
        return -ENOTTY;
    }

    if (_IOC_DIR(cmd) & _IOC_READ)
    {
        ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
    {
        ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if(ret)
    {
        pr_err("%s: ERROR: user space access is not permitted %d (error: %d)\n",
               __func__, _IOC_NR(cmd), -EACCES);
        return -EACCES;
    }

    switch(cmd)
    {
    case FWL_SET_PARAM:
        PRINT_DBG("%s, FWL_SET_PARAM start\n", __func__);
        ret = __copy_from_user(&fwl_desc->fwl_param,
                (struct fwl_parameter __user *)arg,
                sizeof(struct fwl_parameter));
        break;
    case FWL_LOAD_FW:
        PRINT_DBG("%s, FWL_LOAD_FW start\n", __func__);
        __get_user(fwl_desc->phy_addr, (unsigned int __user *)arg);
        fwl_load_firmware();
        break;
    case FWL_START_FW:
        PRINT_DBG("%s, FWL_START_FW start\n", __func__);
        fwl_start_firmware();
        break;

    }

    return ret;
}

static int fwl_loader_open(struct inode *inode, struct file *file)
{

    int ret;
    PRINT_DBG("%s, start\n", __func__);

    ret = request_firmware(&fwl_desc->fw_entry, fwl_desc->name, fwl_loader_device.this_device);

    if(ret)
    {
        pr_err("ERROR: %s, request firmware failed, ret: %d\n", __func__, ret);
    }

    return ret;
}

static int fwl_loader_release(struct inode *inode, struct file *filp)
{
    release_firmware(fwl_desc->fw_entry);
    return 0;
}

static const struct file_operations fwl_loader_fops =
{
    .owner                  = THIS_MODULE,
    .open                   = fwl_loader_open,
    .release                = fwl_loader_release,
    .unlocked_ioctl         = fwl_loader_ioctl,
};

static struct miscdevice fwl_loader_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &fwl_loader_fops,
};

static int __init fwl_loader_init(void)
{
    int ret;

    fwl_desc = (struct fwl_description *)kzalloc(sizeof(struct fwl_description), GFP_KERNEL);
    fwl_desc->name = fw_name;
    fwl_desc->phy_addr = fw_phy_addr;

    ret = misc_register(&fwl_loader_device);

    if(ret < 0)
    {
            pr_err("%s: ERROR: %s registration failed",
                       __func__, DEVICE_NAME);
            return -ENXIO;
    }



    return ret;
}

static void __exit fwl_loader_exit(void)
{
    misc_deregister(&fwl_loader_device);
    kfree(fwl_desc);
    fwl_desc = NULL;
}
module_init(fwl_loader_init);
module_exit(fwl_loader_exit);

MODULE_AUTHOR("QIN");
MODULE_DESCRIPTION("Misc Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: FH");

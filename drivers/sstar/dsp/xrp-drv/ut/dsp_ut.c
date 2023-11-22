/*
 * dsp_ut.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/syscalls.h>
#include "mdrv_dsp.h"
#include <linux/ioctl.h>
#include "dsp_ut.h"
#include "dsp_ut_datatype.h"
#include "dsp_fw.h"

typedef enum DSP_UT_IOC_TYPE
{
    DSP_UT_IOC_SET_CORE = 0,
    DSP_UT_IOC_SET_FIRMWARE,
    DSP_UT_IOC_SET_ADDRESS_MAP,
    DSP_UT_IOC_SET_PRID,
    DSP_UT_IOC_SET_RESET_VECTOR,
    DSP_UT_IOC_SET_DFS,
    DSP_UT_IOC_SET_JTAG,
    DSP_UT_IOC_TEST_BOOT_FIRMWARE,
    DSP_UT_IOC_TEST_GPI,
    DSP_UT_IOC_TEST_GPO,
    DSP_UT_IOC_TEST_APB,
    DSP_UT_IOC_TEST_IRQ,
    DSP_UT_IOC_NUM,
} DSP_UT_IOC_TYPE;

typedef enum DSP_UT_FW_CMD
{
    DSP_UT_FW_CMD_GPI_0 = 0xF0F0F0F0,
    DSP_UT_FW_CMD_GPI_1 = 0x0F0F0F0F,
    DSP_UT_FW_CMD_GPO_0 = 0x101,
    DSP_UT_FW_CMD_GPO_1 = 0x102,
    DSP_UT_FW_CMD_APB   = 0x201,
    DSP_UT_FW_CMD_IRQ_0 = 0x301,
    DSP_UT_FW_CMD_IRQ_1 = 0x302,
} DSP_UT_FW_CMD;

#define DSP_UT_IOC_MAXNR DSP_UT_IOC_NUM

#define BITS(_bits_, _val_)         ((BIT(((1)?_bits_)+1)-BIT(((0)?_bits_))) & (_val_<<((0)?_bits_)))
#define BMASK(_bits_)               (BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))

struct dsp_ut_dev
{
    int         data;
    struct cdev cdev;
} dev;

#define DSP_UT_DEV_MAJOR 0

static int         curCore    = 0;
static dsp_ut_core dspCore[4] = {0};
// static dsp_ut_dev dspDev[4] = { 0 };
static int dsp_ut_major = DSP_UT_DEV_MAJOR;
module_param(dsp_ut_major, int, S_IRUGO);

struct class *dsp_ut_class[4];
struct cdev   cdev[4];

//==================================================
// local  declaration
//==================================================
int dsp_ut_cmd_bootFirmware(MDRV_DSP_CoreID_e eCoreID);
int dsp_ut_cmd_GPI(MDRV_DSP_CoreID_e eCoreID);
int dsp_ut_cmd_GPO(MDRV_DSP_CoreID_e eCoreID);
int dsp_ut_cmd_APB(MDRV_DSP_CoreID_e eCoreID);
int dsp_ut_cmd_IRQ(MDRV_DSP_CoreID_e eCoreID);

long dsp_ut_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0, retval = 0;

    if (_IOC_TYPE(cmd) != DSP_UT_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > DSP_UT_IOC_MAXNR)
        return -ENOTTY;
    if (err)
        return -EFAULT;

    switch (cmd)
    {
        case DSP_UT_SET_CORE:
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = __get_user(curCore, (int __user *)arg);
            printk(KERN_EMERG "DSP_UT_SET_CORE[%d]\n", curCore);
            break;
        case DSP_UT_SET_FIRMWARE:
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            if ((retval = copy_from_user(dspCore[curCore].fwPath, (void __user *)arg, _IOC_SIZE(cmd))))
                return -EFAULT;
            printk(KERN_EMERG "DSP_UT_SET_FIRMWARE :%s\n", dspCore[curCore].fwPath);
            break;
        case DSP_UT_SET_ADDRESS_MAP:
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            if ((retval = copy_from_user(dspCore[curCore].addrMap, (void __user *)arg, _IOC_SIZE(cmd))))
                return -EFAULT;
            printk(KERN_EMERG "DSP_UT_SET_ADDRESS_MAP\n");
            break;
        case DSP_UT_SET_PRID:
            retval = __put_user(dspCore[curCore].prid, (int __user *)arg);
            printk(KERN_EMERG "DSP_UT_SET_PRID[%x]\n", dspCore[curCore].prid);
            break;
        case DSP_UT_SET_RESET_VECTOR:
            retval = __put_user(dspCore[curCore].resetVector, (int __user *)arg);
            printk(KERN_EMERG "DSP_UT_SET_RESET_VECTOR[%x]\n", dspCore[curCore].resetVector);
            break;
        case DSP_UT_SET_DFS:
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = __put_user(dspCore[curCore].dfs, (int __user *)arg);
            printk(KERN_EMERG "DSP_UT_SET_DFS[%x]\n", dspCore[curCore].dfs);
            break;
        case DSP_UT_SET_JTAG:
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = __put_user(dspCore[curCore].jtag, (int __user *)arg);
            printk(KERN_EMERG "DSP_UT_SET_JTAG[%x]\n", dspCore[curCore].jtag);
            break;
        case DSP_UT_TEST_BOOT_FIRMWARE:
            retval = dsp_ut_cmd_bootFirmware(curCore);
            printk(KERN_EMERG "DSP_UT_TEST_BOOT_FIRMWARE\n");
            break;
        case DSP_UT_TEST_GPI:
            retval = dsp_ut_cmd_GPI(curCore);
            printk(KERN_EMERG "DSP_UT_TEST_GPI\n");
            break;
        case DSP_UT_TEST_GPO:
            retval = dsp_ut_cmd_GPO(curCore);
            printk(KERN_EMERG "DSP_UT_TEST_GPO\n");
            break;
        case DSP_UT_TEST_APB:
            retval = dsp_ut_cmd_APB(curCore);
            printk(KERN_EMERG "DSP_UT_TEST_APB\n");
            break;
        case DSP_UT_TEST_IRQ:
            retval = dsp_ut_cmd_IRQ(curCore);
            printk(KERN_EMERG "DSP_UT_TEST_IRQ\n");
            break;
        default:
            printk(KERN_EMERG "DSP_UT_default\n");
            return -ENOTTY;
    }
    return retval;
}

irqreturn_t dsp_ut_irq_handler(int irqnum, void *args)
{
    dsp_ut_core *dspUtCtx = (dsp_ut_core *)args;
    atomic_inc(&dspUtCtx->dspIrqCnt);
    printk("%s %s cnt:%d\n", __FUNCTION__, dspUtCtx->irqName, atomic_read(&dspUtCtx->dspIrqCnt));
    return IRQ_HANDLED;
}

int dsp_ut_cmd_bootFirmware(MDRV_DSP_CoreID_e eCoreID)
{
    MDRV_DSP_ResetCtx_t stRstCtx;
    dsp_ut_core *       dspUtCtx = &dspCore[eCoreID];
    MS_U32              irqNum   = 0;
    MS_U32              irqCnt   = 0;
    dsp_ut_fw_Request(dspUtCtx, dspCore[eCoreID].fwPath);
    dsp_ut_fw_Load(dspUtCtx);
    Mdrv_DSP_Init(eCoreID);
    Mdrv_DSP_RunStallReset(eCoreID, TRUE);
    irqCnt                   = atomic_read(&dspUtCtx->dspIrqCnt);
    stRstCtx.u16Prid         = dspUtCtx->prid;
    stRstCtx.u32ResetVecAddr = dspUtCtx->resetVector;
    stRstCtx.eDbgMode = E_MDRV_DSP_DBG_PDBGEN | E_MDRV_DSP_DBG_DBGEN | E_MDRV_DSP_DBG_NIDEN | E_MDRV_DSP_DBG_SPIDEN
                        | E_MDRV_DSP_DBG_SPNIDEN;
    memcpy(stRstCtx.astAddrMap, dspUtCtx->addrMap, sizeof(MDRV_DSP_AddressMap_t) * E_MDRV_DSP_ADDRESS_SET_NUM);
    Mdrv_DSP_Reset(eCoreID, &stRstCtx);
    Mdrv_DSP_GetDspIrqNumById(eCoreID, &irqNum);
    sprintf(dspUtCtx->irqName, "dsp_ut_irq[%d]", irqNum);
    if (0 != request_irq(irqNum, dsp_ut_irq_handler, IRQF_SHARED, dspUtCtx->irqName, dspUtCtx))
    {
        printk("request_irq [%s][%d] fail\n", __FUNCTION__, __LINE__);
    }
    Mdrv_DSP_RunStallReset(eCoreID, FALSE);
    if (!wait_event_timeout(dspUtCtx->dspWQ, irqCnt < atomic_read(&dspUtCtx->dspIrqCnt), 10))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
void dsp_ut_cmd_StartDSPGpoIrq(MDRV_DSP_CoreID_e eCoreID, MS_U32 irqNum)
{
    MDRV_DSP_IRQStat_t stIRQStat;
    memset(&stIRQStat, 0, sizeof(MDRV_DSP_IRQStat_t));
    stIRQStat.bClearIrq      = 0;
    stIRQStat.bEdgeEn        = 1;
    stIRQStat.bForceValid    = 0;
    stIRQStat.bMskIrq        = 0;
    stIRQStat.bNegPol        = 0;
    stIRQStat.bStatBeforeMsk = 0;
    Mdrv_DSP_SetIRQStat(eCoreID, &stIRQStat, E_MDRV_DSP_IRQ_STAT_ALL, irqNum);
}
void dsp_ut_cmd_StopDSPGpoIrq(MDRV_DSP_CoreID_e eCoreID, MS_U32 irqNum)
{
    MDRV_DSP_IRQStat_t stIRQStat;
    memset(&stIRQStat, 0, sizeof(MDRV_DSP_IRQStat_t));
    stIRQStat.bClearIrq      = 0;
    stIRQStat.bEdgeEn        = 0;
    stIRQStat.bForceValid    = 0;
    stIRQStat.bMskIrq        = 1;
    stIRQStat.bNegPol        = 0;
    stIRQStat.bStatBeforeMsk = 0;
    Mdrv_DSP_SetIRQStat(eCoreID, &stIRQStat, E_MDRV_DSP_IRQ_STAT_ALL, irqNum);
}

int dsp_ut_cmd_GPI(MDRV_DSP_CoreID_e eCoreID)
{
    int          ret      = 0;
    MS_U32       irqCnt   = 0;
    dsp_ut_core *dspUtCtx = &dspCore[eCoreID];

    Mdrv_DSP_SetGPI(eCoreID, DSP_UT_FW_CMD_GPI_0);
    Mdrv_DSP_InterruptTrigger(eCoreID, 1);
    irqCnt = atomic_read(&dspUtCtx->dspIrqCnt);

    if (!wait_event_timeout(dspUtCtx->dspWQ, irqCnt < atomic_read(&dspUtCtx->dspIrqCnt), 10))
    {
        printk(" GPI 0xF0F0F0F0 succ\n");
    }
    else
    {
        printk(" GPI 0xF0F0F0F0 fail\n");
        ret = -1;
        goto out;
    }
    Mdrv_DSP_SetGPI(eCoreID, DSP_UT_FW_CMD_GPI_1);
    Mdrv_DSP_InterruptTrigger(eCoreID, 1);
    irqCnt = atomic_read(&dspUtCtx->dspIrqCnt);

    if (!wait_event_timeout(dspUtCtx->dspWQ, irqCnt < atomic_read(&dspUtCtx->dspIrqCnt), 10))
    {
        printk(" GPI 0x0F0F0F0F succ\n");
    }
    else
    {
        printk(" GPI 0x0F0F0F0F fail\n");
        ret = -1;
        goto out;
    }
out:
    return ret;
}
int dsp_ut_cmd_GPO(MDRV_DSP_CoreID_e eCoreID)
{
    int          ret      = 0;
    MS_U8        irqNum   = 0;
    MS_U32       irqCnt   = 0;
    MS_U32       gpo      = 0;
    dsp_ut_core *dspUtCtx = &dspCore[eCoreID];

    for (; irqNum < 32; irqNum++)
    {
        dsp_ut_cmd_StopDSPGpoIrq(eCoreID, irqNum);
    }
    dsp_ut_cmd_StartDSPGpoIrq(eCoreID, 15);
    Mdrv_DSP_SetGPI(eCoreID, DSP_UT_FW_CMD_GPO_0);
    Mdrv_DSP_InterruptTrigger(eCoreID, 1);
    irqCnt = atomic_read(&dspUtCtx->dspIrqCnt);

    if (!wait_event_timeout(dspUtCtx->dspWQ, irqCnt < atomic_read(&dspUtCtx->dspIrqCnt), 10))
    {
        Mdrv_DSP_GetGPO(eCoreID, &gpo);
        if (gpo == 0xF0F0F0F0)
        {
            printk(" GPO 0xF0F0F0F0 succ\n");
        }
        else
        {
            printk(" GPO 0xF0F0F0F0 %d fail\n", gpo);
            ret = -1;
            goto out;
        }
    }
    else
    {
        printk(" arm irq trigger %d fail\n", irqNum);
        ret = -2;
        goto out;
    }
    dsp_ut_cmd_StopDSPGpoIrq(eCoreID, 15);
    dsp_ut_cmd_StartDSPGpoIrq(eCoreID, 0);
    Mdrv_DSP_SetGPI(eCoreID, DSP_UT_FW_CMD_GPO_1);

    Mdrv_DSP_InterruptTrigger(eCoreID, 1);
    irqCnt = atomic_read(&dspUtCtx->dspIrqCnt);

    if (!wait_event_timeout(dspUtCtx->dspWQ, irqCnt < atomic_read(&dspUtCtx->dspIrqCnt), 10))
    {
        Mdrv_DSP_GetGPO(eCoreID, &gpo);
        if (gpo == 0x0F0F0F0F)
        {
            printk(" GPO 0x0F0F0F0F succ\n");
        }
        else
        {
            printk(" GPO 0x0F0F0F0F %d fail\n", gpo);
            ret = -1;
            goto out;
        }
    }
    else
    {
        printk(" arm irq trigger %d fail\n", irqNum);
        ret = -2;
        goto out;
    }
out:
    return ret;
}
int dsp_ut_cmd_APB(MDRV_DSP_CoreID_e eCoreID)
{
#define DSP_COMPONENT_ID0 0x3FF0
#define DSP_COMPONENT_ID1 0x3FF4
#define DSP_COMPONENT_ID2 0x3FF8
#define DSP_COMPONENT_ID3 0x3FFC

#define DSP_RESET_VAL_COMPONENT_ID0 0xd
#define DSP_RESET_VAL_COMPONENT_ID1 0x90
#define DSP_RESET_VAL_COMPONENT_ID2 0x5
#define DSP_RESET_VAL_COMPONENT_ID3 0xb1

    MS_U32 u32Cnt = 0;
    MS_U32 ret    = 0;
    Mdrv_DSP_APBRead(eCoreID, DSP_COMPONENT_ID0, &u32Cnt, 0XFFFFFFFF);
    if (u32Cnt != DSP_RESET_VAL_COMPONENT_ID0)
    {
        ret = -1;
        printk(" APB read DSP_COMPONENT_ID0 fail\n");
    }
    Mdrv_DSP_APBRead(eCoreID, DSP_COMPONENT_ID1, &u32Cnt, 0XFFFFFFFF);
    if (u32Cnt != DSP_RESET_VAL_COMPONENT_ID1)
    {
        ret = -1;
        printk(" APB read DSP_COMPONENT_ID1 fail\n");
    }
    Mdrv_DSP_APBRead(eCoreID, DSP_COMPONENT_ID2, &u32Cnt, 0XFFFFFFFF);
    if (u32Cnt != DSP_RESET_VAL_COMPONENT_ID2)
    {
        ret = -1;
        printk(" APB read DSP_COMPONENT_ID2 fail\n");
    }
    Mdrv_DSP_APBRead(eCoreID, DSP_COMPONENT_ID3, &u32Cnt, 0XFFFFFFFF);
    if (u32Cnt != DSP_RESET_VAL_COMPONENT_ID3)
    {
        ret = -1;
        printk(" APB read DSP_COMPONENT_ID3 fail\n");
    }
#define DSP_XIU_APB_PMG         0X1000
#define DSP_XIU_APB_PMG_EN      BITS(0 : 0, BIT(0))
#define DSP_XIU_APB_PMG_EN_MASK BMASK(0 : 0)
#define DSP_XIU_APB_PMINTPC     0X1010
#define DSP_XIU_APB_PM0         0X1080
    Mdrv_DSP_APBWrite(eCoreID, DSP_XIU_APB_PMG, DSP_XIU_APB_PMG_EN, DSP_XIU_APB_PMG_EN_MASK);
    Mdrv_DSP_APBRead(eCoreID, DSP_XIU_APB_PM0, &u32Cnt, 0XFFFFFFFF);
    if (u32Cnt == 0)
    {
        ret = -1;
        printk(" APB write fail\n");
    }
    return ret;
}

int dsp_ut_cmd_IRQ(MDRV_DSP_CoreID_e eCoreID)
{
    int          ret      = 0;
    MS_U32       irqCnt   = 0;
    dsp_ut_core *dspUtCtx = &dspCore[eCoreID];
    MS_U8        irqNum   = 1;
    dsp_ut_cmd_StartDSPGpoIrq(eCoreID, 0);
    Mdrv_DSP_SetGPI(eCoreID, DSP_UT_FW_CMD_IRQ_0);

    for (; irqNum <= 5; irqNum++)
    {
        irqCnt = atomic_read(&dspUtCtx->dspIrqCnt);
        Mdrv_DSP_InterruptTrigger(eCoreID, irqNum);
        if (!wait_event_timeout(dspUtCtx->dspWQ, irqCnt < atomic_read(&dspUtCtx->dspIrqCnt), 10))
        {
            printk(" arm irq trigger %d succ\n", irqNum);
        }
        else
        {
            printk(" arm irq trigger %d fail\n", irqNum);
            ret = -2;
            goto out;
        }
    }
    for (irqNum = 1; irqNum < 32; irqNum++)
    {
        dsp_ut_cmd_StartDSPGpoIrq(eCoreID, irqNum);
        irqCnt = atomic_read(&dspUtCtx->dspIrqCnt);
        Mdrv_DSP_SetGPI(eCoreID, DSP_UT_FW_CMD_IRQ_1);
        Mdrv_DSP_InterruptTrigger(eCoreID, 5);
        if (!wait_event_timeout(dspUtCtx->dspWQ, irqCnt < atomic_read(&dspUtCtx->dspIrqCnt), 10))
        {
            MDRV_DSP_IRQStat_t stIRQStat = {0};
            Mdrv_DSP_GetIRQStat(eCoreID, &stIRQStat, E_MDRV_DSP_IRQ_STAT_BEFOREMSK | E_MDRV_DSP_IRQ_STAT_FORCEVALID,
                                irqNum);

            if (stIRQStat.bStatBeforeMsk || stIRQStat.bForceValid)
            {
                printk(" dsp irq trigger %d succ\n", irqNum);
            }
            else
            {
                printk(" dsp irq trigger %d timeout\n", irqNum);
                ret = -1;
                goto out;
            }
        }
        else
        {
            printk(" dsp irq trigger %d fail\n", irqNum);
            ret = -2;
            goto out;
        }
    }
out:
    return ret;
}
int dsp_ut_ioctl_open(struct inode *inode, struct file *filp)
{
    // dev.data = 0;
    printk(KERN_EMERG "dsp_ut_ioctl_open success!\n");
    return 0;
}

int dsp_ut_ioctl_release(struct inode *inode, struct file *filp)
{
    printk(KERN_EMERG "dsp_ut_ioctl_release success!\n");
    return 0;
}

static const struct file_operations dsp_ut_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = dsp_ut_ioctl,
    .open           = dsp_ut_ioctl_open,
    .release        = dsp_ut_ioctl_release,
};
char *testdu[4] = {"dsp_ut_ioctl0", "dsp_ut_ioctl1", "dsp_ut_ioctl2", "dsp_ut_ioctl3"};
int   dsp_ut_init(void)
{
    int   i = 0;
    int   result;
    dev_t devno = 0;

    dsp_ut_major = MAJOR(devno);
    for (; i < 4; i++)
    {
        result = alloc_chrdev_region(&devno, i, 4, testdu[i]);
        if (result < 0)
            return result;
        dsp_ut_class[i] = class_create(THIS_MODULE, testdu[i]);
        device_create(dsp_ut_class[i], NULL, devno, NULL,
                      "dsp_ut_ioctl"
                      "%d",
                      i);

        cdev_init(&cdev[i], &dsp_ut_fops);
        cdev[i].owner = THIS_MODULE;
        cdev_add(&cdev[i], devno, 1);
        printk(KERN_EMERG "dsp_ut_init success!\n");
    }
    return 0;
}
void dsp_ut_exit(void)
{
    int i = 0;
    for (; i < 4; i++)
    {
        cdev_del(&cdev[i]);
        device_destroy(dsp_ut_class[i], MKDEV(dsp_ut_major, 0));
        class_destroy(dsp_ut_class[i]);
    }
    unregister_chrdev_region(MKDEV(dsp_ut_major, 0), 1);
    printk(KERN_EMERG "dsp_ut_exit success!\n");
}

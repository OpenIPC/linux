/*
 * mdrv_rsa.c- Sigmastar
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <../drivers/sstar/include/ms_msys.h>
#include <../drivers/sstar/include/ms_platform.h>
#include <../drivers/sstar/include/mdrv_hvsp_io_st.h>
#include <../drivers/sstar/include/mdrv_hvsp_io.h>
#include <linux/uaccess.h>
#include "mdrv_rsa.h"
#include "halAESDMA.h"
#include <linux/sysctl.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#define RSA_DEBUG_FLAG (0)
#if (RSA_DEBUG_FLAG == 1)
#define RSA_DBG(fmt, arg...) printk(fmt, ##arg) // KERN_DEBUG KERN_ALERT KERN_WARNING
#else
#define RSA_DBG(fmt, arg...)
#endif

#ifndef DISABLE_RSA_DEVICE
#define DRIVER_NAME "rsa"
#endif // DISABLE_RSA_DEVICE

#ifdef SUPPORT_RSA4096

#ifndef DISABLE_RSA_DEVICE
#define RSA_KEY_LEN     (512)
#define RSA_OUTPUT_SIZE (4096 / 8)
#define SIGKEY_PAGE     (64 * 1024)
#endif // DISABLE_RSA_DEVICE

void rsa_crypt_reset(u8 u8lentgh, u32 operation)
{
    HAL_RSA_Reset(u8lentgh, operation);
}

void rsa_crypt_loadsigkey_start(void)
{
    HAL_RSA_LoadSignInverse_start();
}

void rsa_crypt_loadsigkey(unsigned int *pu32RSA_Sig, u32 u32RSA_SigLen, u32 position)
{
    HAL_RSA_LoadSignInverse((u32 *)(pu32RSA_Sig), u32RSA_SigLen, position);
}

void rsa_crypt_loadsigkey_end(void) {}

u32 rsa_crypt_get_info(struct rsa_config *op)
{
    u32 operation = 0;

    if (op->u32RSA_KeyNLen == 0x40)
    {
        operation = PKA_RSA_OPERAND_2048; // Start CLUE operation for 2048-bits
    }
    else if (op->u32RSA_KeyNLen == 0x80)
    {
        operation = PKA_RSA_OPERAND_4096; // Start CLUE operation for 4096-bits
    }
    else if (op->u32RSA_KeyNLen == 0x20)
    {
        operation = PKA_RSA_OPERAND_1024; // Start CLUE operation for 1024-bits
    }
    else if (op->u32RSA_KeyNLen == 0x10)
    {
        operation = PKA_RSA_OPERAND_512; // Start CLUE operation for 512-bits
    }
    else
    {
        printk("[%s][%d]PKA_RSA_ERROR(%x) \n", __FUNCTION__, __LINE__, op->u32RSA_KeyNLen);
        operation = PKA_RSA_OPERAND_NULL;
        return 0;
    }

    return operation;
}

void rsa_crypt_loadNEkey(struct rsa_config *op)
{
    u32 operation;

    operation = rsa_crypt_get_info(op);

    HAL_RSA_LoadKeyNInverse((u32 *)op->pu32RSA_KeyN, (u8)op->u32RSA_KeyNLen);
    HAL_RSA_Init_Status(ELP_CLUE_ENTRY_CALC_R_INV, operation);
    HAL_RSA_WaitDone(FALSE);

    HAL_RSA_Init_Status(ELP_CLUE_ENTRY_CALC_MP, operation);
    HAL_RSA_WaitDone(TRUE);

    HAL_RSA_Init_Status(ELP_CLUE_ENTRY_CALC_R_SQR, operation);
    HAL_RSA_WaitDone(FALSE);

    HAL_RSA_LoadKeyEInverse((u32 *)op->pu32RSA_KeyE, (u8)op->u32RSA_KeyELen, operation);
}

int rsa_crypt_run(struct rsa_config *op)
{
    u32 operation, out_size;
    u32 i;

    operation = rsa_crypt_get_info(op);
    out_size  = op->u32RSA_KeyNLen;

    HAL_RSA_Init_Status(ELP_CLUE_ENTRY_MODEXP, operation);
    HAL_RSA_WaitDone(FALSE);

    HAL_RSA_FileOut_Start();
    for (i = 0; i < out_size; i++)
    {
        *(op->pu32RSA_Output + (out_size - 1) - i) = HAL_RSA_FileOut(i);
    }
    HAL_RSA_Reset(op->u32RSA_KeyNLen, operation);
    return 0;
}

int rsa_crypto(struct rsa_config *op)
{
    rsa_crypt_loadNEkey(op);
    rsa_crypt_loadsigkey_start();
    rsa_crypt_loadsigkey((u32 *)op->pu32RSA_Sig, op->u32RSA_SigLen, 0);
    rsa_crypt_run(op);

    return 0;
}

#else

#ifndef DISABLE_RSA_DEVICE
#define RSA_KEY_LEN     (256)
#define RSA_OUTPUT_SIZE (2048 / 8)
#define SIGKEY_PAGE     (64 * 1024)
#endif // DISABLE_RSA_DEVICE

void rsa_crypt_reset(void)
{
    HAL_RSA_Reset();
}

void rsa_crypt_loadNEkey(struct rsa_config *op)
{
    // msleep(1);
    HAL_RSA_SetKeyLength((op->u32RSA_KeyNLen - 1) & 0x7F);
    HAL_RSA_SetKeyType(FALSE, op->u8RSA_pub_ekey);

    HAL_RSA_Ind32Ctrl(1); // write
    HAL_RSA_LoadKeyNInverse((u32 *)op->pu32RSA_KeyN, (u8)op->u32RSA_KeyNLen);
    HAL_RSA_LoadKeyEInverse((u32 *)op->pu32RSA_KeyE, (u8)op->u32RSA_KeyELen);
}

void rsa_crypt_loadsigkey_start(void)
{
    HAL_RSA_LoadSignInverse_start();
}

void rsa_crypt_loadsigkey(unsigned int *pu32RSA_Sig, u32 u32RSA_SigLen)
{
    HAL_RSA_LoadSignInverse((u32 *)(pu32RSA_Sig), u32RSA_SigLen);
}

void rsa_crypt_loadsigkey_end(void)
{
    HAL_RSA_LoadSignInverse_end();
}

int rsa_crypt_run(struct rsa_config *op)
{
    int           out_size = 32, i = 0;
    unsigned long timeout;

    HAL_RSA_ExponetialStart();
#if (RSA_DEBUG_FLAG)
    RSA_DBG("IN:\n");
    for (i = 0; i < op->u32RSA_SigLen; i += 4)
    {
        RSA_DBG(" x%08X x%08X x%08X x%08X\n", *(op->pu32RSA_Sig + i), *(op->pu32RSA_Sig + i + 1),
                *(op->pu32RSA_Sig + i + 2), *(op->pu32RSA_Sig + i + 3));
    }
    RSA_DBG("\n\n");
#endif

    timeout = jiffies + msecs_to_jiffies(30000);
    while ((HAL_RSA_GetStatus() & RSA_STATUS_RSA_DONE) != RSA_STATUS_RSA_DONE)
    {
        if (time_after_eq(jiffies, timeout))
        {
            printk("rsa timeout!!!\n");
            goto err;
        }
    }

    out_size = op->u32RSA_KeyNLen;

    HAL_RSA_Ind32Ctrl(0); // read
    for (i = 0; i < out_size; i++)
    {
        HAL_RSA_SetFileOutAddr(i);
        HAL_RSA_FileOutStart();
        *(op->pu32RSA_Output + (out_size - 1) - i) = HAL_RSA_FileOut();
    }

#if (RSA_DEBUG_FLAG)
    RSA_DBG("OUT:\n");
    for (i = 0; i < out_size; i += 4)
    {
        RSA_DBG(" x%08X x%08X x%08X x%08X\n", *(op->pu32RSA_Output + i), *(op->pu32RSA_Output + i + 1),
                *(op->pu32RSA_Output + i + 2), *(op->pu32RSA_Output + i + 3));
    }
    RSA_DBG("\n\n");
#endif

    HAL_RSA_FileOutEnd();
err:
    HAL_RSA_ClearInt();
    HAL_RSA_Reset();

    return 0;
}

int rsa_crypto(struct rsa_config *op)
{
    int ret;
    rsa_crypt_loadNEkey(op);
    rsa_crypt_loadsigkey_start();
    rsa_crypt_loadsigkey(op->pu32RSA_Sig, op->u32RSA_SigLen);
    rsa_crypt_loadsigkey_end();

    ret = rsa_crypt_run(op);

    return ret;
}
#endif // SUPPORT_RSA4096

#ifndef DISABLE_RSA_DEVICE

static long rsa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    MSYS_DMEM_INFO *  ioctl_dmem = filp->private_data;
    struct rsa_config data;
    int               retval = 0;

    switch (cmd)
    {
        case MDrv_RSA_Reset:
#ifdef SUPPORT_RSA4096
            rsa_crypt_reset(data.u32RSA_KeyNLen, PKA_RSA_OPERAND_4096);
            rsa_crypt_reset(data.u32RSA_KeyNLen, PKA_RSA_OPERAND_2048);
#else
            rsa_crypt_reset();
#endif
            break;
        case MDrv_RSA_Calculate:
        {
            unsigned int   position  = 0;
            unsigned int   sigkeylen = 0;
            unsigned int   writelen  = 0;
            unsigned char *sigkey;
            unsigned char *Nkey;
            unsigned char *Ekey;
            unsigned char *RSAout;
            unsigned int * pu32RSA_Output;
            unsigned int   outputsize;
            memset(&data, 0, sizeof(struct rsa_config));
            if (copy_from_user(&data, (int __user *)arg, sizeof(struct rsa_config)))
            {
                retval = -EFAULT;
                printk("Cannot copy form userspace\n");
                return retval;
            }
            pu32RSA_Output = data.pu32RSA_Output;
            outputsize     = data.u32RSA_KeyNLen * 4;
            sigkeylen      = data.u32RSA_SigLen * 4;

            sigkey = ioctl_dmem->kvirt;
            Nkey   = (sigkey + (SIGKEY_PAGE));
            Ekey   = (Nkey + (data.u32RSA_KeyNLen) * 4);
            RSAout = (Ekey + (data.u32RSA_KeyELen) * 4);

            if (unlikely(copy_from_user(Nkey, data.pu32RSA_KeyN, data.u32RSA_KeyNLen * 4)))
            {
                retval = -EFAULT;
                printk("[%s][%d]Cannot copy form userspace\n", __FUNCTION__, __LINE__);
                return retval;
            }

            if (unlikely(copy_from_user(Ekey, data.pu32RSA_KeyE, data.u32RSA_KeyELen * 4)))
            {
                retval = -EFAULT;
                printk("[%s][%d]Cannot copy form userspace\n", __FUNCTION__, __LINE__);
                return retval;
            }

            data.pu32RSA_KeyN   = (u32 *)Nkey;
            data.pu32RSA_KeyE   = (u32 *)Ekey;
            data.pu32RSA_Output = (u32 *)RSAout;

            if (unlikely((data.u32RSA_KeyELen < 1) || (data.u32RSA_KeyELen > 0xFF)))
            {
                retval = -EFAULT;
                printk("[RSAERR]KeyELen(%x). invalid\n", data.u32RSA_KeyELen);
                return retval;
            }

            if (unlikely(data.u32RSA_KeyNLen != 0x10 && data.u32RSA_KeyNLen != 0x20 && data.u32RSA_KeyNLen != 0x40
                         && data.u32RSA_KeyNLen != 0x80))
            {
                retval = -EFAULT;
                printk("[RSAERR] KenNLen != 0x10 or 0x20 or 0x40\n");
                return retval;
            }

#ifdef SUPPORT_RSA4096
            rsa_crypt_reset(data.u32RSA_KeyNLen, rsa_crypt_get_info(&data));
#else
            rsa_crypt_reset();
#endif

            rsa_crypt_loadNEkey(&data);
            rsa_crypt_loadsigkey_start();

            while (sigkeylen)
            {
                if (sigkeylen <= SIGKEY_PAGE)
                {
                    writelen  = sigkeylen;
                    sigkeylen = 0;
                }
                else
                {
                    sigkeylen -= SIGKEY_PAGE;
                    writelen = SIGKEY_PAGE;
                }
                if (unlikely(copy_from_user(
                        sigkey, ((data.pu32RSA_Sig + data.u32RSA_SigLen) - ((position + writelen) / 4)), writelen)))
                {
                    retval = -EFAULT;
                    printk("[%s][%d]Cannot copy form userspace\n", __FUNCTION__, __LINE__);
                    goto error;
                }

#ifdef SUPPORT_RSA4096
                rsa_crypt_loadsigkey((unsigned int *)sigkey, writelen / 4, position);
#else
                rsa_crypt_loadsigkey((unsigned int *)sigkey, writelen / 4);
#endif
                position += SIGKEY_PAGE;
            }
            rsa_crypt_loadsigkey_end();

            rsa_crypt_run(&data);

            if (unlikely(copy_to_user(pu32RSA_Output, (void *)RSAout, outputsize)))
            {
                retval = -EFAULT;
                printk("[%s][%d]Cannot copy to userspace\n", __FUNCTION__, __LINE__);
                goto error;
            }

            break;
        }

        default:
            retval = -ENOTTY;
    }

    return retval;

error:
#ifdef SUPPORT_RSA4096
    rsa_crypt_reset(data.u32RSA_KeyNLen, rsa_crypt_get_info(&data));
#else
    HAL_RSA_ClearInt();
    rsa_crypt_reset();
#endif
    return retval;
}

static int rsa_ioctl_close(struct inode *inode, struct file *filp)
{
    if (filp->private_data)
    {
        MSYS_DMEM_INFO *ioctl_dmem = filp->private_data;
        if (ioctl_dmem->kvirt != 0 && ioctl_dmem->phys != 0)
            msys_release_dmem(ioctl_dmem);
        kfree(filp->private_data);
    }
    return 0;
}
static int rsa_ioctl_open(struct inode *inode, struct file *filp)
{
    char            name[] = "rsa_ioctl";
    MSYS_DMEM_INFO *dmem;

    dmem = kmalloc(sizeof(MSYS_DMEM_INFO), GFP_KERNEL);
    if (dmem == NULL)
        return -ENOMEM;

    memset(dmem, 0, sizeof(MSYS_DMEM_INFO));

    // rsaoutputbuf + sigkeypage + keyNsize +keyEsize
    dmem->length = RSA_OUTPUT_SIZE + SIGKEY_PAGE + (RSA_KEY_LEN)*2;

    memcpy(dmem->name, name, strlen(name) + 1);
    if (0 != msys_request_dmem(dmem))
    {
        printk("[%s][%d]Cannot alloc x(%x) space \n", __FUNCTION__, __LINE__, dmem->length);
        return -ENOMEM;
    }

    filp->private_data = dmem;
    return 0;
}

struct file_operations rsa_fops = {
    .owner          = THIS_MODULE,
    .open           = rsa_ioctl_open,
    .release        = rsa_ioctl_close,
    .unlocked_ioctl = rsa_ioctl,
};

struct miscdevice rsadev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "rsa",
    .fops  = &rsa_fops,
    .mode  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("This is test_ioctl module.");
#endif // DISABLE_RSA_DEVICE

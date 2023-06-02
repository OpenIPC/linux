/*
 *#############################################################################
 *
 * Copyright (c) 2006-2011 MStar Semiconductor, Inc.
 * All rights reserved.
 *
 * Unless otherwise stipulated in writing, any and all information contained
 * herein regardless in any format shall remain the sole proprietary of
 * MStar Semiconductor Inc. and be kept in strict confidence
 * (!¡±MStar Confidential Information!¡L) by the recipient.
 * Any unauthorized act including without limitation unauthorized disclosure,
 * copying, use, reproduction, sale, distribution, modification, disassembling,
 * reverse engineering and compiling of the contents of MStar Confidential
 * Information is unlawful and strictly prohibited. MStar hereby reserves the
 * rights to any and all damages, losses, costs and expenses resulting therefrom.
 *
 *#############################################################################
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <../drivers/mstar/include/ms_msys.h>
#include <../drivers/mstar/include/ms_platform.h>
#include <../drivers/mstar/include/mdrv_hvsp_io_st.h>
#include <../drivers/mstar/include/mdrv_hvsp_io.h>
#include <asm/uaccess.h>
#include "mdrv_rsa.h"
#include "halAESDMA.h"
#include <linux/sysctl.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#define DRIVER_NAME "rsa"
//static unsigned int test_ioctl_major = 0;
//static unsigned int num_of_dev = 1;
//static struct cdev test_ioctl_cdev;
//static int ioctl_num = 0;


//static const char* DMEM_RSA_ENG_INPUT="RSA_ENG_IN";

struct rsaConfig{
	U32 *pu32RSA_Sig;
	U32 *pu32RSA_KeyN;
	U32 *pu32RSA_KeyE;
	U32 *pu32RSA_Output;
	U8 u8RSA_KeyNLen;
	U8 u8RSA_SigLen;
	U8 u8RSA_KeyELen;
	U8 u8RSAPublicKey;
};

static long rsa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct rsaConfig *ioctl_data = filp->private_data;
    int retval = 0;
    struct rsa_config data;
    int nOutSize = 0;
	int i = 0;
	bool bRSAHwKey = FALSE;

//	unsigned char* val;
    memset(&data, 0, sizeof(data));

    switch (cmd) {

         case MDrv_RSA_Reset:
            HAL_RSA_Reset();
			goto done;
         break;

         case MDrv_RSA_Setmode:
			memset(&data, 0, sizeof(data));
            if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
                retval = -EFAULT;
                goto done;
            }
//            write_lock(&ioctl_data->lock);
            ioctl_data->u8RSA_KeyNLen = data.u32RSA_KeyNLen;
//            write_unlock(&ioctl_data->lock);
			if (ioctl_data->u8RSA_KeyNLen != 0x10 && ioctl_data->u8RSA_KeyNLen != 0x20 && ioctl_data->u8RSA_KeyNLen != 0x40){
				retval = -EFAULT;
				printk("[RSAERR] Mode error\n");
				printk("[RSAERR] KenNLen != 0x10 or 0x20 or 0x40\n");
                goto done;
			}

            HAL_RSA_SetKeyLength(((ioctl_data->u8RSA_KeyNLen)-1) & 0x3F);
            HAL_RSA_SetKeyType(bRSAHwKey, FALSE);
         break;

         case MDrv_RSA_Calculate:
			memset(&data, 0, sizeof(data));
             if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
                retval = -EFAULT;
                goto done;
             }
//             write_lock(&ioctl_data->lock);
             ioctl_data->pu32RSA_Sig = (U32 *)data.pu32RSA_Sig;
             ioctl_data->pu32RSA_Output = (U32 *)data.pu32RSA_Output;
             ioctl_data->pu32RSA_KeyN = (U32 *)data.pu32RSA_KeyN;
             ioctl_data->pu32RSA_KeyE = (U32 *)data.pu32RSA_KeyE;
             ioctl_data->u8RSA_KeyELen = (U8)(data.u32RSA_KeyELen & 0xFF);
             ioctl_data->u8RSA_KeyNLen = (U8)(data.u32RSA_KeyNLen & 0xFF);
             ioctl_data->u8RSA_SigLen = (U8)(data.u32RSA_SigLen & 0xFF);
//             write_unlock(&ioctl_data->lock);

			if ((ioctl_data->u8RSA_SigLen < 1) || (ioctl_data->u8RSA_SigLen > 0xFF)) {
				 retval = -EFAULT;
				 printk("[RSAERR] SigLen is invalid\n");
                goto done;
			}

			if ((ioctl_data->u8RSA_KeyELen < 1) || (ioctl_data->u8RSA_KeyELen > 0xFF))
			{
				retval = -EFAULT;
				printk("[RSAERR] KeyELen is invalid\n");
                goto done;
			}

			if ((ioctl_data->u8RSA_KeyNLen < 1) || (ioctl_data->u8RSA_KeyNLen > 0xFF))
			{
				retval = -EFAULT;
				printk("[RSAERR] KeyNLen is invalid\n");
                goto done;
			}

			if (!((ioctl_data->pu32RSA_Sig) && (ioctl_data->pu32RSA_Output) && (ioctl_data->pu32RSA_KeyN))) {
				  retval = -EFAULT;
				  printk("[RSAERR] null pointer = 0 \n");
				 goto done;
			}

             HAL_RSA_Ind32Ctrl(1);//1:write

			 if((!bRSAHwKey) && (ioctl_data->pu32RSA_Sig))
			 {
				HAL_RSA_LoadSignInverse(ioctl_data->pu32RSA_Sig,  ioctl_data->u8RSA_SigLen);
			 }
			 else
			 {
				retval = -EFAULT;
				printk("[RSAERR] RSA_Sig = NULL\n");
			 	goto done;
			 }

             if((!bRSAHwKey) && (ioctl_data->pu32RSA_KeyN))
             {

				HAL_RSA_LoadKeyNInverse(ioctl_data->pu32RSA_KeyN,  ioctl_data->u8RSA_KeyNLen);
             }
			 else
			 {
				retval = -EFAULT;
				printk("[RSAERR] RSA_KeyN = NULL\n");
			 	goto done;
			 }

             if((!bRSAHwKey) && (ioctl_data->pu32RSA_KeyE))
             {
                HAL_RSA_LoadKeyEInverse(ioctl_data->pu32RSA_KeyE, ioctl_data->u8RSA_KeyELen); //65535

			 }
			 else
			 {
				retval = -EFAULT;
				printk("[RSAERR] RSA_KeyE = NULL\n");
			 	goto done;
			 }

             HAL_RSA_ExponetialStart();

             while((HAL_RSA_GetStatus() & RSA_STATUS_RSA_DONE) != RSA_STATUS_RSA_DONE);

             if((bRSAHwKey) || (ioctl_data->u8RSA_KeyNLen == 64))
             {
                nOutSize = 64;
             }
             else
             {
                nOutSize = 32;
             }

             HAL_RSA_Ind32Ctrl(0);

             for( i = 0; i<nOutSize; i++)
             {

                HAL_RSA_SetFileOutAddr(i);
                HAL_RSA_FileOutStart();
               *(ioctl_data->pu32RSA_Output + (nOutSize-1) - i) = HAL_RSA_FileOut();

             }

            HAL_RSA_FileOutEnd();
            HAL_RSA_Reset();

//            read_lock(&ioctl_data->lock);

             data.pu32RSA_Output = (unsigned int*)ioctl_data->pu32RSA_Output;

//            read_unlock(&ioctl_data->lock);

            if (copy_to_user((int __user *)arg, &data, sizeof(data)) ) {
                                retval = -EFAULT;
            	goto done;
			}
            break;

        default:
         retval = -ENOTTY;

 }

    done:

    return retval;
}

//ssize_t test_ioctl_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
//{
//    struct rsaConfig *ioctl_data = filp->private_data;
//    unsigned char val;
//    int retval;
//    int i = 0;

//    read_lock(&ioctl_data->lock);
//    val = ioctl_data->val;
//    read_unlock(&ioctl_data->lock);

//    for (;i < count ;i++) {
//        if (copy_to_user(&buf[i], &val, 1)) {
//        retval = -EFAULT;
//    goto out;
//     }
// }

// retval = count;

//out:
// return retval;
//}

static int rsa_ioctl_close(struct inode *inode, struct file *filp)
{
//    printk(KERN_ALERT "%s call.\n", __func__);
        if (filp->private_data) {
            kfree(filp->private_data);
            filp->private_data = NULL;
        }

 return 0;
}

static int rsa_ioctl_open(struct inode *inode, struct file *filp)
{

    struct rsaConfig *ioctl_data;
//    printk(KERN_ALERT "%s call.\n", __func__);

    ioctl_data = kmalloc(sizeof(struct rsaConfig), GFP_KERNEL);
    if (ioctl_data == NULL)
    return -ENOMEM;

//    rwlock_init(&ioctl_data->lock);


    filp->private_data = ioctl_data;

 return 0;
}

struct file_operations rsa_fops = {
    .owner = THIS_MODULE,
    .open = rsa_ioctl_open,
    .release = rsa_ioctl_close,
    .unlocked_ioctl = rsa_ioctl,
};

struct miscdevice rsadev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rsa",
	.fops = &rsa_fops,
	.mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
};

//#define PFX "rsadev: "

//static int  __init rsa_ioctl_init(void)
//{

//int rc;
//	rc = misc_register(&rsadev);
//	if (unlikely(rc)) {
//		pr_err(PFX "registration of /dev/crypto failed\n");
//		return rc;
//	}

//	return 0;
//}

//static void __exit rsa_ioctl_exit(void)
//{
//    misc_deregister(&rsadev);
//}

//module_init(rsa_ioctl_init);
//module_exit(rsa_ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mstar");
MODULE_DESCRIPTION("This is test_ioctl module.");

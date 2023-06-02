////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#include <linux/pfn.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "ms_platform.h"

#include "ms_msys.h"

#include "mdrv_vip_io_st.h"
#include "mdrv_vip_io.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_vip.h"
#include "mdrv_verchk.h"

#define MDRV_MS_VIP_DEVICE_COUNT    1
#define MDRV_MS_VIP_NAME            "mvip"
#define MAX_FILE_HANDLE_SUPPRT      64
#define MDRV_NAME_VIP               "mvip"
#define MDRV_MAJOR_VIP              0xea
#define MDRV_MINOR_VIP              0x07
#define _ms_vip_mem_bus_to_miu(x) (x-0x20000000)
#define CMD_PARSING(x)  ( x == IOCTL_VIP_SET_DNR_CONFIG                           ?    "IOCTL_VIP_SET_DNR_CONFIG"                           : \
                          x == IOCTL_VIP_SET_PEAKING_CONFIG                       ?    "IOCTL_VIP_SET_PEAKING_CONFIG"                               : \
                          x == IOCTL_VIP_SET_DLC_HISTOGRAM_CONFIG                 ?    "IOCTL_VIP_SET_DLC_HISTOGRAM_CONFIG"                 : \
                          x == IOCTL_VIP_GET_DLC_HISTOGRAM_REPORT                 ?    "IOCTL_VIP_GET_DLC_HISTOGRAM_REPORT"                 : \
                          x == IOCTL_VIP_SET_DLC_CONFIG                           ?    "IOCTL_VIP_SET_DLC_CONFIG"                           : \
                          x == IOCTL_VIP_SET_LCE_CONFIG                           ?    "IOCTL_VIP_SET_LCE_CONFIG"                           : \
                          x == IOCTL_VIP_SET_UVC_CONFIG                           ?    "IOCTL_VIP_SET_UVC_CONFIG"                           : \
                          x == IOCTL_VIP_SET_IHC_CONFIG                           ?    "IOCTL_VIP_SET_IHC_CONFIG"                           : \
                          x == IOCTL_VIP_SET_ICE_CONFIG                           ?    "IOCTL_VIP_SET_ICE_CONFIG"                           : \
                          x == IOCTL_VIP_SET_IHC_ICE_ADP_Y_CONFIG                 ?    "IOCTL_VIP_SET_IHC_ICE_ADP_Y_CONFIG"                 : \
                          x == IOCTL_VIP_SET_IBC_CONFIG                           ?    "IOCTL_VIP_SET_IBC_CONFIG"                           : \
                          x == IOCTL_VIP_SET_FCC_CONFIG                           ?    "IOCTL_VIP_SET_FCC_CONFIG"                           : \
                          x == IOCTL_VIP_CMDQ_WRITE_CONFIG                        ?    "IOCTL_VIP_CMDQ_WRITE_CONFIG"                        :\
                          x == IOCTL_VIP_SET_LDC_CONFIG                           ?    "IOCTL_VIP_SET_LDC_OnOff_CONFIG"                     : \
                          x == IOCTL_VIP_SET_LDC_MD_CONFIG                        ?    "IOCTL_VIP_SET_LDC_MD_CONFIG"                        : \
                          x == IOCTL_VIP_SET_LDC_DMAP_CONFIG                      ?    "IOCTL_VIP_SET_LDC_DMAP_CONFIG"                      : \
                          x == IOCTL_VIP_SET_LDC_SRAM_CONFIG                      ?    "IOCTL_VIP_SET_LDC_SRAM_CONFIG"                      : \
                          x == IOCTL_VIP_SET_ACK_CONFIG                           ?    "IOCTL_VIP_SET_ACK_CONFIG"                           : \
                          x == IOCTL_VIP_SET_NLM_CONFIG                           ?    "IOCTL_VIP_SET_NLM_CONFIG"                           : \
                          x == IOCTL_VIP_SET_SNR_CONFIG                           ?    "IOCTL_VIP_SET_SNR_CONFIG"                           : \
                          x == IOCTL_VIP_SET_VIP_CONFIG                           ?    "IOCTL_VIP_SET_VIP_CONFIG"                           : \
                          x == IOCTL_VIP_SET_VTRACK_CONFIG                        ?    "IOCTL_VIP_SET_VTRACK_CONFIG"                        : \
                          x == IOCTL_VIP_SET_VTRACK_ONOFF_CONFIG                  ?    "IOCTL_VIP_SET_VTRACK_ONOFF_CONFIG"                  : \
                                                                                       "UNKNOWN")

int mdrv_ms_vip_open(struct inode *inode, struct file *filp);
int mdrv_ms_vip_release(struct inode *inode, struct file *filp);
long mdrv_ms_vip_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
static int mdrv_ms_vip_probe(struct platform_device *pdev);
static int mdrv_ms_vip_remove(struct platform_device *pdev);
static int mdrv_ms_vip_suspend(struct platform_device *dev, pm_message_t state);
static int mdrv_ms_vip_resume(struct platform_device *dev);
static unsigned int mdrv_ms_vip_poll(struct file *filp, struct poll_table_struct *wait);

//extern atomic_t SC1W_intr_count;

//-------------------------------------------------------------------------------------------------

typedef struct
{
    int s32Major;
    int s32Minor;
    int refCnt;
    struct cdev cdev;
    struct file_operations fops;
	struct device *devicenode;
}ST_DEV_VIP;

static ST_DEV_VIP _dev_ms_vip =
{
    .s32Major = MDRV_MAJOR_VIP,
    .s32Minor = MDRV_MINOR_VIP,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= MDRV_NAME_VIP, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = mdrv_ms_vip_open,
        .release = mdrv_ms_vip_release,
        .unlocked_ioctl = mdrv_ms_vip_ioctl,
        .poll = mdrv_ms_vip_poll,
    }
};

static struct class * m_vip_class = NULL;
static char * vip_classname = "m_vip_class";


static const struct of_device_id ms_vip_of_match_table[] =
{
    { .compatible = "mstar,vip" },
    {}
};

static struct platform_driver st_ms_vip_driver =
{
	.probe 		= mdrv_ms_vip_probe,
	.remove 	= mdrv_ms_vip_remove,
    .suspend    = mdrv_ms_vip_suspend,
    .resume     = mdrv_ms_vip_resume,
	.driver =
	{
		.name	= MDRV_NAME_VIP,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(ms_vip_of_match_table),
	},
};
static u64 ms_vip_dma_mask = 0xffffffffUL;
#if (!CONFIG_OF)

static struct platform_device st_ms_vip_device =
{
    .name = "mvip",
    .id = 0,
    .dev =
    {
        .dma_mask = &ms_vip_dma_mask,
        .coherent_dma_mask = 0xffffffffUL
    }
};
#endif
//-------------------------------------------------------------------------------------------------
static const char* KEY_DMEM_VIP_CMDQ="VIP_CMDQ";
static unsigned char g_bVIPSysInitReady = 0;
dma_addr_t  sg_vip_cmdq_bus_addr;
static u_long sg_vip_cmdq_size = VIP_CMDQ_MEM_16K;//128kb  1F400
static void *sg_vip_cmdq_vir_addr = NULL;
ST_VIP_CMDQ_INIT_CONFIG gstInitCMDQCfg;

//-------------------------------------------------------------------------------------------------
// IOCtrl Driver interface functions
//-------------------------------------------------------------------------------------------------
static ssize_t check_bypass_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return MDrv_VIP_ProcShow(buf);
}
static ssize_t check_bypass_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL != buf)
	{
		const char *str = buf;
        if((int)*str == 49)    //input 1  echo 1 >ptgen_call
        {
            SCL_ERR( "[HVSP1]bypass OPEN %d\n",(int)*str);
            MDrv_VIP_SetVIPBypassConfig(0x7FFFF);
        }
        else if((int)*str == 50)  //input 2  echo 2 >ptgen_call
        {
            SCL_ERR( "[HVSP1]bypass CLOSE %d\n",(int)*str);
            MDrv_VIP_SetVIPBypassConfig(EN_VIP_CONFIG);
        }
        else if((int)*str == 51)  //input 3  echo 3 >ptgen_call
        {
            SCL_ERR( "[HVSP1]bypass CLOSE %d\n",(int)*str);
            MDrv_VIP_SetVIPBypassConfig(EN_VIP_DNR_CONFIG);
        }
        else if((int)*str == 52)  //input 4  echo 4 >ptgen_call
        {
            SCL_ERR( "[HVSP1]bypass CLOSE %d\n",(int)*str);
            MDrv_VIP_SetVIPBypassConfig(EN_VIP_SNR_CONFIG);
        }
        else if((int)*str == 53)  //input 5  echo 5 >ptgen_call
        {
            SCL_ERR( "[HVSP1]bypass CLOSE %d\n",(int)*str);
            MDrv_VIP_SetVIPBypassConfig(EN_VIP_NLM_CONFIG);
        }
        else if((int)*str == 54)  //input 6  echo 6 >ptgen_call
        {
            SCL_ERR( "[HVSP1]bypass CLOSE %d\n",(int)*str);
            MDrv_VIP_SetVIPBypassConfig(EN_VIP_LDC_CONFIG);
        }
        else if((int)*str == 48)  //input 0  echo 0 >ptgen_call
        {
            SCL_ERR( "[HVSP1]bypass CLOSE %d\n",(int)*str);
            MDrv_VIP_SetVIPBypassConfig(0);
        }

        return n;
	}

	return 0;
}

static DEVICE_ATTR(bypass,0600, check_bypass_show, check_bypass_store);

static ssize_t check_CMDQ_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
	    str += scnprintf(str, end - str, "echo 1 > ckCMDQ :by already setting\n");
	    str += scnprintf(str, end - str, "echo 2 > ckCMDQ :by auto testing\n");
	return (str - buf);
}

static ssize_t check_CMDQ_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL != buf)
	{
		const char *str = buf;
        if((int)*str == 49)    //input 1  echo 1 >
        {
            SCL_ERR( "[CMDQ]CMDQ check by already setting %d\n",(int)*str);
            MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_ALREADY_SETINNG);
            MDrv_VIP_Resume();
            MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_RETURN_ORI);
            MDrv_VIP_CheckRegister();
        }
        else if((int)*str == 50)  //input 2  echo 2 >
        {
            SCL_ERR( "[CMDQ]CMDQ check by auto testing %d\n",(int)*str);
            MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_AUTOSETTING);
            MDrv_VIP_Resume();
            MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_RETURN_ORI);
            MDrv_VIP_CheckRegister();
        }

        return n;
	}

	return 0;
}

static DEVICE_ATTR(ckCMDQ,0600, check_CMDQ_show, check_CMDQ_store);

static ssize_t check_PQ_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
	    str += scnprintf(str, end - str, "echo 1 > ckPQ :by already setting\n");
	    str += scnprintf(str, end - str, "echo 2 > ckPQ :by auto testing\n");

	return (str - buf);
}

static ssize_t check_PQ_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL != buf)
	{
		const char *str = buf;
        if((int)*str == 49)    //input 1  echo 1 >
        {
            SCL_ERR( "[HVSP1]PQ check by already setting %d\n",(int)*str);
            MDrv_VIP_CheckConsist();   //consistency
            MDrv_VIP_CheckRegister();       //basic check register
        }
        else if((int)*str == 50)  //input 2  echo 2 >
        {
            SCL_ERR( "[HVSP1]PQ check by auto testing %d\n",(int)*str);
            MDrv_VIP_PrepareStructToCheckRegister(EN_VIP_CMDQ_CHECK_PQ);
            MDrv_VIP_Resume();
            MDrv_VIP_CheckRegister();       //basic check register
        }

        return n;
	}

	return 0;
}
static DEVICE_ATTR(ckPQ,0600, check_PQ_show, check_PQ_store);
static ssize_t CMDQ_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return MDrv_VIP_CMDQShow(buf);
}


static DEVICE_ATTR(CMDQ,0400, CMDQ_show, NULL);
static ssize_t VIP_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return MDrv_VIP_VIPShow(buf);
}

static DEVICE_ATTR(VIPStatus,0400, VIP_show, NULL);

void _mdrv_ms_vip_LogConfigStruct(ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg)
{
    unsigned char u8offset;
    unsigned char *pu8value = NULL;
    for(u8offset = 0;u8offset < stSetPQCfg.u32StructSize;u8offset++)
    {
        pu8value = stSetPQCfg.pPointToCfg + u8offset*1;
        SCL_DBG(SCL_DBG_LV_VIP_LOG(), "[VIP] offset%hhd:%hhx \n",u8offset,*pu8value);

    }
}
void _mdrv_ms_vip_LogConfigByIP(EN_VIP_CONFIG_TYPE enVIPtype,unsigned char *pPointToCfg)
{
    ST_MDRV_VIP_SETPQ_CONFIG stSetPQCfg;
    stSetPQCfg = MDrv_VIP_FillBasicStructSetPQCfg(enVIPtype,pPointToCfg);
    _mdrv_ms_vip_LogConfigStruct(stSetPQCfg);
}
ST_MDRV_VIP_VERSIONCHK_CONFIG _mdrv_ms_vip_io_fill_versionchkstruct
(unsigned int u32StructSize,unsigned int u32VersionSize,unsigned int *pVersion)
{
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;
    stVersion.u32StructSize  = (unsigned int)u32StructSize;
    stVersion.u32VersionSize = (unsigned int)u32VersionSize;
    stVersion.pVersion      = (unsigned int *)pVersion;
    return stVersion;
}
int _mdrv_ms_vip_io_version_check(ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion)
{
    if ( CHK_VERCHK_HEADER(stVersion.pVersion) )
    {
        if( CHK_VERCHK_MAJORVERSION_LESS( stVersion.pVersion, IOCTL_VIP_VERSION) )
        {

            VERCHK_ERR("[VIP] Version(%04x) < %04x!!! \n",
                *(stVersion.pVersion) & VERCHK_VERSION_MASK,
                IOCTL_VIP_VERSION);

            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &stVersion.u32VersionSize, stVersion.u32StructSize) == 0 )
            {
                VERCHK_ERR("[VIP] Size(%04x) != %04x!!! \n",
                    stVersion.u32StructSize,
                    stVersion.u32VersionSize);

                return -EINVAL;
            }
            else
            {
                SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_ELSE, "[VIP] Size(%d) \n",stVersion.u32StructSize );
                return VersionCheckSuccess;
            }
        }
    }
    else
    {
        VERCHK_ERR("[VIP] No Header !!! \n");
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EFAULT;
    }
}
int _mdrv_ms_vip_io_set_dnr_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_DNR_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_DNR_CONFIG),
        (((ST_IOCTL_VIP_DNR_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_DNR_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_DNR_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_DNR_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_DNR_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetDNRConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_peaking_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_PEAKING_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_PEAKING_CONFIG),
        (((ST_IOCTL_VIP_PEAKING_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_PEAKING_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_PEAKING_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_PEAKING_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_PEAKING_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetPeakingConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_dlc_histogram_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG),
        (((ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_DLC_HISTOGRAM_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_DLC_HISTOGRAM_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetHistogramConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_get_dlc_histogram_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_DLC_HISTOGRAM_REPORT stCfg;

    if(!MDrv_VIP_GetDLCHistogramReport((void *)&stCfg))
    {
        return -EFAULT;
    }

    if (copy_to_user( (ST_IOCTL_VIP_DLC_HISTOGRAM_REPORT __user *)arg, &stCfg, sizeof(ST_IOCTL_VIP_DLC_HISTOGRAM_REPORT) ))
    {
       return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_dlc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_DLC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_DLC_CONFIG),
        (((ST_IOCTL_VIP_DLC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_DLC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_DLC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_DLC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_DLC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetDLCConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}


int _mdrv_ms_vip_io_set_lce_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_LCE_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_LCE_CONFIG),
        (((ST_IOCTL_VIP_LCE_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_LCE_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_LCE_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_LCE_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_LCE_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetLCEConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_uvc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_UVC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_UVC_CONFIG),
        (((ST_IOCTL_VIP_UVC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_UVC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_UVC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_UVC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_UVC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetUVCConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}


int _mdrv_ms_vip_io_set_ihc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_IHC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_IHC_CONFIG),
        (((ST_IOCTL_VIP_IHC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_IHC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_IHC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_IHC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_IHC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetIHCConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_icc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_ICC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_ICC_CONFIG),
        (((ST_IOCTL_VIP_ICC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_ICC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_ICC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_ICC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_ICC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetICEConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_ihc_ice_adp_y_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_IHCICC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_IHCICC_CONFIG),
        (((ST_IOCTL_VIP_IHCICC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_IHCICC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_IHCICC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_IHCICC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_IHCICC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetIHCICCADPYConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_ibc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_IBC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_IBC_CONFIG),
        (((ST_IOCTL_VIP_IBC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_IBC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_IBC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_IBC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_IBC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetIBCConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_fcc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_FCC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_FCC_CONFIG),
        (((ST_IOCTL_VIP_FCC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_FCC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_FCC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_FCC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_FCC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetFCCConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}


int _mdrv_ms_vip_io_set_nlm_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_NLM_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_NLM_CONFIG),
        (((ST_IOCTL_VIP_NLM_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_NLM_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_NLM_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_NLM_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_NLM_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetNLMConfig((void *)&stCfg))
    {
        return -EFAULT;
    }
    if(stCfg.stSRAM.bEn)
    {
        MDrv_VIP_SetNLMSRAMConfig(stCfg.stSRAM);
    }
    else if(!stCfg.stSRAM.bEn && stCfg.stSRAM.u32viradr)
    {
        MDrv_VIP_SetNLMSRAMConfig(stCfg.stSRAM);
    }

    return 0;
}

int _mdrv_ms_vip_io_set_ack_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_ACK_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_ACK_CONFIG),
        (((ST_IOCTL_VIP_ACK_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_ACK_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_ACK_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_ACK_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_ACK_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetACKConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}


int _mdrv_ms_vip_io_set_snr_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_SNR_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_SNR_CONFIG),
        (((ST_IOCTL_VIP_SNR_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_SNR_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_SNR_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_SNR_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_SNR_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetSNRConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_cmdq_write_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_CMDQ_CONFIG stCfg;

    if(copy_from_user(&stCfg, (ST_IOCTL_VIP_CMDQ_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_CMDQ_CONFIG)))
    {
        return -EFAULT;
    }

    if(!MDrv_VIP_CMDQWriteConfig(&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_ldc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_LDC_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_LDC_CONFIG),
        (((ST_IOCTL_VIP_LDC_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_LDC_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_LDC_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_LDC_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_LDC_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetLDCConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_vip_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_CONFIG),
        (((ST_IOCTL_VIP_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_SetVIPOtherConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_ldc_md_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_LDC_MD_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_LDC_MD_CONFIG),
        (((ST_IOCTL_VIP_LDC_MD_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_LDC_MD_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_LDC_MD_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_LDC_MD_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_LDC_MD_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetLDCmdConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_ldc_dmap_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_LDC_DMAP_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_LDC_DMAP_CONFIG),
        (((ST_IOCTL_VIP_LDC_DMAP_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_LDC_DMAP_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_LDC_DMAP_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_LDC_DMAP_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_LDC_DMAP_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetLDCDmapConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_ldc_sram_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_LDC_SRAM_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_LDC_SRAM_CONFIG),
        (((ST_IOCTL_VIP_LDC_SRAM_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_LDC_SRAM_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_LDC_SRAM_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_LDC_SRAM_CONFIG)))
        {
            return -EFAULT;
        }
    }

    _mdrv_ms_vip_LogConfigByIP(EN_VIP_LDC_SRAM_CONFIG,(unsigned char *)&stCfg);
    if(!MDrv_VIP_SetLDCSRAMConfig((void *)&stCfg))
    {
        return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_vip_io_set_vtrack_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_VTRACK_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_VTRACK_CONFIG),
        (((ST_IOCTL_VIP_VTRACK_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_VTRACK_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_VTRACK_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_VTRACK_CONFIG)))
        {
            return -EFAULT;
        }
    }

    if(stCfg.bSetKey)
    {
        MDrv_VIP_VtrackSetPayloadData(stCfg.u16Timecode,stCfg.u8OperatorID);
        MDrv_VIP_VtrackSetKey(stCfg.bSetKey,stCfg.u8SetKey);
    }
    else
    {
        MDrv_VIP_VtrackSetPayloadData(stCfg.u16Timecode,stCfg.u8OperatorID);
        MDrv_VIP_VtrackSetKey(0,NULL);
    }
    if(stCfg.bSetUserDef)
    {
        MDrv_VIP_VtrackSetUserDefindedSetting(stCfg.bSetUserDef,stCfg.u8SetUserDef);
    }
    else
    {
        MDrv_VIP_VtrackSetUserDefindedSetting(0,NULL);
    }
    return 0;
}

int _mdrv_ms_vip_io_set_vtrack_onoff_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_VTRACK_ONOFF_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_VTRACK_ONOFF_CONFIG),
        (((ST_IOCTL_VIP_VTRACK_ONOFF_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_VTRACK_ONOFF_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_VTRACK_ONOFF_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_VTRACK_ONOFF_CONFIG)))
        {
            return -EFAULT;
        }
    }

    if(!MDrv_VIP_VtrackEnable(stCfg.u8framerate,stCfg.EnType))
    {
        return -EFAULT;
    }

    return 0;
}


int _mdrv_ms_vip_io_get_version(struct file *filp, unsigned long arg)
{
    int ret = 0;

    if (CHK_VERCHK_HEADER( &(((ST_IOCTL_VIP_VERSION_CONFIG __user *)arg)->VerChk_Version)) )
    {
        if( CHK_VERCHK_MAJORVERSION_LESS( &(((ST_IOCTL_VIP_VERSION_CONFIG __user *)arg)->VerChk_Version), IOCTL_VIP_VERSION) )
        {

            VERCHK_ERR("[VIP] Version(%04x) < %04x!!! \n",
                ((ST_IOCTL_VIP_VERSION_CONFIG __user *)arg)->VerChk_Version & VERCHK_VERSION_MASK,
                IOCTL_VIP_VERSION);

            ret = -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &(((ST_IOCTL_VIP_VERSION_CONFIG __user *)arg)->VerChk_Size), sizeof(ST_IOCTL_VIP_VERSION_CONFIG)) == 0 )
            {
                VERCHK_ERR("[VIP] Size(%04x) != %04x!!! \n",
                    sizeof(ST_IOCTL_VIP_VERSION_CONFIG),
                    (((ST_IOCTL_VIP_VERSION_CONFIG __user *)arg)->VerChk_Size));

                ret = -EINVAL;
            }
            else
            {
                ST_IOCTL_VIP_VERSION_CONFIG stCfg;

                stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size, IOCTL_VIP_VERSION);
                stCfg.u32Version = IOCTL_VIP_VERSION;

                if(copy_to_user((ST_IOCTL_VIP_VERSION_CONFIG __user *)arg, &stCfg, sizeof(ST_IOCTL_VIP_VERSION_CONFIG)))
                {
                    ret = -EFAULT;
                }
                else
                {
                    ret = 0;
                }
            }
        }
    }
    else
    {
        VERCHK_ERR("[VIP] No Header !!! \n");
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        ret = -EINVAL;
    }

    return ret;
}

int _mdrv_ms_vip_io_set_allvip(struct file *filp, unsigned long arg)
{
    ST_IOCTL_VIP_AllSET_CONFIG stCfg;
    ST_MDRV_VIP_VERSIONCHK_CONFIG stVersion;
    stVersion = _mdrv_ms_vip_io_fill_versionchkstruct(sizeof(ST_IOCTL_VIP_AllSET_CONFIG),
        (((ST_IOCTL_VIP_AllSET_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_VIP_AllSET_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_vip_io_version_check(stVersion))
    {
        SCL_ERR( "[VIP]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stCfg, (ST_IOCTL_VIP_AllSET_CONFIG __user *)arg, sizeof(ST_IOCTL_VIP_AllSET_CONFIG)))
        {
            return -EFAULT;
        }
    }
    MDrv_VIP_SetAllVIPOneshot(&stCfg.stvipCfg);
    return 0;
}

//----------------------------------------------------------------------------------------------


//==============================================================================
long mdrv_ms_vip_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg)
{
    int err = 0;
    int retval = 0;

    if(_dev_ms_vip.refCnt <= 0)
    {
        SCL_ERR( "[VIP] VIPIO_IOCTL refCnt =%d!!! \n", _dev_ms_vip.refCnt);
        return -EFAULT;
    }
    /* check u32Cmd valid */
    if(IOCTL_VIP_MAGIC == _IOC_TYPE(u32Cmd))
    {
        if(_IOC_NR(u32Cmd) >= IOCTL_VIP_MAX_NR)
        {
            SCL_ERR( "[VIP] IOCtl NR Error!!! (Cmd=%x)\n",u32Cmd);
            return -ENOTTY;
        }
    }
    else
    {
        SCL_ERR( "[VIP] IOCtl MAGIC Error!!! (Cmd=%x)\n",u32Cmd);
        return -ENOTTY;
    }

    /* verify Access */
    if (_IOC_DIR(u32Cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)u32Arg, _IOC_SIZE(u32Cmd));
    }
    else if (_IOC_DIR(u32Cmd) & _IOC_WRITE)
    {
        err =  !access_ok(VERIFY_READ, (void __user *)u32Arg, _IOC_SIZE(u32Cmd));
    }
    if (err)
    {
        return -EFAULT;
    }
	/* not allow query or command once driver suspend */

    SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "[VIP] IOCTL_NUM:: == %s ==  \n", (CMD_PARSING(u32Cmd)));

    switch(u32Cmd)
    {
    case IOCTL_VIP_CMDQ_WRITE_CONFIG:
        retval = _mdrv_ms_vip_io_cmdq_write_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_LDC_CONFIG:
        retval = _mdrv_ms_vip_io_set_ldc_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_LDC_MD_CONFIG:
        retval = _mdrv_ms_vip_io_set_ldc_md_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_LDC_DMAP_CONFIG:
        retval = _mdrv_ms_vip_io_set_ldc_dmap_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_LDC_SRAM_CONFIG:
        retval = _mdrv_ms_vip_io_set_ldc_sram_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_DNR_CONFIG:
        retval = _mdrv_ms_vip_io_set_dnr_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_PEAKING_CONFIG:
        retval = _mdrv_ms_vip_io_set_peaking_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_DLC_HISTOGRAM_CONFIG:
        retval = _mdrv_ms_vip_io_set_dlc_histogram_config(filp, u32Arg);
        break;
    case IOCTL_VIP_GET_DLC_HISTOGRAM_REPORT:
        retval = _mdrv_ms_vip_io_get_dlc_histogram_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_DLC_CONFIG:
        retval = _mdrv_ms_vip_io_set_dlc_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_LCE_CONFIG:
        retval = _mdrv_ms_vip_io_set_lce_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_UVC_CONFIG:
        retval = _mdrv_ms_vip_io_set_uvc_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_IHC_CONFIG:
        retval = _mdrv_ms_vip_io_set_ihc_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_ICE_CONFIG:
        retval = _mdrv_ms_vip_io_set_icc_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_IHC_ICE_ADP_Y_CONFIG:
        retval = _mdrv_ms_vip_io_set_ihc_ice_adp_y_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_IBC_CONFIG:
        retval = _mdrv_ms_vip_io_set_ibc_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_FCC_CONFIG:
        retval = _mdrv_ms_vip_io_set_fcc_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_ACK_CONFIG:
        retval = _mdrv_ms_vip_io_set_ack_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_NLM_CONFIG:
        retval = _mdrv_ms_vip_io_set_nlm_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_SNR_CONFIG:
        retval = _mdrv_ms_vip_io_set_snr_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_VIP_CONFIG:
        retval = _mdrv_ms_vip_io_set_vip_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_VTRACK_CONFIG:
        retval = _mdrv_ms_vip_io_set_vtrack_config(filp, u32Arg);
        break;
    case IOCTL_VIP_SET_VTRACK_ONOFF_CONFIG:
        retval = _mdrv_ms_vip_io_set_vtrack_onoff_config(filp, u32Arg);
        break;
    case IOCTL_VIP_GET_VERSION_CONFIG:
        retval = _mdrv_ms_vip_io_get_version(filp, u32Arg);
        break;
    case IOCLT_VIP_SET_ALLVIP_CONFIG:
        retval = _mdrv_ms_vip_io_set_allvip(filp, u32Arg);
        break;
    default:  /* redundant, as cmd was checked against MAXNR */
        SCL_ERR( "[VIP] ERROR IOCtl number %x\n ",u32Cmd);
        retval = -ENOTTY;
        break;
    }

    return retval;
}


static unsigned int mdrv_ms_vip_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int ret = 0;
    wait_queue_head_t *event_wait_queue = NULL;
    SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "[VIP]start %s ret=%x\n",__FUNCTION__,ret);
    event_wait_queue = MDrv_VIP_GetWaitQueueHead();
    MDrv_VIP_SetPollWait(filp, event_wait_queue, wait);
    if(MDrv_VIP_GetCMDQHWDone())
    {
        ret = POLLIN;
    }
    else
    {
        ret = 0;
    }
    return ret;
}

static void* alloc_dmem(const char* name, unsigned int size, dma_addr_t *addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name,name,strlen(name)+1);
    dmem.length = size;
    if(0 != msys_request_dmem(&dmem)){
        return NULL;
    }
    *addr = dmem.phys;
    return (void *)((uintptr_t)dmem.kvirt);
}

static void free_dmem(const char* name, unsigned int size, void *virt, dma_addr_t addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name,name,strlen(name)+1);
    dmem.length = size;
    dmem.kvirt  = (unsigned long long)((uintptr_t)virt);
    dmem.phys   = (unsigned long long)((uintptr_t)addr);
    msys_release_dmem(&dmem);
}

static int _ms_vip_mem_allocate(void)
{
    if (!(sg_vip_cmdq_vir_addr = alloc_dmem(KEY_DMEM_VIP_CMDQ,
                                             PAGE_ALIGN(sg_vip_cmdq_size),
                                             &sg_vip_cmdq_bus_addr)))
    {
        SCL_ERR( "%s: unable to allocate screen memory\n", __FUNCTION__);
        return 0;
    }
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[CMDQ]: CMDQ: Phy:%x  Vir:%x\n", sg_vip_cmdq_bus_addr, (u32)sg_vip_cmdq_vir_addr);
    gstInitCMDQCfg.u32CMDQ_Phy  = Chip_Phys_to_MIU(sg_vip_cmdq_bus_addr);
    gstInitCMDQCfg.u32CMDQ_Size = sg_vip_cmdq_size;
    gstInitCMDQCfg.u32CMDQ_Vir  = (unsigned long)sg_vip_cmdq_vir_addr;
    return 1;
}

static void _ms_vip_mem_free(void)
{
	free_dmem(KEY_DMEM_VIP_CMDQ,
              PAGE_ALIGN(sg_vip_cmdq_size),
              sg_vip_cmdq_vir_addr,
              sg_vip_cmdq_bus_addr);

}
#if CONFIG_OF
static int mdrv_ms_vip_probe(struct platform_device *pdev)
{
    ST_MDRV_VIP_INIT_CONFIG stVipInitCfg;
    int s32Ret;
    dev_t  dev;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    //mod
    if(_dev_ms_vip.s32Major)
    {
        dev     = MKDEV(_dev_ms_vip.s32Major, _dev_ms_vip.s32Minor);
        s32Ret  = register_chrdev_region(dev, MDRV_MS_VIP_DEVICE_COUNT, MDRV_MS_VIP_NAME);
    }
    else
    {
        s32Ret = alloc_chrdev_region(&dev, _dev_ms_vip.s32Minor, MDRV_MS_VIP_DEVICE_COUNT, MDRV_MS_VIP_NAME);
        _dev_ms_vip.s32Major = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        SCL_ERR( "[VIP] Unable to get major %d\n", _dev_ms_vip.s32Major);
        return s32Ret;
    }

    cdev_init(&_dev_ms_vip.cdev, &_dev_ms_vip.fops);
    if (0 != (s32Ret= cdev_add(&_dev_ms_vip.cdev, dev, MDRV_MS_VIP_DEVICE_COUNT)))
    {
        SCL_ERR( "[VIP] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_MS_VIP_DEVICE_COUNT);
        return s32Ret;
    }

    m_vip_class = msys_get_sysfs_class();
    if(!m_vip_class)
    {
        m_vip_class = class_create(THIS_MODULE, vip_classname);
    }
    if(IS_ERR(m_vip_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        _dev_ms_vip.devicenode =device_create(m_vip_class, NULL, dev,NULL, "mvip");
        _dev_ms_vip.devicenode->dma_mask=&ms_vip_dma_mask;
        _dev_ms_vip.devicenode->coherent_dma_mask=ms_vip_dma_mask;
    }
    //probe
    if( _ms_vip_mem_allocate() == 0 )
    {
        return -EFAULT;
    }
    stVipInitCfg.u32RiuBase             = 0x1F000000; //ToDo
    stVipInitCfg.CMDQCfg.u32CMDQ_Phy    = gstInitCMDQCfg.u32CMDQ_Phy;
    stVipInitCfg.CMDQCfg.u32CMDQ_Size   = gstInitCMDQCfg.u32CMDQ_Size;
    stVipInitCfg.CMDQCfg.u32CMDQ_Vir    = gstInitCMDQCfg.u32CMDQ_Vir;
    if(MDrv_VIP_Init(&stVipInitCfg) == 0)
    {
        return -EFAULT;
    }
    MDrv_VIP_SuspendResetFlagInit();
    if (device_create_file(_dev_ms_vip.devicenode, &dev_attr_bypass)!= 0)
    {
      dev_err(_dev_ms_vip.devicenode,
       "Failed to create bypass sysfs files\n");
    }
    if (device_create_file(_dev_ms_vip.devicenode, &dev_attr_CMDQ)!= 0)
    {
      dev_err(_dev_ms_vip.devicenode,
       "Failed to create bypass sysfs files\n");
    }
    if (device_create_file(_dev_ms_vip.devicenode, &dev_attr_VIPStatus)!= 0)
    {
      dev_err(_dev_ms_vip.devicenode,
       "Failed to create bypass sysfs files\n");
    }
    if (device_create_file(_dev_ms_vip.devicenode, &dev_attr_ckCMDQ)!= 0)
    {
      dev_err(_dev_ms_vip.devicenode,
       "Failed to create dev_attr_ckCMDQ sysfs files\n");
    }
    if (device_create_file(_dev_ms_vip.devicenode, &dev_attr_ckPQ)!= 0)
    {
      dev_err(_dev_ms_vip.devicenode,
       "Failed to create dev_attr_ckPQ sysfs files\n");
    }
     gbProbeAlready |= EN_DBG_VIP_CONFIG;
    return 0;
}

static int mdrv_ms_vip_remove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    MDrv_VIP_Delete();
    _ms_vip_mem_free();
    g_bVIPSysInitReady = 0;
    cdev_del(&_dev_ms_vip.cdev);
    device_destroy(m_vip_class, MKDEV(_dev_ms_vip.s32Major, _dev_ms_vip.s32Minor));
    class_destroy(m_vip_class);
    unregister_chrdev_region(MKDEV(_dev_ms_vip.s32Major, _dev_ms_vip.s32Minor), MDRV_MS_VIP_DEVICE_COUNT);
    return 0;
}

#else
static int mdrv_ms_vip_probe(struct platform_device *pdev)
{
    ST_MDRV_VIP_INIT_CONFIG stVipInitCfg;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    if( _ms_vip_mem_allocate() == 0 )
    {
        return -EFAULT;
    }
    stVipInitCfg.u32RiuBase             = 0x1F000000; //ToDo
    stVipInitCfg.CMDQCfg.u32CMDQ_Phy    = gstInitCMDQCfg.u32CMDQ_Phy;
    stVipInitCfg.CMDQCfg.u32CMDQ_Size   = gstInitCMDQCfg.u32CMDQ_Size;
    stVipInitCfg.CMDQCfg.u32CMDQ_Vir    = gstInitCMDQCfg.u32CMDQ_Vir;
    if(MDrv_VIP_Init(&stVipInitCfg) == 0)
    {
        return -EFAULT;
    }
    MDrv_VIP_SuspendResetFlagInit();
    if (device_create_file(&pdev->dev, &dev_attr_bypass)!= 0)
    {
      dev_err(&pdev->dev,
       "Failed to create bypass sysfs files\n");
    }
    if (device_create_file(&pdev->dev, &dev_attr_ckCMDQ)!= 0)
    {
      dev_err(&pdev->dev,
       "Failed to create dev_attr_ckCMDQ sysfs files\n");
    }
    if (device_create_file(&pdev->dev, &dev_attr_ckPQ)!= 0)
    {
      dev_err(&pdev->dev,
       "Failed to create dev_attr_ckPQ sysfs files\n");
    }

     return 0;
}

static int mdrv_ms_vip_remove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    MDrv_VIP_Delete();
    _ms_vip_mem_free();
    g_bVIPSysInitReady = 0;
    return 0;
}
#endif

static int mdrv_ms_vip_suspend(struct platform_device *dev, pm_message_t state)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    g_bVIPSysInitReady = 0;
    MDrv_VIP_Delete();

    return 0;
}


static int mdrv_ms_vip_resume(struct platform_device *dev)
{
    ST_MDRV_VIP_INIT_CONFIG stVipInitCfg;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    stVipInitCfg.u32RiuBase             = 0x1F000000; //ToDo
    stVipInitCfg.CMDQCfg.u32CMDQ_Phy    = gstInitCMDQCfg.u32CMDQ_Phy;
    stVipInitCfg.CMDQCfg.u32CMDQ_Size   = gstInitCMDQCfg.u32CMDQ_Size;
    stVipInitCfg.CMDQCfg.u32CMDQ_Vir    = gstInitCMDQCfg.u32CMDQ_Vir;
    MDrv_VIP_Init(&stVipInitCfg);
    MDrv_VIP_Resume();
    MDrv_VIP_Sys_Init(&stVipInitCfg);
    MDrv_VIP_SuspendResetFlagInit();
    g_bVIPSysInitReady = 1;
    return 0;
}


int mdrv_ms_vip_open(struct inode *inode, struct file *filp)
{
    ST_MDRV_VIP_INIT_CONFIG stVipInitCfg;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    stVipInitCfg.u32RiuBase = 0x1F000000; //ToDo
    if(g_bVIPSysInitReady == 0)
    {
        if(MDrv_VIP_Sys_Init(&stVipInitCfg) == 0)
        {
            return -EFAULT;
        }
        g_bVIPSysInitReady = 1;
    }

    SCL_ASSERT(_dev_ms_vip.refCnt >= 0);
    _dev_ms_vip.refCnt++;



    return 0;
}


int mdrv_ms_vip_release(struct inode *inode, struct file *filp)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    _dev_ms_vip.refCnt--;
    if(_dev_ms_vip.refCnt==0)
    {
        MDrv_VIP_Release();
    }
    SCL_ASSERT(_dev_ms_vip.refCnt >= 0);
    //free_irq(INT_IRQ_VIPW, MDrv_VIPW_isr);
     return 0;
}


//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
#if CONFIG_OF
int _mdrv_ms_vip_init(void)
{
    int ret = 0;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s:%d\n",__FUNCTION__,__LINE__);
    ret = platform_driver_register(&st_ms_vip_driver);
    if (!ret)
    {
        SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] platform_driver_register success\n");
        if(gbProbeAlready&EN_DBG_VIP_CONFIG)
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] Probe success\n");
        }
        else
        {
            SCL_ERR( "[VIP] Probe Fail\n");
        }
        if(gbProbeAlready==EN_DBG_SCL_CONFIG)
        {
            SCL_ERR( "[SCL] SCL init success\n");
        }
    }
    else
    {
        SCL_ERR( "[VIP] platform_driver_register failed\n");
        platform_driver_unregister(&st_ms_vip_driver);
    }


    return ret;
}
void _mdrv_ms_vip_exit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);
    platform_driver_unregister(&st_ms_vip_driver);
}
#else
int _mdrv_ms_vip_init(void)
{
    int ret = 0;
    int s32Ret;
    dev_t  dev;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);

    if(_dev_ms_vip.s32Major)
    {
        dev     = MKDEV(_dev_ms_vip.s32Major, _dev_ms_vip.s32Minor);
        s32Ret  = register_chrdev_region(dev, MDRV_MS_VIP_DEVICE_COUNT, MDRV_MS_VIP_NAME);
    }
    else
    {
        s32Ret = alloc_chrdev_region(&dev, _dev_ms_vip.s32Minor, MDRV_MS_VIP_DEVICE_COUNT, MDRV_MS_VIP_NAME);
        _dev_ms_vip.s32Major = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        SCL_ERR( "[VIP] Unable to get major %d\n", _dev_ms_vip.s32Major);
        return s32Ret;
    }

    cdev_init(&_dev_ms_vip.cdev, &_dev_ms_vip.fops);
    if (0 != (s32Ret= cdev_add(&_dev_ms_vip.cdev, dev, MDRV_MS_VIP_DEVICE_COUNT)))
    {
        SCL_ERR( "[VIP] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_MS_VIP_DEVICE_COUNT);
        return s32Ret;
    }

    m_vip_class = class_create(THIS_MODULE, vip_classname);
    if(IS_ERR(m_vip_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        device_create(m_vip_class, NULL, dev,NULL, "mvip");
    }

    /* initial the whole VIP Driver */
    ret = platform_driver_register(&st_ms_vip_driver);

    if (!ret)
    {
        ret = platform_device_register(&st_ms_vip_device);
        if (ret)    /* if register device fail, then unregister the driver.*/
        {
            platform_driver_unregister(&st_ms_vip_driver);
            SCL_ERR( "[VIP] platform_driver_register failed\n");

        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] platform_driver_register success\n");
        }
    }


    return ret;
}


void _mdrv_ms_vip_exit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[VIP] %s\n",__FUNCTION__);

    cdev_del(&_dev_ms_vip.cdev);
    device_destroy(m_vip_class, MKDEV(_dev_ms_vip.s32Major, _dev_ms_vip.s32Minor));
    class_destroy(m_vip_class);
    unregister_chrdev_region(MKDEV(_dev_ms_vip.s32Major, _dev_ms_vip.s32Minor), MDRV_MS_VIP_DEVICE_COUNT);
    platform_driver_unregister(&st_ms_vip_driver);
}
#endif

module_init(_mdrv_ms_vip_init);
module_exit(_mdrv_ms_vip_exit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("ms vip ioctrl driver");
MODULE_LICENSE("GPL");

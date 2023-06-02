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
#include <linux/clk.h>
#include <linux/clk-provider.h>

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
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "ms_platform.h"
#include "ms_msys.h"
#include "mdrv_hvsp_io_st.h"
#include "mdrv_hvsp_io.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_hvsp.h"
#include "mdrv_multiinst.h"
#include "mdrv_verchk.h"
//-------------------------------------------------------------------------------------------------
#define MDRV_MS_HVSP_DEVICE_COUNT   1
#define MDRV_MS_HVSP_NAME           "mhvsp1"
#define MAX_FILE_HANDLE_SUPPRT      64
#define MDRV_NAME_HVSP              "mhvsp1"
#define MDRV_MAJOR_HVSP             0xea
#define MDRV_MINOR_HVSP             0x01

#define CMD_PARSING(x)  (x==IOCTL_HVSP_SET_IN_CONFIG         ?  "IOCTL_HVSP_SET_IN_CONFIG" : \
                         x==IOCTL_HVSP_SET_OUT_CONFIG        ?  "IOCTL_HVSP_SET_OUT_CONFIG" : \
                         x==IOCTL_HVSP_SET_SCALING_CONFIG    ?  "IOCTL_HVSP_SET_SCALING_CONFIG" : \
                         x==IOCTL_HVSP_REQ_MEM_CONFIG        ?  "IOCTL_HVSP_REQ_MEM_CONFIG" : \
                         x==IOCTL_HVSP_SET_MISC_CONFIG       ?  "IOCTL_HVSP_SET_MISC_CONFIG" : \
                         x==IOCTL_HVSP_GET_PRIVATE_ID_CONFIG ?  "IOCTL_HVSP_GET_PRIVATE_ID_CONFIG" : \
                                                                "UNKNOWN")

#define FHDWidth   1920
#define FHDHeight  1080

//-------------------------------------------------------------------------------------------------

#define _ms_hvsp1_mem_bus_to_miu(x) (x-0x2000000)
#define CMDQIRQ_ID
#define SCLIRQ_ID
//-------------------------------------------------------------------------------------------------

int mdrv_ms_hvsp1_open(struct inode *inode, struct file *filp);
int mdrv_ms_hvsp1_release(struct inode *inode, struct file *filp);
long mdrv_ms_hvsp1_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
static int mdrv_ms_hvsp1_probe(struct platform_device *pdev);
static int mdrv_ms_hvsp1_remove(struct platform_device *pdev);
static int mdrv_ms_hvsp1_suspend(struct platform_device *dev, pm_message_t state);
static int mdrv_ms_hvsp1_resume(struct platform_device *dev);
static unsigned int mdrv_ms_hvsp1_poll(struct file *filp, struct poll_table_struct *wait);

//-------------------------------------------------------------------------------------------------

dma_addr_t  sg_hvsp1_dnr_bus_addr = 0;
static u_long sg_hvsp1_dnr_size = 1920*1080*2*2 ;
static void *sg_hvsp1_dnr_vir_addr = NULL;
const static char* KEY_DMEM_SCL_DNR="SCL_DNR";
unsigned int SCL_IRQ_ID = 0; //INT_IRQ_AU_SYSTEM;
unsigned int CMDQ_IRQ_ID = 0; //INT_IRQ_AU_SYSTEM;
unsigned int gu8DNRBufferReadyNum=0;//extern
unsigned char gbdbgmessage[EN_DBGMG_NUM_CONFIG];//extern
#if CONFIG_OF
unsigned int gbProbeAlready = 0;//extern
#endif
static u_long sg_hvsp1_release_dnr_size;
static void *sg_hvsp1_release_dnr_vir_addr = NULL;
dma_addr_t sg_hvsp1_release_dnr_bus_addr;
int gVSyncCount=0;
int gMonitorErrCount = 0;

//-------------------------------------------------------------------------------------------------
typedef struct
{
    unsigned char flag;
    struct task_struct *pThread;
}ST_IOCTL_HVSP_THREAD_CONFIG;
ST_IOCTL_HVSP_THREAD_CONFIG gstThCfg={0,NULL};

typedef struct
{
    int s32Major;
    int s32Minor;
    int refCnt;
    struct cdev cdev;
    struct file_operations fops;
    ST_MDRV_HVSP_CLK_CONFIG stclk;
	struct device *devicenode;
}ST_DEV_HVSP;

static ST_DEV_HVSP _dev_ms_hvsp1 =
{
    .s32Major = MDRV_MAJOR_HVSP,
    .s32Minor = MDRV_MINOR_HVSP,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= MDRV_NAME_HVSP, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = mdrv_ms_hvsp1_open,
        .release = mdrv_ms_hvsp1_release,
        .unlocked_ioctl = mdrv_ms_hvsp1_ioctl,
        .poll = mdrv_ms_hvsp1_poll,
    },
};

static struct class * m_hvsp1_class = NULL;
static char * hvsp1_classname = "m_hvsp1_class";


static const struct of_device_id ms_hvsp1_of_match_table[] =
{
    { .compatible = "mstar,hvsp1" },
    {}
};

static struct platform_driver st_ms_hvsp1_driver =
{
	.probe 		= mdrv_ms_hvsp1_probe,
	.remove 	= mdrv_ms_hvsp1_remove,
    .suspend    = mdrv_ms_hvsp1_suspend,
    .resume     = mdrv_ms_hvsp1_resume,
	.driver =
	{
		.name	= MDRV_NAME_HVSP,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(ms_hvsp1_of_match_table),
	},
};

static u64 ms_hvsp1_dma_mask = 0xffffffffUL;

static struct platform_device st_ms_hvsp1_device =
{
    .name = "mhvsp1",
    .id = 0,
    .dev =
    {
        .dma_mask = &ms_hvsp1_dma_mask,
        .coherent_dma_mask = 0xffffffffUL
    }
};

//-------------------------------------------------------------------------------------------------
// internal function
//-------------------------------------------------------------------------------------------------
void _mdrv_hvsp1_SetInputTestPatternAndTgen(unsigned short Width,unsigned short Height)
{
    ST_MDRV_HVSP_MISC_CONFIG stHvspMiscCfg;
    unsigned char u8InputTgenSetBuf[] =
    {
        0x18, 0x12, 0x80, 0x07, 0xFF,// 4
        0x18, 0x12, 0x81, 0x80, 0xFF,// 9
        0x18, 0x12, 0x82, 0x30, 0xFF,
        0x18, 0x12, 0x83, 0x30, 0xFF,
        0x18, 0x12, 0x84, 0x10, 0xFF,
        0x18, 0x12, 0x85, 0x10, 0xFF,
        0x18, 0x12, 0x86, 0x02, 0xFF,
        0x18, 0x12, 0x87, 0x00, 0xFF,
        0x18, 0x12, 0x88, 0x21, 0xFF,
        0x18, 0x12, 0x89, 0x0C, 0xFF,
        0x18, 0x12, 0xE0, 0x01, 0xFF,
        0x18, 0x12, 0xE1, 0x00, 0xFF,
        0x18, 0x12, 0xE2, 0x01, 0xFF,
        0x18, 0x12, 0xE3, 0x00, 0xFF,
        0x18, 0x12, 0xE4, 0x03, 0xFF,
        0x18, 0x12, 0xE5, 0x00, 0xFF,
        0x18, 0x12, 0xE6, 0x05, 0xFF,
        0x18, 0x12, 0xE7, 0x00, 0xFF,
        0x18, 0x12, 0xE8, 0xE4, 0xFF,//94
        0x18, 0x12, 0xE9, 0x01, 0xFF,//99
        0x18, 0x12, 0xEA, 0x05, 0xFF,
        0x18, 0x12, 0xEB, 0x00, 0xFF,
        0x18, 0x12, 0xEC, 0xE4, 0xFF,//114
        0x18, 0x12, 0xED, 0x01, 0xFF,//119
        0x18, 0x12, 0xEE, 0x72, 0xFF,//124
        0x18, 0x12, 0xEF, 0x02, 0xFF,//129
        0x18, 0x12, 0xF2, 0x04, 0xFF,
        0x18, 0x12, 0xF3, 0x00, 0xFF,
        0x18, 0x12, 0xF4, 0x7F, 0xFF,
        0x18, 0x12, 0xF5, 0x00, 0xFF,
        0x18, 0x12, 0xF6, 0xA8, 0xFF,
        0x18, 0x12, 0xF7, 0x00, 0xFF,
        0x18, 0x12, 0xF8, 0xA7, 0xFF,//164
        0x18, 0x12, 0xF9, 0x01, 0xFF,//169
        0x18, 0x12, 0xFA, 0xA8, 0xFF,
        0x18, 0x12, 0xFB, 0x00, 0xFF,
        0x18, 0x12, 0xFC, 0xA7, 0xFF,//184
        0x18, 0x12, 0xFD, 0x01, 0xFF,//189
        0x18, 0x12, 0xFE, 0x1F, 0xFF,//194
        0x18, 0x12, 0xFF, 0x04, 0xFF,//199
        0x21, 0x12, 0xE0, 0x01, 0x01,//vip
        0x1E, 0x12, 0x70, 0x00, 0xFF,
        0x1E, 0x12, 0x71, 0x04, 0x07,
        0x1E, 0x12, 0x72, 0x00, 0xFF,
        0x1E, 0x12, 0x73, 0x00, 0x01,
        0x1E, 0x12, 0x74, 0x00, 0xFF,
        0x1E, 0x12, 0x75, 0x04, 0x07,
        0x1E, 0x12, 0x76, 0x00, 0xFF,
        0x1E, 0x12, 0x77, 0x00, 0x01,
        0x1E, 0x12, 0x78, 0x00, 0xFF,
        0x1E, 0x12, 0x79, 0x04, 0x07,
        0x1E, 0x12, 0x7A, 0x00, 0xFF,
        0x1E, 0x12, 0x7B, 0x00, 0x01,
        0x1E, 0x12, 0x7C, 0x00, 0xFF,
        0x1E, 0x12, 0x7D, 0x04, 0x07,
        0x1E, 0x12, 0x7E, 0x00, 0xFF,
        0x1E, 0x12, 0x7F, 0x00, 0x01,
    };

    // Input tgen setting
    u8InputTgenSetBuf[93]  = (unsigned char)((0x4+Height)&0x00FF);
    u8InputTgenSetBuf[98]  = (unsigned char)(((0x4+Height)&0xFF00)>>8);
    u8InputTgenSetBuf[113] = (unsigned char)((0x4+Height)&0x00FF);
    u8InputTgenSetBuf[118] = (unsigned char)(((0x4+Height)&0xFF00)>>8);
    u8InputTgenSetBuf[123] = 0xFF;
    u8InputTgenSetBuf[128] = 0x08;
    u8InputTgenSetBuf[163] = (unsigned char)((0xA7+Width)&0x00FF);
    u8InputTgenSetBuf[168] = (unsigned char)(((0xA7+Width)&0xFF00)>>8);
    u8InputTgenSetBuf[183] = (unsigned char)((0xA7+Width)&0x00FF);
    u8InputTgenSetBuf[188] = (unsigned char)(((0xA7+Width)&0xFF00)>>8);
    u8InputTgenSetBuf[193] = 0xFF;
    u8InputTgenSetBuf[198] = 0x08;

    stHvspMiscCfg.u8Cmd     = 0;
    stHvspMiscCfg.u32Size   = sizeof(u8InputTgenSetBuf);
    stHvspMiscCfg.u32Addr   = (unsigned long)u8InputTgenSetBuf;
    MDrv_HVSP_SetMiscConfigForKernel(&stHvspMiscCfg);
}

void _mdrv_hvsp1_OpenInputTestPatternAndTgen(void)
{
    ST_MDRV_HVSP_SCINFORM_CONFIG stCfg;
    MDrv_HVSP_GetSCLInform(E_MDRV_HVSP_ID_1,&stCfg);
    if(stCfg.u16crop2inWidth && stCfg.u16crop2inHeight)
    {
        MDrv_HVSP_SetPatTgenStatus(TRUE);
        _mdrv_hvsp1_SetInputTestPatternAndTgen(stCfg.u16crop2inWidth,stCfg.u16crop2inHeight);
    }
    else
    {
        MDrv_HVSP_SetPatTgenStatus(TRUE);
        _mdrv_hvsp1_SetInputTestPatternAndTgen(FHDWidth,FHDHeight);
        SCL_ERR( "[HVSP1]Open Input Test Pattern And Tgen Default FHD\n");
    }
}

void _mdrv_hvsp1_OpenTestPatternByISPTgen(unsigned char bDynamic)
{
    ST_MDRV_HVSP_MISC_CONFIG stHvspMiscCfg;
    unsigned char input_tgen_buf[] =
    {
        0x18, 0x12, 0x80, 0x03, 0xFF,// 4
        0x18, 0x12, 0x81, 0x80, 0xFF,// 9
        0x18, 0x12, 0x82, 0x30, 0xFF,
        0x18, 0x12, 0x83, 0x30, 0xFF,
        0x18, 0x12, 0x84, 0x10, 0xFF,
        0x18, 0x12, 0x85, 0x10, 0xFF,
        0x18, 0x12, 0x86, 0x02, 0xFF,
        0x18, 0x12, 0x87, 0x00, 0xFF,
        0x18, 0x12, 0x88, 0x21, 0xFF,
        0x18, 0x12, 0x89, 0x0C, 0xFF,
        0x21, 0x12, 0xE0, 0x01, 0x01,//vip
        0x1E, 0x12, 0x70, 0x00, 0xFF,
        0x1E, 0x12, 0x71, 0x04, 0x07,
        0x1E, 0x12, 0x72, 0x00, 0xFF,
        0x1E, 0x12, 0x73, 0x00, 0x01,
        0x1E, 0x12, 0x74, 0x00, 0xFF,
        0x1E, 0x12, 0x75, 0x04, 0x07,
        0x1E, 0x12, 0x76, 0x00, 0xFF,
        0x1E, 0x12, 0x77, 0x00, 0x01,
        0x1E, 0x12, 0x78, 0x00, 0xFF,
        0x1E, 0x12, 0x79, 0x04, 0x07,
        0x1E, 0x12, 0x7A, 0x00, 0xFF,
        0x1E, 0x12, 0x7B, 0x00, 0x01,
        0x1E, 0x12, 0x7C, 0x00, 0xFF,
        0x1E, 0x12, 0x7D, 0x04, 0x07,
        0x1E, 0x12, 0x7E, 0x00, 0xFF,
        0x1E, 0x12, 0x7F, 0x00, 0x01,
    };
    if(bDynamic)
    {
        input_tgen_buf[3]   = (unsigned char)0x7;
    }
    stHvspMiscCfg.u8Cmd     = 0;
    stHvspMiscCfg.u32Size   = sizeof(input_tgen_buf);
    stHvspMiscCfg.u32Addr   = (unsigned long)input_tgen_buf;
    MDrv_HVSP_SetMiscConfigForKernel(&stHvspMiscCfg);

}
void _mdrv_hvsp1_CloseTestPatternByISPTgen(void)
{
    ST_MDRV_HVSP_MISC_CONFIG stHvspMiscCfg;
    unsigned char input_tgen_buf[] =
    {
        0x18, 0x12, 0x80, 0x00, 0xFF,// 4
        0x18, 0x12, 0x81, 0x00, 0xFF,// 9
        0x18, 0x12, 0x86, 0x02, 0xFF,
        0x18, 0x12, 0x88, 0x20, 0xFF,
        0x18, 0x12, 0x89, 0x0C, 0xFF,
        0x18, 0x12, 0xE0, 0x00, 0xFF,
        0x18, 0x12, 0xE1, 0x00, 0xFF,
        0x21, 0x12, 0xE0, 0x00, 0x01,
    };
    stHvspMiscCfg.u8Cmd     = 0;
    stHvspMiscCfg.u32Size   = sizeof(input_tgen_buf);
    stHvspMiscCfg.u32Addr   = (unsigned long)input_tgen_buf;
    MDrv_HVSP_SetPatTgenStatus(FALSE);
    MDrv_HVSP_SetMiscConfigForKernel(&stHvspMiscCfg);

}

unsigned long _mdrv_hvsp1_HWMonitor(unsigned char u8flag)
{
    return MDrv_HVSP_HWMonitor(u8flag);
}
unsigned char _mdrv_hvsp1_InputVSyncMonitor(void)
{
    if(MDrv_HVSP_InputVSyncMonitor())
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static ssize_t ptgen_call_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL!=buf)
	{
		const char *str = buf;
        if((int)*str == 49)    //input 1  echo 1 >ptgen_call
        {
            SCL_ERR( "[HVSP1]ptgen_call_store OK %d\n",(int)*str);
            _mdrv_hvsp1_OpenTestPatternByISPTgen(EN_MDRV_HVSP_CALLPATGEN_STATIC);
        }
        else if((int)*str == 48)  //input 0  echo 0 >ptgen_call
        {
            SCL_ERR( "[HVSP1]ptgen_call_close %d\n",(int)*str);
            _mdrv_hvsp1_CloseTestPatternByISPTgen();
        }
        else if((int)*str == 50)  //input 2
        {
            SCL_ERR( "[HVSP1]dynamic ptgen OK %d\n",(int)*str);
            _mdrv_hvsp1_OpenTestPatternByISPTgen(EN_MDRV_HVSP_CALLPATGEN_DYNAMIC);
        }
        else if((int)*str == 51)  //input 3
        {
            SCL_ERR( "[HVSP1]ptgen_call SCL INPUT Tgen %d\n",(int)*str);
            _mdrv_hvsp1_OpenInputTestPatternAndTgen();
        }
        return n;
	}

	return 0;
}
static ssize_t ptgen_call_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"0:close\n1:open static ptgen\n2:open dynamic ptgen\n3:open scl time gen pattern");
}

static DEVICE_ATTR(ptgen,0644, ptgen_call_show, ptgen_call_store);

static int ms_hvsp1_MonitorThread(void *arg)
{
    unsigned long u32Stime = 0,u32difftime = 0;
    u32Stime = MsOS_GetSystemTime();
    _mdrv_hvsp1_HWMonitor(gstThCfg.flag);
    while(gstThCfg.flag)
    {
        if(_mdrv_hvsp1_InputVSyncMonitor())
        {
            gVSyncCount++;
            gMonitorErrCount += _mdrv_hvsp1_HWMonitor(gstThCfg.flag);
            u32difftime     = MsOS_Timer_DiffTimeFromNow(u32Stime);
        }
    }
    SCL_ERR( "[HVSP1]vsync count:%d MonitorErrCount:%d\n       average:%hd (ms/frm)\n"
        ,gVSyncCount,gMonitorErrCount,(short)(u32difftime/gVSyncCount));
    kthread_stop(gstThCfg.pThread);
    gstThCfg.pThread = NULL;
    return 0;
}
static ssize_t monitorHW_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return MDrv_HVSP_monitorHWShow(buf,gVSyncCount,gMonitorErrCount);
}
void _mdrv_hvsp1_CreateMonitorTask(unsigned char u8MonitorFlag)
{
    const char *pName = {"HVSP_THREAD"};
    gstThCfg.flag   = u8MonitorFlag;
    gVSyncCount    = 0;
    gMonitorErrCount  = 0;
    gstThCfg.pThread = kthread_create(ms_hvsp1_MonitorThread,(void *)&gVSyncCount,pName);
    if (IS_ERR(gstThCfg.pThread))
    {
        gstThCfg.pThread = NULL;
        SCL_ERR( "[HVSP1]Fail:creat thread\n");
    }
    else
    {
        wake_up_process(gstThCfg.pThread );
    }
}
static ssize_t monitorHW_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL!=buf)
	{
		const char *str = buf;
        if((int)*str == 49)    //input 1  echo 1 >crop monitor
        {
            _mdrv_hvsp1_CreateMonitorTask(EN_MDRV_HVSP_MONITOR_CROPCHECK);
        }
        else if((int)*str == 50)    //input 2  echo 2 >dma monitor
        {
            _mdrv_hvsp1_CreateMonitorTask(EN_MDRV_HVSP_MONITOR_DMA1FRMCHECK);
        }
        else if((int)*str == 51)    //input 2  echo 3 >dma monitor
        {
            _mdrv_hvsp1_CreateMonitorTask(EN_MDRV_HVSP_MONITOR_DMA1SNPCHECK);
        }
        else if((int)*str == 52)    //input 2  echo 4 >dma monitor
        {
            _mdrv_hvsp1_CreateMonitorTask(EN_MDRV_HVSP_MONITOR_DMA2FRMCHECK);
        }
        else if((int)*str == 53)    //input 2  echo 5 >dma monitor
        {
            _mdrv_hvsp1_CreateMonitorTask(EN_MDRV_HVSP_MONITOR_DMA3FRMCHECK);
        }
        else if((int)*str == 48)
        {
            gstThCfg.flag =0 ;
        }
        return n;
	}

	return 0;
}

static DEVICE_ATTR(monitorHW,0644, monitorHW_show, monitorHW_store);

static ssize_t check_clk_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ST_MDRV_HVSP_CLK_CONFIG *stclk = NULL;
    stclk = &(_dev_ms_hvsp1.stclk);
    return MDrv_HVSP_ClkFrameworkShow(buf,stclk);
}

static ssize_t check_clk_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL!=buf)
	{
		const char *str = buf;
        if((int)*str == 49)    //input 1
        {
            SCL_ERR( "[CLK]open force mode %d\n",(int)*str);
            MDrv_HVSP_SetCLKForcemode(1);
        }
        else if((int)*str == 48)  //input 0
        {
            SCL_ERR( "[CLK]close force mode %d\n",(int)*str);
            MDrv_HVSP_SetCLKForcemode(0);
        }
        else if((int)*str == 50)  //input 2
        {
            SCL_ERR( "[CLK]fclk1 max %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk1,0);
        }
        else if((int)*str == 51)  //input 3
        {
            SCL_ERR( "[CLK]fclk1 med %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk1,1);
        }
        else if((int)*str == 52)  //input 4
        {
            SCL_ERR( "[CLK]fclk1 open %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.fclk1,1);
        }
        else if((int)*str == 53)  //input 5
        {
            SCL_ERR( "[CLK]fclk1 close %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.fclk1,0);
        }
        else if((int)*str == 54)  //input 6
        {
            SCL_ERR( "[CLK]fclk2 max %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk2,0);
        }
        else if((int)*str == 55)  //input 7
        {
            SCL_ERR( "[CLK]fclk2 med %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk2,1);
        }
        else if((int)*str == 56)  //input 8
        {
            SCL_ERR( "[CLK]fclk2 open %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.fclk2,1);
        }
        else if((int)*str == 57)  //input 9
        {
            SCL_ERR( "[CLK]fclk2 close %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.fclk2,0);
        }
        else if((int)*str == 58)  //input :
        {
            SCL_ERR( "[CLK]idclk ISP %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.idclk,0x10);
        }
        else if((int)*str == 68)  //input D
        {
            SCL_ERR( "[CLK]idclk BT656 %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.idclk,0x21);
        }
        else if((int)*str == 66)  //input B
        {
            SCL_ERR( "[CLK]idclk Open %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.idclk,1);
        }
        else if((int)*str == 61)  //input =
        {
            SCL_ERR( "[CLK]idclk Close %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.idclk,0);
        }
        else if((int)*str == 67)  //input C
        {
            SCL_ERR( "[CLK]odclk MAX %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.odclk,0);
        }
        else if((int)*str == 63)  //input ?
        {
            SCL_ERR( "[CLK]odclk LPLL %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.odclk,3);
        }
        else if((int)*str == 64)  //input @
        {
            SCL_ERR( "[CLK]odclk open %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.odclk,1);
        }
        else if((int)*str == 65)  //input A
        {
            SCL_ERR( "[CLK]odclk close %d\n",(int)*str);
            MDrv_HVSP_SetCLKOnOff(_dev_ms_hvsp1.stclk.odclk,0);
        }
        return n;
	}

	return 0;
}


static DEVICE_ATTR(clk,0644, check_clk_show, check_clk_store);
static ssize_t check_osd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL!=buf)
	{
        MDrv_HVSP_OsdStore(buf,E_MDRV_HVSP_ID_1);
        return n;
	}

	return 0;
}
static ssize_t check_osd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return MDrv_HVSP_OsdShow(buf);
}
static DEVICE_ATTR(osd,0644, check_osd_show, check_osd_store);
static ssize_t check_fbmg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG stFbMgCfg;
    const char *str = buf;
	if(NULL!=buf)
	{
        //if(!)
        if((int)*str == 49)    //input 1
        {
            SCL_ERR( "[OSD]open OSD %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_LDCPATH_ON;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 50)  //input 2
        {
            SCL_ERR( "[OSD]Set OSD before %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_LDCPATH_OFF;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 51)  //input 3
        {
            SCL_ERR( "[OSD]Set OSD After %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_DNR_Read_ON;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 52)  //input 4
        {
            SCL_ERR( "[OSD]Set OSD Bypass %d\n",(int)*str);
            stFbMgCfg.enSet= EN_IOCTL_HVSP_FBMG_SET_DNR_Read_OFF;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 53)  //input 5
        {
            SCL_ERR( "[OSD]Set OSD Bypass Off %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_DNR_Write_ON;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 54)  //input 6
        {
            SCL_ERR( "[OSD]Set OSD WTM Bypass %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_DNR_Write_OFF;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 55)  //input 7
        {
            SCL_ERR( "[OSD]Set OSD WTM Bypass Off %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_DNR_BUFFER_1;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 56)  //input 8
        {
            SCL_ERR( "[OSD]Set OSD WTM Bypass Off %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_DNR_BUFFER_2;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 57)  //input 9
        {
            SCL_ERR( "[OSD]Set OSD WTM Bypass Off %d\n",(int)*str);
            stFbMgCfg.enSet = EN_IOCTL_HVSP_FBMG_SET_UNLOCK;
            MDrv_HVSP_SetFbManageConfig(stFbMgCfg.enSet);
        }
        return n;
	}

	return 0;
}
static ssize_t check_fbmg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return MDrv_HVSP_FBMGShow(buf);
}

static DEVICE_ATTR(fbmg,0644, check_fbmg_show, check_fbmg_store);


static ssize_t check_proc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_ProcShow(buf);
}
static DEVICE_ATTR(proc,0444, check_proc_show, NULL);

static ssize_t check_dbgmg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_DbgmgFlagShow(buf);
}
static unsigned char _mdrv_hvsp1_Changebuf2hex(int u32num)
{
    unsigned char u8level;
    if(u32num==10)
    {
        u8level = 1;
    }
    else if(u32num==48)
    {
        u8level = 0;
    }
    else if(u32num==49)
    {
        u8level = 0x1;
    }
    else if(u32num==50)
    {
        u8level = 0x2;
    }
    else if(u32num==51)
    {
        u8level = 0x3;
    }
    else if(u32num==52)
    {
        u8level = 0x4;
    }
    else if(u32num==53)
    {
        u8level = 0x5;
    }
    else if(u32num==54)
    {
        u8level = 0x6;
    }
    else if(u32num==55)
    {
        u8level = 0x7;
    }
    else if(u32num==56)
    {
        u8level = 0x8;
    }
    else if(u32num==57)
    {
        u8level = 0x9;
    }
    else if(u32num==65)
    {
        u8level = 0xa;
    }
    else if(u32num==66)
    {
        u8level = 0xb;
    }
    else if(u32num==67)
    {
        u8level = 0xc;
    }
    else if(u32num==68)
    {
        u8level = 0xd;
    }
    else if(u32num==69)
    {
        u8level = 0xe;
    }
    else if(u32num==70)
    {
        u8level = 0xf;
    }
    else if(u32num==97)
    {
        u8level = 0xa;
    }
    else if(u32num==98)
    {
        u8level = 0xb;
    }
    else if(u32num==99)
    {
        u8level = 0xc;
    }
    else if(u32num==100)
    {
        u8level = 0xd;
    }
    else if(u32num==101)
    {
        u8level = 0xe;
    }
    else if(u32num==102)
    {
        u8level = 0xf;
    }
    return u8level;
}

static ssize_t check_dbgmg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	if(NULL!=buf)
	{
		const char *str = buf;
        unsigned char u8level;
        SCL_ERR( "[HVSP1]check_dbgmg_store OK %d\n",(int)*str);
        SCL_ERR( "[HVSP1]check_dbgmg_store level %d\n",(int)*(str+1));
        SCL_ERR( "[HVSP1]check_dbgmg_store level2 %d\n",(int)*(str+2));
        if(((int)*(str+2))>=48)//LF :line feed
        {
            u8level = _mdrv_hvsp1_Changebuf2hex((int)*(str+2));
            u8level |= (_mdrv_hvsp1_Changebuf2hex((int)*(str+1))<<4);
        }
        else
        {
            u8level = _mdrv_hvsp1_Changebuf2hex((int)*(str+1));
        }

        if((int)*str == 48)    //input 1  echo 0 >
        {
            Reset_DBGMG_FLAG();
        }
        else if((int)*str == 49)    //input 1  echo 1 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_MDRV_CONFIG,u8level);
        }
        else if((int)*str == 50)    //input 1  echo 2 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_IOCTL_CONFIG,u8level);
        }
        else if((int)*str == 51)    //input 1  echo 3 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_HVSP_CONFIG,u8level);
        }
        else if((int)*str == 52)    //input 1  echo 4 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_SCLDMA_CONFIG,u8level);
        }
        else if((int)*str == 53)    //input 1  echo 5 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_PNL_CONFIG,u8level);
        }
        else if((int)*str == 54)    //input 1  echo 6 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_VIP_CONFIG,u8level);
        }
        else if((int)*str == 55)    //input 1  echo 7 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVPQ_CONFIG,u8level);
        }
        else if((int)*str == 56)    //input 1  echo 8 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_INST_ENTRY_CONFIG,u8level);
        }
        else if((int)*str == 57)    //input 1  echo 9 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_INST_LOCK_CONFIG,u8level);
        }
        else if((int)*str == 65)    //input 1  echo A >
        {
            Set_DBGMG_FLAG(EN_DBGMG_INST_FUNC_CONFIG,u8level);
        }
        else if((int)*str == 66)    //input 1  echo B >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVHVSP_CONFIG,u8level);
        }
        else if((int)*str == 67)    //input 1  echo C >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVSCLDMA_CONFIG,u8level);
        }
        else if((int)*str == 68)    //input 1  echo D >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVSCLIRQ_CONFIG,u8level);
        }
        else if((int)*str == 69)    //input 1  echo E >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVCMDQ_CONFIG,u8level);
        }
        else if((int)*str == 70)    //input 1  echo F >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVVIP_CONFIG,u8level);
        }
        else if((int)*str == 71)    //input 1  echo G >
        {
            Set_DBGMG_FLAG(EN_DBGMG_PRIORITY_CONFIG,1);
        }
        return n;
	}

	return 0;
}

static DEVICE_ATTR(dbgmg,0644, check_dbgmg_show, check_dbgmg_store);

static void* _ms_hvsp1_AllocDmem(const char* name, unsigned int size, dma_addr_t *addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name,name,strlen(name)+1);
    dmem.length = size;
    if(0 != msys_request_dmem(&dmem))
    {
        return NULL;
    }
    else
    {
        gu8DNRBufferReadyNum = 1 + (SCL_DELAY2FRAMEINDOUBLEBUFFERMode);
        sg_hvsp1_release_dnr_size       = sg_hvsp1_dnr_size;
        sg_hvsp1_release_dnr_vir_addr   = sg_hvsp1_dnr_vir_addr;
        sg_hvsp1_release_dnr_bus_addr   = sg_hvsp1_dnr_bus_addr;
    }
    *addr = dmem.phys;
    return (void *)((uintptr_t)dmem.kvirt);
}

static void _ms_hvsp1_FreeDmem(const char* name, unsigned int size, void *virt, dma_addr_t addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name,name,strlen(name)+1);
    dmem.length = size;
    dmem.kvirt  = (unsigned long long)((uintptr_t)virt);
    dmem.phys   = (unsigned long long)((uintptr_t)addr);
    msys_release_dmem(&dmem);
}


static int _ms_hvsp1_mem_allocate(void)
{
    SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP1] allocate memory\n");

    if (!(sg_hvsp1_dnr_vir_addr = _ms_hvsp1_AllocDmem(KEY_DMEM_SCL_DNR,
                                             PAGE_ALIGN(sg_hvsp1_dnr_size),
                                             &sg_hvsp1_dnr_bus_addr)))
    {
        SCL_ERR( "%s: unable to allocate screen memory\n", __FUNCTION__);
        return 0;
    }
    SCL_ERR( "[HVSP1]: DNR: Phy:%x  Vir:%x\n", sg_hvsp1_dnr_bus_addr, (u32)sg_hvsp1_dnr_vir_addr);


    return 1;
}

static void _ms_hvsp1_mem_free(void)
{
    if(sg_hvsp1_dnr_vir_addr != 0)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP1] mem free\n");
        _ms_hvsp1_FreeDmem(KEY_DMEM_SCL_DNR,
                  PAGE_ALIGN(sg_hvsp1_dnr_size),
                  sg_hvsp1_dnr_vir_addr,
                  sg_hvsp1_dnr_bus_addr);

        sg_hvsp1_dnr_vir_addr = 0;
        sg_hvsp1_dnr_bus_addr = 0;
        gu8DNRBufferReadyNum = 0;
    }
}
static int _ms_hvsp1_multiinstSet(EN_MDRV_MULTI_INST_CMD_TYPE enType, void *stCfg ,void *privatedata)
{
    EN_MDRV_MULTI_INST_STATUS_TYPE enMultiInstRet;
    int ret = 0;
    enMultiInstRet = MDrv_MultiInst_Entry_FlashData(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1,privatedata ,enType,stCfg);

    if(enMultiInstRet == E_MDRV_MULTI_INST_STATUS_LOCKED)
    {
        ret = -EINVAL;
    }
    else if (enMultiInstRet == E_MDRV_MULTI_INST_STATUS_FAIL)
    {
        ret = -EFAULT;
    }
    else
    {
        ret = 0;
    }
    return ret;
}
//-------------------------------------------------------------------------------------------------
// IOCtrl Driver interface functions
//-------------------------------------------------------------------------------------------------
ST_MDRV_HVSP_VERSIONCHK_CONFIG _mdrv_ms_hvsp1_io_fill_versionchkstruct
(unsigned int u32StructSize,unsigned int u32VersionSize,unsigned int *pVersion)
{
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;
    stVersion.u32StructSize  = (unsigned int)u32StructSize;
    stVersion.u32VersionSize = (unsigned int)u32VersionSize;
    stVersion.pVersion      = (unsigned int *)pVersion;
    return stVersion;
}
int _mdrv_ms_hvsp1_io_version_check(ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion)
{
    if ( CHK_VERCHK_HEADER(stVersion.pVersion) )
    {
        if( CHK_VERCHK_MAJORVERSION_LESS( stVersion.pVersion, IOCTL_HVSP_VERSION) )
        {

            VERCHK_ERR("[HVSP1] Version(%04x) < %04x!!! \n",
                *(stVersion.pVersion) & VERCHK_VERSION_MASK,
                IOCTL_HVSP_VERSION);

            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &stVersion.u32VersionSize, stVersion.u32StructSize) == 0 )
            {
                VERCHK_ERR("[HVSP1] Size(%04x) != %04x!!! \n",
                    stVersion.u32StructSize,
                    stVersion.u32VersionSize);

                return -EINVAL;
            }
            else
            {
                SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_ELSE, "[HVSP1] Size(%d) \n",stVersion.u32StructSize );
                return VersionCheckSuccess;
            }
        }
    }
    else
    {
        VERCHK_ERR("[HVSP1] No Header !!! \n");
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EFAULT;
    }
}
MS_BOOL _mdrv_hvsp1_CheckModifyMemSize(ST_IOCTL_HVSP_REQ_MEM_CONFIG *stReqMemCfg)
{
    if( (stReqMemCfg->u16Vsize & (15)) || (stReqMemCfg->u16Pitch & (15)))
    {
        SCL_ERR(
            "[HVSP1] Size must be align 16, Vsize=%d, Pitch=%d\n",stReqMemCfg->u16Vsize, stReqMemCfg->u16Pitch);
        if((stReqMemCfg->u16Pitch & (15))&&(stReqMemCfg->u16Vsize & (15)))
        {
            stReqMemCfg->u32MemSize = (((stReqMemCfg->u16Vsize/16)+1)*16) * ((stReqMemCfg->u16Pitch/16)+1)*16 * 4;
        }
        else if(stReqMemCfg->u16Pitch & (15))
        {
            stReqMemCfg->u32MemSize = (stReqMemCfg->u16Vsize) * ((stReqMemCfg->u16Pitch/16)+1)*16 * 4;
        }
        else if(stReqMemCfg->u16Vsize & (15))
        {
            stReqMemCfg->u32MemSize = (((stReqMemCfg->u16Vsize/16)+1)*16) * stReqMemCfg->u16Pitch * 4;
        }
    }
    else
    {
        stReqMemCfg->u32MemSize = (stReqMemCfg->u16Vsize) *(stReqMemCfg->u16Pitch) *4;
    }
    if(Mdrv_HVSP_GetDNRBufferInformation()== 1)
    {
        SCL_ERR(
            "[HVSP1] Buffer is single, Vsize=%d, Pitch=%d\n",stReqMemCfg->u16Vsize, stReqMemCfg->u16Pitch);
        stReqMemCfg->u32MemSize = stReqMemCfg->u32MemSize /2 ;
    }
    sg_hvsp1_dnr_size = stReqMemCfg->u32MemSize;
    if(Mdrv_HVSP_GetDNRBufferInformation()== 1)
    {
        if((unsigned long)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 2) > stReqMemCfg->u32MemSize)
        {
            SCL_ERR( "[HVSP1] Memory size is too small, Vsize*Pitch*2=%lx, MemSize=%lx\n",
             (unsigned long)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 2), stReqMemCfg->u32MemSize);
            return -EFAULT;
        }
    }
    else if((unsigned long)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 4) > stReqMemCfg->u32MemSize)
    {
        SCL_ERR( "[HVSP1] Memory size is too small, Vsize*Pitch*4=%lx, MemSize=%lx\n",
         (unsigned long)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 4), stReqMemCfg->u32MemSize);
        return -EFAULT;
    }
    SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1,
        "[HVSP1], Vsize=%d, Pitch=%d, Size=%lx\n", stReqMemCfg->u16Vsize, stReqMemCfg->u16Pitch,stReqMemCfg->u32MemSize);
    return 0;
}

void _mdrv_hvsp1_DNRBufferMemoryAllocate(void)
{
    if(gu8DNRBufferReadyNum == 0)
    {
        _ms_hvsp1_mem_allocate();
        MDrv_HVSP_SetMemoryAllocateReady(gu8DNRBufferReadyNum);
    }
    else if(gu8DNRBufferReadyNum != 0 && sg_hvsp1_dnr_size > sg_hvsp1_release_dnr_size)
    {
        _ms_hvsp1_mem_free();
        gu8DNRBufferReadyNum = 0;
        _ms_hvsp1_mem_allocate();
        MDrv_HVSP_SetMemoryAllocateReady((gu8DNRBufferReadyNum > 0) ? 1 :0);
    }
}
ST_MDRV_HVSP_IPM_CONFIG _mdrv_hvsp1_FillIPMStructForDriver(ST_IOCTL_HVSP_REQ_MEM_CONFIG stReqMemCfg)
{
    ST_MDRV_HVSP_IPM_CONFIG stIPMCfg;
    stIPMCfg.u16Height = stReqMemCfg.u16Vsize;
    stIPMCfg.u16Width  = stReqMemCfg.u16Pitch;
    stIPMCfg.u32MemSize = stReqMemCfg.u32MemSize;
    if(gu8DNRBufferReadyNum)
    {
        stIPMCfg.enRW       = E_IOCTL_HVSP_DNR_W;
        sg_hvsp1_dnr_size   = stReqMemCfg.u32MemSize;
    }
    else
    {
        stIPMCfg.enRW       = E_IOCTL_HVSP_DNR_NUM;
        sg_hvsp1_dnr_size   = stReqMemCfg.u32MemSize;
    }
    stIPMCfg.u32PhyAddr = Chip_Phys_to_MIU(sg_hvsp1_dnr_bus_addr);
    return stIPMCfg;
}


int _mdrv_ms_hvsp1_io_set_input_config(struct file *filp, unsigned long arg)
{
    ST_MDRV_HVSP_INPUT_CONFIG stInCfg;
    ST_IOCTL_HVSP_INPUT_CONFIG stIOInCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_INPUT_CONFIG),
        (((ST_IOCTL_HVSP_INPUT_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_INPUT_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_hvsp1_io_version_check(stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOInCfg, (ST_IOCTL_HVSP_INPUT_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_INPUT_CONFIG)))
        {
            return -EFAULT;
        }
        else
        {
            stInCfg.enColor       = stIOInCfg.enColor;
            stInCfg.enSrcType     = stIOInCfg.enSrcType;
            memcpy(&stInCfg.stCaptureWin , &stIOInCfg.stCaptureWin,sizeof(ST_MDRV_HVSP_WINDOW_CONFIG));
            memcpy(&stInCfg.stTimingCfg , &stIOInCfg.stTimingCfg,sizeof(ST_MDRV_HVSPTIMING_CONFIG));
        }

    }
    stInCfg.stclk = &(_dev_ms_hvsp1.stclk);

    if(_ms_hvsp1_multiinstSet(E_MDRV_MULTI_INST_CMD_HVSP_IN_CONFIG, (void *)&stInCfg, filp->private_data))
    {
        ret = -EFAULT;
    }
    else
    {
        if(!MDrv_HVSP_SetInputConfig(E_MDRV_HVSP_ID_1,  &stInCfg))
        {
            ret = -EFAULT;
        }
        else
        {
            ret = 0;
        }
    }

    return ret;

}

int _mdrv_ms_hvsp1_io_set_output_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_OUTPUT_CONFIG stOutCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_OUTPUT_CONFIG),
        (((ST_IOCTL_HVSP_OUTPUT_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_OUTPUT_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_hvsp1_io_version_check(stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stOutCfg, (ST_IOCTL_HVSP_OUTPUT_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_OUTPUT_CONFIG)))
        {
            return -EFAULT;
        }
    }

    return ret;
}


int _mdrv_ms_hvsp1_io_set_scaling_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_SCALING_CONFIG stIOSclCfg;
    ST_MDRV_HVSP_SCALING_CONFIG stSclCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_SCALING_CONFIG),
        (((ST_IOCTL_HVSP_SCALING_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_SCALING_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_hvsp1_io_version_check(stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOSclCfg, (ST_IOCTL_HVSP_SCALING_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_SCALING_CONFIG)))
        {
            return -EFAULT;
        }
        else
        {
            stSclCfg.stclk     = (ST_MDRV_HVSP_CLK_CONFIG *)&(_dev_ms_hvsp1.stclk);
            stSclCfg.stCropWin.bEn = stIOSclCfg.bCropEn;
            stSclCfg.stCropWin.u16Height = stIOSclCfg.stCropWin.u16Height;
            stSclCfg.stCropWin.u16Width = stIOSclCfg.stCropWin.u16Width;
            stSclCfg.stCropWin.u16X = stIOSclCfg.stCropWin.u16X;
            stSclCfg.stCropWin.u16Y = stIOSclCfg.stCropWin.u16Y;
            stSclCfg.u16Dsp_Height = stIOSclCfg.u16Dsp_Height;
            stSclCfg.u16Dsp_Width = stIOSclCfg.u16Dsp_Width;
            stSclCfg.u16Src_Height = stIOSclCfg.u16Src_Height;
            stSclCfg.u16Src_Width = stIOSclCfg.u16Src_Width;
        }
    }
    if(_ms_hvsp1_multiinstSet(E_MDRV_MULTI_INST_CMD_HVSP_SCALING_CONFIG, (void *)&stSclCfg, filp->private_data))
    {
        ret = -EFAULT;
    }
    else
    {
        if(!MDrv_HVSP_SetScalingConfig(E_MDRV_HVSP_ID_1,  &stSclCfg ))
        {
            ret = -EFAULT;
        }
        else
        {
            ret = 0;
        }
    }
    return ret;
}
int _mdrv_ms_hvsp1_io_req_mem_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_REQ_MEM_CONFIG stReqMemCfg;
    ST_MDRV_HVSP_IPM_CONFIG stIPMCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_REQ_MEM_CONFIG),
        (((ST_IOCTL_HVSP_REQ_MEM_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_REQ_MEM_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_hvsp1_io_version_check(stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stReqMemCfg, (ST_IOCTL_HVSP_REQ_MEM_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_REQ_MEM_CONFIG)))
        {
            return -EFAULT;
        }
    }
    ret = _mdrv_hvsp1_CheckModifyMemSize(&stReqMemCfg);
    _mdrv_hvsp1_DNRBufferMemoryAllocate();
    stIPMCfg = _mdrv_hvsp1_FillIPMStructForDriver(stReqMemCfg);
    if(_ms_hvsp1_multiinstSet(E_MDRV_MULTI_INST_CMD_HVSP_MEM_REQ_CONFIG, (void *)&stIPMCfg, filp->private_data))
    {
        ret = -EFAULT;
    }
    else
    {
        if(MDrv_HVSP_SetInitIPMConfig(E_MDRV_HVSP_ID_1, &stIPMCfg))
        {
            ret = 0;
        }
        else
        {
            ret = -EFAULT;
        }
    }
    if(gu8DNRBufferReadyNum == 0)
    {
        ret = -EFAULT;
    }
    return ret;
}

int _mdrv_ms_hvsp1_io_set_misc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_MISC_CONFIG stIOMiscCfg;
    ST_MDRV_HVSP_MISC_CONFIG stMiscCfg;
    if(copy_from_user(&stIOMiscCfg, (ST_IOCTL_HVSP_MISC_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_MISC_CONFIG)))
    {
        return -EFAULT;
    }
    else
    {
        memcpy(&stMiscCfg, &stIOMiscCfg,sizeof(ST_IOCTL_HVSP_MISC_CONFIG));
    }

    if(MDrv_HVSP_SetMiscConfig(&stMiscCfg))
    {
        return 0;
    }
    else
    {
        return -EFAULT;
    }
}

int _mdrv_ms_hvsp1_io_set_post_crop_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_POSTCROP_CONFIG stIOPostCfg;
    ST_MDRV_HVSP_POSTCROP_CONFIG stPostCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_POSTCROP_CONFIG),
        (((ST_IOCTL_HVSP_POSTCROP_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_POSTCROP_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_hvsp1_io_version_check(stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOPostCfg, (ST_IOCTL_HVSP_POSTCROP_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_POSTCROP_CONFIG)))
        {
            return -EFAULT;
        }
        else
        {
            stPostCfg.bCropEn = stIOPostCfg.bCropEn;
            stPostCfg.bFmCntEn = stIOPostCfg.bFmCntEn;
            stPostCfg.stclk = (ST_MDRV_HVSP_CLK_CONFIG *)&(_dev_ms_hvsp1.stclk);
            stPostCfg.u16Height = stIOPostCfg.u16Height;
            stPostCfg.u16Width = stIOPostCfg.u16Width;
            stPostCfg.u16X = stIOPostCfg.u16X;
            stPostCfg.u16Y = stIOPostCfg.u16Y;
            stPostCfg.u8FmCnt = stIOPostCfg.u8FmCnt;
        }
    }
    if(_ms_hvsp1_multiinstSet(E_MDRV_MULTI_INST_CMD_HVSP_POST_CROP_CONFIG, (void *)&stPostCfg, filp->private_data))
    {
        ret = -EFAULT;
    }
    else
    {
        if(!MDrv_HVSP_SetPostCropConfig(E_MDRV_HVSP_ID_1,  &stPostCfg))
        {
            ret = -EFAULT;
        }
        else
        {
            ret = 0;
        }
    }
    return ret;
}

int _mdrv_ms_hvsp1_io_get_private_id_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_PRIVATE_ID_CONFIG stCfg;

    if(!MDrv_MultiInst_Entry_GetPirvateId(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1, filp->private_data, &stCfg.s32Id))
    {
        return -EFAULT;
    }

    if(copy_to_user((ST_IOCTL_HVSP_PRIVATE_ID_CONFIG __user *)arg, &stCfg, sizeof(ST_IOCTL_HVSP_PRIVATE_ID_CONFIG)))
    {
       return -EFAULT;
    }

    return 0;
}

int _mdrv_ms_hvsp1_io_get_inform_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_SCINFORM_CONFIG stIOInfoCfg;
    ST_MDRV_HVSP_SCINFORM_CONFIG stInfoCfg;
    int Ret = 0;
    if(!MDrv_HVSP_GetSCLInform(E_MDRV_HVSP_ID_1,  &stInfoCfg))
    {
        Ret = -EFAULT;
    }
    else
    {
        memcpy(&stIOInfoCfg, &stInfoCfg, sizeof(ST_IOCTL_HVSP_SCINFORM_CONFIG));
        if(copy_to_user((ST_IOCTL_HVSP_SCINFORM_CONFIG __user *)arg, &stIOInfoCfg, sizeof(ST_IOCTL_HVSP_SCINFORM_CONFIG)))
        {
            Ret = -EFAULT;
        }
        else
        {
            Ret = 0;
        }
    }

    return Ret;
}

int _mdrv_ms_hvsp1_io_release_mem_config(struct file *filp, unsigned long arg)
{
    _ms_hvsp1_mem_free();
    return 0;
}

int _mdrv_ms_hvsp1_io_set_osd_config(struct file *filp, unsigned long arg)
{
    ST_MDRV_HVSP_OSD_CONFIG stOSDCfg;
    ST_IOCTL_HVSP_OSD_CONFIG stIOOSDCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_OSD_CONFIG),
        (((ST_IOCTL_HVSP_OSD_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_OSD_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_hvsp1_io_version_check(stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOOSDCfg, (ST_IOCTL_HVSP_OSD_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_OSD_CONFIG)))
        {
            return -EFAULT;
        }
        else
        {
            stOSDCfg.enOSD_loc = stIOOSDCfg.enOSD_loc;
            stOSDCfg.stOsdOnOff.bOSDEn = stIOOSDCfg.bEn;
        }
    }
    if(_ms_hvsp1_multiinstSet(E_MDRV_MULTI_INST_CMD_HVSP_SET_OSD_CONFIG, (void *)&stOSDCfg, filp->private_data))
    {
        ret = -EFAULT;
    }
    else
    {
        if(!MDrv_HVSP_SetOSDConfig(E_MDRV_HVSP_ID_1,  &stOSDCfg))
        {
            ret = -EFAULT;
        }
        else
        {
            ret = 0;
        }
    }
    return ret;
}

int _mdrv_ms_hvsp1_io_set_fb_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG stFbMgCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    stVersion = _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG),
        (((ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG __user *)arg)->VerChk_Version));
    if(_mdrv_ms_hvsp1_io_version_check(stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stFbMgCfg, (ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG __user *)arg, sizeof(ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG)))
        {
            return -EFAULT;
        }
    }
    if(!MDrv_HVSP_SetFbManageConfig((EN_MDRV_HVSP_FBMG_SET_TYPE)stFbMgCfg.enSet))
    {
        ret = -EFAULT;
    }
    else
    {
        ret = 0;
    }
    return ret;
}

int _mdrv_ms_hvsp1_io_get_version(struct file *filp, unsigned long arg)
{
    int ret = 0;

    if (CHK_VERCHK_HEADER( &(((ST_IOCTL_HVSP_VERSION_CONFIG __user *)arg)->VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS( &(((ST_IOCTL_HVSP_VERSION_CONFIG __user *)arg)->VerChk_Version), IOCTL_HVSP_VERSION) )
        {

            VERCHK_ERR("[HVSP] Version(%04x) < %04x!!! \n",
                ((ST_IOCTL_HVSP_VERSION_CONFIG __user *)arg)->VerChk_Version & VERCHK_VERSION_MASK,
                IOCTL_HVSP_VERSION);

            ret = -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &(((ST_IOCTL_HVSP_VERSION_CONFIG __user *)arg)->VerChk_Size), sizeof(ST_IOCTL_HVSP_VERSION_CONFIG)) == 0 )
            {
                VERCHK_ERR("[HVSP] Size(%04x) != %04x!!! \n",
                    sizeof(ST_IOCTL_HVSP_VERSION_CONFIG),
                    (((ST_IOCTL_HVSP_VERSION_CONFIG __user *)arg)->VerChk_Size));

                ret = -EINVAL;
            }
            else
            {
                ST_IOCTL_HVSP_VERSION_CONFIG stCfg;

                stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size, IOCTL_HVSP_VERSION);
                stCfg.u32Version = IOCTL_HVSP_VERSION;

                if(copy_to_user((ST_IOCTL_HVSP_VERSION_CONFIG __user *)arg, &stCfg, sizeof(ST_IOCTL_HVSP_VERSION_CONFIG)))
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
            VERCHK_ERR("[HVSP] No Header !!! \n");
            SCL_ERR( "[HVSP]%s  \n", __FUNCTION__);
        ret = -EINVAL;
    }

    return ret;
}

//----------------------------------------------------------------------------------------------


//==============================================================================
long mdrv_ms_hvsp1_ioctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg)
{
    int err = 0;
    int retval = 0;

    if(_dev_ms_hvsp1.refCnt <= 0)
    {
        SCL_ERR( "[HVSP1] HVSP1IO_IOCTL refCnt =%d!!! \n", _dev_ms_hvsp1.refCnt);
        return -EFAULT;
    }
    /* check u32Cmd valid */
    if(IOCTL_HVSP_MAGIC == _IOC_TYPE(u32Cmd))
    {
        if(_IOC_NR(u32Cmd) >= IOCTL_HVSP_MAX_NR)
        {
            SCL_ERR( "[HVSP1] IOCtl NR Error!!! (Cmd=%x)\n",u32Cmd);
            return -ENOTTY;
        }
    }
    else
    {
        SCL_ERR( "[HVSP1] IOCtl MAGIC Error!!! (Cmd=%x)\n",u32Cmd);
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

    SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_SC1, "[HVSP1] IOCTL_NUM:: == %s ==  \n", (CMD_PARSING(u32Cmd)));

    switch(u32Cmd)
    {
    case IOCTL_HVSP_SET_IN_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_input_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_SET_OUT_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_output_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_SET_SCALING_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_scaling_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_REQ_MEM_CONFIG:
        retval = _mdrv_ms_hvsp1_io_req_mem_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_SET_MISC_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_misc_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_SET_POST_CROP_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_post_crop_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_GET_PRIVATE_ID_CONFIG:
        retval = _mdrv_ms_hvsp1_io_get_private_id_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_GET_INFORM_CONFIG:
        retval = _mdrv_ms_hvsp1_io_get_inform_config(filp, u32Arg);
        break;
    case IOCTL_HVSP_RELEASE_MEM_CONFIG:
        retval = _mdrv_ms_hvsp1_io_release_mem_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_SET_OSD_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_osd_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_SET_FB_MANAGE_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_fb_config(filp, u32Arg);
        break;

    case IOCTL_HVSP_GET_VERSION_CONFIG:
        retval = _mdrv_ms_hvsp1_io_get_version(filp, u32Arg);
        break;
    default:  /* redundant, as cmd was checked against MAXNR */
        SCL_ERR( "[HVSP1] ERROR IOCtl number %x\n ",u32Cmd);
        retval = -ENOTTY;
        break;
    }

    return retval;
}


static unsigned int mdrv_ms_hvsp1_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int ret = 0;
    EN_MDRV_MULTI_INST_STATUS_TYPE enMultiInstRet;
    wait_queue_head_t *pWaitQueueHead = NULL;
    enMultiInstRet = MDrv_MultiInst_Etnry_IsFree(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1, filp->private_data);
    SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_SC1, "[HVSP1]start %s ret=%x\n",__FUNCTION__,ret);
    if(enMultiInstRet == E_MDRV_MULTI_INST_STATUS_SUCCESS)
    {
        pWaitQueueHead = MDrv_HVSP_GetWaitQueueHead();
        MDrv_HVSP_SetPollWait(filp, pWaitQueueHead, wait);
        if(MDrv_HVSP_GetCMDQDoneStatus())
        {
            ret = POLLIN;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        ret = 0;
    }
    return ret;
}


static int mdrv_ms_hvsp1_suspend(struct platform_device *dev, pm_message_t state)
{
    ST_MDRV_HVSP_SUSPEND_RESUME_CONFIG stHvspSuspendResumeCfg;
    int ret = 0;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);

    stHvspSuspendResumeCfg.u32IRQNum = SCL_IRQ_ID;
    stHvspSuspendResumeCfg.u32CMDQIRQNum = CMDQ_IRQ_ID;
    if(MDrv_HVSP_Suspend(E_MDRV_HVSP_ID_1, &stHvspSuspendResumeCfg))
    {
        MDrv_HVSP_IDCLKRelease(&_dev_ms_hvsp1.stclk);
        ret = 0;
    }
    else
    {
        ret = -EFAULT;
    }

    return ret;
}

static int mdrv_ms_hvsp1_resume(struct platform_device *dev)
{
    EN_MDRV_MULTI_INST_STATUS_TYPE enMultiInstRet = E_MDRV_MULTI_INST_STATUS_SUCCESS;
    ST_MDRV_HVSP_SUSPEND_RESUME_CONFIG stHvspSuspendResumeCfg;
    int ret = 0;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);

    stHvspSuspendResumeCfg.u32IRQNum     = SCL_IRQ_ID;
    stHvspSuspendResumeCfg.u32CMDQIRQNum = CMDQ_IRQ_ID;
    if(MDrv_HVSP_Resume(E_MDRV_HVSP_ID_1, &stHvspSuspendResumeCfg))
    {
        enMultiInstRet = MDrv_MultiInst_Entry_FlashData(
                            E_MDRV_MULTI_INST_ENTRY_ID_HVSP1,
                            NULL,
                            E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG,
                            NULL);

        if(enMultiInstRet != E_MDRV_MULTI_INST_STATUS_SUCCESS)
        {
            SCL_ERR( "[HVSP1] %s: Fail\n",__FUNCTION__);
            ret = -EINVAL;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        SCL_ERR( "[HVSP1] %s: Fail\n",__FUNCTION__);
        ret = -EFAULT;
    }

    return ret;
}


int mdrv_ms_hvsp1_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);

    SCL_ASSERT(_dev_ms_hvsp1.refCnt>=0);

    if(filp->private_data == NULL)
    {
        if(MDrv_MultiInst_Entry_Alloc(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1, &filp->private_data) == 0)
        {
            ret =  -EFAULT;
        }
    }

    _dev_ms_hvsp1.refCnt++;

    return ret;
}


int mdrv_ms_hvsp1_release(struct inode *inode, struct file *filp)
{

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);

    MDrv_MultiInst_Entry_Free(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1, filp->private_data);
    filp->private_data = NULL;
    _dev_ms_hvsp1.refCnt--;
    SCL_ASSERT(_dev_ms_hvsp1.refCnt>=0);
    if(_dev_ms_hvsp1.refCnt == 0)
    {
        MDrv_HVSP_Release(E_MDRV_HVSP_ID_1);
    }
    return 0;
}
#if CONFIG_OF
static int mdrv_ms_hvsp1_probe(struct platform_device *pdev)
{
    ST_MDRV_HVSP_INIT_CONFIG stHVSPInitCfg;
    unsigned char ret;
    int s32Ret;
    dev_t  dev;
    //struct resource *res_irq;
    //struct device_node *np;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s:%d\n",__FUNCTION__,__LINE__);
//mod init
    if(_dev_ms_hvsp1.s32Major)
    {
        dev     = MKDEV(_dev_ms_hvsp1.s32Major, _dev_ms_hvsp1.s32Minor);
        s32Ret  = register_chrdev_region(dev, MDRV_MS_HVSP_DEVICE_COUNT, MDRV_MS_HVSP_NAME);
    }
    else
    {
        s32Ret                  = alloc_chrdev_region(&dev, _dev_ms_hvsp1.s32Minor, MDRV_MS_HVSP_DEVICE_COUNT, MDRV_MS_HVSP_NAME);
        _dev_ms_hvsp1.s32Major  = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        SCL_ERR( "[HVSP1] Unable to get major %d\n", _dev_ms_hvsp1.s32Major);
        return s32Ret;
    }

    cdev_init(&_dev_ms_hvsp1.cdev, &_dev_ms_hvsp1.fops);
    if (0 != (s32Ret= cdev_add(&_dev_ms_hvsp1.cdev, dev, MDRV_MS_HVSP_DEVICE_COUNT)))
    {
        SCL_ERR( "[HVSP1] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_MS_HVSP_DEVICE_COUNT);
        return s32Ret;
    }
    m_hvsp1_class = msys_get_sysfs_class();
    if(!m_hvsp1_class)
    {
        m_hvsp1_class = class_create(THIS_MODULE, hvsp1_classname);
    }
    if(IS_ERR(m_hvsp1_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        _dev_ms_hvsp1.devicenode = device_create(m_hvsp1_class, NULL, dev,NULL, "mhvsp1");
        _dev_ms_hvsp1.devicenode->dma_mask=&ms_hvsp1_dma_mask;
        _dev_ms_hvsp1.devicenode->coherent_dma_mask=ms_hvsp1_dma_mask;
    }

//probe
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);
    //res_irq                     = platform_get_resource(pdev, IORESOURCE_IRQ, 0);// return NULL
    stHVSPInitCfg.u32Riubase    = 0x1F000000; //ToDo
    //if (res_irq)
    //{
      //SCL_IRQ_ID = res_irq->start;
      //SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] Get resource IORESOURCE_IRQ = 0x%x\n",SCL_IRQ_ID);
    //}
    //else
    {
      SCL_IRQ_ID  = of_irq_to_resource(pdev->dev.of_node, 0, NULL);
      CMDQ_IRQ_ID = of_irq_to_resource(pdev->dev.of_node, 1, NULL);
      SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] Get resource SCL_IRQ = 0x%x\n",SCL_IRQ_ID);
      SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] Get resource CMDQ_IRQ = 0x%x\n",CMDQ_IRQ_ID);
      if (!SCL_IRQ_ID)
      {
          SCL_ERR( "[HVSP1] Can't Get SCL_IRQ\n");
            return -EINVAL;
      }
      if (!CMDQ_IRQ_ID)
      {
          SCL_ERR( "[HVSP1] Can't Get CMDQ_IRQ\n");
            return -EINVAL;
      }
    }
    stHVSPInitCfg.u32IRQNUM     = SCL_IRQ_ID;
    stHVSPInitCfg.u32CMDQIRQNUM = CMDQ_IRQ_ID;
    if( MDrv_HVSP_Init(E_MDRV_HVSP_ID_1, &stHVSPInitCfg) == 0)
    {
        return -EFAULT;
    }
    //clk enable
    st_ms_hvsp1_device.dev.of_node = pdev->dev.of_node;
    _dev_ms_hvsp1.stclk.idclk = of_clk_get(st_ms_hvsp1_device.dev.of_node,0);
    _dev_ms_hvsp1.stclk.fclk1 = of_clk_get(st_ms_hvsp1_device.dev.of_node,1);
    _dev_ms_hvsp1.stclk.fclk2 = of_clk_get(st_ms_hvsp1_device.dev.of_node,2);
    _dev_ms_hvsp1.stclk.odclk = of_clk_get(st_ms_hvsp1_device.dev.of_node,3);
    if (IS_ERR(_dev_ms_hvsp1.stclk.idclk) || IS_ERR(_dev_ms_hvsp1.stclk.fclk1) || IS_ERR(_dev_ms_hvsp1.stclk.fclk2)|| IS_ERR(_dev_ms_hvsp1.stclk.odclk))
    {
        SCL_ERR( "[HVSP1] Can't Get CLK\n");
        return 0 ;
    }
    //create device
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_ptgen);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_monitorHW);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_clk);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_proc);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_dbgmg);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_osd);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_fbmg);

    Reset_DBGMG_FLAG();
    if (ret != 0)
    {
      dev_err(&pdev->dev,
       "Failed to create ptgen_call sysfs files: %d\n", ret);
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] attr ok\n");
    }
    gbProbeAlready |= EN_DBG_HVSP1_CONFIG;
    MDrv_MultiInst_Entry_Init_Variable(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1);
    return 0;
}
static int mdrv_ms_hvsp1_remove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);
    _ms_hvsp1_mem_free();
    MDrv_HVSP_IDCLKRelease(&_dev_ms_hvsp1.stclk);
    cdev_del(&_dev_ms_hvsp1.cdev);
    device_destroy(m_hvsp1_class, MKDEV(_dev_ms_hvsp1.s32Major, _dev_ms_hvsp1.s32Minor));
    class_destroy(m_hvsp1_class);
    unregister_chrdev_region(MKDEV(_dev_ms_hvsp1.s32Major, _dev_ms_hvsp1.s32Minor), MDRV_MS_HVSP_DEVICE_COUNT);
    return 0;
}
#else
static int mdrv_ms_hvsp1_probe(struct platform_device *pdev)
{
    ST_MDRV_HVSP_INIT_CONFIG stHVSPInitCfg;
    unsigned char ret;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);
    SCL_IRQ_ID = SCLIRQ_ID;
    CMDQ_IRQ_ID = CMDQIRQ_ID;
    stHVSPInitCfg.u32IRQNUM     = SCL_IRQ_ID;
    stHVSPInitCfg.u32CMDQIRQNUM = CMDQ_IRQ_ID;
    if( MDrv_HVSP_Init(E_MDRV_HVSP_ID_1, &stHVSPInitCfg) == 0)
    {
        return -EFAULT;
    }
    //clk enable
    //no device tree use hardcode
    //create device

    ret = device_create_file(&pdev->dev, &dev_attr_ptgen);
    ret = device_create_file(&pdev->dev, &dev_attr_monitorHW);
    ret = device_create_file(&pdev->dev, &dev_attr_clk);
    ret = device_create_file(&pdev->dev, &dev_attr_proc);
    ret = device_create_file(&pdev->dev, &dev_attr_dbgmg);
    Reset_DBGMG_FLAG();
    if (ret != 0)
    {
      dev_err(&pdev->dev,
       "Failed to create ptgen_call sysfs files: %d\n", ret);
    }
    MDrv_MultiInst_Entry_Init_Variable(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1);
    return 0;
}
static int mdrv_ms_hvsp1_remove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);
    _ms_hvsp1_mem_free();
    MDrv_HVSP_IDCLKRelease(&_dev_ms_hvsp1.stclk);

    return 0;
}

#endif


//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
#if CONFIG_OF
int _mdrv_ms_hvsp1_init(void)
{
    int ret = 0;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s:%d\n",__FUNCTION__,__LINE__);
    ret = platform_driver_register(&st_ms_hvsp1_driver);
    if (!ret)
    {
        SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] platform_driver_register success\n");
        if(gbProbeAlready&EN_DBG_HVSP1_CONFIG)
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] Probe success\n");
        }
        else
        {
            SCL_ERR( "[HVSP1] Probe Fail\n");
        }
        if(gbProbeAlready==EN_DBG_SCL_CONFIG)
        {
            SCL_ERR( "[SCL] SCL init success\n");
        }
    }
    else
    {
        SCL_ERR( "[HVSP1] platform_driver_register failed\n");
        platform_driver_unregister(&st_ms_hvsp1_driver);
    }


    return ret;
}
void _mdrv_ms_hvsp1_exit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);
    platform_driver_unregister(&st_ms_hvsp1_driver);
}
#else

int _mdrv_ms_hvsp1_init(void)
{
    int ret = 0;
    int s32Ret;
    dev_t  dev;
    //struct device_node *np;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);
    //np = of_find_compatible_node(NULL, NULL, "mstar,hvsp1");
    //if (np)
    //{
    //  SCL_DBG(SCL_DBG_LV_MDRV_IO(), "Find scl dts node\n");
    //  st_ms_hvsp1_device.dev.of_node = of_node_get(np);
    //  of_node_put(np);
    //}
    //else
    //{
    //    return -ENODEV;
    //}

    if(_dev_ms_hvsp1.s32Major)
    {
        dev     = MKDEV(_dev_ms_hvsp1.s32Major, _dev_ms_hvsp1.s32Minor);
        s32Ret  = register_chrdev_region(dev, MDRV_MS_HVSP_DEVICE_COUNT, MDRV_MS_HVSP_NAME);
    }
    else
    {
        s32Ret                  = alloc_chrdev_region(&dev, _dev_ms_hvsp1.s32Minor, MDRV_MS_HVSP_DEVICE_COUNT, MDRV_MS_HVSP_NAME);
        _dev_ms_hvsp1.s32Major  = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        SCL_ERR( "[HVSP1] Unable to get major %d\n", _dev_ms_hvsp1.s32Major);
        return s32Ret;
    }

    cdev_init(&_dev_ms_hvsp1.cdev, &_dev_ms_hvsp1.fops);
    if (0 != (s32Ret= cdev_add(&_dev_ms_hvsp1.cdev, dev, MDRV_MS_HVSP_DEVICE_COUNT)))
    {
        SCL_ERR( "[HVSP1] Unable add a character device\n");
        unregister_chrdev_region(dev, MDRV_MS_HVSP_DEVICE_COUNT);
        return s32Ret;
    }

    m_hvsp1_class = class_create(THIS_MODULE, hvsp1_classname);
    if(IS_ERR(m_hvsp1_class))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        device_create(m_hvsp1_class, NULL, dev,NULL, "mhvsp1");
    }

    ret = platform_driver_register(&st_ms_hvsp1_driver);

    if (!ret)
    {
        ret = platform_device_register(&st_ms_hvsp1_device);
        if (ret)    // if register device fail, then unregister the driver.
        {
            platform_driver_unregister(&st_ms_hvsp1_driver);
            SCL_ERR( "[HVSP1] platform_driver_register failed\n");

        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] platform_driver_register success\n");
        }
    }


    return ret;
}
void _mdrv_ms_hvsp1_exit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);

    cdev_del(&_dev_ms_hvsp1.cdev);
    device_destroy(m_hvsp1_class, MKDEV(_dev_ms_hvsp1.s32Major, _dev_ms_hvsp1.s32Minor));
    class_destroy(m_hvsp1_class);
    unregister_chrdev_region(MKDEV(_dev_ms_hvsp1.s32Major, _dev_ms_hvsp1.s32Minor), MDRV_MS_HVSP_DEVICE_COUNT);
    platform_driver_unregister(&st_ms_hvsp1_driver);
}

#endif


module_init(_mdrv_ms_hvsp1_init);
module_exit(_mdrv_ms_hvsp1_exit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("ms hvsp1 ioctrl driver");
MODULE_LICENSE("GPL");

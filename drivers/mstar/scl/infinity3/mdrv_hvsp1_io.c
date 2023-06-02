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
#include "mdrv_hvsp_io_i3_st.h"
#include "mdrv_hvsp_io_i3.h"
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
                         x==IOCTL_HVSP_GET_INFORM_CONFIG ?  "IOCTL_HVSP_GET_INFORM_CONFIG" : \
                         x==IOCTL_HVSP_SET_PRIMASK_CONFIG ?  "IOCTL_HVSP_SET_PRIMASK_CONFIG" : \
                         x==IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG ?  "IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG" : \
                                                                "UNKNOWN")

#define FHDWidth   1920
#define FHDHeight  1080
#define _3MWidth   2048
#define _3MHeight  1536
//-------------------------------------------------------------------------------------------------

#define _ms_hvsp1_mem_bus_to_miu(x) (x-0x2000000)
#define CMDQIRQ_ID
#define SCLIRQ_ID
#define CIIROPEN gbCIIR
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
static u_long sg_hvsp1_mcnr_size = 1920*1080*2*2 ;
dma_addr_t  sg_hvsp1_mcnr_yc_bus_addr = 0;
static void *sg_hvsp1_mcnr_yc_vir_addr = NULL;
const static char* KEY_DMEM_SCL_MCNR_YC="SCL_MCNR_YC";
dma_addr_t  sg_hvsp1_mcnr_ciir_bus_addr = 0;
static void *sg_hvsp1_mcnr_ciir_vir_addr = NULL;
const static char* KEY_DMEM_SCL_MCNR_CIIR="SCL_MCNR_CIIR";
dma_addr_t  sg_hvsp1_mcnr_m_bus_addr = 0;
static void *sg_hvsp1_mcnr_m_vir_addr = NULL;
const static char* KEY_DMEM_SCL_MCNR_M="SCL_MCNR_M";
unsigned int gu8FrameBufferReadyNum=0;//extern
unsigned char gbdbgmessage[EN_DBGMG_NUM_CONFIG];//extern
#if CONFIG_OF
unsigned int gbProbeAlready = 0;//extern
#endif
dma_addr_t sg_hvsp1_release_mcnr_size;
unsigned char gbCIIR;
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
    .probe      = mdrv_ms_hvsp1_probe,
    .remove     = mdrv_ms_hvsp1_remove,
    .suspend    = mdrv_ms_hvsp1_suspend,
    .resume     = mdrv_ms_hvsp1_resume,
    .driver =
    {
        .name   = MDRV_NAME_HVSP,
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
MS_U8 gu8first;

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
        else if((int)*str == 69)  //input E
        {
            SCL_ERR( "[CLK]fclk1 216 %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk1,2);
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
            SCL_ERR( "[CLK]fclk2 172 %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk2,0);
        }
        else if((int)*str == 55)  //input 7
        {
            SCL_ERR( "[CLK]fclk2 86 %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk2,1);
        }
        else if((int)*str == 70)  //input F
        {
            SCL_ERR( "[CLK]fclk2 216 %d\n",(int)*str);
            MDrv_HVSP_SetCLKRate(_dev_ms_hvsp1.stclk.fclk2,2);
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
static ssize_t check_od_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_OdShow(buf);
}
static DEVICE_ATTR(od,0444, check_od_show, NULL);
static ssize_t check_fbmg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    const char *str = buf;
    if(NULL!=buf)
    {
        //if(!)
        if((int)*str == 49)    //input 1
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_LDCPATH_ON);
        }
        else if((int)*str == 50)  //input 2
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_LDCPATH_OFF);
        }
        else if((int)*str == 51)  //input 3
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_DNR_Read_ON);
        }
        else if((int)*str == 52)  //input 4
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_DNR_Read_OFF);
        }
        else if((int)*str == 53)  //input 5
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_DNR_Write_ON);
        }
        else if((int)*str == 54)  //input 6
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_DNR_Write_OFF);
        }
        else if((int)*str == 55)  //input 7
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_DNR_BUFFER_1);
        }
        else if((int)*str == 56)  //input 8
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_DNR_BUFFER_2);
        }
        else if((int)*str == 57)  //input 9
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_UNLOCK);
        }
        else if((int)*str == 65)  //input A
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_PRVCROP_ON);
        }
        else if((int)*str == 66)  //input B
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_PRVCROP_OFF);
        }
        else if((int)*str == 67)  //input C
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            gbCIIR = 1;
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_CIIR_ON);
        }
        else if((int)*str == 68)  //input D
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            gbCIIR = 0;
            MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_CIIR_OFF);
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

static ssize_t check_SCIQ_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    MDrv_HVSP_SCIQStore(buf,E_MDRV_HVSP_ID_1);
    return n;
}
static ssize_t check_SCIQ_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_SCIQShow(buf,E_MDRV_HVSP_ID_1);
}

static DEVICE_ATTR(SCIQ,0644, check_SCIQ_show, check_SCIQ_store);

static ssize_t check_proc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_ProcShow(buf);
}
static DEVICE_ATTR(proc,0444, check_proc_show, NULL);

static ssize_t check_ints_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_IntsShow(buf);
}
static DEVICE_ATTR(ints,0444, check_ints_show, NULL);


static ssize_t check_dbgmg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_DbgmgFlagShow(buf);
}
static unsigned char _mdrv_hvsp1_Changebuf2hex(int u32num)
{
    unsigned char u8level = 0;
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
static ssize_t check_lock_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrv_HVSP_LockShow(buf);
}
static ssize_t check_lock_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        const char *str = buf;
        if((int)*str == 76)    //input 1  echo L >
        {
            if(!MsOS_ReleaseMutexAll())
            {
                SCL_DBGERR("[HVSP]!!!!!!!!!!!!!!!!!!! HVSP Release Mutex fail\n");
            }
        }
        return n;
    }
    return 0;
}

static DEVICE_ATTR(mutex,0644, check_lock_show, check_lock_store);

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
        gu8FrameBufferReadyNum = (Mdrv_HVSP_GetFrameBufferCountInformation());
        sg_hvsp1_release_mcnr_size       = sg_hvsp1_mcnr_size;
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

void _ms_hvsp1_mem_freeYCbuffer(void)
{
    if(sg_hvsp1_mcnr_yc_vir_addr != 0)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP1] YC free\n");
        _ms_hvsp1_FreeDmem(KEY_DMEM_SCL_MCNR_YC,
                  PAGE_ALIGN(sg_hvsp1_mcnr_size),
                  sg_hvsp1_mcnr_yc_vir_addr,
                  sg_hvsp1_mcnr_yc_bus_addr);

        sg_hvsp1_mcnr_yc_vir_addr = 0;
        sg_hvsp1_mcnr_yc_bus_addr = 0;
        gu8FrameBufferReadyNum = 0;
    }
}
void _ms_hvsp1_mem_freeMbuffer(void)
{
    if(sg_hvsp1_mcnr_m_vir_addr != 0)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP1] Motion free\n");
        _ms_hvsp1_FreeDmem(KEY_DMEM_SCL_MCNR_M,
                  PAGE_ALIGN(sg_hvsp1_mcnr_size/4),
                  sg_hvsp1_mcnr_m_vir_addr,
                  sg_hvsp1_mcnr_m_bus_addr);

        sg_hvsp1_mcnr_m_vir_addr = 0;
        sg_hvsp1_mcnr_m_bus_addr = 0;
    }
}
void _ms_hvsp1_mem_freeCIIRbuffer(void)
{
    if(sg_hvsp1_mcnr_ciir_vir_addr != 0)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP1] CIIR free\n");
        _ms_hvsp1_FreeDmem(KEY_DMEM_SCL_MCNR_CIIR,
                  PAGE_ALIGN(sg_hvsp1_mcnr_size/2),
                  sg_hvsp1_mcnr_ciir_vir_addr,
                  sg_hvsp1_mcnr_ciir_bus_addr);

        sg_hvsp1_mcnr_ciir_vir_addr = 0;
        sg_hvsp1_mcnr_ciir_bus_addr = 0;
    }
}

static int _ms_hvsp1_mem_allocate(void)
{
    SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP1] allocate memory\n");

    if (!(sg_hvsp1_mcnr_yc_vir_addr = _ms_hvsp1_AllocDmem(KEY_DMEM_SCL_MCNR_YC,
                                             PAGE_ALIGN(sg_hvsp1_mcnr_size),
                                             &sg_hvsp1_mcnr_yc_bus_addr)))
    {
        SCL_ERR( "%s: unable to allocate YC memory\n", __FUNCTION__);
        return 0;
    }
    if (!(sg_hvsp1_mcnr_m_vir_addr = _ms_hvsp1_AllocDmem(KEY_DMEM_SCL_MCNR_M,
                                             PAGE_ALIGN(sg_hvsp1_mcnr_size/4),
                                             &sg_hvsp1_mcnr_m_bus_addr)))
    {
        SCL_ERR( "%s: unable to allocate Montion memory\n", __FUNCTION__);
        _ms_hvsp1_mem_freeYCbuffer();
        return 0;
    }
    if(CIIROPEN)
    {
        if (!(sg_hvsp1_mcnr_ciir_vir_addr = _ms_hvsp1_AllocDmem(KEY_DMEM_SCL_MCNR_CIIR,
                                                 PAGE_ALIGN(sg_hvsp1_mcnr_size/2),
                                                 &sg_hvsp1_mcnr_ciir_bus_addr)))
        {
            SCL_ERR( "%s: unable to allocate CIIR memory\n", __FUNCTION__);
            return 0;
        }
        else
        {
            gu8FrameBufferReadyNum |= (Mdrv_HVSP_GetFrameBufferCountInformation()<<2);
        }
    }

    SCL_ERR( "[HVSP1]: MCNR YC: Phy:%x  Vir:%x\n", sg_hvsp1_mcnr_yc_bus_addr, (u32)sg_hvsp1_mcnr_yc_vir_addr);
    SCL_ERR( "[HVSP1]: MCNR CIIR: Phy:%x  Vir:%x\n", sg_hvsp1_mcnr_ciir_bus_addr, (u32)sg_hvsp1_mcnr_ciir_vir_addr);
    SCL_ERR( "[HVSP1]: MCNR M: Phy:%x  Vir:%x\n", sg_hvsp1_mcnr_m_bus_addr, (u32)sg_hvsp1_mcnr_m_vir_addr);
    return 1;
}

static void _ms_hvsp1_mem_free(void)
{
    _ms_hvsp1_mem_freeYCbuffer();
    _ms_hvsp1_mem_freeMbuffer();
    _ms_hvsp1_mem_freeCIIRbuffer();
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
void _mdrv_ms_hvsp1_io_fill_versionchkstruct
(unsigned int u32StructSize,unsigned int u32VersionSize,unsigned int *pVersion,ST_MDRV_HVSP_VERSIONCHK_CONFIG *stVersion)
{
    stVersion->u32StructSize  = (unsigned int)u32StructSize;
    stVersion->u32VersionSize = (unsigned int)u32VersionSize;
    stVersion->pVersion      = (unsigned int *)pVersion;
}
int _mdrv_ms_hvsp1_io_version_check(ST_MDRV_HVSP_VERSIONCHK_CONFIG *stVersion)
{
    if ( CHK_VERCHK_HEADER(stVersion->pVersion) )
    {
        if( CHK_VERCHK_MAJORVERSION_LESS( stVersion->pVersion, IOCTL_HVSP_VERSION) )
        {

            VERCHK_ERR("[HVSP1] Version(%04x) < %04x!!! \n",
                *(stVersion->pVersion) & VERCHK_VERSION_MASK,
                IOCTL_HVSP_VERSION);

            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &stVersion->u32VersionSize, stVersion->u32StructSize) == 0 )
            {
                VERCHK_ERR("[HVSP1] Size(%04x) != %04x!!! \n",
                    stVersion->u32StructSize,
                    stVersion->u32VersionSize);

                return -EINVAL;
            }
            else
            {
                SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_ELSE, "[HVSP1] Size(%d) \n",stVersion->u32StructSize );
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
    if(Mdrv_HVSP_GetFrameBufferCountInformation()== 1)
    {
        SCL_ERR(
            "[HVSP1] Buffer is single, Vsize=%d, Pitch=%d\n",stReqMemCfg->u16Vsize, stReqMemCfg->u16Pitch);
        stReqMemCfg->u32MemSize = stReqMemCfg->u32MemSize /2 ;
    }
    sg_hvsp1_mcnr_size = stReqMemCfg->u32MemSize;
    if(Mdrv_HVSP_GetFrameBufferCountInformation()== 1)
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

void _mdrv_hvsp1_FrameBufferMemoryAllocate(void)
{
    if(gu8FrameBufferReadyNum == 0)
    {
        _ms_hvsp1_mem_allocate();
        MDrv_HVSP_SetMemoryAllocateReady(gu8FrameBufferReadyNum);
    }
    else if(gu8FrameBufferReadyNum != 0 && sg_hvsp1_mcnr_size > sg_hvsp1_release_mcnr_size)
    {
        _ms_hvsp1_mem_free();
        gu8FrameBufferReadyNum = 0;
        _ms_hvsp1_mem_allocate();
        MDrv_HVSP_SetMemoryAllocateReady((gu8FrameBufferReadyNum > 0) ? 1 :0);
    }
}
void _mdrv_hvsp1_FillIPMStructForDriver(ST_IOCTL_HVSP_REQ_MEM_CONFIG *stReqMemCfg,ST_MDRV_HVSP_IPM_CONFIG *stIPMCfg)
{
    stIPMCfg->u16Height = stReqMemCfg->u16Vsize;
    stIPMCfg->u16Width  = stReqMemCfg->u16Pitch;
    stIPMCfg->u32MemSize = stReqMemCfg->u32MemSize;
    if(gu8FrameBufferReadyNum & 0x3)
    {
        if((stIPMCfg->u16Height <= (FHDWidth)) && (stIPMCfg->u16Width <= FHDWidth))
        {
            stIPMCfg->enRW       = E_MDRV_HVSP_MCNR_YCM_W;
        }
        else
        {
            stIPMCfg->enRW       = E_MDRV_HVSP_MCNR_YCM_RW;
        }
        sg_hvsp1_mcnr_size   = stReqMemCfg->u32MemSize;
    }
    else
    {
        stIPMCfg->enRW       = E_MDRV_HVSP_MCNR_NON;
        sg_hvsp1_mcnr_size   = stReqMemCfg->u32MemSize;
    }
    if(sg_hvsp1_mcnr_yc_bus_addr)
    {
        stIPMCfg->u32YCPhyAddr = Chip_Phys_to_MIU(sg_hvsp1_mcnr_yc_bus_addr);
    }
    if(sg_hvsp1_mcnr_m_bus_addr)
    {
        stIPMCfg->u32MPhyAddr = Chip_Phys_to_MIU(sg_hvsp1_mcnr_m_bus_addr);
    }
    if(sg_hvsp1_mcnr_ciir_bus_addr)
    {
        stIPMCfg->u32CIIRPhyAddr = Chip_Phys_to_MIU(sg_hvsp1_mcnr_ciir_bus_addr);
    }
}


int _mdrv_ms_hvsp1_io_set_input_config(struct file *filp, unsigned long arg)
{
    ST_MDRV_HVSP_INPUT_CONFIG stInCfg;
    ST_IOCTL_HVSP_INPUT_CONFIG stIOInCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_INPUT_CONFIG),
        (((ST_IOCTL_HVSP_INPUT_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_INPUT_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOInCfg, (__user ST_IOCTL_HVSP_INPUT_CONFIG *)arg, sizeof(ST_IOCTL_HVSP_INPUT_CONFIG)))
        {
            return -EFAULT;
        }
        else
        {
            stInCfg.enColor       = (EN_MDRV_HVSP_COLOR_TYPE)stIOInCfg.enColor;
            stInCfg.enSrcType     = (EN_MDRV_HVSP_SRC_TYPE)stIOInCfg.enSrcType;
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

     _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_OUTPUT_CONFIG),
        (((ST_IOCTL_HVSP_OUTPUT_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_OUTPUT_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stOutCfg, (__user ST_IOCTL_HVSP_OUTPUT_CONFIG *)arg, sizeof(ST_IOCTL_HVSP_OUTPUT_CONFIG)))
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

     _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_SCALING_CONFIG),
        (((ST_IOCTL_HVSP_SCALING_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_SCALING_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOSclCfg, (__user ST_IOCTL_HVSP_SCALING_CONFIG *)arg, sizeof(ST_IOCTL_HVSP_SCALING_CONFIG)))
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
        MDrv_HVSP_SetPriMaskInstId(MDrv_MultiInst_GetHvspQuantifyPreInstId());
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
    MsOS_Memset(&stReqMemCfg,0,sizeof(ST_IOCTL_HVSP_REQ_MEM_CONFIG));
    MsOS_Memset(&stIPMCfg,0,sizeof(ST_MDRV_HVSP_IPM_CONFIG));
     _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_REQ_MEM_CONFIG),
        (((ST_IOCTL_HVSP_REQ_MEM_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_REQ_MEM_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stReqMemCfg, (__user ST_IOCTL_HVSP_REQ_MEM_CONFIG *)arg, sizeof(ST_IOCTL_HVSP_REQ_MEM_CONFIG)))
        {
            return -EFAULT;
        }
    }
    ret = _mdrv_hvsp1_CheckModifyMemSize(&stReqMemCfg);
    _mdrv_hvsp1_FrameBufferMemoryAllocate();
    _mdrv_hvsp1_FillIPMStructForDriver(&stReqMemCfg,&stIPMCfg);
    if(_ms_hvsp1_multiinstSet(E_MDRV_MULTI_INST_CMD_HVSP_MEM_REQ_CONFIG, (void *)&stIPMCfg, filp->private_data))
    {
        return -EFAULT;
    }
    else
    {
        if(!MDrv_HVSP_SetInitIPMConfig(E_MDRV_HVSP_ID_1, &stIPMCfg))
        {
            return -EFAULT;
        }
    }
    if(gu8FrameBufferReadyNum == 0)
    {
        return -EFAULT;
    }
    return ret;
}

int _mdrv_ms_hvsp1_io_set_misc_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_MISC_CONFIG stIOMiscCfg;
    ST_MDRV_HVSP_MISC_CONFIG stMiscCfg;
    MsOS_Memset(&stIOMiscCfg,0,sizeof(ST_IOCTL_HVSP_MISC_CONFIG));
    MsOS_Memset(&stMiscCfg,0,sizeof(ST_MDRV_HVSP_MISC_CONFIG));
    if(copy_from_user(&stIOMiscCfg, (__user ST_IOCTL_HVSP_MISC_CONFIG *)arg, sizeof(ST_IOCTL_HVSP_MISC_CONFIG)))
    {
        return -EFAULT;
    }
    else
    {
        if(stIOMiscCfg.u32Addr && stIOMiscCfg.u32Size>0 && stIOMiscCfg.u32Size< 0xFFFF)
        {
            stMiscCfg.u32Addr = stIOMiscCfg.u32Addr;
            stMiscCfg.u32Size = stIOMiscCfg.u32Size;
            stMiscCfg.u8Cmd = stIOMiscCfg.u8Cmd;
        }
        else
        {
            return -EFAULT;
        }
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

     _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_POSTCROP_CONFIG),
        (((ST_IOCTL_HVSP_POSTCROP_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_POSTCROP_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOPostCfg, (__user ST_IOCTL_HVSP_POSTCROP_CONFIG  *)arg, sizeof(ST_IOCTL_HVSP_POSTCROP_CONFIG)))
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
        MDrv_HVSP_SetPriMaskInstId(MDrv_MultiInst_GetHvspQuantifyPreInstId());
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

     _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_OSD_CONFIG),
        (((ST_IOCTL_HVSP_OSD_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_OSD_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOOSDCfg, (__user ST_IOCTL_HVSP_OSD_CONFIG  *)arg, sizeof(ST_IOCTL_HVSP_OSD_CONFIG)))
        {
            return -EFAULT;
        }
        else
        {
            stOSDCfg.enOSD_loc = (EN_MDRV_HVSP_OSD_LOC_TYPE)stIOOSDCfg.enOSD_loc;
            stOSDCfg.stOsdOnOff.bOSDEn = stIOOSDCfg.bEn;
            stOSDCfg.stOsdOnOff.bOSDBypass = stIOOSDCfg.bOSDBypass;
            stOSDCfg.stOsdOnOff.bWTMBypass = stIOOSDCfg.bWTMBypass;
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

int _mdrv_ms_hvsp1_io_set_primask_config(struct file *filp, unsigned long arg)
{
    ST_MDRV_HVSP_PRIMASK_CONFIG stCfg;
    ST_IOCTL_HVSP_PRIMASK_CONFIG stIOCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;
    EN_MDRV_MULTI_INST_STATUS_TYPE enMultiInstRet;
    MsOS_Memset(&stIOCfg,0,sizeof(ST_IOCTL_HVSP_PRIMASK_CONFIG));
    MsOS_Memset(&stCfg,0,sizeof(ST_MDRV_HVSP_PRIMASK_CONFIG));
     _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_PRIMASK_CONFIG),
        (((ST_IOCTL_HVSP_PRIMASK_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_PRIMASK_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    if(copy_from_user(&stIOCfg, (__user ST_IOCTL_HVSP_PRIMASK_CONFIG  *)arg, sizeof(ST_IOCTL_HVSP_PRIMASK_CONFIG)))
    {
        return -EFAULT;
    }
    else
    {
        stCfg.bMask = stIOCfg.bMask;
        if(stIOCfg.u8idx>= PriMaskNum)
        {
            return -EINVAL;
        }
        stCfg.u8idx = stIOCfg.u8idx;
        memcpy(&stCfg.stMaskWin,&stIOCfg.stMaskWin,sizeof(ST_IOCTL_HVSP_WINDOW_CONFIG));
    }
    enMultiInstRet = MDrv_MultiInst_Etnry_IsFree(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1, filp->private_data);
    if(enMultiInstRet == E_MDRV_MULTI_INST_STATUS_SUCCESS)
    {
        MDrv_HVSP_SetPriMaskInstId(MDrv_MultiInst_GetHvspQuantifyPreInstId());
        if(!MDrv_HVSP_SetPriMaskConfig(&stCfg))
        {
            ret = -EFAULT;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        ret = -EFAULT;
    }
    return ret;
}

int _mdrv_ms_hvsp1_io_primask_trigger_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG stIOCfg;
    unsigned char bEn;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

     _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG),
        (((ST_IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stIOCfg, (__user ST_IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG  *)arg, sizeof(ST_IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG)))
        {
            return -EFAULT;
        }
        else
        {
            bEn= stIOCfg.bEn;
        }
    }
    if(!MDrv_HVSP_SetPriMaskTrigger(bEn))
    {
        ret = -EFAULT;
    }
    else
    {
        ret = 0;
    }
    return ret;
}

int _mdrv_ms_hvsp1_io_set_fb_config(struct file *filp, unsigned long arg)
{
    ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG stFbMgCfg;
    int ret = 0;
    ST_MDRV_HVSP_VERSIONCHK_CONFIG stVersion;

    _mdrv_ms_hvsp1_io_fill_versionchkstruct(sizeof(ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG),
        (((ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG __user *)arg)->VerChk_Size),
        &(((ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG __user *)arg)->VerChk_Version),&stVersion);
    if(_mdrv_ms_hvsp1_io_version_check(&stVersion))
    {
        SCL_ERR( "[HVSP1]   %s  \n", __FUNCTION__);
        return -EINVAL;
    }
    else
    {
        if(copy_from_user(&stFbMgCfg, (__user ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG  *)arg, sizeof(ST_IOCTL_HVSP_SET_FB_MANAGE_CONFIG)))
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
    case IOCTL_HVSP_SET_PRIMASK_CONFIG:
        retval = _mdrv_ms_hvsp1_io_set_primask_config(filp, u32Arg);
        break;
    case IOCTL_HVSP_PRIMASK_TRIGGER_CONFIG:
        retval = _mdrv_ms_hvsp1_io_primask_trigger_config(filp, u32Arg);
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
        pWaitQueueHead = (wait_queue_head_t *)MDrv_HVSP_GetWaitQueueHead();
        MDrv_HVSP_SetPollWait(filp, pWaitQueueHead, wait);
        if(gu8first)
        {
            ret = POLLIN;
            gu8first = 0;
        }
        else if(MDrv_HVSP_GetCMDQDoneStatus(E_MDRV_HVSP_ID_1))
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

    stHvspSuspendResumeCfg.u32IRQNum = MsOS_GetIrqIDSCL(E_SCLIRQ_SC0);
    stHvspSuspendResumeCfg.u32CMDQIRQNum = MsOS_GetIrqIDCMDQ(E_CMDQIRQ_CMDQ0);
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
    EN_MDRV_MULTI_INST_STATUS_TYPE enMultiInstRet;
    ST_MDRV_HVSP_SUSPEND_RESUME_CONFIG stHvspSuspendResumeCfg;
    int ret = 0;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);

    stHvspSuspendResumeCfg.u32IRQNum     = MsOS_GetIrqIDSCL(E_SCLIRQ_SC0);
    stHvspSuspendResumeCfg.u32CMDQIRQNum = MsOS_GetIrqIDCMDQ(E_CMDQIRQ_CMDQ0);
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

    SCL_ASSERT(_dev_ms_hvsp1.refCnt>=0);

    if(filp->private_data == NULL)
    {
        if(MDrv_MultiInst_Entry_Alloc(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1, &filp->private_data) == 0)
        {
            ret =  -EFAULT;
        }
    }
    if(_dev_ms_hvsp1.refCnt == 0)
    {
        MDrv_HVSP_Open(E_MDRV_HVSP_ID_1);
    }
    if(!ret)
    {
        _dev_ms_hvsp1.refCnt++;
    }
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s:%d\n",__FUNCTION__,_dev_ms_hvsp1.refCnt);
    return ret;
}


int mdrv_ms_hvsp1_release(struct inode *inode, struct file *filp)
{


    MDrv_MultiInst_Entry_Free(E_MDRV_MULTI_INST_ENTRY_ID_HVSP1, filp->private_data);
    filp->private_data = NULL;
    _dev_ms_hvsp1.refCnt--;
    SCL_ASSERT(_dev_ms_hvsp1.refCnt>=0);
    if(_dev_ms_hvsp1.refCnt == 0)
    {
        MDrv_HVSP_IDCLKRelease(&_dev_ms_hvsp1.stclk);
        MDrv_HVSP_Release(E_MDRV_HVSP_ID_1);
        gu8first = 1;
    }
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s:%d\n",__FUNCTION__,_dev_ms_hvsp1.refCnt);
    return 0;
}
#if CONFIG_OF
static int mdrv_ms_hvsp1_probe(struct platform_device *pdev)
{
    ST_MDRV_HVSP_INIT_CONFIG stHVSPInitCfg;
    unsigned char ret;
    int s32Ret;
    unsigned int u32Dropmode;
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
    if(of_property_read_u32(pdev->dev.of_node, "DigitalZoom-Dropmode", &u32Dropmode))
    {
        printk(KERN_WARNING "[HVSP] Failed to read CMDQ-mode property, default on/n");
        u32Dropmode = 1;  //if can't get, default on
    }
    if(u32Dropmode)
    {
        printk(KERN_WARNING "[HVSP]u32Dropmode on/n");
        MsOS_SetHVSPDigitalZoomMode(u32Dropmode);
    }
    else
    {
        printk(KERN_WARNING "[HVSP]u32Dropmode off/n");
        MsOS_SetHVSPDigitalZoomMode(u32Dropmode);
    }
    //res_irq                     = platform_get_resource(pdev, IORESOURCE_IRQ, 0);// return NULL
    stHVSPInitCfg.u32Riubase    = 0x1F000000; //ToDo
    //if (res_irq)
    //{
      //SCL_IRQ_ID = res_irq->start;
      //SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] Get resource IORESOURCE_IRQ = 0x%x\n",SCL_IRQ_ID);
    //}
    //else
    MsOS_SetSclIrqIDFormSys(pdev,0,E_SCLIRQ_SC0);
    MsOS_SetCmdqIrqIDFormSys(pdev,1,E_CMDQIRQ_CMDQ0);
    stHVSPInitCfg.u32IRQNUM     = MsOS_GetIrqIDSCL(E_SCLIRQ_SC0);
    stHVSPInitCfg.u32CMDQIRQNUM = MsOS_GetIrqIDCMDQ(E_CMDQIRQ_CMDQ0);
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
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_clk);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_proc);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_SCIQ);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_ints);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_dbgmg);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_osd);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_od);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_fbmg);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_mutex);
    gbCIIR = 1;
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
    gu8first = 1;
    return 0;
}
static int mdrv_ms_hvsp1_remove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);
    _ms_hvsp1_mem_free();
    MDrv_HVSP_IDCLKRelease(&_dev_ms_hvsp1.stclk);
    gbProbeAlready = (gbProbeAlready&(~EN_DBG_HVSP1_CONFIG));
    if(gbProbeAlready == 0)
    {
        MDrv_HVSP_Exit(1);
    }
    else if(!(gbProbeAlready& (EN_DBG_HVSP1_CONFIG|EN_DBG_HVSP2_CONFIG|EN_DBG_HVSP3_CONFIG)))
    {
        MDrv_HVSP_Exit(0);
    }
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
    stHVSPInitCfg.u32IRQNUM     = MsOS_GetIrqIDSCL(E_SCLIRQ_SC0);
    stHVSPInitCfg.u32CMDQIRQNUM = MsOS_GetIrqIDCMDQ(E_CMDQIRQ_CMDQ0);
    if( MDrv_HVSP_Init(E_MDRV_HVSP_ID_1, &stHVSPInitCfg) == 0)
    {
        return -EFAULT;
    }
    //clk enable
    //no device tree use hardcode
    //create device

    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_ptgen);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_monitorHW);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_clk);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_proc);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_ints);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_dbgmg);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_osd);
    ret = device_create_file(_dev_ms_hvsp1.devicenode, &dev_attr_fbmg);
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

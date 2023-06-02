////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (MStar Confidential Information) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __DRV_ISP_C__
#define __DRV_ISP_C__

#define ISP_DBG 0
#if ISP_DBG
#define DEBUG
#endif

// FPGA ISP IP verifiy
//#define FPGA_VERIFY

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/stringify.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include <cedric/irqs.h>
#include <mdrv_msys_io_st.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clk.h>
//#include <linux/cdev.h>
#include <linux/miscdevice.h>

#include <mdrv_isp_io.h>
#include <mdrv_isp_io_st.h>

#include "ms_msys.h"
#include "hal_isp.h"
#include "i2c.h"
#if defined(FPGA_VERIFY)
#include "drv_fpga.h"
#endif
/*--------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////

/*--------------------------------------------------------------------------*/
/* CONSTANT DEFINITION                                                      */
/*--------------------------------------------------------------------------*/
#if ISP_DBG
#define DBG_INFO pr_debug
#define DBG_INFO_1 pr_debug
#define DBG_INFO_2 pr_debug
#else
#define DBG_INFO(fmt, arg...) {}
#define DBG_INFO_1  DBG_INFO
#define DBG_INFO_2  DBG_INFO
#endif


/*--------------------------------------------------------------------------*/
/* MACRO DEFINITION                                                         */
/*--------------------------------------------------------------------------*/
#define ISP_BIN_ATTR(_a,_b) \
    struct bin_attribute _a##_attr = { \
        .attr = { .name =  __stringify(_a), \
            .mode = _b, }, \
            .size = 0,  \
            .read = _a##_read,  \
            .write = _a##_write, \
}

#define DEV_ATTR(_name,_mode,_size) \
    struct bin_attribute _name##_attr = { \
        .attr = { .name =  __stringify(_name), \
            .mode = _mode, }, \
            .size = _size,  \
            .read = _name##_read,  \
            .write = _name##_write, \
}

#define C_IN(lock)       local_irq_save(flags)
#define C_OUT(lock)      local_irq_restore(flags)

//--------- ISP 3A statistics buffer ---------//
#define AE_WIN_SIZE                 (128*90)
#define AWB_WIN_SIZE                (128*90)
#define AE_HIST_R_SIZE              80
#define AE_HIST_G_SIZE              80
#define AE_HIST_B_SIZE              80
#define AE_HIST_WIN0_SIZE           256
#define AE_HIST_WIN1_SIZE           256
//R,G,B,Y 4bytes statistics
#define AE_STATIS_SIZE  (AE_WIN_SIZE*4)
#define HISTO_STATIS_SIZE (AE_HIST_R_SIZE+AE_HIST_G_SIZE+AE_HIST_B_SIZE+AE_HIST_WIN0_SIZE+AE_HIST_WIN1_SIZE)
#define AF_STATIS_SIZE  (60*4)
#define AWB_STATIS_SIZE  (AWB_WIN_SIZE*3)
#define MOT_STATIS_SIZE (168*168) //for 2688x2688 image
#define RGBIR_STATIS_SIZE (256*4) //for 2688x2688 image

typedef struct
{
    char ae[AE_STATIS_SIZE] __attribute__((aligned(16)));
    char awb[AWB_STATIS_SIZE] __attribute__((aligned(16)));
    char af[AF_STATIS_SIZE] __attribute__((aligned(16)));
    char histo[HISTO_STATIS_SIZE] __attribute__((aligned(16)));  //histogram
    char mot[MOT_STATIS_SIZE] __attribute__((aligned(16)));  //motion detection
    char rgbir[RGBIR_STATIS_SIZE] __attribute__((aligned(16)));
}__attribute__((packed, aligned(16))) ISP_STATIS_MEM;

//#define DNR_FB_SIZE        (((1920+191)/192) * ((1080+15)>>4) * 256 * 16)
/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef struct
{
  u32 frame_interval;
  u32 fps;
  u32 prev_tick_count;
}__attribute__((packed, aligned(1))) FPS_INFO;

typedef struct _isp_ccm_coff
{
  s16 dirty;
  s16 ccm[9];
}isp_ccm_coff;

typedef struct _isp_ae_dgain
{
  s16 dirty;
  s16 enable;
  u32 gain;
}isp_ae_dgain;

typedef struct _isp_fifo_mask
{
  s16 dirty;
  s16 enable;
}isp_fifo_mask;

typedef struct {
    u32             frame_cnt;
    u8              sysfs_int;
    u8              vsync_start;
    u8              vsync_end;
    u8              ae;
    u8              ae_win0;
    u8		 ae_row_int;
    u8              awb;
    u8              epoll_event;
    u8              af;
    u8              isp_busy;
    u8              isp_idle;
    u8              isp_fifofull;
    u8              wdma_fifofull;
    u8              rdma_done;
    u8              wdma_done;
    u8              eis;
    u8              scl_fe;

    unsigned long   AE_Lum[64];   // 8x8 windows
    unsigned long   AE_Hist[64];  // 8x8 windows
    unsigned long   AF_Stat[5*3];  // 5 windows *( R+G+B)
    unsigned long   AWB_Stat[80]; // 5 windows

    //FRAME_STATE   *FrameState;
    ISP_SHARE_DATA *share_data;

    u32  isp_int_count[ISP_INT_MAX];
    u32  isp_int2_count[ISP_INT2_MAX];
    u32  isp_int3_count[ISP_INT3_MAX];

    struct timespec isp_int_time[ISP_INT_MAX];
    struct timespec isp_int2_time[ISP_INT2_MAX];
    struct timespec isp_int3_time[ISP_INT3_MAX];

    u8 hw_frame_cnt;
    u32 int_cnt;

    FPS_INFO fps;
    isp_ccm_coff rgb_ccm; //RGB2YUV matrix
    isp_ccm_coff yuv_ccm; //CCM
    isp_ae_dgain dgain;   //AE dgain
    isp_isr_ae_img_info ae_img_info; //ae statistic block setting
    isp_fifo_mask fifo_mask;
} ISP_INT_DATA;

typedef struct {
    void          *pISPRegs[10];
    void          *pCSIRegs;
    MSYS_DMEM_INFO Base_Meminfo;
    MSYS_DMEM_INFO DNR_FB_Meminfo[2];
    MSYS_DMEM_INFO ShareData_Meminfo;
} ISP_MEM_BUFF;

typedef enum
{
   ISP_INFO_ID_AE=0,
   ISP_INFO_ID_AWB,
   ISP_INFO_ID_IQ,
   ISP_INFO_ID_MAX,
}ISP_INFO_ID;

typedef struct _isp_dev_data
{
    struct miscdevice isp_dev;
    struct device * sysfs_dev; //for node /sys/class/mstar/
    struct clk *clk_isp;                 //isp clock
    struct clk *clk_sr;                  //pclk
    struct clk *clk_sr_mclk;        //sensor mclk
    struct clk *clk_csi_mac;
    ISP_INT_DATA    *isp_int;
    ISP_MEM_BUFF    *isp_mem;
    char isp_info_str[ISP_INFO_ID_MAX][1024];
}isp_dev_data;

#define BANK_TO_ADDR32(b) (b<<9)
#define BANK_SIZE 0x200

static ISP_INT_DATA    isp_int;
static ISP_MEM_BUFF    isp_mem;
//static i2c_handle_t isp_i2c;

void reset_isp_int(ISP_INT_DATA *p_isp_int) //reset isp_int struct
{
    //p_isp_int->frame_cnt = 0;
    p_isp_int->sysfs_int = 0;
    p_isp_int->vsync_start = 0;
    p_isp_int->vsync_end = 0;
    p_isp_int->ae = 0;
    p_isp_int->ae_win0 = 0;
    p_isp_int->ae_row_int = 0;
    p_isp_int->awb = 0;
    p_isp_int->epoll_event = 0;
    p_isp_int->af = 0;
    p_isp_int->isp_busy = 0;
    p_isp_int->isp_idle = 0;
    p_isp_int->isp_fifofull = 0;
    p_isp_int->wdma_fifofull = 0;
    p_isp_int->rdma_done = 0;
    p_isp_int->wdma_done = 0;
    p_isp_int->eis = 0;

    memset(p_isp_int->AE_Lum,0,sizeof(p_isp_int->AE_Lum));
    memset(p_isp_int->AE_Hist,0,sizeof(p_isp_int->AE_Hist));
    memset(p_isp_int->AF_Stat,0,sizeof(p_isp_int->AF_Stat));
    memset(p_isp_int->AWB_Stat,0,sizeof(p_isp_int->AF_Stat));

    p_isp_int->share_data->frame_state.bIspInputEnable = true;
}

void isp_async(u32 enable)
{
    IspAsyncEnable(enable);
}

void fps_init(FPS_INFO *info)
{
  memset(info,0,sizeof(FPS_INFO));
}

u32 fps_update(FPS_INFO *info)
{
    u32 dt;
    struct timespec ts;
    getnstimeofday(&ts);

    if(ts.tv_nsec>info->prev_tick_count)
    {
        dt = ts.tv_nsec - info->prev_tick_count;
    }
    else
    {
        dt = 0xFFFFFFFF - info->prev_tick_count + ts.tv_nsec;
    }

    if(dt<1000000*500) //dt < 500ms
    {
        info->frame_interval = dt;
    }

    info->prev_tick_count = ts.tv_nsec;
    info->fps = 1000000000/dt;
    return info->fps;
}
//#define TIMING_DBG_MSG
#ifdef TIMING_DBG_MSG
void print_timestamp(char* msg)
{
    struct timespec ts;
    getnstimeofday(&ts);
    printk("%d:%lu:%s\r\n", isp_int.frame_cnt, (ts.tv_sec * 1000000)+(ts.tv_nsec/1000), msg);
}
#else
#define print_timestamp(a)
#endif

typedef struct
{
  u64 count;
  spinlock_t lock;
  u8 frame_end; //frame end interrupt flag
  u8 frame_start;

  u32 fs_int_count;    //frame start interrput count
  u32 vc0_int_count; //virtual channel 0 count

  wait_queue_head_t *p_wq_fe;
  wait_queue_head_t *p_wq_fs;

  //HAL
  void* reg_base;
  void* hal_handle;

  u32 csi_err_int_count[CSI_ERR_INT_MAX];
  struct device *sysfs_dev;
}csi_dev_data;

/*--------------------------------------------------------------------------*/
/* VARIABLE DEFINITION                                                      */
/*--------------------------------------------------------------------------*/

irqreturn_t isp_ISR(int num, void *priv);
static int isp_probe(struct platform_device* pdev);
static int isp_remove(struct platform_device* pdev );
static int isp_suspend(struct platform_device *pdev, pm_message_t state);
static int isp_resume(struct platform_device *pdev);
int isp_create_bin_file(struct device *dev);
int isp_create_bin_file_(struct device *dev);

static struct of_device_id isp_dt_ids[] = {
    {.compatible = "isp"},
    {}
};
MODULE_DEVICE_TABLE(of, isp_dt_ids);

irqreturn_t csi_ISR(int num, void *priv);
static int csi_probe(struct platform_device* pdev);
static int csi_remove(struct platform_device* pdev );
static int csi_suspend(struct platform_device *pdev, pm_message_t state);
static int csi_resume(struct platform_device *pdev);

static struct of_device_id csi_dt_ids[] = {
    {.compatible = "csi"},
    {}
};
MODULE_DEVICE_TABLE(of, csi_dt_ids);

static struct platform_device *isp_device;

static struct platform_driver isp_driver =
{
        .probe   = isp_probe,
        .remove = isp_remove,
        .suspend = isp_suspend,
        .resume  = isp_resume,
        .driver  = {
                .name = "isp",
                .owner = THIS_MODULE,
                .of_match_table = of_match_ptr(isp_dt_ids),
        },
};

static struct platform_driver csi_driver =
{
        .probe   = csi_probe,
        .remove = csi_remove,
        .suspend = csi_suspend,
        .resume  = csi_resume,
        .driver  = {
                .name = "csi",
                .owner = THIS_MODULE,
                .of_match_table = of_match_ptr(csi_dt_ids),
        },
};

static DECLARE_WAIT_QUEUE_HEAD(isp_wq_WDMA_FIFO_FULL);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ISP_FIFO_FULL);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_RDMA_DONE);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_WDMA_DONE);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ISP_IDLE);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ISP_BUSY);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_VSTART);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_VEND);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ae);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ae_win0);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ae_win1);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ae_row_int);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_awb);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_af);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_eis);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_sw_int_in);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_sw_int_out);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_hit_line_count1);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_hit_line_count2);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_hit_line_count3);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_hdr_histo_done);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_rgbir_histo_done);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_awb_row_done);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_histo_row_done);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_scl_fe);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_epoll_event);

//CSI//
static DECLARE_WAIT_QUEUE_HEAD(csi_wq_fe);
static DECLARE_WAIT_QUEUE_HEAD(csi_wq_fs);

static struct kobject *kobj_isp = NULL;


/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTION PROTOTYPE DECLARATION AND DEFINITION                      */
/*--------------------------------------------------------------------------*/

extern int msys_request_dmem(MSYS_DMEM_INFO *mem_info);
extern void Drv_SCLIRQ_SetDropFrameFromISP(unsigned char u8Count);

#define member_size(type, member) sizeof(((type *)0)->member)
static void isp_ae_cache_invalidate(void)
{
    Chip_Inv_Cache_Range((unsigned long)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,ae),member_size(ISP_STATIS_MEM,ae));
}

static void isp_awb_cache_invalidate(void)
{
    Chip_Inv_Cache_Range((unsigned long)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,awb),member_size(ISP_STATIS_MEM,awb));
}

static void isp_af_cache_invalidate(void)
{
    Chip_Inv_Cache_Range((unsigned long)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,af),member_size(ISP_STATIS_MEM,af));
}

static void isp_mot_cache_invalidate(void)
{
    Chip_Inv_Cache_Range((unsigned long)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,mot),member_size(ISP_STATIS_MEM,mot));
}

static void isp_histo_cache_invalidate(void)
{
    Chip_Inv_Cache_Range((unsigned long)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,histo),member_size(ISP_STATIS_MEM,histo));
}

#if 1
//set DNR frame buffer address
static int isp_set_dnr_fb_base(MSYS_DMEM_INFO *mem_info,int id)
{
    u32 phys_addr;
    phys_addr =  (unsigned long)mem_info->phys;
    DBG_INFO("DNR_%d PHYS=0x%llX,VIRT=0x%llX\n",id,mem_info->phys,mem_info->kvirt);
    HalISPSetDnrFbAddr(phys_addr,id);
    return 0;
}
#endif

u32 g_isp_flag = ISP_FB_ROT | ISP_FB_DNR;
u32 g_isp_max_res = 3; //max resolutioin, 0:1280x720 1:1920x1080, 2:2048x1536, 3:2688x1520
static unsigned int get_dnr_buffer_size(void)
{
    u32 width = 2688;
    u32 height = 1520;
    switch(g_isp_max_res)
    {
    case 0: //1270x720
        width = 1280;
        height = 720;
    break;
    case 1://1920x1080
        width = 1920;
        height = 1080;
    break;
    case 2://2048x1536 , 3M
        width = 2048;
        height = 1536;
    break;
    case 3://2688x1520 , 4M
        width = 2688;
        height = 1520;
    break;
    case 4://2688x1520 , 4M
        width = 2688;
        height = 1520;
    break;
    case 5://2592x1944 , 5M
        width = 2592;
        height = 1944;
    }
    pr_info("[ISP] max_res= %dx%d\n",width,height);
    return (((width+191)/192) * ((height+15)>>4) * 256 * 16);
}
static unsigned int get_dnr_buffer_num(void)
{
    if(g_isp_flag&ISP_FB_DNR)
    {
        return (g_isp_flag&ISP_FB_ROT)?2:1;
    }
    else
        return 0;
}

static int IspInitDnrFB(u32 nImgW,u32 nImgH,u32 nNum)
{
    int i;
    int ret=0;
    int nBufSize = (((nImgW+191)/192) * ((nImgH+15)>>4) * 256 * 16);

    if(nNum>get_dnr_buffer_num())
    {
        pr_info("[ISP]DNR buffer number exceed max number. num=%d buffers\n",nNum);
        return -1;
    }

    if(nBufSize>get_dnr_buffer_size())
    {
        pr_info("[ISP]DNR buffer size exceed max size. size=%d bytes\n",nBufSize);
        return -1;
    }

    for(i=0;i<2;++i)
    {
        if(isp_mem.DNR_FB_Meminfo[i].phys)
        {
            msys_release_dmem(&isp_mem.DNR_FB_Meminfo[i]);
            memset(&isp_mem.DNR_FB_Meminfo[i],0,sizeof(isp_mem.DNR_FB_Meminfo[i]));
        }
    }

    if(nNum==1)//only DNR buffer
    {
        int err = 0;
        MSYS_DMEM_INFO *info = &isp_mem.DNR_FB_Meminfo[0];
        sprintf(info->name,"ISP_DNR_%d",0);
        info->length = nBufSize;
        pr_info("ISP DNR buffer#%d size = 0x%X\n",0,info->length);
        if(msys_request_dmem(info))
        {
            pr_err("ISP failed to request DNR frame buffer. err=0x%X\n",err);
            ret = -1;
        }
        else
        {
            isp_set_dnr_fb_base(info,0);
            isp_set_dnr_fb_base(info,1);
            HalISPSetDnrUbound((unsigned long)(info->phys+info->length));
        }
    }
    else if(nNum==2)//DNR + ROT buffer
    {
        unsigned long max_addr = 0;
        for(i=0;i<2;++i)
        {
            int err = 0;
            MSYS_DMEM_INFO *info = &isp_mem.DNR_FB_Meminfo[i];
            sprintf(info->name,"ISP_DNR_%d",i);
            info->length = nBufSize;
            pr_info("ISP DNR buffer#%d size = 0x%X\n",i,info->length);
            if(msys_request_dmem(info))
            {
                pr_err("ISP failed to request DNR frame buffer. err=0x%X\n",err);
                ret = -1;
                break;
            }
            isp_set_dnr_fb_base(info,i);
            max_addr = max_addr > info->phys ? max_addr:info->phys;
            HalISPSetDnrUbound((unsigned long)(max_addr+info->length));
        }
    }
    return ret;
}

static int isp_init_buff(void) {
    //int i;
    pr_debug("[ISP] isp_init_buff\n");
    isp_mem.Base_Meminfo.length = sizeof(ISP_STATIS_MEM);

    sprintf(isp_mem.Base_Meminfo.name,"ISP_base");
    if(0 > msys_request_dmem(&isp_mem.Base_Meminfo))
    {
        pr_err("ISP failed to request Base_Meminfo buffer.\n");
        return -1;
    }

    DBG_INFO("mem base phyaddr:0x%08x, viraddr:0x%08x, len:%d", (unsigned int)isp_mem.Base_Meminfo.phys,
        (unsigned int)isp_mem.Base_Meminfo.kvirt,isp_mem.Base_Meminfo.length);

    ////////// isp share data ////////////////
    sprintf(isp_mem.ShareData_Meminfo.name, "ISP_SHARE_DATA");
	isp_int.share_data = kmalloc(sizeof(ISP_SHARE_DATA),GFP_KERNEL);
	memset(isp_int.share_data, 0, sizeof(ISP_SHARE_DATA));
	isp_mem.ShareData_Meminfo.kvirt = (unsigned long)isp_int.share_data;
	isp_mem.ShareData_Meminfo.phys = virt_to_phys(isp_int.share_data);
	isp_mem.ShareData_Meminfo.length = sizeof(ISP_SHARE_DATA);

    pr_info("ShareData_Meminfo phyaddr:0x%08llx, viraddr:0x%08llx, len:%#x\n",
        isp_mem.ShareData_Meminfo.phys,
        isp_mem.ShareData_Meminfo.kvirt, isp_mem.ShareData_Meminfo.length);

    HalISPSetAeBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,ae), offsetof(ISP_STATIS_MEM,awb));
    HalISPSetAwbBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,awb), offsetof(ISP_STATIS_MEM,af)-offsetof(ISP_STATIS_MEM,awb));
    HalISPSetAfBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,af), offsetof(ISP_STATIS_MEM,histo)-offsetof(ISP_STATIS_MEM,af));
    HalISPSetHistoBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,histo), offsetof(ISP_STATIS_MEM,mot)-offsetof(ISP_STATIS_MEM,histo));
    HalISPSetMotBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,mot), offsetof(ISP_STATIS_MEM,rgbir)-offsetof(ISP_STATIS_MEM,mot));
    HalISPSetRgbIRBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,rgbir), sizeof(ISP_STATIS_MEM)-offsetof(ISP_STATIS_MEM,rgbir));

    pr_info("AE Base: virt=0x%X size=0x%X\n",(unsigned int)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,ae), offsetof(ISP_STATIS_MEM,awb));
    pr_info("AWB Base: virt=0x%X size=0x%X\n",(unsigned int)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,awb), offsetof(ISP_STATIS_MEM,af)-offsetof(ISP_STATIS_MEM,awb));
    pr_info("AF Base: virt=0x%X size=0x%X\n",(unsigned int)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,af), offsetof(ISP_STATIS_MEM,histo)-offsetof(ISP_STATIS_MEM,af));
    pr_info("HISTO Base: virt=0x%X size=0x%X\n",(unsigned int)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,histo), offsetof(ISP_STATIS_MEM,mot)-offsetof(ISP_STATIS_MEM,histo));
    pr_info("MOT Base: virt=0x%X size=0x%X\n",(unsigned int)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,mot), offsetof(ISP_STATIS_MEM,rgbir)-offsetof(ISP_STATIS_MEM,mot));
    pr_info("RGBIR Base: virt=0x%X size=0x%X\n",(unsigned int)isp_mem.Base_Meminfo.kvirt + offsetof(ISP_STATIS_MEM,rgbir), sizeof(ISP_STATIS_MEM)-offsetof(ISP_STATIS_MEM,rgbir));

#if 0
    {
        //////////////////////////
        // request DNR frame buffer //
        //////////////////////////
        int i;
        int num_dnr_buf = get_dnr_buffer_num();
        if(num_dnr_buf==1)
        {
            int err = 0;
            MSYS_DMEM_INFO *info = &isp_mem.DNR_FB_Meminfo[0];
            sprintf(info->name,"ISP_DNR_%d",0);
            info->length = get_dnr_buffer_size();
            //pr_info("ISP DNR buffer#%d size = 0x%X\n",0,info->length);
            if(msys_request_dmem(info))
            {
                pr_err("ISP failed to request DNR frame buffer. err=0x%X\n",err);
            }
            else
            {
                isp_set_dnr_fb_base(info,0);
                isp_set_dnr_fb_base(info,1);
            }
        }
        else if(num_dnr_buf==2)
        {
            for(i=0;i<2;++i)
            {
                int err = 0;
                MSYS_DMEM_INFO *info = &isp_mem.DNR_FB_Meminfo[i];
                sprintf(info->name,"ISP_DNR_%d",i);
                info->length = get_dnr_buffer_size();
                //pr_info("ISP DNR buffer#%d size = 0x%X\n",i,info->length);
                if(msys_request_dmem(info))
                {
                    pr_err("ISP failed to request DNR frame buffer. err=0x%X\n",err);
                    break;
                }
                isp_set_dnr_fb_base(info,i);
            }
        }
    }
#endif
    return 0;
}

static int isp_restore_buff(void)
{
    int i;

    HalISPSetAeBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,ae), offsetof(ISP_STATIS_MEM,awb));
    HalISPSetAwbBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,awb), offsetof(ISP_STATIS_MEM,af)-offsetof(ISP_STATIS_MEM,awb));
    HalISPSetAfBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,af), offsetof(ISP_STATIS_MEM,histo)-offsetof(ISP_STATIS_MEM,af));
    HalISPSetHistoBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,histo), offsetof(ISP_STATIS_MEM,mot)-offsetof(ISP_STATIS_MEM,histo));
    HalISPSetMotBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,mot), offsetof(ISP_STATIS_MEM,rgbir)-offsetof(ISP_STATIS_MEM,mot));
    HalISPSetRgbIRBaseAddr((unsigned long)isp_mem.Base_Meminfo.phys + offsetof(ISP_STATIS_MEM,rgbir), sizeof(ISP_STATIS_MEM)-offsetof(ISP_STATIS_MEM,rgbir));

    for(i=0;i<2;++i)
    {
      MSYS_DMEM_INFO *info = &isp_mem.DNR_FB_Meminfo[i];
      isp_set_dnr_fb_base(info,i);
    }

    return 0;
}
static int isp_open(struct inode *inode, struct file *fp)
{
    fp->private_data = isp_device->dev.platform_data;
    return 0;
}

static int isp_release(struct inode *inode, struct file *fp)
{
    fp->private_data = 0;
    return 0;
}
static ssize_t isp_fread(struct file *fp, char __user *buf, size_t size, loff_t *ppos)
{
    return 0;
}

static ssize_t isp_fwrite(struct file *fp,const char __user *buf, size_t size, loff_t *ppos)
{
    return 0;
}

static long isp_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    isp_dev_data *data =  (isp_dev_data*) fp->private_data;
    struct clk *_clk=0;

    pr_debug("ISP IOCTL +\n");
    if (_IOC_TYPE(cmd) != ISP_IOCTL_MAGIC) return -ENOTTY;
    switch(cmd)
    {
        case IOCTL_ISP_CLOCK_CTL:
        {
            isp_ioctl_clock_ctl ctl;

            if(copy_from_user((void*)&ctl, (void __user *)arg, sizeof(ctl)))
            {
                BUG();
            }

            pr_info("ISP CLK CTL : id=%d rate=%d enable=%d\n",ctl.id,ctl.rate,ctl.enable);

            switch(ctl.id)
            {
            case CLK_ID_ISP: //ISP
                _clk = data->clk_isp;
            break;
            case CLK_ID_SR:
                _clk = data->clk_sr;
            break;
            case CLK_ID_SR_MCLK:
                _clk = data->clk_sr_mclk;
            break;
            case CLK_ID_CSI_MAC:
                _clk = data->clk_csi_mac;
            break;
            }

            if(!_clk)
                return -1;

            if(ctl.rate>0)
                clk_set_rate(_clk,ctl.rate);

            if(ctl.enable)
                clk_prepare_enable(_clk);
            else
                clk_disable_unprepare(_clk);
        }
        break;
        case IOCTL_ISP_IQ_RGBCCM:
        {
            if(copy_from_user((void*)&isp_int.rgb_ccm.ccm, (void __user *)arg, sizeof(isp_int.rgb_ccm.ccm)))
            {
                BUG();
            }
            isp_int.rgb_ccm.dirty = 1;
        }break;
        case IOCTL_ISP_IQ_YUVCCM:
        {
            if(copy_from_user((void*)&isp_int.yuv_ccm.ccm, (void __user *)arg, sizeof(isp_int.yuv_ccm.ccm)))
            {
                BUG();
            }
            isp_int.yuv_ccm.dirty = 1;
        }break;
        case IOCTL_ISP_GET_ISP_FLAG:
        {
            isp_ioctl_isp_flag isp_flag;
            isp_flag.flag = g_isp_flag;
            if(copy_to_user((void __user *)arg,(void*)&isp_flag,sizeof(isp_flag)))
            {
                BUG();
            }
        }break;
        case IOCTL_ISP_GET_MEM_INFO:
        {
            isp_ioctl_mem_info info;
            if(copy_from_user((void*)&info, (void __user *)arg, sizeof(info)))
            {
                BUG();
            }
            info.phy_addr = data->isp_mem->Base_Meminfo.phys;
            switch(info.mem_id)
            {
            case ISP_GET_MEM_INFO_BASE:
                info.blk_offset = 0;
                info.blk_size = sizeof(ISP_STATIS_MEM);
            break;
            case ISP_GET_MEM_INFO_AE:
                info.blk_offset = offsetof(ISP_STATIS_MEM,ae);
                info.blk_size = member_size(ISP_STATIS_MEM,ae);
            break;
            case ISP_GET_MEM_INFO_AWB:
                info.blk_offset = offsetof(ISP_STATIS_MEM,awb);
                info.blk_size = member_size(ISP_STATIS_MEM,awb);
            break;
            case ISP_GET_MEM_INFO_AF:
                info.blk_offset = offsetof(ISP_STATIS_MEM,af);
                info.blk_size = member_size(ISP_STATIS_MEM,af);
            break;
            case ISP_GET_MEM_INFO_MOT:
                info.blk_offset = offsetof(ISP_STATIS_MEM,mot);
                info.blk_size = member_size(ISP_STATIS_MEM,mot);
            break;
            case ISP_GET_MEM_INFO_HISTO:
                info.blk_offset = offsetof(ISP_STATIS_MEM,histo);
                info.blk_size = member_size(ISP_STATIS_MEM,histo);
            break;
            case ISP_GET_MEM_INFO_RGBIR:
                info.blk_offset = offsetof(ISP_STATIS_MEM,rgbir);
                info.blk_size = member_size(ISP_STATIS_MEM,rgbir);
            break;
            }
            if(copy_to_user((void __user *)arg,(void*)&info,sizeof(info)))
            {
                BUG();
            }
        }break;
        case IOCTL_ISP_GET_AE_IMG_INFO:
        {
            isp_isr_ae_img_info info;

             info.img_w= isp_int.ae_img_info.img_w;
             info.img_h= isp_int.ae_img_info.img_h;
             info.blk_h= isp_int.ae_img_info.blk_h;
             info.blk_w= isp_int.ae_img_info.blk_w;
             info.rot= isp_int.ae_img_info.rot;

            if(copy_to_user((void __user *)arg,(void*)&info,sizeof(info)))
            {
                BUG();
            }
        }break;
        case IOCTL_ISP_UPDATE_AE_IMG_INFO:
        {
            isp_isr_ae_img_info info;
            if(copy_from_user((void*)&info, (void __user *)arg, sizeof(info)))
            {
                BUG();
            }
            isp_int.ae_img_info.img_w = info.img_w;
            isp_int.ae_img_info.img_h = info.img_h;
            isp_int.ae_img_info.blk_h = info.blk_h;
            isp_int.ae_img_info.blk_w = info.blk_w;
            isp_int.ae_img_info.rot = info.rot;

#if 0
            //printk("##########isp_int.ae_img_info.img_w= %d\n",isp_int.ae_img_info.img_w);
            //printk("##########isp_int.ae_img_info.img_h= %d\n",isp_int.ae_img_info.img_h);
            //printk("##########isp_int.ae_img_info.blk_h= %d\n",isp_int.ae_img_info.blk_h);
            //printk("##########isp_int.ae_img_info.blk_w= %d\n",isp_int.ae_img_info.blk_w);
            //printk("##########isp_int.ae_img_info.rot= %d\n",isp_int.ae_img_info.rot);
#endif
        }break;
        case IOCTL_ISP_SET_AE_DGAIN:
        {
            isp_ioctl_ae_dgian gain;
            if(copy_from_user((void*)&gain, (void __user *)arg, sizeof(gain)))
            {
                BUG();
            }
            isp_int.dgain.enable = gain.enable;
            isp_int.dgain.gain = gain.dgain;
            isp_int.dgain.dirty = 1;
        }break;
        case IOCTL_ISP_SET_FIFO_MASK:
        {
            isp_ioctl_fifo_mask mask;
            unsigned long flags;
            spinlock_t lock;
            spin_lock_init(&lock);

            if(copy_from_user((void*)&mask, (void __user *)arg, sizeof(mask)))
            {
                BUG();
            }

            spin_lock_irqsave(&lock,flags);
            if(!isp_int.fifo_mask.enable) //if enable mask
            {
                //IspInputEnable(0); //enable fifo mask immediately
                isp_int.fifo_mask.dirty = 1;
            }
            else
            {
                IspInputEnable(1); //enable isp input immediately
                isp_int.fifo_mask.dirty = 1;
            }
            C_OUT(isp_lock);
            spin_unlock_irqrestore(&lock,flags);
        }break;
        case IOCTL_ISP_TRIGGER_WDMA:
        {
            isp_ioctl_trigger_wdma_attr attr;

            memset((void *)&attr,0,sizeof(attr));

            if(copy_from_user((void*)&attr, (void __user *)arg, sizeof(attr)))
            {
                BUG();
            }

#if 0
            //printk("attr.width=%d\n",attr.width);
            //printk("attr.height=%d\n",attr.height);
            //printk("attr.x=%d\n",attr.x);
            //printk("attr.y=%d\n",attr.y);
            //printk("attr.wdma_path=%d\n",attr.wdma_path);
            //printk("attr.buf_addr_phy=0x%x\n",attr.buf_addr_phy);
            //printk("attr.buf_addr_vir=0x%x\n",attr.buf_addr_kvir);
#endif
            HalISPWdmaTrigger(attr);

            if(wait_event_interruptible_timeout(isp_wq_WDMA_DONE, isp_int.wdma_done == true,100)){
                Chip_Inv_Cache_Range(attr.buf_addr_kvir,attr.width*attr.height*2);
                pr_debug("!!!!!!! WDMA DONE!!!!!!!!\n");
            }else{

                pr_debug("!!!!!!! WDMA TIMEOUT !!!!!\n");
                return -1;
            }
        }break;
        case IOCTL_ISP_SKIP_FRAME:
        {
            isp_ioctl_trigger_skip_attr attr;

            memset((void *)&attr,0,sizeof(attr));

            if(copy_from_user((void*)&attr, (void __user *)arg, sizeof(attr)))
            {
                BUG();
            }

            Drv_SCLIRQ_SetDropFrameFromISP(attr.skip_cnt);
            pr_debug("!!!!!skip frame:%d\n",attr.skip_cnt);

        }break;
        case IOCTL_ISP_INIT:
        {
            isp_ioctl_isp_init param;
            if(copy_from_user((void*)&param, (void __user *)arg, sizeof(param)))
            {
                BUG();
            }

            if(param.nDnrFlag&ISP_FB_DNR)//only DNR -> 1 buffer, DNR+ROT -> 2 buffer
                return IspInitDnrFB(param.nRawW,param.nRawH,(param.nDnrFlag&ISP_FB_ROT)?2:1);
        }break;
    }
    pr_debug("ISP IOCTL -\n");
    return 0;
}
u32 isp_get_ae_img_info(isp_isr_ae_img_info *info)
{
    info->img_w= isp_int.ae_img_info.img_w;
    info->img_h= isp_int.ae_img_info.img_h;
    info->blk_h= isp_int.ae_img_info.blk_h;
    info->blk_w= isp_int.ae_img_info.blk_w;
    info->rot= isp_int.ae_img_info.rot;

    return 0;
}

//------------ Frame start signal device ----------------------
DEFINE_SPINLOCK(g_IspFsLock);
static struct list_head gIspFsWqList = LIST_HEAD_INIT(gIspFsWqList);
typedef struct
{
    struct list_head list;
    wait_queue_head_t waitq;
    //int id;
    unsigned int fcount;
    int ready;
}IspIrqLink_t;

static int isp_fs_open(struct inode *inode, struct file *fp)
{
    unsigned long flags;
    //static int open_cnt=0;
    IspIrqLink_t *data = kmalloc(sizeof(IspIrqLink_t),GFP_KERNEL);
    if(!data)
        return -ENOENT;
    memset((void*)data,0,sizeof(IspIrqLink_t));
    init_waitqueue_head(&data->waitq);
    spin_lock_irqsave(&g_IspFsLock,flags);
    list_add(&data->list,&gIspFsWqList);
    spin_unlock_irqrestore(&g_IspFsLock,flags);

    fp->private_data = (void*) data;
    pr_info("isp_fs open \n");
    return 0;
}

static int isp_fs_release(struct inode *inode, struct file *fp)
{
    unsigned long flags;
    IspIrqLink_t *data = (IspIrqLink_t*)fp->private_data;

    spin_lock_irqsave(&g_IspFsLock,flags);
    list_del(&data->list);
    spin_unlock_irqrestore(&g_IspFsLock,flags);

    pr_info("isp_fs close \n");

    kfree(data);
    return 0;
}

static ssize_t isp_fs_fread(struct file *fp, char __user *buf, size_t size, loff_t *ppos)
{
    IspIrqLink_t *data = (IspIrqLink_t*)fp->private_data;
    memcpy(buf, (void*)&data->fcount, size>sizeof(data->fcount)?sizeof(data->fcount):size);
    return 0;
}

unsigned int isp_fs_poll(struct file *fp, poll_table *wait)
{
    unsigned int mask = 0;
    IspIrqLink_t *data = (IspIrqLink_t*)fp->private_data;
    poll_wait(fp, &data->waitq, wait);
    if(data->ready)
    {
        data->ready = 0;
        mask |= POLLIN|POLLRDNORM;
    }
    return mask;
}

struct file_operations isp_fs_fops =
{
    .owner = THIS_MODULE,
    .open = isp_fs_open,
    .release = isp_fs_release,
    .read = isp_fs_fread,
    .write = 0,
    .unlocked_ioctl = 0,
    .poll = isp_fs_poll,
};

struct miscdevice g_isp_fs_dev;
static void AddFsNode(void)
{
    g_isp_fs_dev.minor = MISC_DYNAMIC_MINOR;
    g_isp_fs_dev.name = "isp_fs";
    g_isp_fs_dev.fops = &isp_fs_fops;
    g_isp_fs_dev.parent = 0;
    misc_register(&g_isp_fs_dev);
}

//------------ AE signal device ----------------------
DEFINE_SPINLOCK(g_IspAeLock);
static struct list_head gIspAeWqList = LIST_HEAD_INIT(gIspAeWqList);
static int isp_ae_open(struct inode *inode, struct file *fp)
{
    unsigned long flags;
    IspIrqLink_t *data = kmalloc(sizeof(IspIrqLink_t),GFP_KERNEL);
    if(!data)
        return -ENOENT;
    memset((void*)data,0,sizeof(IspIrqLink_t));
	//add wait queue to fs list
    init_waitqueue_head(&data->waitq);
    spin_lock_irqsave(&g_IspAeLock,flags);
    list_add(&data->list,&gIspAeWqList);
    spin_unlock_irqrestore(&g_IspAeLock,flags);

    fp->private_data = (void*) data;
    pr_debug("isp_ae open\n");
    return 0;
}

static int isp_ae_release(struct inode *inode, struct file *fp)
{
    unsigned long flags;
    IspIrqLink_t *data = (IspIrqLink_t*)fp->private_data;

    spin_lock_irqsave(&g_IspAeLock,flags);
    list_del(&data->list);
    spin_unlock_irqrestore(&g_IspAeLock,flags);

    pr_debug("isp_ae close\n");
    kfree(data);
    return 0;
}

static ssize_t isp_ae_fread(struct file *fp, char __user *buf, size_t size, loff_t *ppos)
{
    IspIrqLink_t *data = (IspIrqLink_t*)fp->private_data;
    memcpy(buf, (void*)&data->fcount, size>sizeof(data->fcount)?sizeof(data->fcount):size);
    return 0;
}

unsigned int isp_ae_poll(struct file *fp, poll_table *wait)
{
    unsigned int mask = 0;
    IspIrqLink_t *data = (IspIrqLink_t*)fp->private_data;
    poll_wait(fp, &data->waitq, wait);
    if(data->ready)
    {
        data->ready = 0;
        mask |= POLLIN|POLLRDNORM;
    }
    return mask;
}

struct file_operations isp_ae_fops =
{
    .owner = THIS_MODULE,
    .open = isp_ae_open,
    .release = isp_ae_release,
    .read = isp_ae_fread,
    .write = 0,
    .unlocked_ioctl = 0,
    .poll = isp_ae_poll,
};

struct miscdevice g_isp_ae_dev;
static void AddAeNode(void)
{
    g_isp_ae_dev.minor = MISC_DYNAMIC_MINOR;
    g_isp_ae_dev.name = "isp_ae";
    g_isp_ae_dev.fops = &isp_ae_fops;
    g_isp_ae_dev.parent = 0;
    misc_register(&g_isp_ae_dev);
}

//-------------------------------------------

unsigned int isp_poll(struct file *filp, poll_table *wait)
{
  unsigned int mask = 0;

  poll_wait(filp, &isp_wq_epoll_event, wait);

  if(isp_int.epoll_event == true)
  {
      isp_int.epoll_event = false;
      mask |= POLLIN;
  }
  return mask;
}

struct file_operations isp_fops =
{
    .owner = THIS_MODULE,
    .open = isp_open,
    .release = isp_release,
    .read = isp_fread,
    .write = isp_fwrite,
    .unlocked_ioctl = isp_ioctl,
    .poll = isp_poll,
};

#define MAJOR_ISP_NUM               234
#define MINOR_ISP_NUM               128
#define MINOR_CSI_NUM               127
static int isp_probe(struct platform_device* pdev)
{
    int err, ret;
    int irq;
    unsigned int i, u4IO_PHY_BASE;
    unsigned int u4Banks[10];
    isp_dev_data *data;

    pr_debug("[ISP] = isp_probe\n");

    memset(&isp_int,0,sizeof(isp_int));
    isp_int.int_cnt = 0;

    data = kzalloc(sizeof(isp_dev_data),GFP_KERNEL);
    if(NULL == data)
        return -ENOENT;
    ret = of_property_read_u32(pdev->dev.of_node, "io_phy_addr", &u4IO_PHY_BASE);
    if(ret != 0)
        pr_err("[ISP] read node error!\n");
    ret = of_property_read_u32_array(pdev->dev.of_node, "banks", (unsigned int*)u4Banks, 10);
    if(ret != 0)
        pr_err("[ISP] read node error!\n");
    for (i = 0; i < 10; i++) {
        isp_mem.pISPRegs[i] = (void*)ioremap(BANK_TO_ADDR32(u4Banks[i])+u4IO_PHY_BASE, BANK_SIZE);
    }
    ret = of_property_read_u32(pdev->dev.of_node, "isp-flag", &g_isp_flag);
    if(ret != 0)
        pr_err("[ISP] read node error!\n");
    ret = of_property_read_u32(pdev->dev.of_node, "isp-res", &g_isp_max_res);
    if(ret != 0)
        pr_err("[ISP] read node error!\n");

    //enable clocks
    //ISP
    data->clk_isp = of_clk_get(pdev->dev.of_node, 0);
    clk_set_rate(data->clk_isp,123000000);
    clk_prepare_enable(data->clk_isp);

    data->clk_sr_mclk = of_clk_get(pdev->dev.of_node, 1);
    clk_set_rate(data->clk_sr_mclk,12000000);
    clk_prepare_enable(data->clk_sr_mclk);

    data->clk_sr = of_clk_get(pdev->dev.of_node, 2);
    clk_set_rate(data->clk_sr,86000000);
    clk_prepare_enable(data->clk_sr);

    data->clk_csi_mac = of_clk_get(pdev->dev.of_node, 3);

    data->isp_int = &isp_int;
    data->isp_mem = &isp_mem;
    fps_init(&data->isp_int->fps);  //initial frame interval count

    // init register and disable interrupt
    HalInitRegs((void**)isp_mem.pISPRegs);
    HalISPDisableInt();

    irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
    pr_debug("[ISP] Request IRQ: %d\n", irq);
    if (request_irq(irq, isp_ISR, 0, "isp interrupt", (void*)&isp_int) == 0)
        pr_debug("[ISP] isp interrupt registered\n");
    else
        pr_err("[ISP] isp interrupt failed");

    SETREG8((u4IO_PHY_BASE+(0x112B00*2)+(0x16*4)),0x01);      //set GPIO28 to GPIO function
    CLRREG16((u4IO_PHY_BASE+(0x101A00*2)+(0x12*4)),(0x0001<<12)); //set GPIO28 pad mux to GPIO
    CLRREG16((u4IO_PHY_BASE+(0x102B00*2)+(0x12*4)),(0x0001<<9));  //GPIO28 OEN
    SETREG16((u4IO_PHY_BASE+(0x102B00*2)+(0x12*4)),(0x0001<<8));  //GPIO28 pull ligh
    isp_device = pdev;

    //allocate statistics data and menload table memory
    isp_init_buff();

    // enable interrupt
    HalISPEnableInt();
    isp_int.sysfs_int = true; // turn on IRQ handler
    isp_int.yuv_ccm.dirty = 0;

    isp_int.dgain.dirty = 0;
    isp_int.dgain.enable = 0;
    isp_int.dgain.gain = 1024;

    // setup kernel i2c
    data->isp_dev.minor = MISC_DYNAMIC_MINOR;
    data->isp_dev.name = "isp";
    data->isp_dev.fops = &isp_fops;
    data->isp_dev.parent = &pdev->dev;
    misc_register(&data->isp_dev);

    AddFsNode();
    AddAeNode();

    data->sysfs_dev = device_create(msys_get_sysfs_class(), NULL, MKDEV(MAJOR_ISP_NUM, MINOR_ISP_NUM), NULL, "isp0");
    isp_create_bin_file(data->sysfs_dev);
    err = sysfs_create_link(&pdev->dev.parent->kobj,&data->sysfs_dev->kobj, "isp0"); //create symlink for older firmware version

    data->sysfs_dev->platform_data = pdev->dev.platform_data = (void*)data;

    return 0;
}

static int isp_remove(struct platform_device* pdev)
{
    isp_dev_data *data = dev_get_platdata(&pdev->dev);
    clk_disable_unprepare(data->clk_isp);
    clk_put(data->clk_isp);
    clk_disable_unprepare(data->clk_sr);
    clk_put(data->clk_sr);
    clk_disable_unprepare(data->clk_sr_mclk);
    clk_put(data->clk_sr_mclk);

    misc_deregister(&data->isp_dev);
    device_destroy(msys_get_sysfs_class(),MKDEV(MAJOR_ISP_NUM, MINOR_ISP_NUM));
    kfree(data);
    return 0;
}

static int isp_suspend(struct platform_device *pdev, pm_message_t state)
{
    if (!pdev)
    {
        return -EINVAL;
        //pr_info("isp_suspend with NULL pdev %d", isp_int.frame_cnt);
    }
    else
    {
        isp_dev_data *data = dev_get_platdata(&pdev->dev);
        pr_info("[ISP] = isp_suspend\n");
        isp_int.sysfs_int = false;

        if(data->clk_isp)
            clk_disable_unprepare(data->clk_isp);
        if(data->clk_sr)
            clk_disable_unprepare(data->clk_sr);
        if(data->clk_sr_mclk)
            clk_disable_unprepare(data->clk_sr_mclk);
    }
    return 0;
}

static int isp_resume(struct platform_device *pdev)
{
    isp_dev_data *data;

    if(!pdev)
        return -EINVAL;

    data = dev_get_platdata(&pdev->dev);
    HalISPDisableInt();

    isp_int.sysfs_int = false;
    pr_info("[ISP] = isp_resume\n");

    HalInitRegs((void**)data->isp_mem->pISPRegs);
    isp_restore_buff(); //recover isp registers which contain buffer address ,

    if(data->clk_isp) clk_prepare_enable(data->clk_isp);
    if(data->clk_sr) clk_prepare_enable(data->clk_sr);
    if(data->clk_sr_mclk) clk_prepare_enable(data->clk_sr_mclk);

    HalISPEnableInt();     // enable interrupt
    return 0;
}

void isp_apply_iq_at_vend(void)
{
    if(isp_int.yuv_ccm.dirty)
    {
        HalISPSetYUVCCM(isp_int.yuv_ccm.ccm);
        isp_int.yuv_ccm.dirty = 0;
        print_timestamp("K_CCM2");
    }

    if(isp_int.dgain.dirty)
    {
        HalIspSetAEDgain(isp_int.dgain.enable,isp_int.dgain.gain);
    }

#if 0
    if (isp_int.share_data->frame_state.u4OBCInValid == true){
      HalISPSetOBC(isp_int.share_data->frame_state.u4OBC_a, isp_int.share_data->frame_state.u4OBC_b);
      isp_int.share_data->frame_state.u4OBCInValid = false;
      print_timestamp("K_OBC");
    }
#endif
}

#define INC_TIME_REC(name) getnstimeofday(&isp_int.isp_int_time[name])
#define INC_TIME_REC2(name) getnstimeofday(&isp_int.isp_int_time2[name])
#define INC_TIME_REC3(name) getnstimeofday(&isp_int.isp_int_time3[name]))

#define INC_COUNT(name) {isp_int.isp_int_count[name]++;\
                                            getnstimeofday(&isp_int.isp_int_time[name]);}
#define INC_COUNT2(name) {isp_int.isp_int2_count[name]++;\
                                            getnstimeofday(&isp_int.isp_int2_time[name]);}
#define INC_COUNT3(name) {isp_int.isp_int3_count[name]++;\
                                            getnstimeofday(&isp_int.isp_int3_time[name]);}

irqreturn_t isp_ISR(int num, void *priv)
{
    volatile u32 u4Clear = 0;
    volatile u32 u4Clear2 = 0;
    volatile u32 u4Clear3 = 0;

    volatile u32 u4Status;
    volatile u32 u4Status2;
    volatile u32 u4Status3;

    //volatile u32 u4VsyncPol;
    //volatile u32 u4MIPI;

    isp_int.int_cnt++;

    u4Status = HalISPGetIntStatus1();
    //HalISPMaskInt1(u4Status);
    u4Status2 = HalISPGetIntStatus2();
    //HalISPMaskInt2(u4Status2);
    u4Status3 = HalISPGetIntStatus3();
    //HalISPMaskInt3(u4Status3);

    //u4VsyncPol = HalISPGetVsyncPol();

    //u4MIPI = HalISPGetMIPI();

    if (u4Status == 0 && u4Status2 == 0 && u4Status3 == 0) {
        pr_err("[ISP] False interrupt? mask1 0x%04x, mask2 0x%04x, mask3 0x%04x\n", u4Status, u4Status2, u4Status3);
        return IRQ_HANDLED;
    }

#if 0
    pr_info("c=%d sta=0x%x\n", isp_int.frame_cnt, u4Status);
#endif

    if (ISP_CHECKBITS(u4Status3, INT3_SW_INT_INPUT_DONE)) {
        DBG_INFO_1("INT3_SW_INT_INPUT_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear3, INT3_SW_INT_INPUT_DONE);
        INC_COUNT3(INT3_SW_INT_INPUT_DONE);
        //if(isp_int.fifo_mask.dirty)
        //{
        //    IspInputEnable(~isp_int.fifo_mask.enable);
        //    isp_int.fifo_mask.dirty = 0;
        //    if(!isp_int.fifo_mask.enable) pr_debug("ISP OFF\n");
        //}
        isp_int.vsync_end = true;
        isp_int.share_data->frame_state.bActive = false;
        //isp_apply_iq_at_vend();
        isp_mot_cache_invalidate();
        wake_up_interruptible_all(&isp_wq_VEND);
        wake_up_interruptible_all(&isp_wq_sw_int_in);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= FRAME_END_EVENT;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
      }

    ////////////////////////////////////////////
    //             Vsync Start                //
    ////////////////////////////////////////////
    //all sensor use ISP vsync as Frame start
    if (ISP_CHECKBITS(u4Status, INT_ISPIF_VSYNC)){
        HalISPClearInt(&u4Clear, INT_ISPIF_VSYNC);
        INC_COUNT(INT_ISPIF_VSYNC);
        isp_int.vsync_start = true;
        isp_int.frame_cnt++;
        isp_int.share_data->frame_state.u4FrameCnt = isp_int.frame_cnt;
        isp_int.share_data->frame_state.bActive = true;
        isp_int.hw_frame_cnt = (HalISPGetFrameDoneCount()+ 1) & 0x7F; //get frame done count in VSync and return it at frame end interrupt
        fps_update(&isp_int.fps);
        wake_up_interruptible_all(&isp_wq_VSTART);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= FRAME_START_EVENT;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
        DBG_INFO_1("MIPI VSYNC ACTIVE %d", isp_int.frame_cnt);
        pr_debug("VS\n");

        //TEST CODE
        print_timestamp("K_VS");//TEST CODE

        //isp_fs poll support
        {
            unsigned long flag;
            struct list_head *pos;
            IspIrqLink_t *elm;
            spin_lock_irqsave(&g_IspFsLock,flag);
            list_for_each(pos,&gIspFsWqList)
            {
                elm = list_entry(pos,IspIrqLink_t,list);
                elm->ready = 1;
                elm->fcount = isp_int.frame_cnt;
                wake_up_interruptible_all(&elm->waitq);
            }
            spin_unlock_irqrestore(&g_IspFsLock,flag);
        }
    }

      //////////////////////////////////////////////
      //           statistics                     //
      //////////////////////////////////////////////
    if (ISP_CHECKBITS(u4Status, INT_AE_DONE)){
        pr_debug("AE\n");
        isp_ae_cache_invalidate();
        DBG_INFO_1("INT_AE_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear, INT_AE_DONE);
        INC_COUNT(INT_AE_DONE);
        isp_int.ae = true;
        wake_up_interruptible_all(&isp_wq_ae);
    }

    // AE WIN0/1 Int Row
    if (ISP_CHECKBITS(u4Status3, INT3_AE_WIN0_DONE)) {
        pr_debug("AE_WIN0\n");
        DBG_INFO_1("INT3_AE_WIN0_DONE %d\n", isp_int.frame_cnt);
        isp_int.ae_win0 = true;
        HalISPClearInt(&u4Clear3, INT3_AE_WIN0_DONE);
        wake_up_interruptible_all(&isp_wq_ae_win0);

        //isp_ae poll support
        {
            unsigned long flag;
            struct list_head *pos;
            IspIrqLink_t *elm;
            spin_lock_irqsave(&g_IspAeLock,flag);
            list_for_each(pos,&gIspAeWqList)
            {
                elm = list_entry(pos,IspIrqLink_t,list);
                elm->ready = 1;
                elm->fcount = isp_int.frame_cnt;
                wake_up_interruptible_all(&elm->waitq);
            }
            spin_unlock_irqrestore(&g_IspAeLock,flag);
        }
    }

    if (ISP_CHECKBITS(u4Status3, INT3_AE_BLK_ROW_INT_DONE)) {
        pr_debug("AE_ROW\n");
        DBG_INFO_1("INT3_AE_BLK_ROW_INT_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear3, INT3_AE_BLK_ROW_INT_DONE);
        INC_COUNT3(INT3_AE_BLK_ROW_INT_DONE);
        isp_int.ae_row_int= true;
        wake_up_interruptible_all(&isp_wq_ae_row_int);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= AE_DONE_EVENT;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
    }

    if (ISP_CHECKBITS(u4Status, INT_AWB_DONE)) {
        isp_awb_cache_invalidate(); //invalid cache data
        DBG_INFO_1("INT_AWB_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear, INT_AWB_DONE);
        INC_COUNT(INT_AWB_DONE);
        isp_int.awb = true;
        wake_up_interruptible_all(&isp_wq_awb);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= AWB_DONE_EVENT ;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
    }

    if (ISP_CHECKBITS(u4Status, INT_AF_DONE)) {
        isp_af_cache_invalidate();
        DBG_INFO_1("INT_AF_DONE %d", isp_int.frame_cnt);
        HalISPGetAFStat(isp_int.AF_Stat);
        HalISPClearInt(&u4Clear, INT_AF_DONE);
        INC_COUNT(INT_AF_DONE);
        isp_int.af = true;
        wake_up_interruptible_all(&isp_wq_af);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= AF_DONE_EVENT ;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
    }

	if (ISP_CHECKBITS(u4Status3, INT3_HIT_LINE_COUNT1)) {
		DBG_INFO_1("INT3_HIT_LINE_COUNT1 %d", isp_int.frame_cnt);
		HalISPClearInt(&u4Clear3, INT3_HIT_LINE_COUNT1);
		INC_COUNT3(INT3_HIT_LINE_COUNT1);
		wake_up_interruptible_all(&isp_wq_hit_line_count1);
	}

	if (ISP_CHECKBITS(u4Status3, INT3_HIT_LINE_COUNT2)) {
		DBG_INFO_1("INT3_HIT_LINE_COUNT2 %d", isp_int.frame_cnt);
		HalISPClearInt(&u4Clear3, INT3_HIT_LINE_COUNT2);
		INC_COUNT3(INT3_HIT_LINE_COUNT2);
		wake_up_interruptible_all(&isp_wq_hit_line_count2);
	}

	if (ISP_CHECKBITS(u4Status3, INT3_HIT_LINE_COUNT3)) {
		DBG_INFO_1("INT3_HIT_LINE_COUNT3 %d", isp_int.frame_cnt);
		HalISPClearInt(&u4Clear3, INT3_HIT_LINE_COUNT3);
		INC_COUNT3(INT3_HIT_LINE_COUNT3);
		//wake_up_interruptible_all(&isp_wq_hit_line_count3);
		isp_int.isp_idle = true;
		wake_up_interruptible_all(&isp_wq_ISP_IDLE);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= IDLE_EVENT;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
	}

	if (ISP_CHECKBITS(u4Status3, INT3_HDR_HISTO_DONE)) {
		DBG_INFO_1("INT3_HDR_HISTO_DONE %d", isp_int.frame_cnt);
		HalISPClearInt(&u4Clear3, INT3_HDR_HISTO_DONE);
		INC_COUNT3(INT3_HDR_HISTO_DONE);
        isp_histo_cache_invalidate();
		wake_up_interruptible_all(&isp_wq_hdr_histo_done);
	}

	if (ISP_CHECKBITS(u4Status3, INT3_RGBIR_HISTO_DONE)) {
		DBG_INFO_1("INT3_RGBIR_HISTO_DONE %d", isp_int.frame_cnt);
		HalISPClearInt(&u4Clear3, INT3_RGBIR_HISTO_DONE);
		INC_COUNT3(INT3_RGBIR_HISTO_DONE);
        isp_histo_cache_invalidate();
		wake_up_interruptible_all(&isp_wq_rgbir_histo_done);
	}

	if (ISP_CHECKBITS(u4Status3, INT3_AWB_ROW_DONE)) {
		DBG_INFO_1("INT3_AWB_ROW_DONE %d", isp_int.frame_cnt);
		HalISPClearInt(&u4Clear3, INT3_AWB_ROW_DONE);
		INC_COUNT3(INT3_AWB_ROW_DONE);
		wake_up_interruptible_all(&isp_wq_awb_row_done);
	}

	if (ISP_CHECKBITS(u4Status3, INT3_HISTO_ROW_DONE)) {
		DBG_INFO_1("INT3_HISTO_ROW_DONE %d", isp_int.frame_cnt);
		HalISPClearInt(&u4Clear3, INT3_HISTO_ROW_DONE);
		INC_COUNT3(INT3_HISTO_ROW_DONE);
		wake_up_interruptible_all(&isp_wq_histo_row_done);
	}

	if (ISP_CHECKBITS(u4Status3, INT3_SW_INT_OUTPUT_DONE)) {
        DBG_INFO_1("INT3_SW_INT_OUTPUT_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear3, INT3_SW_INT_OUTPUT_DONE);
        INC_COUNT3(INT3_SW_INT_OUTPUT_DONE);

        if(isp_int.fifo_mask.dirty)
        {
            IspInputEnable(~isp_int.fifo_mask.enable);
            isp_int.fifo_mask.dirty = 0;
            if(!isp_int.fifo_mask.enable) pr_debug("ISP OFF\n");
        }
        isp_apply_iq_at_vend();
        wake_up_interruptible_all(&isp_wq_sw_int_out);
    }

    if (ISP_CHECKBITS(u4Status, INT_ISP_BUSY)) {
        HalISPClearInt(&u4Clear, INT_ISP_BUSY);
        INC_COUNT(INT_ISP_BUSY);
        DBG_INFO_1("INT_ISP_BUSY %d", isp_int.frame_cnt);
        isp_int.isp_busy = true;
        wake_up_interruptible_all(&isp_wq_ISP_BUSY);
        print_timestamp("K_BUSY");//TEST CODE
    }

    if (ISP_CHECKBITS(u4Status, INT_ISP_IDLE)) {
        HalISPClearInt(&u4Clear, INT_ISP_IDLE);
        INC_COUNT(INT_ISP_IDLE);
        DBG_INFO_1("INT_ISP_IDLE %d", isp_int.frame_cnt);
        //isp_int.isp_idle = true;
        //wake_up_interruptible_all(&isp_wq_ISP_IDLE);
        print_timestamp("K_IDLE");//TEST CODE
    }

    //////////////////////////////
    //             Vsync end                  //
    //////////////////////////////

#if 0
    if(ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_RISING))
    {
        INC_COUNT(INT_PAD_VSYNC_RISING);
        HalISPClearInt(&u4Clear, INT_PAD_VSYNC_RISING);
    }

    if(ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_FALLING))
    {
        INC_COUNT(INT_PAD_VSYNC_FALLING);
        HalISPClearInt(&u4Clear, INT_PAD_VSYNC_FALLING);
    }

    if (!u4MIPI){
        if ((ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_RISING) && u4VsyncPol == false) || (ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_FALLING) && u4VsyncPol == true)){
            if (isp_int.vsync_end == false){
                if (ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_RISING) && u4VsyncPol == false ){
                    HalISPClearInt(&u4Clear, INT_PAD_VSYNC_RISING);
                    //INC_COUNT(INT_PAD_VSYNC_RISING);
                    //if(isp_int.fifo_mask.dirty)
                    //{
                    //    IspInputEnable(~isp_int.fifo_mask.enable);
                    //    isp_int.fifo_mask.dirty = 0;
                    //}
                    if (isp_int.frame_cnt%30==0)
                        DBG_INFO_1("Parallel Vsync End INT_PAD_VSYNC_RISING %d", isp_int.frame_cnt);
                }
                else if (ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_FALLING) && u4VsyncPol == true){
                    HalISPClearInt(&u4Clear, INT_PAD_VSYNC_FALLING);
                    //INC_COUNT(INT_PAD_VSYNC_FALLING);
                    //if(isp_int.fifo_mask.dirty)
                    //{
                    //    IspInputEnable(~isp_int.fifo_mask.enable);
                    //    isp_int.fifo_mask.dirty = 0;
                    //}
                    if (isp_int.frame_cnt%30==0)
                        DBG_INFO_1("Parallel Vsync End INT_PAD_VSYNC_FALLING %d", isp_int.frame_cnt);
                }

                DBG_INFO_1(KERN_NOTICE "Parallel VSYNC End...%d %d\n", isp_int.share_data->frame_state.bActive, isp_int.share_data->frame_state.u4FrameCnt);
                isp_int.vsync_end = true;
                isp_int.share_data->frame_state.bActive = false;
                isp_apply_iq_at_vend();
                isp_mot_cache_invalidate();
                wake_up_interruptible_all(&isp_wq_VEND);
                // Using menuload to write IQ table to SRAMs
                pr_debug("VE\n");
                //isp_write_iq_tbl();
            }else{
                //DBG_INFO("Parallel Vsync End ---FALSE---");
            }
        }
    }
#endif
    ////////////////////////////////////////////
    //             DMA                        //
    ////////////////////////////////////////////
    if (ISP_CHECKBITS(u4Status, INT_WDMA_DONE) && isp_int.wdma_done == false) {
        HalISPClearInt(&u4Clear, INT_WDMA_DONE);
        INC_COUNT(INT_WDMA_DONE);
        HalISPClearInt(&u4Clear, INT_PAD_VSYNC_RISING);
        HalISPClearInt(&u4Clear, INT_PAD_VSYNC_FALLING);
        DBG_INFO_1("INT_WDMA_DONE %d", isp_int.frame_cnt);
        isp_int.wdma_done = true;
        wake_up_interruptible_all(&isp_wq_WDMA_DONE);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= WDMA_DONE_EVENT;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
    } else {
        //DBG_INFO("INT_WDMA_DONE %d, --FALSE--", isp_int.frame_cnt);
    }

    if (ISP_CHECKBITS(u4Status, INT_RDMA_DONE) && isp_int.rdma_done == false) {
        HalISPClearInt(&u4Clear, INT_RDMA_DONE);
        INC_COUNT(INT_RDMA_DONE);
        DBG_INFO_1("INT_RDMA_DONE %d", isp_int.frame_cnt);
        isp_int.rdma_done = true;
        wake_up_interruptible_all(&isp_wq_RDMA_DONE);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= RDMA_DONE_EVENT;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
    } else {
        //DBG_INFO("INT_RDMA_DONE %d, --FALSE--", isp_int.frame_cnt);
    }

    ////////////////////////////////////////////
    //             FIFO Full                  //
    ////////////////////////////////////////////
    if (ISP_CHECKBITS(u4Status, INT_ISP_FIFO_FULL))
    {
        HalISPClearInt(&u4Clear, INT_ISP_FIFO_FULL);
        INC_COUNT(INT_ISP_FIFO_FULL);
        //DBG_INFO_1("INT_ISP_FIFO_FULL %d", isp_int.frame_cnt);
        pr_debug("isp fifo full %d", isp_int.frame_cnt);
        isp_int.isp_fifofull = true;
        IspReset(); //force ISP reset , when ISP FIFO FULL happen
        wake_up_interruptible_all(&isp_wq_ISP_FIFO_FULL);
        isp_int.share_data->isp_ints_state.u32IspInt1 |= FIFO_FULL_EVENT;
        isp_int.epoll_event = true;
        wake_up_interruptible_all(&isp_wq_epoll_event);
    }

    if (ISP_CHECKBITS(u4Status, INT_WDMA_FIFO_FULL) && isp_int.wdma_fifofull == false){
        HalISPClearInt(&u4Clear, INT_WDMA_FIFO_FULL);
        INC_COUNT(INT_WDMA_FIFO_FULL);
        DBG_INFO_1("INT_WDMA_FIFO_FULL %d", isp_int.frame_cnt);
        isp_int.wdma_fifofull = true;
        wake_up_interruptible_all(&isp_wq_WDMA_FIFO_FULL);
    }

    if (ISP_CHECKBITS(u4Status2, INT2_VDOS_LINE2)) {
        HalISPClearInt(&u4Clear2, INT2_VDOS_LINE2);
        INC_COUNT2(INT2_VDOS_LINE2);
        DBG_INFO_1("INT2_VDOS_LINE2 %d", isp_int.frame_cnt);
        isp_int.eis = true;
        wake_up_interruptible_all(&isp_wq_eis);
    }

    HalISPMaskClear1(u4Clear);
    HalISPMaskClear2(u4Clear2);
    HalISPMaskClear3(u4Clear3);
    if (isp_int.frame_cnt%1200==0){
        pr_debug("end of ISR, u4Status:0x%04x/0x%04x/0x%04x, u4Clear:0x%04x/0x%04x/0x%04x", u4Status, u4Status2, u4Status3, u4Clear, u4Clear2, u4Clear3);
    }
    return IRQ_HANDLED;
}

static int isp_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
    typedef struct {
        unsigned long u4BaseAddr;
        u32 u4Size;
    } FRAMEINFO_MEM;

    FRAMEINFO_MEM frameinfo;
    frameinfo.u4BaseAddr = isp_mem.ShareData_Meminfo.phys;
    frameinfo.u4Size = sizeof(ISP_SHARE_DATA);
    data_size = sizeof(FRAMEINFO_MEM);
    if (buf && size >= data_size) {
        DBG_INFO("buff in %d, out %d\n", size, data_size);
        memcpy((void *)buf, (void *)&frameinfo, data_size);
    }
    return data_size;
}

static int isp_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    if (buf && size >= 1) {
        reset_isp_int(&isp_int);
        isp_int.sysfs_int = (buf[0] & 0x01);
    } else {
        //memset((void*)&isp_int, 0, sizeof(ISP_INT_DATA));
        reset_isp_int(&isp_int);
        isp_int.sysfs_int = false;
    }

    if (isp_int.sysfs_int == true)
      HalISPEnableInt();
    else
      HalISPDisableInt();

#if defined(FPGA_VERIFY)
	fpgaInit();
#endif

    return size;
}

static int vs_sr_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_VSTART, isp_int.vsync_start == true))
    {
        C_IN(isp_lock);
        isp_int.vsync_start = false;
        C_OUT(isp_lock);
        return -ERESTARTSYS;
    }
    C_IN(isp_lock);
    isp_int.vsync_start = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return size;
}

static int vs_sr_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

//typedef struct
//{
//  u32  frame_cnt;
//  u32  hw_frame_cnt;
//}__attribute__((packed, aligned(1))) ve_isr_data; //vsync end isr data

static int ve_sr_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    ve_isr_data ve_data;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_VEND, isp_int.vsync_end == true)){
        C_IN(isp_lock);
        isp_int.vsync_end = false;
        C_OUT(isp_lock);
        return -ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.vsync_end = false;
    ve_data.frame_cnt = isp_int.frame_cnt;
    ve_data.hw_frame_cnt = isp_int.hw_frame_cnt;
    C_OUT(isp_lock);

    if (buf){
	 size = size>sizeof(ve_data)?sizeof(ve_data):size;
	 memcpy((void*)buf,(void*)&ve_data,size);
    }
    return size;
}

static int ve_sr_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int ae_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ae, isp_int.ae == true)){
        C_IN(isp_lock);
        isp_int.ae = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.ae = false;
    C_OUT(isp_lock);

    // TODO
    //DBG_INFO_2("ae virtual offset[%d]: 0x%08x", isp_mem.AE_ReadIdx, (u32)isp_mem.AE_Offset[isp_mem.AE_ReadIdx]);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return size;
}
static int ae_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int ae_win0_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ae_win0, isp_int.ae_win0 == true)){
        C_IN(isp_lock);
        isp_int.ae_win0 = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.ae_win0 = false;
    C_OUT(isp_lock);

    //DBG_INFO_2("ae virtual offset[%d]: 0x%08x", isp_mem.AE_ReadIdx, (u32)isp_mem.AE_Offset[isp_mem.AE_ReadIdx]);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return size;
}

static int ae_win0_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int ae_win1_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    return 0;
}

static int ae_win1_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int ae_row_int_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ae_row_int, isp_int.ae_row_int== true)){
        C_IN(isp_lock);
        isp_int.ae_row_int = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.ae_row_int = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return size;
}

static int ae_row_int_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int sw_int_in_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    return 0;
}

static int sw_int_in_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int sw_int_out_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    return 0;
}

static int sw_int_out_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int awb_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_awb, isp_int.awb == true))
    {
        C_IN(isp_lock);
        isp_int.awb = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.awb = false;
    C_OUT(isp_lock);

    //DBG_INFO_2("awb virtual offset[%d]: 0x%08x", isp_mem.AWB_ReadIdx, (u32)isp_mem.AWB_Offset[isp_mem.AWB_ReadIdx]);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return size;
}

static int awb_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int af_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_af, isp_int.af == true)){
        C_IN(isp_lock);
        isp_int.af = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.af = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return size;
}
static int af_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int busy_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    //int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ISP_BUSY, isp_int.isp_busy == true))
    {
        C_IN(isp_lock);
        isp_int.isp_busy = false;
        C_OUT(isp_lock);

        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.isp_busy = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return size;
}

static int busy_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

static int idle_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ISP_IDLE, isp_int.isp_idle == true))
    {
        C_IN(isp_lock);
        isp_int.isp_idle = false;
        C_OUT(isp_lock);

        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.isp_idle = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return data_size;
}
static int idle_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

/// isp fifo full
static int isp_fifofull_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ISP_FIFO_FULL, isp_int.isp_fifofull == true))
    {
        C_IN(isp_lock);
        isp_int.isp_fifofull = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.isp_fifofull = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }


    return data_size;
}
static int isp_fifofull_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

// wdma fifo full
static int wdma_fifofull_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_WDMA_FIFO_FULL, isp_int.wdma_fifofull == true))
    {
        C_IN(isp_lock);
        isp_int.wdma_fifofull = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.wdma_fifofull = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return data_size;
}

static int wdma_fifofull_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

// wdma done
static int wdma_done_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_WDMA_DONE, isp_int.wdma_done == true))
    {
        C_IN(isp_lock);
        isp_int.wdma_done = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.wdma_done = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return data_size;
}

static int wdma_done_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

// rdma done
static int rdma_done_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_RDMA_DONE, isp_int.rdma_done == true))
    {
        C_IN(isp_lock);
        isp_int.rdma_done = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }
    C_IN(isp_lock);
    isp_int.rdma_done = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return data_size;
}

static int rdma_done_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

// eis vdos
static int eis_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{  // FIXJASON
    unsigned long flags;

    if (wait_event_interruptible(isp_wq_eis, isp_int.eis == true))
    {
        C_IN(isp_lock);
        isp_int.eis = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.eis = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return 0;
}
static int eis_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{  // FIXJASON
    DBG_INFO();
    return size;
}

// SCL vsync end
static int scl_fe_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_scl_fe, isp_int.scl_fe == true))
    {
        C_IN(isp_lock);
        isp_int.scl_fe = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.scl_fe = false;
    C_OUT(isp_lock);

    if(buf)
    {
        isp_isr_event_data data;
        data.fcount = isp_int.frame_cnt;
        size = size>sizeof(data)?sizeof(data):size;
        memcpy((void*)buf,(void*)&data,size);
    }
    else
    {
        size= 0;
    }

    return 0;
}

static int scl_fe_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

void DrvISPMLoadWriteData(volatile unsigned int Sram_Id, volatile unsigned long Addr, size_t size)
{
	return HalISPMLoadWriteData(Sram_Id, Addr, size);
}

void DrvISPMLoadReadData(ISP_MLOAD_ID Sram_Id, volatile unsigned short *table, size_t size)
{
	// only for gamma10to10 read
	switch (Sram_Id){
		case eMLOAD_ID_LN_GMA10TO10_R:
		case eMLOAD_ID_LN_GMA10TO10_G:
		case eMLOAD_ID_LN_GMA10TO10_B:
			wait_event_interruptible(isp_wq_ISP_IDLE, isp_int.isp_idle == true);
			// wait 8 line buffer time 8 * 30 us
			udelay(300);
			break;
		default:
			break;
	}
	return HalISPMLoadReadData(Sram_Id, table, size);
}

static int mload_ALSC_R_TBL_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = ALSC_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_ALSC_R_TBL, (unsigned short*)buf, ALSC_TBL_SIZE);
	return data_size;
}

static int mload_ALSC_R_TBL_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	//pr_info("[%s] called, offset = %#llx , size = %#x", __func__, offset, size);
	if(offset == 0)
	{
		//pr_info("memset mload_virt_addr\n");
		memset(mload_virt_addr,0, ALSC_TBL_SIZE*2);
	}
	memcpy(mload_virt_addr+offset, buf, size);

	if(offset+size == ((ALSC_TBL_SIZE-1)*2))
	{
		//pr_info("[DrvISPMLoadWriteData] called, offset = %llx , size = %#x", offset, size);
		DrvISPMLoadWriteData(eMLOAD_ID_ALSC_R_TBL, mload_dma_addr, ALSC_TBL_SIZE);
	}
	return size;
}

static int mload_ALSC_G_TBL_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = ALSC_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_ALSC_G_TBL, (unsigned short*)buf, ALSC_TBL_SIZE);
	return data_size;
}

static int mload_ALSC_G_TBL_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	//pr_info("[%s] called, offset = %#llx , size = %#x", __func__, offset, size);
	if(offset == 0)
	{
		//pr_info("memset mload_virt_addr\n");
		memset(mload_virt_addr,0, ALSC_TBL_SIZE*2);
	}
	memcpy(mload_virt_addr+offset, buf, size);

	if(offset+size == ((ALSC_TBL_SIZE-1)*2))
	{
		//pr_info("[DrvISPMLoadWriteData] called, offset = %llx , size = %#x", offset, size);
		DrvISPMLoadWriteData(eMLOAD_ID_ALSC_G_TBL, mload_dma_addr, ALSC_TBL_SIZE);
	}
	return size;
}

static int mload_ALSC_B_TBL_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = ALSC_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_ALSC_B_TBL, (unsigned short*)buf, ALSC_TBL_SIZE);
	return data_size;
}

static int mload_ALSC_B_TBL_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	//pr_info("[%s] called, offset = %#llx , size = %#x", __func__, offset, size);
	if(offset == 0)
	{
		//pr_info("memset mload_virt_addr\n");
		memset(mload_virt_addr,0, ALSC_TBL_SIZE*2);
	}
	memcpy(mload_virt_addr+offset, buf, size);

	if(offset+size == ((ALSC_TBL_SIZE-1)*2))
	{
		//pr_info("[DrvISPMLoadWriteData] called, offset = %llx , size = %#x", offset, size);
		DrvISPMLoadWriteData(eMLOAD_ID_ALSC_B_TBL, mload_dma_addr, ALSC_TBL_SIZE);
	}
	return size;
}


static int mload_LN_GMA12TO10_R_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = GAMMA_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_LN_GMA12TO10_R, (unsigned short*)buf, GAMMA_TBL_SIZE);
	return data_size;
}

static int mload_LN_GMA12TO10_R_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0, GAMMA_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, GAMMA_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_LN_GMA12TO10_R, mload_dma_addr, GAMMA_TBL_SIZE);
	}
	return size;
}

static int mload_LN_GMA12TO10_G_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = GAMMA_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_LN_GMA12TO10_G, (unsigned short*)buf, GAMMA_TBL_SIZE);
	return data_size;
}

static int mload_LN_GMA12TO10_G_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0, GAMMA_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, GAMMA_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_LN_GMA12TO10_G, mload_dma_addr, GAMMA_TBL_SIZE);
	}
	return size;
}

static int mload_LN_GMA12TO10_B_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = GAMMA_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_LN_GMA12TO10_B, (unsigned short*)buf, GAMMA_TBL_SIZE);
	return data_size;
}

static int mload_LN_GMA12TO10_B_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0, GAMMA_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, GAMMA_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_LN_GMA12TO10_B, mload_dma_addr, GAMMA_TBL_SIZE);
	}
	return size;
}

static int mload_DP_TBL_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = DEFECTPIX_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_DP_TBL, (unsigned short*)buf, DEFECTPIX_TBL_SIZE);
	return data_size;
}

static int mload_DP_TBL_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0, DEFECTPIX_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, DEFECTPIX_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_DP_TBL, mload_dma_addr, DEFECTPIX_TBL_SIZE);
	}
	return size;
}

static int mload_LN_GMA10TO10_R_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = GAMMA_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_LN_GMA10TO10_R, (unsigned short*)buf, GAMMA_TBL_SIZE);
	return data_size;
}

static int mload_LN_GMA10TO10_R_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0, GAMMA_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, GAMMA_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_LN_GMA10TO10_R, mload_dma_addr, GAMMA_TBL_SIZE);
	}
	return size;
}

static int mload_LN_GMA10TO10_G_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = GAMMA_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_LN_GMA10TO10_G, (unsigned short*)buf, GAMMA_TBL_SIZE);
	return data_size;
}

static int mload_LN_GMA10TO10_G_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0, GAMMA_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, GAMMA_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_LN_GMA10TO10_G, mload_dma_addr, GAMMA_TBL_SIZE);
	}
	return size;
}

static int mload_LN_GMA10TO10_B_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = GAMMA_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_LN_GMA10TO10_B, (unsigned short*)buf, GAMMA_TBL_SIZE);
	return data_size;
}

static int mload_LN_GMA10TO10_B_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0, GAMMA_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, GAMMA_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_LN_GMA10TO10_B, mload_dma_addr, GAMMA_TBL_SIZE);
	}
	return size;
}

static int mload_FPN_OFFSET_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int data_size = FPN_OFFSET_TBL_SIZE*2;
	memset(buf, 0, data_size);
	DrvISPMLoadReadData(eMLOAD_ID_FPN_OFFSET, (unsigned short*)buf, FPN_OFFSET_TBL_SIZE);
	return data_size;
}

static int mload_FPN_OFFSET_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	if(offset == 0)
	{
		memset(mload_virt_addr,0,FPN_OFFSET_TBL_SIZE*2);
		memcpy(mload_virt_addr,buf, FPN_OFFSET_TBL_SIZE*2);
		DrvISPMLoadWriteData(eMLOAD_ID_FPN_OFFSET, mload_dma_addr, FPN_OFFSET_TBL_SIZE);
	}
	return size;
}

static int mload_ALL_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	return 0;
}

static int mload_ALL_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	//pr_info("[%s] called, offset = %#llx , size = %#x", __func__, offset, size);
	if(offset == 0)
	{
		//pr_info("memset mload_virt_addr\n");
		memset(mload_virt_addr,0, sizeof(MLoadLayout));
	}
	memcpy(mload_virt_addr+offset, buf, size);

	if(offset+size == sizeof(MLoadLayout))
	{
		//pr_info("[HalISPMLoadWriteAllTable] called\n");
		HalISPMLoadWriteAllTable(mload_dma_addr);
	}
	return size;
}


void isp_to_scl_ve_isr(void)
{
    print_timestamp("K_ISP2SCL");
}

void scl_ve_isr(void)
{
    print_timestamp("K_SCL_VE");
    if (!isp_int.scl_fe){
        isp_int.scl_fe = true;
        wake_up_interruptible_all(&isp_wq_scl_fe);
    }
    isp_int.share_data->isp_ints_state.u32IspInt1 |= SCL_VEND_EVENT;
    isp_int.epoll_event = true;
    wake_up_interruptible_all(&isp_wq_epoll_event);
}

void scl_vs_isr(void)
{
    print_timestamp("K_SCL_VS");

}

static ISP_BIN_ATTR(isp, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(vs_sr, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(ve_sr, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(ae, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(ae_win0, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(ae_win1, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(ae_row_int, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(awb, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(af, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(busy, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(idle, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(isp_fifofull, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(wdma_fifofull, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(wdma_done, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(rdma_done, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(eis, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(sw_int_in, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(sw_int_out, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(scl_fe, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_ALSC_R_TBL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_ALSC_G_TBL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_ALSC_B_TBL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_LN_GMA12TO10_R, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_LN_GMA12TO10_G, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_LN_GMA12TO10_B, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_DP_TBL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_LN_GMA10TO10_R, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_LN_GMA10TO10_G, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_LN_GMA10TO10_B, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_FPN_OFFSET, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mload_ALL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);

static ssize_t isp_ints_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%s			: %d\n","frame_count",isp_int.frame_cnt);
    str += scnprintf(str, end - str, "%s		: %d\n","frame_interval(ns)",isp_int.fps.frame_interval);
    str += scnprintf(str, end - str, "%s			: %d\n","SR_VREF_RISING",isp_int.isp_int_count[INT_SR_VREF_RISING]);
    str += scnprintf(str, end - str, "%s			: %d\n","SR_VREF_FALLING",isp_int.isp_int_count[INT_SR_VREF_FALLING]);
    str += scnprintf(str, end - str, "%s			: %d\n","STROBE_DONE",isp_int.isp_int_count[INT_STROBE_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","DB_UPDATE_DONE",isp_int.isp_int_count[INT_DB_UPDATE_DONE]);

    //AF_DONE
    str += scnprintf(str, end - str,  "%s			        : %d ,%u.%09u\n","AF_DONE"
    ,isp_int.isp_int_count[INT_AF_DONE]
    ,(unsigned int)isp_int.isp_int_time[INT_AF_DONE].tv_sec
    ,(unsigned int)isp_int.isp_int_time[INT_AF_DONE].tv_nsec);

    str += scnprintf(str, end - str, "%s			: %d\n","ISP_FIFO_FUL",isp_int.isp_int_count[INT_ISP_FIFO_FULL]);
    str += scnprintf(str, end - str, "%s			: %d\n","ISP_BUSY",isp_int.isp_int_count[INT_ISP_BUSY]);
    str += scnprintf(str, end - str, "%s			: %d\n","ISP_IDLE",isp_int.isp_int_count[INT_ISP_IDLE]);

    //AWB
    str += scnprintf(str, end - str,  "%s			: %d ,%u.%09u\n","AWB_DONE"
    ,isp_int.isp_int_count[INT_AWB_DONE]
    ,(unsigned int)isp_int.isp_int_time[INT_AWB_DONE].tv_sec
    ,(unsigned int)isp_int.isp_int_time[INT_AWB_DONE].tv_nsec);

    str += scnprintf(str, end - str, "%s			: %d\n","WDMA_DONE",isp_int.isp_int_count[INT_WDMA_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","RDMA_DONE",isp_int.isp_int_count[INT_RDMA_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","WDMA_FIFO_FULL",isp_int.isp_int_count[INT_WDMA_FIFO_FULL]);

    //PAD_VSYNC_RISING
    str += scnprintf(str, end - str,  "%s		: %d ,%u.%09u\n","PAD_VSYNC_RISING"
    ,isp_int.isp_int_count[INT_PAD_VSYNC_RISING]
    ,(unsigned int)isp_int.isp_int_time[INT_PAD_VSYNC_RISING].tv_sec
    ,(unsigned int)isp_int.isp_int_time[INT_PAD_VSYNC_RISING].tv_nsec);

    //PAD_VSYNC_FALLING
    str += scnprintf(str, end - str,  "%s		: %d ,%u.%09u\n","PAD_VSYNC_FALLING"
    ,isp_int.isp_int_count[INT_PAD_VSYNC_FALLING]
    ,(unsigned int)isp_int.isp_int_time[INT_PAD_VSYNC_FALLING].tv_sec
    ,(unsigned int)isp_int.isp_int_time[INT_PAD_VSYNC_FALLING].tv_nsec);

    //ISPIF_VSYNC
    str += scnprintf(str, end - str,"%s			: %d ,%u.%09u\n","ISPIF_VSYNC"
    ,isp_int.isp_int_count[INT_ISPIF_VSYNC]
    ,(unsigned int)isp_int.isp_int_time[INT_ISPIF_VSYNC].tv_sec
    ,(unsigned int)isp_int.isp_int_time[INT_ISPIF_VSYNC].tv_nsec);

    //AE_DONE
    str += scnprintf(str, end - str, "%s			: %d ,%u.%09u\n","AE_DONE    "
    ,isp_int.isp_int_count[INT_AE_DONE]
    ,(unsigned int)isp_int.isp_int_time[INT_AE_DONE].tv_sec
    ,(unsigned int)isp_int.isp_int_time[INT_AE_DONE].tv_nsec);

    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_EVERYLINE",isp_int.isp_int2_count[INT2_VDOS_EVERYLINE]);
    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_LINE0",isp_int.isp_int2_count[INT2_VDOS_LINE0]);
    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_LINE1 ",isp_int.isp_int2_count[INT2_VDOS_LINE1]);
    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_LINE2",isp_int.isp_int2_count[INT2_VDOS_LINE2]);

    str += scnprintf(str, end - str, "%s			: %d\n","AE_WIN0_DONE",isp_int.isp_int3_count[INT3_AE_WIN0_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","AE_WIN1_DONE",isp_int.isp_int3_count[INT3_AE_WIN1_DONE]);

    //INT3_AE_BLK_ROW_INT_DONE
    str += scnprintf(str, end - str, "%s	: %d ,%u.%09u\n","INT3_AE_BLK_ROW_INT_DONE"
    ,isp_int.isp_int3_count[INT3_AE_BLK_ROW_INT_DONE]
    ,(unsigned int)isp_int.isp_int3_time[INT3_AE_BLK_ROW_INT_DONE].tv_sec
    ,(unsigned int)isp_int.isp_int3_time[INT3_AE_BLK_ROW_INT_DONE].tv_nsec);

    str += scnprintf(str, end - str, "%s			: %d\n","MENULOAD_DONE",isp_int.isp_int3_count[INT3_MENULOAD_DONE]);
    str += scnprintf(str, end - str, "%s		: %d\n","SW_INT_INPUT_DONE",isp_int.isp_int3_count[INT3_SW_INT_INPUT_DONE]);
    str += scnprintf(str, end - str, "%s		: %d\n","SW_INT_OUTPUT_DONE",isp_int.isp_int3_count[INT3_SW_INT_OUTPUT_DONE]);

    str += scnprintf(str, end - str, "%s		: %d\n","HIT_LINE_COUNT1",isp_int.isp_int3_count[INT3_HIT_LINE_COUNT1]);
    str += scnprintf(str, end - str, "%s		: %d\n","HIT_LINE_COUNT2",isp_int.isp_int3_count[INT3_HIT_LINE_COUNT2]);
    str += scnprintf(str, end - str, "%s		: %d\n","HIT_LINE_COUNT3",isp_int.isp_int3_count[INT3_HIT_LINE_COUNT3]);
    str += scnprintf(str, end - str, "%s		: %d\n","HDR_HISTO_DONE",isp_int.isp_int3_count[INT3_HDR_HISTO_DONE]);
    str += scnprintf(str, end - str, "%s		: %d\n","RGBIR_HISTO_DONE",isp_int.isp_int3_count[INT3_RGBIR_HISTO_DONE]);
    str += scnprintf(str, end - str, "%s		: %d\n","AWB_ROW_DONE",isp_int.isp_int3_count[INT3_AWB_ROW_DONE]);
    str += scnprintf(str, end - str, "%s		: %d\n","HISTO_ROW_DONE",isp_int.isp_int3_count[INT3_HISTO_ROW_DONE]);
    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}
DEVICE_ATTR(isp_ints, 0444, isp_ints_show, NULL);

static ssize_t isp_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    //isp_dev_data *data = (isp_dev_data*) dev->platform_data;
    int n=0;
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    for(n=0;n<ISP_INFO_ID_MAX;n++)
        str += scnprintf(str, end - str, "%s:%d","TEST",n);
        //str += scnprintf(str, end - str, "%s",data->isp_info_str[n]);
    return (str - buf);
}
DEVICE_ATTR(isp_info, 0444, isp_info_show, NULL);

static ssize_t isp_fps_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += scnprintf(str, end - str, "%d\n",1000000000/isp_int.fps.frame_interval);
    return (str - buf);
}
DEVICE_ATTR( isp_fps, 0444, isp_fps_show, NULL);

static ssize_t csi_ints_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    csi_dev_data *data = (csi_dev_data*) dev->platform_data;
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    //frame
    str += scnprintf(str, end - str, "%s			: %d\n","FS_INTS",data->fs_int_count);
    str += scnprintf(str, end - str, "%s			: %d\n","VC0_INTS",data->vc0_int_count);
    //csi error
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_DT",data->csi_err_int_count[ERR_INT_DT]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_PA_LENS",data->csi_err_int_count[ERR_INT_PA_LENS]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_PH_LENS",data->csi_err_int_count[ERR_INT_PH_LENS]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_ECC_ONEBIT",data->csi_err_int_count[ERR_INT_ECC_ONEBIT]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_FRAME_START",data->csi_err_int_count[ERR_INT_FRAME_START]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_FRAME_END",data->csi_err_int_count[ERR_INT_FRAME_END]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_ECC_TWOBIT",data->csi_err_int_count[ERR_INT_ECC_TWOBIT]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_CRC",data->csi_err_int_count[ERR_INT_CRC]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_PA_WC_EQ0",data->csi_err_int_count[ERR_INT_PA_WC_EQ0]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_RAW10_LENS",data->csi_err_int_count[ERR_INT_RAW10_LENS]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_CON_FE",data->csi_err_int_count[ERR_INT_CON_FE]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_CON_FS",data->csi_err_int_count[ERR_INT_CON_FS]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_LE",data->csi_err_int_count[ERR_INT_LE]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_LS",data->csi_err_int_count[ERR_INT_LS]);
    str += scnprintf(str, end - str, "%s			: %d\n","ERR_INT_OVERRUN",data->csi_err_int_count[ERR_INT_OVERRUN]);
    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

DEVICE_ATTR(csi_ints, 0444, csi_ints_show, NULL);

// CSI debug interrupts //
static ssize_t csi_dbg_mask_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    csi_dev_data *data = (csi_dev_data*) dev->platform_data;
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "csi dbg mask = 0x%x",HalCsi_ErrIntMaskGet(data->hal_handle));
    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t csi_dbg_mask_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    csi_dev_data *data = (csi_dev_data*) dev->platform_data;
    if(buf && count>0){
        long mask = 0;
        if( !kstrtol(buf,0,&mask) ){
            pr_debug("new mask=0x%X\n",(unsigned int)mask);
            HalCsi_ErrIntMaskSet(data->hal_handle,mask);
        }
    }
    return count;
}

DEVICE_ATTR(csi_dbg_mask, 0644, csi_dbg_mask_show, csi_dbg_mask_store);

// CSI //
static int csi_fe_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    csi_dev_data *data = (csi_dev_data*) attr->private;
    if (wait_event_interruptible(*data->p_wq_fe, data->frame_end == true))
    {
        spin_lock_irqsave(&data->lock,flags);
        data->frame_end = false;
        spin_unlock_irqrestore(&data->lock,flags);
        return - ERESTARTSYS;
    }

    spin_lock_irqsave(&data->lock,flags);
    data->frame_end = false;
    spin_unlock_irqrestore(&data->lock,flags);

    if (buf && size >= sizeof(data->count) ) {
        memcpy((void *)buf, (void *)&data->count, sizeof(data->count));
        return sizeof(data->count);
    }

    return 0;
}

static int csi_fe_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
  DBG_INFO();
  return size;
}

static int csi_fs_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    unsigned long flags;
    csi_dev_data *data = (csi_dev_data*) attr->private;
    if (wait_event_interruptible(*data->p_wq_fs, data->frame_start == true)){
        spin_lock_irqsave(&data->lock,flags);
        data->frame_start = false;
        spin_unlock_irqrestore(&data->lock,flags);
        return - ERESTARTSYS;
    }

    spin_lock_irqsave(&data->lock,flags);
    data->frame_start = false;
    spin_unlock_irqrestore(&data->lock,flags);

    if (buf && size >= sizeof(data->count) ) {
        memcpy((void *)buf, (void *)&data->count, sizeof(data->count));
        return sizeof(data->count);
    }

    return 0;
}

static int csi_fs_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

// CSI //
static ISP_BIN_ATTR(csi_fe, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(csi_fs, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);

int isp_create_bin_file(struct device *dev)
{
    int error = -EINVAL;
    if (dev != NULL) {
        kobj_isp = &dev->kobj;
        error = sysfs_create_bin_file(kobj_isp, &isp_attr);
        error = sysfs_create_bin_file(kobj_isp, &vs_sr_attr);
        error = sysfs_create_bin_file(kobj_isp, &ve_sr_attr);
        error = sysfs_create_bin_file(kobj_isp, &ae_attr);
        error = sysfs_create_bin_file(kobj_isp, &ae_win0_attr);
        error = sysfs_create_bin_file(kobj_isp, &ae_win1_attr);
        error = sysfs_create_bin_file(kobj_isp, &ae_row_int_attr);
        error = sysfs_create_bin_file(kobj_isp, &awb_attr);
        error = sysfs_create_bin_file(kobj_isp, &af_attr);
        error = sysfs_create_bin_file(kobj_isp, &busy_attr);
        error = sysfs_create_bin_file(kobj_isp, &idle_attr);
        error = sysfs_create_bin_file(kobj_isp, &isp_fifofull_attr);
        error = sysfs_create_bin_file(kobj_isp, &wdma_fifofull_attr);
        error = sysfs_create_bin_file(kobj_isp, &wdma_done_attr);
        error = sysfs_create_bin_file(kobj_isp, &rdma_done_attr);
        error = sysfs_create_bin_file(kobj_isp, &eis_attr);
        error = sysfs_create_bin_file(kobj_isp, &sw_int_in_attr);
        error = sysfs_create_bin_file(kobj_isp, &sw_int_out_attr);
        error = sysfs_create_bin_file(kobj_isp, &scl_fe_attr);
        error = device_create_file(dev,&dev_attr_isp_ints);
        error = device_create_file(dev,&dev_attr_isp_info);
        error = device_create_file(dev,&dev_attr_isp_fps);
        error = sysfs_create_bin_file(kobj_isp, &mload_ALSC_R_TBL_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_ALSC_G_TBL_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_ALSC_B_TBL_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_LN_GMA12TO10_R_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_LN_GMA12TO10_G_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_LN_GMA12TO10_B_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_DP_TBL_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_LN_GMA10TO10_R_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_LN_GMA10TO10_G_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_LN_GMA10TO10_B_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_FPN_OFFSET_attr);
        error = sysfs_create_bin_file(kobj_isp, &mload_ALL_attr);
    }
    return error;
}

void isp_remove_bin_file(void)
{
    sysfs_remove_bin_file(kobj_isp, &isp_attr);
    sysfs_remove_bin_file(kobj_isp, &vs_sr_attr);
    sysfs_remove_bin_file(kobj_isp, &ve_sr_attr);
    sysfs_remove_bin_file(kobj_isp, &ae_attr);
    sysfs_remove_bin_file(kobj_isp, &ae_win0_attr);
    sysfs_remove_bin_file(kobj_isp, &ae_win1_attr);
    sysfs_remove_bin_file(kobj_isp, &ae_row_int_attr);
    sysfs_remove_bin_file(kobj_isp, &awb_attr);
    sysfs_remove_bin_file(kobj_isp, &af_attr);
    sysfs_remove_bin_file(kobj_isp, &busy_attr);
    sysfs_remove_bin_file(kobj_isp, &idle_attr);
    sysfs_remove_bin_file(kobj_isp, &isp_fifofull_attr);
    sysfs_remove_bin_file(kobj_isp, &wdma_fifofull_attr);
    sysfs_remove_bin_file(kobj_isp, &wdma_done_attr);
    sysfs_remove_bin_file(kobj_isp, &rdma_done_attr);
    sysfs_remove_bin_file(kobj_isp, &eis_attr);
    sysfs_remove_bin_file(kobj_isp, &sw_int_in_attr);
    sysfs_remove_bin_file(kobj_isp, &sw_int_out_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_ALSC_R_TBL_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_ALSC_G_TBL_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_ALSC_B_TBL_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_LN_GMA12TO10_R_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_LN_GMA12TO10_G_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_LN_GMA12TO10_B_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_DP_TBL_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_LN_GMA10TO10_R_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_LN_GMA10TO10_G_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_LN_GMA10TO10_B_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_FPN_OFFSET_attr);
    sysfs_remove_bin_file(kobj_isp, &mload_ALL_attr);
}


int csi_create_bin_file(struct device *dev)
{
    int error = -EINVAL;
    if(!dev)
        return error;

    if(dev != NULL)
    {
        csi_dev_data *data = dev_get_platdata(dev);
        //frame end
        csi_fe_attr.private = (void*)data;
        error = sysfs_create_bin_file(&dev->kobj, &csi_fe_attr);

        //frame start
        csi_fs_attr.private = (void*)data;
        error = sysfs_create_bin_file(&dev->kobj, &csi_fs_attr);

        //CSI error report interrupt
        error = device_create_file(dev, &dev_attr_csi_dbg_mask);
        //CSI interrupt counter
        error = device_create_file(dev, &dev_attr_csi_ints);
    }
    return error;
}

void csi_remove_bin_file(struct device *dev)
{
  sysfs_remove_bin_file(&dev->kobj, &csi_fs_attr);
  sysfs_remove_bin_file(&dev->kobj, &csi_fe_attr);
}

static int csi_probe(struct platform_device* pdev)
{
    int err, ret;
    int irq, u4IO_PHY_BASE;
    unsigned int u4Bank;
    csi_dev_data *data;
    data = kzalloc(sizeof(csi_dev_data),GFP_KERNEL);
    if(!data)
        return -ENOENT;

    ret = of_property_read_u32(pdev->dev.of_node, "io_phy_addr", &u4IO_PHY_BASE);  //get custom property, OPR base address
    if(ret != 0)
        pr_err("[ISP] read node error!\n");
    ret = of_property_read_u32(pdev->dev.of_node, "banks", &u4Bank);         //get custom property,  CSI OPR bank offset
    if(ret != 0)
        pr_err("[ISP] read node error!\n");

    data->reg_base = (void*)ioremap(BANK_TO_ADDR32(u4Bank)+u4IO_PHY_BASE, 0x200);
    pr_debug("[%s] IO remap phys:0x%.8x virt: 0x%.8x\n", __FUNCTION__,
                    BANK_TO_ADDR32(u4Bank)+u4IO_PHY_BASE,
                    (u32) data->reg_base );         //(u32) isp_mem.pCSIRegs );

    data->sysfs_dev = device_create(msys_get_sysfs_class(), NULL, MKDEV(MAJOR_ISP_NUM, MINOR_CSI_NUM), NULL, "csi0");
    csi_create_bin_file(data->sysfs_dev);
    err = sysfs_create_link(&pdev->dev.parent->kobj,&data->sysfs_dev->kobj, "csi0"); //create symlink for older firmware version
    data->sysfs_dev->platform_data = pdev->dev.platform_data = (void*)data;

    //TODO, init status
    data->lock = __SPIN_LOCK_UNLOCKED("csi_lock");
    data->count = 0;
    data->frame_end = false;

    data->p_wq_fe = &isp_wq_VEND;  //notify isp frame end
    data->p_wq_fs = &isp_wq_VSTART;    //notify ISP frame start

    //TODO: CSI clock enable
    data->hal_handle = HalCsi_Open(data->reg_base);
    if(!data->hal_handle)
        return -ENOENT;
    HalCsi_RegInit(data->hal_handle);

    pdev->dev.platform_data = (void*)data;

    irq = irq_of_parse_and_map(pdev->dev.of_node, 0); //get device IRQ#0
    pr_debug("request IRQ#%d\n",irq);

    if (request_irq(irq, csi_ISR, 0, "csi interrupt", (void*)data) == 0)
        pr_debug("csi interrupt registered.\n");
    else
        pr_err("failed to request IRQ#%d\n",irq);

   return 0;
}

static int csi_remove(struct platform_device* pdev)
{
    csi_dev_data *data = (csi_dev_data *)pdev->dev.platform_data;
    HalCsi_Close(data->hal_handle);
    iounmap(data->reg_base);
    kfree(data);
    csi_remove_bin_file(&pdev->dev);
    device_destroy(msys_get_sysfs_class(),MKDEV(MAJOR_ISP_NUM, MINOR_CSI_NUM));
    return 0;
}

static int csi_suspend(struct platform_device *pdev, pm_message_t state)
{
    csi_dev_data *data = (csi_dev_data *)pdev->dev.platform_data;
    HalCsi_RegUninit(data->hal_handle);
    return 0;
}

static int csi_resume(struct platform_device *pdev)
{
    csi_dev_data *data = (csi_dev_data *)pdev->dev.platform_data;
    HalCsi_RegInit(data->hal_handle);
    return 0;
}

#define INC_CSI_ERR_COUNT(name) (data->csi_err_int_count[name]++)
irqreturn_t csi_ISR(int num, void *priv)
{
    csi_dev_data *data = (csi_dev_data*)priv;
    void* hal = data->hal_handle;

    u32 rpt_clear = 0;
    u32 err_clear = 0;
    u32 rpt_status = HalCsi_GetRptIntStatus(hal);
    u32 err_status = HalCsi_GetErrorIntStatus(hal);
    HalCsi_MaskRptInt(hal,rpt_status);
    HalCsi_MaskErrorInt(hal,err_status);
#if 0
    if(ISP_CHECKBITS(rpt_status,RPT_INT_FRAME))  //frame start interrupt
    {
        isp_int.vsync_start= true; //notify ISP fs
        isp_int.frame_cnt++;
        ISP_SETBIT(rpt_clear,RPT_INT_FRAME);
        data->fs_int_count++;
        wake_up_interruptible_all(data->p_wq_fs);
        DBG_INFO_1( "[%s] FRAME INT \n", __FUNCTION__);
    }
#endif
    if(ISP_CHECKBITS(rpt_status,RPT_INT_VC0)){  //frame end interrupt
        print_timestamp("K_VE");//TEST CODE
        isp_int.vsync_end = true; //notify ISP fe
        isp_int.share_data->frame_state.bActive = false;
        isp_apply_iq_at_vend();
        if(isp_int.fifo_mask.dirty)
        {
            IspInputEnable(~isp_int.fifo_mask.enable);
            isp_int.fifo_mask.dirty = 0;
        }
        ISP_SETBIT(rpt_clear,RPT_INT_VC0);
        data->vc0_int_count++;
        wake_up_interruptible_all(data->p_wq_fe);
        // Using menuload to write IQ table to SRAMs
        //isp_write_iq_tbl();
        data->count++;

        //record csi frame end interrupt time to PAD_FAILLING
        getnstimeofday(&isp_int.isp_int_time[INT_PAD_VSYNC_FALLING]);

        DBG_INFO( "[%s] VC0 INT %lld \n", __FUNCTION__,data->count);
    }

    if(err_status){
        if(ISP_CHECKBITS(err_status,ERR_INT_DT)){  //frame end interrupt
            ISP_SETBIT(err_clear,ERR_INT_DT);
            INC_CSI_ERR_COUNT(ERR_INT_DT);
            DBG_INFO_1( "[%s] ERR_INT_DT %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_PA_LENS)){  //frame end interrupt
            ISP_SETBIT(err_clear,ERR_INT_PA_LENS);
            INC_CSI_ERR_COUNT(ERR_INT_PA_LENS);
            DBG_INFO_1( "[%s] ERR_INT_PA_LENS %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_PH_LENS))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_PH_LENS);
            INC_CSI_ERR_COUNT(ERR_INT_PH_LENS);
            DBG_INFO_1( "[%s] ERR_INT_PH_LENS %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_ECC_ONEBIT))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_ECC_ONEBIT);
            INC_CSI_ERR_COUNT(ERR_INT_ECC_ONEBIT);
            DBG_INFO_1( "[%s] ERR_INT_ECC_ONEBIT %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_FRAME_START))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_FRAME_START);
            INC_CSI_ERR_COUNT(ERR_INT_FRAME_START);
            DBG_INFO_1( "[%s] ERR_INT_FRAME_START %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_FRAME_END))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_FRAME_END);
            INC_CSI_ERR_COUNT(ERR_INT_FRAME_END);
            DBG_INFO_1( "[%s] ERR_INT_FRAME_START %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_ECC_TWOBIT))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_ECC_TWOBIT);
            INC_CSI_ERR_COUNT(ERR_INT_ECC_TWOBIT);
            DBG_INFO_1( "[%s] ERR_INT_FRAME_START %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_CRC))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_CRC);
            INC_CSI_ERR_COUNT(ERR_INT_CRC);
            DBG_INFO_1( "[%s] ERR_INT_CRC %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_PA_WC_EQ0))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_PA_WC_EQ0);
            INC_CSI_ERR_COUNT(ERR_INT_PA_WC_EQ0);
            DBG_INFO_1( "[%s] ERR_INT_PA_WC_EQ0 %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_RAW10_LENS))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_RAW10_LENS);
            INC_CSI_ERR_COUNT(ERR_INT_RAW10_LENS);
            DBG_INFO_1( "[%s] ERR_INT_RAW10_LENS %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_CON_FE))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_CON_FE);
            INC_CSI_ERR_COUNT(ERR_INT_CON_FE);
            DBG_INFO_1( "[%s] ERR_INT_CON_FE %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_CON_FS))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_CON_FS);
            INC_CSI_ERR_COUNT(ERR_INT_CON_FS);
            DBG_INFO_1( "[%s] ERR_INT_CON_FS %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_LE))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_LE);
            INC_CSI_ERR_COUNT(ERR_INT_LE);
            DBG_INFO_1( "[%s] ERR_INT_LE %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_LS))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_LS);
            INC_CSI_ERR_COUNT(ERR_INT_LS);
            DBG_INFO_1( "[%s] ERR_INT_LS %lld \n", __FUNCTION__,data->count);
        }
        if(ISP_CHECKBITS(err_status,ERR_INT_OVERRUN))  //frame end interrupt
        {
            ISP_SETBIT(err_clear,ERR_INT_OVERRUN);
            INC_CSI_ERR_COUNT(ERR_INT_OVERRUN);
            DBG_INFO_1( "[%s] ERR_INT_OVERRUN %lld \n", __FUNCTION__,data->count);
        }
    }

    //clear ISR
    HalCsi_ClearRptInt(hal,rpt_clear); //clear and unmask interrupt

    //clear Error ISR
    HalCsi_ClearErrorInt(hal,err_clear); //clear and unmask interrupt

    return IRQ_HANDLED;
}

static int __init mstar_isp_init(void)
{
    int ret;
    memset((void*)&isp_int, 0, sizeof(isp_int));
    isp_int.sysfs_int = false;

    ret = platform_driver_register(&isp_driver);
    if(!ret)
    {
        printk(KERN_INFO "[ISP] register driver success");
    }
    else
    {
        pr_err("[ISP] register driver fail");
        platform_driver_unregister(&isp_driver);
        return ret;
    }

    ret = platform_driver_register(&csi_driver);
    if(!ret)
    {
        printk(KERN_INFO "[CSI] register driver success");
    }
    else
    {
        pr_err("[CSI] register driver fail");
        platform_driver_unregister(&csi_driver);
        return ret;
    }
    return 0;
}

static void __exit mstar_isp_exit(void)
{
    platform_driver_unregister(&isp_driver);
    isp_remove_bin_file();
    pr_debug("[ISP] exit");
}

//isp resolution parameter
static int __init isp_res_early_init(char *opt)
{
    long res = 1;
    if(!kstrtol(opt,10,&res))
    {
        g_isp_max_res = res;
    }
    else
    {
        printk("Invaild param : isp_res=%s\n",opt);
    }
    return 0;
}
early_param("isp_res", isp_res_early_init);

//isp flags
static int __init isp_flag_early_init(char *opt)
{
    long flag = ISP_FB_DNR | ISP_FB_ROT;
    if(!kstrtol(opt,16,&flag))
    {
        g_isp_flag = flag;
    }
    return 0;
}
early_param("isp_flag", isp_flag_early_init);

module_init(mstar_isp_init);
module_exit(mstar_isp_exit);

#endif//__DRV_ISP_C__

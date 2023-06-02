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
#include <linux/spinlock.h>
#include <asm/atomic.h>

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/mm.h>

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
#include "../../gpio/mdrv_gpio.h"

/*--------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////

/*--------------------------------------------------------------------------*/
/* CONSTANT DEFINITION                                                      */
/*--------------------------------------------------------------------------*/

//#define TIMMING_DBG_MSG

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

#define AE_RING_BUFF_NUM            3
#define AWB_RING_BUFF_NUM           3
#define AE_AWB_WIN_SIZE             (128*90)
#define AE_HIST_R_SIZE              80
#define AE_HIST_G_SIZE              80
#define AE_HIST_B_SIZE              80
#define AE_HIST_WIN0_SIZE           256
#define AE_HIST_WIN1_SIZE           256
//R,G,B,Y 4bytes statistics
#define AE_STATIS_SIZE  (AE_AWB_WIN_SIZE*4)
#define AE_TOTAL_STATIS_SIZE    (AE_HIST_R_SIZE+AE_HIST_G_SIZE+AE_HIST_B_SIZE+AE_HIST_WIN0_SIZE+AE_HIST_WIN1_SIZE+AE_STATIS_SIZE)

//Each row block's start address(Red) MUST be 16-byte align
#define AWB_STATIS_SIZE (AE_AWB_WIN_SIZE*3)
#define MOT_STATIS_SIZE (16*16*8)
//#define DNR_FB_SIZE        (((1920+191)/192) * ((1080+15)>>4) * 256 * 16)
/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef struct {
    unsigned int u4PhyAddr;
    unsigned int u4VirAddr;
    unsigned long u4Stat[9*2];
} EIS_STS_t;

typedef struct EIS_VDOS_t {
    int width;
    int height;
    int pitch;
    int size;
    //int phyAddr;
    //unsigned char data[5*1024*1024];
} eEIS_VDOS_t;

typedef enum
{
    WDMA_SRC_BAYER_RAW = 0,
    WDMA_SRC_ISP_OUTPUT_0 = 1,
    WDMA_SRC_ISP_OUTPUT_1 = 2,
    WDMA_SRC_ISP_VIDEO_STABILIZATION=3,
}WDMA_SRC;

typedef enum
{
    WDMA_REQ_WAIT = 0,
    WDMA_REQ_PROC = 1,
    WDMA_REQ_DONE = 2,
}WDMA_REQ_STATUS;

typedef struct
{
    u32 uSourceSel;
    u32 uBufferPhysAddr;
    u32 uReqStatus; //WDMA_SRC
}__attribute__((packed, aligned(1))) WDMA_CFG;

#define ISP_STATUS_FRAME_DROP_ENABLE 0x01
typedef struct
{
  u32 bActive;    //Active data state
  u32 u4FrameCnt; //frame count
  u32 bIspInputEnable;
  u32 u4OBCInValid;
  u32 u4OBC_a;
  u32 u4OBC_b;
  u32 uIspStatusSet;           // isp status set from user space
  u32 uIspStatusReport;     //isp satus get from kernel space
  WDMA_CFG wdma_cfg;
}__attribute__((packed, aligned(1))) FRAME_STATE;

#define I2C_RW_DONE 0
#define I2C_RW_WAIT 1
#define I2C_RW_ERROR 2
typedef struct
{
  app_i2c_cfg cfg;
  u32 status;
  u32 num_i2c_data; //number of available i2c data in array
  I2C_ARRAY i2c_data[16];
}__attribute__((packed, aligned(1))) ISP_K_I2C_RW;

typedef struct
{
  FRAME_STATE frame_state;
  ISP_K_I2C_RW i2c_rw;
}__attribute__((packed, aligned(1))) ISP_SHARE_DATA;

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

typedef struct {
    u32             frame_cnt;
    u8              sysfs_int;
    u8              vsync_start;
    u8              vsync_end;
    u8              ae;
    u8              ae_win0;
    u8              ae_row_int;
    u8              awb;
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

    //eEIS_VDOS_t  EIS_VDOS;
    u32  isp_int_count[ISP_INT_MAX];
    u32  isp_int2_count[ISP_INT2_MAX];
    u32  isp_int3_count[ISP_INT3_MAX];

    struct timespec isp_int_time[ISP_INT_MAX];
    struct timespec isp_int2_time[ISP_INT2_MAX];
    struct timespec isp_int3_time[ISP_INT3_MAX];

    u8 hw_frame_cnt;
    u32 int_cnt;

    FPS_INFO fps;

    isp_ccm_coff rgb_ccm;
    isp_ccm_coff yuv_ccm;
} ISP_INT_DATA;

typedef struct{
	u32 int_cnt;
	u32 frame_interval;
}ISP_IOCTL_INFO;

typedef struct {
    char data[AE_AWB_WIN_SIZE];
    struct list_head list;
    int status;
}__attribute__((packed, aligned(1))) AWB_STATIS_DATA;

typedef struct
{
  u32 data_offset; //offset to AE statistic data
  u32 fcount; //frame count
}__attribute__((packed, aligned(1))) ae_isr_data;

typedef struct
{
  u32 data_offset; //offset to AWB statistic data
  u32 fcount; //frame count
}__attribute__((packed, aligned(1))) awb_isr_data;


typedef struct {
    void          *pISPRegs[8];
    void          *pCSIRegs;
    MSYS_DMEM_INFO Base_Meminfo;
    MSYS_DMEM_INFO DNR_FB_Meminfo[2];
    MSYS_DMEM_INFO ShareData_Meminfo;
    //unsigned int AE_BuffNum;
    //unsigned int AWB_BuffNum;
    unsigned int AE_Size;
    unsigned int AWB_Size;
    unsigned int MOT_Size;
    //volatile int AE_WriteIdx;
    //volatile int AE_ReadIdx;
    //volatile int AWB_WriteIdx;
    //volatile int AWB_ReadIdx;
    volatile u8 IsWriteIQtbl;

    //unsigned long AE_Offset[AE_RING_BUFF_NUM];
   // unsigned long AWB_Offset[AWB_RING_BUFF_NUM];
    unsigned long AE_Offset;
    unsigned long AWB_Offset;
    unsigned long IQ_tblOffset[IQ_MEM_NUM];
    unsigned int IQ_tblSize[IQ_MEM_NUM];
    unsigned int MOT_Offset;
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

// ISP watchdog timer
#define MAX_WATCHDOG_TIME (5)
#define WATCHDOG_KICK (99)
#define WATCHDOG_STOP (88)
#define WATCHDOG_UPDATE (77)
static struct timer_list isp_watchdog_timer;
atomic_t isp_watchdog_counter = ATOMIC_INIT(0);
static unsigned int isp_watchdog_update_counter = 0;
DEFINE_SPINLOCK(isp_watchdog_lock);

void isp_watchdog_callback( unsigned long data )
{
	atomic_dec(&isp_watchdog_counter);

	// disable ISP HW
	if(atomic_read(&isp_watchdog_counter) == 0) {
		pr_err("watchdog timeout and disable ISP HW\n");
		IspDisable();
	}

	// kick timer until counter is 0
	if(atomic_read(&isp_watchdog_counter) > 0)
		mod_timer( &isp_watchdog_timer, jiffies + msecs_to_jiffies(1000) );
}

void reset_isp_int(ISP_INT_DATA *p_isp_int) //reset isp_int struct
{
    //p_isp_int->frame_cnt = 0;
    p_isp_int->sysfs_int = 0;
    p_isp_int->vsync_start = 0;
    p_isp_int->vsync_end = 0;
    p_isp_int->ae = 0;
    p_isp_int->ae_win0 = 0;
    p_isp_int->awb = 0;
    p_isp_int->af = 0;
    p_isp_int->isp_busy = 0;
    p_isp_int->isp_idle = 0;
    p_isp_int->isp_fifofull = 0;
    p_isp_int->wdma_fifofull = 0;
    p_isp_int->rdma_done = 0;
    p_isp_int->wdma_done = 0;
    p_isp_int->eis = 0;

    memset(p_isp_int->AE_Lum,0,sizeof(unsigned long)*64);
    memset(p_isp_int->AE_Hist,0,sizeof(unsigned long)*64);
    memset(p_isp_int->AF_Stat,0,sizeof(unsigned long)*60);
    memset(p_isp_int->AWB_Stat,0,sizeof(unsigned long)*80);
    //memset(&p_isp_int->EIS_VDOS,0,sizeof(eEIS_VDOS_t));

    p_isp_int->share_data->frame_state.bIspInputEnable = true;
}

void isp_enable_input(u32 enable)
{
    if(enable)
    {
        isp_int.share_data->frame_state.bIspInputEnable = true;
        IspInputEnable(enable);
    }
    else
    {//do not disable input immediately , to disable isp input when frame blanking (vsync end interrupt)
        isp_int.share_data->frame_state.bIspInputEnable = false;
    }
}
#if 1
void isp_async(u32 enable)
{
    IspAsyncEnable(enable);
}

void fps_init(FPS_INFO *info)
{
  memset(info,sizeof(FPS_INFO),0);
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

#ifdef TIMMING_DBG_MSG
void print_timestamp(char* msg)
{
    struct timespec ts;
    getnstimeofday(&ts);
    printk("%d:%lu:%s\r\n", isp_int.frame_cnt, (ts.tv_sec * 1000000)+(ts.tv_nsec/1000), msg);
}
#else
#define print_timestamp(a)
#endif

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
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_scl_fe);

//CSI//
static DECLARE_WAIT_QUEUE_HEAD(csi_wq_fe);
static DECLARE_WAIT_QUEUE_HEAD(csi_wq_fs);

static struct kobject *kobj_isp = NULL;


/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTION PROTOTYPE DECLARATION AND DEFINITION                      */
/*--------------------------------------------------------------------------*/

extern int msys_request_dmem(MSYS_DMEM_INFO *mem_info);
#if 0
unsigned short pPGscript[][3]={ {1, 0x0018, 0x03e4 },
                                {0, 0x0118, 0x3ba6 },
                                {0, 0x011C, 0x0e36 },
                                {1, 0x0000, 0x003b },
                                {1, 0x0014, 0x0438 },
                                {1, 0x0020, 0x04b1 },
                                {1, 0x0024, 0x00e5 },
                                {1, 0x0028, 0x08fc },
                                {1, 0x002C, 0x0078 },
                                {1, 0x0030, 0x0671 },
                                {1, 0x0034, 0xffe7 },
                                {1, 0x0038, 0x08ef },
                                {1, 0x003C, 0xffb2 },
                                {1, 0x0004, 0x03f2 },
                                {1, 0x0008, 0x02bf },
                                {1, 0x000C, 0x0001 },
                                {1, 0x0010, 0x0000 },
                                {1, 0x0040, 0x4f7c },
                                {1, 0x0044, 0x8b60 },
                                {1, 0x0048, 0x5b85 },
                                {1, 0x004C, 0xc117 },
                                {1, 0x0050, 0x8c5c },
                                {1, 0x0054, 0x2967 },
                                {1, 0x0058, 0xfbbf },
                                {1, 0x005C, 0x6bbe },
                                {1, 0x0060, 0xbd18 },
                                {1, 0x0064, 0xbe21 },
                                {1, 0x0068, 0x491f },
                                {1, 0x006C, 0x6905 },
                                {1, 0x0070, 0x4cc9 },
                                {1, 0x0074, 0xa03d },
                                {1, 0x0078, 0x4f24 },
                                {1, 0x007C, 0x17ff },
                                {1, 0x0080, 0x859c },
                                {1, 0x0084, 0xb985 },
                                {1, 0x0088, 0x4fd5 },
                                {1, 0x008C, 0x5130 },
                                {1, 0x0090, 0x49f7 },
                                {1, 0x0094, 0x4f94 },
                                {1, 0x0098, 0x3f23 },
                                {1, 0x009C, 0xf8de },
                                {1, 0x00A0, 0xa66a },
                                {1, 0x00A4, 0xb0cc },
                                {1, 0x00A8, 0x3245 },
                                {1, 0x00AC, 0xf117 },
                                {1, 0x00B0, 0x144b },
                                {1, 0x00B4, 0x3e8b },
                                {1, 0x00B8, 0xb33b },
                                {1, 0x00BC, 0xd409 },
                                {1, 0x00C0, 0x0154 },
                                {1, 0x00C4, 0x7df2 },
                                {1, 0x00C8, 0x418f },
                                {1, 0x00CC, 0x0f85 },
                                {1, 0x00D0, 0x5070 },
                                {1, 0x00D4, 0x6b92 },
                                {1, 0x00D8, 0x46c7 },
                                {1, 0x00DC, 0xae71 },
                                {1, 0x00E0, 0xe421 },
                                {1, 0x00E4, 0x8a81 },
                                {1, 0x00E8, 0xc88e },
                                {1, 0x00EC, 0xcc36 },
                                {1, 0x00F0, 0x9e25 },
                                {1, 0x00F4, 0xd7b0 },
                                {1, 0x00F8, 0x7f37 },
                                {1, 0x00FC, 0x2263 },
                                {1, 0x01A0, 0x0080 },
                                {1, 0x01E4, 0x0800 },
                                {1, 0x01E8, 0x0901 },
                                {1, 0x01EC, 0x68f5 },
                                {1, 0x01F0, 0x0036 },
                                {1, 0x0120, 0x0000 },
                                {1, 0x0124, 0x0004 },
                                {1, 0x0128, 0x0017 },
                                {1, 0x012C, 0x001d },
                                {1, 0x0130, 0x0735 },
                                {1, 0x0134, 0x042e },
                                {1, 0x0100, 0x0102 },
                                {1, 0x0104, 0x0064 },
                                {1, 0x01C0, 0xac00 },
                                {1, 0x01C4, 0x0001 },
                                {1, 0x01C8, 0x00fa },
                                {1, 0x01CC, 0xe428 },
                                {1, 0x01D0, 0x5a30 },
                                {1, 0x01D4, 0x67ca },
                                {1, 0x01D8, 0x0000 },
                                {1, 0x01DC, 0x002d },
                                {1, 0x01E0, 0x0093 },
                                {3, 0x0000, 0x200d },//0x200d },//Statistics disable
                                {3, 0x0004, 0x0b0e },
                                {3, 0x0008, 0x4154 },
                                {3, 0x000C, 0x0555 },
                                {3, 0x0010, 0x0618 },
                                {3, 0x0014, 0x02d8 },
                                {3, 0x0018, 0x8070 },
                                {3, 0x001C, 0x000c },
                                {3, 0x0020, 0x7800 },
                                {3, 0x0024, 0x000c },
                                {3, 0x0028, 0x0f06 },
                                {3, 0x002C, 0x2302 },
                                {3, 0x0030, 0x3c0d },
                                {3, 0x0034, 0x0f04 },
                                {3, 0x0080, 0x1002 },
                                {3, 0x0084, 0x0000 },
                                {3, 0x0088, 0x0000 },
                                {3, 0x008C, 0x443c },
                                {3, 0x0090, 0x003d },
                                {3, 0x00C0, 0x3152 },
                                {3, 0x00C4, 0x3337 },
                                {3, 0x00C8, 0xba78 },
                                {3, 0x00CC, 0x040f },
                                {3, 0x00D0, 0x6bbc },
                                {3, 0x00D4, 0x59cf },
                                {3, 0x00D8, 0x72a5 },
                                {3, 0x00DC, 0x713a },
                                {3, 0x00E0, 0xfc00 },
                                {3, 0x00E4, 0x0003 },
                                {3, 0x00E8, 0x1c00 },
                                {3, 0x00EC, 0x0006 },
                                {3, 0x00F0, 0x1887 },
                                {3, 0x00F4, 0x7b00 },
                                {3, 0x00F8, 0x8bdf },
                                {3, 0x00FC, 0x000c },
                                {3, 0x0108, 0x0004 },
                                {3, 0x010C, 0x001e },
                                {5, 0x0000, 0x0001 },
                                {5, 0x0008, 0x002f },
                                {5, 0x000C, 0x0024 },
                                {5, 0x0010, 0x001e },
                                {5, 0x0014, 0x0004 },
                                {5, 0x0018, 0x0043 },
                                {5, 0x001C, 0x0068 },
                                {5, 0x0020, 0x002b },
                                {5, 0x0024, 0x002f },
                                {5, 0x0028, 0x01d4 },
                                {5, 0x002C, 0x002b },
                                {5, 0x0030, 0x0007 },
                                {5, 0x0034, 0x0007 },
                                {5, 0x0038, 0x0074 },
                                {5, 0x003C, 0x00a9 },
                                {5, 0x0040, 0x001e },
                                {5, 0x0044, 0x0064 },
                                {5, 0x0048, 0x0200 },
                                {5, 0x004C, 0x0200 },
                                {5, 0x0050, 0x00d5 },
                                {5, 0x0054, 0x011e },
                                {0, 0x00C8, 0x0000 },
                                {0, 0x00CC, 0x0400 },
                                {0, 0x00D0, 0x0000 },
                                {0, 0x00D4, 0x0000 },
                                {0, 0x00D8, 0x0000 },
                                {0, 0x00DC, 0x0400 },
                                {0, 0x00E0, 0x0000 },
                                {0, 0x00E4, 0x0000 },
                                {0, 0x00E8, 0x0000 },
                                {0, 0x00EC, 0x0400 },
                                {0, 0x00F0, 0x0191 },
                                {0, 0x00F4, 0x01c2 },
                                {0, 0x00F8, 0x1e87 },
                                {0, 0x00FC, 0x1fb7 },
                                {0, 0x0100, 0x0107 },
                                {0, 0x0104, 0x0204 },
                                {0, 0x0108, 0x0064 },
                                {0, 0x010C, 0x1f68 },
                                {0, 0x0110, 0x1ed6 },
                                {0, 0x0114, 0x01c2 },
                                {0, 0x00C0, 0x0004 },
                                {0, 0x0180, 0x0001 },
                                {0, 0x01C4, 0x0d07 },
                                {0, 0x01C8, 0x2701 },
                                {0, 0x01CC, 0x040d },
                                {0, 0x01C0, 0x5601 },
                                {0, 0x01A8, 0x0010 },
                                {0, 0x01AC, 0x0003 },
                                {0, 0x01B0, 0x0003 },
                                {0, 0x01B4, 0x0002 },
                                {0, 0x01B8, 0x000c },
                                {0, 0x01BC, 0x0007 },
                                {7, 0x0004, 0x04ff },
                                {7, 0x0008, 0x031f },
                                {7, 0x000C, 0x0106 },
                                {7, 0x0010, 0x2864 },
                                {7, 0x0014, 0x001f },
                                {7, 0x0018, 0x000f },
                                {7, 0x001C, 0x0000 },
                                {7, 0x0020, 0x0000 },
                                {7, 0x0000, 0x0003 },
                                {0, 0x0010, 0x0210 },
                                {0, 0x0040, 0x1700 },
                                {0, 0x0044, 0x0000 },
                                {0, 0x0048, 0x0000 },
                                {0, 0x004C, 0x0000 },
                                {0, 0x0050, 0x04ff },
                                {0, 0x0054, 0x031f },
                                {0, 0x0068, 0x0001 },
                                {0, 0x0198, 0x0088 },
                                {0, 0x0004, 0x8103 },
                                {0, 0x0008, 0xc000 },
                                {0, 0x0000, 0x0001 },
                                {0, 0x0058, 0x0000 },
                                {0, 0x005C, 0x0000 },
                                {0, 0x0060, 0x04ff },
                                {0, 0x0064, 0x031f },
                                {6, 0x0080, 0x0291 },//0x0291 },//WDMA disable
                                {6, 0x0084, 0x00a0 },
                                {6, 0x00A0, 0x7c00 },
                                {6, 0x00A4, 0x0008 },
                                {6, 0x00B0, 0x08f0 },
                                {6, 0x00C0, 0x0100 },
                                {6, 0x0000, 0x0300 },
                                {6, 0x0004, 0x0078 },
                                {6, 0x0020, 0x0000 },
                                {6, 0x0024, 0x0000 },
                                {6, 0x0030, 0x01f4 },
                                {6, 0x0034, 0x003f },
                                {6, 0x0040, 0x0000 }};


static int isp_testscript(void)
{
    unsigned int script_len, i, value;
    unsigned int *address;
    //MSYS_DMEM_INFO raw_meminfo;
    //unsigned long ae_phyaddr, awb_phyaddr, u4PhyAddr;
    //unsigned long long ae_viraddr, awb_viraddr;

    printk(KERN_INFO "[ISP]: test script\n");

    //raw_meminfo.length = 1280*800*2;
    //msys_request_dmem(&raw_meminfo);

    //ae_phyaddr = (unsigned long)isp_mem.Base_Meminfo.phys + isp_mem.AE_Offset[0];
    //awb_phyaddr = (unsigned long)isp_mem.Base_Meminfo.phys + isp_mem.AWB_Offset[0];
    //ae_viraddr = isp_mem.Base_Meminfo.kvirt+ isp_mem.AE_Offset[0];
    //awb_viraddr = isp_mem.Base_Meminfo.kvirt + isp_mem.AWB_Offset[0];

    //u4PhyAddr = raw_meminfo.phys;
    //u4VirAddr = raw_meminfo.kvirt;

    //printk(KERN_INFO "[ISP]: raw address phyiscal = 0x%08x, virtual = 0x%08x\n", u4PhyAddr, u4VirAddr);
    //printk(KERN_INFO "[ISP]: ae address phyiscal  = 0x%08x, virtual64 = 0x%llx\n", ae_phyaddr, ae_viraddr);
    //printk(KERN_INFO "[ISP]: awb address phyiscal  = 0x%08x, virtual64 = 0x%llx\n", awb_phyaddr, awb_viraddr);

    script_len = sizeof(pPGscript)/(sizeof(unsigned short)*3);

    printk(KERN_INFO "[ISP]: script length = %d\n", script_len);
    for (i = 0; i < script_len; i++) {

        address = (unsigned int*)(isp_mem.pISPRegs[pPGscript[i][0]] + pPGscript[i][1]);

        if(pPGscript[i][0] == 3 && pPGscript[i][1] == 0x0018) {

            //u4PhyAddr = ae_phyaddr;
            value = (0x281F4000>>4) & 0xFFFF;//(0x281F4000>>4) & 0xFFFF;//(u4PhyAddr>>4) & 0xFFFF;

            printk(KERN_INFO "[ISP]: wdma base_L = 0x%08x\n", value);

        }
        else if(pPGscript[i][0] == 3 && pPGscript[i][1] == 0x001C) {
            //u4PhyAddr = ae_phyaddr;
            value = (0x281F4000>>20) & 0xFFFF;//(0x281F4000>>20) & 0xFFFF;//(u4PhyAddr>>20) & 0xFFFF;
            printk(KERN_INFO "[ISP]: wdma base_H = 0x%08x\n", value);

        }
        else if(pPGscript[i][0] == 3 && pPGscript[i][1] == 0x0020) {

            //u4PhyAddr = awb_phyaddr;
            value = (0x281FF6F0>>4) & 0xFFFF;//(0x281FF6F0>>4) & 0xFFFF;//(u4PhyAddr>>4) & 0xFFFF;

            printk(KERN_INFO "[ISP]: wdma base_L = 0x%08x\n", value);

        }
        else if(pPGscript[i][0] == 3 && pPGscript[i][1] == 0x0024) {
            //u4PhyAddr = awb_phyaddr;
            value = (0x281FF6F0>>20) & 0xFFFF;//(0x281FF6F0>>20) & 0xFFFF;//(u4PhyAddr>>20) & 0xFFFF;
            printk(KERN_INFO "[ISP]: wdma base_H = 0x%08x\n", value);

        }
        else if(pPGscript[i][0] == 6 && pPGscript[i][1] == 0x00A0) {
            //u4PhyAddr = raw_meminfo.phys;
            value = (0x28000000>>4) & 0xFFFF;//(0x28000000>>4) & 0xFFFF;//(u4PhyAddr>>4) & 0xFFFF;

            printk(KERN_INFO "[ISP]: wdma base_L = 0x%08x\n", value);

        }
        else if(pPGscript[i][0] == 6 && pPGscript[i][1] == 0x00A4) {
            //u4PhyAddr = raw_meminfo.phys;
            value = (0x28000000>>20) & 0xFFFF;//(0x28000000>>20) & 0xFFFF;//(u4PhyAddr>>20) & 0xFFFF;
            printk(KERN_INFO "[ISP]: wdma base_H = 0x%08x\n", value);

        }
        else {
            value = pPGscript[i][2];
        }

        //printk(KERN_INFO "[ISP]: script %d addr:0x%08x value:0x%04x\n", i, (unsigned int)address, value);

        *address = value;

    }


   return 0;
}
#endif

#if 0
static void isp_write_iq_tbl(void)
{
    if(isp_mem.IsWriteIQtbl == 1){
        HalISPMLoadWriteAllTable((unsigned long)isp_mem.Base_Meminfo.phys, isp_mem.IQ_tblOffset);
        //HalISPMLoadWriteData(IQ_MEM_ALSC_RGAIN, isp_mem.Base_Meminfo.phys+isp_mem.IQ_tblOffset[IQ_MEM_ALSC_RGAIN]);
        isp_mem.IsWriteIQtbl = 0;
    }

}
#endif

static void isp_ae_cache_invalidate(void)
{
	Chip_Inv_Cache_Range((unsigned long)isp_mem.Base_Meminfo.kvirt + isp_mem.AE_Offset,isp_mem.AE_Size);
}

static int isp_set_ae_base(void)
{
    u32 phy_addr;
    u32 virt_addr;

    phy_addr = (unsigned long)isp_mem.Base_Meminfo.phys + isp_mem.AE_Offset;
    virt_addr = (unsigned long)isp_mem.Base_Meminfo.kvirt + isp_mem.AE_Offset;

    DBG_INFO("ISP_BASE = 0x%X, AE offset = 0x%X\n",(unsigned int)isp_mem.Base_Meminfo.phys,(unsigned int)isp_mem.AE_Offset);
    HalISPSetAeBaseAddr(phy_addr);

    return 0;
}

static void isp_awb_cache_invalidate(void)
{
	Chip_Inv_Cache_Range((unsigned long)isp_mem.Base_Meminfo.kvirt + isp_mem.AWB_Offset,isp_mem.AWB_Size);
}

static int isp_set_awb_base(void)
{
    u32 phy_addr;
    u32 virt_addr;

    phy_addr = (unsigned long)isp_mem.Base_Meminfo.phys + isp_mem.AWB_Offset;
    virt_addr = (unsigned long)isp_mem.Base_Meminfo.kvirt + isp_mem.AWB_Offset;

    DBG_INFO("set awb_base[%d]:0x%08x , virt = 0x%.8X", isp_mem.AWB_WriteIdx, phy_addr, virt_addr);
    HalISPSetAwbBaseAddr(phy_addr);

    return 0;
}

static int isp_set_mot_base(void)
{
    u32 phys_addr;
    phys_addr =  (unsigned long)isp_mem.Base_Meminfo.phys + isp_mem.MOT_Offset;
    HalISPSetMotBaseAddr(phys_addr);
    return 0;
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

static u32 isp_get16bytes_size(u32 size)
{
    u32 result_size;
    result_size = ((size%16)==0)?size:(size/16 + 1)*16;
    return result_size;
}

#if 0
extern MS_PACKAGE_TYPE Chip_Get_Package_Type(void);
static unsigned int get_dnr_buffer_size(void)
{
    MS_PACKAGE_TYPE pkg_type = 0;//Chip_Get_Package_Type();
    unsigned int img_w;
    unsigned int img_h;

#if 0  // 1 to use /Chip_Get_Package_Type()
    pkg_type = Chip_Get_Package_Type();
#else
    extern unsigned long totalram_pages;
    pr_debug("\n\n SYSTEM Total Pages = 0x%X \n\n",(unsigned int)totalram_pages);
    if(totalram_pages > 0x4000) //if DRAM size > 64MB
        pkg_type = MS_PACKAGE_BGA;
    else
        pkg_type = MS_PACKAGE_QFP;
#endif

    switch(pkg_type)
    {
    case MS_PACKAGE_QFP:
        img_w = 1280;
        img_h = 720;
        return img_w*img_h*2;
    break;
    default:
    case MS_PACKAGE_BGA:
        img_w = 1920;
        img_h = 1088;
        return (((img_w+191)/192) * ((img_h+15)>>4) * 256 * 16);
    break;
    }
}
#else
long g_isp_flag = ISP_FLAG_ENABLE_ROT | ISP_FLAG_ENABLE_DNR;
long g_isp_max_res = 1; //max resolutioin, 0:1280x720 1:1920x1080
static unsigned int get_dnr_buffer_size(void)
{
    u32 width = 1920;
    u32 height = 1080;
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
    }
    pr_info("[ISP] max_res= %dx%d\n",width,height);
    return (((width+191)/192) * ((height+15)>>4) * 256 * 16);
}
static unsigned int get_dnr_buffer_num(void)
{
    if(g_isp_flag&ISP_FLAG_ENABLE_DNR)
    {
        return (g_isp_flag&ISP_FLAG_ENABLE_ROT)?2:1;
    }
    else
        return 0;
}
#endif

static int isp_init_buff(void) {
    int i, base;
    pr_debug("[ISP] isp_init_buff\n");

    //isp_mem.AE_BuffNum = AE_RING_BUFF_NUM;
    isp_mem.AE_Size = AE_TOTAL_STATIS_SIZE;
    //isp_mem.AWB_BuffNum = AWB_RING_BUFF_NUM;
    isp_mem.AWB_Size = AWB_STATIS_SIZE;
    isp_mem.MOT_Size = MOT_STATIS_SIZE;

    isp_mem.IQ_tblSize[IQ_MEM_ALSC_RGAIN] = isp_get16bytes_size(IQ_LEN_ALSC_GAIN)* 2;//IQ_LEN_ALSC_GAIN*2;
    isp_mem.IQ_tblSize[IQ_MEM_ALSC_GGAIN] = isp_get16bytes_size(IQ_LEN_ALSC_GAIN)* 2;//IQ_LEN_ALSC_GAIN*2;
    isp_mem.IQ_tblSize[IQ_MEM_ALSC_BGAIN] = isp_get16bytes_size(IQ_LEN_ALSC_GAIN)* 2;//IQ_LEN_ALSC_GAIN*2;

    isp_mem.IQ_tblSize[IQ_MEM_GAMMA12TO10_RTBL] = IQ_LEN_GAMMA_12TO10*2;
    isp_mem.IQ_tblSize[IQ_MEM_GAMMA12TO10_GTBL] = IQ_LEN_GAMMA_12TO10*2;
    isp_mem.IQ_tblSize[IQ_MEM_GAMMA12TO10_BTBL] = IQ_LEN_GAMMA_12TO10*2;

    isp_mem.IQ_tblSize[IQ_MEM_DEFECT_PIXEL_POS] = IQ_LEN_DEFECT_PIXEL*2;

    isp_mem.IQ_tblSize[IQ_MEM_GAMMA10TO12_RTBL] = IQ_LEN_GAMMA_10TO12*2;
    isp_mem.IQ_tblSize[IQ_MEM_GAMMA10TO12_GTBL] = IQ_LEN_GAMMA_10TO12*2;
    isp_mem.IQ_tblSize[IQ_MEM_GAMMA10TO12_BTBL] = IQ_LEN_GAMMA_10TO12*2;

    isp_mem.IQ_tblSize[IQ_MEM_GAMMA_CORR_RTBL] = IQ_LEN_GAMMA_CORRECT*2;
    isp_mem.IQ_tblSize[IQ_MEM_GAMMA_CORR_GTBL] = IQ_LEN_GAMMA_CORRECT*2;
    isp_mem.IQ_tblSize[IQ_MEM_GAMMA_CORR_BTBL] = IQ_LEN_GAMMA_CORRECT*2;


    isp_mem.Base_Meminfo.length = isp_mem.AE_Size;
    isp_mem.Base_Meminfo.length +=isp_mem.AWB_Size;
    isp_mem.Base_Meminfo.length += isp_mem.MOT_Size;

    DBG_INFO("isp ae_size: %d, awb_size: %d", isp_mem.AE_Size, isp_mem.AWB_Size);

    for (i = 0; i < IQ_MEM_NUM; i++)  {
        isp_mem.Base_Meminfo.length += isp_mem.IQ_tblSize[i];
    }
    sprintf(isp_mem.Base_Meminfo.name,"ISP_base");
    msys_request_dmem(&isp_mem.Base_Meminfo);

    DBG_INFO("mem base phyaddr:0x%08x, viraddr:0x%08x, len:%d", (unsigned int)isp_mem.Base_Meminfo.phys,
        (unsigned int)isp_mem.Base_Meminfo.kvirt,isp_mem.Base_Meminfo.length);

    ////////// isp share data ////////////////
    sprintf(isp_mem.ShareData_Meminfo.name, "ISP_SHARE_DATA");
#if 0
    isp_mem.ShareData_Meminfo.length = sizeof(ISP_SHARE_DATA);
    msys_request_dmem(&isp_mem.ShareData_Meminfo);
    isp_int.share_data = (ISP_SHARE_DATA *)((u32)isp_mem.ShareData_Meminfo.kvirt);
#else
	isp_int.share_data = kmalloc(sizeof(ISP_SHARE_DATA),GFP_KERNEL);
	isp_mem.ShareData_Meminfo.kvirt = (unsigned long)isp_int.share_data;
	isp_mem.ShareData_Meminfo.phys = virt_to_phys(isp_int.share_data);
	isp_mem.ShareData_Meminfo.length = sizeof(ISP_SHARE_DATA);

    DBG_INFO("FrameState phyaddr:0x%08llx, viraddr:0x%08llx, len:%#x\n",
        isp_mem.ShareData_Meminfo.phys,
        isp_mem.ShareData_Meminfo.kvirt, isp_mem.ShareData_Meminfo.length);
#endif

    DBG_INFO(KERN_NOTICE "FrameState phyaddr:0x%08llx, viraddr:0x%08llx, len:%d\n",
        isp_mem.ShareData_Meminfo.phys,
        isp_mem.ShareData_Meminfo.kvirt, isp_mem.ShareData_Meminfo.length);

    base = 0;
    isp_mem.AE_Offset = 0;
    base +=  isp_mem.AE_Size;
    isp_mem.AWB_Offset = base;
    base += isp_mem.AWB_Size;

    //DBG_INFO("IQ vOffset[%d]:0x%08x", 0, (u32)isp_mem.IQ_tblOffset[0]);
    for (i = 0; i < IQ_MEM_NUM; i++)  {
      isp_mem.IQ_tblOffset[i] = base;
      base += isp_mem.IQ_tblSize[i];
      //DBG_INFO("IQ vOffset[%d]:0x%08x", i, (u32)isp_mem.IQ_tblOffset[i]);
    }

    isp_mem.MOT_Offset = base;
    base += isp_mem.MOT_Size;

    //isp_mem.AE_WriteIdx = 0;//AE_RING_BUFF_NUM;
    //isp_mem.AE_ReadIdx = 0;
    //isp_mem.AWB_WriteIdx = 0;//AWB_RING_BUFF_NUM;
    //isp_mem.AWB_ReadIdx = 0;
    isp_mem.IsWriteIQtbl = 0;

    isp_set_ae_base();

    isp_set_awb_base();

    isp_set_mot_base();

#if 1
    {
        //////////////////////////
        // request DNR frame buffer //
        //////////////////////////
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

    isp_set_ae_base();
    isp_set_awb_base();
    isp_set_mot_base();

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
    //int err= 0;
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
        case IOCTL_ISP_GET_INFO:
        {
            ISP_IOCTL_INFO info;
            info.int_cnt =  isp_int.int_cnt;
            info.frame_interval = isp_int.fps.frame_interval;

            if(copy_to_user((void __user *)arg,(void*)&info,sizeof(info)))
            {
                BUG();
            }
        }break;
    }
    pr_debug("ISP IOCTL -\n");
    return 0;
}

struct file_operations isp_fops =
{
    .owner = THIS_MODULE,
    .open = isp_open,
    .release = isp_release,
    .read = isp_fread,
    .write = isp_fwrite,
    //.compat_ioctl = isp_ioctl,
    .unlocked_ioctl = isp_ioctl,
};

#define MAJOR_ISP_NUM               234
#define MINOR_ISP_NUM               128
#define MINOR_CSI_NUM               127
static int isp_probe(struct platform_device* pdev)
{
    int err;
    int irq;
    unsigned int i, u4IO_PHY_BASE;
    unsigned int u4Banks[8];
    isp_dev_data *data;

    pr_debug("[ISP] = isp_probe\n");

    isp_int.int_cnt = 0;

    data = kzalloc(sizeof(isp_dev_data),GFP_KERNEL);
    of_property_read_u32(pdev->dev.of_node, "io_phy_addr", &u4IO_PHY_BASE);
    of_property_read_u32_array(pdev->dev.of_node, "banks", (unsigned int*)u4Banks, 8);
    for (i = 0; i < 8; i++) {
        isp_mem.pISPRegs[i] = (void*)ioremap(BANK_TO_ADDR32(u4Banks[i])+u4IO_PHY_BASE, BANK_SIZE);
    }

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
    //isp_create_bin_file(&isp_device->dev);

    //allocate statistics data and menload table memory
    isp_init_buff();

    // enable interrupt
    HalISPEnableInt();
    isp_int.sysfs_int = true; // turn on IRQ handler
    isp_int.rgb_ccm.dirty = 0;
    isp_int.yuv_ccm.dirty = 0;
    // setup kernel i2c
    //isp_i2c_init(isp_mem.pISPRegs[i]);
    data->isp_dev.minor = MISC_DYNAMIC_MINOR;
    data->isp_dev.name = "isp";
    data->isp_dev.fops = &isp_fops;
    data->isp_dev.parent = &pdev->dev;
    misc_register(&data->isp_dev);

    data->sysfs_dev = device_create(msys_get_sysfs_class(), NULL, MKDEV(MAJOR_ISP_NUM, MINOR_ISP_NUM), NULL, "isp0");
    isp_create_bin_file(data->sysfs_dev);
    //err = sysfs_create_link(&data->sysfs_dev->kobj,&isp_device->dev.kobj, "files"); //create symlink for older firmware version
    err = sysfs_create_link(&pdev->dev.parent->kobj,&data->sysfs_dev->kobj, "isp0"); //create symlink for older firmware version

    data->sysfs_dev->platform_data = pdev->dev.platform_data = (void*)data;

    //TEST CODE
    MDrv_GPIO_Pull_High(0);

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
    isp_dev_data *data = dev_get_platdata(&pdev->dev);

    pr_info("[ISP] = isp_suspend\n");
    if (!pdev)
    {
        pr_info("isp_suspend with NULL pdev %d", isp_int.frame_cnt);
        return 0;
    }
    isp_int.sysfs_int = false;
    //HalISPClkEnable(false);

    if(data->clk_isp) clk_disable_unprepare(data->clk_isp);
    if(data->clk_sr) clk_disable_unprepare(data->clk_sr);
    if(data->clk_sr_mclk) clk_disable_unprepare(data->clk_sr_mclk);

    return 0;
}

static int isp_resume(struct platform_device *pdev)
{
    isp_dev_data *data = dev_get_platdata(&pdev->dev);
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
    if(isp_int.rgb_ccm.dirty)
    {
        HalISPSetRGBCCM(isp_int.rgb_ccm.ccm);
        isp_int.rgb_ccm.dirty = 0;;
        print_timestamp("K_CCM1");
    }

    if(isp_int.yuv_ccm.dirty)
    {
        HalISPSetYUVCCM(isp_int.yuv_ccm.ccm);
        isp_int.yuv_ccm.dirty = 0;
        print_timestamp("K_CCM2");
    }

    if (isp_int.share_data->frame_state.u4OBCInValid == true){
      HalISPSetOBC(isp_int.share_data->frame_state.u4OBC_a, isp_int.share_data->frame_state.u4OBC_b);
      isp_int.share_data->frame_state.u4OBCInValid = false;
      print_timestamp("K_OBC");
    }

}

#define INC_TIME_REC(name) getnstimeofday(&isp_int.isp_int_time[name])
#define INC_TIME_REC2(name) getnstimeofday(&isp_int.isp_int_time2[name])
#define INC_TIME_REC3(name) getnstimeofday(&isp_int.isp_int_time3[name]))

#if 0
#define INC_COUNT(name) (isp_int.isp_int_count[name]++)
#define INC_COUNT2(name) (isp_int.isp_int2_count[name]++)
#define INC_COUNT3(name) (isp_int.isp_int3_count[name]++)
#else
#define INC_COUNT(name) {isp_int.isp_int_count[name]++;\
                                            getnstimeofday(&isp_int.isp_int_time[name]);}
#define INC_COUNT2(name) {isp_int.isp_int2_count[name]++;\
                                            getnstimeofday(&isp_int.isp_int2_time[name]);}
#define INC_COUNT3(name) {isp_int.isp_int3_count[name]++;\
                                            getnstimeofday(&isp_int.isp_int3_time[name]);}
#endif

irqreturn_t isp_ISR(int num, void *priv)
{
    volatile u32 u4Clear = 0;
    volatile u32 u4Clear2 = 0;
    volatile u32 u4Clear3 = 0;

    volatile u32 u4Status;
    volatile u32 u4Status2;
    volatile u32 u4Status3;

    volatile u32 u4VsyncPol;
    volatile u32 u4MIPI;

    isp_int.int_cnt++;

    u4Status = HalISPGetIntStatus1();
    HalISPMaskInt1(u4Status);
    u4Status2 = HalISPGetIntStatus2();
    HalISPMaskInt2(u4Status2);
    u4Status3 = HalISPGetIntStatus3();
    HalISPMaskInt3(u4Status3);

    u4VsyncPol = HalISPGetVsyncPol();
    u4MIPI = HalISPGetMIPI();
    DBG_INFO(KERN_NOTICE "[ISP] -s- sta=0x%X\n",u4Status);

    if (u4Status == 0 && u4Status2 == 0 && u4Status3 == 0) {
        pr_err("[ISP] False interrupt? mask1 0x%04x, mask2 0x%04x, mask3 0x%04x\n", u4Status, u4Status2, u4Status3);
        return IRQ_HANDLED;
    }

#if 0
    if (isp_int.sysfs_int == false) {
        DBG_INFO_1("isp_int.sysfs_int == false");
        HalISPDisableInt();
        return IRQ_HANDLED;
    }
#endif

    DBG_INFO("c=%d sta=0x%x\n", isp_int.frame_cnt, u4Status);

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
        DBG_INFO_1("MIPI VSYNC ACTIVE %d", isp_int.frame_cnt);
        pr_debug("VS\n");

        //TEST CODE
        //MDrv_GPIO_Set_High(0);
        print_timestamp("K_VS");//TEST CODE
    }

      //////////////////////////////////////////////
      //           statistics                     //
      //////////////////////////////////////////////
    if (ISP_CHECKBITS(u4Status, INT_AE_DONE)){
        isp_ae_cache_invalidate();
        DBG_INFO_1("INT_AE_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear, INT_AE_DONE);
        INC_COUNT(INT_AE_DONE);
        isp_int.ae = true;
        wake_up_interruptible_all(&isp_wq_ae);

        //TEST CODE
        //MDrv_GPIO_Set_Low(0);
    }

    // AE WIN0/1 Int Row
    if (ISP_CHECKBITS(u4Status3, INT3_AE_WIN0_DONE)) {
        DBG_INFO_1("INT3_AE_WIN0_DONE %d\n", isp_int.frame_cnt);
        isp_int.ae_win0 = true;
        HalISPClearInt(&u4Clear3, INT3_AE_WIN0_DONE);
        wake_up_interruptible_all(&isp_wq_ae_win0);
    }

    if (ISP_CHECKBITS(u4Status3, INT3_AE_BLK_ROW_INT_DONE)) {
        DBG_INFO_1("INT3_AE_BLK_ROW_INT_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear3, INT3_AE_BLK_ROW_INT_DONE);
        INC_COUNT3(INT3_AE_BLK_ROW_INT_DONE);
        isp_int.ae_row_int= true;
        wake_up_interruptible_all(&isp_wq_ae_row_int);
    }

    if (ISP_CHECKBITS(u4Status, INT_AWB_DONE)) {
        isp_awb_cache_invalidate(); //invalid cache data
        DBG_INFO_1("INT_AWB_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear, INT_AWB_DONE);
        INC_COUNT(INT_AWB_DONE);
        isp_int.awb = true;
        wake_up_interruptible_all(&isp_wq_awb);
    }

    if (ISP_CHECKBITS(u4Status, INT_AF_DONE)) {
        DBG_INFO_1("INT_AF_DONE %d", isp_int.frame_cnt);
        HalISPGetAFStat(isp_int.AF_Stat);
        HalISPClearInt(&u4Clear, INT_AF_DONE);
        INC_COUNT(INT_AF_DONE);
        isp_int.af = true;
        wake_up_interruptible_all(&isp_wq_af);
    }

    if (ISP_CHECKBITS(u4Status3, INT3_SW_INT_INPUT_DONE)) {
        DBG_INFO_1("INT3_SW_INT_INPUT_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear3, INT3_SW_INT_INPUT_DONE);
        INC_COUNT3(INT3_SW_INT_INPUT_DONE);
        wake_up_interruptible_all(&isp_wq_sw_int_in);
      }

    if (ISP_CHECKBITS(u4Status3, INT3_SW_INT_OUTPUT_DONE)) {
        DBG_INFO_1("INT3_SW_INT_OUTPUT_DONE %d", isp_int.frame_cnt);
        HalISPClearInt(&u4Clear3, INT3_SW_INT_OUTPUT_DONE);
        INC_COUNT3(INT3_SW_INT_OUTPUT_DONE);
        wake_up_interruptible_all(&isp_wq_sw_int_out);
    }

    if (ISP_CHECKBITS(u4Status, INT_ISP_BUSY)) {
        HalISPClearInt(&u4Clear, INT_ISP_BUSY);
        INC_COUNT(INT_ISP_BUSY);
        DBG_INFO_1("INT_ISP_BUSY %d", isp_int.frame_cnt);
        isp_int.isp_busy = true;
        wake_up_interruptible_all(&isp_wq_ISP_BUSY);
    }

    if (ISP_CHECKBITS(u4Status, INT_ISP_IDLE)) {
        HalISPClearInt(&u4Clear, INT_ISP_IDLE);
        INC_COUNT(INT_ISP_IDLE);
        DBG_INFO_1("INT_ISP_IDLE %d", isp_int.frame_cnt);
        isp_int.isp_idle = true;
        wake_up_interruptible_all(&isp_wq_ISP_IDLE);
    }

    //////////////////////////////
    //             Vsync end                  //
    //////////////////////////////

    if (!u4MIPI){
        if ((ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_RISING) && u4VsyncPol == false) || (ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_FALLING) && u4VsyncPol == true)){
            if (isp_int.vsync_end == false){
                if (ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_RISING) && u4VsyncPol == false ){
                    HalISPClearInt(&u4Clear, INT_PAD_VSYNC_RISING);
                    INC_COUNT(INT_PAD_VSYNC_RISING);
                    IspInputEnable(isp_int.share_data->frame_state.bIspInputEnable);
                    if (isp_int.frame_cnt%30==0)
                        DBG_INFO_1("Parallel Vsync End INT_PAD_VSYNC_RISING %d", isp_int.frame_cnt);
                }
                else if (ISP_CHECKBITS(u4Status, INT_PAD_VSYNC_FALLING) && u4VsyncPol == true){
                    HalISPClearInt(&u4Clear, INT_PAD_VSYNC_FALLING);
                    INC_COUNT(INT_PAD_VSYNC_FALLING);
                    IspInputEnable(isp_int.share_data->frame_state.bIspInputEnable);
                    if (isp_int.frame_cnt%30==0)
                        DBG_INFO_1("Parallel Vsync End INT_PAD_VSYNC_FALLING %d", isp_int.frame_cnt);
                }

                pr_debug("VE\n");
                DBG_INFO_1(KERN_NOTICE "Parallel VSYNC End...%d %d\n", isp_int.share_data->frame_state.bActive, isp_int.share_data->frame_state.u4FrameCnt);
                isp_int.vsync_end = true;
                isp_int.share_data->frame_state.bActive = false;
                isp_apply_iq_at_vend();
                wake_up_interruptible_all(&isp_wq_VEND);
                // Using menuload to write IQ table to SRAMs
                //isp_write_iq_tbl();
            }else{
                //DBG_INFO("Parallel Vsync End ---FALSE---");
            }
        }
    }

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
    } else {
        //DBG_INFO("INT_WDMA_DONE %d, --FALSE--", isp_int.frame_cnt);
    }

    if (ISP_CHECKBITS(u4Status, INT_RDMA_DONE) && isp_int.rdma_done == false) {
        HalISPClearInt(&u4Clear, INT_RDMA_DONE);
        INC_COUNT(INT_RDMA_DONE);
        DBG_INFO_1("INT_RDMA_DONE %d", isp_int.frame_cnt);
        isp_int.rdma_done = true;
        wake_up_interruptible_all(&isp_wq_RDMA_DONE);
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

static ssize_t isp_watchdog_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
	int watchdog_counter = atomic_read(&isp_watchdog_counter);
    data_size = sizeof(watchdog_counter);
    if (buf && size >= data_size) {
        memcpy((void *)buf, (void *)&watchdog_counter, data_size);
    } else
        data_size = 0;

    return data_size;

}

static ssize_t isp_watchdog_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
	int cmd, watchdogCNT;
	cmd = watchdogCNT = 0;

	sscanf(buf,"%d %d\n", &cmd, &watchdogCNT);
	//pr_info("cmd = %d, watchdogCNT = %d\n", cmd, watchdogCNT);
	// update watchdog timer counter
	if(cmd == WATCHDOG_UPDATE) {
		atomic_set(&isp_watchdog_counter, isp_watchdog_update_counter);
	}
	// initial timer and set isp_watchdog_counter to MAX_WATCHDOG_TIME
	if(cmd == WATCHDOG_KICK) {
		if(watchdogCNT != 0) {
			atomic_set(&isp_watchdog_counter, watchdogCNT);
			isp_watchdog_update_counter = watchdogCNT;
		}else{
			atomic_set(&isp_watchdog_counter, MAX_WATCHDOG_TIME);
			isp_watchdog_update_counter = MAX_WATCHDOG_TIME;
		}
		mod_timer( &isp_watchdog_timer, jiffies + msecs_to_jiffies(1000) );
	}

	// initial timer and set isp_watchdog_counter to MAX_WATCHDOG_TIME
	if(cmd == WATCHDOG_STOP) {
		del_timer_sync(&isp_watchdog_timer);
		atomic_set(&isp_watchdog_counter, 0);
	}
    return size;
}

static int isp_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
    typedef struct {
        unsigned long u4BaseAddr;
        u32 u4Size;
    } FRAMEINFO_MEM;

    FRAMEINFO_MEM frameinfo;
    frameinfo.u4BaseAddr = isp_mem.ShareData_Meminfo.phys + offsetof(ISP_SHARE_DATA,frame_state);
    frameinfo.u4Size = sizeof(FRAME_STATE);
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

    return size;
}

static int vs_sr_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
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

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

    return data_size;
}

static int vs_sr_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

typedef struct
{
  u32  frame_cnt;
  u32  hw_frame_cnt;
}__attribute__((packed, aligned(1))) ve_isr_data; //vsync end isr data

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
    DBG_INFO_2("ae virtual offset[%d]: 0x%08x", isp_mem.AE_ReadIdx, (u32)isp_mem.AE_Offset[isp_mem.AE_ReadIdx]);

    if(buf){
      ae_isr_data ae_data;
      ae_data.data_offset = isp_mem.AE_Offset;
      ae_data.fcount = isp_int.frame_cnt;
      size = size>sizeof(ae_data)?sizeof(ae_data):size;
      memcpy((void*)buf,(void*)&ae_data,size);
    }else {
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

    DBG_INFO_2("ae virtual offset[%d]: 0x%08x", isp_mem.AE_ReadIdx, (u32)isp_mem.AE_Offset[isp_mem.AE_ReadIdx]);

    if(buf){
        ae_isr_data ae_data;
        ae_data.data_offset = isp_mem.AE_Offset;
        ae_data.fcount = isp_int.frame_cnt;
        size = size>sizeof(ae_data)?sizeof(ae_data):size;
        memcpy((void*)buf,(void*)&ae_data,size);
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

    if(buf){
        ae_isr_data ae_data;
        ae_data.data_offset = isp_mem.AE_Offset;
        ae_data.fcount = isp_int.frame_cnt;
        size = size>sizeof(ae_data)?sizeof(ae_data):size;
        memcpy((void*)buf,(void*)&ae_data,size);
    }
    else {
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
    awb_isr_data awb_data;
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

    DBG_INFO_2("awb virtual offset[%d]: 0x%08x", isp_mem.AWB_ReadIdx, (u32)isp_mem.AWB_Offset[isp_mem.AWB_ReadIdx]);

    if (buf) {
        awb_data.data_offset = isp_mem.AWB_Offset;
        awb_data.fcount = isp_int.frame_cnt;
        size = size>sizeof(awb_data)?sizeof(awb_data):size;
        memcpy((void*)buf,(void*)&awb_data, size);
    } else {
        size = 0;
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
    u32 af_size = sizeof(isp_int.AF_Stat);
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

    if (buf && size >= af_size) {
        memcpy((void *)buf, (void *)isp_int.AF_Stat, af_size);
    } else
        af_size = 0;

    return af_size;
}
static int af_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int busy_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    int data_size = 0;
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

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

    return data_size;
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

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

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

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

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

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

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

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

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

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

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

    return 0;
}
static int eis_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{  // FIXJASON
    DBG_INFO();
    return size;
}

static int iq_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    unsigned short *pData;

    if (buf && size >= 1) {
        //Temp using
        if(buf[0] == 1) {
            isp_mem.IsWriteIQtbl = buf[0];

        }
        else if(buf[0] == 2) {
            isp_mem.IsWriteIQtbl = 1;
        }
        else if(buf[0] == 3) {
            pData = (unsigned short*)((uintptr_t)(isp_mem.Base_Meminfo.kvirt+isp_mem.IQ_tblOffset[IQ_MEM_ALSC_BGAIN]));
        }
    }

    return size;
}

// iq
typedef struct {
    u32 u4BaseAddr;
    u32 u4Size;
    u32 u4IqTblOffset[IQ_MEM_NUM];
    u32 u4IqTblSize[IQ_MEM_NUM];
} ISP_KERNEL_MEM;

static int iq_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    if (buf && size >= sizeof(ISP_KERNEL_MEM))
    {
      ISP_KERNEL_MEM *mem_info = (ISP_KERNEL_MEM*) buf;
      mem_info->u4BaseAddr =  isp_mem.Base_Meminfo.phys;
      mem_info->u4Size = isp_mem.Base_Meminfo.length;
      memcpy(mem_info->u4IqTblOffset, (void *)isp_mem.IQ_tblOffset, sizeof(isp_mem.IQ_tblOffset));
      memcpy(mem_info->u4IqTblSize, (void *)isp_mem.IQ_tblSize, sizeof(isp_mem.IQ_tblSize));
    }
    return sizeof(ISP_KERNEL_MEM);
}

// motion statistic
static int mot_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
  if (buf && size >= isp_mem.MOT_Size)
  {
    memcpy( buf, (unsigned char*)((uintptr_t)(isp_mem.Base_Meminfo.kvirt+isp_mem.MOT_Offset)) ,isp_mem.MOT_Size);
    return isp_mem.MOT_Size;
  }
  return 0;
}
static int mot_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
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

    return 0;
}

static int scl_fe_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
    DBG_INFO();
    return size;
}

void isp_to_scl_ve_isr(void)
{
    print_timestamp("K_ISP2SCL");
}

void scl_ve_isr(void)
{
    print_timestamp("K_SCL_DONE");
    if (!isp_int.scl_fe){
        isp_int.scl_fe = true;
        wake_up_interruptible_all(&isp_wq_scl_fe);
    }
}

#if 0
//sensor i2c
typedef struct
{
  u32 cmd;
  u32 data_len;
  char data[1];
}__attribute__((packed, aligned(1))) isp_i2c_k_cmd; //kernel i2c command
#define I2C_K_CMD_GET_BUFFER	0x00
#define I2C_K_CMD_WRITE		0x01
#define I2C_K_CMD_READ			0x02

static int i2c_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
  return 0;
}
static int i2c_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
  i2c_handle_t* handle = (i2c_handle_t*) attr->private;
  if (buf && size >= sizeof(ISP_K_I2C_RW))
  {
    ISP_K_I2C_RW *cmd = (ISP_K_I2C_RW*)buf;
    memcpy(handle->req.data,cmd->i2c_data,sizeof(I2C_ARRAY)*cmd->num_i2c_data); //copy i2c data for write
    handle->req.ndata = cmd->num_i2c_data;
    handle->req.mode = cmd->cfg.mode;
    handle->req.fmt = cmd->cfg.fmt;
    handle->req.slave_addr = cmd->cfg.address;
    handle->req.speed = cmd->cfg.speed;
    handle->req.status = I2C_REQ_STATUS_WAIT;
    pr_debug("i2cK, %d data, slave addr=0x%x",cmd->num_i2c_data,cmd->cfg.address);
  }
  return 0;
}
#endif

#if 0
//share memory address get
typedef struct
{
  u32 id;
  u8 data[1];
}share_mem_ctl;

typedef enum
{
  SHARE_FRAME_STATE = 0,
  SHARE_I2C = 1,
}ISP_SHARE_MEM_ID;

static int share_mem_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
  share_mem_ctl *ctl = (share_mem_ctl*)buf;
  //printk("%s+\n",__FUNCTION__);
  printk("ctl id = %d\n",ctl->id);
  switch(ctl->id)
  {
    case SHARE_FRAME_STATE:
	printk("get share frame stat\n");
    break;
    case SHARE_I2C:
	printk("get share i2c\n");
    break;
  }
  printk("return size =%d\n", size);
  return size/2;
}

static int share_mem_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size)
{
  return 0;
}
#endif

static ISP_BIN_ATTR(isp, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(isp_watchdog, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
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
static ISP_BIN_ATTR(iq, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(mot, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(scl_fe, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
//static ISP_BIN_ATTR(i2c, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
//static ISP_BIN_ATTR(share_mem, S_IRUSR |S_IRGRP |S_IROTH);
//static ISP_BIN_ATTR(share_mem,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);

static ssize_t isp_ints_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%s			: %d\n","frame_count",isp_int.frame_cnt);
    str += scnprintf(str, end - str, "%s		: %d\n","frame_interval(ns)",isp_int.fps.frame_interval);
#if 0
    str += scnprintf(str, end - str, "%s			: %d\n","SR_VREF_RISING",isp_int.isp_int_count[INT_SR_VREF_RISING]);
    str += scnprintf(str, end - str, "%s			: %d\n","SR_VREF_FALLING",isp_int.isp_int_count[INT_SR_VREF_FALLING]);
    str += scnprintf(str, end - str, "%s			: %d\n","STROBE_DONE",isp_int.isp_int_count[INT_STROBE_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","DB_UPDATE_DONE",isp_int.isp_int_count[INT_DB_UPDATE_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","AF_DONE",isp_int.isp_int_count[INT_AF_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","ISP_FIFO_FUL",isp_int.isp_int_count[INT_ISP_FIFO_FULL]);
    str += scnprintf(str, end - str, "%s			: %d\n","ISP_BUSY",isp_int.isp_int_count[INT_ISP_BUSY]);
    str += scnprintf(str, end - str, "%s			: %d\n","ISP_IDLE",isp_int.isp_int_count[INT_ISP_IDLE]);
    str += scnprintf(str, end - str, "%s			: %d\n","AWB_DONE",isp_int.isp_int_count[INT_AWB_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","WDMA_DONE",isp_int.isp_int_count[INT_WDMA_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","RDMA_DONE",isp_int.isp_int_count[INT_RDMA_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","WDMA_FIFO_FULL",isp_int.isp_int_count[INT_WDMA_FIFO_FULL]);
    str += scnprintf(str, end - str, "%s			: %d\n","PAD_VSYNC_RISING",isp_int.isp_int_count[INT_PAD_VSYNC_RISING]);
    str += scnprintf(str, end - str, "%s			: %d\n","PAD_VSYNC_FALLING",isp_int.isp_int_count[INT_PAD_VSYNC_FALLING]);
    str += scnprintf(str, end - str, "%s			: %d\n","ISPIF_VSYNC",isp_int.isp_int_count[INT_ISPIF_VSYNC]);
    str += scnprintf(str, end - str, "%s			: %d\n","AE_DONE",isp_int.isp_int_count[INT_AE_DONE]);

    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_EVERYLINE",isp_int.isp_int2_count[INT2_VDOS_EVERYLINE]);
    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_LINE0",isp_int.isp_int2_count[INT2_VDOS_LINE0]);
    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_LINE1 ",isp_int.isp_int2_count[INT2_VDOS_LINE1]);
    str += scnprintf(str, end - str, "%s			: %d\n","VDOS_LINE2",isp_int.isp_int2_count[INT2_VDOS_LINE2]);

    str += scnprintf(str, end - str, "%s			: %d\n","AE_WIN0_DONE",isp_int.isp_int3_count[INT3_AE_WIN0_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","AE_WIN1_DONE",isp_int.isp_int3_count[INT3_AE_WIN1_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","AE_BLK_ROW_INT_DONE",isp_int.isp_int3_count[INT3_AE_BLK_ROW_INT_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","MENULOAD_DONE",isp_int.isp_int3_count[INT3_MENULOAD_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","SW_INT_INPUT_DONE",isp_int.isp_int3_count[INT3_SW_INT_INPUT_DONE]);
    str += scnprintf(str, end - str, "%s			: %d\n","SW_INT_OUTPUT_DONE",isp_int.isp_int3_count[INT3_SW_INT_OUTPUT_DONE]);
#else
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
#endif
    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}
DEVICE_ATTR(isp_ints, 0444, isp_ints_show, NULL);

static ssize_t isp_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
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
        //error = sysfs_create_link(&dev->parent->kobj, kobj_isp, "isp0");
        //error = sysfs_create_link(&dev->kobj, kobj_isp, "isp0");
        error = sysfs_create_bin_file(kobj_isp, &isp_attr);
        error = sysfs_create_bin_file(kobj_isp, &isp_watchdog_attr);
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
        error = sysfs_create_bin_file(kobj_isp, &iq_attr);
        error = sysfs_create_bin_file(kobj_isp, &mot_attr);
        error = sysfs_create_bin_file(kobj_isp, &scl_fe_attr);

        //i2c_attr.private = (void*) &isp_i2c;
        //error = sysfs_create_bin_file(kobj_isp, &i2c_attr);

        //share_mem_attr.private = 0;
        //error = sysfs_create_bin_file(kobj_isp, &share_mem_attr);

        error = device_create_file(dev,&dev_attr_isp_ints);
        error = device_create_file(dev,&dev_attr_isp_info);
        error = device_create_file(dev,&dev_attr_isp_fps);
    }
    //pr_debug("[%s] end\n", __FUNCTION__);
    return error;
}

void isp_remove_bin_file(void)
{
    sysfs_remove_bin_file(kobj_isp, &isp_attr);
    sysfs_remove_bin_file(kobj_isp, &isp_watchdog_attr);
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
    sysfs_remove_bin_file(kobj_isp, &eis_attr);  // FIXJASON
    sysfs_remove_bin_file(kobj_isp, &sw_int_in_attr);
    sysfs_remove_bin_file(kobj_isp, &sw_int_out_attr);
    sysfs_remove_bin_file(kobj_isp, &iq_attr);
    //sysfs_remove_bin_file(kobj_isp, &share_mem_attr);
    //sysfs_remove_bin_file(kobj_isp, &i2c_attr);
    //    DBG_INFO("[%s] end", __FUNCTION__);
}


int csi_create_bin_file(struct device *dev)
{
    csi_dev_data *data = dev_get_platdata(dev);
    int error = -EINVAL;
    if (dev != NULL) {
        //frame end
        csi_fe_attr.private = (void*) data;
        error = sysfs_create_bin_file(&dev->kobj, &csi_fe_attr);

        //frame start
        csi_fs_attr.private = (void*) data;
        error = sysfs_create_bin_file(&dev->kobj, &csi_fs_attr);

        //CSI error report interrupt
        error = device_create_file(dev, &dev_attr_csi_dbg_mask);
        //CSI interrupt counter
        error = device_create_file(dev,&dev_attr_csi_ints);
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
    int err;
    int irq, u4IO_PHY_BASE;
    unsigned int u4Bank;
    csi_dev_data *data;
    data = kzalloc(sizeof(csi_dev_data),GFP_KERNEL);

    of_property_read_u32(pdev->dev.of_node, "io_phy_addr", &u4IO_PHY_BASE);  //get custom property, OPR base address
    of_property_read_u32(pdev->dev.of_node, "banks", &u4Bank);         //get custom property,  CSI OPR bank offset

    data->reg_base = (void*)ioremap(BANK_TO_ADDR32(u4Bank)+u4IO_PHY_BASE, 0x200);
    pr_debug("[%s] IO remap phys:0x%.8x virt: 0x%.8x\n", __FUNCTION__,
                    BANK_TO_ADDR32(u4Bank)+u4IO_PHY_BASE,
                    (u32) data->reg_base );         //(u32) isp_mem.pCSIRegs );

    csi_create_bin_file(&pdev->dev);

    data->sysfs_dev = device_create(msys_get_sysfs_class(), NULL, MKDEV(MAJOR_ISP_NUM, MINOR_CSI_NUM), NULL, "csi0");
    isp_create_bin_file(data->sysfs_dev);
    err = sysfs_create_link(&pdev->dev.parent->kobj,&data->sysfs_dev->kobj, "csi0"); //create symlink for older firmware version

    //TODO, init status
    data->lock = __SPIN_LOCK_UNLOCKED("csi_lock");
    data->count = 0;
    data->frame_end = false;

    //data->p_wq_fe = &csi_wq_fe;    //notify csi frame end
    data->p_wq_fe = &isp_wq_VEND;  //notify isp frame end
    //data->p_wq_fs = &csi_wq_fs;    //notify csi frame start
    data->p_wq_fs = &isp_wq_VSTART;    //notify ISP frame start

    //TODO: CSI clock enable
    data->hal_handle = HalCsi_Open(data->reg_base);
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
        IspInputEnable(isp_int.share_data->frame_state.bIspInputEnable);
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
	// setup watchdog timer
	setup_timer( &isp_watchdog_timer, isp_watchdog_callback, 0 );
    return 0;
}

static void __exit mstar_isp_exit(void)
{
	// remove isp watchdog timer
	del_timer(&isp_watchdog_timer);
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
    long flag = ISP_FLAG_ENABLE_DNR | ISP_FLAG_ENABLE_ROT;
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

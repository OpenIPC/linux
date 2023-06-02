#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/poll.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "mdrv_jpe.h"
#include "mdrv_jpe_io.h"
#include "mdrv_jpe_io_st.h"
#include "drv_jpe.h"
#include "jpe_scatter.h"
#include "ms_msys.h"

#define MDRV_JPE_DEVICE_COUNT   1               // How many device will be installed
#define MDRV_JPE_NAME           "mstar_jpe"
#define MDRV_JPE_MINOR          0
#define MDRV_JPE_CLASS_NAME     "mstar_jpe_class"


//-------------------------------------------------------------------------------------------------
// Driver Data Structure
//-------------------------------------------------------------------------------------------------
typedef struct jpe_dev_data
{
    struct platform_device  *pdev;          // Platform device
    struct cdev             cdev;           // Character device
    int                     ref_count;      // Reference count, how many file instances opened
    JpeHandle_t             jpe_handle;
//  int                     clk_ref_count;
    int                     hw_enable;
//  int                     jpe_on;
    struct jpe_file_data    *cur_file;
    void __iomem            *regs;          // I/O register base address
    struct clk*             clk;            // Clock
    unsigned int            irq;            // IRQ number
    unsigned int            iobase;
//    spinlock_t              lock;
    struct semaphore        jpe_sem;
    wait_queue_head_t       jpe_wqh;
//  void*                   dev_data;       // driver data
} jpe_dev_data;

struct jpe_enc_outbuf
{
//    struct list_head list;
//    unsigned long    data;
    unsigned long   addr;
    unsigned long   orig_size;              // Original buffer Size
    unsigned long   output_size;            // Output Size
    JpeState_e      eState;
};

typedef struct jpe_file_data
{
    jpe_dev_data            *dev_data;
//    JpeHandle_t             jpe_handle;
    JpeCfg_t                jpeCfg;
#if SCATTER_USED
    struct list_head      outbuf_scatter_head; // Scatter list wrapper header for output buffer
#else
    struct jpe_enc_outbuf   enc_outbuf;
#endif
    unsigned long           nOutBufAddr;
    wait_queue_head_t       wait_queue;
    JpeHalOutBufCfg_t       jpeHalBuf;
    unsigned long           nToggleOutBuf[2];
    u32                     nToggleOutBufIdx;
} jpe_file_data;

//-------------------------------------------------------------------------------------------------
// Clock Enable
//-------------------------------------------------------------------------------------------------

/*******************************************************************************************************************
 * mdrv_jpe_clock_on
 *   enable device clock
 *
 * Parameters:
 *   dev_data:  device data
 *
 * Return:
 *   0: OK, others: failed
 */
static int mdrv_jpe_clock_on(jpe_dev_data *dev_data)
{
    int num_parents, i;
    struct clk **jpe_clks;
    struct clk *clk_parent;
    u32    clk_select;
    int    err;

    num_parents = of_clk_get_parent_count(dev_data->pdev->dev.of_node);
    if(num_parents > 0)
    {
        jpe_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);

        for(i = 0; i < num_parents; i++)
        {
            jpe_clks[i] = of_clk_get(dev_data->pdev->dev.of_node, i);
            if (IS_ERR(jpe_clks[i]))
            {
                JPE_MSG(JPE_MSG_ERR, "can't get clock node from %s\n", dev_data->pdev->dev.of_node->full_name);
                kfree(jpe_clks);
                return -1;
            }
            else
            {
                if(i == 0)
                {
                    /* Get Clock select */
                    if(of_property_read_u32(dev_data->pdev->dev.of_node, "clk-select", &clk_select) != 0)
                    {
                        JPE_MSG(JPE_MSG_ERR, "failed to read clk-select\n");
                        return -1;
                    }
                    JPE_MSG(JPE_MSG_DEBUG, "clk-select = %d\n", clk_select);
                    /* Get parent clock */
                    clk_parent = clk_get_parent_by_index(jpe_clks[i], clk_select);
                    if(IS_ERR(clk_parent))
                    {
                        JPE_MSG(JPE_MSG_ERR, "can't get parent clock\n");
                        return -1;
                    }
                    /* Set clock parent */
                    err = clk_set_parent(jpe_clks[i], clk_parent);
                    if(err != 0)
                        return err;
                    dev_data->clk = jpe_clks[i];
                }
                clk_prepare_enable(jpe_clks[i]);
            }
        }
        kfree(jpe_clks);
    }
    return 0;
}

static int mdrv_jpe_clock_off(jpe_dev_data *dev_data)
{
    int num_parents, i;
    struct clk **jpe_clks;

    num_parents = of_clk_get_parent_count(dev_data->pdev->dev.of_node);
    if(num_parents > 0)
    {
        jpe_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);

        for(i = 0; i < num_parents; i++)
        {
            jpe_clks[i] = of_clk_get(dev_data->pdev->dev.of_node, i);
            if (IS_ERR(jpe_clks[i]))
            {
                JPE_MSG(JPE_MSG_ERR, "can't get clock node from %s\n", dev_data->pdev->dev.of_node->full_name);
                kfree(jpe_clks);
                return -1;
            }
            else
            {
                if(i == 0)
                {
                    dev_data->clk = NULL;
                }
                clk_disable_unprepare(jpe_clks[i]);
                clk_put(jpe_clks[i]);
            }
        }
        kfree(jpe_clks);
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------
// File operations
//-------------------------------------------------------------------------------------------------

/*******************************************************************************************************************
 * mdrv_jpe_drv_isr
 *   ISR handler
 *
 * Parameters:
 *   irq:      IRQ
 *   dev_data: Device data which is assigned from request_irq()
 *
 * Return:
 *   Always IRQ_HANDLED to stop parsing ISR
 */
irqreturn_t  mdrv_jpe_drv_isr(int irq, void* dev_data)
{
    jpe_dev_data* _dev_data = (jpe_dev_data*)dev_data;
    jpe_file_data  *file_data = _dev_data->cur_file;
#if SCATTER_USED
    JpeBufInfo_t out_buf;
//  u32 next_toggle_idx = file_data->nToggleOutBufIdx;
    unsigned long addr, size;
#endif

    /* Call IRQ handler */
    JpeIsrHandler(irq, &_dev_data->jpe_handle);

    switch(_dev_data->jpe_handle.ejpeDevStatus)
    {
    case JPE_DEV_OUTBUF_FULL:
#if SCATTER_USED
        /* Update scatter status */
        jpe_scatter_frag_set(&file_data->dev_data->pdev->dev,
                             &file_data->outbuf_scatter_head,
                             file_data->nOutBufAddr, // file_data->nToggleOutBuf[file_data->nToggleOutBufIdx],
                             file_data->jpe_handle.nEncodeSize,
                             JPE_OUTBUF_FULL_STATE);

        /* Clear processed fragment */
        file_data->nOutBufAddr = 0;
//      file_data->nToggleOutBuf[file_data->nToggleOutBufIdx] = 0;

        /* Try to get next fragment */
        if(jpe_scatter_frag_get(&file_data->outbuf_scatter_head, &addr, &size) != 0)
        {
            /* there is no more fragment!! */
            /* Wake up waiting thread/process */
            wake_up_interruptible(&file_data->wait_queue);
            break;
        }

        /* config setting of output buffer */
        out_buf.nAddr = addr;
        out_buf.nSize = size;

        /* Set output buffer */
        DrvJpeSetOutBuf(&file_data->jpe_handle, &out_buf);

        /* Update status */
        file_data->jpe_handle.ejpeDevStatus = JPE_DEV_BUSY;
        file_data->nOutBufAddr = addr;
#else
        JPE_MSG(JPE_MSG_WARNING, "JPE_DEV_OUTBUF_FULL!!\n");
        file_data->enc_outbuf.addr = file_data->nOutBufAddr;
        file_data->enc_outbuf.output_size = _dev_data->jpe_handle.nEncodeSize;
        file_data->enc_outbuf.eState = JPE_OUTBUF_FULL_STATE;

        /* Clear filled buffer address */
        file_data->nOutBufAddr = 0;

        /* Wake up waiting thread/process */
        wake_up_interruptible(&file_data->wait_queue);

        /* Enter critical section */
        down(&_dev_data->jpe_sem);
        _dev_data->cur_file = NULL;
        /* Leave critical section */
        up(&_dev_data->jpe_sem);
        wake_up(&_dev_data->jpe_wqh);

#endif
        break;

    case JPE_DEV_ENC_DONE:
        /* Update scatter status */
#if SCATTER_USED
        jpe_scatter_frag_set(&file_data->dev_data->pdev->dev,
                             &file_data->outbuf_scatter_head,
                             file_data->nOutBufAddr,
                             file_data->jpe_handle.nEncodeSize,
                             JPE_FRAME_DONE_STATE);
#else
        file_data->enc_outbuf.addr = file_data->nOutBufAddr;
        file_data->enc_outbuf.output_size = _dev_data->jpe_handle.nEncodeSize;
        file_data->enc_outbuf.eState = JPE_FRAME_DONE_STATE;
#endif

        /* Clear filled buffer address */
        file_data->nOutBufAddr = 0;

        /* Wake up waiting thread/process */
        wake_up_interruptible(&file_data->wait_queue);

        /* Enter critical section */
//        spin_lock(&_dev_data->lock);
        down(&_dev_data->jpe_sem);
//      file_data->dev_data->jpe_on = 0;
        _dev_data->cur_file = NULL;
        /* Leave critical section */
//        spin_unlock(&_dev_data->lock);
        up(&_dev_data->jpe_sem);
        wake_up(&_dev_data->jpe_wqh);
        break;

    default:
        break;
    }

    return IRQ_HANDLED;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_open
 *   File open handler
 *   The device can has a instance at the same time, and the open
 *   operator also enable the clock and request q ISR.
 *
 * Parameters:
 *   inode: inode
 *   filp:  file structure
 *
 * Return:
 *   standard return value
 */
int mdrv_jpe_drv_open(struct inode *inode, struct file *filp)
{
    jpe_dev_data   *dev_data;
    jpe_file_data  *file_data;
    int err;

    JPE_MSG(JPE_MSG_DEBUG, "try to open a jpe handle\n");

    dev_data = container_of(inode->i_cdev, struct jpe_dev_data, cdev);

    /* Enter critical section */
//    spin_lock(&dev_data->lock);
    /* Only one file can be create at the same time!! */
//    if(dev_data->ref_count != 0)
//    {
//        spin_unlock(&dev_data->lock); // leave critical section at first
//        JPE_MSG(JPE_MSG_ERR, "error: user can open only one instance at the same time!!\n");
//        return -EMFILE;
//    }
    /* Increase instance */
//    dev_data->ref_count++;
    /* Leave critical section */
//    spin_unlock(&dev_data->lock);

    /* allocate buffer */
    file_data = kmalloc(sizeof(jpe_file_data), GFP_KERNEL);
    if(file_data == NULL)
    {
        JPE_MSG(JPE_MSG_ERR, "error: can't allocate buffer\n");
        err = -ENOSPC;
        goto ERROR_3;
    }

    /* Assgin dev_data and keep file_data in the file structure */
#if    SCATTER_USED
    INIT_LIST_HEAD(&file_data->outbuf_scatter_head);
#else
    file_data->enc_outbuf.addr = 0;
    file_data->enc_outbuf.orig_size = 0;
    file_data->enc_outbuf.output_size = 0;
    file_data->enc_outbuf.eState = JPE_IDLE_STATE;
#endif
    file_data->dev_data = dev_data;
//    dev_data->jpe_handle.jpeHalHandle.nBaseAddr= dev_data->iobase;
//    dev_data->jpe_handle.jpeHalHandle.pJpeReg = kzalloc(sizeof(JpeReg_t), GFP_KERNEL);
    filp->private_data = file_data;
    /* Initialize wait queue */
    init_waitqueue_head(&file_data->wait_queue);

    /* Reset JPE DRV */
    //DrvJpeReset(&file_data->jpe_handle);

    /* Enable clock */
    down(&dev_data->jpe_sem);
//    if( dev_data->clk_ref_count == 0)
//    {
//        dev_data->clk_ref_count++;
//        mdrv_jpe_clock_on(dev_data);
//    }
//    if (!dev_data->ref_count)
//    {
        if(!dev_data->hw_enable)
        {
            JPE_MSG(JPE_MSG_DEBUG, "file(%p): open jpe hw\n", file_data);
//            dev_data->jpe_handle.jpeHalHandle.nBaseAddr= dev_data->iobase;
//            dev_data->jpe_handle.jpeHalHandle.pJpeReg = kzalloc(sizeof(JpeReg_t), GFP_KERNEL);
            mdrv_jpe_clock_on(dev_data);
            /* Register a ISR */
            err = request_irq(dev_data->irq, mdrv_jpe_drv_isr, IRQ_TYPE_LEVEL_HIGH, "isp interrupt", dev_data);
            if(err != 0)
            {
                JPE_MSG(JPE_MSG_ERR, "isp interrupt failed (irq: %d, errno:%d)\n", file_data->dev_data->irq, err);
                up(&dev_data->jpe_sem);
                goto ERROR_1;
            }
            dev_data->hw_enable = 1;
        }
//    }
    /* Increase instance */
    dev_data->ref_count++;
    up(&dev_data->jpe_sem);

    return 0;

ERROR_1:
    clk_disable_unprepare(dev_data->clk);
    clk_put(dev_data->clk);
//ERROR_2:
    kfree(file_data);
ERROR_3:
//    dev_data->clk = NULL;
//    dev_data->irq = 0;
//    dev_data->ref_count--;
    return err;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_release
 *   File close handler
 *   The operator will release clock & ISR
 *
 * Parameters:
 *   inode: inode
 *   filp:  file structure
 *
 * Return:
 *   standard return value
 */
int mdrv_jpe_drv_release(struct inode *inode, struct file *filp)
{
    jpe_file_data *file_data = (jpe_file_data*)filp->private_data;
    jpe_dev_data *dev_data = file_data->dev_data;

    JPE_MSG(JPE_MSG_DEBUG, "relese a jpe handle\n");

    /* Reset JPE DRV */
//    DrvJpeReset(&dev_data->jpe_handle);

#if    SCATTER_USED
    /* Release scatter list */
    jpe_scatter_list_release(&file_data->dev_data->pdev->dev, &file_data->outbuf_scatter_head);
#endif

    /* Free a ISR */
//    free_irq(dev_data->irq, dev_data);

//    spin_lock(&dev_data->lock);
//    /* Reduce the reference count */
//    dev_data->ref_count--;
//    spin_unlock(&dev_data->lock);


    down(&dev_data->jpe_sem);
    /* Reduce the reference count */
    dev_data->ref_count--;
//  if( file_data->dev_data->clk_ref_count > 0)
//  {
//      file_data->dev_data->clk_ref_count--;
//      mdrv_jpe_clock_off(file_data->dev_data);
//  }
        if(!dev_data->ref_count && dev_data->hw_enable)
        {
            JPE_MSG(JPE_MSG_DEBUG, "file(%p): close jpe hw\n", file_data);
            /* Free a ISR */
            free_irq(dev_data->irq, dev_data);
            mdrv_jpe_clock_off(dev_data);
            dev_data->hw_enable = 0;
//            kfree(dev_data->jpe_handle.jpeHalHandle.pJpeReg);
        }
    up(&dev_data->jpe_sem);

    /* Release memory */
//    kfree(file_data->jpe_handle.jpeHalHandle.pJpeReg);
    kfree(file_data);

    return 0;
}

#ifdef JPE_MSG_ENABLE
// g_param_list is a  private data for mdrv_jpe_drv_show_ioctl_cmd()
#define DEF_2_STR(d) {d, #d}
struct
{
    unsigned int cmd;
    char *name;
}
g_param_list[] =
{
    DEF_2_STR(JPE_IOC_INIT),
    DEF_2_STR(JPE_IOC_ENCODE_FRAME),
    DEF_2_STR(JPE_IOC_GETBITS),
    DEF_2_STR(JPE_IOC_GET_CAPS),
    DEF_2_STR(JPE_IOC_SET_OUTBUF)
};

/*******************************************************************************************************************
 * mdrv_jpe_drv_show_ioctl_cmd
 *   Debug function to show the string of ioctl command
 *
 * Parameters:
 *   filp: file structure
 *   cmd:  command
 *   arg:  argument
 *
 * Return:
 *   none
 */
static void mdrv_jpe_drv_show_ioctl_cmd(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int i;

    for(i=0; i<ARRAY_SIZE(g_param_list); i++)
    {
        if(g_param_list[i].cmd == cmd)
        {
            JPE_MSG(JPE_MSG_DEBUG, "ioctl cmd %s, file %p\n", g_param_list[i].name, filp->private_data);
            return;
        }
    }

    JPE_MSG(JPE_MSG_DEBUG, "ioctl cmd 0x%X, file %p\n", cmd, filp->private_data);
}
#endif

/*******************************************************************************************************************
 * mdrv_jpe_drv_ioctl_get_caps
 *   IOCTL handler for JPE_IOC_GET_CAPS.
 *   This command provide the capabilities of JPE
 *
 * Parameters:
 *   file_data: file private data
 *   arg:  argument, a pointer of JpeCaps_t from user-space
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e mdrv_jpe_drv_ioctl_get_caps(jpe_file_data *file_data, unsigned long arg)
{
    JpeCaps_t caps;
    int err;
    jpe_dev_data *dev_data = file_data->dev_data;

    /* Get capability */
    err = DrvJpeGetCaps(&dev_data->jpe_handle, &caps);
    if(err != JPE_IOC_RET_SUCCESS)
    {
        return err;
    }

    /* Copy to user-space */
    err = copy_to_user((void*)arg, &caps, sizeof(caps));
    if(err != 0)
    {
        err = JPE_IOC_RET_FAIL;
    }

    return err;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_ioctl_get_bit_info
 *   IOCTL handler for JPE_IOC_GETBITS
 *   User uses this command to get the processed (encoded) buffer
 *
 * Parameters:
 *   file_data: file private data
 *   arg:  argument, a pointer of JpeBitstreamInfo_t from user-space
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e mdrv_jpe_drv_ioctl_get_bit_info(jpe_file_data *file_data, unsigned long arg)
{
    JpeBitstreamInfo_t bit_info;
    JPE_IOC_RET_STATUS_e ret = JPE_IOC_RET_SUCCESS;
    int err;

#if SCATTER_USED
    /* Get one ready */
    err = jpe_scatter_ready_buf_get(&file_data->dev_data->pdev->dev, &file_data->outbuf_scatter_head,
                                    &bit_info.nAddr, &bit_info.nOrigSize, &bit_info.nOutputSize, &bit_info.eState);
    if(err != 0)
    {
        return JPE_IOC_RET_BAD_OUTBUF;
    }
#else
    if (file_data->enc_outbuf.eState == JPE_FRAME_DONE_STATE || bit_info.nAddr || bit_info.nOutputSize)
    {
        bit_info.nAddr = file_data->enc_outbuf.addr;
        bit_info.nOrigSize = file_data->enc_outbuf.orig_size;
        bit_info.nOutputSize = file_data->enc_outbuf.output_size;
        bit_info.eState = file_data->enc_outbuf.eState;
    }
    else
    {
        JPE_MSG(JPE_MSG_ERR, "found no ready buffer\n");
        return JPE_IOC_RET_BAD_OUTBUF;
    }
#endif

    /* Copy to user-space */
    err = copy_to_user((void*)arg, &bit_info, sizeof(bit_info));
    if(err != 0)
    {
        ret = JPE_IOC_RET_FAIL;
    }

#if SCATTER_USED
    /* Change the status if all ready buffers are pumped out */
    if(!jpe_scatter_ready_buf_check(&file_data->dev_data->pdev->dev, &file_data->outbuf_scatter_head) && file_data->jpe_handle.ejpeDevStatus == JPE_DEV_ENC_DONE)
    {
        file_data->jpe_handle.ejpeDevStatus = JPE_DEV_INIT;
    }
#endif

    return ret;
}

/*******************************************************************************************************************
 * _mdrv_jpe_drv_ioctl_check_inbuf
 *   This function use get_user_pages & scatter to check whether the input buffer is virtual memory
 *   because HW cna't accept a fragmented input buffer.
 *
 * Parameters:
 *   file_data: file private data
 *   in_buf:    input buffer information
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e _mdrv_jpe_drv_ioctl_check_inbuf(jpe_file_data *file_data, JpeCfg_t *pJpeCfg)
{
#if SCATTER_USED
    struct jpe_scatter *scatter;
    int err = JPE_IOC_RET_SUCCESS;
#endif

#if SCATTER_USED
    if(pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr != 0 && pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize != 0)
    {
        scatter = jpe_scatter_create(&file_data->dev_data->pdev->dev, in_buf->nAddr, in_buf->nSize, DMA_TO_DEVICE);
        if(scatter == NULL || scatter->entries > 1)
        {
            /* create failed or a virtual-fragmented memory */
            JPE_MSG(JPE_MSG_ERR, "Address 0x%08lX (size:%ld) is fragment memory from malloc and JPE input doesn't support it\n", in_buf->nAddr, in_buf->nSize);
            err = JPE_IOC_RET_BAD_INBUF;
        }

        /* Free allocated scatter because this is for check only */
        if(scatter != NULL)
        jpe_scatter_release(&file_data->dev_data->pdev->dev, scatter);
    }
#endif
    return JPE_IOC_RET_SUCCESS;
}

/*******************************************************************************************************************
 * _mdrv_jpe_drv_ioctl_set_outbuf
 *   Private IOCTL handler for JPE_IOC_SET_OUTBUF
 *   User uses this command to set output buffer.
 *   This function would not handle the data of user-space
 *
 * Parameters:
 *   file_data: file private data
 *   out_buf:   output buffer information
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e _mdrv_jpe_drv_ioctl_set_outbuf(jpe_file_data *file_data, JpeBufInfo_t *out_buf)
{
#if SCATTER_USED
    struct jpe_scatter *scatter;
#endif
    jpe_dev_data *dev_data = file_data->dev_data;
    int err = JPE_IOC_RET_SUCCESS;

    JPE_MSG(JPE_MSG_DEBUG, "process output buffer\n");

    /* Is a legal buffer? */
    if(out_buf->nAddr == 0 || out_buf->nSize == 0)
    {
        JPE_MSG(JPE_MSG_DEBUG, "The buffer is not legal\n");
        err = JPE_IOC_RET_BAD_INBUF;
        goto RETURN;
    }

#if SCATTER_USED
    /* Process output buffer */
    scatter = jpe_scatter_create(&file_data->dev_data->pdev->dev, out_buf->nAddr, out_buf->nSize, DMA_FROM_DEVICE);
    if(scatter == NULL)
    {
        JPE_MSG(JPE_MSG_ERR, "can't create scatter for 0x%08lX (size:%ld)\n", out_buf->nAddr, out_buf->nSize );
        err = JPE_IOC_RET_BAD_OUTBUF;
        goto RETURN;
    }

    /* Add to list */
    jpe_scatter_list_add(&file_data->outbuf_scatter_head, scatter);

    /* Assign the output buffer info to HW if it is not assign and HW is idle */
    if(file_data->nOutBufAddr == 0 &&
       (file_data->jpe_handle.ejpeDevStatus == JPE_DEV_INIT ||
       file_data->jpe_handle.ejpeDevStatus == JPE_DEV_OUTBUF_FULL))
    {

        /* Replace & get a fragment from scatter */
        if(jpe_scatter_frag_get(&file_data->outbuf_scatter_head, &out_buf->nAddr, &out_buf->nSize) != 0)
        {
            JPE_MSG(JPE_MSG_DEBUG, "can't get fragment\n");
            jpe_scatter_list_release(&file_data->dev_data->pdev->dev, &file_data->outbuf_scatter_head);
            err =  JPE_IOC_RET_BAD_INBUF;
            goto RETURN;
        }
        JPE_MSG(JPE_MSG_DEBUG, "get fragment 0x%08lX, size %ld", out_buf->nAddr, out_buf->nSize);

        /* Set output buffer */
        file_data->jpeHalBuf.nOutBufAddr[0]   = out_buf->nAddr;
        file_data->jpeHalBuf.nOutBufSize[0]   = out_buf->nSize;
        file_data->jpeHalBuf.nOutBufSize[1]   = 0;
        file_data->jpeHalBuf.nOutBufAddr[1]   = 0;
        file_data->jpeHalBuf.nJpeOutBitOffset = file_data->jpe_handle.jpeCfg.nJpeOutBitOffset;

        /* Change the state */
        file_data->nOutBufAddr = out_buf->nAddr;
        if(file_data->jpe_handle.ejpeDevStatus == JPE_DEV_OUTBUF_FULL)
        {
            file_data->jpe_handle.ejpeDevStatus = JPE_DEV_BUSY;
        }
    }
    else
    {
        JPE_MSG(JPE_MSG_DEBUG, "no assignment to HW because addr is 0x%08lX or state is %d\n", file_data->nOutBufAddr, file_data->jpe_handle.ejpeDevStatus);
    }
#else
    file_data->enc_outbuf.addr = out_buf->nAddr;
    file_data->enc_outbuf.orig_size = out_buf->nSize;
    file_data->enc_outbuf.output_size = 0;
    file_data->enc_outbuf.eState = JPE_IDLE_STATE;
    JPE_MSG(JPE_MSG_DEBUG, "enc_outbuf, addr: 0x%08X, orig_size:%ld\n",
            (__u32)(file_data->enc_outbuf.addr), file_data->enc_outbuf.orig_size);
    /* Set output buffer */
    file_data->jpeHalBuf.nOutBufAddr[0]   = out_buf->nAddr;
    file_data->jpeHalBuf.nOutBufSize[0]   = out_buf->nSize;
    file_data->jpeHalBuf.nOutBufSize[1]   = 0;
    file_data->jpeHalBuf.nOutBufAddr[1]   = 0;
    file_data->jpeHalBuf.nJpeOutBitOffset = dev_data->jpe_handle.jpeCfg.nJpeOutBitOffset;
    JPE_MSG(JPE_MSG_DEBUG, "jpeHalBuf, nOutBufAddr[0]: 0x%08X, nOutBufSize[0]:%d\n",
            file_data->jpeHalBuf.nOutBufAddr[0], file_data->jpeHalBuf.nOutBufSize[0]);

#endif

RETURN:
    return err;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_ioctl_set_outbuf
 *   IOCTL handler for JPE_IOC_SET_OUTBUF
 *   This is a wrapper of _mdrv_jpe_drv_ioctl_check_inbuf
 *   to handler the data from user-space
 *
 * Parameters:
 *   file_data: file private data
 *   arg:       argument, a pointer of JpeBufInfo_t from user-space
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e mdrv_jpe_drv_ioctl_set_outbuf(jpe_file_data *file_data, unsigned long arg)
{
    JpeBufInfo_t out_buf;
    int err;

    if(copy_from_user(&out_buf, (void*)arg, sizeof(JpeBufInfo_t)) != 0)
    {
        JPE_MSG(JPE_MSG_DEBUG, "Can't allocate & copy data from user-space\n");
        return JPE_IOC_RET_BAD_INBUF;
    }

    err = _mdrv_jpe_drv_ioctl_set_outbuf(file_data, &out_buf);

    return err;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_ioctl_init
 *   IOCTL handler for JPE_IOC_INIT
 *   User uses this command to reset & init JPE encoder. User has to call this function again
 *   before starting to encode another frame.
 *
 * Parameters:
 *   file_data: file private data
 *   arg:       argument, a pointer of JpeCfg_t from user-space
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e mdrv_jpe_drv_ioctl_init(jpe_file_data *file_data, unsigned long arg)
{
    int err = JPE_IOC_RET_SUCCESS;
    JpeCfg_t jpe_cfg;
//    jpe_dev_data *dev_data = file_data->dev_data;
//  struct jpe_scatter *scatter;
//  unsigned long addr, size;
    JpeBufInfo_t out_buf;

    /* Copy configure from user-space */
    if(copy_from_user(&jpe_cfg, (void*)arg, sizeof(JpeCfg_t)) != 0)
    {
        JPE_MSG(JPE_MSG_DEBUG, "Can't allocate & copy data from user-space\n");
        err = JPE_IOC_RET_BAD_INBUF;
        goto RETURN;
    }

    /* Check input buffer */
    err = _mdrv_jpe_drv_ioctl_check_inbuf(file_data, &jpe_cfg);
    if(err != JPE_IOC_RET_SUCCESS)
    {
        goto RETURN;
    }

    /* Clear output buffer info because lower ioctl function would not process buffer directly. */
    out_buf.nAddr = jpe_cfg.OutBuf.nAddr;
    out_buf.nSize = jpe_cfg.OutBuf.nSize;
    jpe_cfg.OutBuf.nAddr = 0;
    jpe_cfg.OutBuf.nSize = 0;

    /* Run lower ioctl function */
//    err = DrvJpeInit(&dev_data->jpe_handle, &jpe_cfg);
    memcpy(&file_data->jpeCfg, &jpe_cfg, sizeof(JpeCfg_t));

    /* Process output buffer if assigned */
    if(err == JPE_IOC_RET_SUCCESS && out_buf.nAddr != 0 && out_buf.nSize != 0)
    {
        err = _mdrv_jpe_drv_ioctl_set_outbuf(file_data, &out_buf);
    }

RETURN:
//    devm_kfree(&file_data->dev_data->pdev->dev, jpe_cfg);
    return err;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_ioctl
 *   IOCTL handler entry for file operator
 *
 * Parameters:
 *   filp: pointer of file structure
 *   cmd:  command
 *   arg:  argument from user-space
 *
 * Return:
 *   standard return value
 */
long mdrv_jpe_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    jpe_file_data   *file_data = (jpe_file_data*)filp->private_data;
    jpe_dev_data    *dev_data  = file_data->dev_data;
    JPE_IOC_RET_STATUS_e err = JPE_IOC_RET_SUCCESS;

    // Should we do that?
    if(file_data == NULL || dev_data->ref_count <= 0)
    {
        JPE_MSG(JPE_MSG_DEBUG, "driver refCnt = %d!!! \n", dev_data->ref_count);
        return -EFAULT;
    }

#ifdef JPE_MSG_ENABLE
    // Show Command
    mdrv_jpe_drv_show_ioctl_cmd(filp, cmd, arg);
#endif

    switch(cmd)
    {
    case JPE_IOC_INIT:
        err = mdrv_jpe_drv_ioctl_init(file_data, arg);
        break;

    case JPE_IOC_ENCODE_FRAME:
        if (dev_data->cur_file)
        {
            JPE_MSG(JPE_MSG_DEBUG, "file(%p): into waiting queue for hw free\n", file_data);
            wait_event(dev_data->jpe_wqh, dev_data->cur_file == NULL);
            JPE_MSG(JPE_MSG_DEBUG, "file(%p): leave waiting queue\n", file_data);
        }
        /* Enter critical section */
//        spin_lock(&dev_data->lock);
        down(&dev_data->jpe_sem);
        dev_data->cur_file = file_data;
        /* Leave critical section */
//        spin_unlock(&dev_data->lock);
        up(&dev_data->jpe_sem);
        JPE_MSG(JPE_MSG_DEBUG, "[in]JPE_IOC_ENCODE_FRAME\n");
        /* Run lower ioctl function */
        err = DrvJpeInit(&dev_data->jpe_handle, &file_data->jpeCfg);
        err = DrvJpeEncodeOneFrame(&dev_data->jpe_handle, &file_data->jpeHalBuf);
        JPE_MSG(JPE_MSG_DEBUG, "[out]JPE_IOC_ENCODE_FRAME\n");
        break;

    case JPE_IOC_SET_OUTBUF:
        err = mdrv_jpe_drv_ioctl_set_outbuf(file_data, arg);
        break;

    case JPE_IOC_GETBITS:
        err = mdrv_jpe_drv_ioctl_get_bit_info(file_data, arg);
        break;

    case JPE_IOC_GET_CAPS:
        mdrv_jpe_drv_ioctl_get_caps(file_data, arg);
        break;

    default:
        err = JPE_IOC_RET_FAIL;
//      (int)DrvJpeIoctl(&file_data->jpe_handle, cmd, arg);
        break;
    }

//  JPE_MSG(JPE_MSG_DEBUG, "ioctl result = %d\n", err);

    return err;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_ioctl
 *   poll handler entry for file operator
 *
 * Parameters:
 *   filp: pointer of file structure
 *   wait: wait queue
 *
 * Return:
 *   only 0 or POLLIN | POLLRDNORM
 */
static unsigned int mdrv_jpe_drv_poll(struct file *filp, struct poll_table_struct *wait)
{
    jpe_file_data    *file_data = (jpe_file_data*)filp->private_data;
    jpe_dev_data     *dev_data  = file_data->dev_data;

    poll_wait(filp, &file_data->wait_queue, wait);

    switch(dev_data->jpe_handle.ejpeDevStatus)
    {
    case JPE_DEV_ENC_DONE:
    case JPE_DEV_OUTBUF_FULL:
        return POLLIN | POLLRDNORM;
    default:
        break;
    }
    return 0;
}


//-------------------------------------------------------------------------------------------------
// Platform functions
//-------------------------------------------------------------------------------------------------

// Use a struct to gather all global variable
static struct
{
    int major;              // cdev major number
    int minor_star;         // beginning of cdev minor number
    int reg_count;          // registered count
    struct class *class;    // class pointer
} g_jpe_drv = {0, 0, 0, NULL};

static const struct file_operations jpe_fops =
{
    .owner          = THIS_MODULE,
    .open           = mdrv_jpe_drv_open,
    .release        = mdrv_jpe_drv_release,
    .unlocked_ioctl = mdrv_jpe_drv_ioctl,
    .poll           = mdrv_jpe_drv_poll,
};

/*******************************************************************************************************************
 * mdrv_jpe_drv_probe
 *   Platform device probe handler
 *
 * Parameters:
 *   pdev: platform device
 *
 * Return:
 *   standard return value
 */
static int mdrv_jpe_drv_probe(struct platform_device *pdev)
{
    int err;
    jpe_dev_data *dev_data;
    struct resource *res;

    JPE_MSG(JPE_MSG_DEBUG, "mdrv_jpe_drv_probe\n");

    // create drv data buffer
    dev_data = devm_kcalloc(&pdev->dev, 1, sizeof(jpe_dev_data), GFP_KERNEL);
    if(dev_data == NULL)
    {
        JPE_MSG(JPE_MSG_ERR, "can't allocate buffer\n");
        return -ENOMEM;
    }

    /* Initialize spin lock */
//    spin_lock_init(&dev_data->lock);

    /* Initialize semaphore */
    sema_init(&dev_data->jpe_sem, MDRV_JPE_DEVICE_COUNT);

    /* Initialize wait queue */
    init_waitqueue_head(&dev_data->jpe_wqh);

    /* Keep platform device info */
    dev_data->pdev = pdev;

    /* Initialize current file pointer */
    dev_data->cur_file = NULL;

    /* memory-mapped IO registers */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    dev_data->iobase = (uint)res->start;
#if 0
    dev_data->regs = devm_ioremap_resource(&pdev->dev, res);
    if(IS_ERR(dev_data->regs))
    {
        JPE_MSG(JPE_MSG_ERR, "Cannot find register base address\n");
        return PTR_ERR(dev_data->regs);
    }
#endif

    /* Initialize jpe HW handle register */
    dev_data->jpe_handle.jpeHalHandle.nBaseAddr= dev_data->iobase;
    dev_data->jpe_handle.jpeHalHandle.pJpeReg = kzalloc(sizeof(JpeReg_t), GFP_KERNEL);

    /* IRQ registration */
    dev_data->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
    if(dev_data->irq < 0)
    {
        JPE_MSG(JPE_MSG_ERR, "Cannot find IRQ\n");
        return -ENODEV;
    }

    /* Add cdev */
    cdev_init(&dev_data->cdev, &jpe_fops);
    err= cdev_add(&dev_data->cdev, MKDEV(g_jpe_drv.major, g_jpe_drv.minor_star + g_jpe_drv.reg_count), 1);
    if(err)
    {
        JPE_MSG(JPE_MSG_ERR, "Unable add a character device\n");
        return err;
    }

    /* Create a instance in class */
    device_create(g_jpe_drv.class,
                  NULL,
                  MKDEV(g_jpe_drv.major, g_jpe_drv.minor_star + g_jpe_drv.reg_count),
                  dev_data,
                  MDRV_JPE_NAME"%d", g_jpe_drv.minor_star + g_jpe_drv.reg_count);

    /* Increase registered count */
    g_jpe_drv.reg_count++;

    dev_set_drvdata(&pdev->dev, dev_data);

    return 0;
}

/*******************************************************************************************************************
 * mdrv_jpe_drv_remove
 *   Platform device remove handler
 *
 * Parameters:
 *   pdev: platform device
 *
 * Return:
 *   standard return value
 */
static int mdrv_jpe_drv_remove(struct platform_device *pdev)
{
    jpe_dev_data *dev_data = dev_get_drvdata(&pdev->dev);

    JPE_MSG(JPE_MSG_DEBUG, "mdrv_jpe_drv_remove\n");

    kfree(dev_data->jpe_handle.jpeHalHandle.pJpeReg);
    device_destroy(g_jpe_drv.class, dev_data->cdev.dev);
    cdev_del(&dev_data->cdev);

    return 0;
}


/*******************************************************************************************************************
 * mdrv_jpe_drv_suspend
 *   Platform device suspend handler, but nothing to do here
 *
 * Parameters:
 *   pdev: platform device
 *
 * Return:
 *   standard return value
 */
static int mdrv_jpe_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
    jpe_dev_data *dev_data = dev_get_drvdata(&pdev->dev);

    JPE_MSG(JPE_MSG_DEBUG, "into suspend\n");

    down(&dev_data->jpe_sem);
//  if(dev_data->clk_ref_count > 0){
//      dev_data->clk_ref_count--;
//      mdrv_jpe_clock_off(dev_data);
//  }
    if(dev_data->hw_enable)
    {
        mdrv_jpe_clock_off(dev_data);
        dev_data->hw_enable = 0;
    }
    up(&dev_data->jpe_sem);

    return 0;
}


/*******************************************************************************************************************
 * mdrv_jpe_drv_resume
 *   Platform device resume handler, but nothing to do here
 *
 * Parameters:
 *   pdev: platform device
 *
 * Return:
 *   standard return value
 */
static int mdrv_jpe_drv_resume(struct platform_device *pdev)
{
    jpe_dev_data *dev_data = dev_get_drvdata(&pdev->dev);

    JPE_MSG(JPE_MSG_DEBUG, "into resume\n");

    down(&dev_data->jpe_sem);
//  if(dev_data->clk_ref_count == 0){
//      dev_data->clk_ref_count++;
//      mdrv_jpe_clock_on(dev_data);
//  }
    if (!dev_data->hw_enable)
    {
        mdrv_jpe_clock_on(dev_data);
        dev_data->hw_enable = 1;
    }
    up(&dev_data->jpe_sem);

    return 0;
}


//-------------------------------------------------------------------------------------------------
// Data structure for device driver
//-------------------------------------------------------------------------------------------------
static const struct of_device_id mdrv_jpeg_match[] =
{
    {
            .compatible = "mstar,cedric-jpe",
            /*.data = NULL,*/
    },
    {
            .compatible = "mstar,infinity-jpe",
            /*.data = NULL,*/
    },
    {},
};


static struct platform_driver mdrv_jpe_driver =
{
    .probe      = mdrv_jpe_drv_probe,
    .remove     = mdrv_jpe_drv_remove,
    .suspend    = mdrv_jpe_drv_suspend,
    .resume     = mdrv_jpe_drv_resume,

    .driver =
    {
        .of_match_table = of_match_ptr(mdrv_jpeg_match),
        .name   = "mstar_jpe",
        .owner  = THIS_MODULE,
    }
};

//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------

/*******************************************************************************************************************
 * _mdrv_jpe_module_init
 *   module init function
 *
 * Parameters:
 *   N/A
 *
 * Return:
 *   standard return value
 */
int mdrv_jpe_module_init(void)
{
    int err;
    dev_t  dev;

    JPE_MSG(JPE_MSG_DEBUG, "Module Init\n");

    /* Allocate cdev id */
    err = alloc_chrdev_region(&dev, MDRV_JPE_MINOR, MDRV_JPE_DEVICE_COUNT, MDRV_JPE_NAME);
    if(err)
    {
        JPE_MSG(JPE_MSG_ERR, "Unable allocate cdev id\n");
        return err;
    }

    g_jpe_drv.major = MAJOR(dev);
    g_jpe_drv.minor_star = MINOR(dev);
    g_jpe_drv.reg_count = 0;

    /* Register device class */
//  g_jpe_drv.class = class_create(THIS_MODULE, MDRV_JPE_CLASS_NAME);
    g_jpe_drv.class = msys_get_sysfs_class();
    if(IS_ERR(g_jpe_drv.class))
    {
        JPE_MSG(JPE_MSG_ERR, "Failed at class_create().Please exec [mknod] before operate the device\n");
        err = PTR_ERR(g_jpe_drv.class);
        goto ERR_RETURN;
    }

    /* Register platform driver */
    err = platform_driver_register(&mdrv_jpe_driver);
    if(err == 0)
    {
        return 0;
    }

//  class_destroy(g_jpe_drv.class);

ERR_RETURN:
    unregister_chrdev_region(MKDEV(g_jpe_drv.major, g_jpe_drv.minor_star), MDRV_JPE_DEVICE_COUNT);

    return err;
}

/*******************************************************************************************************************
 * mdrv_jpe_module_exit
 *   module exit function
 *
 * Parameters:
 *   N/A
 *
 * Return:
 *   standard return value
 */
void mdrv_jpe_module_exit(void)
{
    /* de-initial the who GFLIPDriver */
    JPE_MSG(JPE_MSG_ERR, "Module Exit\n");

//  class_destroy(g_jpe_drv.class);
    unregister_chrdev_region(MKDEV(g_jpe_drv.major, g_jpe_drv.minor_star), MDRV_JPE_DEVICE_COUNT);
    platform_driver_unregister(&mdrv_jpe_driver);
}

module_init(mdrv_jpe_module_init);
module_exit(mdrv_jpe_module_exit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("JPE ioctrl driver");
MODULE_LICENSE("GPL");

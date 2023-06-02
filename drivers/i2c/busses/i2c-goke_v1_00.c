/* linux/drivers/i2c/busses/i2c-gk.c
 *
 * History:
 *  2014/07/30 - [Kewell Liu] created file
 *
 * Copyright (C) 2002-2014, Goke Microelectronics China.
 *
 * GK I2C Controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_i2c.h>
#include <linux/of_gpio.h>

#include <asm/irq.h>

#include <mach/idc.h>
#include <mach/io.h>

#ifndef CONFIG_I2C_GK_RETRIES
#define CONFIG_I2C_GK_RETRIES        (3)
#endif

#ifndef CONFIG_I2C_GK_ACK_TIMEOUT
#define CONFIG_I2C_GK_ACK_TIMEOUT    (1 * HZ)
#endif

#define CONFIG_I2C_GK_BULK_RETRY_NUM (4)

/* i2c controller state */
enum gk_i2c_state {
    I2C_STATE_IDLE,
    I2C_STATE_START,
    I2C_STATE_START_TEN,
    I2C_STATE_START_NEW,
    I2C_STATE_READ,
    I2C_STATE_READ_STOP,
    I2C_STATE_WRITE,
    I2C_STATE_WRITE_WAIT_ACK,
    I2C_STATE_BULK_WRITE,
    I2C_STATE_NO_ACK,
    I2C_STATE_ERROR
};

struct gk_i2c {
    spinlock_t      lock;
    unsigned int        suspended;

    struct i2c_msg      *msg;
    unsigned int        msg_num;
    unsigned int        msg_idx;
    unsigned int        msg_ptr;
    unsigned int        msg_addr;
    wait_queue_head_t   msg_wait;

    unsigned int        irq;

    enum gk_i2c_state  state;

    void __iomem        *regbase;
    struct device       *dev;
    struct resource     *ioarea;
    struct i2c_adapter  adap;

    struct gk_platform_i2c *pdata;

};

static inline void gk_i2c_start_single_msg(struct gk_i2c *i2c)
{
    if (i2c->msg->flags & I2C_M_TEN)
    {
        i2c->state = I2C_STATE_START_TEN;
        gk_i2c_writel((unsigned int)(i2c->regbase + IDC_DATA_OFFSET),
                    (0xf0 | ((i2c->msg_addr >> 8) & 0x07)));
    }
    else
    {
        i2c->state = I2C_STATE_START;
        gk_i2c_writel((unsigned int)(i2c->regbase + IDC_DATA_OFFSET),
                    (i2c->msg_addr & 0xff));
    }

    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_CTRL_OFFSET),
                    IDC_CTRL_START);
}

static inline void gk_i2c_bulk_write(struct gk_i2c *i2c, u32 fifosize)
{
    do
    {
        gk_i2c_writel((unsigned int)(i2c->regbase + IDC_FMDATA_OFFSET),
                                    i2c->msg->buf[i2c->msg_idx]);
        i2c->msg_idx++;
        fifosize--;

        if (i2c->msg_idx >= i2c->msg->len)
        {
            gk_i2c_writel((unsigned int)(i2c->regbase + IDC_FMCTRL_OFFSET),
                                    (IDC_FMCTRL_IF | IDC_FMCTRL_STOP));
            return;
        }
    } while (fifosize > 1);

    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_FMCTRL_OFFSET),
                                IDC_FMCTRL_IF);
}

static inline void gk_i2c_start_bulk_msg_write(struct gk_i2c *i2c)
{
    u32 fifosize = IDC_FIFO_BUF_SIZE;

    i2c->state = I2C_STATE_BULK_WRITE;

    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_FMCTRL_OFFSET),
                                IDC_FMCTRL_START);

    if (i2c->msg->flags & I2C_M_TEN) {
        gk_i2c_writel((unsigned int)(i2c->regbase + IDC_FMDATA_OFFSET),
                                (0xf0 | ((i2c->msg_addr >> 8) & 0x07)));
        fifosize--;
    }
    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_FMDATA_OFFSET),
                                (i2c->msg_addr & 0xff));
    fifosize -= 2;

    gk_i2c_bulk_write(i2c, fifosize);
}

/* gk_i2c_message_start
 *
 * put the start of a message onto the bus
 */
static void gk_i2c_message_start(struct gk_i2c *i2c)
{
    struct i2c_msg *msg = i2c->msg;
    i2c->msg_idx = 0;

    i2c->msg_addr = ((msg->addr & 0x7f) << 1);

    if (msg->flags & I2C_M_RD)
        i2c->msg_addr |= 1;

    if (msg->flags & I2C_M_REV_DIR_ADDR)
        i2c->msg_addr ^= 1;

    if (msg->flags & I2C_M_RD) {
        gk_i2c_start_single_msg(i2c);
    } else if (msg->len > i2c->pdata->bulk_write_num) {
        gk_i2c_start_bulk_msg_write(i2c);
    } else {
        gk_i2c_start_single_msg(i2c);
    }

    dev_dbg(i2c->dev, "msg_addr[0x%x], len[0x%x]", i2c->msg_addr, i2c->msg->len);

}

static inline void gk_i2c_stop(struct gk_i2c *i2c, enum gk_i2c_state state, u32 *pack)
{
    if(state != I2C_STATE_IDLE) {
        //dev_warn(i2c->dev, "gk_i2c_stop[%d] from %d to %d\n", i2c->msg_num, i2c->state, state);
        *pack |= IDC_CTRL_ACK;
    }

    i2c->state      = state;
    i2c->msg        = NULL;
    i2c->msg_num    = 0;

    *pack |= IDC_CTRL_STOP;
    // StevenYu:Do not check state.
    //if(i2c->state == I2C_STATE_IDLE)
        wake_up(&i2c->msg_wait);
}

static inline u32 gk_i2c_check_ack(struct gk_i2c *i2c, u32 *pack, u32 retry)
{
    u32 retVal = IDC_CTRL_ACK;
    //printk("gk_i2c_check_ack...\n");

gk_i2c_check_ack_enter:
    if (((*pack) & IDC_CTRL_ACK)==0)
    //if (unlikely((*pack) & IDC_CTRL_ACK))
    {
    	//printk("pack: 0x%x\n", *pack);
        if (i2c->msg->flags & I2C_M_IGNORE_NAK)
            goto gk_i2c_check_ack_exit;

        if ((i2c->msg->flags & I2C_M_RD) &&
            (i2c->msg->flags & I2C_M_NO_RD_ACK))
            goto gk_i2c_check_ack_exit;

        if (retry--) {
            udelay(100);
            *pack = gk_i2c_readl((unsigned int)(i2c->regbase + IDC_CTRL_OFFSET));
        //printk("pack = 0x%x \n", *pack);
            goto gk_i2c_check_ack_enter;
        }
        retVal = 0;
        gk_i2c_stop(i2c, I2C_STATE_NO_ACK, pack);
    }

gk_i2c_check_ack_exit:
    return retVal;

}

/* gk_i2c_set_clk
 *
 * work out a divisor for the user requested frequency setting,
 * either by the requested frequency, or scanning the acceptable
 * range of frequencies until something is found
*/
static void gk_i2c_set_clk(struct gk_i2c *i2c)
{
    unsigned int    apb_clk;
    unsigned int    idc_prescale;

    apb_clk = i2c->pdata->get_clock();
    //apb_clk = 30000000;

    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_ENR_OFFSET),
                                IDC_ENR_DISABLE);

    if ((i2c->pdata->clk_limit < 1000) ||
        (i2c->pdata->clk_limit > 400000))
    {
        i2c->pdata->clk_limit = 100000;
    }

//    prescale = ((GD_GET_APB_ClkHz()/(U32)openParamsP->speed - 2) >> 2) - 1;

    idc_prescale = ((apb_clk / i2c->pdata->clk_limit -2) >> 2) - 1;

    //dev_dbg(i2c->dev, "apb_clk[%dHz]\n", apb_clk);
    //dev_dbg(i2c->dev, "idc_prescale[%d]\n", idc_prescale);
    //dev_dbg(i2c->dev, "clk[%dHz]\n", i2c->pdata->clk_limit);

    //dev_dbg(i2c->dev, "regbase 0x%08x\n", i2c->regbase);

    //gk_writel(i2c->regbase + IDC_PSLL_OFFSET,   (idc_prescale & 0xff));
    //gk_writel(i2c->regbase + IDC_PSLH_OFFSET,   ((idc_prescale & 0xff00) >> 8));
	gk_i2c_writel((unsigned int)(i2c->regbase + IDC_PSLL_OFFSET),
	                            (idc_prescale & 0xffff));
    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_ENR_OFFSET),
                                (IDC_ENR_ENABLE));

    //dev_info(i2c->dev, "IDC_PSLL_OFFSET 0x%08x\n", gk_readb(i2c->regbase + IDC_PSLL_OFFSET));
		//dev_info(i2c->dev, "IDC_PSLH_OFFSET 0x%08x\n", gk_readb(i2c->regbase + IDC_PSLH_OFFSET));
		//dev_info(i2c->dev, "IDC_ENR_OFFSET 0x%08x\n", gk_readb(i2c->regbase + IDC_ENR_OFFSET));

}

static inline void gk_i2c_hw_off(struct gk_i2c *i2c)
{
    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_ENR_OFFSET),
                                IDC_ENR_DISABLE);
    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_PSLL_OFFSET), 0);
    //gk_writeb(i2c->regbase + IDC_PSLH_OFFSET, 0);
}

/* gk_i2c_init
 *
 * initialise the controller, set the IO lines and frequency
*/
static int gk_i2c_init(struct gk_i2c *i2c)
{
    gk_i2c_set_clk(i2c);

    i2c->msg      = NULL;
    i2c->msg_num  = 0;
    i2c->state    = I2C_STATE_IDLE;

    return 0;
}

static irqreturn_t gk_i2c_irq(int irqno, void *dev_id)
{
    struct platform_device  *pdev;
    struct gk_i2c       *i2c;

    u32     status_reg;
    u32     control_reg;
    u32     ack_control = IDC_CTRL_CLS;

    i2c     = (struct gk_i2c *)dev_id;
    pdev    = (struct platform_device *)i2c->dev->platform_data;

    status_reg = gk_i2c_readl((unsigned int)(i2c->regbase + IDC_STS_OFFSET));
    control_reg = gk_i2c_readl((unsigned int)(i2c->regbase + IDC_CTRL_OFFSET));

    //dev_dbg(i2c->dev, "i2c[%d]:state[0x%x]\n", pdev->id, i2c->state);
    //dev_dbg(i2c->dev, "i2c[%d]:status_reg[0x%x]\n", pdev->id, status_reg);
    //dev_dbg(i2c->dev, "i2c[%d]:control_reg[0x%x]\n", pdev->id, control_reg);

    switch (i2c->state)
    {
        case I2C_STATE_START:
            if (gk_i2c_check_ack(i2c, &control_reg, 0) == IDC_CTRL_ACK)
            {
                if (i2c->msg->flags & I2C_M_RD)
                {
                    if (i2c->msg->len == 1)
                        ack_control |= IDC_CTRL_ACK;
                    i2c->state = I2C_STATE_READ;
                }
                else
                {
                    i2c->state = I2C_STATE_WRITE;
                    goto i2c_irq_write;
                }
            }
            break;
        case I2C_STATE_START_TEN:
            i2c->state = I2C_STATE_START;
            gk_i2c_writel((unsigned int)(i2c->regbase + IDC_DATA_OFFSET),
                                        (i2c->msg_addr & 0xff));
            break;
        case I2C_STATE_START_NEW:
i2c_irq_start_new:
            gk_i2c_message_start(i2c);
            goto i2c_irq_exit;
            break;
        case I2C_STATE_READ_STOP:
            i2c->msg->buf[i2c->msg_idx] =
                gk_i2c_readl((unsigned int)(i2c->regbase + IDC_DATA_OFFSET));
            i2c->msg_idx++;
i2c_irq_read_stop:
            gk_i2c_stop(i2c, I2C_STATE_IDLE, &ack_control);
            break;
        case I2C_STATE_READ:
            i2c->msg->buf[i2c->msg_idx] =
                gk_i2c_readl((unsigned int)(i2c->regbase + IDC_DATA_OFFSET));
            i2c->msg_idx++;

            if (i2c->msg_idx >= i2c->msg->len - 1)
            {
                if (i2c->msg_num > 1)
                {
                    i2c->msg++;
                    i2c->state = I2C_STATE_START_NEW;
                    i2c->msg_num--;
                }
                else
                {
                    if (i2c->msg_idx > i2c->msg->len - 1)
                    {
                        goto i2c_irq_read_stop;
                    }
                    else
                    {
                        i2c->state = I2C_STATE_READ_STOP;
                        ack_control |= IDC_CTRL_ACK;
                    }
                }
            }
            //ack_control |= IDC_CTRL_ACK;
            break;
        case I2C_STATE_WRITE:
i2c_irq_write:
            i2c->state = I2C_STATE_WRITE_WAIT_ACK;
            gk_i2c_writel((unsigned int)(i2c->regbase + IDC_DATA_OFFSET),
                                        i2c->msg->buf[i2c->msg_idx]);
            break;
        case I2C_STATE_WRITE_WAIT_ACK:
            if (gk_i2c_check_ack(i2c, &control_reg, 0) == IDC_CTRL_ACK)
            {
                i2c->state = I2C_STATE_WRITE;
                i2c->msg_idx++;

                if (i2c->msg_idx >= i2c->msg->len)
                {
                    if (i2c->msg_num > 1)
                    {
                        i2c->msg++;
                        i2c->state = I2C_STATE_START_NEW;
                        i2c->msg_num--;
                        goto i2c_irq_start_new;
                    }
                    gk_i2c_stop(i2c, I2C_STATE_IDLE, &ack_control);
                }
                else
                {
                    goto i2c_irq_write;
                }
            }
            break;
        case I2C_STATE_BULK_WRITE:
            if (gk_i2c_check_ack(i2c, &control_reg, CONFIG_I2C_GK_BULK_RETRY_NUM) == IDC_CTRL_ACK)
            {
                gk_i2c_writel((unsigned int)(i2c->regbase + IDC_CTRL_OFFSET),
                                            IDC_CTRL_ACK);
                if (i2c->msg_idx >= i2c->msg->len)
                {
                    if (i2c->msg_num > 1)
                    {
                        i2c->msg++;
                        i2c->state = I2C_STATE_START_NEW;
                        i2c->msg_num--;
                        goto i2c_irq_start_new;
                    }
                    gk_i2c_stop(i2c, I2C_STATE_IDLE, &ack_control);
                }
                else
                {
                    gk_i2c_bulk_write(i2c, IDC_FIFO_BUF_SIZE);
                }
                goto i2c_irq_exit;
            }
            else
            {
                ack_control = IDC_CTRL_ACK;
            }
            break;
        default:
            dev_err(i2c->dev, "i2c[%d]:gk_i2c_irq in wrong state[0x%x]\n", pdev->id, i2c->state);
            //dev_err(i2c->dev, "i2c[%d]:status_reg[0x%x]\n", pdev->id, status_reg);
            //dev_err(i2c->dev, "i2c[%d]:control_reg[0x%x]\n", pdev->id, control_reg);
            ack_control = IDC_CTRL_STOP | IDC_CTRL_ACK;
            i2c->state = I2C_STATE_ERROR;
            break;
    }

    gk_i2c_writel((unsigned int)(i2c->regbase + IDC_CTRL_OFFSET), ack_control);

i2c_irq_exit:
    return IRQ_HANDLED;

}

/* gk_i2c_doxfer
 *
 * this starts an i2c transfer
 */
static int gk_i2c_doxfer(struct gk_i2c *i2c, struct i2c_msg *msgs, int num)
{
    unsigned long timeout;
    int ret;

    if (i2c->suspended) return -EIO;

    spin_lock_irq(&i2c->lock);

    i2c->msg     = msgs;
    i2c->msg_num = num;
    i2c->msg_ptr = 0;
    i2c->msg_idx = 0;
    i2c->state   = I2C_STATE_START;

    gk_i2c_message_start(i2c);
    spin_unlock_irq(&i2c->lock);

    // StevenYu:delete the delay.
    //mdelay(1);
    timeout = wait_event_timeout(i2c->msg_wait, i2c->msg_num == 0, CONFIG_I2C_GK_ACK_TIMEOUT);

    ret = i2c->msg_idx;

    /* having these next two as dev_err() makes life very
     * noisy when doing an i2cdetect */

    if (timeout == 0)
        dev_dbg(i2c->dev, "timeout\n");
    else if (ret != num)
        dev_dbg(i2c->dev, "incomplete xfer (%d)\n", ret);

    /* ensure the stop has been through the bus */
    dev_dbg(i2c->dev, "%ld jiffies left.\n", timeout);

    if (i2c->state != I2C_STATE_IDLE)
    {
        dev_err(i2c->dev,"I2C state 0x%d, please check address 0x%x!\n", i2c->state, i2c->msg_addr);
        ret = -EAGAIN;
    }

    return ret;

}

static int gk_i2c_doxfer_noevent(struct gk_i2c *i2c, struct i2c_msg *msgs, int num)
{
    unsigned long timeout;
    int ret;
    unsigned long                 flags;    

    if (i2c->suspended) return -EIO;

    spin_lock_irq(&i2c->lock);
    i2c->msg     = msgs;
    i2c->msg_num = num;
    i2c->msg_ptr = 0;
    i2c->msg_idx = 0;
    i2c->state   = I2C_STATE_START;
    gk_i2c_message_start(i2c);
    spin_unlock_irq(&i2c->lock);

    while(i2c->state != I2C_STATE_IDLE)
    {
        udelay(10);
    }

    ret = i2c->msg_idx;

    /* having these next two as dev_err() makes life very
     * noisy when doing an i2cdetect */

    if (timeout == 0)
        dev_dbg(i2c->dev, "timeout\n");
    else if (ret != num)
        dev_dbg(i2c->dev, "incomplete xfer (%d)\n", ret);

    /* ensure the stop has been through the bus */
    dev_dbg(i2c->dev, "%ld jiffies left.\n", timeout);

    if (i2c->state != I2C_STATE_IDLE)
    {
        dev_err(i2c->dev,"I2C state 0x%d, please check address 0x%x!\n", i2c->state, i2c->msg_addr);
        ret = -EAGAIN;
    }

    return ret;
}

/* gk_i2c_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
 */

static int gk_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct gk_i2c *i2c = (struct gk_i2c *)adap->algo_data;
    int retry;
    int ret;

    //pm_runtime_get_sync(&adap->dev);

    for (retry = 0; retry < adap->retries; retry++)
    {
        if (i2c->state != I2C_STATE_IDLE)
        {
            gk_i2c_init(i2c);
        }

        ret = gk_i2c_doxfer(i2c, msgs, num);
				//printk("do xfer ret: %d \n", ret);
        if (ret != -EAGAIN)
        {
            //pm_runtime_put_sync(&adap->dev);
            return num;//ret;
        }

        dev_dbg(i2c->dev, "Retrying transmission (%d)\n", retry);

        udelay(100);
    }

    //pm_runtime_put_sync(&adap->dev);
    return -EREMOTEIO;

}


static int gk_i2c_xfer_noevent(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct gk_i2c *i2c = (struct gk_i2c *)adap->algo_data;
    int retry;
    int ret;

    //pm_runtime_get_sync(&adap->dev);

    for (retry = 0; retry < adap->retries; retry++)
    {
        if (i2c->state != I2C_STATE_IDLE)
        {
            gk_i2c_init(i2c);
        }

        ret = gk_i2c_doxfer_noevent(i2c, msgs, num);
				//printk("do xfer ret: %d \n", ret);
        if (ret != -EAGAIN)
        {
            //pm_runtime_put_sync(&adap->dev);
            return num;//ret;
        }

        dev_dbg(i2c->dev, "Retrying transmission (%d)\n", retry);

        udelay(100);
    }

    //pm_runtime_put_sync(&adap->dev);
    return -EREMOTEIO;

}
/* declare our i2c functionality */
static u32 gk_i2c_func(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_10BIT_ADDR;
}

/* i2c bus registration info */

static const struct i2c_algorithm gk_i2c_algorithm = {
    .master_xfer        = gk_i2c_xfer,
    .master_xfer_noevent = gk_i2c_xfer_noevent,
    .functionality      = gk_i2c_func,
};

/* gk_i2c_probe
 *
 * called by the bus driver when a suitable device is found
 */

static int gk_i2c_probe(struct platform_device *pdev)
{
    struct gk_i2c           *i2c;
    struct gk_platform_i2c  *pdata = NULL;
    struct resource             *res;

    int ret;

    pdata = (struct gk_platform_i2c *)pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "i2c[%d]: Can't get platform_data!\n", pdev->id);
        return -EINVAL;
    }

    i2c = devm_kzalloc(&pdev->dev, sizeof(struct gk_i2c), GFP_KERNEL);
    if (!i2c) {
        dev_err(&pdev->dev, "no memory for i2c[%d]\n", pdev->id);
        return -ENOMEM;
    }

    /* setup info block for the i2c core */
    strlcpy(i2c->adap.name, pdev->name, sizeof(i2c->adap.name));
    i2c->adap.owner         = THIS_MODULE;
    i2c->adap.algo          = &gk_i2c_algorithm;
    i2c->adap.retries       = CONFIG_I2C_GK_RETRIES;
    i2c->adap.class         = (I2C_CLASS_HWMON | I2C_CLASS_SPD);
    i2c->adap.nr            = pdev->id;
    i2c->adap.algo_data     = i2c;
    i2c->adap.dev.parent    = &pdev->dev;

    i2c->pdata = pdata;

    spin_lock_init(&i2c->lock);
    init_waitqueue_head(&i2c->msg_wait);

    /* find the clock and enable it */
    i2c->dev = &pdev->dev;

    /* map the registers */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL) {
        dev_err(&pdev->dev, "Get I2C[%d] mem resource failed!\n", pdev->id);
        ret = -ENOENT;
        goto err_noclk;
    }
#if 0

    i2c->ioarea = request_mem_region(res->start, resource_size(res), pdev->name);
    if (i2c->ioarea == NULL) {
        dev_err(&pdev->dev, "Request I2C[%d] ioarea failed!\n", pdev->id);
        ret = -ENXIO;
        goto err_noclk;
    }


    i2c->regbase = ioremap(res->start, resource_size(res));
    if (i2c->regbase == NULL) {
        dev_err(&pdev->dev, "i2c[%d] cannot map IO\n", pdev->id);
        ret = -ENXIO;
        goto err_ioarea;
    }
	dev_err(&pdev->dev, "i2c[%d]:registers %p (%p, %p)\n", pdev->id, i2c->regbase, i2c->ioarea, res);
#else
	i2c->regbase = (void __iomem*)res->start;
	printk("i2c regbase: 0x%x \n", (u32)i2c->regbase);
#endif

    /* initialise the i2c controller */
    ret = gk_i2c_init(i2c);
    if (ret != 0)
        goto err_iomap;

    /* find the IRQ for this unit (note, this relies on the init call to
        * ensure no current IRQs pending
        */
    i2c->irq = ret = platform_get_irq(pdev, 0);
    if (ret <= 0) {
        dev_err(&pdev->dev, "Get I2C[%d] irq resource failed!\n", pdev->id);
        goto err_iomap;
    }

	dev_info(&pdev->dev, "i2c irq:registers %d\n", i2c->irq);
    ret = request_irq(i2c->irq, gk_i2c_irq, IRQF_TRIGGER_HIGH, dev_name(&pdev->dev), i2c);
    if (ret != 0) {
        dev_err(&pdev->dev, "Request i2c[%d] IRQ=%d failed!\n", pdev->id, i2c->irq);
        goto err_iomap;
    }

    i2c_set_adapdata(&i2c->adap, i2c);
    /* Note, previous versions of the driver used i2c_add_adapter()
     * to add the bus at any number. We now pass the bus number via
     * the platform data, so if unset it will now default to always
     * being bus 0.
     */
    ret = i2c_add_numbered_adapter(&i2c->adap);
    if (ret < 0) {
        dev_err(&pdev->dev, "i2c[%d] Failed to add bus to i2c core\n", pdev->id);
        goto err_irq;
    }

    platform_set_drvdata(pdev, i2c);

    //pm_runtime_enable(&pdev->dev);
    //pm_runtime_enable(&i2c->adap.dev);

    dev_info(&pdev->dev, "GK I2C[%d] adapter[%s] probed!\n", pdev->id, dev_name(&i2c->adap.dev));
    return 0;

 err_irq:
    free_irq(i2c->irq, i2c);

 err_iomap:
//    iounmap(i2c->regbase);

 //err_ioarea:
    //release_mem_region(i2c->ioarea->start, (i2c->ioarea->end - i2c->ioarea->start) + 1);
    kfree(i2c);

 err_noclk:
    return ret;

}

/* gk_i2c_remove
 *
 * called when device is removed from the bus
*/

static int gk_i2c_remove(struct platform_device *pdev)
{
    struct gk_i2c *i2c = platform_get_drvdata(pdev);

    if(i2c)
    {
        gk_i2c_hw_off(i2c);

        //pm_runtime_disable(&i2c->adap.dev);
        //pm_runtime_disable(&pdev->dev);

        i2c_del_adapter(&i2c->adap);
        free_irq(i2c->irq, i2c);
#if 0
        iounmap(i2c->regbase);

        release_mem_region(i2c->ioarea->start, (i2c->ioarea->end - i2c->ioarea->start) + 1);
#endif
        kfree(i2c);
    }

    return 0;

}

#ifdef CONFIG_PM
static int gk_i2c_suspend_noirq(struct device *dev)
{
    struct platform_device *pdev    = to_platform_device(dev);
    struct gk_i2c *i2c          = platform_get_drvdata(pdev);

    if(i2c){
        i2c->suspended = 1;
        disable_irq(i2c->irq);
    }

    dev_dbg(&pdev->dev, "%s\n", __func__);

    return 0;
}

static int gk_i2c_resume(struct device *dev)
{
    struct platform_device *pdev    = to_platform_device(dev);
    struct gk_i2c *i2c          = platform_get_drvdata(pdev);

    if(i2c){
        i2c->suspended = 0;
        gk_i2c_init(i2c);
    }

    dev_dbg(&pdev->dev, "%s\n", __func__);

    return 0;
}

static const struct dev_pm_ops gk_i2c_dev_pm_ops = {
    .suspend_noirq  = gk_i2c_suspend_noirq,
    .resume         = gk_i2c_resume,
};

#define GK_I2C_DEV_PM_OPS   (&gk_i2c_dev_pm_ops)
#else
#define GK_I2C_DEV_PM_OPS   NULL
#endif

/* device driver for platform bus bits */
static struct platform_driver gk_i2c_driver = {
    .probe      = gk_i2c_probe,
    .remove     = gk_i2c_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = "i2c",
        .pm     = GK_I2C_DEV_PM_OPS,
    },
};

static int __init gk_i2c_adap_init(void)
{
    return platform_driver_register(&gk_i2c_driver);
}
subsys_initcall(gk_i2c_adap_init);

static void __exit gk_i2c_adap_exit(void)
{
    platform_driver_unregister(&gk_i2c_driver);
}
module_exit(gk_i2c_adap_exit);

MODULE_DESCRIPTION("GK I2C Bus driver");
MODULE_AUTHOR("Kewell Liu, <liujingke@gokemicro.com>");
MODULE_LICENSE("GPL");


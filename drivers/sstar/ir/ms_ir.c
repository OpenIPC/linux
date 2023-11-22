/*
 * ms_ir.c- Sigmastar
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

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/freezer.h>
#include <media/rc-core.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "ms_platform.h"
#include "IR_CONFIG.h"
#include "reg_ir.h"
#include "ms_ir.h"
#ifdef CONFIG_CAM_CLK
#include "drv_camclk_Api.h"
#endif
#define IR_DBG 0
#if IR_DBG
#define ir_dbg(args...)  printk(args)
#define ir_err(args...)  printk(args)
#define ir_info(args...) printk(args)
#else
#define ir_dbg(args...)
#define ir_err(args...)  printk(args)
#define ir_info(args...) // printk(args)
#endif

#define IR_MODE_SEL IR_MODE_RAWDATA

/**************************************************************
 *struct
 *************************************************************/
struct sstar_ir_dev
{
    int               irq;
    int               rc_irq;
    int               protocol;
    void __iomem *    membase;
    struct rc_dev *   dev;
    struct completion key_done;

    int                      workrun;
    struct work_struct       key_dispatch_wk;
    struct workqueue_struct *key_dispatch_wq;
#ifdef CONFIG_CAM_CLK
    void *pvIrClk;
#else
    struct clk *clk;
#endif
};

static struct sstar_ir_dev *gIr_Dev;

#define READ_WORD(bank)       (*(volatile u16 *)(gIr_Dev->membase + ((bank) << 2)))
#define WRITE_WORD(bank, val) (*(volatile u16 *)(gIr_Dev->membase + ((bank) << 2))) = (u16)(val)

//#define USE_POLLING_MODE
#ifdef USE_POLLING_MODE
struct work_struct       key_polling_wk;
struct workqueue_struct *key_polling_wq;
#endif
/**************************************************************
 *function
 *************************************************************/

static struct Key_Queue queue;
static DEFINE_SEMAPHORE(queue_lock);
static void _key_enqueue(u32 data)
{
    if (down_trylock(&queue_lock))
        return;

    if (queue.front == ((queue.rear + 1) % MaxQueue))
    {
        ir_dbg("queue is full \n");
    }
    else
    {
        queue.item[queue.rear] = data;
        queue.rear             = (queue.rear + 1) % MaxQueue;
    }

    up(&queue_lock);
}

static u32 _key_dequeue(void)
{
    u32 data = 0xFFFF;

    down(&queue_lock);

    if (queue.front == queue.rear)
    {
        ir_dbg("queue is empty \n");
    }
    else
    {
        data        = queue.item[queue.front];
        queue.front = (queue.front + 1) % MaxQueue;
    }

    up(&queue_lock);
    return data;
}
unsigned long _mdrv_get_sys_time(void)
{
    return ((unsigned long)((jiffies) * (1000 / HZ)));
}
#if (IR_MODE_SEL != RC5_MODE)
static U8 _mdrv_read_fifo(void)
{
    U8  keyVal = 0;
    U16 regVal = 0;

    keyVal = READ_WORD(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
    regVal = READ_WORD(REG_IR_FIFO_RD_PULSE) | 0x01;
    WRITE_WORD(REG_IR_FIFO_RD_PULSE, regVal);

    return keyVal;
}
static void _mdrv_clear_fifo(void)
{
    U16 regVal = 0;

    regVal = READ_WORD(REG_IR_SEPR_BIT_FIFO_CTRL);
    regVal |= 0x01 << 15;
    WRITE_WORD(REG_IR_SEPR_BIT_FIFO_CTRL, regVal);
}
#endif
#if (IR_MODE_SEL == IR_MODE_SWDECODE)
static void _mdrv_set_sw_decode(int bEnable)
{
    U16 regVal = 0;

    regVal = READ_WORD(REG_IR_SEPR_BIT_FIFO_CTRL);
    if (bEnable)
    {
        regVal |= 0x01 << 14;
        regVal |= 0x01 << 6;
    }
    else
    {
        regVal &= ~(0x01 << 14);
        regVal &= ~(0x01 << 6);
    }
    WRITE_WORD(REG_IR_SEPR_BIT_FIFO_CTRL, regVal);
}
#endif

static void _mdrv_set_timing(void)
{
    switch (IR_MODE_SEL)
    {
        case IR_MODE_FULLDECODE:
        case IR_MODE_RAWDATA:
        case IR_MODE_SWDECODE:
        case IR_MODE_SWDECODE_KON:
            // header code upper/lower bound
            WRITE_WORD(REG_IR_HDC_UPB, IR_HDC_UPB);
            WRITE_WORD(REG_IR_HDC_LOB, IR_HDC_LOB);

            // off code upper/lower bound
            WRITE_WORD(REG_IR_OFC_UPB, IR_OFC_UPB);
            WRITE_WORD(REG_IR_OFC_LOB, IR_OFC_LOB);

            // off code repeat upper/lower bound
            WRITE_WORD(REG_IR_OFC_RP_UPB, IR_OFC_RP_UPB);
            WRITE_WORD(REG_IR_OFC_RP_LOB, IR_OFC_RP_LOB);

            // logical 0/1 high upper/lower bound
            WRITE_WORD(REG_IR_LG01H_UPB, IR_LG01H_UPB);
            WRITE_WORD(REG_IR_LG01H_LOB, IR_LG01H_LOB);

            // logical 0 upper/lower bound
            WRITE_WORD(REG_IR_LG0_UPB, IR_LG0_UPB);
            WRITE_WORD(REG_IR_LG0_LOB, IR_LG0_LOB);

            // logical 1 upper/lower bound
            WRITE_WORD(REG_IR_LG1_UPB, IR_LG1_UPB);
            WRITE_WORD(REG_IR_LG1_LOB, IR_LG1_LOB);

            // timeout cycles
            WRITE_WORD(REG_IR_TIMEOUT_CYC_L, IR_RP_TIMEOUT & 0xFFFF);
            // set up ccode bytes and code bytes/bits num
            WRITE_WORD(REG_IR_TIMEOUT_CYC_H, IR_CCB_CB | 0x30UL | ((IR_RP_TIMEOUT >> 16) & 0x0F));

            WRITE_WORD(REG_IR_CKDIV_NUM_KEY_DATA, IR_CKDIV_NUM);
            break;
        case RC5_MODE:
            WRITE_WORD(REG_RC_DIV, (READ_WORD(REG_RC_DIV) & ~(RC_CLKDIV_MASE)) | (RC_CKDIV_NUM << 8));
            WRITE_WORD(REG_RC_LONGPL_THR, RC5_LONGPL_THR);
            break;
        default:
            ir_err("UNKNOW MODE\n");
    }
}

void _mdrv_set_int_enable(U8 bEnable)
{
#ifndef USE_POLLING_MODE
    if (bEnable)
    {
        enable_irq(gIr_Dev->irq);
        enable_irq(gIr_Dev->rc_irq);
    }
    else
    {
        disable_irq(gIr_Dev->irq);
        disable_irq(gIr_Dev->rc_irq);
    }
#endif
}

static void _key_dispatch_thread(struct work_struct *work)
{
    int                  ret     = 0;
    U32                  currKey = 0;
    static u32           preKey  = 0xFFFF;
    struct sstar_ir_dev *ir_dev  = container_of(work, struct sstar_ir_dev, key_dispatch_wk);

    ir_dbg("%s:%d enter \n", __func__, __LINE__);
    while (ir_dev->workrun)
    {
        try_to_freeze();

        if (preKey == 0xFFFF)
        {
            // ir_dbg("%s:%d enter \n",__func__, __LINE__);
            ret = wait_for_completion_interruptible(&ir_dev->key_done);
        }
        else
        {
            // ir_dbg("%s:%d enter \n",__func__, __LINE__);
            //  Depend on different IR to wait timeout.
            //  IR_TYPE_MSTAR_DTV, 150 is better, because ISR need such time to get another ir key.
            //
            //  NOTE:
            //  Too small, you will find the repeat function in android don't work. (up immediately)
            //  It will become down->up->down->down.....(not continue down)
            //  In input driver(2.6.35), over REP_DELAY(250 msecs) will auto-repeat, and every REP_PERIOD(33 msecs) will
            //  send repeat key. In input driver(3.0.20), over REP_DELAY(500 msecs) will auto-repeat, and every
            //  REP_PERIOD(125 msecs) will send repeat key. In android, over DEFAULT_LONG_PRESS_TIMEOUT(500 mesc) will
            //  auto-repeat, and every KEY_REPEAT_DELAY(50 mesc) will send repeat key.
            ret = wait_for_completion_interruptible_timeout(&ir_dev->key_done, msecs_to_jiffies(IR_EVENT_TIMEOUT));
        }

        if (ret < 0)
        {
            // ir_dbg("%s:%d %d enter \n",__func__, __LINE__, ret);
            continue;
        }

        currKey = _key_dequeue();
        ir_info("%s:%d currKey=0x%x enter \n", __func__, __LINE__, currKey);
        if ((preKey != 0xFFFF) && (currKey == 0xFFFF))
        {
            // ir_dbg("%s:%d enter \n",__func__, __LINE__);
            rc_keyup(ir_dev->dev);
        }
        else if ((preKey != 0xFFFF) && (currKey != 0xFFFF) && (preKey != currKey))
        {
            // ir_dbg("%s:%d enter \n",__func__, __LINE__);
            rc_keyup(ir_dev->dev);
            rc_keydown_notimeout(ir_dev->dev, ir_dev->protocol, currKey, 0);
        }
        else if ((preKey == 0xFFFF) && (currKey != 0xFFFF))
        {
            // ir_dbg("%s:%d enter \n",__func__, __LINE__);
            rc_keydown_notimeout(ir_dev->dev, ir_dev->protocol, currKey, 0);
        }

        preKey = currKey;
    }
}

#if (IR_MODE_SEL == IR_MODE_FULLDECODE)
static void _mdrv_get_key_fullmode(struct IR_KeyInfo *keyInfo)
{
    U16                  regVal   = 0;
    U8                   currKey  = 0;
    static U8            prevKey  = 0;
    static int           bRepeat  = 0;
    static unsigned long prevTime = 0;
    static unsigned long currTime = 0;

    regVal = READ_WORD(REG_IR_SHOT_CNT_H_FIFO_STATUS);
    if (regVal & IR_FIFO_EMPTY)
    {
        bRepeat          = 0;
        keyInfo->u8Valid = 0;
        return;
    }

    currTime = _mdrv_get_sys_time();
    if (currTime - prevTime >= IR_TIMEOUT_CYC / 1000)
    {
        currKey          = _mdrv_read_fifo();
        bRepeat          = 0;
        prevKey          = currKey;
        keyInfo->u8Key   = currKey;
        keyInfo->u8Valid = 1;
    }
    else
    {
        if (bRepeat == 0)
        {
            bRepeat          = 1;
            keyInfo->u8Valid = 0;
        }
        else
        {
            currKey          = _mdrv_read_fifo();
            regVal           = READ_WORD(REG_IR_SHOT_CNT_H_FIFO_STATUS);
            keyInfo->u8Flag  = (regVal & IR_RPT_FLAG) ? 1 : 0;
            keyInfo->u8Key   = currKey;
            keyInfo->u8Valid = ((keyInfo->u8Flag) && (currKey == prevKey));
        }
    }
    prevTime = currTime;

    while (!(regVal & IR_FIFO_EMPTY))
    {
        currKey = _mdrv_read_fifo();
        regVal  = READ_WORD(REG_IR_SHOT_CNT_H_FIFO_STATUS);
    }
    ir_info("IR FLAG : 0x%x\n", keyInfo->u8Flag);
    ir_info("IR CMD : 0x%x\n", keyInfo->u8Key);
    ir_info("IR VALID : 0x%x\n", keyInfo->u8Valid);
    //_mdrv_clear_fifo();
}
#elif (IR_MODE_SEL == IR_MODE_RAWDATA)
#define IR_RAW_DATA_NUM 4
static void _mdrv_get_key_rawmode(struct IR_KeyInfo *keyInfo)
{
    U16 i, regVal = 0;
    static int bRepeat = 0;
    static U8 preKey = 0;
    static U8 keyCount = 0;
    static U8 rawKey[IR_RAW_DATA_NUM];

    regVal = READ_WORD(REG_IR_SHOT_CNT_H_FIFO_STATUS);
    if (bRepeat == 1 && (regVal & IR_RPT_FLAG) == IR_RPT_FLAG)
    {
        bRepeat = 0;
        keyInfo->u8Valid = 1;
        keyInfo->u8Key = preKey;
        _mdrv_clear_fifo();
        return;
    }
    bRepeat = 1;

    for (i = 0; i < IR_RAW_DATA_NUM; i++)
    {
        regVal = READ_WORD(REG_IR_SHOT_CNT_H_FIFO_STATUS);
        if (regVal & IR_FIFO_EMPTY)
        {
            bRepeat = 0;
            keyInfo->u8Valid = 0;
            return;
        }

        rawKey[keyCount++] = _mdrv_read_fifo();
        if (keyCount == IR_RAW_DATA_NUM)
        {
            keyCount = 0;
            if ((rawKey[0] == IR_HEADER_CODE0) && (rawKey[1] == IR_HEADER_CODE1) && (rawKey[2] == (U8)(~rawKey[3])))
            {
                bRepeat = 0;
                preKey = rawKey[2];
                keyInfo->u8Key = rawKey[2];
                keyInfo->u8Valid = 1;
            }
        }
    }

    _mdrv_clear_fifo();
}
#elif (IR_MODE_SEL == RC5_MODE)
static void _mdrv_get_key_rc5mode(struct IR_KeyInfo *keyInfo)
{
    static U8  rc5_addr;
    static U8  rc5_cmd;
    static U16 rc5_key;
    rc5_key          = READ_WORD(REG_RC_KEY);
    rc5_addr         = rc5_key & RC_KEY_ADDR_MSK;
    rc5_cmd          = ((rc5_key & RC_KEY_CMD_MSK) >> 8);
    keyInfo->u8Flag  = rc5_addr;
    keyInfo->u8Key   = rc5_cmd;
    keyInfo->u8Valid = 1;
    WRITE_WORD(REG_RC_FIFO_RD, BIT0);
    ir_info("IR FLAG : 0x%x\n", keyInfo->u8Flag);
    ir_info("IR CMD : 0x%x\n", keyInfo->u8Key);
    ir_info("IR VALID : 0x%x\n", keyInfo->u8Valid);
}

#endif
static int _mdrv_get_key(void)
{
    struct IR_KeyInfo ir_keyInfo;

    memset((void *)&ir_keyInfo, 0, sizeof(struct IR_KeyInfo));

#if (IR_MODE_SEL == IR_MODE_FULLDECODE)
    // ir_dbg("USE FULL MODE\n");
    _mdrv_get_key_fullmode(&ir_keyInfo);
#elif (IR_MODE_SEL == IR_MODE_RAWDATA)
    // ir_dbg("USE RAW MODE\n");
    _mdrv_get_key_rawmode(&ir_keyInfo);
#elif (IR_MODE_SEL == RC5_MODE)
    // ir_dbg("USE RC5 MODE\n");
    _mdrv_get_key_rc5mode(&ir_keyInfo);
#endif

    if (ir_keyInfo.u8Valid)
    {
        _key_enqueue(ir_keyInfo.u8System << 8 | ir_keyInfo.u8Key);
        complete(&gIr_Dev->key_done);
    }

    ir_dbg("IR_keyInfo.System  is 0x%x\n", ir_keyInfo.u8System);
    ir_dbg("IR_keyInfo.Flag    is 0x%x\n", ir_keyInfo.u8Flag);
    ir_dbg("IR_keyInfo.Valid   is 0x%x\n", ir_keyInfo.u8Valid);
    ir_dbg("IR_keyInfo.Key     is 0x%x\n", ir_keyInfo.u8Key);
    return 0;
}

#ifndef USE_POLLING_MODE
irqreturn_t _mdrv_int_handler(int irq, void *dev_id)
{
    ir_dbg("IR IRQ IN\n");
    return _mdrv_get_key();
}

#else
static void _key_polling_thread(struct work_struct *work)
{
    while (1)
    {
        _mdrv_get_key();
        mdelay(100);
    }
}
#endif

static int _mdrv_input_init(struct sstar_ir_dev *ir_dev)
{
    int            err = 0;
    struct rc_dev *dev;

    rc_map_register(&mdrv_rc_map);

    dev = rc_allocate_device(RC_DRIVER_IR_RAW);
    if (!dev)
    {
        ir_err("%s:%d rc_allocate_device failed() \n", __func__, __LINE__);
        return -ENOMEM;
    }

    dev->driver_name       = "ir";
    dev->map_name          = IR_MAP_NAME;
    dev->driver_type       = RC_DRIVER_IR_RAW;
    dev->allowed_protocols = RC_PROTO_BIT_RC5 | RC_PROTO_BIT_NEC;
    dev->device_name       = IR_INPUT_NAME;
    dev->input_phys        = "/dev/ir";
    dev->input_id.bustype  = BUS_I2C;
    dev->input_id.vendor   = IR_VENDOR_ID;
    dev->input_id.product  = 0x0001;
    dev->input_id.version  = 1;

    err = rc_register_device(dev);
    if (err != 0)
    {
        rc_free_device(dev);
        ir_err("%s:%d rc_register_device failed() \n", __func__, __LINE__);
        return err;
    }

    // clear_bit(EV_REP, dev->input_dev->evbit);

    ir_dev->dev      = dev;
    ir_dev->protocol = RC_PROTO_UNKNOWN;
    init_completion(&ir_dev->key_done);

    return 0;
}

static int _mdrv_input_exit(struct sstar_ir_dev *ir_dev)
{
    rc_unregister_device(ir_dev->dev);
    rc_free_device(ir_dev->dev);
    rc_map_unregister(&mdrv_rc_map);

    return 0;
}

static int _mdrv_workqueue_init(struct sstar_ir_dev *ir_dev)
{
    ir_dev->workrun         = 1;
    ir_dev->key_dispatch_wq = create_workqueue("keydispatch_wq");
    INIT_WORK(&ir_dev->key_dispatch_wk, _key_dispatch_thread);
    queue_work(ir_dev->key_dispatch_wq, &ir_dev->key_dispatch_wk);

    return 0;
}

static int _mdrv_workqueue_exit(struct sstar_ir_dev *ir_dev)
{
    ir_dev->workrun = 0;
    destroy_workqueue(ir_dev->key_dispatch_wq);

    return 0;
}

static int _mdrv_ir_init(void)
{
    U16 regVal = 0;

    _mdrv_set_timing();

    WRITE_WORD(REG_IR_CCODE, (IR_HEADER_CODE1 << 8) | IR_HEADER_CODE0);
    WRITE_WORD(REG_IR_CTRL, 0x01BF);
    WRITE_WORD(REG_IR_GLHRM_NUM, 0x0804);
    WRITE_WORD(REG_IR_SEPR_BIT_FIFO_CTRL, 0x0F00);

    if (IR_MODE_SEL == IR_MODE_RAWDATA)
    {
        WRITE_WORD(REG_IR_CTRL, 0x01B3);
        WRITE_WORD(REG_IR_GLHRM_NUM, READ_WORD(REG_IR_GLHRM_NUM) | (0x02 << 12));
        WRITE_WORD(REG_IR_FIFO_RD_PULSE, READ_WORD(REG_IR_FIFO_RD_PULSE) | 0x20); // wakeup key sel
    }
    else if (IR_MODE_SEL == IR_MODE_FULLDECODE)
    {
        WRITE_WORD(REG_IR_GLHRM_NUM, READ_WORD(REG_IR_GLHRM_NUM) | (0x03 << 12));
        WRITE_WORD(REG_IR_FIFO_RD_PULSE, READ_WORD(REG_IR_FIFO_RD_PULSE) | 0x20); // wakeup key sel
    }
    else if (IR_MODE_SEL == RC5_MODE)
    {
        WRITE_WORD(REG_RC_CFG, RC5_EXT_DECODE);
        WRITE_WORD(REG_RC_WAKE_UP, READ_WORD(REG_RC_WAKE_UP) | (0X1 << 9));
    }
    else
    {
        WRITE_WORD(REG_IR_GLHRM_NUM, READ_WORD(REG_IR_GLHRM_NUM) | (0x01 << 12));
        if (IR_TYPE_SEL == IR_TYPE_RCMM)
        {
            regVal = READ_WORD(REG_IR_SEPR_BIT_FIFO_CTRL) | (0x01 << 12);
            WRITE_WORD(REG_IR_SEPR_BIT_FIFO_CTRL, regVal);
        }
        else
        {
#ifdef IR_INT_NP_EDGE_TRIG
            regVal = READ_WORD(REG_IR_SEPR_BIT_FIFO_CTRL) | (0x03 << 12);
            WRITE_WORD(REG_IR_SEPR_BIT_FIFO_CTRL, regVal);
#else
            WRITE_WORD(REG_IR_SEPR_BIT_FIFO_CTRL, 0x2F00); //[10:8]: FIFO depth, [11]:Enable FIFO full
#endif
        }
    }

#if (IR_MODE_SEL == IR_MODE_SWDECODE)
    _mdrv_set_sw_decode(1);
    WRITE_WORD(REG_IR_CKDIV_NUM_KEY_DATA, 0x00CF);
#endif

#if ((IR_MODE_SEL == IR_MODE_RAWDATA) || (IR_MODE_SEL == IR_MODE_FULLDECODE) || (IR_MODE_SEL == IR_MODE_SWDECODE))
    _mdrv_clear_fifo();
#endif

    return 0;
}

static int mdrv_ir_probe(struct platform_device *pdev)
{
    int                  retval;
    struct resource *    res;
    struct sstar_ir_dev *ir_dev;
#ifdef CONFIG_CAM_CLK
    u32                  IrClk = 0;
    CAMCLK_Set_Attribute stSetCfg;
    CAMCLK_Get_Attribute stGetCfg;
#else
    struct clk *ir_clk;
#endif

    ir_dbg("%s:%d enter \n", __func__, __LINE__);
    ir_dev = devm_kzalloc(&pdev->dev, sizeof(*ir_dev), GFP_KERNEL);
    if (!ir_dev)
    {
        ir_err("%s:%d devm_kzalloc() failed\n", __func__, __LINE__);
        return -ENOMEM;
    }

    ir_dev->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
    ir_dbg("%s:%d irq_of_parse_and_map(%d) \n", __func__, __LINE__, ir_dev->irq);
    ir_dev->rc_irq = irq_of_parse_and_map(pdev->dev.of_node, 1);
    ir_dbg("%s:%d irq_of_parse_and_map(%d) \n", __func__, __LINE__, ir_dev->rc_irq);
    if ((!ir_dev->irq) & (!ir_dev->rc_irq))
    {
        ir_err("%s:%d irq_of_parse_and_map() failed\n", __func__, __LINE__);
        return -ENODEV;
    }

    res             = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    ir_dev->membase = (void *)(IO_ADDRESS(res->start));
#ifdef CONFIG_CAM_CLK
    of_property_read_u32_index(pdev->dev.of_node, "camclk", 0, &(IrClk));
    if (!IrClk)
    {
        printk(KERN_DEBUG "[%s] Fail to get clk!\n", __func__);
    }
    else
    {
        if (CamClkRegister("Ir", IrClk, &(ir_dev->pvIrClk)) == CAMCLK_RET_OK)
        {
            CamClkAttrGet(ir_dev->pvIrClk, &stGetCfg);
            CAMCLK_SETPARENT(stSetCfg, stGetCfg.u32Parent[0]);
            CamClkAttrSet(ir_dev->pvIrClk, &stSetCfg);
            CamClkSetOnOff(ir_dev->pvIrClk, 1);
        }
    }
#else
    // 2. set clk
    ir_clk = of_clk_get(pdev->dev.of_node, 0);
    if (IS_ERR(ir_clk))
    {
        retval = PTR_ERR(ir_clk);
        ir_err("[%s]: of_clk_get failed\n", __func__);
    }
    else
    {
        clk_set_rate(ir_clk, XTAL_CLOCK_FREQ);
        clk_prepare_enable(ir_clk);
        ir_dbg("[IR] clk_prepare_enable\n");
    }
    ir_dev->clk = ir_clk;
#endif
    gIr_Dev = ir_dev;
    platform_set_drvdata(pdev, ir_dev);

    retval = _mdrv_input_init(ir_dev);
    if (retval != 0)
    {
        ir_err("%s:%d mdrv_input_init() failed\n", __func__, __LINE__);
        return retval;
    }

    _mdrv_ir_init();
    _mdrv_workqueue_init(ir_dev);

#ifndef USE_POLLING_MODE
    // ir_dbg("USE IRQ HANDLER\n");
    retval = request_irq(ir_dev->irq, _mdrv_int_handler, IRQF_SHARED, "IR", ir_dev);
    retval = request_irq(ir_dev->rc_irq, _mdrv_int_handler, IRQF_SHARED, "RC", ir_dev);
    if (retval != 0)
    {
        ir_err("%s:%d request_irq() failed\n", __func__, __LINE__);
        _mdrv_input_exit(ir_dev);
        _mdrv_workqueue_exit(ir_dev);
        return retval;
    }
#else
    key_polling_wq = create_workqueue("keypolling_wq");
    INIT_WORK(&key_polling_wk, _key_polling_thread);
    queue_work(key_polling_wq, &key_polling_wk);
#endif

    return 0;
}

static int mdrv_ir_remove(struct platform_device *pdev)
{
    struct sstar_ir_dev *ir_dev = platform_get_drvdata(pdev);
#ifndef CONFIG_CAM_CLK
    struct clk *ir_clk;
#endif
    ir_dbg("%s:%d enter \n", __func__, __LINE__);
    _mdrv_set_int_enable(0);
    _mdrv_workqueue_exit(ir_dev);
    _mdrv_input_exit(ir_dev);

#ifndef USE_POLLING_MODE
    free_irq(ir_dev->irq, ir_dev);
    free_irq(ir_dev->rc_irq, ir_dev);
#else
    destroy_workqueue(key_polling_wq);
#endif
#ifdef CONFIG_CAM_CLK
    if (ir_dev->pvIrClk)
    {
        CamClkSetOnOff(ir_dev->pvIrClk, 0);
        CamClkUnregister(ir_dev->pvIrClk);
    }
#else
    ir_clk = of_clk_get(pdev->dev.of_node, 0);
    if (IS_ERR(ir_clk))
    {
        ir_err("[IR] Fail to get clk!\n");
    }
    else
    {
        clk_disable_unprepare(ir_clk);
        clk_put(ir_clk);
    }
#endif
    return 0;
}

#ifdef CONFIG_PM
static int mdrv_ir_suspend(struct platform_device *pdev, pm_message_t state)
{
    struct clk_hw *      irclk_hw;
    struct clk_hw *      ir_parentclk_hw;
    struct sstar_ir_dev *ir_dev = platform_get_drvdata(pdev);
    ir_dbg("%s:%d enter \n", __func__, __LINE__);
    _mdrv_set_int_enable(0);

#if (IR_MODE_SEL == IR_MODE_SWDECODE)
    _mdrv_set_sw_decode(0);
#endif

    if (ir_dev && ir_dev->clk)
    {
        irclk_hw        = __clk_get_hw(ir_dev->clk);
        ir_parentclk_hw = clk_hw_get_parent_by_index(irclk_hw, 0);
        clk_set_parent(ir_dev->clk, ir_parentclk_hw->clk);
        clk_disable_unprepare(ir_dev->clk);
    }

    return 0;
}

static int mdrv_ir_resume(struct platform_device *pdev)
{
    struct sstar_ir_dev *ir_dev = platform_get_drvdata(pdev);
    ir_dbg("%s:%d enter \n", __func__, __LINE__);
    if (ir_dev && ir_dev->clk)
    {
        clk_set_rate(ir_dev->clk, XTAL_CLOCK_FREQ);
        clk_prepare_enable(ir_dev->clk);
    }
    _mdrv_ir_init();
    _mdrv_set_int_enable(1);

    return 0;
}
#endif

static const struct of_device_id sstar_ir_match_table[] = {{.compatible = "sstar,infinity-ir"}, {}};

static struct platform_driver sstar_ir_driver = {.probe  = mdrv_ir_probe,
                                                 .remove = mdrv_ir_remove,
#ifdef CONFIG_PM
                                                 .suspend = mdrv_ir_suspend,
                                                 .resume  = mdrv_ir_resume,
#endif
                                                 .driver = {
                                                     .name           = "Sstar-ir",
                                                     .owner          = THIS_MODULE,
                                                     .of_match_table = sstar_ir_match_table,
                                                 }};

module_platform_driver(sstar_ir_driver);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("IR driver");
MODULE_LICENSE("GPL");

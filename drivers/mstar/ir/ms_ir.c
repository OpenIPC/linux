/*
 * ms_ir.c
 *
 *  Created on: 2015年7月1日
 *      Author: Administrator
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/time.h>  //added
#include <linux/timer.h> //added
#include <linux/types.h> //added
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <media/rc-core.h>

#include <linux/of.h>
#include <linux/of_irq.h>

#include <asm/io.h>

#include "ms_ir.h"
#include "ms_platform.h"

#ifndef  IR_TYPE_SEL
#define IR_TYPE_SEL IR_TYPE_MSTAR_DTV
#endif

#if(IR_TYPE_SEL == IR_TYPE_MSTAR_DTV)
#include "IR_MSTAR_DTV.h"
#else
#error "undefined IR_TYPE_SEL"
#endif


#define MS_IR_INPUT_NAME      "Mstar IR"

#define IRFLAG_IRENABLE         0x00000001
#define IRFLAG_HWINITED         0x00000002

#define REG(addr)                   (*(volatile u32 *)(addr + (u32)ms_ir->membase))
#define IR_PRINT(fmt, args...)     //printk(KERN_EMERG "IR: [%05d] " fmt, __LINE__, ## args)

static int ms_ir_probe(struct platform_device *);
static int ms_ir_remove(struct platform_device *);
static int ms_ir_suspend(struct platform_device *, pm_message_t state);
static int ms_ir_resume(struct platform_device *);
static BOOL _MDrv_IR_GetKey(U8 *pu8Key, U8 *pu8System, U8 *pu8Flag);
static void _MDrv_IR_ClearFIFO(void);
//
//#ifdef CONFIG_MSTAR_SOFTWARE_IR_MODULE
//extern int take_over_by_software_ir(unsigned char, unsigned char, unsigned char, struct input_dev *, struct ir_input_state *);
//extern int software_ir_enable();
//extern int take_over_by_software_ir_dfb(unsigned char, unsigned char);
//extern int software_ir_processing_undone();
//extern int set_software_ir_processing_undone();
//#endif


//-------------------------------------------------------------------------------------------------
// Macros
//-------------------------------------------------------------------------------------------------
#define IR_PRINT(fmt, args...)     //printk(KERN_EMERG "IR: [%05d] " fmt, __LINE__, ## args)

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

static U8 bIRPass = 0;
static u8 _u8IRHeaderCode0 = IR_HEADER_CODE0;
static u8 _u8IRHeaderCode1 = IR_HEADER_CODE1;
static u8 ir_irq_depth;

#if (IR_MODE_SEL == IR_MODE_SWDECODE_MODE)
#ifndef IR2_HEADER_CODE0
#define IR2_HEADER_CODE0 0xff
#endif
#ifndef IR2_HEADER_CODE1
#define IR2_HEADER_CODE1 0xff
#endif
static u8 _u8IR2HeaderCode0 = IR2_HEADER_CODE0;
static u8 _u8IR2HeaderCode1 = IR2_HEADER_CODE1;
#endif

#define IR_RAW_DATA_NUM	        4
//#define IR_FILTER_REPEAT_NUM    1

#if (IR_MODE_SEL == IR_MODE_SWDECODE_MODE)
#define IR_SWDECODE_MODE_BUF_LEN        100
#endif

#if (IR_MODE_SEL == IR_MODE_SWDECODE_MODE)
#if(IR_TYPE_SEL != IR_TYPE_RCMM)
static u32  _u32IRData[IR_SWDECODE_MODE_BUF_LEN];
static u32  _u32IRCount=0;
#endif
#endif

//-------------------------------------------------------------------------------------------------
//  Local Structurs
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_IR_KEY_PROPERTY_INIT,
    E_IR_KEY_PROPERTY_1st,
    E_IR_KEY_PROPERTY_FOLLOWING
} IRKeyProperty;

struct MS_IR_DEVICE
{
    struct rc_dev *dev;
    unsigned char __iomem	*membase;		/* read/write[bwl] */
    int irq_in;
    int irq_rc;
    int protocol;
    struct fasync_struct        *async_queue; /* asynchronous readers */
    struct completion key_completion;
	struct work_struct key_dispatch_work;
	unsigned int IRFlag;
};


static struct MS_IR_DEVICE *ms_ir;


//#ifdef CONFIG_MSTAR_SOFTWARE_IR_MODULE
//u8 u8Key_for_mdrv_software_ir = 0;
//u8 u8RepeatFlag_for_mdrv_software_ir = 0;
//#endif


//--------------------------------------------------------------------------------------------------
// Forward declaration
//--------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------
#if (IR_MODE_SEL == IR_MODE_RAWDATA_MODE)
static u8   _u8IRRawModeBuf[IR_RAW_DATA_NUM];
static u32  _u8IRRawModeCount;
static unsigned long  _ulPrevKeyTime;
#endif

#if (IR_MODE_SEL == IR_MODE_SWDECODE_MODE)

#if(IR_TYPE_SEL == IR_TYPE_RCMM)
#define MAX_RCBYTE_LEN  4
static BOOL StartDecodeFlag = FALSE;
static u8 RCBitsCnt;
static u32 tgbits = 0;
static u8 RCByte[MAX_RCBYTE_LEN];
static u8 _u8IrPreRcmmData[MAX_RCBYTE_LEN];
static U16 u16CustomerID;
static BOOL UpDataFlage = FALSE;
static u8 RCMode;
static unsigned long  _ulPrevKeyTime;
#endif

static u32  _u32IRData[IR_SWDECODE_MODE_BUF_LEN];

#if (IR_TYPE_SEL == IR_TYPE_MSTAR_DTV)
static u8   _u8IRHeadReceived=0;
static u8   _u8IRRepeateDetect=0;
static u8   _u8IRRepeated=0;
static u8   _u8IRRepeatedNum=0;
static u8   _u8IRType=0;

#elif (IR_TYPE_SEL == IR_TYPE_CUS08_RC5)
static U16  _u16IrRc5Data=0;          // for store shift ir data
static U16  _u16PreIrRc5Data=0;          // for store previous ir data
static u8 _u8IrRc5Bits=0;                // for store bit count
static u8 _u8IrRc5LastBit=0;//IR_RC5_LAST_BIT;            // for store bit var
static unsigned long  _ulPrevKeyTime;

#elif (IR_TYPE_SEL == IR_TYPE_CUS21SH)
static u8   _u8IRReceiveDetect = 0;
static u8   _u8IRRepeateDetect = 0;
static U16  _u16IRKeyMap = 0;
static BOOL _bKeyValueHit = FALSE;

/////////
u8 g_bIrDetect;//!<IR command detect flag

u8 g_ucIrRepeatSkipCnt = 0; //IR_REPEAT_SKIP_COUNT;
u32 gIRTimeOutCount = 0;
static U16 _u16BufferCurrent=0;
static U16 _u16BufferPrev=0;
static u8 LastKeyData=0xFF;
static u8 IsRepeatData=0;
static u8 keyDatArrayWTIdx=0 ;
static u8 keyDatArrayRDIdx=0 ;
static BOOL _bExpectedTCome=TRUE;
static BOOL    FirstRXFlag=FALSE;
BOOL   SecondRXFlag=FALSE;
static u32 RxTimeOutCount;
static u32 CurrentTime;
static BOOL RxInProcess=FALSE;
static u32 _u32KeyOffTimeOutCount;
static BOOL ReceivingMode=FALSE;
static BOOL SetToStandbyMode=FALSE;

u8 g_u8Key;
u8 g_u8Flag;

////////
#endif
#endif

static u32  _u32_1stDelayTimeMs;
static u32  _u32_2ndDelayTimeMs;
static IRKeyProperty _ePrevKeyProperty;


static u8   _u8PrevKeyCode;
static u8   _u8PrevSystemCode;

#if (IR_MODE_SEL == IR_MODE_FULLDECODE_MODE)
static unsigned long  _ulPrevKeyTime;
#endif


static unsigned long  _ulLastKeyPresentTime;
static MS_IR_KeyInfo _KeyReceived;   //temporary solution


static struct workqueue_struct *key_dispatch_workqueue;




static const struct of_device_id ms_ir_of_match_table[] = {
	{ .compatible = "mstar,ir" },
	{}
};


static struct platform_driver ms_ir_driver = {
	.probe 		= ms_ir_probe,
	.remove 	= ms_ir_remove,
    .suspend    = ms_ir_suspend,
    .resume     = ms_ir_resume,

	.driver = {
		.name	= "Mstar-ir",
        .owner  = THIS_MODULE,
		.of_match_table = ms_ir_of_match_table,
	}
};


static void _MDrv_IR_Timing(void)
{
    // header code upper bound
    REG(REG_IR_HDC_UPB) = IR_HDC_UPB;

    // header code lower bound
    REG(REG_IR_HDC_LOB) = IR_HDC_LOB;

    // off code upper bound
    REG(REG_IR_OFC_UPB) = IR_OFC_UPB;

    // off code lower bound
    REG(REG_IR_OFC_LOB) = IR_OFC_LOB;

    // off code repeat upper bound
    REG(REG_IR_OFC_RP_UPB) = IR_OFC_RP_UPB;

    // off code repeat lower bound
    REG(REG_IR_OFC_RP_LOB) = IR_OFC_RP_LOB;

    // logical 0/1 high upper bound
    REG(REG_IR_LG01H_UPB) = IR_LG01H_UPB;

    // logical 0/1 high lower bound
    REG(REG_IR_LG01H_LOB) = IR_LG01H_LOB;

    // logical 0 upper bound
    REG(REG_IR_LG0_UPB) = IR_LG0_UPB;

    // logical 0 lower bound
    REG(REG_IR_LG0_LOB) = IR_LG0_LOB;

    // logical 1 upper bound
    REG(REG_IR_LG1_UPB) = IR_LG1_UPB;

    // logical 1 lower bound
    REG(REG_IR_LG1_LOB) = IR_LG1_LOB;

    // timeout cycles
    REG(REG_IR_TIMEOUT_CYC_L) = IR_RP_TIMEOUT & 0xFFFF;
    //set up ccode bytes and code bytes/bits num
    REG(REG_IR_TIMEOUT_CYC_H_CODE_BYTE) = IR_CCB_CB | 0x30 | ((IR_RP_TIMEOUT >> 16) & 0x0F);

    REG(REG_IR_CKDIV_NUM_KEY_DATA) = IR_CKDIV_NUM;   // clock divider
}

#define MaxQueue 100
static struct queue
{
    u32 item[MaxQueue];
    int front;
    int rear;
} q;

static DEFINE_SEMAPHORE(queue_lock);


static void enqueue(u32 data)
{
    if (down_trylock(&queue_lock))
       return;

    if (q.rear == ((q.front + 1) % MaxQueue))
    {
        IR_PRINT("queue is full \n");
    }
    else
    {
        q.rear = (q.rear + 1) % MaxQueue;
        q.item[q.rear] = data;
    }

    up(&queue_lock);
}


static u32 dequeue(void)
{
    u32 data = 0xFFFF;

    down(&queue_lock);

    if (q.front == q.rear)
    {
        IR_PRINT("queue is empty \n");
    }
    else
    {
        q.front = (q.front + 1) % MaxQueue;
        data = q.item[q.front];
    }

    up(&queue_lock);
    return data;
}

static unsigned long _MDrv_IR_GetSystemTime(void)
{
    return((unsigned long)((jiffies)*(1000/HZ)));
    //return 0;
}

static void key_dispatch(struct work_struct *work)
{
    static u32 prev_scancode = 0xFFFF;
    struct MS_IR_DEVICE *ir;

    printk("[KEY_INPUT]:key_dispatch thread start\n");
    //daemonize("ir_key_dispatch");

    ir=container_of( work, struct MS_IR_DEVICE, key_dispatch_work);

    while(1)
    {
        int ret;
        u32 scancode;

        try_to_freeze();

        //IR_PRINT("xxxxx 0\n");
        if (prev_scancode == 0xFFFF)
        {
            //IR_PRINT("xxxxx 1\n");
            ret = wait_for_completion_interruptible(&ir->key_completion);
        }
        else
        {
            //IR_PRINT("xxxxx 2\n");
            // Depend on different IR to wait timeout.
            // or IR_TYPE_MSTAR_DTV, 150 is better, because ISR need such time to get another ir key.
            //
            // NOTE:
            // Too small, you will find the repeat function in android don't work. (up immediately)
            // It will become down->up->down->down.....(not continue down)
            // In input driver(2.6.35), over REP_DELAY(250 msecs) will auto-repeat, and every REP_PERIOD(33 msecs) will send repeat key.
            // In input driver(3.0.20), over REP_DELAY(500 msecs) will auto-repeat, and every REP_PERIOD(125 msecs) will send repeat key.
            // In android, over DEFAULT_LONG_PRESS_TIMEOUT(500 mesc) will auto-repeat, and every KEY_REPEAT_DELAY(50 mesc) will send repeat key.
            ret = wait_for_completion_interruptible_timeout(&ir->key_completion, msecs_to_jiffies(150));
        }
        if (ret < 0)
        {
            IR_PRINT("completion interruptible\n");
            continue;
        }

        scancode = dequeue();
        if ((prev_scancode != 0xFFFF) && (scancode == 0xFFFF))
        {
            //IR_PRINT("xxxxx 3\n");
            rc_keyup(ir->dev);
        }
        else if ((prev_scancode != 0xFFFF) && (scancode != 0xFFFF))
        {
           //IR_PRINT("xxxxx 4\n");
           if ((scancode != prev_scancode))
           {
               //IR_PRINT("xxxxx 5\n");
               rc_keyup(ir->dev);

               rc_keydown_notimeout(ir->dev,ir->protocol, scancode, 0);

           }
        }
        else if ((prev_scancode == 0xFFFF) && (scancode != 0xFFFF))
        {

            rc_keydown_notimeout(ir->dev,ir->protocol, scancode, 0);

        }

        //IR_PRINT("xxxxx 7, scancode=%d\n", scancode);
        prev_scancode = scancode;
    }
    printk(" key_dispatch thread end\n");

    return;
}


//-------------------------------------------------------------------------------------------------
/// ISR when receive IR key.
/// @return None
//-------------------------------------------------------------------------------------------------
irqreturn_t _MDrv_IR_ISR(int irq, void *dev_id)
//irqreturn_t _MDrv_IR_ISR(int irq, void *dev_id, struct pt_regs *regs)
{

    u8 u8Key=0, u8RepeatFlag=0;
    u8 u8System = 0;
    BOOL bHaveKey = FALSE;

#if (IR_MODE_SEL == IR_MODE_SWDECODE_MODE)
    #if(IR_TYPE_SEL == IR_TYPE_RCMM)
    U16 u16IrCounter;
    static unsigned long PreTime;
    u16IrCounter = ((REG(REG_IR_SHOT_CNT_H_FIFO_STATUS)&0xF) << 16) | ((REG(REG_IR_SHOT_CNT_L))&0xFFFF);
    if ((_MDrv_IR_GetSystemTime() - PreTime > IR_TIMEOUT_CYC/1000))//reset
    {
        RCBitsCnt = 0;
        _u8PrevKeyCode = 0xff;
    }
    //i++;
    //if(i % 2)
       // printk("%d:%d\n",i,u16IrCounter);
    //return IRQ_HANDLED;

    if(P25_MIN < u16IrCounter && u16IrCounter < P25_MAX)
    {
        tgbits = 0x00;
        RCByte[0] = 0x00;
        RCByte[1] = 0x00;
        RCByte[2] = 0x00;
        RCByte[3] = 0x00;
        RCBitsCnt = 0;
        RCMode = 0;

        StartDecodeFlag = TRUE;
        //printk("START\n");
        //head code start
    }
    else if( (P16_MIN < u16IrCounter && u16IrCounter < P16_MAX) && StartDecodeFlag) //! it is 00 bit sequence
    {
        tgbits = 0x00;
        //printk("00\n");
        //_u32IrRcmmData <<= 2;
        //_u32IrRcmmData |= tgbits;
        RCByte[RCBitsCnt>>3] <<= 2;
        RCByte[RCBitsCnt>>3] |= tgbits;
        RCBitsCnt += 2;
    }
    else if( (P22_MIN < u16IrCounter && u16IrCounter< P22_MAX) && StartDecodeFlag) //! it is 01 bit sequence
    {
        tgbits = 0x01;
        //printk("01\n");
        //_u32IrRcmmData <<= 2;
        //_u32IrRcmmData |= tgbits;
        RCByte[RCBitsCnt>>3] <<= 2;
        RCByte[RCBitsCnt>>3] |= tgbits;
        RCBitsCnt += 2;
    }
    else if( (P28_MIN < u16IrCounter && u16IrCounter < P28_MAX) && StartDecodeFlag) //! it is 10 bit sequence
    {
        tgbits = 0x02;
        //printk("10\n");
        //_u32IrRcmmData <<= 2;
        //_u32IrRcmmData |= tgbits;
        RCByte[RCBitsCnt>>3] <<= 2;
        RCByte[RCBitsCnt>>3] |= tgbits;
        RCBitsCnt += 2;

    }
    else if( (P34_MIN < u16IrCounter && u16IrCounter < P34_MAX) && StartDecodeFlag) //! it is 11 bit sequence
    {
        tgbits = 0x03;
        //printk("11\n");
        //_u32IrRcmmData <<= 2;
        //_u32IrRcmmData |= tgbits;
        RCByte[RCBitsCnt>>3] <<= 2;
        RCByte[RCBitsCnt>>3] |= tgbits;
        RCBitsCnt += 2;
    }
    else
    {
        StartDecodeFlag = FALSE;
        RCBitsCnt = 0;
        UpDataFlage = FALSE;
        tgbits = 0x00;
        RCByte[0] = 0x00;
        RCByte[1] = 0x00;
        RCByte[2] = 0x00;
        RCByte[3] = 0x00;
    }

    if(RCBitsCnt == 24)
    {
        if(RCByte[0] & 0x20)
        {
            RCMode |= RCMMOEM_LONGID_MODE;

            tgbits = (RCByte[1]&0x0C) >> 2;
            RCMode |= 1<<tgbits; //OEM_LONGID_RC, //OEM_LONGID_Mouse, //OEM_LONGID_keyboard, //OEM_LONGID_joystick
        }
        else if( ((RCByte[0]&0xFC)>>2) == 0x03 )
        {
            RCMode |= RCMMOEM_SHORTID_MODE;

            tgbits = (RCByte[1]&0x0C) >> 2;
            RCMode |= 1<<tgbits; //OEM_SHORTID_RC, //OEM_SHORTID_Mouse, //OEM_SHORTID_keyboard, //OEM_SHORTID_joystick
        }

        if( (RCMode & MOUSE_MODE) || (RCMode & KEYBOARD_MODE) )
        {
            StartDecodeFlag = FALSE;
            UpDataFlage = TRUE;
        }

    }
    else if(RCBitsCnt >= 32)
    {
        if( (RCMode & RC_MODE) || (RCMode & JOYSTICK_MODE) )
        {
            StartDecodeFlag = FALSE;
            UpDataFlage = TRUE;
        }
        else
        {
            RCBitsCnt = 0;
            u16CustomerID = 0;
            RCMode = 0;
            UpDataFlage = FALSE;
            tgbits = 0x00;
            RCByte[0] = 0x00;
            RCByte[1] = 0x00;
            RCByte[2] = 0x00;
            RCByte[3] = 0x00;
        }
    }
    PreTime = _MDrv_IR_GetSystemTime();

#elif (IR_TYPE_SEL == IR_TYPE_CUS08_RC5)

    U16 u16IrCounter,u16N_Shot;

    u16IrCounter=((REG(REG_IR_SHOT_CNT_H_FIFO_STATUS)&0xF) << 16) | ((REG(REG_IR_SHOT_CNT_L))&0xFFFF);
    u16N_Shot=(REG(REG_IR_SHOT_CNT_H_FIFO_STATUS))&0x10;

    if ((u16IrCounter > ONE_BIT_UB) || (u16IrCounter < HALF_BIT_LB))    // if ir counter so big, then reset and return
    {
        _u8IrRc5LastBit = TRUE;
        _u16IrRc5Data = 0;
        _u8IrRc5Bits = 0;
    }
    else
    {
        if (u16IrCounter >= HALF_BIT_LB && u16IrCounter <= HALF_BIT_UB)     //detect
        {
            _u8IrRc5Bits++;
            if (_u8IrRc5LastBit)
            {
                _u8IrRc5LastBit = FALSE;
                _u16IrRc5Data <<= 1;
                if (u16N_Shot==0) // Is nshot
                    _u16IrRc5Data |= 1;
            }
            else
            {
                _u8IrRc5LastBit = TRUE;
            }
        }
        else if (u16IrCounter >= ONE_BIT_LB && u16IrCounter <= ONE_BIT_UB)
        {
            if (_u8IrRc5LastBit)
            {
                _u16IrRc5Data <<= 1;
                if (u16N_Shot==0) // Is nshot
                    _u16IrRc5Data |= 1;
                _u8IrRc5Bits += 2;
            }
            else    //error stste
            {
                _u16IrRc5Data = 0;
                _u8IrRc5Bits = 0;
            }
        }

        if (_u8IrRc5Bits == IR_RC5_DATA_BITS)
        {
            if(u16N_Shot==0)
            {
                _u16IrRc5Data <<= 1;
                _u8IrRc5Bits++;
            }
        }
    }

    #elif (IR_TYPE_SEL == IR_TYPE_MSTAR_DTV)
    static unsigned long ulPreTime;
    u32 tmp;

    if ((_MDrv_IR_GetSystemTime() - ulPreTime > IR_TIMEOUT_CYC/1000))//||_u8IRRepeated) //timeout or not handler yet
    {
        _u32IRCount = 0;
		_u8IRRepeateDetect=0;
		_u8IRRepeated=0;
		_u8IRRepeatedNum=0;
		_u8IRHeadReceived=0;
    }
    if (_u32IRCount <IR_SWDECODE_MODE_BUF_LEN)
    {
        tmp=((REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & 0xF) << 16) | REG(REG_IR_SHOT_CNT_L);
		if(tmp<IR_LG0_LOB)//error signal
		{
        	_u32IRCount = 0;
			_u8IRRepeateDetect=0;
			_u8IRRepeated=0;
			_u8IRRepeatedNum=0;
			_u8IRHeadReceived=0;
		}

		if( tmp>(IR_HDC_LOB))//Head received
		{
			_u32IRCount=0;
			_u8IRHeadReceived=1;
		}
        /*
        printk("\n[%d][%d][%d]\n",tmp,IR_OFC_RP_LOB,IR_OFC_RP_UPB);
        //eg haier repeate: tmp|IR_OFC_RP_LOB|IR_OFC_RP_UPB
        [92715][1846][2769]
        [4655][1846][2769]
        [2047][1846][2769]

        //eg mstar dtv repeate tmp|IR_OFC_RP_LOB|IR_OFC_RP_UPB
        [97137][1846][2769]
        [2581][1846][2769]
        so mstar dtv  has no IR_OFC_UPB code
        */
		if(_u8IRHeadReceived)//begin data received
		{
	        _u32IRData[_u32IRCount++] = tmp;
			if(_u8IRRepeateDetect)
			{
				if( tmp>(IR_OFC_RP_LOB) && tmp<(IR_OFC_RP_UPB) )
				{
                    if(_u32IRCount<4)//for quickly change ir
                    {
                        _u8IRRepeated=1;
                        _u8IRHeadReceived=0;
                    }
				}
			}

		}

    }
    ulPreTime = _MDrv_IR_GetSystemTime();

#elif (IR_TYPE_SEL == IR_TYPE_CUS21SH)

    u32 tmp;

    #if (1)
    tmp = 0;
    tmp |=  ((REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & 0xF) << 16);
    tmp |=  (REG(REG_IR_SHOT_CNT_L) & 0xFFFF);


    if(tmp >= 420L && tmp < 4000L)
    {
        if(tmp  >=1580L && tmp < 4000L)
        {
            _u16IRKeyMap|=(1<<_u32IRCount);
        }
        _u32IRCount++;
        //ReceivingMode=TRUE;
        RxInProcess=TRUE;
        //RxTimeOutCount = _MDrv_IR_GetSystemTime();
        //ResetKeyoffTimer();
    }
    else if(tmp >= 4000L)
    {
        _bExpectedTCome = FALSE;
        SetToStandbyMode = TRUE;
        ReceivingMode = TRUE;
        _u32IRCount=0;
        _u16IRKeyMap=0;
    }


    //if(BufIdx >= 15)
        //printk("IR: %s:%d %x,%x\n",__FUNCTION__,__LINE__,_u32IRCount,_u16IRKeyMap);
    #else

    #endif

    #else
    static unsigned long ulPreTime;
    if (_MDrv_IR_GetSystemTime() - ulPreTime > IR_TIMEOUT_CYC/1000) //timeout
    {
        _u32IRCount = 0;
    }

    if (_u32IRCount <IR_SWDECODE_MODE_BUF_LEN)
    {
        _u32IRData[_u32IRCount++] = ((REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & 0xF) << 16) | REG(REG_IR_SHOT_CNT_L);
    }
    ulPreTime = _MDrv_IR_GetSystemTime();
    #endif
#endif  //#if (IR_MODE_SEL == IR_MODE_SWDECODE_MODE)


#ifdef CONFIG_IR_SUPPLY_RNG
    {
        unsigned int u32Temp;
        unsigned short u16Temp;

        u16Temp = MIPS_REG(REG_RNG_OUT);
        memcpy((unsigned char *)&u32Temp+0, &u16Temp, 2);
        u16Temp = MIPS_REG(REG_RNG_OUT);
        memcpy((unsigned char *)&u32Temp+2, &u16Temp, 2);
        add_input_randomness(EV_MSC, MSC_SCAN, u32Temp);
    }
#endif
    //regs = NULL;


    if ((bHaveKey=_MDrv_IR_GetKey(&u8Key, &u8System, &u8RepeatFlag)) != 0)
    {
        _ulLastKeyPresentTime = _MDrv_IR_GetSystemTime();

        //temporary solution, need to implement ring buffer for this
        _KeyReceived.u8Key = u8Key;
        _KeyReceived.u8System = u8System;
        _KeyReceived.u8Flag = u8RepeatFlag;
        _KeyReceived.u8Valid = 1;

       	if (ms_ir->async_queue)
    		kill_fasync(&ms_ir->async_queue, SIGIO, POLL_IN);
    }

    IR_PRINT("HaveKey=%d, KEY=%d, RepeatFalg=%d\n", bHaveKey, u8Key, u8RepeatFlag);


    if (ms_ir && bHaveKey)
    {
#ifdef CONFIG_MSTAR_SOFTWARE_IR_MODULE
        if (software_ir_enable())
        {
            take_over_by_software_ir(u8Key, u8System, u8RepeatFlag, ir->dev, &ir->ir);
        }
        else
        {
#endif
            enqueue((u8System << 8) | u8Key);
            complete(&ms_ir->key_completion);
#ifdef CONFIG_MSTAR_SOFTWARE_IR_MODULE
        }
#endif
    }

    return IRQ_HANDLED;
}


static int ir_input_init(void)
{
	int err = 0;
	struct rc_dev *dev;


	rc_map_register(&ms_rc_map);


	dev=rc_allocate_device();
	if (!dev)
	{
		return -ENOMEM;
	}

	ms_ir->dev=dev;
	ms_ir->protocol=RC_TYPE_UNKNOWN;

	dev->driver_name = "ir";
	dev->map_name = MS_IR_MAP_NAME;
	dev->driver_type = RC_DRIVER_IR_RAW;
	dev->input_name = MS_IR_INPUT_NAME;
	dev->input_phys = "/dev/ir";
	dev->input_id.bustype = BUS_I2C;
	dev->input_id.vendor = MS_IR_VENDOR_ID;
	dev->input_id.product = 0x0001;
	dev->input_id.version = 1;

    err = rc_register_device(dev);

	if (err)
	{
		rc_free_device(dev);
		return err;
	}

    clear_bit(EV_REP, dev->input_dev->evbit);


    init_completion(&ms_ir->key_completion);

    INIT_WORK(&ms_ir->key_dispatch_work,key_dispatch);
    key_dispatch_workqueue = create_workqueue("keydispatch_wq");

    return 0;
}


#if (IR_MODE_SEL == IR_MODE_FULLDECODE_MODE)
//-------------------------------------------------------------------------------------------------
/// Get IR key. It is a non-blocking function.
/// @param pu8Key  \b IN: Return IR key value.
/// @param pu8Flag \b IN: Return IR repeat code.
///
/// @return TRUE:  Success
/// @return FALSE: No key or repeat key is faster than the specified period
//-------------------------------------------------------------------------------------------------
static BOOL _MDrv_IR_GetKey(U8 *pu8Key, U8 *pu8System, U8 *pu8Flag)
{
#ifndef CONFIG_MS_IR_INPUT_DEVICE
    static unsigned long  _ulPrevKeyRepeatTime;
#endif
    static BOOL  _bCheckQuickRepeat;
    BOOL bRet=FALSE;
    *pu8System = 0;

    if(REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_FIFO_EMPTY)
    {
        _bCheckQuickRepeat = 0;
        return FALSE;
    }

    if(((_MDrv_IR_GetSystemTime() - _ulPrevKeyTime) >= IR_TIMEOUT_CYC/1000))
    {
        *pu8Key = REG(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
        REG(REG_IR_FIFO_RD_PULSE) |= 0x0001; //read

        mdelay(10);

        _u8PrevKeyCode = *pu8Key;
        *pu8Flag = 0;
        _ulPrevKeyTime = _MDrv_IR_GetSystemTime();
        _ePrevKeyProperty = E_IR_KEY_PROPERTY_INIT;
        _bCheckQuickRepeat = 0;
        _MDrv_IR_ClearFIFO();
        return TRUE;
    }
    else
    {
        if(_bCheckQuickRepeat==0)
        {
            _bCheckQuickRepeat = 1;
            _ulPrevKeyTime = _MDrv_IR_GetSystemTime();
            _MDrv_IR_ClearFIFO();
            return FALSE;
        }
        *pu8Key = REG(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
        *pu8Flag = (REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_RPT_FLAG)? 1 : 0;
        REG(REG_IR_FIFO_RD_PULSE) |= 0x0001; //read
        bRet = FALSE;
        _ulPrevKeyTime = _MDrv_IR_GetSystemTime();
        if ( (*pu8Flag == 1) && ( *pu8Key == _u8PrevKeyCode ))
        {
            bRet = TRUE;

        }
    }
    //printk("*pu8Key=%d",*pu8Key);

    // Empty the FIFO
    _MDrv_IR_ClearFIFO();
    return bRet;

}


#if((IR_MODE_SEL==IR_MODE_RAWDATA_MODE) || (IR_MODE_SEL==IR_MODE_FULLDECODE_MODE))
static void _MDrv_IR_ClearFIFO(void)
{
    unsigned long i;

    // Empty the FIFO
    for(i=0; i<8; i++)
    {
        U8 u8Garbage;

        if(REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_FIFO_EMPTY)
            break;

        u8Garbage = REG(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
        REG(REG_IR_FIFO_RD_PULSE) |= 0x0001; //read
    }

}
#endif


#elif (IR_MODE_SEL == IR_MODE_RAWDATA_MODE)
//-------------------------------------------------------------------------------------------------
/// Get IR key.
/// @param pu8Key  \b IN: Return IR key value.
/// @param pu8Flag \b IN: Return IR repeat code.
///
/// @return TRUE:  Success
/// @return FALSE: Failure
//-------------------------------------------------------------------------------------------------
#if 0
static unsigned long  _ulPrevKeyRepeatTime;
static BOOL  _bCheckQuickRepeat;
static BOOL msIR_GetRawKey(U8 *pu8key,U8 *pu8flag,BOOL bChkCCode)
{
    U8 u8IRRawModeDone;
    U8 u8IRRawModeCount;
    U8 k;

    u8IRRawModeCount = 0;

    *pu8key = 0xFF;
    *pu8flag = (REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_RPT_FLAG)? 1 : 0;

    for(k=0;k<IR_RAW_DATA_NUM;k++)
    {
        if ( REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_FIFO_EMPTY)  // check FIFO empty
            break;
        _u8IRRawModeBuf[_u8IRRawModeCount++] = REG(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
        REG(REG_IR_FIFO_RD_PULSE) |= 0x0001; //read
        u8IRRawModeCount++;
    }
    REG(REG_IR_SEPR_BIT_FIFO_CTRL) |= 0x8000; //IMPORTANT!!!

    if ( u8IRRawModeCount >= 4 )
        u8IRRawModeDone = 1;

    if (u8IRRawModeDone)
    {
        u8IRRawModeDone = 0;
        if(bChkCCode==TRUE)
        {
            if ( (_u8IRRawModeBuf[0] == _u8IRHeaderCode0) && (_u8IRRawModeBuf[1] == _u8IRHeaderCode1) )
            {
                if(_u8IRRawModeBuf[2] == (U8)(~_u8IRRawModeBuf[3]))
                {
                    *pu8key = _u8IRRawModeBuf[2];
                    return TRUE;
                }
            }
        }
        else
        {
            *pu8key = _u8IRRawModeBuf[2]; //driectly assign key with index==2
            return TRUE;
        }
        u8IRRawModeCount = 0;
    }
    return FALSE;
}

static BOOL _MDrv_IR_GetKey(U8 *pu8Key, U8 *pu8System, U8 *pu8Flag)
{
    unsigned long i;
    BOOL bRet=FALSE;
    *pu8System = 0;

    if(REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_FIFO_EMPTY)
    {
        _bCheckQuickRepeat = 0;
        return FALSE;
    }

    if(((_MDrv_IR_GetSystemTime() - _ulPrevKeyTime) >= IR_TIMEOUT_CYC/1000))
    {
        *pu8Key = REG(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
        REG(REG_IR_FIFO_RD_PULSE) |= 0x0001; //read
        #ifdef __arm__
        __udelay(1000*10);
        #else
        udelay(1000*10);
        #endif
        msIR_GetRawKey(pu8Key,pu8Flag,TRUE);
        _u8PrevKeyCode = *pu8Key;
        *pu8Flag = 0;
        _ulPrevKeyTime = _MDrv_IR_GetSystemTime();
        _ePrevKeyProperty = E_IR_KEY_PROPERTY_INIT;
        _bCheckQuickRepeat = 0;
        _MDrv_IR_ClearFIFO();
        return TRUE;
    }
    else
    {
        if(_bCheckQuickRepeat==0)
        {
           _bCheckQuickRepeat = 1;
            _ulPrevKeyTime = _MDrv_IR_GetSystemTime();
            _MDrv_IR_ClearFIFO();
            return FALSE;
        }

        msIR_GetRawKey(pu8Key,pu8Flag,TRUE);
        bRet = FALSE;
        _ulPrevKeyTime = _MDrv_IR_GetSystemTime();
        if ( (*pu8Flag == 1) && ( *pu8Key == _u8PrevKeyCode ))
        {
            i = _MDrv_IR_GetSystemTime();
            if( _ePrevKeyProperty == E_IR_KEY_PROPERTY_INIT)
            {
                _u8PrevKeyCode     = *pu8Key;
                _ulPrevKeyRepeatTime    = i;
                _ePrevKeyProperty  = E_IR_KEY_PROPERTY_1st;
            }
            else if(_ePrevKeyProperty == E_IR_KEY_PROPERTY_1st)
            {
                if( (i - _ulPrevKeyRepeatTime) > _u32_1stDelayTimeMs)
                {
                    _ulPrevKeyRepeatTime = i;
                    _ePrevKeyProperty  = E_IR_KEY_PROPERTY_FOLLOWING;
                    bRet = TRUE;
                }
            }
            else //E_IR_KEY_PROPERTY_FOLLOWING
            {
                if( (i - _ulPrevKeyRepeatTime) > _u32_2ndDelayTimeMs)
                {
                    _ulPrevKeyRepeatTime = i;
                    bRet = TRUE;
                }
            }
        }
    }

    // Empty the FIFO
    _MDrv_IR_ClearFIFO();
    return bRet;

}
#else
static BOOL _MDrv_IR_GetKey(U8 *pu8Key, U8 *pu8System, U8 *pu8Flag)
{
    BOOL bRet = FALSE;

    u32 i, j;
    *pu8System = 0;

    for (j=0; j<IR_RAW_DATA_NUM; j++)
    {
        if ( REG(REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_FIFO_EMPTY)  // check FIFO empty
            break;

        _u8IRRawModeBuf[_u8IRRawModeCount++] = REG(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
        REG(REG_IR_FIFO_RD_PULSE) |= 0x0001; //read

    	for(i=0;i<5;i++); //Delay

        if(_u8IRRawModeCount == IR_RAW_DATA_NUM)
        {
            _u8IRRawModeCount = 0;
            if( (_u8IRRawModeBuf[0]==_u8IRHeaderCode0) &&
                (_u8IRRawModeBuf[1]==_u8IRHeaderCode1) )
            {
                if(_u8IRRawModeBuf[2] == (U8)(~_u8IRRawModeBuf[3]))
                {
                    //if (!MDrv_SYS_GetDisplayControllerSeparated())
                        *pu8Key = _MDrv_IR_ParseKey(_u8IRRawModeBuf[2]);    // translate for MIPS

                    //TODO: Implement repeat code later.
                    *pu8Flag = 0;
                    bRet = TRUE;
                    break;
                }
            }
        }
    }

    // Empty the FIFO
    _MDrv_IR_ClearFIFO();

    return bRet;
}
#endif

#elif(IR_MODE_SEL == IR_MODE_SWDECODE_MODE)
//-------------------------------------------------------------------------------------------------
/// Get IR key.
/// @param pu8Key  \b IN: Return IR key value.
/// @param pu8Flag \b IN: Return IR repeat code.
///
/// @return TRUE:  Success
/// @return FALSE: Failure
//-------------------------------------------------------------------------------------------------
static BOOL _MDrv_IR_GetKey(U8 *pu8Key, U8 *pu8System, U8 *pu8Flag)
{
    BOOL bRet = FALSE;

#if(IR_TYPE_SEL == IR_TYPE_RCMM)
    unsigned long i;
#else
//#if ((IR_TYPE_SEL != IR_TYPE_CUS21SH) && (IR_TYPE_SEL != IR_TYPE_TOSHIBA))
    //#if (IR_TYPE_SEL != IR_TYPE_CUS08_RC5)
    U32 u8Byte, u8Bit;

    U8 u8IRSwModeBuf[IR_RAW_DATA_NUM];

    U32 *pu32IRData = NULL;
    *pu8System = 0;
    //#else
    //unsigned long i;
    //#endif
//#endif
#endif

#if(IR_TYPE_SEL == IR_TYPE_RCMM)

    if(UpDataFlage)
    {
        //printk("GetKey\n");
        UpDataFlage = FALSE;

        switch(RCMode)
        {
            case RCMMOEM_LONGID_MODE|RC_MODE:
            {
                if((_u8IrPreRcmmData[0] == RCByte[0]) && (_u8IrPreRcmmData[1] == RCByte[1]) && (_u8IrPreRcmmData[2] == RCByte[2]) && (_u8IrPreRcmmData[3] == RCByte[3]))
                {
                    *pu8Flag = TRUE;
                }

                u16CustomerID = ((RCByte[0] & 0x1F) << 4) | ((RCByte[1] & 0xF0) >> 4);

                if(u16CustomerID == 0x007D)
                {
                    *pu8Key = RCByte[3];
                    if(*pu8Key>=30 && *pu8Key<=132)
                        *pu8Key = RCByte[3]+111;
                     //printk("0x%x:,%d\n",*pu8Key,*pu8Key);
                    *pu8System = (RCByte[2] & 0x7F);

                    _u8IrPreRcmmData[0] = RCByte[0];
                    _u8IrPreRcmmData[1] = RCByte[1];
                    _u8IrPreRcmmData[2] = RCByte[2];
                    _u8IrPreRcmmData[3] = RCByte[3];

                    RCByte[0] = 0x0000;
                    RCByte[1] = 0x0000;
                    RCByte[2] = 0x0000;
                    RCByte[3] = 0x0000;

                    RCMode = 0;
                    RCBitsCnt = 0;
                    bRet = TRUE;
                }

                break;
            }
            case RCMMOEM_LONGID_MODE|KEYBOARD_MODE:
            {
                if((_u8IrPreRcmmData[0] == RCByte[0]) && (_u8IrPreRcmmData[1] == RCByte[1]) && (_u8IrPreRcmmData[2] == RCByte[2]))
                {
                    *pu8Flag = TRUE;
                }

                u16CustomerID = ((RCByte[0] & 0x1F) << 4) | ((RCByte[1] & 0xF0) >> 4);
                if(u16CustomerID == 0x007D)
                {
                    if(_u8PrevKeyCode==61)
                    {
                        *pu8Key = (RCByte[2] & 0x7F) + 0x3D;
                        if(*pu8Key>=97&&*pu8Key<=99)
                            *pu8Key += 150;
                    }
                    else
                    {
                        *pu8Key = RCByte[2] & 0x7F;
                    }
                    //printk("0x%x:,%d\n",*pu8Key,*pu8Key);
                    _u8IrPreRcmmData[0] = RCByte[0];
                    _u8IrPreRcmmData[1] = RCByte[1];
                    _u8IrPreRcmmData[2] = RCByte[2];

                    RCByte[0] = 0x0000;
                    RCByte[1] = 0x0000;
                    RCByte[2] = 0x0000;
                    RCByte[3] = 0x0000;

                    RCBitsCnt = 0;
                    bRet = TRUE;
                }


            break;
            }

            case RCMMOEM_LONGID_MODE|MOUSE_MODE:
            break;
            case RCMMOEM_LONGID_MODE|JOYSTICK_MODE:
            break;
            default:
                bRet = FALSE;
                break;
        }

    }

    if(bRet)
    {
        //printk("_u8PrevKeyCode=%d,*pu8Key=%d,_ePrevKeyProperty=%d",_u8PrevKeyCode,*pu8Key,_ePrevKeyProperty);
        if (_u8PrevKeyCode != *pu8Key)
        {
            _ePrevKeyProperty = E_IR_KEY_PROPERTY_INIT;
        }
        if ((_u8PrevKeyCode == (*pu8Key + 61))||(_u8PrevKeyCode == (*pu8Key + 211)))
        {
            _ePrevKeyProperty = E_IR_KEY_PROPERTY_1st;
        }

        i = _MDrv_IR_GetSystemTime();
        if( _ePrevKeyProperty == E_IR_KEY_PROPERTY_INIT)
        {
            _u8PrevKeyCode     = *pu8Key;
            _ulPrevKeyTime    = i;
            _ePrevKeyProperty  = E_IR_KEY_PROPERTY_1st;
        }
        else if(_ePrevKeyProperty == E_IR_KEY_PROPERTY_1st)
        {
        //printk("i=%ld,_ulPrevKeyTime=%ld",i,_ulPrevKeyTime);
            if( (i - _ulPrevKeyTime) > 150)
            {
                _ulPrevKeyTime = i;
                _ePrevKeyProperty  = E_IR_KEY_PROPERTY_FOLLOWING;
            }
            else
            {
                bRet = FALSE;
            }
        }
        else //E_IR_KEY_PROPERTY_FOLLOWING
        {
            if( (i - _ulPrevKeyTime) > 150)
            {
                _ulPrevKeyTime = i;
            }
            else
            {
                bRet = FALSE;
            }
        }
    }

    return bRet;

#elif (IR_TYPE_SEL == IR_TYPE_CUS08_RC5)

        if(_u8IrRc5Bits > IR_RC5_DATA_BITS)
        {
            if ((_u16IrRc5Data&0xF000)==0x3000)
            {
                if(_u16PreIrRc5Data==_u16IrRc5Data)
                    *pu8Flag = TRUE;

                *pu8Key = _u16IrRc5Data&0x3F;
                *pu8System = (_u16IrRc5Data&0x3C0)>>6;
                _u16PreIrRc5Data=_u16IrRc5Data;
                _u8IrRc5Bits=0;
                bRet = TRUE;
            }
        }

        if(bRet)
        {
            if ( (_u8PrevKeyCode != *pu8Key) || (!*pu8Flag) )
            {
                _ePrevKeyProperty = E_IR_KEY_PROPERTY_INIT;
            }

            i = _MDrv_IR_GetSystemTime();
            if( _ePrevKeyProperty == E_IR_KEY_PROPERTY_INIT)
            {
                _u8PrevKeyCode     = *pu8Key;
                _ulPrevKeyTime    = i;
                _ePrevKeyProperty  = E_IR_KEY_PROPERTY_1st;
            }
            else if(_ePrevKeyProperty == E_IR_KEY_PROPERTY_1st)
            {
                if( (i - _ulPrevKeyTime) > _u32_1stDelayTimeMs)
                {
                    _ulPrevKeyTime = i;
                    _ePrevKeyProperty  = E_IR_KEY_PROPERTY_FOLLOWING;
                }
                else
                {
                    bRet = FALSE;
                }
            }
            else //E_IR_KEY_PROPERTY_FOLLOWING
            {
                if( (i - _ulPrevKeyTime) > _u32_2ndDelayTimeMs)
                {
                    _ulPrevKeyTime = i;
                }
                else
                {
                    bRet = FALSE;
                }
            }
        }

        return bRet;

#elif (IR_TYPE_SEL == IR_TYPE_CUS21SH)

    #if (1)
    bRet = FALSE;

    if(_u32IRCount == 15)
    {
        if(_MDrv_SH_IR_GetKey(pu8Key, pu8System, pu8Flag))
        {
            bRet = TRUE;
        }
        _u32IRCount = 0;
        _u16IRKeyMap = 0;
    }
    #else
    //printk("Linux: %s:%s:%d \n",__FILE__,__FUNCTION__,__LINE__);
    bRet = FALSE;

    for( u8Byte=0; u8Byte<16/*IR_RAW_DATA_NUM*/; u8Byte++)
    {
       u8IRSwModeBuf[u8Byte] = 0;
    }

    if (_u32IRCount< 32)//3+IR_RAW_DATA_NUM*8)
        return FALSE; //not complete yet

    DEBUG_IR(printk("_u32IRCount=%d", _u32IRCount));
    for( u8Byte=0; u8Byte<_u32IRCount; u8Byte++)
    {
       DEBUG_IR(printk(" 0x%x", _u32IRData[u8Byte]));
    }
    printk("\n");
    printk("_u16IRKeyMap:0x%x \n",_u16IRKeyMap);

    if(_bKeyValueHit == FALSE)
    {
        *pu8Key = _MDrv_IR_CUS21SH_ParseKey(_u16IRKeyMap);    // translate for SH

        printk("_u16IRKeyMap:0x%x ==> *pu8Key:%x\n",_u16IRKeyMap,*pu8Key);
        //TODO: Implement repeat code later.
        *pu8Flag = 0;
        //_bKeyValueHit = 1;
        bRet = TRUE;
    }
    #endif

    return bRet;

#else
    for( u8Byte=0; u8Byte<IR_RAW_DATA_NUM; u8Byte++)
    {
       u8IRSwModeBuf[u8Byte] = 0;
    }
    #if (IR_TYPE_SEL == IR_TYPE_MSTAR_DTV)
    if(_u8IRRepeated)//repeate key
    {
        _u8IRRepeatedNum++;
        if (_u8IRRepeatedNum < 5)
        {
            return FALSE;
        }
        _u8IRRepeated = 0;
        _u8IRHeadReceived = 0;//clear head receive flag
        *pu8Key = _u8PrevKeyCode;
        *pu8Flag = 1;
        bRet = TRUE;

        goto done;
    }
    if (_u32IRCount<(2+IR_RAW_DATA_NUM*8))
        return FALSE; //not complete yet
    #else
    if (_u32IRCount< 3+IR_RAW_DATA_NUM*8)
        return FALSE; //not complete yet
    #endif
    DEBUG_IR(printk("_u32IRCount=%d", _u32IRCount));
    for( u8Byte=0; u8Byte<_u32IRCount; u8Byte++)
    {
       DEBUG_IR(printk(" %d", _u32IRData[u8Byte]));
    }

    if( _u32IRData[0] > IR_HDC_LOB && _u32IRData[1] > IR_OFC_LOB+IR_LG01H_LOB && _u32IRData[1] < REG_IR_OFC_UPB+IR_LG01H_UPB )
    {
        pu32IRData = &_u32IRData[2];
        DEBUG_IR(printk(" H1 "));
    }
    else if( _u32IRData[1] > IR_HDC_LOB && _u32IRData[2] > IR_OFC_LOB+IR_LG01H_LOB && _u32IRData[2] < REG_IR_OFC_UPB+IR_LG01H_UPB )
    {
        pu32IRData = &_u32IRData[3];
        DEBUG_IR(printk(" H2 "));
    }
    else
    {
        DEBUG_IR(printk(" invalid leader code\n"));
        bRet = FALSE;
        goto done;
    }

    for( u8Byte=0; u8Byte<IR_RAW_DATA_NUM; u8Byte++)
    {
        for( u8Bit=0; u8Bit<8; u8Bit++)
        {
            u32 u32BitLen = pu32IRData[u8Byte*8+u8Bit];
            u8IRSwModeBuf[u8Byte] >>= 1;

            if( u32BitLen > IR_LG0_LOB && u32BitLen < IR_LG0_UPB ) //0
            {
                u8IRSwModeBuf[u8Byte] |= 0x00;
            }
            else if (u32BitLen > IR_LG1_LOB && u32BitLen < IR_LG1_UPB) //1
            {
                u8IRSwModeBuf[u8Byte] |= 0x80;
            }
            else
            {
                DEBUG_IR(printk(" invalid waveform,0x%x\n",u32BitLen));
                bRet = FALSE;
                goto done;
            }
        }
    }

    if(u8IRSwModeBuf[0] == _u8IRHeaderCode0)
    {
        if(u8IRSwModeBuf[1] == _u8IRHeaderCode1)
        {
            if(u8IRSwModeBuf[2] == (u8)~u8IRSwModeBuf[3])
            {
                *pu8Key = u8IRSwModeBuf[2];
                *pu8Flag = 0;
                bRet = TRUE;
                #if (IR_TYPE_SEL == IR_TYPE_MSTAR_DTV)
                _u8PrevKeyCode = *pu8Key;
                _u8PrevSystemCode = *pu8System;
                _u8IRRepeateDetect = 1;
                _u8IRHeadReceived = 0;
                _u8IRType = 0;
                #endif

                goto done;
            }
        }
    }
    if(u8IRSwModeBuf[0] == _u8IR2HeaderCode0)
    {
        if(u8IRSwModeBuf[1] == _u8IR2HeaderCode1)
        {
            if(u8IRSwModeBuf[2] == (u8)~u8IRSwModeBuf[3])
            {
                *pu8Key = u8IRSwModeBuf[2];
                *pu8Flag = 0;
                bRet = TRUE;
                #if (IR_TYPE_SEL == IR_TYPE_MSTAR_DTV)
                _u8PrevKeyCode = *pu8Key;
                _u8PrevSystemCode = *pu8System;
                _u8IRRepeateDetect = 1;
                _u8IRHeadReceived = 0;
                _u8IRType = 0;
                #endif

                goto done;
            }
        }
    }
    DEBUG_IR(printk(" invalid data\n"));
    bRet = FALSE;

done:
    _u32IRCount = 0;
    return bRet;
#endif
}
#endif

static int ms_ir_init(int bResumeInit)
{

	int result;
    ir_irq_depth = 1;
    _u8PrevSystemCode = 0;


#if (IR_MODE_SEL == IR_MODE_RAWDATA_MODE)
    _u8IRRawModeCount = 0;
#endif

    printk("[ms_ir_init]%d\n",__LINE__);

#if (IR_MODE_SEL == IR_MODE_SWDECODE_MODE)
#if(IR_TYPE_SEL != IR_TYPE_RCMM)
    _u32IRCount = 0;
#endif

#endif


#if (IR_MODE_SEL == IR_MODE_RAWDATA_MODE)
    {
        REG(REG_IR_CTRL) = IR_TIMEOUT_CHK_EN |
                                   IR_INV            |
                                   IR_LG01H_CHK_EN   |
                                   IR_LDCCHK_EN      |
                                   IR_EN;
    }
#else
    {
        REG(REG_IR_CTRL) = IR_TIMEOUT_CHK_EN |
                                   IR_INV            |
                                   IR_RPCODE_EN      |
                                   IR_LG01H_CHK_EN   |
                                   IR_DCODE_PCHK_EN  |
                                   IR_CCODE_CHK_EN   |
                                   IR_LDCCHK_EN      |
                                   IR_EN;
    }
#endif

    _MDrv_IR_Timing();
    REG(REG_IR_CCODE) = ((u16)_u8IRHeaderCode1<<8) | _u8IRHeaderCode0;
    REG(REG_IR_SEPR_BIT_FIFO_CTRL) = 0xF00;
    REG(REG_IR_GLHRM_NUM) = 0x804;
#if (IR_MODE_SEL==IR_MODE_FULLDECODE_MODE)
    REG(REG_IR_GLHRM_NUM) |= (0x3 <<12);
    REG(REG_IR_FIFO_RD_PULSE) |= 0x0020; //wakeup key sel
#elif (IR_MODE_SEL==IR_MODE_RAWDATA_MODE)
    REG(REG_IR_GLHRM_NUM) |= (0x2 <<12);
    REG(REG_IR_FIFO_RD_PULSE) |= 0x0020; //wakeup key sel
#else
    REG(REG_IR_GLHRM_NUM) |= (0x1 <<12);
#if(IR_TYPE_SEL == IR_TYPE_RCMM)

    REG(REG_IR_SEPR_BIT_FIFO_CTRL) |= 0x1 <<12;

#else
	#ifdef IR_INT_NP_EDGE_TRIG	//for N/P edge trigger
    	REG(REG_IR_SEPR_BIT_FIFO_CTRL) |= 0x3 <<12;
    #else
    	REG(REG_IR_SEPR_BIT_FIFO_CTRL) |= 0x2 <<12;
	#endif
#endif
#endif


#if((IR_MODE_SEL==IR_MODE_RAWDATA_MODE) || (IR_MODE_SEL==IR_MODE_FULLDECODE_MODE))
    // Empty the FIFO
    _MDrv_IR_ClearFIFO();
#endif

    _u32_1stDelayTimeMs = 0;
    _u32_2ndDelayTimeMs = 0;
    _ePrevKeyProperty = E_IR_KEY_PROPERTY_INIT;
    memset(&_KeyReceived, 0 , sizeof(_KeyReceived) );
    if(!bResumeInit)
    {
#ifdef CONFIG_MSTAR_PM_SWIR
		//result = request_irq(E_FIQ_IR, MDrv_SWIR_PM51_ISR, SA_INTERRUPT, "IR", NULL); //Doyle change ISR
#else
		result = request_irq(ms_ir->irq_in, _MDrv_IR_ISR, IRQF_SHARED, "IR", ms_ir);
#endif
		ir_irq_depth = 0;
		if (result)
		{
			printk(KERN_ERR"IR IRQ registration ERROR!!\n");

			return -1;
		}

    }

    Chip_Function_Set(CHIP_FUNC_IR_ENABLE,0);


    irq_get_chip(ms_ir->irq_in)->irq_unmask(irq_get_irq_data(ms_ir->irq_in));




#ifdef CONFIG_MSTAR_PM_SWIR
    MBX_Result MbxResult=E_MBX_UNKNOW_ERROR;

    extern int MDrv_MBX_NotifyMsgRecCbFunc(void *);

    if( E_MBX_SUCCESS != MDrv_MBX_Init(SWIR_FILEOPS, E_MBX_CPU_MIPS, E_MBX_ROLE_HK, IR_MBX_TIMEOUT))
	{
        swir_pm_status = -1;
	}
	else
	{
	    MDrv_MBX_Enable(SWIR_FILEOPS, TRUE);
        swir_pm_status = 1;

        MbxResult = MDrv_MBX_RegisterMSG(SWIR_FILEOPS, E_MBX_CLASS_IRKEY_NOWAIT, IR_MBX_QUEUESIZE);

        MDrv_MBX_NotifyMsgRecCbFunc(MDrv_PMSWIR_ReceiveMsg);



        //send msg to PM to unlock interrupt mask
        MBX_Msg MB_Command;
        MS_U8 password[] = {'M', 'S', 'T', 'A', 'R'};
        int i;

        memset((void*)&MB_Command, 0, sizeof(MBX_Msg));
        MB_Command.eRoleID = E_MBX_ROLE_PM;
        MB_Command.eMsgType = E_MBX_MSG_TYPE_INSTANT;
        MB_Command.u8Ctrl = 0;
        MB_Command.u8MsgClass = E_MBX_CLASS_IRKEY_NOWAIT;
        MB_Command.u8Index = E_IR_CPUTo51_CMD_SWIR_INT;
        MB_Command.u8ParameterCount = sizeof(password);
        for(i = 0; i < sizeof(password); i++)
            MB_Command.u8Parameters[i] = password[i];
        MbxResult = MDrv_MBX_SendMsg(SWIR_FILEOPS, &MB_Command);
	}
#endif

    printk("[MDrv_IR_Init]%d\n",__LINE__);


	return 0;
}

static int ms_ir_probe(struct platform_device *pdev)
{

	int retval=-1;
	int irq;// = irq_of_parse_and_map(pdev->dev.of_node, 0);
	int irq_rc;// = irq_of_parse_and_map(pdev->dev.of_node, 1);
	struct resource *mem;

	ms_ir = devm_kzalloc(&pdev->dev, sizeof(*ms_ir), GFP_KERNEL);
	if (!ms_ir)
	{
		retval=-ENOMEM;
		goto DONE;
	}

	irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	irq_rc = irq_of_parse_and_map(pdev->dev.of_node, 1);

	if (!irq)
	{
		printk(KERN_ERR"IR irq error!!");
		return -1;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ms_ir->membase = (void *)(IO_ADDRESS(mem->start));

	ms_ir->irq_in=irq;
	ms_ir->irq_rc=irq_rc;

	platform_set_drvdata(pdev, &ms_ir);

	if((retval=ir_input_init())<0) goto DONE;
	if((retval=ms_ir_init(0))<0) goto DONE;
	ms_ir->IRFlag |= (IRFLAG_IRENABLE|IRFLAG_HWINITED);

DONE:

	printk(KERN_INFO"IR init done!! retval=%d\n",retval);

	return retval;
}


static int ms_ir_remove(struct platform_device *pdev)
{

	struct MS_IR_DEVICE *ir=platform_get_drvdata(pdev);

    destroy_workqueue(key_dispatch_workqueue);
    rc_unregister_device(ir->dev);
    rc_free_device(ir->dev);
	return 0;
}

void MDrv_IR_EnableIR(U8 bEnable)
{
    //irqreturn_t isrStatus;
    bIRPass = !bEnable;
    if (bEnable)
    {
        //isrStatus = request_irq(E_FIQ_IR, _MDrv_IR_ISR, SA_INTERRUPT, "IR", NULL);
        if(ir_irq_depth > 0){

            irq_get_chip(ms_ir->irq_in)->irq_unmask(irq_get_irq_data(ms_ir->irq_in));
            ir_irq_depth--;
        }
    }
    else
    {


        irq_get_chip(ms_ir->irq_in)->irq_mask(irq_get_irq_data(ms_ir->irq_in));
        ir_irq_depth++;
        //free_irq(E_FIQ_IR, NULL);
    }
}


static int ms_ir_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct MS_IR_DEVICE *ir=platform_get_drvdata(pdev);
    if(ir && (ir->IRFlag&IRFLAG_HWINITED))
    {
        if(ir->IRFlag & IRFLAG_IRENABLE)
        {
            MDrv_IR_EnableIR(0);
        }
    }
    return 0;
}

static int ms_ir_resume(struct platform_device *pdev)
{
	struct MS_IR_DEVICE *ir=platform_get_drvdata(pdev);
	if(ir && (ir->IRFlag&IRFLAG_HWINITED))
	{
		ms_ir_init(1);
	    if(ir->IRFlag & IRFLAG_IRENABLE)
	    {
	    	MDrv_IR_EnableIR(1);
	    }
	}
	return 0;
}



static int __init ms_ir_module_init(void)
{
    int retval=0;
    retval = platform_driver_register(&ms_ir_driver);

    return retval;
}

static void __exit ms_ir_module_exit(void)
{
    platform_driver_unregister(&ms_ir_driver);
}


module_init(ms_ir_module_init);
module_exit(ms_ir_module_exit);


MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("IR driver");
MODULE_LICENSE("GPL");




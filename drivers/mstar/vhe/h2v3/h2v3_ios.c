
#include <mhve_ios.h>

#include <h2v3_def.h>
#include <h2v3_reg.h>
#include <h2v3_ios.h>
#include <linux/delay.h>

static int  _set_bank(mhve_ios*, mhve_reg*);
static int  _enc_fire(mhve_ios*, mhve_job*);
static int  _enc_poll(mhve_ios*);
static int  _isr_func(mhve_ios*, int);
static int  _irq_mask(mhve_ios*, int);
static void _ios_free(void* p) { if (p) MEM_FREE(p); }

mhve_ios* mvheios_acquire(char* tags)
{
    h2v3_ios* asic = MEM_ALLC(sizeof(h2v3_ios));
    if (asic)
    {
        mhve_ios* mios = &asic->mios;
        mios->release  = _ios_free;
        mios->set_bank = _set_bank;
        mios->enc_fire = _enc_fire;
        mios->enc_poll = _enc_poll;
        mios->isr_func = _isr_func;
        mios->irq_mask = _irq_mask;
    }
    return (mhve_ios*)asic;
}

static int _set_bank(mhve_ios* mios, mhve_reg* mreg)
{
    h2v3_ios* asic = (h2v3_ios*)mios;
    switch (mreg->i_id)
    {
    case 0:
        asic->bank = mreg->base;
        break;
    case 1:
        asic->conf = mreg->base;
        break;
    default:
        return -1;
    }
    return 0;
}

/* asic->conf(0x1328) 16 bits reg used */
#define REGW(r)             (*((volatile unsigned short*)(r)))
#define REGW_SAVE(r,i,v)    do { REGW(r+(2*i))=(uint16)(v); } while(0)
#define REGW_LOAD(r,i)      ({ uint16 v=REGW(r+(2*i)); (v); })

//#define MIU_PROTECT_LO_ADDRESS      0x00000000+2*1024*1024
//#define MIU_PROTECT_UP_ADDRESS      0x00000000+128*1024*1024
//static void miu_protect(uint16* conf, int on)
//{
////    uint addr = Chip_Phys_to_MIU(MIU_PROTECT_START_ADDRESS);
//    uint addr = MIU_PROTECT_LO_ADDRESS;
//    uint16 w = REGW_LOAD(conf, 0x15) | 0x02;
//
//    if (on)
//    {
//        REGW_SAVE(conf, 0x15, w);
//
//        addr = MIU_PROTECT_LO_ADDRESS;
//        w = addr & 0xFFFF;
//        REGW_SAVE(conf, 0x22, w);
//        w = (addr >> 16) & 0xFFFF;
//        REGW_SAVE(conf, 0x23, w);
//
//        addr = MIU_PROTECT_UP_ADDRESS;
//        w = addr & 0xFFFF;
//        REGW_SAVE(conf, 0x20, w);
//        w = (addr >> 16) & 0xFFFF;
//        REGW_SAVE(conf, 0x21, (addr >> 16) &0xFFFF);
//    }
//    else
//    {
//        w = REGW_LOAD(conf, 0x15) & ~0x02;
//        REGW_SAVE(conf, 0x22, 0);
//        REGW_SAVE(conf, 0x23, 0);
//        REGW_SAVE(conf, 0x20, 0);
//        REGW_SAVE(conf, 0x21, 0);
//    }
//}

static void irq_clr(uint16* conf)
{
    uint16 w = REGW_LOAD(conf,0x04)|0xF00;
    REGW_SAVE(conf,0x04,w);
}

static void irq_msk(uint16* conf, uint16 mask)
{
    uint16 w = REGW_LOAD(conf,0x48)&~0xF;
    w = w | (0xF&mask);
    REGW_SAVE(conf,0x48,w);
}

//#define MIU_BANK    0xFD202400
//static void miu_mask(int mask)
//{
////    uint16* bank = (uint16*)MIU_BANK;
////    uint16 w;
//    volatile unsigned short* reg = (uint16*)0xFD20248C;
//
//    if (mask)
//    {
////        w = REGW_LOAD(bank, 0x23*2) | 0x08;
//        *reg = *reg | 0x08;
//    }
//    else
//    {
////        w = REGW_LOAD(bank, 0x23*2) & ~0x08;
//        *reg = *reg & ~0x08;
//    }
////    REGW_SAVE(bank, 0x23*2, w);
////    w = REGW_LOAD(bank, 0x23*2);
////    printk(KERN_ERR"vhe-0x%p(0x%04X)\n", bank+0x23*2, (uint)w);
////    printk(KERN_ERR"vhe-0x%p(0x%04X)\n", reg, (uint)(*reg));
//}

/* asic->bank(0x1329) 32 bits used */
#define REGL(r)             (*((volatile unsigned int*)(r)))
#define REGL_LO(v)          (((v)>> 0)&0xFFFF)
#define REGL_HI(v)          (((v)>>16)&0xFFFF)
#define REGL_SAVE(r,i,v)    do { REGL(r+(2*i)+0)=REGL_LO(v); REGL(r+(2*i)+1)=REGL_HI(v); } while(0)
#define REGL_LOAD(r,i)      ({ uint l=REGL(r+(2*i));uint h=REGL(r+(2*i)+1); ((h<<16)|l); })

static void _sw_rst(h2v3_ios* asic, h2v3_mir* mirr, int err)
{
//    uint* regs = mirr->regs;
//    uint* base = asic->bank;
    uint16* conf = asic->conf;
    uint16 w = REGW_LOAD(asic->conf, 0x01) & ~0x01;

    if (err)
        printk(KERN_ERR"vhe-%s(%d)\n", __func__, err);

    /* check HW stop */
//    regs[ASIC_ENC] = REGL_LOAD(base, ASIC_ENC);
//    if (regs[ASIC_ENC] & ASIC_ENC_TRIGGER)
//    {
//        printk(KERN_ERR "vhe-hw reset\n");
//        REGL_SAVE(asic->bank, ASIC_ENC, regs[ASIC_ENC] & ~0x01);
//    }
//    miu_mask(1);
    /* sw reset */
    w = REGW_LOAD(conf, 0x01) & ~0x01;
    REGW_SAVE(conf, 0x01, w);
    w = w | 0x01;
    REGW_SAVE(conf, 0x01, w);
}

#define DBG_PRINT_REG (0)
#if DBG_PRINT_REG
static int baseidcs[]=
{
    8,9,10,12,13,14,15,16,18,19,20,21,42,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,83,84,85,86,87,88,89,90,92,93,94,95,96,97,98,99,100,101,102,103,109,110,114,115,116,117,-1
};
#endif
static int _enc_fire(mhve_ios* mios, mhve_job* mjob)
#if 1
{
    h2v3_ios* asic = (h2v3_ios*)mios;
    h2v3_mir* mirr = (h2v3_mir*)mjob;
    uint* base = asic->bank;
    uint* regs = mirr->regs;
    uint tick;
    int i;
#if DBG_PRINT_REG
    int j;
#endif
    asic->pjob = mirr;

    if (mjob->i_code == MHVEJOB_TIME_OUT || mjob->i_code == MHVEJOB_BUF_FULL)
    {
#if DBG_PRINT_REG
        printk("[H2REG]register io:\n");
        for (i = 1; i < H2V3_REG_SIZE; i++)
        {
            for (j = 0; baseidcs[j] >= 0 && baseidcs[j] != i; j++);
                if (baseidcs[j]<0)
                    printk("[H2REG].[%3d]=0x%08X\n",i,regs[i]);
        }
#endif
        if (mjob->i_code == MHVEJOB_TIME_OUT)
        {
            tick = REGL_LOAD(base, ASIC_CYCLE_COUNT);
            printk(KERN_ERR"vhe-to, last tick<%u>\n", tick);
        }
    }

    _sw_rst(asic, mirr, 0);
    irq_clr(asic->conf);
    irq_msk(asic->conf, 0x0);
    /* flush registers */
    regs[ASIC_PRODUCT_REV] = REGL_LOAD(base, ASIC_PRODUCT_REV);
    regs[ASIC_SUPPORT_CFG] = REGL_LOAD(base, ASIC_SUPPORT_CFG);
    regs[ASIC_CYCLE_COUNT] = 0;

    for (i = 1; i < H2V3_REG_SIZE; i++)
    {
//        if (i <= H2V3_MAX_REG_NUM)
            REGL_SAVE(base,i,regs[i]);
    }
#if defined(TRACEREG)
    static int baseidcs[]=
    {
        8,9,10,12,13,14,15,16,18,19,20,21,42,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,83,84,85,86,87,88,89,90,92,93,94,95,96,97,98,99,100,101,102,103,109,110,114,115,116,117,-1
    };
    int j;
    printk("[H2REG]register io:\n");
    for (i = 1; i < H2V3_REG_SIZE; i++)
    {
        for (j=0;baseidcs[j]>=0&&baseidcs[j]!=i;j++);
        if (baseidcs[j]<0)
            printk("[H2REG].[%3d]=0x%08X\n",i,regs[i]);
    }
#endif

    /* MIU write protect */
//    miu_protect(asic->conf, 1);
//    miu_mask(0);
    /* trigger encoder */
    REGL_SAVE(base,ASIC_ENC, regs[ASIC_ENC] | ASIC_ENC_TRIGGER);
    return 0;
}
#else
{
    h2v3_ios* asic = (h2v3_ios*)mios;
    h2v3_mir* mirr = (h2v3_mir*)mjob;
    uint* base = asic->bank;
    uint* regs = mirr->regs;
    int i, j;
    static int baseidcs[]={
    8,9,10,12,13,14,15,16,18,19,20,21,42,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,83,84,85,86,87,88,89,90,92,93,94,95,96,97,98,99,100,101,102,103,109,110,114,115,116,117,-1
    };

    asic->pjob = mirr;
    if (mjob->i_debug)
    {
        regs[ASIC_PRODUCT_REV] = REGL_LOAD(base,ASIC_PRODUCT_REV);
        regs[ASIC_SUPPORT_CFG] = REGL_LOAD(base,ASIC_SUPPORT_CFG);
        printk("[H2REG]register io:\n");
        for (i = 1; i < H2V3_REG_SIZE; i++)
        {
            for (j=0;baseidcs[j]>=0&&baseidcs[j]!=i;j++);
            if (baseidcs[j]<0)
                printk("[H2REG].[%3d]=0x%08X\n",i,regs[i]);
        }
        return 0;
    }

//    sw_rst(asic->conf);
    irq_clr(asic->conf);
    irq_msk(asic->conf, 0x0);
    /* flush registers */
    regs[ASIC_PRODUCT_REV] = REGL_LOAD(base,ASIC_PRODUCT_REV);
    regs[ASIC_SUPPORT_CFG] = REGL_LOAD(base,ASIC_SUPPORT_CFG);
    regs[ASIC_CYCLE_COUNT] = 0;
    for (i = 1; i < H2V3_REG_SIZE; i++)
        REGL_SAVE(base,i,regs[i]);
#if defined(TRACEREG)
    {
    static int baseidcs[]={
    8,9,10,12,13,14,15,16,18,19,20,21,42,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,83,84,85,86,87,88,89,90,92,93,94,95,96,97,98,99,100,101,102,103,109,110,114,115,116,117,-1
    };
    int j;
    printk("[H2REG]register io:\n");
    for (i = 1; i < H2V3_REG_SIZE; i++)
    {
        for (j=0;baseidcs[j]>=0&&baseidcs[j]!=i;j++);
        if (baseidcs[j]<0)
            printk("[H2REG].[%3d]=0x%08X\n",i,regs[i]);
    }
    }
#endif
    /* trigger encoder */
    REGL_SAVE(base,ASIC_ENC,regs[ASIC_ENC]|ASIC_ENC_TRIGGER);
    return 0;
}

#endif

static int _enc_poll(mhve_ios* mios)
{
    h2v3_ios* asic = (h2v3_ios*)mios;
    h2v3_mir* mirr = asic->pjob;
    uint* base = asic->bank;
    uint* regs = mirr->regs;
    uint size, tick, regv;
    int err = ASIC_SLICE_DONE;
    int ret = -1;
    do
    {
        regv = REGL_LOAD(base,ASIC_STATUS);
        if (!(regv&ASIC_STATUS_ALL))
            break;
        regv &= ~ASIC_STATUS_IRQ_LINE;
        REGL_SAVE(base,ASIC_STATUS,regv&~ASIC_STATUS_SLICE_DONE);
        if (regv & ASIC_STATUS_ERROR)
            err = ASIC_ERROR;
        else if (regv & ASIC_STATUS_HW_TIMEOUT)
            err = ASIC_ERR_HWTIMEOUT;
        else if (regv & ASIC_STATUS_FRAME_DONE)
            err = ASIC_FRAME_DONE;
        else if (regv & ASIC_STATUS_BUFF_FULL)
            err = ASIC_ERR_BUFF_FULL;
        else if (regv & ASIC_STATUS_HW_RESET)
            err = ASIC_ERR_HWTIMEOUT;
        else if (regv & ASIC_STATUS_FUSE)
            err = ASIC_ERROR;

        if (err!= ASIC_SLICE_DONE)
            ret = 0;
    }
    while (0);
    if (ret != 0)
        return ret;
    regv = REGL_LOAD(base,ASIC_ENC);
    REGL_SAVE(base,ASIC_ENC,regv&~ASIC_ENC_TRIGGER);
    regs[ASIC_OUTPUT_SIZE] = REGL_LOAD(base,ASIC_OUTPUT_SIZE);
    regs[ASIC_CYCLE_COUNT] = REGL_LOAD(base,ASIC_CYCLE_COUNT);
    H2GetRegVal(regs, HWIF_ENC_OUTBS_BUF_LIMIT, &size);
    H2GetRegVal(regs, HWIF_ENC_HW_PERFORMANCE,  &tick);
    regs[ASIC_NZCOUNT_AVQ] = REGL_LOAD(base,ASIC_NZCOUNT_AVQ);
    regs[ASIC_INTRACU8NUM] = REGL_LOAD(base,ASIC_INTRACU8NUM);
    regs[ASIC_SKIP_CU8NUM] = REGL_LOAD(base,ASIC_SKIP_CU8NUM);
    regs[ASIC_PBF4NRDCOST] = REGL_LOAD(base,ASIC_PBF4NRDCOST);
    regs[ASIC_CTBBITS_BOUND] = REGL_LOAD(base,ASIC_CTBBITS_BOUND);
    regs[ASIC_TOTAL_LCUBITS] = REGL_LOAD(base,ASIC_TOTAL_LCUBITS);
    regs[ASIC_SIGMA0] = REGL_LOAD(base,ASIC_SIGMA0);
    regs[ASIC_SIGMA1] = REGL_LOAD(base,ASIC_SIGMA1);
    regs[ASIC_SIGMA2] = REGL_LOAD(base,ASIC_SIGMA2);
    regs[ASIC_QP_SUM] = REGL_LOAD(base,ASIC_QP_SUM);
    regs[ASIC_QP_NUM] = REGL_LOAD(base,ASIC_QP_NUM);
    regs[ASIC_PIC_COMPLEX] = REGL_LOAD(base,ASIC_PIC_COMPLEX);
    mirr->mjob.i_bits = size*8;
    mirr->mjob.i_tick = tick;
    asic->pjob = NULL;
    return ret;
}

static int _isr_func(mhve_ios* mios, int irq)
{
    h2v3_ios* asic = (h2v3_ios*)mios;
    h2v3_mir* mirr = asic->pjob;
    uint* base = asic->bank;
    uint* regs = mirr->regs;
    uint size, tick, bits = REGL_LOAD(base,ASIC_STATUS);
    int err = MHVEJOB_ENC_FAIL;
    mirr->mjob.i_code = MHVEJOB_OKAY;
    mirr->mjob.i_bits = 0;
    mirr->mjob.i_tick = 0;
    if (!(bits&ASIC_STATUS_IRQ_LINE))
        return 0;
    REGL_SAVE(base, ASIC_STATUS, 0);
    regs[ASIC_OUTPUT_SIZE] = REGL_LOAD(base, ASIC_OUTPUT_SIZE);
    regs[ASIC_CYCLE_COUNT] = REGL_LOAD(base, ASIC_CYCLE_COUNT);
    H2GetRegVal(regs, HWIF_ENC_OUTBS_BUF_LIMIT, &size);
    H2GetRegVal(regs, HWIF_ENC_HW_PERFORMANCE,  &tick);
    if (bits & ASIC_STATUS_FRAME_DONE)
    {
        regs[ASIC_NZCOUNT_AVQ] = REGL_LOAD(base,ASIC_NZCOUNT_AVQ);
        regs[ASIC_INTRACU8NUM] = REGL_LOAD(base,ASIC_INTRACU8NUM);
        regs[ASIC_SKIP_CU8NUM] = REGL_LOAD(base,ASIC_SKIP_CU8NUM);
        regs[ASIC_PBF4NRDCOST] = REGL_LOAD(base,ASIC_PBF4NRDCOST);
        regs[ASIC_CTBBITS_BOUND] = REGL_LOAD(base,ASIC_CTBBITS_BOUND);
        regs[ASIC_TOTAL_LCUBITS] = REGL_LOAD(base,ASIC_TOTAL_LCUBITS);
        regs[ASIC_SIGMA0] = REGL_LOAD(base,ASIC_SIGMA0);
        regs[ASIC_SIGMA1] = REGL_LOAD(base,ASIC_SIGMA1);
        regs[ASIC_SIGMA2] = REGL_LOAD(base,ASIC_SIGMA2);
        regs[ASIC_QP_SUM] = REGL_LOAD(base,ASIC_QP_SUM);
        regs[ASIC_QP_NUM] = REGL_LOAD(base,ASIC_QP_NUM);
        regs[ASIC_PIC_COMPLEX] = REGL_LOAD(base,ASIC_PIC_COMPLEX);
        mirr->mjob.i_bits = size*8;
        mirr->mjob.i_tick = tick;
        err = MHVEJOB_ENC_DONE;
    }
    else if (bits & ASIC_STATUS_BUFF_FULL)
    {
        err = MHVEJOB_BUF_FULL;
        if (bits & ASIC_STATUS_HW_TIMEOUT)
            printk(KERN_ERR "vhe-enc buf full(to)<%u/%u>\n", regs[ASIC_OUTPUT_SIZE], size);
        else
            printk(KERN_ERR "vhe-enc buf full<%u/%u>\n", regs[ASIC_OUTPUT_SIZE], size);
        /* SW reset */
        _sw_rst(asic, mirr, 1);
    }
    else if (bits & ASIC_STATUS_HW_TIMEOUT)
    {
        err = MHVEJOB_TIME_OUT;
        printk(KERN_ERR "vhe-enc to(tick:%u)\n", tick);
        /* SW reset */
        _sw_rst(asic, mirr, 1);
    }
    irq_clr(asic->conf);
    irq_msk(asic->conf, 0xF);
    asic->pjob = NULL;
    mirr->mjob.i_code = err;
    return err;
}

static int _irq_mask(mhve_ios* mios, int msk)
{
    h2v3_ios* asic = (h2v3_ios*)mios;
    irq_clr(asic->conf);
    irq_msk(asic->conf, 0xF);
    return 0;
}

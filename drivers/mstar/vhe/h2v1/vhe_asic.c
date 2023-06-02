
#include <linux/delay.h>

#include <vhe_defs.h>
#include <vhe_regs.h>
#include <vhe_asic.h>

static void _release(void* p) { if (p) kfree(p); }

static int _set_bank(mhve_ios*, mhve_reg*);
static int _enc_fire(mhve_ios*, mhve_job*);
static int _enc_poll(mhve_ios*);
static int _isr_func(mhve_ios*, int);
static int _irq_mask(mhve_ios*, int);

mhve_ios* mvheios_acquire(
    char* tags)
{
    vhe_asic* asic = MEM_ALLC(sizeof(vhe_asic));
    if (asic)
    {
        mhve_ios* mios = &asic->mios;
        mios->release = _release;
        mios->set_bank = _set_bank;
        mios->enc_fire = _enc_fire;
        mios->enc_poll = _enc_poll;
        mios->isr_func = _isr_func;
        mios->irq_mask = _irq_mask;
    }
    return (mhve_ios*)asic;
}

static int _set_bank(
    mhve_ios*   ios,
    mhve_reg*   reg)
{
    int err = 0;
    vhe_asic* asic = (vhe_asic*)ios;
    switch (reg->i_id)
    {
    case 0:
        asic->p_bank = reg->base;
        break;
    case 1:
        asic->p_conf = reg->base;
        break;
    default:
        err = -1;
        break;
    }
    return err;
}

#define REGW(r)         (*((volatile unsigned short*)(r)))
#define REGWR(r,i,v)    do{ REGW(r+(2*i))=(uint16)(v); } while(0)
#define REGRD(r,i)      ({ uint16 v=REGW(r+(2*i));(v);})

#define REGL(r)         (*((volatile unsigned int*)(r)))
#define U32L(v)         (((v)>> 0)&0xFFFF)
#define U32H(v)         (((v)>>16)&0xFFFF)
#define REGLWR(r,i,v)   do{ REGL(r+(2*i)+0)=U32L(v);REGL(r+(2*i)+1)=U32H(v); } while(0)
#define REGLRD(r,i)     ({ uint l=REGL(r+(2*i)+0);uint h=REGL(r+(2*i)+1);((h<<16)|l);})

static void irq_clr(uint16* conf)
{
    uint16 w = REGRD(conf,0x04)|0xF00;
    REGWR(conf,0x04,w);
}

static void irq_msk(uint16* conf, uint16 mask)
{
    uint16 w = REGRD(conf,0x48)&~0xF;
    w = w | (0xF&mask);
    REGWR(conf,0x48,w);
}

static int _enc_fire(
    mhve_ios*   ios,
    mhve_job*   job)
{
    vhe_asic* asic = (vhe_asic*)ios;
    vhe_mirr* mirr = (vhe_mirr*)job;
    uint* base = asic->p_bank;
    uint* regs = mirr->swregs,v;
    int i;
    asic->p_mirr = mirr;
    irq_clr(asic->p_conf);
    irq_msk(asic->p_conf, 0x0);
    /* flush registers */
    for (i = 1; i < VHE_REGS_MAX_SIZE; i++)
    {
        v = regs[i];
        REGLWR(base,i,v);
    }
    /* trigger encoder */
    v = regs[ASIC_ENC];
    REGLWR(base,ASIC_ENC,v|ASIC_ENC_TRIGGER);
    return 0;
}

static int _enc_poll(
    mhve_ios*   ios)
{
    vhe_asic* asic = (vhe_asic*)ios;
    vhe_mirr* mirr = asic->p_mirr;
    uint* base = asic->p_bank;
    uint* regs = mirr->swregs;
    uint size, tick, regv;
    int err = ASIC_SLICE_READY;
    do
    {
        regv = REGLRD(base,ASIC_STATUS);
        if (!(regv&ASIC_STATUS_ALL))
        {
            msleep(3);
            continue;
        }
        regv &= ~ASIC_STATUS_IRQ_LINE;
        REGLWR(base,ASIC_STATUS,regv&~ASIC_STATUS_SLICE_READY);

        if (regv & ASIC_STATUS_ERROR)
            err = ASIC_ERROR;
        else if (regv & ASIC_STATUS_HW_TIMEOUT)
            err = ASIC_ERR_HWTIMEOUT;
        else if (regv & ASIC_STATUS_FRAME_READY)
            err = ASIC_FRAME_READY;
        else if (regv & ASIC_STATUS_BUFFER_FULL)
            err = ASIC_ERR_BUFF_FULL;
        else if (regv & ASIC_STATUS_HW_RESET)
            err = ASIC_ERR_HWTIMEOUT;
        else if (regv & ASIC_STATUS_FUSE)
            err = ASIC_ERROR;
        else
            err = ASIC_SLICE_READY;
    }
    while (ASIC_SLICE_READY == err);
    regv = REGLRD(base,ASIC_ENC);
    REGLWR(base,ASIC_ENC,regv&~ASIC_ENC_TRIGGER);
    regs[ASIC_OUTPUT_SIZE] = REGLRD(base,ASIC_OUTPUT_SIZE);
    regs[ASIC_CYCLE_COUNT] = REGLRD(base,ASIC_CYCLE_COUNT);
    vhe_get_regs(regs, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT, &size);
    vhe_get_regs(regs, HWIF_ENC_HW_PERFORMANCE,           &tick);
    mirr->mjob.i_bits = size*8;
    mirr->mjob.i_tick = tick;
    asic->p_mirr = NULL;
    return err>0?ASIC_OKAY:err;
}

static int _isr_func(
    mhve_ios*   ios,
    int         irq)
{
    vhe_asic* asic = (vhe_asic*)ios;
    vhe_mirr* mirr = asic->p_mirr;
    uint* base = asic->p_bank;
    uint* regs = mirr->swregs;
    uint size, tick, bits = REGLRD(base,ASIC_STATUS);
    if (!(bits&ASIC_STATUS_IRQ_LINE))
        return 0;
    REGLWR(base,ASIC_STATUS,0);
    if (bits & ASIC_STATUS_FRAME_READY)
    {
        regs[ASIC_OUTPUT_SIZE] = REGLRD(base,ASIC_OUTPUT_SIZE);
        regs[ASIC_CYCLE_COUNT] = REGLRD(base,ASIC_CYCLE_COUNT);
        vhe_get_regs(regs, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT, &size);
        vhe_get_regs(regs, HWIF_ENC_HW_PERFORMANCE,           &tick);
        mirr->mjob.i_bits = size*8;
        mirr->mjob.i_tick = tick;
        irq_clr(asic->p_conf);
        irq_msk(asic->p_conf, 0xF);
        asic->p_mirr = NULL;
        return 0;
    }
    if (bits & ASIC_STATUS_BUFFER_FULL)
        printk("vhe:buffer_full\n");
    return 1;
}

static int _irq_mask(
    mhve_ios*   ios,
    int         msk)
{
    vhe_asic* asic = (vhe_asic*)ios;
    irq_clr(asic->p_conf);
    irq_msk(asic->p_conf, 0xF);
    return 0;
}

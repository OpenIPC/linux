
#include <mfe6_def.h>
#include <mfe6_reg.h>
#include <mfe6_ios.h>

static void _release(void* p) { if (p) kfree(p); }

static int _set_bank(mhve_ios*, mhve_reg*);
static int _enc_fire(mhve_ios*, mhve_job*);
static int _enc_poll(mhve_ios*);
static int _isr_func(mhve_ios*, int);
static int _irq_mask(mhve_ios*, int);
static int _busywait(int count)
{
    volatile int i=count;
    while (--i>0) ;
    return 0;
}

mhve_ios* mmfeios_acquire(
    char* tags)
{
    mfe6_ios* asic = MEM_ALLC(sizeof(mfe6_ios));
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
    mfe6_ios* asic = (mfe6_ios*)ios;
    switch (reg->i_id)
    {
    case 0:
        asic->p_base = reg->base;
        break;
    default:
        err = -1;
        break;
    }
    return err;
}

#define REGW(base,idx)          (*(((volatile unsigned short*)(base))+2*(idx)))

#if defined(MMFE_REGS_TRACE)
#define PRINT(s,idx,v)          printk(#s":bank%d[%02x]=%04x\n",(idx&0x80)?1:0,idx&0x7F,v)
#define REGWR(base,idx,v,cmt)   do{REGW(base,idx)=(v);PRINT(w,idx,v);}while(0)
#define REGRD(base,idx,cmt)     ({ushort v=REGW(base,idx);PRINT(r,idx,v); v;})
#else
#define REGWR(base,idx,val,cmt) REGW(base,idx)=(val)
#define REGRD(base,idx,cmt)     REGW(base,idx)
#endif


static void
clr_irq(
    void*       base,
    uint        bits)
{
    REGWR(base,0x1d,bits,"clr irq");
}

static void
msk_irq(
    void*       base,
    uint        bits)
{
    REGWR(base,0x1c,bits,"msk irq");
}

static uint
bit_irq(
    void*       base)
{
    return 0xFF&REGRD(base,0x1e,"bit irq");
}

static void
set_outbs_addr(
    void*       base,
    mfe6_reg*   regs,
    uint32      addr,
    int         size)
{
    uint32  value;
    // Enable set-obuf
    regs->reg3f_s_bspobuf_update_adr = 1;
    REGWR(base,0x3f,regs->reg3f,"ref_mfe_s_bspobuf_update_adr");
    regs->reg3f_s_bspobuf_update_adr = 0;  // write-one-clear
    // address value
    value = addr;
    regs->reg3e = 0;
    // For Safety
    regs->reg3e_s_obuf_write_id_adr = 0;
    REGWR(base,0x3e,regs->reg3e,"write_id_addr");
    REGWR(base,0x3e,regs->reg3e,"write_id_addr");
    regs->reg3c_s_bspobuf_lo = (uint16)(value>> 3);
    regs->reg3d_s_bspobuf_hi = (uint16)(value>>19);
    regs->reg3d_s_obuf_id = 0; // start. addr
    REGWR(base,0x3c,regs->reg3c,"bspobuf addr_lo");
    REGWR(base,0x3d,regs->reg3d,"bspobuf addr_hi");
    regs->reg3e_s_obuf_write_id_adr = 1;
    REGWR(base,0x3e,regs->reg3e,"write_id_addr");
//    value += size;
    value += size-1;
    // For Safety
    regs->reg3e_s_obuf_write_id_adr = 0;
    REGWR(base,0x3e,regs->reg3e,"write_id_addr");
    REGWR(base,0x3e,regs->reg3e,"write_id_addr");
    regs->reg3c_s_bspobuf_lo = (uint16)(value>> 3);
    regs->reg3d_s_bspobuf_hi = (uint16)(value>>19);
    regs->reg3d_s_obuf_id = 1; // end. addr
    REGWR(base,0x3c,regs->reg3c,"bspobuf addr_lo");
    REGWR(base,0x3d,regs->reg3d,"bspobuf addr_hi");
    regs->reg3e_s_obuf_write_id_adr = 1;
    REGWR(base,0x3e,regs->reg3e,"write_id_addr");
    // address set
    regs->reg3b_s_mvobuf_set_adr = 0;
    regs->reg3b_s_mvobuf_fifo_th = 0;
    regs->reg3b_s_bspobuf_fifo_th = 1;
    regs->reg3b_s_bspobuf_set_adr = 1;
    REGWR(base,0x3b,regs->reg3b,"set bsp obuf");
    regs->reg3b_s_bspobuf_set_adr = 0; // HW is write-one-clear
}

static void
put_coded_bits(
    void*       base,
    mfe6_reg*   regs,
    uchar*      code,
    int         size)
{
    int round = (size+15)>>4;
    ushort word = 0;

    regs->reg46 = regs->reg47 = regs->reg48 = 0;

    if (size == 0)
    {
        regs->reg47_s_fdc_bs_count = 0;
        REGWR(base,0x47,regs->reg47,"fdc round count");
        regs->reg46_s_fdc_bs = 0;
        regs->reg47_s_fdc_bs_len = 0;
        regs->reg48_s_fdc_bs_vld = 1;
        REGWR(base,0x46,regs->reg46,"fdc bs ");
        REGWR(base,0x47,regs->reg47,"fdc len");
        REGWR(base,0x48,regs->reg48,"fdc vld");
        regs->reg48_s_fdc_bs_vld = 0; // write-one-clear
        return;
    }
    // Fill the total round regs
    regs->reg47_s_fdc_bs_count = round - 1; // Count from 0
    REGWR(base,0x47,regs->reg47,"fdc round count");

    while (size > 16)
    {
        word = (code[0]<<8) | code[1];
        code += 2;
        size -= 16;

        regs->reg46_s_fdc_bs = word;
        regs->reg47_s_fdc_bs_len = 15;
        regs->reg48_s_fdc_bs_vld = 1;
        REGWR(base,0x46,regs->reg46,"fdc bs ");
        REGWR(base,0x47,regs->reg47,"fdc len");
        REGWR(base,0x48,regs->reg48,"fdc vld");
        regs->reg48_s_fdc_bs_vld = 0; // write-one-clear
    }

    if (size > 0)
    {
        word = code[0]<<8;
        if (size > 8)
            word += code[1];
        regs->reg46_s_fdc_bs = word;
        regs->reg47_s_fdc_bs_len = size;
        regs->reg48_s_fdc_bs_vld = 1;
        REGWR(base,0x46,regs->reg46,"fdc bs ");
        REGWR(base,0x47,regs->reg47,"fdc len");
        REGWR(base,0x48,regs->reg48,"fdc vld");
        regs->reg48_s_fdc_bs_vld = 0; // write-one-clear
    }
}

static int _enc_fire(
    mhve_ios*   ios,
    mhve_job*   job)
{
    mfe6_ios* asic = (mfe6_ios*)ios;
    mfe6_reg* regs = (mfe6_reg*)job;
    void* base = asic->p_base;
    asic->p_regs = regs;
    /* flush regs to mfe-hw */
    msk_irq(base, 0x00);
    // switch to sw mode
    regs->reg03_g_tbc_mode = 0;
    REGWR(base,0x03,regs->reg03,"tbc_mode=0");
    // switch to hw mode
    regs->reg03_g_tbc_mode = 1;
    REGWR(base,0x03,regs->reg03,"tbc_mode=1");
	// sw reset
    regs->reg00_g_soft_rstz = 0;
    REGWR(base,0x00,regs->reg00,"SW reset 0");
    _busywait(800);
    regs->reg00_g_soft_rstz = 1;
    REGWR(base,0x00,regs->reg00,"SW reset 1");
    REGWR(base,0x80,regs->reg80,"pp mode");
    REGWR(base,0x01,regs->reg01,"pic width");
    REGWR(base,0x02,regs->reg02,"pic height");
    REGWR(base,0x90,regs->reg90,"capture width y");
    REGWR(base,0x91,regs->reg91,"capture width c");
    REGWR(base,0xf2,regs->regf2,"roi settings");
    REGWR(base,0xf3,regs->regf3,"plnrldr setting");
    REGWR(base,0xf4,regs->regf4,"plnrldr enable");
    REGWR(base,0xfb,regs->regfb,"roi qmap addr lo");
    REGWR(base,0xfc,regs->regfc,"roi qmap addr hi");
    REGWR(base,0xfd,regs->regfd,"eco");
    REGWR(base,0x03,regs->reg03,"value");
    REGWR(base,0x04,regs->reg04,"er_bs mode threshold");
    REGWR(base,0x05,regs->reg05,"inter prediction perference");
    /* mb-pitch / packed422 / yuv-loader */
    REGWR(base,0x8b,regs->reg8b,"mb pitch");
    REGWR(base,0xd2,regs->regd2,"packed422 mode");
    REGWR(base,0xd6,regs->regd6,"yuv loader");
    /* sambc */
    REGWR(base,0xdb,regs->regdb,"sambc");
    REGWR(base,0xdc,regs->regdc,"sambc");
    REGWR(base,0xdd,regs->regdd,"sambc");
    REGWR(base,0xde,regs->regde,"sambc");
    REGWR(base,0xdf,regs->regdf,"sambc");
    REGWR(base,0xe0,regs->rege0,"sambc");
    REGWR(base,0xe1,regs->rege1,"sambc");
    /* set pict-buffer address */
    REGWR(base,0x06,regs->reg06,"curr luma base addr_lo");
    REGWR(base,0x07,regs->reg07,"curr luma base addr_hi");
    REGWR(base,0x08,regs->reg08,"curr cbcr base addr_lo");
    REGWR(base,0x09,regs->reg09,"curr cbcr base addr_hi");
    REGWR(base,0x0a,regs->reg0a,"ref0 luma base addr_lo");
    REGWR(base,0x0b,regs->reg0b,"ref0 luma base addr_hi");
    REGWR(base,0x0c,regs->reg0c,"ref1 luma base addr_lo");
    REGWR(base,0x0d,regs->reg0d,"ref1 luma base addr_hi");
    REGWR(base,0x0e,regs->reg0e,"ref0 cbcr base addr_lo");
    REGWR(base,0x0f,regs->reg0f,"ref0 cbcr base addr_hi");
    REGWR(base,0x10,regs->reg10,"ref1 cbcr base addr_lo");
    REGWR(base,0x11,regs->reg11,"ref1 cbcr base addr_hi");
    REGWR(base,0x12,regs->reg12,"recn luma base addr_lo");
    REGWR(base,0x13,regs->reg13,"recn luma base addr_hi");
    REGWR(base,0x14,regs->reg14,"recn cbcr base addr_lo");
    REGWR(base,0x15,regs->reg15,"recn cbcr base addr_hi");
    /*!set pict-buffer address */
    /* clock gating */
    REGWR(base,0x16,regs->reg16,"clock gating");
    /* me setting */
    REGWR(base,0x20,regs->reg20,"me partition setting");
    REGWR(base,0x21,regs->reg21,"value");
    REGWR(base,0x22,regs->reg22,"me search range max depth");
    REGWR(base,0x23,regs->reg23,"me mvx");
    REGWR(base,0x24,regs->reg24,"me mvy");
    REGWR(base,0x25,regs->reg25,"FME");
    // GN
    REGWR(base,0x4c,regs->reg4c,"regmfe_s_gn_saddr_lo");
    REGWR(base,0x4d,regs->reg4d,"regmfe_s_gn_saddr_hi");
    // MBR
    REGWR(base,0x26,regs->reg26,"MBR: mbbits");
    REGWR(base,0x27,regs->reg27,"MBR: frame qstep");
    REGWR(base,0x29,regs->reg29,"h264 qp offset");
    REGWR(base,0x2a,regs->reg2a,"QP min/max");
    REGWR(base,0x6e,regs->reg6e,"QStep min");
    REGWR(base,0x6f,regs->reg6f,"QStep max");
    // INTRA UPDATE
    REGWR(base,0x2f,regs->reg2f,"value");
    REGWR(base,0x30,regs->reg30,"value");
    REGWR(base,0x31,regs->reg31,"value");
    REGWR(base,0x32,regs->reg32,"value");
    // MDC
    REGWR(base,0x39,regs->reg39,"value");
    // DBF
    REGWR(base,0x3a,regs->reg3a,"value");

    regs->reg73_g_crc_mode = 0xC;
    regs->reg73_g_debug_tcycle_chk_en = 0x1;
    regs->reg73_g_debug_tcycle_chk_sel = 0x1;
    regs->reg73_g_debug_en = 0; // TEST
    REGWR(base,0x73,regs->reg73,"crc mode");
    REGWR(base,0x2c,regs->reg2c,"last zigzag");
    // IEAP
    REGWR(base,0x2b,regs->reg2b,"ieap");
    // Cross-format wrong regs setting prevention
    REGWR(base,0x18,regs->reg18,"jpe encode mode");
    REGWR(base,0x1b,regs->reg1b,"mpeg4 field dct");
    // regs->reg19 MUST be zero
    REGWR(base,0x19,0,"0/1:MPEG4 enable/disable p skip mode");
    // Prefetch & Low bandwidth mode
    REGWR(base,0x68,regs->reg68,"prefetch & low bandwidth mode");
    // Prefetch
    REGWR(base,0x6d,regs->reg6d,"prefetch mb idle count");
    // Low bandwidth
    REGWR(base,0x6b,regs->reg6b,"low bandwidth: IMI addr_lo");
    REGWR(base,0x6c,regs->reg6c,"low bandwidth: IMI addr_hi");
    // IMI DBF
    REGWR(base,0x4e,regs->reg4e,"DBF: IMI addr_lo");
    REGWR(base,0x4f,regs->reg4f,"DBF: IMI addr_hi");
    // Zmv
    REGWR(base,0x86,regs->reg86,"ZMv base addr lo");
    // penalties
    REGWR(base,0x87,regs->reg87,"ZMv base addr hi & i4x penalty");
    REGWR(base,0x88,regs->reg88,"penalties i16 & inter");
    REGWR(base,0x89,regs->reg89,"penalties i16  planar");
    // Reset any StopAndGo or StopAndDrop setting.
    regs->reg2d_s_txip_sng_mb = 0;
    REGWR(base,0x2d,regs->reg2d,"reg2d_s_txip_sng_mb=0");
    // enable eco item
    REGWR(base,0x7d,regs->reg7d,"reg7d_s_txip_eco0=1");
    // poc
    REGWR(base,0xf7,regs->regf7,"sh->i_poc");
    REGWR(base,0xf8,regs->regf8,"poc_enable & poc_width");
    /* set output address */
    set_outbs_addr(base, regs, regs->outbs_addr, regs->outbs_size);
    /* Enable HW */
    regs->reg00_g_frame_start_sw = 1;
    REGWR(base,0x00,regs->reg00,"frame start");
    regs->reg00_g_frame_start_sw = 0;    // HW is write-one-clear
    /* write slice header */
    put_coded_bits(base, regs, regs->coded_data, regs->coded_bits);
    return 0;
}

static int _enc_poll(
    mhve_ios*   ios)
{
    mfe6_ios* asic = (mfe6_ios*)ios;
    mfe6_reg* regs = asic->p_regs;
    void* base = asic->p_base;
    int err = -1;
    int bit = bit_irq(base);
    if (bit&BIT_ENC_DONE)
        err = 0;
    else if (bit&BIT_BUF_FULL)
        err = 1;
    if (err != 0)
        return err;
    // feedback
    regs->reg28 = REGRD(base,0x28,"sum-qstep:lo");
    regs->reg29 = REGRD(base,0x29,"sum-qstep:hi");
    regs->regf5 = REGRD(base,0xf5,"sum-qstep:lo");
    regs->regf6 = REGRD(base,0xf6,"sum-qstep:hi");
    regs->reg42 = REGRD(base,0x42,"bits size:lo");
    regs->reg43 = REGRD(base,0x43,"bits size:hi");
    regs->reg76 = REGRD(base,0x76,"cycle count0");
    regs->reg77 = REGRD(base,0x77,"cycle count1");
    regs->enc_cycles = ((uint)(regs->reg77&0xFF)<<16) + regs->reg76;
    regs->enc_bitcnt = ((uint)(regs->reg43_s_bsp_bit_cnt_hi)<<16) + regs->reg42_s_bsp_bit_cnt_lo;
    regs->enc_sumpqs = ((uint)(regs->regf6_s_mbr_last_frm_avg_qp_hi)<<16) + regs->regf5_s_mbr_last_frm_avg_qp_lo;
    regs->mjob.i_tick = (int)(regs->enc_cycles);
    regs->mjob.i_bits = (int)(regs->enc_bitcnt - regs->bits_delta);
    clr_irq(base, 0xFF);
    return err;
}

static int _isr_func(
    mhve_ios*   ios,
    int         irq)
{
    mfe6_ios* asic = (mfe6_ios*)ios;
    mfe6_reg* regs = asic->p_regs;
    void* base = asic->p_base;
    uint bit = 0;
    bit = bit_irq(base);
    msk_irq(base, 0xFF);
    //if (bit & BIT_BUF_FULL)
    //{
    //    printk(KERN_ERR"mfe-enc buffer full(%d/%u)\n", regs->outbs_size, (((uint)(regs->reg43_s_bsp_bit_cnt_hi)<<16) + regs->reg42_s_bsp_bit_cnt_lo)/8);
    //    clr_irq(base, 0xFF);
    //    msk_irq(base, 0x00);
    //    return 1;
    //}
    // TODO: handle enc_done/buf_full
    regs->reg28 = REGRD(base,0x28,"avg-qp:lo");
    regs->reg29 = REGRD(base,0x29,"avg-qp:hi");
    regs->regf5 = REGRD(base,0xf5,"avg-qp:lo");
    regs->regf6 = REGRD(base,0xf6,"avg-qp:hi");
    regs->reg42 = REGRD(base,0x42,"bits size:lo");
    regs->reg43 = REGRD(base,0x43,"bits size:hi");
    regs->reg76 = REGRD(base,0x76,"cycle count0");
    regs->reg77 = REGRD(base,0x77,"cycle count1");
    regs->enc_cycles = ((uint)(regs->reg77&0xFF)<<16) + regs->reg76;
    regs->enc_bitcnt = ((uint)(regs->reg43_s_bsp_bit_cnt_hi)<<16) + regs->reg42_s_bsp_bit_cnt_lo;
    regs->enc_sumpqs = ((uint)(regs->regf6_s_mbr_last_frm_avg_qp_hi)<<16) + regs->regf5_s_mbr_last_frm_avg_qp_lo;
    regs->mjob.i_tick = (int)(regs->enc_cycles);
    regs->mjob.i_bits = (int)(regs->enc_bitcnt - regs->bits_delta);
    if (bit & BIT_BUF_FULL)
    {
        printk(KERN_ERR"mfe-enc buffer full(%d/%u)\n", regs->outbs_size, (((uint)(regs->reg43_s_bsp_bit_cnt_hi)<<16) + regs->reg42_s_bsp_bit_cnt_lo)/8);
    }
    clr_irq(base, 0xFF);
    return 0;
}

static int _irq_mask(
    mhve_ios*   ios,
    int         msk)
{
    mfe6_ios* asic = (mfe6_ios*)ios;
    void* base = asic->p_base;
    clr_irq(base, 0xFF);
    msk_irq(base, 0xFF);
    return 0;
}

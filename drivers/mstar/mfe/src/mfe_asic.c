
#include <linux/device.h>
#include <linux/delay.h>

#include <ms_platform.h>

#include <types.h>

#include <mfe_avc.h>
#include <mfe_rc.h>
#include <mfe_regs.h>
#include <mfe_asic.h>
#include <mfe_hdlr.h>

#define REGR(base,idx,cmt)      ms_readw(((uint)base+(idx)*4))
#define REGW(base,idx,val,cmt)  ms_writew(val,((uint)base+(idx)*4))

static void clear_irq(void* iobase, uint bits) {
    REGW(iobase,0x1d,bits,"clear irq");
}

static void set_outbs_addr(void* iobase, mfe_regs* reg, uint32 base, int len) {
    uint32 value;
    // Enable set-obuf
    reg->reg3f_s_bspobuf_update_adr = 1;
    REGW(iobase,0x3f,reg->reg3f,"ref_mfe_s_bspobuf_update_adr");
    reg->reg3f_s_bspobuf_update_adr = 0;  // write-one-clear
    // address value
    value = base;
    reg->reg3e = 0;
    // For Safety
    reg->reg3e_s_obuf_write_id_adr = 0;
    REGW(iobase,0x3e,reg->reg3e,"write_id_addr");
    REGW(iobase,0x3e,reg->reg3e,"write_id_addr");
    reg->reg3c_s_bspobuf_lo = (uint16)(value>> 3);
    reg->reg3d_s_bspobuf_hi = (uint16)(value>>19);
    reg->reg3d_s_obuf_id = 0; // sta. addr
    REGW(iobase,0x3c,reg->reg3c,"bspobuf addr_lo");
    REGW(iobase,0x3d,reg->reg3d,"bspobuf addr_hi");
    reg->reg3e_s_obuf_write_id_adr = 1;
    REGW(iobase,0x3e,reg->reg3e,"write_id_addr");
    value += len;
    // For Safety
    reg->reg3e_s_obuf_write_id_adr = 0;
    REGW(iobase,0x3e,reg->reg3e,"write_id_addr");
    REGW(iobase,0x3e,reg->reg3e,"write_id_addr");
    reg->reg3c_s_bspobuf_lo = (uint16)(value>> 3);
    reg->reg3d_s_bspobuf_hi = (uint16)(value>>19);
    reg->reg3d_s_obuf_id = 1; // end. addr
    REGW(iobase,0x3c,reg->reg3c,"bspobuf addr_lo");
    REGW(iobase,0x3d,reg->reg3d,"bspobuf addr_hi");
    reg->reg3e_s_obuf_write_id_adr = 1;
    REGW(iobase,0x3e,reg->reg3e,"write_id_addr");
    // address set
    reg->reg3b_s_mvobuf_set_adr = 0;
    reg->reg3b_s_mvobuf_fifo_th = 0;
    reg->reg3b_s_bspobuf_fifo_th = 1;
    reg->reg3b_s_bspobuf_set_adr = 1;
    REGW(iobase,0x3b,reg->reg3b,"set bsp obuf");
    reg->reg3b_s_bspobuf_set_adr = 0; // HW is write-one-clear
}

static void put_coded_bits(void* iobase, mfe_regs* reg, uchar* code, int len) {
    int round = (len+15)>>4;
    ushort word = 0;

    reg->reg46 = reg->reg47 = reg->reg48 = 0;

    if (len == 0) {
        reg->reg47_s_fdc_bs_count = 0;
        REGW(iobase,0x47,reg->reg47,"fdc round count");
        reg->reg46_s_fdc_bs = 0;
        reg->reg47_s_fdc_bs_len = 0;
        reg->reg48_s_fdc_bs_vld = 1;
        REGW(iobase,0x46,reg->reg46,"fdc bs ");
        REGW(iobase,0x47,reg->reg47,"fdc len");
        REGW(iobase,0x48,reg->reg48,"fdc vld");
        reg->reg48_s_fdc_bs_vld = 0; // write-one-clear
        return;
    }
    // Fill the total round reg
    reg->reg47_s_fdc_bs_count = round - 1; // Count from 0
    REGW(iobase,0x47,reg->reg47,"fdc round count");

    while (len > 16) {
        word = (code[0]<<8) | code[1];
        code += 2;
        len -= 16;

        reg->reg46_s_fdc_bs = word;
        reg->reg47_s_fdc_bs_len = 15;
        reg->reg48_s_fdc_bs_vld = 1;
        REGW(iobase,0x46,reg->reg46,"fdc bs ");
        REGW(iobase,0x47,reg->reg47,"fdc len");
        REGW(iobase,0x48,reg->reg48,"fdc vld");
        reg->reg48_s_fdc_bs_vld = 0; // write-one-clear
    }

    if (len > 0) {
        word = code[0]<<8;
        if (len > 8) {
            word += code[1];
        }
        reg->reg46_s_fdc_bs = word;
        reg->reg47_s_fdc_bs_len = len;
        reg->reg48_s_fdc_bs_vld = 1;
        REGW(iobase,0x46,reg->reg46,"fdc bs ");
        REGW(iobase,0x47,reg->reg47,"fdc len");
        REGW(iobase,0x48,reg->reg48,"fdc vld");
        reg->reg48_s_fdc_bs_vld = 0; // write-one-clear
    }
}

void mfe_regs_power_on(void* iobase, int on, int power_level) {
    if (!on) {
        REGW(iobase,0x18,0x01,"power off mfe");
    }
    else {
        switch (power_level) {
        case 0:
            REGW(iobase,0x18,0x01,"power off mfe");
            break;
        case 1:
            REGW(iobase,0x18,0x00,"power on very slow");
            break;
        case 2:
            REGW(iobase,0x18,0x04,"power on slow");
            break;
        case 3:
            REGW(iobase,0x18,0x08,"power on medium");
            break;
        case 4:
        default:
            REGW(iobase,0x18,0x12,"power on fast");
            break;
        }
    }
}

void mfe_regs_enc_fire(void* iobase, mfe_regs* regs) {
    /* flush regs to mfe-hw */
    clear_irq(iobase, 0x3F);
    // switch to sw mode
    regs->reg03_g_tbc_mode = 0;
    REGW(iobase,0x03,regs->reg03,"tbc_mode=0");
    // switch to hw mode
    regs->reg03_g_tbc_mode = 1;
    REGW(iobase,0x03,regs->reg03,"tbc_mode=1");
	// sw reset
    regs->reg00_g_soft_rstz = 0;
    REGW(iobase,0x00,regs->reg00,"SW reset 0");
    regs->reg00_g_soft_rstz = 1;
    REGW(iobase,0x00,regs->reg00,"SW reset 1");

    REGW(iobase,0x01,regs->reg01,"pic width");
    REGW(iobase,0x02,regs->reg02,"pic height");
    REGW(iobase,0x03,regs->reg03,"value");
    REGW(iobase,0x04,regs->reg04,"er_bs mode threshold");
    REGW(iobase,0x05,regs->reg05,"inter prediction perference");
    /* set pict-buffer address */
    REGW(iobase,0x06,regs->reg06,"curr luma base addr_lo");
    REGW(iobase,0x07,regs->reg07,"curr luma base addr_hi");
    REGW(iobase,0x08,regs->reg08,"curr cbcr base addr_lo");
    REGW(iobase,0x09,regs->reg09,"curr cbcr base addr_hi");
    REGW(iobase,0x0a,regs->reg0a,"ref0 luma base addr_lo");
    REGW(iobase,0x0b,regs->reg0b,"ref0 luma base addr_hi");
    REGW(iobase,0x0c,regs->reg0c,"ref1 luma base addr_lo");
    REGW(iobase,0x0d,regs->reg0d,"ref1 luma base addr_hi");
    REGW(iobase,0x0e,regs->reg0e,"ref0 cbcr base addr_lo");
    REGW(iobase,0x0f,regs->reg0f,"ref0 cbcr base addr_hi");
    REGW(iobase,0x10,regs->reg10,"ref1 cbcr base addr_lo");
    REGW(iobase,0x11,regs->reg11,"ref1 cbcr base addr_hi");
    REGW(iobase,0x12,regs->reg12,"recn luma base addr_lo");
    REGW(iobase,0x13,regs->reg13,"recn luma base addr_hi");
    REGW(iobase,0x14,regs->reg14,"recn cbcr base addr_lo");
    REGW(iobase,0x15,regs->reg15,"recn cbcr base addr_hi");
    /*!set pict-buffer address */
    /* clock gating */
    REGW(iobase,0x16,regs->reg16,"clock gating");
    /* me setting */
    REGW(iobase,0x20,regs->reg20,"me partition setting");
    REGW(iobase,0x21,regs->reg21,"value");
    REGW(iobase,0x22,regs->reg22,"me search range max depth");
    REGW(iobase,0x23,regs->reg23,"me mvx");
    REGW(iobase,0x24,regs->reg24,"me mvy");
    REGW(iobase,0x25,regs->reg25,"FME");
    // GN
    REGW(iobase,0x4c,regs->reg4c,"regmfe_s_gn_saddr_lo");
    REGW(iobase,0x4d,regs->reg4d,"regmfe_s_gn_saddr_hi");
    // MBR
    REGW(iobase,0x26,regs->reg26,"MBR: mbbits");
    REGW(iobase,0x27,regs->reg27,"MBR: frame qstep");
    REGW(iobase,0x29,regs->reg29,"h264 qp offset");
    REGW(iobase,0x2a,regs->reg2a,"QP min/max");
    REGW(iobase,0x6e,regs->reg6e,"QStep min");
    REGW(iobase,0x6f,regs->reg6f,"QStep max");
    // INTRA UPDATE
    REGW(iobase,0x2f,regs->reg2f,"value");
    REGW(iobase,0x30,regs->reg30,"value");
    REGW(iobase,0x31,regs->reg31,"value");
    REGW(iobase,0x32,regs->reg32,"value");
    // MDC
    REGW(iobase,0x39,regs->reg39,"value");
    // DBF
    REGW(iobase,0x3a,regs->reg3a,"value");

    regs->reg73_g_crc_mode = 0xC;
    regs->reg73_g_debug_tcycle_chk_en = 0x1;
    regs->reg73_g_debug_tcycle_chk_sel = 0x0;
    regs->reg73_g_debug_en = 0; // TEST
    REGW(iobase,0x73,regs->reg73,"crc mode");
    REGW(iobase,0x2c,regs->reg2c,"last zigzag");
    // IEAP
    REGW(iobase,0x2b,regs->reg2b,"ieap");
    // Cross-format wrong reg setting prevention
    REGW(iobase,0x18,regs->reg18,"jpe encode mode");
    REGW(iobase,0x1b,regs->reg1b,"mpeg4 field dct");
    // regs->reg19 MUST be zero
    REGW(iobase,0x19,0,"0/1:MPEG4 enable/disable p skip mode");
    // Prefetch & Low bandwidth mode
    REGW(iobase,0x68,regs->reg68,"prefetch & low bandwidth mode");
    // Prefetch
    REGW(iobase,0x6d,regs->reg6d,"prefetch mb idle count");
    // Low bandwidth
    REGW(iobase,0x6b,regs->reg6b,"low bandwidth: IMI addr_lo");
    REGW(iobase,0x6c,regs->reg6c,"low bandwidth: IMI addr_hi");
    // Reset any StopAndGo or StopAndDrop setting.
    regs->reg2d_s_txip_sng_mb = 0;
    REGW(iobase,0x2d,regs->reg2d,"reg2d_s_txip_sng_mb=0");
    // enable eco item
    REGW(iobase,0x7d,regs->reg7d,"reg7d_s_txip_eco0=1");
    /* set output address */
    set_outbs_addr(iobase, regs, regs->outbs_addr, regs->outbs_size);
    /* Enable HW */
    regs->reg00_g_frame_start_sw = 1;
    REGW(iobase,0x00,regs->reg00,"frame start");
    regs->reg00_g_frame_start_sw = 0;    // HW is write-one-clear
    /* write slice header */
    put_coded_bits(iobase, regs, regs->coded_data, regs->coded_bits);
}

void mfe_regs_enc_done(void* iobase, mfe_regs* regs) {
    regs->reg28 = REGR(iobase,0x28,"avg-qp:lo");
    regs->reg29 = REGR(iobase,0x29,"avg-qp:hi");
    regs->reg42 = REGR(iobase,0x42,"bits size:lo");
    regs->reg43 = REGR(iobase,0x43,"bits size:hi");
}

uint mfe_regs_enc_wait(void* iobase) {
    ushort reg0x1e = 0;

    do {
        reg0x1e = REGR(iobase,0x1e,"get irq");
        if (!reg0x1e) {
            mdelay(8);
        }
    } while (!reg0x1e);

    return reg0x1e;
}


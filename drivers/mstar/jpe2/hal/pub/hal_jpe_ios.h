////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2017 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __HAL_JPE_IOS_H__
#define __HAL_JPE_IOS_H__

#include "hal_jpe_defs.h"

#define    REG_CKG_BASE         0x1F207000
#define    REG_CKG_JPE_BASE     (REG_CKG_BASE+0x6A*4)

typedef struct JpeJob_t
{
    u32     i_code;
    u32     i_tick;
    u32     i_bits;
} JpeJob_t;

#define ADD_JPDE_0_1 //support HW multi-obuf mode
#define ADD_JPDE_1_1 //support Jpeg YUV420 encode mode
#define JPE_MIAW 27//25//24
#define ADDR_HI_BITS    ((JPE_MIAW-6)-16)
#define OUTBUF_HI_BITS    (JPE_MIAW-16)
#define ADDR_MASK (0xffffffff>>(32-(JPE_MIAW-6)))
#define DMA_BOUND 27
#define DMA_HI_BITS (DMA_BOUND-16)
#define LAST_FRAME_AVGQP_HI_BITS (24-16)
#define BITCOUNT_HI_BITS (23-16)

typedef struct
{
    JpeJob_t mjob;
    union
    {
        struct
        {
            u16 nRegJpeGFrameStartSw: 1;   // frame start (1T clk_mfe) (write one clear)
            u16 nRegJpeGSoftRstz: 1;       // software reset; 0/1: reset/not reset
            u16 nRegJpeGEncMode: 1;        // 0/1: JPEG/H263
#if defined(ADD_JPDE_1_1)
            u16 nRegJpeGEncFormat: 1;      // 0/1: YUV420/YUV422
            u16 nRegJpeGFrameIdx: 2;
            u16 nReg0000Dummy: 2;
#else
            u16 nReg0000Dummy: 5;
#endif
            u16 nRegJpeGQscale: 5;         // frame level qscale: [H263]: 1 ~ 31
            u16 nRegJpeGMreq: 1;           // mreq_jpe; miu clock gating enable
            u16 nRegJpeGDctOnlyEn: 1;      // 0: normal function, 1: DCT only to accelerate SW encoding
            u16 nRegJpeGMode: 1;           //0/1: JPE/JPD
        } tReg00;
        u16 nReg00;
    } uReg00;

    union
    {
        struct
        {
            u16 nRegJpeGPicWidth: 12;   // picture width
        } tReg01;
        u16 nReg01;
    } uReg01;

    union
    {
        struct
        {
            u16 nRegJpeGPicHeight: 12;   // picture height
        } tReg02;
        u16 nReg02;
    } uReg02;

    union
    {
        struct
        {
            u16 nRegMfeGErModeDummy: 2;
            u16 nRegMfeGErMbyDummy: 2;
            u16 nRegMfeGPackedModeDummy: 1;
            u16 nRegMfeGQmodeDummy: 1;
            u16 nRegJpeGTbcMode: 1;       // table mode; 0: SW control, 1: HW control
            u16 nRegMfeGFldpicEnDummy: 1;
        } tReg03;
        u16 nReg03;
    } uReg03;

    union
    {
        struct
        {
            u16 nRegJpeGBufYAdr0Low: 16;
        } tReg06;
        u16 nReg06;
    } uReg06;

    union
    {
        struct
        {
            u16 nRegJpeGBufYAdr0High: ADDR_HI_BITS;    // Y buffer base address0
        } tReg07;
        u16 nReg07;
    } uReg07;

    union
    {
        struct
        {
            u16 nRegJpeGBufYAdr1Low: 16;   // Y buffer base address1
        } tReg08;
        u16 nReg08;
    } uReg08;

    union
    {
        struct
        {
            u16 nRegJpeGBufYAdr1High: ADDR_HI_BITS;    // Y buffer base address1
        }tReg09;
        u16 nReg09;
    } uReg09;

    union
    {
        struct
        {
            u16 nRegJpeGBufCAdr0Low: 16;   // C buffer base address0
        } tReg0a;
        u16 nReg0a;
    } uReg0a;

    union
    {
        struct
        {
            u16 nRegJpeGBufCAdr0High: ADDR_HI_BITS;    // C buffer base address0
        } tReg0b;
        u16 nReg0b;
    } uReg0b;

    union
    {
        struct
        {
            u16 nRegJpeGBufCAdr1Low: 16;   // C buffer base address1
        } tReg0c;
        u16 nReg0c;
    } uReg0c;

    union
    {
        struct
        {
            u16 nRegJpeGBufCAdr1High: ADDR_HI_BITS;    // C buffer base address1
        } tReg0d;
        u16 nReg0d;
    } uReg0d;

    union
    {
        struct
        {
            u16 nRegJpeGFsvsWptr: 8; //fsvs to jpe write pointer
        } tReg0f;
        u16 nReg0f;
    } uReg0f;

    union
    {
        struct
        {
            u16 nRegJpeGRowDownCnt: 8;  //row_done counter in clk_jpe domain
        } tReg10;
        u16 nReg10;
    } uReg10;

    union
    {
        struct
        {
            // clock gating
            //[0]: clock gating: clk_jpe (used by JPE); 0: clk_jpe enable, 1: clk_jpe disable
            //[1]: clock gating: clk_jpd (used by JPD); 0: clk_jpd enable, 1: clk_jpd disable
            //[2]: clock gating: clk_miu0 (used by JPE); 0: clk_miu0 enable, 1: clk_miu0 disable
            //[3]: clock gating: clk_miu1 (used by JPD); 0: clk_miu1 enable, 1: clk_miu1 disable
            //[4]: clock gating: clk_viu (used by JPE); 0: clk_viu enable, 1: clk_viu disable"
            u16 nRegJpeGClkGateEn: 5;   //0x0a : gating jpd, 0x15 : gating jpe
        } tReg16;
        u16 nReg16;
    } uReg16;

    // [JPEG]
    union
    {
        struct
        {
            u16 nRegMfeGJpeEncModeDummy: 2;
            u16 nRegJpeGJpeBuffermode: 1;       // JPE buffer mode; 0/1: double buffer mode/frame buffer mode
            u16 nRegJpeSAutoRstWaitCnt: 6;      // the waiting count for regen_soft_rstz and regen_fs_sw generation
            u16 nRegJpeGJpeFsvsMode: 2;         // JPE fsvs generation mode; 0/1/2/3: pure sw/sw+hw/hw wo auto-restart/hw with auto-restart, but fsvs function is removed...
            u16 nReg18Dummy: 4;
            u16 nRegJpeGViuSoftRstz: 1;         //viu software reset; 0/1: reset/not reset
        } tReg18;
        u16 nReg18;
    } uReg18;


    // [IRQ & important IP status checkings]
    union
    {
        struct
        {
            u16 nRegJpeGIrqMask: 8;               // 0/1: irq not-mask/mask
        } tReg1c;
        u16 nReg1c;
    } uReg1c;

    union
    {
        struct
        {
#if 0
            u16 nRegJpeGIrqClr0: 1;               // 0/1: not clear interrupt/clear interrupt 0 (write one clear)
            u16 nRegJpeGIrqClr1: 1;               // 0/1: not clear interrupt/clear interrupt 1 (write one clear)
            u16 nRegJpeGIrqClr2: 1;               // 0/1: not clear interrupt/clear interrupt 2 (write one clear)
            u16 nRegJpeGIrqClr3: 1;               // 0/1: not clear interrupt/clear interrupt 3 (write one clear)
            u16 nRegJpeGIrqClr4: 1;               // 0/1: not clear interrupt/clear interrupt 4 (write one clear)
            u16 nRegJpeGIrqClr5: 1;               // 0/1: not clear interrupt/clear interrupt 5 (write one clear)
            u16 nRegJpeGIrqClr6: 1;               // 0/1: not clear interrupt/clear interrupt 6 (write one clear)
            u16 nRegJpeGIrqClr7: 1;               // 0/1: not clear interrupt/clear interrupt 7 (write one clear)
            u16 nRegJpeGIrqClr8: 1;               // 0/1: not clear interrupt/clear interrupt 8 (write one clear)
#else
            u16 nRegJpeGIrqClr: 9;               // bitwise 0/1: not clear interrupt/clear interrupt 8 (write one clear)
#endif
            u16 nRegJpeGSwrstSafe: 1;           // to indicate there're no miu activities that need to pay attention to
            u16 nRegJpeGLastDoneZ: 1;           // to indicate all miu data has been written
        } tReg1d;
        u16 nReg1d;
    } uReg1d;

    union
    {
        struct
        {
            u16 nRegJpeGIrqCpu: 8;               // status of interrupt on CPU side ({1'b0, netrigger, fs_fail_irq, txipime_out, bspobuf_full_irq, img_buf_full_irq, marb_bspobuf_ful, frame_done_irq})
        } tReg1e;
        u16 nReg1e;
    } uReg1e;

    union
    {
        struct
        {
            u16 nRegJpeGIrqIp: 8;               // status of interrupt on IP side ({1'b0, netrigger, fs_fail_irq, txipime_out, bspobuf_full_irq, img_buf_full_irq, marb_bspobuf_ful, frame_done_irq})
        } tReg1f;
        u16 nReg1f;
    } uReg1f;

    union
    {
        struct
        {
            u16 nRegJpeGIrqForce: 8;               // 0/1: set corresponding interrupt as usual/force corresponding interrupt
        } tReg20;
        u16 nReg20;
    } uReg20;

    union
    {
        struct
        {
            u16 nRegJpeGSramSdEn: 3;               //SRAM SD enable
        } tReg21;
        u16 nReg21;
    } uReg21;

    // [TXIP PIPELINE ]
    // QUAN
    union
    {
        struct
        {
            u16 nRegJpeSQuanIdxLast: 6;       //the index of the last non-zero coefficient in the zig-zag order
            u16 nRegJpeSQuanIdxSwlast: 1;   //software control of the index of the last non-zero coefficient in the zig-zag order; 0/1: disable/enable
        } tReg2c;
        u16 nReg2c;
    } uReg2c;

    // TXIP control & debug
    union
    {
        struct
        {
            u16 nRegJpeSTxipMbx: 9;             //txip mbx
            u16 nRegJpeSTxipSngMb: 1;          //0/1: disable/enable txip controller stop-and-go mechanism using (txip_mbx == reg_mfe_g_debugrig_mbx) & (txip_mby == reg_mfe_g_debugrig_mby)
            u16 nRegJpeSTxipSngSet: 1;         //txip controller stop-and-go mechanism using this register bit: 0/1: go/stop
            u16 nRegJpeSTxipDbfFullHaltEn: 1; //txip controller stop-and-go mechanism using double buffer fullness as criterion; 0/1: disable/enable
        } tReg2d;
        u16 nReg2d;
    } uReg2d;

    union
    {
        struct
        {
            u16 nRegJpeSTxipMby: 9;               // txip mby
        } tReg2e;
        u16 nReg2e;
    } uReg2e;

    union
    {
        struct
        {
            u16 nRegMfeSTxipIrfshMbE1Dummy: 11;
            u16 nRegJpeSTxipTimeoutEn: 1;   // txip time out enable
            u16 nRegJpeSTxipWaitMode: 1;   // txip waiting mode to move to next MB; 0/1: idle count/cycle count
        } tReg32;
        u16 nReg32;
    } uReg32;

    union
    {
        struct
        {
            u16 nRegJpeSTxipIdlecnt: 16;   //wait mode is 0: txip idle count (x 64T)/ wait mode is 1: txip total processing count (x 64T)
        } tReg33;
        u16 nReg33;
    } uReg33;

    union
    {
        struct
        {
            u16 nRegJpeSTxipTimeout: 16;       // txip timeout count (x 64T)
        } tReg34;
        u16 nReg34;
    } uReg34;

    // [ECDB PIPELINE]
    // ECDB control & debug
    union
    {
        struct
        {
            u16 nRegJpeSEcdbMbx: 9;           // ecdb mbx
        } tReg35;
        u16 nReg35;
    } uReg35;

    union
    {
        struct
        {
            u16 nRegJpeSEcdbMby: 9;           // ecdb mby
        } tReg36;
        u16 nReg36;
    } uReg36;

    // BSP/BSPOBUF
    union
    {
        struct
        {
            u16 nRegJpeSBspobufSetAdr: 1;       // set bsp obuf start address(write one clear)
            u16 nRegJpeSBspFdcOffset: 5;           // bsp's fdc offset
#ifdef ADD_JPDE_0_1
            u16 nRegJpeObuffullIrqAtFrameEnd: 1; // [JPDE_0_1] keep obuf full irq at frame end
            u16 nRegJpeObufToggleObuf0Status: 1; // [JPDE_0_1] toggle buf0 status(write one clear)
            u16 nRegJpeObufToggleObuf1Status: 1; // [JPDE_0_1] toggle buf1 status(write one clear)
            u16 nRegJpeObufHwEn: 1;             // [JPDE_0_1] enable HW obuf automatic mechanism
            u16 nRegJpeObufUpdateAdr: 1;        // [JPDE_0_1] update obuf address(write one clear)
            u16 nRegJpeObufAdrRchkSe1: 2;       // [JPDE_0_1] obuf adr read back check selection: 0/1/2/3: s0/e0/s1/e1
            u16 nRegJpeObufAdrRchkEn: 1;        // [JPDE_0_1] enable bspobuf adr read back check through reg_jpe_s_bspobuf_wptr
#endif
        } tReg3b;
        u16 nReg3b;
    } uReg3b;

    union
    {
        struct
        {
#ifdef ADD_JPDE_0_1
            u16 nRegJpeSBspobufAdrLow: 16;   // bsp obuf start address (4 byte unit)
#else
            u16 nRegJpeSBspobufSadrLow: 16;   // bsp obuf start address (4 byte unit)
#endif
        } tReg3c;
        u16 nReg3c;
    } uReg3c;

    union
    {
        struct
        {
#ifdef ADD_JPDE_0_1
            u16 nRegJpeSBspobufAdrHigh: OUTBUF_HI_BITS;    // bsp obuf start address (4 byte unit)
            u16 nRegJpeSBspobufDummyBit: (16 - OUTBUF_HI_BITS - 2);
            u16 nRegJpeSBspobufId:2;
#else
            u16 nRegJpeSBspobufSadrHigh: OUTBUF_HI_BITS;    // bsp obuf start address (4 byte unit)
#endif
        } tReg3d;
        u16 nReg3d;
    } uReg3d;

    union
    {
        struct
        {
#ifdef ADD_JPDE_0_1
            u16 nRegJpeSBspspobufWriteIdAdr: 1;             // [JPDE_0_1] write to this address to enable writing of bspobuf address
#else
            u16 nRegJpeSBspobufEadrLow: 16;                   // bsp obuf end address (4 byte unit)
#endif
        } tReg3e;
        u16 nReg3e;
    } uReg3e;

    union
    {
        struct
        {
#ifdef ADD_JPDE_0_1
            u16 nReg3fReserved;
#else
            u16 nRegJpeSBspobufEadrHigh:OUTBUF_HI_BITS;    // bsp obuf end address (4 byte unit)
#endif
        } tReg3f;
        u16 nReg3f;
    } uReg3f;

    union
    {
        struct
        {

            u16 nRegJpeSBspByteIdx: 2;   // last encoded byte index in 4-byte MIU
#ifdef ADD_JPDE_0_1
#if 0 // Old one and not the same as spec
            u16 nRegJpeObufIdx: 2;  // HW obuf index(0->1->2->3->0->1¡K)
            // SW obuf index(00->01->11->10->00...)
            u16 nRegJpeObuf0Status: 1;
            u16 nRegJpeObuf1Status: 1;
            // reg_jpe_s_obuf_adr_status:6 {fifo_not_full, fifo_not_empty, enable_obufadr_update, obufadr_update_cnt[2:0]}
            u16 nRegJpeObufAdrUpdateCnt: 3;
            u16 nRegJpeEnableObufAdrUpdate: 1;
            u16 nRegJpeFifoNotEmpty: 1;
            u16 nRegJpeFifoNotFull: 1;
#else
            u16 nRegJpeSObufIdx: 2;         // HW obuf index(0->1->2->3->0->1¡K)
            u16 nRegJpeSObufSwIdx: 2;       // SW obuf index(00->01->11->10->00...)
            u16 RegJpeSObufAdrStatus: 6;    // {fifo_not_full, fifo_not_empty, enable_obufadr_update, obufadr_update_cnt[2:0]}
#endif
#endif
        } tReg42;
        u16 nReg42;
    } uReg42;

    union
    {
        struct
        {
            u16 nRegJpeSBspobufWptrLow: 16;   // bspobuf write pointer (4 byte unit)
        } tReg44;
        u16 nReg44;
    } uReg44;

    union
    {
        struct
        {
            u16 nRegJpeSBspobufWptrHigh:OUTBUF_HI_BITS; // bspobuf write pointer (4 byte unit)
        } tReg45;
        u16 nReg45;
    } uReg45;

    // [Table Control]
    union
    {
        struct
        {
            u16 nRegMfeSFdcAckDummy: 1;
            u16 nRegMfeSFdcDoneClrDummy: 1;
            u16 nRegMfeSFdcDoneDummy: 1;
            u16 nReg48Dummy: 8;
            u16 nRegMfeSFdcBsVldDummy: 1;
            u16 nRegJpeSTbcEn: 1;           // set for table read & write ; 1: enable, 0: disable (write one clear)
        } tReg48;
        u16 nReg48;
    } uReg48;

    union
    {
        struct
        {
            u16 nRegJpeSTbcRw: 1;               // table mode; 0: read, 1: write
            u16 nRegJpeSTbcDoneClr: 1;           // table done clear (write one clear)
            u16 nRegJpeSTbcDone: 1;               // table done; indicate to CPU that (1) data has been written to table (2) table output is ready at reg_mfe_sbc_rdata
            u16 nReg49Dummy: 5;
            u16 nRegJpeSTbcAdr: 8;               // table address

        } tReg49;
        u16 nReg49;
    } uReg49;

    union
    {
        struct
        {
            u16 nRegJpeSTbcWdata: 8;           // table write data
        } tReg4a;
        u16 nReg4a;
    } uReg4a;

    union
    {
        struct
        {
            u16 nRegJpeSTbcRdata: 8;           // table read data
        } tReg4b;
        u16 nReg4b;
    } uReg4b;

    // [Miu Arbiter]
    union
    {
        struct
        {
            u16 nRegJpeSDmaMrBurstThd: 5;           //miu read burst bunch up threshold
            u16 nRegJpeSDmaMrTimeout: 3;           //miu read burst timeout
            u16 nRegJpeSDmaMwBurstThd: 5;         //miu write burst bunch up threshold
            u16 nRegJpeSDmaMwTimeout: 3;           //miu write burst timeout
        } tReg54;
        u16 nReg54;
    } uReg54;

    union
    {
        struct
        {
            u16 nRegJpeSDmaMrpriorityThd: 7;       //hardware mfe2mi_rpriority threshold
            u16 nRegJpeSDmaMrpriorityType: 1;       //hardware mfe2mi_rpriority threshold
            u16 nRegJpeSDmaMwpriorityThd: 7;       //hardware mfe2mi_wpriority threshold
            u16 nRegJpeSDmaMwpriorityType: 1;
        } tReg55;
        u16 nReg55;
    } uReg55;

    union
    {
        struct
        {
            u16 reg_jpe_s_dma_mrpriority_sw: 2;           //mfe2mi_rpriority software programmable
            u16 reg5600_dummy: 1;
            u16 reg_jpe_s_dma_mr_nwait_mw: 1;          //miu read not wait mi2mfe_wrdy
            u16 reg_jpe_s_dma_mwpriority_sw: 2;        //mfe2mi_wpriority software programmable
            u16 reg5601_dummy: 1;
            u16 reg_jpe_s_dma_mw_nwait_mr: 1;          //miu read not wait mi2mfe_wrdy
            u16 reg_jpe_s_dma_mr_pending: 4;          //miu read not wait mi2mfe_wrdy
            u16 reg_jpe_s_dma_32b_ad_nswap: 1;          //32bits miu address not swap. only for 32bits mode
            u16 reg_jpe_s_dma_miu_wmode: 1;           //0/1: original miu protocol/new miu protocol(wd_en)
        } tReg56;
        u16 nReg56;
    } uReg56;

    union
    {
        struct
        {
            u16 nRegJpeSDmaMrlastThd: 5;           //auto mfe2mi_rlast threshold
            u16 nRegJpeSDmaMwlastThd: 5;           //auto mfe2mi_wlast threshold
            u16 nReg5700Dummy: 2;
            u16 nRegJpeSDmaBurstSplit: 4;          //0: disable; N: MIU Request IDLE ¡§N¡¨ cycles with every last signal
        } tReg57;
        u16 nReg57;
    } uReg57;

    union
    {
        struct
        {
            u16 nRegJpeSDmaUbound0Low: 16;           //miu write protection, miu upper bound 0  (BSP obuf)
        } tReg58;
        u16 nReg58;
    } uReg58;

    union
    {
        struct
        {
            u16 nRegJpeSDmaUbound0High:DMA_HI_BITS;        //miu write protection, miu upper bound 0  (BSP obuf)
        } tReg59;
        u16 nReg59;
    } uReg59;


    union
    {
        struct
        {
            u16 nRegJpeSDmaLbound0Low: 16;   // miu write protection, miu lower bound 0
        } tReg5a;
        u16 nReg5a;
    } uReg5a;

    union
    {
        struct
        {
            u16 nRegJpeSDmaLbound0High: DMA_HI_BITS;    // miu write protection, miu lower bound 0
            u16 nReg5bDummy:((16 - DMA_HI_BITS) - 2);    // miu write protection, miu lower bound 0
            u16 nRegJpeSDmaMiuOff: 1;                //miu write protection, miu off
            u16 nRegJpeSDmaMiuBoundErr: 1;
        } tReg5b;
        u16 nReg5b;
    } uReg5b;

    union
    {
        struct
        {
            u16 nRegJpeSDmaMrRdffPriorThd: 4;   // 4¡¦hx: high priority when rdff depth < {xxx,2¡¦b00}
            u16 nRegJpeSDmaMrRdffPriorMask: 4;  //Read data fifo depth priority initial mask,  0:disable, 1:release the mask after 8 requests
            u16 nRegJpeSDmaMlastStallOld: 1;    //jpde2mi Last in stall state  0:Guarantee jpde2mi Last, 1:No jpde2mi Last in Stall State
            u16 nRegJpeSDmaRsv5c: 7;            //reserved registers
        } tReg5c;
        u16 nReg5c;
    } uReg5c;

    union
    {
        struct
        {
            u16 nRegJpeSDmaMreqAlwaysActive: 1; //0:dynamic mreq, 1:always set mreq = 1
            u16 nRegJpeSDmaMreqModeSel: 1;      //mreq mode selection  0:clk_miu enabled dynamically, 1:clk_miu enabled from Enstart -> Frame_done
            u16 nRegJpeSDmaMiuBoundEn: 4;       //miu write protection, miu bound enable for write port 0 ~ 3
        } tReg5d;
        u16 nReg5d;
    } uReg5d;

    union
    {
        struct
        {
            u16 nRegGJpeYPitch: 8; // Y frame buffer pitch
            u16 nRegGJpeCPitch: 8; // C frame buffer pitch
        } tReg5e;
        u16 nReg5e;
    } uReg5e;

    union
    {
        struct
        {
            u16 nRegSJpeYuvldrRlastThd: 4;          // YUV loader MIU burst read interval
            u16 nRegSJpeYuvldrMrprioritySw: 2;      // YUV loader MIU read priority SW programmable
            u16 nRegSJpeYuvldrMrpriorityThd: 7;     // YUV loader MIU read priority setting threshold
            u16 nRegSJpeYuvldrMrpriorityType: 1;    // YUV loader MIU read priority type
            u16 nRegSJpeYuvldrImiEn: 1;             // YUV loader IMI enable
            u16 nRegSJpeYuvldrCWwap: 1;             // YUV loader Chroma swap
        } tReg5f;
        u16 nReg5f;
    } uReg5f;

    union
    {
        struct
        {
            u16 nRegJpeSBspobufWlastThd: 4;         // BSP obuf MIU burst write interval
            u16 nRegJpeSBspobufMwpriorityWw: 2;     // BSP obuf MIU write priority SW programmable
            u16 nRegJpeSBspobufMwpriorityThd: 7;    // BSP obuf MIU write priority setting threshold
            u16 nRegJpeSBspobufMwpriorityType: 1;   // BSP obuf MIU write priority type
        } tReg60;
        u16 nReg60;
    } uReg60;

    union
    {
        struct
        {
            u16 nRegJpeSYuvldrImiBound: 9;          // YUV loader IMI bound (unit: Macro)
        } tReg61;
        u16 nReg61;
    } uReg61;

    union
    {
        struct
        {
            u16 nRegJpeTlb: 1;                   // YUV loader IMI bound (unit: Macro)
        } tReg64;
        u16 nReg64;
    } uReg64;

    // [Debug]
    union
    {
        struct
        {
            u16 nRegJpeGPatGenInit: 16;           // pattern generation initial value
        } tReg6f;
        u16 nReg6f;
    } uReg6f;


    union
    {
        struct
        {
            u16 nRegJpeGDebugMode: 5;           // debug mode
            u16 nRegJpeGPatGenEn: 1;           //enable pattern generation
            u16 nRegJpeGDebugTrigCycle: 10;       // wait (8 * reg_mfe_g_debugrig_cycle) cycles

        } tReg70;
        u16 nReg70;
    } uReg70;

    union
    {
        struct
        {
            u16 nRegJpeGDebugTrigMbx: 9;       // debug trigger mbx
        } tReg71;
        u16 nReg71;
    } uReg71;

    union
    {
        struct
        {
            u16 nRegJpeGDebugTrigMby: 9;       // debug trigger mby
        } tReg72;
        u16 nReg72;
    } uReg72;

    union
    {
        struct
        {
            u16 nRegJpeGDebugTrig: 1;           // reg trigger (write one clear)
            u16 nRegJpeGDebugTrigMode: 2;       // debug trigger mode; 0/1/2/3: regrigger/3rd stage (mbx, mby)/frame start
            u16 nRegJpeGDebugEn: 1;               // debug enable
            u16 nRegJpeGCrcMode: 4;               //'h0: Disable,¡¥hc: bsp obuf, 'hd: mc obuf, 'hd: mc obuf
            u16 nRegJpeGDebugTcycleChkEn: 1; //enable total cycle check
            u16 nRegJpeGDebugTcycleChkSel: 1; //select total cycle and report it on reg_mfe_g_crc_result[15:0]
            u16 nRegJpeGRowDownIrqEn: 1; //row done irq enable; 0: disable, 1: enable
            u16 nRegJpeGVsIrqEn: 1; //vs irq enable; 0: disable, 1: enable
        } tReg73;
        u16 nReg73;
    } uReg73;

    union
    {
        struct
        {
            u16 nRegJpeGDebugState0: 16;      // "debug state for TXIP/ECDB submodule {txip2q_en, txip2iq_en, txip2mbr_en, txip2zmem_en, txip2dpcm_en,
        } tReg74;
        u16 nReg74;
    } uReg74;

    union
    {
        struct
        {
            u16 nRegJpeGSwBufferMode: 1;       //0/1: hw/sw buffer mode, but not work anymore...
            u16 nRegJpeGSwRowDone: 1;          //sw row done (1T clk_jpe) (write one clear)
            u16 nRegJpeGSwVsync: 1;            //sw vsync (1T clk_jpe) (write one clear), but not work anymore...
        } tReg75;
        u16 nReg75;
    } uReg75;

    union
    {
        struct
        {
            u16 nRegJpeGCrcResult0: 16;   // CRC64[15..0]
        } tReg76;
        u16 nReg76;
    } uReg76;

    union
    {
        struct
        {
            u16 nRegJpeGCrcResult1: 16;   // CRC64[31..16]
        } tReg77;
        u16 nReg77;
    } uReg77;

    union
    {
        struct
        {
            u16 nRegJpeGCrcResult2: 16;   // CRC64[47..32]
        } tReg78;
        u16 nReg78;
    } uReg78;

    union
    {
        struct
        {
            u16 nRegJpeGCrcResult3: 16;   // CRC64[63..48]
        } tReg79;
        u16 nReg79;
    } uReg79;

    union
    {
        struct
        {
            u16 nRegJpeGBistFail0: 16;
        } tReg7a;
        u16 nReg7a;
    } uReg7a;

    union
    {
        struct
        {
            u16 nRegJpeGDebugOutLow: 16;
        } tReg7b;
        u16 nReg7b;
    } uReg7b;

    union
    {
        struct
        {
        u16 nRegJpeGDebugOutHigh:
            (21 - 16);
        } tReg7c;
        u16 nReg7c;
    } uReg7c;

    //Reserved
    union
    {
        struct
        {
            u16 nRegJpeRsv0: 16; //reserved registers
        } tReg7d;
        u16 nReg7d;
    } uReg7d;

    union
    {
        struct
        {
            u16 nRegJpeRsv1: 16; //reserved registers
        } tReg7e;
        u16 nReg7e;
    } uReg7e;

    union
    {
        struct
        {
            u16 nRegJpeRsv2: 16; //reserved registers
        } tReg7f;
        u16 nReg7f;
    } uReg7f;

    union
    {
        struct
        {
            u16 regClk_jpe_clock_setting: 2;
            u16 regClk_clock_source: 3;
        };
        u16 regClk;
    };
} JpeReg_t;


typedef enum
{
    JPE_HAL_QLEVEL_1 = 0,
    JPE_HAL_QLEVEL_2,
    JPE_HAL_QLEVEL_3,
    JPE_HAL_QLEVEL_4,
    JPE_HAL_QLEVEL_5,
    JPE_HAL_QLEVEL_6,
} JpeHalQLevel_e;

typedef enum
{
    JPE_HAL_IRQ_FRAME_DONE      = 0x00000001,   // bit 0, jpe_framedone
    JPE_HAL_IRQ_BSPOBUF0_FULL   = 0x00000002,   // bit 1, jpe_bspobuf0_full, SW mode: buf full; HW mode: buf0 full
    JPE_HAL_IRQ_IMGBUF_FULL     = 0x00000004,   // bit 2, img_buf_full_irq
    JPE_HAL_IRQ_BSPOBUF1_FULL   = 0x00000008,   // bit 3, jpe_bspobuf1_full, SW mode: not used; HW mode: buf1 full
    JPE_HAL_IRQ_FS_FAIL         = 0x00000020,   // bit 5, fs_fail_irq
    JPE_HAL_IRQ_LESS_ROW_DONE   = 0x00000080,   // bit 7, less_row_down_irq
    JPE_HAL_IRQ_IMI_OVERFLOW    = 0x00000100,   // bit 8, jpe2scl_imi_overflow_int
    JPE_HAL_IRQ_TXIP_TIMEOUT    = 0x10000000,   // bit 4 when reg_jpe_g_vs_irq_en = 0, txip_time_out_irq
    JPE_HAL_IRQ_VS              = 0x20000000,   // bit 4 when reg_jpe_g_vs_irq_en = 1, vs
    JPE_HAL_IRQ_NET_TRIGGER     = 0x40000000,   // bit 4 when reg_jpe_g_row_down_irq_en = 0, net_trigger
    JPE_HAL_IRQ_VIU_ROW_DONE    = 0x80000000,   // bit 6 when reg_jpe_g_row_down_irq_en = 1, viu2jpe_row_done_jpe
    JPE_HAL_IRQ_ALL             = 0xF00001AF    // ALL bits
} JpeHalIrqEvent_e;

typedef enum
{
    JPE_OBUF_0 = 0,
    JPE_OBUF_1 = 1,
} JpeHalOBufOdr_e;

typedef enum
{
    JPE_FSVS_SW_MODE = 0,
    JPE_FSVS_SW_HW_MODE,
    JPE_FSVS_HW_MODE,
    JPE_FSVS_HW_WO_RESTART_MODE,
    JPE_FSVS_HW_WI_RESTART_MODE,
    JPE_FSVS_INVALID,
} JpeFsVsGenerationMode_e;

typedef enum
{
    JPE_HAL_OBUF_SINGLE_MODE = 0x0,
    JPE_HAL_OBUF_MULTI_MODE = 0x1
} JpeHalOutBufMode_e;

typedef enum
{
    JPE_HAL_BUFFER_ROW_MODE = 0x0,
    JPE_HAL_BUFFER_FRAME_MODE = 0x1
} JpeHalInBufMode_e;

typedef enum
{
    JPE_HAL_CODEC_JPEG = 0x1,
    JPE_HAL_CODEC_H263I = 0x2,
    JPE_HAL_CODEC_ENCODE_DCT = 0x4,
} JpeHalCodecFormat_e;

typedef enum
{
    JPE_HAL_SCL_HANDSHAKE_OFF = 0,
    JPE_HAL_SCL_HANDSHAKE_ON
} JpeHalSclHandShake_e;

typedef enum
{
    JPE_HAL_RAW_YUYV = 0x0,
    JPE_HAL_RAW_YVYU = 0x1,
    JPE_HAL_RAW_NV12 = 0x3,
    JPE_HAL_RAW_NV21 = 0x4,
} JpeHalRawFormat_e;

typedef struct
{
    u32 nInBufYAddr[2];
    u32 nInBufCAddr[2];
} JpeHalInBufCfg_t, *pJpeHalInBufCfg;


typedef struct
{
    u32 nOutBufAddr[2];
    u32 nOutBufSize[2];
    u32 nJpeOutBitOffset;
} JpeHalOutBufCfg_t, *pJpeHalOutBufCfg;

typedef struct
{
    u32 nBaseAddr;
    u32 nSize;
    JpeReg_t tJpeReg;

    // Jpeg configuration
    int   nInBufMode;
    int   nRawFormat;
    int   nCodecFormat;

    u32 nWidth;
    u32 nHeight;

    u16 YQTable[64];
    u16 CQTable[64];
    u16 nQScale;

    JpeHalInBufCfg_t    tJpeHalInBufCfg;
    JpeHalOutBufCfg_t   tJpeHalOutBufCfg;
} JpeHalHandle_t;


typedef struct JpeRegIndex_t
{
    int     i_id;
    void*   base;
    int     size;
} JpeRegIndex_t;

typedef struct JpeIosCB_t
{
    void (*release)(void*);                                     //! release this object.
    JPE_IOC_RET_STATUS_e(*setBank)(void*, JpeRegIndex_t*);      //! assign register base pointer.
    JPE_IOC_RET_STATUS_e(*encFire)(void*, void*, JpeJob_t*);    //! process and trigger encoder job.
    JPE_IOC_RET_STATUS_e(*isrFunc)(void*, int);                 //! interrupt service routine.
    JPE_IOC_RET_STATUS_e(*irqMask)(void*, int);                 //! masking irq.
} JpeIosCB_t;

typedef struct JpeIos_t
{
    JpeIosCB_t       mios;

    // Configuration used for encode
    u32              nBaseAddr;
    u16 YQTable[64];
    u16 CQTable[64];
    JpeReg_t tJpeReg;
    JpeHalInBufCfg_t    tJpeHalInBufCfg;
    JpeHalOutBufCfg_t   tJpeHalOutBufCfg;

    // Result after encode
    JpeDevStatus_e   eJpeDevStatus;
    u32              nEncodeSize;
} JpeIos_t;

extern JpeIosCB_t* JpeIosAcquire(char* pTags);
#endif // __HAL_JPE_IOS_H__
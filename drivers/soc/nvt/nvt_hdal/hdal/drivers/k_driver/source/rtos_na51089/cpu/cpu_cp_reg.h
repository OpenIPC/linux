/*
    Header file for cpu co processor register

    This file is the header file that define register for CPU module.

    @file       cpu_cp_reg.h
    @ingroup    mIDrvSys_Core
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/

#ifndef _CPU_CP_REG_H
#define _CPU_CP_REG_H

#include <rcw_macro.h>

#define CA53_GIC_CONFIG_REG_OFS     0x28

REGDEF_BEGIN(CA53_GIC_CONFIG_REG)               //declare register of co processor cause register "CA53_GIC_CONFIG_REG" start
REGDEF_BIT(periphbase, 22)                  //GIC controller base address related to CBAR
REGDEF_BIT(, 2)
REGDEF_BIT(edbgrq, 4)                       //halt coreX bit, each bit represent core0~3
REGDEF_BIT(, 1)
REGDEF_BIT(addr_high_irq_mask, 1)
REGDEF_BIT(, 2)
REGDEF_END(CA53_GIC_CONFIG_REG)                 //declare register of co processor cause register "CA53_GIC_CONFIG_REG" end

#define CA53_INT_STS_REG_OFS        0x30

REGDEF_BEGIN(CA53_INT_STS_REG)                  //declare register of co processor status
REGDEF_BIT(pmuirq, 4)                       //PMU interrupt signal
REGDEF_BIT(exterirq, 1)                     //Error indicator for AXI or CHI transactions with a read or write
//response error condition that cannot be attributed back to a particular core
REGDEF_BIT(, 1)
REGDEF_BIT(l2flushdone, 1)                  //L2 hardware flush complete.
REGDEF_BIT(, 1)
REGDEF_BIT(warmrstreq, 4)                   //Processor warm reset request
REGDEF_BIT(smpen, 4)                        //Indicates whether a core is taking part in coherency.
REGDEF_BIT(cntpnsirq, 4)                    //Non-secure physical timer event
REGDEF_BIT(cntpsirq, 4)                     //Secure physical timer event
REGDEF_BIT(cntvirq, 4)                      //Virtual timer event
REGDEF_BIT(cnthpirq, 4)                     //Hypervisor physical timer event
REGDEF_END(CA53_INT_STS_REG)

#define CA53_CORE_RESET_REG_OFS   0x1004
REGDEF_BEGIN(CA53_CORE_RESET_REG)               //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" start
REGDEF_BIT(CPUPORESET_CORE0, 1)
REGDEF_BIT(l2RSTDISABLE, 1)
REGDEF_BIT(CPUPORESET_CORE1, 1)
REGDEF_BIT(DBGL1RSTDISABLE, 1)
REGDEF_BIT(, 4)
REGDEF_BIT(CORERESET_CORE0, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(CORERESET_CORE1, 1)
REGDEF_BIT(, 14)

REGDEF_BIT(PERIPHRESET, 1)
REGDEF_BIT(PRESETDBG, 1)
REGDEF_BIT(L2RESET, 1)
REGDEF_BIT(ATRESET, 1)
REGDEF_BIT(CTMRESET, 1)
REGDEF_BIT(OSCI12M_FREECNT_RESET, 1)
REGDEF_BIT(FREECNT_CPUDIV8_RESET, 1)

REGDEF_END(CA53_CORE_RESET_REG)                 //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" end

#define CA53_CHANGE_CLOCK_REG_OFS   0x1010
REGDEF_BEGIN(CA53_CHANGE_CLOCK_REG)             //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" start
REGDEF_BIT(WFI_MASK_BIT, 3)                 //There are three source core0/core1/L2 can be wfi, each source occupy 1 bit (bit[2..0])
REGDEF_BIT(, 1)
REGDEF_BIT(L2arm_clksel, 3)                 //L2 arm clock select Clock = clock source / L2arm_clksel +1
REGDEF_BIT(, 1)

REGDEF_BIT(periph_clksel, 3)                //peripheral clock select Clock = clock source / periph_clksel +1
REGDEF_BIT(, 1)
REGDEF_BIT(coresight_clksel, 3)
REGDEF_BIT(, 1)

REGDEF_BIT(armclk_divreg_chg, 1)
REGDEF_BIT(armclk_chg_done_inten, 1)
REGDEF_BIT(, 2)

REGDEF_BIT(trace_regsel, 5)
REGDEF_BIT(trace_src_sel, 1)                //Trace clock source select
//    0: APB clock
//    1: External OSC
REGDEF_BIT(, 3)                             //28

REGDEF_BIT(arm_bypass_sel_grpi, 1)
REGDEF_BIT(l2aclk_bypass_div_sel, 1)
REGDEF_BIT(arm_clk_enb_grpi, 1)             // Trace clock source select
//  0: Disable osprey clock
//  1: Enable osprey clock

REGDEF_END(CA53_CHANGE_CLOCK_REG)               //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" end

#if 0
#define CA53_COMMAND_TYPE_REG_OFS   0x1014
REGDEF_BEGIN(CA53_COMMAND_TYPE_REG)             //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" start
REGDEF_BIT(L2_arb1_ppre_rd_off, 1)          //There are three source core0/core1/L2 can be wfi, each source occupy 1 bit (bit[2..0])
REGDEF_BIT(L2_arb1_postd, 1)                //L2 to ARB1 AXI bridge outstanding enable bit
REGDEF_BIT(L2_arb1_pawba_mode, 1)           //L2 to ARB1 AXI write buffer-able
REGDEF_BIT(L2_arb1_awba_set, 1)             //L2 to ARB1 AXI AWBA fixed setting

REGDEF_BIT(L2_arb2_ppre_rd_off, 1)          //L2 to ARB2 AXI bridge outstanding
REGDEF_BIT(L2_arb2_postd, 1)
REGDEF_BIT(L2_arb2_pawba_mode, 1)
REGDEF_BIT(L2_arb2_awba_set, 1)

REGDEF_BIT(cs_arb1_ppre_rd_off, 1)
REGDEF_BIT(cs_arb1_postd, 1)
REGDEF_BIT(cs_arb1_pawba_mode, 1)
REGDEF_BIT(cs_arb1_awba_set, 1)

REGDEF_BIT(cs_arb2_ppre_rd_off, 1)
REGDEF_BIT(cs_arb2_postd, 1)
REGDEF_BIT(cs_arb2_pawba_mode, 1)
REGDEF_BIT(cs_arb2_awba_set, 1)

REGDEF_BIT(, 16)
REGDEF_END(CA53_COMMAND_TYPE_REG)               //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" end
#else
#define CA9_COMMAND_TYPE_REG_OFS   		0x2000
#define CA9_COMMAND_TYPE_REG_BRG2_OFS   0x3000
REGDEF_BEGIN(CA9_COMMAND_TYPE_REG)             //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" start
REGDEF_BIT(L2_arb1_ppre_rd_off, 1)          //There are three source core0/core1/L2 can be wfi, each source occupy 1 bit (bit[2..0])
REGDEF_BIT(L2_arb1_postd, 1)                //L2 to ARB1 AXI bridge outstanding enable bit
REGDEF_BIT(L2_arb1_pawba_mode, 1)           //L2 to ARB1 AXI write buffer-able
REGDEF_BIT(L2_arb1_awba_set, 1)             //L2 to ARB1 AXI AWBA fixed setting
REGDEF_BIT(, 28)
REGDEF_END(CA9_COMMAND_TYPE_REG)
#endif
REGDEF_BEGIN(CA53_DBGWCRn_REG)                  //declare register of co processor watch point register
REGDEF_BIT(enable, 1)                       //Enable watchpoint n
REGDEF_BIT(pac, 2)                          //PAC:Privilege of access control
REGDEF_BIT(lsc, 2)                          //Load/store access control. This field enables watchpoint matching on the type of access being made. The possible
//  values are:
//  0b01  Match instructions that load from a watchpointed address.
//  0b10  Match instructions that store to a watchpointed address.
//  0b11  Match instructions that load from or store to a watchpointed address.
REGDEF_BIT(bas, 8)                          //Byte address select. Each bit of this field selects whether a byte from within the word or
//double-word addressed by DBGWVR<n>_EL1 is being watched

REGDEF_BIT(hmc, 1)                          //Higher mode control
REGDEF_BIT(ssc, 2)                          //Security state control. Determines the Security states under which a watchpoint debug event for
//watchpoint n is generated
REGDEF_BIT(lbn, 4)                          //Linked breakpoint number. For Linked data address watchpoints, this specifies the index of the
//Context-matching breakpoint linked to.
//On Cold reset, the field reset value is architecturally UNKNOWN
REGDEF_BIT(wt, 1)                           //Watchpoint type. Possible values are:
//  0  Unlinked data address match.
//  1  Linked data address match.

REGDEF_BIT(, 3)
REGDEF_BIT(mask, 5)                         // Address mask. Only objects up to 2GB can be watched using a single mask.
//0b0000  No mask
//0b0001  Reserved
//0b0010  Reserved
//Other values mask the corresponding number of address bits, from 0b00011 masking 3 address bits
//(0x00000007 mask for address) to 0b11111 masking 31 address bits (0x7FFFFFFF mask for address).

REGDEF_BIT(, 3)
REGDEF_END(CA53_DBGWCRn_REG)                //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" end

REGDEF_BEGIN(CA53_PMCR_REG)                     //declare register of co processor watch point register
REGDEF_BIT(E, 1)                            //Enable PMCR
REGDEF_BIT(P, 1)                            //Event counter reset. This bit is WO. The effects of writing to this bit are:
//  0 No action. This is the reset value.
//  1 Reset all event counters, not including PMCCNTR_EL0, to zero.
REGDEF_BIT(C, 1)                            //Clock counter reset. This bit is WO. The effects of writing to this bit are:
//  0 No action. This is the reset value.
//  1 Reset PMCCNTR_EL0 to 0.
REGDEF_BIT(D, 1)                            //Clock divider:
//  0 When enabled, PMCCNTR_EL0 counts every clock cycle. This is the reset value.
//  1 When enabled, PMCCNTR_EL0 counts every 64 clock cycles.
REGDEF_BIT(X, 1)                            //Export enable. This bit permits events to be exported to another debug device, such as a trace macrocell, over an event bus:
//  0 Export of events is disabled. This is the reset value.
//  1 Export of events is enabled.
REGDEF_BIT(DP, 1)                           //Disable cycle counter, PMCCNTR_EL0 when event counting is prohibited:
//  0 Cycle counter operates regardless of the non-invasive debug authentication settings. This is the reset value.
//  1 Cycle counter is disabled if non-invasive debug is not permitted and enabled.

REGDEF_BIT(LC, 1)                           //Long cycle count enable. Determines which PMCCNTR_EL0 bit generates an overflow recorded in PMOVSR[31]. The possible values are:
//  0 Overflow on increment that changes PMCCNTR_EL0[31] from 1 to 0.
//  1 Overflow on increment that changes PMCCNTR_EL0[63] from 1 to 0.

REGDEF_BIT(, 4)              // Reserved

REGDEF_BIT(N, 4)                            // Number of event counters.

REGDEF_BIT(IDCODE, 8)                       // Identification code:
//  0x03 Cortex-A53.
//  This is a read-only field.

REGDEF_BIT(IMP, 8)                          // Implementer code:
//  0x41 ARM.
//  This is a read-only field.


REGDEF_END(CA53_PMCR_REG)                   //declare register of co processor cause register "CA53_PMCR_REG" end


#define CA9_COMMAND_TIMEOUT_REG_OFS   	0x2018
REGDEF_BEGIN(CA9_COMMAND_TIMEOUT_REG)       //declare register of co processor cause register "CA53_CHANGE_CLOCK_REG" start
REGDEF_BIT(en_arb_w, 1)          			//arbiter write path timeout enable
REGDEF_BIT(en_arb_r, 1)                		//arbiter read path timeout enable
REGDEF_BIT(en_axi_aw, 1)           			//axi command write timeout enable (awvalid & ~awready)
REGDEF_BIT(en_axi_w, 1)             		//axi write data timeout enable (wvalid & ~wready)
REGDEF_BIT(en_axi_b,1)						//axi b channel timeout enable (bvalid & ~bready)
REGDEF_BIT(en_axi_ar,1)						//axi command read timeout enable (arvalid & ~arready)
REGDEF_BIT(en_axi_r,1)						//axi read data timeout enable (rvalid & ~rready)
REGDEF_BIT(, 1)
REGDEF_BIT(sw_rst, 1)						//sw reset
REGDEF_BIT(, 23)
REGDEF_END(CA9_COMMAND_TIMEOUT_REG)




#define CNTCR_OFS           0x000
#define CNTSR_OFS           0x004
#define CNTCVL_OFS          0x008
#define CNTCVU_OFS          0x00c
#define CNTFID_OFS          0x020

#define CNTCR_EN            (1 << 0)    /* Enables the counter      */
#define CNTCR_HDBG          (1 << 1)    /* Halt-on-debug            */
#define CNTCR_FCREQ(x)      ((x) << 8)  /* Frequency change request */

#define CNTP_CTL_ENABLE_SHIFT       0
#define CNTP_CTL_IMASK_SHIFT        1
#define CNTP_CTL_ISTATUS_SHIFT      2

#define CNTP_CTL_ENABLE_MASK        1
#define CNTP_CTL_IMASK_MASK         1
#define CNTP_CTL_ISTATUS_MASK       1

#define CNTK_CTL_ACCESS_FROM_EL0    (1<<0)

#endif // _CPU_CP_REG_H


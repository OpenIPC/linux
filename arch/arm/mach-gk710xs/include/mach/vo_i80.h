/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/vo_i80.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __MACH_VO_I80_H
#define __MACH_V0_I80_H

#include <linux/types.h>

#define VO_I80_BASE		(GK_VA_VO)
#define VO_I80_REG(x)	(VO_I80_BASE + (x))

#define REG_VO_I80_DATA_FORMAT                              VO_I80_REG(0xC00) /* read/write */
#define REG_VO_I80_PIC_RESOLUTION                           VO_I80_REG(0xC04) /* read/write */
#define REG_VO_I80_PIXEL_RDWRCMD                            VO_I80_REG(0xC08) /* read/write */
#define REG_VO_I80_CMD_FORMAT                               VO_I80_REG(0xC0C) /* read/write */
#define REG_VO_I80_LCD_RST_PARA1                            VO_I80_REG(0xC10) /* read/write */
#define REG_VO_I80_LCD_RST_PARA2                            VO_I80_REG(0xC14) /* read/write */
#define REG_VO_I80_DELAY_PARA                               VO_I80_REG(0xC18) /* read/write */
#define REG_VO_I80_TWR_TIMING                               VO_I80_REG(0xC1C) /* read/write */
#define REG_VO_I80_TRD_TIMING                               VO_I80_REG(0xC20) /* read/write */
#define REG_VO_I80_TCS_TIMING                               VO_I80_REG(0xC24) /* read/write */
#define REG_VO_I80_POLAR_CTRL                               VO_I80_REG(0xC28) /* read/write */
#define REG_VO_I80_CTRL                                     VO_I80_REG(0xC2C) /* read/write */
#define REG_VO_I80_FRAME_COUNTER                            VO_I80_REG(0xC30) /* read */
#define REG_VO_I80_I80_STATE                                VO_I80_REG(0xC34) /* read */
#define REG_VO_I80_CMD_SRAM_STATE                           VO_I80_REG(0xC38) /* read/write */
#define REG_VO_I80_TCSREF_WT_TIMING                         VO_I80_REG(0xC3C) /* read/write */
#define REG_VO_I80_TCSREF_RD_TIMING                         VO_I80_REG(0xC40) /* read/write */
#define REG_VO_I80_TODH_TIMING                              VO_I80_REG(0xC44) /* read/write */
#define REG_VO_I80_LCD_STATE                                VO_I80_REG(0xC48) /* read */
#define REG_VO_I80_LCD_STATE0                               VO_I80_REG(0xC4C) /* read */
#define REG_VO_I80_LCD_STATE1                               VO_I80_REG(0xC50) /* read */
#define REG_VO_I80_LCD_STATE2                               VO_I80_REG(0xC54) /* read */
#define REG_VO_I80_LCD_STATE3                               VO_I80_REG(0xC58) /* read */
#define REG_VO_I80_LCD_STATE4                               VO_I80_REG(0xC5C) /* read */
#define REG_VO_I80_SRAM_CMDPARA                             VO_I80_REG(0xC80) /* read/write */



#define I80_DESIGN_REF_CLK_FREQ 200//MHz   GK7101S-->200MHz GK8601-->300MHz   

#define I80_CLK_FREQ			200//MHz   GK7101S: FPGA-->100MHz       IC-->200MHz
#define I80_CLK_PERIOD			(1000/I80_CLK_FREQ)//FPGA-->10ns/Cycle  IC-->5ns/cycle

#define MAX_RESET_1ST_H_L_MS	( ((1<<11)-1)*I80_DESIGN_REF_CLK_FREQ/(100*I80_CLK_FREQ) )//FPGA-->40ms   IC-->20ms
#define MAX_RESET_2ND_H_MS		( ((1<<11)-1)*I80_DESIGN_REF_CLK_FREQ/(10*I80_CLK_FREQ) ) //FPGA-->409ms   IC-->204ms
#define MAX_DELAY_MS            ( ((1<<11)-1)*I80_DESIGN_REF_CLK_FREQ/(10*I80_CLK_FREQ) ) //FPGA-->409ms   IC-->204ms
//#define MAX_TRANS_NS            ((1<<9)-1)*I80_CLK_PERIOD //FPGA-->5110ns  IC-->2555ns
//#define MAX_TAS_NS              ((1<<6)-1)*I80_CLK_PERIOD //FPGA-->630ns   IC-->315ns

#define TIMING_PARA_CEIL_NS(x)	(((x)*I80_CLK_FREQ+999)/1000)

#define I80_1ST_H_L_COUNTER     (10*I80_DESIGN_REF_CLK_FREQ)
#define I80_2ND_H_COUNTER       (100*I80_DESIGN_REF_CLK_FREQ)
#define I80_HW_DELAY_COUNTER    (100*I80_DESIGN_REF_CLK_FREQ)

#define TIMING_PARA_FIRST_H_CEIL_MS(x)  ((1000*I80_CLK_FREQ*(x)+I80_1ST_H_L_COUNTER-1)/I80_1ST_H_L_COUNTER)
#define TIMING_PARA_FIRST_L_CEIL_MS(x)  ((1000*I80_CLK_FREQ*(x)+I80_1ST_H_L_COUNTER-1)/I80_1ST_H_L_COUNTER)
#define TIMING_PARA_SECOND_H_CEIL_MS(x) ((1000*I80_CLK_FREQ*(x)+I80_2ND_H_COUNTER-1)/I80_2ND_H_COUNTER)
#define TIMING_PARA_HW_DELAY_CEIL_MS(x) ((1000*I80_CLK_FREQ*(x)+I80_HW_DELAY_COUNTER-1)/I80_HW_DELAY_COUNTER)


#endif  /* __MACH_VO_I80_H */



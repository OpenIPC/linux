/*
 *  arch/arm/mach-GM/include/mach/ftpmu010.h
 *
 *  Copyright (C) 2009 Faraday Technology.
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

#ifndef __FTPMU010_H
#define __FTPMU010_H

#include <mach/platform/pmu.h>

#define PMU010_NAME "ftpmu010"
#define NAME_SZ     20


/*  MACROs for reading clock source
 */
#define PLL1_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_PLL1)
#define PLL2_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_PLL2)
#define PLL3_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_PLL3)
#define PLL4_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_PLL4)
#define PLL5_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_PLL5)
#define AXI_CLK_IN      ftpmu010_get_attr(ATTR_TYPE_AXI)
#define AXI0_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_AXI0)
#define AXI1_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_AXI1)
#define AXI2_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_AXI2)
#define AHB_CLK_IN      ftpmu010_get_attr(ATTR_TYPE_AHB)
#define APB_CLK_IN      ftpmu010_get_attr(ATTR_TYPE_APB)
#define APB0_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_APB0)
#define APB1_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_APB1)
#define APB2_CLK_IN     ftpmu010_get_attr(ATTR_TYPE_APB2)
#define CPU_CLK_IN      ftpmu010_get_attr(ATTR_TYPE_CPU)

typedef enum {
    FTPMU_H264E_0 = 0x1,
    FTPMU_H264D_0,
    FTPMU_H264E_1,
    FTPMU_H264D_1,
    FTPMU_SCALER_0,
    FTPMU_SCALER_1,
    FTPMU_3DI_0,
    FTPMU_3DI_1,
    FTPMU_LCD_0,
    FTPMU_LCD_1,
    FTPMU_LCD_2,
    FTPMU_CAP_0,
    FTPMU_CAP_1,
    FTPMU_CAP_2,
    FTPMU_CAP_3,
    FTPMU_ISP_0,
    FTPMU_ISP_1,
    FTPMU_AES,
    FTPMU_MCP100_0,
    FTPMU_NONE = 0xFFFFF,
} ftpmu010_midx_t;

typedef struct {
    ftpmu010_midx_t midx;   /* module idx */
    int             num;    /* number of clock gate */
    struct {
        unsigned int ofs;
        unsigned int bit_val;    //specify the enable value
        unsigned int bit_mask;   //specify gating clock bits
    } reg[3];
} ftpmu010_gate_clk_t;

/* PMU init function
 * Input parameters: virtual address of PMU
 *  tbl: clock gating table for IPs
 *  pmu_handler: the callback from pmu to reload/reconfigure pmu. NULL if unecessary
 * Return: 0 for success, < 0 for fail
 */
int ftpmu010_init(void __iomem  *base, ftpmu010_gate_clk_t *tbl, void *pmu_handler);

typedef enum
{
    ATTR_TYPE_NONE = 0,
    ATTR_TYPE_PLL1,
    ATTR_TYPE_PLL2,
    ATTR_TYPE_PLL3,
    ATTR_TYPE_PLL4,
    ATTR_TYPE_PLL5,
    ATTR_TYPE_AXI,
    ATTR_TYPE_AHB,
    ATTR_TYPE_APB,
    ATTR_TYPE_APB0,
    ATTR_TYPE_APB1,
    ATTR_TYPE_APB2,
    ATTR_TYPE_CPU,
    ATTR_TYPE_PMUVER,
    ATTR_TYPE_CHIPVER,
    ATTR_TYPE_EPCNT,
    ATTR_TYPE_CPUENUM,  //for: 0 for RC_FA726, 1: RC_FA626, 2: RC_fC7500......
    ATTR_TYPE_AXI0,
    ATTR_TYPE_AXI1,
    ATTR_TYPE_AXI2,
    ATTR_TYPE_IDPIN,
} ATTR_TYPE_T;

/* cpu enumerator in whole system */
typedef enum {
    CPU_RC_FA726    = 0,
    CPU_RC_FA626    = 1,
    CPU_RC_FC7500   = 2,
    CPU_EP0_FA726   = 3,
    CPU_EP0_FA626   = 4,
    CPU_EP0_FC7500  = 5,
} attr_cpu_enum_t;

typedef enum {
    PMUVER_A = 0,
    PMUVER_B,
    PMUVER_C,
    PMUVER_D,
    PMUVER_UNKNOWN,
} pmuver_t;

typedef struct
{
    char          name[NAME_SZ+1];    /* hclk, .... */
    ATTR_TYPE_T   attr_type;
    unsigned int  value;
} attrInfo_t;

/* register attribute
 */
int ftpmu010_register_attr(attrInfo_t *attr);
int ftpmu010_deregister_attr(attrInfo_t *attr);
/* get attribute value
 * return value: 0 for fail, > 0 for success
 */
unsigned int ftpmu010_get_attr(ATTR_TYPE_T attr);


/*
 * Structure for pinMux
 */
typedef struct
{
    unsigned int  reg_off;    /* register offset from PMU base */
    unsigned int  bits_mask;  /* bits this module covers */
    unsigned int  lock_bits;  /* bits this module locked */
    unsigned int  init_val;   /* initial value */
    unsigned int  init_mask;  /* initial mask */
} pmuReg_t;

typedef struct
{
    char        name[NAME_SZ+1];    /* module name length */
    int         num;                /* number of register entries */
    ATTR_TYPE_T clock_src;          /* which clock this module uses */
    pmuReg_t    *pRegArray;         /* register array */
} pmuRegInfo_t;

/* register/de-register the register table
 * return value:
 *  give an unique fd if return value >= 0, otherwise < 0 if fail.
 */
int ftpmu010_register_reg(pmuRegInfo_t *info);
int ftpmu010_deregister_reg(int fd);

/* lock/unlock/replace the bits in lock_bits field
 * return value:
 *  0 for success, < 0 for fail
 */
int ftpmu010_add_lockbits(int fd, unsigned int reg_off, unsigned int lock_bits);
int ftpmu010_del_lockbits(int fd, unsigned int reg_off, unsigned int unlock_bits);
int ftpmu010_update_lockbits(int fd, unsigned int reg_off, unsigned int new_lock_bits);
/* @int ftpmu010_bits_is_locked(int reg_off, unsigned int bits)
 * @Purpose: This function is used to check if the bits are locked by any module or not.
 * @Parameter:
 *   reg_off: register offset
 *   bits: the checked bits
 * @Return:
 *      If the any bit in bits is locked, then the returned value will be 0
 *      otherwise, -1 is returned to indicates all bits are available.
 *
 */
int ftpmu010_bits_is_locked(int fd, unsigned int reg_off, unsigned int bits);

/* PMU register read/write
 */
unsigned int ftpmu010_read_reg(unsigned int reg_off);
/* return value < 0 for fail */
int ftpmu010_write_reg(int fd, unsigned int reg_off, unsigned int val, unsigned int mask);

/* Purpose: calculate the divisor by input clock
 * Input: fd, in_clock, near
 * Output: None
 * Return: quotient if > 0, 0 for fail
 * Note: The return value will take the nearest value if near is 1. For example: 17.6 will be treated as 18,
 *          but 17.4 will be treated as 17.
 */
unsigned int ftpmu010_clock_divisor(int fd, unsigned int in_clock, int near);

/* Purpose: calculate the divisor by input clock attribute
 * Input: clock_src, in_clock, near
 * Output: None
 * Return: quotient if > 0, 0 for fail
 * Note: The return value will take the nearest value if near is 1. For example: 17.6 will be treated as 18,
 *          but 17.4 will be treated as 17.
 */
unsigned int ftpmu010_clock_divisor2(ATTR_TYPE_T clock_src, unsigned int in_clock, int near);

/* @Purpose: request the pmu PINs
 * @Parameter:
 *      fd: unique identifier
 *      reg_off: register offset
 *      req_bits: request registers
 *      b_wait: 1 for blocking until the resource is available
 * Output: None
 * Return: 0 for success, !0 for fail
 */
int ftpmu010_request_pins(int fd, unsigned int reg_off, unsigned int req_bits, int b_wait);


/* Purpose: release the pmu PINs
 * Input: fd, reg_off, req_bits
 * Output: None
 * Return: 0 for success, !0 for fail
 */
int ftpmu010_release_pins(int fd, unsigned int reg_off, unsigned int req_bits);

/* Purpose: check if the PINs was requested by others except myself.
 * Input: fd, reg_off, req_bits
 * Output: None
 * Return: those pins occupied by others. zero indicates the pin are available.
 */
unsigned int ftpmu010_pins_is_requested(int fd, unsigned int reg_off, unsigned int req_bits);
/*
 * The following is used for specific functionality to PMU
 */
#define FUNC_TYPE_RELOAD_ATTR   0
#define FUNC_TYPE_INTR_CLR      1
#define FUNC_TYPE_INTR_FIRE     2

/*
 * The following function are used for module to send/clear interrupt.
 * return: 0 for success, -1 for fail
 */
/* send interrupt from pmu */
static inline int ftpmu010_trigger_intr(int irq)
{
    extern int ftpmu010_pmu_doaction(u32 cmd, u32 data1, u32 data2);

    return ftpmu010_pmu_doaction(FUNC_TYPE_INTR_FIRE , irq, 0);
}
/* clear interupt in pmu */
static inline int ftpmu010_clear_intr(int irq)
{
    extern int ftpmu010_pmu_doaction(u32 cmd, u32 data1, u32 data2);

    return ftpmu010_pmu_doaction(FUNC_TYPE_INTR_CLR , irq, 0);
}
/* force the attribute refresh again */
static inline int ftpmu010_reload_attr(ATTR_TYPE_T attr_type)
{
    extern int ftpmu010_pmu_doaction(u32 cmd, u32 data1, u32 data2);

    return ftpmu010_pmu_doaction(FUNC_TYPE_RELOAD_ATTR , attr_type, 0);
}

#endif	/* __FTPMU010_H */

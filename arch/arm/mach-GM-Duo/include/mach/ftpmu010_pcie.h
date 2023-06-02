/*
 *  arch/arm/mach-GM-Duo/include/mach/ftpmu010_pcie.h
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

#ifndef __FTPMU010_PCIE_H__
#define __FTPMU010_PCIE_H__

#include <mach/ftpmu010.h>

#define GM8312_CHIP_ID  0x831210
#define PCIE_NAME_SZ    20

/*  MACROs for reading clock source
 */
#define PCIE_PLL1_CLK_IN    ftpmu010_pcie_get_attr(ATTR_TYPE_PCIE_PLL1)
#define PCIE_AXI_CLK_IN     ftpmu010_pcie_get_attr(ATTR_TYPE_PCIE_AXI)
#define PCIE_AHB_CLK_IN     ftpmu010_pcie_get_attr(ATTR_TYPE_PCIE_AHB)
#define PCIE_APB_CLK_IN     ftpmu010_pcie_get_attr(ATTR_TYPE_PCIE_APB)

typedef enum {
    FTPMU_PCIE_SATA_0 = 0x1,
    FTPMU_PCIE_NONE = 0xFFFFF,
} ftpmu010_pcie_midx_t;

typedef struct {
    ftpmu010_pcie_midx_t midx;   /* module idx */
    int             num;    /* number of clock gate */
    struct {
        unsigned int ofs;
        unsigned int bit_val;    //specify the enable value
        unsigned int bit_mask;   //specify gating clock bits
    } reg[3];
} ftpmu010_pcie_gate_clk_t;

typedef enum
{
    ATTR_TYPE_PCIE_NONE = 0,
    ATTR_TYPE_PCIE_PLL1,
    ATTR_TYPE_PCIE_AXI,
    ATTR_TYPE_PCIE_AHB,
    ATTR_TYPE_PCIE_APB,
    ATTR_TYPE_PCIE_PMUVER,
} ATTR_PCIE_TYPE_T;

typedef struct
{
    char                name[PCIE_NAME_SZ + 1];    /* hclk, .... */
    ATTR_PCIE_TYPE_T    attr_type;
    unsigned int        value;
} attrPcieInfo_t;

/* register attribute
 */
int ftpmu010_pcie_register_attr(attrPcieInfo_t *attr);
int ftpmu010_pcie_deregister_attr(attrPcieInfo_t *attr);
/* get attribute value
 * return value: 0 for fail, > 0 for success
 */
unsigned int ftpmu010_pcie_get_attr(ATTR_PCIE_TYPE_T attr);

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
} pmuPcieReg_t;

typedef struct
{
    char                name[PCIE_NAME_SZ + 1];    /* module name length */
    int                 num;                /* number of register entries */
    ATTR_PCIE_TYPE_T    clock_src;          /* which clock this module uses */
    pmuPcieReg_t        *pRegArray;         /* register array */
} pmuPcieRegInfo_t;

/* PMU init function
 * Input parameters: virtual address of PMU
 *  tbl: clock gating table for IPs
 *  pmu_handler: the callback from pmu to reload/reconfigure pmu. NULL if unecessary
 * Return: 0 for success, < 0 for fail
 */
int ftpmu010_pcie_init(void __iomem  *base, ftpmu010_pcie_gate_clk_t *tbl, void *pmu_handler);

/* register/de-register the register table
 * return value:
 *  give an unique fd if return value >= 0, otherwise < 0 if fail.
 */
int ftpmu010_pcie_register_reg(pmuPcieRegInfo_t *info);
int ftpmu010_pcie_deregister_reg(int fd);

unsigned int ftpmu010_pcie_read_reg(unsigned int reg_off);
int ftpmu010_pcie_write_reg(int fd, unsigned int reg_off, unsigned int val, unsigned int mask);

#endif	/* __FTPMU010_PCIE_H__ */

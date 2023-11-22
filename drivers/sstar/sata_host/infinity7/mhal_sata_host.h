/*
 * mhal_sata_host.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _MHAL_SATA_HOST_H_
#define _MHAL_SATA_HOST_H_

#include <ms_platform.h>
#include <registers.h>

#define SSTAR_SATA_DTS_NAME  "sstar,sata"
#define SSTAR_SATA1_DTS_NAME "sstar,sata1"

//#define SSTAR_RIU_BASE      0xFD000000
//#define SSTAR_RIU_BASE      (0x1F000000 + 0xFFFFFF80DE000000)

#define REG_PM_SLEEP_BASE 0x000E00
#define REG_PM_TOP_BASE   0x001E00
#define REG_CLKGEN_BASE   0x103800
#define REG_SATA2_CLKGEN2 0x103F00

#ifndef BIT // for Linux_kernel type, BIT redefined in <linux/bitops.h>
#define BIT(_bit_) (1 << (_bit_))
#endif
#define BMASK(_bits_)               (BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))

#define SATA_REG_ADDR(x, y) (GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, x) + y)

#define REG_PD_XTAL_HV           (REG_PM_SLEEP_BASE + (0x3D << 1))
#define SATA_HV_CRYSTAL_CLK_MASK BMASK(5 : 0)

#define REG_CHIP_INFORM_SHADOW (REG_PM_TOP_BASE + (0x67 << 1))

#define REG_CKG_SATA_FCLK        (REG_CLKGEN_BASE + (0x46 << 1))
#define REG_CKG_SATA_PM          (REG_CLKGEN_BASE + (0x6C << 1))
#define REG_CKG_SATA_AXI         (REG_CLKGEN_BASE + (0x6E << 1))
#define CKG_SATA_FCLK_PHY_GATED  BIT(0)
#define CKG_SATA_FCLK_PHY_INVERT BIT(1)
#define CKG_SATA_FCLK_PHY_MASK   BMASK(3 : 2)
#define CKG_SATA_FCLK_108MHZ     (0 << 2)
#define CKG_SATA_FCLK_GATED      BIT(8)
#define CKG_SATA_FCLK_INVERT     BIT(9)
#define CKG_SATA_FCLK_MASK       BMASK(11 : 10)
#define CKG_SATA_FCLK_432MHZ     (0 << 2)

#define SSTAR_MAIL_BOX 0x100400

#define BASE_REG_SATA0_SNPS_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3400)
#define BASE_REG_SATA0_SNPS_PORT0_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3480)
#define BASE_REG_SATA0_MISC_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143A00)
#define BASE_REG_SATA0_PHYD_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143B00)
#define BASE_REG_SATA0_PHYA_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143C00)
#define BASE_REG_SATA0_PHYA_2_PA     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143D00)
#define BASE_REG_SATA0_GP_CTRL_PA    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144500)

#define BASE_REG_SATA1_SNPS_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3500)
#define BASE_REG_SATA1_SNPS_PORT0_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3580)
#define BASE_REG_SATA1_MISC_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144100)
#define BASE_REG_SATA1_PHYD_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144200)
#define BASE_REG_SATA1_PHYA_PA       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144300)
#define BASE_REG_SATA1_PHYA_2_PA     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144400)
#define BASE_REG_SATA1_GP_CTRL_PA    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144700)

#define BASE_REG_SATA2_CLKGEN_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103F00)
#define BASE_REG_CLKGEN_BASE     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103800)

#if 1
#define PORT0_CMD  (0x0118)
#define PORT0_SCTL (0x012C)
#define PORT0_SSTS (0x0128)
#endif

enum
{
    /* global controller registers */
    SS_HOST_CAP        = 0x00, /* host capabilities */
    SS_HOST_CTL        = 0x04, /* global host control */
    SS_HOST_IRQ_STAT   = 0x08, /* interrupt status */
    SS_HOST_PORTS_IMPL = 0x0c, /* bitmap of implemented ports */
    SS_HOST_VERSION    = 0x10, /* AHCI spec. version compliancy */
    SS_HOST_CAP2       = 0x24, /* host capabilities, extended */

    /* Registers for each SATA port */
    SS_PORT_LST_ADDR    = 0x00, /* command list DMA addr */
    SS_PORT_LST_ADDR_HI = 0x04, /* command list DMA addr hi */
    SS_PORT_FIS_ADDR    = 0x08, /* FIS rx buf addr */
    SS_PORT_FIS_ADDR_HI = 0x0c, /* FIS rx buf addr hi */
    SS_PORT_IRQ_STAT    = 0x10, /* interrupt status */
    SS_PORT_IRQ_MASK    = 0x14, /* interrupt enable/disable mask */
    SS_PORT_CMD         = 0x18, /* port command */
    SS_PORT_TFDATA      = 0x20, /* taskfile data */
    SS_PORT_SIG         = 0x24, /* device TF signature */
    SS_PORT_SCR_STAT    = 0x28, /* SATA phy register: SStatus */
    SS_PORT_SCR_CTL     = 0x2c, /* SATA phy register: SControl */
    SS_PORT_SCR_ERR     = 0x30, /* SATA phy register: SError */
    SS_PORT_SCR_ACT     = 0x34, /* SATA phy register: SActive */
    SS_PORT_CMD_ISSUE   = 0x38, /* command issue */
    SS_PORT_SCR_NTF     = 0x3c, /* SATA phy register: SNotification */
    SS_PORT_DMA_CTRL    = 0x70, /* SATA phy register: SNotification */
};

enum
{
    E_PORT_SPEED_MASK           = (0xF << 4),
    E_PORT_SPEED_NO_RESTRICTION = (0x0 < 4),
    E_PORT_SPEED_GEN1           = (0x1 << 4),
    E_PORT_SPEED_GEN2           = (0x2 << 4),
    E_PORT_SPEED_GEN3           = (0x3 << 4),

    E_PORT_DET_MASK            = (0xF << 0), // Device  Detection  Initialization
    E_PORT_DET_NODEVICE_DETECT = 0x0,
    E_PORT_DET_HW_RESET        = 0x1, // Cause HW send initial sequence
    E_PORT_DET_DISABLE_PHY     = 0x4, // Put PHY into Offline
    E_PORT_DET_DEVICE_NOEST    = 0x1, // not established
    E_PORT_DET_DEVICE_EST      = 0x3, // established
    E_PORT_DET_PHY_OFFLINE     = 0x4, // Put PHY into Offline

    DATA_COMPLETE_INTERRUPT = (1 << 31),

};

// void MHal_SATA_Clock_Config(u32 misc_base, u32 port_base, bool enable);
void MHal_SATA_Clock_Config(unsigned long misc_base, unsigned long port_base, bool enable);

void MHal_SATA_HW_Inital(u32 misc_base, u32 port_base, u32 hba_base);
// void MHal_SATA_Setup_Port_Implement(u32 misc_base, u32 port_base, u32 hba_base);
void MHal_SATA_Setup_Port_Implement(unsigned long misc_base, unsigned long port_base, unsigned long hba_base);

u32 MHal_SATA_bus_address(u32 phy_address);
u32 MHal_SATA_get_max_speed(void);

#endif

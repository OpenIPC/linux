#ifndef __HI3531A_CHIP_REGS_H__
#define __HI3531A_CHIP_REGS_H__

/* -------------------------------------------------------------------- */
/* Clock and Reset Generator REG */
/* -------------------------------------------------------------------- */
#define CRG_REG_BASE		0x12040000

#define REG_CRG20           0x0050
#define REG_CRG32           0x0080
#define REG_CRG72           0x0120
#define REG_CRG75           0x012c
#define REG_CRG76           0x0130
#define REG_CRG77           0x0134
#define REG_CRG79           0x013c
#define REG_CRG81           0x0144
#define REG_CRG82           0x0148
#define REG_CRG83           0x014c
#define REG_CRG85           0x0154
#define REG_CRG87           0x015c
#define REG_CRG91           0x016c

/* -------------------------------------------------------------------- */
/* System controller register                                           */
/* -------------------------------------------------------------------- */
#define SYS_CTRL_REG_BASE	0x12050000

/* -------------------------------------------------------------------- */
/* MISC controller register                                           */
/* -------------------------------------------------------------------- */
#define MISC_CTRL_REG_BASE	0x12120000

/* -------------------------------------------------------------------- */
/* A9 soft reset request register offset                                */
/* -------------------------------------------------------------------- */
#define REG_A9_SRST_CRG		REG_CRG32
#define WDG1_SRST_REQ		BIT(6)
#define DBG1_SRST_REQ		BIT(5)
#define CPU1_SRST_REQ		BIT(4)

/* -------------------------------------------------------------------- */
/* CORTTX-A9 internal Register */
/* -------------------------------------------------------------------- */
#define A9_PERI_BASE		0x10300000
#define REG_A9_PERI_SCU		0x0000

/* -------------------------------------------------------------------- */
#define REG_BASE_L2CACHE	0x10700000

/* -------------------------------------------------------------------- */
/* SATA register                                                        */
/* -------------------------------------------------------------------- */
#define HISI_SATA_PORT_FIFOTH	0x44
#define HISI_SATA_PORT_PHYCTL1	0x48
#define HISI_SATA_PORT_PHYCTL	0x74

#define HISI_SATA_PHY_CTL0		0xA0
#define HISI_SATA_PHY_CTL1		0xA4
#define HISI_SATA_PHY_CTL2		0xB0
#define HISI_SATA_RST_PHY_MASK	0xAC

#define HISI_SATA_FIFOTH_VALUE	0x6ED9F24
#define HISI_SATA_BIGENDINE		BIT(3)

#endif /* End of __HI3531A_CHIP_REGS_H__ */

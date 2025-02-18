/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef _DRIVERS_MMC_SDHCI_XMEDIA_H
#define _DRIVERS_MMC_SDHCI_XMEDIA_H


/* Xmedia extended host controller registers. */
#define  SDHCI_CTRL_HOST_VER4_ENABLE	0x1000
#define  SDHCI_CLOCK_PLL_EN	0x0008
#define  SDHCI_CTRL_64BIT_ADDR	0x2000
#define  SDHCI_CAN_DO_ADMA3	0x08000000

/* Xmedia extended registers */
#define SDHCI_MSHC_CTRL		0x508
#define SDHCI_CMD_CONFLIT_CHECK	0x01

#define SDHCI_AXI_MBIIU_CTRL	0x510
#define SDHCI_GM_WR_OSRC_LMT_MASK	(0x7 << 24)
#define SDHCI_GM_WR_OSRC_LMT_SEL(x)	((x) << 24)
#define SDHCI_GM_RD_OSRC_LMT_MASK	(0x7 << 16)
#define SDHCI_GM_RD_OSRC_LMT_SEL(x)	((x) << 16)
#define SDHCI_UNDEFL_INCR_EN		0x1

#define SDHCI_EMMC_CTRL		0x52C
#define  SDHCI_CARD_IS_EMMC	0x0001
#define  SDHCI_ENH_STROBE_EN	0x0100

#define SDHCI_EMMC_HW_RESET	0x534

#define SDHCI_AT_CTRL		0x540
#define  SDHCI_SAMPLE_EN	0x00000010

#define SDHCI_AT_STAT		0x544
#define  SDHCI_PHASE_SEL_MASK	0x000000FF

#define SDHCI_MULTI_CYCLE	0x54C
#define  SDHCI_FOUND_EDGE	(0x1 << 11)
#define  SDHCI_EDGE_DETECT_EN	(0x1 << 8)
#define  SDHCI_DOUT_EN_F_EDGE	(0x1 << 6)
#define  SDHCI_DATA_DLY_EN	(0x1 << 3)
#define  SDHCI_CMD_DLY_EN	(0x1 << 2)


#endif /* _DRIVERS_MMC_SDHCI_XMEDIA_H */

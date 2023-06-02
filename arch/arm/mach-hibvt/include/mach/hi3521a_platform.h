#ifndef __HI3521A_H__
#define __HI3521A_H__

/* -------------------------------------------------------------------- */
/* Clock and Reset Generator REG                                        */
/* -------------------------------------------------------------------- */
#define REG_CRG_BASE			0x12040000

/* -------------------------------------------------------------------- */
/* SATA REG                                                             */
/* -------------------------------------------------------------------- */
#define HISI_SATA_PHY0_CTLL		0xA0
#define HISI_SATA_PHY0_CTLH		0xA4
#define HISI_SATA_PHY1_CTLL		0xAC
#define HISI_SATA_PHY1_CTLH		0xB0

#define HISI_SATA_PORT_FIFOTH	0x44
#define HISI_SATA_PORT_PHYCTL1	0x48
#define HISI_SATA_PORT_PHYCTL2	0x4C
#define HISI_SATA_PORT_PHYCTL	0x74

#define HISI_SATA_PHY_RESET		BIT(0)
#define HISI_SATA_BIGENDINE		BIT(3)
#define HISI_SATA_PHY_REV_CLK	BIT(9)
#define HISI_SATA_LANE0_RESET	BIT(18)

#endif /* End of __HI3521A_H__ */

#ifndef __HISI_PCIE_H__
#define __HISI_PCIE_H__

#define MISC_CTRL_BASE		0x12030000
#define PCIE_MEM_BASE		0x30000000
#define PCIE_EP_CONF_BASE	0x20000000
#define PCIE_DBI_BASE		0x12200000
#define PERI_CRG_BASE		0x12010000

#define PERI_CRG44		0x18c
#define PCIE_X2_SRST_REQ	2

#define PCIE_X2_AUX_CKEN	7
#define PCIE_X2_PIPE_CKEN	6
#define PCIE_X2_SYS_CKEN	5
#define PCIE_X2_BUS_CKEN	4
#define PCIE_PAD_OE_MASK	(0x7 << 8)

#define PCIE_SYS_CTRL0		0xc00
#define PCIE_DEVICE_TYPE	28
#define PCIE_WM_EP		0x0
#define PCIE_WM_LEGACY		0x1
#define PCIE_WM_RC		0x4

#define PCIE_SYS_CTRL7		0xc1C
#define PCIE_APP_LTSSM_ENBALE	11

#define PCIE_SYS_STATE0		0xf00
#define PCIE_XMLH_LINK_UP	15
#define PCIE_RDLH_LINK_UP	5

#define PCIE_IRQ_INTA		160
#define PCIE_IRQ_INTB		161
#define PCIE_IRQ_INTC		162
#define PCIE_IRQ_INTD		163
#define PCIE_IRQ_EDMA		164
#define PCIE_IRQ_MSI		165
#define PCIE_IRQ_LINK_DOWN	166

#define PCIE_INTA_PIN		1
#define PCIE_INTB_PIN		2
#define PCIE_INTC_PIN		3
#define PCIE_INTD_PIN		4

#define MISC_CTRL33		0x128
#define COM_PHY_TEST_VAL1		((0x1 << 11) | (0x1))
#define COM_PHY_TEST_VAL2		((0x1 << 6) | (0x1 << 11) | (0x1))

#define REG_SC_STAT         0x008c
#define PCI_CARD			0x44
#define MISC_CTRL5			0x14
#endif

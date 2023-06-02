#ifndef HI3559AV100_UFS_H
#define HI3559AV100_UFS_H

#define UFS_USE_HISI_MPHY_TC
//#define UFS_FPGA
#define COMBO_PHY_V120

#define BIT_UFS_SRST_REQ	(0x1 << 12)
#define BIT_UFS_CLK_EN		(0x1 << 13)

#define UFS_SRST_REQ	(0x1 << 0)
#define UFS_RST_DEVICE		(0x1 << 31)

#define BIT_UFS_PAD_RESET		  (0x1 << 15)
#define BIT_DA_UFS_RESET_DS2	(0x1 << 14)
#define BIT_DA_UFS_RESET_DS1	(0x1 << 13)
#define BIT_DA_UFS_RESET_DS0	(0x1 << 12)
#define BIT_DA_UFS_RESET_SL		(0x1 << 11)
#define BIT_DA_UFS_RESET_OEN	(0x1 << 10)
#define BIT_DA_UFS_RESET_PS		(0x1 << 9)
#define BIT_DA_UFS_RESET_PE		(0x1 << 8)
#define BIT_DA_UFS_REFCLK_DS2	(0x1 << 7)
#define BIT_DA_UFS_REFCLK_DS1	(0x1 << 6)
#define BIT_DA_UFS_REFCLK_DS0	(0x1 << 5)
#define BIT_DA_UFS_REFCLK_SL	(0x1 << 4)
#define BIT_DA_UFS_REFCLK_OEN	(0x1 << 3)
#define BIT_DA_UFS_REFCLK_PS	(0x1 << 2)
#define BIT_DA_UFS_REFCLK_PE	(0x1 << 1)
#define BIT_UFS_ENABLE			  (0x1 << 0)

#define MASK_DA_UFS_RESET_DS	(0x7 << 12)
#define MASK_DA_UFS_REFCLK_DS	(0x7 << 5)


#define UFS_AUTO_HIBERNATE_BIT	BIT(23)
#define UFS_CAPS_64AS_BIT		BIT(24)
#define UFS_HCE_RESET_BIT		BIT(0)
#define UFS_HCS_DP_BIT			BIT(0)
#define UFS_HCS_UCRDY_BIT		BIT(3)
#define UFS_HCS_UPMCRS_OFF		(8)
#define UFS_HCS_UPMCRS_MASK		(0x3 << UFS_HCS_UPMCRS_OFF)
#define UFS_IS_UE_BIT			BIT(2)
#define UFS_IS_UPMS_BIT			BIT(4)
#define UFS_IS_UHXS_BIT			BIT(5)
#define UFS_IS_UHES_BIT			BIT(6)
#define UFS_IS_ULSS_BIT			BIT(8)
#define UFS_IS_UCCS_BIT			BIT(10)
#define UFS_UTP_RUN_BIT			BIT(0)
#define UFS_LBMCFG_DEFAULT_VALUE	0xb01
#define UFS_HCLKDIV_NORMAL_VALUE	0xFA
#define UFS_HCLKDIV_SLOW_VALUE		0x14
#define UFS_HCLKDIV_FPGA_VALUE		0x28

#define MTX_L0				0x0000	/*GenSelectorIndex for TX lane 0*/
#define MTX_L1				0x0001	/*GenSelectorIndex for TX lane 1*/
#define MRX_L0				0x0004  /*GenSelectorIndex for RX lane 0*/
#define MRX_L1				0x0005  /*GenSelectorIndex for RX lane 1*/

#define ATTR_M_SHIFT			16
#define ATTR_UNIPRO_SHIFT		ATTR_M_SHIFT

#define ATTR_MTX0(x)			((x << ATTR_M_SHIFT) | MTX_L0)
#define ATTR_MTX1(x)			((x << ATTR_M_SHIFT) | MTX_L1)
#define ATTR_MRX0(x)			((x << ATTR_M_SHIFT) | MRX_L0)
#define ATTR_MRX1(x)			((x << ATTR_M_SHIFT) | MRX_L1)
#define ATTR_MCB(x)			(x << ATTR_M_SHIFT)
#define ATTR_UNIPRO(x)			(x << ATTR_UNIPRO_SHIFT)
/*SYNOPSYS UniPro register*/
#define UNIPRO_DME_RESET		0xD010
#define UNIPRO_DME_LAYBER_ENABLE	0xD000
/*MPHY registers addr*/
/*RX*/
#define AD_DIF_P_LS_TIMEOUT_VAL		0x0003
#define PWM_PREPARE_TO			0x0000000A
#define SKP_DET_SEL			0x0009
#define SKP_DET_SEL_EN			0x00000001

#define MRX_EN				0x00F0
#define MRX_ENABLE			(0x01 << 0)
#define RX_SQ_VREF			0x00F1
#define RX_SQ_VREF_175mv		0x00000002
#define VCO_AUTO_CHG			0x00DF
#define VCO_AUTO_CHG_EN			(0x01 << 0)
#define VCO_FORCE_ON_EN			(0x01 << 1)

#define PG_PLL_SWC_ENABLE		0x01


#define HS_R_A_FBK_P			0x41
#define HS_R_B_FBK_P			0x4C
#define HS_G_1_TXRXHSGR			0x02
#define HS_G_2_TXRXHSGR			0x01
#define HS_G_3_TXRXHSGR			0x00


/*PHY CB*/
#define RG_PLL_TXHS_EN			0x00C7
#define RG_PLL_TXHS_ENANBLE		(0x01 << 0)
#define RG_PLL_TXHS_EN_CONTROL		(0x01 << 1)

#define RG_PLL_TXLS_EN			0x00C8
#define RG_PLL_TXLS_ENABLE		(0x01 << 0)
#define RG_PLL_TXLS_EN_CONTROL		(0x01 << 1)



struct ufshcd_dme_attr_val {
	u32 attr_sel;
	u32 mib_val;
	u8 peer;
};


/* DWC HC UFSHCI specific Registers */
enum dwc_specific_registers {
	DWC_UFS_REG_HCLKDIV	= 0xFC,
};

/* Clock Divider Values: Hex equivalent of frequency in MHz */
enum clk_div_values {
	DWC_UFS_REG_HCLKDIV_DIV_62_5	= 0x3e,
	DWC_UFS_REG_HCLKDIV_DIV_125	= 0x7d,
	DWC_UFS_REG_HCLKDIV_DIV_200	= 0xc8,
};

/* Selector Index */
enum selector_index {
	SELIND_LN0_TX		= 0x00,
	SELIND_LN1_TX		= 0x01,
	SELIND_LN0_RX		= 0x04,
	SELIND_LN1_RX		= 0x05,
};
#endif

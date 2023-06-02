#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/delay.h>

#include "ufshcd.h"
#include "unipro.h"
#include "ufshcd-pltfrm.h"
#include "hi3559av100_ufs.h"

#define UFS_PRINT printk
#define UFS_AHIT_AH8ITV_MASK		(0x3FF)
#define UFS_AHIT_AUTOH8_TIMER		(0x1001)

#define UFS_AHIT_OFF            0x18
#define UFS_UTRLRSR_OFF         0x60
#define UFS_UTMRLRSR_OFF        0x80
#define UFS_BUSTHRTL_OFF        0xC0

#define HIBERNATE_TIMER_VALUE_MASK (~(BIT(9)))
#define HIBERNATE_TIMER_SCALE_SHIFT (10)
#define LP_AH8_PGE   (BIT(17))
#define LP_PGE	(BIT(16))
#define ULP_ULP_CTRLMODE (BIT(3))

#define TX_SLEEP_CONTROL  0x00c80000
#define RX_SLEEP_CONTROL  0x00c60000
/* config the pll */
#define RG_PLL_PRE_DIV 0x00c20000
#define RG_PLL_SWC_EN  0x00c90000
#define RG_PLL_FBK_S   0x00c40000
#define RG_PLL_FBK_P  0x00c30000
#define RG_PLL_TXHSGR 0x00cf0000
#define RG_PLL_RXHSGR 0x00cd0000

#define RG_PLL_TXLSGR 0x00d00000
#define RG_PLL_RXLSGR 0x00ce0000


/* power mode specific define */
struct pwrModeParams {
	uint8_t txGear;
	uint8_t rxGear;
	uint8_t hsSeries;
	uint8_t txLanes;
	uint8_t rxLanes;
	uint8_t pwrMode;
};
struct ufs_phy_remap_add
{
	void __iomem *crg_base;
	void __iomem *misc_base;
};
struct ufs_phy_remap_add remap_vir_add;

extern void ufs_dump(struct ufs_hba *hba);
void setup_snps_mphy_tc(struct ufs_hba *hba)
{
	uint32_t retry = 10;
	uint32_t value = 0;

	do {
		ufshcd_dme_get(hba, 0xD0100000, &value);
		if (1 == value)
			break;
		mdelay(1);
	} while (retry--);

	/* DME layer enable */
	ufshcd_dme_set_attr(hba, 0xd0000000, 0x00, 0x01, DME_LOCAL);
}
static void hiufs_pltfm_clk_init(void __iomem *crg_base)
{
	uint32_t reg;
#ifndef UFS_FPGA
	reg = readl(crg_base);
	reg |= BIT_UFS_CLK_EN | BIT_UFS_SRST_REQ;
	writel(reg, crg_base);
	udelay(1);
	reg = readl(crg_base);
	reg &= ~BIT_UFS_SRST_REQ;
	writel(reg, crg_base);
#else
	reg = readl(crg_base);
	reg |= UFS_SRST_REQ;
	writel(reg, crg_base);
	udelay(1);

	reg = readl(crg_base);
	reg &= ~UFS_SRST_REQ;
	writel(reg, crg_base);
	udelay(1);

	reg = readl(crg_base);
	reg &= ~UFS_RST_DEVICE;
	writel(reg, crg_base);
	udelay(1);

	reg = readl(crg_base);
	reg |= UFS_RST_DEVICE;
	writel(reg, crg_base);
	udelay(1);
#endif
}
static void hiufs_pltfm_hardware_init(void __iomem *misc_base)
{
	uint32_t reg;
	reg = readl(misc_base);
	reg |= BIT_UFS_ENABLE;
	writel(reg, misc_base);
	udelay(1);
	reg = readl(misc_base);
	reg &= ~BIT_DA_UFS_REFCLK_OEN;
	reg &= ~MASK_DA_UFS_REFCLK_DS;
	reg |= (BIT_DA_UFS_REFCLK_DS0|
			BIT_DA_UFS_REFCLK_DS1|
			BIT_DA_UFS_REFCLK_SL);
	writel(reg, misc_base);
	udelay(1);

	reg = readl(misc_base);
	reg &= ~(BIT_DA_UFS_RESET_OEN |
		MASK_DA_UFS_RESET_DS |
		BIT_UFS_PAD_RESET);
	reg |= BIT_DA_UFS_RESET_SL;
	writel(reg, misc_base);
	udelay(10);

	reg = readl(misc_base);
	reg |= BIT_UFS_PAD_RESET;
	writel(reg, misc_base);

}
static void hiufs_clk_hardware_init_notify(void)
{
	hiufs_pltfm_clk_init(remap_vir_add.crg_base);
	hiufs_pltfm_hardware_init(remap_vir_add.misc_base);
}
static int hiufs_phy_init(struct ufs_hba *hba, u32 hs_rate)
{
	uint32_t val;

#ifdef COMBO_PHY_V120
	/*Rx SKP_DET_SEL, lane0 */
	ufshcd_dme_set_attr(hba, ATTR_MRX0(SKP_DET_SEL), 0x00,
		SKP_DET_SEL_EN, DME_LOCAL);
	/*Rx SKP_DET_SEL, lane1 */
	ufshcd_dme_set_attr(hba, ATTR_MRX1(SKP_DET_SEL), 0x00,
		SKP_DET_SEL_EN, DME_LOCAL);

	/*VCO_AUTO_CHG */
	ufshcd_dme_set_attr(hba, 0xdf0000, 0x00,
		(VCO_AUTO_CHG_EN | VCO_FORCE_ON_EN), DME_LOCAL);

	/*RX_SQ_VREF, lane0 */
	ufshcd_dme_set_attr(hba, ATTR_MRX0(RX_SQ_VREF), 0x00,
		RX_SQ_VREF_175mv, DME_LOCAL);
	/*RX_SQ_VREF, lane1 */
	ufshcd_dme_set_attr(hba, ATTR_MRX1(RX_SQ_VREF), 0x00,
		RX_SQ_VREF_175mv, DME_LOCAL);

	/*Dif_N debouse*/
	ufshcd_dme_set_attr(hba, ATTR_MRX0(0xeb), 0x00, 0x60, DME_LOCAL);
	/*Dif_N debouse*/
	ufshcd_dme_set_attr(hba, ATTR_MRX1(0xeb), 0x00, 0x60, DME_LOCAL);

	/*dvalid timer*/
	ufshcd_dme_set_attr(hba, ATTR_MRX0(0x0e), 0x00, 0x64, DME_LOCAL);
	/*dvalid timer*/
	ufshcd_dme_set_attr(hba, ATTR_MRX1(0x0e), 0x00, 0x64, DME_LOCAL);
	/*RX LINE RESET DETECT TIME*/
	ufshcd_dme_set_attr(hba, ATTR_MRX0(0xef), 0x00, 0xfa, DME_LOCAL);
	/*RX LINE RESET DETECT TIME*/
	ufshcd_dme_set_attr(hba, ATTR_MRX1(0xef), 0x00, 0xfa, DME_LOCAL);

	/*AD_DIF_P_LS_TIMEOUT_VAL, lane0 */
	ufshcd_dme_set_attr(hba, ATTR_MRX0(AD_DIF_P_LS_TIMEOUT_VAL),
		0x00, PWM_PREPARE_TO, DME_LOCAL);
	/*AD_DIF_P_LS_TIMEOUT_VAL, lane1 */
	ufshcd_dme_set_attr(hba, ATTR_MRX1(AD_DIF_P_LS_TIMEOUT_VAL),
		0x00, PWM_PREPARE_TO, DME_LOCAL);
	/*RX_EQ_SEL_R, lane0 */
	ufshcd_dme_set_attr(hba, 0x00F40004, 0x00, 0x1, DME_LOCAL);
	/*RX_EQ_SEL_R, lane1 */
	ufshcd_dme_set_attr(hba, 0x00F40005, 0x00, 0x1, DME_LOCAL);

	/*RX_EQ_SEL_C, lane0 */
	ufshcd_dme_set_attr(hba, 0x00F20004, 0x00, 0x3, DME_LOCAL);
	/*RX_EQ_SEL_C, lane1 */
	ufshcd_dme_set_attr(hba, 0x00F20005, 0x00, 0x3, DME_LOCAL);

	/*RX_VSEL, lane0 */
	ufshcd_dme_set_attr(hba, 0x00FB0004, 0x00, 0x3, DME_LOCAL);
	/*RX_VSEL, lane1 */
	ufshcd_dme_set_attr(hba, 0x00FB0005, 0x00, 0x3, DME_LOCAL);

	/* RX_DLF, lane0 */
	ufshcd_dme_set_attr(hba, 0x00f60004, 0x00, 0x3, DME_LOCAL);
	/* RX_DLF, lane1 */
	ufshcd_dme_set_attr(hba, 0x00f60005, 0x00, 0x3, DME_LOCAL);

	//modefy
	/* RX H8_TIMEOUT_VAL, lane0 */
	//ufshcd_dme_set_attr(hba, 0x000a0004, 0x00, 0x3, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x000a0004, 0x00, 0x1e, DME_LOCAL);
	/* RX H8_TIMEOUT_VAL, lane1 */
	//ufshcd_dme_set_attr(hba, 0x000a0005, 0x00, 0x3, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x000a0005, 0x00, 0x1e, DME_LOCAL);
	//modefy

	/* RG_PLL_DMY0 */
	ufshcd_dme_set_attr(hba, 0x00d40000, 0x00, 0x31, DME_LOCAL);
	//new add
	ufshcd_dme_set_attr(hba, 0x00730000, 0x00, 0x00000004, DME_LOCAL);/* TX_PHY_CONFIG II */
	ufshcd_dme_set_attr(hba, 0x00730001, 0x00, 0x00000004, DME_LOCAL);/* TX_PHY_CONFIG II */
	//new add
#else
	/* in low temperature to solve the PLL's starting of oscillation */
	/* RG_PLL_CP */
	ufshcd_dme_set_attr(hba, 0x00c10000, 0x00, 0x01, DME_LOCAL);
	/* RG_PLL_DMY0 */
	ufshcd_dme_set_attr(hba, 0x00d40000, 0x00, 0x51, DME_LOCAL);
	/* rate A->B 's VCO stable time */
	/*ufshcd_dme_set_attr(hba, 0x00db0000, 0x00, 0x05, DME_LOCAL);*/
	#ifdef COMBO_PHY_V110
	/* H8's workaround */
	/*RX_SQ_VREF, lane0 */
	ufshcd_dme_set_attr(hba, 0x00f10004, 0x00, 0x07, DME_LOCAL);
	/*RX_SQ_VREF, lane1 */
	ufshcd_dme_set_attr(hba, 0x00f10005, 0x00, 0x07, DME_LOCAL);
	#endif
#endif /*end of COMBO_PHY_V120*/

	/*RX enable, lane0 */
	ufshcd_dme_set_attr(hba, ATTR_MRX0(MRX_EN), 0x00,
		MRX_ENABLE, DME_LOCAL);
	/*RX enable, lane1 */
	ufshcd_dme_set_attr(hba, ATTR_MRX1(MRX_EN), 0x00,
		MRX_ENABLE, DME_LOCAL);


#if 0
	/* disable auto H8 */
	reg = ufshcd_readl(hba, UFS_AHIT_OFF);
	reg = reg & (~UFS_AHIT_AH8ITV_MASK);
	ufshcd_writel(hba, reg, UFS_AHIT_OFF);
#endif

	setup_snps_mphy_tc(hba);

#if !defined(COMBO_PHY_V120)
	/*RX_MC_PRESENT */
	ufshcd_dme_set_attr(hba, 0x00c20004, 0x00, 0x01, DME_LOCAL);
	/*RX_MC_PRESENT */
	ufshcd_dme_set_attr(hba, 0x00c20005, 0x00, 0x01, DME_LOCAL);
#endif

	/* disable Vswing change */
    /* measure the power, can close it */
    ufshcd_dme_set_attr(hba, 0x00C70000, 0x0, 0x3, DME_LOCAL);
    /* measure the power, can close it */
    ufshcd_dme_set_attr(hba, 0x00C80000, 0x0, 0x3, DME_LOCAL);
#if !defined(COMBO_PHY_V120)
    ufshcd_dme_set_attr(hba, 0x007A0000, 0x0, 0x1c, DME_LOCAL);
    ufshcd_dme_set_attr(hba, 0x007A0001, 0x0, 0x1c, DME_LOCAL);
    ufshcd_dme_set_attr(hba, 0x007C0000, 0x0, 0xd4, DME_LOCAL);
    ufshcd_dme_set_attr(hba, 0x007C0001, 0x0, 0xd4, DME_LOCAL);
#endif
	/* no need to exitH8 before linkup anymore */
	/*exit TX_HIBERNATE_CONTROL */
	/*ufshcd_dme_set_attr(hba, 0x802b0000, 0x00, 0x00, DME_LOCAL);*/
	/*exit TX_HIBERNATE_CONTROL */
	/*ufshcd_dme_set_attr(hba, 0x802b0001, 0x00, 0x00, DME_LOCAL);*/
	/* ufs_waitms(20); */
#ifdef CLOSE_CLK_GATING
	/*RX_STALL*/
	ufshcd_dme_set_attr(hba, 0x00cf0004, 0x00, 0x02, DME_LOCAL);
	/*RX_STALL*/
	ufshcd_dme_set_attr(hba, 0x00cf0005, 0x00, 0x02, DME_LOCAL);
	/*RX_SLEEP*/
	ufshcd_dme_set_attr(hba, 0x00d00004, 0x00, 0x02, DME_LOCAL);
	/*RX_SLEEP*/
	ufshcd_dme_set_attr(hba, 0x00d00005, 0x00, 0x02, DME_LOCAL);

	/*RX_HS_CLK_EN*/
	ufshcd_dme_set_attr(hba, 0x00cc0004, 0x00, 0x03, DME_LOCAL);
	/*RX_HS_CLK_EN*/
	ufshcd_dme_set_attr(hba, 0x00cc0005, 0x00, 0x03, DME_LOCAL);
	/*RX_LS_CLK_EN*/
	ufshcd_dme_set_attr(hba, 0x00cd0004, 0x00, 0x03, DME_LOCAL);
	/*RX_LS_CLK_EN*/
	ufshcd_dme_set_attr(hba, 0x00cd0005, 0x00, 0x03, DME_LOCAL);
#endif

#if defined(COMBO_PHY_V120)
	/*RG_PLL_RXHS_EN*/
	ufshcd_dme_set_attr(hba, 0x00c50000, 0x00, 0x03, DME_LOCAL);
	/*RG_PLL_RXLS_EN*/
	ufshcd_dme_set_attr(hba, 0x00c60000, 0x00, 0x03, DME_LOCAL);
	/*RX_HS_DATA_VALID_TIMER_VAL0*/
	ufshcd_dme_set_attr(hba, 0x00E90004, 0x00, 0x00, DME_LOCAL);
	/*RX_HS_DATA_VALID_TIMER_VAL0*/
	ufshcd_dme_set_attr(hba, 0x00E90005, 0x00, 0x00, DME_LOCAL);
	/*RX_HS_DATA_VALID_TIMER_VAL1*/
	ufshcd_dme_set_attr(hba, 0x00EA0004, 0x00, 0x10, DME_LOCAL);
	/*RX_HS_DATA_VALID_TIMER_VAL1*/
	ufshcd_dme_set_attr(hba, 0x00EA0005, 0x00, 0x10, DME_LOCAL);
#else
	/*enhance the accuracy of squelch detection*/
	/*RX_H8_EXIT*/
	ufshcd_dme_set_attr(hba, 0x00ce0004, 0x00, 0x03, DME_LOCAL);
	/*RX_H8_EXIT*/
	ufshcd_dme_set_attr(hba, 0x00ce0005, 0x00, 0x03, DME_LOCAL);

/* try to solve the OCS=5 */
	/*RX_HS_DATA_VALID_TIMER_VAL0*/
	ufshcd_dme_set_attr(hba, 0x00E90004, 0x00, 0x20, DME_LOCAL);
	/*RX_HS_DATA_VALID_TIMER_VAL0*/
	ufshcd_dme_set_attr(hba, 0x00E90005, 0x00, 0x20, DME_LOCAL);
	/*RX_HS_DATA_VALID_TIMER_VAL1*/
	ufshcd_dme_set_attr(hba, 0x00EA0004, 0x00, 0x01, DME_LOCAL);
	/*RX_HS_DATA_VALID_TIMER_VAL1*/
	ufshcd_dme_set_attr(hba, 0x00EA0005, 0x00, 0x01, DME_LOCAL);
#endif
#if 1
	/* set the HS-prepare length and sync length to MAX value,
		try to solve the data check error problem, the device
		seems not receive the write cmd. */
	/* PA_TxHsG1SyncLength , can not set MPHY's register directly */
	ufshcd_dme_set_attr(hba, 0x15520000, 0x00, 0x4F, DME_LOCAL);
	/* PA_TxHsG2SyncLength , can not set MPHY's register directly */
	ufshcd_dme_set_attr(hba, 0x15540000, 0x00, 0x4F, DME_LOCAL);
	/* PA_TxHsG3SyncLength , can not set MPHY's register directly */
	ufshcd_dme_set_attr(hba, 0x15560000, 0x00, 0x4F, DME_LOCAL);

	/*enlarge TX_LS_PREPARE_LENGTH*/
	/*enable override*/
/*
	temp = uic_cmd_read(0x1, 0xd0f00000);
	temp |= (1<<3);
	ufshcd_dme_set_attr(hba, 0xd0f00000, 0x00, temp);
*/
	/*Set to max value 0xf*/
/*	ufshcd_dme_set_attr(hba, 0xd0f40000, 0x00, 0x0f, DME_LOCAL);*/
#endif
#if defined(OCS_5_WORKAROUND)
	ufshcd_dme_set_attr(hba, 0x00FF0004, 0x00, 0x2, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x00FF0005, 0x00, 0x2, DME_LOCAL);
#endif
#if defined(NOISE_PROOF)
	ufshcd_dme_set_attr(hba, 0x00F40004, 0x00, 0x2, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x00F40005, 0x00, 0x2, DME_LOCAL);

	ufshcd_dme_set_attr(hba, 0x00F20004, 0x00, 0x2, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x00F20005, 0x00, 0x2, DME_LOCAL);

	ufshcd_dme_set_attr(hba, 0x00FC0004, 0x00, 0x1F, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x00FC0005, 0x00, 0x1F, DME_LOCAL);

	ufshcd_dme_set_attr(hba, 0x00FB0004, 0x00, 0x0, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x00FB0005, 0x00, 0x0, DME_LOCAL);
#endif

	/* pll always on*/
	ufshcd_dme_set_attr(hba, 0x00ca0000, 0x0, 0x3, DME_LOCAL);
	/* update */
	ufshcd_dme_set_attr(hba, 0xD0850000, 0x0, 0x1, DME_LOCAL);

	/* to check if the unipro have to close the LCC */
	/* Unipro PA_Local_TX_LCC_Enable */
	ufshcd_dme_set_attr(hba, 0x155E0000, 0x0, 0x0, DME_LOCAL);
	/* close Unipro VS_Mk2ExtnSupport */
	ufshcd_dme_set_attr(hba, 0xD0AB0000, 0x0, 0x0, DME_LOCAL);

	ufshcd_dme_get(hba, 0xD0AB0000, &val);
	if (0 != val) {
		/* Ensure close success */
		UFS_PRINT("Warring!!! close VS_Mk2ExtnSupport failed\n");
	}

	return 0;
}

static void hiufs_program_clk_div(struct ufs_hba *hba, u32 divider_val)
{
	ufshcd_writel(hba, divider_val, DWC_UFS_REG_HCLKDIV);
}

static int hiufs_link_is_up(struct ufs_hba *hba)
{
	int dme_result = 0;

	ufshcd_dme_get(hba, UIC_ARG_MIB(VS_POWERSTATE), &dme_result);

	if (dme_result == UFSHCD_LINK_IS_UP) {
		ufshcd_set_link_active(hba);
		return 0;
	}

	return 1;
}

static int hiufs_connection_setup(struct ufs_hba *hba)
{
	u32 regv;

	ufshcd_dme_set_attr(hba, 0x20440000, 0, 0x0, DME_LOCAL);
	/* Unipro DL_AFC0CreditThreshold */
	ufshcd_dme_set_attr(hba, 0x20450000, 0, 0x0, DME_LOCAL);
	/* Unipro DL_TC0OutAckThreshold */
	ufshcd_dme_set_attr(hba, 0x20400000, 0, 0x9, DME_LOCAL);
	/* Unipro DL_TC0TXFCThreshold */


	/*set the PA_Granularity  to 2. need to check in ASIC...*/
	/*send_uic_command(0x02, 0x15aa0000, 0x00, 4, DME_LOCAL);*/
	ufshcd_dme_set_attr(hba, 0x15aa0000, 0, 0x2, DME_LOCAL);
	/* Unipro DL_TC0TXFCThreshold */
	/*PA_Hibern8Time*/
	ufshcd_dme_set_attr(hba, 0x15a70000, 0x00, 0x80, DME_LOCAL);
	/*set the PA_TActivate to 128. need to check in ASIC...*/
	/*send_uic_command(0x02, 0x15a80000, 0x00, 9);*/

	//delete
	//ufshcd_dme_set_attr(hba, 0x80da0000, 0, 0x2d, DME_LOCAL);
	//delete
	ufshcd_dme_set_attr(hba, 0xd0ab0000, 0, 0x0, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0xd0a00000, 0, 0xc, DME_LOCAL);
/*
	ufshcd_writel(hba, UFS_UTP_RUN_BIT, UFS_UTRLRSR_OFF);
	ufshcd_writel(hba, UFS_UTP_RUN_BIT, UFS_UTMRLRSR_OFF);
*/

#if 1
/*#ifdef UFS_AUTO_H8_ENABLE*/

	/* disable auto H8 Power-Gating */
	regv = ufshcd_readl(hba, UFS_BUSTHRTL_OFF);
	regv &= (uint32_t)(~LP_AH8_PGE);
	ufshcd_writel(hba, regv, UFS_BUSTHRTL_OFF);

	/* enable auto H8 */
	/*ufshcd_writel(hba, UFS_AHIT_AUTOH8_TIMER, UFS_AHIT_OFF);*/
#endif

	return 0;
}

static int hiufs_link_startup_notify(struct ufs_hba *hba,
		enum ufs_notify_change_status status)
{
	int err = 0;

	if (status == PRE_CHANGE) {
		hiufs_program_clk_div(hba, UFS_HCLKDIV_NORMAL_VALUE);
		err = hiufs_phy_init(hba, PA_HS_MODE_B);
		if (err) {
			dev_err(hba->dev, "Phy setup failed (%d)\n", err);
			goto out;
		}
	} else { /* POST_CHANGE */
		err = hiufs_link_is_up(hba);
		if (err) {
			dev_err(hba->dev, "Link is not up\n");
			goto out;
		}

		err = hiufs_connection_setup(hba);
		if (err)
			dev_err(hba->dev, "Connection setup failed (%d)\n", err);
	}

out:
	return err;
}

int do_mode_change(struct ufs_hba *hba,
		const struct pwrModeParams *pmp,
		int scramble, int termination)
{

	/*RX enable, lane0 */
	/*ufshcd_dme_set_attr(hba, 0x00f00004, 0x00, 0x01, DME_LOCAL);*/
	/*ufshcd_dme_set_attr(hba, 0xD0850000, 0x0, 0x1, DME_LOCAL);*/

	/* PA_TxSkip */
	ufshcd_dme_set_attr(hba, 0x155c0000, 0x0, 0x0, DME_LOCAL);
	/* PA_TxGear */
	ufshcd_dme_set_attr(hba, 0x15680000, 0x0, pmp->txGear, DME_LOCAL);
	/* PA_RxGear */
	ufshcd_dme_set_attr(hba, 0x15830000, 0x0, pmp->rxGear, DME_LOCAL);

	if (pmp->pwrMode == FAST_MODE || pmp->pwrMode == FASTAUTO_MODE) {
		/* PA_HSSeries */
		ufshcd_dme_set_attr(hba, 0x156a0000, 0x0, pmp->hsSeries, DME_LOCAL);
		if (termination) {
			/* PA_TxTermination */
			ufshcd_dme_set_attr(hba, 0x15690000, 0x0, 0x1, DME_LOCAL);
			/* PA_RxTermination */
			ufshcd_dme_set_attr(hba, 0x15840000, 0x0, 0x1, DME_LOCAL);
		} else {
			/* PA_TxTermination */
			ufshcd_dme_set_attr(hba, 0x15690000, 0x0, 0x0, DME_LOCAL);
			/* PA_RxTermination */
			ufshcd_dme_set_attr(hba, 0x15840000, 0x0, 0x0, DME_LOCAL);
		}

		if (scramble)
			 /* PA_Scrambling */
			ufshcd_dme_set_attr(hba, 0x15850000, 0x0, 0x1, DME_LOCAL);
		else
			 /* PA_Scrambling */
			ufshcd_dme_set_attr(hba, 0x15850000, 0x0, 0x0, DME_LOCAL);

	} else if (pmp->pwrMode == SLOW_MODE || pmp->pwrMode == SLOWAUTO_MODE) {
		ufshcd_dme_set_attr(hba, 0x15690000, 0x0, 0x0, DME_LOCAL);
		ufshcd_dme_set_attr(hba, 0x15840000, 0x0, 0x0, DME_LOCAL);
		/* PA_Scrambling */
		ufshcd_dme_set_attr(hba, 0x15850000, 0x0, 0x0, DME_LOCAL);
	}

	/* PA_ActiveTxDataLanes */
	ufshcd_dme_set_attr(hba, 0x15600000, 0x0, pmp->txLanes, DME_LOCAL);
	/* PA_ActiveRxDataLanes */
	ufshcd_dme_set_attr(hba, 0x15800000, 0x0, pmp->rxLanes, DME_LOCAL);

	/*PA_PWRModeUserData0 = 8191, default is 0*/
	ufshcd_dme_set_attr(hba, 0x15b00000, 0x0, 8191, DME_LOCAL);
	/*PA_PWRModeUserData1 = 65535, default is 0*/
	ufshcd_dme_set_attr(hba, 0x15b10000, 0x0, 65535, DME_LOCAL);
	/*PA_PWRModeUserData2 = 32767, default is 0*/
	ufshcd_dme_set_attr(hba, 0x15b20000, 0x0, 32767, DME_LOCAL);
	/*DME_FC0ProtectionTimeOutVal = 8191, default is 0*/
	ufshcd_dme_set_attr(hba, 0xd0410000, 0x0, 8191, DME_LOCAL);
	/*DME_TC0ReplayTimeOutVal = 65535, default is 0*/
	ufshcd_dme_set_attr(hba, 0xd0420000, 0x0, 65535, DME_LOCAL);
	/*DME_AFC0ReqTimeOutVal = 32767, default is 0*/
	ufshcd_dme_set_attr(hba, 0xd0430000, 0x0, 32767, DME_LOCAL);

	/*PA_PWRModeUserData3 = 8191, default is 0*/
	ufshcd_dme_set_attr(hba, 0x15b30000, 0x0, 8191, DME_LOCAL);
	/*PA_PWRModeUserData4 = 65535, default is 0*/
	ufshcd_dme_set_attr(hba, 0x15b40000, 0x0, 65535, DME_LOCAL);
	/*PA_PWRModeUserData5 = 32767, default is 0*/
	ufshcd_dme_set_attr(hba, 0x15b50000, 0x0, 32767, DME_LOCAL);
	/*DME_FC1ProtectionTimeOutVal = 8191, default is 0*/
	ufshcd_dme_set_attr(hba, 0xd0440000, 0x0, 8191, DME_LOCAL);
	/*DME_TC1ReplayTimeOutVal = 65535, default is 0*/
	ufshcd_dme_set_attr(hba, 0xd0450000, 0x0, 65535, DME_LOCAL);
	/*DME_AFC1ReqTimeOutVal = 32767, default is 0*/
	ufshcd_dme_set_attr(hba, 0xd0460000, 0x0, 32767, DME_LOCAL);

	/*VS_DebugCounter0Mask*/
	ufshcd_dme_set_attr(hba, 0xd09a0000, 0x0, 0x80000000, DME_LOCAL);
	/*ufshcd_dme_set_attr(hba, 0xd09a0000, 0x0, 0x80000000);*/
	/*UFS_PRINT("Set VS_DebugCounter0Mask  to	0x%x\n",
		uic_cmd_read(DME_GET, 0xd09a0000));*/

	/*VS_DebugCounter1Mask*/
	ufshcd_dme_set_attr(hba, 0xd09b0000, 0x0, 0x78000000, DME_LOCAL);
	/*ufshcd_dme_set_attr(hba, 0xd09b0000, 0x0, 0x78000000);*/
	/*UFS_PRINT("Set VS_DebugCounter1Mask  to	0x%x\n",
		uic_cmd_read(DME_GET, 0xd09b0000));*/

	/* PA_PWRMode */
	ufshcd_dme_set_attr(hba, 0x15710000, 0x0,
		(pmp->pwrMode<<4|pmp->pwrMode), DME_LOCAL);
	/*FUNC_EXIT();*/
	return 0;
}

static void adapt_pll_to_power_mode(struct ufs_hba *hba,
		uint8_t pwrmode, uint8_t gear, uint8_t rate, uint32_t line)
{
#ifdef COMBO_PHY_V100
	uint32_t value;
	if (FAST_MODE == pwrmode || SLOW_MODE == pwrmode)
		/*don't change PLL in fast and slow mode*/
		return 0;
#endif

#ifdef COMBO_PHY_V100
	/*Note that CB registers for PLL can only be set at SAVE (Stall/Sleep) state*/
	while (1) {
		value = uic_cmd_read(0x1, 0x00410000);
		PRINT_ERROR("value is %d \n", value);
		if ((3 == value) || (2 == value)) break;
	}
#endif
#if 1
	if (FAST_MODE == pwrmode || FASTAUTO_MODE == pwrmode) {
		if (UFS_HS_G1 == gear) {
			if (PA_HS_MODE_A == rate) {
				ufshcd_dme_set_attr(hba, RG_PLL_PRE_DIV, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_SWC_EN, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_S, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_P, 0x00, 0x41, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_TXHSGR, 0x00, 0x02, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_RXHSGR, 0x00, 0x02, DME_LOCAL);
			}	else if (PA_HS_MODE_B == rate) {
				ufshcd_dme_set_attr(hba, RG_PLL_PRE_DIV, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_SWC_EN, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_S, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_P, 0x00, 0x4c, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_TXHSGR, 0x00, 0x02, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_RXHSGR, 0x00, 0x02, DME_LOCAL);
			}

		} else if (UFS_HS_G2 == gear) {
			if (PA_HS_MODE_A == rate) {
				ufshcd_dme_set_attr(hba, RG_PLL_PRE_DIV, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_SWC_EN, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_S, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_P, 0x00, 0x41, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_TXHSGR, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_RXHSGR, 0x00, 0x01, DME_LOCAL);
			} else if (PA_HS_MODE_B == rate) {
				ufshcd_dme_set_attr(hba, RG_PLL_PRE_DIV, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_SWC_EN, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_S, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_P, 0x00, 0x4C, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_TXHSGR, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_RXHSGR, 0x00, 0x01, DME_LOCAL);
			}
		} else if (UFS_HS_G3 == gear) {
			if (PA_HS_MODE_A == rate) {
				ufshcd_dme_set_attr(hba, RG_PLL_PRE_DIV, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_SWC_EN, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_S, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_P, 0x00, 0x41, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_TXHSGR, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_RXHSGR, 0x00, 0x00, DME_LOCAL);
			} else if (PA_HS_MODE_B == rate) {
				ufshcd_dme_set_attr(hba, RG_PLL_PRE_DIV, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_SWC_EN, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_S, 0x00, 0x01, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_FBK_P, 0x00, 0x4c, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_TXHSGR, 0x00, 0x00, DME_LOCAL);
				ufshcd_dme_set_attr(hba, RG_PLL_RXHSGR, 0x00, 0x00, DME_LOCAL);
			}
		}
	}
	#endif

	/*the PWM's clk is been derived from the pll above*/
	if (SLOW_MODE == pwrmode || SLOWAUTO_MODE == pwrmode) {
		if (UFS_PWM_G1 == gear) {
			ufshcd_dme_set_attr(hba, RG_PLL_TXLSGR, 0x00, 0x07, DME_LOCAL);
			ufshcd_dme_set_attr(hba, RG_PLL_RXLSGR, 0x00, 0x06, DME_LOCAL);
		} else if (UFS_PWM_G2 == gear) {
			ufshcd_dme_set_attr(hba, RG_PLL_TXLSGR, 0x00, 0x06, DME_LOCAL);
			ufshcd_dme_set_attr(hba, RG_PLL_RXLSGR, 0x00, 0x05, DME_LOCAL);
		} else if (UFS_PWM_G3 == gear) {
			ufshcd_dme_set_attr(hba, RG_PLL_TXLSGR, 0x00, 0x05, DME_LOCAL);
			ufshcd_dme_set_attr(hba, RG_PLL_RXLSGR, 0x00, 0x04, DME_LOCAL);
		} else if (UFS_PWM_G4 == gear) {
			ufshcd_dme_set_attr(hba, RG_PLL_TXLSGR, 0x00, 0x04, DME_LOCAL);
			ufshcd_dme_set_attr(hba, RG_PLL_RXLSGR, 0x00, 0x03, DME_LOCAL);
		}
	}

	#ifdef COMBO_PHY_V100
	/* just for test begin */
	ufshcd_dme_set_attr(hba, 0x00f50004, 0x00, 0x01, DME_LOCAL);
	/* mphy config update */
	ufshcd_dme_set_attr(hba, 0xd0850000, 0x00, 0x01, DME_LOCAL);
	/* just for test end */
	#endif

	/*ufs_waitms(50);*/
	/*exit TX_HIBERNATE_CONTROL */
	/*ufshcd_dme_set_attr(hba, 0x002b0000, 0x00, 0x00);*/


	/* mphy config update */
	/*ufshcd_dme_set_attr(hba, 0xd0850000, 0x00, 0x01, DME_LOCAL);*/
}

static void ufsphy_eye_configuration(struct ufs_hba *hba,
		uint8_t pwrmode, uint8_t gear, uint8_t rate, uint32_t line)
{

	if (FAST_MODE == pwrmode || FASTAUTO_MODE == pwrmode) {
		if (UFS_HS_G1 == gear) {
			if (PA_HS_MODE_A == rate) {
				ufshcd_dme_set_attr(hba, 0x007e0000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0000, 0x00, 0x11, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370000, 0x00, 0xf, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0000, 0x00, 0x6f, DME_LOCAL);
				if (line == 2) {
				ufshcd_dme_set_attr(hba, 0x007e0001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0001, 0x00, 0x11, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370001, 0x00, 0xf, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0001, 0x00, 0x6f, DME_LOCAL);
				}
			} else if (PA_HS_MODE_B == rate) {
				ufshcd_dme_set_attr(hba, 0x007e0000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0000, 0x00, 0x11, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370000, 0x00, 0xf, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0000, 0x00, 0x6f, DME_LOCAL);
				if (line == 2) {
				ufshcd_dme_set_attr(hba, 0x007e0001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0001, 0x00, 0x11, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370001, 0x00, 0xf, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0001, 0x00, 0x6f, DME_LOCAL);
				}
			}

		} else if (UFS_HS_G2 == gear) {
			if (PA_HS_MODE_A == rate) {
				ufshcd_dme_set_attr(hba, 0x007e0000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0000, 0x00, 0x1a, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370000, 0x00, 0x02, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0000, 0x00, 0x22, DME_LOCAL);
				if (line == 2) {
				ufshcd_dme_set_attr(hba, 0x007e0001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0001, 0x00, 0x1a, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370001, 0x00, 0x02, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0001, 0x00, 0x22, DME_LOCAL);
				}
			} else if (PA_HS_MODE_B == rate) {
				ufshcd_dme_set_attr(hba, 0x007e0000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0000, 0x00, 0x1e, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370000, 0x00, 0x03, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0000, 0x00, 0x23, DME_LOCAL);
				if (line == 2) {
				ufshcd_dme_set_attr(hba, 0x007e0001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0001, 0x00, 0x1e, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370001, 0x00, 0x03, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0001, 0x00, 0x23, DME_LOCAL);
				}
			}
		} else if (UFS_HS_G3 == gear) {
			if (PA_HS_MODE_A == rate) {
				ufshcd_dme_set_attr(hba, 0x007e0000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0000, 0x00, 0x20, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0000, 0x00, 0x25, DME_LOCAL);
				if (line == 2) {
				ufshcd_dme_set_attr(hba, 0x007e0001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0001, 0x00, 0x20, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0001, 0x00, 0x25, DME_LOCAL);
				}
			} else if (PA_HS_MODE_B == rate) {
				ufshcd_dme_set_attr(hba, 0x007e0000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0000, 0x00, 0x20, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370000, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0000, 0x00, 0x25, DME_LOCAL);
				if (line == 2) {
				ufshcd_dme_set_attr(hba, 0x007e0001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007f0001, 0x00, 0x20, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x00370001, 0x00, 0x05, DME_LOCAL);
				ufshcd_dme_set_attr(hba, 0x007b0001, 0x00, 0x25, DME_LOCAL);
				}
			}
		}
	}
}

static int change_power_mode(struct ufs_hba *hba, uint8_t pwrmode,
		uint8_t gear, uint8_t rate,
		int line, int scramble, int termination)
{
	int err;
	u32 set;
	uint32_t value;
	struct pwrModeParams pmp;
	uint32_t tx_lane_num;
	uint32_t rx_lane_num;
	int retry = 1000;
	u32 auto_chg = 0;

	tx_lane_num = line;
	rx_lane_num = line;

	if (3 == gear) {
		value = 0x26; /* 4.44 dB */
		/*value = 0x20;*/
		/* HS-G3- 0 dB, other configuration is the same*/
		ufshcd_dme_set_attr(hba, 0x007e0000, 0x0, 0x05, DME_LOCAL);
		//modefy begin
		//ufshcd_dme_set_attr(hba, 0x00250000, 0x0, 0x22, DME_LOCAL);
		ufshcd_dme_set_attr(hba, 0x007f0000, 0x0, 0x22, DME_LOCAL);
		//modefy end
		ufshcd_dme_set_attr(hba, 0x007d0000, 0x0, 0x22, DME_LOCAL);
		//modefy begin
		ufshcd_dme_set_attr(hba, 0x00fc0004, 0x0, 0x1f, DME_LOCAL);/*RX_S*/
		ufshcd_dme_set_attr(hba, 0x00fd0004, 0x0, 0x00, DME_LOCAL);/*RX_GEAR1_SET*/
		//modefy end
		if ((tx_lane_num > 1) && (rx_lane_num > 1)) {
			ufshcd_dme_set_attr(hba, 0x007e0001, 0x0, 0x05, DME_LOCAL);
		//modefy begin
		//ufshcd_dme_set_attr(hba, 0x00250000, 0x0, 0x22, DME_LOCAL);
		ufshcd_dme_set_attr(hba, 0x007f0001, 0x0, 0x22, DME_LOCAL);
		//modefy end
			ufshcd_dme_set_attr(hba, 0x007d0001, 0x0, 0x22, DME_LOCAL);
		//modefy add
		ufshcd_dme_set_attr(hba, 0x00fc0005, 0x0, 0x1f, DME_LOCAL);/*RX_S*/
		ufshcd_dme_set_attr(hba, 0x00fd0005, 0x0, 0x00, DME_LOCAL);/*RX_GEAR1_SET*/
		//modefy add
		}
	} else {
		value = 0x6F; /* 6.85 dB */
		ufshcd_dme_set_attr(hba, 0x007e0000, 0x0, 0x05, DME_LOCAL);
		ufshcd_dme_set_attr(hba, 0x007f0000, 0x0, 0x15, DME_LOCAL);
		ufshcd_dme_set_attr(hba, 0x007d0000, 0x0, 0x15, DME_LOCAL);
		if ((tx_lane_num > 1) && (rx_lane_num > 1)) {
			ufshcd_dme_set_attr(hba, 0x007e0001, 0x0, 0x05, DME_LOCAL);
			ufshcd_dme_set_attr(hba, 0x007f0001, 0x0, 0x15, DME_LOCAL);
			ufshcd_dme_set_attr(hba, 0x007d0001, 0x0, 0x15, DME_LOCAL);
		}
		//modefy add
			ufshcd_dme_set_attr(hba, 0x00fc0004, 0x0, 0x1b, DME_LOCAL);/*RX_S*/
			if ((tx_lane_num > 1) && (rx_lane_num > 1))
			{
				ufshcd_dme_set_attr(hba, 0x00fc0005, 0x0, 0x1b, DME_LOCAL);/*RX_S*/
			}
			if(1 == gear)
			{
				ufshcd_dme_set_attr(hba, 0x00fd0004, 0x0, 0x01, DME_LOCAL);/*RX_GEAR1_SET*/
				if ((tx_lane_num > 1) && (rx_lane_num > 1))
				{
					ufshcd_dme_set_attr(hba, 0x00fd0005, 0x0, 0x01, DME_LOCAL);/*RX_GEAR1_SET*/
				}
			}
			else if(2 == gear)
			{
				ufshcd_dme_set_attr(hba, 0x00fd0004, 0x0, 0x00, DME_LOCAL);/*RX_GEAR1_SET*/
				if ((tx_lane_num > 1) && (rx_lane_num > 1))
				{
					ufshcd_dme_set_attr(hba, 0x00fd0005, 0x0, 0x00, DME_LOCAL);/*RX_GEAR1_SET*/
				}
			}
		//modefy add
	}

	ufshcd_dme_set_attr(hba, 0x00370000, 0x0, value, DME_LOCAL);
	ufshcd_dme_set_attr(hba, 0x007b0000, 0x0, value, DME_LOCAL);
	if ((tx_lane_num > 1) && (rx_lane_num > 1)) {
		ufshcd_dme_set_attr(hba, 0x00370001, 0x0, value, DME_LOCAL);
		ufshcd_dme_set_attr(hba, 0x007b0001, 0x0, value, DME_LOCAL);
	}

	/* PaTactive */
	/*ufshcd_dme_set_attr(hba, 0x15A80000, 0x0, 0x7, DME_LOCAL);*/

	if (1 == line) {
		tx_lane_num = 1;
		rx_lane_num = 1;
	} else {
		/* PA_ConnectedTxDataLanes */
		ufshcd_dme_get_attr(hba, 0x15610000, (u32 *)&tx_lane_num, DME_LOCAL);
		/* PA_ConnectedRxDataLanes */
		ufshcd_dme_get_attr(hba, 0x15810000, (u32 *)&rx_lane_num, DME_LOCAL);
	}

	pmp.txLanes = tx_lane_num;    /* Tx One Lane */
	pmp.rxLanes = rx_lane_num;    /* Rx One Lane */
	pmp.txGear = gear;
	pmp.rxGear = gear;
	pmp.hsSeries = rate;
	pmp.pwrMode = pwrmode;

	set = ufshcd_readl(hba, REG_INTERRUPT_ENABLE);
	set &= ~UFS_IS_UPMS_BIT;
	ufshcd_writel(hba, set, REG_INTERRUPT_ENABLE);

#ifdef UFS_USE_HISI_MPHY_TC
	/*RX mc, lane0 */
	/*ufshcd_dme_set_attr(hba, 0x00C20004, 0x00, 0x00, DME_LOCAL);*/
	/*RX MC, lane1 */
	/*ufshcd_dme_set_attr(hba, 0x00C20005, 0x00, 0x00, DME_LOCAL);*/

	ufshcd_dme_get_attr(hba, 0xdf0000, &auto_chg, DME_LOCAL);
	printk("auto_chg = %#x\n", auto_chg);
	if ( !auto_chg & VCO_AUTO_CHG_EN )
		adapt_pll_to_power_mode(hba, pwrmode,  gear,  rate, rx_lane_num);
#endif
		ufsphy_eye_configuration(hba, pwrmode,  gear,  rate, rx_lane_num);
	err = do_mode_change(hba, &pmp, scramble, termination);
	if (err) {
		UFS_PRINT("do_mode_change failed\n");
		return err;
	}

	while (--retry) {
		value = ufshcd_readl(hba, REG_INTERRUPT_STATUS);
		if (value & UFS_IS_UPMS_BIT) {
			/*ufshcd_writel(hba,
				UFS_IS_UPMS_BIT,
				REG_INTERRUPT_STATUS);*/
			ufshcd_writel(hba, value, REG_INTERRUPT_STATUS);
			break;
		}
		msleep(1);
	}
	set |= UFS_IS_UPMS_BIT;
	ufshcd_writel(hba, set, REG_INTERRUPT_ENABLE);

	if (retry <= 0)
		UFS_PRINT("\nwait IS.UPMS time out  value:%#X\n", value);

	value = ufshcd_readl(hba, REG_CONTROLLER_STATUS);
	if (((value & UFS_HCS_UPMCRS_MASK) >> UFS_HCS_UPMCRS_OFF) != 0x1) {
		UFS_PRINT("check HCS.UPMCRS error, HCS = 0x%x\n", value);
		return -1;
	}
	UFS_PRINT("power mode change success\n");

	UFS_PRINT("UFS %s Gear-%d Rate-%c Lanes-%d Scrmbl-%d Termn-%d\n",
				 ((SLOW_MODE == pwrmode)?"Slow" :
				 ((SLOWAUTO_MODE == pwrmode)?"SlowAuto" :
				 ((FAST_MODE == pwrmode)?"Fast" : "FastAuto"))),
				 gear, (rate == 1)?'A' : 'B', rx_lane_num,
				 scramble, termination);
	/* RX_ERR_STATUS */
	ufshcd_dme_set_attr(hba, 0x00c40004, 0x0, 0x80, DME_LOCAL);
	/* RX_ERR_STATUS */
	ufshcd_dme_set_attr(hba, 0x00c50004, 0x0, 0x01, DME_LOCAL);

	if (rx_lane_num == 2) {
		/* RX_ERR_STATUS */
		ufshcd_dme_set_attr(hba, 0x00c40005, 0x0, 0x80, DME_LOCAL);
		/* RX_ERR_STATUS */
		ufshcd_dme_set_attr(hba, 0x00c50005, 0x0, 0x01, DME_LOCAL);
	}

	return SUCCESS;
}

static int hiufs_config_power_mode(struct ufs_hba *hba,
	struct ufs_pa_layer_attr *dev_max_params)
{
	uint32_t gear = dev_max_params->gear_rx;
	uint32_t rate = dev_max_params->hs_rate;
	uint32_t pwrmode = dev_max_params->pwr_rx;
	uint32_t lane = dev_max_params->lane_rx;

	//ufshcd_dme_set_attr(hba, 0x00370000, 0x0, 0x1, DME_LOCAL);
	change_power_mode(hba, pwrmode, gear, rate, lane, 0, 1);

	return 0;
}

static int hiufs_pwr_change_notify(struct ufs_hba *hba,
			enum ufs_notify_change_status status,
			struct ufs_pa_layer_attr *dev_max_params,
			struct ufs_pa_layer_attr *dev_req_params)
{
	int err = 0;

	if (status == PRE_CHANGE) {
		err = hiufs_config_power_mode(hba, dev_max_params);
		memcpy(dev_req_params, dev_max_params,
			sizeof(struct ufs_pa_layer_attr));
    memcpy(&hba->pwr_info, dev_max_params,
			sizeof(struct ufs_pa_layer_attr));
	}

	return err;
}

static int hiufs_pltfm_init(struct ufs_hba *hba)
{
	hba->clk_gating.state = CLKS_ON;
	hba->caps &= ~UFSHCD_CAP_CLK_GATING;
	return 0;
}

static struct ufs_hba_variant_ops hiufs_pltfm_hba_vops = {
	.name	= "hiufs_pltfm",
	.init	= hiufs_pltfm_init,
	.link_startup_notify	= hiufs_link_startup_notify,
	.pwr_change_notify	= hiufs_pwr_change_notify,
	.clk_hareware_init_notify	= hiufs_clk_hardware_init_notify,
};

static const struct of_device_id hiufs_pltfm_pm_match[] = {
	{
		.compatible = "hiufs,hiufs_pltfm",
		.data = &hiufs_pltfm_hba_vops,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, hiufs_pltfm_pm_match);

static int hiufs_pltfm_probe(struct platform_device *pdev)
{
	int err;
	void __iomem *crg_base;
	void __iomem *misc_base;
	struct resource *mem_res;
	const struct of_device_id *of_id;
	struct ufs_hba_variant_ops *vops;
	struct device *dev = &pdev->dev;

	of_id = of_match_node(hiufs_pltfm_pm_match, dev->of_node);
	vops = (struct ufs_hba_variant_ops *)of_id->data;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	crg_base = devm_ioremap_resource(dev, mem_res);
	if (IS_ERR(*(void **)&crg_base))
		return PTR_ERR(*(void **)&crg_base);
	remap_vir_add.crg_base = crg_base;
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	misc_base = devm_ioremap_resource(dev, mem_res);
	if (IS_ERR(*(void **)&misc_base))
		return PTR_ERR(*(void **)&misc_base);
	remap_vir_add.misc_base = misc_base;
	hiufs_pltfm_clk_init(crg_base);
	hiufs_pltfm_hardware_init(misc_base);

	/* Perform generic probe */
	err = ufshcd_pltfrm_init(pdev, vops);
	if (err)
		dev_err(dev, "ufshcd_pltfrm_init() failed %d\n", err);

	return err;
}

static int hiufs_pltfm_remove(struct platform_device *pdev)
{
	struct ufs_hba *hba =  platform_get_drvdata(pdev);

	pm_runtime_get_sync(&(pdev)->dev);
	ufshcd_remove(hba);

	return 0;
}

static const struct dev_pm_ops hiufs_pltfm_pm_ops = {
	.suspend	= ufshcd_pltfrm_suspend,
	.resume		= ufshcd_pltfrm_resume,
	.runtime_suspend = ufshcd_pltfrm_runtime_suspend,
	.runtime_resume  = ufshcd_pltfrm_runtime_resume,
	.runtime_idle    = ufshcd_pltfrm_runtime_idle,
};

static struct platform_driver hiufs_pltfm_driver = {
	.probe		= hiufs_pltfm_probe,
	.remove		= hiufs_pltfm_remove,
	.shutdown = ufshcd_pltfrm_shutdown,
	.driver		= {
		.name	= "hiufs_pltfm",
		.pm	= &hiufs_pltfm_pm_ops,
		.of_match_table	= of_match_ptr(hiufs_pltfm_pm_match),
	},
};

module_platform_driver(hiufs_pltfm_driver);

MODULE_AUTHOR("Shengjun Liang");
MODULE_LICENSE("Dual BSD/GPL");

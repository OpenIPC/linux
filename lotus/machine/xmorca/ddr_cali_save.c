//#define DUMP_DDR_PARAM				1

#define HASH_SIZE_PER_BYTE			32
#define DDR_CALIB_SIZE				832
struct lpds_param {
	uint8_t lpds_hash[HASH_SIZE_PER_BYTE];
	uint8_t ddr_hash[HASH_SIZE_PER_BYTE];
	uint32_t data_start;
	uint32_t data_size;
	uint32_t hash_flag;
	uint32_t str_flag;
	uint32_t resume_addr;
	uint32_t resume_addr_backup;
	uint8_t reserve0[24];
	uint8_t scrambling_key[16];
	uint8_t ddr_calib[DDR_CALIB_SIZE];
	uint32_t ddr_size;
	uint32_t hpm_state;
	uint32_t power_state;
	uint8_t reserve1[20];
	uint32_t ddr_suspend_crc;
	uint32_t ddr_resume_crc;
};

struct ddr_calib_data {
	uint32_t calib_data_saved;

	uint32_t gate_dqs0_result;
	uint32_t gate_dqs1_result;

	uint32_t rx_dqs0_result;
	uint32_t rx_dqsb0_result;
	uint32_t rx_dqs1_result;
	uint32_t rx_dqsb1_result;

	uint32_t rx_dm0_result;
	uint32_t rx_dm1_result;

	uint32_t rx_dq_result[16];

	uint32_t tx_dqs0_result;
	uint32_t tx_dqsb0_result;
	uint32_t tx_dqs1_result;
	uint32_t tx_dqsb1_result;

	uint32_t tx_dm0_result;
	uint32_t tx_dm1_result;

	uint32_t tx_dq_result[16];
};

void ddr_phy_save_gate_training(void __iomem *ddrphy_vir, void __iomem *ddr_param_vir)
{
	struct ddr_calib_data *pddr_param = (struct ddr_calib_data *)ddr_param_vir;

	pddr_param->gate_dqs0_result = ((readl(ddrphy_vir + 0x288) >> 16) & 0x7ff);
	pddr_param->gate_dqs1_result = ((readl(ddrphy_vir + 0x388) >> 16) & 0x7ff);

}

#define ddr_phy_read_reg_set(addr, index) 		do {			\
							uint32_t tmp;						\
							tmp = readl((addr));				\
							tmp &= (~(0x1f << 24));				\
							tmp |= (((index) & 0x1f) << 24);	\
							writel(tmp, (addr));				\
													} while (0)

void ddr_phy_save_rd_training(void __iomem *ddrphy_vir, void __iomem *ddr_param_vir)
{
	struct ddr_calib_data *pddr_param = (struct ddr_calib_data *)ddr_param_vir;
	uint32_t i;

	ddr_phy_read_reg_set(ddrphy_vir + 0x228, 0x19);
	pddr_param->rx_dqs0_result = readl(ddrphy_vir + 0x28c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x228, 0x1a);
	pddr_param->rx_dqsb0_result = readl(ddrphy_vir + 0x28c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x328, 0x19);
	pddr_param->rx_dqs1_result = readl(ddrphy_vir + 0x38c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x328, 0x1a);
	pddr_param->rx_dqsb1_result = readl(ddrphy_vir + 0x38c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x228, 0x18);
	pddr_param->rx_dm0_result = readl(ddrphy_vir + 0x28c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x328, 0x18);
	pddr_param->rx_dm1_result = readl(ddrphy_vir + 0x38c);

	for (i = 0; i < 8; i++) {
		ddr_phy_read_reg_set(ddrphy_vir + 0x228, 0x10 + i);
		pddr_param->rx_dq_result[i] = readl(ddrphy_vir + 0x28c);
	}

	for (i = 0; i < 8; i++) {
		ddr_phy_read_reg_set(ddrphy_vir + 0x328, 0x10 + i);
		pddr_param->rx_dq_result[i + 8] = readl(ddrphy_vir + 0x38c);
	}
}

void ddr_phy_save_wr_training(void __iomem *ddrphy_vir, void __iomem *ddr_param_vir)
{
	struct ddr_calib_data *pddr_param = (struct ddr_calib_data *)ddr_param_vir;
	uint32_t i;

	ddr_phy_read_reg_set(ddrphy_vir + 0x228, 0x9);
	pddr_param->tx_dqs0_result = readl(ddrphy_vir + 0x28c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x228, 0xa);
	pddr_param->tx_dqsb0_result = readl(ddrphy_vir + 0x28c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x328, 0x9);
	pddr_param->tx_dqs1_result = readl(ddrphy_vir + 0x38c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x328, 0xa);
	pddr_param->tx_dqsb1_result = readl(ddrphy_vir + 0x38c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x228, 0x8);
	pddr_param->tx_dm0_result = readl(ddrphy_vir + 0x28c);

	ddr_phy_read_reg_set(ddrphy_vir + 0x328, 0x8);
	pddr_param->tx_dm1_result = readl(ddrphy_vir + 0x38c);

	for (i = 0; i < 8; i++) {
		ddr_phy_read_reg_set(ddrphy_vir + 0x228, i);
		pddr_param->tx_dq_result[i] = readl(ddrphy_vir + 0x28c);
	}

	for (i = 0; i < 8; i++) {
		ddr_phy_read_reg_set(ddrphy_vir + 0x328, i);
		pddr_param->tx_dq_result[i + 8] = readl(ddrphy_vir + 0x38c);
	}
}
#ifdef DUMP_DDR_PARAM
void ddr_phy_reg_dump(void __iomem *ddrphy_vir)
{
	u32* phyreg = (u32 *)ddrphy_vir;
	u32 i = 0;

	printk("\nphy reg dump:\n");
	while (i < 0x100) {
		printk("phyreg-0x%x:0x%08x\n", ((u32)(phyreg + i)) & 0xfff, *(phyreg + i));

		i ++;
	}
}

void dump_ddr_param(void __iomem *ddr_param_vir)
{
	u32 *tmp = (u32 *)ddr_param_vir;
	u32 num = sizeof(struct ddr_calib_data) / sizeof(u32);
	u32 i;

	printk("ddr parm:");
	for (i = 0; i < num; i++) {
		if ((i % 8) == 0)
			printk("\n");

		printk("0x%08x ", *tmp);
		tmp++;

	}
	printk("\n\n");
}
#endif
void ddr3_phy_save_calib(void __iomem *ddrphy_vir, void __iomem *ddr_param_vir)
{
	ddr_phy_save_gate_training(ddrphy_vir, ddr_param_vir);
	ddr_phy_save_rd_training(ddrphy_vir, ddr_param_vir);
	ddr_phy_save_wr_training(ddrphy_vir, ddr_param_vir);
#ifdef DUMP_DDR_PARAM
	dump_ddr_param(ddr_param_vir);
	ddr_phy_reg_dump(ddrphy_vir);

#endif
}

void ddr2_phy_save_calib(void __iomem *ddrphy_vir, void __iomem *ddr_param_vir)
{
	ddr_phy_save_gate_training(ddrphy_vir, ddr_param_vir);
}

#define DDR_TYPE_DDR2			0
#define DDR_TYPE_DDR3			1
#define DDR_TYPE_DDR4			2
#define DDR_TYPE_DDR4L			3
static int get_ddr_type(struct xmorca_pm_param *pm_param)
{
	uint32_t val;

	val = readl(pm_param->base_ddrc_vir + 0x0);

	switch (val & 0x7) {
		case 0:
			return DDR_TYPE_DDR2;
		case 1:
			return DDR_TYPE_DDR3;
		case 2:
			return DDR_TYPE_DDR4;
		case 3:
			return DDR_TYPE_DDR4L;
		default:
			return DDR_TYPE_DDR3;
	}
}

static void ddr_phy_save_calib(struct xmorca_pm_param *pm_param)
{
	int ddr_type;
	struct lpds_param *p_lpds_param = (struct lpds_param *)pm_param->ddr_param_vir;

	ddr_type = get_ddr_type(pm_param);
	if (ddr_type == DDR_TYPE_DDR2)
		ddr2_phy_save_calib(pm_param->base_ddrphy_vir, p_lpds_param->ddr_calib);
	else if (ddr_type == DDR_TYPE_DDR3)
		ddr3_phy_save_calib(pm_param->base_ddrphy_vir, p_lpds_param->ddr_calib);

	/* do not support ddr4 and ddr4L */
	else
		ddr3_phy_save_calib(pm_param->base_ddrphy_vir, p_lpds_param->ddr_calib);
}

static void ddr_param_read(struct xmorca_pm_param *pm_param, struct lpds_param *p_lpds_param)
{
	int ddr_type;

	ddr_type = get_ddr_type(pm_param);

	if (ddr_type == DDR_TYPE_DDR2)
		ddr2_phy_save_calib(pm_param->base_ddrphy_vir, p_lpds_param->ddr_calib);
	else if (ddr_type == DDR_TYPE_DDR3)
		ddr3_phy_save_calib(pm_param->base_ddrphy_vir, p_lpds_param->ddr_calib);

	/* do not support ddr4 and ddr4L */
	else
		ddr3_phy_save_calib(pm_param->base_ddrphy_vir, p_lpds_param->ddr_calib);
}

#ifndef _PL310_H_
#define _PL310_H_

#define L2_REG1_CONTROL			(*((volatile unsigned long *)(L2_REG1_BASE + 0x00)))
#define K_L2_REG1_CONTROL_EN_ON		1
#define K_L2_REG1_CONTROL_EN_OFF	0
#define S_L2_REG1_CONTROL_EN		(0)

/* Register bit fields */
#define PL310_AUX_CTRL_ASSOCIATIVITY_MASK	(1 << 16)
#define L2X0_DYNAMIC_CLK_GATING_EN		(1 << 1)
#define L2X0_STNDBY_MODE_EN			(1 << 0)
#define L2X0_CTRL_EN				1

#define L310_SHARED_ATT_OVERRIDE_ENABLE		(1 << 22)
#define L310_AUX_CTRL_DATA_PREFETCH_MASK	(1 << 28)
#define L310_AUX_CTRL_INST_PREFETCH_MASK	(1 << 29)


typedef struct _PL310_REGS {
	unsigned int pl310_cache_id;
	unsigned int pl310_cache_type;
	unsigned int pad1[62];
	unsigned int pl310_ctrl;
	unsigned int pl310_aux_ctrl;
	unsigned int pl310_tag_latency_ctrl;
	unsigned int pl310_data_latency_ctrl;
	unsigned int pad2[60];
	unsigned int pl310_event_cnt_ctrl;
	unsigned int pl310_event_cnt1_cfg;
	unsigned int pl310_event_cnt0_cfg;
	unsigned int pl310_event_cnt1_val;
	unsigned int pl310_event_cnt0_val;
	unsigned int pl310_intr_mask;
	unsigned int pl310_masked_intr_stat;
	unsigned int pl310_raw_intr_stat;
	unsigned int pl310_intr_clear;
	unsigned int pad3[323];
	unsigned int pl310_cache_sync;
	unsigned int pad4[15];
	unsigned int pl310_inv_line_pa;
	unsigned int pad5[2];
	unsigned int pl310_inv_way;
	unsigned int pad6[12];
	unsigned int pl310_clean_line_pa;
	unsigned int pad7[1];
	unsigned int pl310_clean_line_idx;
	unsigned int pl310_clean_way;
	unsigned int pad8[12];
	unsigned int pl310_clean_inv_line_pa;
	unsigned int pad9[1];
	unsigned int pl310_clean_inv_line_idx;
	unsigned int pl310_clean_inv_way;
	unsigned int pad10[64];
	unsigned int pl310_lockdown_dbase;
	unsigned int pl310_lockdown_ibase;
	unsigned int pad11[190];
	unsigned int pl310_addr_filter_start;
	unsigned int pl310_addr_filter_end;
	unsigned int pad12[190];
	unsigned int pl310_test_operation;
	unsigned int pad13[3];
	unsigned int pl310_line_data;
	unsigned int pad14[7];
	unsigned int pl310_line_tag;
	unsigned int pad15[3];
	unsigned int pl310_debug_ctrl;
	unsigned int pad16[7];
	unsigned int pl310_prefetch_ctrl;
	unsigned int pad17[7];
	unsigned int pl310_power_ctrl;
} PL310_REGS;

#endif
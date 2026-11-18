
#ifndef _AX_MIPI_DSI_H_
#define _AX_MIPI_DSI_H_


#define  CDNS_MODE_VIDEO_BURST_CLK_RATIO    (2)

struct cdns_dsi_output {
	struct mipi_dsi_device *dev;
	struct drm_panel *panel;
	struct drm_bridge *bridge;
};

enum cdns_dsi_input_id {
	CDNS_SDI_INPUT,
	CDNS_DPI_INPUT,
	CDNS_DSC_INPUT,
};

struct cdns_dsi_cfg {
	unsigned int hfp;
	unsigned int hsa;
	unsigned int hbp;
	unsigned int hact;
	unsigned int htotal;
};

struct cdns_dsi_input {
	enum cdns_dsi_input_id id;
	struct drm_bridge bridge;
};

struct cdns_dphy_cfg {
	u8 pll_ipdiv;
	u8 pll_opdiv;
	u16 pll_fbdiv;
	unsigned long lane_bps;
	unsigned int nlanes;
};

struct cdns_dphy;

enum cdns_dphy_clk_lane_cfg {
	DPHY_CLK_CFG_LEFT_DRIVES_ALL = 0,
	DPHY_CLK_CFG_LEFT_DRIVES_RIGHT = 1,
	DPHY_CLK_CFG_LEFT_DRIVES_LEFT = 2,
	DPHY_CLK_CFG_RIGHT_DRIVES_ALL = 3,
};
struct cdns_dphy_ops {
	int (*probe)(struct cdns_dphy *dphy);
	void (*remove)(struct cdns_dphy *dphy);
	void (*set_psm_div)(struct cdns_dphy *dphy, u8 div);
	void (*set_clk_lane_cfg)(struct cdns_dphy *dphy, enum cdns_dphy_clk_lane_cfg cfg);
	void (*set_pll_cfg)(struct cdns_dphy *dphy, const struct cdns_dphy_cfg *cfg);
	unsigned long (*get_wakeup_time_ns)(struct cdns_dphy *dphy);
};

struct cdns_dphy {
	struct cdns_dphy_cfg cfg;
	void __iomem *regs;
	void __iomem *power_regs;
	const struct cdns_dphy_ops *ops;
};

struct cdns_dsi {
	struct mipi_dsi_host base;
	void __iomem *regs;
	void __iomem *dispc_sys_regs;
	void __iomem *dphytx_regs;
	void __iomem *comm_sys_regs;
	struct cdns_dsi_input input;
	struct cdns_dsi_output output;
	unsigned int direct_cmd_fifo_depth;
	unsigned int rx_fifo_depth;
	struct completion direct_cmd_comp;
	struct clk *dsi_p_clk;
	struct clk *dsi_sys_clk;
	struct clk *dsi_txesc_clk;
	struct clk *dsi_hs_clk;
	struct clk *pll_ref_clk;
	struct clk *dphytx_esc_clk;
	struct clk *comm_dphytx_tlb_clk;
	struct reset_control *dsi_p_rst;
	struct reset_control *dsi_txpix_rst;
	struct reset_control *dsi_txesc_rst;
	struct reset_control *dsi_sys_rst;
	struct reset_control *dphytx_rst;
	struct reset_control *dphy2dsi_rst;
	struct reset_control *dsi_rx_esc_rst;
	bool link_initialized;
	struct cdns_dphy *dphy;
};

struct mipi_dsi_priv {
	u32 lanes;
	u32 format;
	unsigned long mode_flags;

	void *data;
};

struct ax_mipi_dsi {
	struct mipi_dsi_priv dsi_priv;
	struct drm_encoder encoder;
	struct drm_crtc *crtc;
	bool status;
};

#endif /* end _AX_MIPI_DSI_H_ */

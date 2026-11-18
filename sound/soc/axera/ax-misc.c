/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax-misc.h"
#include <linux/reset.h>
#include <linux/io.h>

int ax_config_mapping(struct dw_i2s_dev *dev)
{
	int ret;
	void __iomem *pinmux_base;
	u32 i2s_exter_codec_en;
	u32 i2s_m_exter_codec_en;
	u32 i2s_exter_codec_mst;
	u32 i2s_m_exter_codec_mst;
	u32 i2s_inner_codec_en;
	u32 i2s_m_aec_cycle_sel;
	u32 i2s_m_aec_sclk_sel;
	u32 i2s_m_rx0_sel;
	u32 i2s_m_rx1_sel;
	u32 i2s_s_rx0_sel;
	u32 i2s_s_rx1_sel;
	u32 i2s_s_sclk_sel;
	u32 iis_out_tdm_en;
	u32 iis_m_out_tdm_en;
	u32 tdm_s_sclk_sel;
	u32 tdm_m_rx_sel;
	u32 tdm_s_rx_sel;
	u32 pinmux_value;
	struct device_node *np = dev->dev->of_node;

/* config pin_sd misc register */
	pinmux_base = devm_ioremap(dev->dev, dev->pinmux_res->start, resource_size(dev->pinmux_res));
	if (IS_ERR(pinmux_base)) {
		pr_err("remap pinmux_base failed\n");
		return PTR_ERR(pinmux_base);
	}
	ret = of_property_read_u32(np, "i2s-m-aec-cycle-sel", &i2s_m_aec_cycle_sel);
	if (ret) {
		pr_err("get i2s_m_aec_cycle_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-m-aec-sclk-sel", &i2s_m_aec_sclk_sel);
	if (ret) {
		pr_err("get i2s_m_aec_sclk_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-inner-codec-en", &i2s_inner_codec_en);
	if (ret) {
		pr_err("get i2s_inner_codec_en failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-exter-codec-en", &i2s_exter_codec_en);
	if (ret) {
		pr_err("get i2s_exter_codec_en failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-m-exter-codec-en", &i2s_m_exter_codec_en);
	if (ret) {
		pr_err("get i2s_m_exter_codec_en failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-exter-codec-mst", &i2s_exter_codec_mst);
	if (ret) {
		pr_err("get i2s_exter_codec_mst failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-m-exter-codec-mst", &i2s_m_exter_codec_mst);
	if (ret) {
		pr_err("get i2s_m_exter_codec_mst failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "iis-out-tdm-en", &iis_out_tdm_en);
	if (ret) {
		pr_err("get iis_out_tdm_en failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "iis-m-out-tdm-en", &iis_m_out_tdm_en);
	if (ret) {
		pr_err("get iis_m_out_tdm_en failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-m-rx0-sel", &i2s_m_rx0_sel);
	if (ret) {
		pr_err("get i2s_m_rx0_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-m-rx1-sel", &i2s_m_rx1_sel);
	if (ret) {
		pr_err("get i2s_m_rx1_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-s-rx0-sel", &i2s_s_rx0_sel);
	if (ret) {
		pr_err("get i2s_s_rx0_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-s-rx1-sel", &i2s_s_rx1_sel);
	if (ret) {
		pr_err("get i2s_s_rx1_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "i2s-s-sclk-sel", &i2s_s_sclk_sel);
	if (ret) {
		pr_err("get i2s_s_sclk_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "tdm-m-rx-sel", &tdm_m_rx_sel);
	if (ret) {
		pr_err("get tdm_m_rx_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "tdm-s-rx-sel", &tdm_s_rx_sel);
	if (ret) {
		pr_err("get tdm_s_rx_sel failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, "tdm-s-sclk-sel", &tdm_s_sclk_sel);
	if (ret) {
		pr_err("get tdm_s_sclk_sel failed\n");
		return ret;
	}

	pinmux_value = (i2s_m_aec_cycle_sel << 23) |
			    (i2s_m_aec_sclk_sel << 21) |
			    (i2s_inner_codec_en << 20) |
				(i2s_exter_codec_en << 19) |
			    (i2s_m_exter_codec_en << 18) |
			    (i2s_exter_codec_mst << 17) |
			    (i2s_m_exter_codec_mst << 16) |
			    (iis_out_tdm_en << 15) |
			    (iis_m_out_tdm_en << 14) |
			    (i2s_m_rx0_sel << 13) |
			    (i2s_m_rx1_sel << 11)|
				(i2s_s_rx0_sel << 9) |
			    (i2s_s_rx1_sel << 7) |
			    (i2s_s_sclk_sel << 5) |
			    (tdm_m_rx_sel << 3) |
				(tdm_s_rx_sel << 1) |
			     tdm_s_sclk_sel ;

	writel(pinmux_value, pinmux_base + PINMUX_OFFSET);
	devm_iounmap(dev->dev, pinmux_base);
	pr_info("config pin_sd_misc to 0x%x\n", pinmux_value);

	return 0;

}


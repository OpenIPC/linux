#if defined(__KERNEL__)
#include <linux/of.h>
#include <linux/clk.h>
#include "plat/pad.h"
#include "kwrap/debug.h"
#include "audcap_builtin.h"
#include "audcap_builtin_platform.h"

void _audcap_builtin_set_clk(int sampling_rate)
{
	struct clk *pll7_clk;

	switch (sampling_rate) {
		case 11025:
		case 22050:
		case 44100: {
            pll7_clk = clk_get(NULL, "pll7");
            if (IS_ERR(pll7_clk)) {
				DBG_ERR("get pll7 failed\r\n");
			}
            clk_set_rate(pll7_clk, 338688000);
		} break;

		case 8000:
		case 12000:
		case 16000:
		case 24000:
		case 32000:
		case 48000:
		default: {
			pll7_clk = clk_get(NULL, "pll7");
            if (IS_ERR(pll7_clk)) {
				DBG_ERR("get pll7 failed\r\n");
			}
            clk_set_rate(pll7_clk, 344064000);
		} break;
	}

		clk_put(pll7_clk);
}

void _audcap_builtin_set_pad(void)
{
	PAD_POWER_STRUCT    pad_power;

	pad_power.pad_power_id     = PAD_POWERID_ADC;
	pad_power.pad_power		   = PAD_1P8V;
	pad_power.bias_current	   = FALSE;
	pad_power.opa_gain		   = FALSE;
	pad_power.pull_down		   = FALSE;
	pad_power.enable		   = ENABLE;
	pad_power.pad_vad		   = PAD_VAD_3P0V;
	pad_set_power(&pad_power);
}

int audcap_builtin_get_samplerate(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int aud_sr = -1;

	if (of_node) {
		if (of_property_read_u32(of_node, "samplerate", &aud_sr) != 0) {
			pr_err("cannot find /fastboot/acap/samplerate");
		}
	}

	return aud_sr;
}

int audcap_builtin_get_channel(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int aud_ch = -1;

	if (of_node) {
		if (of_property_read_u32(of_node, "channel", &aud_ch) != 0) {
			pr_err("cannot find /fastboot/acap/channel");
		}
	}

	return aud_ch;
}

int audcap_builtin_get_bufcount(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int aud_que_cnt = -1;

	if (of_node) {
		if (of_property_read_u32(of_node, "bufnum", &aud_que_cnt) != 0) {
			pr_err("cannot find /fastboot/acap/bufnum");
		}
	}

	return aud_que_cnt;
}

int audcap_builtin_get_bufsamplecnt(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int aud_buf_sample_cnt = -1;

	if (of_node) {
		if (of_property_read_u32(of_node, "bufsamplecnt", &aud_buf_sample_cnt) != 0) {
			pr_err("cannot find /fastboot/acap/bufsamplecnt");
		}
	}

	return aud_buf_sample_cnt;
}

int audcap_builtin_get_rec_src(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int rec_src = -1;

	if (of_node) {
		if (of_property_read_u32(of_node, "recsrc", &rec_src) != 0) {
			pr_err("cannot find /fastboot/acap/recsrc");
		}
	}

	return rec_src;
}

int audcap_builtin_get_vol(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int aud_vol = -1;

	if (of_node) {
		if (of_property_read_u32(of_node, "vol", &aud_vol) != 0) {
			pr_err("cannot find /fastboot/acap/vol");
		}
	}

	return aud_vol;
}

UINT32 audcap_builtin_get_aec_en(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int aec_en = 0;

	if (of_node) {
		if (of_property_read_u32(of_node, "aec", &aec_en) != 0) {
			pr_err("cannot find /fastboot/acap/aec");
		}
	}

	return aec_en;
}

UINT32 audcap_builtin_get_txchannel(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	UINT32 txchannel = 0;

	if (of_node) {
		if (of_property_read_u32(of_node, "txchannel", &txchannel) != 0) {
			pr_err("cannot find /fastboot/acap/txchannel");
		}
	}

	return txchannel;
}

UINT32 audcap_builtin_get_default_setting(void)
{
	struct device_node* of_node = of_find_node_by_path("/fastboot/acap");
	int default_setting = 0;

	if (of_node) {
		if (of_property_read_u32(of_node, "defaultsetting", &default_setting) != 0) {
			pr_err("cannot find /fastboot/acap/defaultsetting");
		}
	}

	return default_setting;
}
#endif

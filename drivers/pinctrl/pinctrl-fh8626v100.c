// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 pin controller
 *
 * Each pad has its own configuration register: mux, bias and a raw
 * drive code. There is no conversion from that code to milliamps.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/slab.h>
#include <linux/soc/fullhan/fh8626-pmu.h>

#define FH_NUM_PADS			77
#define FH_PAD_REG_START		0x80

#define FH_PAD_MUX_SHIFT		24
#define FH_PAD_MUX_MASK			GENMASK(27, 24)
#define FH_PAD_PULL_DOWN		BIT(20)
#define FH_PAD_PULL_UP			BIT(16)
#define FH_PAD_PULL_MASK		(FH_PAD_PULL_DOWN | FH_PAD_PULL_UP)
#define FH_PAD_OUT_EN			BIT(12)
#define FH_PAD_IN_EN			BIT(8)
#define FH_PAD_DRIVE_SHIFT		4
#define FH_PAD_DRIVE_MASK		GENMASK(6, 4)
#define FH_PAD_DRIVE_MAX		7

static const struct pinctrl_pin_desc fh_pins[] = {
	PINCTRL_PIN(0, "pad0"),
	PINCTRL_PIN(1, "pad1"),
	PINCTRL_PIN(2, "pad2"),
	PINCTRL_PIN(3, "pad3"),
	PINCTRL_PIN(4, "pad4"),
	PINCTRL_PIN(5, "pad5"),
	PINCTRL_PIN(6, "pad6"),
	PINCTRL_PIN(7, "pad7"),
	PINCTRL_PIN(8, "pad8"),
	PINCTRL_PIN(9, "pad9"),
	PINCTRL_PIN(10, "pad10"),
	PINCTRL_PIN(11, "pad11"),
	PINCTRL_PIN(12, "pad12"),
	PINCTRL_PIN(13, "pad13"),
	PINCTRL_PIN(14, "pad14"),
	PINCTRL_PIN(15, "pad15"),
	PINCTRL_PIN(16, "pad16"),
	PINCTRL_PIN(17, "pad17"),
	PINCTRL_PIN(18, "pad18"),
	PINCTRL_PIN(19, "pad19"),
	PINCTRL_PIN(20, "pad20"),
	PINCTRL_PIN(21, "pad21"),
	PINCTRL_PIN(22, "pad22"),
	PINCTRL_PIN(23, "pad23"),
	PINCTRL_PIN(24, "pad24"),
	PINCTRL_PIN(25, "pad25"),
	PINCTRL_PIN(26, "pad26"),
	PINCTRL_PIN(27, "pad27"),
	PINCTRL_PIN(28, "pad28"),
	PINCTRL_PIN(29, "pad29"),
	PINCTRL_PIN(30, "pad30"),
	PINCTRL_PIN(31, "pad31"),
	PINCTRL_PIN(32, "pad32"),
	PINCTRL_PIN(33, "pad33"),
	PINCTRL_PIN(34, "pad34"),
	PINCTRL_PIN(35, "pad35"),
	PINCTRL_PIN(36, "pad36"),
	PINCTRL_PIN(37, "pad37"),
	PINCTRL_PIN(38, "pad38"),
	PINCTRL_PIN(39, "pad39"),
	PINCTRL_PIN(40, "pad40"),
	PINCTRL_PIN(41, "pad41"),
	PINCTRL_PIN(42, "pad42"),
	PINCTRL_PIN(43, "pad43"),
	PINCTRL_PIN(44, "pad44"),
	PINCTRL_PIN(45, "pad45"),
	PINCTRL_PIN(46, "pad46"),
	PINCTRL_PIN(47, "pad47_cis_clk"),
	PINCTRL_PIN(48, "pad48_gpio13"),
	PINCTRL_PIN(49, "pad49_gpio14"),
	PINCTRL_PIN(50, "pad50_uart0_rx"),
	PINCTRL_PIN(51, "pad51_uart0_tx"),
	PINCTRL_PIN(52, "pad52_i2c0_scl"),
	PINCTRL_PIN(53, "pad53_i2c0_sda"),
	PINCTRL_PIN(54, "pad54"),
	PINCTRL_PIN(55, "pad55"),
	PINCTRL_PIN(56, "pad56_ssi0_clk"),
	PINCTRL_PIN(57, "pad57_ssi0_txd"),
	PINCTRL_PIN(58, "pad58_gpio54"),
	PINCTRL_PIN(59, "pad59"),
	PINCTRL_PIN(60, "pad60_ssi0_rxd"),
	PINCTRL_PIN(61, "pad61"),
	PINCTRL_PIN(62, "pad62"),
	PINCTRL_PIN(63, "pad63"),
	PINCTRL_PIN(64, "pad64_sd0_clk"),
	PINCTRL_PIN(65, "pad65_sd0_cmd"),
	PINCTRL_PIN(66, "pad66_sd0_d0"),
	PINCTRL_PIN(67, "pad67_sd0_d1"),
	PINCTRL_PIN(68, "pad68_sd0_d2"),
	PINCTRL_PIN(69, "pad69_sd0_d3"),
	PINCTRL_PIN(70, "pad70"),
	PINCTRL_PIN(71, "pad71"),
	PINCTRL_PIN(72, "pad72"),
	PINCTRL_PIN(73, "pad73"),
	PINCTRL_PIN(74, "pad74"),
	PINCTRL_PIN(75, "pad75"),
	PINCTRL_PIN(76, "pad76"),
};

static const unsigned int i2c0_pins[] = { 52, 53 };
static const unsigned int uart0_pins[] = { 50, 51 };
static const unsigned int sensor_pins[] = { 47, 48 };
static const unsigned int cis_clk_pins[] = { 47 };
static const unsigned int gpio13_pins[] = { 48 };
/* SSI0 CLK, TXD and RXD; chip select is GPIO54 on pad 58. */
static const unsigned int spi0_pins[] = { 56, 57, 60 };
static const unsigned int spi0_cs_pins[] = { 58 };
/* GPIO2, the reset button input. */
static const unsigned int reset_btn_pins[] = { 37 };
static const unsigned int mmc0_pins[] = { 64, 65, 66, 67, 68, 69 };

struct fh_pinctrl_group {
	const char		*name;
	const unsigned int	*pins;
	unsigned int		num_pins;
};

static const struct fh_pinctrl_group fh_groups[] = {
	{ "i2c0_grp", i2c0_pins, ARRAY_SIZE(i2c0_pins) },
	{ "uart0_grp", uart0_pins, ARRAY_SIZE(uart0_pins) },
	{ "sensor_grp", sensor_pins, ARRAY_SIZE(sensor_pins) },
	{ "cis_clk_grp", cis_clk_pins, ARRAY_SIZE(cis_clk_pins) },
	{ "gpio13_grp", gpio13_pins, ARRAY_SIZE(gpio13_pins) },
	{ "spi0_grp", spi0_pins, ARRAY_SIZE(spi0_pins) },
	{ "spi0_cs_grp", spi0_cs_pins, ARRAY_SIZE(spi0_cs_pins) },
	{ "reset_btn_grp", reset_btn_pins, ARRAY_SIZE(reset_btn_pins) },
	{ "mmc0_grp", mmc0_pins, ARRAY_SIZE(mmc0_pins) },
};

static const char * const i2c0_funcs[] = { "i2c0_grp" };
static const char * const uart0_funcs[] = { "uart0_grp" };
static const char * const sensor_funcs[] = {
	"sensor_grp", "cis_clk_grp", "gpio13_grp"
};

static const char * const spi0_funcs[] = { "spi0_grp" };
static const char * const mmc0_funcs[] = { "mmc0_grp" };
static const char * const gpio_funcs[] = {
	"i2c0_grp", "uart0_grp", "sensor_grp", "cis_clk_grp", "gpio13_grp",
	"spi0_grp", "spi0_cs_grp", "reset_btn_grp", "mmc0_grp"
};

struct fh_pmx_func {
	const char		*name;
	const char * const	*groups;
	unsigned int		num_groups;
};

static const struct fh_pmx_func fh_pmx_functions[] = {
	{ "i2c0", i2c0_funcs, ARRAY_SIZE(i2c0_funcs) },
	{ "uart0", uart0_funcs, ARRAY_SIZE(uart0_funcs) },
	{ "sensor", sensor_funcs, ARRAY_SIZE(sensor_funcs) },
	{ "spi0", spi0_funcs, ARRAY_SIZE(spi0_funcs) },
	{ "mmc0", mmc0_funcs, ARRAY_SIZE(mmc0_funcs) },
	{ "gpio", gpio_funcs, ARRAY_SIZE(gpio_funcs) },
};

struct fh_pinctrl {
	struct device		*dev;
	struct pinctrl_dev	*pctl;
};

static int fh_pad_update_raw(unsigned int pad_id, u32 mask, u32 val)
{
	unsigned int reg = FH_PAD_REG_START + pad_id * 4;

	if (pad_id >= FH_NUM_PADS)
		return -EINVAL;

	/* Pad registers share the PMU lock with the clock provider. */
	return fh8626_pmu_update_bits(reg, mask, val);
}

/*
 * Function-select nibble for a named SoC function. Bias and drive are not
 * part of this table; those come from the pin configuration in the DT.
 * FH_NUM_PADS matches every pad of that function.
 */
struct fh_func_sel {
	const char	*func;
	unsigned int	pad;
	u8		sel;
};

static const struct fh_func_sel fh_func_sels[] = {
	{ "i2c0", FH_NUM_PADS, 0 },
	/* Pad 50 function 1 is GPIO48. UART0 uses function 0. */
	{ "uart0", FH_NUM_PADS, 0 },
	{ "sensor", 47, 1 },
	{ "sensor", 48, 0 },
	{ "spi0", FH_NUM_PADS, 0 },
	{ "mmc0", FH_NUM_PADS, 0 },
	{ "gpio", 47, 0 },
	{ "gpio", 48, 0 },
	{ "gpio", FH_NUM_PADS, 1 },
};

static int fh_func_sel(const char *func, unsigned int pad, u8 *sel)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(fh_func_sels); i++) {
		if (strcmp(fh_func_sels[i].func, func))
			continue;
		if (fh_func_sels[i].pad != FH_NUM_PADS &&
		    fh_func_sels[i].pad != pad)
			continue;
		*sel = fh_func_sels[i].sel;
		return 0;
	}

	return -EINVAL;
}

static int fh_pctl_get_groups_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(fh_groups);
}

static const char *fh_pctl_get_group_name(struct pinctrl_dev *pctldev,
					  unsigned int selector)
{
	return fh_groups[selector].name;
}

static int fh_pctl_get_group_pins(struct pinctrl_dev *pctldev,
				  unsigned int selector,
				  const unsigned int **pins,
				  unsigned int *num_pins)
{
	*pins = fh_groups[selector].pins;
	*num_pins = fh_groups[selector].num_pins;
	return 0;
}

static const struct pinctrl_ops fh_pctl_ops = {
	.get_groups_count	= fh_pctl_get_groups_count,
	.get_group_name		= fh_pctl_get_group_name,
	.get_group_pins		= fh_pctl_get_group_pins,
	.dt_node_to_map		= pinconf_generic_dt_node_to_map_all,
	.dt_free_map		= pinconf_generic_dt_free_map,
};

static int fh_pmx_get_funcs_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(fh_pmx_functions);
}

static const char *fh_pmx_get_func_name(struct pinctrl_dev *pctldev,
					unsigned int selector)
{
	return fh_pmx_functions[selector].name;
}

static int fh_pmx_get_groups(struct pinctrl_dev *pctldev,
			     unsigned int selector,
			     const char * const **groups,
			     unsigned int * const num_groups)
{
	*groups = fh_pmx_functions[selector].groups;
	*num_groups = fh_pmx_functions[selector].num_groups;
	return 0;
}

static int fh_pmx_set_mux(struct pinctrl_dev *pctldev,
			  unsigned int func_selector,
			  unsigned int group_selector)
{
	struct fh_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_group *grp = &fh_groups[group_selector];
	const char *func_name = fh_pmx_functions[func_selector].name;
	unsigned int i;
	int ret;

	for (i = 0; i < grp->num_pins; i++) {
		unsigned int pad_id = grp->pins[i];
		u32 mask = FH_PAD_MUX_MASK | FH_PAD_OUT_EN | FH_PAD_IN_EN;
		u8 func;
		u32 val;

		ret = fh_func_sel(func_name, pad_id, &func);
		if (ret) {
			dev_err(pctl->dev, "no mux select for %s on pad %u\n",
				func_name, pad_id);
			return ret;
		}

		/*
		 * Enable the pin with the selected function. Bias and drive
		 * stay with pinconf.
		 */
		val = ((u32)func << FH_PAD_MUX_SHIFT) |
		      FH_PAD_OUT_EN | FH_PAD_IN_EN;
		ret = fh_pad_update_raw(pad_id, mask, val);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct pinmux_ops fh_pmx_ops = {
	.get_functions_count	= fh_pmx_get_funcs_count,
	.get_function_name	= fh_pmx_get_func_name,
	.get_function_groups	= fh_pmx_get_groups,
	.set_mux		= fh_pmx_set_mux,
};

/*
 * Raw pad drive code. Generic drive-strength is in milliamps, and the
 * code-to-milliamp mapping is unknown.
 */
#define FH_PIN_CONFIG_DRIVE	(PIN_CONFIG_END + 1)

static const struct pinconf_generic_params fh_custom_params[] = {
	{ "fullhan,drive", FH_PIN_CONFIG_DRIVE, 0 },
};

#ifdef CONFIG_DEBUG_FS
static const struct pin_config_item fh_custom_conf_items[] = {
	PCONFDUMP(FH_PIN_CONFIG_DRIVE, "drive code", NULL, true),
};
#endif

static int fh_pinconf_get(struct pinctrl_dev *pctldev, unsigned int pin,
			  unsigned long *config)
{
	unsigned int param = pinconf_to_config_param(*config);
	u32 val = 0;
	int ret;

	if (pin >= FH_NUM_PADS)
		return -EINVAL;

	ret = fh8626_pmu_read(FH_PAD_REG_START + pin * 4, &val);
	if (ret)
		return ret;

	switch (param) {
	case PIN_CONFIG_BIAS_PULL_UP:
		if (!(val & FH_PAD_PULL_UP))
			return -EINVAL;
		*config = pinconf_to_config_packed(param, 1);
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		if (!(val & FH_PAD_PULL_DOWN))
			return -EINVAL;
		*config = pinconf_to_config_packed(param, 1);
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		if (val & FH_PAD_PULL_MASK)
			return -EINVAL;
		*config = pinconf_to_config_packed(param, 1);
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		return -ENOTSUPP;
	case FH_PIN_CONFIG_DRIVE:
		*config = pinconf_to_config_packed(param,
			(val & FH_PAD_DRIVE_MASK) >> FH_PAD_DRIVE_SHIFT);
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		if (!(val & FH_PAD_IN_EN))
			return -EINVAL;
		*config = pinconf_to_config_packed(param, 1);
		break;
	case PIN_CONFIG_OUTPUT:
		/*
		 * The GPIO data register holds the output level. This
		 * controller only has the pad output enable.
		 */
		return -ENOTSUPP;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int fh_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			  unsigned long *configs, unsigned int num_configs)
{
	unsigned int i;
	u32 mask = 0;
	u32 val = 0;

	if (pin >= FH_NUM_PADS)
		return -EINVAL;

	for (i = 0; i < num_configs; i++) {
		unsigned int param = pinconf_to_config_param(configs[i]);
		u32 arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
		case PIN_CONFIG_BIAS_PULL_UP:
			mask |= FH_PAD_PULL_MASK;
			val |= FH_PAD_PULL_UP;
			val &= ~FH_PAD_PULL_DOWN;
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			mask |= FH_PAD_PULL_MASK;
			val |= FH_PAD_PULL_DOWN;
			val &= ~FH_PAD_PULL_UP;
			break;
		case PIN_CONFIG_BIAS_DISABLE:
			mask |= FH_PAD_PULL_MASK;
			val &= ~FH_PAD_PULL_MASK;
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			return -ENOTSUPP;
		case FH_PIN_CONFIG_DRIVE:
			if (arg > FH_PAD_DRIVE_MAX)
				return -EINVAL;
			mask |= FH_PAD_DRIVE_MASK;
			val &= ~FH_PAD_DRIVE_MASK;
			val |= arg << FH_PAD_DRIVE_SHIFT;
			break;
		case PIN_CONFIG_INPUT_ENABLE:
			mask |= FH_PAD_IN_EN;
			val &= ~FH_PAD_IN_EN;
			if (arg)
				val |= FH_PAD_IN_EN;
			break;
		case PIN_CONFIG_OUTPUT:
			return -ENOTSUPP;
		default:
			return -ENOTSUPP;
		}
	}

	return fh_pad_update_raw(pin, mask, val);
}

static int fh_pinconf_group_set(struct pinctrl_dev *pctldev, unsigned int group,
				unsigned long *configs,
				unsigned int num_configs)
{
	const struct fh_pinctrl_group *grp = &fh_groups[group];
	unsigned int i;
	int ret;

	for (i = 0; i < grp->num_pins; i++) {
		ret = fh_pinconf_set(pctldev, grp->pins[i], configs,
				     num_configs);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct pinconf_ops fh_conf_ops = {
	.is_generic		= true,
	.pin_config_get		= fh_pinconf_get,
	.pin_config_set		= fh_pinconf_set,
	.pin_config_group_set	= fh_pinconf_group_set,
};

static struct pinctrl_desc fh_pinctrl_desc = {
	.name		= "fh8626v100-pinctrl",
	.pins		= fh_pins,
	.npins		= ARRAY_SIZE(fh_pins),
	.pctlops	= &fh_pctl_ops,
	.pmxops		= &fh_pmx_ops,
	.confops	= &fh_conf_ops,
	.owner		= THIS_MODULE,
	.num_custom_params = ARRAY_SIZE(fh_custom_params),
	.custom_params	= fh_custom_params,
#ifdef CONFIG_DEBUG_FS
	.custom_conf_items = fh_custom_conf_items,
#endif
};

static int fh_pinctrl_probe(struct platform_device *pdev)
{
	struct fh_pinctrl *pctl;
	u32 pad;
	int ret;

	/*
	 * The pad registers belong to the PMU, which the clock provider maps
	 * and locks; this defers until that provider has probed.
	 */
	ret = fh8626_pmu_read(FH_PAD_REG_START, &pad);
	if (ret)
		return ret;

	pctl = devm_kzalloc(&pdev->dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl)
		return -ENOMEM;

	pctl->dev = &pdev->dev;

	pctl->pctl = devm_pinctrl_register(&pdev->dev, &fh_pinctrl_desc, pctl);
	if (IS_ERR(pctl->pctl)) {
		ret = PTR_ERR(pctl->pctl);
		dev_err(&pdev->dev, "failed to register pinctrl: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, pctl);
	return 0;
}

static const struct of_device_id fh_pinctrl_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-pinctrl" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_pinctrl_of_match);

static struct platform_driver fh_pinctrl_driver = {
	.probe = fh_pinctrl_probe,
	.driver = {
		.name = "fh8626v100-pinctrl",
		.of_match_table = fh_pinctrl_of_match,
	},
};
module_platform_driver(fh_pinctrl_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 Pin Controller Driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");

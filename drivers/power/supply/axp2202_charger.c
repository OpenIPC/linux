#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include "linux/irq.h"
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/err.h>
#include "linux/mfd/axp2101.h"

#define AXP2202_VBAT_MAX        (8000)
#define AXP2202_VBAT_MIN        (2000)
#define AXP2202_SOC_MAX         (100)
#define AXP2202_SOC_MIN         (0)
#define AXP2202_MASK_VBUS_STATE BIT(5)
#define AXP2202_MODE_RSTGAUGE   BIT(3)
#define AXP2202_MODE_RSTMCU     BIT(2)
#define AXP2202_MAX_PARAM       512
#define AXP2202_BROMUP_EN       BIT(0)
#define AXP2202_CFG_UPDATE_MARK BIT(4)

#define AXP2202_CHARGING_TRI  (0)
#define AXP2202_CHARGING_PRE  (1)
#define AXP2202_CHARGING_CC   (2)
#define AXP2202_CHARGING_CV   (3)
#define AXP2202_CHARGING_DONE (4)
#define AXP2202_CHARGING_NCHG (5)
#define AXP2202_MANUFACTURER  "xpower,axp2202"

struct axp_interrupts {
	char *name;
	irq_handler_t isr;
	int irq;
};

struct axp2202_dts_config {
	u32 pmu_battery_cap;
	u32 pmu_chg_ic_temp;
	u32 pmu_init_chgvol;
	u32 pmu_chgled_type;
	u32 pmu_battery_warning_level1;
	u32 pmu_battery_warning_level2;
	u32 pmu_chled_enable;
	u32 pmu_usbpc_cur;
	u32 pmu_runtime_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_bat_det;;
	u32 pmu_btn_chg_en;
	u32 pmu_btn_chg_cfg;

	bool wakeup_usb_in;
	bool wakeup_usb_out;
	bool wakeup_ac_in;
	bool wakeup_ac_out;
	bool wakeup_bat_in;
	bool wakeup_bat_out;
	bool wakeup_bat_charging;
	bool wakeup_bat_charge_over;
	bool wakeup_low_warning1;
	bool wakeup_low_warning2;
	bool wakeup_bat_untemp_work;
	bool wakeup_bat_ovtemp_work;
	bool wakeup_untemp_chg;
	bool wakeup_ovtemp_chg;
};

struct axp2202_state {
	int bat_stat;
	int bat_full;
	int bat_read;
	int usb_connect;
};

struct axp2202_device_info {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *bat;
	struct power_supply       *usb;
	struct power_supply       *ac;
	struct delayed_work        bat_chk;
	struct axp2202_dts_config  dts_info;
	/* for bug can not detect battery */
	struct axp2202_state   stat;
};

static enum power_supply_property axp2202_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
};

static enum power_supply_property axp2202_usb_ac_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};

static int axp2202_read_vbat(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	uint8_t data[2];
	uint16_t vtemp[3], tempv;
	int ret = 0;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2202_VBAT_H, data, 2);
		if (ret < 0)
			return ret;

		vtemp[i] = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	}
	if (vtemp[0] > vtemp[1]) {
		tempv = vtemp[0];
		vtemp[0] = vtemp[1];
		vtemp[1] = tempv;
	}
	if (vtemp[1] > vtemp[2]) {
		tempv = vtemp[1];
		vtemp[1] = vtemp[2];
		vtemp[2] = tempv;
	}
	if (vtemp[0] > vtemp[1]) {
		tempv = vtemp[0];
		vtemp[0] = vtemp[1];
		vtemp[1] = tempv;
	}
	/*incase vtemp[1] exceed AXP210X_VBAT_MAX */
	if ((vtemp[1] > AXP2202_VBAT_MAX) || (vtemp[1] < AXP2202_VBAT_MIN)) {
		val->intval = 0;
		return 0;
	}

	val->intval = vtemp[1];

	return 0;
}

/* read temperature */
static int axp2202_read_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	/*
	 * uint8_t data[2];
	 * int ret = 0;
	 * struct axp2202_device_info *di = power_supply_get_drvdata(ps);
	 * struct regmap *regmap = di->regmap;
	 * val->intval = 0;
	 */
	val->intval = 0;

	return 0;
}

static int axp2202_read_soc(struct power_supply *ps,
			    union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2202_GAUGE_SOC, &data);
	if (ret < 0)
		return ret;

	if (data > AXP2202_SOC_MAX)
		data = AXP2202_SOC_MAX;
	else if (data < AXP2202_SOC_MIN)
		data = AXP2202_SOC_MIN;

	val->intval = data;

	return 0;
}

static int axp2202_read_time2empty(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	uint8_t data[2];
	uint16_t ttemp[3], tempt;
	int ret = 0;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2202_GAUGE_TIME2EMPTY_H, data,
				       2);
		if (ret < 0)
			return ret;

		ttemp[i] = ((data[0] << 0x08) | (data[1]));
	}

	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	if (ttemp[1] > ttemp[2]) {
		tempt = ttemp[1];
		ttemp[1] = ttemp[2];
		ttemp[2] = tempt;
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}

	val->intval = ttemp[1];

	return 0;
}

static int axp2202_read_vbus_state(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2202_COMM_STAT0, &data);
	if (ret < 0)
		return ret;

	/* vbus is good when vbus state set */
	val->intval = !!(data & AXP2202_MASK_VBUS_STATE);

	return ret;
}

static int axp2202_read_time2full(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	uint16_t ttemp[3], tempt;
	uint8_t data[2];
	int ret = 0;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2202_GAUGE_TIME2FULL_H, data,
				       2);
		if (ret < 0)
			return ret;

		ttemp[i] = ((data[0] << 0x08) | (data[1]));
	}

	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	if (ttemp[1] > ttemp[2]) {
		tempt = ttemp[1];
		ttemp[1] = ttemp[2];
		ttemp[2] = tempt;
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}

	val->intval = ttemp[1];

	return 0;
}

static int axp2202_read_lowsocth(struct power_supply *ps,
				 union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2202_GAUGE_THLD, &data);
	if (ret < 0)
		return ret;

	val->intval = (data >> 4) + 5;

	return 0;
}

static int axp2202_set_lowsocth(struct regmap *regmap, int v)
{
	unsigned int data;
	int ret = 0;

	data = v;

	if (data > 20 || data < 5)
		return -EINVAL;

	data = (data - 5);
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_THLD, GENMASK(7, 4),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp2202_reset_gauge(struct regmap *regmap)
{
	int ret = 0;

	ret = regmap_update_bits(regmap, AXP2202_RESET_CFG,
				 AXP2202_MODE_RSTGAUGE, AXP2202_MODE_RSTGAUGE);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp2202_reset_mcu(struct regmap *regmap)
{
	int ret = 0;

	ret = regmap_update_bits(regmap, AXP2202_RESET_CFG, AXP2202_MODE_RSTMCU,
				 AXP2202_MODE_RSTMCU);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(regmap, AXP2202_RESET_CFG, AXP2202_MODE_RSTMCU,
				 0);
	if (ret < 0)
		return ret;

	return 0;
}

/**
 * axp2202_get_param - get battery config from dts
 *
 * is not get battery config parameter from dts,
 * then it use the default config.
 */
static int axp2202_get_param(struct axp2202_device_info *di, uint8_t *d,
			     unsigned int *len)
{
	struct device_node *n_para, *r_para;
	const char *pparam;
	int cnt;

	n_para = of_parse_phandle(di->dev->of_node, "param", 0);
	if (!n_para)
		goto e_n_para;

	if (of_property_read_string(n_para, "select", &pparam))
		goto e_para;

	r_para = of_get_child_by_name(n_para, pparam);
	if (!r_para)
		goto e_para;

	cnt = of_property_read_variable_u8_array(r_para, "parameter", d, 1,
						 *len);
	if (cnt <= 0)
		goto e_n_parameter;
	*len = cnt;

	of_node_put(r_para);
	of_node_put(n_para);

	return 0;

e_n_parameter:
	of_node_put(r_para);
e_para:
	of_node_put(n_para);
e_n_para:
	return -ENODATA;
}

static int axp2202_model_update(struct axp2202_device_info *di)
{
	struct regmap *regmap = di->regmap;
	int ret = 0;
	unsigned int data;
	unsigned int len;
	uint8_t i;
	uint8_t *param;

	/* reset_mcu */
	ret = axp2202_reset_mcu(di->regmap);
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset and open brom */
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, 0);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, AXP2202_BROMUP_EN);
	if (ret < 0)
		goto UPDATE_ERR;

	/* down load battery parameters */
	len = AXP2202_MAX_PARAM;
	param = devm_kzalloc(di->dev, AXP2202_MAX_PARAM, GFP_KERNEL);
	if (!param) {
		pr_err("can not find memory for param\n");
		goto UPDATE_ERR;
	}
	ret = axp2202_get_param(di, param, &len);
	if (ret < 0)
		goto err_param;

	for (i = 0; i < len; i++) {
		ret = regmap_write(regmap, AXP2202_GAUGE_BROM, param[i]);
		if (ret < 0)
			goto err_param;
	}
	/* reset and open brom */
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, 0);
	if (ret < 0)
		goto err_param;

	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, AXP2202_BROMUP_EN);
	if (ret < 0)
		goto err_param;

	/* check battery parameters is ok ? */
	for (i = 0; i < len; i++) {
		ret = regmap_read(regmap, AXP2202_GAUGE_BROM, &data);
		if (ret < 0)
			goto err_param;

		if (data != param[i]) {
			pr_err("model param check %02x error!\n", i);
			goto err_param;
		}
	}

	devm_kfree(di->dev, param);

	/* close brom and set battery update flag */
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG, AXP2202_BROMUP_EN,
				 0);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_CFG_UPDATE_MARK,
				 AXP2202_CFG_UPDATE_MARK);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_read(regmap, AXP2202_GAUGE_CONFIG, &data);
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset_mcu */
	ret = axp2202_reset_mcu(regmap);
	if (ret < 0)
		goto UPDATE_ERR;

	/* update ok */
	return 0;

err_param:
	devm_kfree(di->dev, param);

UPDATE_ERR:
	regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG, AXP2202_BROMUP_EN, 0);
	axp2202_reset_mcu(regmap);

	return ret;
}


static bool axp2202_model_update_check(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;

	ret = regmap_read(regmap, AXP2202_GAUGE_CONFIG, &data);
	if (ret < 0)
		goto CHECK_ERR;

	if ((data & AXP2202_CFG_UPDATE_MARK) == 0)
		goto CHECK_ERR;


	return true;

CHECK_ERR:
	regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG, AXP2202_BROMUP_EN, 0);
	axp2202_reset_mcu(regmap);
	return false;
}

static int axp2202_reg_update(struct regmap *regmap)
{
	int ret = 0;
	uint8_t data[2];

	data[0] = 0x10;
	ret = regmap_bulk_write(regmap, AXP2202_TS_CFG, data, 1);
	if (ret < 0)
		return ret;

	/* set led not bright power on first */
	regmap_update_bits(regmap, AXP2202_MODULE_EN, BIT(2), 0);

	return 0;
}

static int axp2202_usb_ac_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp2202_read_vbus_state(psy, val);
		break;
	default:
		break;
	}

	return ret;
}

static int axp2202_get_bat_status(struct power_supply *psy,
				  union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(psy);
	struct regmap *regmap = di->regmap;
	unsigned int data;
	int ret;

	/* some bug cause can't get battery out */
	if (!di->stat.bat_stat) {
		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		return 0;
	}

	ret = regmap_read(regmap, AXP2202_COMM_STAT1, &data);
	if (ret < 0) {
		dev_dbg(&psy->dev, "error read AXP210X_COM_STAT1\n");
		return ret;
	}

	/* chg_stat = bit[2:0] */
	switch (data & 0x07) {
	case AXP2202_CHARGING_TRI:
	case AXP2202_CHARGING_NCHG:
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case AXP2202_CHARGING_PRE:
	case AXP2202_CHARGING_CC:
	case AXP2202_CHARGING_CV:
		val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case AXP2202_CHARGING_DONE:
		if (di->stat.bat_full)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	default:
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	}

	return 0;
}

static int axp2202_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;
	struct axp2202_device_info *di = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL: // customer modify
		ret = axp2202_read_soc(psy, val);
		if (ret < 0)
			return ret;

		if (val->intval == 100)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
		else if (val->intval > 80)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_HIGH;
		else if (val->intval > di->dts_info.pmu_battery_warning_level1)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		else if (val->intval > di->dts_info.pmu_battery_warning_level2)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
		else if (val->intval >= 0)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp2202_get_bat_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = di->stat.bat_stat;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp2202_read_vbat(psy, val);
		if (ret < 0)
			return ret;

		val->intval = val->intval * 1000; /* unit uV; */
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = di->dts_info.pmu_battery_cap;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if ((di->stat.bat_stat) && (di->stat.bat_read))
			ret = axp2202_read_soc(psy, val); // unit %;
		else
			val->intval = -1;

		if (ret < 0)
			return ret;

		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp2202_read_lowsocth(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp2202_read_temp(psy, val);
		if (ret < 0)
			return ret;

		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MIN:
		val->intval = 85;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = axp2202_read_time2empty(psy, val);
		if (ret < 0)
			return ret;

		val->intval = val->intval * 60;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = axp2202_read_time2full(psy, val);
		if (ret < 0)
			return ret;

		val->intval = val->intval * 60;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP2202_MANUFACTURER;
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int axp2202_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp2202_device_info *di = power_supply_get_drvdata(psy);
	struct regmap *regmap = di->regmap;
	int ret = 0;

	if (psp != POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN)
		ret = -EINVAL;
	else
		ret = axp2202_set_lowsocth(regmap, val->intval);

	return ret;
}

static int axp2202_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;

	if (psp != POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN)
		ret = -EINVAL;
	else
		ret = 0;

	return ret;
}

static int axp2202_register_battery(struct axp2202_device_info *di)
{
	int ret = 0;
	struct power_supply_desc *psy_desc;
	struct power_supply_desc *usb_desc, *ac_desc;
	struct power_supply_config psy_cfg = {
		.of_node = di->dev->of_node,
		.drv_data = di,
	};

	psy_desc = devm_kzalloc(di->dev, sizeof(*psy_desc), GFP_KERNEL);
	if (!psy_desc)
		return -ENOMEM;

	psy_desc->name = "battery";
	psy_desc->type = POWER_SUPPLY_TYPE_BATTERY;
	psy_desc->properties = axp2202_props;
	psy_desc->num_properties = ARRAY_SIZE(axp2202_props);
	psy_desc->get_property = axp2202_get_property;
	psy_desc->set_property = axp2202_set_property;
	psy_desc->property_is_writeable = axp2202_writeable;

	di->bat = power_supply_register(di->dev, psy_desc, &psy_cfg);
	if (IS_ERR(di->bat)) {
		dev_err(di->dev, "failed to register battery\n");
		ret = PTR_ERR(di->bat);
		return ret;
	}

	usb_desc = devm_kzalloc(di->dev, sizeof(*usb_desc), GFP_KERNEL);
	if (!usb_desc) {
		ret = -ENOMEM;
		goto err1;
	}

	usb_desc->name = "usb";
	usb_desc->type = POWER_SUPPLY_TYPE_USB;
	usb_desc->properties = axp2202_usb_ac_props;
	usb_desc->num_properties = ARRAY_SIZE(axp2202_usb_ac_props);
	usb_desc->get_property = axp2202_usb_ac_get_property;
	usb_desc->set_property = NULL;
	usb_desc->property_is_writeable = NULL;

	di->usb = power_supply_register(di->dev, usb_desc, &psy_cfg);
	if (IS_ERR(di->usb)) {
		dev_err(di->dev, "failed to register usb\n");
		ret = PTR_ERR(di->bat);
		goto err1;
	}

	ac_desc = devm_kzalloc(di->dev, sizeof(*ac_desc), GFP_KERNEL);
	if (!ac_desc) {
		ret = -ENOMEM;
		goto err2;
	}

	ac_desc->name = "ac";
	ac_desc->type = POWER_SUPPLY_TYPE_MAINS;
	ac_desc->properties = axp2202_usb_ac_props;
	ac_desc->num_properties = ARRAY_SIZE(axp2202_usb_ac_props);
	ac_desc->get_property = axp2202_usb_ac_get_property;
	ac_desc->set_property = NULL;
	;
	ac_desc->property_is_writeable = NULL;

	di->ac = power_supply_register(di->dev, ac_desc, &psy_cfg);
	if (IS_ERR(di->ac)) {
		dev_err(di->dev, "failed to register battery\n");
		ret = PTR_ERR(di->bat);
		goto err2;
	}

	return ret;

err2:
	power_supply_unregister(di->usb);
err1:
	power_supply_unregister(di->bat);

	return ret;
}

static void axp2202_teardown_battery(struct axp2202_device_info *di)
{
	if (di->bat)
		power_supply_unregister(di->bat);

	if (di->ac)
		power_supply_unregister(di->ac);

	if (di->usb)
		power_supply_unregister(di->usb);
}

static int axp2202_init_chip(struct axp2202_device_info *di)
{
	int ret = 0;

	if (di == NULL) {
		dev_err(di->dev, "axp2202_info is invalid!\n");
		return -ENODEV;
	}

	ret = axp2202_reg_update(di->regmap);
	if (ret < 0) {
		dev_err(di->dev, "axp2202 reg update, i2c communication err!\n");
		return ret;
	}

	if (!axp2202_model_update_check(di->regmap)) {
		ret = axp2202_model_update(di);
		if (ret < 0) {
			dev_err(di->dev, "axp2202 model update fail!\n");
			return ret;
		}
	}
	dev_dbg(di->dev, "axp2202 model update ok\n");

	/* after 500ms can read soc */
	ret = axp2202_reg_update(di->regmap);
	if (ret < 0) {
		dev_err(di->dev,
			"axp2202 reg update, i2c communication err!\n");
		return ret;
	}

	return ret;
}

static irqreturn_t axp2202_irq_handler_usb_in(int irq, void *data)
{
	struct axp2202_device_info *di = data;
	unsigned int value = 0;

	power_supply_changed(di->bat);
	regmap_read(di->regmap, AXP2202_COMM_STAT0, &value);

	if (value & BIT(5)) {
		di->stat.usb_connect = 1;
	} else {
		di->stat.usb_connect = 0;
	}

	return IRQ_HANDLED;
}

static irqreturn_t axp2202_irq_handler_usb_out(int irq, void *data)
{
	struct axp2202_device_info *di = data;
	u32 value = 0;

	power_supply_changed(di->bat);
	regmap_read(di->regmap, AXP2202_COMM_STAT0, &value);

	if (value & BIT(5)) {
		di->stat.usb_connect = 1;
	} else {
		di->stat.usb_connect = 0;
	}

	return IRQ_HANDLED;
}

static irqreturn_t axp2202_irq_handler_thread(int irq, void *data)
{
	struct irq_desc *id = irq_to_desc(irq);
	struct axp2202_device_info *di = data;

	pr_debug("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(di->bat);

	switch (id->irq_data.hwirq) {
	case AXP2202_IRQ_CHGDN:
		pr_debug("interrupt:charger done");
		break;
	case AXP2202_IRQ_CHGST:
		pr_debug("interrutp:charger start");
		break;
	case AXP2202_IRQ_BINSERT:
		pr_debug("interrupt:battery insert");
		break;
	case AXP2202_IRQ_BREMOVE:
		pr_debug("interrupt:battery remove");
		break;
	default:
		pr_debug("interrupt:others");
		break;
	}

	return IRQ_HANDLED;
}

enum axp2202_virq_index {
	AXP2202_VIRQ_USB_IN,
	AXP2202_VIRQ_USB_OUT,
	AXP2202_VIRQ_BAT_IN,
	AXP2202_VIRQ_BAT_OUT,
	AXP2202_VIRQ_CHARGING,
	AXP2202_VIRQ_CHARGE_OVER,
	AXP2202_VIRQ_LOW_WARNING1,
	AXP2202_VIRQ_LOW_WARNING2,
	AXP2202_VIRQ_BAT_UNTEMP_WORK,
	AXP2202_VIRQ_BAT_OVTEMP_WORK,
	AXP2202_VIRQ_BAT_UNTEMP_CHG,
	AXP2202_VIRQ_BAT_OVTEMP_CHG,

	AXP2202_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_charger_irq[] = {
	[AXP2202_VIRQ_USB_IN] = { "vbus_insert", axp2202_irq_handler_usb_in },
	[AXP2202_VIRQ_USB_OUT] = { "vbus_remove", axp2202_irq_handler_usb_out },
	[AXP2202_VIRQ_BAT_IN] = { "battery_insert",
				  axp2202_irq_handler_thread },
	[AXP2202_VIRQ_BAT_OUT] = { "battery_remove",
				   axp2202_irq_handler_thread },
	[AXP2202_VIRQ_CHARGING] = { "battery_charge_start",
				    axp2202_irq_handler_thread },
	[AXP2202_VIRQ_CHARGE_OVER] = { "battery_charge_done",
				       axp2202_irq_handler_thread },
	[AXP2202_VIRQ_LOW_WARNING1] = { "soc_drop_w1",
					axp2202_irq_handler_thread },
	[AXP2202_VIRQ_LOW_WARNING2] = { "soc_drop_w2",
					axp2202_irq_handler_thread },
	[AXP2202_VIRQ_BAT_UNTEMP_WORK] = { "battery_under_temp_work",
					   axp2202_irq_handler_thread },
	[AXP2202_VIRQ_BAT_OVTEMP_WORK] = { "battery_over_temp_work",
					   axp2202_irq_handler_thread },
	[AXP2202_VIRQ_BAT_UNTEMP_CHG] = { "battery_under_temp_chg",
					  axp2202_irq_handler_thread },
	[AXP2202_VIRQ_BAT_OVTEMP_CHG] = { "battery_over_temp_chg",
					  axp2202_irq_handler_thread },
};

static void axp2202_charger_sysconfig(struct axp2202_device_info *di)
{
	struct regmap *regmap = di->regmap;
	struct axp2202_dts_config *dinfo = &di->dts_info;
	uint8_t value;

	if (dinfo->pmu_chg_ic_temp)
		regmap_update_bits(regmap, AXP2202_TS_CFG, 0x0a, 0x0a);
	else
		regmap_update_bits(regmap, AXP2202_TS_CFG, 0x0a, 0x00);

	/* set charger voltage limit */
	if (dinfo->pmu_init_chgvol < 4100) {
		regmap_update_bits(regmap, AXP2202_VTERM_CFG, 0x07, 0x00);
	} else if (dinfo->pmu_init_chgvol < 4200) {
		regmap_update_bits(regmap, AXP2202_VTERM_CFG, 0x07, 0x01);
	} else if (dinfo->pmu_init_chgvol < 4350) {
		regmap_update_bits(regmap, AXP2202_VTERM_CFG, 0x07, 0x02);
	} else if (dinfo->pmu_init_chgvol < 4400) {
		regmap_update_bits(regmap, AXP2202_VTERM_CFG, 0x07, 0x03);
	} else if (dinfo->pmu_init_chgvol < 5000) {
		regmap_update_bits(regmap, AXP2202_VTERM_CFG, 0x07, 0x04);
	} else {
		regmap_update_bits(regmap, AXP2202_VTERM_CFG, 0x07, 0x07);
	}
	/* set button battery charge termination voltage to 2.9v */
	regmap_update_bits(regmap, AXP2202_BTN_CHG_CFG, 0x07, 0x03);
	/* set chglend func 0x69 */
	regmap_update_bits(regmap, AXP2202_CHGLED_CFG, 0x06,
			   dinfo->pmu_chgled_type << 1);
	/* set gauge_thld */
	value = clamp_val(dinfo->pmu_battery_warning_level1 - 5, 0, 15) << 4;
	value |= clamp_val(dinfo->pmu_battery_warning_level2, 0, 15);
	regmap_write(regmap, AXP2202_GAUGE_THLD, value);
}

#define AXP_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &axp_config->name))\
		axp_config->name = def_value;\
} while (0)

int axp2202_charger_dt_parse(struct device_node *node,
			 struct axp2202_dts_config *axp_config)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_battery_cap,                4000);
	AXP_OF_PROP_READ(pmu_chg_ic_temp,                   0);
	AXP_OF_PROP_READ(pmu_runtime_chgcur,              500);
	AXP_OF_PROP_READ(pmu_suspend_chgcur,             1200);
	AXP_OF_PROP_READ(pmu_shutdown_chgcur,            1200);
	AXP_OF_PROP_READ(pmu_init_chgvol,                 500);
	AXP_OF_PROP_READ(pmu_usbpc_cur,                     0);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);

	axp_config->wakeup_usb_in =
		of_property_read_bool(node, "wakeup_usb_in");
	axp_config->wakeup_usb_out =
		of_property_read_bool(node, "wakeup_usb_out");
	axp_config->wakeup_bat_in =
		of_property_read_bool(node, "wakeup_bat_in");
	axp_config->wakeup_bat_out =
		of_property_read_bool(node, "wakeup_bat_out");
	axp_config->wakeup_bat_charging =
		of_property_read_bool(node, "wakeup_bat_charging");
	axp_config->wakeup_bat_charge_over =
		of_property_read_bool(node, "wakeup_bat_charge_over");
	axp_config->wakeup_low_warning1 =
		of_property_read_bool(node, "wakeup_low_warning1");
	axp_config->wakeup_low_warning2 =
		of_property_read_bool(node, "wakeup_low_warning2");
	axp_config->wakeup_bat_untemp_work =
		of_property_read_bool(node, "wakeup_bat_untemp_work");
	axp_config->wakeup_bat_ovtemp_work =
		of_property_read_bool(node, "wakeup_bat_ovtemp_work");
	axp_config->wakeup_untemp_chg =
		of_property_read_bool(node, "wakeup_bat_untemp_chg");
	axp_config->wakeup_ovtemp_chg =
		of_property_read_bool(node, "wakeup_bat_ovtemp_chg");

	return 0;
}

static void axp2202_parse_device_tree(struct axp2202_device_info *di)
{
	int ret;
	uint32_t prop = 0;
	struct axp2202_dts_config *cfg;

	/* set input current limit */
	if (!di->dev->of_node) {
		pr_info("can not find device tree\n");
		return;
	}

	cfg = &di->dts_info;
	ret = axp2202_charger_dt_parse(di->dev->of_node, cfg);
	if (ret) {
		pr_info("can not parse device tree err\n");
		return;
	}

	/* old sysconfig parse */
	axp2202_charger_sysconfig(di);

	/* prechg default change to 100mA */
	if (!of_property_read_u32(di->dev->of_node, "pmu_pre_chg", &prop)) {
		if (prop < 128)
			prop = 0;
		else
			prop = prop / 64 - 1;

		regmap_update_bits(di->regmap, AXP2202_IPRECHG_CFG, 0x0f, prop);
	} else {
		regmap_update_bits(di->regmap, AXP2202_IPRECHG_CFG, 0x0f, 2);
	}

	/* Termination default current limit to 50mA */
	if (!of_property_read_u32(di->dev->of_node, "pmu_iterm_limit", &prop)) {
		if (prop) {
			regmap_update_bits(di->regmap, AXP2202_ITERM_CFG, 0x0f,
					   prop / 64 ? prop / 64 - 1 :
						       prop / 64);
			regmap_update_bits(di->regmap, AXP2202_ITERM_CFG,
					   BIT(4), BIT(4));
		} else {
			regmap_write(di->regmap, AXP2202_ITERM_CFG, 0x00);
		}
	} else {
		regmap_update_bits(di->regmap, AXP2202_ITERM_CFG, 0x0f, 0x02);
	}

	if (!of_property_read_u32(di->dev->of_node, "pmu_chled_enable",
				  &prop) &&
	    !prop) {
		regmap_update_bits(di->regmap, AXP2202_CHGLED_CFG, 0x01, 0x00);
	}

	prop = di->dts_info.pmu_usbpc_cur;
	if (!of_property_read_u32(di->dev->of_node, "iin_limit", &prop) ||
	    true) {

		prop = clamp_val(prop, 100, 3250);
		prop = prop < 150 ? 0 : (prop - 150) / 50 + 1;

		regmap_update_bits(di->regmap, AXP2202_IIN_LIM, GENMASK(5, 0),
				   prop);
	}

	prop = di->dts_info.pmu_runtime_chgcur;
	if (!of_property_read_u32(di->dev->of_node, "icc_cfg", &prop) || true) {
		prop = clamp_val(prop, 0, 3072);
		/* step is 64mA */
		prop = prop / 64;

		regmap_update_bits(di->regmap, AXP2202_ICC_CFG, GENMASK(5, 0),
				   prop);
	}

	if (!of_property_read_u32(di->dev->of_node, "pmu_bat_det", &prop)) {
		regmap_write(di->regmap, AXP2202_BAT_DET, (unsigned int)prop);
	}

	if (of_property_read_bool(di->dev->of_node, "pmu_btn_chg_en")) {
		regmap_update_bits(di->regmap, AXP2202_MODULE_EN, BIT(3),
				   BIT(2));
	} else {
		regmap_update_bits(di->regmap, AXP2202_MODULE_EN, BIT(3), 0);
	}

	if (!of_property_read_u32(di->dev->of_node, "pmu_btn_chg_cfg", &prop)) {
		prop = (prop - 2600) / 100;
		regmap_write(di->regmap, AXP2202_BTN_CHG_CFG,
			     (unsigned int)(prop & 0x07));
	}
}

static void battery_set_full(int *rs)
{
	static ktime_t l_time;

	if (ktime_ms_delta(ktime_get(), l_time) > MSEC_PER_SEC) {
		WRITE_ONCE(*rs, true);
	}

	l_time = ktime_get();
}

static void battery_chk_online(struct work_struct *work)
{
	int ret;
	static int cnt;
	static int rst[3] = { 1, 1, 1 };
	unsigned int data;
	uint8_t d[2];
	ktime_t s_chg;
	struct axp2202_device_info *di =
		container_of(work, typeof(*di), bat_chk.work);

	/*
	 * check_full of batt, because bug of axp2202b, full flag can generate
	 * in batt plugged out
	 */
	ret = regmap_read(di->regmap, AXP2202_COMM_STAT1, &data);
	if (ret < 0) {
		pr_info("read AXP2202_REG_VBAT error\n");
		goto err_read;
	}
	/* chg_stat is full with bit[2:0] is b100 */
	if ((data & 0x07) == 0x04) {
		battery_set_full(&di->stat.bat_full);
	} else {
		WRITE_ONCE(di->stat.bat_full, false);
	}

	ret = regmap_bulk_read(di->regmap, AXP2202_VBAT_H, d, 2);
	if (ret < 0) {
		pr_info("read AXP2202_REG_VBAT error\n");
		goto err_read;
	}

	rst[cnt] = (((d[0] & GENMASK(5, 0)) << 0x08) | (d[1]));
	if (rst[cnt] < AXP2202_VBAT_MIN)
		rst[cnt] = 0;
	cnt = ++cnt == 3 ? 0 : cnt;

	/* there's one zero to indicate battery disconnect */
	if (!rst[0] || !rst[1] || !rst[2]) {
		if (di->stat.bat_stat != 0) {
			pr_debug("bat_stat change to none\n");
			di->stat.bat_stat = 0;
			WRITE_ONCE(di->stat.bat_read, false);
			axp2202_reset_gauge(di->regmap);
			s_chg = ktime_get();
			regmap_update_bits(di->regmap,
				      AXP2202_MODULE_EN,
				      BIT(2), 0);
			power_supply_changed(di->bat);
		}
	} else {
		if (di->stat.bat_stat != 1) {
			pr_debug("bat_stat change to exist\n");
			di->stat.bat_stat = 1;
			axp2202_reset_gauge(di->regmap);
			s_chg = ktime_get();
			regmap_update_bits(di->regmap,
				      AXP2202_MODULE_EN,
				      BIT(2), BIT(2));
			power_supply_changed(di->bat);
		}
	}

	if (di->stat.bat_stat &&
	    ktime_ms_delta(ktime_get(), s_chg) > MSEC_PER_SEC) {
		WRITE_ONCE(di->stat.bat_read, true);
	}

err_read:
	schedule_delayed_work(&di->bat_chk, msecs_to_jiffies(500));
}


static int axp2202_charger_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;
	struct axp2202_device_info *di;

	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (!axp_dev->irq) {
		pr_err("can not register axp2202-charger without irq\n");
		return -EINVAL;
	}

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (di == NULL) {
		pr_err("axp2202_device_info alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	di->name = "axp2202_chip";
	di->dev = &pdev->dev;
	di->regmap = axp_dev->regmap;

	/* for device tree parse */
	axp2202_parse_device_tree(di);

	ret = axp2202_init_chip(di);
	if (ret < 0) {
		dev_err(di->dev, "axp2202 init chip fail!\n");
		ret = -ENODEV;
		goto err;
	}

	ret = axp2202_register_battery(di);
	if (ret < 0) {
		dev_err(di->dev, "axp210x register battery dev fail!\n");
		goto err;
	}

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			dev_err(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_charger_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_charger_irq[i].isr, 0,
						   axp_charger_irq[i].name, di);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_charger_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_charger_irq[i].name, irq, ret);
	}
	platform_set_drvdata(pdev, di);
	INIT_DELAYED_WORK(&di->bat_chk, battery_chk_online);
	schedule_delayed_work(&di->bat_chk, msecs_to_jiffies(500));

	return ret;


err:
	pr_err("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp2202_charger_remove(struct platform_device *pdev)
{
	struct axp2202_device_info *di = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "==============AXP2202 unegister==============\n");
	axp2202_teardown_battery(di);
	dev_dbg(&pdev->dev, "axp210x teardown battery dev\n");

	return 0;
}

static void axp2202_icchg_set(struct axp2202_device_info *di, int mA)
{

	mA = clamp_val(mA, 0, 3072);
	mA = mA / 64;
	/* bit 5:0 is the ctrl bit */
	regmap_update_bits(di->regmap, AXP2202_ICC_CFG, GENMASK(5, 0), mA);
}

static inline void axp2202_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2202_virq_dts_set(struct axp2202_device_info *di, bool enable)
{
	struct axp2202_dts_config *dts_info = &di->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_USB_IN].irq,
				enable);
	if (!dts_info->wakeup_usb_out)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_USB_OUT].irq,
				enable);
	if (!dts_info->wakeup_bat_in)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_BAT_IN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_BAT_OUT].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_CHARGING].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_CHARGE_OVER].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_LOW_WARNING1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp2202_irq_set(axp_charger_irq[AXP2202_VIRQ_LOW_WARNING2].irq,
				enable);
	if (!dts_info->wakeup_bat_untemp_work)
		axp2202_irq_set(
			axp_charger_irq[AXP2202_VIRQ_BAT_UNTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_bat_ovtemp_work)
		axp2202_irq_set(
			axp_charger_irq[AXP2202_VIRQ_BAT_OVTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_untemp_chg)
		axp2202_irq_set(
			axp_charger_irq[AXP2202_VIRQ_BAT_UNTEMP_CHG].irq,
			enable);
	if (!dts_info->wakeup_ovtemp_chg)
		axp2202_irq_set(
			axp_charger_irq[AXP2202_VIRQ_BAT_OVTEMP_CHG].irq,
			enable);
}

static void axp2202_shutdown(struct platform_device *p)
{
	struct axp2202_device_info *di = platform_get_drvdata(p);

	axp2202_icchg_set(di, di->dts_info.pmu_shutdown_chgcur);

}

static int axp2202_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp2202_device_info *di = platform_get_drvdata(p);

	axp2202_icchg_set(di, di->dts_info.pmu_suspend_chgcur);

	axp2202_virq_dts_set(di, false);
	return 0;
}

static int axp2202_resume(struct platform_device *p)
{
	struct axp2202_device_info *di = platform_get_drvdata(p);

	axp2202_icchg_set(di, di->dts_info.pmu_runtime_chgcur);

	axp2202_virq_dts_set(di, true);

	return 0;
}

static const struct platform_device_id axp2202_charger_dt_ids[] = {
	{ .name = "axp2202-power-supply", },
	{},
};
MODULE_DEVICE_TABLE(of, axp2202_charger_dt_ids);

static struct platform_driver axp2202_charger_driver = {
	.driver = {
		.name = "axp2202-power-supply",
	},
	.probe = axp2202_charger_probe,
	.remove = axp2202_charger_remove,
	.id_table = axp2202_charger_dt_ids,
	.shutdown = axp2202_shutdown,
	.suspend = axp2202_suspend,
	.resume = axp2202_resume,
};

module_platform_driver(axp2202_charger_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp2202 i2c driver");
MODULE_LICENSE("GPL");

/*
 * Silergy SY6974 charger driver
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#ifdef CONFIG_ARCH_SUNXI
#include <linux/sunxi-gpio.h>
#endif

#define SY6974_REG_0			0x00
#define SY6974_REG_1			0x01
#define SY6974_REG_2			0x02
#define SY6974_REG_3			0x03
#define SY6974_REG_4			0x04
#define SY6974_REG_5			0x05
#define SY6974_REG_6			0x06
#define SY6974_REG_7			0x07
#define SY6974_REG_8			0x08
#define SY6974_REG_9			0x09
#define SY6974_REG_10			0x0A
#define SY6974_REG_11			0x0B

#define SY6974_MANUFACTURER		"Silergy"

#define SY6974_ILIM_SET_DELAY		1000	/* msec */

enum sy6974_fields {
	F_EN_HIZ, F_STAT_DIS, F_IINLIM,				/* REG 0 */
	F_PFM_DIS, F_WD_RST, F_OTG_CONFIG, F_CHG_CONFIG,	/* REG 1 */
	F_SYS_MIN, F_OTG_BAT,					/* REG 1 */
	F_BOOST_LIM, F_Q1_FULLON, F_ICHG,			/* REG 2 */
	F_IPRECHG, F_ITERM,					/* REG 3 */
	F_VREG, F_TOPOFF_TIMER, F_VRECHG,			/* REG 4 */
	F_EN_TERM, F_WATCHDOG, F_EN_TIMER, F_CHG_TIMER,		/* REG 5 */
	F_TREG, F_JEITA_ISET,					/* REG 5 */
	F_OVP, F_BOOSTV, F_VINDPM,				/* REG 6 */
	F_FORCE_INDET, F_TMR2X_EN, F_BATFET_DIS, F_JEITA_VSET,	/* REG 7 */
	F_BATFET_DLY, F_BATFET_RST_EN, F_VDPM_BAT_TRACK,	/* REG 7 */
	F_BUS_STAT, F_CHRG_STAT, F_PG_STAT, F_THERM_STAT,	/* REG 8 */
	F_VSYS_STAT,						/* REG 8 */
	F_FAULT,						/* REG 9 */
	F_BUS_GD, F_VDPM_STAT, F_IDPM_STAT, F_TOPOFF_ACTIVE,	/* REG 10 */
	F_ACOV_STAT, F_VINDPM_INT_MASK, F_IINDPM_INT_MASK,	/* REG 10 */
	F_REG_RST, F_PN, F_DEV_REV,				/* REG 11 */
	F_MAX_FIELDS
};

/* initial field values, converted from uV/uA */
struct sy6974_init_data {
	u8 ichg;	/* charge current      */
	u8 vbat;	/* regulation voltage  */
	u8 iterm;	/* termination current */
	u8 iilimit;	/* input current limit */
	u8 vovp;	/* over voltage protection voltage */
	u8 vindpm;	/* VDMP input threshold voltage */
	u8 sdchg;	/* shutdown charge current */
};

struct sy6974_state {
	u8 status;
	u8 fault;
	bool power_good;
};

struct sy6974_device {
	struct i2c_client *client;
	struct device *dev;
	struct power_supply *charger;

	struct regmap *rmap;
	struct regmap_field *rmap_fields[F_MAX_FIELDS];

	/*struct gpio_desc *pg;*/

	/*struct delayed_work iilimit_setup_work;*/

	struct sy6974_init_data init_data;
	struct sy6974_state state;

	struct mutex lock; /* protect state data */

	/*bool iilimit_autoset_enable;*/
};

static bool sy6974_is_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case SY6974_REG_8:
	case SY6974_REG_9:
		return false;

	default:
		return true;
	}
}

static const struct regmap_config sy6974_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = SY6974_REG_11,
	.cache_type = REGCACHE_NONE,

	.volatile_reg = sy6974_is_volatile_reg,
};

static const struct reg_field sy6974_reg_fields[] = {
	/* REG 0 */
	[F_EN_HIZ]		= REG_FIELD(SY6974_REG_0, 7, 7),
	[F_STAT_DIS]		= REG_FIELD(SY6974_REG_0, 5, 6),
	[F_IINLIM]		= REG_FIELD(SY6974_REG_0, 0, 4),
	/* REG 1 */
	[F_PFM_DIS]		= REG_FIELD(SY6974_REG_1, 7, 7),
	[F_WD_RST]		= REG_FIELD(SY6974_REG_1, 6, 6),
	[F_OTG_CONFIG]		= REG_FIELD(SY6974_REG_1, 5, 5),
	[F_CHG_CONFIG]		= REG_FIELD(SY6974_REG_1, 4, 4),
	[F_SYS_MIN]		= REG_FIELD(SY6974_REG_1, 1, 3),
	[F_OTG_BAT]		= REG_FIELD(SY6974_REG_1, 0, 0),
	/* REG 2 */
	[F_BOOST_LIM]		= REG_FIELD(SY6974_REG_2, 7, 7),
	[F_Q1_FULLON]		= REG_FIELD(SY6974_REG_2, 6, 6),
	[F_ICHG]		= REG_FIELD(SY6974_REG_2, 0, 5),
	/* REG 3 */
	[F_IPRECHG]		= REG_FIELD(SY6974_REG_3, 4, 7),
	[F_ITERM]		= REG_FIELD(SY6974_REG_3, 0, 3),
	/* REG 4 */
	[F_VREG]		= REG_FIELD(SY6974_REG_4, 3, 7),
	[F_TOPOFF_TIMER]	= REG_FIELD(SY6974_REG_4, 1, 2),
	[F_VRECHG]		= REG_FIELD(SY6974_REG_4, 0, 0),
	/* REG 5 */
	[F_EN_TERM]		= REG_FIELD(SY6974_REG_5, 7, 7),
	[F_WATCHDOG]		= REG_FIELD(SY6974_REG_5, 4, 5),
	[F_EN_TIMER]		= REG_FIELD(SY6974_REG_5, 3, 3),
	[F_CHG_TIMER]		= REG_FIELD(SY6974_REG_5, 2, 2),
	[F_TREG]		= REG_FIELD(SY6974_REG_5, 1, 1),
	[F_JEITA_ISET]		= REG_FIELD(SY6974_REG_5, 0, 0),
	/* REG 6 */
	[F_OVP]			= REG_FIELD(SY6974_REG_6, 6, 7),
	[F_BOOSTV]		= REG_FIELD(SY6974_REG_6, 4, 5),
	[F_VINDPM]		= REG_FIELD(SY6974_REG_6, 0, 3),
	/* REG 7 */
	[F_FORCE_INDET]		= REG_FIELD(SY6974_REG_7, 7, 7),
	[F_TMR2X_EN]		= REG_FIELD(SY6974_REG_7, 6, 6),
	[F_BATFET_DIS]		= REG_FIELD(SY6974_REG_7, 5, 5),
	[F_JEITA_VSET]		= REG_FIELD(SY6974_REG_7, 4, 4),
	[F_BATFET_DLY]		= REG_FIELD(SY6974_REG_7, 3, 3),
	[F_BATFET_RST_EN]	= REG_FIELD(SY6974_REG_7, 2, 2),
	[F_VDPM_BAT_TRACK]	= REG_FIELD(SY6974_REG_7, 0, 1),
	/* REG 8 */
	[F_BUS_STAT]		= REG_FIELD(SY6974_REG_8, 5, 7),
	[F_CHRG_STAT]		= REG_FIELD(SY6974_REG_8, 3, 4),
	[F_PG_STAT]		= REG_FIELD(SY6974_REG_8, 2, 2),
	[F_THERM_STAT]		= REG_FIELD(SY6974_REG_8, 1, 1),
	[F_VSYS_STAT]		= REG_FIELD(SY6974_REG_8, 0, 0),
	/* REG 9 */
	[F_FAULT]		= REG_FIELD(SY6974_REG_9, 0, 7),
	/* REG 10 */
	[F_BUS_GD]		= REG_FIELD(SY6974_REG_10, 7, 7),
	[F_VDPM_STAT]		= REG_FIELD(SY6974_REG_10, 6, 6),
	[F_IDPM_STAT]		= REG_FIELD(SY6974_REG_10, 5, 5),
	[F_TOPOFF_ACTIVE]	= REG_FIELD(SY6974_REG_10, 3, 3),
	[F_ACOV_STAT]		= REG_FIELD(SY6974_REG_10, 2, 2),
	[F_VINDPM_INT_MASK]	= REG_FIELD(SY6974_REG_10, 1, 1),
	[F_IINDPM_INT_MASK]	= REG_FIELD(SY6974_REG_10, 0, 0),
	/* REG 11 */
	[F_REG_RST]		= REG_FIELD(SY6974_REG_10, 7, 7),
	[F_PN]			= REG_FIELD(SY6974_REG_10, 3, 6),
	[F_DEV_REV]		= REG_FIELD(SY6974_REG_10, 0, 1)
};

enum sy6974_table_ids {
	TBL_IINLIM,
	TBL_ICHG,
	TBL_IPRECHG,
	TBL_ITERM,
	TBL_VREG,
	TBL_TOPOFF_TIMER,
	TBL_VINDPM,

	TBL_SYS_MIN,
	TBL_OVP,
};

struct sy6974_range {
	u32 min;
	u32 max;
	u32 step;
};

struct sy6974_lookup {
	const u32 *tbl;
	u32 size;
};

static const u32 sy6974_sys_min_tbl[] = { 2600, 2800, 3000, 3200, 3400,
					3500, 3600, 3700};
static const u32 sy6974_ovp_tbl[] = { 5500, 6500, 10500, 14000};

static const union {
	struct sy6974_range  rt;
	struct sy6974_lookup lt;
} sy6974_tables[] = {
	/* range tables */
	[TBL_IINLIM] =	{ .rt = {100,	  3200, 100} },	 /* mA */
	[TBL_ICHG] =	{ .rt = {0,	  3000, 60} },	 /* mA */
	[TBL_IPRECHG] =	{ .rt = {60,	  780, 60} },	 /* mA */
	[TBL_ITERM] =	{ .rt = {60,	  960, 60} },	 /* mA */
	[TBL_VREG] =	{ .rt = {3856,	  4624, 32} },	 /* mV */
	[TBL_TOPOFF_TIMER] =	{ .rt = {0,	  45, 15} },	 /* min */
	[TBL_VINDPM] =	{ .rt = {3900,	  5400, 100} },	 /* mV */
	/* lookup tables */
	[TBL_SYS_MIN] =	{ .lt = {
				sy6974_sys_min_tbl,
				ARRAY_SIZE(sy6974_sys_min_tbl)}
			},
	[TBL_OVP] =	{ .lt = {sy6974_ovp_tbl, ARRAY_SIZE(sy6974_ovp_tbl)} },
};

static u8 sy6974_find_idx(u32 value, enum sy6974_table_ids id)
{
	u8 idx;

	if (id >= TBL_SYS_MIN) {
		const u32 *tbl = sy6974_tables[id].lt.tbl;
		u32 tbl_size = sy6974_tables[id].lt.size;

		for (idx = 1; idx < tbl_size && tbl[idx] <= value; idx++)
			;
	} else {
		const struct sy6974_range *rtbl = &sy6974_tables[id].rt;
		u8 rtbl_size;

		rtbl_size = (rtbl->max - rtbl->min) / rtbl->step + 1;

		for (idx = 1;
		     idx < rtbl_size && (idx * rtbl->step + rtbl->min <= value);
		     idx++)
			;
	}

	return idx - 1;
}


static u32 sy6974_find_val_max(enum sy6974_table_ids id)
{
	const struct sy6974_range *rtbl;

	/* lookup table? */
	if (id >= TBL_SYS_MIN)
		return sy6974_tables[id].lt.tbl[sy6974_tables[id].lt.size-1];

	/* range table */
	rtbl = &sy6974_tables[id].rt;

	return rtbl->max;
}

static u32 sy6974_find_val(u8 idx, enum sy6974_table_ids id)
{
	const struct sy6974_range *rtbl;

	/* lookup table? */
	if (id >= TBL_SYS_MIN)
		return sy6974_tables[id].lt.tbl[idx];

	/* range table */
	rtbl = &sy6974_tables[id].rt;

	return (rtbl->min + idx * rtbl->step);
}

static int sy6974_field_read(struct sy6974_device *sy,
			      enum sy6974_fields field_id)
{
	int ret;
	int val;

	ret = regmap_field_read(sy->rmap_fields[field_id], &val);
	if (ret < 0)
		return ret;

	return val;
}

static int sy6974_field_write(struct sy6974_device *sy,
			       enum sy6974_fields field_id, u8 val)
{
	return regmap_field_write(sy->rmap_fields[field_id], val);
}

enum sy6974_status {
	STATUS_NOT_CHARGING,
	STATUS_PRE_CHARGE,
	STATUS_FAST_CHARGE,
	STATUS_CHARGE_TERM,
};

static bool sy6974_state_changed(struct sy6974_device *sy,
				  struct sy6974_state *new_state)
{
	int ret;

	mutex_lock(&sy->lock);
	ret = (sy->state.status != new_state->status ||
	       sy->state.fault != new_state->fault ||
	       sy->state.power_good != new_state->power_good);
	mutex_unlock(&sy->lock);

	return ret;
}

static int sy6974_get_chip_state(struct sy6974_device *sy,
				  struct sy6974_state *state)
{
	int ret;

	ret = sy6974_field_read(sy, F_CHRG_STAT);
	if (ret < 0)
		return ret;

	state->status = ret;

	ret = sy6974_field_read(sy, F_FAULT);
	if (ret < 0)
		return ret;

	state->fault = ret;

	ret = sy6974_field_read(sy, F_PG_STAT);
	if (ret < 0)
		return ret;

	state->power_good = ret;

	return 0;
}


static irqreturn_t sy6974_irq_handler_thread(int irq, void *private)
{
	int ret;
	struct sy6974_device *sy = private;
	struct sy6974_state state;

	ret = sy6974_get_chip_state(sy, &state);
	if (ret < 0)
		return IRQ_HANDLED;

	if (!sy6974_state_changed(sy, &state))
		return IRQ_HANDLED;

	dev_info(sy->dev, "state change: status/fault/pg=%d/%d/%d->%d/%d/%d\n",
		sy->state.status, sy->state.fault, sy->state.power_good,
		state.status, state.fault, state.power_good);
#if 0
	if (state.power_good && !sy->state.power_good) {
		/*
		 * PSEL Pull Up, current 500mA
		 * PSEL Pull Down, currnet 2.4A
		 * set iilimit here.
		 */
		sy6974_field_write(sy, F_IINLIM, sy->init_data.iilimit);
	}
#endif


	mutex_lock(&sy->lock);
	sy->state = state;
	mutex_unlock(&sy->lock);

	power_supply_changed(sy->charger);

	return IRQ_HANDLED;
}

static int sy6974_set_input_current_limit(struct sy6974_device *sy,
					const union power_supply_propval *val)
{
	unsigned int value = val->intval;

	pr_debug("current=%d, iilimit=%d\n", value, sy->init_data.iilimit);
	if (!value)
		value = sy->init_data.iilimit;
	else
		value = sy6974_find_idx(value, TBL_IINLIM);

	return sy6974_field_write(sy, F_IINLIM, value);
}

static int sy6974_get_input_current_limit(struct sy6974_device *sy,
					   union power_supply_propval *val)
{
	int ret;

	ret = sy6974_field_read(sy, F_IINLIM);
	if (ret < 0)
		return ret;

	val->intval = sy6974_find_val(ret, TBL_IINLIM);

	return 0;
}

static int sy6974_hw_init(struct sy6974_device *sy)
{
	int ret;
	int i;
	struct sy6974_state state;

	const struct {
		int field;
		u32 value;
	} init_data[] = {
		{F_ICHG, sy->init_data.ichg},
		{F_VREG, sy->init_data.vbat},
		{F_ITERM, sy->init_data.iterm},
		{F_OVP, sy->init_data.vovp},
		{F_VINDPM, sy->init_data.vindpm},
	};

	/*
	 * Disable the watchdog timer to prevent the IC from going back to
	 * default settings after 50 seconds of I2C inactivity.
	 */
	ret = sy6974_field_write(sy, F_WATCHDOG, 0x00);
	if (ret < 0)
		return ret;

	/* configure the charge currents and voltages */
	for (i = 0; i < ARRAY_SIZE(init_data); i++) {
		ret = sy6974_field_write(sy, init_data[i].field,
					  init_data[i].value);
		if (ret < 0)
			return ret;
	}

	ret = sy6974_get_chip_state(sy, &state);
	if (ret < 0)
		return ret;

	mutex_lock(&sy->lock);
	sy->state = state;
	mutex_unlock(&sy->lock);

	/* program fixed input current limit */
	ret = sy6974_field_write(sy, F_IINLIM, sy->init_data.iilimit);
	if (ret < 0)
		return ret;

	return ret;
}

static ssize_t sy6974_show_ovp_voltage(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sy6974_device *sy = power_supply_get_drvdata(psy);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 sy6974_find_val(sy->init_data.vovp, TBL_OVP));
}
static DEVICE_ATTR(ovp_voltage, 0444, sy6974_show_ovp_voltage, NULL);
static struct attribute *sy6974_charger_attr[] = {
	&dev_attr_ovp_voltage.attr,
	NULL,
};
static const struct attribute_group sy6974_attr_group = {
	.attrs = sy6974_charger_attr,
};

#define WATCHDOG_FAULT_MASK	(0x80)
#define BOOST_FAULT_MASK	(0x40)
#define CHRG_FAULT_MASK		(0x30)
enum chrg_fault_mask {
	CHRG_FAULT_INPUT = 0x01,
	CHRG_FAULT_THERMAL_SHUTDOWN = 0x02,
	CHRG_FAULT_CHARGE_SAFETY = 0x03,
};
#define BAT_FAULT_MASK		(0x08)
#define NTC_FAULT_MASK		(0x07)
enum ntc_fault {
	NTC_FAULT_WARM = 0x02,
	NTC_FAULT_COOL = 0x03,
	NTC_FAULT_COLD = 0x05,
	NTC_FAULT_HOT = 0x06,
};

static int sy6974_power_supply_get_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     union power_supply_propval *val)
{
	struct sy6974_device *sy = power_supply_get_drvdata(psy);
	struct sy6974_state state;

	mutex_lock(&sy->lock);
	sy6974_get_chip_state(sy, &state);
	sy->state = state;
	mutex_unlock(&sy->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (!state.power_good)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (state.status == STATUS_NOT_CHARGING)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.status == STATUS_PRE_CHARGE ||
			state.status == STATUS_FAST_CHARGE)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (state.status == STATUS_CHARGE_TERM)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;

	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = SY6974_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = "sy6974";
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.power_good;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		/* NTC_FAULT */
		if (state.fault & NTC_FAULT_MASK) {
			switch (state.fault & NTC_FAULT_MASK) {
			case NTC_FAULT_COOL:
			case NTC_FAULT_COLD:
				val->intval = POWER_SUPPLY_HEALTH_COLD;
				break;
			case NTC_FAULT_WARM:
			case NTC_FAULT_HOT:
				val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
				break;
			default:
				val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
				break;
			}
		} else if (state.fault & BAT_FAULT_MASK) {
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} else if (state.fault & CHRG_FAULT_MASK) {
			switch (state.fault & CHRG_FAULT_MASK) {
			case CHRG_FAULT_INPUT:
				val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
				break;
			case CHRG_FAULT_THERMAL_SHUTDOWN:
				val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
				break;
			case CHRG_FAULT_CHARGE_SAFETY:
				val->intval =
				POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
				break;
			}
		} else if (state.fault & BOOST_FAULT_MASK) {
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} else if (state.fault & WATCHDOG_FAULT_MASK) {
			val->intval = POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE;
		} else {
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		}
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = sy6974_find_val(sy->init_data.ichg, TBL_ICHG);
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = sy6974_find_val_max(TBL_ICHG);
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		val->intval = sy6974_find_val(sy->init_data.vbat, TBL_VREG);
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->intval = sy6974_find_val_max(TBL_VREG);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		val->intval = sy6974_find_val(sy->init_data.iterm, TBL_ITERM);
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return sy6974_get_input_current_limit(sy, val);

	default:
		return -EINVAL;
	}

	return 0;
}

static int sy6974_power_supply_set_property(struct power_supply *psy,
					enum power_supply_property prop,
					const union power_supply_propval *val)
{
	struct sy6974_device *sy = power_supply_get_drvdata(psy);

	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return sy6974_set_input_current_limit(sy, val);
	default:
		return -EINVAL;
	}

	return 0;
}

static int sy6974_power_supply_property_is_writeable(struct power_supply *psy,
					enum power_supply_property psp)
{
	return false;
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return true;
	default:
		return false;
	}
}

static enum power_supply_property sy6974_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
};

static const struct power_supply_desc sy6974_power_supply_desc = {
	.name = "sy6974-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = sy6974_power_supply_props,
	.num_properties = ARRAY_SIZE(sy6974_power_supply_props),
	.get_property = sy6974_power_supply_get_property,
	.set_property = sy6974_power_supply_set_property,
	.property_is_writeable = sy6974_power_supply_property_is_writeable,
};

static char *sy6974_charger_supplied_to[] = {
	"main-battery",
};

struct power_supply_config sy6974_psy_cfg = {
	.supplied_to = sy6974_charger_supplied_to,
	.num_supplicants = ARRAY_SIZE(sy6974_charger_supplied_to),
};

static int sy6974_power_supply_init(struct sy6974_device *sy)
{
	sy6974_psy_cfg.drv_data = sy,
	sy->charger = devm_power_supply_register(sy->dev,
						 &sy6974_power_supply_desc,
						 &sy6974_psy_cfg);

	return PTR_ERR_OR_ZERO(sy->charger);
}

static int sy6974_fw_probe(struct sy6974_device *sy)
{
	int ret;
	u32 property;
	struct device_node *np = sy->dev->of_node;
	int gpio;
#ifdef CONFIG_ARCH_SUNXI
	struct gpio_config flags;
#else
	enum of_gpio_flags flags;
#endif

	gpio = of_get_named_gpio_flags(np, "sy,irq-gpio", 0,
					(enum of_gpio_flags *)&flags);
	if (gpio > 0) {
#ifdef CONFIG_ARCH_SUNXI
		char pin_name[32];
		__u32 config;

		/* wakeup_gpio set pull */
		memset(pin_name, 0, sizeof(pin_name));
		sunxi_gpio_to_name(gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 1);
		if (gpio < SUNXI_PL_BASE)
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		else
			pin_config_set(SUNXI_R_PINCTRL, pin_name, config);
#endif
		sy->client->irq = gpio_to_irq(gpio);
	}

	/* Required properties */
	ret = device_property_read_u32(sy->dev, "sy,charge-current", &property);
	if (ret < 0)
		return ret;

	sy->init_data.ichg = sy6974_find_idx(property, TBL_ICHG);
	pr_debug("charge-current=%d, ichg=%d\n", property, sy->init_data.ichg);

	ret = device_property_read_u32(sy->dev, "sy,shutdown-current", &property);
	if (ret < 0)
		return ret;

	sy->init_data.sdchg = sy6974_find_idx(property, TBL_ICHG);
	pr_debug("shutdown-current=%d, sdchg=%d\n", property, sy->init_data.sdchg);

	ret = device_property_read_u32(sy->dev, "sy,battery-regulation-voltage",
				       &property);
	if (ret < 0)
		return ret;

	sy->init_data.vbat = sy6974_find_idx(property, TBL_VREG);
	pr_debug("Vtarget=%d, vbat=%d\n", property, sy->init_data.vbat);

	ret = device_property_read_u32(sy->dev, "sy,termination-current",
				       &property);
	if (ret < 0)
		return ret;

	sy->init_data.iterm = sy6974_find_idx(property, TBL_ITERM);
	pr_debug("Iterm=%d, iterm=%d\n", property, sy->init_data.iterm);

	/* Optional properties. If not provided use reasonable default. */
	ret = device_property_read_u32(sy->dev, "sy,current-limit",
				       &property);
	if (ret < 0) {
		/*sy->iilimit_autoset_enable = true;*/

		/*
		 * Explicitly set a default value which will be needed for
		 * devices that don't support the automatic setting of the input
		 * current limit through the charger type detection mechanism.
		 */
		sy->init_data.iilimit = sy6974_find_idx(500, TBL_IINLIM);
	} else
		sy->init_data.iilimit = sy6974_find_idx(property, TBL_IINLIM);
	pr_debug("Ilimit=%d, iilimit=%d\n", property, sy->init_data.iilimit);

	ret = device_property_read_u32(sy->dev, "sy,ovp-voltage",
				       &property);
	if (ret < 0)
		sy->init_data.vovp = sy6974_find_idx(6500, TBL_OVP);
	else
		sy->init_data.vovp = sy6974_find_idx(property, TBL_OVP);
	pr_debug("Vovp=%d, vovp=%d\n", property, sy->init_data.vovp);

	ret = device_property_read_u32(sy->dev, "sy,in-dpm-voltage",
				       &property);
	if (ret < 0)
		sy->init_data.vindpm = sy6974_find_idx(4500, TBL_VINDPM);
	else
		sy->init_data.vindpm =
				sy6974_find_idx(property, TBL_VINDPM);
	pr_debug("Vindpm=%d, vindpm=%d\n", property, sy->init_data.vindpm);

	return 0;
}

static int sy6974_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct device *dev = &client->dev;
	struct sy6974_device *sy;
	int ret = 0;
	int i;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "No support for SMBUS_BYTE_DATA\n");
		return -ENODEV;
	}

	sy = devm_kzalloc(dev, sizeof(*sy), GFP_KERNEL);
	if (!sy)
		return -ENOMEM;

	sy->client = client;
	sy->dev = dev;

	mutex_init(&sy->lock);

	sy->rmap = devm_regmap_init_i2c(client, &sy6974_regmap_config);
	if (IS_ERR(sy->rmap)) {
		dev_err(dev, "failed to allocate register map\n");
		return PTR_ERR(sy->rmap);
	}

	for (i = 0; i < ARRAY_SIZE(sy6974_reg_fields); i++) {
		/*const struct reg_field *reg_fields = sy6974_reg_fields;*/
		sy->rmap_fields[i] = devm_regmap_field_alloc(dev, sy->rmap,
						     sy6974_reg_fields[i]);
		if (IS_ERR(sy->rmap_fields[i])) {
			dev_err(dev, "cannot allocate regmap field\n");
			return PTR_ERR(sy->rmap_fields[i]);
		}
	}

	i2c_set_clientdata(client, sy);

	ret = sy6974_fw_probe(sy);
	if (ret < 0) {
		dev_err(dev, "Cannot read device properties.\n");
		return ret;
	}
	ret = sy6974_hw_init(sy);
	if (ret < 0) {
		dev_err(dev, "Cannot initialize the chip.\n");
		return ret;
	}

	ret = sy6974_power_supply_init(sy);
	if (ret < 0) {
		dev_err(dev, "Failed to register power supply\n");
		return ret;
	}

	if (client->irq > 0) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						sy6974_irq_handler_thread,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT | IRQF_SHARED,
						"sy7964", sy);
		if (ret) {
			dev_err(dev, " request IRQ#%d failed\n", client->irq);
			return ret;
		}
	}

	ret = sysfs_create_group(&sy->charger->dev.kobj, &sy6974_attr_group);
	if (ret < 0) {
		dev_err(dev, "Can't create sysfs entries\n");
		return ret;
	}

	return 0;
}

static int sy6974_remove(struct i2c_client *client)
{
	struct sy6974_device *sy = i2c_get_clientdata(client);

	sysfs_remove_group(&sy->charger->dev.kobj, &sy6974_attr_group);

	return 0;
}

static void sy6974_shutdown(struct i2c_client *client)
{
#if 0
	struct sy6974_device *sy = i2c_get_clientdata(client);
	struct sy6974_state state;

	/* shutdown charge current */
	sy6974_field_write(sy, F_ICHG, sy->init_data.sdchg);
	sy6974_get_chip_state(sy, &state);
	if (state.status == STATUS_NOT_CHARGING) {
		/* not charging, enter ship mode */
		sy6974_field_write(sy, F_BATFET_DIS, 1);
		sy6974_field_write(sy, F_BATFET_DLY, 0);
	}
#endif

	return;
}

#ifdef CONFIG_PM_SLEEP
static int sy6974_suspend(struct device *dev)
{
#if 0
	struct sy6974_device *sy = dev_get_drvdata(dev);
	int ret = 0;

	return ret;
#else
	return 0;
#endif
}

static int sy6974_resume(struct device *dev)
{
#if 0
	struct sy6974_device *sy = dev_get_drvdata(dev);
	int ret;
	/* signal userspace, maybe state changed while suspended */
	power_supply_changed(sy->charger);
	return ret;
#else
	return 0;
#endif
}
#endif

static const struct dev_pm_ops sy6974_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(sy6974_suspend, sy6974_resume)
};

static const struct i2c_device_id sy6974_i2c_ids[] = {
	{ "sy6974", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sy6974_i2c_ids);

static const struct of_device_id sy6974_of_match[] = {
	{ .compatible = "silergy,sy6974", },
	{ },
};
MODULE_DEVICE_TABLE(of, sy6974_of_match);

static struct i2c_driver sy6974_driver = {
	.driver = {
		.name = "sy6974-charger",
		.of_match_table = of_match_ptr(sy6974_of_match),
		.pm = &sy6974_pm,
	},
	.probe = sy6974_probe,
	.remove = sy6974_remove,
	.id_table = sy6974_i2c_ids,
	.shutdown = sy6974_shutdown,
};
module_i2c_driver(sy6974_driver);

MODULE_AUTHOR("ForeverCai <caiyongheng@allwinnertech.com>");
MODULE_DESCRIPTION("sy6974 charger driver");
MODULE_LICENSE("GPL");

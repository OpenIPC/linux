/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/phy.h>
#include <linux/module.h>
#include <linux/delay.h>

#define AX620E_EPHY_ID		0x00441400
#define AXERA_PHY_ID_MASK	0x0ffffff0
//#define EPHY_SAVEING_POWER  1

static void ax_ephy_ieee_en(struct phy_device *phydev)
{
	unsigned int value=0;

	//enable iEEE
	phy_write(phydev, 0x1f, 0x0100);	/* switch to page 1 */
	value = phy_read(phydev, 0x17);
	value |= (1<<3);			/* reg 0x17 bit 3, set 1 to enable iEEE */
	phy_write(phydev, 0x17, value);
	phy_write(phydev, 0x1f, 0x0000);	/* switch to page 0 */
}

static void ax_ephy_ieee_disable(struct phy_device *phydev)
{
	unsigned int value=0;

	//disable iEEE
	phy_write(phydev, 0x1f, 0x0100);	/* switch to page 1 */
	value = phy_read(phydev, 0x17);
	value &= ~(1<<3);			/* disable IEEE set 0 to disable iEEE */
	phy_write(phydev, 0x17, value);
	phy_write(phydev, 0x1f, 0x0000);	/* switch to page 0 */
}

static void ax_ephy_802_3az_eee_en(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x3c);
	phy_write(phydev, 0xd, BIT(14) | 0x7);
	value = phy_read(phydev, 0xe);
	value |= BIT(1);
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x3c);
	phy_write(phydev, 0xd, BIT(14) | 0x7);
	phy_write(phydev, 0xe, value);

	phy_write(phydev, 0x1f, 0x0200);	/* switch to page 2 */
	phy_write(phydev, 0x18, 0x1000);
}

static void ax_ephy_802_3az_eee_disable(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x3c);
	phy_write(phydev, 0xd, BIT(14) | 0x7);
	value = phy_read(phydev, 0xe);
	value &= ~BIT(1);
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x3c);
	phy_write(phydev, 0xd, BIT(14) | 0x7);
	phy_write(phydev, 0xe, value);

	phy_write(phydev, 0x1f, 0x0200);	/* switch to page 2 */
	phy_write(phydev, 0x18, 0x0000);
}

static void ax_ephy_uaps_en(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, 0x1f, 0x0100);	/* Switch to Page 1 */
	value = phy_read(phydev, 0x13);
	value |= BIT(15);                 /*reg 0x13 bit 15, set 1 to eanble UAPS*/
	phy_write(phydev, 0x13, value);
	phy_write(phydev, 0x1f, 0x0000);	/* switch to page 0 */
}

static void ax_ephy_uaps_disable(struct phy_device *phydev)
{
	unsigned int value;

	/* Disable Auto Power Saving mode */
	phy_write(phydev, 0x1f, 0x0100);	/* Switch to Page 1 */
	value = phy_read(phydev, 0x13);
	value &= ~BIT(15);                 /*reg 0x13 bit 15, set 0 to disable UAPS*/
	phy_write(phydev, 0x13, value);
	phy_write(phydev, 0x1f, 0x0000);	/* switch to page 0 */
}

static void ax_ephy_power_down(struct phy_device *phydev)
{
	phy_write(phydev, 0x1f, 0x0100);	/* Switch to Page 1 */
	phy_write(phydev, 0x12, 0x4824);	/* Disable APS */

	phy_write(phydev, 0x1f, 0x0600);	/* Switch to Page 6 */
	phy_write(phydev, 0x19, 0x000c);
	phy_write(phydev, 0x1c, 0x8880);	/* PHYAFE PDCW optimization */

	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */
	phy_write(phydev, 0x00, 0x3900);	/* Power down analog block */
}

static void ax_ephy_set_loopback(struct phy_device *phydev, unsigned int loopback)
{
	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */

	switch (loopback) {
	case 0:
		phy_write(phydev, 0x12, 0x0080);	/*enable PHY loopback*/
		break;
	case 1:
		phy_write(phydev, 0x12, 0x8000);	/*enable Interface loopback*/
		break;
	case 2:
		phy_write(phydev, 0x12, 0x2000);	/*enable PCS loopback*/
		break;
	case 3:
		phy_write(phydev, 0x12, 0x1000);	/*enable PMA loopback*/
		break;
	default:
		break;
	}
}

static void ax_ephy_epg_start(struct phy_device *phydev)
{
	unsigned int read_value = 0;
	pr_info("ax_ephy_epg_start\n");

	//begin to configure epg
	phy_write(phydev, 0x1f, 0x0900);	/* Switch to Page 9 */

	read_value = phy_read(phydev, 0x10);
	read_value |= (1<<15); 		/*reg 0x10 bit 15, epg enable*/
	read_value &= ~(1<<14);
	read_value &= ~(1<<13);
	read_value &= ~(1<<12);
	read_value |= (1<<11);
	read_value |= (1<<10);
	read_value &= ~(1<<9);
	// read_value &= ~(1<<8);
	read_value |= (1<<8);
	read_value |= (1<<6);
	read_value |= (1<<5);
	phy_write(phydev, 0x10, read_value);

	read_value = phy_read(phydev, 0x11);
	read_value |= (1<<11);
	read_value |= (1<<10);
	phy_write(phydev, 0x11, read_value);

	read_value =  phy_read(phydev, 0x12);
	read_value |= (1<<14);
	read_value = 0xffff;
	phy_write(phydev, 0x12, read_value);

	phy_write(phydev, 0x13, 0x0001);
	phy_write(phydev, 0x14, 0xaaaa);

	read_value =  phy_read(phydev, 0x10);
	read_value |= (1<<0);
	phy_write(phydev, 0x10, read_value);
}

static void ax_ephy_epg_stop(struct phy_device *phydev)
{
	unsigned int value = 0;

	pr_info("ax_ephy_epg_stop\n");
	phy_write(phydev, 0x1f, 0x0900);	/* Switch to Page 9 */

	value =  phy_read(phydev, 0x10);
	value &= ~0x3; //2b00 stop
	phy_write(phydev, 0x10, value);
}

static void ax_ephy_cnt_sts(struct phy_device *phydev)
{
	unsigned int tx_pkt_cnt, tx_byte_cnt;
	unsigned int rx_pkt_cnt, rx_byte_cnt, rx_crc_err_cnt;
	unsigned int read_value;

	//begin to pkt statistic
	phy_write(phydev, 0x1f, 0x0900);	/* Switch to Page 9 */

	//read tx_byte_cnt
	read_value = phy_read(phydev, 0x15);
	tx_byte_cnt = read_value << 16;
	read_value = phy_read(phydev, 0x16);
	tx_byte_cnt |= read_value;

	//read tx_pkt_cnt
	read_value = phy_read(phydev, 0x17);
	tx_pkt_cnt = read_value << 16;
	read_value = phy_read(phydev, 0x18);
	tx_pkt_cnt |= read_value;

	//read rx_byte_cnt
	read_value = phy_read(phydev, 0x19);
	rx_byte_cnt = read_value << 16;
	read_value = phy_read(phydev, 0x1a);
	rx_byte_cnt |= read_value;

	//read rx_pkt_cnt
	read_value = phy_read(phydev, 0x1b);
	rx_pkt_cnt = read_value << 16;
	read_value = phy_read(phydev, 0x1c);
	rx_pkt_cnt |= read_value;

	//read rx_crc_err_cnt
	read_value = phy_read(phydev, 0x1d);
	rx_crc_err_cnt = read_value << 16;
	read_value = phy_read(phydev, 0x1e);
	rx_crc_err_cnt |= read_value;

	printk(KERN_ERR"------Axera EPHY Statistic Start----------\n");
	printk(KERN_ERR"TX transmitted packet number is  %0u\n", tx_pkt_cnt);
	printk(KERN_ERR"TX transmitted byte number is  %0u\n", tx_byte_cnt);
	printk(KERN_ERR"RX recevied packet number is  %0u\n", rx_pkt_cnt);
	printk(KERN_ERR"RX recevied byte number is  %0u\n", rx_byte_cnt);
	printk(KERN_ERR"RX recevied CRC packet number is  %0u\n", rx_crc_err_cnt);
	printk(KERN_ERR"------Axera EPHY Statistic End----------\n");
}

static void ax_ephy_clear_sts(struct phy_device *phydev)
{
	unsigned int read_value = 0;

	phy_write(phydev, 0x1f, 0x0900);	/* Switch to Page 9 */

	read_value = phy_read(phydev, 0x10);
	read_value |= (1<<2) | (1<<3); // clear cnts
	phy_write(phydev, 0x10, read_value);
}

static int ax_ephy_reset(struct phy_device *phydev)
{
	int bmcr;
	int timeout = 500;

	// pr_info("ax_ephy_reset\n");

	/* Software Reset PHY */
	bmcr = phy_write(phydev, MII_BMCR, BMCR_RESET);
	if (bmcr < 0) {
		phydev_err(phydev, "PHY soft reset failed\n");
		return -1;
	}

	udelay(12);
	/*
	 * Poll the control register for the reset bit to go to 0 (it is
	 * auto-clearing).  This should happen within 0.5 seconds per the
	 * IEEE spec.
	 */
	bmcr = phy_read(phydev, MII_BMCR);
	while ((bmcr & BMCR_RESET) && timeout--) {
		bmcr = phy_read(phydev, MII_BMCR);
		if (bmcr < 0) {
			phydev_err(phydev,"PHY status read failed\n");
			return -1;
		}
		udelay(1000);
	}

	if (bmcr & BMCR_RESET) {
		phydev_err(phydev, "PHY reset timed out\n");
		return -1;
	}

	return 0;
}

static int ax_ephy_afe_rx_set(struct phy_device *phydev)
{
	unsigned int value;

	// pr_info("ax_ephy_afe_rx_set\n");

	phy_write(phydev, 0x1f, 0x0600);	/* Switch to Page 6 */
	value = phy_read(phydev, 0x10);
	value &= ~0x7;
	value |= 0x6;
	value &= ~(0x7<<3);
	value |= (0x5<<3);
	phy_write(phydev, 0x10, value);	/* Adc gain optimization */

	value = phy_read(phydev, 0x14);
	value &= ~(0x3<<13);
	phy_write(phydev, 0x14, value);	/* Adc gain optimization */

	return 0;
}

static int ax_ephy_reg_dump(struct phy_device *phydev)
{
	unsigned int value;
	int i=0;
	int j=0;
	printk(KERN_ERR"------EPHY REGs DUMP Start----------\n");
	for (i=0; i<=9; i++) {
		printk(KERN_ERR"--------Page:%d -------\n", i);
		phy_write(phydev, 0x1f, 0x0100*i);
		for (j=0; j<=31; j++) {
			value = phy_read(phydev, j);
			printk(KERN_ERR"reg%02d = 0x%04x\n", j, value);
		}
		printk(KERN_ERR"---------------\n");
	}
	printk(KERN_ERR"------EPHY REGs DUMP End----------\n");
	return 0;
}

static int ax_ephy_config_init(struct phy_device *phydev)
{
	pr_info("%s\n", __func__);

	//ax_ephy_reset(phydev);

	phy_write(phydev, 0x1f, 0x0100);	/* Switch to Page 1 */
	phy_write(phydev, 0x12, 0x4824);	/* Disable APS */

	phy_write(phydev, 0x1f, 0x0200);	/* Switch to Page 2 */
	phy_write(phydev, 0x18, 0x0000);	/* 10 base-t filter selector */

	phy_write(phydev, 0x1f, 0x0300);	/* Switch to Page 3 */
	phy_write(phydev, 0x11, 0x8010);	/* AGC to minimum */

	phy_write(phydev, 0x1f, 0x0600);	/* Switch to Page 6 */
	// phy_write(phydev, 0x10, 0x5540);	/* Adc gain optimization */
	// phy_write(phydev, 0x12, 0x8400);	/* Adc gain optimization */
	// phy_write(phydev, 0x14, 0x1088);	/* Adc gain optimization */
	phy_write(phydev, 0x15, 0x3333);	/* a_TX_level optimiztion */
	phy_write(phydev, 0x19, 0x004c);	/* PHYAFE TX optimization and invorce ADC clock */
	phy_write(phydev, 0x1b, 0x888f);	/* TX_BIAS */
	phy_write(phydev, 0x1c, 0x8880);	/* PHYAFE PDCW optimization */

	phy_write(phydev, 0x1f, 0x0800);	/* Switch to Page 8 */
	phy_write(phydev, 0x1d, 0x0844);	/* PHYAFE Tx_auto */

	ax_ephy_802_3az_eee_disable(phydev);		/* Disable 802.3az IEEE */
	//ax_ephy_ieee_disable(phydev);	/* Disable Intelligent IEEE */

#ifdef EPHY_SAVEING_POWER
	ax_ephy_uaps_en(phydev);
#endif

	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */

	return 0;
}

/* epg */
static unsigned long epg_val;
static ssize_t ephy_show_epg(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%ld\n", epg_val);
}

static ssize_t ephy_store_epg(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	epg_val = val;

	if (val) {
		ax_ephy_epg_start(phydev);
	} else {
		ax_ephy_epg_stop(phydev);
	}

	return count;
}

//reg dump
static ssize_t ephy_show_regdump(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct phy_device *phydev = to_phy_device(dev);

	ax_ephy_reg_dump(phydev);

	return 0;
}

static ssize_t ephy_store_regdump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	return count;
}

//statistics
static ssize_t ephy_show_stats(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct phy_device *phydev = to_phy_device(dev);

	ax_ephy_cnt_sts(phydev);
	return 0;
}

static ssize_t ephy_store_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	if (val) {
		pr_info("clear ephy statistic counter\n");
		ax_ephy_clear_sts(phydev);
	}

	return count;
}

/* loopback */
static unsigned long loop_val;
static ssize_t ephy_show_loopback(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned int value;

	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */
	value = phy_read(phydev, 0x12);

	return sprintf(buf, "0x%04x\n", value);
}

static ssize_t ephy_store_loopback(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	if (val > 4) {
		pr_info("loopback mode is 0-3\n");
		return count;
	}

	loop_val = val;

	ax_ephy_set_loopback(phydev, val);

	return count;
}

/* aneg */
static unsigned long aneg_val;
static ssize_t ephy_show_aneg(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	// struct phy_device *phydev = to_phy_device(dev);
	pr_info("any number trigger auto negotiation\n");
	return sprintf(buf, "%ld\n", aneg_val);
}

static ssize_t ephy_store_aneg(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;
	int ret;

	pr_info("restart auto negotiation\n");
	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	aneg_val = val;

	genphy_restart_aneg(phydev);

	return count;
}

/* calibration */
static unsigned long cal_val;
static ssize_t ephy_show_cal(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	// struct phy_device *phydev = to_phy_device(dev);
	pr_info("0: set default, other number: restart calibration\n");
	return sprintf(buf, "%ld\n", cal_val);
}

static ssize_t ephy_store_cal(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	cal_val = val;
	if (cal_val) {
		pr_info("restart calibration\n");
		phy_write(phydev, 0x1f, 0x0800);	/* Switch to Page 8 */
		phy_write(phydev, 0x13, 0xc0a6);	/* bit[14]restart calibration */
	} else {
		pr_info("set calibration default\n");
		phy_write(phydev, 0x1f, 0x0800);	/* Switch to Page 8 */
		phy_write(phydev, 0x13, 0x80a6);	/* Set default value 0x80a6 */
	}

	return count;
}

/* ADC */
static unsigned long adc_val;
static ssize_t ephy_show_adc(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	// struct phy_device *phydev = to_phy_device(dev);
	pr_info("adc mode:0-reset default, 1-user reference, 2-rx enhance\n");
	return sprintf(buf, "%ld\n", adc_val);
}

static ssize_t ephy_store_adc(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	adc_val = val;
	switch (adc_val)
	{
	case 0:
		pr_info("adc mode: reset default\n");
		phy_write(phydev, 0x1f, 0x0600);	/* Switch to Page 6 */
		phy_write(phydev, 0x10, 0x5563);	/* Adc gain optimization */
		phy_write(phydev, 0x12, 0x0400);	/* Adc gain optimization */
		phy_write(phydev, 0x14, 0x7088);	/* Adc gain optimization */
		break;
	case 1:
		pr_info("adc mode: user reference\n");
		phy_write(phydev, 0x1f, 0x0600);	/* Switch to Page 6 */
		phy_write(phydev, 0x10, 0x5540);	/* Adc gain optimization */
		phy_write(phydev, 0x12, 0x8400);	/* Adc gain optimization */
		phy_write(phydev, 0x14, 0x1088);	/* Adc gain optimization */
		break;
	case 2:
		pr_info("adc mode: rx enhance\n");
		ax_ephy_afe_rx_set(phydev);
		break;
	default:
		pr_info("adc mode:0-1-2\n");
		break;
	}

	return count;
}

/* mdix mode */
static unsigned long mdix_val = 2;
static ssize_t ephy_show_mdix(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	// struct phy_device *phydev = to_phy_device(dev);
	pr_info("midx mode: 0-mdi, 1-mdix, 2-auto mdi/midx\n");
	return sprintf(buf, "%ld\n", mdix_val);
}

static ssize_t ephy_store_mdix(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;
	unsigned int value;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;
	mdix_val = val;

	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */
	value = phy_read(phydev, 0x13); // P0Reg19
	value &= ~(0x3);

	if (mdix_val == 0) {
		pr_info("force mdi mode\n");
		phy_write(phydev, 0x13, value);
	} else if (mdix_val == 1) {
		pr_info("force mdix mode\n");
		value |= 0x1;
		phy_write(phydev, 0x13, value);
	} else if (mdix_val == 2) {
		pr_info("auto mdi/mdix mode\n");
		value |= 0x2;
		phy_write(phydev, 0x13, value);
	} else {
		pr_info("only support 0/1/2 mode\n");
	}

	return count;
}

static DEVICE_ATTR(epg, S_IWUSR | S_IRUGO,
		   ephy_show_epg, ephy_store_epg);

static DEVICE_ATTR(regdump, S_IWUSR | S_IRUGO,
		   ephy_show_regdump, ephy_store_regdump);

static DEVICE_ATTR(stats, S_IWUSR | S_IRUGO,
		   ephy_show_stats, ephy_store_stats);

static DEVICE_ATTR(loopback, S_IWUSR | S_IRUGO,
		   ephy_show_loopback, ephy_store_loopback);

static DEVICE_ATTR(aneg, S_IWUSR | S_IRUGO,
		   ephy_show_aneg, ephy_store_aneg);

static DEVICE_ATTR(calibration, S_IWUSR | S_IRUGO,
		   ephy_show_cal, ephy_store_cal);

static DEVICE_ATTR(adc, S_IWUSR | S_IRUGO,
		   ephy_show_adc, ephy_store_adc);

static DEVICE_ATTR(mdix, S_IWUSR | S_IRUGO,
		   ephy_show_mdix, ephy_store_mdix);

static struct attribute *ax_ephy_attributes[] = {
	&dev_attr_epg.attr,
	&dev_attr_regdump.attr,
	&dev_attr_stats.attr,
	&dev_attr_loopback.attr,
	&dev_attr_aneg.attr,
	&dev_attr_calibration.attr,
	&dev_attr_adc.attr,
	&dev_attr_mdix.attr,
	NULL
};


static const struct attribute_group ax_ephy_attr_group = {
	.attrs = ax_ephy_attributes,
};

static int ax_ephy_probe(struct phy_device *phydev)
{
	int func_call=0;
	int err = 0;

	if (!phydev)
		return -ENODEV;

	// pr_info("axera ephy probe\n");

	//avoid func warning
	if (func_call) {
		ax_ephy_ieee_en(phydev);
		ax_ephy_ieee_disable(phydev);
		ax_ephy_802_3az_eee_en(phydev);
		ax_ephy_802_3az_eee_disable(phydev);
		ax_ephy_uaps_en(phydev);
		ax_ephy_uaps_disable(phydev);
		ax_ephy_power_down(phydev);
		ax_ephy_set_loopback(phydev, 0);
		ax_ephy_epg_start(phydev);
		ax_ephy_epg_stop(phydev);
		ax_ephy_cnt_sts(phydev);
		ax_ephy_reset(phydev);
		ax_ephy_reg_dump(phydev);
		ax_ephy_afe_rx_set(phydev);
	}

	/* register sysfs hooks */
	err = sysfs_create_group(&phydev->mdio.dev.kobj, &ax_ephy_attr_group);
	if (err)
		return err;

    return 0;
};

static void ax_ephy_remove(struct phy_device *phydev)
{
	// pr_info("axera ephy ax_ephy_remove\n");
	ax_ephy_power_down(phydev);
}

static int ax_ephy_resume(struct phy_device *phydev)
{
	if (phydev->suspended) {
		phy_init_hw(phydev);
	}
	return genphy_resume(phydev);
}

static int ax_ephy_suspend(struct phy_device *phydev)
{
	ax_ephy_power_down(phydev);
	return genphy_suspend(phydev);
}

static struct phy_driver ax_ephy_drvs[] = {
	{
	.phy_id		= AX620E_EPHY_ID,
	.phy_id_mask    = AXERA_PHY_ID_MASK,
	.name           = "AX620E Fast Ethernet",
	/* PHY_BASIC_FEATURES */
	.features	= PHY_BASIC_FEATURES,
	.probe		= ax_ephy_probe,
	.config_init    = ax_ephy_config_init,
	.soft_reset	= genphy_soft_reset,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
	.suspend        = ax_ephy_suspend,
	.resume         = ax_ephy_resume,
	.remove	        = ax_ephy_remove,
	},
};

module_phy_driver(ax_ephy_drvs);

static struct mdio_device_id __maybe_unused ax_ephy_tbl[] = {
	{AX620E_EPHY_ID, AXERA_PHY_ID_MASK},
	{ }
};

MODULE_DEVICE_TABLE(mdio, ax_ephy_tbl);
MODULE_DESCRIPTION("Axera EPHY driver");
MODULE_AUTHOR("Axera");
MODULE_LICENSE("GPL");

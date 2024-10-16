/*
 * SUNXI NSI driver
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 * Author: xiafeng <xiafeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/sunxi_nsi.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>

#define DRIVER_NAME          "NSI"
#define DRIVER_NAME_PMU      DRIVER_NAME"_PMU"

#define MBUS_QOS_MAX      0x3

#define for_each_ports(port) for (port = 0; port < MBUS_PMU_MAX; port++)

/* n = 0~19 */
#define IAG_MODE(n)					(0x0010 + (0x200 * n))
#define IAG_PRI_CFG(n)				(0x0014 + (0x200 * n))
#define IAG_INPUT_OUTPUT_CFG(n)		(0x0018 + (0x200 * n))
/* Counter n = 0 ~ 19 */
#define MBUS_PMU_ENABLE(n)         (0x00c0 + (0x200 * n))
#define MBUS_PMU_CLR(n)            (0x00c4 + (0x200 * n))
#define MBUS_PMU_CYCLE(n)          (0x00c8 + (0x200 * n))
#define MBUS_PMU_RQ_RD(n)          (0x00cc + (0x200 * n))
#define MBUS_PMU_RQ_WR(n)          (0x00d0 + (0x200 * n))
#define MBUS_PMU_DT_RD(n)          (0x00d4 + (0x200 * n))
#define MBUS_PMU_DT_WR(n)          (0x00d8 + (0x200 * n))
#define MBUS_PMU_LA_RD(n)          (0x00dc + (0x200 * n))
#define MBUS_PMU_LA_WR(n)          (0x00e0 + (0x200 * n))

#define MBUS_PORT_MODE          (MBUS_PMU_MAX + 0)
#define MBUS_PORT_QOS           (MBUS_PMU_MAX + 1)
#define MBUS_INPUT_OUTPUT       (MBUS_PMU_MAX + 2)
#define MBUS_PORT_TIMER         (MBUS_PMU_MAX + 3)

static unsigned int total_data;
static unsigned int total_r_data;
static unsigned int total_w_data;

struct sunxi_nsi_port {
	void __iomem *base;
	unsigned long phys;
	struct device_node *dn;
	int irq;
};

static struct sunxi_nsi_port *ports;
static void __iomem *nsi_ctrl_base;
static unsigned long nsi_ctrl_phys;
static unsigned long rate;

static DEFINE_MUTEX(nsi_seting);
static DEFINE_MUTEX(nsi_pmureading);

static struct clk *pclk;	/* PLL clock */
static struct clk *mclk;	/* spi module clock */
static unsigned int mod_clk;

/**
 * nsi_port_setthd() - set a master priority
 *
 * @pri, priority
 */
int notrace nsi_port_setmode(enum nsi_pmu port, unsigned int mode)
{
	unsigned int value = 0;

	if (port >= MBUS_PMU_MAX)
		return -ENODEV;

	mutex_lock(&nsi_seting);
	value = readl_relaxed(nsi_ctrl_base + IAG_MODE(port));
	value &= ~0x3;
	writel_relaxed(value | mode, nsi_ctrl_base + IAG_MODE(port));
	mutex_unlock(&nsi_seting);

	return 0;
}

EXPORT_SYMBOL_GPL(nsi_port_setmode);

/**
 * nsi_port_setpri() - set a master QOS
 *
 * @qos: the qos value want to set
 */
int notrace nsi_port_setpri(enum nsi_pmu port, unsigned int pri)
{
	unsigned int value;

	if (port >= MBUS_PMU_MAX)
		return -ENODEV;

	if (pri > MBUS_QOS_MAX)
		return -EPERM;

	mutex_lock(&nsi_seting);
	value = readl_relaxed(nsi_ctrl_base + IAG_PRI_CFG(port));
	value &= ~0xf;
	writel_relaxed(value | (pri << 2) | pri,
		       nsi_ctrl_base + IAG_PRI_CFG(port));
	mutex_unlock(&nsi_seting);

	return 0;
}

EXPORT_SYMBOL_GPL(nsi_port_setpri);

/**
 * nsi_port_setio() - set a master's qos in or out
 *
 * @wt: the wait time want to set, based on MCLK
 */
int notrace nsi_port_setio(enum nsi_pmu port, bool io)
{
	unsigned int value;

	if (port >= MBUS_PMU_MAX)
		return -ENODEV;

	mutex_lock(&nsi_seting);
	value = readl_relaxed(nsi_ctrl_base + IAG_INPUT_OUTPUT_CFG(port));
	value &= ~0x1;
	writel_relaxed(value | io, nsi_ctrl_base + IAG_INPUT_OUTPUT_CFG(port));
	mutex_unlock(&nsi_seting);

	return 0;
}

EXPORT_SYMBOL_GPL(nsi_port_setio);

static void nsi_pmu_disable(enum nsi_pmu port)
{
	unsigned int value =
	    readl_relaxed(nsi_ctrl_base + MBUS_PMU_ENABLE(port));
	value &= ~(0x1);
	writel_relaxed(value, nsi_ctrl_base + MBUS_PMU_ENABLE(port));
}

static void nsi_pmu_enable(enum nsi_pmu port)
{
	unsigned int value =
	    readl_relaxed(nsi_ctrl_base + MBUS_PMU_ENABLE(port));
	value |= (0x1);
	writel_relaxed(value, nsi_ctrl_base + MBUS_PMU_ENABLE(port));
}

static void nsi_pmu_clear(enum nsi_pmu port)
{
	unsigned int value = readl_relaxed(nsi_ctrl_base + MBUS_PMU_CLR(port));
	value |= (0x1);
	writel_relaxed(value, nsi_ctrl_base + MBUS_PMU_CLR(port));
}

/**
 * nsi_port_set_timer() - set a master check bitwidth in pre time
 * function
 *
 * @port: index of the port to setup
 * @us
 */
int notrace nsi_pmu_set_timer(enum nsi_pmu port, unsigned int us)
{
	unsigned int value, cycle;
	rate = clk_get_rate(mclk);
	/*
	 *if mclk = 400MHZ, the signel time = 1000*1000*1000/(400*1000*1000)=2.5ns
	 *if check the 10us, the cycle count = 10*1000ns/2.5us = 4000
	 * us*1000/(1s/400Mhz) = us*1000*400000000(rate)/1000000000
	 */
	cycle = us * rate / 1000000;

	if (port >= MBUS_PMU_MAX)
		return -ENODEV;

	mutex_lock(&nsi_seting);

	value = readl_relaxed(nsi_ctrl_base + MBUS_PMU_CYCLE(port));
	value = cycle;
	writel_relaxed(value, nsi_ctrl_base + MBUS_PMU_CYCLE(port));

	/* disabled the pmu count */
	nsi_pmu_disable(port);

	/* clean the insight counter */
	nsi_pmu_clear(port);

	/* enabled the pmu count */
	nsi_pmu_enable(port);

	udelay(us);
	mutex_unlock(&nsi_seting);

	return 0;
}

EXPORT_SYMBOL_GPL(nsi_pmu_set_timer);

static const struct of_device_id sunxi_nsi_matches[] = {
#ifdef CONFIG_ARCH_SUN8I
	{.compatible = "allwinner,sun8i-nsi",},
#endif
#ifdef CONFIG_ARCH_SUN50I
	{.compatible = "allwinner,sun50i-nsi",},
#endif
	{},
};

static int nsi_probe(void)
{
	int ret;
	unsigned int port;
	struct device_node *np;
	struct resource res;
	struct device_node *child;

	np = of_find_matching_node(NULL, sunxi_nsi_matches);
	if (!np)
		return -ENODEV;

	ports = kcalloc(1, sizeof(*ports), GFP_KERNEL);
	if (!ports)
		return -ENOMEM;

	ret = of_address_to_resource(np, 0, &res);
	if (!ret) {
		nsi_ctrl_base = ioremap(res.start, resource_size(&res));
		nsi_ctrl_phys = res.start;

	}
	if (ret || !nsi_ctrl_base) {
		WARN(1, "unable to ioremap nsi ctrl\n");
		ret = -ENXIO;
		goto memalloc_err;
	}

	pclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(pclk)) {
		pr_err("Unable to acquire module clock, return %x\n",
		       PTR_RET(pclk));
		return -1;
	}

	mclk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(mclk)) {
		pr_err("Unable to acquire module clock, return %x\n",
		       PTR_RET(mclk));
		return -1;
	}

	ret = of_property_read_u32(np, "clock-frequency", &mod_clk);
	if (ret) {
		pr_err("Get clock-frequency property failed\n");
		return -1;
	}
//      clk_disable_unprepare(mclk);

	ret = clk_set_parent(mclk, pclk);
	if (ret != 0) {
		pr_err("clk_set_parent() failed! return\n");
		return -1;
	}

	rate = clk_round_rate(mclk, mod_clk);
	if (clk_set_rate(mclk, rate)) {
		pr_err("clk_set_rate failed\n");
		return -1;
	}

	if (clk_prepare_enable(mclk)) {
		pr_err("Couldn't enable nsi clock\n");
		return -EBUSY;
	}

	/* the purpose freq of MBUS is 400M, has been configied by boot */

	for_each_available_child_of_node(np, child) {

		for_each_ports(port) {
			if (strcmp(child->name, get_name(port)))
				continue;
			else
				break;
		}

#if 0
		unsigned int val;
		if (!of_property_read_u32(child, "mode", &val)) {
			nsi_port_setmode(port, val);
		}

		if (!of_property_read_u32(child, "pri", &val)) {
			nsi_port_setpri(port, val);
		}

		if (!of_property_read_u32(child, "select", &val))
			nsi_port_setio(port, val);
#endif

	}

	/* all the port is default opened */

	/* set default bandwidth */

	/* set default QOS */

	/* set masters' request number sequency */

	/* set masters' bandwidth limit0/1/2 */

memalloc_err:
	kfree(ports);

	return 0;
}

static int nsi_init_status = -EAGAIN;
static DEFINE_MUTEX(nsi_proing);

static int nsi_init(void)
{
	if (nsi_init_status != -EAGAIN)
		return nsi_init_status;

	mutex_lock(&nsi_proing);
	if (nsi_init_status == -EAGAIN)
		nsi_init_status = nsi_probe();
	mutex_unlock(&nsi_proing);

	return nsi_init_status;
}

/**
 * To sort out early init calls ordering a helper function is provided to
 * check if the nsi driver has beed initialized. Function check if the driver
 * has been initialized, if not it calls the init function that probes
 * the driver and updates the return value.
 */
bool nsi_probed(void)
{
	return nsi_init() == 0;
}

EXPORT_SYMBOL_GPL(nsi_probed);

struct nsi_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	bool valid;
	unsigned long last_updated;
	int kind;
};

static struct nsi_data hw_nsi_pmu;

/*
static void nsi_pmu_set_cycle(enum nsi_pmu port, long us)
{
	long rate = clk_get_rate(mclk);
	unsigned int cycle = us * 1000 * rate / 1000000000;

	unsigned int value = readl_relaxed(nsi_ctrl_base + MBUS_PMU_CYCLE(port));
	value = cycle;
	writel_relaxed(value, nsi_ctrl_base + MBUS_PMU_CYCLE(port));
}
*/

static unsigned int nsi_read_bandwidth(struct nsi_data *data,
				       enum nsi_pmu port, char *buf)
{
	unsigned int size;
	unsigned int x, y, z, o, p, q;

	if (port == 20) {
#if 0
		size =
		    sprintf(buf,
			    "%6s: total_data: %-6uMB, r_data: %-6uMB, w_data: %-6uMB\n",
			    get_name(port), (total_data) / 1024 / 1024,
			    total_r_data / 1024 / 1024,
			    total_w_data / 1024 / 1024);
#else
		size = sprintf(buf, "%-6u\n", total_data / 1024);
#endif
		total_r_data = 0;
		total_w_data = 0;
		total_data = 0;
		return size;
	}

	mutex_lock(&data->update_lock);

	/* check the request read */
	x = readl_relaxed(nsi_ctrl_base + MBUS_PMU_RQ_RD(port));

	/* check the request write */
	y = readl_relaxed(nsi_ctrl_base + MBUS_PMU_RQ_WR(port));

	/* check the data read */
	z = readl_relaxed(nsi_ctrl_base + MBUS_PMU_DT_RD(port));

	/* check the data write */
	o = readl_relaxed(nsi_ctrl_base + MBUS_PMU_DT_WR(port));

	/* check the read latency */
	p = readl_relaxed(nsi_ctrl_base + MBUS_PMU_LA_RD(port));

	/* check the write latency */
	q = readl_relaxed(nsi_ctrl_base + MBUS_PMU_LA_WR(port));

	if (port != 20) {
		total_r_data += z;
		total_w_data += o;
		total_data = total_r_data + total_w_data;
	}

	/* read pmu conter */
#if 0
	size = sprintf(buf, "r_request:%-5u w_request:%-5u r_data:%-5u \
			w_data:%-5u r_delay:%-5u w_delay:%-5u\n", x, y, z, o, p, q);
#else
	size = sprintf(buf, "%-6u\n", (z + o) / 1024);
#endif

	mutex_unlock(&data->update_lock);

	return size;
}

static unsigned int nsi_get_value(struct nsi_data *data,
				  unsigned int index, char *buf)
{
	unsigned int i, size = 0;
	unsigned long value;

	mutex_lock(&data->update_lock);
	switch (index) {
	case MBUS_PORT_MODE:	//0x10    fixed limit regulator
		for_each_ports(i) {
			value = readl_relaxed(nsi_ctrl_base + IAG_MODE(i));
			size +=
			    sprintf(buf + size, "master:%-8s qos_mode:%lu\n",
				    get_name(i), (value & 0x3));
		}
		break;
	case MBUS_PORT_QOS:	//0x14
		for_each_ports(i) {
			value = readl_relaxed(nsi_ctrl_base + IAG_PRI_CFG(i));
			size +=
			    sprintf(buf + size, "master:%-8s read_priority:%lu \
					write_priority:%lu\n", get_name(i), (value >> 2), (value & 0x3));
		}
		break;
	case MBUS_INPUT_OUTPUT:	//0x18
		for_each_ports(i) {
			value = readl_relaxed(nsi_ctrl_base +
					      IAG_INPUT_OUTPUT_CFG(i));
			size +=
			    sprintf(buf + size,
				    "master:%-8s qos_sel(0-out 1-input):%lu\n",
				    get_name(i), (value & 1));
		}
		break;
	case MBUS_PORT_TIMER:
		for_each_ports(i) {
			value = readl_relaxed(nsi_ctrl_base +
					      MBUS_PMU_CYCLE(i));
			if (rate > 0)
				value = value * 1000000 / rate;
			size +=
			    sprintf(buf + size,
				    "master%-2d:%-8s timer_us:%lu\n", i,
				    get_name(i), value);
		}
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		value = 0;
		break;
	}
	mutex_unlock(&data->update_lock);

	return size;
}

static ssize_t nsi_show_value(struct device *dev,
			      struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	unsigned int len;

	if (attr->index >= MBUS_PMU_MAX) {
		len = nsi_get_value(&hw_nsi_pmu, attr->index, buf);
		len = (len < PAGE_SIZE) ? len : PAGE_SIZE;
		return len;
	} else {
		len = nsi_read_bandwidth(&hw_nsi_pmu, attr->index, buf);
		return len;
	}
/*
	return snprintf(buf, PAGE_SIZE, "%u\n",
			nsi_update_device(&hw_nsi_pmu, attr->index));
*/
}

static unsigned int nsi_set_value(struct nsi_data *data, unsigned int index,
				  enum nsi_pmu port, unsigned int val)
{
	unsigned int value;

	mutex_lock(&data->update_lock);
	switch (index) {
	case MBUS_PORT_MODE:
		nsi_port_setmode(port, val);
		break;
	case MBUS_PORT_QOS:
		nsi_port_setpri(port, val);
		break;
	case MBUS_INPUT_OUTPUT:
		nsi_port_setio(port, val);
		break;
	case MBUS_PORT_TIMER:
		nsi_pmu_set_timer(port, val);
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		value = 0;
		break;
	}
	mutex_unlock(&data->update_lock);

	return 0;
}

static ssize_t nsi_store_value(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int nr = to_sensor_dev_attr(attr)->index;
	unsigned long port, val;
	unsigned char buffer[64];
	unsigned char *pbuf, *pbufi;
	int err;

	if (strlen(buf) >= 64) {
		dev_err(dev, "arguments out of range!\n");
		return -EINVAL;
	}

	while (*buf == ' ')	/* find the first unblank character */
		buf++;
	strncpy(buffer, buf, strlen(buf));

	pbufi = buffer;
	while (*pbufi != ' ')	/* find the first argument */
		pbufi++;
	*pbufi = 0x0;
	pbuf = (unsigned char *)buffer;
	err = kstrtoul(pbuf, 10, &port);
	if (err < 0)
		return err;
	if (port >= MBUS_PMU_MAX) {
		dev_err(dev, "master is illegal\n");
		return -EINVAL;
	}

	pbuf = ++pbufi;
	while (*pbuf == ' ')	/* remove extra space character */
		pbuf++;
	pbufi = pbuf;
	while ((*pbufi != ' ') && (*pbufi != '\n'))
		pbufi++;
	*pbufi = 0x0;

	err = kstrtoul(pbuf, 10, &val);
	if (err < 0)
		return err;

	nsi_set_value(&hw_nsi_pmu, nr, (enum nsi_pmu)port, (unsigned int)val);

	return count;
}

/* CPU bandwidth of DDR channel 0 */
static SENSOR_DEVICE_ATTR(pmu_cpuddr, 0400, nsi_show_value, NULL, MBUS_PMU_CPU);
#if (defined CONFIG_ARCH_SUN50IW10)
/* GPU bandwidth of DDR channel 0 */
static SENSOR_DEVICE_ATTR(pmu_gpuddr, 0400, nsi_show_value, NULL, MBUS_PMU_GPU);
static SENSOR_DEVICE_ATTR(pmu_sd1_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_SD1);
static SENSOR_DEVICE_ATTR(pmu_mstg_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_MSTG);
static SENSOR_DEVICE_ATTR(pmu_gmac0_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_GMAC0);
static SENSOR_DEVICE_ATTR(pmu_gmac1_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_GMAC1);
static SENSOR_DEVICE_ATTR(pmu_usb0_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_USB0);
static SENSOR_DEVICE_ATTR(pmu_usb1_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_USB1);
static SENSOR_DEVICE_ATTR(pmu_ndfc_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_NDFC);
static SENSOR_DEVICE_ATTR(pmu_dmac_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_DMAC);
static SENSOR_DEVICE_ATTR(pmu_ce_ddr, 0400, nsi_show_value, NULL, MBUS_PMU_CE);
static SENSOR_DEVICE_ATTR(pmu_de0_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_DE0);
static SENSOR_DEVICE_ATTR(pmu_de1_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_DE1);
static SENSOR_DEVICE_ATTR(pmu_ve_ddr, 0400, nsi_show_value, NULL, MBUS_PMU_VE);
static SENSOR_DEVICE_ATTR(pmu_csi_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_CSI);
static SENSOR_DEVICE_ATTR(pmu_isp_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_ISP);
static SENSOR_DEVICE_ATTR(pmu_g2d_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_G2D);
static SENSOR_DEVICE_ATTR(pmu_eink_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_EINK);
static SENSOR_DEVICE_ATTR(pmu_iommu_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_IOMMU);
static SENSOR_DEVICE_ATTR(pmu_syscpu_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_SYS_CPU);
static SENSOR_DEVICE_ATTR(pmu_total_ddr, 0400,
			  nsi_show_value, NULL, MBUS_PMU_TOTAL);
#endif

/* get all masters' mode or set a master's mode */
static SENSOR_DEVICE_ATTR(port_mode, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_MODE);
/* get all masters' prio or set a master's prio */
static SENSOR_DEVICE_ATTR(port_prio, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_QOS);
/* get all masters' inout or set a master's inout */
static SENSOR_DEVICE_ATTR(port_select, 0644,
			  nsi_show_value, nsi_store_value, MBUS_INPUT_OUTPUT);
/* get all masters' sample timer or set a master's timer */
static SENSOR_DEVICE_ATTR(port_timer, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_TIMER);

/*****************************************/
/*
static SENSOR_DEVICE_ATTR(port_acs, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_ACS);
static SENSOR_DEVICE_ATTR(port_bwl0, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_BWL0);
static SENSOR_DEVICE_ATTR(port_bwl1, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_BWL1);
static SENSOR_DEVICE_ATTR(port_bwl2, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_BWL2);
static SENSOR_DEVICE_ATTR(port_bwlen, 0644,
			  nsi_show_value, nsi_store_value, MBUS_PORT_BWLEN);
static SENSOR_DEVICE_ATTR(port_abs_bwlen, 0644,
			nsi_show_value, nsi_store_value, MBUS_PORT_ABS_BWLEN);
static SENSOR_DEVICE_ATTR(port_abs_bwl, 0644,
			nsi_show_value, nsi_store_value, MBUS_PORT_ABS_BWL);
static SENSOR_DEVICE_ATTR(port_bw_satu, 0644,
			nsi_show_value, nsi_store_value, MBUS_PORT_BW_SATU);
*/
/**************************************/

/* pointers to created device attributes */
static struct attribute *nsi_attributes[] = {
	&sensor_dev_attr_pmu_cpuddr.dev_attr.attr,

#if (defined CONFIG_ARCH_SUN50IW10)
	&sensor_dev_attr_pmu_gpuddr.dev_attr.attr,
	&sensor_dev_attr_pmu_sd1_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_mstg_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_gmac0_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_gmac1_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_usb0_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_usb1_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_ndfc_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_dmac_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_ce_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_de0_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_de1_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_ve_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_csi_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_isp_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_g2d_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_eink_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_iommu_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_syscpu_ddr.dev_attr.attr,
	&sensor_dev_attr_pmu_total_ddr.dev_attr.attr,
#endif

	&sensor_dev_attr_port_mode.dev_attr.attr,
	&sensor_dev_attr_port_prio.dev_attr.attr,
	&sensor_dev_attr_port_select.dev_attr.attr,
	&sensor_dev_attr_port_timer.dev_attr.attr,
/*
	&sensor_dev_attr_port_acs.dev_attr.attr,
	&sensor_dev_attr_port_bwl0.dev_attr.attr,
	&sensor_dev_attr_port_bwl1.dev_attr.attr,
	&sensor_dev_attr_port_bwl2.dev_attr.attr,
	&sensor_dev_attr_port_bwlen.dev_attr.attr,
	&sensor_dev_attr_port_abs_bwlen.dev_attr.attr,
	&sensor_dev_attr_port_abs_bwl.dev_attr.attr,
	&sensor_dev_attr_port_bw_satu.dev_attr.attr,
*/
	NULL,
};

static struct attribute_group nsi_group = {
	.attrs = nsi_attributes,
};

static const struct attribute_group *nsi_groups[] = {
	&nsi_group,
	NULL,
};

static int nsi_pmu_probe(struct platform_device *pdev)
{
	int ret;

	hw_nsi_pmu.hwmon_dev =
	    devm_hwmon_device_register_with_groups(&pdev->dev, "nsi_pmu", NULL,
						   nsi_groups);

	if (IS_ERR(hw_nsi_pmu.hwmon_dev)) {
		ret = PTR_ERR(hw_nsi_pmu.hwmon_dev);
		goto out_err;
	}

	hw_nsi_pmu.last_updated = 0;
	hw_nsi_pmu.valid = 0;
	mutex_init(&hw_nsi_pmu.update_lock);

	return 0;

out_err:
	dev_err(&(pdev->dev), "probed failed\n");
	sysfs_remove_group(&pdev->dev.kobj, &nsi_group);

	return ret;
}

static int nsi_pmu_remove(struct platform_device *pdev)
{
	hwmon_device_unregister(hw_nsi_pmu.hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &nsi_group);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_nsi_suspend(struct device *dev)
{
	dev_info(dev, "suspend okay\n");

	return 0;
}

static int sunxi_nsi_resume(struct device *dev)
{
	dev_info(dev, "resume okay\n");

	return 0;
}

static const struct dev_pm_ops sunxi_nsi_pm_ops = {
	.suspend = sunxi_nsi_suspend,
	.resume = sunxi_nsi_resume,
};

#define SUNXI_MBUS_PM_OPS (&sunxi_nsi_pm_ops)
#else
#define SUNXI_MBUS_PM_OPS NULL
#endif

static struct platform_driver nsi_pmu_driver = {
	.driver = {
		   .name = DRIVER_NAME_PMU,
		   .owner = THIS_MODULE,
		   .pm = SUNXI_MBUS_PM_OPS,
		   .of_match_table = sunxi_nsi_matches,
		   },
	.probe = nsi_pmu_probe,
	.remove = nsi_pmu_remove,
};

static int __init nsi_pmu_init(void)
{
	int ret;

	ret = platform_driver_register(&nsi_pmu_driver);
	if (ret) {
		pr_err("register sunxi nsi platform driver failed\n");
		goto drv_err;
	}

	return ret;

drv_err:
	platform_driver_unregister(&nsi_pmu_driver);

	return -EINVAL;
}

early_initcall(nsi_init);
device_initcall(nsi_pmu_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SUNXI NSI support");
MODULE_AUTHOR("xiafeng");

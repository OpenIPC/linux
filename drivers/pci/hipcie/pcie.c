#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/mbus.h>
#include <asm/irq.h>
#include <linux/of.h>
#include <linux/of_pci.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <asm/siginfo.h>
#include "pci.h"

#define PCIE_DBG_REG		1
#define PCIE_DBG_FUNC		2
#define PCIE_DBG_MODULE		3

#define PCIE_DEBUG_LEVEL PCIE_DBG_MODULE

/*#define PCIE_DEBUG*/
#ifdef PCIE_DEBUG
#define pcie_debug(level, str, arg...)\
	do {\
		if ((level) <= PCIE_DEBUG_LEVEL) {\
			pr_debug("%s->%d," str "\n", \
				__func__, __LINE__, ##arg);\
		} \
	} while (0)
#else
#define pcie_debug(level, str, arg...)
#endif

#define pcie_assert(con)\
	do {\
		if (!(con)) {\
			pr_err("%s->%d,assert fail!\n", \
				__func__, __LINE__);\
		} \
	} while (0)

#define pcie_error(str, arg...)\
	pr_err("%s->%d" str "\n", __func__, __LINE__, ##arg)


#define __256MB__	0x10000000
#define __128MB__	0x8000000
#define __4KB__		0x1000
#define __8KB__		0x2000

enum pcie_sel {
	/*
	 * No controller selected.
	 */
	pcie_sel_none,
	/*
	 * PCIE0 selected.
	 */
	pcie0_x1_sel,
	/*
	 * PCIE1 selected.
	 */
	pcie1_x1_sel
};

enum pcie_rc_sel {
	pcie_controller_unselected,
	pcie_controller_selected
};

enum pcie_controller {
	pcie_controller_none = -1,
	pcie_controller_0 = 0,
	pcie_controller_1 = 1
};

struct pcie_iatu {
	unsigned int viewport;          /* iATU Viewport Register        */
	unsigned int region_ctrl_1;     /* Region Control 1 Register     */
	unsigned int region_ctrl_2;     /* Region Control 2 Register     */
	unsigned int lbar;              /* Lower Base Address Register   */
	unsigned int ubar;              /* Upper Base Address Register   */
	unsigned int lar;               /* Limit Address Register        */
	unsigned int ltar;		/* Lower Target Address Register */
	unsigned int utar;              /* Upper Target Address Register */
};

#define MAX_IATU_PER_CTRLLER	(6)

struct pcie_info {
	/*
	 * Root bus number
	 */
	u8		root_bus_nr;
	enum		pcie_controller controller;

	/*
	 * Devices configuration space base
	 */
	unsigned long	base_addr;

	/*
	 * RC configuration space base
	 */
	unsigned long	conf_base_addr;
};

static struct pcie_info pcie_info[2] = {
	{.root_bus_nr = -1,},
	{.root_bus_nr = -1,}
};

static int pcie_controllers_nr;

static unsigned int pcie0_sel = pcie_controller_unselected;
static unsigned int pcie0_mem_space_size = 0x0;

static unsigned int pcie_errorvalue;

struct device_node *g_of_node = NULL;

static DEFINE_SPINLOCK(cw_lock);

#define PCIE0_MODE_SEL  (1 << 0)
#define PCIE1_MODE_SEL  (1 << 1)


#if defined(CONFIG_ARCH_HI3559AV100)
#include "pcie_hi3559av100.c"
#elif defined(CONFIG_ARCH_HI3531A)
#include "pcie_hi3531a.c"
#else
#error You must have defined CONFIG_ARCH_HI35xx...
#endif

/*
 * PCIE memory size bootargs: pcie0_mem_size=0xa00000;pcie1_mem_size=0xa00000
 */
static int __init pcie0_mem_size_parser(char *str)
{
	unsigned int size;

	if (kstrtoul(str, 16, (long *)&size) < 0)
		return 0;

	/* if size >= 256MB, set default 256MB */
	if (size >= 0x10000000)
		size = 0x10000000;
	pcie0_mem_space_size = size;

	return 1;
}
__setup("pcie0_mem_size=", pcie0_mem_size_parser);

/*
 * PCIE sel bootargs: pcie0_sel=x1 pcie1=x1 or pcie1=x2
 * Any other value after "pcieX_sel=" prefix
 * will be treated as none controller selected.
 * e.g. "pcie0_sel=none" will be treated as no PCIE0 selected.
 */
static int __init pcie0_sel_parser(char *str)
{
	if (strncasecmp(str, "x1", 2) == 0)
		pcie0_sel = pcie0_x1_sel;
	else
		pcie0_sel = pcie_sel_none;

	return 1;
}
__setup("pcie0_sel=", pcie0_sel_parser);

static struct pcie_info *bus_to_info(int busnr)
{
	int i = pcie_controllers_nr;
	for (; i >= 0; i--) {
		if (pcie_info[i].controller != pcie_controller_none
				&& pcie_info[i].root_bus_nr <= busnr
				&& pcie_info[i].root_bus_nr != -1)
			return &pcie_info[i];
	}

	return NULL;
}


#define PCIE_CFG_BUS(busnr)	((busnr & 0xff) << 20)
#define PCIE_CFG_DEV(devfn)	((devfn & 0xff) << 12)
#define PCIE_CFG_REG(reg)	(reg & 0xffc)	/*set dword align*/

static inline unsigned long to_pcie_address(struct pci_bus *bus,
		unsigned int devfn, int where)
{
	struct pcie_info *info = bus_to_info(bus->number);
	unsigned long address = 0;

	if (unlikely(!info)) {
		pcie_error(
		"%s:Cannot find corresponding controller for appointed device!", __func__);
		BUG();
	}

	address = info->base_addr + (PCIE_CFG_BUS(bus->number)
		| PCIE_CFG_DEV(devfn) | PCIE_CFG_REG(where));

	return address;
}

static inline int is_pcie_link_up(struct pcie_info *info)
{
	int i;

	 for (i = 0; i < 10000; i++) {
		if (__arch_check_pcie_link(info))
			break;
		udelay(100);
	}

	return (i < 10000);
}

static int pcie_read_from_device(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 *value)
{
	struct pcie_info *info = bus_to_info(bus->number);
	unsigned int val;
	void __iomem *addr;
	int i = 0;

	if (unlikely(!info)) {
		pcie_error(
		"%s:Cannot find corresponding controller for appointed device!", __func__);
		BUG();
	}
	if (!is_pcie_link_up(info)) {
		pcie_debug(PCIE_DBG_MODULE, "pcie %d not link up!",
				info->controller);
		return -1;
	}

	addr = (void __iomem *)to_pcie_address(bus, devfn, where);

	val = readl(addr);

	i = 0;
	while (i < 2000) {
		__asm__ __volatile__("nop\n");
		i++;
	}

	if (pcie_errorvalue == 1) {
		pcie_errorvalue = 0;
		val = 0xffffffff;
	}

	if (size == 1)
		*value = ((val >> ((where & 0x3) << 3)) & 0xff);
	else if (size == 2)
		*value = ((val >> ((where & 0x3) << 3)) & 0xffff);
	else if (size == 4)
		*value = val;
	else{
		pcie_error("Unknown size(%d) for read ops", size);
		BUG();
	}

	return PCIBIOS_SUCCESSFUL;
}

static int pcie_read_from_dbi(struct pcie_info *info, unsigned int devfn,
		int where, int size, u32 *value)
{
	unsigned int val;

	/*
	 * For host-side config space read, ignore device func nr.
	 */
	if (devfn > 0)
		return -EIO;

	val = (u32)readl((void *)(info->conf_base_addr + (where & (~0x3))));

	if (1 == size)
		*value = (val >> ((where & 0x3) << 3)) & 0xff;
	else if (2 == size)
		*value = (val >> ((where & 0x3) << 3)) & 0xffff;
	else if (4 == size)
		*value = val;
	else {
		pcie_error("Unknown size for config read operation!");
		BUG();
	}

	return PCIBIOS_SUCCESSFUL;
}

static int pcie_read_conf(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 *value)
{
	struct pcie_info *info = bus_to_info(bus->number);
	int ret;

	if (unlikely(!info)) {
		pcie_error(
		"%s:Cannot find corresponding controller for appointed device!", __func__);
		BUG();
	}

	if (bus->number == info->root_bus_nr)
		ret =  pcie_read_from_dbi(info, devfn, where, size, value);
	else
		ret =  pcie_read_from_device(bus, devfn, where, size, value);

	pcie_debug(PCIE_DBG_REG,
		"bus %d, devfn %d, where 0x%x, size 0x%x, value 0x%x",
		bus->number & 0xff, devfn, where, size, *value);

	return ret;
}

static int pcie_write_to_device(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 value)
{
	struct pcie_info *info = bus_to_info(bus->number);
	void __iomem *addr;
	unsigned int org;
	unsigned long flag;

	if (unlikely(!info)) {
		pcie_error(
		"%s:Cannot find corresponding controller for appointed device!", __func__);
		BUG();
	}

	if (!is_pcie_link_up(info)) {
		pcie_debug(PCIE_DBG_MODULE, "pcie %d not link up!",
				info->controller);
		return -1;
	}

	spin_lock_irqsave(&cw_lock, flag);

	pcie_read_from_device(bus, devfn, where, 4, &org);

	addr = (void __iomem *)to_pcie_address(bus, devfn, where);

	if (size == 1) {
		org &= (~(0xff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	} else if (size == 2) {
		org &= (~(0xffff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	} else if (size == 4) {
		org = value;
	} else {
		pcie_error("Unknown size(%d) for read ops", size);
		BUG();
	}
	writel(org, addr);

	spin_unlock_irqrestore(&cw_lock, flag);

	return PCIBIOS_SUCCESSFUL;

}

static int pcie_write_to_dbi(struct pcie_info *info, unsigned int devfn,
		int where, int size, u32 value)
{
	unsigned long flag;
	unsigned int org;

	spin_lock_irqsave(&cw_lock, flag);

	if (pcie_read_from_dbi(info, devfn, where, 4, &org)) {
		pcie_error("Cannot read from dbi! 0x%x:0x%x:0x%x!",
				0, devfn, where);
		spin_unlock_irqrestore(&cw_lock, flag);
		return -EIO;
	}
	if (size == 1) {
		org &= (~(0xff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	} else if (size == 2) {
		org &= (~(0xffff << ((where & 0x3) << 3)));
		org |= (value << ((where & 0x3) << 3));
	} else if (size == 4) {
		org = value;
	} else {
		pcie_error("Unknown size(%d) for read ops", size);
		BUG();
	}
	writel(org, ((void __iomem *)info->conf_base_addr + (where & (~0x3))));

	spin_unlock_irqrestore(&cw_lock, flag);

	return PCIBIOS_SUCCESSFUL;
}

static int pcie_write_conf(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 value)
{
	struct pcie_info *info = bus_to_info(bus->number);

	pcie_debug(PCIE_DBG_REG,
		"bus %d, devfn %d, where 0x%x, size 0x%x, value 0x%x",
		bus->number & 0xff, devfn, where, size, value);

	if (unlikely(!info)) {
		pcie_error(
		"%s:Cannot find corresponding controller for appointed device!", __func__);
		BUG();
	}

	if (bus->number == info->root_bus_nr)
		return pcie_write_to_dbi(info, devfn, where, size, value);
	else
		return pcie_write_to_device(bus, devfn, where, size, value);
}

static struct pci_ops pcie_ops = {
	.read = pcie_read_conf,
	.write = pcie_write_conf,
};

void pci_set_max_rd_req_size(const struct pci_bus *bus)
{
	struct pci_dev *dev;
	struct pci_bus *child;
	int pos;
	unsigned short dev_contrl_reg_val = 0;
	unsigned int max_rd_req_size = 0;

	list_for_each_entry(dev, &bus->devices, bus_list) {

		/* set device max read requset size*/
		pos = pci_find_capability(dev, PCI_CAP_ID_EXP);
		if (pos) {
			pci_read_config_word(dev, pos + PCI_EXP_DEVCTL,
					&dev_contrl_reg_val);
			max_rd_req_size = (dev_contrl_reg_val >> 12) & 0x7;
			if (max_rd_req_size > 0x0) {
				dev_contrl_reg_val &= ~(max_rd_req_size << 12);
				pci_write_config_word(dev, pos + PCI_EXP_DEVCTL,
						dev_contrl_reg_val);
			}

		}
	}

	list_for_each_entry(dev, &bus->devices, bus_list) {
		BUG_ON(!dev->is_added);
		child = dev->subordinate;
		if (child)
			pci_set_max_rd_req_size(child);
	}
}

static struct hw_pci hipcie __initdata = {
	.nr_controllers = 1,
};
#ifdef CONFIG_ARM64

static int pci_common_init(struct platform_device *pdev, struct hw_pci *hipcie)
{
	struct device_node *dn = pdev->dev.of_node;
	struct pcie_info *info;
	struct pci_bus *bus;
	resource_size_t io_addr;
	int ret;
	int pcie_contrl;
	LIST_HEAD(res);

	of_property_read_u32(dn, "pcie_controller", &pcie_contrl);
	ret = of_pci_get_host_bridge_resources(dn, 0, 0xff, &res, &io_addr);
	if (ret)
		return ret;


	bus = pci_create_root_bus(&pdev->dev, 0, &pcie_ops, hipcie, &res);
	if (!bus)
		return -ENOMEM;

#ifdef CONFIG_LIMIT_MAX_RD_REQ_SIZE
	pci_set_max_rd_req_size(bus);
#endif

	pcie_info[pcie_contrl].root_bus_nr = bus->number;
	info = bus_to_info(bus->number);
	if (info != NULL)
		__arch_config_iatu_tbl(info, NULL);

	pci_scan_child_bus(bus);
	pci_assign_unassigned_bus_resources(bus);
	pci_bus_add_devices(bus);

	platform_set_drvdata(pdev, hipcie);

	return 0;
}
#else
static int pci_common_init_bvt(struct platform_device *pdev, struct hw_pci *hipcie)
{
	struct device_node *dn = pdev->dev.of_node;
	struct pcie_info *info;
	struct pci_bus *bus;
	resource_size_t io_addr;
	int ret;
	int bus_start;
	int pcie_contrl;

	LIST_HEAD(res);

	of_property_read_u32(dn, "pcie_controller", &pcie_contrl);
	if (pcie_contrl == 0)
		bus_start = 0;
	else
		bus_start = 2;

	ret = of_pci_get_host_bridge_resources(dn, bus_start, 0xff, &res, &io_addr);
	if (ret)
		return ret;

	bus = pci_create_root_bus(&pdev->dev, bus_start, &pcie_ops, hipcie, &res);
	if (!bus)
		return -ENOMEM;

#ifdef CONFIG_LIMIT_MAX_RD_REQ_SIZE
	pci_set_max_rd_req_size(bus);
#endif

	pcie_info[pcie_contrl].root_bus_nr = bus->number;
	info = bus_to_info(bus->number);
	if (info != NULL) {
		__arch_config_iatu_tbl(info, NULL);
	}

	pci_scan_child_bus(bus);
	pci_assign_unassigned_bus_resources(bus);
	pci_bus_add_devices(bus);

	platform_set_drvdata(pdev, hipcie);

	return 0;

}
#endif

static int __init pcie_init(struct platform_device *pdev)
{
	g_of_node = pdev->dev.of_node;
	if (!g_of_node) {
		pr_err("get node from dts failed! controller:%d\n", pcie_controllers_nr);
		return -EIO;
	}

	of_property_read_u32(g_of_node, "pcie_controller", &pcie_controllers_nr);

	if (__arch_pcie_info_setup(pcie_info, &pcie_controllers_nr))
		return -EIO;

	if (__arch_pcie_sys_init(pcie_info))
		goto pcie_init_err;
	hipcie.nr_controllers = pcie_controllers_nr;
	pr_err("Number of PCIe controllers: %d\n",
		hipcie.nr_controllers);

#ifdef CONFIG_ARM64
	pci_common_init(pdev, &hipcie);
#else
	pci_common_init_bvt(pdev, &hipcie);
#endif
	return 0;
pcie_init_err:
	__arch_pcie_info_release(pcie_info);

	return -EIO;
}

static int __exit pcie_uinit(struct platform_device *pdev)
{
	__arch_pcie_info_release(pcie_info);
	return 0;
}

#include <linux/platform_device.h>
#include <linux/pm.h>

int  hisi_pcie_plat_driver_probe(struct platform_device *pdev)
{
	return 0;
}
int  hisi_pcie_plat_driver_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
int hisi_pcie_plat_driver_suspend(struct device *dev)
{
	__arch_pcie_sys_exit();
	return 0;
}

int hisi_pcie_plat_driver_resume(struct device *dev)
{
	return __arch_pcie_sys_init(pcie_info);
}

const struct dev_pm_ops hisi_pcie_pm_ops = {
	.suspend = NULL,
	.suspend_noirq = hisi_pcie_plat_driver_suspend,
	.resume = NULL,
	.resume_noirq = hisi_pcie_plat_driver_resume
};

#define HISI_PCIE_PM_OPS (&hisi_pcie_pm_ops)
#else
#define HISI_PCIE_PM_OPS NULL
#endif

#define PCIE_RC_DRV_NAME "hisi pcie root complex"


static const struct of_device_id hisi_pcie_match_table[] = {
	{.compatible = "hisilicon,hisi-pcie",},
	{},
};

static struct platform_driver hisi_pcie_driver = {
	.driver = {
		.name = "hisi-pcie",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_pcie_match_table),
	},
	.probe = pcie_init,
};
module_platform_driver(hisi_pcie_driver);

MODULE_DESCRIPTION("Hisilicon PCI-Express Root Complex driver");
MODULE_LICENSE("GPL");

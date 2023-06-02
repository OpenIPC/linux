/*
 *  linux/arch/arm/mach-faraday/ftpci100.c
 *
 *  Faraday FTPCI100 PCI Bridge Controller
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/pci.h>
#include <asm/mach/pci.h>
#include <linux/ioport.h>
#include <asm/mach/irq.h>

#include <mach/ftpci100.h>

#include <mach/board-a320.h>

#define CONFIG_FTPCI100_CASCADE_IRQ

#ifndef MAX_FTPCI100_NR
#define MAX_FTPCI100_NR	1
#endif

struct ftpci100_chip_data {
	void __iomem *base;

#ifdef CONFIG_FTPCI100_CASCADE_IRQ
	unsigned int busnr;
	unsigned int irq_offset;
#endif
};

static struct ftpci100_chip_data ftpci100_data[MAX_FTPCI100_NR];

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline void __iomem *ftpci100_busnr_to_base(unsigned int busnr)
{
	void __iomem *base;

	if (busnr >= MAX_FTPCI100_NR)
		BUG();

	base = ftpci100_data[busnr].base;

	if (!base)
		BUG();

	return base;
}

static inline unsigned int
ftpci100_config_command(unsigned int busnr, unsigned int devfn, int where)
{
	return FTPCI100_CONFIG_ENABLE
		| FTPCI100_CONFIG_BUS(busnr)
		| FTPCI100_CONFIG_DEVFN(devfn)
		| FTPCI100_CONFIG_WHERE(where);
}

/******************************************************************************
 * struct pci_ops functions
 *****************************************************************************/
static int ftpci100_read_config(struct pci_bus *bus, unsigned int devfn, int where,
			     int size, u32 * val)
{
	void __iomem *base = ftpci100_busnr_to_base(bus->number);
	unsigned int command = ftpci100_config_command(bus->number, devfn, where);
	unsigned int data;

	writel(command, base + FTPCI100_OFFSET_CONFIG);
	data = readl(base + FTPCI100_OFFSET_CONFIGDATA);

	switch (size) {
	case 1:
		if (where & 2) data >>= 16;
		if (where & 1) data >>= 8;
		data &= 0xff;

		break;

	case 2:
		if (where & 2) data >>= 16;
		data &= 0xffff;
		break;

	default:
		break;
	}

	*val = data;

	return PCIBIOS_SUCCESSFUL;
}

static int ftpci100_write_config(struct pci_bus *bus, unsigned int devfn,
			      int where, int size, u32 val)
{
	void __iomem *base = ftpci100_busnr_to_base(bus->number);
	unsigned int command = ftpci100_config_command(bus->number, devfn, where);
	unsigned int data;
	unsigned int shift;

	writel(command, base + FTPCI100_OFFSET_CONFIG);

	switch (size) {
	case 1:
		data = readl(base + FTPCI100_OFFSET_CONFIGDATA);
		shift = (where & 0x3) * 8;
		data &= ~(0xff << shift);
		data |= (val & 0xff) << shift;
		writel(data, base + FTPCI100_OFFSET_CONFIGDATA);
		break;

	case 2:
		data = readl(base + FTPCI100_OFFSET_CONFIGDATA);
		shift = (where & 0x3) * 8;
		data &= ~(0xffff << shift);
		data |= (val & 0xffff) << shift;
		writel(data, base + FTPCI100_OFFSET_CONFIGDATA);
		break;

	case 4:
		writel(val, base + FTPCI100_OFFSET_CONFIGDATA);
		break;

	default:
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops ftpci100_ops = {
	.read	= ftpci100_read_config,
	.write	= ftpci100_write_config,
};


/******************************************************************************
 * struct hw_pci functions
 *****************************************************************************/

/* using virtual address for pci_resource_start() function*/
static struct resource ftpci100_io = {
	.name	= "Faraday PCI I/O Space",
	.start	= PCIIO_VA_BASE,
	.end	= PCIIO_VA_LIMIT,
	.flags	= IORESOURCE_IO,
};

/* using physical address for memory resource*/
static struct resource ftpci100_mem = {
	.name	= "Faraday PCI non-prefetchable Memory Space",
	.start	= PCIMEM_PA_BASE,
	.end	= PCIMEM_PA_LIMIT,
	.flags	= IORESOURCE_MEM,
};

static int __init ftpci100_setup_resource(struct resource **resource)
{
	int ret;

	ret = request_resource(&ioport_resource, &ftpci100_io);
	if (ret) {
		printk(KERN_ERR "PCI: unable to allocate io region (%d)\n", ret);
		goto out;
	}

	ret = request_resource(&iomem_resource, &ftpci100_mem);
	if (ret) {
		printk(KERN_ERR "PCI: unable to allocate non-prefetchable "
		       "memory region\n");
		goto release_io;
	}

	/*
	 * bus->resource[0] is the IO resource for this bus
	 * bus->resource[1] is the mem resource for this bus
	 * bus->resource[2] is the prefetch mem resource for this bus
	 */

	resource[0] = &ftpci100_io;
	resource[1] = &ftpci100_mem;
	resource[2] = NULL;

	return 1;

release_io:
	release_resource(&ftpci100_io);
out:
	return ret;
}

static int __init ftpci100_setup(int nr, struct pci_sys_data *sys)
{
	int ret = 0;

	if (nr == 0) {
		void __iomem *base = ftpci100_busnr_to_base(nr);
		unsigned int command;
		unsigned int data;

		writel(FTPCI100_CONFIG_ENABLE, base + FTPCI100_OFFSET_CONFIG);
		if (readl(base + FTPCI100_OFFSET_CONFIG) != FTPCI100_CONFIG_ENABLE)
			return 0;

		printk(KERN_INFO "PCI bridge %d found\n", nr);

		/* enable interrupts (INTA/INTB/INTC/INTD) */

		command = ftpci100_config_command(nr, 0, PCI_CR2);
		writel(command, base + FTPCI100_OFFSET_CONFIG);

		data = readl(base + FTPCI100_OFFSET_CONFIGDATA);
		data |= PCI_CR2_INTA_ENABLE
			| PCI_CR2_INTB_ENABLE
			| PCI_CR2_INTC_ENABLE
			| PCI_CR2_INTD_ENABLE;

		writel(data, base + FTPCI100_OFFSET_CONFIGDATA);

		/* write DMA start address/size data to the bridge configuration space */

		command = ftpci100_config_command(nr, 0, PCI_MEM_BASE_SIZE_1);
		writel(command, base + FTPCI100_OFFSET_CONFIG);

		writel(PCI_MEM_SIZE_1G, base + FTPCI100_OFFSET_CONFIGDATA);

		/* setup resources */

		ret = ftpci100_setup_resource(sys->resource);

		sys->mem_offset = 0;	//PCIMEM_VA_BASE - PCIMEM_PA_BASE;
		sys->io_offset = PCIIO_VA_BASE - PCIIO_PA_BASE;
	}

	return ret;
}

static struct pci_bus __init *ftpci100_scan_bus(int nr, struct pci_sys_data *sys)
{
	return pci_scan_bus(sys->busnr, &ftpci100_ops, sys);
}

/*
 * map the specified device/slot/pin to an IRQ
 * different backplanes may need to modify this
 */
static int __init ftpci100_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	int devslot = PCI_SLOT(dev->devfn);
	int irq = -1;

	switch (devslot) {
	case 8:
		irq = IRQ_FTPCI100_0_A;
		break;

	case 9:
		irq = IRQ_FTPCI100_0_B;
		break;

	case 10:
		irq = IRQ_FTPCI100_0_C;
		break;

	case 11:
		irq = IRQ_FTPCI100_0_D;
		break;

	default:
		printk(KERN_INFO "slot %d not supported\n", slot);
		break;
	}

	printk(KERN_INFO "PCI map irq: slot %d, pin %d, devslot %d, irq: %d\n",
		slot, pin, devslot, irq);

	return irq;
}
static struct hw_pci ftpci100_hw __initdata = {
	.nr_controllers	= 1,
	.setup		= ftpci100_setup,
	.scan		= ftpci100_scan_bus,
	.map_irq	= ftpci100_map_irq,
};

/******************************************************************************
 * initialization
 *****************************************************************************/
void __init ftpci100_init(unsigned int nr, void __iomem *base)
{
	if (nr >= MAX_FTPCI100_NR)
		BUG();

	ftpci100_data[nr].base = base;
}

static int __init ftpci100_bus_init(void)
{
	pci_common_init(&ftpci100_hw);

	return 0;
}

subsys_initcall(ftpci100_bus_init);

/******************************************************************************
 * irq handling
 *
 * There are 4 irq lines on FTPCI100.
 *
 * If the 4 irq lines are connected to the same irq line of the upper interrupt
 * controller, FTPCI100 needs to pretend that it is a secondary interrupt
 * controller.
 *
 * If the 4 lines are connected to 4 irq lines of the upper interrupt
 * controller directly, we do not need the following stuff at all.
 *****************************************************************************/
#ifdef CONFIG_FTPCI100_CASCADE_IRQ

/******************************************************************************
 * struct irq_chip functions
 *****************************************************************************/
static void ftpci100_int_ack(unsigned int irq)
{
	/* do nothing */
}

static void ftpci100_int_mask(unsigned int irq)
{
	/* do nothing */
}

static void ftpci100_int_unmask(unsigned int irq)
{
	/* do nothing */
}

static struct irq_chip ftpci100_int_chip = {
	.ack	= ftpci100_int_ack,
	.mask	= ftpci100_int_mask,
	.unmask	= ftpci100_int_unmask,
};

/******************************************************************************
 * initialization functions
 *****************************************************************************/
static void ftpci100_int_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct ftpci100_chip_data *chip_data = get_irq_data(irq);
	struct irq_chip *chip = get_irq_chip(irq);
	unsigned int command;
	void __iomem *base;
	unsigned int cascade_irq, hw_irq;
	unsigned int cr2;

	/* primary controller ack'ing */
	chip->mask(irq);
	chip->ack(irq);

	command = ftpci100_config_command(chip_data->busnr, 0, PCI_CR2);
	base = chip_data->base;

	writel(command, base + FTPCI100_OFFSET_CONFIG);
	cr2 = readl(base + FTPCI100_OFFSET_CONFIGDATA);


	if (cr2 & PCI_CR2_STATUS_INTA) {
		hw_irq = 0;
		cr2 &= ~PCI_CR2_STATUS_MASK;
		cr2 |= PCI_CR2_STATUS_INTA;

	} else if (cr2 & PCI_CR2_STATUS_INTB) {
		hw_irq = 1;
		cr2 &= ~PCI_CR2_STATUS_MASK;
		cr2 |= PCI_CR2_STATUS_INTB;

	} else if (cr2 & PCI_CR2_STATUS_INTC) {
		hw_irq = 2;
		cr2 &= ~PCI_CR2_STATUS_MASK;
		cr2 |= PCI_CR2_STATUS_INTC;

	} else if (cr2 & PCI_CR2_STATUS_INTD) {
		hw_irq = 3;
		cr2 &= ~PCI_CR2_STATUS_MASK;
		cr2 |= PCI_CR2_STATUS_INTD;

	} else
		goto out;

	cascade_irq = hw_irq + chip_data->irq_offset;
	generic_handle_irq(cascade_irq);

	/* clear interrupt status */

	writel(command, base + FTPCI100_OFFSET_CONFIG);
	writel(cr2, base + FTPCI100_OFFSET_CONFIGDATA);

out:
	/* primary controller unmasking */
	chip->unmask(irq);
}

void __init ftpci100_int_cascade_irq(unsigned int nr, unsigned int irq)
{
	if (nr >= MAX_FTPCI100_NR)
		BUG();

	if (set_irq_data(irq, &ftpci100_data[nr]) != 0)
		BUG();

	set_irq_chained_handler(irq, ftpci100_int_handle_cascade_irq);
}

void __init ftpci100_int_init(unsigned int nr, unsigned int irq_start)
{
	int i;

	if (nr >= MAX_FTPCI100_NR)
		BUG();

	ftpci100_data[nr].irq_offset = irq_start;
	ftpci100_data[nr].busnr = nr;

	for (i = irq_start; i < ftpci100_data[nr].irq_offset + 4; i++) {
		set_irq_chip(i ,&ftpci100_int_chip);
		set_irq_chip_data(i, &ftpci100_data[nr]);
		set_irq_handler(i, handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}
}
#endif

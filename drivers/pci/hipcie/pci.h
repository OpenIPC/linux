/*
 *  arch/arm/include/asm/mach/pci.h
 *
 *  Copyright (C) 2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_MACH_PCI_H
#define __ASM_MACH_PCI_H

#include <linux/ioport.h>

struct pci_sys_data;
struct pci_ops;
struct pci_bus;
struct device;

struct hw_pci {
#ifdef CONFIG_PCI_DOMAINS
    int     domain;
#endif
    struct pci_ops  *ops;
    int     nr_controllers;
    void        **private_data;
    int     (*setup)(int nr, struct pci_sys_data *);
    struct pci_bus *(*scan)(int nr, struct pci_sys_data *);
    void        (*preinit)(void);
    void        (*postinit)(void);
    u8      (*swizzle)(struct pci_dev *dev, u8 *pin);
    int     (*map_irq)(const struct pci_dev *dev, u8 slot, u8 pin);
    resource_size_t (*align_resource)(struct pci_dev *dev,
                                      const struct resource *res,
                                      resource_size_t start,
                                      resource_size_t size,
                                      resource_size_t align);
    void        (*add_bus)(struct pci_bus *bus);
    void        (*remove_bus)(struct pci_bus *bus);
};

/*
 * Per-controller structure
 */
struct pci_sys_data {
#ifdef CONFIG_PCI_DOMAINS
    int     domain;
#endif
    struct list_head node;
    int     busnr;      /* primary bus number           */
    u64     mem_offset; /* bus->cpu memory mapping offset   */
    unsigned long   io_offset;  /* bus->cpu IO mapping offset       */
    struct pci_bus  *bus;       /* PCI bus              */
    struct list_head resources; /* root bus resources (apertures)       */
    struct resource io_res;
    char        io_res_name[12];
    /* Bridge swizzling         */
    u8      (*swizzle)(struct pci_dev *, u8 *);
    /* IRQ mapping              */
    int     (*map_irq)(const struct pci_dev *, u8, u8);
    /* Resource alignement requirements */
    resource_size_t (*align_resource)(struct pci_dev *dev,
                                      const struct resource *res,
                                      resource_size_t start,
                                      resource_size_t size,
                                      resource_size_t align);
    void        (*add_bus)(struct pci_bus *bus);
    void        (*remove_bus)(struct pci_bus *bus);
    void        *private_data;  /* platform controller private data */
};

void __weak pcibios_update_irq(struct pci_dev *dev, int irq)
{
    dev_dbg(&dev->dev, "assigning IRQ %02d\n", irq);
    pci_write_config_byte(dev, PCI_INTERRUPT_LINE, irq);
}


#endif /* __ASM_MACH_PCI_H */

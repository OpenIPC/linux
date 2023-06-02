/*
 * Copyright (c) 2019 HiSilicon Technologies Co., Ltd.
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <linux/dma-mapping.h>
#include <asm/irq.h>

#define __24MHZ     24000000

#define PLL_TEST_NR 0xffffffff
#define LOOP_COUNT  5

#define PINMUX_BASE_PHYS0   0x4058000
#define PINMUX_BASE_PHYS1   0x47b8000
#define PINMUX_BASE_PHYS2   0x47e0000
#define PINMUX_BASE_PHYS3   0x47e8000

#define THE_CRG_BASE        0x4510000
#define PERI_CRG_PLL7       0x1c
#define PERI_CRG58          0xe8
#define PERI_CRG91          0x16C
#define PERI_CRG102     0x198
#define PERI_CRG105     0x1a4

#define APB_AXI_CLOCK_REG   PERI_CRG58

#define MISC_REG_BASE       0x4528000
#define MISC_CTRL73     0x124
#define MISC_CTRL74     0x128
#define MISC_CTRL75     0x12c
#define MISC_CTRL76     0x130
#define MISC_CTRL77     0x134
#define MISC_CTRL78     0x138
#define MISC_CTRL79     0x13c
#define MISC_CTRL80     0x140

#define SVB_CTRL_BASE       0x4528000
#define SVB_SYS_VOLTAGE_OFFSET  0x60

#define UART0_REG_PHYS      0x4540000
#define UART1_REG_PHYS      0x4541000
#define UART2_REG_PHYS      0x4542000
#define UART3_REG_PHYS      0x4543000
#define UART4_REG_PHYS      0x4544000
#define UART5_REG_PHYS      0x4545000
#define UART6_REG_PHYS      0x4546000
#define UART7_REG_PHYS      0x4547000
#define UART8_REG_PHYS      0x4548000

#define UART_DR         0x0
#define UART_CR         0x30

#define SPI0_REG_PHYS       0x4570000
#define SPI1_REG_PHYS       0x4571000
#define SPI2_REG_PHYS       0x4572000
#define SPI3_REG_PHYS       0x4573000
#define SPI4_REG_PHYS       0x4574000

#define HIEDMA0_REG_BASE    0x04040000
#define INT_TC1         0x4
#define INT_TC1_RAW     0x600

#define DMA_CHNL0_DONE      (1<<0)
#define DMA_CHNL1_DONE      (1<<1)

static char *dma_data;
static dma_addr_t dma_phys;

static void *crg_base;
static void *misc_base;
static void *edma0_reg_base;

struct pin_info {
	unsigned long pinmx_reg_base;
	unsigned long pinmx_reg_offset;
	unsigned int pinmx_func_num;
	unsigned int restored_val;
	unsigned int pinmx_func_changed;
};

struct pinmux_control {
	char *name;
	unsigned int pinmux_nr;
	struct pin_info pins[12];
};

static struct pinmux_control pmx_ctrl_list[] = {
	{
		.name = "uart1",
		.pinmux_nr = 4,
		.pins[0] = {0x047B8000, 0x30, 0x1, 0x0, 0},
		.pins[1] = {0x047B8000, 0x34, 0x1, 0x0, 0},
		.pins[2] = {0x047B8000, 0x38, 0x1, 0x0, 0},
		.pins[3] = {0x047B8000, 0x3c, 0x1, 0x0, 0},
	},
	{
		.name = "uart2",
		.pinmux_nr = 2,
		.pins[0] = {0x047B8000, 0x40, 0x1, 0x0, 0},
		.pins[1] = {0x047B8000, 0x44, 0x1, 0x0, 0},
	},
	{
		.name = "uart3",
		.pinmux_nr = 12,
		.pins[0] = {0x047e8000, 0x00, 0x1, 0x0, 0},
		.pins[1] = {0x047e8000, 0x04, 0x1, 0x0, 0},
		.pins[2] = {0x047e8000, 0x08, 0x1, 0x0, 0},
		.pins[3] = {0x047e8000, 0x0c, 0x1, 0x0, 0},
		.pins[4] = {0x047e0000, 0x00, 0x1, 0x0, 0},
		.pins[5] = {0x047e0000, 0x04, 0x1, 0x0, 0},
		.pins[6] = {0x047e0000, 0x08, 0x1, 0x0, 0},
		.pins[7] = {0x047e0000, 0x0c, 0x1, 0x0, 0},
		.pins[8] = {0x047e0000, 0x34, 0x4, 0x0, 0},
		.pins[9] = {0x047e0000, 0x38, 0x4, 0x0, 0},
		.pins[10] = {0x047e0000, 0x3c, 0x4, 0x0, 0},
		.pins[11] = {0x047e0000, 0x40, 0x4, 0x0, 0},
	},
	{
		.name = "uart4",
		.pinmux_nr = 2,
		.pins[0] = {0x047B8000, 0x38, 0x2, 0x0, 0},
		.pins[1] = {0x047B8000, 0x3c, 0x2, 0x0, 0},
	},
	{
		.name = "uart5",
		.pinmux_nr = 6,
		.pins[0] = {0x047e8000, 0x10, 0x1, 0x0, 0},
		.pins[1] = {0x047e8000, 0x14, 0x1, 0x0, 0},
		.pins[2] = {0x047e0000, 0x10, 0x1, 0x0, 0},
		.pins[3] = {0x047e0000, 0x14, 0x1, 0x0, 0},
		.pins[4] = {0x047e0000, 0x24, 0x6, 0x0, 0},
		.pins[5] = {0x047e0000, 0x28, 0x6, 0x0, 0},
	},
	{
		.name = "uart6",
		.pinmux_nr = 12,
		.pins[0] = {0x047e8000, 0x08, 0x2, 0x0, 0},
		.pins[1] = {0x047e8000, 0x0c, 0x2, 0x0, 0},
		.pins[2] = {0x047e0000, 0x08, 0x2, 0x0, 0},
		.pins[3] = {0x047e0000, 0x0c, 0x2, 0x0, 0},
		.pins[4] = {0x047e0000, 0x18, 0x1, 0x0, 0},
		.pins[5] = {0x047e0000, 0x1c, 0x1, 0x0, 0},
		.pins[6] = {0x047e8000, 0x18, 0x1, 0x0, 0},
		.pins[7] = {0x047e8000, 0x1c, 0x1, 0x0, 0},
		.pins[8] = {0x047e0000, 0x2c, 0x6, 0x0, 0},
		.pins[9] = {0x047e0000, 0x30, 0x6, 0x0, 0},
		.pins[10] = {0x047e0000, 0x3c, 0x6, 0x0, 0},
		.pins[11] = {0x047e0000, 0x40, 0x6, 0x0, 0},
	},
	{
		.name = "uart7",
		.pinmux_nr = 4,
		.pins[0] = {0x04058000, 0x30, 0x3, 0x0, 0},
		.pins[1] = {0x04058000, 0x34, 0x3, 0x0, 0},
		.pins[2] = {0x04058000, 0x38, 0x3, 0x0, 0},
		.pins[3] = {0x04058000, 0x3c, 0x3, 0x0, 0},
	},
	{
		.name = "uart8",
		.pinmux_nr = 4,
		.pins[0] = {0x04058000, 0x40, 0x3, 0x0, 0},
		.pins[1] = {0x04058000, 0x44, 0x3, 0x0, 0},
		.pins[2] = {0x04058000, 0x48, 0x3, 0x0, 0},
		.pins[3] = {0x04058000, 0x4c, 0x3, 0x0, 0},
	},
	{
		.name = "spi0",
		.pinmux_nr = 4,
		.pins[0] = {0x04058000, 0x54, 0x1, 0x0, 0},
		.pins[1] = {0x04058000, 0x58, 0x1, 0x0, 0},
		.pins[2] = {0x04058000, 0x5c, 0x1, 0x0, 0},
		.pins[3] = {0x04058000, 0x60, 0x1, 0x0, 0},
	},
	{
		.name = "spi1",
		.pinmux_nr = 4,
		.pins[0] = {0x04058000, 0x64, 0x1, 0x0, 0},
		.pins[1] = {0x04058000, 0x68, 0x1, 0x0, 0},
		.pins[2] = {0x04058000, 0x6c, 0x1, 0x0, 0},
		.pins[3] = {0x04058000, 0x70, 0x1, 0x0, 0},
	},
	{
		.name = "spi2",
		.pinmux_nr = 4,
		.pins[0] = {0x04058000, 0xc4, 0x1, 0x0, 0},
		.pins[1] = {0x04058000, 0xc8, 0x1, 0x0, 0},
		.pins[2] = {0x04058000, 0xcc, 0x1, 0x0, 0},
		.pins[3] = {0x04058000, 0xd0, 0x1, 0x0, 0},
	},
	{
		.name = "spi3",
		.pinmux_nr = 12,
		.pins[0] = {0x047e0000, 0x24, 0x2, 0x0, 0},
		.pins[1] = {0x047e0000, 0x28, 0x2, 0x0, 0},
		.pins[2] = {0x047e0000, 0x2c, 0x2, 0x0, 0},
		.pins[3] = {0x047e0000, 0x30, 0x2, 0x0, 0},
		.pins[4] = {0x047e0000, 0x38, 0x3, 0x0, 0},
		.pins[5] = {0x047e0000, 0x44, 0x3, 0x0, 0},
		.pins[6] = {0x047e0000, 0x5c, 0x3, 0x0, 0},
		.pins[7] = {0x047e0000, 0x60, 0x3, 0x0, 0},
		.pins[8] = {0x047e0000, 0x44, 0x4, 0x0, 0},
		.pins[9] = {0x047e0000, 0x48, 0x4, 0x0, 0},
		.pins[10] = {0x047e0000, 0x4c, 0x4, 0x0, 0},
		.pins[11] = {0x047e0000, 0x50, 0x4, 0x0, 0},
	},
	{
		.name = "spi4",
		.pinmux_nr = 4,
		.pins[0] = {0x04058000, 0x7c, 0x2, 0x0, 0},
		.pins[1] = {0x04058000, 0x80, 0x2, 0x0, 0},
		.pins[2] = {0x04058000, 0x84, 0x2, 0x0, 0},
		.pins[3] = {0x04058000, 0x88, 0x2, 0x0, 0},
	},
};

static void save_one_pin(struct pin_info *pin)
{
	unsigned int val;
	void *pmx_reg_virt = ioremap_nocache(pin->pinmx_reg_base, 0x1000);
	pmx_reg_virt += pin->pinmx_reg_offset;
	val = readl(pmx_reg_virt);

	if ((val & 0xf) == pin->pinmx_func_num) {
		pin->restored_val = val;
		val &= 0xfffffff0; // set as gpio
		writel(val, pmx_reg_virt);
		pin->pinmx_func_changed = 1;
	} else {
		pin->pinmx_func_changed = 0;

	}
	iounmap(pmx_reg_virt);
}

static void save_pmx_ctrl(struct pinmux_control *pmx_ctrl)
{
	int i;
	struct pin_info *pin = NULL;
	for (i = 0; i < pmx_ctrl->pinmux_nr; i++) {
		pin = &pmx_ctrl->pins[i];
		save_one_pin(pin);
	}
}

static void ptdev_pinmux_save(void)
{
	unsigned pmx_ctrl_nr = ARRAY_SIZE(pmx_ctrl_list);
	struct pinmux_control *pmx_ctrl = NULL;
	int i;

	for (i = 0; i < pmx_ctrl_nr; i++) {
		pmx_ctrl = &pmx_ctrl_list[i];
		save_pmx_ctrl(pmx_ctrl);
	}

}

static void restore_one_pin(struct pin_info *pin)
{
	void *pmx_reg_virt = ioremap_nocache(pin->pinmx_reg_base, 0x1000);
	pmx_reg_virt += pin->pinmx_reg_offset;

	if (pin->pinmx_func_changed) {
		writel(pin->restored_val, pmx_reg_virt);
	}

	iounmap(pmx_reg_virt);
}

static void restore_pmx_ctrl(struct pinmux_control *pmx_ctrl)
{
	int i;
	struct pin_info *pin = NULL;
	for (i = 0; i < pmx_ctrl->pinmux_nr; i++) {
		pin = &pmx_ctrl->pins[i];
		restore_one_pin(pin);
	}
}

static void ptdev_pinmux_restore(void)
{
	unsigned pmx_ctrl_nr = ARRAY_SIZE(pmx_ctrl_list);
	struct pinmux_control *pmx_ctrl = NULL;
	int i;

	for (i = 0; i < pmx_ctrl_nr; i++) {
		pmx_ctrl = &pmx_ctrl_list[i];
		restore_pmx_ctrl(pmx_ctrl);
	}
}

struct pll_trainning_dev {
	char *name;
	unsigned long ctrlreg_phys;
	void *ctrlreg_base;
	unsigned int tx_dma_reqline_val;
	unsigned int rx_dma_reqline_val;
	unsigned int reset_bit;
	unsigned int clock_gate_bit;

	void (*dev_clock_enable)(struct pll_trainning_dev *dev);
	void (*dev_rx_dma_init)(struct pll_trainning_dev *dev, unsigned int req_line);
	void (*dev_tx_dma_init)(struct pll_trainning_dev *dev, unsigned int req_line);
	void (*dev_dma_exit)(struct pll_trainning_dev *dev);
	int (*tx_trans_test)(struct pll_trainning_dev *dev);
	int (*rx_trans_test)(struct pll_trainning_dev *dev);
};

struct voltage_regulator {
	unsigned int curr;
	unsigned int max;
	unsigned int min;
	unsigned int reg_base;
	void *reg_virt;
	unsigned int steps[5];
	void (*init)(struct voltage_regulator *regulator);
	int (*adjust)(struct voltage_regulator *regulator, int step);
	void (*restore)(struct voltage_regulator *regulator);
	void (*exit)(struct voltage_regulator *regulator);
};

static unsigned int apb_axi_clock_switch(void *crg_base, unsigned long frequency)
{
	unsigned int old, val;
	void *addr = crg_base;
	if (frequency == __24MHZ) {
		addr += APB_AXI_CLOCK_REG;
		old = readl(addr);
		val = old & 0xffffffe6;
		writel(val, addr);
	}
	return old;
}

static void apb_axi_clock_restore(void *crg_base, unsigned int val)
{
	void *addr = crg_base;
	addr += APB_AXI_CLOCK_REG;
	writel(val, addr);
}

static void pll_reset(int select)
{
	unsigned int val, shift;
	void *addr = crg_base;
	addr += PERI_CRG_PLL7;

	if (select == 0) {
		shift = 21;
	} else {
		shift = 20;
	}

	val = readl(addr);
	val |= (1 << shift);
	writel(val, addr);

	udelay(1);

	val &= ~(1 << shift);
	writel(val, addr);
	udelay(1);
}

static void dma_reqline_clean(unsigned int val, void *base)
{
	writel(val, base + MISC_CTRL73);
	writel(val, base + MISC_CTRL74);
	writel(val, base + MISC_CTRL75);
	writel(val, base + MISC_CTRL76);
	writel(val, base + MISC_CTRL77);
	writel(val, base + MISC_CTRL78);
	writel(val, base + MISC_CTRL79);
	writel(val, base + MISC_CTRL80);
}

static void edma_apb_axi_clock_enable(void)
{
	void *addr = crg_base + PERI_CRG91;
	unsigned int val = readl(addr);

	val |= ((1 << 5) | (1 << 6));
	writel(val, addr);
	val |= (1 << 4);
	writel(val, addr);
	val &= ~(1 << 4);
	writel(val, addr);
}

static void pt_dev_clock_enable(struct pll_trainning_dev *dev)
{
	void *addr = crg_base + PERI_CRG102;
	unsigned int val = readl(addr);


	val |= (1 << dev->clock_gate_bit);
	writel(val, addr);

	val |= (1 << dev->reset_bit);
	writel(val, addr);

	val &= ~(1 << dev->reset_bit);
	writel(val, addr);
}

static void uart_rx_dma_init(struct pll_trainning_dev *dev,
			     unsigned int req_line)
{
	edma_apb_axi_clock_enable();
	if (dev->dev_clock_enable) {
		dev->dev_clock_enable(dev);
	}

	/* dma cfg */
	writel(0x0, edma0_reg_base + 0x688);
	writel(0xff, edma0_reg_base + 0x600);
	writel(0xff, edma0_reg_base + 0x608);
	writel(0xff, edma0_reg_base + 0x610);
	writel(0xff, edma0_reg_base + 0x618);
	writel(0xff, edma0_reg_base + 0x620);
	writel(0x0, edma0_reg_base + 0x18);
	writel(0x0, edma0_reg_base + 0x1c);
	writel(0x0, edma0_reg_base + 0x20);
	writel(0x0, edma0_reg_base + 0x24);
	writel(0x0, edma0_reg_base + 0x28);
	writel(0x0, edma0_reg_base + 0x830);
	writel(dev->ctrlreg_phys, edma0_reg_base + 0x820); //dma src address
	writel(0, edma0_reg_base + 0x824);
	writel(dma_phys + 0x800, edma0_reg_base + 0x828);//dma dest address
	writel(0, edma0_reg_base + 0x82c);
	writel(0x20, edma0_reg_base + 0x81c);//len

	writel(0x0, edma0_reg_base + 0x870);
	writel(dma_phys, edma0_reg_base + 0x860); //dma src address
	writel(0, edma0_reg_base + 0x864);
	writel(dev->ctrlreg_phys, edma0_reg_base + 0x868);//dma dest address
	writel(0, edma0_reg_base + 0x86c);//
	writel(0x20, edma0_reg_base + 0x85c);//len

	/* uart cfg */
	writel(0x7ff, dev->ctrlreg_base + 0x44);
	writel(0x1, dev->ctrlreg_base + 0x24);
	writel(0x0, dev->ctrlreg_base + 0x28);
	writel(0x70, dev->ctrlreg_base + 0x2c);
	writel(0x0, dev->ctrlreg_base + 0x34);
	writel(0x40, dev->ctrlreg_base + 0x38);
	writel(0x3, dev->ctrlreg_base + 0x48);
	udelay(1);
	writel(0x381, dev->ctrlreg_base + 0x30);
	udelay(1);

}

static void uart_dma_exit(struct pll_trainning_dev *dev)
{
	edma_apb_axi_clock_enable();
	if (dev->dev_clock_enable) {
		dev->dev_clock_enable(dev);
	}
}


/*
 * return: on -1, dma timeout;
 *     on -2, data err;
 */
static int do_uart_dma_rx_tst(struct pll_trainning_dev *dev,
			      unsigned int req_line, int revert)
{
	unsigned int left = req_line % 2;
	unsigned int result = req_line / 2;
	unsigned int shift;

	int i, j;
	unsigned int val;
	int ret = -1;

	void *addr = misc_base;
	val = 0x30303030;
	dma_reqline_clean(val, addr);

	if (!revert) {
		shift = left * 16;
		val &= ~(0xffff << shift);
		val |= ((dev->tx_dma_reqline_val << shift)
			| (dev->rx_dma_reqline_val << (shift + 8)));
		writel(val, addr + MISC_CTRL73 + result * 4);
	} else {
		shift = left * 16;
		val &= ~(0xffff << shift);
		val |= ((dev->rx_dma_reqline_val << shift)
			| (dev->tx_dma_reqline_val << (shift + 8)));
		writel(val, addr + MISC_CTRL73 + result * 4);
	}

	addr = dev->ctrlreg_base + UART_DR;
	for (i = 0; i < 1; i++) {
		ret = -1;

		if (!revert) {
			writel(0x47700005 + (req_line << 5) + 16, edma0_reg_base + 0x830);
			writel(0x87700005 + (req_line << 5), edma0_reg_base + 0x870);
		} else {
			writel(0x47700005 + (req_line << 5), edma0_reg_base + 0x830);
			writel(0x87700005 + (req_line << 5) + 16, edma0_reg_base + 0x870);
		}

		for (j = 0; j < 1000; j++) {
			val = readl(edma0_reg_base + INT_TC1_RAW);
			if ((val & DMA_CHNL0_DONE) && (val & DMA_CHNL1_DONE)) {
				ret = 0;
				writel(0x3, edma0_reg_base + INT_TC1_RAW);
				break;
			}
			udelay(1);
		}

		if (ret) {
			pr_debug("DMA time out[0x%x]!\n", val);
			return -1;
		}
	}
	return 0;
}

static int uart_rx_trans_test(struct pll_trainning_dev *dev)
{
	unsigned int i;
	int ret = 0;
	for (i = 0; i < 16; i++) {
		if (dev->dev_rx_dma_init) {
			dev->dev_rx_dma_init(dev, i);
		}

		ret = do_uart_dma_rx_tst(dev, i, 0);
		if (ret) {
			dev->dev_dma_exit(dev);
			pr_debug("dma[req line=%d] revert = %d, rx tst failed", i, 0);
			return ret;
		}

		ret = do_uart_dma_rx_tst(dev, i, 1);
		if (ret) {
			dev->dev_dma_exit(dev);
			pr_debug("dma[req line=%d] revert = %d, rx tst failed", i, 1);
			return ret;
		}

		dev->dev_dma_exit(dev);
	}

	return ret;
}

static int start_training(struct pll_trainning_dev *devs, unsigned int dev_nr)
{
	int i, ret;
	int pass = 1;

	for (i = 0; i < dev_nr; i++) {
		struct pll_trainning_dev *pt_dev = &devs[i];
		if (pt_dev->tx_trans_test) {
			ret = pt_dev->tx_trans_test(pt_dev);
			if (ret) {
				pass = 0;
				pr_debug("        Dev[%s/%d]tx trainning failed!\n",
					 pt_dev->name, i);
				break;
			}
		}
		if (pt_dev->rx_trans_test) {
			ret = pt_dev->rx_trans_test(pt_dev);
			if (ret) {
				pass = 0;
				pr_debug("        Dev[%s/%d]rx trainning failed!\n",
					 pt_dev->name, i);
				break;
			}
		}
	}

	return pass;
}

static void spi_rx_dma_init(struct pll_trainning_dev *dev, unsigned int req_line)
{

	edma_apb_axi_clock_enable();
	if (dev->dev_clock_enable) {
		dev->dev_clock_enable(dev);
	}


	/* dma cfg */
	writel(0x0, edma0_reg_base + 0x688);
	writel(0xff, edma0_reg_base + 0x600);
	writel(0xff, edma0_reg_base + 0x608);
	writel(0xff, edma0_reg_base + 0x610);
	writel(0xff, edma0_reg_base + 0x618);
	writel(0xff, edma0_reg_base + 0x620);
	writel(0x0, edma0_reg_base + 0x18);
	writel(0x0, edma0_reg_base + 0x1c);
	writel(0x0, edma0_reg_base + 0x20);
	writel(0x0, edma0_reg_base + 0x24);
	writel(0x0, edma0_reg_base + 0x28);

	/* spi rx  */
	writel(0x0, edma0_reg_base + 0x830);
	writel(dev->ctrlreg_phys + 0x8, edma0_reg_base + 0x820); //dma src address
	writel(0, edma0_reg_base + 0x824);
	writel(dma_phys + 0x800, edma0_reg_base + 0x828);//dma dest address
	writel(0, edma0_reg_base + 0x82c);
	writel(0x407, edma0_reg_base + 0x81c);//len

	/* spi tx */
	writel(0x0, edma0_reg_base + 0x870);
	writel(dma_phys, edma0_reg_base + 0x860); //dma src address
	writel(0, edma0_reg_base + 0x864);
	writel(dev->ctrlreg_phys + 0x8, edma0_reg_base + 0x868);//dma dest address
	writel(0, edma0_reg_base + 0x86c);
	writel(0x407, edma0_reg_base + 0x85c);//len

	/* spi cfg */
	writel(0x4, dev->ctrlreg_base + 0x10);
	writel(0x1f, dev->ctrlreg_base);
	writel(0x0, dev->ctrlreg_base + 0x14);
	writel(0x2, dev->ctrlreg_base + 0x28);
	writel(0x2, dev->ctrlreg_base + 0x2c);
	writel(0x3, dev->ctrlreg_base + 0x24);

}

static void spi_dma_exit(struct pll_trainning_dev *dev)
{
	edma_apb_axi_clock_enable();
	if (dev->dev_clock_enable) {
		dev->dev_clock_enable(dev);
	}
}

static int do_spi_dma_rx_tst(struct pll_trainning_dev *dev, unsigned int req_line, int revert)
{
	unsigned int left = req_line % 2;
	unsigned int result = req_line / 2;
	unsigned int shift;

	int i, j;
	unsigned int val;
	int ret = -1;

	void *addr = misc_base;
	val = 0x30303030;
	dma_reqline_clean(val, addr);

	if (!revert) {
		shift = left * 16;
		val &= ~(0xffff << shift);
		val |= ((dev->tx_dma_reqline_val << shift)
			| (dev->rx_dma_reqline_val << (shift + 8)));
		writel(val, addr + MISC_CTRL73 + result * 4);
	} else {
		shift = left * 16;
		val &= ~(0xffff << shift);
		val |= ((dev->rx_dma_reqline_val << shift)
			| (dev->tx_dma_reqline_val << (shift + 8)));
		writel(val, addr + MISC_CTRL73 + result * 4);
	}
	for (i = 0; i < 1; i++) {
		ret = -1;
		writel(0x0, dev->ctrlreg_base + 0x4);
		if (!revert) {
			writel(0x47711005 + (req_line << 5) + 16, edma0_reg_base + 0x830);
			writel(0x87711005 + (req_line << 5), edma0_reg_base + 0x870);
		} else {
			writel(0x47711005 + (req_line << 5), edma0_reg_base + 0x830);
			writel(0x87711005 + (req_line << 5) + 16, edma0_reg_base + 0x870);
		}
		writel(0x3, dev->ctrlreg_base + 0x4);

		for (j = 0; j < 1000; j++) {
			val = readl(edma0_reg_base + INT_TC1_RAW);
			if ((val & DMA_CHNL0_DONE) && (val & DMA_CHNL1_DONE)) {
				ret = 0;
				writel(0x3, edma0_reg_base + INT_TC1_RAW);
				break;
			}
			udelay(1);
		}

		if (ret) {
			pr_debug("DMA time out[0x%x]!\n", val);
			return -1;
		}
	}
	return 0;
}

static int spi_rx_trans_test(struct pll_trainning_dev *dev)
{
	unsigned int i;
	int ret = 0;
	for (i = 0; i < 16; i++) {
		if (dev->dev_rx_dma_init) {
			dev->dev_rx_dma_init(dev, i);
		}

		ret = do_spi_dma_rx_tst(dev, i, 0);
		if (ret) {
			dev->dev_dma_exit(dev);
			pr_debug("dma[req line=%d] revert = %d, rx tst failed", i, 0);
			return ret;
		}

		ret = do_spi_dma_rx_tst(dev, i, 1);
		if (ret) {
			dev->dev_dma_exit(dev);
			pr_debug("dma[req line=%d] revert = %d, rx tst failed", i, 1);
			return ret;
		}

		dev->dev_dma_exit(dev);
	}

	return ret;
}


struct pll_trainning_dev train_prev_devs[] = {
	{
		.name = "spi0",
		.ctrlreg_phys = SPI0_REG_PHYS,
		.tx_dma_reqline_val = 0x26,
		.rx_dma_reqline_val = 0x27,
		.clock_gate_bit = 24,
		.reset_bit = 8,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = spi_rx_dma_init,
		.dev_dma_exit = spi_dma_exit,
		.rx_trans_test = spi_rx_trans_test
	},
	{
		.name = "spi1",
		.ctrlreg_phys = SPI1_REG_PHYS,
		.tx_dma_reqline_val = 0x28,
		.rx_dma_reqline_val = 0x29,
		.clock_gate_bit = 25,
		.reset_bit = 9,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = spi_rx_dma_init,
		.dev_dma_exit = spi_dma_exit,
		.rx_trans_test = spi_rx_trans_test
	},
	{
		.name = "spi2",
		.ctrlreg_phys = SPI2_REG_PHYS,
		.tx_dma_reqline_val = 0x2a,
		.rx_dma_reqline_val = 0x2b,
		.clock_gate_bit = 26,
		.reset_bit = 10,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = spi_rx_dma_init,
		.dev_dma_exit = spi_dma_exit,
		.rx_trans_test = spi_rx_trans_test
	},
	{
		.name = "spi3",
		.ctrlreg_phys = SPI3_REG_PHYS,
		.tx_dma_reqline_val = 0x2c,
		.rx_dma_reqline_val = 0x2d,
		.clock_gate_bit = 27,
		.reset_bit = 11,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = spi_rx_dma_init,
		.dev_dma_exit = spi_dma_exit,
		.rx_trans_test = spi_rx_trans_test
	},
	{
		.name = "spi4",
		.ctrlreg_phys = SPI4_REG_PHYS,
		.tx_dma_reqline_val = 0x2e,
		.rx_dma_reqline_val = 0x2f,
		.clock_gate_bit = 28,
		.reset_bit = 12,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = spi_rx_dma_init,
		.dev_dma_exit = spi_dma_exit,
		.rx_trans_test = spi_rx_trans_test
	},
};

struct pll_trainning_dev train_devs[] = {
	{
		.name = "uart7",
		.ctrlreg_phys = UART7_REG_PHYS,
		.tx_dma_reqline_val = 0x3c,
		.rx_dma_reqline_val = 0x3d,
		.clock_gate_bit = 23,
		.reset_bit = 7,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},
	{
		.name = "uart1",
		.ctrlreg_phys = UART1_REG_PHYS,
		.tx_dma_reqline_val = 0x2,
		.rx_dma_reqline_val = 0x3,
		.clock_gate_bit = 17,
		.reset_bit = 1,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},

	{
		.name = "uart2",
		.ctrlreg_phys = UART2_REG_PHYS,
		.tx_dma_reqline_val = 0x4,
		.rx_dma_reqline_val = 0x5,
		.clock_gate_bit = 18,
		.reset_bit = 2,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},
	{
		.name = "uart3",
		.ctrlreg_phys = UART3_REG_PHYS,
		.tx_dma_reqline_val = 0x6,
		.rx_dma_reqline_val = 0x7,
		.clock_gate_bit = 19,
		.reset_bit = 3,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},
	{
		.name = "uart4",
		.ctrlreg_phys = UART4_REG_PHYS,
		.tx_dma_reqline_val = 0x8,
		.rx_dma_reqline_val = 0x9,
		.clock_gate_bit = 20,
		.reset_bit = 4,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},
	{
		.name = "uart5",
		.ctrlreg_phys = UART5_REG_PHYS,
		.tx_dma_reqline_val = 0xa,
		.rx_dma_reqline_val = 0xb,
		.clock_gate_bit = 21,
		.reset_bit = 5,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},
	{
		.name = "uart6",
		.ctrlreg_phys = UART6_REG_PHYS,
		.tx_dma_reqline_val = 0xc,
		.rx_dma_reqline_val = 0xd,
		.clock_gate_bit = 22,
		.reset_bit = 6,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},

	{
		.name = "uart8",
		.ctrlreg_phys = UART8_REG_PHYS,
		.tx_dma_reqline_val = 0x3e,
		.rx_dma_reqline_val = 0x3f,
		.clock_gate_bit = 29,
		.reset_bit = 13,
		.dev_clock_enable = pt_dev_clock_enable,
		.dev_rx_dma_init = uart_rx_dma_init,
		.dev_dma_exit = uart_dma_exit,
		.rx_trans_test = uart_rx_trans_test
	},
};

static void svb_voltage_regulator_init(struct voltage_regulator *regulator)
{
	int i;
	unsigned int val;
	regulator->reg_virt = ioremap_nocache(regulator->reg_base, 0x1000);
	val = readl(regulator->reg_virt);
	regulator->curr = (val >> 16) & 0xff;

	regulator->steps[0] = regulator->curr + 74;
	regulator->steps[1] = regulator->curr + 37;
	regulator->steps[2] = regulator->curr;
	if (regulator->curr > 37) {
		regulator->steps[3] = regulator->curr - 37;
	} else {
		regulator->steps[3] = 0;
	}

	if (regulator->curr > 74) {
		regulator->steps[4] = regulator->curr - 74;
	} else {
		regulator->steps[4] = 0;
	}

	if (regulator->curr >= regulator->max) {
		regulator->steps[0] = 0xffffffff;
		regulator->steps[1] = 0xffffffff;
		goto out1;
	}

	if (regulator->curr <= regulator->min) {
		regulator->steps[3] = 0xffffffff;
		regulator->steps[4] = 0xffffffff;
		goto out1;
	}

	if ((regulator->curr + 37) >= regulator->max) {
		regulator->steps[0] = 0xffffffff;
		regulator->steps[1] = regulator->max;
		goto out1;
	}

	if ((regulator->curr + 74) >= regulator->max) {
		regulator->steps[0] = regulator->max;
		goto out1;
	}

	if ((regulator->curr > 37) &&
	    ((regulator->curr - 37) <= regulator->min)) {
		regulator->steps[4] = 0xffffffff;
		regulator->steps[3] = regulator->min;
		goto out1;
	}
	if ((regulator->curr > 74) &&
	    ((regulator->curr - 74) <= regulator->min)) {
		regulator->steps[4] = regulator->min;
	}

out1:
	regulator->max = ((regulator->curr + 74) < regulator->max)
			 ? (regulator->curr + 74) : regulator->max;
	regulator->min = ((regulator->curr - 74) > regulator->min)
			 ? (regulator->curr - 74) : regulator->min;

	pr_debug("svb voltage min/max[0x%x/0x%x] steps: ",
		 regulator->min, regulator->max);
	for (i = 0; i < 5; i++) {
		pr_debug(" 0x%x ", regulator->steps[i]);
	}
	pr_debug("\n");
}

static int svb_voltage_adjust(struct voltage_regulator *regulator, int step)
{
	unsigned int val;

	if (regulator->steps[step] == 0xffffffff) {
		return -1;
	}

	val = readl(regulator->reg_virt);
	val &= 0xff00ffff;
	val |= regulator->steps[step] << 16;
	val |= (0x1 << 2);
	writel(val, regulator->reg_virt);

	return 0;
}

static void svb_voltage_restore(struct voltage_regulator *regulator)
{
	unsigned int val = readl(regulator->reg_virt);
	val &= 0xff00ffff;
	val |= regulator->curr << 16;
	val |= (0x1 << 2);
	writel(val, regulator->reg_virt);
}

static void svb_voltage_regulator_exit(struct voltage_regulator *regulator)
{
	iounmap(regulator->reg_virt);
}


struct voltage_regulator volt_regulator = {
	.min    = 0,
	.max    = 0x19f,
	.reg_base = 0x4528060,
	.steps  = {0},
	.init   = svb_voltage_regulator_init,
	.adjust = svb_voltage_adjust,
	.restore = svb_voltage_restore,
	.exit   = svb_voltage_regulator_exit
};

static int __init pll_trainning_init(void)
{
	int ret = 0, i, j;
	unsigned int dev_nr;
	unsigned int ssp_dev_nr;
	unsigned long old_clock_freq;
	unsigned int pll_reset_counts = PLL_TEST_NR - 1;
	int trainning_pass = 1;
	unsigned int old_uart_freq;

	pr_debug("=========================================================================\n");
	ssp_dev_nr = ARRAY_SIZE(train_prev_devs);
	dev_nr = ARRAY_SIZE(train_devs);
	crg_base = ioremap_nocache(THE_CRG_BASE, 0x1000);
	if (!crg_base) {
		pr_debug("crg ioremap error!\n");
		return -EINVAL;
	}
	misc_base = ioremap_nocache(MISC_REG_BASE, 0x1000);
	if (!misc_base) {
		iounmap(crg_base);
		pr_debug("misc ioremap error!\n");
		return -EINVAL;
	}
	edma0_reg_base = ioremap_nocache(HIEDMA0_REG_BASE, 0x1000);
	if (!edma0_reg_base) {
		pr_debug("dma reg ioremap error!\n");
		iounmap(misc_base);
		iounmap(crg_base);
		return -EINVAL;
	}
	/* set uart clock to 50MHz to speed up the trainning */
	old_uart_freq = readl(crg_base + PERI_CRG105);
	writel(0x0, crg_base + PERI_CRG105);

	ptdev_pinmux_save();

	dma_data = dma_alloc_coherent(NULL, 0x1000, &dma_phys, GFP_KERNEL);
	memset(dma_data, 0x5e, 0x800);

	for (i = 0; i < ssp_dev_nr; i++) {
		struct pll_trainning_dev *pt_dev = &train_prev_devs[i];
		pt_dev->ctrlreg_base = ioremap_nocache(pt_dev->ctrlreg_phys, 0x1000);
	}

	for (i = 0; i < dev_nr; i++) {
		struct pll_trainning_dev *pt_dev = &train_devs[i];
		pt_dev->ctrlreg_base = ioremap_nocache(pt_dev->ctrlreg_phys, 0x1000);
	}

	if (volt_regulator.init) {
		volt_regulator.init(&volt_regulator);
	}

	do {
		pr_debug("### SSP pll reset count %d\n",
			 (PLL_TEST_NR - pll_reset_counts));

		for (j = 0; (j < LOOP_COUNT) && (trainning_pass == 1); j++) {
			for (i = 0; (i < ARRAY_SIZE(volt_regulator.steps)) && (trainning_pass == 1); i++) {

				ret = volt_regulator.adjust(&volt_regulator, i);
				if (ret) {
					/*
					 * invalid voltage range, no need to do the trainning.
					 */
					continue;
				}

				trainning_pass = start_training(train_prev_devs, ssp_dev_nr);
			}
		}

		if (trainning_pass != 1) {
			pr_debug("%s %d: trainning failed!!\n", __func__, __LINE__);
			pr_debug("%s %d: LOOP_COUNT=%d\n", __func__, __LINE__, j);
			trainning_pass = 1;
		} else {
			pr_debug("\n\n	SSP TRAINNING PASS!\n\n");
			goto __ssp_done;
		}

		/* Trainning failed in current pll, try next one. */
		old_clock_freq = apb_axi_clock_switch(crg_base, __24MHZ);
		pll_reset(0);
		apb_axi_clock_restore(crg_base, old_clock_freq);
	} while (pll_reset_counts--);

	/* ssp trainning failed, warn and give it up */
	pr_debug("\n\n	SSP PLL trainning failed!!!\n\n");
	dump_stack();
	goto __all_done;

__ssp_done:

	pll_reset_counts = PLL_TEST_NR - 1;
	do {
		pr_debug("### UART pll reset count %d\n",
			 (PLL_TEST_NR - pll_reset_counts));

		for (j = 0; (j < LOOP_COUNT) && (trainning_pass == 1); j++) {
			for (i = 0; (i < ARRAY_SIZE(volt_regulator.steps)) && (trainning_pass == 1); i++) {

				ret = volt_regulator.adjust(&volt_regulator, i);
				if (ret) {
					/*
					 * invalid voltage range, no need to do the trainning.
					 */
					continue;
				}

				trainning_pass = start_training(train_devs, dev_nr);
			}
		}

		if (trainning_pass != 1) {
			pr_debug("%s %d: trainning failed!!\n", __func__, __LINE__);
			pr_debug("%s %d: LOOP_COUNT=%d\n", __func__, __LINE__, j);
			trainning_pass = 1;
		} else {
			pr_err("\n\n	EDMA PLL TRAINNING ALL PASS!\n\n");
			goto __all_done;
		}

		/* Trainning failed in current pll, try next one. */
		old_clock_freq = apb_axi_clock_switch(crg_base, __24MHZ);
		pll_reset(1);
		apb_axi_clock_restore(crg_base, old_clock_freq);
	} while (pll_reset_counts--);

	/* Trainning all failed, warn and give it up */
	pr_debug("\n\n	PLL trainning failed!!!\n\n");
	dump_stack();

__all_done:


	volt_regulator.restore(&volt_regulator);
	volt_regulator.exit(&volt_regulator);

	ptdev_pinmux_restore();


	for (i = 0; i < ssp_dev_nr; i++) {
		struct pll_trainning_dev *pt_dev = &train_prev_devs[i];
		iounmap(pt_dev->ctrlreg_base);
	}

	for (i = 0; i < dev_nr; i++) {
		struct pll_trainning_dev *pt_dev = &train_devs[i];
		iounmap(pt_dev->ctrlreg_base);
	}

	/* restore the old uart clock (24MHz) */
	writel(old_uart_freq, crg_base + PERI_CRG105);

	dma_free_coherent(NULL, 0x1000, dma_data, dma_phys);
	iounmap(edma0_reg_base);
	iounmap(misc_base);
	iounmap(crg_base);

	return ret;
}

arch_initcall(pll_trainning_init);

MODULE_DESCRIPTION("Hi3519aV100 PLL Trainning");
MODULE_AUTHOR("Hisilicon");
MODULE_LICENSE("GPL");

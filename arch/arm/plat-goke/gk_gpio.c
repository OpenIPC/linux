/*
 * arch/arm/mach-gk7101/gpio.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/io.h>

/*
    m instances per chip
    n banks per instance
    max 64 pins per bank
    pin: used as pin number at chip
    offset: used as offset at it's instance
    pin = it's instance base number + offset
*/

static DEFINE_MUTEX(gk_gpio_mtx);
extern struct gk_gpio_inst gk_gpio_insts[CONFIG_GK_GPIO_INSTANCES];

void GH_GPIO_set_INPUT_CFG_in_sel(u8 index, u8 data)
{
    GH_GPIO_INPUT_CFG_S  in_data;
    in_data.all = gk_gpio_insts[0].input_cfg[index];
    in_data.bitc.in_sel = data;
    gk_gpio_insts[0].input_cfg[index] = in_data.all;
	gk_gpio_writel(gk_gpio_insts[0].gpio_bank[0].base_reg + REG_GPIO_INPUT_CFG_OFFSET + (index * 0x4), in_data.all);
}

/* ==========================================================================*/
#define to_gk_gpio_chip(c) \
    container_of(c, struct gk_gpio_bank, chip)

static struct gk_gpio_inst* gk_gpio_id_to_inst(u32 pin)
{
    u32 i;
    u32 j;
    if (pin < 0)
    {
        return NULL;
    }
    else
    {
        for(i=0; i<CONFIG_GK_GPIO_INSTANCES; i++)
        {
            for(j=0; j<gk_gpio_insts[i].bank_num; j++)
            {
                if((pin >= gk_gpio_insts[i].gpio_bank[j].chip.base) &&
                   (pin < (gk_gpio_insts[i].gpio_bank[j].chip.base +
                          gk_gpio_insts[i].gpio_bank[j].chip.ngpio)))
                {
                    return &gk_gpio_insts[i];
                }
            }
        }
    }
    return NULL;
}

static struct gk_gpio_bank* gk_gpio_id_to_bank(u32 pin)
{
    u32 i;
    u32 j;
    if (pin< 0)
    {
        return NULL;
    }
    else
    {
        for(i=0; i<CONFIG_GK_GPIO_INSTANCES; i++)
        {
            for(j=0; j<gk_gpio_insts[i].bank_num; j++)
            {
                if((pin >= gk_gpio_insts[i].gpio_bank[j].chip.base) &&
                   (pin < (gk_gpio_insts[i].gpio_bank[j].chip.base +
                          gk_gpio_insts[i].gpio_bank[j].chip.ngpio)))
                {
                    return &gk_gpio_insts[i].gpio_bank[j];
                }
            }
        }
    }
    return NULL;
}



int gk_gpio_request(struct gpio_chip *chip, u32 offset)
{
    int ret = 0;
    struct gk_gpio_bank *bank;
    u32 pin;

    bank = to_gk_gpio_chip(chip);
    mutex_lock(&gk_gpio_mtx);

    pin = offset + chip->base;
    if (test_bit(pin, (const volatile unsigned long *)
        gk_gpio_insts[bank->index].gpio_valid))
    {
        if (test_bit(pin, (const volatile unsigned long *)
            gk_gpio_insts[bank->index].gpio_freeflag))
        {
            __clear_bit(pin, (volatile unsigned long *)
                gk_gpio_insts[bank->index].gpio_freeflag);
        }
        else
        {
            ret = -EACCES;
        }
    }
    else
    {
        ret = -EPERM;
    }

    mutex_unlock(&gk_gpio_mtx);

    return ret;
}

void gk_gpio_free(struct gpio_chip *chip, u32 offset)
{
    u32 pin;
    struct gk_gpio_bank *bank;

    mutex_lock(&gk_gpio_mtx);

    pin = offset + chip->base;
    bank = to_gk_gpio_chip(chip);

    __set_bit(pin, (volatile unsigned long *)
        gk_gpio_insts[bank->index].gpio_freeflag);

    mutex_unlock(&gk_gpio_mtx);
}

int gk_gpio_func_config(u32 pin, u32 func)
{
    int                 ret = 0;
    unsigned long       flags;
    struct gk_gpio_bank *bank;

    bank = gk_gpio_id_to_bank(pin);
    if (bank == NULL)
    {
        pr_err("%s: invalid GPIO %d for func %d.\n", __func__, pin, func);
        return -EINVAL;
    }
    spin_lock_irqsave(&bank->lock, flags);

    if(GPIO_GET_FUNC(func) > GPIO_FUNC_INOUT)
    {
        pr_err("%s: invalid GPIO func %d for GPIO:%d.\n", __func__, func, pin);
        spin_unlock_irqrestore(&bank->lock, flags);
        return -EINVAL;
    }

    //printk("gpio==============[%s %d]\n", __func__, __LINE__);
    ret =  gk_gpio_set_type(bank, pin, func);
    spin_unlock_irqrestore(&bank->lock, flags);

    return ret;

}

int gk_gpio_direction_input(struct gpio_chip *chip, u32 offset, int val)
{
    int ret = 0;
    u32 pin;

    pin = offset + chip->base;
    if (val == 0)
        ret = gk_gpio_func_config(pin, GPIO_TYPE_INPUT_0);
    else
        ret = gk_gpio_func_config(pin, GPIO_TYPE_INPUT_1);
    return ret;
}

static inline int gk_gpio_inline_get(struct gk_gpio_bank *bank, u32 pin)
{
    unsigned long   flags;
    u32 val = 0;

    spin_lock_irqsave(&bank->lock, flags);

    if((pin - bank->chip.base) < 32)
    {
        val = gk_gpio_readl(bank->base_reg + REG_GPIO_DIN_LOW_OFFSET);
    }
    else
    {
        val = gk_gpio_readl(bank->base_reg + REG_GPIO_DIN_HIGH_OFFSET);
    }

    spin_unlock_irqrestore(&bank->lock, flags);

    val = (val >> (pin % 32)) & 0x1;
    return (val ? GPIO_HIGH : GPIO_LOW);

}

int gk_gpio_get(u32 pin)
{
    struct gk_gpio_bank *bank;

    bank = gk_gpio_id_to_bank(pin);
    if (bank == NULL)
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pin);
        return 0;
    }

    return gk_gpio_inline_get(bank, (u32)pin);
}
EXPORT_SYMBOL(gk_gpio_get);

int gk_gpio_get_ex(struct gpio_chip *chip, unsigned offset)
{
    u32                 pin;

    pin = offset + chip->base;
    return gk_gpio_get(pin);
}
EXPORT_SYMBOL(gk_gpio_get_ex);

void gk_gpio_set_out(u32 pin, u32 value)
{
    struct gk_gpio_bank* bank;
    GH_GPIO_OUTPUT_CFG_S data;
    bank = gk_gpio_id_to_bank(pin);
    data.all = gk_gpio_insts[bank->index].output_cfg[pin];
    data.bitc.out_sel = 1;
    if (value == GPIO_LOW)
    {
        data.bitc.out_sel &= ~0x01;
    }
    else
    {
        data.bitc.out_sel |= 0x01;
    }
    gk_gpio_writel(bank->base_reg + REG_GPIO_OUTPUT_CFG_OFFSET + ((pin - bank->chip.base) * 0x4),
        data.all);
    gk_gpio_insts[bank->index].output_cfg[pin] = data.all;
}
EXPORT_SYMBOL(gk_gpio_set_out);

static inline void gk_gpio_inline_set(struct gk_gpio_bank *bank, u32 pin, int value)
{
    unsigned long               flags;

    spin_lock_irqsave(&bank->lock, flags);
    gk_gpio_set_out(pin, value);
    spin_unlock_irqrestore(&bank->lock, flags);
}

int gk_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int val)
{
    int                 ret = 0;
    u32                 pin;
    struct gk_gpio_bank *bank;

    pin = offset + chip->base;

    bank = gk_gpio_id_to_bank(pin);
    ret = gk_gpio_func_config(pin, val ? GPIO_TYPE_OUTPUT_1 : GPIO_TYPE_OUTPUT_0);
    gk_gpio_inline_set(bank, pin, val);

    return ret;
}

void gk_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
    struct gk_gpio_bank *bank;
    u32                 pin;

    pin = offset + chip->base;

    bank = gk_gpio_id_to_bank(pin);
    if (bank == NULL)
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pin);
        return;
    }
    gk_gpio_inline_set(bank, pin, val);
}
EXPORT_SYMBOL(gk_gpio_set);

int gk_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
    struct gk_gpio_bank *bank;
    u32                 pin;
    pin = offset + chip->base;

    bank = to_gk_gpio_chip(chip);
    return gk_gpio_insts[bank->index].irq_no;
}

void gk_gpio_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
    int                 i;
    struct gk_gpio_bank *bank;
    u32                 afsel;
    u32                 lmask;
    u32                 data;
    u32                 hmask;
    unsigned long       flags;

    bank = to_gk_gpio_chip(chip);

    spin_lock_irqsave(&bank->lock, flags);
    afsel = gk_gpio_readl(gk_gpio_insts[bank->index].base_bus + REG_GPIO_PER_SEL_OFFSET);
    lmask = gk_gpio_readl(bank->base_reg + REG_GPIO_IE_LOW_OFFSET);
    hmask = gk_gpio_readl(bank->base_reg + REG_GPIO_IE_HIGH_OFFSET);
    data = gk_gpio_readl(gk_gpio_insts[bank->index].base_bus + REG_GPIO_INT_EN_OFFSET);
    spin_unlock_irqrestore(&bank->lock, flags);

    seq_printf(s, "GPIO_BASE:\t0x%08X\n", bank->base_reg);
    seq_printf(s, "GPIO_PSEL:\t0x%08X\n", afsel);
    seq_printf(s, "GPIO_MASK:\t0x%08X:0x%08X\n", hmask, lmask);
    seq_printf(s, "GPIO_GPEN:\t0x%08X\n", data);

    for (i = 0; i < chip->ngpio; i++)
    {
        seq_printf(s, "GPIO %d: HW\n", (chip->base + i));
    }
}

int gk_gpio_set_type(struct gk_gpio_bank* bank, u32 pin, u32 type)
{
    GH_GPIO_OUTPUT_CFG_S out_data;
    GH_GPIO_INPUT_CFG_S  in_data;
    GH_PLL_IOCTRL_GPIO_S io_data;
    out_data.all    = gk_gpio_insts[bank->index].output_cfg[pin];
    switch(GPIO_GET_FUNC(type))
    {
    case GPIO_FUNC_OUT:     // out
        out_data.bitc.out_sel = GPIO_GET_OUT_SEL(type);
        out_data.bitc.oen_sel = GPIO_GET_OEN_SEL(type);
        break;
    case GPIO_FUNC_IN:     // in
        if(GPIO_GET_IN_SEL(type) >= 2)
        {
            out_data.bitc.out_sel   = GPIO_GET_OUT_SEL(type);
            out_data.bitc.oen_sel   = GPIO_GET_OEN_SEL(type);
            in_data.all             = gk_gpio_insts[bank->index].input_cfg[GPIO_GET_IN_SEL(type) - 2];
            in_data.bitc.in_sel     = pin;
            gk_gpio_writel(bank->base_reg + REG_GPIO_INPUT_CFG_OFFSET + ((GPIO_GET_IN_SEL(type) - 2) * 0x4),
                in_data.all);
            gk_gpio_insts[bank->index].input_cfg[GPIO_GET_IN_SEL(type) - 2] = in_data.all;
        }
        else
        {
            out_data.bitc.out_sel = GPIO_GET_OUT_SEL(type);
            out_data.bitc.oen_sel = GPIO_GET_OEN_SEL(type);
        }
        break;
    case GPIO_FUNC_INOUT:     // in+out
        // don't change, otherwise if out_sel at first might output a 0, then change to 1
        in_data.all             = gk_gpio_insts[bank->index].input_cfg[GPIO_GET_IN_SEL(type) - 2];
        in_data.bitc.in_sel     = pin;
        gk_gpio_writel(bank->base_reg + REG_GPIO_INPUT_CFG_OFFSET + ((GPIO_GET_IN_SEL(type) - 2) * 0x4),
            in_data.all);
        gk_gpio_insts[bank->index].input_cfg[GPIO_GET_IN_SEL(type) - 2] = in_data.all;
        out_data.bitc.oen_sel = GPIO_GET_OEN_SEL(type);
        out_data.bitc.out_sel = GPIO_GET_OUT_SEL(type);
        break;
    default:
        return -EINVAL;
    }
    out_data.bitc.oen_invert = GPIO_GET_OEN_INVERT(type);
    out_data.bitc.out_invert = GPIO_GET_OUT_INVERT(type);
    gk_gpio_writel(bank->base_reg + REG_GPIO_OUTPUT_CFG_OFFSET + ((pin - bank->chip.base) * 0x4),
        out_data.all);
    gk_gpio_insts[bank->index].output_cfg[pin] = out_data.all;
    // Pull up/down & 2mA......
#if defined(CONFIG_ARCH_GK710XS)
    if(pin<4)//gpio0-3
    {
        io_data.all = gk_gpio_readl((bank->io_reg + ((((55-pin)-bank->chip.base)/0x04) * 0x04)));
        switch(pin%4)
        {
        case 0:
            io_data.bitc.io2 = GPIO_GET_IOCTRL(type);
            break;
        case 1:
            io_data.bitc.io0 = GPIO_GET_IOCTRL(type);
            break;
        case 2:
            io_data.bitc.io1 = GPIO_GET_IOCTRL(type);
            break;
        case 3:
            io_data.bitc.io3 = GPIO_GET_IOCTRL(type);
            break;
        }
        gk_gpio_writel((bank->io_reg + ((((55-pin)-bank->chip.base)/0x04) * 0x04)), io_data.all);
    }
    else if(pin<52)//gpio4-51
    {
        io_data.all = gk_gpio_readl((bank->io_reg + ((((55-pin)-bank->chip.base)/0x04) * 0x04)));
        switch(pin%4)
        {
        case 0:
            io_data.bitc.io1 = GPIO_GET_IOCTRL(type);
            break;
        case 1:
            io_data.bitc.io2 = GPIO_GET_IOCTRL(type);
            break;
        case 2:
            io_data.bitc.io0 = GPIO_GET_IOCTRL(type);
            break;
        case 3:
            io_data.bitc.io3 = GPIO_GET_IOCTRL(type);
            break;
        }
        gk_gpio_writel((bank->io_reg + ((((55-pin)-bank->chip.base)/0x04) * 0x04)), io_data.all);

    }
    else if(pin<56)//gpio52-55
    {
        io_data.all = gk_gpio_readl((bank->io_reg + ((((55-pin)-bank->chip.base)/0x04) * 0x04)));
        switch(pin%4)
        {
        case 0:
            io_data.bitc.io1 = GPIO_GET_IOCTRL(type);
            break;
        case 1:
            io_data.bitc.io0 = GPIO_GET_IOCTRL(type);
            break;
        case 2:
            io_data.bitc.io2 = GPIO_GET_IOCTRL(type);
            break;
        case 3:
            io_data.bitc.io3 = GPIO_GET_IOCTRL(type);
            break;
        }
        gk_gpio_writel((bank->io_reg + ((((55-pin)-bank->chip.base)/0x04) * 0x04)), io_data.all);
    }
    else//gpio56-62
    {
        io_data.all = gk_gpio_readl((bank->io_reg + (((pin-bank->chip.base)/0x04) * 0x04)));
        switch(pin%4)
        {
        case 0:
            io_data.bitc.io0 = GPIO_GET_IOCTRL(type);
            break;
        case 1:
            io_data.bitc.io1 = GPIO_GET_IOCTRL(type);
            break;
        case 2:
            io_data.bitc.io2 = GPIO_GET_IOCTRL(type);
            break;
        case 3:
            io_data.bitc.io3 = GPIO_GET_IOCTRL(type);
            break;
        }
        gk_gpio_writel((bank->io_reg + (((pin-bank->chip.base)/0x04) * 0x04)), io_data.all);
    }
#else
    io_data.all = gk_gpio_readl((bank->io_reg - (((pin-bank->chip.base)/0x04) * 0x04)));
    switch(pin%4)
    {
    case 0:
        io_data.bitc.io0 = GPIO_GET_IOCTRL(type);
        break;
    case 1:
        io_data.bitc.io1 = GPIO_GET_IOCTRL(type);
        break;
    case 2:
        io_data.bitc.io2 = GPIO_GET_IOCTRL(type);
        break;
    case 3:
        io_data.bitc.io3 = GPIO_GET_IOCTRL(type);
        break;
    }
	gk_gpio_writel((bank->io_reg - (((pin-bank->chip.base)/0x04) * 0x04)), io_data.all);
#endif

    return(0);
}

void gk_gpio_config(u32 pin, u32 func)
{
    if(gk_gpio_func_config((u32)pin, func))
    {
        pr_err("%s: failed to configure GPIO %d for func %d.\n", __func__, pin, func);
    }
}
EXPORT_SYMBOL(gk_gpio_config);

void gk_gpio_raw_lock(u32 pin, unsigned long *pflags)
{
    struct gk_gpio_bank *bank;
    bank = gk_gpio_id_to_bank(pin);
    if (bank == NULL)
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pin);
        return;
    }
    spin_lock_irqsave(&bank->lock, *pflags);
}

void gk_gpio_raw_unlock(u32 pin, unsigned long *pflags)
{
    struct gk_gpio_bank *bank;
    bank = gk_gpio_id_to_bank(pin);
    if (bank == NULL)
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pin);
        return;
    }
    spin_unlock_irqrestore(&bank->lock, *pflags);
}

u32 gk_gpio_suspend(u32 level)
{
    u32             i;
    u32             j;
    unsigned long   flags;

    for (i = 0; i < CONFIG_GK_GPIO_INSTANCES; i++)
    {
        for (j = 0; j < gk_gpio_insts[i].bank_num; j++)
        {
            spin_lock_irqsave(&gk_gpio_insts[i].gpio_bank[j].lock, flags);
            gk_gpio_insts[i].gpio_bank[j].pm_info.isl_reg  =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IS_LOW_OFFSET);
            gk_gpio_insts[i].gpio_bank[j].pm_info.ish_reg  =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IS_HIGH_OFFSET);
            gk_gpio_insts[i].gpio_bank[j].pm_info.ibel_reg =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IBE_LOW_OFFSET);
            gk_gpio_insts[i].gpio_bank[j].pm_info.ibeh_reg =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IBE_HIGH_OFFSET);
            gk_gpio_insts[i].gpio_bank[j].pm_info.ievl_reg =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IEV_LOW_OFFSET);
            gk_gpio_insts[i].gpio_bank[j].pm_info.ievh_reg =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IEV_HIGH_OFFSET);
            gk_gpio_insts[i].gpio_bank[j].pm_info.iel_reg  =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IE_LOW_OFFSET);
            gk_gpio_insts[i].gpio_bank[j].pm_info.ieh_reg  =
                gk_gpio_readl(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IE_HIGH_OFFSET);
            spin_unlock_irqrestore(&gk_gpio_insts[i].gpio_bank[j].lock, flags);
        }
        gk_gpio_insts[i].per_sel_reg      =
            gk_gpio_readl(gk_gpio_insts[i].base_bus + REG_GPIO_PER_SEL_OFFSET);
        gk_gpio_writel(gk_gpio_insts[i].base_bus + REG_GPIO_INT_EN_OFFSET, 0x00000000);
    }

    return 0;
}

u32 gk_gpio_resume(u32 level)
{
    u32             i;
    u32             j;
    unsigned long   flags;

    for (i = 0; i < CONFIG_GK_GPIO_INSTANCES; i++)
    {
        for (j = 0; j < gk_gpio_insts[i].bank_num; j++)
        {
            spin_lock_irqsave(&gk_gpio_insts[i].gpio_bank[j].lock, flags);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IS_LOW_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.isl_reg);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IS_HIGH_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.ish_reg);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IBE_LOW_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.ibel_reg);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IBE_HIGH_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.ibeh_reg);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IEV_LOW_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.ievl_reg);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IEV_HIGH_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.ievh_reg);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IE_LOW_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.iel_reg);
            gk_gpio_writel(gk_gpio_insts[i].gpio_bank[j].base_reg + REG_GPIO_IE_HIGH_OFFSET,
                gk_gpio_insts[i].gpio_bank[j].pm_info.ieh_reg);
            spin_unlock_irqrestore(&gk_gpio_insts[i].gpio_bank[j].lock, flags);
        }
        gk_gpio_writel(gk_gpio_insts[i].base_bus + REG_GPIO_PER_SEL_OFFSET,
            gk_gpio_insts[i].per_sel_reg);
        gk_gpio_writel(gk_gpio_insts[i].base_bus + REG_GPIO_INT_EN_OFFSET, 0x00000001);
    }
    return 0;
}

int gk_set_gpio_output_can_sleep(struct gk_gpio_io_info *pinfo, u32 on, int can_sleep)
{
    if (pinfo == NULL)
    {
        pr_err("%s: pinfo is NULL.\n", __func__);
        return -1;
    }
    if (pinfo->gpio_id < 0 )
    {
        pr_debug("%s: wrong gpio id %d.\n", __func__, pinfo->gpio_id);
        return -1;
    }

    pr_debug("%s: Gpio[%d] %s, level[%s], delay[%dms].\n", __func__,
        pinfo->gpio_id, on ? "ON" : "OFF",
        pinfo->active_level ? "HIGH" : "LOW",
        pinfo->active_delay);
    if (pinfo->gpio_id >= CONFIG_ARCH_NR_GPIO)
    {
        pr_debug("%s: try expander gpio id %d.\n",
            __func__, pinfo->gpio_id);
        return -1;
    }
    else
    {
        if (on)
        {
            gk_gpio_config(pinfo->gpio_id, pinfo->active_level ? GPIO_TYPE_OUTPUT_1 : GPIO_TYPE_OUTPUT_0);
            gk_gpio_set_out(pinfo->gpio_id, pinfo->active_level);
        }
        else
        {
            gk_gpio_config(pinfo->gpio_id, pinfo->active_level ? GPIO_TYPE_OUTPUT_0 : GPIO_TYPE_OUTPUT_1);
            gk_gpio_set_out(pinfo->gpio_id, !pinfo->active_level);
        }
    }
    if (can_sleep)
    {
        msleep(pinfo->active_delay);
    }
    else
    {
        mdelay(pinfo->active_delay);
    }

    return 0;
}
EXPORT_SYMBOL(gk_set_gpio_output_can_sleep);

u32 gk_get_gpio_input_can_sleep(struct gk_gpio_io_info *pinfo, int can_sleep)
{
    u32                 gpio_value = 0;

    if (pinfo == NULL)
    {
        pr_err("%s: pinfo is NULL.\n", __func__);
        goto gk_get_gpio_input_can_sleep_exit;
    }
    if (pinfo->gpio_id < 0 )
    {
        pr_debug("%s: wrong gpio id %d.\n", __func__, pinfo->gpio_id);
        goto gk_get_gpio_input_can_sleep_exit;
    }

    if (pinfo->gpio_id >= CONFIG_ARCH_NR_GPIO)
    {
        pr_debug("%s: try expander gpio id %d.\n",
            __func__, pinfo->gpio_id);
        goto gk_get_gpio_input_can_sleep_exit;
    }
    else
    {
        gk_gpio_config(pinfo->gpio_id, pinfo->active_level ? GPIO_TYPE_OUTPUT_1 : GPIO_TYPE_OUTPUT_0);
        if (can_sleep)
        {
            msleep(pinfo->active_delay);
        }
        else
        {
            mdelay(pinfo->active_delay);
        }
        gpio_value = gk_gpio_get(pinfo->gpio_id);
    }

    pr_debug("%s: {gpio[%d], level[%s], delay[%dms]} get[%d].\n",
        __func__, pinfo->gpio_id,
        pinfo->active_level ? "HIGH" : "LOW",
        pinfo->active_delay, gpio_value);

gk_get_gpio_input_can_sleep_exit:
    return (gpio_value == pinfo->active_level) ? 1 : 0;
}
EXPORT_SYMBOL(gk_get_gpio_input_can_sleep);

int gk_set_gpio_reset_can_sleep(struct gk_gpio_io_info *pinfo, int can_sleep)
{
    int                 ret = 0;

    if (pinfo == NULL)
    {
        pr_err("%s: pinfo is NULL.\n", __func__);
        ret = -1;
        goto gk_set_gpio_reset_can_sleep_exit;
    }
    if (pinfo->gpio_id < 0 )
    {
        pr_debug("%s: wrong gpio id %d.\n", __func__, pinfo->gpio_id);
        ret = -1;
        goto gk_set_gpio_reset_can_sleep_exit;
    }

    pr_debug("%s: Reset gpio[%d], level[%s], delay[%dms].\n",
        __func__, pinfo->gpio_id,
        pinfo->active_level ? "HIGH" : "LOW",
        pinfo->active_delay);
    if (pinfo->gpio_id >= CONFIG_ARCH_NR_GPIO)
    {
        pr_debug("%s: try expander gpio id %d.\n",
            __func__, pinfo->gpio_id);
        ret = -1;
        goto gk_set_gpio_reset_can_sleep_exit;
    }
    else
    {
        gk_gpio_config(pinfo->gpio_id, GPIO_TYPE_OUTPUT_0);
        gk_gpio_set_out(pinfo->gpio_id, pinfo->active_level);
        if (can_sleep)
        {
            msleep(pinfo->active_delay);
        }
        else
        {
            mdelay(pinfo->active_delay);
        }
        gk_gpio_set_out(pinfo->gpio_id, !pinfo->active_level);
        if (can_sleep)
        {
            msleep(pinfo->active_delay);
        }
        else
        {
            mdelay(pinfo->active_delay);
        }
    }

gk_set_gpio_reset_can_sleep_exit:
    return ret;
}
EXPORT_SYMBOL(gk_set_gpio_reset_can_sleep);

int gk_set_gpio_output(struct gk_gpio_io_info *pinfo, u32 on)
{
    return gk_set_gpio_output_can_sleep(pinfo, on, 0);
}
EXPORT_SYMBOL(gk_set_gpio_output);

u32 gk_get_gpio_input(struct gk_gpio_io_info *pinfo)
{
    return gk_get_gpio_input_can_sleep(pinfo, 0);
}
EXPORT_SYMBOL(gk_get_gpio_input);

int gk_set_gpio_reset(struct gk_gpio_io_info *pinfo)
{
    return gk_set_gpio_reset_can_sleep(pinfo, 0);
}
EXPORT_SYMBOL(gk_set_gpio_reset);

int gk_is_valid_gpio_irq(struct gk_gpio_irq_info *pinfo)
{
    struct gk_gpio_inst     *gpio_inst;
    struct gk_gpio_bank     *gpio_bank;
    if (pinfo == NULL)
    {
        printk("%s: pinfo is NULL.\n", __func__);
        return 0;
    }

    gpio_inst = gk_gpio_id_to_inst(pinfo->pin);
    gpio_bank = gk_gpio_id_to_bank(pinfo->pin);
    if ((gpio_bank == NULL) || (gpio_inst == NULL))
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pinfo->pin);
        return 0;
    }

    if ((pinfo->type != IRQ_TYPE_EDGE_RISING) &&
        (pinfo->type != IRQ_TYPE_EDGE_FALLING) &&
        (pinfo->type != IRQ_TYPE_EDGE_BOTH) &&
        (pinfo->type != IRQ_TYPE_LEVEL_HIGH) &&
        (pinfo->type != IRQ_TYPE_LEVEL_LOW))
        return 0;

    return 1;
}
EXPORT_SYMBOL(gk_is_valid_gpio_irq);

static irqreturn_t gk_gpio_irq(int irq, void *dev)
{
    struct gk_gpio_inst     *gpio_inst;
    struct gk_gpio_bank     *gpio_bank;
    struct gk_gpio_irq_info *pinfo = (struct gk_gpio_irq_info*)dev;
    unsigned long   flags;
    u32 misl_reg;
    u32 mish_reg;
    u32 index;
    u32 mask = 1;
    u32 i = 1;
    mdelay(20);

    if (pinfo == NULL)
    {
        pr_err("%s: pinfo is NULL.\n", __func__);
        return IRQ_NONE;
    }
    gpio_inst = gk_gpio_id_to_inst(pinfo->pin);
    if (gpio_inst == NULL)
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pinfo->pin);
        return IRQ_NONE;
    }
    for(i=0;i<gpio_inst->bank_num;i++)
    {
        gpio_bank = &gpio_inst->gpio_bank[i];
        spin_lock_irqsave(&gpio_bank->lock, flags);
        misl_reg    = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_MIS_LOW_OFFSET);
        mish_reg    = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_MIS_HIGH_OFFSET);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IC_LOW_OFFSET, misl_reg);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IC_HIGH_OFFSET, mish_reg);
        spin_unlock_irqrestore(&gpio_bank->lock, flags);

        for(index=0; index < 32; index++)
        {
            if(misl_reg & mask)
            {
                pinfo[index+gpio_bank->chip.base].val = gk_gpio_inline_get(gpio_bank, index+gpio_bank->chip.base);
                if (pinfo[index+gpio_bank->chip.base].handler)
                {
                    pinfo[index+gpio_bank->chip.base].handler(irq, &pinfo[index+gpio_bank->chip.base]);
                }
            }
            if(mish_reg & mask)
            {
                pinfo[index + gpio_bank->chip.base + 32].val = gk_gpio_inline_get(gpio_bank, index+gpio_bank->chip.base + 32);
                if (pinfo[index + gpio_bank->chip.base + 32].handler)
                {
                    pinfo[index + gpio_bank->chip.base + 32].handler(irq, &pinfo[index + gpio_bank->chip.base + 32]);
                }
            }
            mask <<= 1;
        }
    }
    return IRQ_HANDLED;
}

int gk_gpio_request_irq(struct gk_gpio_irq_info *pinfo)
{
    struct gk_gpio_inst     *gpio_inst;
    struct gk_gpio_bank     *gpio_bank;
    u32 mask;
    u32 offset;
    unsigned long   flags;
    if (pinfo == NULL)
    {
        pr_err("%s: pinfo is NULL.\n", __func__);
        return -1;
    }

    gpio_inst = gk_gpio_id_to_inst(pinfo->pin);
    gpio_bank = gk_gpio_id_to_bank(pinfo->pin);
    if ((gpio_bank == NULL) || (gpio_inst == NULL))
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pinfo->pin);
        return -1;
    }
    spin_lock_irqsave(&gpio_bank->lock, flags);
    memcpy(&gpio_inst->irq_info[pinfo->pin], pinfo, sizeof(struct gk_gpio_irq_info));
    gk_gpio_writel(gpio_inst->base_bus + REG_GPIO_INT_EN_OFFSET, 0x00);
    mask = 0x01<<(pinfo->pin % 0x20);
    offset = pinfo->pin - gpio_bank->chip.base;
    if(offset / 0x20)
    {
        gpio_bank->pm_info.ish_reg        = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IS_HIGH_OFFSET);
        gpio_bank->pm_info.ibeh_reg       = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IBE_HIGH_OFFSET);
        gpio_bank->pm_info.ievh_reg       = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IEV_HIGH_OFFSET);
        gpio_bank->pm_info.ieh_reg        = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IE_HIGH_OFFSET);

        gpio_bank->pm_info.ieh_reg       |= mask;
        switch(pinfo->type)
        {
        case IRQ_TYPE_LEVEL_LOW:
            gpio_bank->pm_info.ish_reg        |= mask;
            gpio_bank->pm_info.ibeh_reg       &= ~mask;
            gpio_bank->pm_info.ievh_reg       &= ~mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_1);
            break;
        case IRQ_TYPE_LEVEL_HIGH:
            gpio_bank->pm_info.ish_reg        |= mask;
            gpio_bank->pm_info.ibeh_reg       &= ~mask;
            gpio_bank->pm_info.ievh_reg       |= mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_0);
            break;
        case IRQ_TYPE_EDGE_FALLING:
            gpio_bank->pm_info.ish_reg        &= ~mask;
            gpio_bank->pm_info.ibeh_reg       &= ~mask;
            gpio_bank->pm_info.ievh_reg       &= ~mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_1);
            break;
        case IRQ_TYPE_EDGE_RISING:
            gpio_bank->pm_info.ish_reg        &= ~mask;
            gpio_bank->pm_info.ibeh_reg       &= ~mask;
            gpio_bank->pm_info.ievh_reg       |= mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_0);
            break;
        case IRQ_TYPE_EDGE_BOTH:
            gpio_bank->pm_info.ish_reg        &= ~mask;
            gpio_bank->pm_info.ibeh_reg       |= mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_1);
            break;
        }
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IS_HIGH_OFFSET, gpio_bank->pm_info.ish_reg);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IBE_HIGH_OFFSET, gpio_bank->pm_info.ibeh_reg);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IEV_HIGH_OFFSET, gpio_bank->pm_info.ievh_reg);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IE_HIGH_OFFSET, gpio_bank->pm_info.ieh_reg);
    }
    else
    {
        gpio_bank->pm_info.isl_reg        = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IS_LOW_OFFSET);
        gpio_bank->pm_info.ibel_reg       = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IBE_LOW_OFFSET);
        gpio_bank->pm_info.ievl_reg       = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IEV_LOW_OFFSET);
        gpio_bank->pm_info.iel_reg        = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IE_LOW_OFFSET);

        gpio_bank->pm_info.iel_reg       |= mask;
        switch(pinfo->type)
        {
        case IRQ_TYPE_LEVEL_LOW:
            gpio_bank->pm_info.isl_reg        |= mask;
            gpio_bank->pm_info.ibel_reg       &= ~mask;
            gpio_bank->pm_info.ievl_reg       &= ~mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_1);
            break;
        case IRQ_TYPE_LEVEL_HIGH:
            gpio_bank->pm_info.isl_reg        |= mask;
            gpio_bank->pm_info.ibel_reg       &= ~mask;
            gpio_bank->pm_info.ievl_reg       |= mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_0);
            break;
        case IRQ_TYPE_EDGE_FALLING:
            gpio_bank->pm_info.isl_reg        &= ~mask;
            gpio_bank->pm_info.ibel_reg       &= ~mask;
            gpio_bank->pm_info.ievl_reg       &= ~mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_1);
            break;
        case IRQ_TYPE_EDGE_RISING:
            gpio_bank->pm_info.isl_reg        &= ~mask;
            gpio_bank->pm_info.ibel_reg       &= ~mask;
            gpio_bank->pm_info.ievl_reg       |= mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_0);
            break;
        case IRQ_TYPE_EDGE_BOTH:
            gpio_bank->pm_info.isl_reg        &= ~mask;
            gpio_bank->pm_info.ibel_reg       |= mask;
            gk_gpio_set_type(gpio_bank, pinfo->pin, GPIO_TYPE_INPUT_1);
            break;
        }
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IS_LOW_OFFSET, gpio_bank->pm_info.isl_reg);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IBE_LOW_OFFSET, gpio_bank->pm_info.ibel_reg);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IEV_LOW_OFFSET, gpio_bank->pm_info.ievl_reg);
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IE_LOW_OFFSET, gpio_bank->pm_info.iel_reg);
    }
    __set_bit(offset, (volatile unsigned long *)gpio_inst->irq_flag);
    if(gpio_inst->irq_now == 0x00)
    {
        request_irq(gpio_inst->irq_no, gk_gpio_irq, IRQF_TRIGGER_HIGH, "gpio_irq", (void*)(&gpio_inst->irq_info[0]));
        gpio_inst->irq_now = 0x01;
        gk_gpio_writel(gpio_inst->base_bus + REG_GPIO_INT_EN_OFFSET, 0x01);
    }
    spin_unlock_irqrestore(&gpio_bank->lock, flags);
    return 0;
}
EXPORT_SYMBOL(gk_gpio_request_irq);

int gk_gpio_release_irq(u32 pin)
{
    struct gk_gpio_inst     *gpio_inst;
    struct gk_gpio_bank     *gpio_bank;
    u32 mask, i;
    u32 offset;
    unsigned long   flags;

    gpio_inst = gk_gpio_id_to_inst(pin);
    gpio_bank = gk_gpio_id_to_bank(pin);
    if ((gpio_bank == NULL) || (gpio_inst == NULL))
    {
        pr_err("%s: invalid GPIO %d.\n", __func__, pin);
        return -1;
    }
    spin_lock_irqsave(&gpio_bank->lock, flags);
    memset(&gpio_inst->irq_info[pin], 0x00, sizeof(struct gk_gpio_irq_info));

    offset = pin - gpio_bank->chip.base;
    mask = 0x01<<(offset % 0x20);
    if(offset / 0x20)
    {
        gpio_bank->pm_info.ieh_reg        = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IE_HIGH_OFFSET);
        gpio_bank->pm_info.ieh_reg       &= ~mask;
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IE_HIGH_OFFSET, gpio_bank->pm_info.ieh_reg);
    }
    else
    {
        gpio_bank->pm_info.iel_reg        = gk_gpio_readl(gpio_bank->base_reg + REG_GPIO_IE_LOW_OFFSET);
        gpio_bank->pm_info.iel_reg       &= ~mask;
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_IE_LOW_OFFSET, gpio_bank->pm_info.iel_reg);
    }
    __clear_bit(offset, (volatile unsigned long *)gpio_inst->irq_flag);
    offset = 0;
    for (i = 0; i < (sizeof(gpio_inst->irq_flag) /sizeof(gpio_inst->irq_flag[0])); i++)
    {
        if(gpio_inst->irq_flag[i] != 0)
        {
            offset = 1;
            break;
        }
    }
    if(!offset)
    {
        gk_gpio_writel(gpio_bank->base_reg + REG_GPIO_INT_EN_OFFSET, 0x00);
    }
    spin_unlock_irqrestore(&gpio_bank->lock, flags);
    return 0;
}
EXPORT_SYMBOL(gk_gpio_release_irq);

int __init goke_init_gpio(void)
{
    u32 i,j,ret=0;
    for(i=0;i<CONFIG_GK_GPIO_INSTANCES;i++)
    {
        mutex_lock(&gk_gpio_mtx);
        memset(gk_gpio_insts[i].gpio_valid, 0xff, sizeof(gk_gpio_insts[i].gpio_valid));
        memset(gk_gpio_insts[i].gpio_freeflag, 0xff, sizeof(gk_gpio_insts[i].gpio_freeflag));
        /* clear unused gpio */
        for (j = CONFIG_ARCH_NR_GPIO; j < (BITS_TO_LONGS(CONFIG_ARCH_NR_GPIO) * 0x20); j++)
        {
            __clear_bit(j, (volatile unsigned long *)gk_gpio_insts[i].gpio_valid);
            __clear_bit(j, (volatile unsigned long *)gk_gpio_insts[i].gpio_freeflag);
        }
        mutex_unlock(&gk_gpio_mtx);
        for (j = 0; j < gk_gpio_insts[i].bank_num; j++)
        {
            spin_lock_init(&gk_gpio_insts[i].gpio_bank[j].lock);
            ret = gpiochip_add(&gk_gpio_insts[i].gpio_bank[j].chip);
            if (ret)
            {
                pr_err("%s: gpiochip_add %s fail %d.\n", __func__,
                    gk_gpio_insts[i].gpio_bank[j].chip.label, ret);
                break;
            }
        }
    }
    return ret;
}

int gk_set_ircut(u32 mode)
{
    // A=SYSTEM_GPIO_IR_CUT2
    // B=SYSTEM_GPIO_IR_CUT1
    if(mode==0) // clear A clear B
    {
        gk_gpio_set_out(gk_all_gpio_cfg.ir_cut1, 0);
        gk_gpio_set_out(gk_all_gpio_cfg.ir_cut2, 0);
    }
    else if(mode==1) // night set A clear B
    {
        gk_gpio_set_out(gk_all_gpio_cfg.ir_cut1, 0);
        gk_gpio_set_out(gk_all_gpio_cfg.ir_cut2, 1);
    }
    else if(mode==2) // day clear A set B
    {
        gk_gpio_set_out(gk_all_gpio_cfg.ir_cut1, 1);
        gk_gpio_set_out(gk_all_gpio_cfg.ir_cut2, 0);
    }
    else
    {
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(gk_set_ircut);

unsigned int gk_board_version(void)
{
    return gk_all_gpio_cfg.board_version;
}
EXPORT_SYMBOL(gk_board_version);

const char *gk_board_type(void)
{
    return (const char *)gk_all_gpio_cfg.board_type;
}
EXPORT_SYMBOL(gk_board_type);



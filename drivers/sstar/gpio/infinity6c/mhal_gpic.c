/*
 * mhal_gpic.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <gpio.h>
#include <gpi-irqs.h>
#include "mhal_gpic.h"
#include "mhal_gpio.h"

#define GPIC_CTRL_OFFSET  0x00
#define GPIC_ENABLE_MASK  0x0001
#define GPIC_DIR_MASK     0x0002
#define GPIC_UP_DOWN_MASK 0x0004
#define GPIC_SEL_MASK     0x7F00
#define GPIC_SEL_SHIRT    0x08

#define GPIC_INIT_OFFSET   0x01
#define GPIC_TARGET_OFFSET 0x02
#define GPIC_COUNT_OFFSET  0x03

#define READ_WORD(_reg)        (*(volatile u16*)(_reg))
#define WRITE_WORD(_reg, _val) (*((volatile u16*)(_reg))) = (u16)(_val)
#define WRITE_WORD_MASK(_reg, _val, _mask) \
    (*((volatile u16*)(_reg))) = ((*((volatile u16*)(_reg))) & ~(_mask)) | ((u16)(_val) & (_mask))

#define GPIC_READ(_reg_)                    READ_WORD(hal->base + ((_reg_) << 2))
#define GPIC_WRITE(_reg_, _val_)            WRITE_WORD(hal->base + ((_reg_) << 2), (_val_))
#define GPIC_WRITE_MASK(_reg_, _val_, mask) WRITE_WORD_MASK(hal->base + ((_reg_) << 2), (_val_), (mask))

int MHal_GPIC_Enable(struct hal_gpic_t* hal)
{
    int pad_sel;

    if (!hal->base)
        return -1;

    pad_sel = GPI_GPIO_To_Irq(hal->pad);
    if (pad_sel < 0)
        return -1;

    GPIC_WRITE_MASK(GPIC_CTRL_OFFSET, pad_sel << GPIC_SEL_SHIRT, GPIC_SEL_MASK);
    if (hal->edge)
        GPIC_WRITE_MASK(GPIC_CTRL_OFFSET, GPIC_DIR_MASK, GPIC_DIR_MASK);
    else
        GPIC_WRITE_MASK(GPIC_CTRL_OFFSET, 0, GPIC_DIR_MASK);
    if (hal->dir)
        GPIC_WRITE_MASK(GPIC_CTRL_OFFSET, GPIC_UP_DOWN_MASK, GPIC_UP_DOWN_MASK);
    else
        GPIC_WRITE_MASK(GPIC_CTRL_OFFSET, 0, GPIC_UP_DOWN_MASK);
    GPIC_WRITE(GPIC_INIT_OFFSET, hal->init);
    GPIC_WRITE(GPIC_TARGET_OFFSET, hal->target);
    GPIC_WRITE_MASK(GPIC_CTRL_OFFSET, GPIC_ENABLE_MASK, GPIC_ENABLE_MASK);
    hal->enable = 1;

    return 0;
}
int MHal_GPIC_Disable(struct hal_gpic_t* hal)
{
    if (!hal->base)
        return -1;
    GPIC_WRITE_MASK(GPIC_CTRL_OFFSET, 0, GPIC_ENABLE_MASK);
    hal->enable = 0;

    return 0;
}
int MHal_GPIC_GetCount(struct hal_gpic_t* hal, U16* count)
{
    if (!hal->base)
        return -1;

    *count = GPIC_READ(GPIC_COUNT_OFFSET);

    return 0;
}

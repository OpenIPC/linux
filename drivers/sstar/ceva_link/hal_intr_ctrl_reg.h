#ifndef HAL_INTR_CTRL_1_REG_H
#define HAL_INTR_CTRL_1_REG_H

#include <linux/kernel.h>

typedef struct
{
    union
    {
        struct
        {
            u16 reg_hst1_fiq_force_47_32:16;
        };
        u16 reg22;
    };

} int_ctl_hal_reg;
#endif
#endif //HAL_INTR_CTRL_1_REG_H
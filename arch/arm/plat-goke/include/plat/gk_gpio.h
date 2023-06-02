/*!
*****************************************************************************
** \file        arch/arm/plat-goke/include/plat/gk_gpio.h
**
** \version     $Id: gk_gpio.h 9588 2016-08-16 01:55:35Z dengbiao $
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2016 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#ifndef _GK_GPIO_H_
#define _GK_GPIO_H_
#include <asm-generic/gpio.h>
#include <mach/irqs.h>



//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************
#define GK_GPIO(n)          (n)
#define GPIO_BANK_SIZE      64

/* SW definitions */
#define GPIO_HIGH           1
#define GPIO_LOW            0

#define GK_GPIO_IO_MODULE_PARAM_CALL(name_prefix, arg, perm) \
    module_param_cb(name_prefix##gpio_id, &param_ops_int, &(arg.gpio_id), perm); \
    module_param_cb(name_prefix##active_level, &param_ops_int, &(arg.active_level), perm); \
    module_param_cb(name_prefix##active_delay, &param_ops_int, &(arg.active_delay), perm)
#define GK_GPIO_RESET_MODULE_PARAM_CALL(name_prefix, arg, perm) \
    module_param_cb(name_prefix##gpio_id, &param_ops_int, &(arg.gpio_id), perm); \
    module_param_cb(name_prefix##active_level, &param_ops_int, &(arg.active_level), perm); \
    module_param_cb(name_prefix##active_delay, &param_ops_int, &(arg.active_delay), perm)

/* GPIO function selection */
/* Select SW or HW control and input/output direction of S/W function */
#define GPIO_FUNC_SW_INPUT      0
#define GPIO_FUNC_SW_OUTPUT     1
#define GPIO_FUNC_HW            2

//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************
/* GPIO function selection */
typedef enum
{
    GPIO_FUNC_IN     = 1,
    GPIO_FUNC_OUT    = 2,
    GPIO_FUNC_INOUT  = 3,
}GPIO_FUNC_TYPE_E;



//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************
struct gk_gpio_irq_info
{
    int pin;
    int type;
    int val;    // when is input, the value will be transfered to handler
    irq_handler_t handler;
};
struct gk_gpio_io_info
{
    int gpio_id;
    int active_level;
    int active_delay;       //ms
};

typedef struct
{
    u32     pin;
    u32     type;
}GPIO_XREF_S;

struct gk_gpio_bank
{
    struct gpio_chip    chip;
    spinlock_t          lock;
    u32                 base_reg;   // for is/ibe/iev/ie/ic/ris/mis/din/
    u32                 io_reg;     // PLL_IOCTRL
    u32                 index;      // instance no
    struct
    {
        u32 isl_reg;
        u32 ish_reg;
        u32 ibel_reg;
        u32 ibeh_reg;
        u32 ievl_reg;
        u32 ievh_reg;
        u32 iel_reg;
        u32 ieh_reg;
    }pm_info;
};

struct gk_gpio_inst
{
    u32                     bank_num;
    struct  gk_gpio_bank*   gpio_bank;
    u32                     output_cfg[CONFIG_ARCH_NR_GPIO];            // CONFIG_GK_GPIO_MAX_OUTPUT_TYPE != CONFIG_ARCH_NR_GPIO
    u32                     input_cfg[CONFIG_GK_GPIO_MAX_INPUT_TYPE];   // CONFIG_GK_GPIO_MAX_INPUT_TYPE  != CONFIG_ARCH_NR_GPIO
    u32                     irq_no;
    struct gk_gpio_irq_info irq_info[CONFIG_ARCH_NR_GPIO];
    u32                     irq_now;
    u32                     gpio_valid[BITS_TO_LONGS(CONFIG_ARCH_NR_GPIO)];
    u32                     gpio_freeflag[BITS_TO_LONGS(CONFIG_ARCH_NR_GPIO)];
    u32                     irq_flag[BITS_TO_LONGS(CONFIG_ARCH_NR_GPIO)];
    u32                     base_bus;   // for sel/in
    u32                     per_sel_reg;
};

#define GK_GPIO_BANK(name, reg_base, io_base, base_gpio, gpio_num, bank)    \
{                                                       \
    .chip =                                             \
    {                                                   \
        .label              = name,                     \
        .owner              = THIS_MODULE,              \
        .request            = gk_gpio_request,          \
        .free               = gk_gpio_free,             \
        .direction_input    = gk_gpio_direction_input,  \
        .get                = gk_gpio_get_ex,              \
        .direction_output   = gk_gpio_direction_output, \
        .set                = gk_gpio_set,              \
        .to_irq             = gk_gpio_to_irq,           \
        .dbg_show           = gk_gpio_dbg_show,         \
        .base               = base_gpio,                \
        .ngpio              = gpio_num,                 \
        .can_sleep          = 0,                        \
        .exported           = 0,                        \
    },                                                  \
    .base_reg               = reg_base,                 \
    .io_reg                 = io_base,                  \
    .index                  = bank,                     \
    .pm_info                = {0},                      \
}


//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
#ifdef CONFIG_GK_GPIO_V1_00
#include <plat/gk_gpio_v1_00.h>
#endif
#ifdef CONFIG_GK_GPIO_V1_10
#include <plat/gk_gpio_v1_10.h>
#endif
#ifdef CONFIG_GK_GPIO_V1_20
#include <plat/gk_gpio_v1_20.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern int __init gk_init_gpio(void);
extern int gk_gpio_set_type(struct gk_gpio_bank* bank, u32 pin, u32 type); // GPIO_TYPE_E
int gk_gpio_request(struct gpio_chip *chip, u32 pin);
void gk_gpio_free(struct gpio_chip *chip, u32 pin);
int gk_gpio_direction_input(struct gpio_chip *chip, u32 pin, int val);
int gk_gpio_get(unsigned offset);
int gk_gpio_get_ex(struct gpio_chip *chip, unsigned offset);
int gk_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int val);
void gk_gpio_set(struct gpio_chip *chip, unsigned offset, int val);
int gk_gpio_to_irq(struct gpio_chip *chip, unsigned offset);
void gk_gpio_dbg_show(struct seq_file *s, struct gpio_chip *chip);
int gk_is_valid_gpio_irq(struct gk_gpio_irq_info *pinfo);
extern void gk_gpio_set_out(u32 id, u32 value);

int gk_gpio_func_config(u32 pin, u32 func);

extern int gk_set_gpio_output(struct gk_gpio_io_info *pinfo, u32 on);
extern u32 gk_get_gpio_input(struct gk_gpio_io_info *pinfo);
extern int gk_set_gpio_reset(struct gk_gpio_io_info *pinfo);
extern int gk_set_gpio_output_can_sleep(struct gk_gpio_io_info *pinfo, u32 on, int can_sleep);
extern u32 gk_get_gpio_input_can_sleep(struct gk_gpio_io_info *pinfo, int can_sleep);
extern int gk_set_gpio_reset_can_sleep(struct gk_gpio_io_info *pinfo, int can_sleep);
extern void gk_gpio_config(u32 pin, u32 func);


#define GK_GPIO_IRQ_MODULE_PARAM_CALL(name_prefix, arg, perm) \
    module_param_cb(name_prefix##pin, &param_ops_int, &(arg.pin), perm); \
    module_param_cb(name_prefix##type, &param_ops_int, &(arg.type), perm); \
    module_param_cb(name_prefix##val, &param_ops_int, &(arg.val), perm);

#define GK_IRQ_MODULE_PARAM_CALL(name_prefix, arg, perm) \
    module_param_cb(name_prefix##irq_gpio, &param_ops_int, &(arg.irq_gpio), perm); \
    module_param_cb(name_prefix##irq_line, &param_ops_int, &(arg.irq_line), perm); \
    module_param_cb(name_prefix##irq_type, &param_ops_int, &(arg.irq_type), perm); \
    module_param_cb(name_prefix##irq_gpio_val, &param_ops_int, &(arg.irq_gpio_val), perm); \
    module_param_cb(name_prefix##irq_gpio_mode, &param_ops_int, &(arg.irq_gpio_mode), perm)
extern int gk_is_valid_gpio_irq(struct gk_gpio_irq_info *pgpio_irq);
extern int gk_gpio_request_irq(struct gk_gpio_irq_info *pinfo);
extern int gk_gpio_release_irq(u32 pin);
int __init goke_init_gpio(void);
void GH_GPIO_set_INPUT_CFG_in_sel(u8 index, u8 data);

#ifdef __cplusplus
}
#endif



#endif /* _GK_GPIO_H_ */


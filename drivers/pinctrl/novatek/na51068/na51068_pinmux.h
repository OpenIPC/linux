#ifndef __PINMUX_DRV_H__
#define __PINMUX_DRV_H__
#include <plat/top.h>
#include <plat/top_reg.h>
#include <mach/rcw_macro.h>
#include <plat/pad.h>

typedef enum {
	PINMUX_FUNC_ID_LCD2,
	PINMUX_FUNC_ID_COUNT,                           //< Total function count
} PINMUX_FUNC_ID;

struct nvt_pad_info {
	u32 pad_ds_pin;
	u32 driving;
	u32 pad_gpio_pin;
	u32 direction;
};

struct nvt_gpio_info {
	u32 gpio_pin;
	u32 direction;
};

struct nvt_pinctrl_info {
	void __iomem *top_base;
	void __iomem *pad_base;
	void __iomem *gpio_base;
	PIN_GROUP_CONFIG top_pinmux[PIN_FUNC_MAX];
	struct nvt_pad_info pad[PAD_PIN_MAX];
};

#define MAX_MODULE_NAME 9

void pad_preset(struct nvt_pinctrl_info *info);
ER pad_init(struct nvt_pinctrl_info *info, u32 nr_pad);

void gpio_init(struct nvt_gpio_info *gpio, int nr_gpio, struct nvt_pinctrl_info *info);

void pinmux_gpio_parsing(struct nvt_pinctrl_info *info);
void pinmux_parsing(struct nvt_pinctrl_info *info);
void pinmux_preset(struct nvt_pinctrl_info *info);
ER pinmux_init(struct nvt_pinctrl_info *info);

int nvt_pinmux_proc_init(void);

int pinmux_set_config(PINMUX_FUNC_ID id, u32 pinmux);
static inline int pinmux_set_host(struct nvt_pinctrl_info *info, PINMUX_FUNC_ID id, u32 pinmux) {return 0;};

#endif


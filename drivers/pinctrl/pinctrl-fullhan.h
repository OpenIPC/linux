/*
 * Core driver for Fullhan pin controller
 *
 * Copyright (C) 2018 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * Author: Wang Yuliang <wangyl307@fullhan.com>
 *
 */

#ifndef __PINCTRL_FULLHAN_H
#define __PINCTRL_FULLHAN_H

#define DEVICE_NAME					"fh_pinctrl"
#define FH_PINCTRL_PROC_FILE "driver/pinctrl"

struct platform_device;


/**
 * struct fh_pmx_func - describes pinmux functions
 * @name: the name of this specific function
 * @groups: corresponding pin groups
 * @num_groups: the number of groups
 */
struct fh_pmx_func {
	const char *name;
	const char **groups;
	unsigned num_groups;
};


typedef union {
	struct {
		unsigned int sl		: 1;	/*0*/
		unsigned int		: 3;	/*1~3*/
		unsigned int ds		: 3;	/*4~6*/
		unsigned int msc	: 1;	/*7*/
		unsigned int st		: 1;	/*8*/
		unsigned int        : 3;	/*9~11*/
		unsigned int ie     : 1;    /*12*/
		unsigned int        : 3;    /*13~15*/
		unsigned int pd     : 1;    /*16*/
		unsigned int        : 3;    /*17~19*/
		unsigned int pu     : 1;    /*20*/
		unsigned int        : 3;	/*21~23*/
		unsigned int mfs    : 4;    /*24~27*/
		unsigned int oe     : 1;    /*28*/
		unsigned int        : 3;    /*29~31*/
    } bit;
    unsigned int dw;
} fh_pin_reg;

struct fh_pinpad {
	int index;
	const char *name;
	int nfunc;
	int func_sel;
	const char **funcs;
	fh_pin_reg config;	/* config[reg'val] of read from reg or write to reg */
	fh_pin_reg *reg;	/* real reg data, vaddr --> paddr: 0xf000000xx */
};

struct fh_pinmux {
	const char *name;
	unsigned int mux_id;
	unsigned int select;
	int ds;
	unsigned int npads;
	struct fh_pinpad **pads;
};

struct fh_pin {
	const char *name;
	unsigned int mux_sel;
	fh_pin_reg config; /* the corresponding pinpad'config */
	struct fh_pinmux *mux;
};

struct fh_pingroup {
	const char *name;
	unsigned npins;
	unsigned *pin_ids;
	struct fh_pin *pins;
};

struct fh_pinctrl_soc_info {
	struct device *dev;
	void *pbase;
	void *vbase;
	struct pinctrl_pin_desc *pins;
	unsigned int npins;
	unsigned int nmuxs;
	unsigned int max_mux;
	fh_pin_reg **pin_regs;
	struct fh_pinpad *pin_pads;
	struct fh_pinmux *pin_muxs;
	struct fh_pingroup *pin_groups;
	unsigned int ngroups;
	struct fh_pmx_func *functions;
	unsigned int nfunctions;
	unsigned int flags;
	unsigned int pupd_enable;
	unsigned int pupd_disable;
	unsigned int pupd_zero;
};

/**
 * @dev: a pointer back to containing device
 * @base: the offset to the controller in virtual memory
 */
struct fh_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctl;
	const struct fh_pinctrl_soc_info *info;
};

#define NEED_CHECK_PINLIST  (1)
#define PIN_BACKUP			(1<<1)
#define PIN_RESTORE			(1<<2)

#define PUPD_NONE           (0)
#define PUPD_UP             (1)
#define PUPD_DOWN           (2)

#define MAX_NAME_CHARS		(32)
#define PINCONFIG_MASK		(0x0FFFFFFF)

#ifndef SET_REG
#define SET_REG(addr, val)	writel(val, (void __iomem *)addr)
#endif
#ifndef GET_REG
#define GET_REG(addr)		readl((void __iomem *)addr)
#endif


#define PINCTRL_PARSE_PROPERTY_U32(np, pp, val) \
	if (of_property_read_u32(np, pp, &val)) { \
		dev_err(info->dev, "error when read property "pp" %s in L%d", \
				np->full_name, __LINE__); \
		return -EINVAL; \
	}

#define PINCTRL_DEVM_KZALLOC_ARRAY(dev, ptr, count) \
	if ((int)count <= 0) \
		dev_err(dev, "error "#count" : %d in L%d", count, __LINE__); \
	ptr = devm_kzalloc(dev, sizeof(*ptr) * count, GFP_KERNEL); \
	if (!ptr) \
		return -ENOMEM;

#define PINCTRL_DEVM_KZALLOC(dev, ptr)	PINCTRL_DEVM_KZALLOC_ARRAY(dev, ptr, 1)

#define PINCTRL_DEVM_KFREE(dev, ptr)	devm_kfree(dev, ptr)

// #define PINCTRL_DEBUG
#if defined PINCTRL_DEBUG
#undef dev_dbg
#define dev_dbg dev_info
#define DBG_LINE_ARGS(fmt, ...) printk(KERN_CRIT"%s L%d: "fmt"\n", \
				 __func__, __LINE__, __VA_ARGS__)
#define DBG_LINE() printk(KERN_CRIT"%s L%d\n", __func__, __LINE__)
#else
#define DBG_LINE_ARGS(fmt, ...)
#define DBG_LINE()
#endif

#endif /* __PINCTRL_FULLHAN_H */

/*
 * JZSOC GPIO port, usually used in arch code.
 *
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __JZSOC_JZ_GPIO_H__
#define __JZSOC_JZ_GPIO_H__

enum gpio_function {
	GPIO_FUNC_0	= 0x00,  //0000, GPIO as function 0 / device 0
	GPIO_FUNC_1	= 0x01,  //0001, GPIO as function 1 / device 1
	GPIO_FUNC_2	= 0x02,  //0010, GPIO as function 2 / device 2
	GPIO_FUNC_3	= 0x03,  //0011, GPIO as function 3 / device 3
	GPIO_OUTPUT0	= 0x04,  //0100, GPIO output low  level
	GPIO_OUTPUT1	= 0x05,  //0101, GPIO output high level
	GPIO_INPUT	= 0x06,  //0110, GPIO as input.7 also.
	GPIO_INT_LO	= 0x08,  //1000, Low  Level trigger interrupt
	GPIO_INT_HI	= 0x09,  //1001, High Level trigger interrupt
	GPIO_INT_FE	= 0x0a,  //1010, Fall Edge trigger interrupt
	GPIO_INT_RE	= 0x0b,  //1011, Rise Edge trigger interrupt
	GPIO_PULL_HIZ = 0x80,//UP and DOWN Disable
	GPIO_PULL_UP  = 0x90,
	GPIO_PULL_DOWN = 0xa0,
	GPIO_PULL_BUSHOLD = 0xb0,
    GPIO_PULL_DOWN_DIS = 0xc0,
	GPIO_INPUT_PULL_HI   = 0x96
};
#define GPIO_AS_FUNC(func)  (! ((func) & 0xc))

enum gpio_port {
	GPIO_PORT_A = 0, GPIO_PORT_B,
	GPIO_PORT_C,
	/* this must be last */
	GPIO_NR_PORTS,
};

struct jz_gpio_func_def {
	char *name;
	int port;
	int func;
	unsigned long pins;
};

typedef enum {
	DS_4_MA = 0,
	DS_8_MA,
	DS_12_MA,
} gpio_drv_level_t;

#ifndef GPIO_PG
#define GPIO_PG(n)      (5*32 + 23 + n)
#endif

/* PHY hard reset */
struct jz_gpio_phy_reset {
	enum gpio_port		port;
	unsigned short		pin;
	enum gpio_function	start_func;
	enum gpio_function	end_func;
	unsigned int		delaytime_usec;
};

/*
 * must define this array in board special file.
 * define the gpio pins in this array, use GPIO_DEF_END
 * as end of array. it will inited in function
 * setup_gpio_pins()
 */

extern struct jz_gpio_func_def platform_devio_array[];
extern int platform_devio_array_size;

struct gpio_reg_func {
	unsigned int save[6];
};

/*
 * This functions are used in special driver which need
 * operate the device IO pin.
 */
int jzgpio_set_func(enum gpio_port port,
		    enum gpio_function func,unsigned long pins);

int jz_gpio_set_func(int gpio, enum gpio_function func);

int jzgpio_ctrl_pull(enum gpio_port port, int enable_pull,
		     unsigned long pins);
void jz_gpio_set_drive_strength(int gpio, gpio_drv_level_t lvl);
int jz_gpio_save_reset_func(enum gpio_port port, enum gpio_function dst_func,
			    unsigned long pins, struct gpio_reg_func *rfunc);
int jz_gpio_restore_func(enum gpio_port port,
			 unsigned long pins, struct gpio_reg_func *rfunc);
int mcu_gpio_register(unsigned int reg);
int jzgpio_phy_reset(struct jz_gpio_phy_reset *gpio_phy_reset);

#endif

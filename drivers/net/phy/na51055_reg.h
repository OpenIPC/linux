/*
 * drivers/net/phy/novatek_reg.h
 *
 * Novatek PHYs register
 *
 * Copyright (c) 2019 Novatek Microelectronics Corp.
 *
 */

#define HW_RESET_ADDRESS     0xFD020088
#define HW_RESET_MASK        0xC0000000

#define SW_RESET_ADDRESS     0xFD2B3800
#define SW_RESET_MASK        0x00000002
#define PLLREGWR_MASK        0x00000008

#define EQ_RESET_ADDRESS     0xFD2B3818
#define EQ_RESET_MASK        0x00000002

#define PHY_REGISTER_READ(data,addr) do { \
        data = ioread32((void *)addr); \
}while(0)

#define phy_hw_reset_enable() do { \
        unsigned long v; \
        v = ioread32((void *)HW_RESET_ADDRESS); \
        v &= ~(HW_RESET_MASK); \
        iowrite32(v, (void *)HW_RESET_ADDRESS);\
}while(0)

#define phy_hw_reset_disable() do { \
        unsigned long v; \
        v = ioread32((void *)HW_RESET_ADDRESS); \
        v |= HW_RESET_MASK; \
        iowrite32(v, (void *)HW_RESET_ADDRESS);\
}while(0)

#define phy_sw_reset_enable() do { \
        unsigned long v; \
        v = ioread32((void *)SW_RESET_ADDRESS); \
        v |= SW_RESET_MASK; \
        iowrite32(v, (void *)SW_RESET_ADDRESS);\
}while(0)

#define phy_sw_reset_disable() do { \
        unsigned long v; \
        v = ioread32((void *)SW_RESET_ADDRESS); \
        v |= PLLREGWR_MASK; \
        v &= ~(SW_RESET_MASK); \
        iowrite32(v, (void *)SW_RESET_ADDRESS);\
}while(0)

#define eq_reset_enable() do { \
        unsigned long v; \
        v = ioread32((void *)EQ_RESET_ADDRESS); \
        v |= EQ_RESET_MASK; \
        iowrite32(v, (void *)EQ_RESET_ADDRESS);\
}while(0)

#define eq_reset_disable() do { \
        unsigned long v; \
        v = ioread32((void *)EQ_RESET_ADDRESS); \
        v &= ~(EQ_RESET_MASK); \
        iowrite32(v, (void *)EQ_RESET_ADDRESS);\
}while(0)

#define set_break_link_timer() do { \
        iowrite32(0x53, (void *)0xFD2B3A88);\
        iowrite32(0x07, (void *)0xFD2B3A8C);\
        iowrite32(0xC9, (void *)0xFD2B3A84);\
}while(0)

#define set_one_led_link_act() do { \
        iowrite32(0x40, (void *)0xFD2B3900);\
        iowrite32(0x300, (void *)0xFD2B300C);\
}while(0)

#define set_two_leds_link_act() do { \
        iowrite32(0x40, (void *)0xFD2B3900);\
        iowrite32(0x312, (void *)0xFD2B300C);\
}while(0)

#define set_led_inv() do { \
        unsigned long v; \
        v = ioread32((void *)0xFD2B300C); \
	v &= ~(0x3<<8); \
        iowrite32(v, (void *)0xFD2B300C);\
} while (0)

#define set_best_setting() do { \
        unsigned long v; \
        v = ioread32((void *)0xFD2B3AF8); \
        v &= ~(0x07); \
        v |= (0x03); \
        iowrite32(v, (void *)0xFD2B3AF8);\
        v = ioread32((void *)0xFD2B3B54); \
        v |= (1<<7); \
        iowrite32(v, (void *)0xFD2B3B54);\
        v = ioread32((void *)0xFD2B3B58); \
        v |= (0x3<<4); \
        iowrite32(v, (void *)0xFD2B3B58);\
}while(0)


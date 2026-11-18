#ifndef _AX_PINMUX_H_
#define _AX_PINMUX_H_

#include <linux/soc/axera/ax_boardinfo.h>

#define DPHYTX_BASE                   0x230A000UL
#define DPHY_REG_LEN                  0x1000
#define DPHYTX_SW_RST_SET             0x46000B8
#define DPHYTX_SW_RST_SHIFT           BIT(6)
#define DPHYTX_MIPI_EN                0x23F110C
#define PINMUX_FUNC_SEL               GENMASK(18, 16)

#define REG_REMAP_SIZE                0x1000

//G6 pinmux register controls sleep mode misc_g6[1]
#define PIN_BASE_G6                    0x2304000
#define PIN_GROUP_SLEEP_EN_OFFECT      0xA8
#define PIN_GROUP_SLEEP_EN_SHIFT       1
#define PIN_GROUP_SLEEP_EN_SHIFT_MASK  1

struct pinmux {
	unsigned int *data;
	unsigned int size;
};

#endif
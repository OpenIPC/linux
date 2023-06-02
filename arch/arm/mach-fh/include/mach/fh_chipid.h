/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-08-20     wangyl       add license Apache-2.0
 */

#ifndef __FH_CHIPID_H__
#define __FH_CHIPID_H__

#define FH_CHIP_FH8830          0x883000A1
#define FH_CHIP_FH8630M         0x883000B1
#define FH_CHIP_FH8632          0x863200A1
#define FH_CHIP_FH8632v2        0x863200A2
#define FH_CHIP_FH8856          0x885600A1
#define FH_CHIP_FH8852          0x885600B1
#define FH_CHIP_FH8626V100      0x8626A100

struct fh_chip_info
{
    int _plat_id; /* 芯片寄存器中的plat_id */
    int _chip_id; /* 芯片寄存器中的chip_id */
    int _chip_mask; /* 芯片寄存器中的chip_id */
    int chip_id; /* 芯片chip_id，详见上述定义 */
    int ddr_size; /* 芯片DDR大小，单位Mbit */
    char chip_name[32]; /* 芯片名称 */
};

void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id);
unsigned int fh_get_ddrsize_mbit(void);
char *fh_get_chipname(void);
struct fh_chip_info *fh_get_chip_info(void);

unsigned int fh_is_8830(void);
unsigned int fh_is_8632(void);
unsigned int fh_is_8852(void);
unsigned int fh_is_8856(void);
unsigned int fh_is_8626v100(void);

int fh_chipid_init(void);

#endif /* __FH_CHIPID_H__ */

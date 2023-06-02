/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __BSP_NAND_TEST_H__
#define __BSP_NAND_TEST_H__

extern int nand_driver_test_init(void);
extern int nand_driver_test_exit(void);
extern unsigned int  get_nftl_num(void);
extern unsigned int get_nftl_cap(void);
extern unsigned int get_first_nftl_cap(void);
extern unsigned int nftl_test_read(unsigned int start_sector, unsigned int len, unsigned char *buf);
extern unsigned int nftl_test_write(unsigned int start_sector, unsigned int len, unsigned char *buf);
extern unsigned int nftl_test_flush_write_cache(void);


#endif  /*ifndef __BSP_NAND_TEST_H__*/

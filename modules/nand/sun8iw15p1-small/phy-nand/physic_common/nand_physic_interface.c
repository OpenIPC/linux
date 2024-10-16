/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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
#define _NPHYI_COMMON_C_

#include "../physic_v2/nand_physic_inc.h"

extern int rawnand_physic_read_super_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
extern __s32 spinand_super_page_read(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap,
		void *pBuf, void *pSpare);
extern __s32 spinand_super_page_write(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare);
extern __s32 spinand_super_page_write(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare);
extern __u32 spinand_get_twoplane_flag(void);
extern __u32 rawnand_get_twoplane_flag(void);
extern __u32 SPINAND_GetPageSize(void);
extern __u32 SPINAND_GetLsbblksize(void);
extern __u32 RAWNAND_GetLsbPages(void);
extern __u32 SPINAND_GetLsbPages(void);
extern __u32 SPINAND_GetPhyblksize(void);
extern __u32 RAWNAND_UsedLsbPages(void);
extern __u32 SPINAND_UsedLsbPages(void);
extern u32 SPINAND_GetPageNo(u32 lsb_page_no);
extern __u32 RAWNAND_GetPageCntPerBlk(void);
extern __u32 SPINAND_GetPageCntPerBlk(void);
extern __u32 RAWNAND_GetPageSize(void);
extern __u32 SPINAND_GetPageSize(void);


__u32 storage_type = 0;

__u32 get_storage_type_from_init(void)
{
	return storage_type;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
struct _nand_info *NandHwInit(void)
{
	struct _nand_info *nand_info_temp;

	if (get_storage_type() == 1) {
		nand_info_temp = RawNandHwInit();
	} else if (get_storage_type() == 2) {
		nand_info_temp = SpiNandHwInit();
	} else {
		storage_type = 1;
		nand_info_temp = RawNandHwInit();
		if (nand_info_temp == NULL) {
			storage_type = 2;
			nand_info_temp = SpiNandHwInit();
		}
	}
	return nand_info_temp;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int NandHwExit(void)
{
	int ret = 0;

	if (get_storage_type() == 1) {
		ret = RawNandHwExit();
	} else if (get_storage_type() == 2) {
		ret = SpiNandHwExit();
	} else {
		RawNandHwExit();
		SpiNandHwExit();
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int NandHwSuperStandby(void)
{
	int ret = 0;

	if (get_storage_type() == 1) {
		ret = RawNandHwSuperStandby();
	} else if (get_storage_type() == 2) {
		ret = SpiNandHwSuperStandby();
	}

	return ret;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int NandHwSuperResume(void)
{
	int ret = 0;

	if (get_storage_type() == 1) {
		ret = RawNandHwSuperResume();
	} else if (get_storage_type() == 2) {
		ret = SpiNandHwSuperResume();
	}

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int NandHwNormalStandby(void)
{
	int ret = 0;

	if (get_storage_type() == 1) {
		ret = RawNandHwNormalStandby();
	} else if (get_storage_type() == 2) {
		ret = SpiNandHwNormalStandby();
	}

	return ret;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int NandHwNormalResume(void)
{
	int ret = 0;

	if (get_storage_type() == 1) {
		ret = RawNandHwNormalResume();
	} else if (get_storage_type() == 2) {
		ret = SpiNandHwNormalResume();
	}

	return ret;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int NandHwShutDown(void)
{
	int ret = 0;

	if (get_storage_type() == 1) {
		ret = RawNandHwShutDown();
	} else if (get_storage_type() == 2) {
		ret = SpiNandHwShutDown();
	}

	return ret;
}
/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
void *NAND_GetIOBaseAddr(u32 no)
{
	if (get_storage_type() == 1) {
		if (no != 0)
			return (void *)RAWNAND_GetIOBaseAddrCH1();
		else
			return (void *)RAWNAND_GetIOBaseAddrCH0();
	} else if (get_storage_type() == 2) {
		if (no != 0)
			return (void *)SPINAND_GetIOBaseAddrCH1();
		else
			return (void *)SPINAND_GetIOBaseAddrCH0();
	}

	return NULL;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
u32 NAND_GetLogicPageSize(void)
{
	return 512 * aw_nand_info.SectorNumsPerPage;
}


/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int nand_physic_erase_block(unsigned int chip, unsigned int block)
{
	if (get_storage_type() == 1)
		return rawnand_physic_erase_block(chip, block);
	else if (get_storage_type() == 2)
		return spinand_physic_erase_block(chip, block);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int nand_physic_read_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf)
{
	if (get_storage_type() == 1)
		return rawnand_physic_read_page(chip, block, page, bitmap, mbuf, sbuf);
	else if (get_storage_type() == 2)
		return spinand_physic_read_page(chip, block, page, bitmap, mbuf, sbuf);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int nand_physic_write_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf)
{
	if (get_storage_type() == 1)
		return rawnand_physic_write_page(chip, block, page, bitmap, mbuf, sbuf);
	else if (get_storage_type() == 2)
		return spinand_physic_write_page(chip, block, page, bitmap, mbuf, sbuf);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int nand_physic_bad_block_check(unsigned int chip, unsigned int block)
{
	if (get_storage_type() == 1)
		return rawnand_physic_bad_block_check(chip, block);
	else if (get_storage_type() == 2)
		return spinand_physic_bad_block_check(chip, block);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int nand_physic_bad_block_mark(unsigned int chip, unsigned int block)
{
	if (get_storage_type() == 1)
		return rawnand_physic_bad_block_mark(chip, block);
	else if (get_storage_type() == 2)
		return spinand_physic_bad_block_mark(chip, block);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int nand_physic_block_copy(unsigned int chip_s, unsigned int block_s, unsigned int chip_d, unsigned int block_d)
{
	if (get_storage_type() == 1)
		return rawnand_physic_block_copy(chip_s, block_s, chip_d, block_d);
	else if (get_storage_type() == 2)
		return spinand_physic_block_copy(chip_s, block_s, chip_d, block_d);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare)
{
	if (get_storage_type() == 1)
		return rawnand_physic_read_super_page(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
	else if (get_storage_type() == 2)
		return spinand_super_page_read(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare)
{
	if (get_storage_type() == 1)
		return rawnand_physic_write_super_page(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
	else if (get_storage_type() == 2)
		return spinand_super_page_write(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum)
{
	if (get_storage_type() == 1)
		return rawnand_physic_erase_super_block(nDieNum, nBlkNum);
	else if (get_storage_type() == 2)
		return spinand_super_block_erase(nDieNum, nBlkNum);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int PHY_VirtualBadBlockCheck(unsigned int nDieNum, unsigned int nBlkNum)
{
	if (get_storage_type() == 1)
		return rawnand_physic_super_bad_block_check(nDieNum, nBlkNum);
	else if (get_storage_type() == 2)
		return spinand_super_badblock_check(nDieNum, nBlkNum);
	return 0;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       : 0:ok  -1:fail
 *Note         :
 *****************************************************************************/
int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum)
{
	if (get_storage_type() == 1)
		return rawnand_physic_super_bad_block_mark(nDieNum, nBlkNum);
	else if (get_storage_type() == 2)
		return spinand_super_badblock_mark(nDieNum, nBlkNum);
	return 0;
}

__u32 NAND_GetLsbblksize(void)
{
	if (get_storage_type() == 1)
		return RAWNAND_GetLsbblksize();
	else if (get_storage_type() == 2)
		return SPINAND_GetLsbblksize();
	return 0;
}

__u32 NAND_GetLsbPages(void)
{
	if (get_storage_type() == 1)
		return RAWNAND_GetLsbPages();
	else if (get_storage_type() == 2)
		return SPINAND_GetLsbPages();

	return 0;
}

__u32 NAND_GetPhyblksize(void)
{
	if (get_storage_type() == 1)
		return RAWNAND_GetPhyblksize();
	else if (get_storage_type() == 2)
		return SPINAND_GetPhyblksize();
	return 0;
}

__u32 NAND_UsedLsbPages(void)
{
	if (get_storage_type() == 1)
		return RAWNAND_UsedLsbPages();
	else if (get_storage_type() == 2)
		return SPINAND_UsedLsbPages();
	return 0;
}

u32 NAND_GetPageNo(u32 lsb_page_no)
{
	if (get_storage_type() == 1)
		return RAWNAND_GetPageNo(lsb_page_no);
	else if (get_storage_type() == 2)
		return SPINAND_GetPageNo(lsb_page_no);

	return 0;
}

__u32 NAND_GetPageCntPerBlk(void)
{
	if (get_storage_type() == 1)
		return RAWNAND_GetPageCntPerBlk();
	else if (get_storage_type() == 2)
		return SPINAND_GetPageCntPerBlk();

	return 0;
}

__u32 NAND_GetPageSize(void)
{
	if (get_storage_type() == 1)
		return RAWNAND_GetPageSize();
	else if (get_storage_type() == 2)
		return SPINAND_GetPageSize();

	return 0;
}

__u32 nand_get_twoplane_flag(void)
{
	if (get_storage_type() == 1)
		return rawnand_get_twoplane_flag();
	else if (get_storage_type() == 2)
		return spinand_get_twoplane_flag();
	return 0;
}


int nand_set_gpio_power(struct _nand_chip_info *nci)
{
	if (nci == NULL) {
		NAND_Print("%s %d nci is null\n", __func__, __LINE__);
		return -1;
	}

	if (nci->npi->operation_opt & NAND_VCCQ_1p8V)
		nand_set_gpio_power_1p8v();
	else
		NAND_Print("%s %d use default 3.3V\n", __func__, __LINE__);
	return 0;
}

/*
 * mdrvSpinandBbtBbm.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <mdrvFlashOsImpl.h>
#include <mdrvSpinandBbtBbm.h>
#include <drvSPINAND.h>

#define BBT_HEADER                0x42425448
#define BLOCK_INFO_NUM            1024
#define MAX_BBT_SIZE              sizeof(BBT_INFO_t)
#define FLASH_BBT_PAGE_SIZE       g_st_nand_info.u16_PageSize
#define FLASH_BBT_SIZE            g_st_nand_info.u32_Capacity
#define FLASH_BBT_BLOCK_SIZE      g_st_nand_info.u32_BlockSize
#define FLASH_BBT_PAGES_PER_BLOCK g_st_nand_info.u16_BlkPageCnt

typedef struct block_info // 1 byte records the status of 2 blocks
{
    u8 b0 : 4; // block0, 0000:factory bad block  0001:Ecc correctable  0010:running-time bad block  1111:good block
    u8 b1 : 4; // block0, 0000:factory bad block  0001:Ecc correctable  0010:running-time bad block  1111:good block
} BLOCK_INFO_t;

typedef struct bbt_info
{
    u32          u32_Header;                // the header of bbt£¬0x42425448(bbth)
    u32          u32_Crc32;                 // the crc value of bbt
    BLOCK_INFO_t stBlkInfo[BLOCK_INFO_NUM]; // record the blocks' status,the maximum Block number is 1024*2=2048
} BBT_INFO_t;

static FLASH_NAND_INFO_t g_st_nand_info;
static u8 *              g_bbt_buf;

#if defined(CONFIG_FLASH_ENABLE_BBM)
static u8    g_do_bbm;
PARTS_INFO_t st_bbt_part;
#endif

/********************************************************************************
*Function: _MDRV_BBT_is_valid_tbl(u32 u32_address)
*Description: Check it is a valid bbt table or not
*Input:
        u32_address: The address where stored the bbt info
*Output:
*Retrun:
        TRUE : It is a valid bbt table
        0: It is not a valid bbt table
********************************************************************************/
static u8 _MDRV_BBT_is_valid_tbl(u8 *bbt_buf)
{
    BBT_INFO_t *pst_bbt_info = (BBT_INFO_t *)bbt_buf;

    if (BBT_HEADER == pst_bbt_info->u32_Header
        && pst_bbt_info->u32_Crc32
               == FLASH_IMPL_CRC32(0, (void *)pst_bbt_info->stBlkInfo, sizeof(BLOCK_INFO_t) * BLOCK_INFO_NUM))
    {
        return 1;
    }

    return 0;
}

#if defined(CONFIG_FLASH_WRITE_BACK)
/********************************************************************************
*Function: _MDRV_BBT_init_blk_info(u32 u32_address)
*Description: Init the block info
*Input:
        u32_address: The address where to store bbt info
*Output:
*Retrun:
********************************************************************************/
static void _MDRV_BBT_init_blk_info(u8 *bbt_buf)
{
    u16         u16_i;
    BBT_INFO_t *pst_bbt_info;

    pst_bbt_info = (BBT_INFO_t *)bbt_buf;

    // init block info
    for (u16_i = 0; u16_i < BLOCK_INFO_NUM; u16_i++)
    {
        pst_bbt_info->stBlkInfo[u16_i].b0 = GOOD_BLOCK;
        pst_bbt_info->stBlkInfo[u16_i].b1 = GOOD_BLOCK;
    }

    pst_bbt_info->u32_Crc32 =
        FLASH_IMPL_CRC32(0, (void *)pst_bbt_info->stBlkInfo, sizeof(BLOCK_INFO_t) * BLOCK_INFO_NUM);
}

/********************************************************************************
*Function: void _MDRV_BBT_init_tbl(u32 u32_address)
*Description: Init the bbt table
*Input:
        u32_address: The address where to store the bbt info
*Output:
*Retrun:
********************************************************************************/
static u8 _MDRV_BBT_init_tbl(u8 *bbt_buf)
{
    u32         u32_page;
    u32         u32_end;
    BBT_INFO_t *pst_bbt_info = (BBT_INFO_t *)bbt_buf;

    pst_bbt_info->u32_Header = BBT_HEADER;

    FLASH_IMPL_PRINTF("[BBT] Create bbt\r\n");
    // Fill Block Info
    _MDRV_BBT_init_blk_info((u8 *)pst_bbt_info);
    u32_page = 0;
    u32_end  = FLASH_BBT_PAGES_PER_BLOCK * BLOCK_INFO_NUM;

    for (; u32_end > u32_page; u32_page += FLASH_BBT_PAGES_PER_BLOCK)
    {
        /*Scan all flash for bad blocks
        If no bad block return NO_ERR_BLOCK(0xFFFFFFFF)
        Record bad blocks info to stFlashInfo*/
        if (mdrv_spinand_block_isbad(u32_page))
        {
            FLASH_IMPL_PRINTF_HEX("[BBT] find bad block 0x", u32_page * FLASH_BBT_PAGE_SIZE, "\r\n");
            MDRV_BBT_fill_blk_info(u32_page * FLASH_BBT_PAGE_SIZE, FACTORY_BAD_BLOCK);
        }
    }

    g_bbt_buf = bbt_buf;

    if (E_BBTBBM_FAIL == MDRV_BBT_save_bbt(0))
    {
        g_bbt_buf = NULL;
        return E_BBTBBM_FAIL;
    }

    return E_BBTBBM_SUCCESS;
}
#else
#define _MDRV_BBT_init_tbl(x) E_BBTBBM_FAIL
#endif

static u8 _MDRV_BBT_get_part(u32 *u32_offset, u32 *u32_size)
{
    *u32_offset = 0;
    *u32_size   = FLASH_BBT_PAGES_PER_BLOCK * 10;

    return 1;
}

/********************************************************************************
*Function: MDRV_PARTS_load_bbt(u32 u32_address)
*Description: Read bbt from flash to dram
*Input:
        u32_address: The addresss to store the bbt
*Output:
*Retrun:
        TRUE : Read bbt successful
        0: Read bbt failed
********************************************************************************/
u8 MDRV_BBT_init(u8 *bbt_buf)
{
    s8  s8_bbt_count;
    u32 u32_page;
    u32 u32_end;

    g_bbt_buf = NULL;

    if (_MDRV_BBT_is_valid_tbl(bbt_buf))
    {
        g_bbt_buf = bbt_buf;
        return E_BBTBBM_SUCCESS;
    }

    mdrv_spinand_info(&g_st_nand_info);

    if (0 == g_st_nand_info.u32_Capacity)
    {
        FLASH_IMPL_PRINTF("No flash in board!\r\n");
        return E_BBTBBM_FAIL;
    }

    if (!_MDRV_BBT_get_part(&u32_page, &u32_end))
    {
        return E_BBTBBM_FAIL;
    }

    for (; u32_end > u32_page; u32_page += FLASH_BBT_PAGES_PER_BLOCK)
    {
        if (u32_page & FLASH_BBT_PAGES_PER_BLOCK)
        {
            for (s8_bbt_count = 1; s8_bbt_count >= 0; s8_bbt_count--)
            {
                if (ERR_SPINAND_SUCCESS
                    == mdrv_spinand_page_read(u32_page + s8_bbt_count, 0, bbt_buf, MAX_BBT_TBL_SIZE))
                {
                    if (_MDRV_BBT_is_valid_tbl(bbt_buf))
                    {
                        FLASH_IMPL_PRINTF_HEX("[BBT] Found table @ 0x", FLASH_BBT_PAGE_SIZE * (u32_page + s8_bbt_count),
                                              "\r\n");
                        g_bbt_buf = bbt_buf;
                        return E_BBTBBM_SUCCESS;
                    }
                }
            }
        }
    }

    return _MDRV_BBT_init_tbl(bbt_buf);
}

/********************************************************************************
*Function: nand_bbt_get_blk_info(u32 u32Offset)
*Description: Get the block info
*Input:
         u32Offset: The address(byte) of block
*Output:
*Retrun: 0x00:factory bad block       0x01:Ecc correctable  0x02:running-time bad block
         0x03:Ecc correctable-not fix 0x0f:good block       0xff:No bbt
********************************************************************************/
u8 MDRV_BBT_get_blk_info(u32 u32Offset)
{
    u8          u8Remd      = 0;
    u8          u8Ret       = 0x0f;
    u16         u16BlkIndex = 0;
    BBT_INFO_t *pst_bbt_info;

    if (!g_bbt_buf)
    {
        return E_BBTBBM_FAIL;
    }

    pst_bbt_info = (BBT_INFO_t *)g_bbt_buf;

    u16BlkIndex = u32Offset / FLASH_BBT_BLOCK_SIZE; // u32Offset/u32BlkSz
    u8Remd      = u16BlkIndex & 0x01;               // u16BlkIndex%2
    u16BlkIndex = u16BlkIndex >> 1;                 // u16BlkIndex/2
    if (u8Remd)
        u8Ret &= pst_bbt_info->stBlkInfo[u16BlkIndex].b1;
    else
        u8Ret &= pst_bbt_info->stBlkInfo[u16BlkIndex].b0;

    return u8Ret;
}

/********************************************************************************
*Function: MDRV_BBT_fill_blk_info(u32 u32Offset, u32 u32_address, u8 u8BlkType)
*Description: Fill the block type to bbt
*Input:
        u32Offset  : The address(byte) of block
        u32_address: The address where stored the bbt info
        u8BlkType  : The bad block type
*Output:
*Retrun:
********************************************************************************/
u8 MDRV_BBT_fill_blk_info(u32 u32Offset, BLKTYPE_e BlkType)
{
    u8          u8Remd      = 0;
    u8          u8BlkSzBits = 0;
    u16         u16BlkIndex = 0;
    BBT_INFO_t *pst_bbt_info;

    if (!g_bbt_buf)
    {
        return E_BBTBBM_FAIL;
    }

    pst_bbt_info = (BBT_INFO_t *)g_bbt_buf;

    u8BlkSzBits = FLASH_IMPL_count_bits(FLASH_BBT_BLOCK_SIZE);
    u16BlkIndex = u32Offset >> u8BlkSzBits; // u32Offset/u32BlkSz
    u8Remd      = u16BlkIndex & 0x01;       // u16BlkIndex%2
    u16BlkIndex = u16BlkIndex >> 1;         // u16BlkIndex/2

    if (u16BlkIndex >= BLOCK_INFO_NUM)
        return E_BBTBBM_FAIL;

    if (u8Remd)
        pst_bbt_info->stBlkInfo[u16BlkIndex].b1 = BlkType & 0xF;
    else
        pst_bbt_info->stBlkInfo[u16BlkIndex].b0 = BlkType & 0xF;

    pst_bbt_info->u32_Crc32 =
        FLASH_IMPL_CRC32(0, (void *)pst_bbt_info->stBlkInfo, sizeof(BLOCK_INFO_t) * BLOCK_INFO_NUM);

    return E_BBTBBM_SUCCESS;
}

/********************************************************************************
*Function: MDRV_PARTS_save_bbt(u32 u32_address, BOOL b_update)
*Description: Save bbt table to flash
*Input:
        u32_address: The addresss where store the new bbt
        b_update   : TRUE-Uptate bbt to page1   0-Program bbt to page0
*Output:
*Retrun:
        TRUE : Save bbt successful
        0: Save bbt failed
********************************************************************************/
u8 MDRV_BBT_save_bbt(u8 u8_update)
{
    u8  u8_val = 0;
    u32 u32_page;
    u32 u32_end;

    if (!g_bbt_buf || !_MDRV_BBT_is_valid_tbl(g_bbt_buf))
    {
        return E_BBTBBM_FAIL;
    }

    _MDRV_BBT_get_part(&u32_page, &u32_end);

    if (u32_page != u32_end)
    {
        for (; u32_end > u32_page; u32_page += FLASH_BBT_PAGES_PER_BLOCK)
        {
            if ((u32_page & FLASH_BBT_PAGES_PER_BLOCK))
            {
                if (!mdrv_spinand_block_isbad(u32_page))
                {
                    /*When do bbm flow,Update bbt in odd block page1*/
                    if (u8_update)
                    {
                        if (ERR_SPINAND_SUCCESS == mdrv_spinand_page_program(u32_page + 1, 0, g_bbt_buf, MAX_BBT_SIZE))
                        {
                            FLASH_IMPL_PRINTF("Upate BBT\r\n");
                            return E_BBTBBM_SUCCESS;
                        }
                    }
                    else /*When creat bbt,program bbt in odd block page0*/
                    {
                        if (ERR_SPINAND_SUCCESS == mdrv_spinand_block_erase(u32_page)
                            && ERR_SPINAND_SUCCESS == mdrv_spinand_page_program(u32_page, 0, g_bbt_buf, MAX_BBT_SIZE))
                        {
                            FLASH_IMPL_PRINTF("Program BBT\r\n");
                            return E_BBTBBM_SUCCESS;
                        }

                        FLASH_IMPL_PRINTF("Program BBT fail\r\n");
                    }

                    // markbad
                    mdrv_spinand_page_program(u32_page, FLASH_BBT_PAGE_SIZE, &u8_val, 1);
                }
                MDRV_BBT_fill_blk_info(u32_page * FLASH_BBT_PAGE_SIZE, RUN_TIME_BAD_BLOCK);
            }
        }
    }

    return E_BBTBBM_FAIL;
}

#if defined(CONFIG_FLASH_ENABLE_BBM)
#include <mdrvFlash.h>
#include <mdrvParts.h>

/********************************************************************************
*Function: _MDRV_BBT_has_corrected(u32 u32_address, u32 u32_offset)
*Description: Find the block info in bbt,check it is ecc correctable block or not
*Input:
        u32_address: The address where stored the bbt info
        u32Offset  : The address(byte) of block
*Output:
*Retrun:
        0x01(ECC_CORRECT_BLOCK)    : It is ecc correctable block
        0x03(ECC_CORR_NOTFIX_BLOCK): It is not ecc correctable block
        0x0f(GOOD_BLOCK)           ï¼šIt is a good block
********************************************************************************/
static u8 _MDRV_BBT_has_corrected(u8 *bbt_buf, u32 u32_offset)
{
    BBT_INFO_t *pst_bbt_info;
    u8          u8_block_status;
    u32         u32_block_index;

    pst_bbt_info    = (BBT_INFO_t *)bbt_buf;
    u32_block_index = u32_offset >> FLASH_IMPL_count_bits(FLASH_BBT_BLOCK_SIZE);
    u8_block_status = GOOD_BLOCK;

    if ((u32_block_index >> 1) >= BLOCK_INFO_NUM)
        return u8_block_status;

    if (u32_block_index & 0x01)
    {
        u8_block_status = pst_bbt_info->stBlkInfo[u32_block_index >> 1].b1;
    }
    else
    {
        u8_block_status = pst_bbt_info->stBlkInfo[u32_block_index >> 1].b0;
    }

    return u8_block_status;
}

/********************************************************************************
 *Function: MDRV_BBM_init(void)
 *Description:
 *Input:
 *Output:
 *Retrun:
 ********************************************************************************/
void MDRV_BBM_init(void)
{
    g_do_bbm = 0;

    if (MDRV_PARTS_get_active_part((u8 *)"BBM", &st_bbt_part)) // bbm flag
    {
        g_do_bbm = 1;
    }
}

/********************************************************************************
*Function: MDRV_BBM_failure_recover(PARTS_INFO_t *pst_PartInfo,u32 u32_address)
*Description: Do poweroff recovery according the bbt in page1
*Input:
        pst_PartInfo: The partition info
        u32_address : The address where to store the partition image
*Output:
*Retrun:
********************************************************************************/
u8 MDRV_BBM_failure_recover(PARTS_INFO_t *pst_PartInfo, u8 *pu8_data)
{
    u32 u32_offset;
    u32 u32_part_end = 0;

    if (!g_do_bbm) // bbm flag
    {
        return E_BBTBBM_FAIL;
    }

    if (!g_bbt_buf || !pst_PartInfo)
    {
        return E_BBTBBM_FAIL;
    }

    u32_part_end = pst_PartInfo->u32_Offset + pst_PartInfo->u32_Size;

    for (u32_offset = pst_PartInfo->u32_Offset; u32_offset < u32_part_end; u32_offset += FLASH_BBT_BLOCK_SIZE)
    {
        if (ECC_CORRECT_BLOCK == _MDRV_BBT_has_corrected(g_bbt_buf, u32_offset))
        {
            FLASH_IMPL_PRINTF("To do poweroff recover\r\n");
            /*read data from reserved block to dram*/

            if (FLASH_BBT_BLOCK_SIZE
                != MDRV_FLASH_read_skip_bad(st_bbt_part.u32_Offset, st_bbt_part.u32_Size, pu8_data,
                                            FLASH_BBT_BLOCK_SIZE))
            {
                return E_BBTBBM_FAIL;
            }

            /*write back to ecc correctable block*/
            if (FLASH_BBT_BLOCK_SIZE != MDRV_FLASH_is_programmable(u32_offset, pu8_data, FLASH_BBT_BLOCK_SIZE))
            {
                /*If the block had been mark bad,needn't to mark*/
                if (NO_ERR_BLOCK == MDRV_FLASH_get_bad(u32_offset, FLASH_BBT_BLOCK_SIZE))
                {
                    MDRV_FLASH_mark_bad(u32_offset, RUN_TIME_BAD_BLOCK);
                }

                MDRV_BBT_fill_blk_info(u32_offset, RUN_TIME_BAD_BLOCK);
                if (pst_PartInfo->u8_BackupTrunk != pst_PartInfo->u8_Trunk)
                {
                    MDRV_PARTS_mark_active(pst_PartInfo->au8_partName, pst_PartInfo->u8_BackupTrunk);
                }
            }
            else
            {
                MDRV_BBT_fill_blk_info(u32_offset, GOOD_BLOCK);
            }

            return MDRV_BBT_save_bbt(0);
        }
    }

    return E_BBTBBM_SUCCESS;
}

/********************************************************************************
*Function: MDRV_BBM_load_part_bbm(PARTS_INFO_t *pst_PartInfo, u32 u32_address, u32 u32_size)
*Description: When loading partition image,if found there have ecc correctable blocks,do bbm flow
*Input:
        pst_PartInfo: The partition info
        u32_address : The address where to store the partition image
        u32_size    : The size of partition
*Output:
*Retrun:
********************************************************************************/
u8 MDRV_BBM_load_part_bbm(PARTS_INFO_t *pst_PartInfo, u8 *pu8_data, u32 u32_size)
{
    u32 u32_part_end;
    u32 u32_bytes_read;
    u32 u32_bad_offset;
    u32 u32_program_size;
    u32 u32_program_offset;
    u32 u32_backup_offset;
    u32 u32_corrected_offset;
    u32 u32_corrected_size;
    u32 u32_corrected_limit;

    if (!g_do_bbm) // bbm flag
    {
        return E_BBTBBM_FAIL;
    }

    if (!g_bbt_buf || !pst_PartInfo)
    {
        return E_BBTBBM_FAIL;
    }

    u32_corrected_size   = u32_size;
    u32_corrected_offset = pst_PartInfo->u32_Offset;
    u32_corrected_limit  = pst_PartInfo->u32_Size;
    u32_part_end         = pst_PartInfo->u32_Offset + pst_PartInfo->u32_Size;

    while (NO_ERR_BLOCK != u32_corrected_offset)
    {
        // Needn't to do bbm when the size is smaller then page size,because it may be a image header,Doing BBM is a
        // waste of time
        if (FLASH_BBT_PAGE_SIZE > u32_size)
        {
            break;
        }

        /*find the correctable block address*/
        if (NO_ERR_BLOCK
            != (u32_corrected_offset =
                    MDRV_FLASH_get_corrected(u32_corrected_offset, u32_corrected_limit, u32_corrected_size)))
        {
            FLASH_IMPL_PRINTF("[BBM] Found corrected @ 0x");
            FLASH_ERR_VAR32(u32_corrected_offset);
            FLASH_IMPL_PRINTF("\r\n");
            if (ECC_CORR_NOTFIX_BLOCK == _MDRV_BBT_has_corrected(g_bbt_buf, u32_corrected_offset))
            {
                FLASH_IMPL_PRINTF("NO reserved block to do bbm,skip\r\n");
                break;
            }
            MDRV_BBT_fill_blk_info(u32_corrected_offset, ECC_CORRECT_BLOCK);

            u32_bad_offset = pst_PartInfo->u32_Offset;
            u32_bytes_read = u32_corrected_offset - pst_PartInfo->u32_Offset;

            /*Skip the bad block,calculate the bytes have already been read*/
            while (NO_ERR_BLOCK != u32_bad_offset)
            {
                if (NO_ERR_BLOCK
                    != (u32_bad_offset = MDRV_FLASH_get_bad(u32_bad_offset, u32_corrected_offset - u32_bad_offset)))
                {
                    u32_bytes_read -= FLASH_BBT_BLOCK_SIZE;
                    u32_bad_offset += FLASH_BBT_BLOCK_SIZE;
                }
            }

            u32_program_size = FLASH_BBT_BLOCK_SIZE;
            if ((u32_size - u32_bytes_read) < FLASH_BBT_BLOCK_SIZE)
            {
                u32_program_size = u32_size - u32_bytes_read;
            }

            /*Write the ecc correctable block data to reserved block*/
            while (NO_ERR_BLOCK
                   != (u32_backup_offset = MDRV_FLASH_get_reserved(st_bbt_part.u32_Offset, st_bbt_part.u32_Size, 0)))
            {
                if (u32_program_size
                    == MDRV_FLASH_is_programmable(u32_backup_offset, pu8_data + u32_bytes_read, u32_program_size))
                {
                    FLASH_IMPL_PRINTF("Got and Write to reserved block successful\r\n");
                    break;
                }
                MDRV_FLASH_mark_bad(u32_backup_offset, RUN_TIME_BAD_BLOCK);
                MDRV_BBT_fill_blk_info(u32_backup_offset, RUN_TIME_BAD_BLOCK);
            }

            if (NO_ERR_BLOCK != u32_backup_offset)
            {
                /*If there has reserved block, record now is in bbm stage,for power off recover*/
                MDRV_BBT_save_bbt(TRUE);
                /*Write back the data to ecc correctable block*/
                if (u32_program_size
                    == MDRV_FLASH_is_programmable(u32_corrected_offset, pu8_data + u32_bytes_read, u32_program_size))
                {
                    FLASH_IMPL_PRINTF("Repair corrected block successful\r\n");
                    MDRV_BBT_fill_blk_info(u32_corrected_offset, GOOD_BLOCK);
                }
                else
                {
                    FLASH_IMPL_PRINTF("Repair corrected block failed,reprogram image\r\n");
                    /*If write back failed,rewrite the data from next block of the ecc correctable block to the end*/
                    MDRV_FLASH_mark_bad(u32_corrected_offset, RUN_TIME_BAD_BLOCK);
                    MDRV_BBT_fill_blk_info(u32_corrected_offset, RUN_TIME_BAD_BLOCK);
                    u32_program_size = u32_size - u32_bytes_read;
                    for (u32_program_offset = u32_corrected_offset + FLASH_BBT_BLOCK_SIZE;
                         u32_part_end > u32_program_offset
                         && FLASH_BBT_BLOCK_SIZE <= (u32_part_end - u32_program_offset);
                         u32_program_offset += FLASH_BBT_BLOCK_SIZE)
                    {
                        if (NO_ERR_BLOCK == MDRV_FLASH_get_bad(u32_program_offset, FLASH_BBT_BLOCK_SIZE))
                        {
                            if (MDRV_FLASH_is_programmable(u32_program_offset, pu8_data + u32_bytes_read,
                                                           (FLASH_BBT_BLOCK_SIZE < u32_program_size)
                                                               ? FLASH_BBT_BLOCK_SIZE
                                                               : u32_program_size))
                            {
                                u32_bytes_read +=
                                    (FLASH_BBT_BLOCK_SIZE < u32_program_size) ? FLASH_BBT_BLOCK_SIZE : u32_program_size;
                                u32_program_size -=
                                    (FLASH_BBT_BLOCK_SIZE < u32_program_size) ? FLASH_BBT_BLOCK_SIZE : u32_program_size;
                            }
                            else
                            {
                                MDRV_FLASH_mark_bad(u32_corrected_offset, RUN_TIME_BAD_BLOCK);
                                MDRV_BBT_fill_blk_info(u32_corrected_offset, RUN_TIME_BAD_BLOCK);
                            }
                        }

                        if (0 == u32_program_size)
                        {
                            FLASH_IMPL_PRINTF("Reprogram partition successful\r\n");
                            break;
                        }
                    }

                    /*If no enough block to rewrite the partition,switch active partition,
                    but needn't to reload image,because the image has been read to ram*/
                    if (0 < u32_program_size)
                    {
                        if (pst_PartInfo->u8_BackupTrunk != pst_PartInfo->u8_Trunk)
                        {
                            FLASH_IMPL_PRINTF("Reprogram partition failed,mark backup trunk as active\r\n");
                            MDRV_PARTS_mark_active(pst_PartInfo->au8_partName, pst_PartInfo->u8_BackupTrunk);
                        }
                    }
                }
            }
            else
            {
                FLASH_IMPL_PRINTF("No reserved block to do bbm,not fix\r\n");
                MDRV_BBT_fill_blk_info(u32_corrected_offset, ECC_CORR_NOTFIX_BLOCK);
            }
            MDRV_BBT_save_bbt(0);
            if (u32_corrected_size < FLASH_BBT_BLOCK_SIZE)
            {
                break;
            }

            u32_corrected_offset += FLASH_BBT_BLOCK_SIZE;
            u32_corrected_limit = u32_part_end - u32_corrected_offset;
            u32_corrected_size  = u32_size - u32_bytes_read;
        }
    }

    return E_BBTBBM_SUCCESS;
}
#endif

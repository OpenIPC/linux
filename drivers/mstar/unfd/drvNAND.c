#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/mm.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mutex.h>
#include <linux/smp.h>
#include "ms_platform.h"
#include "drvNAND.h"

#if defined(CONFIG_OF)
struct clk_data{
	int num_parents;
	struct clk **clk_fcie;
	//struct clk *clk_ecc;
};
struct clk_data* clkdata;
#endif

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
extern irqreturn_t NC_FCIE_IRQ(int irq, void *dummy);
static int fcie_irq = 0;
#endif

#define CONFIG_MTD_PARTITIONS
extern int parse_mtd_partitions(struct mtd_info *master, const char *const *types,
			 struct mtd_partition **pparts,
			 struct mtd_part_parser_data *data);
extern int add_mtd_partitions(struct mtd_info *master,
				const struct mtd_partition *parts,
				int nbparts);



#ifdef CONFIG_MTD_PARTITIONS
static const char *part_probes[] = { "cmdlinepart", NULL };
#define MTD_PARTITION_MAX		64
static struct mtd_partition partition_info[MTD_PARTITION_MAX];
#endif

#ifndef CONFIG_NANDDRV_DEBUG
#define CONFIG_NANDDRV_DEBUG 0
#endif

#if (CONFIG_NANDDRV_DEBUG)
#define NAND_DEBUG(fmt, args...) pr_info(fmt, ##args)
#else
#define NAND_DEBUG(fmt, args...)
#endif

#define DRIVER_NAME "ms-nand"
#define DRIVER_DESC "MS FCIE NAND Device Driver"

//DEFINE_MUTEX(FCIE3_mutex);
extern struct mutex FCIE3_mutex;
#define CRIT_SECT_BEGIN(x)	mutex_lock(x)
#define CRIT_SECT_END(x)	mutex_unlock(x)

static uint8_t scan_ff_pattern[] = { 0xff };
static struct mtd_info *nand_mtd = NULL;

/* struct nand_bbt_descr - bad block table descriptor */
static struct nand_bbt_descr _unfd_nand_bbt_descr = {
	.options		= NAND_BBT_2BIT | NAND_BBT_LASTBLOCK | NAND_BBT_VERSION | NAND_BBT_CREATE | NAND_BBT_WRITE,
	.offs			= 0,
	.len			= 1,
	.pattern		= scan_ff_pattern
};

/*
===========================================
MTD ECC Structure
===========================================
*/
static struct nand_ecclayout unfd_nand_oob_custom;

static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

static struct nand_bbt_descr _unfd_bbt_main_descr = {
	.options		= NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
					  NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs			= 1,
	.len			= 3,
	.veroffs		= 4,
	.maxblocks		= 4,
	.pattern		= bbt_pattern
};

static struct nand_bbt_descr _unfd_bbt_mirror_descr = {
	.options		= NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
					  NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs			= 1,
	.len			= 3,
	.veroffs		= 4,
	.maxblocks		= 4,
	.pattern		= mirror_pattern
};

static U32 u32_WriteLen = 0;
static U32 u32_CurRow = 0;
static U32 u32_CIFDIdx = 0;
static U32 u32_CurCol = 0;
static U16 u16_ByteIdxofPage = 0;
static U32 u32_UnfdRet = 0;
static int s32_ECCStatus = 0;
/*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
======================================UNFD FUNCTION ==============================================
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
*/
void nand_lock_fcie(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	CRIT_SECT_BEGIN(&FCIE3_mutex);

	#if defined (CONFIG_OF)
	{
		int i;
		for(i = 0 ;i < clkdata->num_parents; i ++)
			clk_prepare_enable(clkdata->clk_fcie[i]);
	}
	#endif
	#if defined (IF_FCIE_SHARE_PINS) && IF_FCIE_SHARE_PINS
	nand_pads_switch(pNandDrv->u8_PadMode);
	#endif
	#if defined (IF_FCIE_SHARE_IP) && IF_FCIE_SHARE_IP
	nand_clock_setting(pNandDrv->u32_Clk);
	NC_ResetFCIE();
	NC_Config();
	#endif
}

void nand_unlock_fcie(void)
{
	#if defined (CONFIG_OF)
	{
		int i;
		for(i = 0 ;i < clkdata->num_parents; i ++)
			clk_disable_unprepare(clkdata->clk_fcie[i]);
	}
	#endif
	CRIT_SECT_END(&FCIE3_mutex);
}


/*
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
======================================MTD MAP Function=============================================
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
*/


static void _unfd_nand_hwcontrol(struct mtd_info *mtdinfo, int cmd, unsigned int ctrl)
{
	NAND_DEBUG("%s()\n", __func__);

	return;
}

static int _unfd_nand_device_ready(struct mtd_info *mtdinfo)
{
	NAND_DEBUG("%s()\n", __func__);

	return 1;
}

static void _unfd_nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U8 *pu8_MainBuf = pNandDrv->PlatCtx_t.pu8_PageDataBuf;
	U8 *pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;

	nand_lock_fcie();
	NAND_DEBUG("%s():%X, %X\n", __func__, buf, len);

	u16_ByteIdxofPage += len;
	/*
	for(u16_i = 0; u16_i < u16_len; u16_i ++)
	{
		printf("%2X ",pu8_buf[u16_i]);
		if((u16_i & 0xf) == 0xf)
			printf("\r\n");
	}
	*/

	if( len >= pNandDrv->u16_PageByteCnt )	// whole page or Main area only
	{
		u32_WriteLen += len;
		if( len > pNandDrv->u16_PageByteCnt )	// whole page
		{
			if((U32)buf >= VMALLOC_START && (U32)buf <= VMALLOC_END)		//whether buf is from vmalloc ?
			{
				memcpy(pu8_MainBuf, buf, pNandDrv->u16_PageByteCnt);
				memcpy(pu8_SpareBuf, buf+pNandDrv->u16_PageByteCnt, pNandDrv->u16_SpareByteCnt);
			}
			else
			{
				pu8_MainBuf = (U8*)buf;
				pu8_SpareBuf = (U8*)buf + (pNandDrv->u16_PageByteCnt);
			}

			u32_UnfdRet = NC_WritePages(u32_CurRow, pu8_MainBuf, pu8_SpareBuf, 1);
			if(u32_UnfdRet != UNFD_ST_SUCCESS)
				goto END_WRITE_BUF;
		}
		else	// main area only
		{
			memcpy(pu8_MainBuf, buf, len);
		}
	}
	else
	{
		if((u32_WriteLen == 0) && (u16_ByteIdxofPage>=pNandDrv->u16_PageByteCnt)) // mtd skip prepare main area, default all oxff
		{
			u32_WriteLen += pNandDrv->u16_PageByteCnt;
			memset(pu8_MainBuf, 0xFF, pNandDrv->u16_PageByteCnt);
			memset(pu8_SpareBuf, 0xFF, pNandDrv->u16_SpareByteCnt);
		}
		u32_WriteLen += len;
		memcpy(pu8_SpareBuf, buf, len);

		if( u32_WriteLen == (pNandDrv->u16_PageByteCnt + pNandDrv->u16_SpareByteCnt) )
		{
			u32_UnfdRet = NC_WritePages(u32_CurRow, pu8_MainBuf, pu8_SpareBuf, 1);
			if(u32_UnfdRet != UNFD_ST_SUCCESS)
				goto END_WRITE_BUF;
		}
	}

END_WRITE_BUF:
	nand_unlock_fcie();
	return;
}

static void _unfd_nand_read_buf(struct mtd_info *mtd, u_char* const buf, int len)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U8 *pu8_MainBuf = pNandDrv->PlatCtx_t.pu8_PageDataBuf;
	U8 *pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;

	nand_lock_fcie();
	NAND_DEBUG("%s():%X, %X\n", __func__, buf, len);

	u16_ByteIdxofPage = len;

	if( len >= pNandDrv->u16_PageByteCnt )
	{
		if( len > pNandDrv->u16_PageByteCnt )
		{
			if((U32)buf >=VMALLOC_START && (U32)buf <= VMALLOC_END)
			{
				u32_UnfdRet = NC_ReadPages(u32_CurRow, pu8_MainBuf, pu8_SpareBuf, 1);
				if(u32_UnfdRet != UNFD_ST_SUCCESS)
				{
					s32_ECCStatus = -1;
					goto END_READ_BUF;
				}
				NC_CheckECC(&s32_ECCStatus);
				memcpy(buf, pu8_MainBuf, pNandDrv->u16_PageByteCnt);
				memcpy(buf+pNandDrv->u16_PageByteCnt, pu8_SpareBuf, pNandDrv->u16_SpareByteCnt);
			}
			else
			{
				pu8_MainBuf = buf;
				pu8_SpareBuf = (U8*)buf + (pNandDrv->u16_PageByteCnt);
				u32_UnfdRet = NC_ReadPages(u32_CurRow, pu8_MainBuf, pu8_SpareBuf, 1);
				if(u32_UnfdRet != UNFD_ST_SUCCESS)
				{
					s32_ECCStatus = -1;
					goto END_READ_BUF;
				}
				NC_CheckECC(&s32_ECCStatus);
			}
		}
		else
		{
			if((U32)buf >=VMALLOC_START && (U32)buf <= VMALLOC_END)
			{
				u32_UnfdRet = NC_ReadPages(u32_CurRow, pu8_MainBuf, pu8_SpareBuf, 1);
				if(u32_UnfdRet != UNFD_ST_SUCCESS)
				{
					s32_ECCStatus = -1;
					goto END_READ_BUF;
				}
				NC_CheckECC(&s32_ECCStatus);
				memcpy(buf, pu8_MainBuf, pNandDrv->u16_PageByteCnt);
			}
			else
			{
				pu8_MainBuf = buf;
				u32_UnfdRet = NC_ReadPages(u32_CurRow, pu8_MainBuf, pu8_SpareBuf, 1);
				if(u32_UnfdRet != UNFD_ST_SUCCESS)
				{
					s32_ECCStatus = -1;
					goto END_READ_BUF;
				}
				NC_CheckECC(&s32_ECCStatus);
			}
		}
	}
	else
	{
		memcpy(buf, pu8_SpareBuf, len);
	}
END_READ_BUF:
	nand_unlock_fcie();

	return;
}

static u16 _unfd_nand_read_word(struct mtd_info *mtd)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U8 *pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;
	U16 u16_tmp= 0;

	nand_lock_fcie();
	NAND_DEBUG("%s()\n", __func__);
	u16_tmp = (((U16)pu8_SpareBuf[u32_CIFDIdx+1])<<8) + ((U16)pu8_SpareBuf[u32_CIFDIdx]);
	u32_CIFDIdx+=2;
	nand_unlock_fcie();

	return u16_tmp;
}

static u_char _unfd_nand_read_byte(struct mtd_info *mtd)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U8 *pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;
	U8 u8Ret = 0;
	nand_lock_fcie();
	NAND_DEBUG("%s()\n", __func__);
	u8Ret = pu8_SpareBuf[u32_CIFDIdx];
	u32_CIFDIdx++;
	nand_unlock_fcie();
	return (u8Ret);
}

static int _unfd_nand_wait(struct mtd_info *mtd, struct nand_chip *this)
{
	int status = 0;

	nand_lock_fcie();
	NAND_DEBUG("%s()\n", __func__);
	if(u32_UnfdRet != UNFD_ST_SUCCESS)
		status |= NAND_STATUS_FAIL;
	nand_unlock_fcie();

	return (status);
}

static void _unfd_nand_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	NAND_DRIVER * pNandDrv =  (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U8 *pu8_MainBuf = pNandDrv->PlatCtx_t.pu8_PageDataBuf;
	U8 *pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;

	nand_lock_fcie();
	switch (command) {
		case NAND_CMD_READ0:
			NAND_DEBUG("%s(READ0):%X, %X\n", __func__, column, page_addr);
			u32_CurRow = page_addr;
			u32_CurCol = column;
			break;

		case NAND_CMD_READ1:
			NAND_DEBUG("%s(READ1):%X, %X\n", __func__, column, page_addr);
			pr_info("\033[31mUNFD not support READ1(CMD 0x01) now!!!\033[m\n");
			nand_die();
			break;

		case NAND_CMD_READOOB:
			NAND_DEBUG("%s(READOOB):%X, %X\n", __func__, column, page_addr);
			u32_CIFDIdx = 0;
			u16_ByteIdxofPage = 0;
			//u32_UnfdRet = NC_Read_RandomIn(page_addr, column + pNandDrv->u16_PageByteCnt, pu8_SpareBuf, pNandDrv->u16_SpareByteCnt-column);
			u32_UnfdRet = NC_ReadPages(page_addr, pu8_MainBuf, pu8_SpareBuf, 1);
			break;

		case NAND_CMD_READID:
			NAND_DEBUG("%s(READID)\n", __func__);
			u32_CIFDIdx = 0;
			u32_UnfdRet = NC_ReadID();
			memcpy((void *) pu8_SpareBuf, (const void *) pNandDrv->au8_ID, NAND_ID_BYTE_CNT);
			break;

		case NAND_CMD_PAGEPROG:
			/* sent as a multicommand in NAND_CMD_SEQIN */
			NAND_DEBUG("%s(PAGEPROG):%X, %X\n", __func__, column, page_addr);
			break;

		case NAND_CMD_ERASE1:
			NAND_DEBUG("%s(ERASE1):%X, %X\n", __func__, column, page_addr);
			u32_UnfdRet = NC_EraseBlk(page_addr);
			break;

		case NAND_CMD_ERASE2:
			NAND_DEBUG("%s(ERASE2):%X, %X\n", __func__, column, page_addr);
			break;

		case NAND_CMD_SEQIN:
			/* send PAGE_PROG command(0x1080) */
			NAND_DEBUG("%s(SEQIN):%X, %X\n", __func__, column, page_addr);
			u32_CurRow = page_addr;
			u32_CurCol = column;
			u16_ByteIdxofPage = column;
			u32_WriteLen = 0;
			break;

		case NAND_CMD_STATUS:
			NAND_DEBUG("%s(STATUS)\n", __func__);
			u32_CIFDIdx = 0;
			u32_UnfdRet = NC_ReadStatus();
			break;

		case NAND_CMD_RESET:
			NAND_DEBUG("%s(RESET)\n", __func__);
			u32_UnfdRet = NC_ResetNandFlash();
			break;

//		case NAND_CMD_STATUS_MULTI:
//			NAND_DEBUG("%s(STATUS_MULTI)\n", __func__);
//			u32_CIFDIdx = 0;
//			u32_UnfdRet = NC_ReadStatus();
//			break;

		case NAND_CMD_READSTART:
			NAND_DEBUG("%s(READSTART):%X, %X\n", __func__, column, page_addr);
			break;

		case NAND_CMD_CACHEDPROG:
			NAND_DEBUG("%s(CACHEDPROG):%X, %X\n", __func__, column, page_addr);
			pr_info("\033[31mUNFD not support CACHEPROG (CMD 0x15) now!!!\033[m\n");
			while(1);
			break;

		default:
			pr_info("_unfd_nand_cmdfunc: error, unsupported command.\n");
			break;
	}

	nand_unlock_fcie();
	return;
}

static void _unfd_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	NAND_DEBUG("%s()\n", __func__);

	// default enable
}

static int _unfd_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
	NAND_DEBUG("%s()\n", __func__);

	return 0;
}

static int _unfd_nand_correct_data(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
	int s32ECCStatus = 0;

	nand_lock_fcie();
	NAND_DEBUG("%s()\n", __func__);
	s32ECCStatus = s32_ECCStatus;
	nand_unlock_fcie();

	return s32ECCStatus;
}




/*
 * Board-specific NAND initialization.
 * - hwcontrol: hardwarespecific function for accesing control-lines
 * - dev_ready: hardwarespecific function for  accesing device ready/busy line
 * - eccmode: mode of ecc, see defines
 */

int nand_unfd_init(void)
{
	U16 u16_i;
	U32 u32_Err;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv->pPartInfo = drvNAND_get_DrvContext_PartInfo();
	memset(pNandDrv->pPartInfo, '\0', NAND_PARTITAION_BYTE_CNT);
	u32_Err = drvNAND_Init();

	if(UNFD_ST_ERR_NO_CIS == u32_Err)
	{
		nand_printf("NAND No CIS found\n");
		while(1);
		nand_printf("NAND ID:");
		for(u16_i = 0; u16_i < pNandDrv->u8_IDByteCnt; u16_i++)
				nand_printf("0x%X ", pNandDrv->au8_ID[u16_i]);
		nand_printf("\n");

		#if defined(FCIE4_DDR) && FCIE4_DDR
		if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
		{
			nand_printf("NAND Error: Detect DDR NAND but have no CIS in NAND\n");
			nand_printf("Please use command, \"ncisbl\", to write cis\n");
			return -1;
		}
		#if 0
		if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_TOGGLE)
		{
			NC_ReadToggleParamPage();
			NC_ConfigContext();
			NC_ReInit();
			NC_Config();
		}
		else if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_ONFI)
		{
			NC_ReadONFIParamPage();
			NC_ConfigContext();
			NC_ReInit();
			NC_Config();
		}
		else
		#endif
		#endif
		{
			NC_PlatformInit();

			NC_ResetFCIE(); // Reset FCIE3

			nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "NC_ResetFCIE done\n");


			// We need to config Reg 0x40 first because we we need to get nand ID first
		//	pNandDrv->u16_Reg40_Signal =
		//	(BIT_NC_WP_AUTO | BIT_NC_WP_H | BIT_NC_CE_AUTO | BIT_NC_CE_H) &
		//	~(BIT_NC_CHK_RB_EDGEn | BIT_NC_CE_SEL_MASK);
			REG_WRITE_UINT16(NC_SIGNAL, pNandDrv->u16_Reg40_Signal);


#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
			nand_enable_intr_mode();
#endif

			NC_ResetNandFlash(); // Reset NAND flash

			nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "NC_ResetNandFlash done\n");

			NC_ReadID();

			nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "NC_ReadID done\n");

/*
			if( drvNAND_CHECK_FLASH_TYPE() != 0)
			{
				nand_printf("[%s]\tUnsupported NAND Type, Need to update CIS or modify code\n",__func__);
				return -1;
			}
*/
			nand_debug(UNFD_DEBUG_LEVEL_HIGH,1,"drvNAND_CHECK_FLASH_TYPE done\n");

			NC_ConfigNandFlashContext();
			NC_Init();

		}
	}
	else if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_printf("[%s]\tdrvNAND_Init Error : %lX", __func__, (long unsigned int)u32_Err);
		return -1;
	}
	/*Config clock timing either for DDR NAND or SDR NAND */
	nand_config_clock(pNandDrv->u16_tRC);

	return 0;

}

/*
mtd init function
*/

int ms_mtd_param_init(struct mtd_info *mtd,
                          struct nand_chip *chip,
                          int *maf_id, int *dev_id,
                          const struct nand_flash_dev *type)
{
    NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

    if (!mtd->name)
        mtd->name = "edb64M-nand";
    mtd->writesize = pNandDrv->u16_PageByteCnt;
    mtd->oobsize = pNandDrv->u16_SpareByteCnt;
    mtd->erasesize = pNandDrv->u16_BlkPageCnt * pNandDrv->u16_PageByteCnt;
    chip->chipsize = (uint64_t)pNandDrv->u16_BlkCnt * (uint64_t)pNandDrv->u16_BlkPageCnt * (uint64_t)pNandDrv->u16_PageByteCnt;

    if(!mtd->writesize || !mtd->oobsize || !mtd->erasesize)
    {
        int i;
        nand_printf("Unsupported NAND Flash type is detected with ID");
        for(i = 0; i < pNandDrv->u8_IDByteCnt; i++)
            nand_printf(" 0x%X", pNandDrv->au8_ID[i]);
        nand_printf("\n");
        return  -EINVAL;
    }
    chip->onfi_version = 0;
//    chip->cellinfo = pNandDrv->au8_ID[2];
//    /* Get chip options, preserve non chip based options */
//    chip->options &= ~NAND_CHIPOPTIONS_MSK;
//
//    /*
//	 ** Set chip as a default. Board drivers can override it, if necessary
// 	**/
//    chip->options |= NAND_NO_AUTOINCR;

    /* Get chip options */
    //chip->options |= type->options;
    chip->ecc.strength=1;
    if(pNandDrv->u8_WordMode)
        chip->options |= NAND_BUSWIDTH_16;
    /* Calculate the address shift from the page size */
    chip->page_shift = ffs(mtd->writesize) - 1;
    /* Convert chipsize to number of pages per chip -1. */
    chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;
    chip->bbt_erase_shift = chip->phys_erase_shift =
        ffs(mtd->erasesize) - 1;
    if (chip->chipsize & 0xffffffff)
        chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else
        chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32)) + 31;
    //chip->blocknum = chip->chipsize >> chip->phys_erase_shift;
    /* set bbt block number to 0.8% of total blocks, or blocks * (2 / 256) */
    //mtd->bbt_block_num = ((chip->blocknum >> 8) * 2);

    /* Set the bad block position */
    chip->badblockpos = mtd->writesize > 512 ?
        NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;
//    chip->erase = single_erase;
	chip->options |= NAND_SKIP_BBTSCAN;
	chip->badblockbits = 8;
    return 0;

}

/*
+------------------------------------------------------------------------------
| FUNCTION	: ms_unfd_probe
+------------------------------------------------------------------------------
| DESCRIPTION : The generic driver interface function which called for initial unfd
|
| RETURN	  : zero on success, else positive error code
+------------------------------------------------------------------------------
| Variable Name	  |IN |OUT|					Usage
|--------------------+---+---+-------------------------------------------------
| pdev				|x  |	| platform_device struct point
|--------------------+---+---+-------------------------------------------------
*/

static int ms_unfd_probe(struct platform_device *pdev)
{
	struct nand_chip * nand;
	struct mtd_partition *parts;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	PARTITION_INFO_t *pPartInfo;
	PARTITION_RECORD_t *pRecord;
	int NumOfPart;
	U32 u32_BlkSize;
	U32 u32_Err;
	int	err;
	U8 u8_PartNo;
	U16 u16_tmp;
	U16 u16_oob_poi;
	U16 u16_i;
	U16 u16_SectorSpareByteCnt;
	U16 u16_LastPartType;

#if defined(CONFIG_OF)
	clkdata = kzalloc(sizeof(struct clk_data), GFP_KERNEL);
	if(!clkdata)
	{
		printk(KERN_CRIT"Unable to allocate nand clock data\n");
		return -ENOMEM;
	}

	clkdata->num_parents = of_clk_get_parent_count(pdev->dev.of_node);
        if(clkdata->num_parents > 0)
        {
            clkdata->clk_fcie = kzalloc(sizeof(struct clk*) * clkdata->num_parents, GFP_KERNEL);
        }
        else
        {
            printk(KERN_ERR "Unable to get nand clk count from dts\n");
            return -ENODEV;
        }

	for(u16_i = 0 ; u16_i < clkdata->num_parents; u16_i ++)
	{
		clkdata->clk_fcie[u16_i] = of_clk_get(pdev->dev.of_node, u16_i);
		if(IS_ERR(clkdata->clk_fcie[u16_i]))
		{
			printk(KERN_CRIT"Unable to get nand clk from dts\n");
			return -ENODEV;
		}
	}
//  clkdata->clk_ecc = of_clk_get(pdev->dev.of_node, 1);
//	if(IS_ERR(clkdata->clk_fcie) || IS_ERR(clkdata->clk_ecc))
//	{
//		printk(KERN_CRIT"Unable to get nand clk from dts\n");
//		return -ENODEV;
//	}

#endif

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	fcie_irq = platform_get_irq(pdev, 0);
	if (fcie_irq < 0)
		return -ENXIO;

	err = request_irq(fcie_irq, NC_FCIE_IRQ, IRQF_SHARED, DRIVER_NAME, (void *)pdev);
	if (err)
		return err;
#endif

	nand_lock_fcie();
	//u32_Err = drvNAND_Init();
	u32_Err = nand_unfd_init();
	nand_unlock_fcie();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		pr_info("%s: NAND Init fail, ErrCode: %lx\n",__FUNCTION__,(long unsigned int)u32_Err);
		return -ENODEV;
	}

	nand_mtd = kzalloc(sizeof(struct mtd_info) + sizeof (struct nand_chip), GFP_KERNEL);
	if (!nand_mtd) {
		pr_info ("%s: Unable to allocate NAND MTD device structure.\n",__FUNCTION__);
		return -ENOMEM;
	}

	/* Get pointer to private data */
	nand = (struct nand_chip *) (&nand_mtd[1]);

	nand_mtd->priv = nand;
	nand_mtd->name = dev_name(&pdev->dev);
	nand_mtd->owner = THIS_MODULE;

	/* please refer to include/linux/nand.h for more info. */

	nand->dev_ready						= _unfd_nand_device_ready;
	nand->cmd_ctrl						= _unfd_nand_hwcontrol;
	nand->ecc.mode						= NAND_ECC_HW;

	nand->ecc.size						= pNandDrv->u16_PageByteCnt;
	nand->ecc.bytes						= pNandDrv->u16_ECCCodeByteCnt * pNandDrv->u16_PageSectorCnt;

#if 0
	if( pNandDrv->u16_PageByteCnt == 2048 )
	{
			nand->ecc.layout				= &unfd_nand_oob_64;
			_unfd_nand_bbt_descr.offs	= 0;
		}
		else if( pNandDrv->u16_PageByteCnt == 512 )
		{
		nand->ecc.layout				= &unfd_nand_oob_16;
		}
#else
		u16_SectorSpareByteCnt = (pNandDrv->u16_SpareByteCnt/pNandDrv->u16_PageSectorCnt);
		unfd_nand_oob_custom.eccbytes = pNandDrv->u16_ECCCodeByteCnt * pNandDrv->u16_PageSectorCnt;
		for(u16_tmp=0 ; u16_tmp<pNandDrv->u16_PageSectorCnt ; u16_tmp++)
		{
			u16_oob_poi = ((u16_tmp+1)*u16_SectorSpareByteCnt)-pNandDrv->u16_ECCCodeByteCnt;
			for(u16_i=0 ; u16_i<pNandDrv->u16_ECCCodeByteCnt ; u16_i++)
			{
				unfd_nand_oob_custom.eccpos[(u16_tmp*pNandDrv->u16_ECCCodeByteCnt)+u16_i] = u16_oob_poi++;
			}

			if( u16_tmp == 0)
			{
				unfd_nand_oob_custom.oobfree[u16_tmp].offset = 2;
				unfd_nand_oob_custom.oobfree[u16_tmp].length= u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt - 2;
			}
			else
			{
				unfd_nand_oob_custom.oobfree[u16_tmp].offset = u16_tmp*u16_SectorSpareByteCnt;
				unfd_nand_oob_custom.oobfree[u16_tmp].length= u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt;
			}
		}
		nand->ecc.layout = &unfd_nand_oob_custom;
		//dump_mem((unsigned char *) &unfd_nand_oob_custom, sizeof(unfd_nand_oob_custom));
#endif

	//if( pNandDrv->u16_PageByteCnt > 512 )
//		nand->options					= NAND_NO_AUTOINCR;

	nand->options  |= NAND_SKIP_BBTSCAN;
//	nand->options  |= NAND_NO_READRDY;
	nand->options  |= NAND_NO_SUBPAGE_WRITE;
	if(pNandDrv->u8_WordMode)
		nand->options  |= NAND_BUSWIDTH_16;

	nand->waitfunc		= _unfd_nand_wait;
	nand->read_byte		= _unfd_nand_read_byte;
	nand->read_word		= _unfd_nand_read_word;
	nand->read_buf		= _unfd_nand_read_buf;
	nand->write_buf		= _unfd_nand_write_buf;
	nand->chip_delay	= 0;	//@FIXME: unite: us, please refer to nand_base.c 20us is default.

	nand->ecc.hwctl						= _unfd_nand_enable_hwecc;
	nand->ecc.correct					= _unfd_nand_correct_data;
	nand->ecc.calculate					= _unfd_nand_calculate_ecc;
	nand->cmdfunc						= _unfd_nand_cmdfunc;
	_unfd_nand_bbt_descr.offs = ((pNandDrv->u32_Config>>1) & 0x07);
	nand->badblock_pattern				= &_unfd_nand_bbt_descr; //using default badblock pattern.
	nand->bbt_td						= &_unfd_bbt_main_descr;
	nand->bbt_md						= &_unfd_bbt_mirror_descr;

	nand->mtd_param_init = ms_mtd_param_init;

	pr_info("%s: Before nand_scan()...\n",__FUNCTION__);

	if(nand_scan(nand_mtd, 1))
	{
		pr_info("%s: can't register NAND\n",__FUNCTION__);
		kfree(nand_mtd);
		return -ENXIO;
	}
#ifdef CONFIG_MTD_PARTITIONS
	err = parse_mtd_partitions(nand_mtd, part_probes, &parts, 0);
	if (err > 0)
	{
		pr_info("parse_mtd_partitions ok\n");
		add_mtd_partitions(nand_mtd, parts, err);
	}
	else
	{
		pPartInfo = pNandDrv->pPartInfo;
		u32_BlkSize = (U32)pNandDrv->u16_PageByteCnt*(U32)pNandDrv->u16_BlkPageCnt;
		NumOfPart = 0;
		pRecord = pPartInfo->records;
		u16_LastPartType = 0xFFFF;
		u8_PartNo = 0;
		while(pRecord - pPartInfo->records < pPartInfo->u16_PartCnt)
		{
			if((pRecord->u16_PartType&UNFD_LOGI_PART) == UNFD_LOGI_PART)
			{
				break;
			}

			if( u16_LastPartType == pRecord->u16_PartType)
				u8_PartNo++;
			else
				u8_PartNo = 0;

			u16_LastPartType = pRecord->u16_PartType;

			switch(pRecord->u16_PartType)
			{
				case UNFD_PART_IPL_CUST:
					if(u8_PartNo == 0)
						partition_info[NumOfPart].name = "IPL_CUST0";
					else
						partition_info[NumOfPart].name = "IPL_CUST1";
					break;
				case UNFD_PART_BOOTLOGO:
					partition_info[NumOfPart].name = "BOOTLOGO";
					break;
				case UNFD_PART_IPL:
					if(u8_PartNo == 0)
						partition_info[NumOfPart].name = "IPL0";
					else
						partition_info[NumOfPart].name = "IPL1";
					break;
				case UNFD_PART_OS:
					partition_info[NumOfPart].name = "OS";
					break;
				case UNFD_PART_CUS:
					partition_info[NumOfPart].name = "CUS";
					break;
				case UNFD_PART_UBOOT:
					if(u8_PartNo == 0)
						partition_info[NumOfPart].name = "UBOOT0";
					else
						partition_info[NumOfPart].name = "UBOOT1";
					break;
				case UNFD_PART_SECINFO:
					partition_info[NumOfPart].name = "SECINFO";
					break;
				case UNFD_PART_OTP:
					partition_info[NumOfPart].name = "OTP";
					break;
				case UNFD_PART_RECOVERY:
					partition_info[NumOfPart].name = "RECOVERY";
					break;
				case UNFD_PART_E2PBAK:
					partition_info[NumOfPart].name = "E2PBAK";
					break;
				case UNFD_PART_NVRAMBAK:
					partition_info[NumOfPart].name = "NVRAMBAK";
					break;
				case UNFD_PART_NPT:
					partition_info[NumOfPart].name = "NPT";
					break;
				case UNFD_PART_ENV:
					partition_info[NumOfPart].name = "ENV";
					break;
				default:
					partition_info[NumOfPart].name = "UNKNOWN";
					break;
			}
			partition_info[NumOfPart].offset = pRecord->u16_StartBlk*u32_BlkSize;
			partition_info[NumOfPart].size = (pRecord->u16_BlkCnt+pRecord->u16_BackupBlkCnt)*u32_BlkSize;
			partition_info[NumOfPart].mask_flags = 0;
			pr_info("%s:%llX, %llX\n", partition_info[NumOfPart].name,
									partition_info[NumOfPart].offset,
									partition_info[NumOfPart].size);

			NumOfPart++;
			pRecord++;
		}
		partition_info[NumOfPart].name = "UBI";
		partition_info[NumOfPart].offset = pRecord->u16_StartBlk*u32_BlkSize;
		partition_info[NumOfPart].size = (pNandDrv->u16_BlkCnt-pRecord->u16_StartBlk)*u32_BlkSize;
		partition_info[NumOfPart].mask_flags = 0;
		pr_info("%s:%llX, %llX\n", partition_info[NumOfPart].name,
									partition_info[NumOfPart].offset,
									partition_info[NumOfPart].size);
		NumOfPart++;

		pr_info("parse_mtd_partitions from CIS ok\n");
		add_mtd_partitions(nand_mtd, partition_info, NumOfPart);
	}
#else
	add_mtd_device(mtd);
#endif

	return 0;
}

static int ms_unfd_remove(struct platform_device *pdev)
{

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	free_irq(fcie_irq, pdev);
#endif
	nand_release(nand_mtd);
	kfree(nand_mtd);

	return 0;
}
#ifdef CONFIG_PM
static int ms_unfd_suspend(struct platform_device *dev, pm_message_t state)
{
#if defined(CONFIG_OF)
	//disable clock here with clcok framework.
	//do nothing here clock is disable when nand_unlock_fcie
#endif
	if(mutex_trylock(&FCIE3_mutex))
	{
        printk("mstar_nand_suspend\n");
		return 0;
	}
	else
	{
        printk("mstar_nand_suspend BUSY!!!\n");
        return -EBUSY;
	}
}
static int ms_unfd_resume(struct platform_device *dev)
{
	U32 u32_Err;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	int ret = 0;

	printk("mstar_nand_resume\n");

#if defined(CONFIG_OF)
	//enable clock here with clock framework
	{
		int i;
		for(i = 0 ;i < clkdata->num_parents; i ++)
			clk_prepare_enable(clkdata->clk_fcie[i]);
	}
#endif
	nand_clock_setting(pNandDrv->u32_Clk);
	nand_pads_switch(pNandDrv->u8_PadMode);

	u32_Err = NC_ResetFCIE();
    if(u32_Err == UNFD_ST_SUCCESS)
    {
		NC_Config();

        u32_Err = NC_ResetNandFlash();
        if(u32_Err != UNFD_ST_SUCCESS)
        {
            ret = -1;
        }
    }
    else
    {
        ret = -1;
    }

	nand_unlock_fcie();
	return ret;
}

#endif

#if defined(CONFIG_OF)
static struct of_device_id ms_nand_dt_ids[] = {
	{
		.compatible = "ms-nand"
	},
	{},
};
#endif

static struct platform_driver ms_unfd_driver =
{
	.driver =
	{
		.name  = DRIVER_NAME,
		.owner = THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table = ms_nand_dt_ids,
#endif
	},
	.probe	= ms_unfd_probe,
	.remove	= ms_unfd_remove,
#ifdef CONFIG_PM
	.suspend = ms_unfd_suspend,
	.resume = ms_unfd_resume,
#endif
};

//xxx tomodify
#define IRQ_MSTAR_NAND 56
static struct resource ms_unfd_device_resource[] =
{
	[0] = {
		.start = 0xA0004000,
		.end	= 0xA0004800 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MSTAR_NAND,
		.end	= IRQ_MSTAR_NAND,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device ms_unfd_device =
{

	.name			 = "ms-nand",
	.id				= -1,
	.num_resources	= ARRAY_SIZE(ms_unfd_device_resource),
	.resource		 = ms_unfd_device_resource,
};


extern int Chip_Boot_Get_Dev_Type(void);
bool ms_NAND_has_device(void)
{

	if(MS_BOOT_DEV_EMMC!= (MS_BOOT_DEV_TYPE)Chip_Boot_Get_Dev_Type())
	{
		return true;
	}
	else
	{
		return false;
	}

}

#if !defined(CONFIG_OF)
static int __init ms_unfd_device_init(void)
{

	if(!ms_NAND_has_device())
	{
		pr_info("[%s] skipping device initialization\n",ms_unfd_device.name);
		return -1;
	}

	pr_info("[%s] device initializing\n", ms_unfd_device.name);

	return platform_device_register(&ms_unfd_device);
}
#endif

static int __init ms_unfd_driver_init(void)
{

	if(!ms_NAND_has_device())
	{
		pr_info("[%s] skipping driver initialization\n",ms_unfd_driver.driver.name);
		return -1;
	}
#ifdef CONFIG_MS_NAND_MODULE
#if !defined(CONFIG_OF)
	ms_unfd_device_init();
#endif
#endif

	pr_info("[%s] driver initializing\n", ms_unfd_driver.driver.name);

	return platform_driver_register(&ms_unfd_driver);

}

static void __exit ms_unfd_driver_cleanup(void)
{
	platform_driver_unregister(&ms_unfd_driver);
}


#ifndef CONFIG_MS_NAND_MODULE
#if !defined(CONFIG_OF)
subsys_initcall(ms_unfd_device_init);
#endif
#endif
module_init(ms_unfd_driver_init);
module_exit(ms_unfd_driver_cleanup);

MODULE_ALIAS(DRIVER_NAME);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);

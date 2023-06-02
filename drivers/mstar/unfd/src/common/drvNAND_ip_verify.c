//=======================================================================
//  MStar Semiconductor - Unified Nand Flash Driver
//
//  drvNAND_IPVerify.c - Storage Team, 2009/08/20
//
//  Design Notes:
//    1. 2009/08/22 - Unified IP Verification code
//
//=======================================================================

#include "drvNAND.h"
#include "drvNAND_device.h"

#if IF_IP_VERIFY
//========================================================
// exposed function
//========================================================
U32  drvNAND_IPVerify_Main(void);
U32  NCTEST_TestReadWriteSpeed(void);

void NCTEST_EarseAllBlk(void);
U32  NCTEST_EraseAllGoodBlks(void);
void nand_dump_FMap(void);
void nand_LUT_Test(void);
void NCTEST_DataCompare(U8 *pu8_buf0, U8 *pu8_buf1, U32 u32_byteCnt);
void NCTEST_LogiWR_TestWBQ(void);
void NCTEST_LogiWR_TestFBQ(void);
void NCTEST_LogiWR_Stress(U32 u32_TotalSectorCnt);
void NCTEST_LogiWR_Once(U32 u32_StartSectorIdx, U32 u32_SectorCnt);
#if 1
#define LOGI_STRESS_TEST_SECTOR_CNT  0x136  // 8MB
#define LOGI_STRESS_TEST_SECTOR_ADR  0x4500 // 32MB
#else
#define LOGI_STRESS_TEST_SECTOR_CNT  0x101 // 8MB
#define LOGI_STRESS_TEST_SECTOR_ADR  0x10000 // 32MB
#endif
volatile U8 au8_TestBuf[LOGI_STRESS_TEST_SECTOR_CNT<<9]; // [CAUTION]

static const U8 sgau8_ID_HardCode[] = NAND_DEVICE_ID;
//static U8* const pu8_string = "abcde";

#define ECC_TEST_PATTERN             0xFF

// FIXME:
#define RTC_REG_BASE_ADDR 0

#define RTC_CTRL        GET_REG_ADDR(RTC_REG_BASE_ADDR, 0x40)
#define BIT_EN          BIT1
#define BIT_TRIGGER     BIT4
#define RTC_SECOND      GET_REG_ADDR(RTC_REG_BASE_ADDR, 0x48)

static void count_down(int sec)
{
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Delay for %d seconds for triggering LA:", sec);
	while (sec)
	{
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, " %d", sec);
		nand_hw_timer_delay(1000000);
		sec--;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, " go!\n");
}

static U32 ECCSectorSetup(void)
{
	U32 u32_Err, u32_j;
	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECCSectorSetup 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for (u32_j = 0; u32_j < pNandDrv->u16_SectorByteCnt; u32_j++)
	{
		au8_W[u32_j] = ECC_TEST_PATTERN;
		au8_R[u32_j] = ~au8_W[u32_j];
	}
	for (u32_j = 0; u32_j < pNandDrv->u16_SectorSpareByteCnt; u32_j++)
	{
		au8_WSpare[u32_j] = ECC_TEST_PATTERN;
		au8_RSpare[u32_j] = ~au8_WSpare[u32_j];
	}

	u32_Err = NC_WriteSectors(0, 0, au8_W, au8_WSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECCSectorSetup 1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECCDataReadBack(U32 u32_TestRow, U32 u32_DebugLevel)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_ByteIdx;
	U16 u16_Reg51, u16_Reg52, u16_Reg53;
	volatile U8  *au8_R = (U8*)DMA_R_ADDR, *au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	/* read back */
	u32_Err = NC_ReadPages(u32_TestRow, (U8*)au8_R, (U8*)au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECCDataReadBack 6, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* ECC status */
	REG_READ_UINT16(NC_ECC_STAT0, u16_Reg51);
	REG_READ_UINT16(NC_ECC_STAT1, u16_Reg52);
	REG_READ_UINT16(NC_ECC_STAT2, u16_Reg53);
	nand_debug(u32_DebugLevel, 1, "Reg51: 0x%04x  Reg52: 0x%04x  Reg53: 0x%04x\n",
			   u16_Reg51, u16_Reg52, u16_Reg53);

	/* check data */
	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
	{
		if (au8_R[u32_ByteIdx] != ECC_TEST_PATTERN)
		{
			nand_debug(u32_DebugLevel, 1, "Error: au8_R[0x%04x]: 0x%02x != 0x%02x\n",
					u32_ByteIdx, au8_R[u32_ByteIdx], ECC_TEST_PATTERN);
		}
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
	{
		if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
				pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
		{
			u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
			continue;
		}

		if (au8_RSpare[u32_ByteIdx] != ECC_TEST_PATTERN)
		{
			nand_debug(u32_DebugLevel, 1, "Error: au8_RSpare[0x%04x]: 0x%02x != 0x%02x\n",
					u32_ByteIdx, au8_RSpare[u32_ByteIdx], ECC_TEST_PATTERN);
		}
	}

#if 0
	{
		U8 au8_Tmp[0x40], u8_i;
		NC_GetCIFD(au8_Tmp, 0, 0x40);
		for (u8_i = 0; u8_i < 0x40; u8_i++)
		{

			if (0 == (u8_i % 0x10))
				nand_debug(0, 1, "\n");

			nand_debug(0, 1, "0x%02x ", au8_Tmp[u8_i]);
		}
		nand_debug(0, 1, "\n");
	}
#endif

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECCSectorReadBack(U32 u32_PageIdx)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_ByteIdx;
	U16 u16_Reg51, u16_Reg52, u16_Reg53;
	volatile U8  *au8_R = (U8*)DMA_R_ADDR, *au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	/* read back */
	u32_Err = NC_ReadSectors(u32_PageIdx, 0, (U8*)au8_R, (U8*)au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECCSectorReadBack 6, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* ECC status */
	REG_READ_UINT16(NC_ECC_STAT0, u16_Reg51);
	REG_READ_UINT16(NC_ECC_STAT1, u16_Reg52);
	REG_READ_UINT16(NC_ECC_STAT2, u16_Reg53);
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Reg51: 0x%04x  Reg52: 0x%04x  Reg53: 0x%04x\n",
			   u16_Reg51, u16_Reg52, u16_Reg53);

	/* check data */
	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SectorByteCnt; u32_ByteIdx++)
	{
		if (au8_R[u32_ByteIdx] != ECC_TEST_PATTERN)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err: au8_R[0x%04x]: 0x%02x != 0x%02x\n",
					u32_ByteIdx, au8_R[u32_ByteIdx], ECC_TEST_PATTERN);
		}
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SectorSpareByteCnt; u32_ByteIdx++)
	{
		if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
				pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
		{
			u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
			continue;
		}

		if (ECC_TEST_PATTERN != au8_RSpare[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err: au8_RSpare[0x%04x]: 0x%02x != 0x%02x\n",
					u32_ByteIdx, au8_RSpare[u32_ByteIdx], ECC_TEST_PATTERN);
		}
	}

	return UNFD_ST_SUCCESS;
}

static void NCTEST_Check_DBus(void)
{
	nand_pads_switch(1);

	/* random data out to check if data bus ok*/
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DOUT);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 16);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); /*offset 0*/

	REG_WRITE_UINT16(NC_CIFD_ADDR(0), 0xFFFF);
	REG_WRITE_UINT16(NC_CIFD_ADDR(1), 0x0100);
	REG_WRITE_UINT16(NC_CIFD_ADDR(2), 0x0402);
	REG_WRITE_UINT16(NC_CIFD_ADDR(3), 0x1008);
	REG_WRITE_UINT16(NC_CIFD_ADDR(4), 0x4020);
	REG_WRITE_UINT16(NC_CIFD_ADDR(5), 0xFF80);
	REG_WRITE_UINT16(NC_CIFD_ADDR(6), 0x5555);
	REG_WRITE_UINT16(NC_CIFD_ADDR(7), 0xAAAA);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_CIFD_ACCESS | BIT_NC_JOB_START);
	nand_hw_timer_delay(HW_TIMER_DELAY_1ms);
}

static U32 NCTEST_RW_CIFD(void)
{
	U32 u32_Idx;
	volatile U16 u16_Reg;

	for (u32_Idx = 0; u32_Idx < NC_CIFD_BYTE_CNT >> 1; u32_Idx++)
		REG_WRITE_UINT16(NC_CIFD_ADDR(u32_Idx), (U16)(u32_Idx + (u32_Idx << 8)));

	for (u32_Idx = 0; u32_Idx < NC_CIFD_BYTE_CNT >> 1; u32_Idx++)
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_Idx), u16_Reg);
		if (u16_Reg != (U16)(u32_Idx + (u32_Idx << 8)))
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "CIFD W/R Err: Idx: %u, W: 0x%04x, R: 0x%04x\n",
					u32_Idx, (U16)(u32_Idx + (u32_Idx << 8)), u16_Reg);
			return UNFD_ST_ERR_NO_NFIE;
		}
	}

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_AccessCIFD(void)
{
	U16 u16_i, u16_j, u16_Tmp;
	U8 au8_W[NC_CIFD_BYTE_CNT], au8_R[NC_CIFD_BYTE_CNT];

	if (NCTEST_RW_CIFD() != UNFD_ST_SUCCESS)
		return UNFD_ST_ERR_NO_NFIE;

	for (u16_i = 0; u16_i < NC_CIFD_BYTE_CNT; u16_i++)
	{
		for (u16_j = u16_i; u16_j < NC_CIFD_BYTE_CNT; u16_j++)
		{
			au8_W[u16_j] = u16_j + 1;
			au8_R[u16_j] = ~au8_W[u16_j];
		}

		NC_SetCIFD(&au8_W[u16_i], u16_i, NC_CIFD_BYTE_CNT - u16_i);
		NC_GetCIFD(&au8_R[u16_i], u16_i, NC_CIFD_BYTE_CNT - u16_i);

		for (u16_j = u16_i; u16_j < NC_CIFD_BYTE_CNT; u16_j++)
		{
			if (au8_W[u16_j] != au8_R[u16_j])
			{
				REG_READ_UINT16(NC_CIFD_ADDR(u16_j >> 1), u16_Tmp);
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR 1: Idx_i: 0x%x  Idx_j: 0x%x  Val: 0x%x  W: 0x%x  R: 0x%x\n",
						u16_i, u16_j, u16_Tmp, au8_W[u16_j], au8_R[u16_j]);

				return UNFD_ST_ERR_NO_NFIE;
			}
		}
	}

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ReadID(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U8 u8_Idx;

	u32_Err = NC_ReadID();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_ReadID fail, ErrCode: 0x%08x\n", u32_Err);
		return UNFD_ST_ERR_UNKNOWN_ID;
	}

	if (pNandDrv->u8_IDByteCnt != NAND_DEVICE_ID_LEN)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR! ID Byte Cnt incorrect\n");
		return UNFD_ST_ERR_UNKNOWN_ID;
	}

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ID Byte Cnt: %d\n", pNandDrv->u8_IDByteCnt);

	for (u8_Idx = 0; u8_Idx < NAND_DEVICE_ID_LEN; u8_Idx++)
	{
		if ( pNandDrv->au8_ID[u8_Idx] != sgau8_ID_HardCode[u8_Idx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ID[%u]: R: 0x%02x != 0x%02x\n", u8_Idx,
					pNandDrv->au8_ID[u8_Idx], sgau8_ID_HardCode[u8_Idx]);
			return UNFD_ST_ERR_UNKNOWN_ID;
		}
	}

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ReadID ok\n");

	return UNFD_ST_SUCCESS;
}

#define NCTEST_RANDOM_BYTE_CNT  0x40
static U32 NCTEST_RandomRW(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_PageIdx, u32_ByteIdx;
	U8  au8_W[NCTEST_RANDOM_BYTE_CNT], au8_R[NCTEST_RANDOM_BYTE_CNT];

	u32_PageIdx = pNandDrv->u16_BlkPageCnt - 1;

	/* erase the first blk */
	u32_Err = NC_EraseBlk(u32_PageIdx);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RandomRW 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < NCTEST_RANDOM_BYTE_CNT; u32_ByteIdx++)
	{
		au8_W[u32_ByteIdx] = 0xFF-(u32_ByteIdx + 1);
		au8_R[u32_ByteIdx] = ~au8_W[u32_ByteIdx];
	}

	u32_Err = NC_Write_RandomOut(u32_PageIdx, 2, au8_W, NCTEST_RANDOM_BYTE_CNT);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RandomRW 1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_Read_RandomIn(u32_PageIdx, 2, au8_R, NCTEST_RANDOM_BYTE_CNT);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RandomRW 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < NCTEST_RANDOM_BYTE_CNT; u32_ByteIdx++)
	{
		if (au8_W[u32_ByteIdx] != au8_R[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RandomRW 3, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
					u32_ByteIdx + 2, au8_W[u32_ByteIdx], au8_R[u32_ByteIdx]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	}

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_VerifyDMAAddress(void)
{
	U8  *au8_W, *au8_R;

	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;

	au8_W[0] = 'H';  au8_W[1] = 'i';
	au8_W[2] = ',';  au8_W[3] = 0;

	au8_R[0] = 'H';  au8_R[1] = 'i';
	au8_R[2] = 'l';  au8_R[3] = 'l'; 
	au8_R[4] = '\n'; au8_R[5] = 0;

	nand_debug(0, 1, "Hi, DMA Addr. Re:\"%s %s\"\r\n", au8_W, au8_R);
	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_RWSinglePages_RM(U32 u32_StartRowIdx, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_PageIdx, u32_SectorIdx, u32_ByteIdx;
	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* erase the first blk */
	u32_Err = NC_EraseBlk(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_RM 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	/* W/R/C each page */
	for (u32_PageIdx = 0; u32_PageIdx < u32_PageCnt; u32_PageIdx++)
	{
		/* erase */
		if (u32_PageIdx && !((u32_StartRowIdx + u32_PageIdx) & pNandDrv->u16_BlkPageCntMask))
		{
			u32_Err = NC_EraseBlk(u32_StartRowIdx + u32_PageIdx);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_RM 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}
		}

		/* init test pattern */
		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
		{
			if (pNandDrv->u8_IfECCTesting)
				au8_W[u32_ByteIdx] = ECC_TEST_PATTERN;
			else
				au8_W[u32_ByteIdx] = u32_ByteIdx + 1;

			au8_R[u32_ByteIdx] = ~au8_W[u32_ByteIdx];
		}

		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
		{
			if (pNandDrv->u8_IfECCTesting)
				au8_WSpare[u32_ByteIdx] = ECC_TEST_PATTERN;
			else
				au8_WSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;

			au8_RSpare[u32_ByteIdx] = ~au8_WSpare[u32_ByteIdx];
		}

		u32_Err = NC_WritePage_RIUMode(u32_StartRowIdx + u32_PageIdx, au8_W, au8_WSpare);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_RM 2, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}

		/* read test pattern */
		for (u32_SectorIdx = 0; u32_SectorIdx < pNandDrv->u16_PageSectorCnt; u32_SectorIdx++)
		{
			u32_Err = NC_ReadSector_RIUMode(u32_StartRowIdx + u32_PageIdx, u32_SectorIdx,
					au8_R + (u32_SectorIdx << pNandDrv->u8_SectorByteCntBits),
					//au8_RSpare + (u32_SectorIdx << pNandDrv->u8_SectorSpareByteCntBits));
					au8_RSpare + (u32_SectorIdx * pNandDrv->u16_SectorSpareByteCnt));
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_RM 3, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}
		}

		/* compare test pattern */
		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
		{
			if (au8_W[u32_ByteIdx] != au8_R[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_RM 4, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_W[u32_ByteIdx], au8_R[u32_ByteIdx]);
				return UNFD_ST_ERR_DATA_CMP_FAIL;
			}
		}

		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
		{

			if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
					pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
			{
				u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
				continue;
			}

			if (au8_WSpare[u32_ByteIdx] != au8_RSpare[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_RM 5, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_WSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
				return UNFD_ST_ERR_DATA_CMP_FAIL;
			}
		}
	}

	return UNFD_ST_SUCCESS;
}


#if defined(__VER_UNFD_FTL__)
static void NCTEST_DumpMG(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err, u32_i;
	U16 u16_PBA, u16_page;

	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* setup buffers */
	au8_R = (U8*)DMA_R_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for(u16_PBA=3; u16_PBA<8; u16_PBA++)
	{
		u16_page = 0;
		
		do
		{
			u32_Err = NC_ReadSectors((u16_PBA<<pNandDrv->u8_BlkPageCntBits)+u16_page, 
				                   0, au8_R, au8_RSpare, 1);
		    if (u32_Err != UNFD_ST_SUCCESS)
		        nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR.1: ErrCode: 0x%08x\n", u32_Err);
		
            if(0xFF==au8_RSpare[0] && UNFD_FTL_BLK_ID_MG==au8_RSpare[1] && 
			  u16_page<pNandDrv->u16_BlkPageCnt)
		    {
			    nand_debug(0,0,"PBA:%X  Page:%X\n", u16_PBA, u16_page);
			    dump_mem(au8_R, pNandDrv->u16_SectorByteCnt);
			    nand_debug(0,0,"\n");
			    dump_mem(au8_RSpare, pNandDrv->u16_SectorSpareByteCnt);
            }

			u16_page++;

		}while(0xFF==au8_RSpare[0] && UNFD_FTL_BLK_ID_MG==au8_RSpare[1] && 
			  u16_page<pNandDrv->u16_BlkPageCnt);
				
	}

}
#endif

static void NCTEST_DumpPages(U32 u32_StartRowIdx, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_PageIdx;
	U8  *au8_R, *au8_RSpare;

	nand_debug(0,1,"\n========================== \n");
	nand_debug(0,1,"StartRow: %X  PageCnt %X \n", u32_StartRowIdx, u32_PageCnt);
	nand_debug(0,1,"========================== \n");
	/* setup buffers */
	au8_R = (U8*)DMA_R_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for (u32_PageIdx = 0; u32_PageIdx < u32_PageCnt; u32_PageIdx++)
	{
		/* read test pattern */
		u32_Err = NC_ReadPages(u32_StartRowIdx + u32_PageIdx, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR.1: ErrCode: 0x%08x\n", u32_Err);
			//return u32_Err;
		}

		nand_debug(0,0,"\n");
		dump_mem(au8_R, pNandDrv->u16_PageByteCnt);
		nand_debug(0,0,"\n");
		dump_mem(au8_RSpare, pNandDrv->u16_SpareByteCnt);		
	}

}

static U32 NCTEST_RWSinglePages(U32 u32_StartRowIdx, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_PageIdx, u32_ByteIdx;

	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* erase the first blk */
	u32_Err = NC_EraseBlk(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	/* W/R/C each page */
	for (u32_PageIdx = 0; u32_PageIdx < u32_PageCnt; u32_PageIdx++)
	{
		/* erase */
		if (u32_PageIdx && !((u32_StartRowIdx + u32_PageIdx) & pNandDrv->u16_BlkPageCntMask))
		{
			u32_Err = NC_EraseBlk(u32_StartRowIdx + u32_PageIdx);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}
		}

		/* init test pattern */
		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
		{
			if (pNandDrv->u8_IfECCTesting)
				au8_W[u32_ByteIdx] = ECC_TEST_PATTERN;
			else
				au8_W[u32_ByteIdx] = u32_ByteIdx + 1;

			au8_R[u32_ByteIdx] = ~au8_W[u32_ByteIdx];
            #if 0
			if(u32_ByteIdx<0x20)
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "set: [%2u]: adr:%X val:%02X  adr:%X val:%02X\n", 
				    u32_ByteIdx, &au8_W[u32_ByteIdx], au8_W[u32_ByteIdx], 
				    &au8_R[u32_ByteIdx], au8_R[u32_ByteIdx]);
			#endif
		}

		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
		{
			if (pNandDrv->u8_IfECCTesting)
				au8_WSpare[u32_ByteIdx] = ECC_TEST_PATTERN;
			else
				au8_WSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;

			au8_RSpare[u32_ByteIdx] = ~au8_WSpare[u32_ByteIdx];
		}

		u32_Err = NC_WritePages(u32_StartRowIdx + u32_PageIdx, au8_W, au8_WSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 2, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}

		/* read test pattern */
		u32_Err = NC_ReadPages(u32_StartRowIdx + u32_PageIdx, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 3, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}
		// [CAUTION]: if the above 2 of nand_translate_DMA_address_Ex, data area must be handled later.
		//nand_translate_DMA_address_Ex((U32)sgau8_RBuf, pNandDrv->u16_PageByteCnt);
		//nand_translate_DMA_address_Ex((U32)sgau8_RSpareBuf, pNandDrv->u16_PageByteCnt);

		/* compare test pattern */
		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
		{
			if (au8_W[u32_ByteIdx] != au8_R[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages, data, Row: 0x%08X, Col: 0x%04X, W: 0x%02X, R: 0x%02X\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_W[u32_ByteIdx], au8_R[u32_ByteIdx]);
				//if(u32_ByteIdx>=0x20)
					return UNFD_ST_ERR_DATA_CMP_FAIL;
			}
		}

		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
		{
			if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
					pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
			{
				u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
				continue;
			}

			if (au8_WSpare[u32_ByteIdx] != au8_RSpare[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages, spare, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_WSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
				return UNFD_ST_ERR_DATA_CMP_FAIL;
			}
		}
	}

	return UNFD_ST_SUCCESS;
}

// test Repeat Cnt
// (test within one blk)
static U32 NCTEST_RWMultiplePages_OneTime(U32 u32_StartRowIdx, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_ByteIdx, u32_DataByteCnt, u32_SpareByteCnt;

	U32 *au32_W, *au32_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* check parameters */
	#if 0
	if (((u32_StartRowIdx + u32_PageCnt) &~ pNandDrv->u16_BlkPageCntMask) !=
			(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask))
	#else
    if(u32_StartRowIdx + u32_PageCnt >= pNandDrv->u16_BlkPageCnt +
	    ((u32_StartRowIdx>>pNandDrv->u8_BlkPageCntBits)<<pNandDrv->u8_BlkPageCntBits))
	#endif
	{
		nand_debug(0,1,"%X %X\n", u32_StartRowIdx, u32_PageCnt);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime 0, ErrCode: 0x%08x\n",
				UNFD_ST_ERR_INVALID_PARAM);
		return UNFD_ST_ERR_INVALID_PARAM;
	}

	/* setup buffers */
	au32_W = (U32*)DMA_W_ADDR;
	au32_R = (U32*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	u32_DataByteCnt = u32_PageCnt << pNandDrv->u8_PageByteCntBits;

	/* NOTE: when use RepeatCnt, for the Spare area is still using the first Spare size byte cnt. */
	u32_SpareByteCnt = pNandDrv->u16_SpareByteCnt;

	/* erase first blk */
	u32_Err = NC_EraseBlk(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* init test pattern */
	for (u32_ByteIdx = 0; u32_ByteIdx < u32_DataByteCnt >> 2; u32_ByteIdx++)
	{
		au32_W[u32_ByteIdx] = u32_ByteIdx + 1;
		au32_R[u32_ByteIdx] = ~au32_W[u32_ByteIdx];
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < u32_SpareByteCnt; u32_ByteIdx++)
	{
		au8_WSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;
		au8_RSpare[u32_ByteIdx] = ~au8_WSpare[u32_ByteIdx];
	}

	/* write test pattern */
	u32_Err = NC_WritePages(u32_StartRowIdx, (U8*)au32_W, (U8*)au8_WSpare, u32_PageCnt);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* read test pattern */
	u32_Err = NC_ReadPages(u32_StartRowIdx, (U8*)au32_R, (U8*)au8_RSpare, u32_PageCnt);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime 3, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}
	// [CAUTION]: if the above 2 of nand_translate_DMA_address_Ex, data area must be handled later.
	//nand_translate_DMA_address_Ex((U32)sgau32_RBuf, pNandDrv->u16_PageByteCnt);
	//nand_translate_DMA_address_Ex((U32)sgau8_RSpareBuf, pNandDrv->u16_PageByteCnt);

	/* compare test pattern */
	for (u32_ByteIdx = 0; u32_ByteIdx < u32_DataByteCnt >> 2; u32_ByteIdx++)
	{
		if (au32_W[u32_ByteIdx] != au32_R[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime 4, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
					(u32_ByteIdx << 2) >> pNandDrv->u8_PageByteCntBits, (u32_ByteIdx << 2) & pNandDrv->u16_PageByteCntMask,
					au32_W[u32_ByteIdx], au32_R[u32_ByteIdx]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < u32_SpareByteCnt; u32_ByteIdx++)
	{

		if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
				pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
		{
			u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
			continue;
		}
		
		//if(au8_WSpare[u32_ByteIdx&pNandDrv->u16_SpareByteCntMask] != au8_RSpare[u32_ByteIdx])
		if(au8_WSpare[u32_ByteIdx] != au8_RSpare[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime 5\n");//, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
					//u32_ByteIdx >> pNandDrv->u8_SpareByteCntBits, u32_ByteIdx & pNandDrv->u16_SpareByteCntMask,
					//au8_WSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	}

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_RWSinglePages_Partial(U32 u32_StartRowIdx, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_PageIdx, u32_SectorIdx,	u32_ByteIdx;
	U16 u16_PageByteCnt, u16_SpareByteCnt, u16_PageSectorCnt;

	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;
	U32 *pu32_R, *pu32_W;

	/* erase the first blk */
	u32_Err = NC_EraseBlk(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	u16_PageByteCnt = pNandDrv->u16_PageByteCnt;
	u16_SpareByteCnt = pNandDrv->u16_SpareByteCnt;
	u16_PageSectorCnt = pNandDrv->u16_PageSectorCnt;

	/* W/R/C each page */
	for (u32_PageIdx = 0; u32_PageIdx < u32_PageCnt; u32_PageIdx++)
	{
		/* erase */
		if (u32_PageIdx && !((u32_StartRowIdx + u32_PageIdx) & pNandDrv->u16_BlkPageCntMask))
		{
			u32_Err = NC_EraseBlk(u32_StartRowIdx + u32_PageIdx);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}
		}

		/* init test pattern */
		pu32_R = (U32*)au8_R;
		pu32_W = (U32*)au8_W;
		for (u32_ByteIdx = 0; u32_ByteIdx < u16_PageByteCnt; u32_ByteIdx++)
		{
			pu32_W[u32_ByteIdx] = u32_ByteIdx + 1;
			pu32_R[u32_ByteIdx] = ~pu32_W[u32_ByteIdx];
		}

		pu32_R = (U32*)au8_RSpare;
		pu32_W = (U32*)au8_WSpare;
		for (u32_ByteIdx = 0; u32_ByteIdx < u16_SpareByteCnt; u32_ByteIdx++)
		{
			pu32_W[u32_ByteIdx] = u32_ByteIdx + 1 ;
			pu32_R[u32_ByteIdx] = ~pu32_W[u32_ByteIdx];
		}

		/* write test pattern */
		for (u32_SectorIdx = 0; u32_SectorIdx < u16_PageSectorCnt; u32_SectorIdx++)
		{
			u32_Err = NC_WriteSectors(u32_StartRowIdx + u32_PageIdx, u32_SectorIdx,
					au8_W + (u32_SectorIdx << pNandDrv->u8_SectorByteCntBits),
					//au8_WSpare + (u32_SectorIdx << pNandDrv->u8_SectorSpareByteCntBits), 1);
					au8_WSpare + (u32_SectorIdx * pNandDrv->u16_SectorSpareByteCnt), 1);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 2, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}
		}

		/* read test pattern */
		if (u32_PageIdx == 1)
		{
			for (u32_SectorIdx = 0; u32_SectorIdx < u16_PageSectorCnt; u32_SectorIdx++)
			{
				u32_Err = NC_ReadSectors(u32_StartRowIdx + u32_PageIdx, u32_SectorIdx,
						au8_R + (u32_SectorIdx << pNandDrv->u8_SectorByteCntBits),
						//au8_RSpare + (u32_SectorIdx << pNandDrv->u8_SectorSpareByteCntBits), 1);
						au8_RSpare + (u32_SectorIdx * pNandDrv->u16_SectorSpareByteCnt), 1);
				if (u32_Err != UNFD_ST_SUCCESS)
				{
					nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 3.0, ErrCode: 0x%08x\n", u32_Err);
					return u32_Err;
				}
			}
		}
		else if (u32_PageIdx == 0)
		{
			u32_SectorIdx = 0;
			u32_Err = NC_ReadSectors(u32_StartRowIdx + u32_PageIdx, u32_SectorIdx,
					au8_R + (u32_SectorIdx << pNandDrv->u8_SectorByteCntBits),
					//au8_RSpare + (u32_SectorIdx << pNandDrv->u8_SectorSpareByteCntBits), 1);
					au8_RSpare + (u32_SectorIdx * pNandDrv->u16_SectorSpareByteCnt), 1);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 3.1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}

			u32_SectorIdx += 1;
			if (u32_SectorIdx < u16_PageSectorCnt)
			{
				u32_Err = NC_ReadSectors(u32_StartRowIdx + u32_PageIdx, u32_SectorIdx,
						au8_R + (u32_SectorIdx << pNandDrv->u8_SectorByteCntBits),
						//au8_RSpare + (u32_SectorIdx << pNandDrv->u8_SectorSpareByteCntBits), 2);
						au8_RSpare + (u32_SectorIdx * pNandDrv->u16_SectorSpareByteCnt), 2);
				if (u32_Err != UNFD_ST_SUCCESS)
				{
					nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 3.2, ErrCode: 0x%08x\n", u32_Err);
					return u32_Err;
				}
			}

			u32_SectorIdx += 2;
			if (u32_SectorIdx < u16_PageSectorCnt)
			{
				u32_Err = NC_ReadSectors(u32_StartRowIdx + u32_PageIdx, u32_SectorIdx,
						au8_R + (u32_SectorIdx << pNandDrv->u8_SectorByteCntBits),
						//au8_RSpare + (u32_SectorIdx << pNandDrv->u8_SectorSpareByteCntBits),
						au8_RSpare + (u32_SectorIdx * pNandDrv->u16_SectorSpareByteCnt),
						u16_PageSectorCnt - u32_SectorIdx);
				if (u32_Err != UNFD_ST_SUCCESS)
				{
					nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 3.3, ErrCode: 0x%08x\n", u32_Err);
					return u32_Err;
				}
			}

			NC_WritePages(u32_StartRowIdx + u32_PageIdx, au8_R, au8_RSpare, 1);
		}
		else
		{
			u32_SectorIdx = 0;
			u32_Err = NC_ReadSectors(u32_StartRowIdx + u32_PageIdx, u32_SectorIdx,
					au8_R + (u32_SectorIdx << pNandDrv->u8_SectorByteCntBits),
					//au8_RSpare + (u32_SectorIdx << pNandDrv->u8_SectorSpareByteCntBits),
					au8_RSpare + (u32_SectorIdx * pNandDrv->u16_SectorSpareByteCnt),
					u16_PageSectorCnt);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 3.4, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}
		}
		// [CAUTION]: if the above 2 of nand_translate_DMA_address_Ex, data area must be handled later.
		//nand_translate_DMA_address_Ex((U32)sgau8_RBuf, pNandDrv->u16_PageByteCnt);
		//nand_translate_DMA_address_Ex((U32)sgau8_RSpareBuf, pNandDrv->u16_PageByteCnt);

		/* compare */
		for (u32_ByteIdx = 0; u32_ByteIdx < u16_PageByteCnt; u32_ByteIdx++)
		{
			if (au8_W[u32_ByteIdx] != au8_R[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 4, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_W[u32_ByteIdx], au8_R[u32_ByteIdx]);
				return UNFD_ST_ERR_DATA_CMP_FAIL;
			}
		}

		for (u32_ByteIdx = 0; u32_ByteIdx < u16_SpareByteCnt; u32_ByteIdx++)
		{
			if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
					pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
			{
				u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
				continue;
			}

			if (au8_WSpare[u32_ByteIdx] != au8_RSpare[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWSinglePages_Partial 5, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_WSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
				return UNFD_ST_ERR_DATA_CMP_FAIL;
			}
		}
	}

	return UNFD_ST_SUCCESS;
}


static U32 NCTEST_DumpBadBlk(U8 *pu8_BadBlkBitmap)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_BlkIdx = 0;

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Dump BadBlk Bitmap\n");
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "===================================\n");
	for (u32_BlkIdx = 0; u32_BlkIdx < pNandDrv->u16_BlkCnt >> 3; u32_BlkIdx++)
	{
		if (!(u32_BlkIdx & 0xF))
			nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, "\n");

		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, "%02x ", pu8_BadBlkBitmap[u32_BlkIdx]);
	}

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, "\n");
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "===================================\n");
	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ScanBadBlk(U8 *pu8_BadBlkBitmap)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U8  *au8_R, *au8_RSpare;
	U32 u32_PageIdx, u32_BlkIdx;
	U16 u16_BadBlkCnt = 0;

	au8_R = (U8*)DMA_R_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for (u32_BlkIdx = 0; u32_BlkIdx < pNandDrv->u16_BlkCnt; u32_BlkIdx++)
	{
		u32_PageIdx = u32_BlkIdx << pNandDrv->u8_BlkPageCntBits;
        #if 0
		u32_Err = NC_EraseBlk(u32_PageIdx);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ScanBadBlk 0, u32_BlkIdx: 0x%08x, ErrCode: 0x%08x\n",
					   u32_BlkIdx, u32_Err);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "bad blk : 0x%04x\n", u32_BlkIdx);
			u16_BadBlkCnt++;
			pu8_BadBlkBitmap[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			continue;
		}
		#endif

		u32_Err = NC_ReadPages(u32_PageIdx, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ScanBadBlk 1, ErrCode: 0x%08x\n", u32_Err);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "bad blk : 0x%04x\n", u32_BlkIdx);
			u16_BadBlkCnt++;
			pu8_BadBlkBitmap[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			continue;
		}
		if (au8_RSpare[0] != 0xFF)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "bad blk +: 0x%04x\n", u32_BlkIdx);
			u16_BadBlkCnt++;
			pu8_BadBlkBitmap[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			continue;
		}

		u32_PageIdx += pNandDrv->u16_BlkPageCnt - 1;
		u32_Err = NC_ReadPages(u32_PageIdx,	au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ScanBadBlk 2, ErrCode: 0x%08x\n", u32_Err);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "bad blk : 0x%04x\n", u32_BlkIdx);
			u16_BadBlkCnt++;
			pu8_BadBlkBitmap[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			continue;
		}
		if (au8_RSpare[0] != 0xFF)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "bad blk -: 0x%04x\n", u32_BlkIdx);
			u16_BadBlkCnt++;
			pu8_BadBlkBitmap[u32_BlkIdx >> 3] &= ~(1 << u32_BlkIdx & 0x7);
			continue;
		}
	}

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "\n Total Bad Blk Cnt: 0x%04x\n", u16_BadBlkCnt);

	NCTEST_DumpBadBlk(pu8_BadBlkBitmap);

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_SignatureCheck(void)
{
#if 1 /* temporarily disabled, Andersen will update for SW later. */
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_ByteIdx;

	U8 *au8_R, *au8_RSpare, *au8_W, *au8_WSpare;
	volatile U16 u16_Reg, u16_Tmp0, u16_Tmp1;

	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	/* init test pattern */
	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
	{
		au8_W[u32_ByteIdx] = u32_ByteIdx + 1;
		au8_R[u32_ByteIdx] = ~au8_W[u32_ByteIdx];
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
	{
		/* Important: The signature should be exactly the same for each sector */
		au8_WSpare[u32_ByteIdx] = 0xAA;
		// au8_WSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;
		au8_RSpare[u32_ByteIdx] = ~au8_WSpare[u32_ByteIdx];
	}

	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: SignatureCheck 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_WritePages(0, au8_W, au8_WSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: SignatureCheck 1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}
	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
	{
		// skip the ECC code in the spare area
		if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
				pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
		{
			u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
			continue;
		}

		/* match case */
		REG_WRITE_UINT16(NC_SIGN_DAT, (au8_WSpare[u32_ByteIdx] << 8) + au8_WSpare[u32_ByteIdx]);
		REG_WRITE_UINT16(NC_SIGN_ADR, (u32_ByteIdx << 8) + u32_ByteIdx);
		REG_SET_BITS_UINT16(NC_SIGN_CTRL, BIT_NC_SIGN_CHECK_EN);

		u32_Err = NC_ReadPages(0, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: SignatureCheck 2, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}

		REG_READ_UINT16(NC_SIGN_CTRL, u16_Reg);
		if (u16_Reg & BIT_NC_SIGN_DAT0_MISS)
		{
			REG_READ_UINT16(NC_SIGN_DAT, u16_Tmp0);
			REG_READ_UINT16(NC_SIGN_ADR, u16_Tmp1);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: DAT0 miss 0x%x, 0x%x 0x%x 0x%x\n",
					u32_ByteIdx, u16_Tmp0, u16_Tmp1, u16_Reg);
			return UNFD_ST_ERR_SIGNATURE_FAIL;
		}
		if (u16_Reg & BIT_NC_SIGN_DAT1_MISS)
		{
			REG_READ_UINT16(NC_SIGN_DAT, u16_Tmp0);
			REG_READ_UINT16(NC_SIGN_ADR, u16_Tmp1);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: DAT1 miss 0x%x, 0x%x 0x%x 0x%x\n",
					u32_ByteIdx, u16_Tmp0, u16_Tmp1, u16_Reg);
			return UNFD_ST_ERR_SIGNATURE_FAIL;
		}
		REG_SET_BITS_UINT16(NC_SIGN_CTRL, BIT_NC_SIGN_CLR_STATUS);
		REG_WRITE_UINT16(NC_SIGN_CTRL, 0);

		/* miss case */
		REG_WRITE_UINT16(NC_SIGN_DAT, ~((au8_WSpare[u32_ByteIdx] << 8) + au8_WSpare[u32_ByteIdx]));
		REG_WRITE_UINT16(NC_SIGN_ADR, (u32_ByteIdx << 8) + u32_ByteIdx);
		REG_SET_BITS_UINT16(NC_SIGN_CTRL, BIT_NC_SIGN_CHECK_EN);

		u32_Err = NC_ReadPages(0, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: SignatureCheck 3, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}
		REG_READ_UINT16(NC_SIGN_CTRL, u16_Reg);
		if (0 == (u16_Reg & BIT_NC_SIGN_DAT0_MISS))
		{
			REG_READ_UINT16(NC_SIGN_DAT, u16_Tmp0);
			REG_READ_UINT16(NC_SIGN_ADR, u16_Tmp1);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: DAT0 match 0x%x, 0x%x 0x%x 0x%x\n",
					u32_ByteIdx, u16_Tmp0, u16_Tmp1, u16_Reg);
			return UNFD_ST_ERR_SIGNATURE_FAIL;
		}
		if (0 == (u16_Reg & BIT_NC_SIGN_DAT1_MISS))
		{
			REG_READ_UINT16(NC_SIGN_DAT, u16_Tmp0);
			REG_READ_UINT16(NC_SIGN_ADR, u16_Tmp1);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: DAT1 match 0x%x, 0x%x 0x%x 0x%x\n",
					u32_ByteIdx, u16_Tmp0, u16_Tmp1, u16_Reg);
			return UNFD_ST_ERR_SIGNATURE_FAIL;
		}
		REG_SET_BITS_UINT16(NC_SIGN_CTRL, BIT_NC_SIGN_CLR_STATUS);
		REG_WRITE_UINT16(NC_SIGN_CTRL, 0);
	}
#endif
	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_RWMultiplePages_OneTime_Partial(U32 u32_StartRowIdx, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_ByteIdx;
	U32 *au32_W, *au32_R;
	U8  *au8_WSpare, *au8_RSpare;
	U32 u32_DataByteCnt, u32_SpareByteCnt;

	/* check parameters */
	#if 0
	if (((u32_StartRowIdx + u32_PageCnt) &~ pNandDrv->u16_BlkPageCntMask) !=
			(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask))
	#else
    if(u32_StartRowIdx + u32_PageCnt > pNandDrv->u16_BlkPageCnt +
	    ((u32_StartRowIdx>>pNandDrv->u8_BlkPageCntBits)<<pNandDrv->u8_BlkPageCntBits))
	#endif
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime_Partial 0, ErrCode: 0x%08x\n",
				UNFD_ST_ERR_INVALID_PARAM);
		return UNFD_ST_ERR_INVALID_PARAM;
	}

	/* setup buffers */
	au32_W = (U32*)DMA_W_ADDR;
	au32_R = (U32*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	u32_DataByteCnt = u32_PageCnt << pNandDrv->u8_PageByteCntBits;

	/* NOTE: when use RepeatCnt, for the Spare area is still using the first Spare size byte cnt. */
	u32_SpareByteCnt = pNandDrv->u16_SpareByteCnt;

	/* erase the first blk */
	u32_Err = NC_EraseBlk(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime_Partial 1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* init test pattern */
	for (u32_ByteIdx = 0; u32_ByteIdx < u32_DataByteCnt >> 2; u32_ByteIdx++)
	{
		au32_W[u32_ByteIdx] = u32_ByteIdx + 1;
		au32_R[u32_ByteIdx] = ~au32_W[u32_ByteIdx];
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < u32_SpareByteCnt; u32_ByteIdx++)
	{
		au8_WSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;
		au8_RSpare[u32_ByteIdx] = ~au8_WSpare[u32_ByteIdx];
	}

	/* write test pattern */
	u32_Err = NC_WritePages(u32_StartRowIdx, (U8*)au32_W, au8_WSpare, u32_PageCnt);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime_Partial 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* read test pattern */
	u32_Err = NC_ReadSectors(u32_StartRowIdx, 0, (U8*)au32_R, au8_RSpare,
			(u32_PageCnt << pNandDrv->u8_PageSectorCntBits) - 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime_Partial 3.0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_ReadSectors(u32_StartRowIdx + u32_PageCnt - 1, pNandDrv->u16_PageSectorCnt - 1,
			(U8*)au32_R + (((u32_PageCnt << pNandDrv->u8_PageSectorCntBits) - 1) << pNandDrv->u8_SectorByteCntBits),
			au8_RSpare + pNandDrv->u16_SectorSpareByteCnt * (pNandDrv->u16_PageSectorCnt - 1), 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime_Partial 3.1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}
	// [CAUTION]: if the above 2 of nand_translate_DMA_address_Ex, data area must be handled later.
	//nand_translate_DMA_address_Ex((U32)sgau32_RBuf, pNandDrv->u16_PageByteCnt);
	//nand_translate_DMA_address_Ex((U32)sgau8_RSpareBuf, pNandDrv->u16_PageByteCnt);

	// compare
	for (u32_ByteIdx = 0; u32_ByteIdx < u32_DataByteCnt >> 2; u32_ByteIdx++)
	{
		if (au32_W[u32_ByteIdx] != au32_R[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime_Partial 4, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
					(u32_ByteIdx << 2) >> pNandDrv->u8_PageByteCntBits, (u32_ByteIdx << 2) & pNandDrv->u16_PageByteCntMask,
					au32_W[u32_ByteIdx], au32_R[u32_ByteIdx]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < u32_SpareByteCnt; u32_ByteIdx++)
	{
		if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
				pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
		{
			u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
			continue;
		}

		//if(au8_WSpare[u32_ByteIdx&pNandDrv->u16_SpareByteCntMask] != au8_RSpare[u32_ByteIdx])
		if(au8_WSpare[u32_ByteIdx] != au8_RSpare[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RWMultiplePages_OneTime_Partial 5\n");//, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
					//u32_ByteIdx >> pNandDrv->u8_SpareByteCntBits, u32_ByteIdx&pNandDrv->u16_SpareByteCntMask,
					//au8_WSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	}

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_PageCopy(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;

	U32 u32_SrcTestRow, u32_DstTestRow;
	U32 u32_ByteIdx;
	U8 *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	/* Copy 0th block 3rd page to 1st block 2nd page */
	u32_SrcTestRow = 3 + (5 << pNandDrv->u8_BlkPageCntBits);
	u32_DstTestRow = 2 + (1 << pNandDrv->u8_BlkPageCntBits);

	/* init test pattern */
	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++) {
		au8_W[u32_ByteIdx] = u32_ByteIdx + 1;
		au8_R[u32_ByteIdx] = ~au8_W[u32_ByteIdx];
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++) {
		au8_WSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;
		au8_RSpare[u32_ByteIdx] = ~au8_WSpare[u32_ByteIdx];
	}

	u32_Err = NC_EraseBlk(5 << pNandDrv->u8_BlkPageCntBits);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PageCopy 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_EraseBlk(1 << pNandDrv->u8_BlkPageCntBits);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PageCopy 1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_WritePages(u32_SrcTestRow, au8_W, au8_WSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PageCopy 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_PageCopy(u32_SrcTestRow, u32_DstTestRow, (U8*)au8_W, au8_WSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PageCopy 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_ReadPages(u32_DstTestRow, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PageCopy 3, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
	{
		if (au8_W[u32_ByteIdx] != au8_R[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PageCopy 4, ByteIdx: 0x%08x, W: 0x%02x, R: 0x%02x\n",
					u32_ByteIdx, au8_W[u32_ByteIdx], au8_R[u32_ByteIdx]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	}

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
	{
		//if(au8_WSpare[u32_ByteIdx&pNandDrv->u16_SpareByteCntMask] != au8_RSpare[u32_ByteIdx])
		if(au8_WSpare[u32_ByteIdx] != au8_RSpare[u32_ByteIdx])
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PageCopy 5, ByteIdx: 0x%08x, W: 0x%02x, R: 0x%02x\n",
					u32_ByteIdx, au8_WSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	}
	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECC_For_All_0xFF_Pages(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;

	U8 *au8_R;
	U8 *au8_RSpare;

	/* setup buffers */
	au8_R = (U8*)DMA_R_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_For_All_0xFF_Pages 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_ReadPages(0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_For_All_0xFF_Pages 1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_WritePages(0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_For_All_0xFF_Pages 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ALL0xFF_ECC_CHECK;
	NC_Config();

	u32_Err = NC_ReadPages(0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_For_All_0xFF_Pages 3, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* Expected UNFD_ST_ERR_ECC_FAIL, not UNFD_ST_SUCCESS */
	u32_Err = NC_ReadPages(1, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_ERR_ECC_FAIL)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_For_All_0xFF_Pages 4, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_ALL0xFF_ECC_CHECK;
	NC_Config();

	u32_Err = NC_ReadPages(1, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_For_All_0xFF_Pages 5, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECC_Generation_Bypass(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;

	U32 u32_ByteIdx;
	U8 *au8_R;
	U8 *au8_RSpare;

	/* setup buffers */
	au8_R = (U8*)DMA_R_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
		au8_R[u32_ByteIdx] = u32_ByteIdx + 1;

	for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
		au8_RSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;

	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_Generation_Bypass 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_Err = NC_WritePages(0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_Generation_Bypass 1, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_BYPASS_ECC;
	NC_Config();

	u32_Err = NC_WritePages(1, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_Generation_Bypass 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_BYPASS_ECC;
	NC_Config();

	u32_Err = NC_ReadPages(0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_Generation_Bypass 3, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* Expected UNFD_ST_ERR_ECC_FAIL, not UNFD_ST_SUCCESS */
	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "expect to get ECC error\n");
	u32_Err = NC_ReadPages(1, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_ERR_ECC_FAIL)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_Generation_Bypass 4, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_BYPASS_ECC;
	NC_Config();

	u32_Err = NC_ReadPages(1, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_Generation_Bypass 5, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_BYPASS_ECC;
	NC_Config();

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECC_RS(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_TestCol, u32_TestRow;
	U8  u8_Data, u8_TestFlag;

	u8_Data = 0;
	u32_TestRow = 0;
#if 0
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "============= corrupt data =============\n");
    nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "setup data...\n");
	u32_Err = NCTEST_RWSinglePages(u32_TestRow, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_RS 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_TestCol = 1;
	for (u32_SectorIdx = 0; u32_SectorIdx < pNandDrv->u16_PageSectorCnt; u32_SectorIdx++)
	{
		/* corrupt 1~5 bytes */
		for (u8_TestFlag = 0; u8_TestFlag < 5; u8_TestFlag++)
		{
			nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "TestRow: 0x%08x,  TestCol: 0x%08x, corrupt %u byte...\n",
					   u32_TestRow, u32_TestCol, u8_TestFlag + 1);
			u32_Err = NC_Write_RandomOut(u32_TestRow, u32_TestCol + u8_TestFlag, &u8_Data, 1);
	        if (u32_Err != UNFD_ST_SUCCESS)
	        {
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_RS 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
	        }

			NCTEST_ECCDataReadBack(u32_TestRow, 0);
		}

		u32_TestCol += pNandDrv->u16_SectorByteCnt;
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, "\n");
	}
#endif

#if 1
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "============= corrupt code =============\n");
    nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "setup data...\n");
	u32_Err = NCTEST_RWSinglePages(u32_TestRow, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_RS 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_TestCol = pNandDrv->u16_PageByteCnt + pNandDrv->u16_SpareByteCnt
				  - pNandDrv->u16_ECCCodeByteCnt;

	/* corrupt 1-5 bytes */
	for (u8_TestFlag = 0; u8_TestFlag < 5; u8_TestFlag++)
	{
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "TestRow: 0x%08x,  TestCol: 0x%08x, corrupt %u byte...\n",
				   u32_TestRow, u32_TestCol, u8_TestFlag + 1);
		u32_Err = NC_Write_RandomOut(u32_TestRow, u32_TestCol + u8_TestFlag , &u8_Data, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_RS 3, u8_TestFlag: %u, ErrCode: 0x%08x\n",
					   u8_TestFlag, u32_Err);
			return u32_Err;
		}

		NCTEST_ECCDataReadBack(u32_TestRow, 7);
	}
#endif

#if 1
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "============= corrupt data + code =============\n");
    nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "setup data...\n");
	u32_Err = NCTEST_RWSinglePages(u32_TestRow, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_RS 4, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_TestCol = pNandDrv->u16_PageByteCnt + pNandDrv->u16_SpareByteCnt
				  - pNandDrv->u16_ECCCodeByteCnt - 3;

	/* corrupt 1~5 bytes */
	for (u8_TestFlag = 0; u8_TestFlag < 5; u8_TestFlag++)
	{
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "TestRow: 0x%08x,  TestCol: 0x%08x, corrupt %u byte...\n",
				   u32_TestRow, u32_TestCol, u8_TestFlag + 1);
		u32_Err = NC_Write_RandomOut(u32_TestRow, u32_TestCol + u8_TestFlag, &u8_Data, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_ECC_RS 5, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}

		if(3 == u8_TestFlag)
		{
			nand_hw_timer_delay(0x100000);
			nand_debug(0,1,"LA 0\n");
			nand_hw_timer_delay(0x100000);
			nand_debug(0,1,"LA 1\n");
			nand_hw_timer_delay(0x100000);
			nand_debug(0,1,"LA 2\n");
		}
		NCTEST_ECCDataReadBack(u32_TestRow, 7);
	}
#endif

	return UNFD_ST_SUCCESS;
}


static U32 NCTEST_RW_BurnIn(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err = 0;
	U32 u32_PageIdx = 0;
	U32 u32_TotalPages = pNandDrv->u16_BlkPageCnt * pNandDrv->u16_BlkCnt;
	while (1)
	{
		if (!(u32_PageIdx & pNandDrv->u16_BlkPageCntMask))
			nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ReadWrite Burn-in: PageIdx: 0x%08x\n", u32_PageIdx);

		u32_Err = NCTEST_RWSinglePages(u32_PageIdx, 1);
		if (u32_Err == UNFD_ST_ERR_DATA_CMP_FAIL)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Compare fail at u32_PageIdx: 0x%08x\n!", u32_PageIdx);
			nand_die();
		}
		else if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Aceess fail at u32_PageIdx: 0x%08x, ErrorCode: 0x%08x\n!",
					   u32_PageIdx, u32_Err);
		}

		if (++u32_PageIdx == u32_TotalPages)
			u32_PageIdx = 0;
	}
}


#define ECC_RS_BURNIN_16x    0
static U32 NCTEST_ECC_RS_BurnIn(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U8  *au8_W, *au8_R, *au8_WSpare, *au8_RSpare, *pu8_BadBlk;
	U16 u16_Reg51 = 0, u16_Reg52 = 0, u16_Reg53 = 0;
	U32 u32_PageIdx;
	U32 u32_Err;
	U32 u32_Tmp;
	U32 u32_Round;

	U8  u8_CorruptByteCnt;
	U32 au32_SymPos[4];

	/* init test pattern */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;
	pu8_BadBlk = (U8*)DMA_BAD_BLK_BUF;

	for (u32_Tmp = 0; u32_Tmp < pNandDrv->u16_PageByteCnt; u32_Tmp++)
		au8_W[u32_Tmp] = ECC_TEST_PATTERN;

	for (u32_Tmp = 0; u32_Tmp < pNandDrv->u16_SpareByteCnt; u32_Tmp++)
		au8_WSpare[u32_Tmp] = ECC_TEST_PATTERN;

	/*
	 * Scan bad blocks
	 * 1 bit for 1 blk, 0: bad blk, 1: good blk
	 */
	memset(pu8_BadBlk, 0xFF, pNandDrv->u16_BlkCnt >> 3);
	if(pNandDrv->u8_WordMode)
	{
		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_WORD_MODE;
		NC_Config();
		u8_CorruptByteCnt = 2;
	}
	else
	{
		u8_CorruptByteCnt = 4;
	}
	NCTEST_ScanBadBlk(pu8_BadBlk);

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "-----------------------------------\n");
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ECC_RS_BurnIn testing ...\n");
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "-----------------------------------\n");

	u32_PageIdx = 0;
	u32_Round = 0;
	au32_SymPos[0] = 0;
	au32_SymPos[1] = 0;
	au32_SymPos[2] = 0;
	au32_SymPos[3] = 0;

	pu8_BadBlk[0] &= ~1; // protect blk0

	while (1)
	{
		U32 u32_BlkIdx = u32_PageIdx >> pNandDrv->u8_BlkPageCntBits;

        #if 0
		nand_debug(0, 1, "u32_PageIdx: 0x%08x, i: 0x%05x  j: 0x%05x  k: 0x%05x  l: 0x%05x\n",
				u32_PageIdx, au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
        #endif
		// nand_reset_WatchDog();

		/* init the NAND sector data */
		if (!(u32_PageIdx & pNandDrv->u16_BlkPageCntMask))
		{
			//nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "u32_BlkIdx: 0x%08x\n", u32_BlkIdx);
			if (!(pu8_BadBlk[u32_BlkIdx >> 3] & (1 << (u32_BlkIdx & 0x7))))
			{
				nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Skip bad blk: 0x%08x\n", u32_BlkIdx);
				u32_PageIdx += pNandDrv->u16_BlkPageCnt;
				continue;
			}

		    u32_Err = NC_EraseBlk(u32_PageIdx);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RS_BurnIn 0, ErrCode: 0x%08x\n", u32_Err);
				goto LABEL_END_OF_BURN_IN;
			}
		}

		u32_Err = NC_WriteSectors(u32_PageIdx, 0, au8_W, au8_WSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "INFO: RS_BurnIn 1, ErrCode: 0x%08x\n", u32_Err);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Mark bad blk: 0x%08x\n", u32_BlkIdx);
			pu8_BadBlk[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			NCTEST_DumpBadBlk(pu8_BadBlk);
			continue;
		}

		/* corrupt 4 bytes data */
		for (u32_Tmp = 0; u32_Tmp < u8_CorruptByteCnt; u32_Tmp++)
		{
			U32 u32_ByteIdx = au32_SymPos[u32_Tmp];

			au8_W[u32_ByteIdx] ^= 0xFF;
			u32_Err = NC_Write_RandomOut(u32_PageIdx, u32_ByteIdx,
										 &au8_W[u32_ByteIdx], 1);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "INFO: RS_BurnIn 2, ErrCode: 0x%08x\n", u32_Err);
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Mark bad blk: 0x%08x\n", u32_BlkIdx);
				pu8_BadBlk[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
				NCTEST_DumpBadBlk(pu8_BadBlk);
				continue;
			}

			au8_W[u32_ByteIdx] = ECC_TEST_PATTERN;
		}

		/* read the sector back and check ECC registers */
		u32_Err = NC_ReadSectors(u32_PageIdx, 0, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "INFO: RS_BurnIn 3, ErrCode: 0x%08x\n", u32_Err);

			#if ECC_RS_BURNIN_16x
            nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "BitPos, i: 0x%05x  j: 0x%05x \n",
			    au32_SymPos[0], au32_SymPos[1]);
			#else
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "BitPos, i: 0x%05x  j: 0x%05x  k: 0x%05x  l: 0x%05x\n",
			    au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
			#endif

			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Mark bad blk: 0x%08x\n", u32_BlkIdx);
			pu8_BadBlk[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			//NCTEST_DumpBadBlk(pu8_BadBlk);
			continue;
		}

		REG_READ_UINT16(NC_ECC_STAT0, u16_Reg51);
		REG_READ_UINT16(NC_ECC_STAT1, u16_Reg52);
		REG_READ_UINT16(NC_ECC_STAT2, u16_Reg53);
		if (u16_Reg51 & BIT_NC_ECC_FAIL)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECC Fail Reg51: 0x%04x\n", u16_Reg51);
			goto LABEL_END_OF_BURN_IN;
		}

#if 0
		if (((u16_Reg51 & BIT_NC_ECC_MAX_BITS_MASK) >> 1) != 4)
		{
			nand_debug(0, 1, "Bits < 4: 0x%x\n", (u16_Reg51 & BIT_NC_ECC_MAX_BITS_MASK)>>1);
			nand_debug(0, 1, "TestCnt: 0x%08x, i: 0x%08x  j: 0x%08x  k:% 0x08x  l: 0x%08x\n",
					u32_TestCnt, au32_BitPos[0], au32_BitPos[1], au32_BitPos[2], au32_BitPos[3]);
			nand_debug(0, 1, "Reg51: 0x%04x  Reg52: 0x%04x  Reg53: 0x%04x\n",
					u16_Reg51, u16_Reg52, u16_Reg53);
		}
#endif
		if (u16_Reg52 != 1)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECC Sector Cnt != 1, Reg52: 0x%04x\n", u16_Reg52);
			goto LABEL_END_OF_BURN_IN;
		}
		if ((u16_Reg53 & BIT_NC_ECC_FLGA_MASK) == BIT_NC_ECC_FLGA_FAIL)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECC Fail Reg53: 0x%04x\n", u16_Reg53);
			goto LABEL_END_OF_BURN_IN;
		}
		if ((u16_Reg53 & BIT_NC_ECC_CORRECT_CNT) >> BIT_NC_ECC_CNT_SHIFT !=
				(u16_Reg51 & BIT_NC_ECC_MAX_BITS_MASK) >> 1)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: Bit Cnt mismatch, Reg51 vs Reg53\n");
			goto LABEL_END_OF_BURN_IN;
		}

		/* change error bits' positions */
		#if 0
		au32_SymPos[0] = (au32_SymPos[0] - 3) & pNandDrv->u16_SectorByteCntMask;
		au32_SymPos[1] = (au32_SymPos[1] + 5) & pNandDrv->u16_SectorByteCntMask;
		au32_SymPos[2] = (au32_SymPos[2] + 1) & pNandDrv->u16_SectorByteCntMask;
		au32_SymPos[3] = (au32_SymPos[3] - 1) & pNandDrv->u16_SectorByteCntMask;
		#else
		au32_SymPos[0] = (au32_SymPos[0]-1) & ((pNandDrv->u16_SectorByteCntMask+1)/(4/u8_CorruptByteCnt)-1);
		if(0 == au32_SymPos[0])
			au32_SymPos[1] = (au32_SymPos[1]-1) & ((pNandDrv->u16_SectorByteCntMask+1)/(4/u8_CorruptByteCnt)-1);
		if(0 == au32_SymPos[0] && 0 == au32_SymPos[1])
			au32_SymPos[2] = (au32_SymPos[2]-1) & ((pNandDrv->u16_SectorByteCntMask+1)/(4/u8_CorruptByteCnt)-1);
		if(0 == au32_SymPos[0] && 0 == au32_SymPos[1] && 0 == au32_SymPos[2])
			au32_SymPos[3] = (au32_SymPos[3]-1) & ((pNandDrv->u16_SectorByteCntMask+1)/(4/u8_CorruptByteCnt)-1);
		#endif

		u32_PageIdx++;

		if (u32_PageIdx >= pNandDrv->u16_BlkCnt << pNandDrv->u8_BlkPageCntBits)
		{
			u32_PageIdx = 0;
			u32_Round++;
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Round %08X, Start Pos: %04X %04X %04X %04X\n", u32_Round,
				au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
			//NCTEST_DumpBadBlk(pu8_BadBlk);
		}

		NC_ResetFCIE();
		NC_Config();
	}

LABEL_END_OF_BURN_IN:
	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Burn-in failed\n");
	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "u32_PageIdx: 0x%08x, i: 0x%05x  j: 0x%05x  k: 0x%05x  l: 0x%05x\n",
			u32_PageIdx, au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);

	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Reg51: 0x%04x, Reg52: 0x%04x, Reg53: 0x%04x\n",
			u16_Reg51, u16_Reg52, u16_Reg53);

	NCTEST_DumpBadBlk(pu8_BadBlk);
	nand_die();
	return UNFD_ST_SUCCESS;
}


static U32 NCTEST_Verify_16bits(U32 u32_StartRowIdx, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U32 u32_PageIdx, u32_ByteIdx;

	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_WORD_MODE;
	NC_Config();

	/* erase the first blk */
	u32_Err = NC_EraseBlk(u32_StartRowIdx &~ pNandDrv->u16_BlkPageCntMask);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"LA 0\n");
	nand_hw_timer_delay(0x100000);
    nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"LA 1\n");
    nand_hw_timer_delay(0x100000);
    nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"LA 2\n");    
    nand_hw_timer_delay(0x100000);

	/* W/R/C each page */
	for (u32_PageIdx = 0; u32_PageIdx < u32_PageCnt; u32_PageIdx++)
	{
		/* erase */
		if (u32_PageIdx && !((u32_StartRowIdx + u32_PageIdx) & pNandDrv->u16_BlkPageCntMask))
		{
			u32_Err = NC_EraseBlk(u32_StartRowIdx + u32_PageIdx);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}
		}

		/* init test pattern */
		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
		{
			if (pNandDrv->u8_IfECCTesting)
				au8_W[u32_ByteIdx] = ECC_TEST_PATTERN;
			else
				au8_W[u32_ByteIdx] = u32_ByteIdx + 1;

			au8_R[u32_ByteIdx] = ~au8_W[u32_ByteIdx];
            #if 0
			if(u32_ByteIdx<0x20)
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "set: [%2u]: adr:%X val:%02X  adr:%X val:%02X\n",
				    u32_ByteIdx, &au8_W[u32_ByteIdx], au8_W[u32_ByteIdx],
				    &au8_R[u32_ByteIdx], au8_R[u32_ByteIdx]);
			#endif
		}

		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
		{
			if (pNandDrv->u8_IfECCTesting)
				au8_WSpare[u32_ByteIdx] = ECC_TEST_PATTERN;
			else
				au8_WSpare[u32_ByteIdx] = u32_ByteIdx + 1 ;

			au8_RSpare[u32_ByteIdx] = ~au8_WSpare[u32_ByteIdx];
		}

		u32_Err = NC_WritePages(u32_StartRowIdx + u32_PageIdx, au8_W, au8_WSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 2, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}

		/* read test pattern */
		u32_Err = NC_ReadPages(u32_StartRowIdx + u32_PageIdx, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages 3, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}

		/* compare test pattern */
		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_PageByteCnt; u32_ByteIdx++)
		{
			if (au8_W[u32_ByteIdx] != au8_R[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages, data, Row: 0x%08X, Col: 0x%04X, W: 0x%02X, R: 0x%02X\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_W[u32_ByteIdx], au8_R[u32_ByteIdx]);
				//if(u32_ByteIdx>=0x20)
					return UNFD_ST_ERR_DATA_CMP_FAIL;
			}
			#if 0
			if(u32_ByteIdx<0x20)
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "get data: [%2u]: adr:%X val:%02X  adr:%X val:%02X\n",
				    u32_ByteIdx, &au8_W[u32_ByteIdx], au8_W[u32_ByteIdx],
				    &au8_R[u32_ByteIdx], au8_R[u32_ByteIdx]);
			#endif
		}

		for (u32_ByteIdx = 0; u32_ByteIdx < pNandDrv->u16_SpareByteCnt; u32_ByteIdx++)
		{
			if (u32_ByteIdx == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
					pNandDrv->u16_SectorSpareByteCnt * (u32_ByteIdx / pNandDrv->u16_SectorSpareByteCnt))
			{
				u32_ByteIdx += pNandDrv->u16_ECCCodeByteCnt - 1;
				continue;
			}

			if (au8_WSpare[u32_ByteIdx] != au8_RSpare[u32_ByteIdx])
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_RWSinglePages, spare, Row: 0x%08x, Col: 0x%04x, W: 0x%02x, R: 0x%02x\n",
						u32_StartRowIdx + u32_PageIdx, u32_ByteIdx, au8_WSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
				return UNFD_ST_ERR_DATA_CMP_FAIL;
			}

			#if 0
			if(u32_ByteIdx<0x20)
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "get spare: [%2u]: adr:%X val:%02X  adr:%X val:%02X\n",
				    u32_ByteIdx, &au8_WSpare[u32_ByteIdx], au8_WSpare[u32_ByteIdx],
				    &au8_RSpare[u32_ByteIdx], au8_RSpare[u32_ByteIdx]);
			#endif
		}
	}

	pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_WORD_MODE;
	NC_Config();

	return UNFD_ST_SUCCESS;
}


#if 0
static U32 NCTEST_ECC_BCH8b_512B_BurnIn(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err, u32_i, u32_j;
	U32 u32_TestCol, u32_TestRow;
	U16 u16_Tmp;
	volatile U16 u16_Reg51, u16_Reg52, u16_Reg53;

	U8 *au8_W, *au8_R, *pu8_BadBlk;
	U8 *au8_WSpare, *au8_RSpare, au8_Spare[0x100]; // [CAUTION]: 128 bytes
	U8 u8_ErrBytes, u8_SpareErrBytes;

	U32 u32_PageIdx, u32_Round,  au32_SymPos[8];

	// ----------------------------
    // setup data
    // ----------------------------
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;
	pu8_BadBlk = (U8*)DMA_BAD_BLK_BUF;

	for (u32_i = 0; u32_i < pNandDrv->u16_PageByteCnt; u32_i++)
	{
		au8_W[u32_i] = ECC_TEST_PATTERN;
		au8_R[u32_i] = ~au8_W[u32_i];
	}
	for (u32_i = 0; u32_i < pNandDrv->u16_SpareByteCnt; u32_i++)
	{
		au8_WSpare[u32_i] = ECC_TEST_PATTERN;
		au8_RSpare[u32_i] = ~au8_WSpare[u32_i];
	}
	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 1, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}

	// ----------------------------
    // W/R to get ECC in CIFD
    // ----------------------------
    dump_mem(au8_WSpare, pNandDrv->u16_SectorSpareByteCnt);
	u32_Err = NC_WriteSector(0, 0, au8_W, au8_WSpare);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 2, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}
	u32_Err = NC_ReadSectors(0, 0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 3, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}
	nand_debug(0, 1, "dump: \n");
	NC_GetCIFD(au8_Spare, 0, pNandDrv->u16_SectorSpareByteCnt);
	dump_mem(au8_Spare, pNandDrv->u16_SectorSpareByteCnt);
	for(u32_i=0; u32_i<pNandDrv->u16_SectorByteCnt; u32_i++)
		if(au8_W[u32_i] != au8_R[u32_i])
		{
			nand_debug(0, 0, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 4, I:%X W:%X R:%X\n", 
				u32_i, au8_W[u32_i], au8_R[u32_i]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	for(u32_i=0; u32_i<pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt; u32_i++)
		if(au8_WSpare[u32_i] != au8_RSpare[u32_i])
		{
			nand_debug(0, 0, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 5, I:%X W:%X R:%X\n", 
				u32_i, au8_WSpare[u32_i], au8_RSpare[u32_i]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}

    // ----------------------------
    // Scan bad blocks:
    // 1 bit for 1 blk, 0: bad blk, 1: good blk
    // ----------------------------
	memset(pu8_BadBlk, 0xFF, pNandDrv->u16_BlkCnt >> 3);
	if(pNandDrv->u8_WordMode)
	{
		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_WORD_MODE;
		NC_Config();
	}
	NCTEST_ScanBadBlk(pu8_BadBlk);
	//while(1);
	nand_debug(2, 1, "-----------------------------------\n");
	nand_debug(2, 1, "ECC_RS_BurnIn testing ...\n");
	nand_debug(2, 1, "-----------------------------------\n");
	
	pu8_BadBlk[0] &= ~1; // protect blk0

	u32_PageIdx = 0;
	u32_Round = 0;

	for(u32_i=0; u32_i<8; u32_i++)
		au32_SymPos[u32_i] = 0;

	while(1)
	{
		U32 u32_BlkIdx = u32_PageIdx >> pNandDrv->u8_BlkPageCntBits;
		
		if (0 == (u32_PageIdx & pNandDrv->u16_BlkPageCntMask))
		{
			//nand_debug(0, 1, "u32_BlkIdx: 0x%08x\n", u32_BlkIdx);
			if(!(pu8_BadBlk[u32_BlkIdx >> 3] & (1 << (u32_BlkIdx & 0x7))))
			{
				nand_debug(0, 1, "Skip bad blk: 0x%08x\n", u32_BlkIdx);
				u32_PageIdx += pNandDrv->u16_BlkPageCnt;
				continue;
			}

		    u32_Err = NC_EraseBlk(u32_PageIdx);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 6, ErrCode: 0x%08x\n", u32_Err);
				goto LABEL_END_OF_BURN_IN;
			}
		}

		nand_clock_setting((u32_PageIdx & FCIE3_CLK_MASK)%9);

		/* corrupt bits in buffer */
		for(u16_Tmp = 0; u16_Tmp < pNandDrv->u16_SectorSpareByteCnt; u16_Tmp++)
			au8_WSpare[u16_Tmp] = au8_Spare[u16_Tmp];
			
		for(u16_Tmp = 0; u16_Tmp < 8; u16_Tmp++)
		{
			U32 u32_BitIdx = au32_SymPos[u16_Tmp];
            if(u32_BitIdx < (pNandDrv->u16_SectorByteCnt<<3))
            {
				if(0xFF == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
				else if(0 == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));
				//nand_debug(0,1,"Data: %X\n", au8_W[u32_BitIdx/8]);
            }
			else
			{
				u32_BitIdx -= pNandDrv->u16_SectorByteCnt<<3;
				if(u32_BitIdx < ((pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt)<<3))
				{
					if(0xFF == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
				    else if(0 == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));				
				}
				else
				{
					U8 u8_Tmp, u8_PosFlag=1;
					for(u8_Tmp=u16_Tmp; u8_Tmp>0; u8_Tmp--)
						if(au32_SymPos[u16_Tmp] == au32_SymPos[u8_Tmp-1])
							u8_PosFlag = 0;
					if(u8_PosFlag)
					    au8_WSpare[u32_BitIdx/8] ^= (1<<(u32_BitIdx&7));	
				}					
				//nand_debug(0,1,"spare: %X\n", au8_WSpare[u32_BitIdx/8]);
			}			
		}

		#if 0
		nand_debug(0,1,"LA 0\n");
        nand_hw_timer_delay(0x100000);
        nand_debug(0,1,"LA 1\n");
        nand_hw_timer_delay(0x100000);
        nand_debug(0,1,"LA 2\n");
	    nand_hw_timer_delay(0x100000);  
		#endif

		/* w with ECC bypass */
		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_BYPASS_ECC;
	    NC_Config();
		u32_Err = NC_WriteSector(u32_PageIdx, 0, au8_W, au8_WSpare);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(0, 1, "INFO: NCTEST_ECC_BCH8b_512B_BurnIn 7, ErrCode: 0x%08x\n", u32_Err);
			nand_debug(0, 1, "Mark bad blk: 0x%08x\n", u32_BlkIdx);
			pu8_BadBlk[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			NCTEST_DumpBadBlk(pu8_BadBlk);
			continue;
		}
		pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_BYPASS_ECC;
	    NC_Config();

		u32_Err = NC_ReadSectors(u32_PageIdx, 0, au8_R, au8_RSpare, 1);
	    if (u32_Err != UNFD_ST_SUCCESS)
	    {
		    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 8, ErrCode: 0x%08X\n", u32_Err);
		    return u32_Err;
 	    }

		// check registers
		REG_READ_UINT16(NC_ECC_STAT0, u16_Reg51);
	    REG_READ_UINT16(NC_ECC_STAT1, u16_Reg52);
	    REG_READ_UINT16(NC_ECC_STAT2, u16_Reg53);
	    //nand_debug(0, 1, "Reg51: 0x%04x  Reg52: 0x%04x  Reg53: 0x%04x\n",
		//	   u16_Reg51, u16_Reg52, u16_Reg53);
		if(1!=(u16_Reg53 & BIT_NC_ECC_FLGA_MASK))// && 3!=(u16_Reg53 & BIT_NC_ECC_FLGA_MASK))
		{
			nand_debug(0, 1, "Reg53 incorrect: %X \n", u16_Reg53);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		if(1!=u16_Reg52)
		{
			nand_debug(0, 1, "Reg52 incorrect: %X \n", u16_Reg52);
			return UNFD_ST_ERR_ECC_FAIL;
		}

		if(u16_Reg51&BIT_NC_ECC_FAIL)
		{
			nand_debug(0, 1, "Reg51 incorrect: %X \n", u16_Reg51);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		//if(((u16_Reg51&BIT_NC_ECC_MAX_BITS_MASK)>>1) != (u32_j+1) && u32_j<(u8_BCHBitCnt-1))
		//{
		//	nand_debug(0, 1, "Reg51 incorrect 2: %X \n", u16_Reg51);
		//	return UNFD_ST_ERR_ECC_FAIL;
		//}

		// restore data
		for(u16_Tmp = 0; u16_Tmp < 8; u16_Tmp++)
		{
			U32 u32_BitIdx = au32_SymPos[u16_Tmp];
            if(u32_BitIdx < (pNandDrv->u16_SectorByteCnt<<3))
            {
				if(0xFF == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));
				else if(0 == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
			}
			else
			{	u32_BitIdx -= pNandDrv->u16_SectorByteCnt<<3;
				if(u32_BitIdx < ((pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt)<<3))
				{
					if(0xFF == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));
				    else if(0 == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
				}
				else
				{
					U8 u8_Tmp, u8_PosFlag=1;
					for(u8_Tmp=u16_Tmp; u8_Tmp>0; u8_Tmp--)
						if(au32_SymPos[u16_Tmp] == au32_SymPos[u8_Tmp-1])
							u8_PosFlag = 0;
					if(u8_PosFlag)
					    au8_WSpare[u32_BitIdx/8] ^= (1<<(u32_BitIdx&7));	
				}					
				//nand_debug(0,1,"spare: %X\n", au8_WSpare[u32_BitIdx/8]);
			}			
		}
		// check data
		u8_ErrBytes = 0;
		for(u32_i=0; u32_i<pNandDrv->u16_SectorByteCnt; u32_i++)
		    if(au8_W[u32_i] != au8_R[u32_i])
		    {
				u8_ErrBytes++;
			    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 8, I:%X W:%X R:%X, Pos: %X %X %X %X\n", 
				     u32_i, au8_W[u32_i], au8_R[u32_i], 
				     au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
			}
		u8_SpareErrBytes = 0;
	    for(u32_i=0; u32_i<pNandDrv->u16_SectorSpareByteCnt; u32_i++)
		    if(au8_WSpare[u32_i] != au8_RSpare[u32_i])
		    {
				u8_SpareErrBytes++;
			    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH8b_512B_BurnIn 9, I:%X W:%X R:%X, Pos: %X %X %X %X\n", 
				     u32_i, au8_WSpare[u32_i], au8_RSpare[u32_i], 
				     au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
		    }
		if(u8_ErrBytes > 8 || u8_SpareErrBytes > 8 || 
			(u8_ErrBytes+u8_SpareErrBytes) > 8)
		{
			nand_debug(0, 1, "NCTEST_ECC_BCH8b_512B_BurnIn 10, Pos: %X %X %X %X\n", 
				     au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
		    return UNFD_ST_ERR_ECC_FAIL;
		}		

		//while(1);
		#if 1
		au32_SymPos[0] = (au32_SymPos[0] - 3) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[1] = (au32_SymPos[1] + 5) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[2] = (au32_SymPos[2] + 1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[3] = (au32_SymPos[3] - 1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[4] = (au32_SymPos[0] - 13) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[5] = (au32_SymPos[1] + 7) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[6] = (au32_SymPos[2] + 11) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[7] = (au32_SymPos[3] - 17) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		#else
		au32_SymPos[0] = (au32_SymPos[0]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		if(0 == au32_SymPos[0])
			au32_SymPos[1] = (au32_SymPos[1]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		if(0 == au32_SymPos[0] && 0 == au32_SymPos[1])
			au32_SymPos[2] = (au32_SymPos[2]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		if(0 == au32_SymPos[0] && 0 == au32_SymPos[1] && 0 == au32_SymPos[2])
			au32_SymPos[3] = (au32_SymPos[3]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		#endif

		u32_PageIdx++;
		if (u32_PageIdx >= pNandDrv->u16_BlkCnt << pNandDrv->u8_BlkPageCntBits)
		{
			u32_PageIdx = 0;
			u32_Round++;
			nand_debug(0, 1, "Round %08X, Start Pos: %04X %04X %04X %04X %04X %04X %04X %04X\n", u32_Round,
				au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3],
				au32_SymPos[4], au32_SymPos[5], au32_SymPos[6], au32_SymPos[7]);
			//NCTEST_DumpBadBlk(pu8_BadBlk);
		}
		
	}
	
	
    LABEL_END_OF_BURN_IN:
	nand_debug(0, 1, "Burn-in failed\n");
	nand_debug(0, 1, "PageIdx: 0x%08X, Pos: i: 0x%05X  j: 0x%05x  k: 0x%05X  l: 0x%05X\n",
			u32_PageIdx, au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);

	nand_debug(0, 1, "Reg51: 0x%04x, Reg52: 0x%04x, Reg53: 0x%04x\n",
			u16_Reg51, u16_Reg52, u16_Reg53);

	NCTEST_DumpBadBlk(pu8_BadBlk);
	nand_die();
	return UNFD_ST_SUCCESS;
}


static U32 NCTEST_ECC_BCH4b_512B_BurnIn(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err, u32_i, u32_j;
	U32 u32_TestCol, u32_TestRow;
	U16 u16_Tmp;
	volatile U16 u16_Reg51, u16_Reg52, u16_Reg53;

	U8 *au8_W, *au8_R, *pu8_BadBlk;
	U8 *au8_WSpare, *au8_RSpare, au8_Spare[0x100]; // [CAUTION]: 128 bytes
	U8 u8_ErrBytes, u8_SpareErrBytes;

	U32 u32_PageIdx, u32_Round,  au32_SymPos[4];

	// ----------------------------
    // setup data
    // ----------------------------
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;
	pu8_BadBlk = (U8*)DMA_BAD_BLK_BUF;

	for (u32_i = 0; u32_i < pNandDrv->u16_PageByteCnt; u32_i++)
	{
		au8_W[u32_i] = ECC_TEST_PATTERN;
		au8_R[u32_i] = ~au8_W[u32_i];
	}
	for (u32_i = 0; u32_i < pNandDrv->u16_SpareByteCnt; u32_i++)
	{
		au8_WSpare[u32_i] = ECC_TEST_PATTERN;
		au8_RSpare[u32_i] = ~au8_WSpare[u32_i];
	}
	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 1, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}

	// ----------------------------
    // W/R to get ECC in CIFD
    // ----------------------------
    dump_mem(au8_WSpare, pNandDrv->u16_SectorSpareByteCnt);
	u32_Err = NC_WriteSector(0, 0, au8_W, au8_WSpare);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 2, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}
	u32_Err = NC_ReadSectors(0, 0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 3, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}
	nand_debug(0, 1, "dump: \n");
	NC_GetCIFD(au8_Spare, 0, pNandDrv->u16_SectorSpareByteCnt);
	dump_mem(au8_Spare, pNandDrv->u16_SectorSpareByteCnt);
	for(u32_i=0; u32_i<pNandDrv->u16_SectorByteCnt; u32_i++)
		if(au8_W[u32_i] != au8_R[u32_i])
		{
			nand_debug(0, 0, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 4, I:%X W:%X R:%X\n", 
				u32_i, au8_W[u32_i], au8_R[u32_i]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	for(u32_i=0; u32_i<pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt; u32_i++)
		if(au8_WSpare[u32_i] != au8_RSpare[u32_i])
		{
			nand_debug(0, 0, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 5, I:%X W:%X R:%X\n", 
				u32_i, au8_WSpare[u32_i], au8_RSpare[u32_i]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}

    // ----------------------------
    // Scan bad blocks:
    // 1 bit for 1 blk, 0: bad blk, 1: good blk
    // ----------------------------
	memset(pu8_BadBlk, 0xFF, pNandDrv->u16_BlkCnt >> 3);
	if(pNandDrv->u8_WordMode)
	{
		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_WORD_MODE;
		NC_Config();
	}
	NCTEST_ScanBadBlk(pu8_BadBlk);
	//while(1);
	nand_debug(2, 1, "-----------------------------------\n");
	nand_debug(2, 1, "ECC_RS_BurnIn testing ...\n");
	nand_debug(2, 1, "-----------------------------------\n");
	
	pu8_BadBlk[0] &= ~1; // protect blk0

	u32_PageIdx = 0;
	u32_Round = 0;
	
	au32_SymPos[0] = 0;
	au32_SymPos[1] = 0;
	au32_SymPos[2] = 0;
	au32_SymPos[3] = 0;

	while(1)
	{
		U32 u32_BlkIdx = u32_PageIdx >> pNandDrv->u8_BlkPageCntBits;
		
		if (0 == (u32_PageIdx & pNandDrv->u16_BlkPageCntMask))
		{
			//nand_debug(0, 1, "u32_BlkIdx: 0x%08x\n", u32_BlkIdx);
			if(!(pu8_BadBlk[u32_BlkIdx >> 3] & (1 << (u32_BlkIdx & 0x7))))
			{
				nand_debug(0, 1, "Skip bad blk: 0x%08x\n", u32_BlkIdx);
				u32_PageIdx += pNandDrv->u16_BlkPageCnt;
				continue;
			}

		    u32_Err = NC_EraseBlk(u32_PageIdx);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 6, ErrCode: 0x%08x\n", u32_Err);
				goto LABEL_END_OF_BURN_IN;
			}
		}

		nand_clock_setting((u32_PageIdx & FCIE3_CLK_MASK)%9);

		/* corrupt bits in buffer */
		for(u16_Tmp = 0; u16_Tmp < pNandDrv->u16_SectorSpareByteCnt; u16_Tmp++)
			au8_WSpare[u16_Tmp] = au8_Spare[u16_Tmp];
			
		for(u16_Tmp = 0; u16_Tmp < 4; u16_Tmp++)
		{
			U32 u32_BitIdx = au32_SymPos[u16_Tmp];
            if(u32_BitIdx < (pNandDrv->u16_SectorByteCnt<<3))
            {
				if(0xFF == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
				else if(0 == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));
				//nand_debug(0,1,"Data: %X\n", au8_W[u32_BitIdx/8]);
            }
			else
			{
				u32_BitIdx -= pNandDrv->u16_SectorByteCnt<<3;
				if(u32_BitIdx < ((pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt)<<3))
				{
					if(0xFF == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
				    else if(0 == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));				
				}
				else
				{
					U8 u8_Tmp, u8_PosFlag=1;
					for(u8_Tmp=u16_Tmp; u8_Tmp>0; u8_Tmp--)
						if(au32_SymPos[u16_Tmp] == au32_SymPos[u8_Tmp-1])
							u8_PosFlag = 0;
					if(u8_PosFlag)
					    au8_WSpare[u32_BitIdx/8] ^= (1<<(u32_BitIdx&7));	
				}					
				//nand_debug(0,1,"spare: %X\n", au8_WSpare[u32_BitIdx/8]);
			}			
		}

		#if 0
		nand_debug(0,1,"LA 0\n");
        nand_hw_timer_delay(0x100000);
        nand_debug(0,1,"LA 1\n");
        nand_hw_timer_delay(0x100000);
        nand_debug(0,1,"LA 2\n");
	    nand_hw_timer_delay(0x100000);  
		#endif

		/* w with ECC bypass */
		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_BYPASS_ECC;
	    NC_Config();
		u32_Err = NC_WriteSector(u32_PageIdx, 0, au8_W, au8_WSpare);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(0, 1, "INFO: NCTEST_ECC_BCH4b_512B_BurnIn 7, ErrCode: 0x%08x\n", u32_Err);
			nand_debug(0, 1, "Mark bad blk: 0x%08x\n", u32_BlkIdx);
			pu8_BadBlk[u32_BlkIdx >> 3] &= ~(1 << (u32_BlkIdx & 0x7));
			NCTEST_DumpBadBlk(pu8_BadBlk);
			continue;
		}
		pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_BYPASS_ECC;
	    NC_Config();

		u32_Err = NC_ReadSectors(u32_PageIdx, 0, au8_R, au8_RSpare, 1);
	    if (u32_Err != UNFD_ST_SUCCESS)
	    {
		    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 8, ErrCode: 0x%08X\n", u32_Err);
		    return u32_Err;
 	    }

		// check registers
		REG_READ_UINT16(NC_ECC_STAT0, u16_Reg51);
	    REG_READ_UINT16(NC_ECC_STAT1, u16_Reg52);
	    REG_READ_UINT16(NC_ECC_STAT2, u16_Reg53);
	    //nand_debug(0, 1, "Reg51: 0x%04x  Reg52: 0x%04x  Reg53: 0x%04x\n",
		//	   u16_Reg51, u16_Reg52, u16_Reg53);
		if(1!=(u16_Reg53 & BIT_NC_ECC_FLGA_MASK))// && 3!=(u16_Reg53 & BIT_NC_ECC_FLGA_MASK))
		{
			nand_debug(0, 1, "Reg53 incorrect: %X \n", u16_Reg53);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		if(1!=u16_Reg52)
		{
			nand_debug(0, 1, "Reg52 incorrect: %X \n", u16_Reg52);
			return UNFD_ST_ERR_ECC_FAIL;
		}

		if(u16_Reg51&BIT_NC_ECC_FAIL)
		{
			nand_debug(0, 1, "Reg51 incorrect: %X \n", u16_Reg51);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		//if(((u16_Reg51&BIT_NC_ECC_MAX_BITS_MASK)>>1) != (u32_j+1) && u32_j<(u8_BCHBitCnt-1))
		//{
		//	nand_debug(0, 1, "Reg51 incorrect 2: %X \n", u16_Reg51);
		//	return UNFD_ST_ERR_ECC_FAIL;
		//}

		// restore data
		for(u16_Tmp = 0; u16_Tmp < 4; u16_Tmp++)
		{
			U32 u32_BitIdx = au32_SymPos[u16_Tmp];
            if(u32_BitIdx < (pNandDrv->u16_SectorByteCnt<<3))
            {
				if(0xFF == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));
				else if(0 == ECC_TEST_PATTERN)
					au8_W[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
			}
			else
			{	u32_BitIdx -= pNandDrv->u16_SectorByteCnt<<3;
				if(u32_BitIdx < ((pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt)<<3))
				{
					if(0xFF == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] |= (1<<(u32_BitIdx&7));
				    else if(0 == ECC_TEST_PATTERN)
					    au8_WSpare[u32_BitIdx/8] &= ~(1<<(u32_BitIdx&7));
				}
				else
				{
					U8 u8_Tmp, u8_PosFlag=1;
					for(u8_Tmp=u16_Tmp; u8_Tmp>0; u8_Tmp--)
						if(au32_SymPos[u16_Tmp] == au32_SymPos[u8_Tmp-1])
							u8_PosFlag = 0;
					if(u8_PosFlag)
					    au8_WSpare[u32_BitIdx/8] ^= (1<<(u32_BitIdx&7));	
				}					
				//nand_debug(0,1,"spare: %X\n", au8_WSpare[u32_BitIdx/8]);
			}			
		}
		// check data
		u8_ErrBytes = 0;
		for(u32_i=0; u32_i<pNandDrv->u16_SectorByteCnt; u32_i++)
		    if(au8_W[u32_i] != au8_R[u32_i])
		    {
				u8_ErrBytes++;
			    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 8, I:%X W:%X R:%X, Pos: %X %X %X %X\n", 
				     u32_i, au8_W[u32_i], au8_R[u32_i], 
				     au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
			}
		u8_SpareErrBytes = 0;
	    for(u32_i=0; u32_i<pNandDrv->u16_SectorSpareByteCnt; u32_i++)
		    if(au8_WSpare[u32_i] != au8_RSpare[u32_i])
		    {
				u8_SpareErrBytes++;
			    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH4b_512B_BurnIn 9, I:%X W:%X R:%X, Pos: %X %X %X %X\n", 
				     u32_i, au8_WSpare[u32_i], au8_RSpare[u32_i], 
				     au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
		    }
		if(u8_ErrBytes > 4 || u8_SpareErrBytes > 4 || 
			(u8_ErrBytes+u8_SpareErrBytes) > 4)
		{
			nand_debug(0, 1, "NCTEST_ECC_BCH4b_512B_BurnIn 10, Pos: %X %X %X %X\n", 
				     au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
		    return UNFD_ST_ERR_ECC_FAIL;
		}		

		//while(1);
		#if 1
		au32_SymPos[0] = (au32_SymPos[0] - 3) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[1] = (au32_SymPos[1] + 5) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[2] = (au32_SymPos[2] + 1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		au32_SymPos[3] = (au32_SymPos[3] - 1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		#else
		au32_SymPos[0] = (au32_SymPos[0]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		if(0 == au32_SymPos[0])
			au32_SymPos[1] = (au32_SymPos[1]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		if(0 == au32_SymPos[0] && 0 == au32_SymPos[1])
			au32_SymPos[2] = (au32_SymPos[2]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		if(0 == au32_SymPos[0] && 0 == au32_SymPos[1] && 0 == au32_SymPos[2])
			au32_SymPos[3] = (au32_SymPos[3]+1) % ((pNandDrv->u16_SectorByteCnt+pNandDrv->u16_SectorSpareByteCnt)<<3);
		#endif

		u32_PageIdx++;
		if (u32_PageIdx >= pNandDrv->u16_BlkCnt << pNandDrv->u8_BlkPageCntBits)
		{
			u32_PageIdx = 0;
			u32_Round++;
			nand_debug(0, 1, "Round %08X, Start Pos: %04X %04X %04X %04X\n", u32_Round,
				au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);
			//NCTEST_DumpBadBlk(pu8_BadBlk);
		}
		
	}
	
	
    LABEL_END_OF_BURN_IN:
	nand_debug(0, 1, "Burn-in failed\n");
	nand_debug(0, 1, "PageIdx: 0x%08X, Pos: i: 0x%05X  j: 0x%05x  k: 0x%05X  l: 0x%05X\n",
			u32_PageIdx, au32_SymPos[0], au32_SymPos[1], au32_SymPos[2], au32_SymPos[3]);

	nand_debug(0, 1, "Reg51: 0x%04x, Reg52: 0x%04x, Reg53: 0x%04x\n",
			u16_Reg51, u16_Reg52, u16_Reg53);

	NCTEST_DumpBadBlk(pu8_BadBlk);
	nand_die();
	return UNFD_ST_SUCCESS;
}


static U32 NCTEST_ECC_BCH_512B(U8 u8_BCHBitCnt) // ok for BCH 4/8
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err, u32_i, u32_j;
	volatile U8  u8_Data, u8_Flag;
	volatile U16 u16_Reg51, u16_Reg52, u16_Reg53;

	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	// ----------------------------
    // setup data
    // ----------------------------
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for (u32_i = 0; u32_i < pNandDrv->u16_PageByteCnt; u32_i++)
	{
		au8_W[u32_i] = ECC_TEST_PATTERN;
		au8_R[u32_i] = ~au8_W[u32_i];
	}
	for (u32_i = 0; u32_i < pNandDrv->u16_SectorSpareByteCnt; u32_i++)
	{
		au8_WSpare[u32_i] = ECC_TEST_PATTERN;
		au8_RSpare[u32_i] = ~au8_WSpare[u32_i];
	}
	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH_512B 1, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}

	// ----------------------------
    // W/R to get ECC in CIFD
    // ----------------------------
    //nand_debug(0,1,"LA 0\n");
    //nand_hw_timer_delay(0x100000);
    //nand_debug(0,1,"LA 1\n");
    //nand_hw_timer_delay(0x100000);
    //nand_debug(0,1,"LA 2\n");
	//nand_hw_timer_delay(0x100000);
    	
	dump_mem(au8_WSpare, pNandDrv->u16_SpareByteCnt);
	u32_Err = NC_WriteSector(0, 0, au8_W, au8_WSpare);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH_512B 2, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}
	u32_Err = NC_ReadSectors(0, 0, au8_R, au8_RSpare, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH_512B 3, ErrCode: 0x%08X\n", u32_Err);
		return u32_Err;
	}
	nand_debug(0, 1, "dump: \n");
	NC_GetCIFD(au8_RSpare, 0, pNandDrv->u16_SpareByteCnt);
	dump_mem(au8_RSpare, pNandDrv->u16_SpareByteCnt);
	
	for(u32_i=0; u32_i<pNandDrv->u16_SectorByteCnt; u32_i++)
		if(au8_W[u32_i] != au8_R[u32_i])
		{
			nand_debug(0, 0, "ERROR: NCTEST_ECC_BCH_512B 4, I:%X W:%X R:%X\n", 
				u32_i, au8_W[u32_i], au8_R[u32_i]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}
	for(u32_i=0; u32_i<pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt; u32_i++)
		if(au8_WSpare[u32_i] != au8_RSpare[u32_i])
		{
			nand_debug(0, 0, "ERROR: NCTEST_ECC_BCH_512B 5, I:%X W:%X R:%X\n", 
				u32_i, au8_WSpare[u32_i], au8_RSpare[u32_i]);
			return UNFD_ST_ERR_DATA_CMP_FAIL;
		}

	// ----------------------------
    // corrupt data bits
    // ----------------------------
    u8_Flag = 0;
    
    for(u32_j=0; u32_j<u8_BCHBitCnt; u32_j++)
    {
		nand_debug(0, 1, "corrupt %Xh bits\n", u32_j+1);
		if(0 == (u32_j & 7))
			u8_Data = ECC_TEST_PATTERN;

		#if 0xFF == ECC_TEST_PATTERN
		u8_Data &= ~(1<<(u32_j&7));
		#elif 0 == ECC_TEST_PATTERN
        u8_Data |= (1<<(u32_j&7));
		#endif
		au8_W[u32_j/8] = u8_Data;
		nand_debug(0,1,"Data: %X\n", au8_W[u32_j/8]);

		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_BYPASS_ECC;
	    NC_Config();
	    u32_Err = NC_WriteSector(u32_j+1, 0, au8_W, au8_RSpare);
	    if (u32_Err != UNFD_ST_SUCCESS)
	    {
		    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH_512B 6, ErrCode: 0x%08X\n", u32_Err);
		    return u32_Err;
	    }
	    pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_BYPASS_ECC;
	    NC_Config();

		u32_Err = NC_ReadSectors(u32_j+1, 0, au8_R, au8_RSpare, 1);
	    if (u32_Err != UNFD_ST_SUCCESS)
	    {
		    nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH_512B 7, ErrCode: 0x%08X\n", u32_Err);
		    return u32_Err;
	    }

		//dump_mem(au8_W, 8);
		//dump_mem(au8_R, 8);

		// check data
		for(u32_i=0; u32_i<pNandDrv->u16_SectorByteCnt; u32_i++)
		    if(au8_W[u32_i] != au8_R[u32_i])
		    {
			    nand_debug(0, 1, "NCTEST_ECC_BCH_512B 8, I:%X W:%X R:%X\n", 
				     u32_i, au8_W[u32_i], au8_R[u32_i]);

				u8_Flag = 1;
			}
	    for(u32_i=0; u32_i<pNandDrv->u16_SectorSpareByteCnt-pNandDrv->u16_ECCCodeByteCnt; u32_i++)
		    if(au8_WSpare[u32_i] != au8_RSpare[u32_i])
		    {
			    nand_debug(0, 1, "NCTEST_ECC_BCH_512B 9, I:%X W:%X R:%X\n", 
				    u32_i, au8_WSpare[u32_i], au8_RSpare[u32_i]);
			    u8_Flag = 1;
		    }

		// check registers
		REG_READ_UINT16(NC_ECC_STAT0, u16_Reg51);
	    REG_READ_UINT16(NC_ECC_STAT1, u16_Reg52);
	    REG_READ_UINT16(NC_ECC_STAT2, u16_Reg53);
	    nand_debug(0, 1, "Reg51: 0x%04x  Reg52: 0x%04x  Reg53: 0x%04x\n",
			   u16_Reg51, u16_Reg52, u16_Reg53);

		if(1!=(u16_Reg53 & BIT_NC_ECC_FLGA_MASK) && u32_j<(u8_BCHBitCnt-1))
		{
			nand_debug(0, 1, "Reg53 incorrect 0: %X \n", u16_Reg53);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		if(2!=(u16_Reg53 & BIT_NC_ECC_FLGA_MASK) && u32_j==(u8_BCHBitCnt-1))
		{
			nand_debug(0, 1, "Reg53 incorrect 1: %X \n", u16_Reg53);
			return UNFD_ST_ERR_ECC_FAIL;
		}

		if(1!=u16_Reg52 && u32_j<(u8_BCHBitCnt-1))
		{
			nand_debug(0, 1, "Reg52 incorrect 0: %X \n", u16_Reg52);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		if(1==u16_Reg52 && u32_j==(u8_BCHBitCnt-1))
		{
			nand_debug(0, 1, "Reg52 incorrect 1: %X \n", u16_Reg52);
			return UNFD_ST_ERR_ECC_FAIL;
		}

		if((u16_Reg51&BIT_NC_ECC_FAIL) && u32_j<(u8_BCHBitCnt-1))
		{
			nand_debug(0, 1, "Reg51 incorrect 0: %X \n", u16_Reg51);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		if(0==(u16_Reg51&BIT_NC_ECC_FAIL) && u32_j==(u8_BCHBitCnt-1))
		{
			nand_debug(0, 1, "Reg51 incorrect 1: %X \n", u16_Reg51);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		if(((u16_Reg51&BIT_NC_ECC_MAX_BITS_MASK)>>1)!=(u32_j+1) && u32_j<(u8_BCHBitCnt-1))
		{
			nand_debug(0, 1, "Reg51 incorrect 2: %X \n", u16_Reg51);
			return UNFD_ST_ERR_ECC_FAIL;
		}
		
		//u8_Flag = 0;
    }

	return UNFD_ST_SUCCESS;
}


static U32 NCTEST_ECC_BCH24b_1KB_RIU(void)
{
	U32 u32_Err, u32_i, u32_j, u32_TestCol, u32_TestRow;
	U8  u8_Data;
	volatile U16 u16_Reg51, u16_Reg52, u16_Reg53, u16_Reg54;
	volatile U8  *au8_R = (U8*)DMA_R_ADDR, *au8_RSpare = (U8*)DMA_R_SPARE_ADDR;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	nand_debug(2, 1, "setup data...\n");
	u32_Err = ECCSectorSetup();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB_RIU 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_TestRow = 0;
	u32_TestCol = 1;

#if 1
	u8_Data = 0xFE;
	nand_debug(2, 1, "\ncorrupt data: 0x%x\n", u8_Data);
	nand_debug(2, 1, "=============\n");
	for (u32_i = 0; u32_i < 1; u32_i++) // test 1 sector for FCIE3 can not support 1KB page size
	{
		nand_debug(0, 1, "\nTestRow: 0x%x,  TestCol: 0x%x\n",
				u32_TestRow, u32_TestCol);

		for (u32_j = 0; u32_j < 24; u32_j += 1) // corrupt 1~24 bytes
		{
			nand_debug(2, 1, "  corrupt %u byte...\n", u32_j + 1);

			u32_Err = NC_Write_RandomOut(u32_TestRow,
					u32_TestCol + u32_j, &u8_Data, 1);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB_RIU 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}

			NCTEST_ECCSectorReadBack(u32_TestRow);

			if (u32_j == 12)
				u32_TestCol = 0x800;
		}

		u32_TestCol += pNandDrv->u16_SectorByteCnt;
	}

	// read back
	u32_Err = NC_ReadSector_RIUMode(u32_TestRow, 0, (U8*)au8_R, (U8*)au8_RSpare);
	if (u32_Err != UNFD_ST_SUCCESS && UNFD_ST_ERR_ECC_FAIL_RM != u32_Err)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB_RIU 2, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}
	// check data
	for (u32_i = 0; u32_i < pNandDrv->u16_SectorByteCnt; u32_i++)
	{
		if (ECC_TEST_PATTERN != au8_R[u32_i])
		{
			nand_debug(0, 1, "Err: au8_R[0x%04x]: 0x%02x != 0x%x\n", u32_i, au8_R[u32_i], ECC_TEST_PATTERN);
			//while (1) nand_reset_WatchDog();
		}
	}
	for (u32_i = 0; u32_i < pNandDrv->u16_SectorSpareByteCnt; u32_i++)
	{
		if (u32_i == pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt +
				pNandDrv->u16_SectorSpareByteCnt * (u32_i / pNandDrv->u16_SectorSpareByteCnt))
		{
			u32_i += pNandDrv->u16_ECCCodeByteCnt;
			u32_i -= 1;  continue;
		}

		if (ECC_TEST_PATTERN != au8_RSpare[u32_i])
		{
			nand_debug(0, 1, "Err: au8_RSpare[0x%04x]: 0x%02x != 0x%x\n", u32_i, au8_RSpare[u32_i], ECC_TEST_PATTERN);
			//while (1) nand_reset_WatchDog();
		}
	}

	//check RIU Mode ECC registers
	REG_READ_UINT16(NC_ECC_STAT0, u16_Reg51);
	REG_READ_UINT16(NC_ECC_STAT1, u16_Reg52);
	REG_READ_UINT16(NC_ECC_STAT2, u16_Reg53);
	nand_debug(2, 1, "Reg51: 0x%04x  Reg52: 0x%04x  Reg53: 0x%04x\n", u16_Reg51, u16_Reg52, u16_Reg53);
	if (u16_Reg51 & BIT_NC_ECC_FAIL)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB_RIU 3, ErrCode: 0x%08x\n", UNFD_ST_ERR_INVALID_ECC_REG51);
		return UNFD_ST_ERR_INVALID_ECC_REG51;
	}
	/*if (1 != u16_Reg52)
	  {
	  nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB_RIU 4, ErrCode: 0x%08x\n", UNFD_ST_ERR_INVALID_ECC_REG52);
	  return UNFD_ST_ERR_INVALID_ECC_REG52;
	  }*/
	if ((u16_Reg53 & BIT_NC_ECC_FLGA_MASK) != BIT_NC_ECC_FLGA_CORRECT)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB_RIU 5, ErrCode: 0x%08x\n", UNFD_ST_ERR_INVALID_ECC_REG53);
		return UNFD_ST_ERR_INVALID_ECC_REG53;
	}
	u32_i = (u16_Reg51 & BIT_NC_ECC_MAX_BITS_MASK)>>1;
	nand_debug(0, 1, "0x%x bits to be corrected\n", u32_i);
	for (u32_j = 0; u32_j < u32_i; u32_j++)
	{
		REG_CLR_BITS_UINT16(NC_ECC_STAT2, BIT_NC_ECC_SEL_LOC_MASK);
		REG_SET_BITS_UINT16(NC_ECC_STAT2, u32_j<<BIT_NC_ECC_SEL_LOC_SHIFT);
		REG_READ_UINT16(NC_ECC_LOC, u16_Reg54);
		nand_debug(0, 1, " err:bit-0x%02x, loc: 0x%04x-th bit\n", u32_j, u16_Reg54);
	}

#endif
	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECC_BCH32b_1KB(void)
{
	U32 u32_Err, u32_i, u32_j, u32_TestCol, u32_TestRow;
	U8  u8_Data;
	volatile U16 u16_Reg51, u16_Reg52, u16_Reg53;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	nand_debug(2, 1, "setup data...\n");
	u32_Err = ECCSectorSetup();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH32b_1KB 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_TestRow = 0;
	u32_TestCol = 1;

#if 1
	u8_Data = 0xFE;
	nand_debug(2, 1, "\ncorrupt data: 0x%x\n", u8_Data);
	nand_debug(2, 1, "=============\n");
	for (u32_i = 0; u32_i < 1; u32_i++) // test 1 sector for FCIE3 can not support 1KB page size
	{
		nand_debug(0, 1, "\nTestRow: 0x%x,  TestCol: 0x%x\n",
				u32_TestRow, u32_TestCol);

		for (u32_j = 0; u32_j < 33; u32_j += 1) // corrupt 1~33 bytes
		{
			nand_debug(2, 1, "  corrupt %u byte...\n", u32_j + 1);
			u32_Err = NC_Write_RandomOut(u32_TestRow,
					u32_TestCol + u32_j, &u8_Data, 1);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "ERROR: ECC_BCH32b_1KB 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}

			NCTEST_ECCSectorReadBack(u32_TestRow);
		}

		u32_TestCol += pNandDrv->u16_SectorByteCnt;
	}

#endif

	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECC_BCH24b_1KB(void)
{
	U32 u32_Err, u32_i, u32_j, u32_TestCol, u32_TestRow;
	U8  u8_Data;
	volatile U16 u16_Reg51, u16_Reg52, u16_Reg53;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	nand_debug(2, 1, "setup data...\n");
	u32_Err = ECCSectorSetup();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_TestRow = 0;
	u32_TestCol = 1;

#if 1
	u8_Data = 0xFE;
	nand_debug(2, 1, "\ncorrupt data: 0x%x\n", u8_Data);
	nand_debug(2, 1, "=============\n");
	for (u32_i = 0; u32_i < 1; u32_i++) // test 1 sector for FCIE3 can not support 1KB page size
	{
		nand_debug(0, 1, "\nTestRow: 0x%x,  TestCol: 0x%x\n",
				u32_TestRow, u32_TestCol);

		for (u32_j = 0; u32_j < 25; u32_j += 1) // corrupt 1~25 bytes
		{
			nand_debug(2, 1, "  corrupt %u byte...", u32_j + 1);
			u32_Err = NC_Write_RandomOut(u32_TestRow,
					u32_TestCol + u32_j, &u8_Data, 1);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}

			NCTEST_ECCSectorReadBack(u32_TestRow);
		}

		u32_TestCol += pNandDrv->u16_SectorByteCnt;
	}

#endif
	return UNFD_ST_SUCCESS;
}

static U32 NCTEST_ECC_BCH12b_512B(void)
{
	U32 u32_Err, u32_i, u32_j, u32_TestCol, u32_TestRow;
	U8  u8_Data;
	volatile U16 u16_Reg51, u16_Reg52, u16_Reg53;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	nand_debug(2, 1, "setup data...\n");
	// [CAUTION]:
	u32_Err = NCTEST_RWSinglePages(0, 1);//pNandDrv->u16_BlkPageCnt);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH12b 0, ErrCode: 0x%08x\n", u32_Err);
		return u32_Err;
	}

	u32_TestRow = 0;
	u32_TestCol = 1;

#if 1
	u8_Data = 0xFE;
	nand_debug(2, 1, "\ncorrupt data: 0x%x\n", u8_Data);
	nand_debug(2, 1, "=============");
	for (u32_i = 0; u32_i < pNandDrv->u16_PageSectorCnt; u32_i++)
	{
		nand_debug(0, 1, "\nTestRow: 0x%x,  TestCol: 0x%x\n",
				u32_TestRow, u32_TestCol);

		for (u32_j = 0; u32_j < 13; u32_j++) // corrupt 1~13 bytes
		{
			nand_debug(2, 1, "  corrupt %u byte...\n", u32_j + 1);
			u32_Err = NC_Write_RandomOut(u32_TestRow,
					u32_TestCol + u32_j, &u8_Data, 1);
			if (u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "ERROR: NCTEST_ECC_BCH12b 1, ErrCode: 0x%08x\n", u32_Err);
				return u32_Err;
			}

			NCTEST_ECCDataReadBack(u32_TestRow);
		}

		u32_TestCol += pNandDrv->u16_SectorByteCnt;
	}

#endif
	return UNFD_ST_SUCCESS;
}

#endif

//volatile struct fcie_ctrl_struct *fcie = (struct fcie_ctrl_struct*)FCIE_REG_BASE_ADDR;

static U32 NCTEST_PowerSave(void)
{
  #if defined(FCIE_POWER_SAVE_MODE) && FCIE_POWER_SAVE_MODE
	U16 u16_Reg;
	/*
	 * Enable Power Save HW and use RIU interface to test power save function
	 */
    #if 0
    // [CAUTION]: have to reset at first.
    //REG_CLR_BITS_UINT16(NC_PWRSAVE_CTL, BIT_SD_POWER_SAVE_RST);

	REG_WRITE_UINT16(NC_PWRSAVE_CTL,
	    BIT_POWER_SAVE_MOUE | BIT_POWER_SAVE_MODE_INT_EN | BIT_SD_POWER_SAVE_RST);
    #endif
	/*
	 * Write script file to Power_Save_FIFO
	 */

	/* (1) Clear HW Enable */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x40), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x41),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0A);

	/* (2) Clear All Interrupt */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x42), 0xffff);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x43),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x00);

	/* (3) Clear SDE MODE Enable */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x44), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x45),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x10);

	/* (4) Clear SDE CTL Enable */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x46), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x47),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x11);

	/* (5) Reset Start */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x48), 0x4800);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x49),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x30);

	/* (6) Reset End */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x4A), 0x5800);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x4B),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x30);

	/* (7) Set "SD_MOD" */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x4C), 0x0053);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x4D),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x10);

	/* (8) Enable "csreg_sd_en" */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x4E), 0x0002);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x4F),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0A);

	/* (9) Command Content, STOP */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x50), 0x004C);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x51),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK1 | 0x00);

	/* (10) Command Content, STOP */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x52), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x53),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK1 | 0x01);

	/* (11) Command Content, STOP */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x54), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x55),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK1 | 0x02);

	/* (12) Command Size */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x56), 0x0005);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x57),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0E);

	/* (13) Response Size */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x58), 0x0005);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x59),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0D);

	/* (14) Enable Interrupt, SD_CMD_END, NC_JOB_END */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x5A), 0x0202);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x5B),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x01);

	/* (15) Command Enable */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x5C), 0x0006);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x5D),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x11);

	/* (16) Wait Interrupt */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x5E), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x5F),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WINT);

	/* (17) Clear Interrupt */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x60), 0x0002);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x61),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x00);

	/* (18) Command Content, IDLE */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x62), 0x0040);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x63),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK1 | 0x00);

	/* (19) Command Enable */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x64), 0x0004);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x65),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x11);

	/* (20) Wait Interrupt */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x66), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x67),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WINT);

	/* (21) Clear Interrupt */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x68), 0x0002);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x69),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x00);

	/* (22) Enable "csreg_nc_en */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x6A), 0x0020);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x6B),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0A);

	/* (23) Enable "ce_en, ce_auto, wp_en, wp_auto" */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x6C), 0x003C);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x6D),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x40);

	/* (24) RESET Command queue */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x6E), 0x0020);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x6F),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x43);

	/* (25) RESET Command queue	*/
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x70), 0x8804);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x71),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x44);

	/* (26) RESET Command queue	*/
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x72), 0x8888);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x73),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x44);

	/* (27) JOB_START */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x74), 0x0001);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x75),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x45);

	/* (28) Wait Interrupt */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x76), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x77),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WINT);

	/* (29) Clear JOB_END Interrupt */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x78), 0x0200);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x79),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x00);

	/* (30) Clear HW Enable */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x7A), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x7B),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WREG | PWR_CMD_BK0 | 0x0A);

	/* (31) STOP */
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x7C), 0x0000);
	REG_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE1_ADDR, 0x7D),
					 PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_STOP);

	/* Start "Nand/Card IP" to read/write data from/to "Nand Flash/Card" */
	{
		NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

		U16 u16_Tmp;
		U32 u32_PageCnt = 0x40;
		U32 u32_PhyRowIdx = 0;
		U32 u32_DataDMAAddr;
		U8* pu8_DataBuf = (U8*)DMA_W_ADDR;

		nand_pads_switch(1);
		REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END|BIT_MMA_DATA_END);

		u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_PageByteCnt * u32_PageCnt);

		REG_WRITE_UINT16(NC_JOB_BL_CNT, u32_PageCnt << pNandDrv->u8_PageSectorCntBits);
		REG_WRITE_UINT16(NC_SDIO_ADDR0, u32_DataDMAAddr & 0xFFFF);
		REG_WRITE_UINT16(NC_SDIO_ADDR1, u32_DataDMAAddr >> 16);
		REG_SET_BITS_UINT16(NC_MMA_PRI_REG, BIT_NC_DMA_DIR_W);
		REG_SET_BITS_UINT16(NC_PATH_CTL, BIT_MMA_EN);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x80));
		REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_SER_DOUT);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_REPEAT << 8) | ACT_CHKSTATUS);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt - 1);

		for (u16_Tmp=0; u16_Tmp < pNandDrv->u16_PageSectorCnt; u16_Tmp++)
			NC_SetCIFD_Const(0xFF, pNandDrv->u16_SectorSpareByteCnt * u16_Tmp,
							 pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt);

		REG_WRITE_UINT16(NC_CTRL,
						 (BIT_NC_CIFD_ACCESS|BIT_NC_JOB_START|BIT_NC_IF_DIR_W));
		//while(1);
	}

	nand_hw_timer_delay(HW_TIMER_DELAY_1ms);
	/* Step 4: Before Nand IP DMA end, use RIU interface to test power save function */
	REG_WRITE_UINT16(NC_PWRSAVE_CTL, BIT_POWER_SAVE_MODE
					 | BIT_SD_POWER_SAVE_RIU
					 | BIT_POWER_SAVE_MODE_INT_EN
					 | BIT_SD_POWER_SAVE_RST);

	/* Step 5: Check Interrupt and interrupt type. */

	if (NC_WaitComplete(BIT_NC_POWER_SAVE, WAIT_RESET_TIME) == WAIT_RESET_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: timeout on waiting Reg00[BIT_NC_POWER_SAVE]\n");
		return UNFD_ST_ERR_RST_TIMEOUT;
	}

	REG_READ_UINT16(NC_PWRSAVE_CTL, u16_Reg);
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "NC_PWRSAVE_CTL: 0x%04x\n", u16_Reg);

	REG_WRITE_UINT16(NC_PWRSAVE_CTL, 0);
//	nand_die();
	//NC_Init();

	return UNFD_ST_SUCCESS;
  #endif
}

U32 NCTEST_ResetTorture(void)
{
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Rest torture\n");

	while(1)
	{
#if 0
		REG_CLR_BITS_UINT16(NC_TEST_MODE, BIT_FCIE_SOFT_RST); /* active low */
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms);
		REG_SET_BITS_UINT16(NC_TEST_MODE, BIT_FCIE_SOFT_RST);
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms);
#else
		U32 u32_Err = NC_ResetNandFlash();
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RestTorture failed with ErrCode: 0x%08x\n", u32_Err);
		}
#endif
	}
}



#define FCIE_IP_TEST_SUITE_ENABLE  0
U32 drvNAND_IPVerify_Main(void)
{
	volatile U32 u32_Err;
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "===================================\n");
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Unified NFIE Verification Program  \n");
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "===================================\n");

	/* pad switch */
	nand_pads_switch(1);

	/* set clock */
	nand_clock_setting(FCIE3_SW_DEFAULT_CLK);

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RIU_BASE: 0x%08x\n", RIU_BASE);
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "FCIE_REG_BASE_ADDR: 0x%08x\n", FCIE_REG_BASE_ADDR);
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "FCIE_NC_CIFD_BASE: 0x%08x\n", FCIE_NC_CIFD_BASE);
	/* clean NandDrv */
	memset(pNandDrv, '\0', sizeof(*pNandDrv));
	pNandDrv->pPartInfo = drvNAND_get_DrvContext_PartInfo();

	/* config NandDrv */
	pNandDrv->u8_OpCode_RW_AdrCycle = NAND_RW_ADR_CYCLE_CNT;
	pNandDrv->u8_OpCode_Erase_AdrCycle = NAND_E_ADR_CYCLE_CNT;
	pNandDrv->u16_ECCType = NAND_ECC_TYPE;

	pNandDrv->u16_BlkCnt = NAND_BLK_CNT;
	pNandDrv->u16_BlkPageCnt = NAND_BLK_PAGE_CNT;
	pNandDrv->u16_PageByteCnt = NAND_PAGE_BYTE_CNT;
	pNandDrv->u16_SpareByteCnt = NAND_SPARE_BYTE_CNT;
	pNandDrv->u8_IDByteCnt = NAND_DEVICE_ID_LEN;

    NC_PlatformInit();
	NC_ConfigContext();
	/* init & config NC */
	u32_Err = NC_Init();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: init FCIE fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}

	//NC_Config();
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "NFIE RIU access ok!\n\n");

	dump_nand_driver(pNandDrv);

#if 0
	//for(u32_Err=0; u32_Err<10; u32_Err++)
	//	NCTEST_DumpPages(pNandDrv->u16_BlkPageCnt * u32_Err, 1);
	NCTEST_DumpMG();
    while(1);	
#endif 

#if 0
	//NCTEST_EarseAllBlk();
	NCTEST_EraseAllGoodBlks();
	while(1);
#endif 

#if 0
	/* BA_IT_CTRL     x34001800 */
	nand_debug(0, 1, "BA_IT_CTRL: 0x%08x\n", BA_IT_CTRL);
	/* common/kernel.c: volatile W5IrqControl_t *const W5IrqControl */
	REG_CLR_BITS_UINT16(NC_MIE_EVENT, BIT_MMA_DATA_END | BIT_NC_JOB_END | BIT_NC_R2N_RDY | BIT_NC_R2N_ECC);
	REG_WRITE_UINT16(NC_MIE_INT_EN, BIT_MMA_DATA_END | BIT_NC_JOB_END | BIT_NC_R2N_RDY | BIT_NC_R2N_ECC);
	REG_WRITE_UINT16(NC_FORCE_INT, BIT6 | BIT7 |BIT9);
#endif

	//pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_DYNAMIC_OFFCLK; // for test
	//pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ALL0xFF_ECC_CHECK; // for test
	//pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_BYPASS_ECC; // for test
	//REG_WRITE_UINT16(NC_RAND_R_CMD, 0x2233);
	//REG_WRITE_UINT16(NC_RAND_W_CMD, 0x0066);
	//pNandDrv->u16_Reg48_Spare |= BIT_NC_HW_AUTO_RANDOM_CMD_DISABLE;
	// FIXME
	// nand_platform_init();

#if 0
	/* check the data bus */
	count_down(5);
	NCTEST_Check_DBus();  //goto LABEL_END_OF_TESTS;
	nand_die();
#endif

#if 0
	nand_debug(2, 1, "RW Verify 16bits waveform...\n");
	u32_Err = NCTEST_Verify_16bits(3, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: RW Verify 16bits waveform fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "RW Verify 16bits waveform end\n\n");
	while(1);
#endif


#if 1 || FCIE_IP_TEST_SUITE_ENABLE
	u32_Err = NCTEST_AccessCIFD();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: Access CIFD fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "CIFD scan ok!\n\n");
#endif


#if 1 || FCIE_IP_TEST_SUITE_ENABLE
    NCTEST_VerifyDMAAddress();
#endif


#if 1 || FCIE_IP_TEST_SUITE_ENABLE
    NC_ResetNandFlash();
	u32_Err = NCTEST_ReadID();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ReadID fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Read ID ok! -> issue CMD/ADR/DAT ok\n\n");
    #if 0
	NCTEST_InitDisp();
	nand_debug(0,1,"repeat reading id...\n");	
	while(1)
		if(UNFD_ST_SUCCESS != NCTEST_ReadID())
			break;

	*(volatile U16*)0x2A00724C |= (U16)BIT13; // not gate Disp clk]
	while(1);
	#endif	
#endif

	// NCTEST_ResetTorture();
#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	u32_Err = NC_EraseBlk(0);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_EraseBlk fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Erase Blk ok!\n\n");
#endif


#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RandomRW Test...\n");
	u32_Err = NCTEST_RandomRW();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RandomRW fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RandomRW ok!\n\n");
#endif


#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW Single Pages RIU Mode Test...\n");
	u32_Err = NCTEST_RWSinglePages_RM(0, 3);//3);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW Single Pages RM fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW Single Pages RM ok!\n");
#endif


#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW Single Pages Test...\n");
	u32_Err = NCTEST_RWSinglePages(4, 3);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW Single Pages fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW Single Pages ok!\n\n");
#endif


#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW MultiplePages_OneTime Test...\n");
	u32_Err = NCTEST_RWMultiplePages_OneTime(0, 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW MultiplePages_OneTime fail 0, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW MultiplePages_OneTime Test 0 ok!...\n");

	u32_Err = NCTEST_RWMultiplePages_OneTime(0, pNandDrv->u16_BlkPageCnt - 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW MultiplePages_OneTime fail 1, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW MultiplePages_OneTime Test 1 ok!...\n");

	/* The following test expect failed with "UNFD_ST_ERR_INVALID_PARAM" */
	u32_Err = NCTEST_RWMultiplePages_OneTime(0, pNandDrv->u16_BlkPageCnt);
	if (u32_Err != UNFD_ST_ERR_INVALID_PARAM)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW MultiplePages_OneTime fail 2, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW MultiplePages_OneTime ok! -> Repeat Cnt ok.\n\n");
#endif

#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW Single Pages Partial Mode Test...\n");
	u32_Err = NCTEST_RWSinglePages_Partial(0, 1);//3);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW Single Pages Partial Mode fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW Single Pages Partial Mode ok!\n\n");
    
	u32_Err = NCTEST_RWMultiplePages_OneTime_Partial(0, 1);//pNandDrv->u16_BlkPageCnt - 1);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW MultiplePages_OneTime_Partial fail 1, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}

	/* The following test expect failed with "UNFD_ST_ERR_INVALID_PARAM" */
	u32_Err = NCTEST_RWMultiplePages_OneTime_Partial(0, 1);//pNandDrv->u16_BlkPageCnt);
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: RW MultiplePages_OneTime_Partial fail 2, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "RW MultiplePages_OneTime ok! -> Repeat Cnt ok.\n\n");
#endif

#if 0  || FCIE_IP_TEST_SUITE_ENABLE
    /* SignatureCheck is temporarily disabled, Andersen will update for SW later. */
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "SignatureCheck Test...\n");
	u32_Err = NCTEST_SignatureCheck();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: SignatureCheck fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "SignatureCheck ok!\n\n");
#endif

#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "PageCopy Test...\n");
	u32_Err = NCTEST_PageCopy();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: PageCopy fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "PageCopy ok!\n\n");
#endif

#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ECC_For_All_0xff_Pages Test...\n");
	u32_Err = NCTEST_ECC_For_All_0xFF_Pages();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECC_For_All_0xff_Pages fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ECC_For_All_0xff_Pages ok!\n\n");
#endif


#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ECC_Generation_Bypass Test...\n");
	u32_Err = NCTEST_ECC_Generation_Bypass();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECC_Generation_Bypass fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ECC_Generation_Bypass ok!\n\n");
#endif

#if 0 || FCIE_IP_TEST_SUITE_ENABLE
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ECC_RS Test...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_RS();
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ECC_RS fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "ECC_RS ok!\n\n");
#endif

#if (0 || FCIE_IP_TEST_SUITE_ENABLE) && (defined(FCIE_POWER_SAVE_MODE) && FCIE_POWER_SAVE_MODE)
	u32_Err = NCTEST_PowerSave();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NCTEST_PowerSave fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "PowerSave Mode ok!\n\n");
#endif

#if 0
	nand_debug(2, 1, "ECC_BCH4b Test...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH4b_512B();
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH4b fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH4b ok!\n");
#endif

#if 0
	nand_debug(2, 1, "ECC_BCH8b Test...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH8b_512B();
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH8b fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH8b ok!\n");
#endif

#if 0
	nand_debug(2, 1, "ECC_BCH12b Test...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH12b_512B();
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH12b fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH12b ok!\n");
#endif

#if 0
	nand_debug(2, 1, "ECC_BCH24b Test...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH24b_1KB();
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH24b fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH24b ok!\n");
#endif

#if 0
	nand_debug(2, 1, "ECC_BCH32b Test...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH32b_1KB();
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH32b fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH32b ok!\n");
#endif

#if 0
	nand_debug(2, 1, "ECC_BCH24b_1KB_RIU Test...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH24b_1KB_RIU();
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: ECC_BCH24b_1KB_RIU fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH24b ok!\n");
#endif

#if 0
	nand_debug(2, 1, "Burn-In Test - ECC_BCH4b ...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH4b_512B_BurnIn(); /* never return */
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: _ECC_BCH4b_512B_BurnIn fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH4b_512B_BurnIn ok!\n");
#endif

#if 0
	nand_debug(2, 1, "Burn-In Test - ECC_BCH8b ...\n");
	pNandDrv->u8_IfECCTesting = 1;
	u32_Err = NCTEST_ECC_BCH8b_512B_BurnIn(); /* never return */
	pNandDrv->u8_IfECCTesting = 0;
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(0, 1, "ERROR: _ECC_BCH8b_512B_BurnIn fail, ErrCode: 0x%08x\n", u32_Err);
		goto LABEL_END_OF_TESTS;
	}
	nand_debug(2, 1, "ECC_BCH8b_512B_BurnIn ok!\n");
#endif

#if 0
    //NCTEST_InitDisp(); // [CAUTION]: Disp can not pattern generate, need Bradd or Jeff.Tsai help
    
    //NCTEST_TestReadWriteSpeed();
    //NCTEST_EraseAllGoodBlks();
    /*while(1)
    {
		u32_Err++;
		nand_clock_setting((u32_Err & 7) ? (u32_Err & 7):1);
		
		if(UNFD_ST_SUCCESS != NC_Write_RandomOut(pNandDrv->u16_BlkPageCnt<<1, 0, &u32_Err, 1))
		{
			nand_debug(0,1,"clk: %X\n", u32_Err & 7);
			while(1);
		}
    }*/
	NCTEST_ECC_RS_BurnIn();
	//NCTEST_RW_BurnIn();
#endif

#if defined(__VER_UNFD_FTL__) // for FTL verification
    {
		int int_Ret;  
		int_Ret = drv_unfd_init();
		nand_debug(0,1,"drv_unfd_init: %X\n", int_Ret);
		if(0==int_Ret)
			while(1);

		//nand_LUT_Test();

		//NCTEST_LogiWR_Once(0, 1);
		//NCTEST_LogiWR_Once(0x4500-3, 5);
		//NCTEST_LogiWR_Once(0x4503, 5);
		//nand_dump_FMap();
		
		NCTEST_LogiWR_Stress(LOGI_STRESS_TEST_SECTOR_CNT);

		//NCTEST_LogiWR_TestWBQ();
		
    }
#endif
    nand_debug(0, 1, "FCIE3 IP Verified OK!\n");
    while(1);
LABEL_END_OF_TESTS:
	nand_debug(UNFD_DEBUG_LEVEL, 1, "end of fail \n\n");
	nand_die();

	return UNFD_ST_SUCCESS;
}


#if defined(__VER_UNFD_FTL__)
void NCTEST_LogiWR_Stress(U32 u32_TotalSectorCnt)
{
	U32 u32_SecPos, u32_SecCnt, u32_i;
	volatile U8  *au8_W, *au8_R;
	volatile U8  *au8_WSpare, *au8_RSpare;

	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	if(u32_TotalSectorCnt > LOGI_STRESS_TEST_SECTOR_CNT)
	{
		nand_debug(0,1,"error invalid total sector cnt: %X \n", u32_TotalSectorCnt);
		return;
	}
	if(u32_TotalSectorCnt > LOGI_STRESS_TEST_SECTOR_ADR)
	{
		nand_debug(0,1,"error invalid total sector cnt: %X \n", u32_TotalSectorCnt);
		return;
	}

	// start test
	for (u32_SecPos=0; u32_SecPos<LOGI_STRESS_TEST_SECTOR_ADR; u32_SecPos++)
	{
		//nand_LUT_Test();
		nand_debug(0, 1, "\n StartSec: %08X \n", u32_SecPos);
		
		for (u32_SecCnt=0; u32_SecCnt<=u32_TotalSectorCnt; u32_SecCnt++)
		{
			for (u32_i=0; u32_i < u32_SecCnt<<9; u32_i++)
			{
				au8_TestBuf[u32_i] = (U8)(u32_i + u32_SecCnt);
			}
			nand_WriteFlow(u32_SecPos, u32_SecCnt, (U32)au8_TestBuf);

			for (u32_i=0; u32_i<(u32_SecCnt<<9); u32_i++)
			{
				au8_TestBuf[u32_i] = ~au8_TestBuf[u32_i];
			}				
			//nand_debug(0,0,"\n");
			nand_ReadFlow(u32_SecPos, u32_SecCnt, (U32)au8_TestBuf);
			for (u32_i=0; u32_i<(u32_SecCnt<<9); u32_i++)
			{
				if (au8_TestBuf[u32_i] != (U8)(u32_i + u32_SecCnt))
				{
					nand_debug(0, 1, "error.1.: u32_SecPos:%X  u32_SecCnt:%X  u32_i:%X  W:%X  R:%X\n", 
						u32_SecPos, u32_SecCnt, u32_i, (U8)(u32_i + u32_SecCnt), au8_TestBuf[u32_i]);
					while(1);
				}
			}

			//nand_debug(0,0,"\n\n");

			for (u32_i=0; u32_i < u32_SecCnt<<9; u32_i++)
			{
				au8_TestBuf[u32_i] = (U8)(u32_i + u32_SecCnt + 1);
			}
			nand_WriteFlow(LOGI_STRESS_TEST_SECTOR_ADR-u32_SecPos-1, u32_SecCnt, (U32)au8_TestBuf);

			for (u32_i=0; u32_i < u32_SecCnt<<9; u32_i++)
			{
				au8_TestBuf[u32_i] = ~au8_TestBuf[u32_i];
			}
			//nand_debug(0,0,"\n");
			nand_ReadFlow(LOGI_STRESS_TEST_SECTOR_ADR-u32_SecPos-1, u32_SecCnt, (U32)au8_TestBuf);
			for (u32_i=0; u32_i < u32_SecCnt<<9; u32_i++)
			{
				if (au8_TestBuf[u32_i] != (U8)(u32_i + u32_SecCnt + 1))
				{
					nand_debug(0, 1, "error.1.: u32_SecPos:%X  u32_SecCnt:%X  u32_i:%X  W:%X  R:%X\n", 
						u32_SecPos, u32_SecCnt, u32_i, (U8)(u32_i + u32_SecCnt + 1), au8_TestBuf[u32_i]);
					while(1);
				}
			}			
		}		
	}
		
}

void NCTEST_DataCompare(U8 *pu8_buf0, U8 *pu8_buf1, U32 u32_byteCnt)
{
	U32 u32_i;
	
	for (u32_i=0; u32_i < u32_byteCnt; u32_i++)
	{
		if (pu8_buf0[u32_i] != pu8_buf1[u32_i])
		{
			nand_debug(0, 1, "error: i:%X  buf0:%X  buf1:%X \n", 
				u32_i, pu8_buf0[u32_i], pu8_buf1[u32_i]);
			while(1);
		}
	}			
}
void NCTEST_LogiWR_TestFBQ(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_i, u32_Ret;
	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for(u32_i=0; u32_i<(512*0x200); u32_i++) // use 512 sectors
	{
		au8_W[u32_i] = u32_i;
		au8_R[u32_i] = ~au8_W[u32_i];
	}

	//------------------------------------------
	// test case.0
	u32_Ret = nand_WriteFlow(pNandDrv->u16_BlkSectorCnt*0, pNandDrv->u16_PageSectorCnt, (U32)au8_W + (0<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.0.0: %X \n", u32_Ret);

	u32_Ret = nand_WriteFlow(pNandDrv->u16_BlkSectorCnt*1, pNandDrv->u16_PageSectorCnt, (U32)au8_W + (1<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.0.1: %X \n", u32_Ret);
	
	u32_Ret = nand_WriteFlow(pNandDrv->u16_BlkSectorCnt*2, pNandDrv->u16_PageSectorCnt, (U32)au8_W + (2<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.0.2: %X \n", u32_Ret);

	u32_Ret = nand_WriteFlow(pNandDrv->u16_BlkSectorCnt*3, pNandDrv->u16_PageSectorCnt, (U32)au8_W + (3<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.0.3: %X \n", u32_Ret);
	
}

void NCTEST_LogiWR_TestWBQ(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_i, u32_Ret;
	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for(u32_i=0; u32_i<(512*0x200); u32_i++) // use 512 sectors
	{
		au8_W[u32_i] = u32_i;
		au8_R[u32_i] = ~au8_W[u32_i];
	}

	//------------------------------------------
	// test case.0
	for(u32_i=0; u32_i<10; u32_i++)
	{
		u32_Ret = nand_WriteFlow(u32_i, 1, (U32)au8_W + (u32_i<<9));
		if(UNFD_ST_SUCCESS != u32_Ret)
			nand_debug(0,1,"Err.0.0: %X \n", u32_Ret);
	}
	for(u32_i=0; u32_i<10; u32_i++)
	{
		u32_Ret = nand_ReadFlow(u32_i, 1, (U32)au8_R + (u32_i<<9));
		if(UNFD_ST_SUCCESS != u32_Ret)
			nand_debug(0,1,"Err.0.1: %X \n", u32_Ret);
	}
	NCTEST_DataCompare(au8_W, au8_R, 512*10);
	nand_debug(0,1," case.0 ok\n");	

	//------------------------------------------
	// test case.1
	u32_Ret = nand_WriteFlow(3, 5, (U32)au8_W + (3<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.1.0: %X \n", u32_Ret);
	
	u32_Ret = nand_ReadFlow(0, 10, (U32)au8_R);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.1.1: %X \n", u32_Ret);
	
	NCTEST_DataCompare(au8_W, au8_R, 512*10);
	nand_debug(0,1," case.1 ok\n");

	//------------------------------------------
	// test case.2
	u32_Ret = nand_WriteFlow(pNandDrv->u16_BlkSectorCnt+1, 3, (U32)au8_W+((pNandDrv->u16_BlkSectorCnt+1)<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.2.0: %X \n", u32_Ret);
	u32_Ret = nand_WriteFlow(pNandDrv->u16_BlkSectorCnt*2+1, 3, (U32)au8_W+((pNandDrv->u16_BlkSectorCnt*2+1)<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.2.1: %X \n", u32_Ret);
    u32_Ret = nand_WriteFlow(pNandDrv->u16_BlkSectorCnt*3+1, 3, (U32)au8_W+((pNandDrv->u16_BlkSectorCnt*3+1)<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.2.2: %X \n", u32_Ret);

	u32_Ret = nand_ReadFlow(pNandDrv->u16_BlkSectorCnt+1, 3, (U32)au8_R+((pNandDrv->u16_BlkSectorCnt+1)<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.2.3: %X \n", u32_Ret);
	u32_Ret = nand_ReadFlow(pNandDrv->u16_BlkSectorCnt*2+1, 3, (U32)au8_R+((pNandDrv->u16_BlkSectorCnt*2+1)<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.2.4: %X \n", u32_Ret);
	u32_Ret = nand_ReadFlow(pNandDrv->u16_BlkSectorCnt*3+1, 3, (U32)au8_R+((pNandDrv->u16_BlkSectorCnt*3+1)<<9));
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"Err.2.5: %X \n", u32_Ret);
	
	NCTEST_DataCompare((U8*)((U32)au8_W+((pNandDrv->u16_BlkSectorCnt+1)<<9)), (U8*)((U32)au8_R+((pNandDrv->u16_BlkSectorCnt+1)<<9)), 512*3);
	NCTEST_DataCompare((U8*)((U32)au8_W+((pNandDrv->u16_BlkSectorCnt*2+1)<<9)), (U8*)((U32)au8_R+((pNandDrv->u16_BlkSectorCnt*2+1)<<9)), 512*3);
	NCTEST_DataCompare((U8*)((U32)au8_W+((pNandDrv->u16_BlkSectorCnt*3+1)<<9)), (U8*)((U32)au8_R+((pNandDrv->u16_BlkSectorCnt*3+1)<<9)), 512*3);
	nand_debug(0,1," case.2 ok\n");

	
        
}


void NCTEST_LogiWR_Once(U32 u32_StartSectorIdx, U32 u32_SectorCnt)
{
	U32 u32_i, u32_Ret;
	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	/* setup buffers */
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	for(u32_i=0; u32_i<0x200; u32_i++)
	{
		au8_W[u32_i] = u32_i;
		au8_R[u32_i] = ~au8_W[u32_i];
	}

	u32_Ret = nand_WriteFlow(u32_StartSectorIdx, u32_SectorCnt, (U32)au8_W);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"WriteFlow: %X \n", u32_Ret);

	u32_Ret = nand_ReadFlow(u32_StartSectorIdx, u32_SectorCnt, (U32)au8_R);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(0,1,"ReadFlow: %X \n", u32_Ret);

	for(u32_i=0; u32_i<0x200; u32_i++)
	{
		if(au8_W[u32_i] != au8_R[u32_i])
			nand_debug(0,1,"Pos:%X  W:%X  R:%X\n", u32_i, au8_W[u32_i], au8_R[u32_i]);
	}
        
}
#endif


U32 NCTEST_TestReadWriteSpeed(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_i, u32_Err;

	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;
	
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	nand_debug(0, 1, "w start\n");
	
	for(u32_i=0; u32_i<(pNandDrv->u16_BlkCnt<<pNandDrv->u8_BlkPageCntBits); u32_i++)
	{
		#if 1
		u32_Err = NC_WritePages(u32_i, au8_W, au8_WSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_Test, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}
		#else
		u32_Err = NC_ReadPages(u32_i, au8_R, au8_RSpare, 1);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_Test, ErrCode: 0x%08x\n", u32_Err);
			return u32_Err;
		}
		#endif
	}
	
	nand_debug(0, 1, "end\n");
}



//==============================================
// for special WinTarget
//==============================================
void NCTEST_EarseAllBlk(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Ret;
	U16 u16_i, u16_Cnt;
    #if 0
	u32_Ret = drvNAND_Init();
	if(UNFD_ST_SUCCESS != u32_Ret)
	{
		nand_debug(0,1,"ERROR, NAND, no CIS\n");
		while(1);
	}
	#endif

	u16_Cnt=0;
	for(u16_i=10; u16_i<pNandDrv->u16_BlkCnt; u16_i++)
	{
		if(UNFD_ST_SUCCESS != NC_EraseBlk(u16_i << pNandDrv->u8_BlkPageCntBits))
		{
			nand_debug(0,1,"bad blk: %X \n", u16_i);
			u16_Cnt++;
		}
	}
	nand_debug(0,1,"total bad blk: %X \n", u16_Cnt);
}


U32 NCTEST_EraseAllGoodBlks(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Blk, u32_Row, u32_Err, u32_BadBlkCnt;

	U8  *au8_W, *au8_R;
	U8  *au8_WSpare, *au8_RSpare;

	#if 0
	//u32_Err = drvNAND_SearchCIS();
	u32_Err = drvNAND_Init();
	if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_debug(0,1,"ERROR, NAND, no CIS\n");
		while(1);
	}
	#endif
	au8_W = (U8*)DMA_W_ADDR;
	au8_R = (U8*)DMA_R_ADDR;
	au8_WSpare = (U8*)DMA_W_SPARE_ADDR;
	au8_RSpare = (U8*)DMA_R_SPARE_ADDR;

	u32_BadBlkCnt = 0;

	nand_debug(0, 1, "start\n");
	
	for(u32_Blk=10; u32_Blk<pNandDrv->u16_BlkCnt; u32_Blk++)
	{
		u32_Row = u32_Blk << pNandDrv->u8_BlkPageCntBits;
		
		u32_Err = NC_ReadPages(u32_Row, au8_R, au8_RSpare, 1);
		#if 1
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_Test, ErrCode: 0x%08x\n", u32_Err);
            drvNAND_MarkBadBlk(u32_Blk);
			u32_BadBlkCnt++;
			continue;
		}
		#endif
		
		if(0xFF != au8_RSpare[0])
		{
			nand_debug(0, 1, "bad blk: %X\n", u32_Blk);
			u32_BadBlkCnt++;
			continue;
		}		

		u32_Err = NC_EraseBlk(u32_Row);
		if (u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "bad blk : %X\n", u32_Blk);
			u32_BadBlkCnt++;
			continue;
		}
	}
	
	nand_debug(0,1,"total bad blk: %X \n", u32_BadBlkCnt);
}


#endif

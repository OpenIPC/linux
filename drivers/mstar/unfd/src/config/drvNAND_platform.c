//=======================================================================
//  MStar Semiconductor - Unified Nand Flash Driver
//
//  drvNAND_platform.c - Storage Team, 2009/08/20
//
//  Design Notes: defines common platform-dependent functions.
//
//    1. 2009/08/25 - support C5 eCos platform
//
//=======================================================================
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h>
//#include "hwreg.h"
#include "ms_platform.h"
#include "ms_types.h"
#include "drvNAND.h"


//=============================================================
//=============================================================
U32 nand_pads_init(void)
{
    NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();


#if (ENABLE_AGATE)
    U16 u16_NandMode;
 
	#if defined(reg_fcie_data_strength)
	REG_SET_BITS_UINT16(reg_fcie_data_strength,0xff);
	REG_SET_BITS_UINT16(reg_fcie_control_strength,0xff<<6);
	#endif

	REG_READ_UINT16(reg_nf_en, u16_NandMode);
	if( (BIT6)==(u16_NandMode & REG_NAND_MODE_MASK))
	{
		REG_SET_BITS_UINT16(reg_pcmd_pe, BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7);
	}
	else if( (BIT7)==(u16_NandMode & REG_NAND_MODE_MASK))
	{
		REG_SET_BITS_UINT16(reg_pcma_pe, BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
	}
#endif
#if (ENABLE_EAGLE)
    U16 u16_NandMode;
	REG_WRITE_UINT16(NC_PATH_CTL, 0x20);

	#if defined(reg_fcie_data_strength)
	REG_SET_BITS_UINT16(reg_fcie_data_strength,(0xff<<8));
	REG_SET_BITS_UINT16(reg_fcie_control_strength1,(0xff<<8));
	REG_SET_BITS_UINT16(reg_fcie_control_strength2, BIT0);
	#endif

	/*Check NAND Mode for PE setting*/
	REG_READ_UINT16(reg_nf_en, u16_NandMode);
	
	u16_NandMode &= REG_NAND_MODE_MASK;
	
	if(NAND_MODE3 == u16_NandMode || NAND_MODE1 == u16_NandMode)
	{
		//set nand mode to mode 3
		REG_READ_UINT16(reg_nf_en, u16_NandMode);
		u16_NandMode &= ~REG_NAND_MODE_MASK;
		u16_NandMode |= NAND_MODE3;
		REG_WRITE_UINT16(reg_nf_en, u16_NandMode);
	
		REG_SET_BITS_UINT16(REG_PCM_D_PE, BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7);
	}
	else if(NAND_MODE2 == u16_NandMode)
	{

		REG_SET_BITS_UINT16(REG_PCM_A_PE, BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15);
	}
	else if(NAND_MODE5 == u16_NandMode)
	{
		REG_SET_BITS_UINT16(REG_PCM2_CD_N, BIT9);
	}


#endif

	//Read CEz Configure Setting from u-boot before FCIE is reset.
#if defined(CONFIG_MSTAR_EAGLE) || defined(CONFIG_MSTAR_AGATE)
	if((REG(REG_NAND_CS1_EN) & BIT_NAND_CS1_EN) == BIT_NAND_CS1_EN)
	{
		pNandDrv->u16_Reg40_Signal =
			(BIT_NC_CE1Z | BIT_NC_WP_AUTO | BIT_NC_WP_H | BIT_NC_CE_AUTO | BIT_NC_CE_H) &
			~(BIT_NC_CHK_RB_EDGEn);
	}
	else
	{
		pNandDrv->u16_Reg40_Signal =
			(BIT_NC_WP_AUTO | BIT_NC_WP_H | BIT_NC_CE_AUTO | BIT_NC_CE_H) &
			~(BIT_NC_CHK_RB_EDGEn | BIT_NC_CE_SEL_MASK);
	}
#else
	pNandDrv->u16_Reg40_Signal =
			(BIT_NC_WP_AUTO | BIT_NC_WP_H | BIT_NC_CE_AUTO | BIT_NC_CE_H) &
			~(BIT_NC_CHK_RB_EDGEn | BIT_NC_CE_SEL_MASK);
#endif


	return UNFD_ST_SUCCESS;
}



#if defined(NAND_DRV_C5_ROM) && NAND_DRV_C5_ROM
extern void MDrv_EnableWatchdog(int enable);
extern void MDrv_ResetWatchdog(void);

U32  nand_hw_timer_delay(U32 u32usTick)
{
    delay_us(u32usTick+1);
    return u32usTick+1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	volatile U16 u16RegVal;

	//REG_SET_BITS_UINT16(NC_TEST_MODE, 0xDA<<8);
	REG_SET_BITS_UINT16(NC_TEST_MODE, 0xD8<<8);

	REG_SET_BITS_UINT16(reg_nf_en, BIT4|BIT7|BIT8);
	REG_CLR_BITS_UINT16(reg_nf_en, BIT5);

	REG_CLR_BITS_UINT16(reg_test_pad, BIT2|BIT3|BIT13);

	REG_SET_BITS_UINT16(reg_cardio_puz, BIT0|BIT1|BIT4);
	REG_CLR_BITS_UINT16(reg_cardio_puz, BIT2|BIT3);
	REG_SET_BITS_UINT16(reg_cardio_pd, BIT0|BIT1|BIT4);
	REG_CLR_BITS_UINT16(reg_cardio_pd, BIT2|BIT3);

	/* check Package ID	*/
	/* [CAUTION] ask HW designer */
	REG_READ_UINT16(reg_top_03, u16RegVal);
	if (0 == (u16RegVal & (BIT2|BIT1|BIT0)))
		REG_SET_BITS_UINT16(NC_TEST_MODE, BIT5);
	else
		REG_CLR_BITS_UINT16(NC_TEST_MODE, BIT5);

	if (PACKAGE_MSPD6521E==(u16RegVal & (BIT15-1)) ||
		PACKAGE_MSPD6251E==(u16RegVal & (BIT15-1))) {
		REG_READ_UINT16(reg_top_07, u16RegVal);
		if (1 < u16RegVal)
			REG_SET_BITS_UINT16(NC_REG_PAD_SWITCH, BIT0);
	}
	else {
		REG_CLR_BITS_UINT16(NC_REG_PAD_SWITCH, BIT0);
	}

#if NAND_DRIVER_ENV == NAND_ENV_FPGA
	REG_SET_BITS_UINT16(NC_TEST_MODE, BIT5);
#endif

#if defined(NAND_DESIGNIN_TEST) && 0!=NAND_DESIGNIN_TEST
	REG_READ_UINT16(reg_nf_en, u16RegVal);
	if (0==(u16RegVal & (BIT4|BIT7|BIT8)) || 1==(u16RegVal & BIT5)) {
		nand_deug(0, 1, "Err: switch pads fail! 0x%Xh \r\n", u16RegVal));
		return 1; // error
	}
#endif

	REG_WRITE_UINT16(NC_PATH_CTL, BIT_NC_EN);
	REG_WRITE_UINT16(NC_SDIO_CTL, pNandDrv->u16_Reg1B_SdioCtrl);
	return UNFD_ST_SUCCESS;
}

U32  nand_clock_setting(U32 u32ClkParam)
{
	REG_CLR_BITS_UINT16(reg_mclk_fcie_en, BIT5);

	/* enable FCIE clk, set to lowest clk */
	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT6-1);

	REG_SET_BITS_UINT16(reg_ckg_fcie, u32ClkParam);

	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	MDrv_EnableWatchdog(u8_IfEnable);
}

void nand_reset_WatchDog(void)
{
	MDrv_ResetWatchdog();
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	return u32_DMAAddr | 0x80000000;
}

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    /* 0xC0000000 ~ 0xC0000003 4B at 3GB address */
	return (void*)((*(U32*)0xC0000000) | 0x80000000);
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
	return (void*)((U32)drvNAND_get_DrvContext_address() + 1024);
}

U32 NC_PlatformInit(void)
{
	return UNFD_ST_SUCCESS;
}


//=============================================================
#elif (defined(NAND_DRV_C5_ECOS) && NAND_DRV_C5_ECOS) || (defined(NAND_DRV_C5_NPRG) && NAND_DRV_C5_NPRG)
U32 nand_hw_timer_delay(U32 u32usTick)
{
#if 1
	volatile U32 u32_Idx, u32Tick;

	u32Tick = (u32usTick >> 7)+ (u32usTick >> 7) +
		      (u32usTick >> 7)+ (u32usTick >> 7) +
		      (u32usTick >> 7)+ (u32usTick >> 7) +
		      (u32usTick >> 7)+ (u32usTick >> 6) + 8;

	for (u32_Idx = 0; u32_Idx < u32Tick; u32_Idx++)
		;	/* soft timer, temporarily coded. */

	return u32Tick;
#else
    delay_us((u32usTick/TIMER_DELAY_1us)+1);
    return (u32usTick/TIMER_DELAY_1us)+1;
#endif
}

/* if pin-shared with Card IF, need to call before every JOB_START. */
U32 nand_pads_switch(U32 u32EnableFCIE)
{
	volatile U16 u16_RegVal, u16_Tmp;

	REG_SET_BITS_UINT16(NC_TEST_MODE, 0xD8 << 8);

	if(u32EnableFCIE)
	{
		REG_SET_BITS_UINT16(reg_nf_en, BIT4|BIT7|BIT8);
		REG_CLR_BITS_UINT16(reg_nf_en, BIT5);

		REG_CLR_BITS_UINT16(reg_test_pad, BIT2|BIT3|BIT13);

		REG_SET_BITS_UINT16(reg_cardio_puz, BIT0|BIT1|BIT4);
		REG_CLR_BITS_UINT16(reg_cardio_puz, BIT2|BIT3);
		REG_SET_BITS_UINT16(reg_cardio_pd, BIT0|BIT1|BIT4);
		REG_CLR_BITS_UINT16(reg_cardio_pd, BIT2|BIT3);

        /* check Package ID */
        /* [CAUTION] ask HW designer */
		REG_READ_UINT16(reg_top_03, u16RegVal);
		if(0 == (u16RegVal & (BIT2|BIT1|BIT0)))
		{
		    REG_SET_BITS_UINT16(NC_TEST_MODE, BIT5);
			REG_READ_UINT16(NC_TEST_MODE, u16Tmp);
		}
		else
		{	REG_CLR_BITS_UINT16(NC_TEST_MODE, BIT5);
			REG_READ_UINT16(NC_TEST_MODE, u16Tmp);
		}

#if NAND_DRIVER_ENV == NAND_ENV_FPGA
        REG_SET_BITS_UINT16(NC_TEST_MODE, BIT5);
		REG_READ_UINT16(NC_TEST_MODE, u16Tmp);
		nand_printf("FPGA set Reg30h bit5: %Xh\r\n", u16Tmp);
#endif

#if NAND_TEST_IN_DESIGN
        REG_READ_UINT16(reg_nf_en, u16RegVal);
        if(0==(u16RegVal & (BIT4|BIT7|BIT8)) || 1==(u16RegVal & BIT5))
        {
			nand_debug(0, 1, "Err: switch pads fail! 0x%Xh \r\n", u16RegVal);
			return 1; // error
        }
#endif
	}

	REG_WRITE_UINT16(NC_PATH_CTL, BIT_NC_EN);
	return UNFD_ST_SUCCESS;
}

U32  nand_clock_setting(U32 u32ClkParam)
{
	REG_CLR_BITS_UINT16(reg_mclk_fcie_en, BIT5);

	/* enable FCIE clk, set to lowest clk */
	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT6-1);
	REG_SET_BITS_UINT16(reg_ckg_fcie, u32ClkParam);

	return 0;
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	NAND_DRIVER_PLAT_CTX *pCtx	= &pNandDrv->PlatCtx_t;
	U32 invbuf = u32_DMAAddr;

	if(invbuf & (HAL_DCACHE_LINE_SIZE-1))
	{
		/* for UNFD, the min u32_ByteCnt would be 512B. */
		u32_ByteCnt -= ((U32)invbuf & (HAL_DCACHE_LINE_SIZE-1));
        invbuf &= ~(HAL_DCACHE_LINE_SIZE-1);
        HAL_DCACHE_FLUSH( invbuf, HAL_DCACHE_LINE_SIZE );
        invbuf += HAL_DCACHE_LINE_SIZE;
    }

	HAL_DCACHE_FLUSH( invbuf, (u32_ByteCnt & ~(HAL_DCACHE_LINE_SIZE-1)) );
	//HAL_DCACHE_INVALIDATE( invbuf, (u32_ByteCnt & ~(HAL_DCACHE_LINE_SIZE-1)) );
    invbuf += u32_ByteCnt & ~(HAL_DCACHE_LINE_SIZE-1);

	if (u32_ByteCnt & (HAL_DCACHE_LINE_SIZE-1))
	{
		HAL_DCACHE_FLUSH( invbuf, HAL_DCACHE_LINE_SIZE );
    }

	return pCtx->u32_DMAAddrOffset + u32_DMAAddr;
}

extern NAND_DRIVER sg_NandDrv_t;
U32 drvNAND_get_DrvContext_address(void)
{
	return (U32)&sg_NandDrv_t;
}

void nand_reset_WatchDog(void)
{
	MDrv_ResetWatchdog();
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

        pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)malloc(pNandDrv->u16_SpareByteCnt);
        if (!pNandDrv->PlatCtx_t.pu8_PageSpareBuf)
        {
		nand_debug(0, 1, "Error: drvNAND_PlatformInit failed with EC: 0x%08X\n",
				   UNFD_ST_ERR_PLAT_INIT_0);
		return UNFD_ST_ERR_PLAT_INIT_0;
	}

	return UNFD_ST_SUCCESS;
}

//=============================================================
//=============================================================
#elif defined(NAND_DRV_B3_ROM) && NAND_DRV_B3_ROM
#include "bootrom.h"
#include "sysapi.h"

/* return Timer tick */
U32  nand_hw_timer_delay(U32 u32usTick)
{
#ifndef __ASIC_SIM__
  #if 1
    hal_delay_us(u32usTick);
  #else
	volatile int i = 0;

	for (i = 0; i < u32usTick; i++)
	{
		volatile int j = 0, tmp;
		for (j = 0; j < 0x2; j++)
		{
			tmp = j;
		}
	}
  #endif
#endif
    return u32usTick + 1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	return UNFD_ST_SUCCESS;
}

U32  nand_clock_setting(U32 u32ClkParam)
{
	NC_PlatformResetPre();
#if NAND_DRIVER_ENV == NAND_ENV_ASIC
#ifdef __BOOTROM__
	sys_pllCtrl(CRU_NFIE, CLK_10M40HZ);
#else
    REG_WRITE_UINT16(0x74007D4C, 0x0000); // disable clock fcie

    REG_CLR_BITS_UINT16(0x74007D4C, BIT7+BIT6+BIT5+BIT4); // mask all clock select

    switch(u32ClkParam)
    {
        case NFIE_CLK_187_5K:
            // no need to set
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 187.5K\r\n");
            break;
        case NFIE_CLK_750K:
            REG_SET_BITS_UINT16(0x74007D4C, BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 750K\r\n");
            break;
        case NFIE_CLK_6M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT5);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 6M\r\n");
            break;
        case NFIE_CLK_10_4M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 10.4M\r\n");
            break;
        case NFIE_CLK_13M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 13M\r\n");
            break;
        case NFIE_CLK_19_5M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 19.5M\r\n");
            break;
        case NFIE_CLK_22_29M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6+BIT5);
           //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 22.29M\r\n");
            break;
        case NFIE_CLK_26M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6+BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 26M\r\n");
            break;
        case NFIE_CLK_39M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 39M\r\n");
            break;
        case NFIE_CLK_44_57M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 44.57M\r\n");
            break;
        case NFIE_CLK_52M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7+BIT5);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 52M\r\n");
            break;
        case NFIE_CLK_78M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7+BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 78M\r\n");
            break;
        case NFIE_CLK_31_2M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT9+BIT8+BIT7+BIT6+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 31.2M\r\n");
            break;
        case NFIE_CLK_34_67M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT9+BIT8+BIT7+BIT6+BIT5);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 34.6M\r\n");
            break;
        case NFIE_CLK_29_71M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT10+BIT8+BIT7+BIT6+BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 29.7M\r\n");
            break;
        default:
            REG_SET_BITS_UINT16(0x74007D4C, BIT5+BIT4);
            nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"nand_clock_setting error parameter!!!\r\n");
            break;
    }

    REG_SET_BITS_UINT16(0x74007D4C, BIT3); // fast clock enable

	NC_PlatformResetPost();
#endif
#endif
	return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	u32_Clk = 2000000000/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_187_5K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_187_5K && u32_Clk < NFIE_CLK_750K )
		pNandDrv->u32_Clk = NFIE_CLK_187_5K;
	else if( u32_Clk >= NFIE_CLK_750K && u32_Clk < NFIE_CLK_6M )
		pNandDrv->u32_Clk = NFIE_CLK_750K;
	else if( u32_Clk >= NFIE_CLK_6M && u32_Clk < NFIE_CLK_10_4M )
		pNandDrv->u32_Clk = NFIE_CLK_6M;
	else if( u32_Clk >= NFIE_CLK_10_4M && u32_Clk < NFIE_CLK_13M )
		pNandDrv->u32_Clk = NFIE_CLK_10_4M;
	else if( u32_Clk >= NFIE_CLK_13M && u32_Clk < NFIE_CLK_19_5M )
		pNandDrv->u32_Clk = NFIE_CLK_13M;
	else if( u32_Clk >= NFIE_CLK_19_5M && u32_Clk < NFIE_CLK_22_29M )
		pNandDrv->u32_Clk = NFIE_CLK_19_5M;
	else if( u32_Clk >= NFIE_CLK_22_29M && u32_Clk < NFIE_CLK_26M )
		pNandDrv->u32_Clk = NFIE_CLK_22_29M;
	else if( u32_Clk >= NFIE_CLK_26M && u32_Clk < NFIE_CLK_29_71M )
		pNandDrv->u32_Clk = NFIE_CLK_26M;
	else if( u32_Clk >= NFIE_CLK_29_71M && u32_Clk < NFIE_CLK_31_2M )
		pNandDrv->u32_Clk = NFIE_CLK_29_71M;
	else if( u32_Clk >= NFIE_CLK_31_2M && u32_Clk < NFIE_CLK_34_67M )
		pNandDrv->u32_Clk = NFIE_CLK_31_2M;
	else if( u32_Clk >= NFIE_CLK_34_67M && u32_Clk < NFIE_CLK_39M )
		pNandDrv->u32_Clk = NFIE_CLK_34_67M;
	else if( u32_Clk >= NFIE_CLK_39M && u32_Clk < NFIE_CLK_44_57M )
		pNandDrv->u32_Clk = NFIE_CLK_39M;
	else if( u32_Clk >= NFIE_CLK_44_57M && u32_Clk < NFIE_CLK_52M )
		pNandDrv->u32_Clk = NFIE_CLK_44_57M;
	else if( u32_Clk >= NFIE_CLK_52M && u32_Clk < NFIE_CLK_78M )
		pNandDrv->u32_Clk = NFIE_CLK_52M;
	else
		pNandDrv->u32_Clk = NFIE_CLK_78M;

	if(sys_get_chipRev() < BIG3_U03)
	{
		switch(pNandDrv->u32_Clk)
		{
			case NFIE_CLK_29_71M:
			case NFIE_CLK_31_2M:
			case NFIE_CLK_34_67M:
				pNandDrv->u32_Clk = NFIE_CLK_26M;
				break;
			default:
				break;
		}
	}

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
}

void nand_reset_WatchDog(void)
{
}

/*
 * return DMA address for FCIE3 registers
 * mark to save code size
 */
U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
        hal_dcache_flush((void *)u32_DMAAddr, u32_ByteCnt);
        return sys_PHY2MIUAddr(u32_DMAAddr);
}

void nand_Invalidate_data_cache_buffer(U32 u32_addr, S32 s32_size)
{
	hal_dcache_invalidate((void *)u32_addr , s32_size);
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1 UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */

U32 NC_PlatformResetPre(void)
{
	//HalMiuMaskReq(MIU_CLT_FCIE);
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	//HalMiuUnMaskReq(MIU_CLT_FCIE);
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

    pNandDrv->u8_WordMode = sys_get_extMemType() == EXTMEM_NAND_16BITS;

	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;
	if(sys_get_chipRev() < BIG3_U03)
	{
		switch(pNandDrv->u32_Clk)
		{
			case NFIE_CLK_29_71M:
			case NFIE_CLK_31_2M:
			case NFIE_CLK_34_67M:
				pNandDrv->u32_Clk = NFIE_CLK_26M;
				break;
			default:
				break;
		}
	}
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);

    #if NAND_DRIVER_ENV == NAND_ENV_ASIC
    sys_pllCtrl(CRU_NFIE, CLK_10M40HZ);
    #endif

	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	nand_debug(0,1, "\n %X %X\n", pNandDrv->PlatCtx_t.pu8_PageDataBuf, pNandDrv->PlatCtx_t.pu8_PageSpareBuf);

	pNandDrv->u8_SwPatchWaitRb= 0;
	pNandDrv->u8_SwPatchJobStart= 0;
	NC_ConfigHal(NAND_HAL_RAW);

    return UNFD_ST_SUCCESS;
}

U32 drvNAND_init_sem(void)
{
	// ROM code environment doesn't need semaphore
	return UNFD_ST_SUCCESS;
}

U32 drvNAND_lock_sem(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	NC_ResetFCIE();	
	nand_clock_setting(pNandDrv->u32_Clk);
	// ROM code environment doesn't need semaphore
	return UNFD_ST_SUCCESS;
}

U32 drvNAND_unlock_sem(void)
{
	// ROM code environment doesn't need semaphore
	return UNFD_ST_SUCCESS;
}

void drvNAND_SetAssertPBA(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 u16_i;

	for(u16_i=0; u16_i<pNandDrv->pPartInfo->u16_PartCnt; u16_i++)
	{
		if(DEBUG_ASSERT_PARTITION_TYPE == pNandDrv->pPartInfo->records[u16_i].u16_PartType)
		{
			pNandDrv->u16_FirstPBA_AssertDebug = pNandDrv->pPartInfo->records[u16_i].u16_StartBlk;
			return;
		}		
	}
	pNandDrv->u16_FirstPBA_AssertDebug = 10; // protect CIS from FTL bugs
}

#if IF_IP_VERIFY
#define WRITE_DISP_REG(addr, type, data)  ((*(volatile type *)(addr)) = (data))
U32 NCTEST_InitDisp(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 i;

	nand_debug(0, 1, "init disp - x \n");

	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_SHARE_PAD_EN;
	NC_Config();

    //select display clock to 13MHz
    //WRITE_DISP_REG( 0x74007DD0, U16 , 0x308); // 13 MHz
    WRITE_DISP_REG( 0x74007DD0, U16 , 0x528);  // 20MHz

	*(volatile U16*)0x7400724C &= (U16)~BIT13; // not gate Disp clk
	*(volatile U16*)0x74007240 &= (U16)~BIT4; // not gate FCIE3 clk

    //Set reg_cmd_phase = 1
    WRITE_DISP_REG(0x740031a0, U16, 0x0001);

    ////PAD swtich to display.....
    WRITE_DISP_REG(0x74006614, U16, 0x0040);
    ////PAD swtich to display.....

    WRITE_DISP_REG(0x740031f4, U16, 0x0003);
    WRITE_DISP_REG(0x74003008, U16, 0x0034);
    WRITE_DISP_REG(0x74003120, U16, 0x0024);
    WRITE_DISP_REG(0x74003170, U16, 0x000f);
    WRITE_DISP_REG(0x74003174, U16, 0x0000);
    WRITE_DISP_REG(0x7400309c, U16, 0x00e4);
    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    WRITE_DISP_REG(0x7400313c, U16, 0x0001);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0017);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*25);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0010);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x17b0);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0011);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0012);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0119);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0013);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x1400);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0015);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0029);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000e);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x00fd);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0012);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0139);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*25);
    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0100);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0002);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0700);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0003);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x1030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0008);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0808);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0009);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0031);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0032);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0033);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0034);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0035);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0036);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0037);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0038);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0039);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003b);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0f0f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0050);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0051);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x00ef);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0052);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0053);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x013f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0060);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x2700);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0061);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x006a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0090);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0016);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0092);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0093);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);
    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0061);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);
    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0173);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0031);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0407);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0032);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0203);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0033);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0034);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0035);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0202);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0036);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x001f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0037);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0038);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0407);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0039);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0203);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003b);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0202);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x001f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0022);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    //Set reg_cmd_phase = 0
    WRITE_DISP_REG(0x740031a0, U16, 0x0000);


    WRITE_DISP_REG(0x740031a0, U16, 0x0001);

    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);
    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    WRITE_DISP_REG(0x74003128, U16, 0x8000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x7400312c, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);


    WRITE_DISP_REG(0x74003008, U16, 0x0034);
    WRITE_DISP_REG(0x74003120, U16, 0x0024);
    WRITE_DISP_REG(0x74003170, U16, 0x000f);
    WRITE_DISP_REG(0x74003174, U16, 0x0000);
    WRITE_DISP_REG(0x7400309c, U16, 0x00e4);
    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    WRITE_DISP_REG(0x7400313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);





    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0050);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0051);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x00ef);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0052);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0053);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x013f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0022);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003048, U16, 0x00f0);

    WRITE_DISP_REG(0x7400304c, U16, 0x013F);
    WRITE_DISP_REG(0x74003050, U16, 0x01e0);
    WRITE_DISP_REG(0x74003044, U16, 0x0004);
    WRITE_DISP_REG(0x74003040, U16, 0xb000);
    WRITE_DISP_REG(0x74003100, U16, 0x0000);
    WRITE_DISP_REG(0x74003094, U16, 0x0003);
    WRITE_DISP_REG(0x7400309c, U16, 0x00e4);

#if 0	// 13MHz, 35 fps
    WRITE_DISP_REG(0x7400300C, U16, 0x0100);  //H total
    WRITE_DISP_REG(0x74003010, U16, 0x0003);  //H st
    WRITE_DISP_REG(0x74003014, U16, 0x00f3);  //H end
    WRITE_DISP_REG(0x74003018, U16, 0x0002);  //H high
    WRITE_DISP_REG(0x7400301C, U16, 0x0001);  //H low

    WRITE_DISP_REG(0x74003020, U16, 0x02E0);  //V total
    WRITE_DISP_REG(0x74003024, U16, 0x0003);  //V st
    WRITE_DISP_REG(0x74003028, U16, 0x0142);  //V end
    WRITE_DISP_REG(0x7400302C, U16, 0x02DF);  //V high
    WRITE_DISP_REG(0x74003030, U16, 0x02DE);  //V low
#endif

#if 1	// 20MHz, 31 fps
    WRITE_DISP_REG(0x7400300C, U16, 0x0140);  //H total
    WRITE_DISP_REG(0x74003010, U16, 0x0003);  //H st
    WRITE_DISP_REG(0x74003014, U16, 0x00f3);  //H end
    WRITE_DISP_REG(0x74003018, U16, 0x0002);  //H high
    WRITE_DISP_REG(0x7400301C, U16, 0x0001);  //H low

    WRITE_DISP_REG(0x74003020, U16, 0x03F0);  //V total
    WRITE_DISP_REG(0x74003024, U16, 0x0003);  //V st
    WRITE_DISP_REG(0x74003028, U16, 0x0142);  //V end
    WRITE_DISP_REG(0x7400302C, U16, 0x03EF);  //V high
    WRITE_DISP_REG(0x74003030, U16, 0x03EE);  //V low
#endif

    WRITE_DISP_REG(0x7400313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms);

    WRITE_DISP_REG(0x74003150, U16, 0x0010);  //set vsync mode
    WRITE_DISP_REG(0x74003000, U16, 0x0019);
    WRITE_DISP_REG(0x7400313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);

	//WRITE_DISP_REG(0x74003004, U16, 0x001C); // pattern gen
	//WRITE_DISP_REG(0x74003004, U16, 0x0010); // pattern gen
	WRITE_DISP_REG(0x74003004, U16, 0x0013); // pattern gen

#if 0
	for(i=0;i<10000;i++)
	{
		WRITE_DISP_REG(0x74003004, U16, 0 );
		WRITE_DISP_REG(0x7400313c, U16, 0x0001);
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);

		WRITE_DISP_REG(0x74003004, U16, (0x0010 | (i&0xf)) );
		WRITE_DISP_REG(0x7400313c, U16, 0x0001);
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);
	}
#endif

	return 0;
}
#endif

//=============================================================
//=============================================================
#elif defined(NAND_DRV_B3_RTK) && NAND_DRV_B3_RTK
#include "sys_arm.h"
#include "hal_drv_util.h"
#include "sys.ho"
#include "cus_os.hc"
#include "cus_os_sem.h"
#include "cus_os_flag.h"
#include "cus_os_util.h"
#include "drv_clkgen_cmu.h"
#include "Mdl_pm.h"
#include "rtk_backtrace.h"
#include "Sys_chip.h"
#include "hal_miu_arb.h"

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
extern void MDrvInitialFcieIrq(void);
extern Ms_Flag_t g_tFcieMieFlag;
extern U8 g_u8FcieIntrInitialized;
extern bool initializedForTinyHwl;
#endif

U32  nand_hw_timer_delay(U32 u32usTick)
{
  #if 1
    SYS_UDELAY(u32usTick);
  #else
	volatile int i = 0;

	for (i = 0; i < u32usTick; i++)
	{
		volatile int j = 0, tmp;
		for (j = 0; j < 0x2; j++)
		{
			tmp = j;
		}
	}
  #endif

    return u32usTick + 1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	return UNFD_ST_SUCCESS;
}

U32  nand_clock_setting(U32 u32ClkParam)
{
#if NAND_DRIVER_ENV == NAND_ENV_ASIC
#ifdef __I_SW__
    REG_WRITE_UINT16(0x74007D4C, 0x0000); // disable clock fcie

    REG_CLR_BITS_UINT16(0x74007D4C, BIT7+BIT6+BIT5+BIT4); // mask all clock select

    switch(u32ClkParam)
    {
        case NFIE_CLK_187_5K:
            // no need to set
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 187.5K\r\n");
            break;
        case NFIE_CLK_750K:
            REG_SET_BITS_UINT16(0x74007D4C, BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 750K\r\n");
            break;
        case NFIE_CLK_6M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT5);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 6M\r\n");
            break;
        case NFIE_CLK_10_4M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 10.4M\r\n");
            break;
        case NFIE_CLK_13M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 13M\r\n");
            break;
        case NFIE_CLK_19_5M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 19.5M\r\n");
            break;
        case NFIE_CLK_22_29M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6+BIT5);
           //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 22.29M\r\n");
            break;
        case NFIE_CLK_26M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT6+BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 26M\r\n");
            break;
        case NFIE_CLK_39M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 39M\r\n");
            break;
        case NFIE_CLK_44_57M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 44.57M\r\n");
            break;
        case NFIE_CLK_52M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7+BIT5);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 52M\r\n");
            break;
        case NFIE_CLK_78M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT7+BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 78M\r\n");
            break;
        case NFIE_CLK_31_2M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT9+BIT8+BIT7+BIT6+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 31.2M\r\n");
            break;
        case NFIE_CLK_34_67M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT9+BIT8+BIT7+BIT6+BIT5);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 34.6M\r\n");
            break;
        case NFIE_CLK_29_71M:
            REG_SET_BITS_UINT16(0x74007D4C, BIT10+BIT8+BIT7+BIT6+BIT5+BIT4);
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 29.7M\r\n");
            break;
        default:
            REG_SET_BITS_UINT16(0x74007D4C, BIT5+BIT4);
            nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"nand_clock_setting error parameter!!!\r\n");
            break;
    }

    REG_SET_BITS_UINT16(0x74007D4C, BIT3); // fast clock enable
#else
	ClkgenClkSpeed_e eClkSpeed;

	NC_PlatformResetPre();
    switch(u32ClkParam)
    {
        case NFIE_CLK_187_5K:
            eClkSpeed = CLKGEN_187P5KHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 187.5K\r\n");
            break;
        case NFIE_CLK_750K:
            eClkSpeed = CLKGEN_750KHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 750K\r\n");
            break;
        case NFIE_CLK_6M:
            eClkSpeed = CLKGEN_6MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 6M\r\n");
            break;
        case NFIE_CLK_10_4M:
            eClkSpeed = CLKGEN_10P4MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 10.4M\r\n");
            break;
        case NFIE_CLK_13M:
            eClkSpeed = CLKGEN_13MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 13M\r\n");
            break;
        case NFIE_CLK_19_5M:
            eClkSpeed = CLKGEN_19P5MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 19.5M\r\n");
            break;
        case NFIE_CLK_22_29M:
            eClkSpeed = CLKGEN_22P29MHZ;
           //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 22.29M\r\n");
            break;
        case NFIE_CLK_26M:
            eClkSpeed = CLKGEN_26MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 26M\r\n");
            break;
        case NFIE_CLK_39M:
            eClkSpeed = CLKGEN_39MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 39M\r\n");
            break;
        case NFIE_CLK_44_57M:
            eClkSpeed = CLKGEN_44P57MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 44.57M\r\n");
            break;
        case NFIE_CLK_52M:
            eClkSpeed = CLKGEN_52MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 52M\r\n");
            break;
        case NFIE_CLK_78M:
            eClkSpeed = CLKGEN_78MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 78M\r\n");
            break;
        case NFIE_CLK_31_2M:
            eClkSpeed = CLKGEN_31P2MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 31.2M\r\n");
            break;
        case NFIE_CLK_34_67M:
            eClkSpeed = CLKGEN_34P67MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 34.6M\r\n");
            break;
        case NFIE_CLK_29_71M:
            eClkSpeed = CLKGEN_29P71MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 29.7M\r\n");
            break;
        default:
            eClkSpeed = CLKGEN_10P4MHZ;
            nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"nand_clock_setting error parameter!!!\r\n");
            break;
    }

    HalClkgenBbtopSetClkSpeed(BBTOP_CLK_NFIE, eClkSpeed, CLKSRC_PLL22);
#endif
#endif
    NC_PlatformResetPost();
    return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	u32_Clk = 2000000000/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_187_5K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_187_5K && u32_Clk < NFIE_CLK_750K )
		pNandDrv->u32_Clk = NFIE_CLK_187_5K;
	else if( u32_Clk >= NFIE_CLK_750K && u32_Clk < NFIE_CLK_6M )
		pNandDrv->u32_Clk = NFIE_CLK_750K;
	else if( u32_Clk >= NFIE_CLK_6M && u32_Clk < NFIE_CLK_10_4M )
		pNandDrv->u32_Clk = NFIE_CLK_6M;
	else if( u32_Clk >= NFIE_CLK_10_4M && u32_Clk < NFIE_CLK_13M )
		pNandDrv->u32_Clk = NFIE_CLK_10_4M;
	else if( u32_Clk >= NFIE_CLK_13M && u32_Clk < NFIE_CLK_19_5M )
		pNandDrv->u32_Clk = NFIE_CLK_13M;
	else if( u32_Clk >= NFIE_CLK_19_5M && u32_Clk < NFIE_CLK_22_29M )
		pNandDrv->u32_Clk = NFIE_CLK_19_5M;
	else if( u32_Clk >= NFIE_CLK_22_29M && u32_Clk < NFIE_CLK_26M )
		pNandDrv->u32_Clk = NFIE_CLK_22_29M;
	else if( u32_Clk >= NFIE_CLK_26M && u32_Clk < NFIE_CLK_29_71M )
		pNandDrv->u32_Clk = NFIE_CLK_26M;
	else if( u32_Clk >= NFIE_CLK_29_71M && u32_Clk < NFIE_CLK_31_2M )
		pNandDrv->u32_Clk = NFIE_CLK_29_71M;
	else if( u32_Clk >= NFIE_CLK_31_2M && u32_Clk < NFIE_CLK_34_67M )
		pNandDrv->u32_Clk = NFIE_CLK_31_2M;
	else if( u32_Clk >= NFIE_CLK_34_67M && u32_Clk < NFIE_CLK_39M )
		pNandDrv->u32_Clk = NFIE_CLK_34_67M;
	else if( u32_Clk >= NFIE_CLK_39M && u32_Clk < NFIE_CLK_44_57M )
		pNandDrv->u32_Clk = NFIE_CLK_39M;
	else if( u32_Clk >= NFIE_CLK_44_57M && u32_Clk < NFIE_CLK_52M )
		pNandDrv->u32_Clk = NFIE_CLK_44_57M;
	else if( u32_Clk >= NFIE_CLK_52M && u32_Clk < NFIE_CLK_78M )
		pNandDrv->u32_Clk = NFIE_CLK_52M;
	else
		pNandDrv->u32_Clk = NFIE_CLK_78M;

	if(sys_GetChipRevisionId() < SYS_CHIP_ID_U03)
	{
		switch(pNandDrv->u32_Clk)
		{
			case NFIE_CLK_29_71M:
			case NFIE_CLK_31_2M:
			case NFIE_CLK_34_67M:
				pNandDrv->u32_Clk = NFIE_CLK_26M;
				break;
			default:
				break;
		}
	}

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
}

void nand_reset_WatchDog(void)
{
	sys_DelayWatchDogReset();
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	sys_Clean_and_flush_data_cache_buffer(u32_DMAAddr, u32_ByteCnt);
	return HalUtilPHY2MIUAddr(u32_DMAAddr);
}

void nand_Invalidate_data_cache_buffer(U32 u32_addr, S32 s32_size)
{
	sys_Invalidate_data_cache_buffer(u32_addr, s32_size);
}

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
U32 u32_NandWaitStartTick;
U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	Ms_flag_value_t tFlagValue;
	U32 u32_WaitMs = u32_MicroSec/1000;

	u32_NandWaitStartTick = MsGetOsTick();
	tFlagValue = MsFlagTimedWait (&g_tFcieMieFlag, (Ms_flag_value_t) u16_WaitEvent,
								MS_FLAG_WAITMODE_AND|MS_FLAG_WAITMODE_CLR,
								RTK_MS_TO_TICK(u32_WaitMs));
	if (tFlagValue==0) // return 0 means time out
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1 ,"MsFlagTimedWait %d us time out!\r\n", u32_MicroSec);
		return UNFD_ST_ERR_E_TIMEOUT;
	}

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_WaitEvent); /*clear events*/
	return UNFD_ST_SUCCESS;
}

void nand_enable_intr_mode(void)
{
	if ((backtrace_entry_counter_check() == 0) && (g_u8FcieIntrInitialized == 1) && (MsCurrTask() != 0) && (!initializedForTinyHwl))
	{
		// not in exception handler and intr is initialized
		REG_SET_BITS_UINT16(NC_MIE_INT_EN, BIT_MMA_DATA_END|BIT_NC_JOB_END);
	}
	else
	{
		REG_CLR_BITS_UINT16(NC_MIE_INT_EN, BIT_MMA_DATA_END|BIT_NC_JOB_END);
	}
}
#endif

U16 gu16_unfd_mem_guard_w0 = 0xFE00;
static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
U16 gu16_unfd_mem_guard_w1 = 0xFE01;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;
U16 gu16_unfd_mem_guard_w2 = 0xFE02;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
U16 gu16_unfd_mem_guard_w3 = 0xFE03;
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */
U16 gu16_unfd_mem_guard_w4 = 0xFE04;

U16 gu16_unfd_mem_guard_locker0 = 0; // 0xFE05
U16 gu16_unfd_mem_guard_locker1 = 0; // 0xFE06

U32 NC_PlatformResetPre(void)
{
	HalMiuMaskReq(MIU_CLT_FCIE);
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	HalMiuUnMaskReq(MIU_CLT_FCIE);
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 u16_reg;
	U32 u32_ChipId;

	//pNandDrv->u8_WordMode = sys_get_extMemType() == EXTMEM_NAND_16BITS;
	REG_READ_UINT16(GET_REG_ADDR(0x74007C00, 0x20), u16_reg);
	u16_reg &= 0x0F;
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1 ,"sys_get_extMemType=0x%X\r\n", u16_reg);
	if( u16_reg == 0x00 || u16_reg == 0x01 || u16_reg == 0x02 )
		pNandDrv->u8_WordMode= 1;
	else
		pNandDrv->u8_WordMode= 0;

	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;
	u32_ChipId = sys_GetChipRevisionId();
	if(u32_ChipId < SYS_CHIP_ID_U03)
	{
		switch(pNandDrv->u32_Clk)
		{
			case NFIE_CLK_29_71M:
			case NFIE_CLK_31_2M:
			case NFIE_CLK_34_67M:
				pNandDrv->u32_Clk = NFIE_CLK_26M;
				break;
			default:
				break;
		}
	}
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);

	nand_clock_setting(pNandDrv->u32_Clk);

	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	if(u32_ChipId<SYS_CHIP_ID_U03)
	{
		pNandDrv->u8_SwPatchWaitRb= 1;
		pNandDrv->u8_SwPatchJobStart= 1;
		NC_ConfigHal(NAND_HAL_PATCH);
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"B3 U%02X needs SW patch\r\n", u32_ChipId);
	}
	else
	{
		pNandDrv->u8_SwPatchWaitRb= 0;
		pNandDrv->u8_SwPatchJobStart= 0;
		NC_ConfigHal(NAND_HAL_RAW);
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"B3 U%02X has HW patch\r\n", u32_ChipId);
	}

	if(backtrace_entry_counter_check() == 0)
		MdlPmDevPowerCtl(MDL_PM_DEV_NAND, MDL_PM_PWR_REQ_ON, NULL);

	return UNFD_ST_SUCCESS;
}

U32 drvNAND_init_sem(void)
{
	// RTK will init semaphore according to cus_InitSem during booting.
#if 0
	if( MsInitSem (FCIE_LOCK_SEM, 1) == CUS_OS_OK )
		return UNFD_ST_SUCCESS;
	else
		return UNFD_ST_ERR_SEMAPHORE;
#else
	return UNFD_ST_SUCCESS;
#endif
}

U32 u32_NandLockStartTick;
U32 u32_NandLockEndTick;
U32 drvNAND_lock_sem(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	// exception handler cannot wait sem.
	if(backtrace_entry_counter_check()== 0)
	{
		if( MsConsumeSemRtkReady(FCIE_LOCK_SEM) != CUS_OS_OK )
		{
			nand_die();
			return UNFD_ST_ERR_SEMAPHORE;
		}
		u32_NandLockStartTick = MsGetOsTick();		
	}

	NC_ResetFCIE();
	DrvClkgenDrvClockCtl(CLKGEN_DRV_NAND, CLKGEN_CLK_REQ_ON);
	nand_clock_setting(pNandDrv->u32_Clk);
#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	nand_enable_intr_mode();
#endif

	return UNFD_ST_SUCCESS;
}

U32 drvNAND_unlock_sem(void)
{
	DrvClkgenDrvClockCtl(CLKGEN_DRV_NAND, CLKGEN_CLK_REQ_OFF);

	// exception handler cannot release sem.
	if(backtrace_entry_counter_check()!= 0)
	{
		return UNFD_ST_SUCCESS;
	}

	// disable BIT_NC_EN for power consumption
	REG_WRITE_UINT16(NC_PATH_CTL, 0);

	u32_NandLockEndTick = MsGetOsTick();
	if( MsProduceSemRtkReady(FCIE_LOCK_SEM) == CUS_OS_OK )
		return UNFD_ST_SUCCESS;
	else
	{
		nand_die();
		return UNFD_ST_ERR_SEMAPHORE;
	}
}

U8 drvNAND_IsActive(void)
{
	short SemCount = 1;

	MsGetSemCount(FCIE_LOCK_SEM, &SemCount);

	if(SemCount > 0)
		return 0;
	else
		return 1;
}

void drvNAND_SetAssertPBA(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 u16_i;

	for(u16_i=0; u16_i<pNandDrv->pPartInfo->u16_PartCnt; u16_i++)
	{
		if(DEBUG_ASSERT_PARTITION_TYPE == pNandDrv->pPartInfo->records[u16_i].u16_PartType)
		{
			pNandDrv->u16_FirstPBA_AssertDebug = pNandDrv->pPartInfo->records[u16_i].u16_StartBlk;
			return;
		}		
	}
	pNandDrv->u16_FirstPBA_AssertDebug = 10; // protect CIS from FTL bugs
}

#if IF_IP_VERIFY
#define WRITE_DISP_REG(addr, type, data)  ((*(volatile type *)(addr)) = (data))
U32 NCTEST_InitDisp(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 i;

	nand_debug(0, 1, "init disp - x \n");

	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_SHARE_PAD_EN;
	NC_Config();

    //select display clock to 13MHz
    //WRITE_DISP_REG( 0x74007DD0, U16 , 0x308); // 13 MHz
    WRITE_DISP_REG( 0x74007DD0, U16 , 0x528);  // 20MHz

	*(volatile U16*)0x7400724C &= (U16)~BIT13; // not gate Disp clk
	*(volatile U16*)0x74007240 &= (U16)~BIT4; // not gate FCIE3 clk

    //Set reg_cmd_phase = 1
    WRITE_DISP_REG(0x740031a0, U16, 0x0001);

    ////PAD swtich to display.....
    WRITE_DISP_REG(0x74006614, U16, 0x0040);
    ////PAD swtich to display.....

    WRITE_DISP_REG(0x740031f4, U16, 0x0003);
    WRITE_DISP_REG(0x74003008, U16, 0x0034);
    WRITE_DISP_REG(0x74003120, U16, 0x0024);
    WRITE_DISP_REG(0x74003170, U16, 0x000f);
    WRITE_DISP_REG(0x74003174, U16, 0x0000);
    WRITE_DISP_REG(0x7400309c, U16, 0x00e4);
    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    WRITE_DISP_REG(0x7400313c, U16, 0x0001);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0017);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*25);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0010);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x17b0);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0011);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0012);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0119);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0013);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x1400);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0015);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0029);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000e);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x00fd);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0012);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0139);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*25);
    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0100);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0002);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0700);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0003);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x1030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0008);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0808);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0009);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x000d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0031);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0032);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0033);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0034);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0035);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0036);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0037);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0038);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0039);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003b);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0f0f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0050);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0051);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x00ef);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0052);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0053);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x013f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0060);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x2700);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0061);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x006a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0090);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0016);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0092);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0093);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);
    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0061);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);
    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0173);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0031);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0407);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0032);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0203);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0033);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0034);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0035);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0202);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0036);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x001f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0037);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0038);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0407);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0039);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0203);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003b);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0202);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x003d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x001f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003124, U16, 0x0022);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    //Set reg_cmd_phase = 0
    WRITE_DISP_REG(0x740031a0, U16, 0x0000);


    WRITE_DISP_REG(0x740031a0, U16, 0x0001);

    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);
    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    WRITE_DISP_REG(0x74003128, U16, 0x8000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x7400312c, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);


    WRITE_DISP_REG(0x74003008, U16, 0x0034);
    WRITE_DISP_REG(0x74003120, U16, 0x0024);
    WRITE_DISP_REG(0x74003170, U16, 0x000f);
    WRITE_DISP_REG(0x74003174, U16, 0x0000);
    WRITE_DISP_REG(0x7400309c, U16, 0x00e4);
    WRITE_DISP_REG(0x74003000, U16, 0x0001);
    WRITE_DISP_REG(0x7400313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);





    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0050);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0051);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x00ef);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0052);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0053);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x013f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x74003128, U16, 0x0000);
    WRITE_DISP_REG(0x74003124, U16, 0x0022);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x74003048, U16, 0x00f0);

    WRITE_DISP_REG(0x7400304c, U16, 0x013F);
    WRITE_DISP_REG(0x74003050, U16, 0x01e0);
    WRITE_DISP_REG(0x74003044, U16, 0x0004);
    WRITE_DISP_REG(0x74003040, U16, 0xb000);
    WRITE_DISP_REG(0x74003100, U16, 0x0000);
    WRITE_DISP_REG(0x74003094, U16, 0x0003);
    WRITE_DISP_REG(0x7400309c, U16, 0x00e4);

#if 0	// 13MHz, 35 fps
    WRITE_DISP_REG(0x7400300C, U16, 0x0100);  //H total
    WRITE_DISP_REG(0x74003010, U16, 0x0003);  //H st
    WRITE_DISP_REG(0x74003014, U16, 0x00f3);  //H end
    WRITE_DISP_REG(0x74003018, U16, 0x0002);  //H high
    WRITE_DISP_REG(0x7400301C, U16, 0x0001);  //H low

    WRITE_DISP_REG(0x74003020, U16, 0x02E0);  //V total
    WRITE_DISP_REG(0x74003024, U16, 0x0003);  //V st
    WRITE_DISP_REG(0x74003028, U16, 0x0142);  //V end
    WRITE_DISP_REG(0x7400302C, U16, 0x02DF);  //V high
    WRITE_DISP_REG(0x74003030, U16, 0x02DE);  //V low
#endif

#if 1	// 20MHz, 31 fps
    WRITE_DISP_REG(0x7400300C, U16, 0x0140);  //H total
    WRITE_DISP_REG(0x74003010, U16, 0x0003);  //H st
    WRITE_DISP_REG(0x74003014, U16, 0x00f3);  //H end
    WRITE_DISP_REG(0x74003018, U16, 0x0002);  //H high
    WRITE_DISP_REG(0x7400301C, U16, 0x0001);  //H low

    WRITE_DISP_REG(0x74003020, U16, 0x03F0);  //V total
    WRITE_DISP_REG(0x74003024, U16, 0x0003);  //V st
    WRITE_DISP_REG(0x74003028, U16, 0x0142);  //V end
    WRITE_DISP_REG(0x7400302C, U16, 0x03EF);  //V high
    WRITE_DISP_REG(0x74003030, U16, 0x03EE);  //V low
#endif

    WRITE_DISP_REG(0x7400313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms);

    WRITE_DISP_REG(0x74003150, U16, 0x0010);  //set vsync mode
    WRITE_DISP_REG(0x74003000, U16, 0x0019);
    WRITE_DISP_REG(0x7400313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);

	//WRITE_DISP_REG(0x74003004, U16, 0x001C); // pattern gen
	//WRITE_DISP_REG(0x74003004, U16, 0x0010); // pattern gen
	WRITE_DISP_REG(0x74003004, U16, 0x0013); // pattern gen

#if 0
	for(i=0;i<10000;i++)
	{
		WRITE_DISP_REG(0x74003004, U16, 0 );
		WRITE_DISP_REG(0x7400313c, U16, 0x0001);
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);

		WRITE_DISP_REG(0x74003004, U16, (0x0010 | (i&0xf)) );
		WRITE_DISP_REG(0x7400313c, U16, 0x0001);
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);
	}
#endif

	return 0;
}
#endif

//=============================================================
//=============================================================
#elif defined(NAND_DRV_G1_ROM) && NAND_DRV_G1_ROM
#include "bootrom.h"

/* return Timer tick */
U32  nand_hw_timer_delay(U32 u32usTick)
{
	#if 1
	hal_delay_us(u32usTick);
	#else
	volatile int i = 0;

	for (i = 0; i < u32usTick/3+1; i++)
	{
		volatile int j = 0, tmp;
		for (j = 0; j < 0x0; j++)
		{
			tmp = j;
		}
	}
	#endif

    return u32usTick + 1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	// G1 ROM code does not need the pads_switch
	return UNFD_ST_SUCCESS;
}

U32  nand_clock_setting(U32 u32ClkParam)
{
	NC_PlatformResetPre();
	//REG_SET_BITS_UINT16(reg_ckg_disp, BIT13);// gate Disp clk

    REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT6); // un-select fast
    REG_CLR_BITS_UINT16(reg_ckg_fcie, NFIE_CLK_MASK); // set clk

    switch(u32ClkParam)
    {
        case NFIE_CLK_300K:

            break;
        case NFIE_CLK_800K:
			REG_SET_BITS_UINT16(reg_ckg_fcie, BIT0);
            break;
        case NFIE_CLK_5_3M:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT1);
            break;
        case NFIE_CLK_12_8M:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT1+BIT0);
            break;
        case NFIE_CLK_19_2M:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT2);
            break;
        case NFIE_CLK_24M:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT2+BIT0);
            break;
        case NFIE_CLK_27_4M:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT2+BIT1);
            break;
        case NFIE_CLK_32M:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT2+BIT1+BIT0);
            break;
        case NFIE_CLK_42_6M:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT3);
            break;
        default:
            REG_SET_BITS_UINT16(reg_ckg_fcie, BIT1+BIT0);
            break;
    }

	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT4); // not gate FCIE3 clk
	REG_SET_BITS_UINT16(reg_ckg_fcie, BIT6); // select fast
    REG_CLR_BITS_UINT16(reg_gate_miu_fcie, BIT3); // not gate clk_miu_fcie

	//nand_debug(0, 1, "NFIE CLK: %X\n", u32ClkParam);
    NC_PlatformResetPost();
	return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	u32_Clk = 2000000000/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_300K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_300K && u32_Clk < NFIE_CLK_800K )
		pNandDrv->u32_Clk = NFIE_CLK_300K;
	else if( u32_Clk >= NFIE_CLK_800K && u32_Clk < NFIE_CLK_5_3M )
		pNandDrv->u32_Clk = NFIE_CLK_800K;
	else if( u32_Clk >= NFIE_CLK_5_3M && u32_Clk < NFIE_CLK_12_8M )
		pNandDrv->u32_Clk = NFIE_CLK_5_3M;
	else if( u32_Clk >= NFIE_CLK_12_8M && u32_Clk < NFIE_CLK_19_2M )
		pNandDrv->u32_Clk = NFIE_CLK_12_8M;
	else if( u32_Clk >= NFIE_CLK_19_2M && u32_Clk < NFIE_CLK_24M )
		pNandDrv->u32_Clk = NFIE_CLK_19_2M;
	else if( u32_Clk >= NFIE_CLK_24M && u32_Clk < NFIE_CLK_27_4M )
		pNandDrv->u32_Clk = NFIE_CLK_24M;
	else if( u32_Clk >= NFIE_CLK_27_4M && u32_Clk < NFIE_CLK_32M )
		pNandDrv->u32_Clk = NFIE_CLK_27_4M;
	else if( u32_Clk >= NFIE_CLK_32M && u32_Clk < NFIE_CLK_42_6M )
		pNandDrv->u32_Clk = NFIE_CLK_32M;
	else
		pNandDrv->u32_Clk = NFIE_CLK_42_6M;

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// G1 ROM code disable WatchDog
}

void nand_reset_WatchDog(void)
{
	// G1 ROM code disable WatchDog
}

/*
 * return DMA address for FCIE3 registers
 * mark to save code size
 */
U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	// G1 ROM code use R2N mode
    return u32_DMAAddr;
}

void nand_Invalidate_data_cache_buffer(U32 u32_addr, S32 s32_size)
{

}

//======================================
// driver memory allocations
static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */

U32 NC_PlatformResetPre(void)
{
	//HalMiuMaskReq(MIU_CLT_FCIE);
	*((volatile unsigned short *)(0x2a007c0c))|=0x02;
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	//HalMiuUnMaskReq(MIU_CLT_FCIE);
    *((volatile unsigned short *)(0x2a007c0c))&=~0x02;
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	pNandDrv->u8_WordMode = sys_get_extMemType() == EXTMEM_NAND_16BITS;

	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);

	nand_clock_setting(pNandDrv->u32_Clk);

	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	pNandDrv->u8_SwPatchWaitRb= 0;
	pNandDrv->u8_SwPatchJobStart= 0;
	NC_ConfigHal(NAND_HAL_RAW);

	return UNFD_ST_SUCCESS;
}

U32 drvNAND_init_sem(void)
{
	// ROM code environment doesn't need semaphore
	return UNFD_ST_SUCCESS;
}

U32 drvNAND_lock_sem(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	NC_ResetFCIE();
	nand_clock_setting(pNandDrv->u32_Clk);
	// ROM code environment doesn't need semaphore
	return UNFD_ST_SUCCESS;
}

U32 drvNAND_unlock_sem(void)
{
	// ROM code environment doesn't need semaphore
	return UNFD_ST_SUCCESS;
}

void drvNAND_SetAssertPBA(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 u16_i;

	for(u16_i=0; u16_i<pNandDrv->pPartInfo->u16_PartCnt; u16_i++)
	{
		if(DEBUG_ASSERT_PARTITION_TYPE == pNandDrv->pPartInfo->records[u16_i].u16_PartType)
		{
			pNandDrv->u16_FirstPBA_AssertDebug = pNandDrv->pPartInfo->records[u16_i].u16_StartBlk;
			return;
		}		
	}
	pNandDrv->u16_FirstPBA_AssertDebug = 10; // protect CIS from FTL bugs
}

//=============================================================
//=============================================================
#elif defined(NAND_DRV_G1_RTK) && NAND_DRV_G1_RTK
#include "sys_arm.h"
#include "hal_drv_util.h"
#include "sys.ho"
#include "cus_os.hc"
#include "cus_os_sem.h"
#include "cus_os_flag.h"
#include "cus_os_util.h"
#include "rtk_backtrace.h"
#include "Sys_chip.h"
#include "hal_miu_arb.h"

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
extern void MDrvInitialFcieIrq(void);
extern Ms_Flag_t g_tFcieMieFlag;
extern U8 g_u8FcieIntrInitialized;
#endif

/* return Timer tick */
U32  nand_hw_timer_delay(U32 u32usTick)
{
  #if 1
    SYS_UDELAY(u32usTick);
  #else
	volatile int i = 0;

	for (i = 0; i < u32usTick; i++)
	{
		volatile int j = 0, tmp;
		for (j = 0; j < 0x2; j++)
		{
			tmp = j;
		}
	}
  #endif

    return u32usTick + 1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	// G1 ROM code does not need the pads_switch
	return UNFD_ST_SUCCESS;
}

#define EN_G1_CPCM_FCIE_CTRL  
U32  nand_clock_setting(U32 u32ClkParam)
{	
#if 1
	#if !defined(EN_G1_CPCM_FCIE_CTRL) 
	//g_ptKeCmu->reg_ckg_disp = 1;  // gate disp clock
    NC_PlatformResetPre();
	g_ptKeCmu->reg_ckg_fcie &= ~4; // unselect fast

	switch(u32ClkParam)
	{
		case NFIE_CLK_300K:
			g_ptKeCmu->reg_ckm_fcie = 0;
			break;
		case NFIE_CLK_800K:
			g_ptKeCmu->reg_ckm_fcie = BIT0;
			break;
		case NFIE_CLK_5_3M:
			g_ptKeCmu->reg_ckm_fcie = BIT1;
			break;
		case NFIE_CLK_12_8M:
			g_ptKeCmu->reg_ckm_fcie = BIT1+BIT0;
			break;
		case NFIE_CLK_19_2M:
			g_ptKeCmu->reg_ckm_fcie = BIT2;
			break;
		case NFIE_CLK_24M:
			g_ptKeCmu->reg_ckm_fcie = BIT2+BIT0;
			break;
		case NFIE_CLK_27_4M:
			g_ptKeCmu->reg_ckm_fcie = BIT2+BIT1;
			break;
		case NFIE_CLK_32M:
			g_ptKeCmu->reg_ckm_fcie = BIT2+BIT1+BIT0;
			break;
		case NFIE_CLK_42_6M:
			g_ptKeCmu->reg_ckm_fcie = BIT3;
			break;
		default:
			g_ptKeCmu->reg_ckm_fcie = BIT1+BIT0;
		break;
	}

	g_ptKeCmu->reg_ckg_fcie = 0; // ungate
	g_ptKeCmu->reg_ckg_fcie = 4; // select fast

	g_ptKeCmuCpu->reg_gate_miu_fcie = 0; // ungate clk_miu_fcie
	#else
	/* Use CPCM driver */
	ClkgenClkSpeed_e eClkSpeed;
    NC_PlatformResetPre();
	switch(u32ClkParam)
	{ 
		case NFIE_CLK_300K:
			eClkSpeed = CLKGEN_300KHZ;
			break;
		case NFIE_CLK_800K:
			eClkSpeed = CLKGEN_800KHZ;
			break;
		case NFIE_CLK_5_3M:
			eClkSpeed = CLKGEN_5P3MHZ;
			break;
		case NFIE_CLK_12_8M:
			eClkSpeed = CLKGEN_12P8MHZ;
			break;
		case NFIE_CLK_19_2M:
			eClkSpeed = CLKGEN_19P2MHZ;
			break;
		case NFIE_CLK_24M:
			eClkSpeed = CLKGEN_24MHZ;
			break;
		case NFIE_CLK_27_4M:
			eClkSpeed = CLKGEN_27P4MHZ;
			break;
		case NFIE_CLK_32M:
			eClkSpeed = CLKGEN_32MHZ;
			break;
		case NFIE_CLK_42_6M:
			eClkSpeed = CLKGEN_42P6MHZ;
			break;
		default:
			eClkSpeed = CLKGEN_12P8MHZ;
			break;
	}
	
	if( CLKGEN_SUCCESS != DrvClkgenDrvClockUpdate(CLKGEN_DRV_NAND, CLKGEN_CLK_FCIE, eClkSpeed))
	{
		NC_PlatformResetPost();
		return UNFD_ST_ERR_INVALID_PARAM;
	}

	#endif /* EN_G1_CPCM_FCIE_CTRL */ 

#else
	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT6); // un-select fast
	REG_CLR_BITS_UINT16(reg_ckg_fcie, NFIE_CLK_MASK); // set clk
	REG_SET_BITS_UINT16(reg_ckg_fcie, u32ClkParam);
	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT4); // not gate FCIE3 clk
	REG_SET_BITS_UINT16(reg_ckg_fcie, BIT6); // select fast
	REG_CLR_BITS_UINT16(reg_gate_miu_fcie, BIT3); // not gate clk_miu_fcie
		
	//nand_debug(0, 1, "NFIE CLK: %X\n", u32ClkParam);
#endif /* 1 */

    NC_PlatformResetPost();
	return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	u32_Clk = 2000000000/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_300K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_300K && u32_Clk < NFIE_CLK_800K )
		pNandDrv->u32_Clk = NFIE_CLK_300K;
	else if( u32_Clk >= NFIE_CLK_800K && u32_Clk < NFIE_CLK_5_3M )
		pNandDrv->u32_Clk = NFIE_CLK_800K;
	else if( u32_Clk >= NFIE_CLK_5_3M && u32_Clk < NFIE_CLK_12_8M )
		pNandDrv->u32_Clk = NFIE_CLK_5_3M;
	else if( u32_Clk >= NFIE_CLK_12_8M && u32_Clk < NFIE_CLK_19_2M )
		pNandDrv->u32_Clk = NFIE_CLK_12_8M;
	else if( u32_Clk >= NFIE_CLK_19_2M && u32_Clk < NFIE_CLK_24M )
		pNandDrv->u32_Clk = NFIE_CLK_19_2M;
	else if( u32_Clk >= NFIE_CLK_24M && u32_Clk < NFIE_CLK_27_4M )
		pNandDrv->u32_Clk = NFIE_CLK_24M;
	else if( u32_Clk >= NFIE_CLK_27_4M && u32_Clk < NFIE_CLK_32M )
		pNandDrv->u32_Clk = NFIE_CLK_27_4M;
	else if( u32_Clk >= NFIE_CLK_32M && u32_Clk < NFIE_CLK_42_6M )
		pNandDrv->u32_Clk = NFIE_CLK_32M;
	else
		pNandDrv->u32_Clk = NFIE_CLK_42_6M;

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// G1 ROM code disable WatchDog
}

void nand_reset_WatchDog(void)
{
	// G1 ROM code disable WatchDog
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	sys_Clean_and_flush_data_cache_buffer(u32_DMAAddr, u32_ByteCnt);
	return HalUtilPHY2MIUAddr(u32_DMAAddr);
}

void nand_Invalidate_data_cache_buffer(U32 u32_addr, S32 s32_size)
{
	sys_Invalidate_data_cache_buffer(u32_addr, s32_size);
}

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
U32 u32_NandWaitStartTick;
U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	Ms_flag_value_t tFlagValue;
	U32 u32_WaitMs = u32_MicroSec/1000;

	u32_NandWaitStartTick = MsGetOsTick();
	tFlagValue = MsFlagTimedWait (&g_tFcieMieFlag, (Ms_flag_value_t) u16_WaitEvent,
								MS_FLAG_WAITMODE_AND|MS_FLAG_WAITMODE_CLR,
								RTK_MS_TO_TICK(u32_WaitMs));
	if (tFlagValue==0) // return 0 means time out
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1 ,"MsFlagTimedWait %d us time out!\r\n", u32_MicroSec);
		return UNFD_ST_ERR_E_TIMEOUT;
	}

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_WaitEvent); /*clear events*/
	return UNFD_ST_SUCCESS;
}

void nand_enable_intr_mode(void)
{
	if ((backtrace_entry_counter_check() == 0) && (g_u8FcieIntrInitialized == 1) && (MsCurrTask() != 0))
	{
		// not in exception handler and intr is initialized
		REG_SET_BITS_UINT16(NC_MIE_INT_EN, BIT_MMA_DATA_END|BIT_NC_JOB_END);
	}
	else
	{
		REG_CLR_BITS_UINT16(NC_MIE_INT_EN, BIT_MMA_DATA_END|BIT_NC_JOB_END);
	}
}
#endif

U16 gu16_unfd_mem_guard_w0 = 0xFE00;
static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
U16 gu16_unfd_mem_guard_w1 = 0xFE01;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;
U16 gu16_unfd_mem_guard_w2 = 0xFE02;
void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
U16 gu16_unfd_mem_guard_w3 = 0xFE03;
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */
U16 gu16_unfd_mem_guard_w4 = 0xFE04;

U16 gu16_unfd_mem_guard_locker0 = 0; // 0xFE05
U16 gu16_unfd_mem_guard_locker1 = 0; // 0xFE06
static bool bFCIEClkInit=FALSE;

U32 NC_PlatformResetPre(void)
{
	HalMiuMaskReq(MIU_CLT_FCIE);
	//*((volatile unsigned short *)(0x2a007c0c))|=0x02;
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	HalMiuUnMaskReq(MIU_CLT_FCIE);
    //*((volatile unsigned short *)(0x2a007c0c))&=~0x02;
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 u16_reg;
	U32 u32_ChipId;

	// check bootstrap pin, to define x8 or x16
	#if IF_IP_VERIFY
	pNandDrv->u8_WordMode = NAND_IF_WORD_MODE; // [CAUTION]
	#else
	REG_READ_UINT16(GET_REG_ADDR(0x2A007000, 0x74), u16_reg);
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1 ,"sys_get_extMemType=0x%X\r\n", u16_reg);
	if ((u16_reg & (1<<8))!=0) // bit 8 : NAND bits, 0=16bit, 1=8bit
		pNandDrv->u8_WordMode= 0;
	else
		pNandDrv->u8_WordMode= 1;
	#endif

	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);

#ifdef EN_G1_CPCM_FCIE_CTRL
			if(!bFCIEClkInit)
			{
				DrvClkgenDrvClockCtl(CLKGEN_DRV_NAND, CLKGEN_CLK_REQ_ON);  ///< for G1
				bFCIEClkInit = TRUE;
			}		
#endif 

	// set clock
	nand_clock_setting(pNandDrv->u32_Clk);

	// set UNFD internal memory
		pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
		pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	u32_ChipId = sys_GetChipRevisionId();
	if(u32_ChipId<CHIP_ID(G1, U02))
	{
		pNandDrv->u8_SwPatchWaitRb= 1;
		pNandDrv->u8_SwPatchJobStart= 1;
		NC_ConfigHal(NAND_HAL_PATCH);
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"G1 U%02X needs SW patch\r\n", u32_ChipId);
	}
	else
	{
		pNandDrv->u8_SwPatchWaitRb= 0;
		pNandDrv->u8_SwPatchJobStart= 0;
		NC_ConfigHal(NAND_HAL_RAW);
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"G1 U%02X has HW patch\r\n", u32_ChipId);
	}

	return UNFD_ST_SUCCESS;
}

U32 drvNAND_init_sem(void)
{
	// RTK will init semaphore according to cus_InitSem during booting.
    #if 0
	if( MsInitSem (FCIE_LOCK_SEM, 1) == CUS_OS_OK )
		return UNFD_ST_SUCCESS;
	else
		return UNFD_ST_ERR_SEMAPHORE;
    #else
	return UNFD_ST_SUCCESS;
    #endif
}

U32 u32_NandLockStartTick;
U32 u32_NandLockEndTick;
U32 drvNAND_lock_sem(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	// exception handler cannot wait sem.
	if(backtrace_entry_counter_check()== 0)
	{
		if( MsConsumeSemRtkReady(FCIE_LOCK_SEM) != CUS_OS_OK )
		{
			nand_die();
			return UNFD_ST_ERR_SEMAPHORE;
		}
		u32_NandLockStartTick = MsGetOsTick();		
	}	

	NC_ResetFCIE();
#if 0
#ifdef EN_G1_CPCM_FCIE_CTRL
				if(!bFCIEClkInit)
				{
					DrvClkgenDrvClockCtl(CLKGEN_DRV_NAND, CLKGEN_CLK_REQ_ON);  ///< for G1
					bFCIEClkInit = TRUE;
				}		
#endif 
#endif 
	nand_clock_setting(pNandDrv->u32_Clk);
#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	nand_enable_intr_mode();
#endif

	return UNFD_ST_SUCCESS;
}

U32 drvNAND_unlock_sem(void)
{
	// exception handler cannot release sem.
	if(backtrace_entry_counter_check()!= 0)
	{
		return UNFD_ST_SUCCESS;
	}

	// disable BIT_NC_EN for power consumption
	REG_WRITE_UINT16(NC_PATH_CTL, 0);
#ifdef EN_G1_CPCM_FCIE_CTRL	
	//DrvClkgenDrvClockCtl(CLKGEN_DRV_NAND, CLKGEN_CLK_REQ_OFF);   ///< for G1
#endif 

	u32_NandLockEndTick = MsGetOsTick();
	if( MsProduceSemRtkReady(FCIE_LOCK_SEM) == CUS_OS_OK )
		return UNFD_ST_SUCCESS;
	else
	{
		nand_die();
		return UNFD_ST_ERR_SEMAPHORE;
	}
}

U8 drvNAND_IsActive(void)
{
	short SemCount = 1;

	MsGetSemCount(FCIE_LOCK_SEM, &SemCount);

	if(SemCount > 0)
		return 0;
	else
		return 1;
}

void drvNAND_SetAssertPBA(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 u16_i;

	for(u16_i=0; u16_i<pNandDrv->pPartInfo->u16_PartCnt; u16_i++)
	{
		if(DEBUG_ASSERT_PARTITION_TYPE == pNandDrv->pPartInfo->records[u16_i].u16_PartType)
		{
			pNandDrv->u16_FirstPBA_AssertDebug = pNandDrv->pPartInfo->records[u16_i].u16_StartBlk;
			return;
		}		
	}
	pNandDrv->u16_FirstPBA_AssertDebug = 10; // protect CIS from FTL bugs
}


#if IF_IP_VERIFY
#define WRITE_DISP_REG(addr, type, data)  ((*(volatile type *)(addr)) = (data))
U32 NCTEST_InitDisp(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	nand_debug(0, 1, "init disp - x \n");
	U16 i;
	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_SHARE_PAD_EN;
	NC_Config();

	#if 1
	//*(volatile U16*)0x2A00724C &= (U16)~BIT13; // not gate Disp clk
	*(volatile U16*)0x2A00724C = 0x8730; // not gate Disp clk (19.5MHz)
	//*(volatile U16*)0x2A001C5C = 0x0EBC; // not gate Disp clk
	*(volatile U16*)0x2A007240 &= (U16)~BIT4; // not gate FCIE3 clk
	#else
	*(volatile U16*)0x2A00724C &= (U16)~BIT13; // not gate Disp clk
	*(volatile U16*)0x2A007240 &= (U16)~BIT4; // not gate FCIE3 clk
	#endif
    //# set display clock
    //w32 0x2a00724C 0x8930
    //# set FCIE clock // 3o-O!������AA3!MO!P||U?v3]
    //w32 0x2a007240 0x0020
    //# clock gating disable
    //w32 0x2a001C5C 0x0CBC
    //# FCIE share pin configure
    //w32 0x2a005140 0x4000
    //# PAD swtich to display.....
    //w32 0x2a006614 0x0040

    //Set reg_cmd_phase = 1
    WRITE_DISP_REG(0x2A0031a0, U16, 0x0001);

    ////PAD swtich to display.....
    WRITE_DISP_REG(0x2A006614, U16, 0x0040);
    ////PAD swtich to display.....

    WRITE_DISP_REG(0x2A0031f4, U16, 0x0000);
    WRITE_DISP_REG(0x2A003008, U16, 0x0034);
    WRITE_DISP_REG(0x2A003120, U16, 0x0024);
    WRITE_DISP_REG(0x2A003170, U16, 0x000f);
    WRITE_DISP_REG(0x2A003174, U16, 0x0000);
    WRITE_DISP_REG(0x2A00309c, U16, 0x00e4);
    WRITE_DISP_REG(0x2A003000, U16, 0x0001);
    WRITE_DISP_REG(0x2A00313c, U16, 0x0001);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0017);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*25);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0010);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x17b0);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0011);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0012);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0119);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0013);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x1400);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0015);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0029);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x000e);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x00fd);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0012);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0139);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*25);
    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0100);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0002);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0700);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0003);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x1030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0008);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0808);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0009);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x000a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x000c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x000d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0031);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0032);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0033);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0034);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0035);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0036);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0037);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0038);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0039);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003b);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0f0f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0050);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0051);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x00ef);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0052);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0053);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x013f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0060);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x2700);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0061);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x006a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0090);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0016);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0092);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0093);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);
    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0061);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);
    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0007);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0173);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0030);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0031);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0407);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0032);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0203);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0033);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0034);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0035);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0202);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0036);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x001f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0037);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0707);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0038);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0407);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0039);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0203);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003a);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003b);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0303);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003c);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0202);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x003d);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x001f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003124, U16, 0x0022);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    //Set reg_cmd_phase = 0
    WRITE_DISP_REG(0x2A0031a0, U16, 0x0000);


    WRITE_DISP_REG(0x2A0031a0, U16, 0x0001);

    WRITE_DISP_REG(0x2A003000, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);
    WRITE_DISP_REG(0x2A003000, U16, 0x0001);
    WRITE_DISP_REG(0x2A003128, U16, 0x8000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A00312c, U16, 0x0001);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*10);


    WRITE_DISP_REG(0x2A003008, U16, 0x0034);
    WRITE_DISP_REG(0x2A003120, U16, 0x0024);
    WRITE_DISP_REG(0x2A003170, U16, 0x000f);
    WRITE_DISP_REG(0x2A003174, U16, 0x0000);
    WRITE_DISP_REG(0x2A00309c, U16, 0x00e4);
    WRITE_DISP_REG(0x2A003000, U16, 0x0001);
    WRITE_DISP_REG(0x2A00313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);





    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0050);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0051);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x00ef);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0052);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0053);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x013f);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0020);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0021);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0004);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003138, U16, 0x0000);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);
    WRITE_DISP_REG(0x2A003128, U16, 0x0000);
    WRITE_DISP_REG(0x2A003124, U16, 0x0022);
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*3);

    WRITE_DISP_REG(0x2A003048, U16, 0x00f0);

    WRITE_DISP_REG(0x2A00304c, U16, 0x013F);
    WRITE_DISP_REG(0x2A003050, U16, 0x01e0);
    WRITE_DISP_REG(0x2A003044, U16, 0x0004);
    WRITE_DISP_REG(0x2A003040, U16, 0xb000);
    WRITE_DISP_REG(0x2A003100, U16, 0x0000);
    WRITE_DISP_REG(0x2A003094, U16, 0x0003);
    WRITE_DISP_REG(0x2A00309c, U16, 0x00e4);

#if 0 // 13MHz, 35 fps

    WRITE_DISP_REG(0x2A00300C, U16, 0x0100);  //H total

    WRITE_DISP_REG(0x2A003010, U16, 0x0003);  //H st

    WRITE_DISP_REG(0x2A003014, U16, 0x00f3);  //H end

    WRITE_DISP_REG(0x2A003018, U16, 0x0002);  //H high

    WRITE_DISP_REG(0x2A00301C, U16, 0x0001);  //H low



    WRITE_DISP_REG(0x2A003020, U16, 0x02E0);  //V total

    WRITE_DISP_REG(0x2A003024, U16, 0x0003);  //V st

    WRITE_DISP_REG(0x2A003028, U16, 0x0142);  //V end

    WRITE_DISP_REG(0x2A00302C, U16, 0x02DF);  //V high

    WRITE_DISP_REG(0x2A003030, U16, 0x02DE);  //V low

#endif



#if 1 // 17MHz, fps

    WRITE_DISP_REG(0x2A00300C, U16, 0x0140);  //H total

    WRITE_DISP_REG(0x2A003010, U16, 0x0003);  //H st

    WRITE_DISP_REG(0x2A003014, U16, 0x00f3);  //H end

    WRITE_DISP_REG(0x2A003018, U16, 0x0002);  //H high

    WRITE_DISP_REG(0x2A00301C, U16, 0x0001);  //H low



    WRITE_DISP_REG(0x2A003020, U16, 0x03F0);  //V total

    WRITE_DISP_REG(0x2A003024, U16, 0x0003);  //V st

    WRITE_DISP_REG(0x2A003028, U16, 0x0142);  //V end

    WRITE_DISP_REG(0x2A00302C, U16, 0x03EF);  //V high

    WRITE_DISP_REG(0x2A003030, U16, 0x03EE);  //V low

#endif

    WRITE_DISP_REG(0x2A00313c, U16, 0x0001);
    //////force update
    nand_hw_timer_delay(HW_TIMER_DELAY_1ms);

    WRITE_DISP_REG(0x2A003150, U16, 0x0010);  //set vsync mode
    WRITE_DISP_REG(0x2A003000, U16, 0x0019);

    WRITE_DISP_REG(0x2A00313c, U16, 0x0001);

    //////force update

    nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);

        //WRITE_DISP_REG(0x2A003004, U16, 0x001C); // pattern gen

        //WRITE_DISP_REG(0x2A003004, U16, 0x0010); // pattern gen

        WRITE_DISP_REG(0x2A003004, U16, 0x0013); // pattern gen

#if 0
	for(i=0;i<10000;i++)
	{
		WRITE_DISP_REG(0x2A003004, U16, 0 );
		WRITE_DISP_REG(0x2A00313c, U16, 0x0001);
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);

		WRITE_DISP_REG(0x2A003004, U16, (0x0010 | (i&0xf)) );
		WRITE_DISP_REG(0x2A00313c, U16, 0x0001);
		nand_hw_timer_delay(HW_TIMER_DELAY_1ms*100);
	}
#endif

}
#endif // IF_IP_VERIFY


//=============================================================
//=============================================================
#elif (defined(NAND_DRV_CB2_ROM) && NAND_DRV_CB2_ROM)

/* return Timer tick */
U32  nand_hw_timer_delay(U32 u32usTick)
{
  #if 1
  loop_delay_timer(u32usTick);
  #else
	volatile U32 i = 0;

	for (i = 0; i < (u32usTick>>4); i++)
	{
		#if 0
		volatile int j = 0, tmp;
		for (j = 0; j < 0; j++)
		{
			tmp = j;
		}
		#endif
	}
  #endif

    return u32usTick + 1;
}


#define REG_CHIP33h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x33)
#define reg_sd2c_drv            BIT6
#define reg_sd2c_pe             BIT7
#define reg_sd2c_ps             BIT8
#define REG_CHIP59h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x59)
#define reg_nf_mode_chip_mask   (BIT9|BIT8|BIT7)
#define reg_nf_mode_chip_shift  7
#define REG_PMU7Ch              NAND_GET_REG_ADDR(PMUTOP_BASE, 0x7C)
#define reg_nf_mode_pmu_mask    (BIT3|BIT2|BIT1)
#define reg_nf_mode_pmu_shift   1        
#define REG_FCIE10h             NAND_GET_REG_ADDR(FCIE0_BASE, 0x10)
#define BIT_reg_sdio_sd_bus_sw  BIT9
#define BIT_reg_sdio_port_sel   BIT12

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	// G1 ROM code does not need the pads_switch
	volatile U16 u16_reg;

	REG_READ_UINT16(REG_CHIP33h, u16_reg);
	u16_reg |= reg_sd2c_drv|reg_sd2c_pe|reg_sd2c_ps; // set R/B strong pull-high
	REG_WRITE_UINT16(REG_CHIP33h, u16_reg);

	REG_READ_UINT16(REG_CHIP59h, u16_reg);
	u16_reg &= ~reg_nf_mode_chip_mask;
	u16_reg |= 1<<reg_nf_mode_chip_shift;
	REG_WRITE_UINT16(REG_CHIP59h, u16_reg);

	REG_READ_UINT16(REG_PMU7Ch, u16_reg);
	u16_reg &= ~reg_nf_mode_pmu_mask;
	u16_reg |= 1<<reg_nf_mode_pmu_shift;
	REG_WRITE_UINT16(REG_PMU7Ch, u16_reg);

	REG_SET_BITS_UINT16(REG_FCIE10h, BIT_reg_sdio_sd_bus_sw);
	REG_CLR_BITS_UINT16(REG_FCIE10h, BIT_reg_sdio_port_sel);

	return UNFD_ST_SUCCESS;
}


#define REG_CHIP25h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x25)
U32  nand_clock_setting(U32 u32ClkParam)
{
	REG_CLR_BITS_UINT16(REG_CHIP25h, NFIE_REG_CLK_MASK);
	switch(u32ClkParam)
	{
		case NFIE_CLK_300K:
			u32ClkParam = NFIE_REG_CLK_300K; break;
		case NFIE_CLK_5_3M:
			u32ClkParam = NFIE_REG_CLK_5_3M; break;
		case NFIE_CLK_24M:
			u32ClkParam = NFIE_REG_CLK_24M;  break;
		case NFIE_CLK_32M:
			u32ClkParam = NFIE_REG_CLK_32M;  break;
		case NFIE_CLK_40M:
			u32ClkParam = NFIE_REG_CLK_40M;  break;
		case NFIE_CLK_48M:
			u32ClkParam = NFIE_REG_CLK_48M;  break;
		default:
			nand_die();
	}
	
	REG_SET_BITS_UINT16(REG_CHIP25h, u32ClkParam);
    return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	// NAND ns -> FCIE MHz, 
	// NAND clock = 1/2 FCIE clock
	u32_Clk = 2*(1000*1000*1000)/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_300K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_300K && u32_Clk < NFIE_CLK_5_3M)
		pNandDrv->u32_Clk = NFIE_CLK_300K;
	else if( u32_Clk >= NFIE_CLK_5_3M && u32_Clk < NFIE_CLK_24M)
		pNandDrv->u32_Clk = NFIE_CLK_5_3M;
	else if( u32_Clk >= NFIE_CLK_24M && u32_Clk < NFIE_CLK_32M)
		pNandDrv->u32_Clk = NFIE_CLK_24M;
	else if( u32_Clk >= NFIE_CLK_32M && u32_Clk < NFIE_CLK_40M)
		pNandDrv->u32_Clk = NFIE_CLK_32M;
	else if( u32_Clk >= NFIE_CLK_40M && u32_Clk < NFIE_CLK_48M)
		pNandDrv->u32_Clk = NFIE_CLK_40M;
	else 
		pNandDrv->u32_Clk = NFIE_CLK_48M;

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// G1 ROM code disable WatchDog
}

void nand_reset_WatchDog(void)
{
	// G1 ROM code disable WatchDog
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	//return 0;
	return u32_DMAAddr;
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;
void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
   return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */

U32 NC_PlatformResetPre(void)
{
	//HalMiuMaskReq(MIU_CLT_FCIE);
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	//HalMiuUnMaskReq(MIU_CLT_FCIE);
    return UNFD_ST_SUCCESS;
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	
	#if defined(UNFD_DEBUG_LEVEL) // test HW timer
	nand_debug(0,1,"test HW Timer\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	nand_debug(0,1,"sec\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	nand_debug(0,1,"sec\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	#endif
	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	pNandDrv->u8_WordMode = 0; // Columbus2 did not support x16 NAND flash
	nand_pads_switch(1);
	nand_clock_setting(FCIE3_SW_DEFAULT_CLK);

	// enable MIU_ECC patch
	REG_SET_BITS_UINT16(NC_REG_2Fh, miu_ecc_patch);

	// no shared-bus with Disp
	pNandDrv->u8_SwPatchWaitRb= 0;
	pNandDrv->u8_SwPatchJobStart= 0;
	NC_ConfigHal(NAND_HAL_RAW);

	return UNFD_ST_SUCCESS;
}

#elif (defined(NAND_DRV_CB2_UBOOT) && NAND_DRV_CB2_UBOOT)
 
/* return Timer tick */
U32  nand_hw_timer_delay(U32 u32usTick)
{
  #if 1
  udelay(u32usTick);
  #else
	volatile U32 i = 0;

	for (i = 0; i < (u32usTick>>4); i++)
	{
		#if 0
		volatile int j = 0, tmp;
		for (j = 0; j < 0; j++)
		{
			tmp = j;
		}
		#endif
	}
  #endif

    return u32usTick + 1;
}


#define REG_CHIP33h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x33)
#define reg_sd2c_drv            BIT6
#define reg_sd2c_pe             BIT7
#define reg_sd2c_ps             BIT8
#define REG_CHIP59h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x59)
#define reg_nf_mode_chip_mask   (BIT9|BIT8|BIT7)
#define reg_nf_mode_chip_shift  7
#define REG_PMU7Ch              NAND_GET_REG_ADDR(PMUTOP_BASE, 0x7C)
#define reg_nf_mode_pmu_mask    (BIT3|BIT2|BIT1)
#define reg_nf_mode_pmu_shift   1        
#define REG_FCIE10h             NAND_GET_REG_ADDR(FCIE0_BASE, 0x10)
#define BIT_reg_sdio_sd_bus_sw  BIT9
#define BIT_reg_sdio_port_sel   BIT12

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	// G1 ROM code does not need the pads_switch
	volatile U16 u16_reg;

	REG_READ_UINT16(REG_CHIP33h, u16_reg);
	u16_reg |= reg_sd2c_drv|reg_sd2c_pe|reg_sd2c_ps; // set R/B strong pull-high
	REG_WRITE_UINT16(REG_CHIP33h, u16_reg);

	REG_READ_UINT16(REG_CHIP59h, u16_reg);
	u16_reg &= ~reg_nf_mode_chip_mask;
	u16_reg |= 1<<reg_nf_mode_chip_shift;
	REG_WRITE_UINT16(REG_CHIP59h, u16_reg);

	REG_READ_UINT16(REG_PMU7Ch, u16_reg);
	u16_reg &= ~reg_nf_mode_pmu_mask;
	u16_reg |= 1<<reg_nf_mode_pmu_shift;
	REG_WRITE_UINT16(REG_PMU7Ch, u16_reg);

	REG_SET_BITS_UINT16(REG_FCIE10h, BIT_reg_sdio_sd_bus_sw);
	REG_CLR_BITS_UINT16(REG_FCIE10h, BIT_reg_sdio_port_sel);

	return UNFD_ST_SUCCESS;
}


#define REG_CHIP25h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x25)
U32  nand_clock_setting(U32 u32ClkParam)
{
	REG_CLR_BITS_UINT16(REG_CHIP25h, NFIE_REG_CLK_MASK);
	switch(u32ClkParam)
	{
		case NFIE_CLK_300K:
			u32ClkParam = NFIE_REG_CLK_300K; break;
		case NFIE_CLK_5_3M:
			u32ClkParam = NFIE_REG_CLK_5_3M; break;
		case NFIE_CLK_24M:
			u32ClkParam = NFIE_REG_CLK_24M;  break;
		case NFIE_CLK_32M:
			u32ClkParam = NFIE_REG_CLK_32M;  break;
		case NFIE_CLK_40M:
			u32ClkParam = NFIE_REG_CLK_40M;  break;
		case NFIE_CLK_48M:
			u32ClkParam = NFIE_REG_CLK_48M;  break;
		default:
			nand_die();
	}
	
	REG_SET_BITS_UINT16(REG_CHIP25h, u32ClkParam);
    return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
#if 0
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	// NAND ns -> FCIE MHz, 
	// NAND clock = 1/2 FCIE clock
	u32_Clk = 2*(1000*1000*1000)/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_300K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_300K && u32_Clk < NFIE_CLK_5_3M)
		pNandDrv->u32_Clk = NFIE_CLK_300K;
	else if( u32_Clk >= NFIE_CLK_5_3M && u32_Clk < NFIE_CLK_24M)
		pNandDrv->u32_Clk = NFIE_CLK_5_3M;
	else if( u32_Clk >= NFIE_CLK_24M && u32_Clk < NFIE_CLK_32M)
		pNandDrv->u32_Clk = NFIE_CLK_24M;
	else if( u32_Clk >= NFIE_CLK_32M && u32_Clk < NFIE_CLK_40M)
		pNandDrv->u32_Clk = NFIE_CLK_32M;
	else if( u32_Clk >= NFIE_CLK_40M && u32_Clk < NFIE_CLK_48M)
		pNandDrv->u32_Clk = NFIE_CLK_40M;
	else 
		pNandDrv->u32_Clk = NFIE_CLK_48M;

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
#endif	
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// G1 ROM code disable WatchDog
}

void nand_reset_WatchDog(void)
{
	// G1 ROM code disable WatchDog
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	// TODO: flush cache
	return u32_DMAAddr;
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;
void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
   return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */

U32 NC_PlatformResetPre(void)
{
	//HalMiuMaskReq(MIU_CLT_FCIE);
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	//HalMiuUnMaskReq(MIU_CLT_FCIE);
    return UNFD_ST_SUCCESS;
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	
	#if 0//defined(UNFD_DEBUG_LEVEL) // test HW timer
	nand_debug(0,1,"test HW Timer\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	nand_debug(0,1,"sec\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	nand_debug(0,1,"sec\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	#endif
	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	pNandDrv->u8_WordMode = 0; // Columbus2 did not support x16 NAND flash
	nand_pads_switch(1);
	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;	
	nand_clock_setting(FCIE3_SW_DEFAULT_CLK);

	// enable MIU_ECC patch
	REG_SET_BITS_UINT16(NC_REG_2Fh, miu_ecc_patch);

	// no shared-bus with Disp
	pNandDrv->u8_SwPatchWaitRb= 0;
	pNandDrv->u8_SwPatchJobStart= 0;
	NC_ConfigHal(NAND_HAL_RAW);

	return UNFD_ST_SUCCESS;
}

#elif (defined(NAND_DRV_CB2_LINUX)&&NAND_DRV_CB2_LINUX)
#include <linux/delay.h>

U32 nand_hw_timer_delay(U32 u32usTick)
{
    udelay(u32usTick);
	
	return u32usTick+1;
}

#define REG_CHIP33h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x33)
#define reg_sd2c_drv            BIT6
#define reg_sd2c_pe             BIT7
#define reg_sd2c_ps             BIT8
#define REG_CHIP59h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x59)
#define reg_nf_mode_chip_mask   (BIT9|BIT8|BIT7)
#define reg_nf_mode_chip_shift  7
#define REG_PMU7Ch              NAND_GET_REG_ADDR(PMUTOP_BASE, 0x7C)
#define reg_nf_mode_pmu_mask    (BIT3|BIT2|BIT1)
#define reg_nf_mode_pmu_shift   1        
#define REG_FCIE10h             NAND_GET_REG_ADDR(FCIE0_BASE, 0x10)
#define BIT_reg_sdio_sd_bus_sw  BIT9
#define BIT_reg_sdio_port_sel   BIT12

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	volatile U16 u16_reg;

	REG_READ_UINT16(REG_CHIP33h, u16_reg);
	u16_reg |= reg_sd2c_drv|reg_sd2c_pe|reg_sd2c_ps; // set R/B strong pull-high
	REG_WRITE_UINT16(REG_CHIP33h, u16_reg);

	REG_READ_UINT16(REG_CHIP59h, u16_reg);
	u16_reg &= ~reg_nf_mode_chip_mask;
	u16_reg |= 1<<reg_nf_mode_chip_shift;
	REG_WRITE_UINT16(REG_CHIP59h, u16_reg);

	REG_READ_UINT16(REG_PMU7Ch, u16_reg);
	u16_reg &= ~reg_nf_mode_pmu_mask;
	u16_reg |= 1<<reg_nf_mode_pmu_shift;
	REG_WRITE_UINT16(REG_PMU7Ch, u16_reg);

	REG_SET_BITS_UINT16(REG_FCIE10h, BIT_reg_sdio_sd_bus_sw);
	REG_CLR_BITS_UINT16(REG_FCIE10h, BIT_reg_sdio_port_sel);

	return UNFD_ST_SUCCESS;
}

#define REG_CHIP25h             NAND_GET_REG_ADDR(CHIPTOP_BASE, 0x25)
U32 nand_clock_setting(U32 u32ClkParam)
{
	REG_CLR_BITS_UINT16(REG_CHIP25h, NFIE_REG_CLK_MASK);
	switch(u32ClkParam)
	{
		case NFIE_CLK_300K:
			u32ClkParam = NFIE_REG_CLK_300K; break;
		case NFIE_CLK_5_3M:
			u32ClkParam = NFIE_REG_CLK_5_3M; break;
		case NFIE_CLK_24M:
			u32ClkParam = NFIE_REG_CLK_24M;  break;
		case NFIE_CLK_32M:
			u32ClkParam = NFIE_REG_CLK_32M;  break;
		case NFIE_CLK_40M:
			u32ClkParam = NFIE_REG_CLK_40M;  break;			
		case NFIE_CLK_48M:
			u32ClkParam = NFIE_REG_CLK_48M;  break;
		case NFIE_CLK_80M:
			u32ClkParam = NFIE_REG_CLK_80M;  break;			
		default:
			nand_die();
	}
	
	REG_SET_BITS_UINT16(REG_CHIP25h, u32ClkParam);
    return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
#if 1
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	// NAND ns -> FCIE MHz, 
	// NAND clock = 1/2 FCIE clock
	u32_Clk = 2*(1000*1000*1000)/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_300K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_300K && u32_Clk < NFIE_CLK_5_3M)
		pNandDrv->u32_Clk = NFIE_CLK_300K;
	else if( u32_Clk >= NFIE_CLK_5_3M && u32_Clk < NFIE_CLK_24M)
		pNandDrv->u32_Clk = NFIE_CLK_5_3M;
	else if( u32_Clk >= NFIE_CLK_24M && u32_Clk < NFIE_CLK_32M)
		pNandDrv->u32_Clk = NFIE_CLK_24M;
	else if( u32_Clk >= NFIE_CLK_32M && u32_Clk < NFIE_CLK_40M)
		pNandDrv->u32_Clk = NFIE_CLK_32M;
/*	
	else if( u32_Clk >= NFIE_CLK_40M && u32_Clk < NFIE_CLK_48M)
		pNandDrv->u32_Clk = NFIE_CLK_40M;
	else if( u32_Clk >= NFIE_CLK_48M && u32_Clk < NFIE_CLK_80M)
		pNandDrv->u32_Clk = NFIE_CLK_48M;
*/		
	else 
		pNandDrv->u32_Clk = NFIE_CLK_80M;

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
#endif	
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// do nothing in Linux
}

void nand_reset_WatchDog(void)
{
	// do nothing in Linux
}

extern void hal_dcache_flush(void *base , u32 asize);
U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	hal_dcache_flush((void *)u32_DMAAddr, u32_ByteCnt);

	return (virt_to_phys((void*)u32_DMAAddr));
}

void nand_flush_miu_pipe(void)
{

}

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);
U16 u16_FcieMieEvent = 0;

irqreturn_t NC_FCIE_IRQ(int irq, void *dummy)
{
	U16 u16_Reg;

	u16_Reg = REG(NC_PATH_CTL); 
	if((u16_Reg&BIT_NC_EN) != BIT_NC_EN)
	{
		//nand_debug(UNFD_DEBUG_LEVEL, 1 ,"not nand interrupt\r\n");
		// not NAND interrupt
		return IRQ_NONE;
	}

	u16_Reg = REG(NC_MIE_EVENT);
	//nand_debug(UNFD_DEBUG_LEVEL, 1 ,"NC_MIE_EVENT:%X\r\n",u16_Reg);	
	u16_FcieMieEvent |= u16_Reg;
	u16_Reg &= REG(NC_MIE_INT_EN);	
	REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_Reg); /*clear events*/
	wake_up(&fcie_wait);
	return IRQ_HANDLED;
}

U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	U16 u16_Reg;
		
	wait_event_timeout(fcie_wait, ((u16_FcieMieEvent&u16_WaitEvent) == u16_WaitEvent), usecs_to_jiffies(u32_MicroSec));
	if((u16_FcieMieEvent&u16_WaitEvent) != u16_WaitEvent)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "u16_FcieMieEvent: %X\r\n", u16_FcieMieEvent);
	
		return UNFD_ST_ERR_E_TIMEOUT;
	}

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_WaitEvent); /*clear events*/
	return UNFD_ST_SUCCESS;
}

void nand_enable_intr_mode(void)
{
	u16_FcieMieEvent = 0;
	REG_WRITE_UINT16(NC_MIE_INT_EN, BIT_NC_JOB_END);
}
#endif

U32 NC_PlatformResetPre(void)
{
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	
	#if defined(DUTY_CYCLE_PATCH)&&DUTY_CYCLE_PATCH
	if( pNandDrv->u32_Clk == NFIE_CLK_80M )
		REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);	// duty cycle 3:1 in 86Mhz
	#endif
	return UNFD_ST_SUCCESS;
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	
	#if 0//defined(UNFD_DEBUG_LEVEL) // test HW timer
	nand_debug(0,1,"test HW Timer\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	nand_debug(0,1,"sec\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	nand_debug(0,1,"sec\n");
	nand_hw_timer_delay(HW_TIMER_DELAY_1s);
	#endif
	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	pNandDrv->u8_WordMode = 0; // Columbus2 did not support x16 NAND flash
	nand_pads_switch(1);
	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;	
	nand_clock_setting(FCIE3_SW_DEFAULT_CLK);

	// enable MIU_ECC patch
	REG_SET_BITS_UINT16(NC_REG_2Fh, miu_ecc_patch);

	// no shared-bus with Disp
	pNandDrv->u8_SwPatchWaitRb= 0;
	pNandDrv->u8_SwPatchJobStart= 0;
	NC_ConfigHal(NAND_HAL_RAW);

	return UNFD_ST_SUCCESS;
}

#elif (defined(NAND_DRV_CEDRIC_LINUX) && NAND_DRV_CEDRIC_LINUX)
/* return Timer tick */
#include <linux/delay.h>
U32  nand_hw_timer_delay(U32 u32usTick)   
{
  #if 1	//xxx tomodify
  udelay(u32usTick); 
  #else
	volatile U32 i = 0;

	for (i = 0; i < (u32usTick>>4); i++)
	{
		#if 0
		volatile int j = 0, tmp;
		for (j = 0; j < 0; j++)
		{
			tmp = j;
		}
		#endif
	}
  #endif

    return u32usTick + 1;
}

U32 nand_pads_switch(U32 u32EnableFCIE) 
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
#if defined(FCIE4_DDR) && FCIE4_DDR
	pNandDrv->u16_Reg58_DDRCtrl &= ~(BIT_DDR_MASM|BIT_SDR_DIN_FROM_MACRO);
	
	if(NAND_PAD_BYPASS_MODE == u32EnableFCIE)
	{
		//printf("nand_pads_switch BYPASS\r\n");
    REG_SET_BITS_UINT16(NC_REG_2Fh, BIT11);
    REG_CLR_BITS_UINT16(REG_ALL_PAD_IN, BIT0);
    REG_SET_BITS_UINT16(REG_BPS_MODE, (BIT10 - 1));
    REG_SET_BITS_UINT16(REG_NF_MODE, BIT6);	
	}
	else if(NAND_PAD_TOGGLE_MODE == u32EnableFCIE)
	{
		//RETAILMSG(1, (_T("nand_pads_switch TOGGLE\r\n")));
		//printf("nand_pads_switch TOGGLE\r\n");
		REG_CLR_BITS_UINT16(NC_REG_2Fh, BIT11);
    REG_CLR_BITS_UINT16(REG_ALL_PAD_IN, BIT0);
    REG_CLR_BITS_UINT16(REG_BPS_MODE, (BIT10 - 1));
    REG_SET_BITS_UINT16(REG_NF_MODE, BIT6);
		pNandDrv->u16_Reg58_DDRCtrl |= (BIT_DDR_TOGGLE|BIT_SDR_DIN_FROM_MACRO);
	}
	else if(NAND_PAD_ONFI_SYNC_MODE == u32EnableFCIE)
	{
		//printf("nand_pads_switch ONFI\r\n"); 
		REG_CLR_BITS_UINT16(NC_REG_2Fh, BIT11);
    REG_CLR_BITS_UINT16(REG_ALL_PAD_IN, BIT0);
    REG_CLR_BITS_UINT16(REG_BPS_MODE, (BIT10 - 1));
    REG_SET_BITS_UINT16(REG_NF_MODE, BIT6);
		pNandDrv->u16_Reg58_DDRCtrl |= (BIT_DDR_ONFI|BIT_SDR_DIN_FROM_MACRO);
		
	}

	REG_WRITE_UINT16(NC_DDR_CTRL, pNandDrv->u16_Reg58_DDRCtrl);
	
#else	//defined(FCIE4_DDR) && FCIE4_DDR

		pNandDrv->u16_Reg58_DDRCtrl &= ~(BIT_DDR_MASM|BIT_SDR_DIN_FROM_MACRO);
    REG_SET_BITS_UINT16(NC_REG_2Fh, BIT11);
    REG_CLR_BITS_UINT16(REG_ALL_PAD_IN, BIT0);
    REG_SET_BITS_UINT16(REG_BPS_MODE, (BIT10 - 1));
    REG_SET_BITS_UINT16(REG_NF_MODE, BIT6);	
		REG_WRITE_UINT16(NC_DDR_CTRL, pNandDrv->u16_Reg58_DDRCtrl);
		
#endif
		return 0;
}

U32 nand_clock_setting(U32 u32ClkParam)   
{
		NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
		//RETAILMSG(1, (_T("nand_clock_setting : %X\r\n"), u32ClkParam));
    REG_CLR_BITS_UINT16(REG_CLK_NFIE, NFIE_CLK_MASK);
    //printf("nand_clock_setting : %X\r\n", u32ClkParam);
    /*
    switch(u32ClkParam)
    {
    case NFIE_CLK_300K:
        u32ClkParam = NFIE_REG_CLK_300K;
        break;
    case NFIE_CLK_1200K:
        u32ClkParam = NFIE_REG_CLK_1200K;
        break;
    case NFIE_CLK_12M:
        u32ClkParam = NFIE_REG_CLK_12M;
        break;        
    case NFIE_CLK_24M:
        u32ClkParam = NFIE_REG_CLK_24M;
        break;
    case NFIE_CLK_32M:
        u32ClkParam = NFIE_REG_CLK_32M;
        break;
    case NFIE_CLK_36M:
        u32ClkParam = NFIE_REG_CLK_36M;
        break;
    case NFIE_CLK_40M:
        u32ClkParam = NFIE_REG_CLK_40M;
        break;
    case NFIE_CLK_43_2M:
        u32ClkParam = NFIE_REG_CLK_43_2M;
        break;
    case NFIE_CLK_48M:
        u32ClkParam = NFIE_REG_CLK_48M;
        break;
    case NFIE_CLK_54M:
        u32ClkParam = NFIE_REG_CLK_54M;
        break;
    case NFIE_CLK_62M:
        u32ClkParam = NFIE_REG_CLK_62M;
        break;
    case NFIE_CLK_72M:
        u32ClkParam = NFIE_REG_CLK_72M;
        break;
    case NFIE_CLK_80M:
        u32ClkParam = NFIE_REG_CLK_80M;
        break;
    case NFIE_CLK_86M:
        u32ClkParam = NFIE_REG_CLK_86M;
        break;
    case NFIE_CLK_108M:
        u32ClkParam = NFIE_REG_CLK_108M;
        break;
    case NFIE_CLK_123M:
        u32ClkParam = NFIE_REG_CLK_123M;
        break;
    case NFIE_CLK_144M:
        u32ClkParam = NFIE_REG_CLK_144M;
        break;
    case NFIE_CLK_172M:
        u32ClkParam = NFIE_REG_CLK_172M;
        break;
    case NFIE_CLK_192M:
        u32ClkParam = NFIE_REG_CLK_192M;
        break;
    case NFIE_CLK_216M:
        u32ClkParam = NFIE_REG_CLK_216M;
        break;
    case NFIE_CLK_240M:
        u32ClkParam = NFIE_REG_CLK_240M;
        break;
    case NFIE_CLK_345M:
        u32ClkParam = NFIE_REG_CLK_345M;
        break;
    case NFIE_CLK_432M:
        u32ClkParam = NFIE_REG_CLK_432M;
        break;
    default:
        RETAILMSG(1, (_T(__FUNCTION__) _T("Clock setting error\r\n")));
        while(1);
    }
    */
    REG_SET_BITS_UINT16(REG_CLK_NFIE, u32ClkParam);

#if defined(FCIE4_DDR) && FCIE4_DDR
		//if (sg_NandDev.u8_nand_pads_switch_mode == NAND_PAD_BYPASS_MODE)  
		if ((pNandDrv->u16_Reg58_DDRCtrl & (BIT_DDR_MASM|BIT_SDR_DIN_FROM_MACRO)) == 0)
		{
				REG_CLR_BITS_UINT16(FCIE_REG_CLK_4X_DIV_EN, BIT0);
			
				REG_CLR_BITS_UINT16(NC_LATCH_DATA, BIT_NC_LATCH_DATA_MASK);
				if (pNandDrv->u8_RequireRandomizer)
				{
						//RETAILMSG(1, (_T("delay 1.5T\r\n")));
						REG_SET_BITS_UINT16(NC_LATCH_DATA, BIT_NC_LATCH_DATA_1_5_T);
				}
				else
				{
						//RETAILMSG(1, (_T("delay 0.5T\r\n")));
						REG_SET_BITS_UINT16(NC_LATCH_DATA, BIT_NC_LATCH_DATA_0_5_T);
				}
		}
		else
		{
				//printf("SET_BITS_UINT16(FCIE_REG_CLK_4X_DIV_EN, BIT0)\r\n");
				REG_SET_BITS_UINT16(FCIE_REG_CLK_4X_DIV_EN, BIT0);
				REG_WRITE_UINT16(NC_LATCH_DATA,pNandDrv->u16_Reg57_RELatch);
		}
#else
ttt
		REG_CLR_BITS_UINT16(NC_LATCH_DATA, BIT_NC_LATCH_DATA_MASK);
		if (pNandDrv->u8_RequireRandomizer)
		{
				//RETAILMSG(1, (_T("delay 1.5T\r\n")));
				REG_SET_BITS_UINT16(NC_LATCH_DATA, BIT_NC_LATCH_DATA_1_5_T);
		}
		else
		{
				//RETAILMSG(1, (_T("delay 0.5T\r\n")));
				REG_SET_BITS_UINT16(NC_LATCH_DATA, BIT_NC_LATCH_DATA_0_5_T);
		}
#endif //defined(FCIE4_DDR) && FCIE4_DDR

		return UNFD_ST_SUCCESS;
}

void nand_DumpPadClk(void)
{
#if 0
	nand_debug(0, 1, "clk setting: \n");
	nand_debug(0, 1, "reg_ckg_fcie(0x%X):0x%x\n", reg_ckg_fcie, REG(reg_ckg_fcie));
#if (ENABLE_EAGLE)
	nand_debug(0, 1, "REG_CLK_EMMC(0x%X):0x%x\n", REG_CLK_EMMC, REG(REG_CLK_EMMC));
#endif

#if (ENABLE_AGATE)
	nand_debug(0, 1, "reg_clk4x_div_en(0x%X):0x%x\n", reg_clk4x_div_en, REG(reg_clk4x_div_en));
#endif
	nand_debug(0, 1, "pad setting: \n");
	//fcie pad register
	nand_debug(0, 1, "NC_REG_2Fh(0x%X):0x%x\n", NC_REG_2Fh, REG(NC_REG_2Fh));
	nand_debug(0, 1, "NC_DDR_CTRL(0x%X):0x%x\n", NC_DDR_CTRL, REG(NC_DDR_CTRL));
	//chiptop pad register
	nand_debug(0, 1, "reg_all_pad_in(0x%X):0x%x\n", reg_allpad_in, REG(reg_allpad_in));
	//platform depend reg_sd_use_bypass
#if (ENABLE_EAGLE)
	nand_debug(0, 1, "REG_SDR_BYPASS_MODE(0x%X):0x%x\n", REG_SDR_BYPASS_MODE, REG(REG_SDR_BYPASS_MODE));
#endif
#if (ENABLE_AGATE)
	nand_debug(0, 1, "reg_fcie2macro_sd_bypass(0x%X):0x%x\n", reg_fcie2macro_sd_bypass, REG(reg_fcie2macro_sd_bypass));
#endif
	nand_debug(0, 1, "reg_nf_en(0x%X):0x%x\n", reg_nf_en, REG(reg_nf_en));
#endif
}


#if defined(DECIDE_CLOCK_BY_NAND) && DECIDE_CLOCK_BY_NAND
#define MAX(a,b) ((a) > (b) ? (a) : (b))

U32 nand_config_timing(U16 u16_1T)  
{
	#if 1	// defined(FCIE4_DDR) && FCIE4_DDR
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U16 u16_DefaultTRR;
	U16 u16_DefaultTCS;
	U16 u16_DefaultTWW;
	U16 u16_DefaultRX40Cmd;
	U16 u16_DefaultRX40Adr;
	U16 u16_DefaultRX56;
	U16 u16_DefaultTADL;
	U16 u16_DefaultTCWAW;
	U16 u16_DefaultTCLHZ = 0;
	U16 u16_DefaultTWHR;
	U16 u16_Tmp, u16_Cnt;
	U16 u16_Tmp2, u16_Cnt2;


	if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_ONFI)
	{
		u16_DefaultTRR = NC_ONFI_DEFAULT_TRR;
		u16_DefaultTCS = NC_ONFI_DEFAULT_TCS;
		u16_DefaultTWW = NC_ONFI_DEFAULT_TWW;
		u16_DefaultRX40Cmd = NC_ONFI_DEFAULT_RX40CMD;
		u16_DefaultRX40Adr = NC_ONFI_DEFAULT_RX40ADR;
		u16_DefaultRX56 = NC_ONFI_DEFAULT_RX56;
		u16_DefaultTADL = NC_ONFI_DEFAULT_TADL;
		u16_DefaultTCWAW = NC_ONFI_DEFAULT_TCWAW;
		u16_DefaultTWHR = NC_ONFI_DEFAULT_TWHR;
	}
	else if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_TOGGLE)
	{
		u16_DefaultTRR = NC_TOGGLE_DEFAULT_TRR;
		u16_DefaultTCS = NC_TOGGLE_DEFAULT_TCS;
		u16_DefaultTWW = NC_TOGGLE_DEFAULT_TWW;
		u16_DefaultRX40Cmd = NC_TOGGLE_DEFAULT_RX40CMD;
		u16_DefaultRX40Adr = NC_TOGGLE_DEFAULT_RX40ADR;
		u16_DefaultRX56 = NC_TOGGLE_DEFAULT_RX56;
		u16_DefaultTADL = NC_TOGGLE_DEFAULT_TADL;
		u16_DefaultTCWAW = NC_TOGGLE_DEFAULT_TCWAW;
		u16_DefaultTWHR = NC_TOGGLE_DEFAULT_TWHR;
	}
	else

	{
		u16_DefaultTRR = NC_SDR_DEFAULT_TRR;
		u16_DefaultTCS = NC_SDR_DEFAULT_TCS;
		u16_DefaultTWW = NC_SDR_DEFAULT_TWW;
		u16_DefaultRX40Cmd = NC_SDR_DEFAULT_RX40CMD;
		u16_DefaultRX40Adr = NC_SDR_DEFAULT_RX40ADR;
		u16_DefaultRX56 = NC_SDR_DEFAULT_RX56;
		u16_DefaultTADL = NC_SDR_DEFAULT_TADL;
		u16_DefaultTCWAW = NC_SDR_DEFAULT_TCWAW;
		u16_DefaultTWHR = NC_SDR_DEFAULT_TWHR;		
	}
	#endif
	
	#if defined(NC_INST_DELAY) && NC_INST_DELAY
	// Check CMD_END
	u16_Tmp = MAX(pNandDrv->u16_tWHR, pNandDrv->u16_tCWAW);
	u16_Cnt = (u16_Tmp+u16_1T-1)/u16_1T;

	if(u16_DefaultRX40Cmd >= u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultRX40Cmd > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultRX40Cmd;

	// Check ADR_END
	u16_Tmp2 = MAX(MAX(pNandDrv->u16_tWHR, pNandDrv->u16_tADL), pNandDrv->u16_tCCS);
	u16_Cnt2 = (u16_Tmp2+u16_1T-1)/u16_1T;

	if(u16_DefaultRX40Adr >= u16_Cnt2)
		u16_Cnt2 = 0;
	else if(u16_Cnt2-u16_DefaultRX40Adr > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt2 -= u16_DefaultRX40Adr;

	// get the max cnt
	u16_Cnt = MAX(u16_Cnt, u16_Cnt2);

	pNandDrv->u16_Reg40_Signal &= ~(0x00FF<<8);
	pNandDrv->u16_Reg40_Signal |= (u16_Cnt<<8);
	nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "u16_Reg40_Signal =  %X\n",pNandDrv->u16_Reg40_Signal);	
	#endif

	#if defined(NC_HWCMD_DELAY) && NC_HWCMD_DELAY
	u16_Cnt = (pNandDrv->u16_tRHW+u16_1T-1)/u16_1T + 2;

	if(u16_DefaultRX56 >= u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultRX56 > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultRX56;

	pNandDrv->u16_Reg56_Rand_W_Cmd &= ~(0x00FF<<8);
	pNandDrv->u16_Reg56_Rand_W_Cmd |= (u16_Cnt<<8);
	nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "u16_Reg56_Rand_W_Cmd =  %X\n",pNandDrv->u16_Reg56_Rand_W_Cmd);
	#endif
	
	#if defined(NC_TRR_TCS) && NC_TRR_TCS
	u16_Cnt = (pNandDrv->u8_tRR+u16_1T-1)/u16_1T + 2;

	if(u16_DefaultTRR >= u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultTRR > 0x0F)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultTRR;

	u16_Tmp = (pNandDrv->u8_tCS+u16_1T-1)/u16_1T + 2;

	if(u16_DefaultTCS >= u16_Tmp)
		u16_Tmp = 0;
	else if(u16_Tmp-u16_DefaultTCS > 0x0F)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Tmp -= u16_DefaultTCS;

	u16_Tmp2 = (pNandDrv->u16_tWW+u16_1T-1)/u16_1T + 2;

    if(u16_DefaultTWW >= u16_Tmp2)
        u16_Tmp2 = 0;
    else if(u16_Tmp2-u16_DefaultTWW > 0x0F)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
        u16_Tmp2 -= u16_DefaultTWW;

	u16_Cnt2 = MAX(u16_Tmp, u16_Tmp2);

	pNandDrv->u16_Reg59_LFSRCtrl &= ~(0x00FF);
	pNandDrv->u16_Reg59_LFSRCtrl |= (u16_Cnt|(u16_Cnt2<<4));
	nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "u16_Reg59_LFSRCtrl =  %X\n",pNandDrv->u16_Reg59_LFSRCtrl);	
	#endif

	#if defined(NC_TCWAW_TADL) && NC_TCWAW_TADL 
	u16_Cnt = (pNandDrv->u16_tADL + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTADL > u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultTADL > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultTADL;
	
	u16_Cnt2 = (pNandDrv->u16_tCWAW + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTADL > u16_Cnt2)
		u16_Cnt2 = 0;
	else if(u16_Cnt2-u16_DefaultTCWAW > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt2 -= u16_DefaultTCWAW;

	pNandDrv->u16_Reg5D_tCWAW_tADL &= ~(0xFFFF);
	pNandDrv->u16_Reg5D_tCWAW_tADL |= (u16_Cnt|(u16_Cnt2<<8));
	nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "u16_Reg5D_tCWAW_tADL =  %X\n",pNandDrv->u16_Reg5D_tCWAW_tADL);	
	#endif

	#if defined(NC_TWHR_TCLHZ) && NC_TWHR_TCLHZ 
	u16_Cnt = (pNandDrv->u8_tCLHZ + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTCLHZ > u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultTCLHZ > 0xF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultTCLHZ;
	
	u16_Cnt2 = (pNandDrv->u16_tWHR + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTWHR > u16_Cnt2)
		u16_Cnt2 = 0;
	else if(u16_Cnt2-u16_DefaultTWHR > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt2 -= u16_DefaultTWHR;

	pNandDrv->u16_Reg5A_tWHR_tCLHZ &= ~(0xFFFF);
	pNandDrv->u16_Reg5A_tWHR_tCLHZ |= ((u16_Cnt&0xF)|(u16_Cnt2<<8));
	nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "u16_Reg5A_tWHR_tCLHZ =  %X\n",pNandDrv->u16_Reg5A_tWHR_tCLHZ);
	#endif

	NC_Config();
	return UNFD_ST_SUCCESS;
}

U32 nand_find_timing(U8 *pu8_ClkIdx, U8 u8_find_DDR_timg)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 au32_1TTable[NFIE_CLK_TABLE_CNT] = NFIE_1T_TABLE;
	U32 au32_ClkValueTable[NFIE_CLK_TABLE_CNT] = NFIE_CLK_VALUE_TABLE;
	
	#if defined(FCIE4_DDR) && FCIE4_DDR
	U32 au32_4Clk1TTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_1T_TABLE;
	U32 au32_4ClkValueTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_VALUE_TABLE;
	#endif
	U32 u32_Clk;
	U16 u16_SeqAccessTime, u16_Tmp, u16_Tmp2, u16_1T, u16_RE_LATCH_DELAY;
	S8 s8_ClkIdx;
	
	
	if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_ONFI)
	{
		u16_SeqAccessTime = 10;
	}
	else if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_TOGGLE)
	{
		u16_Tmp = MAX(MAX(2*pNandDrv->u8_tRP, 2*pNandDrv->u8_tREH), pNandDrv->u16_tRC);
		u16_Tmp2 = MAX(MAX(pNandDrv->u8_tWP, pNandDrv->u8_tWH), (pNandDrv->u16_tWC+1)/2);
		u16_SeqAccessTime = MAX(u16_Tmp, u16_Tmp2);
	}
	else
	{
		u16_Tmp = MAX(MAX(pNandDrv->u8_tRP, pNandDrv->u8_tREH), (pNandDrv->u16_tRC+1)/2);
		u16_Tmp2 = MAX(MAX(pNandDrv->u8_tWP, pNandDrv->u8_tWH), (pNandDrv->u16_tWC+1)/2);
		u16_SeqAccessTime = MAX(u16_Tmp, u16_Tmp2);

		u16_Tmp = (pNandDrv->u8_tREA + NAND_SEQ_ACC_TIME_TOL)/2;
		u16_Tmp2 = u16_SeqAccessTime;
		u16_SeqAccessTime = MAX(u16_Tmp, u16_Tmp2);
	}

	nand_printf("u16_SeqAccessTime :%X\r\n", u16_SeqAccessTime);

	u32_Clk = 1000000000/((U32)u16_SeqAccessTime); 

	if(!u8_find_DDR_timg)
	{
		for(s8_ClkIdx =  0; s8_ClkIdx <= NFIE_CLK_TABLE_CNT - 1; s8_ClkIdx ++)
		{
			if(u32_Clk <= au32_ClkValueTable[s8_ClkIdx])
			{
				break;
			}
		}
	}
	else
	{
		#if defined(FCIE4_DDR) && FCIE4_DDR
		for(s8_ClkIdx =  0; s8_ClkIdx <= NFIE_4CLK_TABLE_CNT - 1; s8_ClkIdx ++)
		{
			if(u32_Clk < au32_4ClkValueTable[s8_ClkIdx])
			{
				break;
			}
		}
		#endif
	}
	s8_ClkIdx --;

	
RETRY:
	if(s8_ClkIdx<0)
		return UNFD_ST_ERR_INVALID_PARAM;
	if(!u8_find_DDR_timg)
		u16_1T = au32_1TTable[s8_ClkIdx];
	#if defined(FCIE4_DDR) && FCIE4_DDR
	else
		u16_1T = au32_4Clk1TTable[s8_ClkIdx];
	#endif
	
	if(nand_config_timing(u16_1T) != UNFD_ST_SUCCESS)
	{
		s8_ClkIdx--;
		goto RETRY;
	}

	u16_RE_LATCH_DELAY = BIT_NC_LATCH_DATA_1_0_T;

	#if 0
	if( (1 + 0.5) * (float)u16_1T  >= (float)pNandDrv->u8_tREA + 3)
	{
		u16_RE_LATCH_DELAY = BIT_NC_LATCH_DATA_0_5_T;
	}
	else if((1 + 1) * (float)u16_1T >= (float)pNandDrv->u8_tREA + 3)
	{
		u16_RE_LATCH_DELAY = BIT_NC_LATCH_DATA_1_0_T;
	}
	else if((1 + 1.5) * (float)u16_1T >= (float)pNandDrv->u8_tREA + 3)
	{
		u16_RE_LATCH_DELAY = BIT_NC_LATCH_DATA_1_5_T;
	}
	else if((1 + 2) * (float)u16_1T >= (float)pNandDrv->u8_tREA + 3)
	{
		u16_RE_LATCH_DELAY = BIT_NC_LATCH_DATA_2_0_T;
	}
	#endif
	pNandDrv->u16_Reg57_RELatch &= ~BIT_NC_LATCH_DATA_MASK;
	pNandDrv->u16_Reg57_RELatch |= u16_RE_LATCH_DELAY;
	
	*pu8_ClkIdx	= (U8)s8_ClkIdx;
	
	return UNFD_ST_SUCCESS;
	
}
#endif	//DECIDE_CLOCK_BY_NAND

#if 0
U32 nand_detect_ddr_timing(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Err;
	DDR_TIMING_GROUP_t *ptDDR;	
	U8 u8_2Ch_dqsmode, u8_delaycell, u8_57h,u8_max_delay_cell;;
	U8 u8_57h_start = 0;
	U8 u8_pass_cnt;
	U8 u8_clk, u8_clk_default;
	const U8 au8_dqsmode[DQS_MODE_TABLE_CNT] = DQS_MODE_TABLE;	// defined in platform-depedent .h
	U32 au32_ClkTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_TABLE; 
	U32 au32_1TTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_1T_TABLE; 	
	U8 u8_DqsMatch;
	
	U8 pu8buffer1[(16*DQS_MODE_TABLE_CNT*sizeof(DDR_TIMING_GROUP_t))];
	U8 pu8buffer2[1024];
	U8 pu8buffer3[1024];
	ptDDR = (DDR_TIMING_GROUP_t *)pu8buffer1;
	//ptDDR = (DDR_TIMING_GROUP_t *)malloc(16*DQS_MODE_TABLE_CNT*sizeof(DDR_TIMING_GROUP_t));
	au16_WritePageBuf = (U16 *)pu8buffer2;
	//au16_WritePageBuf = (U16 *)malloc(pNandDrv->u16_SectorByteCnt);
	au16_ReadPageBuf = (U16 *)pu8buffer3;
	//au16_ReadPageBuf = (U16 *)malloc(pNandDrv->u16_SectorByteCnt);	
	if(!ptDDR || !au16_WritePageBuf || !au16_ReadPageBuf)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err, NAND, cannot allocate memory\n");
		nand_die();
		return UNFD_ST_ERR_INVALID_ADDR;
	}

	u32_Err = nand_prepare_test_pattern();    
	if(u32_Err < UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err, NAND, cannot prepare test pattern\n");	
		nand_die();
		return UNFD_ST_ERR_LACK_BLK;
	}

	/**************** search min ddr timing *********************/
	nand_debug(UNFD_DEBUG_LEVEL, 0,"*** Try to detect min ddr timing ***\r\n");			
	nand_printf("*** Try to detect min ddr timing ***\r\n");
	u8_DqsMatch = 0;
	pNandDrv->u32_Clk = au32_ClkTable[0];
	nand_clock_setting(pNandDrv->u32_Clk);
	nand_config_timing(au32_1TTable[0]);
	gu8_unfd_disable_uart = 0;
	nand_debug(UNFD_DEBUG_LEVEL, 0," **********************************************\r\n");
    nand_debug(UNFD_DEBUG_LEVEL, 0," CLK[%X]\r\n", pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 0," **********************************************\r\n");	
	u8_max_delay_cell=0x1;
	
	for(u8_2Ch_dqsmode=0 ; u8_2Ch_dqsmode<DQS_MODE_TABLE_CNT ; u8_2Ch_dqsmode++)
	{
		u8_pass_cnt = 0;
		for(u8_delaycell=0; u8_delaycell<u8_max_delay_cell; u8_delaycell++)
		{
        	for(u8_57h=3 ; u8_57h<0x10 ; u8_57h++) // ddr timing shouldn't match from 0~2   
        	{
        		NC_FCIE4SetInterface(1, au8_dqsmode[u8_2Ch_dqsmode], u8_delaycell, u8_57h);    
        		if(UNFD_ST_SUCCESS == nand_probe_ddr())        
        		{
        			nand_debug(UNFD_DEBUG_LEVEL, 0,"clk[%X], dqsmode[%u], DelayCell[%u], ddr_timing[%u]\r\n",
        						pNandDrv->u32_Clk, au8_dqsmode[u8_2Ch_dqsmode], u8_delaycell,
        						u8_57h);				
        			if(u8_pass_cnt == 0)
        				u8_57h_start = u8_57h;
        
        			if((++u8_pass_cnt)==MIN_PASS_CNT)
        			{
        				ptDDR[u8_2Ch_dqsmode].u8_ClkIdx = 0;
        				ptDDR[u8_2Ch_dqsmode].u8_DqsMode = au8_dqsmode[u8_2Ch_dqsmode];
        				ptDDR[u8_2Ch_dqsmode].u8_DelayCell = u8_delaycell;
        				ptDDR[u8_2Ch_dqsmode].u8_DdrTiming = u8_57h_start+1;
        				u8_DqsMatch |= (1<<u8_2Ch_dqsmode);
        				break;
        			}
        		}
        		else
        		{
        			if(u8_pass_cnt != 0)
        				break;
        		}		
        	}
		}
	}
    gu8_unfd_disable_uart = 0;
	u8_2Ch_dqsmode = 0;
	switch(u8_DqsMatch)
	{
		case 0x0:
		case 0x8:
		case 0x4:
		case 0xA:
		case 0x1:
		case 0x9:
		case 0x5:
		case 0xD:
		case 0xB:
			nand_debug(UNFD_DEBUG_LEVEL, 0,"Cannot find ddr timing @clk=%X, DQS combination=%X\r\n", pNandDrv->u32_Clk, u8_DqsMatch);			
			break;
			
		case 0x2:
		case 0x3:			
			if(nand_probe_ddr_ex(aau8_half_delay_cell[u8_clk][1], 1) == UNFD_ST_SUCCESS)  
				u8_2Ch_dqsmode = 1; 
			break;	
			
		case 0xC: // 2.5T
			u8_2Ch_dqsmode = 3;
			break;
		case 0x6: // 1.5T
			u8_2Ch_dqsmode = 1;
			break;
		case 0xE: // 2T
			u8_2Ch_dqsmode = 2;
			break;			
		case 0x7: // 1.5T
			u8_2Ch_dqsmode = 1;
			break;
		case 0xF: // 2T
			u8_2Ch_dqsmode = 2;
			break;
	}
    if(u8_2Ch_dqsmode == 0)
	{
		nand_printf("Err, NAND, can't detect min ddr timing\n");
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err, NAND, can't detect min ddr timing\n");
		nand_die();
		return UNFD_ST_ERR_NO_TOGGLE_DDR_TIMING;
	}
	else
	{
		pNandDrv->tMinDDR.u8_ClkIdx = ptDDR[u8_2Ch_dqsmode].u8_ClkIdx;
		pNandDrv->tMinDDR.u8_DqsMode = ptDDR[u8_2Ch_dqsmode].u8_DqsMode;
		pNandDrv->tMinDDR.u8_DelayCell = ptDDR[u8_2Ch_dqsmode].u8_DelayCell;
		pNandDrv->tMinDDR.u8_DdrTiming = ptDDR[u8_2Ch_dqsmode].u8_DdrTiming;
		nand_debug(UNFD_DEBUG_LEVEL,1,"ok, get min DDR timing: clk[%X], dqsmode[%u], DelayCell[%u], ddr_timing[%u]\r\n",
				pNandDrv->tMinDDR.u8_ClkIdx, pNandDrv->tMinDDR.u8_DqsMode, pNandDrv->tMinDDR.u8_DelayCell,
				pNandDrv->tMinDDR.u8_DdrTiming);			
	}
	pNandDrv->u32_minClk = au32_ClkTable[pNandDrv->tMinDDR.u8_ClkIdx];
	
	/**************** search default ddr timing *********************/
	nand_debug(UNFD_DEBUG_LEVEL, 0,"*** Try to detect default ddr timing ***\r\n");
	nand_printf("*** Try to detect default ddr timing ***\r\n");
	u32_Err = nand_find_timing(&u8_clk, 1);

	if(u32_Err != UNFD_ST_SUCCESS)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err, NAND, Cannot config nand timing\n");
		nand_die();
		return(u32_Err);
	}
	#if IF_IP_VERIFY//owen-cl
    u8_clk =au8_nand_clk_max;
	#endif
    for( ; u8_clk>0 ; u8_clk--) // search from high-speed clk
	{
		u8_DqsMatch = 0;
		pNandDrv->u32_Clk = au32_ClkTable[u8_clk];
		nand_debug(UNFD_DEBUG_LEVEL, 0," **********************************************\r\n");
	    nand_debug(UNFD_DEBUG_LEVEL, 0," CLK[%X]\r\n", au32_ClkTable[u8_clk]);
		nand_debug(UNFD_DEBUG_LEVEL, 0," **********************************************\r\n");		
		nand_clock_setting(pNandDrv->u32_Clk);
		if(nand_config_timing(au32_1TTable[u8_clk]) != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL, 0,"Cannot match nand timing @clk=%X, DQS combination=%X\r\n", pNandDrv->u32_Clk);		
			continue;
		}
	    u8_max_delay_cell=0x1;
		for(u8_2Ch_dqsmode=0 ; u8_2Ch_dqsmode<DQS_MODE_TABLE_CNT ; u8_2Ch_dqsmode++)
		{
        	u8_pass_cnt = 0;
            for(u8_delaycell=0; u8_delaycell<u8_max_delay_cell; u8_delaycell++)//0x20; u8_delaycell++)
            {
        		for(u8_57h=3 ; u8_57h<0x10 ; u8_57h++) // ddr timing shouldn't match from 0~2
        		{
        			NC_FCIE4SetInterface(1, au8_dqsmode[u8_2Ch_dqsmode], u8_delaycell, u8_57h);

        			if(UNFD_ST_SUCCESS == nand_probe_ddr())
        			{
        				nand_debug(UNFD_DEBUG_LEVEL, 0,"clk[%X], dqsmode[%u], DelayCell[%u], ddr_timing[%u]\r\n",
        							pNandDrv->u32_Clk, au8_dqsmode[u8_2Ch_dqsmode], u8_delaycell,
        							u8_57h);				
        				if(u8_pass_cnt == 0)
        					u8_57h_start = u8_57h;
        
        				if((++u8_pass_cnt)==MIN_PASS_CNT)
        				{
        					ptDDR[u8_2Ch_dqsmode].u8_ClkIdx = u8_clk;
        					ptDDR[u8_2Ch_dqsmode].u8_DqsMode = au8_dqsmode[u8_2Ch_dqsmode];
        					ptDDR[u8_2Ch_dqsmode].u8_DelayCell = u8_delaycell;
        					ptDDR[u8_2Ch_dqsmode].u8_DdrTiming = u8_57h_start+1;
        					u8_DqsMatch |= (1<<u8_2Ch_dqsmode);
        					break;
        				}
        			}
        			else
        			{
        				if(u8_pass_cnt != 0)
        					break;
        			}		
        		}
            }
		}
        gu8_unfd_disable_uart=0;
		u8_2Ch_dqsmode = 0;
		switch(u8_DqsMatch)
		{
			case 0x0:
			case 0x8:
			case 0x4:
			case 0xA:
			case 0x1:
			case 0x9:
			case 0x5:
			case 0xD:
			case 0xB:
				nand_debug(UNFD_DEBUG_LEVEL, 0,"Cannot find ddr timing @clk=%X, DQS combination=%X\r\n", pNandDrv->u32_Clk, u8_DqsMatch);			
				break;
				
			case 0x2:
			case 0x3:			
				if(nand_probe_ddr_ex(aau8_half_delay_cell[u8_clk][1], 1) == UNFD_ST_SUCCESS)
					u8_2Ch_dqsmode = 1;	
				break;	
				
			case 0xC: // 2.5T
				u8_2Ch_dqsmode = 3;
				break;
			case 0x6: // 1.5T
				u8_2Ch_dqsmode = 1;
				break;
			case 0xE: // 2T
				u8_2Ch_dqsmode = 2;
				break;			
			case 0x7: // 1.5T
				u8_2Ch_dqsmode = 1;
				break;
			case 0xF: // 2T
				u8_2Ch_dqsmode = 2;
				break;
		}
        #if 0
		pNandDrv->tDefaultDDR.u8_ClkIdx = ptDDR[u8_2Ch_dqsmode].u8_ClkIdx;
		pNandDrv->tDefaultDDR.u8_DqsMode = ptDDR[u8_2Ch_dqsmode].u8_DqsMode;
		pNandDrv->tDefaultDDR.u8_DelayCell = ptDDR[u8_2Ch_dqsmode].u8_DelayCell;
		pNandDrv->tDefaultDDR.u8_DdrTiming = ptDDR[u8_2Ch_dqsmode].u8_DdrTiming;
		nand_debug(UNFD_DEBUG_LEVEL,1,"ok, get default DDR timing: clk[%X], dqsmode[%u], DelayCell[%u], ddr_timing[%u]\r\n",
				pNandDrv->tDefaultDDR.u8_ClkIdx, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell,
				pNandDrv->tDefaultDDR.u8_DdrTiming);	
		break;
		#else
		if(u8_2Ch_dqsmode != 0)
		{
			pNandDrv->tDefaultDDR.u8_ClkIdx = ptDDR[u8_2Ch_dqsmode].u8_ClkIdx;
			pNandDrv->tDefaultDDR.u8_DqsMode = ptDDR[u8_2Ch_dqsmode].u8_DqsMode;
			pNandDrv->tDefaultDDR.u8_DelayCell = ptDDR[u8_2Ch_dqsmode].u8_DelayCell;
			pNandDrv->tDefaultDDR.u8_DdrTiming = ptDDR[u8_2Ch_dqsmode].u8_DdrTiming;
			nand_printf("ok, get default DDR timing: clk[%X], dqsmode[%u], DelayCell[%u], ddr_timing[%u]\r\n",
					pNandDrv->tDefaultDDR.u8_ClkIdx, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell,
					pNandDrv->tDefaultDDR.u8_DdrTiming);	
			//nand_debug(UNFD_DEBUG_LEVEL,1,"ok, get default DDR timing: clk[%X], dqsmode[%u], DelayCell[%u], ddr_timing[%u]\r\n",
			//		pNandDrv->tDefaultDDR.u8_ClkIdx, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell,
			//		pNandDrv->tDefaultDDR.u8_DdrTiming);	
			break;
		}
		#endif

	}

	if(u8_clk == 0)
	{
		//nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Warning, NAND, can't detect higher default timing, set min as default\n");
		nand_printf("Warning, NAND, can't detect higher default timing, set min as default\n");
		pNandDrv->tDefaultDDR.u8_ClkIdx = pNandDrv->tMinDDR.u8_ClkIdx;
		pNandDrv->tDefaultDDR.u8_DqsMode = pNandDrv->tMinDDR.u8_DqsMode;
		pNandDrv->tDefaultDDR.u8_DelayCell = pNandDrv->tMinDDR.u8_DelayCell;
		pNandDrv->tDefaultDDR.u8_DdrTiming = pNandDrv->tMinDDR.u8_DdrTiming;
		u8_clk_default = 1;
	}
	else
	{
		u8_clk_default = u8_clk;
		nand_debug(UNFD_DEBUG_LEVEL, 0,"u8_clk_default:%x\r\n",u8_clk_default);
	}

	/**************** search max ddr timing *********************/
	// TODO
	/************************************************************/

	pNandDrv->u32_Clk = au32_ClkTable[pNandDrv->tDefaultDDR.u8_ClkIdx];
	nand_clock_setting(pNandDrv->u32_Clk);
	nand_config_timing(au32_1TTable[pNandDrv->tDefaultDDR.u8_ClkIdx]);
	NC_FCIE4SetInterface(1, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell, pNandDrv->tDefaultDDR.u8_DdrTiming);

	NC_EraseBlk(u32_TestBlk*pNandDrv->u16_BlkPageCnt);
	//free(ptDDR);
	//free(au16_WritePageBuf);
	//free(au16_ReadPageBuf);
	
	return UNFD_ST_SUCCESS;
}
#endif

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
#if defined(DECIDE_CLOCK_BY_NAND) && DECIDE_CLOCK_BY_NAND
	NAND_DRIVER * pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err = 0;
	U32 au32_ClkTable[NFIE_CLK_TABLE_CNT] = NFIE_CLK_TABLE;
	char *ClkStrTable[NFIE_CLK_TABLE_CNT] = NFIE_CLK_TABLE_STR;

	#if defined(FCIE4_DDR) && FCIE4_DDR
	U32 au32_4ClkTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_TABLE;
	char *Clk4StrTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_TABLE_STR;
	U32 au32_1TTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_1T_TABLE; 	
	#endif

	U8 u8_ClkIdx = 0;

	#if defined(FCIE4_DDR) && FCIE4_DDR
	if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_MASM)
	{
		if(pNandDrv->tDefaultDDR.u8_DdrTiming == 0)
		{
			//nand_detect_ddr_timing();
			nand_printf("CIS pNandDrv->tDefaultDDR.u8_DdrTiming is 0\r\n");
			while(1);
		}
		else
		{
			pNandDrv->u32_minClk = au32_4ClkTable[pNandDrv->tMinDDR.u8_ClkIdx];			
			pNandDrv->u32_Clk = au32_4ClkTable[pNandDrv->tDefaultDDR.u8_ClkIdx];
			NC_FCIE4SetInterface(1, pNandDrv->tDefaultDDR.u8_DqsMode, 
				pNandDrv->tDefaultDDR.u8_DelayCell, pNandDrv->tDefaultDDR.u8_DdrTiming);

			if(nand_config_timing(au32_1TTable[pNandDrv->tDefaultDDR.u8_ClkIdx]) != UNFD_ST_SUCCESS)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err, NAND, Cannot config nand timing\n");
				nand_die();
				return(u32_Err);
			}			
		}
		nand_debug(UNFD_DEBUG_LEVEL,1,"ok, get default DDR timing: 2Ch:%X, 57h:%X\n", 
					pNandDrv->u16_Reg2C_SMStatus, pNandDrv->u16_Reg57_RELatch);	
		u8_ClkIdx = pNandDrv->tDefaultDDR.u8_ClkIdx;
		nand_printf("FCIE is set to %sHz\n", Clk4StrTable[u8_ClkIdx]);
	}
	else
	#endif
	{
		
		u32_Err = nand_find_timing(&u8_ClkIdx, 0);   
		if(u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err, NAND, Cannot config nand timing\n");
			nand_die();
			return(u32_Err);
		}
		pNandDrv->u32_Clk = au32_ClkTable[u8_ClkIdx];
		nand_printf("FCIE is set to %sHz\n", ClkStrTable[u8_ClkIdx]);
	}

	nand_clock_setting(pNandDrv->u32_Clk);
	REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);
	//nand_debug(UNFD_DEBUG_LEVEL, 0,"reg_ckg_fcie(%08X)=%08X\n", reg_ckg_fcie, REG(reg_ckg_fcie));
	nand_printf("RE LATCH is set to %X\n", pNandDrv->u16_Reg57_RELatch);
	
#endif

  return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// do nothing in Linux
}

void nand_reset_WatchDog(void)
{
	// do nothing in Linux
}

extern void hal_dcache_flush(void *base , u32 asize);
//extern void Chip_Flush_Memory(void);
U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt, int mode)
{
		//extern void ___dma_single_cpu_to_dev(const void *kaddr, size_t size, enum dma_data_direction dir);
		extern void Chip_Clean_Cache_Range_VA_PA(unsigned long u32VAddr,unsigned long u32PAddr,unsigned long u32Size);
		extern void Chip_Flush_Cache_Range_VA_PA(unsigned long u32VAddr,unsigned long u32PAddr,unsigned long u32Size);
    //mode 0 for write, 1 for read
    if( mode == WRITE_TO_NAND )	//Write
    {
        //Write (DRAM->NAND)-> flush
        //Chip_Clean_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), u32_ByteCnt);
        //printk("mode=0:u32_ByteCnt=%d\n",u32_ByteCnt);
        //Chip_Clean_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), 1024*2);
        Chip_Clean_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), u32_ByteCnt);
        //Chip_Clean_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), 1024*1024);
        //Chip_Clean_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), 1024*4);
        //Chip_Clean_Cache_Range_VA_PA(nand_mtd,__pa(nand_mtd),1024*1024);
        //Chip_Flush_Cache_All();
        //printk("delay test\n");
        //mdelay(1);
    }
    else //Read
    {
        //Read (NAND->DRAM) -> inv
        //Chip_Inv_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr),u32_ByteCnt);
        //Chip_Inv_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr),1024*2);
        //Chip_Inv_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr),1024*4);
        //Chip_Inv_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr),1024*1024);
        //Chip_Inv_Cache_Range_VA_PA(nand_mtd,__pa(nand_mtd),1024*1024);
        //Chip_Flush_Cache_All();
        //printk("mode=1:u32_ByteCnt=%d\n",u32_ByteCnt);
        //Chip_Flush_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), 1024*2);
        Chip_Flush_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), u32_ByteCnt);
        //printk("delay test\n");
        //mdelay(1);
    }
    /*
	if(virt_to_phys((void *)u32_DMAAddr) >= MSTAR_MIU1_BUS_BASE) 
	{
    	REG_SET_BITS_UINT16( NC_MIU_DMA_SEL, BIT_MIU1_SELECT);
    }
    else
    	REG_CLR_BITS_UINT16( NC_MIU_DMA_SEL, BIT_MIU1_SELECT);
		*/

	return virt_to_phys((void *)u32_DMAAddr);
		
		
		
}


#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);
U16 u16_FcieMieEvent = 0;

irqreturn_t NC_FCIE_IRQ(int irq, void *dummy)
{
	U16 u16_Reg;

	u16_Reg = REG(NC_PATH_CTL); 
	if((u16_Reg&BIT_NC_EN) != BIT_NC_EN)
	{
		//nand_debug(UNFD_DEBUG_LEVEL, 1 ,"not nand interrupt\r\n");
		// not NAND interrupt
		return IRQ_NONE;
	}

	u16_Reg = REG(NC_MIE_EVENT);
	//nand_debug(UNFD_DEBUG_LEVEL, 1 ,"NC_MIE_EVENT:%X\r\n",u16_Reg);	
	u16_FcieMieEvent |= u16_Reg;
	u16_Reg &= REG(NC_MIE_INT_EN);	
	REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_Reg); /*clear events*/
	wake_up(&fcie_wait);
	return IRQ_HANDLED;
}

U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	U16 u16_Reg;
		
	wait_event_timeout(fcie_wait, ((u16_FcieMieEvent&u16_WaitEvent) == u16_WaitEvent), usecs_to_jiffies(u32_MicroSec));
	if((u16_FcieMieEvent&u16_WaitEvent) != u16_WaitEvent)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "u16_FcieMieEvent: %X\r\n", u16_FcieMieEvent);
	
		return UNFD_ST_ERR_E_TIMEOUT;
	}

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_WaitEvent); /*clear events*/
	return UNFD_ST_SUCCESS;
}

//xxx tomodify
void nand_enable_intr_mode(void)
{
	u16_FcieMieEvent = 0;
	REG_WRITE_UINT16(NC_MIE_INT_EN, BIT_NC_JOB_END);
}
#endif

U32 NC_PlatformResetPre(void)
{
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	pNandDrv=pNandDrv;
    REG_WRITE_UINT16(NC_PATH_CTL, 0x20);

	#if defined(DUTY_CYCLE_PATCH)&&DUTY_CYCLE_PATCH
	if( pNandDrv->u32_Clk == NFIE_CLK_80M )
		REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);	// duty cycle 3:1 in 86Mhz
	#endif
	return UNFD_ST_SUCCESS;
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}
EXPORT_SYMBOL(drvNAND_get_DrvContext_address);
void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

PAIRED_PAGE_MAP_t ga_tPairedPageMap[512] = {{0,0}};   

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[512/4] UNFD_ALIGN1; /*   512 Bytes */

U32 NC_PlatformInit(void)
{
  NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
  REG_WRITE_UINT16(NC_PATH_CTL, 0x20);

  nand_pads_init();
  pNandDrv->u8_WordMode = 0; // TV/Set-Top Box projects did not support x16 NAND flash
  nand_pads_switch(1);
  
  #if defined (DECIDE_CLOCK_BY_NAND) && DECIDE_CLOCK_BY_NAND
  pNandDrv->u32_Clk =FCIE3_SW_SLOWEST_CLK;
  nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
  #else
  pNandDrv->u32_Clk =FCIE3_SW_DEFAULT_CLK;
  nand_clock_setting(FCIE3_SW_DEFAULT_CLK);
  #endif
  // print clock setting
  //nand_debug(UNFD_DEBUG_LEVEL, 0,"reg_ckg_fcie(%08X)=%08X\n", reg_ckg_fcie, REG(reg_ckg_fcie));

  pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
  pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;
  // no shared-bus with Disp
  pNandDrv->u8_SwPatchWaitRb= 0;
  pNandDrv->u8_SwPatchJobStart= 0;
  NC_ConfigHal(NAND_HAL_RAW);

  return UNFD_ST_SUCCESS;
}


#elif (defined(NAND_DRV_INFINITY_LINUX) && NAND_DRV_INFINITY_LINUX)

#if defined(CONFIG_OF)
struct clk_data{
	int num_parents;
	struct clk **clk_fcie;
//	struct clk *clk_ecc;
};
extern struct clk_data *clkdata;
#endif

/* return Timer tick */
#include <linux/delay.h>
U32  nand_hw_timer_delay(U32 u32usTick)
{
  #if 1	//xxx tomodify
  udelay(u32usTick);
  #else
	volatile U32 i = 0;

	for (i = 0; i < (u32usTick>>4); i++)
	{
		#if 0
		volatile int j = 0, tmp;
		for (j = 0; j < 0; j++)
		{
			tmp = j;
		}
		#endif
	}
  #endif

    return u32usTick + 1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

    pNandDrv->u16_Reg58_DDRCtrl &= ~(BIT_DDR_MASM|BIT_SDR_DIN_FROM_MACRO|BIT_NC_32B_MODE);

	if(u32EnableFCIE)
	{
		REG_CLR_BITS_UINT16(REG_ALLPAD_IN, BIT15);
	    REG_SET_BITS_UINT16(REG_NAND_MODE, NAND_MODE1);	//set nand mode
		REG_CLR_BITS_UINT16(REG_SD_MODE, REG_SD_MODE_MASK);

		REG_WRITE_UINT16(NC_DDR_CTRL, pNandDrv->u16_Reg58_DDRCtrl);
		REG_SET_BITS_UINT16(REG_CTRL_PE, 0x1F);

	}
	return 0;
}

U32 nand_clock_setting(U32 u32ClkParam)
{
#if defined(CONFIG_OF)
	U32 u32_clkrate = 0;
	clk_set_rate(clkdata->clk_fcie[1], 160*1000*1000);	//set ecc to 160M

	switch(u32ClkParam)
	{
		case NFIE_CLK_300K:
			u32_clkrate = 300*1000;
			break;
		case NFIE_CLK_5_4M:
			u32_clkrate = 5375*1000;
			break;
		case NFIE_CLK_12M:
			u32_clkrate = 12*1000*1000;
			break;
		case NFIE_CLK_20M:
			u32_clkrate = 20*1000*1000;
			break;
		case NFIE_CLK_32M:
			u32_clkrate = 32*1000*1000;
			break;
		case NFIE_CLK_36M:
			u32_clkrate = 36*1000*1000;
			break;
		case NFIE_CLK_40M:
			u32_clkrate = 40*1000*1000;
			break;
		case NFIE_CLK_43_2M:
			u32_clkrate = 43*1000*1000;
			break;
		case NFIE_CLK_48M:
			u32_clkrate = 48*1000*1000;
			break;
		case NFIE_CLK_54M:
			u32_clkrate = 54*1000*1000;
			break;
		case NFIE_CLK_62M:
			u32_clkrate = 61500*1000;
			break;
		case NFIE_CLK_72M:
			u32_clkrate = 72*1000*1000;
			break;
		case NFIE_CLK_86M:
			u32_clkrate = 86*1000*1000;
			break;
		default:
			nand_die();
			break;
	}
	//printk("set clock %d\n", u32_clkrate);
	clk_set_rate(clkdata->clk_fcie[0], u32_clkrate);
#else
    REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT0|BIT1);
    REG_CLR_BITS_UINT16(reg_ckg_fcie, NFIE_CLK_MASK);
    REG_SET_BITS_UINT16(reg_ckg_fcie, u32ClkParam);

	//ecc clock
	REG_CLR_BITS_UINT16(reg_ckg_ecc, BIT0|BIT1);
    REG_CLR_BITS_UINT16(reg_ckg_ecc, BIT2|BIT3|BIT4);	//set to 160M
    REG_SET_BITS_UINT16(reg_ckg_ecc, BIT2|BIT3);	//set to 12M
#endif
	return UNFD_ST_SUCCESS;
}

void nand_DumpPadClk(void)
{
	nand_debug(0, 1, "clk setting: \n");
	nand_debug(0, 1, "reg_ckg_fcie(0x%X):0x%x\n", reg_ckg_fcie, REG(reg_ckg_fcie));

	nand_debug(0, 1, "pad setting: \n");
	//fcie pad register
	nand_debug(0, 1, "NC_DDR_CTRL(0x%X):0x%x\n", NC_DDR_CTRL, REG(NC_DDR_CTRL));
	//chiptop pad register
	nand_debug(0, 1, "reg_all_pad_in(0x%X):0x%x\n", REG_ALLPAD_IN, REG(REG_ALLPAD_IN));
	//platform depend reg_sd_use_bypass
	nand_debug(0, 1, "REG_NAND_MODE(0x%X):0x%x\n", REG_NAND_MODE, REG(REG_NAND_MODE));
}


#if defined(DECIDE_CLOCK_BY_NAND) && DECIDE_CLOCK_BY_NAND
#define MAX(a,b) ((a) > (b) ? (a) : (b))

U32 nand_config_timing(U16 u16_1T)
{
	#if 1	// defined(FCIE4_DDR) && FCIE4_DDR
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U16 u16_DefaultTRR;
	U16 u16_DefaultTCS;
	U16 u16_DefaultTWW;
	U16 u16_DefaultRX40Cmd;
	U16 u16_DefaultRX40Adr;
	U16 u16_DefaultRX56;
	U16 u16_DefaultTADL;
	U16 u16_DefaultTCWAW;
	U16 u16_DefaultTCLHZ = 0;
	U16 u16_DefaultTWHR;
	U16 u16_Tmp, u16_Cnt;
	U16 u16_Tmp2, u16_Cnt2;


	if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_ONFI)
	{
		u16_DefaultTRR = NC_ONFI_DEFAULT_TRR;
		u16_DefaultTCS = NC_ONFI_DEFAULT_TCS;
		u16_DefaultTWW = NC_ONFI_DEFAULT_TWW;
		u16_DefaultRX40Cmd = NC_ONFI_DEFAULT_RX40CMD;
		u16_DefaultRX40Adr = NC_ONFI_DEFAULT_RX40ADR;
		u16_DefaultRX56 = NC_ONFI_DEFAULT_RX56;
		u16_DefaultTADL = NC_ONFI_DEFAULT_TADL;
		u16_DefaultTCWAW = NC_ONFI_DEFAULT_TCWAW;
		u16_DefaultTWHR = NC_ONFI_DEFAULT_TWHR;
	}
	else if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_TOGGLE)
	{
		u16_DefaultTRR = NC_TOGGLE_DEFAULT_TRR;
		u16_DefaultTCS = NC_TOGGLE_DEFAULT_TCS;
		u16_DefaultTWW = NC_TOGGLE_DEFAULT_TWW;
		u16_DefaultRX40Cmd = NC_TOGGLE_DEFAULT_RX40CMD;
		u16_DefaultRX40Adr = NC_TOGGLE_DEFAULT_RX40ADR;
		u16_DefaultRX56 = NC_TOGGLE_DEFAULT_RX56;
		u16_DefaultTADL = NC_TOGGLE_DEFAULT_TADL;
		u16_DefaultTCWAW = NC_TOGGLE_DEFAULT_TCWAW;
		u16_DefaultTWHR = NC_TOGGLE_DEFAULT_TWHR;
	}
	else

	{
		u16_DefaultTRR = NC_SDR_DEFAULT_TRR;
		u16_DefaultTCS = NC_SDR_DEFAULT_TCS;
		u16_DefaultTWW = NC_SDR_DEFAULT_TWW;
		u16_DefaultRX40Cmd = NC_SDR_DEFAULT_RX40CMD;
		u16_DefaultRX40Adr = NC_SDR_DEFAULT_RX40ADR;
		u16_DefaultRX56 = NC_SDR_DEFAULT_RX56;
		u16_DefaultTADL = NC_SDR_DEFAULT_TADL;
		u16_DefaultTCWAW = NC_SDR_DEFAULT_TCWAW;
		u16_DefaultTWHR = NC_SDR_DEFAULT_TWHR;
	}
	#endif

	#if defined(NC_INST_DELAY) && NC_INST_DELAY
	// Check CMD_END
	u16_Tmp = MAX(pNandDrv->u16_tWHR, pNandDrv->u16_tCWAW);
	u16_Cnt = (u16_Tmp+u16_1T-1)/u16_1T;

	if(u16_DefaultRX40Cmd >= u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultRX40Cmd > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultRX40Cmd;

	// Check ADR_END
	u16_Tmp2 = MAX(MAX(pNandDrv->u16_tWHR, pNandDrv->u16_tADL), pNandDrv->u16_tCCS);
	u16_Cnt2 = (u16_Tmp2+u16_1T-1)/u16_1T;

	if(u16_DefaultRX40Adr >= u16_Cnt2)
		u16_Cnt2 = 0;
	else if(u16_Cnt2-u16_DefaultRX40Adr > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt2 -= u16_DefaultRX40Adr;

	// get the max cnt
	u16_Cnt = MAX(u16_Cnt, u16_Cnt2);

	pNandDrv->u16_Reg40_Signal &= ~(0x00FF<<8);
	pNandDrv->u16_Reg40_Signal |= (u16_Cnt<<8);
	nand_debug(UNFD_DEBUG_LEVEL_LOW,1, "u16_Reg40_Signal =  %X\n",pNandDrv->u16_Reg40_Signal);
	#endif

	#if defined(NC_HWCMD_DELAY) && NC_HWCMD_DELAY
	u16_Cnt = (pNandDrv->u16_tRHW+u16_1T-1)/u16_1T + 2;

	if(u16_DefaultRX56 >= u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultRX56 > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultRX56;

	pNandDrv->u16_Reg56_Rand_W_Cmd &= ~(0x00FF<<8);
	pNandDrv->u16_Reg56_Rand_W_Cmd |= (u16_Cnt<<8);
	nand_debug(UNFD_DEBUG_LEVEL_LOW,1, "u16_Reg56_Rand_W_Cmd =  %X\n",pNandDrv->u16_Reg56_Rand_W_Cmd);
	#endif

	#if defined(NC_TRR_TCS) && NC_TRR_TCS
	u16_Cnt = (pNandDrv->u8_tRR+u16_1T-1)/u16_1T + 2;

	if(u16_DefaultTRR >= u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultTRR > 0x0F)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultTRR;

	u16_Tmp = (pNandDrv->u8_tCS+u16_1T-1)/u16_1T + 2;

	if(u16_DefaultTCS >= u16_Tmp)
		u16_Tmp = 0;
	else if(u16_Tmp-u16_DefaultTCS > 0x0F)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Tmp -= u16_DefaultTCS;

	u16_Tmp2 = (pNandDrv->u16_tWW+u16_1T-1)/u16_1T + 2;

    if(u16_DefaultTWW >= u16_Tmp2)
        u16_Tmp2 = 0;
    else if(u16_Tmp2-u16_DefaultTWW > 0x0F)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
        u16_Tmp2 -= u16_DefaultTWW;

	u16_Cnt2 = MAX(u16_Tmp, u16_Tmp2);

	pNandDrv->u16_Reg59_LFSRCtrl &= ~(0x00FF);
	pNandDrv->u16_Reg59_LFSRCtrl |= (u16_Cnt|(u16_Cnt2<<4));
	nand_debug(UNFD_DEBUG_LEVEL_LOW,1, "u16_Reg59_LFSRCtrl =  %X\n",pNandDrv->u16_Reg59_LFSRCtrl);
	#endif

	#if defined(NC_TCWAW_TADL) && NC_TCWAW_TADL
	u16_Cnt = (pNandDrv->u16_tADL + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTADL > u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultTADL > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultTADL;

	u16_Cnt2 = (pNandDrv->u16_tCWAW + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTADL > u16_Cnt2)
		u16_Cnt2 = 0;
	else if(u16_Cnt2-u16_DefaultTCWAW > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt2 -= u16_DefaultTCWAW;

	pNandDrv->u16_Reg5D_tCWAW_tADL &= ~(0xFFFF);
	pNandDrv->u16_Reg5D_tCWAW_tADL |= (u16_Cnt|(u16_Cnt2<<8));
	nand_debug(UNFD_DEBUG_LEVEL_LOW,1, "u16_Reg5D_tCWAW_tADL =  %X\n",pNandDrv->u16_Reg5D_tCWAW_tADL);
	#endif

	#if defined(NC_TWHR_TCLHZ) && NC_TWHR_TCLHZ
	u16_Cnt = (pNandDrv->u8_tCLHZ + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTCLHZ > u16_Cnt)
		u16_Cnt = 0;
	else if(u16_Cnt-u16_DefaultTCLHZ > 0xF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt -= u16_DefaultTCLHZ;

	u16_Cnt2 = (pNandDrv->u16_tWHR + u16_1T - 1) / u16_1T + 2;

	if(u16_DefaultTWHR > u16_Cnt2)
		u16_Cnt2 = 0;
	else if(u16_Cnt2-u16_DefaultTWHR > 0xFF)
		return UNFD_ST_ERR_INVALID_PARAM;
	else
		u16_Cnt2 -= u16_DefaultTWHR;

	pNandDrv->u16_Reg5A_tWHR_tCLHZ &= ~(0xFFFF);
	pNandDrv->u16_Reg5A_tWHR_tCLHZ |= ((u16_Cnt&0xF)|(u16_Cnt2<<8));
	nand_debug(UNFD_DEBUG_LEVEL_LOW,1, "u16_Reg5A_tWHR_tCLHZ =  %X\n",pNandDrv->u16_Reg5A_tWHR_tCLHZ);
	#endif

	NC_Config();
	return UNFD_ST_SUCCESS;
}

U32 nand_find_timing(U8 *pu8_ClkIdx, U8 u8_find_DDR_timg)
{
    NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
    U32 au32_1TTable[NFIE_CLK_TABLE_CNT] = NFIE_1T_TABLE;
    U32 au32_ClkValueTable[NFIE_CLK_TABLE_CNT] = NFIE_CLK_VALUE_TABLE;

    #if defined(FCIE4_DDR) && FCIE4_DDR || defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
    U32 au32_4Clk1TTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_1T_TABLE;
    U32 au32_4ClkValueTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_VALUE_TABLE;
    #endif
    #if defined(NC_SEL_FCIE5) && NC_SEL_FCIE5
    U16 u16_i, u16_j, u16_pass_begin = 0xFF, u16_pass_cnt = 0;
    U16 u16_nandstatus;
    U32 u32_Err;
    U8 au8_ID[NAND_ID_BYTE_CNT];
    U32 au32_ClkTable[NFIE_CLK_TABLE_CNT] = NFIE_CLK_TABLE;
    #endif
    U32 u32_Clk;
    U16 u16_SeqAccessTime, u16_Tmp, u16_Tmp2, u16_1T;
    S8 s8_ClkIdx;
    #if defined(NC_SEL_FCIE3) && NC_SEL_FCIE3
    U16 u16_RE_LATCH_DELAY = 0;
    #endif
    s8_ClkIdx = 0;
    u16_1T = 0;

    if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_ONFI)
    {
        u16_SeqAccessTime = 10;
    }
    else if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_TOGGLE)
    {
        u16_Tmp = MAX(MAX(2*pNandDrv->u8_tRP, 2*pNandDrv->u8_tREH), pNandDrv->u16_tRC);
        u16_Tmp2 = MAX(MAX(pNandDrv->u8_tWP, pNandDrv->u8_tWH), (pNandDrv->u16_tWC+1)/2);
        u16_SeqAccessTime = MAX(u16_Tmp, u16_Tmp2);
    }
    else
    {
        u16_Tmp = MAX(MAX(pNandDrv->u8_tRP, pNandDrv->u8_tREH), (pNandDrv->u16_tRC+1)/2);
        u16_Tmp2 = MAX(MAX(pNandDrv->u8_tWP, pNandDrv->u8_tWH), (pNandDrv->u16_tWC+1)/2);
        u16_SeqAccessTime = MAX(u16_Tmp, u16_Tmp2);

        u16_Tmp = (pNandDrv->u8_tREA + NAND_SEQ_ACC_TIME_TOL)/2;
        u16_Tmp2 = u16_SeqAccessTime;
        u16_SeqAccessTime = MAX(u16_Tmp, u16_Tmp2);
    }

    u32_Clk = 1000000000/((U32)u16_SeqAccessTime); //FCIE5 needs to be update .....

    if(!u8_find_DDR_timg)
    {
        for(s8_ClkIdx =  0; s8_ClkIdx <= NFIE_CLK_TABLE_CNT - 1; s8_ClkIdx ++)
        {
            if(u32_Clk <= au32_ClkValueTable[s8_ClkIdx])
            {
                break;
            }
        }
    }
    else
    {
        #if defined(FCIE4_DDR) && FCIE4_DDR || defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
        for(s8_ClkIdx =  0; s8_ClkIdx <= NFIE_4CLK_TABLE_CNT - 1; s8_ClkIdx ++)
        {
            if(u32_Clk < au32_4ClkValueTable[s8_ClkIdx])
            {
                break;
            }
        }
        #endif
    }
    s8_ClkIdx --;


RETRY:
    if(s8_ClkIdx<0)
        return UNFD_ST_ERR_INVALID_PARAM;
    if(!u8_find_DDR_timg)
        u16_1T = au32_1TTable[s8_ClkIdx];
    #if defined(FCIE4_DDR) && FCIE4_DDR || defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
    else
        u16_1T = au32_4Clk1TTable[s8_ClkIdx];
    #endif

    if(nand_config_timing(u16_1T) != UNFD_ST_SUCCESS)
    {
        s8_ClkIdx--;
        goto RETRY;
    }

    #if defined(NC_SEL_FCIE3) && NC_SEL_FCIE3
    u16_RE_LATCH_DELAY = BIT_NC_LATCH_DATA_1_0_T;

    pNandDrv->u16_Reg57_RELatch &= ~BIT_NC_LATCH_DATA_MASK;
    pNandDrv->u16_Reg57_RELatch |= u16_RE_LATCH_DELAY;
    #elif defined(NC_SEL_FCIE5) && NC_SEL_FCIE5
    if (!u8_find_DDR_timg)
    {
        NC_ReadStatus();
        REG_READ_UINT16(NC_ST_READ, u16_nandstatus);

        pNandDrv->u32_Clk = au32_ClkTable[s8_ClkIdx];
        nand_clock_setting(pNandDrv->u32_Clk);

        //using read id to detect relatch
        memcpy(au8_ID, pNandDrv->au8_ID, NAND_ID_BYTE_CNT);
        for(u16_i = 0 ; u16_i < 16 ; u16_i ++)
        {

            pNandDrv->u16_Reg57_RELatch &= ~(BIT_NC_LATCH_DATA_MASK|BIT_NC_PAD_SEL_FAILLING);
            if(u16_i &1)    //select falling edge otherwise rising edge is selected
                pNandDrv->u16_Reg57_RELatch|=BIT_NC_PAD_SEL_FAILLING;
            pNandDrv->u16_Reg57_RELatch |= ((u16_i/2) << 1) &BIT_NC_LATCH_DATA_MASK;

            NC_Config();
            u32_Err = NC_ReadID();
            if(u32_Err != UNFD_ST_SUCCESS)
            {
                nand_debug(0, 1, "ReadID Error with ErrCode 0x%X\n", u32_Err);
                nand_die();
            }
            for(u16_j = 0; u16_j < NAND_ID_BYTE_CNT; u16_j++)
            {
                if(pNandDrv->au8_ID[u16_j] != au8_ID[u16_j])
                {
                    break;
                }
            }
            if(u16_j == NAND_ID_BYTE_CNT)
            {
                if(u16_pass_begin == 0xFF)
                    u16_pass_begin = u16_i;
                u16_pass_cnt ++;
            }
            //  break;
        }

        if(u16_pass_cnt == 0)
        {
            nand_debug(0, 1, "Read ID detect timing fails\n");
            pNandDrv->u16_Reg57_RELatch = BIT_NC_LATCH_DATA_2_0_T|BIT_NC_LATCH_STS_2_0_T;
            NC_Config();
            s8_ClkIdx = 0;
            *pu8_ClkIdx = (U8)s8_ClkIdx;
            memcpy(pNandDrv->au8_ID, au8_ID, NAND_ID_BYTE_CNT);
            return UNFD_ST_SUCCESS;
        }
        else
        {
            u16_i = u16_pass_begin + u16_pass_cnt/2;
            pNandDrv->u16_Reg57_RELatch &= ~(BIT_NC_LATCH_DATA_MASK|BIT0);
            if(u16_i &1)    //select falling edge otherwise rising edge is selected
                pNandDrv->u16_Reg57_RELatch|=BIT0;
            pNandDrv->u16_Reg57_RELatch |= ((u16_i/2) << 1) &BIT_NC_LATCH_DATA_MASK;
            memcpy(pNandDrv->au8_ID, au8_ID, NAND_ID_BYTE_CNT);
        }

        //detect read status

        u16_pass_begin = 0xFF;
        u16_pass_cnt = 0;

        for(u16_i = 0 ; u16_i < 8 ; u16_i ++)
        {
            U16 u16_tmpStatus;
            pNandDrv->u16_Reg57_RELatch &= ~(BIT_NC_LATCH_STS_MASK);
            pNandDrv->u16_Reg57_RELatch |= ((u16_i) << 5) & BIT_NC_LATCH_STS_MASK;

            NC_Config();
            NC_ReadStatus();
            REG_READ_UINT16(NC_ST_READ, u16_tmpStatus);

            if(u16_tmpStatus == u16_nandstatus)
            {
                if(u16_pass_begin == 0xFF)
                    u16_pass_begin = u16_i;
                u16_pass_cnt ++;
            }
        }
        if(u16_pass_cnt == 0)
        {
            nand_debug(0, 1, "Read status detect timing fails\n");
            pNandDrv->u16_Reg57_RELatch = BIT_NC_LATCH_DATA_2_0_T|BIT_NC_LATCH_STS_2_0_T;
            NC_Config();
            s8_ClkIdx = 0;
            *pu8_ClkIdx = (U8)s8_ClkIdx;

            return UNFD_ST_SUCCESS;
        }
        else
        {
            u16_i = u16_pass_begin + u16_pass_cnt/2;
            pNandDrv->u16_Reg57_RELatch &= ~(BIT_NC_LATCH_STS_MASK);
            pNandDrv->u16_Reg57_RELatch |= ((u16_i) << 5) & BIT_NC_LATCH_STS_MASK;
        }
    }

    #endif

    *pu8_ClkIdx = (U8)s8_ClkIdx;

    return UNFD_ST_SUCCESS;

}
#endif	//DECIDE_CLOCK_BY_NAND

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
#if defined(DECIDE_CLOCK_BY_NAND) && DECIDE_CLOCK_BY_NAND
	NAND_DRIVER * pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Err = 0;
	U32 au32_ClkTable[NFIE_CLK_TABLE_CNT] = NFIE_CLK_TABLE;
	char *ClkStrTable[NFIE_CLK_TABLE_CNT] = NFIE_CLK_TABLE_STR;

	#if defined(FCIE4_DDR) && FCIE4_DDR
	U32 au32_4ClkTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_TABLE;
	char *Clk4StrTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_TABLE_STR;
	U32 au32_1TTable[NFIE_4CLK_TABLE_CNT] = NFIE_4CLK_1T_TABLE;
	#endif

	U8 u8_ClkIdx = 0;

	{
		u32_Err = nand_find_timing(&u8_ClkIdx, 0);
		if(u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err, NAND, Cannot config nand timing\n");
			nand_die();
			return(u32_Err);
		}
		pNandDrv->u32_Clk = au32_ClkTable[u8_ClkIdx];
		nand_printf("FCIE is set to %sHz\n", ClkStrTable[u8_ClkIdx]);
	}

	nand_clock_setting(pNandDrv->u32_Clk);
	REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);
	//nand_debug(UNFD_DEBUG_LEVEL, 0,"reg_ckg_fcie(%08X)=%08X\n", reg_ckg_fcie, REG(reg_ckg_fcie));
	nand_printf("RE LATCH is set to %X\n", pNandDrv->u16_Reg57_RELatch);

#endif

  return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// do nothing in Linux
}

void nand_reset_WatchDog(void)
{
	// do nothing in Linux
}

extern void hal_dcache_flush(void *base , u32 asize);
//extern void Chip_Flush_Memory(void);
extern void Chip_Clean_Cache_Range_VA_PA(unsigned long u32VAddr,unsigned long u32PAddr,unsigned long u32Size);
extern void Chip_Flush_Cache_Range_VA_PA(unsigned long u32VAddr,unsigned long u32PAddr,unsigned long u32Size);

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt, int mode)
{
		//extern void ___dma_single_cpu_to_dev(const void *kaddr, size_t size, enum dma_data_direction dir);
    //mode 0 for write, 1 for read
    if( mode == WRITE_TO_NAND )	//Write
    {
        //Write (DRAM->NAND)-> flush
        //printk("mode=0:u32_ByteCnt=%d\n",u32_ByteCnt);
        Chip_Clean_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), u32_ByteCnt);
        //printk("delay test\n");
        //mdelay(1);
    }
    else //Read
    {
        //Read (NAND->DRAM) -> inv
        //printk("mode=1:u32_ByteCnt=%d\n",u32_ByteCnt);
        Chip_Flush_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), u32_ByteCnt);
        //printk("delay test\n");
        //mdelay(1);
    }

	return virt_to_phys((void *)u32_DMAAddr);
}


U32 nand_translate_Spare_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt, int mode)
{
		//extern void ___dma_single_cpu_to_dev(const void *kaddr, size_t size, enum dma_data_direction dir);
    //mode 0 for write, 1 for read
    if( mode == WRITE_TO_NAND )	//Write
    {
        //Write (DRAM->NAND)-> flush
        //printk("mode=0:u32_ByteCnt=%d\n",u32_ByteCnt);
        Chip_Clean_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), u32_ByteCnt);
        //printk("delay test\n");
        //mdelay(1);
    }
    else //Read
    {
        //Read (NAND->DRAM) -> inv
        //printk("mode=1:u32_ByteCnt=%d\n",u32_ByteCnt);
        Chip_Flush_Cache_Range_VA_PA(u32_DMAAddr,__pa(u32_DMAAddr), u32_ByteCnt);
        //printk("delay test\n");
        //mdelay(1);
    }

	return virt_to_phys((void *)u32_DMAAddr);
}


#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);
U16 u16_FcieMieEvent = 0;

irqreturn_t NC_FCIE_IRQ(int irq, void *dummy)
{
	U16 u16_Reg;

	u16_Reg = REG(NC_FUN_CTL);
	if((u16_Reg&BIT_NC_EN) != BIT_NC_EN)
	{
		//nand_debug(UNFD_DEBUG_LEVEL, 1 ,"not nand interrupt\r\n");
		// not NAND interrupt
		return IRQ_NONE;
	}
    u16_FcieMieEvent |= REG(NC_MIE_EVENT);
    REG_WRITE_UINT16(NC_MIE_EVENT, u16_FcieMieEvent);
	wake_up(&fcie_wait);
	return IRQ_HANDLED;
}

U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	U16 u16_Reg;
    U32 u32_Timeout = (usecs_to_jiffies(u32_MicroSec) > 0) ? usecs_to_jiffies(u32_MicroSec) : 1; // timeout time

	wait_event_timeout(fcie_wait, ((u16_FcieMieEvent&u16_WaitEvent) == u16_WaitEvent), u32_Timeout);
	if((u16_FcieMieEvent&u16_WaitEvent) != u16_WaitEvent)
	{
        u16_FcieMieEvent = 0;
        REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);     // Read all events
        nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Timeout: REG(NC_MIE_EVENT) = 0x%X\n", u16_Reg);

		return UNFD_ST_ERR_E_TIMEOUT;
	}

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_WaitEvent); /*clear events*/
	return UNFD_ST_SUCCESS;
}

//xxx tomodify
void nand_enable_intr_mode(void)
{
	u16_FcieMieEvent = 0;
	REG_WRITE_UINT16(NC_MIE_INT_EN, BIT_NC_JOB_END);
}
#endif

U32 NC_PlatformResetPre(void)
{
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
    U16 u16_Reg;
    REG_WRITE_UINT16(NC_FUN_CTL, BIT_NC_EN);

    REG_READ_UINT16(FCIE_NC_WBUF_CIFD_BASE, u16_Reg); // dummy read for WCIFD clock
    REG_READ_UINT16(FCIE_NC_WBUF_CIFD_BASE, u16_Reg); // dummy read for WCIFD clock

    REG_READ_UINT16(FCIE_NC_RBUF_CIFD_BASE, u16_Reg); // dummy read for RCIFD clock twice said by designer
    REG_READ_UINT16(FCIE_NC_RBUF_CIFD_BASE, u16_Reg); // dummy read for RCIFD clock twice said by designer

    return UNFD_ST_SUCCESS;
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}
EXPORT_SYMBOL(drvNAND_get_DrvContext_address);
void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

PAIRED_PAGE_MAP_t ga_tPairedPageMap[512] = {{0,0}};

//static UNFD_ALIGN0 U32 drvNandPageBuf[16384/4] UNFD_ALIGN1; /* 8192 Bytes */
//static UNFD_ALIGN0 U32 drvNandSpareBuf[1280/4] UNFD_ALIGN1; /*   512 Bytes */

struct page *drvNAND_DataPage = 0;
struct page *drvNAND_SparePage = 0;

U32 NC_PlatformInit(void)
{
  NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

  nand_pads_init();
  pNandDrv->u8_WordMode = 0; // TV/Set-Top Box projects did not support x16 NAND flash
  nand_pads_switch(1);

  pNandDrv->u8_MacroType = MACRO_TYPE_32BIT;
  pNandDrv->u32_Clk =FCIE3_SW_SLOWEST_CLK;
  nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
  pNandDrv->u16_Reg57_RELatch = BIT_NC_LATCH_DATA_2_0_T|BIT_NC_LATCH_STS_2_0_T;
  REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);
//  pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
//  pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;

	if(pNandDrv->PlatCtx_t.pu8_PageDataBuf == NULL)
	{
	    drvNAND_DataPage = alloc_pages(__GFP_COMP, 2);
		if(drvNAND_DataPage ==NULL)
		{
			nand_debug(0, 1, "Err allocate page 1 fails\n");
			nand_die();
		}
		pNandDrv->PlatCtx_t.pu8_PageDataBuf =(U8*) kmap(drvNAND_DataPage);
	}

	if(pNandDrv->PlatCtx_t.pu8_PageSpareBuf == NULL)
	{
	    drvNAND_SparePage = alloc_pages(__GFP_COMP, 0);
		if(drvNAND_SparePage ==NULL)
		{
			nand_debug(0, 1, "Err allocate page 2 fails\n");
			nand_die();
		}
		pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)kmap(drvNAND_SparePage);
	}
  // no shared-bus with Disp
  pNandDrv->u8_SwPatchWaitRb= 0;
  pNandDrv->u8_SwPatchJobStart= 0;

  return UNFD_ST_SUCCESS;
}


#elif (defined(NAND_DRV_TV_LINUX)&&NAND_DRV_TV_LINUX)

NAND_DRIVER sg_NandDrv;

U32 nand_hw_timer_delay(U32 u32usTick)
{
#if 1	// Use PIU timer

	U32 u32HWTimer = 0;
	volatile U16 u16TimerLow = 0;
	volatile U16 u16TimerHigh = 0;

	// reset HW timer
	REG_WRITE_UINT16(TIMER0_MAX_LOW, 0xFFFF);
	REG_WRITE_UINT16(TIMER0_MAX_HIGH, 0xFFFF);
	REG_WRITE_UINT16(TIMER0_ENABLE, 0);

	// start HW timer
	REG_SET_BITS_UINT16(TIMER0_ENABLE, 0x0001);

	while( u32HWTimer < 12*u32usTick ) // wait for u32usTick micro seconds
	{
		REG_READ_UINT16(TIMER0_CAP_LOW, u16TimerLow);
		REG_READ_UINT16(TIMER0_CAP_HIGH, u16TimerHigh);

		u32HWTimer = (u16TimerHigh<<16) | u16TimerLow;
	}

	REG_WRITE_UINT16(TIMER0_ENABLE, 0);
	
#else	// Use kernel udelay

	udelay(u32usTick);

#endif

	return u32usTick+1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	#if defined(CONFIG_MSTAR_TITANIA4) || defined(CONFIG_MSTAR_TITANIA9) || defined(CONFIG_MSTAR_TITANIA13) || \
		defined(CONFIG_MSTAR_AMBER1)

	REG_SET_BITS_UINT16(reg_nf_en, BIT4);

	#elif defined(CONFIG_MSTAR_JANUS)

	REG_SET_BITS_UINT16(reg_nf_en, BIT0);

	#elif defined(CONFIG_MSTAR_URANUS4)

	// Let sboot to determine the pad, pcmcia driver will recover it after it finishes job
	
	REG_CLR_BITS_UINT16(NC_TEST_MODE, BIT5);
	nand_printf("NC_TEST_MODE(%08X)=%04X\n", NC_TEST_MODE, REG(NC_TEST_MODE));

	REG_CLR_BITS_UINT16(reg_boot_from_pf, BIT10);
	nand_printf("reg_boot_from_pf(%08X)=%04X\n", reg_boot_from_pf, REG(reg_boot_from_pf));

	#elif defined(CONFIG_MSTAR_TITANIA8) || defined(CONFIG_MSTAR_TITANIA12) || \
		  defined(CONFIG_MSTAR_JANUS2) || defined(CONFIG_MSTAR_AMBER5)

	// Let sboot to determine the pad, pcmcia driver will recover it after it finishes job

	#endif

	nand_printf("reg_nf_en(%08X)=%04X\n", reg_nf_en, REG(reg_nf_en));
	nand_printf("reg_allpad_in(%08X)=%04X\n", reg_allpad_in, REG(reg_allpad_in));

	return UNFD_ST_SUCCESS;
}

U32 nand_clock_setting(U32 u32ClkParam)
{
	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT6-1); // enable FCIE clk, set to lowest clk

	switch(u32ClkParam)
	{
		#if defined(CONFIG_MSTAR_TITANIA8) || defined(CONFIG_MSTAR_TITANIA12) || defined(CONFIG_MSTAR_JANUS2) || \
			defined(CONFIG_MSTAR_TITANIA9) || defined(CONFIG_MSTAR_TITANIA13)
			
		case NFIE_CLK_5_4M:		break;
		case NFIE_CLK_27M:		break;
		case NFIE_CLK_32M:		break;
		case NFIE_CLK_43M:		break;
		case NFIE_CLK_86M:		break;

		#if defined(CONFIG_MSTAR_TITANIA12) || defined(CONFIG_MSTAR_TITANIA13)
		
		case NFIE_CLK_36M:		break;
		case NFIE_CLK_40M:		break;
		case NFIE_CLK_SSC:		break;
		
		#endif

		#elif defined(CONFIG_MSTAR_AMBER1)

		case NFIE_CLK_5_4M:		break;
		case NFIE_CLK_27M:		break;
		case NFIE_CLK_32M:		break;
		case NFIE_CLK_36M:		break;
		case NFIE_CLK_40M:		break;
		case NFIE_CLK_43M:		break;
		case NFIE_CLK_54M:		break;
		case NFIE_CLK_62M:		break;
		case NFIE_CLK_72M:		break;
		case NFIE_CLK_80M:		break;
		case NFIE_CLK_86M:		break;
		case NFIE_CLK_SSC:		break;

		#elif defined(CONFIG_MSTAR_AMBER5)

		case NFIE_CLK_300K:		break;
		case NFIE_CLK_20M:		break;
		case NFIE_CLK_24M:		break;
		case NFIE_CLK_27M:		break;
		case NFIE_CLK_32M:		break;
		case NFIE_CLK_36M:		break;
		case NFIE_CLK_40M:		break;
		case NFIE_CLK_43M:		break;
		case NFIE_CLK_48M:		break;
		case NFIE_CLK_54M:		break;
		case NFIE_CLK_62M:		break;
		case NFIE_CLK_72M:		break;
		case NFIE_CLK_80M:		break;
		case NFIE_CLK_86M:		break;
		case NFIE_CLK_SSC:		break;

		#else // defined(CONFIG_MSTAR_TITANIA4) || defined(CONFIG_MSTAR_JANUS) || defined(CONFIG_MSTAR_URANUS4)

		case NFIE_CLK_300K:		break;
		case NFIE_CLK_750K:		break;
		case NFIE_CLK_5_4M:		break;
		case NFIE_CLK_13_5M:	break;
		case NFIE_CLK_18M:		break;
		case NFIE_CLK_22_7M:	break;
		case NFIE_CLK_27M:		break;
		case NFIE_CLK_32M:		break;
		case NFIE_CLK_43M:		break;
		case NFIE_CLK_54M:		break;
		case NFIE_CLK_72M:		break;
		case NFIE_CLK_86M:		break;

		#endif

		default:
			nand_die();
	}
	
	REG_SET_BITS_UINT16(reg_ckg_fcie, u32ClkParam);
	nand_printf("reg_ckg_fcie(%08X)=%08X\n", reg_ckg_fcie, REG(reg_ckg_fcie));

	return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	// do nothing in Linux
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// do nothing in Linux
}

void nand_reset_WatchDog(void)
{
	// do nothing in Linux
}

DECLARE_MUTEX(FCIE3_mutex);
atomic_t nand_need_init = ATOMIC_INIT(0);
atomic_t nand_in_use = ATOMIC_INIT(0);
static u16 reg_sdio_mod;
static u16 reg_sdio_ctl;
static u16 reg_mie_evt_en;

void nand_lock_fcie(void)
{
	U32 cnt = 0;

	down(&FCIE3_mutex);

	atomic_set(&nand_in_use, 1);

    HAL_READ_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x10), reg_sdio_mod);
    HAL_READ_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x11), reg_sdio_ctl);
    HAL_READ_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x01), reg_mie_evt_en);
    HAL_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x01), 0);
    HAL_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x11), 0);

    do {
		HAL_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x10), 0);
		if( ++cnt > 0xFFFFFF )
		{
			printk("\033[31mTimeout!!!\033[m\n");
			break;
		}
	} while( REG(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x10)));

	HAL_WRITE_UINT16(NC_PATH_CTL, BIT_NC_EN);

	if( atomic_read(&nand_need_init))
	{
		NC_ResetFCIE();
		//NC_ResetNandFlash();
		NC_Config();
		atomic_set(&nand_need_init, 0);
	}
}

void nand_unlock_fcie(void)
{
    HAL_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x0A), 0x02);
    HAL_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x10), reg_sdio_mod);
    HAL_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x11), reg_sdio_ctl);
    HAL_WRITE_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x01), reg_mie_evt_en);
    atomic_set(&nand_in_use, 0);

	up(&FCIE3_mutex);
}


U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	#if CONFIG_MIPS

	_dma_cache_wback_inv(u32_DMAAddr, u32_ByteCnt);

	#if defined(CONFIG_MSTAR_URANUS4)
	Chip_Flush_Memory();
	#endif
	
	return u32_DMAAddr;
	
	#else	// JANUS

	return virt_to_phys((void*)u32_DMAAddr);
	
	#endif
}

void nand_flush_miu_pipe(void)
{
	#if defined(CONFIG_MSTAR_URANUS4)
	Chip_Read_Memory();
	#endif
}

static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);
static U16 fcie_events = 0;
static int fcie_int = 0;

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
static U16 u16CurNCMIEEvent = 0;			// Used to store current IRQ state

irqreturn_t NC_FCIE_IRQ(int irq, void *dummy)
{
	u16CurNCMIEEvent |= REG(NC_MIE_EVENT);					// Get current event
	REG_CLR_BITS_UINT16(NC_MIE_INT_EN, u16CurNCMIEEvent);	// Disable the IRQ which is invoked
	
	if( (u16CurNCMIEEvent & fcie_events) == fcie_events )
	{
		REG_READ_UINT16(NC_MIE_EVENT, u16CurNCMIEEvent);	// Read all the events
		REG_WRITE_UINT16(NC_MIE_EVENT, u16CurNCMIEEvent);	// Clear all the events
		u16CurNCMIEEvent = 0;								// Reset the current IRQ state
		fcie_int = 1;
		wake_up(&fcie_wait);
	}

	return IRQ_HANDLED;
}

U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	U16 u16_Reg;
	U32 u32_Count = 1; // Return microsec if timout
	U32 u32_Timeout = ((u32_MicroSec/1000) > 0) ? (u32_MicroSec/1000) : 1;	// timeout time

	fcie_events = u16_WaitEvent;
	REG_WRITE_UINT16(NC_MIE_INT_EN, u16_WaitEvent);
		
	if( wait_event_timeout(fcie_wait, (fcie_int == 1), u32_Timeout) == 0 ) // wait at least 2 second for FCIE3 events
	{
		u32_Count = u32_MicroSec;
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);		// Read all events
		REG_WRITE_UINT16(NC_MIE_EVENT, u16_Reg);	// Clear all events
	}
			
	fcie_int = 0;
	
	return UNFD_ST_SUCCESS;
}

void nand_enable_intr_mode(void)
{
	request_irq(13, NC_FCIE_IRQ, SA_INTERRUPT, "fcie", NULL);
}
#endif

void *drvNAND_get_DrvContext_address(void) // exposed API
{
	return &sg_NandDrv;
}

//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
const U8 u8FSTYPE[256] =
{	0,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
	0,18, 0, 6, 0, 8,10, 0, 0,12, 0, 0, 0, 0, 0, 0, // 7
	0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, // 8
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C
	0,13, 0,16, 0,17, 3, 0, 0, 0,15, 0,14, 0, 0, 0, // D
	0, 0, 0, 2, 0, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, // E
   20,13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F
};

U16 PairedPageMap_Hynix_LSB[] = {
0x0, 0x1, 0x2, 0x3, 0x6, 0x7, 0xA, 0xB, 0xE, 0xF, 0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B,
0x1E, 0x1F, 0x22, 0x23, 0x26, 0x27, 0x2A, 0x2B, 0x2E, 0x2F, 0x32, 0x33, 0x36, 0x37, 0x3A, 0x3B,
0x3E, 0x3F, 0x42, 0x43, 0x46, 0x47, 0x4A, 0x4B, 0x4E, 0x4F, 0x52, 0x53, 0x56, 0x57, 0x5A, 0x5B,
0x5E, 0x5F, 0x62, 0x63, 0x66, 0x67, 0x6A, 0x6B, 0x6E, 0x6F, 0x72, 0x73, 0x76, 0x77, 0x7A, 0x7B,
0x7E, 0x7F, 0x82, 0x83, 0x86, 0x87, 0x8A, 0x8B, 0x8E, 0x8F, 0x92, 0x93, 0x96, 0x97, 0x9A, 0x9B,
0x9E, 0x9F, 0xA2, 0xA3, 0xA6, 0xA7, 0xAA, 0xAB, 0xAE, 0xAF, 0xB2, 0xB3, 0xB6, 0xB7, 0xBA, 0xBB,
0xBE, 0xBF, 0xC2, 0xC3, 0xC6, 0xC7, 0xCA, 0xCB, 0xCE, 0xCF, 0xD2, 0xD3, 0xD6, 0xD7, 0xDA, 0xDB,
0xDE, 0xDF, 0xE2, 0xE3, 0xE6, 0xE7, 0xEA, 0xEB, 0xEE, 0xEF, 0xF2, 0xF3, 0xF6, 0xF7, 0xFA, 0xFB
};
U16 PairedPageMap_Hynix_MSB[] = {
0x4, 0x5, 0x8, 0x9, 0xC, 0xD, 0x10, 0x11, 0x14, 0x15, 0x18, 0x19, 0x1C, 0x1D, 0x20, 0x21,
0x24, 0x25, 0x28, 0x29, 0x2C, 0x2D, 0x30, 0x31, 0x34, 0x35, 0x38, 0x39, 0x3C, 0x3D, 0x40, 0x41,
0x44, 0x45, 0x48, 0x49, 0x4C, 0x4D, 0x50, 0x51, 0x54, 0x55, 0x58, 0x59, 0x5C, 0x5D, 0x60, 0x61,
0x64, 0x65, 0x68, 0x69, 0x6C, 0x6D, 0x70, 0x71, 0x74, 0x75, 0x78, 0x79, 0x7C, 0x7D, 0x80, 0x81,
0x84, 0x85, 0x88, 0x89, 0x8C, 0x8D, 0x90, 0x91, 0x94, 0x95, 0x98, 0x99, 0x9C, 0x9D, 0xA0, 0xA1,
0xA4, 0xA5, 0xA8, 0xA9, 0xAC, 0xAD, 0xB0, 0xB1, 0xB4, 0xB5, 0xB8, 0xB9, 0xBC, 0xBD, 0xC0, 0xC1,
0xC4, 0xC5, 0xC8, 0xC9, 0xCC, 0xCD, 0xD0, 0xD1, 0xD4, 0xD5, 0xD8, 0xD9, 0xDC, 0xDD, 0xE0, 0xE1,
0xE4, 0xE5, 0xE8, 0xE9, 0xEC, 0xED, 0xF0, 0xF1, 0xF4, 0xF5, 0xF8, 0xF9, 0xFC, 0xFD, 0xFE, 0xFF
};

void drvNAND_CHECK_FLASH_TYPE(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	
	if( (pNandDrv->au8_ID[0] != VENDOR_SAMSUNG) &&
		(pNandDrv->au8_ID[0] != VENDOR_TOSHIBA) &&
		(pNandDrv->au8_ID[0] != VENDOR_RENESAS) &&
		(pNandDrv->au8_ID[0] != VENDOR_HYNIX)  &&
		(pNandDrv->au8_ID[0] != VENDOR_FUJITSU) &&
		(pNandDrv->au8_ID[0] != VENDOR_MICRON) &&
		(pNandDrv->au8_ID[0] != VENDOR_NATIONAL)  &&
		(pNandDrv->au8_ID[0] != VENDOR_ST) )
	{
		pNandDrv->u16_BlkCnt = 0;
		pNandDrv->u16_BlkPageCnt = 0;
		pNandDrv->u16_PageByteCnt = 0;
		pNandDrv->u16_SectorByteCnt = 0;
		pNandDrv->u16_SpareByteCnt = 0;

		printk("Unsupport Vendor %02X\n", pNandDrv->au8_ID[0]);

		return; // unsupported flash maker
	}

	switch(u8FSTYPE[pNandDrv->au8_ID[1]])
	{
		case 0:
			pNandDrv->u16_BlkCnt = 0;
			pNandDrv->u16_BlkPageCnt = 0;
			pNandDrv->u16_PageByteCnt = 0;
			pNandDrv->u16_SectorByteCnt = 0;
			pNandDrv->u16_SpareByteCnt = 0;
			break;
		case 2:
			pNandDrv->u16_BlkCnt = 512;
			pNandDrv->u16_BlkPageCnt = 16;
			pNandDrv->u16_PageByteCnt = 512;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			break;
		case 4:
			pNandDrv->u16_BlkCnt = 1024;
			pNandDrv->u16_BlkPageCnt = 16;
			pNandDrv->u16_PageByteCnt = 512;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C3TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			break;
		case 6:
			pNandDrv->u16_BlkCnt = 1024;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u16_PageByteCnt = 512;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C3TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			break;
		case 7:
			//_fsinfo.eFlashConfig |= FLASH_WP;
		case 8:
			pNandDrv->u16_BlkCnt = 2048;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u16_PageByteCnt = 512;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C3TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			break;
		case 10:
			pNandDrv->u16_BlkCnt = 4096;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u16_PageByteCnt = 512;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			break;
		case 12:
			pNandDrv->u16_BlkCnt = 8192;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u16_PageByteCnt = 512;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			break;
		case 13:
			pNandDrv->u16_BlkCnt = 1024;
			pNandDrv->u16_BlkPageCnt = 64;
			pNandDrv->u16_PageByteCnt = 2048;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 64;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			break;
		case 14:
			if(pNandDrv->au8_ID[0]==VENDOR_HYNIX)
			{
				pNandDrv->u16_BlkCnt = 4096;
				pNandDrv->u16_BlkPageCnt = 128;
				pNandDrv->u16_PageByteCnt = 2048;
				pNandDrv->u16_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else
			{
				pNandDrv->u16_BlkCnt = 4096;
				pNandDrv->u16_BlkPageCnt = 64;
				pNandDrv->u16_PageByteCnt = 2048;
				pNandDrv->u16_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			break;
		case 15:
			pNandDrv->u16_BlkCnt = 2048;
			pNandDrv->u16_BlkPageCnt = 64;
			pNandDrv->u16_PageByteCnt = 2048;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 64;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			break;
		case 16:
			 if((pNandDrv->au8_ID[0]==VENDOR_HYNIX))
			{
				pNandDrv->u16_BlkCnt = 4096;
				pNandDrv->u16_BlkPageCnt = 128;
				pNandDrv->u16_PageByteCnt = 2048;
				pNandDrv->u16_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else if(pNandDrv->au8_ID[0] != VENDOR_ST)
			{
				pNandDrv->u16_BlkCnt = 2048;
				pNandDrv->u16_BlkPageCnt = 64;
				pNandDrv->u16_PageByteCnt = 2048;
				pNandDrv->u16_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else
			{
				pNandDrv->u16_BlkCnt = 8192;
				pNandDrv->u16_BlkPageCnt = 64;
				pNandDrv->u16_PageByteCnt = 2048;
				pNandDrv->u16_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			break;
		case 17:
			if(pNandDrv->au8_ID[0] != VENDOR_SAMSUNG)
			{
				pNandDrv->u16_BlkCnt = 8192;
				pNandDrv->u16_BlkPageCnt = 128;
				pNandDrv->u16_PageByteCnt = 2048;
				pNandDrv->u16_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else
			{
				pNandDrv->u16_BlkCnt = 16384;
				pNandDrv->u16_BlkPageCnt = 64;
				pNandDrv->u16_PageByteCnt = 2048;
				pNandDrv->u16_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			break;
		case 18:
			pNandDrv->u16_BlkCnt = 16384;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u16_PageByteCnt = 512;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			break;
		case 20:
			pNandDrv->u16_BlkCnt = 512;
			pNandDrv->u16_BlkPageCnt = 64;
			pNandDrv->u16_PageByteCnt = 2048;
			pNandDrv->u16_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 64;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			break;
		default:
			pNandDrv->u16_BlkCnt = 0;
			pNandDrv->u16_BlkPageCnt = 0;
			pNandDrv->u16_PageByteCnt = 0;
			pNandDrv->u16_SectorByteCnt = 0;
			pNandDrv->u16_SpareByteCnt = 0;
			break;
	}

	pNandDrv->u16_ECCType = ECC_TYPE_4BIT;

}

U32 NC_PlatformResetPre(void)
{
	//HalMiuMaskReq(MIU_CLT_FCIE);
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	U16 u16_Reg;

	REG_READ_UINT16(FCIE_NC_CIFD_BASE, u16_Reg); // dummy read for CIFD clock
	
	#if defined(DUTY_CYCLE_PATCH)&&DUTY_CYCLE_PATCH
	REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);	// duty cycle 3:1 in 86Mhz
	#endif

	//HalMiuUnMaskReq(MIU_CLT_FCIE);
	
    return UNFD_ST_SUCCESS;
}

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	
	pNandDrv->u8_WordMode = 0; // TV/Set-Top Box projects did not support x16 NAND flash
	nand_pads_switch(1);
	nand_clock_setting(FCIE3_SW_DEFAULT_CLK);

	// miu eco
	REG_SET_BITS_UINT16(NC_REG_2Fh, BIT0);
	
	// no shared-bus with Disp
	pNandDrv->u8_SwPatchWaitRb= 0;
	pNandDrv->u8_SwPatchJobStart= 0;
	NC_ConfigHal(NAND_HAL_RAW);
	
	return UNFD_ST_SUCCESS;
}

#elif (defined(NAND_DRV_B3_LINUX)&&NAND_DRV_B3_LINUX)
#include "hal_drv_util.h"
#include "sys.ho"
#include "drv_clkgen_cmu.h"
#include "hal_clkgen_bbtop.h"
#include "hal_miu_arb.h"

U32 nand_hw_timer_delay(U32 u32usTick)
{
    SYS_UDELAY(u32usTick);
	
	return u32usTick+1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	return UNFD_ST_SUCCESS;
}

U32 nand_clock_setting(U32 u32ClkParam)
{
	ClkgenClkSpeed_e eClkSpeed;

    switch(u32ClkParam)
    {
        case NFIE_CLK_187_5K:
            eClkSpeed = CLKGEN_187P5KHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 187.5K\r\n");
            break;
        case NFIE_CLK_750K:
            eClkSpeed = CLKGEN_750KHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 750K\r\n");
            break;
        case NFIE_CLK_6M:
            eClkSpeed = CLKGEN_6MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 6M\r\n");
            break;
        case NFIE_CLK_10_4M:
            eClkSpeed = CLKGEN_10P4MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 10.4M\r\n");
            break;
        case NFIE_CLK_13M:
            eClkSpeed = CLKGEN_13MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 13M\r\n");
            break;
        case NFIE_CLK_19_5M:
            eClkSpeed = CLKGEN_19P5MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 19.5M\r\n");
            break;
        case NFIE_CLK_22_29M:
            eClkSpeed = CLKGEN_22P29MHZ;
           //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 22.29M\r\n");
            break;
        case NFIE_CLK_26M:
            eClkSpeed = CLKGEN_26MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 26M\r\n");
            break;
        case NFIE_CLK_39M:
            eClkSpeed = CLKGEN_39MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 39M\r\n");
            break;
        case NFIE_CLK_44_57M:
            eClkSpeed = CLKGEN_44P57MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 44.57M\r\n");
            break;
        case NFIE_CLK_52M:
            eClkSpeed = CLKGEN_52MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 52M\r\n");
            break;
        case NFIE_CLK_78M:
            eClkSpeed = CLKGEN_78MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 78M\r\n");
            break;
        case NFIE_CLK_31_2M:
            eClkSpeed = CLKGEN_31P2MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 31.2M\r\n");
            break;
        case NFIE_CLK_34_67M:
            eClkSpeed = CLKGEN_34P67MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 34.6M\r\n");
            break;
        case NFIE_CLK_29_71M:
            eClkSpeed = CLKGEN_29P71MHZ;
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 29.7M\r\n");
            break;
        default:
            eClkSpeed = CLKGEN_10P4MHZ;
            nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"nand_clock_setting error parameter!!!\r\n");
            break;
    }

    HalClkgenBbtopSetClkSpeed(BBTOP_CLK_NFIE, eClkSpeed, CLKSRC_PLL22);

	return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	u32_Clk = 2000000000/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_187_5K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_187_5K && u32_Clk < NFIE_CLK_750K )
		pNandDrv->u32_Clk = NFIE_CLK_187_5K;
	else if( u32_Clk >= NFIE_CLK_750K && u32_Clk < NFIE_CLK_6M )
		pNandDrv->u32_Clk = NFIE_CLK_750K;
	else if( u32_Clk >= NFIE_CLK_6M && u32_Clk < NFIE_CLK_10_4M )
		pNandDrv->u32_Clk = NFIE_CLK_6M;
	else if( u32_Clk >= NFIE_CLK_10_4M && u32_Clk < NFIE_CLK_13M )
		pNandDrv->u32_Clk = NFIE_CLK_10_4M;
	else if( u32_Clk >= NFIE_CLK_13M && u32_Clk < NFIE_CLK_19_5M )
		pNandDrv->u32_Clk = NFIE_CLK_13M;
	else if( u32_Clk >= NFIE_CLK_19_5M && u32_Clk < NFIE_CLK_22_29M )
		pNandDrv->u32_Clk = NFIE_CLK_19_5M;
	else if( u32_Clk >= NFIE_CLK_22_29M && u32_Clk < NFIE_CLK_26M )
		pNandDrv->u32_Clk = NFIE_CLK_22_29M;
	else if( u32_Clk >= NFIE_CLK_26M && u32_Clk < NFIE_CLK_29_71M )
		pNandDrv->u32_Clk = NFIE_CLK_26M;
	else if( u32_Clk >= NFIE_CLK_29_71M && u32_Clk < NFIE_CLK_31_2M )
		pNandDrv->u32_Clk = NFIE_CLK_29_71M;
	else if( u32_Clk >= NFIE_CLK_31_2M && u32_Clk < NFIE_CLK_34_67M )
		pNandDrv->u32_Clk = NFIE_CLK_31_2M;
	else if( u32_Clk >= NFIE_CLK_34_67M && u32_Clk < NFIE_CLK_39M )
		pNandDrv->u32_Clk = NFIE_CLK_34_67M;
	else if( u32_Clk >= NFIE_CLK_39M && u32_Clk < NFIE_CLK_44_57M )
		pNandDrv->u32_Clk = NFIE_CLK_39M;
	else if( u32_Clk >= NFIE_CLK_44_57M && u32_Clk < NFIE_CLK_52M )
		pNandDrv->u32_Clk = NFIE_CLK_44_57M;
	else if( u32_Clk >= NFIE_CLK_52M && u32_Clk < NFIE_CLK_78M )
		pNandDrv->u32_Clk = NFIE_CLK_52M;
	else
		pNandDrv->u32_Clk = NFIE_CLK_78M;

	if(sys_GetChipRevisionId() < SYS_CHIP_ID_U03)
	{
		switch(pNandDrv->u32_Clk)
		{
			case NFIE_CLK_29_71M:
			case NFIE_CLK_31_2M:
			case NFIE_CLK_34_67M:
				pNandDrv->u32_Clk = NFIE_CLK_26M;
				break;
			default:
				break;
		}
	}

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// do nothing in Linux
}

void nand_reset_WatchDog(void)
{
	// do nothing in Linux
}

#define SYSHAL_DCACHE_LINE_SIZE 32
#define HAL_DCACHE_START_ADDRESS(_addr_) \
    (((u32)(_addr_)) & ~(SYSHAL_DCACHE_LINE_SIZE-1))

#define HAL_DCACHE_END_ADDRESS(_addr_, _asize_) \
    (((u32)((_addr_) + (_asize_) + (SYSHAL_DCACHE_LINE_SIZE-1) )) & \
     ~(SYSHAL_DCACHE_LINE_SIZE-1))
void hal_dcache_flush(void *base , u32 asize)
{
	register u32	_addr_ = HAL_DCACHE_START_ADDRESS((u32)base);
	register u32	_eaddr_ = HAL_DCACHE_END_ADDRESS((u32)(base), asize);

	for( ; _addr_ < _eaddr_; _addr_ += SYSHAL_DCACHE_LINE_SIZE )
	__asm__ __volatile__ ("MCR p15, 0, %0, c7, c14, 1" : : "r" (_addr_));

	/* Drain write buffer */
	_addr_ = 0x00UL;
	__asm__ __volatile__ ("MCR p15, 0, %0, c7, c10, 4" : : "r" (_addr_));
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	hal_dcache_flush((void *)u32_DMAAddr, u32_ByteCnt);

	return HalUtilPHY2MIUAddr(virt_to_phys(u32_DMAAddr));
}

void nand_flush_miu_pipe(void)
{

}

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);
static U16 fcie_events = 0;
static int fcie_int = 0;
static U16 u16CurNCMIEEvent = 0;			// Used to store current IRQ state

irqreturn_t NC_FCIE_IRQ(int irq, void *dummy)
{
	u16CurNCMIEEvent |= REG(NC_MIE_EVENT);					// Get current event
	REG_CLR_BITS_UINT16(NC_MIE_INT_EN, u16CurNCMIEEvent);	// Disable the IRQ which is invoked
	
	if( (u16CurNCMIEEvent & fcie_events) == fcie_events )
	{
		REG_READ_UINT16(NC_MIE_EVENT, u16CurNCMIEEvent);	// Read all the events
		REG_WRITE_UINT16(NC_MIE_EVENT, u16CurNCMIEEvent);	// Clear all the events
		u16CurNCMIEEvent = 0;								// Reset the current IRQ state
		fcie_int = 1;
		wake_up(&fcie_wait);
	}

	return IRQ_HANDLED;
}

U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	U16 u16_Reg;
	U32 u32_Count = 1; // Return microsec if timout
	U32 u32_Timeout = ((u32_MicroSec/1000) > 0) ? (u32_MicroSec/1000) : 1;	// timeout time

	fcie_events = u16_WaitEvent;
	REG_WRITE_UINT16(NC_MIE_INT_EN, u16_WaitEvent);
		
	if( wait_event_timeout(fcie_wait, (fcie_int == 1), u32_Timeout) == 0 ) // wait at least 2 second for FCIE3 events
	{
		u32_Count = u32_MicroSec;
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);		// Read all events
		REG_WRITE_UINT16(NC_MIE_EVENT, u16_Reg);	// Clear all events
	}
			
	fcie_int = 0;
	
	return UNFD_ST_SUCCESS;
}

void nand_enable_intr_mode(void)
{
	request_irq(13, NC_FCIE_IRQ, SA_INTERRUPT, "fcie", NULL);
}
#endif

U32 NC_PlatformResetPre(void)
{
	HalMiuMaskReq(MIU_CLT_FCIE);
	
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	HalMiuUnMaskReq(MIU_CLT_FCIE);
	
	return UNFD_ST_SUCCESS;
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U16 u16_reg;
	U32 u32_ChipId;
	
	REG_READ_UINT16(GET_REG_ADDR(0x74007C00, 0x20), u16_reg);
	u16_reg &= 0x0F;
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1 ,"sys_get_extMemType=0x%X\r\n", u16_reg);
	if( u16_reg == 0x00 || u16_reg == 0x01 || u16_reg == 0x02 )
		pNandDrv->u8_WordMode= 1;
	else
		pNandDrv->u8_WordMode= 0;

	nand_pads_switch(1);
	
	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;
	u32_ChipId = sys_GetChipRevisionId();
	if(u32_ChipId < SYS_CHIP_ID_U03)
	{
		switch(pNandDrv->u32_Clk)
		{
			case NFIE_CLK_29_71M:
			case NFIE_CLK_31_2M:
			case NFIE_CLK_34_67M:
				pNandDrv->u32_Clk = NFIE_CLK_26M;
				break;
			default:
				break;
		}
	}
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);

	nand_clock_setting(pNandDrv->u32_Clk);

	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;
	
	//if(u32_ChipId<SYS_CHIP_ID_U03)
	if(0)		
	{
		pNandDrv->u8_SwPatchWaitRb= 1;
		pNandDrv->u8_SwPatchJobStart= 1;
		NC_ConfigHal(NAND_HAL_PATCH);
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"B3 U%02X needs SW patch\r\n", u32_ChipId);
	}
	else
	{
		pNandDrv->u8_SwPatchWaitRb= 0;
		pNandDrv->u8_SwPatchJobStart= 0;
		NC_ConfigHal(NAND_HAL_RAW);
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"B3 U%02X has HW patch\r\n", u32_ChipId);
	}
	
	return UNFD_ST_SUCCESS;
}

#elif (defined(NAND_DRV_G2_LINUX)&&NAND_DRV_G2_LINUX)
#include "sys.ho"

U32 nand_hw_timer_delay(U32 u32usTick)
{
    SYS_UDELAY(u32usTick);
	
	return u32usTick+1;
}

U32 nand_pads_switch(U32 u32EnableFCIE)
{
	REG_SET_BITS_UINT16(reg_mreq_dyn_disable, BIT6); // mreq dynamic gating clk_miu_fcie disable
	REG_SET_BITS_UINT16(reg_pad_ctrl_idx5, BIT6); // reg_lcd_mode
	REG_CLR_BITS_UINT16(reg_pad_ctrl_idx5, BIT7); // normal order : 1
	REG_CLR_BITS_UINT16(reg_pad_ctrl_idx5, BIT15); // reg_all_pad_in
	REG_SET_BITS_UINT16(reg_pad_ctrl_idx16, BIT0); // for nand mode

	//nand_debug(3, 1, "reg_mreq_dyn_disable(0x%X):0x%x\n", reg_mreq_dyn_disable, REG(reg_mreq_dyn_disable));
	//nand_debug(3, 1, "reg_pad_ctrl_idx5(0x%X):0x%x\n", reg_pad_ctrl_idx5, REG(reg_pad_ctrl_idx5));
	//nand_debug(3, 1, "reg_pad_ctrl_idx16(0x%X):0x%x\n", reg_pad_ctrl_idx16, REG(reg_pad_ctrl_idx16));

	return UNFD_ST_SUCCESS;
}

U32 nand_clock_setting(U32 u32ClkParam)
{
	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT4); // not gate FCIE3 clk
	REG_SET_BITS_UINT16(reg_ckg_fcie, BIT6); // select fast

	REG_CLR_BITS_UINT16(reg_gate_miu_fcie, BIT3); // not gate clk_miu_fcie

	REG_CLR_BITS_UINT16(reg_ckg_fcie, NFIE_CLK_MASK); // set clk

    switch(u32ClkParam)
    {
        case NFIE_CLK_300K:

            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 300K\r\n");
            break;
        case NFIE_CLK_800K:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 1);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 800K\r\n");
            break;
        case NFIE_CLK_5_3M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 2);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 5.3M\r\n");
            break;
        case NFIE_CLK_12_8M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 3);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 12.8M\r\n");
            break;
        case NFIE_CLK_19_2M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 4);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 19.2M\r\n");
            break;
        case NFIE_CLK_24_6M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 5);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 24.6M\r\n");
            break;
        case NFIE_CLK_27_4M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 6);	
           //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 27.4M\r\n");
            break;
        case NFIE_CLK_32M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 7);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 32M\r\n");
            break;
        case NFIE_CLK_42_6M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 8);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 42.6M\r\n");
            break;
        case NFIE_CLK_51_2M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 9);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 51.2M\r\n");
            break;
        case NFIE_CLK_76_8M:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 10);	
            //nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"FCIE clock 76.8M\r\n");
            break;
        default:
			REG_SET_BITS_UINT16(reg_ckg_fcie, 2);	
            nand_debug(UNFD_DEBUG_LEVEL_LOW, 1 ,"nand_clock_setting error parameter!!!\r\n");
            break;
    }

	return UNFD_ST_SUCCESS;
}

U32 nand_config_clock(U16 u16_SeqAccessTime)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U32 u32_Clk;

	if(u16_SeqAccessTime == 0)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1 ,"u16_SeqAccessTime is not set\r\n");
		return UNFD_ST_SUCCESS;
	}

	u32_Clk = 2000000000/((U32)(u16_SeqAccessTime+NAND_SEQ_ACC_TIME_TOL)); // increase several ns for tolerance

	if( u32_Clk < NFIE_CLK_300K )
	{
		nand_die();
	}
	else if( u32_Clk >= NFIE_CLK_300K && u32_Clk < NFIE_CLK_800K )
		pNandDrv->u32_Clk = NFIE_CLK_300K;
	else if( u32_Clk >= NFIE_CLK_800K && u32_Clk < NFIE_CLK_5_3M )
		pNandDrv->u32_Clk = NFIE_CLK_800K;
	else if( u32_Clk >= NFIE_CLK_5_3M && u32_Clk < NFIE_CLK_12_8M )
		pNandDrv->u32_Clk = NFIE_CLK_5_3M;
	else if( u32_Clk >= NFIE_CLK_12_8M && u32_Clk < NFIE_CLK_19_2M )
		pNandDrv->u32_Clk = NFIE_CLK_12_8M;
	else if( u32_Clk >= NFIE_CLK_19_2M && u32_Clk < NFIE_CLK_24_6M )
		pNandDrv->u32_Clk = NFIE_CLK_19_2M;
	else if( u32_Clk >= NFIE_CLK_24_6M && u32_Clk < NFIE_CLK_27_4M )
		pNandDrv->u32_Clk = NFIE_CLK_24_6M;
	else if( u32_Clk >= NFIE_CLK_27_4M && u32_Clk < NFIE_CLK_32M )
		pNandDrv->u32_Clk = NFIE_CLK_27_4M;
	else if( u32_Clk >= NFIE_CLK_32M && u32_Clk < NFIE_CLK_42_6M )
		pNandDrv->u32_Clk = NFIE_CLK_32M;
	else if( u32_Clk >= NFIE_CLK_42_6M && u32_Clk < NFIE_CLK_51_2M )
		pNandDrv->u32_Clk = NFIE_CLK_42_6M;
	else if( u32_Clk >= NFIE_CLK_51_2M && u32_Clk < NFIE_CLK_76_8M )
		pNandDrv->u32_Clk = NFIE_CLK_51_2M;
	else
		pNandDrv->u32_Clk = NFIE_CLK_76_8M;

	nand_clock_setting(pNandDrv->u32_Clk);
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	return UNFD_ST_SUCCESS;
}

void nand_set_WatchDog(U8 u8_IfEnable)
{
	// do nothing in Linux
}

void nand_reset_WatchDog(void)
{
	// do nothing in Linux
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	_dma_cache_wback_inv(u32_DMAAddr, u32_ByteCnt);
	return (u32_DMAAddr);
}

void nand_flush_miu_pipe(void)
{

}

#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);
static U16 fcie_events = 0;
static int fcie_int = 0;
static U16 u16CurNCMIEEvent = 0;			// Used to store current IRQ state

irqreturn_t NC_FCIE_IRQ(int irq, void *dummy)
{
	u16CurNCMIEEvent |= REG(NC_MIE_EVENT);					// Get current event
	REG_CLR_BITS_UINT16(NC_MIE_INT_EN, u16CurNCMIEEvent);	// Disable the IRQ which is invoked
	
	if( (u16CurNCMIEEvent & fcie_events) == fcie_events )
	{
		REG_READ_UINT16(NC_MIE_EVENT, u16CurNCMIEEvent);	// Read all the events
		REG_WRITE_UINT16(NC_MIE_EVENT, u16CurNCMIEEvent);	// Clear all the events
		u16CurNCMIEEvent = 0;								// Reset the current IRQ state
		fcie_int = 1;
		wake_up(&fcie_wait);
	}

	return IRQ_HANDLED;
}

U32 nand_WaitCompleteIntr(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	U16 u16_Reg;
	U32 u32_Count = 1; // Return microsec if timout
	U32 u32_Timeout = ((u32_MicroSec/1000) > 0) ? (u32_MicroSec/1000) : 1;	// timeout time

	fcie_events = u16_WaitEvent;
	REG_WRITE_UINT16(NC_MIE_INT_EN, u16_WaitEvent);
		
	if( wait_event_timeout(fcie_wait, (fcie_int == 1), u32_Timeout) == 0 ) // wait at least 2 second for FCIE3 events
	{
		u32_Count = u32_MicroSec;
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);		// Read all events
		REG_WRITE_UINT16(NC_MIE_EVENT, u16_Reg);	// Clear all events
	}
			
	fcie_int = 0;
	
	return UNFD_ST_SUCCESS;
}

void nand_enable_intr_mode(void)
{
	request_irq(13, NC_FCIE_IRQ, SA_INTERRUPT, "fcie", NULL);
}
#endif

U32 NC_PlatformResetPre(void)
{
	*((volatile unsigned short *)(0x25007DCC))|=0x02;	// emi mask
	*((volatile unsigned short *)(0x25007C18))|=0x02;	// imi0 mask
	*((volatile unsigned short *)(0x25007C58))|=0x02;	// imi1 mask
	
	return UNFD_ST_SUCCESS;
}

U32 NC_PlatformResetPost(void)
{
	*((volatile unsigned short *)(0x25007DCC))&=(~0x02);	// emi unmask
	*((volatile unsigned short *)(0x25007C18))&=(~0x02);	// imi0 unmask
	*((volatile unsigned short *)(0x25007C58))&=(~0x02);	// imi1 unmask
	
	return UNFD_ST_SUCCESS;
}

static UNFD_ALIGN0 NAND_DRIVER sg_NandDrv UNFD_ALIGN1;
static UNFD_ALIGN0 U32 gau32_PartInfo[NAND_PARTITAION_BYTE_CNT/4] UNFD_ALIGN1;

void *drvNAND_get_DrvContext_address(void) // exposed API
{
    return (void*)&sg_NandDrv;
}

void *drvNAND_get_DrvContext_PartInfo(void)
{
    return (void*)((U32)gau32_PartInfo);
}

static UNFD_ALIGN0 U32 drvNandPageBuf[8192/4] UNFD_ALIGN1; /* 8192 Bytes */
static UNFD_ALIGN0 U32 drvNandSpareBuf[448/4] UNFD_ALIGN1; /*   448 Bytes */

U32 NC_PlatformInit(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U16 u16_reg;
	U32 u32_ChipId;
	
	pNandDrv->u8_WordMode = 1;

	nand_pads_switch(1);
	
	pNandDrv->u32_Clk = FCIE3_SW_DEFAULT_CLK;
	nand_debug(UNFD_DEBUG_LEVEL, 1 ,"Nand Clk=%d\r\n", pNandDrv->u32_Clk);
	nand_pads_switch(1);	
	nand_clock_setting(pNandDrv->u32_Clk);

	pNandDrv->PlatCtx_t.pu8_PageDataBuf= (U8*)drvNandPageBuf;
	pNandDrv->PlatCtx_t.pu8_PageSpareBuf = (U8*)drvNandSpareBuf;
	
	pNandDrv->u8_SwPatchWaitRb= 0;
	pNandDrv->u8_SwPatchJobStart= 0;

	NC_ConfigHal(NAND_HAL_RAW);
	
	return UNFD_ST_SUCCESS;
}
#else
  #error "Error! no platform functions."
#endif

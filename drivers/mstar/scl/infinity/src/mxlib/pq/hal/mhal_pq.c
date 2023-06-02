////////////////////////////////////////////////////////////////////////////////
// $Change: 617839 $
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#define MHAL_PQ_C


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <string.h>
#endif

#include "MsCommon.h"
#include "MsOS.h"

// Internal Definition
#include "hwreg_utility2.h"
#include "hwreg.h"
#include "drvPQ_Define.h"

#include "Infinity_Main.h"             // table config parameter


#include "drvPQ_Datatypes.h"
#include "mhal_pq.h"
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#ifndef UNUSED //to avoid compile warnings...
#define UNUSED(var) (void)((var) = (var))
#endif

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
MS_U32 PQ_RIU_BASE;

// Put this function here because hwreg_utility2 only for hal.
void Hal_PQ_init_riu_base(MS_U32 u32riu_base)
{
    PQ_RIU_BASE = u32riu_base;
}

#if (PQ_ENABLE_UNUSED_FUNC)

MS_U8 Hal_PQ_get_sync_flag(PQ_WIN ePQWin)
{
    return 0;
}


MS_U8 Hal_PQ_get_input_vsync_value(PQ_WIN ePQWin)
{
    return (Hal_PQ_get_sync_flag(ePQWin) & 0x04) ? 1:0;
}

MS_U8 Hal_PQ_get_output_vsync_value(PQ_WIN ePQWin)
{
    return (Hal_PQ_get_sync_flag(ePQWin) & 0x01) ? 1 : 0;
}

MS_U8 Hal_PQ_get_input_vsync_polarity(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_memfmt_doublebuffer(PQ_WIN ePQWin, MS_BOOL bEn)
{

}
#endif

void Hal_PQ_set_sourceidx(PQ_WIN ePQWin, MS_U16 u16Idx)
{

}

#if (PQ_ENABLE_UNUSED_FUNC)

void Hal_PQ_set_mem_fmt(PQ_WIN ePQWin, MS_U16 u16val, MS_U16 u16Mask)
{

}

void Hal_PQ_set_mem_fmt_en(PQ_WIN ePQWin, MS_U16 u16val, MS_U16 u16Mask)
{

}

void Hal_PQ_set_420upSample(PQ_WIN ePQWin, MS_U16 u16value)
{

}

void Hal_PQ_set_force_y_motion(PQ_WIN ePQWin, MS_U16 u16value)
{

}

MS_U8 Hal_PQ_get_force_y_motion(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_force_c_motion(PQ_WIN ePQWin, MS_U16 u16value)
{

}

MS_U8 Hal_PQ_get_force_c_motion(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_dipf_temporal(PQ_WIN ePQWin, MS_U16 u16val)
{
}

MS_U16 Hal_PQ_get_dipf_temporal(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_dipf_spatial(PQ_WIN ePQWin, MS_U16 u16val)
{
}

MS_U8 Hal_PQ_get_dipf_spatial(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_vsp_sram_filter(PQ_WIN ePQWin, MS_U8 u8vale)
{

}

MS_U8 Hal_PQ_get_vsp_sram_filter(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_dnr(PQ_WIN ePQWin, MS_U8 u8val)
{

}

MS_U8 Hal_PQ_get_dnr(PQ_WIN ePQWin)
{
    return 0;
}


void Hal_PQ_set_presnr(PQ_WIN ePQWin, MS_U8 u8val)
{
}

MS_U8 Hal_PQ_get_presnr(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_film(PQ_WIN ePQWin, MS_U16 u16val)
{

}

MS_U8 Hal_PQ_get_film(PQ_WIN ePQWin)
{
    return 0;
}
#endif

void Hal_PQ_set_yc_sram(MS_U8 enID, MS_U8 u8sramtype, void *pData)
{
    MS_U32 u32reg_41, u32reg_42, u32reg_43;
    MS_U32 u32Addr;
    MS_U8 u8Ramcode[10];
    MS_U16 u16IdxBase = 0;
    MS_U16 i, j, x;
    MS_BOOL bC_SRAM;

#ifdef C3_SIM
    return;
#endif

    if(enID == FILTER_SRAM_SC2)
    {
        u32reg_41 = REG_SCL_HVSP1_41_L;
        u32reg_42 = REG_SCL_HVSP1_42_L;
        u32reg_43 = REG_SCL_HVSP1_43_L;
    }
    else if(enID == FILTER_SRAM_SC3)
    {
        u32reg_41 = REG_SCL_HVSP2_41_L;
        u32reg_42 = REG_SCL_HVSP2_43_L;
        u32reg_43 = REG_SCL_HVSP2_43_L;
    }
    else
    {
        u32reg_41 = REG_SCL_HVSP0_41_L;
        u32reg_42 = REG_SCL_HVSP0_42_L;
        u32reg_43 = REG_SCL_HVSP0_43_L;
    }

    bC_SRAM = u8sramtype >= SC_FILTER_C_SRAM1 ? TRUE : FALSE;

    if(u8sramtype == SC_FILTER_Y_SRAM1 || u8sramtype == SC_FILTER_C_SRAM1)
    {
        u16IdxBase = 0x00;
    }
    else if(u8sramtype == SC_FILTER_Y_SRAM2 || u8sramtype == SC_FILTER_C_SRAM2)
    {
        u16IdxBase = 0x40;
    }
    else if(u8sramtype == SC_FILTER_Y_SRAM3 || u8sramtype == SC_FILTER_C_SRAM3)
    {
        u16IdxBase = 0x80;
    }
    else
    {
        u16IdxBase = 0xC0;
    }

    u32Addr = (MS_U32)pData;

    W2BYTEMSK(u32reg_41, bC_SRAM ? BIT1 : BIT0, BIT1|BIT0); // reg_cram_rw_en

    for(i=0; i<64; i++)
    {
        while(MApi_XC_R2BYTE(u32reg_41) & BIT8);
        j=i*5;

        MApi_XC_W2BYTEMSK(u32reg_42,(i|u16IdxBase), 0x00FF);
        for ( x=0;x<5;x++ )
        {
            u8Ramcode[x] = *((MS_U8 *)(u32Addr + (j+x)));
        }

        MApi_XC_W2BYTEMSK(u32reg_43+0x00, (((MS_U16)u8Ramcode[1])<<8|(MS_U16)u8Ramcode[0]), 0xFFFF);
        MApi_XC_W2BYTEMSK(u32reg_43+0x02, (((MS_U16)u8Ramcode[3])<<8|(MS_U16)u8Ramcode[2]), 0xFFFF);
        MApi_XC_W2BYTEMSK(u32reg_43+0x04, ((MS_U16)u8Ramcode[4]), 0x00FF);


        MApi_XC_W2BYTEMSK(u32reg_41, BIT8, BIT8);
    }
}

void Hal_PQ_set_sram_color_index_table(PQ_WIN ePQWin, MS_U8 u8sramtype, void *pData)
{
    //ToDo

}

void Hal_PQ_set_sram_color_gain_snr_table(PQ_WIN ePQWin, MS_U8 u8sramtype, void *pData)
{

}


void Hal_PQ_set_sram_color_gain_dnr_table(PQ_WIN ePQWin, MS_U8 u8sramtype, void *pData)
{

}

#define ICC_H_SIZE	32
#define ICC_V_SIZE	32
void Hal_PQ_set_sram_icc_crd_table(PQ_WIN ePQWin, MS_U8 u8sramtype, void *pData)
{
    MS_U16 i;
    MS_U16 u16Ramcode_L,u16Ramcode_H;
    MS_U16 u16Ramcode;
    MS_U32 u32Addr;
    MS_U32 u32BK18_78, u32BK18_79, u32BK18_7A;

    if(u8sramtype != SC_FILTER_SRAM_ICC_CRD)
    {
        return;
    }

    if(ePQWin == PQ_MAIN_WINDOW)
    {
        u32BK18_78 = REG_VIP_ACE2_78_L;
        u32BK18_79 = REG_VIP_ACE2_79_L;
        u32BK18_7A = REG_VIP_ACE2_7A_L;
    }
    else
    {
        return;
    }

    u32Addr = (MS_U32)pData;

    MApi_XC_W2BYTEMSK(u32BK18_78, BIT(0), BIT(0)); // io_en disable

    for(i=0; i<(ICC_H_SIZE*ICC_V_SIZE); i++)
    {
        u16Ramcode_L = *((MS_U8 *)(u32Addr + (2*i)));
        u16Ramcode_H = *((MS_U8 *)(u32Addr + (2*i+1)));
        u16Ramcode = ((u16Ramcode_H & 0x01)<<8)+u16Ramcode_L;

        //while (MApi_XC_R2BYTE(u32BK18_7A) & BIT(15));
        MApi_XC_W2BYTEMSK(u32BK18_79, i, 0x03FF); // address
        MApi_XC_W2BYTEMSK(u32BK18_7A, (u16Ramcode & 0x1FF), 0x01FF); //data[8:0]
        MApi_XC_W2BYTEMSK(u32BK18_7A, BIT(15), BIT(15)); // io_w enable

    }

    MApi_XC_W2BYTEMSK(u32BK18_78, 0, BIT(0)); // io_en enable
}


#define IHC_H_SIZE	33
#define IHC_V_SIZE	33
#define BUF_BYTE_SIZE (IHC_H_SIZE*IHC_V_SIZE)
#define MAX_SRAM_SIZE 0x124

#define SRAM1_IHC_COUNT 289//81
#define SRAM2_IHC_COUNT 272//72
#define SRAM3_IHC_COUNT 272//72
#define SRAM4_IHC_COUNT 256//64

#define SRAM1_IHC_OFFSET 0
#define SRAM2_IHC_OFFSET SRAM1_IHC_COUNT
#define SRAM3_IHC_OFFSET (SRAM2_IHC_OFFSET + SRAM2_IHC_COUNT)
#define SRAM4_IHC_OFFSET (SRAM3_IHC_OFFSET + SRAM3_IHC_COUNT)

#define SRAM_IHC_TOTAL_COUNT SRAM1_IHC_COUNT + SRAM2_IHC_COUNT + SRAM3_IHC_COUNT + SRAM4_IHC_COUNT


void _Hal_PQ_set_sram_ihc_crd_table(PQ_WIN ePQWin, MS_U16 *pBuf, MS_U8 u8SRAM_Idx, MS_U16 u16Cnt)
{
    MS_U16 i;
    MS_U32 u32BK18_7C, u32BK18_7D, u32BK18_7E;

    if(u8SRAM_Idx > 3)
    {
        u8SRAM_Idx = 0;
    }

    if(ePQWin == PQ_MAIN_WINDOW)
    {
        u32BK18_7C = REG_VIP_ACE2_7C_L;
        u32BK18_7D = REG_VIP_ACE2_7D_L;
        u32BK18_7E = REG_VIP_ACE2_7E_L;
    }
    else
    {
        return;
    }


    MApi_XC_W2BYTEMSK(u32BK18_7C, BIT(0), BIT(0)); // io_en disable
    MApi_XC_W2BYTEMSK(u32BK18_7C, u8SRAM_Idx<<1, BIT(2)|BIT(1)); // sram select

    for(i=0; i<u16Cnt; i++)
    {
        MApi_XC_W2BYTEMSK(u32BK18_7D, i, 0x01FF); // address
        MApi_XC_W2BYTEMSK(u32BK18_7E, pBuf[i], 0x01FF); //data

        MApi_XC_W2BYTEMSK(u32BK18_7E, BIT(15), BIT(15)); // io_w enable
    }

    MApi_XC_W2BYTEMSK(u32BK18_7C, 0, BIT(0)); // io_en enable
}

MS_U16 PQ_SRAM1_IHC[MAX_SRAM_SIZE];
MS_U16 PQ_SRAM2_IHC[MAX_SRAM_SIZE];
MS_U16 PQ_SRAM3_IHC[MAX_SRAM_SIZE];
MS_U16 PQ_SRAM4_IHC[MAX_SRAM_SIZE];

void Hal_PQ_set_sram_ihc_crd_table(PQ_WIN ePQWin, MS_U8 u8sramtype, void *pData)
{
    // New method for download IHC
    MS_U16* u32Addr = (MS_U16*)pData;
    MS_U16 u16Index = 0 ;
    MS_U16 cont1=0, cont2=0, cont3=0, cont4=0;

    if(u8sramtype != SC_FILTER_SRAM_IHC_CRD)
    {
        return;
    }

    for (;u16Index < SRAM_IHC_TOTAL_COUNT ; u16Index++)
    {
        if (u16Index < SRAM2_IHC_OFFSET)
        {
            PQ_SRAM1_IHC[cont1++] = u32Addr[u16Index];
        }
        else if (u16Index < SRAM3_IHC_OFFSET)
        {
            PQ_SRAM2_IHC[cont2++] = u32Addr[u16Index];
        }
        else if (u16Index < SRAM4_IHC_OFFSET)
        {
            PQ_SRAM3_IHC[cont3++] = u32Addr[u16Index];
        }
        else
        {
            PQ_SRAM4_IHC[cont4++] = u32Addr[u16Index];
        }
    }

    _Hal_PQ_set_sram_ihc_crd_table(ePQWin, &PQ_SRAM1_IHC[0], 0, SRAM1_IHC_COUNT);
    _Hal_PQ_set_sram_ihc_crd_table(ePQWin, &PQ_SRAM2_IHC[0], 1, SRAM2_IHC_COUNT);
    _Hal_PQ_set_sram_ihc_crd_table(ePQWin, &PQ_SRAM3_IHC[0], 2, SRAM3_IHC_COUNT);
    _Hal_PQ_set_sram_ihc_crd_table(ePQWin, &PQ_SRAM4_IHC[0], 3, SRAM4_IHC_COUNT);

}


#if (PQ_ENABLE_UNUSED_FUNC)
MS_U16 Hal_PQ_get_420_cup_idx(MS_420_CUP_TYPE etype)
{
    return 0xFFFF;
}

MS_U16 Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_TYPE etype)
{
    return 0xFFFF;
}


MS_U16 Hal_PQ_get_madi_idx(PQ_WIN ePQWin, MS_MADI_TYPE etype)
{
    return 0xFFFF;
}
#endif


MS_U16 Hal_PQ_get_sram_size(MS_U16 u16sramtype)
{
    MS_U16 u16ret = 0;

    switch(u16sramtype)
    {
    case SC_FILTER_Y_SRAM1:
        u16ret = PQ_IP_SRAM1_SIZE_Main;
        break;

    case SC_FILTER_Y_SRAM2:
        u16ret = PQ_IP_SRAM2_SIZE_Main;
        break;

    case SC_FILTER_Y_SRAM3:
        u16ret = PQ_IP_SRAM3_SIZE_Main;
        break;

    case SC_FILTER_Y_SRAM4:
        u16ret = PQ_IP_SRAM4_SIZE_Main;
        break;

    case SC_FILTER_C_SRAM1:
        u16ret = PQ_IP_C_SRAM1_SIZE_Main;
        break;

    case SC_FILTER_C_SRAM2:
        u16ret = PQ_IP_C_SRAM2_SIZE_Main;
        break;

    case SC_FILTER_C_SRAM3:
        u16ret = PQ_IP_C_SRAM3_SIZE_Main;
        break;

    case SC_FILTER_C_SRAM4:
        u16ret = PQ_IP_C_SRAM4_SIZE_Main;
        break;

    case SC_FILTER_SRAM_ICC_CRD:
        u16ret = PQ_IP_VIP_ICC_CRD_SRAM_SIZE_Main;
        break;

    case SC_FILTER_SRAM_IHC_CRD:
        u16ret = PQ_IP_VIP_IHC_CRD_SRAM_SIZE_Main;
        break;

    default:
        u16ret = 0;
        break;
    }

    return u16ret;
}

#if (PQ_ENABLE_UNUSED_FUNC)

MS_U16 Hal_PQ_get_ip_idx(MS_PQ_IP_TYPE eType)
{
    return 0xFFFF;
}


MS_U16 Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_TYPE eType)
{
    return 0xFFFF;
}

void Hal_PQ_OPWriteOff_Enable(MS_BOOL bEna)
{

}

MS_U16 Hal_PQ_get_adc_sampling_idx(MS_ADC_SAMPLING_TYPE eType)
{
    return 0xFFFF;

}

MS_U16 Hal_PQ_get_rw_method(PQ_WIN ePQWin)
{
    return 0;
}

void Hal_PQ_set_sram_xvycc_gamma_table(MS_U8 u8sramtype, void *pData)
{
}

void Hal_PQ_set_rw_method(PQ_WIN ePQWin, MS_U16 u16method)
{
}

MS_BOOL HAL_PQ_user_mode_control(void)
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
// set equation to VIP CSC
// Both equation selection rules are
// 0: SDTV(601) R  G  B  : 16-235
// 1: SDTV(601) R  G  B  : 0-255
/// @param bMainWin       \b IN: Enable
//-------------------------------------------------------------------------------------------------
void HAL_PQ_set_SelectCSC(MS_U16 u16selection, PQ_WIN ePQWin)
{

}

MS_U8 Hal_PQ_get_madi_fbl_mode(MS_BOOL bMemFmt422,MS_BOOL bInterlace)
{
    return 0;
}

MS_U16 Hal_PQ_get_csc_ip_idx(MS_CSC_IP_TYPE enCSC)
{
    return 0;
}

MS_U16 Hal_PQ_get_422To444_idx(MS_422To444_TYPE etype)
{

    return 0;
}

MS_U8 Hal_PQ_get_422To444_mode(MS_BOOL bMemFmt422)
{
    return 0;
}
#endif

#ifndef FOTG2XX_EHCI_MACRO_H_CHK
#define FOTG2XX_EHCI_MACRO_H_CHK

#include "fotg2xx-config.h"
#include <asm/io.h>
/*-------------------------------------------------------------------------*/

#define H200_NAME "FUSBH200"
#define OTG2XX_NAME "FOTG2XX"

void fotg200_handle_irq(int irq);

#if defined(CONFIG_GM_FOTG2XX)
  /* FOTG2XX Global Controller Register Macro Definition */
  //<1>.Macro volatile
  #define mwOTG20Port(reg,bOffset)	            *((volatile u32 __force *)((u32)reg|bOffset))
  #define mwOTG20Bit_Rd(reg,bByte,wBitNum)            (mwOTG20Port(reg,bByte)&wBitNum)
  #define mwOTG20Bit_Set(reg,bByte,wBitNum)           (mwOTG20Port(reg,bByte)|=wBitNum)
  #define mwOTG20Bit_Clr(reg,bByte,wBitNum)           (mwOTG20Port(reg,bByte)&=~wBitNum)
  // 0x0C0(Interrupt Status)
  #define mwOTG20_Interrupt_Status_HOST_Rd(x)       (mwOTG20Bit_Rd(x,0xC0,0x00000004))
  #define mwOTG20_Interrupt_Status_HOST_Clr(x)	   (mwOTG20Port(x,0xC0)= 0x00000004)

  // 0x0C4(Interrupt Mask)
  #define mwOTG20_Interrupt_Mask_HOST_Clr(x)	    mwOTG20Bit_Clr(x,0xC4,0x00000004)

  //0x100(Device Controller Registers(Address = 100h~1FFh) )
  #define mwPeri20_Control_ChipEnable_Set(x)         mwOTG20Bit_Set(x,0x100,BIT5)
  #define mwPeri20_Control_HALFSPEEDEnable_Set(x)    mwOTG20Bit_Set(x,0x100,BIT1)
  #define mwPeri20_Control_HALFSPEEDEnable_Clr(x)    mwOTG20Bit_Clr(x,0x100,BIT1)
  #define mwPeri20_Control_ForceDeviceFS_Set(x)    mwOTG20Bit_Set(x,0x100,BIT9)
  #define mwPeri20_Control_ForceDeviceFS_Clr(x)    mwOTG20Bit_Clr(x,0x100,BIT9)
  //0x80 <OTG Controller Register>
  #define mwOTG20_Control_ForceFullSpeed_Set(x)      mwOTG20Bit_Set(x,0x80,BIT12)
  #define mwOTG20_Control_ForceHighSpeed_Set(x)      (mwOTG20Bit_Set(x,0x80,BIT14))
  #define mwOTG20_Control_ForceFullSpeed_Clr(x)      mwOTG20Bit_Clr(x,0x80,BIT12)
  #define mwOTG20_Control_ForceHighSpeed_Clr(x)      (mwOTG20Bit_Clr(x,0x80,BIT14))

  #ifdef CONFIG_711MA_PHY
     // Cover 711MA PHY (Full speed reset issue)
     #define mwOTG20_Control_COVER_FS_PHY_Reset_Set(x)  mwOTG20Bit_Set(x,0x80,BIT28)
     #define mwOTG20_Control_COVER_FS_PHY_Reset_Clr(x)  mwOTG20Bit_Clr(x,0x80,BIT28)
  #endif
#endif /* CONFIG_GM_FOTG2XX */

#ifdef CONFIG_GM_FUSBH200
  /* FOTG2XX Global Controller Register Macro Definition */
  //<1>.Macro volatile
  #define mwH20Port(reg,bOffset)               *((volatile u32 __force *)((u32)reg | bOffset))
  #define mwH20Bit_Rd(x,bByte,wBitNum)            (mwH20Port(x,bByte)&wBitNum)
  #define mwH20Bit_Set(x,bByte,wBitNum)           (mwH20Port(x,bByte)|=wBitNum)
  #define mwH20Bit_Clr(x,bByte,wBitNum)           (mwH20Port(x,bByte)&=~wBitNum)

  // 0x40(Bus Monitor Control/Status)
  #define mwH20_Control_VBUS_FLT_SEL_Set(x)          mwH20Bit_Set(x,0x40,BIT0)
  #define mwH20_Control_VBUS_FLT_SEL_Clr(x)          mwH20Bit_Clr(x,0x40,BIT0)
  #define mwH20_Control_HDISCON_FLT_SEL_Set(x)       mwH20Bit_Set(x,0x40,BIT1)
  #define mwH20_Control_HDISCON_FLT_SEL_Clr(x)       mwH20Bit_Clr(x,0x40,BIT1)
  #define mwH20_Control_HALFSPEEDEnable_Set(x)       mwH20Bit_Set(x,0x40,BIT2)
  #define mwH20_Control_HALFSPEEDEnable_Clr(x)       mwH20Bit_Clr(x,0x40,BIT2)
  #define mwH20_Interrupt_OutPut_High_Set(x)         mwH20Bit_Set(x,0x40,BIT3)
  #define mwH20_Interrupt_OutPut_High_Clr(x)         mwH20Bit_Clr(x,0x40,BIT3)
  #define mwH20_Control_VBUS_OFF_Set(x)              mwH20Bit_Set(x,0x40,BIT4)
  #define mwH20_Control_VBUS_OFF_Clr(x)              mwH20Bit_Clr(x,0x40,BIT4)
  #define mwH20_Control_Phy_Reset_Set(x)             mwH20Bit_Set(x,0x40,BIT5)
  #define mwH20_Control_Phy_Reset_Clr(x)             mwH20Bit_Clr(x,0x40,BIT5)
  #define mwH20_Control_ForceFullSpeed_Set(x)        mwH20Bit_Set(x,0x40,BIT7)
  #define mwH20_Control_ForceHighSpeed_Set(x)        mwH20Bit_Set(x,0x40,BIT6)
  #define mwH20_Control_ForceFullSpeed_Clr(x)        mwH20Bit_Clr(x,0x40,BIT7)
  #define mwH20_Control_ForceHighSpeed_Clr(x)        mwH20Bit_Clr(x,0x40,BIT6)
  #define mwH20_Control_VBUS_VLD_Rd(x)               mwH20Bit_Rd(x,0x40,BIT8)
  #define mwH20_Control_HOST_SPD_TYP_Rd(x)           mwH20Bit_Rd(x,0x40,(BIT9|BIT10))
  #ifdef CONFIG_711MA_PHY
     // Cover 711MA PHY (Full speed reset issue)
     #define mwH20_Control_COVER_FS_PHY_Reset_Set(x)    mwH20Bit_Set(x,0x40,BIT12)
     #define mwH20_Control_COVER_FS_PHY_Reset_Clr(x)    mwH20Bit_Clr(x,0x40,BIT12)
  #endif

  // 0x044(Bus Monitor Interrupt Status)
  #define mwH20_Int_Status_BM_DMA_ERROR_Rd(x)       mwH20Bit_Rd(x,0x44,0x00000010)
  #define mwH20_Int_Status_BM_DMA_CMPLT_Rd(x)       mwH20Bit_Rd(x,0x44,0x00000008)
  #define mwH20_Int_Status_BM_DPLGRMV_Rd(x)         mwH20Bit_Rd(x,0x44,0x00000004)
  #define mwH20_Int_Status_BM_OVC_Rd(x)             mwH20Bit_Rd(x,0x44,0x00000002)
  #define mwH20_Int_Status_BM_VBUS_ERR_Rd(x)        mwH20Bit_Rd(x,0x44,0x00000001)
  #define mwH20_Int_Status_BM_DMA_ERROR_Clr(x)      (mwH20Port(x,0x44)= 0x00000010)
  #define mwH20_Int_Status_BM_DMA_CMPLT_Clr(x)      (mwH20Port(x,0x44)= 0x00000008)
  #define mwH20_Int_Status_BM_DPLGRMV_Clr(x)        (mwH20Port(x,0x44)= 0x00000004)
  #define mwH20_Int_Status_BM_OVC_Clr(x)            (mwH20Port(x,0x44)= 0x00000002)
  #define mwH20_Int_Status_BM_VBUS_ERR_Clr(x)       (mwH20Port(x,0x44)= 0x00000001)
  // 0x048(Bus Monitor Interrupt Enable)
  #define mwH20_Int_BM_DMA_ERROR_En(x)              mwH20Bit_Set(x,0x48,0x00000010)
  #define mwH20_Int_BM_DMA_CMPLT_En(x)              mwH20Bit_Set(x,0x48,0x00000008)
  #define mwH20_Int_BM_BPLGRMV_En(x)                mwH20Bit_Set(x,0x48,0x00000004)
  #define mwH20_Int_BM_OVC_En(x)                    mwH20Bit_Set(x,0x48,0x00000002)
  #define mwH20_Int_BM_VBUS_ERR_En(x)               mwH20Bit_Set(x,0x48,0x00000001)
  #define mwH20_Int_BM_DMA_ERROR_Dis(x)             mwH20Bit_Clr(x,0x48,0x00000010)
  #define mwH20_Int_BM_DMA_CMPLT_Dis(x)             mwH20Bit_Clr(x,0x48,0x00000008)
  #define mwH20_Int_BM_DPLGRMV_Dis(x)               mwH20Bit_Clr(x,0x48,0x00000004)
  #define mwH20_Int_BM_OVC_Dis(x)                   mwH20Bit_Clr(x,0x48,0x00000002)
  #define mwH20_Int_BM_VBUS_ERR_Dis(x)              mwH20Bit_Clr(x,0x48,0x00000001)
  #define mwH20_Int_BM_DMA_ERROR_Rd(x)              mwH20Bit_Rd(x,0x48,0x00000010)
  #define mwH20_Int_BM_DMA_CMPLT_Rd(x)              mwH20Bit_Rd(x,0x48,0x00000008)
  #define mwH20_Int_BM_BPLGRMV_Rd(x)                mwH20Bit_Rd(x,0x48,0x00000004)
  #define mwH20_Int_BM_OVC_Rd(x)                    mwH20Bit_Rd(x,0x48,0x00000002)
  #define mwH20_Int_BM_VBUS_ERR_Rd(x)               mwH20Bit_Rd(x,0x48,0x00000001)
	// 0x50(Test)
  #define mwH20_Test_TST_JSTA_Set(x)                mwH20Bit_Set(x,0x50,BIT0)
  #define mwH20_Test_TST_JSTA_Clr(x)                mwH20Bit_Clr(x,0x50,BIT0)
  #define mwH20_Test_TST_KSTA_Set(x)                mwH20Bit_Set(x,0x50,BIT1)
  #define mwH20_Test_TST_KSTA_Clr(x)                mwH20Bit_Clr(x,0x50,BIT1)
  #define mwH20_Test_TST_PKT_Set(x)                 mwH20Bit_Set(x,0x50,BIT2)
  #define mwH20_Test_TST_PKT_Clr(x)                 mwH20Bit_Clr(x,0x50,BIT2)
  #define mwH20_Test_TST_MOD_Set(x)                 mwH20Bit_Set(x,0x50,BIT3)
  #define mwH20_Test_TST_MOD_Clr(x)                 mwH20Bit_Clr(x,0x50,BIT3)
  #define mwH20_Test_TST_LOOPBK_Set(x)              mwH20Bit_Set(x,0x50,BIT4)
  #define mwH20_Test_TST_LOOPBK_Clr(x)              mwH20Bit_Clr(x,0x50,BIT4)
  // 0x54(Vendor Specific I/O Control Register)
  #define mwH20_Test_VCTL_Rd(x)                     mwH20Bit_Rd(x,0x50,0x0000001F)
  #define mwH20_Test_VCTL_Set(x,Value)               mwH20Bit_Set(x,0x50,Value)
  #define mwH20_Test_VCTLOAD_N_Set(x)               mwH20Bit_Set(x,0x50,BIT5)
  #define mwH20_Test_VCTLOAD_N_Clr(x)               mwH20Bit_Clr(x,0x50,BIT5)

#endif /* CONFIG_GM_FUSBH200 */

#endif

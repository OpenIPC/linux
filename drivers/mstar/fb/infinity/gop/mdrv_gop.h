
/******************************************************************************
 *-----------------------------------------------------------------------------
 *
 *    Copyright (c) 2011 MStar Semiconductor, Inc.  All rights reserved.
 *
 *-----------------------------------------------------------------------------
 * FILE NAME      mdrv_gop.h
 * DESCRIPTION
 *          Including some MACRO needed in msb250xfb.c
 *                    Defined CLRREG16(), SETREG16()
 *          (refer to include/asm-arm/arch-msb25xx/io.h)
 *                    Defined used MASK and setting value
 *                    Defined CHIPTOP, OP2, VOP, GOP Base Addres in Physical Address
 *                    and theirs offset    (drvop2.h and drvgop.h)
 *                    Declared Boot_splash array. the image content shown in boot
 *          (boot_splasy.h)
 *            refering sources list
 *                    drvop2.h and drvgop.h
 *              boot_splasy.h
 *
 * AUTHOR         Chun Fan
 *
 * HISTORY
 *                2008/05/06    Chun    initial version
 *                2008/10/02    Chun    add MSB251x code, add CONFIG_ARCH_MSB251X
 *                                      add some IP base address Macro
 *                                      BK_LPLL_BASE, BK_DAC_BASE,
 *                                      and OP2_DITH_REG_BASE
 *
 *        linux/drivers/video/msb250xfb.h    --    msb250xfb frame buffer device
 ******************************************************************************/

#include <asm/io.h>
#include "../include/ms_types.h"
#include "../include/ms_platform.h"
#include <linux/pfn.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>               /* for dma_alloc_coherent */
#include <linux/slab.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>      /* for dma_alloc_coherent */
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/uaccess.h>            /* access_ok(), and VERIFY_WRITE/READ */
#include <linux/ioctl.h>            /* for _IO() macro */
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/time.h>             /* do_gettimeofday() */

#include "hal/gop_c2_reg.h"


#define CHIPTOP_REG_BASE_PA  (0x101E00)
#define SC_REG_BASE_PA       (0x130000)
#define LPLL_REG_BASE_PA     (0x103100)
#define CLKG0_REG_BASE_PA    (0x100B00)
#define MOD_REG_BASE_PA      (0x103200)
#define GPIO0_REG_BASE_PA    (0x102B00)
#define GPIO1_REG_BASE_PA    (0x101A00)

#define GOP_DEBUG 0
#if (GOP_DEBUG==1)
#define GOPDBG(fmt, arg...) printk(KERN_INFO fmt, ##arg)
#else
#define GOPDBG(fmt, arg...)
#endif


//#define  GOPIO_DEBUG_ENABLE

#define GOP_DBG_LV_IOCTL            0
#define GOP_DBG_LV_0                1


#ifdef   GOPIO_DEBUG_ENABLE
#define GOPIO_ASSERT(_con)   \
    do {\
        if (!(_con)) {\
            printk(KERN_CRIT "BUG at %s:%d assert(%s)\n",\
                    __FILE__, __LINE__, #_con);\
            BUG();\
        }\
    } while (0)

#define GOP_DBG(dbglv, _fmt, _args...)             \
    do                                             \
    if(dbglv)                                      \
    {                                              \
        printk(KERN_WARNING _fmt, ## _args);       \
    }while(0)

#else
#define     GOPIO_ASSERT(arg)
#define     GOP_DBG(dbglv, _fmt, _args...)

#endif

#define BK_REG(reg)                   ((reg) * 4)

/* ----------------------------------------------------------------------------
 * GOP_OFFSET
 * ----------------------------------------------------------------------------
 */
#define REG_GOP_00                BK_REG(0x00)
#define REG_GOP_01                BK_REG(0x01)
#define REG_GOP_02                BK_REG(0x02)
#define REG_GOP_03                BK_REG(0x03)
#define REG_GOP_04                BK_REG(0x04)
#define REG_GOP_05                BK_REG(0x05)
#define REG_GOP_06                BK_REG(0x06)
#define REG_GOP_07                BK_REG(0x07)
#define REG_GOP_08                BK_REG(0x08)
#define REG_GOP_09                BK_REG(0x09)
#define REG_GOP_0A                BK_REG(0x0A)
#define REG_GOP_0B                BK_REG(0x0B)
#define REG_GOP_0C                BK_REG(0x0C)
#define REG_GOP_0D                BK_REG(0x0D)
#define REG_GOP_0E                BK_REG(0x0E)
#define REG_GOP_0F                BK_REG(0x0F)


#define REG_GOP_10                BK_REG(0x10)
#define REG_GOP_11                BK_REG(0x11)
#define REG_GOP_12                BK_REG(0x12)
#define REG_GOP_13                BK_REG(0x13)
#define REG_GOP_14                BK_REG(0x14)
#define REG_GOP_15                BK_REG(0x15)
#define REG_GOP_16                BK_REG(0x16)
#define REG_GOP_17                BK_REG(0x17)
#define REG_GOP_18                BK_REG(0x18)
#define REG_GOP_19                BK_REG(0x19)
#define REG_GOP_1A                BK_REG(0x1A)
#define REG_GOP_1B                BK_REG(0x1B)
#define REG_GOP_1C                BK_REG(0x1C)
#define REG_GOP_1D                BK_REG(0x1D)
#define REG_GOP_1E                BK_REG(0x1E)
#define REG_GOP_1F                BK_REG(0x1F)

#define REG_GOP_20                BK_REG(0x20)
#define REG_GOP_21                BK_REG(0x21)
#define REG_GOP_22                BK_REG(0x22)
#define REG_GOP_23                BK_REG(0x23)
#define REG_GOP_24                BK_REG(0x24)
#define REG_GOP_25                BK_REG(0x25)
#define REG_GOP_26                BK_REG(0x26)
#define REG_GOP_27                BK_REG(0x27)
#define REG_GOP_28                BK_REG(0x28)
#define REG_GOP_29                BK_REG(0x29)
#define REG_GOP_2A                BK_REG(0x2A)
#define REG_GOP_2B                BK_REG(0x2B)
#define REG_GOP_2C                BK_REG(0x2C)
#define REG_GOP_2D                BK_REG(0x2D)
#define REG_GOP_2E                BK_REG(0x2E)
#define REG_GOP_2F                BK_REG(0x2F)


#define REG_GOP_30                BK_REG(0x30)
#define REG_GOP_31                BK_REG(0x31)
#define REG_GOP_32                BK_REG(0x32)
#define REG_GOP_33                BK_REG(0x33)
#define REG_GOP_34                BK_REG(0x34)
#define REG_GOP_35                BK_REG(0x35)
#define REG_GOP_36                BK_REG(0x36)
#define REG_GOP_37                BK_REG(0x37)
#define REG_GOP_38                BK_REG(0x38)
#define REG_GOP_39                BK_REG(0x39)
#define REG_GOP_3A                BK_REG(0x3A)
#define REG_GOP_3B                BK_REG(0x3B)
#define REG_GOP_3C                BK_REG(0x3C)
#define REG_GOP_3D                BK_REG(0x3D)
#define REG_GOP_3E                BK_REG(0x3E)
#define REG_GOP_3F                BK_REG(0x3F)


#define REG_GOP_40                BK_REG(0x40)
#define REG_GOP_41                BK_REG(0x41)
#define REG_GOP_42                BK_REG(0x42)
#define REG_GOP_43                BK_REG(0x43)
#define REG_GOP_44                BK_REG(0x44)
#define REG_GOP_45                BK_REG(0x45)
#define REG_GOP_46                BK_REG(0x46)
#define REG_GOP_47                BK_REG(0x47)
#define REG_GOP_48                BK_REG(0x48)
#define REG_GOP_49                BK_REG(0x49)
#define REG_GOP_4A                BK_REG(0x4A)
#define REG_GOP_4B                BK_REG(0x4B)
#define REG_GOP_4C                BK_REG(0x4C)
#define REG_GOP_4D                BK_REG(0x4D)
#define REG_GOP_4E                BK_REG(0x4E)
#define REG_GOP_4F                BK_REG(0x4F)


#define REG_GOP_50                BK_REG(0x50)
#define REG_GOP_51                BK_REG(0x51)
#define REG_GOP_52                BK_REG(0x52)
#define REG_GOP_53                BK_REG(0x53)
#define REG_GOP_54                BK_REG(0x54)
#define REG_GOP_55                BK_REG(0x55)
#define REG_GOP_56                BK_REG(0x56)
#define REG_GOP_57                BK_REG(0x57)
#define REG_GOP_58                BK_REG(0x58)
#define REG_GOP_59                BK_REG(0x59)
#define REG_GOP_5A                BK_REG(0x5A)
#define REG_GOP_5B                BK_REG(0x5B)
#define REG_GOP_5C                BK_REG(0x5C)
#define REG_GOP_5D                BK_REG(0x5D)
#define REG_GOP_5E                BK_REG(0x5E)
#define REG_GOP_5F                BK_REG(0x5F)


#define REG_GOP_60                BK_REG(0x60)
#define REG_GOP_61                BK_REG(0x61)
#define REG_GOP_62                BK_REG(0x62)
#define REG_GOP_63                BK_REG(0x63)
#define REG_GOP_64                BK_REG(0x64)
#define REG_GOP_65                BK_REG(0x65)
#define REG_GOP_66                BK_REG(0x66)
#define REG_GOP_67                BK_REG(0x67)
#define REG_GOP_68                BK_REG(0x68)
#define REG_GOP_69                BK_REG(0x69)
#define REG_GOP_6A                BK_REG(0x6A)
#define REG_GOP_6B                BK_REG(0x6B)
#define REG_GOP_6C                BK_REG(0x6C)
#define REG_GOP_6D                BK_REG(0x6D)
#define REG_GOP_6E                BK_REG(0x6E)
#define REG_GOP_6F                BK_REG(0x6F)


#define REG_GOP_70                BK_REG(0x70)
#define REG_GOP_71                BK_REG(0x71)
#define REG_GOP_72                BK_REG(0x72)
#define REG_GOP_73                BK_REG(0x73)
#define REG_GOP_74                BK_REG(0x74)
#define REG_GOP_75                BK_REG(0x75)
#define REG_GOP_76                BK_REG(0x76)
#define REG_GOP_77                BK_REG(0x77)
#define REG_GOP_78                BK_REG(0x78)
#define REG_GOP_79                BK_REG(0x79)
#define REG_GOP_7A                BK_REG(0x7A)
#define REG_GOP_7B                BK_REG(0x7B)
#define REG_GOP_7C                BK_REG(0x7C)
#define REG_GOP_7D                BK_REG(0x7D)
#define REG_GOP_7E                BK_REG(0x7E)
#define REG_GOP_7F                BK_REG(0x7F)

void SwitchSubBank(int subbank);

void GOP_Setmode(int bResume, int Screen,int Panel_Width,int  Panel_Height,U32  FB_Start_Addr,U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut);
void GOP_Setmode_Stretch_H(int bResume, int Screen,int Panel_Width,int  Panel_Height,U32  FB_Start_Addr,U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut, int StretchRatio);

void GOP_Change_Alpha(int bResume, int Screen,int Panel_Width,int  Panel_Height,U32  FB_Start_Addr,U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut);

void GOP_Pan_Display(int Screen, U32 FB_Start_Addr);
void GOP_Set_OutFormat(int screen, int byuvoutput);
void GOP_Set_Constant_Alpha(int screen, int bEn, int alpha);
void GOP_Set_Color_Key(int screen, int bEn, int u8R, int u8G, int u8B);
void GOP_Set_PipeDelay(int screen, int delay);
void GOP_Setmode_and_win(int bResume, int Screen, int H_Start, int V_Start, int H_End, int V_End, int Panel_Width, int Panel_Height, U32 FB_Start_Addr,U32 Pixel_Format,U16 Bytes_Per_Pixel, int bInterlaceOut);

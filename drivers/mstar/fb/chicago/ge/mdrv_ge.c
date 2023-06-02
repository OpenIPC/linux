////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (��MStar Confidential Information��) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// file    devGE.c
/// @author MStar Semiconductor Inc.
/// @brief  GE Device Driver
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <linux/module.h>
#include <linux/fs.h>    // for MKDEV()
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include "mach/ms_types.h"
#include "mhal_ge.h"
#include "mdrv_ge.h"

#define MDRV_NAME_GE                    "ge"
#define MDRV_MAJOR_GE                   124
#define MDRV_MINOR_GE                   1

static int MDrv_GE_Open(struct inode *inode, struct file *filp);
static int MDrv_GE_Release(struct inode *inode, struct file *filp);
static long MDrv_GE_IOCtl(struct file *filp, unsigned int cmd, unsigned long arg);

//------------------------------------------------------------------------------
// structure
//------------------------------------------------------------------------------
struct file_operations _ge_fops =
{
    .owner =    THIS_MODULE,
    .unlocked_ioctl = MDrv_GE_IOCtl,
    .open =     MDrv_GE_Open,
    .release =  MDrv_GE_Release,
};

// -----------------------------------------------------------------------------
// Global variable
// -----------------------------------------------------------------------------
struct cdev *g_pGEDev = NULL;
struct class *g_pGEClass = NULL;

// -----------------------------------------------------------------------------
// Local function
// -----------------------------------------------------------------------------



//------------------------------------------------------------------------------
//                     GE & PE Function
//------------------------------------------------------------------------------

void MDrv_GE_Init(unsigned long arg)
{
    MHal_GE_Init();
}

void MDrv_GE_PowerOff(unsigned long arg)
{
    MHal_GE_PowerOff();
}

int MDrv_GE_ScreenCopy(unsigned long arg)
{
    GE_BLOCK geSrcBlk, geDstBlk;

    if (copy_from_user(&geSrcBlk, ((MS_GE_SCREEN_COPY __user *)arg)->pSrcBlk, sizeof(GE_BLOCK)))
        return EFAULT;

    if (copy_from_user(&geDstBlk, ((MS_GE_SCREEN_COPY __user *)arg)->pDstBlk, sizeof(GE_BLOCK)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_ScreenCopy(&geSrcBlk, &geDstBlk))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_GetFrameBufferInfo(unsigned long arg)
{
    U32 u32Width;
    U32 u32Height;
    U32 u32Pitch;
    U32 u32FbFmt;
    U32 u32Addr;

    if (GESTATUS_SUCCESS == MHal_GE_GetFrameBufferInfo(&u32Width, &u32Height, &u32Pitch, &u32FbFmt, &u32Addr))
    {
        __put_user(u32Width, &(((MS_GE_GET_FRAMEBUFFERINFO __user *)arg)->u32Width));
        __put_user(u32Height, &(((MS_GE_GET_FRAMEBUFFERINFO __user *)arg)->u32Height));
        __put_user(u32Pitch, &(((MS_GE_GET_FRAMEBUFFERINFO __user *)arg)->u32Pitch));
        __put_user(u32FbFmt, &(((MS_GE_GET_FRAMEBUFFERINFO __user *)arg)->u32FbFmt));
        __put_user(u32Addr, &(((MS_GE_GET_FRAMEBUFFERINFO __user *)arg)->u32Addr));
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_DrawLine(unsigned long arg)
{
    GE_DRAW_LINE_INFO geDrawLineInfo;

    if (copy_from_user(&geDrawLineInfo, ((GE_DRAW_LINE_INFO __user *)arg), sizeof(GE_DRAW_LINE_INFO)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_DrawLine(&(geDrawLineInfo)))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_DrawOval(unsigned long arg)
{
    GE_OVAL_FILL_INFO geOval;

    if (copy_from_user(&geOval, ((GE_OVAL_FILL_INFO __user *)arg), sizeof(GE_OVAL_FILL_INFO)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_DrawOval(&geOval))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_RectFill(unsigned long arg)
{
    GE_RECT_FILL_INFO geRectFillInfo;

    if (copy_from_user(&geRectFillInfo, ((GE_RECT_FILL_INFO __user *)arg), sizeof(GE_RECT_FILL_INFO)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_RectFill(&geRectFillInfo))
    {
        //printk("MDrv_GE_RectFill Success\n");
        return 0;
    }
    else
    {
        printk("MDrv_GE_RectFill Fail\n");
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetClip(unsigned long arg)
{
    GE_POINT_t geV0, geV1;

    if (copy_from_user(&geV0, ((MS_GE_SET_CLIP __user *)arg)->pPoint0, sizeof(GE_POINT_t)))
        return EFAULT;

    if (copy_from_user(&geV1, ((MS_GE_SET_CLIP __user *)arg)->pPoint1, sizeof(GE_POINT_t)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetClip(&geV0, &geV1))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetDither(unsigned long arg)
{
    U16  blDither;

    if (__get_user(blDither, (U16 __user *)arg))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetDither(blDither))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetSrcBufferInfo(unsigned long arg)
{
    GE_BUFFER_INFO geBufferInfo;
    U32 u32OffsetofByte;

    if (copy_from_user(&geBufferInfo, ((MS_GE_SET_BUFFERINFO __user *)arg)->pBufferInfo, sizeof(GE_BUFFER_INFO)))
        return EFAULT;
    if (__get_user(u32OffsetofByte, &(((MS_GE_SET_BUFFERINFO __user *)arg)->u32OffsetofByte)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetSrcBufferInfo(&geBufferInfo, u32OffsetofByte))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetDstBufferInfo(unsigned long arg)
{
    GE_BUFFER_INFO geBufferInfo;
    U32 u32OffsetofByte;

    if (copy_from_user(&geBufferInfo, ((MS_GE_SET_BUFFERINFO __user *)arg)->pBufferInfo, sizeof(GE_BUFFER_INFO)))
        return EFAULT;
    if (__get_user(u32OffsetofByte, &(((MS_GE_SET_BUFFERINFO __user *)arg)->u32OffsetofByte)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetDstBufferInfo(&geBufferInfo, u32OffsetofByte))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetNearestMode(unsigned long arg)
{
    U16  bNearestMode;

    if (__get_user(bNearestMode, (U16 __user *)arg))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetNearestMode(bNearestMode))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetMirror(unsigned long arg)
{
    MS_GE_SET_MIRROR  geSetMirror;

    if (copy_from_user(&geSetMirror, (MS_GE_SET_MIRROR __user *)arg, sizeof(MS_GE_SET_MIRROR)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetMirror(geSetMirror.blIsMirrorX, geSetMirror.blIsMirrorY))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetROP2(unsigned long arg)
{
    MS_GE_SET_ROP2  geSetROP2;

    if (copy_from_user(&geSetROP2, (MS_GE_SET_ROP2 __user *)arg, sizeof(MS_GE_SET_ROP2)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetROP2(geSetROP2.blEnable, geSetROP2.eRopMode))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetRotate(unsigned long arg)
{
    GEROTATE_ANGLE  geRotateAngle;

    if (__get_user(geRotateAngle, (GEROTATE_ANGLE __user *)arg))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetRotate(geRotateAngle))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetSrcColorKey(unsigned long arg)
{
    U16 blEnable;
    GE_COLOR_KEY_MODE enColorKeyMode;
    GE_Buffer_Format enBufferFmt;
    U32 u32PS_Color;
    U32 u32GE_Color;

    if (__get_user(blEnable, &((MS_GE_SET_COLORKEY __user *)arg)->blEnable))
        return EFAULT;

    if (__get_user(enColorKeyMode, &((MS_GE_SET_COLORKEY __user *)arg)->eOPMode))
        return EFAULT;

    if (__get_user(enBufferFmt, &((MS_GE_SET_COLORKEY __user *)arg)->enBufferFmt))
        return EFAULT;

    if (__get_user(u32PS_Color, ((MS_GE_SET_COLORKEY __user *)arg)->pu32PS_Color))
        return EFAULT;

    if (__get_user(u32GE_Color, ((MS_GE_SET_COLORKEY __user *)arg)->pu32GE_Color))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetSrcColorKey(blEnable, enColorKeyMode, enBufferFmt, &u32PS_Color, &u32GE_Color))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetDstColorKey(unsigned long arg)
{
    U16 blEnable;
    GE_COLOR_KEY_MODE enColorKeyMode;
    GE_Buffer_Format enBufferFmt;
    U32 u32PS_Color;
    U32 u32GE_Color;

    if (__get_user(blEnable, &((MS_GE_SET_COLORKEY __user *)arg)->blEnable))
        return EFAULT;

    if (__get_user(enColorKeyMode, &((MS_GE_SET_COLORKEY __user *)arg)->eOPMode))
        return EFAULT;

    if (__get_user(enBufferFmt, &((MS_GE_SET_COLORKEY __user *)arg)->enBufferFmt))
        return EFAULT;

    if (__get_user(u32PS_Color, ((MS_GE_SET_COLORKEY __user *)arg)->pu32PS_Color))
        return EFAULT;

    if (__get_user(u32GE_Color, ((MS_GE_SET_COLORKEY __user *)arg)->pu32GE_Color))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetDstColorKey(blEnable, enColorKeyMode, enBufferFmt, &u32PS_Color, &u32GE_Color))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetAlphaSrcFrom(unsigned long arg)
{
    GE_ALPHA_SRC_FROM eMode;

    if (__get_user(eMode, (GE_ALPHA_SRC_FROM __user *)arg))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetAlphaSrcFrom(eMode))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetAlphaBlending(unsigned long arg)
{
    MS_GE_SET_ALPHABLENDING geSetAlphaBlending;

    if (copy_from_user(&geSetAlphaBlending, (MS_GE_SET_ALPHABLENDING __user *)arg, sizeof(MS_GE_SET_ALPHABLENDING)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetAlphaBlending(geSetAlphaBlending.enBlendCoef, geSetAlphaBlending.u8BlendFactor))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_EnableAlphaBlending(unsigned long arg)
{
    U16 blEnable;

    if (__get_user(blEnable, (U16 __user *)arg))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_EnableAlphaBlending(blEnable))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_Line_Pattern_Reset(unsigned long arg)
{
    if (GESTATUS_SUCCESS == MHal_GE_Line_Pattern_Reset())
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_Set_Line_Pattern(unsigned long arg)
{
    U16 blEnable;
    U8   u8LinePattern;
    U8   u8RepeatFactor;

    if (__get_user(blEnable, &(((MS_GE_SET_LINEPATTERN __user *)arg)->blEnable)))
        return EFAULT;
    if (__get_user(u8LinePattern, &(((MS_GE_SET_LINEPATTERN __user *)arg)->u8LinePattern)))
        return EFAULT;
    if (__get_user(u8RepeatFactor, &(((MS_GE_SET_LINEPATTERN __user *)arg)->u8RepeatFactor)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_Set_Line_Pattern(blEnable, u8LinePattern, u8RepeatFactor))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_BitBlt(unsigned long arg)
{
    GE_DRAW_RECT geDrawRect;
    U32 u32DrawFlag;

    if (copy_from_user(&geDrawRect, ((MS_GE_SET_BITBLT __user *)arg)->pGEDrawRect, sizeof(GE_DRAW_RECT)))
        return EFAULT;
    if (__get_user(u32DrawFlag, &(((MS_GE_SET_BITBLT __user *)arg)->u32DrawFlag)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_BitBlt(&geDrawRect, u32DrawFlag))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_BitBltEx(unsigned long arg)
{
    GE_DRAW_RECT geDrawRect;
    U32 u32DrawFlag;
    GE_SCALE_INFO geScaleInfo;

    if (copy_from_user(&geDrawRect, ((MS_GE_SET_BITBLTEX __user *)arg)->pDrawRect, sizeof(GE_DRAW_RECT)))
        return EFAULT;
    if (__get_user(u32DrawFlag, &(((MS_GE_SET_BITBLTEX __user *)arg)->u32DrawFlag)))
        return EFAULT;
    if (copy_from_user(&geScaleInfo, ((MS_GE_SET_BITBLTEX __user *)arg)->pScaleInfo, sizeof(GE_SCALE_INFO)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_BitBltEx(&geDrawRect, u32DrawFlag, &geScaleInfo))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_GetScaleBltInfo(unsigned long arg)
{
    GE_DRAW_RECT geDrawRect;
    GE_BLOCK  geSrcBlk;
    GE_BLOCK  geDstBlk;
    GE_SCALE_INFO geScaleInfo;

    if (copy_from_user(&geDrawRect, ((MS_GE_GET_SCALEBLTINFO __user *)arg)->pDrawRect, sizeof(GE_DRAW_RECT)))
        return EFAULT;
    if (copy_from_user(&geSrcBlk, ((MS_GE_GET_SCALEBLTINFO __user *)arg)->pSrcBlk, sizeof(GE_BLOCK)))
        return EFAULT;
    if (copy_from_user(&geDstBlk, ((MS_GE_GET_SCALEBLTINFO __user *)arg)->pDstBlk, sizeof(GE_BLOCK)))
        return EFAULT;
    if (copy_from_user(&geScaleInfo, ((MS_GE_GET_SCALEBLTINFO __user *)arg)->pScaleInfo, sizeof(GE_SCALE_INFO)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_GetScaleBltInfo(&geDrawRect, &geSrcBlk, &geDstBlk, &geScaleInfo))
    {
        unsigned long r = 0;
        r = copy_to_user(((MS_GE_GET_SCALEBLTINFO __user *)arg)->pSrcBlk, &geSrcBlk, sizeof(GE_BLOCK));
        if (r) return r;
        r = copy_to_user(((MS_GE_GET_SCALEBLTINFO __user *)arg)->pDstBlk, &geDstBlk, sizeof(GE_BLOCK));
        if (r) return r;
        r = copy_to_user(((MS_GE_GET_SCALEBLTINFO __user *)arg)->pScaleInfo, &geScaleInfo, sizeof(GE_SCALE_INFO));
        if (r) return r;

        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_SetIntensity(unsigned long arg)
{
    U32 u32ID;
    GE_Buffer_Format enBufferFmt;
    U32 u32Color;

    if (__get_user(u32ID, &(((MS_GE_SET_INTENSITY __user *)arg)->u32ID)))
        return EFAULT;
    if (__get_user(enBufferFmt, &(((MS_GE_SET_INTENSITY __user *)arg)->enBufferFmt)))
        return EFAULT;
    if (__get_user(u32Color, ((MS_GE_SET_INTENSITY __user *)arg)->pu32Color))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_SetIntensity(u32ID, enBufferFmt, &u32Color))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_BeginDraw(unsigned long arg)
{
    if (GESTATUS_SUCCESS == MHal_GE_BeginDraw())
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_EndDraw(unsigned long arg)
{
    if (GESTATUS_SUCCESS == MHal_GE_EndDraw())
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_YUV_Set(unsigned long arg)
{
    GE_YUV_INFO geYUVInfo;
    if (copy_from_user(&geYUVInfo, (GE_BUFFER_INFO __user *)arg, sizeof(geYUVInfo)))
        return EFAULT;

    if (GESTATUS_SUCCESS == MHal_GE_YUV_Set(&geYUVInfo))
    {
        return 0;
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_YUV_Get(unsigned long arg)
{
    GE_YUV_INFO geYUVInfo;

    if (GESTATUS_SUCCESS == MHal_GE_YUV_Get(&geYUVInfo))
    {
        return copy_to_user(( GE_BUFFER_INFO __user *)arg, &geYUVInfo, sizeof(GE_BUFFER_INFO));
    }
    else
    {
        // The return value is not so appropriate.
        return EFAULT;
    }
}

int MDrv_GE_Palette_Set(unsigned long arg)
{
    GePaletteEntry *pGePalArray;
    U32 u32PalStart;
    U32 u32PalEnd;
    U32 u32PalSize;
    int iRet = 0;

    if (__get_user(u32PalStart, &((MS_GE_SET_PALETTE __user *)arg)->u32PalStart))
        iRet = EFAULT;
    if (__get_user(u32PalEnd, &((MS_GE_SET_PALETTE __user *)arg)->u32PalEnd))
        iRet = EFAULT;

    u32PalSize = u32PalEnd - u32PalStart + 1;

    pGePalArray = kmalloc(sizeof(GePaletteEntry) * u32PalSize, GFP_KERNEL);

    if (pGePalArray)
    {
    if (copy_from_user(pGePalArray, ((MS_GE_SET_PALETTE __user *)arg)->pPalArray, sizeof(GePaletteEntry)*u32PalSize))
        {
            iRet = EFAULT;
        }

        if (MHal_GE_Palette_Set(pGePalArray, u32PalStart, u32PalEnd))
        {
            iRet = EFAULT;
        }

        kfree(pGePalArray);
    }
    else
    {
        iRet = EFAULT;
    }

    return iRet;
}


// -----------------------------------------------------------------------------
// Device Methods
// -----------------------------------------------------------------------------

/*
 * Open and close
 */

int MDrv_GE_Open(struct inode *inode, struct file *filp)
{
    struct cdev *pDev = inode->i_cdev;
    filp->private_data = pDev; /* for other methods */

    printk("GE opens successfully\n");

    return 0;          /* success */
}

int MDrv_GE_Release(struct inode *inode, struct file *filp)
{
    printk("GE closes successfully\n");

    return 0;
}

extern void dummyRegWrite( void ) ;
extern void outGE_WaitAvailableCMDQueue(void);

long MDrv_GE_IOCtl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    int retval = 0;

    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != MDRV_GE_IOC_MAGIC) {
        printk("IOCtl Type Error!!! (Cmd=%x)\n",cmd);
        return -ENOTTY;
    }
    if (_IOC_NR(cmd) > MDRV_GE_IOC_MAXNR) {
        printk("IOCtl NR Error!!! (Cmd=%x)\n",cmd);
        return -ENOTTY;
    }

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) {
        printk("IOCtl Error!!! (cmd=%x)\n",cmd);
        return -EFAULT;
    }

    switch(cmd) {
      case MDRV_GE_IOC_INIT:
        MDrv_GE_Init(arg);
        break;

      case MDRV_GE_IOC_POWER_OFF:
        MDrv_GE_PowerOff(arg);
        break;

      case MDRV_GE_IOC_SCREEN_COPY:
        retval = MDrv_GE_ScreenCopy(arg);
        break;

      case MDRV_GE_IOC_GET_FRAMEBUFFERINFO:
        retval = MDrv_GE_GetFrameBufferInfo(arg);
        break;

      case MDRV_GE_IOC_DRAW_LINE:
        retval = MDrv_GE_DrawLine(arg);
        break;

      case MDRV_GE_IOC_DRAW_OVAL:
        retval = MDrv_GE_DrawOval(arg);
        break;

      case MDRV_GE_IOC_RECT_FILL:
        MDrv_GE_RectFill(arg);
        break;

      case MDRV_GE_IOC_SET_CLIP:
        MDrv_GE_SetClip(arg);
        break;

      case MDRV_GE_IOC_SET_DITHER:
        MDrv_GE_SetDither(arg);
        break;

      case MDRV_GE_IOC_SET_SRCBUFFERINO:
        MDrv_GE_SetSrcBufferInfo(arg);
        break;

      case MDRV_GE_IOC_SET_DSTBUFFERINO:
        MDrv_GE_SetDstBufferInfo(arg);
        break;

      case MDRV_GE_IOC_SET_NEARESTMODE:
        MDrv_GE_SetNearestMode(arg);
        break;

      case MDRV_GE_IOC_SET_MIRROR:
        MDrv_GE_SetMirror(arg);
        break;

      case MDRV_GE_IOC_SET_ROP2:
        MDrv_GE_SetROP2(arg);
        break;

      case MDRV_GE_IOC_SET_ROTATE:
        MDrv_GE_SetRotate(arg);
        break;

      case MDRV_GE_IOC_SET_SRCCOLORKEY:
        MDrv_GE_SetSrcColorKey(arg);
        break;

      case MDRV_GE_IOC_SET_DSTCOLORKEY:
        MDrv_GE_SetDstColorKey(arg);
        break;

      case MDRV_GE_IOC_SET_ALPHASRCFROM:
        MDrv_GE_SetAlphaSrcFrom(arg);
        break;

      case MDRV_GE_IOC_SET_ALPHABLENDING:
        MDrv_GE_SetAlphaBlending(arg);
        break;

      case MDRV_GE_IOC_ENABLE_ALPHABLENDING:
        MDrv_GE_EnableAlphaBlending(arg);
        break;

      case MDRV_GE_IOC_LINEPATTERN_RESET:
        MDrv_GE_Line_Pattern_Reset(arg);
        break;

      case MDRV_GE_IOC_SET_LINEPATTERN:
        MDrv_GE_Set_Line_Pattern(arg);
        break;

      case MDRV_GE_IOC_BITBLT:
        MDrv_GE_BitBlt(arg);
        break;

      case MDRV_GE_IOC_BITBLTEX:
        MDrv_GE_BitBltEx(arg);
        break;

      case MDRV_GE_IOC_Get_SCALEBLTINFO:
        MDrv_GE_GetScaleBltInfo(arg);
        break;

      case MDRV_GE_IOC_SET_INTENSITY:
        MDrv_GE_SetIntensity(arg);
        break;

      case MDRV_GE_IOC_BEGIN_DRAW:
        MDrv_GE_BeginDraw(arg);
        break;

      case MDRV_GE_IOC_END_DRAW:
        MDrv_GE_EndDraw(arg);
        break;

      case MDRV_GE_IOC_SET_YUV:
        MDrv_GE_YUV_Set(arg);
        break;

      case MDRV_GE_IOC_GET_YUV:
        MDrv_GE_YUV_Get(arg);
        break;

      case MDRV_GE_IOC_SET_PALETTE:
        MDrv_GE_Palette_Set(arg);
        break;

      default:  /* redundant, as cmd was checked against MAXNR */
        printk("Unknow IOCTL %x\n",cmd);
        return -ENOTTY;
    }

    outGE_WaitAvailableCMDQueue();

    dummyRegWrite() ;

    return retval;
}

static int __init MDrv_GE_Module_Init(void)
{
    int err, devno = MKDEV(MDRV_MAJOR_GE, MDRV_MINOR_GE);

    printk("GE driver inits\n");

    /* register the 'dummy_dev' char device */
    g_pGEDev = cdev_alloc();
    cdev_init(g_pGEDev, &_ge_fops);

    g_pGEDev->owner = THIS_MODULE;

    err = cdev_add(g_pGEDev, devno, 1);
    if (err != 0)
        printk("dummy pci device register failed!\n");

    /* creating your own class */
    g_pGEClass = class_create(THIS_MODULE, "ge");
    if(IS_ERR(g_pGEClass)) {
        printk("Err: failed in creating class.\n");
        return -1;
    }

    /* register your own device in sysfs, and this will cause udevd to create corresponding device node */
    device_create(g_pGEClass, NULL, devno, NULL, "ge");

    // Maybe need to move to MHal_GE_Module_Init
    MHal_GE_Init();

    return 0;
}

static void __exit MDrv_GE_Module_Exit(void)
{
    cdev_del(g_pGEDev);

    device_destroy(g_pGEClass, MKDEV(MDRV_MAJOR_GE, MDRV_MINOR_GE));
    class_destroy(g_pGEClass);

    printk("GE driver exits\n");
}

module_init(MDrv_GE_Module_Init);
module_exit(MDrv_GE_Module_Exit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("GE driver");
MODULE_LICENSE("MSTAR");

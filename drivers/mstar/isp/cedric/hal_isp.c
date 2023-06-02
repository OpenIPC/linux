////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (MStar Confidential Information) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include "MsTypes.h"

#include "hal_isp.h"

static volatile u32 *isp_cpu_int =
        (volatile u32*)(RIUBASE_ISP_0 + 0x2b*4);
static volatile u32 *isp_raw_int =
        (volatile u32*)(RIUBASE_ISP_0 + 0x2c*4);
static volatile u32 *isp_cpu_int2 =
        (volatile u32*)(RIUBASE_ISP_0 + 0x2f*4);

// FIXJASON
//static volatile u32 *isp_int_force =
//        (volatile u32*)(RIUBASE_ISP_0 + 0x29*4);

static volatile u32 *isp_int_mask =
        (volatile u32*)(RIUBASE_ISP_0 + 0x28*4);
static volatile u32 *isp_int_mask2 =
        (volatile u32*)(RIUBASE_ISP_0 + 0x2d*4);

static volatile u32 *isp_int_clear =
        (volatile u32*)(RIUBASE_ISP_0 + 0x2a*4);
static volatile u32 *isp_int_clear2 =
        (volatile u32*)(RIUBASE_ISP_0 + 0x2e*4);

static volatile u32 *isp_vsync_pol =
        (volatile u32*)(RIUBASE_ISP_0 + 0x10*4);
static volatile u32 *isp_sif =
        (volatile u32*)(RIUBASE_ISP_0 + 0x05*4);

static volatile u32 *isp_aests = (volatile u32*)(RIUBASE_ISP_4 + 0x40*4); // 0x7f
static volatile u32 *isp_aehist = (volatile u32*)(RIUBASE_ISP_4 + 0x00*4);// 0x3f
static volatile u32 *isp_awbsts = (volatile u32*)(RIUBASE_ISP_3 + 0x18*4);// 0x3f
static volatile u32 *isp_afsts = (volatile u32*)(RIUBASE_ISP_5 + 0x18*4); // 0x35

#if 1  // FIXJASON
//static volatile u32 *isp_vdos_width = (volatile u32*)(RIUBASE_ISP_0 + 0x6E*4);
//static volatile u32 *isp_vdos_height = (volatile u32*)(RIUBASE_ISP_0 + 0x6F*4);

static volatile u32 *isp_vdos_pitch = (volatile u32*)(RIUBASE_ISP_6 + 0x21*4);
static volatile u32 *isp_vdos_addr = (volatile u32*)(RIUBASE_ISP_6 + 0x28*4);
#endif


inline void HalISPMaskInt(void) {
    *(u32*)isp_int_mask |=
        *(u32*)isp_cpu_int;
    *(u32*)isp_int_mask2 |=
            (*(u32*)isp_cpu_int2 & 0x0f);

//    printk(KERN_INFO "[ISP] %s(%d): mask 0x%04x, cpu 0x%04x, clear 0x%04x, force 0x%04x", __FUNCTION__, __LINE__,
//            *(u32*)isp_int_mask,
//            *(u32*)isp_cpu_int,
//            *(u32*)isp_int_clear,
//            *(u32*)isp_int_force);
}

inline void HalISPDisableInt() {
    *(volatile u32*)isp_int_mask = 0xffff;
    *(volatile u32*)isp_int_mask2 = 0x000f;
    *(volatile u32*)isp_int_clear = 0xffff;
    *(volatile u32*)isp_int_clear2 = 0x000f;
}

inline void HalISPEnableInt() {
    *(volatile u32*)isp_int_mask = 0x000b;
    *(volatile u32*)isp_int_mask2 = 0x0000;
    *(volatile u32*)isp_int_clear = 0x000b;
    *(volatile u32*)isp_int_clear2 = 0x0000;
}

inline void HalISPMaskIntAll() {
    *(volatile u32*)isp_int_mask = 0xffff;
    *(volatile u32*)isp_int_mask2 = 0x000f;

    *(volatile u32*)isp_int_clear = 0xffff;
    *(volatile u32*)isp_int_clear2 = 0x000f;
}


inline void HalISPMaskClear1(u32 clear) {
    *(volatile u32*)isp_int_clear |= clear;
    *(volatile u32*)isp_int_clear = 0x000b;
    *(volatile u32*)isp_int_mask &= ~clear;
    *(volatile u32*)isp_int_mask |= 0x000b;
}
inline void HalISPMaskClear2(u32 clear) {
    *(volatile u32*)isp_int_clear2 |= clear;
    *(volatile u32*)isp_int_clear2 = 0;
    *(volatile u32*)isp_int_mask2 &= ~clear;
}

inline void HalISPClearInt(volatile u32 *clear, ISP_INT_STATUS int_num) {
    *clear |= (0x01<<int_num);
}

inline void HalISPClearInt2(volatile u32 *clear, ISP_INT2_STATUS int_num) {
    *clear |= (0x01<<int_num);
}

inline void HalISPClearIntAll() {
    *(volatile u32*)isp_int_mask = *(u32*)isp_int_clear;
    *(volatile u32*)isp_int_clear = 0x000b;

    // always clear 0-3
    //# [0]: Sensor Source VREF rising edge
    //# [1]: Sensor Source VREF falling edge
    //# [2] Hardware Flash Strobe done
    //# [3] Update double buffer register done

    *(volatile u32*)isp_int_mask2 = *(u32*)isp_int_clear2;
    *(volatile u32*)isp_int_clear2 = 0x0000;
}

inline void HalISPClkEnable(bool enable) {
    if (enable == false)
        *(volatile u32*)(RIUBASE_ISP_0 + 0x66*4) |=  (u16)0x04; // 0x35
    else
        *(volatile u32*)(RIUBASE_ISP_0 + 0x66*4) &= ~((u16)0x04);
}

inline u32 HalISPGetIntStatus() {
    return *(u32*)isp_int_mask;

}

inline u32 HalISPGetRawInt() {
    return *(u32*)isp_raw_int;

}

inline u32 HalISPGetIntStatus2() {
    return *(u32*)isp_int_mask2;

}

inline u32 HalISPGetVsyncPol() {
    return !(((*isp_vsync_pol) >> VSYNC_BIT) & 0x01);
}

inline u32 HalISPGetMIPI(void) {
    return (*isp_sif) & 0x01;
}

inline void HalISPGetAEStatSize(volatile int *size) {
    *size = 0x40*4;
}

inline void HalISPGetAEStat(volatile unsigned long *Stat) {
    if (Stat)
        memcpy((void *)Stat, (void *)isp_aests, 0x40*4);
}

inline void HalISPGetAEHistSize(volatile int *size) {
    *size = 0x40*4;
}

inline void HalISPGetAEHist(volatile unsigned long *Stat) {
    if (Stat)
        memcpy((void *)Stat, (void *)isp_aehist, 0x40*4);
}

inline void HalISPGetAWBStatSize(volatile int *size) {
    if (size)
        *size = 20*4;
}

inline void HalISPGetAWBStat(volatile unsigned long *Stat) {
    int i = 0;
    unsigned long value = 0, LSB = 0, MSB = 0;
//    printk(KERN_INFO "[%s]", __FUNCTION__);
//    printk(KERN_INFO "u32 %d, long long %d", sizeof(u32), sizeof(long long));
    if (Stat)
        for (i = 0; i < 20*2; i+=2) {
            LSB = *(u32 *)(isp_awbsts+i)&0xffff;
            MSB = *(u32 *)(isp_awbsts+(i+1))&0xffff;
            value = LSB | (MSB << 16);
//            printk(KERN_INFO "Stat: 0x%04x 0x%04x:0x%04x 0x%04x:0x%04x",
//                    (u32)value,
//                    isp_awbsts+i,
//                    LSB, //*(u32 *)(isp_awbsts+i)&0xffff,
//                    isp_awbsts+(i+1),
//                    MSB);//*(u32 *)(isp_awbsts+(i+1))&0xffff);
            *((u32*)Stat+i/2) = value;
        }
}

inline void HalISPGetAFStatSize(volatile int *size) {
    if (size)
        *size = 15 * 4;
}

inline void HalISPGetAFStat(volatile unsigned long *Stat) {
    int i = 0;
    unsigned long value = 0, LSB = 0, MSB = 0;
    if (Stat)
        for (i = 0; i < 15*2; i+=2) {
            LSB = *(u32 *)(isp_afsts+i)&0xffff;
            MSB = *(u32 *)(isp_afsts+(i+1))&0xffff;
            value = LSB | (MSB << 16);
            *(u32 *)(Stat+i/2) = value;
        }
}

#if 1  // FIXJASON
//inline void HalISPGetVDOSWidth(volatile int *pWidth) {
//    if (pWidth)
//        *pWidth = *(int *)isp_vdos_width;
//}

//inline void HalISPGetVDOSHight(volatile int *pHeight) {
//    if (pHeight)
//        *pHeight = *(int *)isp_vdos_height;
//}

inline void HalISPGetVDOSPitch(volatile int *pPitch) {
    if (pPitch)
        *pPitch = *(int *)isp_vdos_pitch;
}

inline void HalISPGetVDOSSize(volatile int *pSize, volatile int Pitch) {
    if (pSize)
        *pSize = Pitch * 24;  // pitch * 3 * 8bytes
}

inline void HalISPGetVDOSData(volatile unsigned long *pAddr, volatile int Size) {
    if (pAddr)
        memcpy((void *)pAddr, (void *)isp_vdos_addr, Size);
}
#endif




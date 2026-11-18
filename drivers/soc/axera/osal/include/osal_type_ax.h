/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_TYPE_AX__H__
#define __OSAL_TYPE_AX__H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0))
typedef void (*swap_func_t)(void *a, void *b, int size);
typedef int (*cmp_r_func_t)(const void *a, const void *b, const void *priv);
typedef int (*cmp_func_t)(const void *a, const void *b);
#endif

/*define 64BIT*/

/*define 32BIT*/
#define AX_USHRT_MAX    ((u16)(~0U))
#define AX_SHRT_MAX ((s16)(AX_USHRT_MAX>>1))
#define AX_SHRT_MIN ((s16)(-AX_SHRT_MAX - 1))
#define AX_INT_MAX      ((int)(~0U>>1))
#define AX_INT_MIN      (-AX_INT_MAX - 1)
#define AX_UINT_MAX (~0U)
#define AX_LONG_MAX ((long)(~0UL>>1))
#define AX_LONG_MIN (-AX_LONG_MAX - 1)
#define AX_ULONG_MAX    (~0UL)
#define AX_LLONG_MAX    ((long long)(~0ULL>>1))
#define AX_LLONG_MIN    (-AX_LLONG_MAX - 1)
#define AX_ULLONG_MAX   (~0ULL)
#define AX_SIZE_MAX (~(AX_SIZE_T)0)

#define AX_U8_MAX       ((u8)~0U)
#define AX_S8_MAX       ((s8)(AX_U8_MAX>>1))
#define AX_S8_MIN       ((s8)(-AX_S8_MAX - 1))
#define AX_U16_MAX      ((u16)~0U)
#define AX_S16_MAX      ((s16)(AX_U16_MAX>>1))
#define AX_S16_MIN      ((s16)(-AX_S16_MAX - 1))
#define AX_U32_MAX      ((u32)~0U)
#define AX_S32_MAX      ((s32)(AX_U32_MAX>>1))
#define AX_S32_MIN      ((s32)(-AX_S32_MAX - 1))
#define AX_U64_MAX      ((u64)~0ULL)
#define AX_S64_MAX      ((s64)(AX_U64_MAX>>1))
#define AX_S64_MIN      ((s64)(-AX_S64_MAX - 1))

#ifdef __cplusplus
}
#endif

#endif /*__OSAL_TYPE_AX__H__*/

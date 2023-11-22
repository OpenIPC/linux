/*
 * drv_sysdesc.h - Sigmastar
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

#ifndef _DRV_SYSDESC_H_
#define _DRV_SYSDESC_H_

#include "device_id.h"
#include "property_id.h"

typedef enum
{
    E_SYS_DESC_PASS,
    E_SYS_DESC_MAGIC_ERROR,
    E_SYS_DESC_VERSION_ERROR,
    E_SYS_DESC_DEVICE_ID_ERROR,
    E_SYS_DESC_PROPERTY_ID_ERROR,
    E_SYS_DESC_OVERFLOW,
    E_SYS_DESC_CONTENT_ERROR,
    E_SYS_DESC_TOTALSIZE_ERROR,
    E_SYS_DESC_PROPERTY_EMPTY,
    E_SYS_DESC_CONTENT_LEN_ERROR,
    E_SYS_DESC_ELEMENT_COUNT_ERROR,
} enumSysDescErrCode;

enumSysDescErrCode MDrv_SysDesc_GetElemsOfSize(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                               unsigned short *pu16GetData);
enumSysDescErrCode MDrv_SysDesc_GetContentOfLen(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                                unsigned short *pu16GetData);
enumSysDescErrCode MDrv_SysDesc_GetElementCount(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                                unsigned short *pu16Quotient, unsigned short *pu16Remainder);
enumSysDescErrCode MDrv_SysDesc_Read_U8(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                        unsigned char *pu8GetData);
enumSysDescErrCode MDrv_SysDesc_Read_U16(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                         unsigned short *pu16GetData);
enumSysDescErrCode MDrv_SysDesc_Read_U32(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                         unsigned int *pu32GetData);
enumSysDescErrCode MDrv_SysDesc_Read_U8_Array(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                              unsigned char *pu8GetData, unsigned short u16Length);
enumSysDescErrCode MDrv_SysDesc_Read_U16_Array(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                               unsigned short *pu16GetData, unsigned short u16Length);
enumSysDescErrCode MDrv_SysDesc_Read_U32_Array(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                               unsigned int *pu32GetData, unsigned short u16Length);
enumSysDescErrCode MDrv_SysDesc_Read_MultiTypes(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                                void *pvGetData, unsigned short u16DataTypeSize,
                                                unsigned short u16Length);
enumSysDescErrCode MDrv_SysDesc_Read_String(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                            const char *pGetString, unsigned short u16Length);
#endif

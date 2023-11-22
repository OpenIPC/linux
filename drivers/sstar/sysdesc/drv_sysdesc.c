/*
 * drv_sysdesc.c- Sigmastar
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

#define __DRV_SYSDESC_C__

/*==========================================================*/
//  Include Files
/*==========================================================*/
#include "cam_os_wrapper.h"
#include "hal_sysdesc.h"
#include "drv_sysdesc.h"

/*==========================================================*/
//  Data Type Definition
/*==========================================================*/
typedef struct
{
    unsigned int   u32Magic;
    unsigned char  u8Minor;
    unsigned char  u8Major;
    unsigned short u16DeviceStartAddr;
    unsigned short u16PropertyStartAddr;
    unsigned short u16Size;
} __attribute__((__packed__)) sysDescHeader;

typedef struct
{
    unsigned short u16DeviceAddr;
    unsigned short u16Device1stPropertyAddr_Bin;
    unsigned short u16PropertyAddr;
    unsigned short u16PropertyID_Bin;
    unsigned short u16ElementSizeAddr;
    unsigned short u16ElementSize_Bin;
    unsigned short u16ContentLenAddr;
    unsigned short u16ContentLen_Bin;
    unsigned short u16PropertyContentAddr;
} sysDescConfig;

/*==========================================================*/
//  Macro definition
/*==========================================================*/
#define MAGIC_CODE              0xbabe5aa5
#define MAJOR_CODE              1
#define MINOR_CODE              0
#define ELEMENT_SIZE_OFFSET     2
#define CONTENT_LEN_OFFSET      4
#define PROPERTY_CONTENT_OFFSET 6

#define BYTE_SIZE  1
#define WORD_SIZE  2
#define DWORD_SIZE 4

/*==========================================================*/
//  Variable definition
/*==========================================================*/
static unsigned char  u8Init = 0;
static sysDescHeader *header_ops;

/*==========================================================*/
//  Local function definition
/*==========================================================*/
enumSysDescErrCode _Drv_SysDesc_Init(void)
{
    header_ops = (sysDescHeader *)u8SysDesTable;
    if (header_ops->u32Magic != MAGIC_CODE)
        return E_SYS_DESC_MAGIC_ERROR;
    if ((header_ops->u8Major != MAJOR_CODE) && (header_ops->u8Minor != MINOR_CODE))
        return E_SYS_DESC_VERSION_ERROR;
    if (header_ops->u16Size != (sizeof(u8SysDesTable) / sizeof(unsigned char)))
        return E_SYS_DESC_TOTALSIZE_ERROR;

    u8Init = 1;

    return E_SYS_DESC_PASS;
}

enumSysDescErrCode _Drv_SysDesc_CheckID(unsigned short u16DeviceID, unsigned short u16PropertyID)
{
    if ((u16DeviceID < SYSDESC_DEV_MIN) || (u16DeviceID > SYSDESC_DEV_MAX))
        return E_SYS_DESC_DEVICE_ID_ERROR;

    if ((u16DeviceID % 2) != 0)
        return E_SYS_DESC_DEVICE_ID_ERROR;

    if ((u16PropertyID < PROPERTY_MIN) || (u16PropertyID > PROPERTY_MAX))
        return E_SYS_DESC_PROPERTY_ID_ERROR;
    return E_SYS_DESC_PASS;
}

enumSysDescErrCode _Drv_SysDesc_FindPropertyID(unsigned short u16DeviceID, sysDescConfig *ops)
{
    ops->u16DeviceAddr = header_ops->u16DeviceStartAddr + u16DeviceID;
    ops->u16Device1stPropertyAddr_Bin =
        (u8SysDesTable[ops->u16DeviceAddr + 1] << 8) | u8SysDesTable[ops->u16DeviceAddr];
    ops->u16PropertyAddr = ops->u16Device1stPropertyAddr_Bin;
    if (ops->u16PropertyAddr == 0x0000)
        return E_SYS_DESC_PROPERTY_EMPTY;
    ops->u16PropertyID_Bin = (u8SysDesTable[ops->u16PropertyAddr + 1] << 8) | u8SysDesTable[ops->u16PropertyAddr];
    return E_SYS_DESC_PASS;
}

enumSysDescErrCode _Drv_SysDesc_GetPropertyContent(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                                   unsigned char *pu8GetData, unsigned short *pu16GetData,
                                                   unsigned int *pu32GetData, unsigned short u16Length,
                                                   unsigned char u8DataType)
{
    sysDescConfig      current_ops = {0};
    sysDescConfig      next_ops    = {0};
    unsigned short     u16NextDeviceID, u16Index;
    enumSysDescErrCode enRet;
    unsigned short     u16TrueLen = u16Length * u8DataType;
    unsigned char      u8DataArray[DWORD_SIZE], i = 0;

    if (u8Init == 0)
    {
        enRet = _Drv_SysDesc_Init(); // get SysDesc header info
        if (enRet != E_SYS_DESC_PASS)
            return enRet;
    }

    enRet = _Drv_SysDesc_CheckID(u16DeviceID, u16PropertyID); // check device ID and property ID if out of range
    if (enRet != E_SYS_DESC_PASS)
        return enRet;

    enRet = _Drv_SysDesc_FindPropertyID(u16DeviceID, &current_ops); // get property ID
    if (enRet != E_SYS_DESC_PASS)
        return enRet;

    u16NextDeviceID = u16DeviceID + 2; // find next device ID info
    do
    {
        if (u16NextDeviceID > SYSDESC_DEV_MAX) // last device ID
        {
            u16NextDeviceID = 0;
            break;
        }
        enRet = _Drv_SysDesc_FindPropertyID(u16NextDeviceID, &next_ops);
        if (enRet == E_SYS_DESC_PROPERTY_EMPTY)
            u16NextDeviceID = u16NextDeviceID + 2;
    } while (enRet == E_SYS_DESC_PROPERTY_EMPTY);

    while (1)
    {
        if (current_ops.u16PropertyID_Bin == u16PropertyID)
        {
            current_ops.u16ContentLenAddr = current_ops.u16PropertyAddr + CONTENT_LEN_OFFSET;
            current_ops.u16ContentLen_Bin =
                (u8SysDesTable[current_ops.u16ContentLenAddr + 1] << 8) | u8SysDesTable[current_ops.u16ContentLenAddr];
            if (u16TrueLen > current_ops.u16ContentLen_Bin)
            {
                enRet = E_SYS_DESC_CONTENT_LEN_ERROR;
                break;
            }

            current_ops.u16PropertyContentAddr = current_ops.u16PropertyAddr + PROPERTY_CONTENT_OFFSET;
            while (u16TrueLen--)
            {
                u8DataArray[i] = u8SysDesTable[current_ops.u16PropertyContentAddr];
                i++;
                if (i == u8DataType)
                {
                    if (u8DataType == BYTE_SIZE)
                        *pu8GetData++ = u8DataArray[0];
                    else if (u8DataType == WORD_SIZE)
                        *pu16GetData++ = (u8DataArray[1] << 8) | u8DataArray[0];
                    else
                        *pu32GetData++ =
                            (u8DataArray[3] << 24) | (u8DataArray[2] << 16) | (u8DataArray[1] << 8) | u8DataArray[0];
                    i = 0;
                }
                current_ops.u16PropertyContentAddr++;
            }
            enRet = E_SYS_DESC_PASS;
            break;
        }
        else // In the same of Device ID , to find next property ID
        {
            current_ops.u16ContentLenAddr = current_ops.u16PropertyAddr + CONTENT_LEN_OFFSET;
            current_ops.u16ContentLen_Bin =
                (u8SysDesTable[current_ops.u16ContentLenAddr + 1] << 8) | u8SysDesTable[current_ops.u16ContentLenAddr];
            current_ops.u16PropertyContentAddr = current_ops.u16PropertyAddr + PROPERTY_CONTENT_OFFSET;
            u16Index                           = current_ops.u16PropertyContentAddr + current_ops.u16ContentLen_Bin;
            if ((u16Index >= header_ops->u16Size) && (u16NextDeviceID == 0)) // last device ID and last property ID
            {
                enRet = E_SYS_DESC_PROPERTY_ID_ERROR;
                break;
            }

            if ((u16Index >= next_ops.u16PropertyAddr) && (next_ops.u16PropertyAddr != 0)) // last property ID
            {
                enRet = E_SYS_DESC_PROPERTY_ID_ERROR;
                break;
            }

            current_ops.u16PropertyAddr = u16Index;
            current_ops.u16PropertyID_Bin =
                (u8SysDesTable[current_ops.u16PropertyAddr + 1] << 8) | u8SysDesTable[current_ops.u16PropertyAddr];
        }
    }

    return enRet;
}

/*==========================================================*/
//  Global function definition
/*==========================================================*/
enumSysDescErrCode MDrv_SysDesc_GetElemsOfSize(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                               unsigned short *pu16GetData)
{
    enumSysDescErrCode enRet;
    sysDescConfig      current_ops = {0};
    sysDescConfig      next_ops    = {0};
    unsigned short     u16NextDeviceID, u16Index;

    if (u8Init == 0)
    {
        enRet = _Drv_SysDesc_Init();
        if (enRet != E_SYS_DESC_PASS)
            return enRet;
    }

    enRet = _Drv_SysDesc_CheckID(u16DeviceID, u16PropertyID);
    if (enRet != E_SYS_DESC_PASS)
        return enRet;

    enRet = _Drv_SysDesc_FindPropertyID(u16DeviceID, &current_ops);
    if (enRet != E_SYS_DESC_PASS)
        return enRet;

    u16NextDeviceID = u16DeviceID + 2;
    do
    {
        if (u16NextDeviceID > SYSDESC_DEV_MAX)
        {
            u16NextDeviceID = 0;
            break;
        }
        enRet = _Drv_SysDesc_FindPropertyID(u16NextDeviceID, &next_ops);
        if (enRet == E_SYS_DESC_PROPERTY_EMPTY)
            u16NextDeviceID = u16NextDeviceID + 2;
    } while (enRet == E_SYS_DESC_PROPERTY_EMPTY);

    while (1)
    {
        if (current_ops.u16PropertyID_Bin == u16PropertyID)
        {
            current_ops.u16ElementSizeAddr = current_ops.u16PropertyAddr + ELEMENT_SIZE_OFFSET;
            current_ops.u16ElementSize_Bin = (u8SysDesTable[current_ops.u16ElementSizeAddr + 1] << 8)
                                             | u8SysDesTable[current_ops.u16ElementSizeAddr];
            *pu16GetData = current_ops.u16ElementSize_Bin;
            enRet        = E_SYS_DESC_PASS;
            break;
        }
        else // In the same of Device ID , to find next property ID
        {
            current_ops.u16ContentLenAddr = current_ops.u16PropertyAddr + CONTENT_LEN_OFFSET;
            current_ops.u16ContentLen_Bin =
                (u8SysDesTable[current_ops.u16ContentLenAddr + 1] << 8) | u8SysDesTable[current_ops.u16ContentLenAddr];
            current_ops.u16PropertyContentAddr = current_ops.u16PropertyAddr + PROPERTY_CONTENT_OFFSET;
            u16Index                           = current_ops.u16PropertyContentAddr + current_ops.u16ContentLen_Bin;
            if ((u16Index >= header_ops->u16Size) && (u16NextDeviceID == 0)) // last device ID and last property ID
            {
                enRet = E_SYS_DESC_PROPERTY_ID_ERROR;
                break;
            }

            if ((u16Index >= next_ops.u16PropertyAddr) && (next_ops.u16PropertyAddr != 0)) // last property ID
            {
                enRet = E_SYS_DESC_PROPERTY_ID_ERROR;
                break;
            }

            current_ops.u16PropertyAddr = u16Index;
            current_ops.u16PropertyID_Bin =
                (u8SysDesTable[current_ops.u16PropertyAddr + 1] << 8) | u8SysDesTable[current_ops.u16PropertyAddr];
        }
    }

    return enRet;
}

enumSysDescErrCode MDrv_SysDesc_GetContentOfLen(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                                unsigned short *pu16GetData)
{
    enumSysDescErrCode enRet;
    sysDescConfig      current_ops = {0};
    sysDescConfig      next_ops    = {0};
    unsigned short     u16NextDeviceID, u16Index;

    if (u8Init == 0)
    {
        enRet = _Drv_SysDesc_Init();
        if (enRet != E_SYS_DESC_PASS)
            return enRet;
    }

    enRet = _Drv_SysDesc_CheckID(u16DeviceID, u16PropertyID);
    if (enRet != E_SYS_DESC_PASS)
        return enRet;

    enRet = _Drv_SysDesc_FindPropertyID(u16DeviceID, &current_ops);
    if (enRet != E_SYS_DESC_PASS)
        return enRet;

    u16NextDeviceID = u16DeviceID + 2;
    do
    {
        if (u16NextDeviceID > SYSDESC_DEV_MAX)
        {
            u16NextDeviceID = 0;
            break;
        }
        enRet = _Drv_SysDesc_FindPropertyID(u16NextDeviceID, &next_ops);
        if (enRet == E_SYS_DESC_PROPERTY_EMPTY)
            u16NextDeviceID = u16NextDeviceID + 2;
    } while (enRet == E_SYS_DESC_PROPERTY_EMPTY);

    while (1)
    {
        if (current_ops.u16PropertyID_Bin == u16PropertyID)
        {
            current_ops.u16ContentLenAddr = current_ops.u16PropertyAddr + CONTENT_LEN_OFFSET;
            current_ops.u16ContentLen_Bin =
                (u8SysDesTable[current_ops.u16ContentLenAddr + 1] << 8) | u8SysDesTable[current_ops.u16ContentLenAddr];
            *pu16GetData = current_ops.u16ContentLen_Bin;
            enRet        = E_SYS_DESC_PASS;
            break;
        }
        else // In the same of Device ID , to find next property ID
        {
            current_ops.u16ContentLenAddr = current_ops.u16PropertyAddr + CONTENT_LEN_OFFSET;
            current_ops.u16ContentLen_Bin =
                (u8SysDesTable[current_ops.u16ContentLenAddr + 1] << 8) | u8SysDesTable[current_ops.u16ContentLenAddr];
            current_ops.u16PropertyContentAddr = current_ops.u16PropertyAddr + PROPERTY_CONTENT_OFFSET;
            u16Index                           = current_ops.u16PropertyContentAddr + current_ops.u16ContentLen_Bin;
            if ((u16Index >= header_ops->u16Size) && (u16NextDeviceID == 0)) // last device ID and last property ID
            {
                enRet = E_SYS_DESC_PROPERTY_ID_ERROR;
                break;
            }

            if ((u16Index >= next_ops.u16PropertyAddr) && (next_ops.u16PropertyAddr != 0)) // last property ID
            {
                enRet = E_SYS_DESC_PROPERTY_ID_ERROR;
                break;
            }

            current_ops.u16PropertyAddr = u16Index;
            current_ops.u16PropertyID_Bin =
                (u8SysDesTable[current_ops.u16PropertyAddr + 1] << 8) | u8SysDesTable[current_ops.u16PropertyAddr];
        }
    }

    return enRet;
}

enumSysDescErrCode MDrv_SysDesc_GetElementCount(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                                unsigned short *pu16Quotient, unsigned short *pu16Remainder)
{
    enumSysDescErrCode enRet, enRet1;
    unsigned short     u16ElementSize, u16ContentLen;

    enRet  = MDrv_SysDesc_GetElemsOfSize(u16DeviceID, u16PropertyID, &u16ElementSize);
    enRet1 = MDrv_SysDesc_GetContentOfLen(u16DeviceID, u16PropertyID, &u16ContentLen);
    if ((enRet == E_SYS_DESC_PASS) && (enRet1 == E_SYS_DESC_PASS))
    {
        *pu16Quotient  = u16ContentLen / u16ElementSize;
        *pu16Remainder = u16ContentLen % u16ElementSize;
        return E_SYS_DESC_PASS;
    }
    else
    {
        if (enRet != E_SYS_DESC_PASS)
            return enRet;
        else
            return enRet1;
    }
}

enumSysDescErrCode MDrv_SysDesc_Read_U8_Array(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                              unsigned char *pu8GetData, unsigned short u16Length)
{
    return _Drv_SysDesc_GetPropertyContent(u16DeviceID, u16PropertyID, pu8GetData, NULL, NULL, u16Length, BYTE_SIZE);
}

enumSysDescErrCode MDrv_SysDesc_Read_U16_Array(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                               unsigned short *pu16GetData, unsigned short u16Length)
{
    return _Drv_SysDesc_GetPropertyContent(u16DeviceID, u16PropertyID, NULL, pu16GetData, NULL, u16Length, WORD_SIZE);
}

enumSysDescErrCode MDrv_SysDesc_Read_U32_Array(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                               unsigned int *pu32GetData, unsigned short u16Length)
{
    return _Drv_SysDesc_GetPropertyContent(u16DeviceID, u16PropertyID, NULL, NULL, pu32GetData, u16Length, DWORD_SIZE);
}

enumSysDescErrCode MDrv_SysDesc_Read_U8(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                        unsigned char *pu8GetData)
{
    return MDrv_SysDesc_Read_U8_Array(u16DeviceID, u16PropertyID, pu8GetData, 1);
}

enumSysDescErrCode MDrv_SysDesc_Read_U16(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                         unsigned short *pu16GetData)
{
    return MDrv_SysDesc_Read_U16_Array(u16DeviceID, u16PropertyID, pu16GetData, 1);
}

enumSysDescErrCode MDrv_SysDesc_Read_U32(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                         unsigned int *pu32GetData)
{
    return MDrv_SysDesc_Read_U32_Array(u16DeviceID, u16PropertyID, pu32GetData, 1);
}

enumSysDescErrCode MDrv_SysDesc_Read_MultiTypes(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                                void *pvGetData, unsigned short u16DataTypeSize,
                                                unsigned short u16Length)
{
    enumSysDescErrCode enRet;
    unsigned short     u16Quotient, u16Remainder;
    unsigned short     u16Quotient1, u16Remainder1;

    u16Quotient1  = u16Length / u16DataTypeSize;
    u16Remainder1 = u16Length % u16DataTypeSize;
    enRet         = MDrv_SysDesc_GetElementCount(u16DeviceID, u16PropertyID, &u16Quotient, &u16Remainder);
    if ((u16Remainder != 0) || (u16Remainder1 != 0))
    {
        enRet = E_SYS_DESC_CONTENT_LEN_ERROR;
        return enRet;
    }

    if (u16Quotient != u16Quotient1) // element count isn't match
    {
        enRet = E_SYS_DESC_ELEMENT_COUNT_ERROR;
        return enRet;
    }

    enRet = MDrv_SysDesc_Read_U8_Array(u16DeviceID, u16PropertyID, (unsigned char *)pvGetData, u16Length);
    return enRet;
}

enumSysDescErrCode MDrv_SysDesc_Read_String(unsigned short u16DeviceID, unsigned short u16PropertyID,
                                            const char *pGetString, unsigned short u16Length)
{
    return _Drv_SysDesc_GetPropertyContent(u16DeviceID, u16PropertyID, (unsigned char *)pGetString, NULL, NULL,
                                           u16Length, BYTE_SIZE);
}

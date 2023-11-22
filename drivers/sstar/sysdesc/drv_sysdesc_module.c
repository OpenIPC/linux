/*
 * drv_sysdesc_module.c - Sigmastar
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

#define __DRV_SYSDESC__MODULE_C__

/*==========================================================*/
//  Include Files
/*==========================================================*/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/io.h>

#include "drv_sysdesc.h"

/*==========================================================*/
//  Data Type Definition
/*==========================================================*/

/*==========================================================*/
//  Macro definition
/*==========================================================*/

/*==========================================================*/
//  Variable definition
/*==========================================================*/

/*==========================================================*/
//  Local function definition
/*==========================================================*/

/*==========================================================*/
//  Global function definition
/*==========================================================*/
EXPORT_SYMBOL(MDrv_SysDesc_GetElemsOfSize);
EXPORT_SYMBOL(MDrv_SysDesc_GetContentOfLen);
EXPORT_SYMBOL(MDrv_SysDesc_GetElementCount);
EXPORT_SYMBOL(MDrv_SysDesc_Read_U8);
EXPORT_SYMBOL(MDrv_SysDesc_Read_U16);
EXPORT_SYMBOL(MDrv_SysDesc_Read_U32);
EXPORT_SYMBOL(MDrv_SysDesc_Read_U8_Array);
EXPORT_SYMBOL(MDrv_SysDesc_Read_U16_Array);
EXPORT_SYMBOL(MDrv_SysDesc_Read_U32_Array);
EXPORT_SYMBOL(MDrv_SysDesc_Read_MultiTypes);
EXPORT_SYMBOL(MDrv_SysDesc_Read_String);

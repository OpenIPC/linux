/*
 *#############################################################################
 *
 * Copyright (c) 2006-2011 MStar Semiconductor, Inc.
 * All rights reserved.
 *
 * Unless otherwise stipulated in writing, any and all information contained
 * herein regardless in any format shall remain the sole proprietary of
 * MStar Semiconductor Inc. and be kept in strict confidence
 * (¡§MStar Confidential Information¡¨) by the recipient.
 * Any unauthorized act including without limitation unauthorized disclosure,
 * copying, use, reproduction, sale, distribution, modification, disassembling,
 * reverse engineering and compiling of the contents of MStar Confidential
 * Information is unlawful and strictly prohibited. MStar hereby reserves the
 * rights to any and all damages, losses, costs and expenses resulting therefrom.
 *
 *#############################################################################
*/


#ifndef _IOCTL_TEST_H
#define _IOCTL_TEST_H

#include <linux/ioctl.h>

struct rsa_config {
unsigned int *pu32RSA_Sig;
unsigned int *pu32RSA_KeyN;
unsigned int *pu32RSA_KeyE;
unsigned int *pu32RSA_Output;
unsigned int u32RSA_KeyNLen;
unsigned int u32RSA_KeyELen;
unsigned int u32RSA_SigLen;
};




/* ocumentation/ioctl/ioctl-number.txt */
#define IOC_MAGIC '\x66'

#define MDrv_RSA_Reset                _IO(IOC_MAGIC, 0x92)
#define MDrv_RSA_Setmode              _IO(IOC_MAGIC, 0x93)
#define MDrv_RSA_Calculate            _IO(IOC_MAGIC, 0x94)

#endif
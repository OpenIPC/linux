/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "osal_ax.h"

void AX_OSAL_prefetch(void *addr)
{
	prefetch(addr);
}
EXPORT_SYMBOL(AX_OSAL_prefetch);

void AX_OSAL_prefetchw(void *addr)
{
	prefetchw(addr);
}
EXPORT_SYMBOL(AX_OSAL_prefetchw);

/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_GZIPD_HAL_TYPE_H_
#define _AX_GZIPD_HAL_TYPE_H_

#include "ax_gzipd_api.h"

typedef struct {
    AX_S32 *handle;
    AX_GZIPD_HEADER_INFO_T headerInfo;
    AX_VOID *gzipdData;
} ax_gzip_handle_info_t;

#endif
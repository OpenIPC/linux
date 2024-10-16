/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file config.h
 * Config options file
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "armchina_aipu.h"

#define KMD_VERSION  "3.0.11"

#if ((defined BUILD_DEBUG_VERSION) && (BUILD_DEBUG_VERSION == 1))
#define KMD_BUILD_DEBUG_FLAG "debug"
#define AIPU_ENABLE_SYSFS    1
#else
#define KMD_BUILD_DEBUG_FLAG "release"
#endif /* BUILD_DEBUG_VERSION */

#define AIPU_CONFIG_ENABLE_SRAM             0
#define AIPU_CONFIG_ENABLE_FALL_BACK_TO_DDR 1
#define AIPU_CONFIG_DEFAULT_SRAM_DATA_TYPE  AIPU_MM_DATA_TYPE_REUSE

#endif /* __CONFIG_H__ */

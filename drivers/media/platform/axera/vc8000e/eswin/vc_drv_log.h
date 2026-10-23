/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2026, Beijing ESWIN Computing Technology Co., Ltd.. All rights reserved.
 *
 */

#ifndef __VC_DRV_LOG__H
#define __VC_DRV_LOG__H

#include <linux/kernel.h>

#define VC_LOG_LEVEL_TRACE 0x01
#define VC_LOG_LEVEL_DBG 0x02
#define VC_LOG_LEVEL_INFO 0x04
#define VC_LOG_LEVEL_NOTICE 0x08
#define VC_LOG_LEVEL_WARN 0x10
#define VC_LOG_LEVEL_ERR 0x20

// define in makefile
#define OUTPUT_LOG_LEVEL 0xFC

#if (OUTPUT_LOG_LEVEL & VC_LOG_LEVEL_ERR)
#define LOG_ERR(fmt, args...) do { pr_err("[" LOG_TAG "][pid=%u]" fmt, current->pid, ##args); } while (0)
#else
#define LOG_ERR(fmt, args...)
#endif

#if (OUTPUT_LOG_LEVEL & VC_LOG_LEVEL_WARN)
#define LOG_WARN(fmt, args...) do { pr_warn("[" LOG_TAG "][pid=%u]" fmt, current->pid, ##args); } while (0)
#else
#define LOG_WARN(fmt, args...)
#endif

#if (OUTPUT_LOG_LEVEL & VC_LOG_LEVEL_NOTICE)
#define LOG_NOTICE(fmt, args...) do { pr_notice("[" LOG_TAG "][pid=%u]" fmt, current->pid, ##args); } while (0)
#else
#define LOG_NOTICE(fmt, args...)
#endif

#if (OUTPUT_LOG_LEVEL & VC_LOG_LEVEL_INFO)
#define LOG_INFO(fmt, args...) do { pr_info("[" LOG_TAG "][pid=%u]" fmt, current->pid, ##args); } while (0)
#else
#define LOG_INFO(fmt, args...)
#endif

#if (OUTPUT_LOG_LEVEL & VC_LOG_LEVEL_DBG)
#define LOG_DBG(fmt, args...) do { pr_debug("[" LOG_TAG "][pid=%u]" fmt, current->pid, ##args); } while (0)
#else
#define LOG_DBG(fmt, args...)
#endif

#if (OUTPUT_LOG_LEVEL & VC_LOG_LEVEL_TRACE)
#define LOG_TRACE(fmt, args...) do { pr_info("[" LOG_TAG "][pid=%u]" fmt, current->pid, ##args); } while (0)
#else
#define LOG_TRACE(fmt, args...)
#endif

#define VENC_DEV_NAME  "es_venc"

#endif

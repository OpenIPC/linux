
/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : cdc_log.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2015/04/13
*   Comment :
*
*
*/

#ifndef LOG_H
#define LOG_H

#include <cdc_config.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#ifndef LOG_TAG
#define LOG_TAG "cedarc"
#endif

#define ENABLE_LOG_LEVEL 1

#define LOG_LEVEL_DEBUG "debug"
#define LOG_LEVEL_INFO "info"
#define LOG_LEVEL_VERBOSE "verbose"
#define LOG_LEVEL_WARNING "warn"
#define LOG_LEVEL_ERROR "error"

#if ENABLE_LOG_LEVEL
#define logd(fmt, arg...) printk(KERN_DEBUG "%s:%s <%s:%u> " fmt "\n", LOG_LEVEL_DEBUG, LOG_TAG, __FUNCTION__, __LINE__, ##arg)
#define logi(fmt, arg...) printk(KERN_INFO "%s:%s <%s:%u> " fmt "\n", LOG_LEVEL_INFO, LOG_TAG, __FUNCTION__, __LINE__, ##arg)
//#define logv(fmt, arg...) printk(KERN_INFO"%s:%s <%s:%u> "fmt"\n", LOG_LEVEL_VERBOSE, LOG_TAG, __FUNCTION__, __LINE__, ##arg)
#define logv(fmt, arg...)
#else
#define logd(fmt, arg...)
#define logi(fmt, arg...)
#define logv(fmt, arg...)
#endif

#define logw(fmt, arg...) printk(KERN_WARNING "%s:%s <%s:%u> " fmt "\n", LOG_LEVEL_WARNING, LOG_TAG, __FUNCTION__, __LINE__, ##arg)
#define loge(fmt, arg...) printk(KERN_ERR "%s:%s <%s:%u> " fmt "\n", LOG_LEVEL_ERROR, LOG_TAG, __FUNCTION__, __LINE__, ##arg)

#define CEDARC_PRINTF_LINE logd("Run this line")

#define CEDARC_UNUSE(param) (void)param //just for remove compile warning

#define CEDARC_DEBUG (0)

#endif

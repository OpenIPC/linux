/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_GZIPD_DEV_LOG_H_
#define _AX_GZIPD_DEV_LOG_H_

#include "ax_global_type.h"
#include "ax_gzipd_adapter.h"

// #define GZIPD_CRC32_ENABLE
#define AX_GZIP_DEBUG_LOG_EN
#define AX_SYSLOG_EN

#ifdef AX_GZIP_DEBUG_LOG_EN

#ifdef AX_SYSLOG_EN
#define AX_GZIP_DEV_LOG_EMERG(fmt,...) do{\
                ax_pr_emerg(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }while(0)

#define AX_GZIP_DEV_LOG_ALERT(fmt,...) do{\
                ax_pr_alert(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }while(0)

#define AX_GZIP_DEV_LOG_CRIT(fmt,...) do{\
                ax_pr_crit(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }while(0)

#define AX_GZIP_DEV_LOG_ERR(fmt,...) do{\
                ax_pr_err(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
            }while(0)

#define AX_GZIP_DEV_LOG_WARNING(fmt,...) do{\
                ax_pr_warning(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }while(0)

#define AX_GZIP_DEV_LOG_NOTICE(fmt,...) do{\
                ax_pr_notice(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }while(0)

#define AX_GZIP_DEV_LOG_INFO(fmt,...) do{\
                ax_pr_info(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }while(0)

#define AX_GZIP_DEV_LOG_DBG(fmt,...) do{\
                ax_pr_debug(AX_ID_AXGZIPD, "AXGZIPD", "[%s] [%d] :" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }while(0)
#else /* AX_SYSLOG_EN */
#define AX_GZIP_DEV_LOG_DBG(fmt, arg...) printk("[GZIP][D]: [%6d][%s : %d] " fmt "\n", AX_GZIPD_GET_PID, __func__, __LINE__, ##arg)
#define AX_GZIP_DEV_LOG_ERR(fmt, arg...) printk("[GZIP][E]: [%6d][%s : %d] " fmt "\n", AX_GZIPD_GET_PID, __func__, __LINE__, ##arg)
#endif
#else /* AX_GZIP_DEBUG_LOG_EN */
#define AX_GZIP_DEV_LOG_EMERG(fmt,...)
#define AX_GZIP_DEV_LOG_ALERT(fmt,...)
#define AX_GZIP_DEV_LOG_CRIT(fmt,...)
#define AX_GZIP_DEV_LOG_ERR(fmt,...)
#define AX_GZIP_DEV_LOG_WARNING(fmt,...)
#define AX_GZIP_DEV_LOG_NOTICE(fmt,...)
#define AX_GZIP_DEV_LOG_INFO(fmt,...)
#define AX_GZIP_DEV_LOG_DBG(fmt, arg...)
#endif


#endif
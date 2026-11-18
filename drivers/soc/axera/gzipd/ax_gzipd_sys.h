/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_GZIPD_DEV_SYS__
#define __AX_GZIPD_DEV_SYS__

#include "ax_base_type.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define GZIPD_NAME  "/dev/gzipd"

#define GZIPD_IOCTL_MAGIC_NUM  'z'

#define GZIPD_IOCTL_CMD_DEV_INIT            _IO(GZIPD_IOCTL_MAGIC_NUM, 0)
#define GZIPD_IOCTL_CMD_DEV_CREATE_HANDLE   _IOWR(GZIPD_IOCTL_MAGIC_NUM, 1, void *)
#define GZIPD_IOCTL_CMD_DEV_CONFIG          _IOWR(GZIPD_IOCTL_MAGIC_NUM, 2, void *)
#define GZIPD_IOCTL_CMD_DEV_TILES_RUN       _IOWR(GZIPD_IOCTL_MAGIC_NUM, 3, void *)
#define GZIPD_IOCTL_CMD_DEV_LASTTILE_RUN    _IOWR(GZIPD_IOCTL_MAGIC_NUM, 4, void *)
#define GZIPD_IOCTL_CMD_DEV_DESTROY_HANDLE  _IOWR(GZIPD_IOCTL_MAGIC_NUM, 5, void *)
#define GZIPD_IOCTL_CMD_DEV_DEINIT          _IOWR(GZIPD_IOCTL_MAGIC_NUM, 6, void *)
#define GZIPD_IOCTL_CMD_DEV_WAIT_FINISH     _IOWR(GZIPD_IOCTL_MAGIC_NUM, 7, void *)

typedef struct {
	AX_CHAR ctype[2];
	AX_U16 nBlkNum;
	AX_U32 nOutSize;
	AX_U32 nInSize;
	AX_U32 nICrc32;
} GZIPD_FILE_HEADER_T;

typedef enum {
	gzipd_error = -1,
	gzipd_ok = 0,
} gzipd_ret_e;

typedef struct {
    AX_U32 handle;
    AX_U64 u64PhyAddr;
    AX_U64 u64DataLen;
	AX_U32 u32EnquedTileCnt;
	AX_U32 u32CompleteLen;
} GZIPD_DEV_PARAM_INFO_T;

typedef struct {
    AX_U32 handle;
    AX_VOID *pInputParam;
} GZIPD_DEV_CONF_T;

#endif
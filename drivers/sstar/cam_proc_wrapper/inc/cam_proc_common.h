/*
 * cam_proc_common.h- Sigmastar
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

#ifndef __CAM_PROC_COMMON_H__
#define __CAM_PROC_COMMON_H__

#include "cam_os_util_list.h"
#include "cam_proc_wrapper.h"

#ifdef CAM_OS_RTK
typedef struct
{
    struct CamOsListHead_t link;
    char                   aEntryName[31];
    struct CamOsListHead_t sSubNodeList;
    CamProcReadCb          pfReadCb;
    CamProcWriteCb         pfWriteCb;
    void*                  pPrivData;
} CamProcEntryPriv_t;

#elif defined(__KERNEL__)
#include <linux/seq_file.h>

typedef struct
{
    void (*cat)(CamProcSeqFile_t *, void *);
    void (*echo)(char *, int, void *);
    void *pPrivData;
} CamProcNodeOp_t;

typedef struct
{
    struct proc_ops pFileOps;
    CamProcNodeOp_t sProcOps;
} CamProcLinuxPrivate_t;

typedef struct
{
    struct CamOsListHead_t link;
    char                   aEntryName[31];
    struct CamOsListHead_t sSubNodeList;
    struct proc_dir_entry *pde;
    CamProcLinuxPrivate_t *pdata;
} CamProcEntryPriv_t;
#endif

extern struct CamOsListHead_t gProcRootList;

#endif /* __CAM_PROC_COMMON_H__ */

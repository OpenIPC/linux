/*
 * cam_proc_wrapper.h- Sigmastar
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

///////////////////////////////////////////////////////////////////////////////
/// @file      cam_proc_wrapper.h
/// @brief     Cam PROC Wrapper Header File for
///            1. RTK OS
///            2. Linux Kernel Space
///////////////////////////////////////////////////////////////////////////////

#ifndef __CAM_PROC_WRAPPER_H__
#define __CAM_PROC_WRAPPER_H__

#include "cam_os_wrapper.h"

typedef void CamProcEntry_t;

#ifdef CAM_OS_RTK
typedef struct CamProcSeqBuf_s
{
    void *buf;
    u32   size;
    u32   used;
} CamProcSeqFile_t;
#elif defined(__KERNEL__)
typedef struct seq_file CamProcSeqFile_t;
#endif

typedef void (*CamProcReadCb)(CamProcSeqFile_t *pProcBuf, void *pPrivData);
typedef void (*CamProcWriteCb)(char *pProcBuf, int nLen, void *pPrivData);

//=============================================================================
// Description:
//      Create virtual directory entry
// Parameters:
//      [in]  name: Pointer to a naming buffer with end character for this
//                  directory entry.
//      [in]  pParentEntry: Pointer to parent directory entry, it can be NULL
//                          when creating root entry
// Return:
//      Pointer to a entry handler
//=============================================================================
CamProcEntry_t *CamProcMkdir(const char *name, CamProcEntry_t *pParentEntry);

//=============================================================================
// Description:
//      Create sub-node under a specified directory entry
// Parameters:
//      [in]  name: Pointer to a naming buffer with end character for this
//                  sub-node.
//      [in]  pParentEntry: Pointer to parent directory entry, it cannot be NULL.
//      [in]  pCatCB: Pointer to callback function for cat command.
//      [in]  pEchoCB: Pointer to callback function for echo command.
// Return:
//      Pointer to a sub-node handler
//=============================================================================
CamProcEntry_t *CamProcCreate(const char *name, CamProcEntry_t *pParentEntry, CamProcReadCb pReadCb,
                              CamProcWriteCb pWriteCb, void *pPrivData);

//=============================================================================
// Description:
//      Remove directory entry/entries or node/nodes
// Parameters:
//      [in]  name: Pointer to a naming buffer with end character for the
//                  entry/node.
//      [in]  pParentEntry: Pointer to parent directory entry, it can be NULL
//                          only when the entry it root entry
// Return:
//      N/A
//=============================================================================
void CamProcRemoveEntry(const char *name, CamProcEntry_t *pEntry);

//=============================================================================
// Description:
//      Writes the C string pointed by format to seq_struct.
// Parameters:
//      [in]  pSeq: seq_struct handle
//      [in]  fmt: C string that contains the text to be written, it can
//                 optionally contain embedded format specifiers.
// Return:
//      N/A
//=============================================================================
void CamProcSeqPrintf(CamProcSeqFile_t *pSeq, const char *fmt, ...);

#endif /* __CAM_PROC_WRAPPER_H__ */

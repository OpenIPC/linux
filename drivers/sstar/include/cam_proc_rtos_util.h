/*
 * cam_proc_rtos_util.h- Sigmastar
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
/// @file      cam_proc_rtos_util.h
/// @brief     Cam PROC Wrapper Header File for
///            1. RTK OS
///            2. Linux Kernel Space
///////////////////////////////////////////////////////////////////////////////

#ifndef __CAM_PROC_RTOS_UTIL_H__
#define __CAM_PROC_RTOS_UTIL_H__

#include "cam_proc_wrapper.h"

//=============================================================================
// Description:
//      Execute read operation of a procfs sub-node
// Parameters:
//      [in]  name: Pointer to a naming buffer with end character for this
//                  sub-node.
//      [in]  pSeq: seq_struct handle
// Return:
//      N/A
//=============================================================================
void CamProcExecRead(char *name, CamProcSeqFile_t *pSeq);

//=============================================================================
// Description:
//      Execute write operation of a procfs sub-node
// Parameters:
//      [in]  name: Pointer to a naming buffer with end character for this
//                  sub-node.
//      [in]  stSeq: seq_struct handle
// Return:
//      N/A
//=============================================================================
void CamProcExecWrite(char *name, char *pBuf);

//=============================================================================
// Description:
//      List all entry/node in specific path
// Parameters:
//      [in]  name: Pointer to a naming buffer with end character for this
//                  sub-node.
// Return:
//      N/A
//=============================================================================
void CamProcListEntry(char *name);

#endif /* __CAM_PROC_RTOS_UTIL_H__ */

/**
    vdodisp, Service command function declare

    @file       vdodisp_cmd.h
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _VDODISPCMD_H
#define _VDODISPCMD_H

#include "../vdodisp_api.h"

typedef VDODISP_ER(*VDODISP_FP_CMD)(const VDODISP_CMD *p_cmd);  ///< for Command Function Table

/**
    Command Function Table Element
*/
typedef struct _VDODISP_CMD_DESC {
	VDODISP_CMD_IDX  idx;            ///< Command Index
	VDODISP_FP_CMD   fp_cmd;          ///< mapped function pointer
	UINT32          ui_num_byte_in;    ///< require data in size
	UINT32          ui_num_byte_out;   ///< require data out size
} VDODISP_CMD_DESC;

const VDODISP_CMD_DESC *x_vdodisp_get_call_tbl(UINT32 *p_num);

VDODISP_ER x_vdodisp_cmd_set_disp(const VDODISP_CMD *p_cmd);
VDODISP_ER x_vdodisp_cmd_get_disp(const VDODISP_CMD *p_cmd);
VDODISP_ER x_vdodisp_cmd_set_flip(const VDODISP_CMD *p_cmd);
VDODISP_ER x_vdodisp_cmd_dump_flip(const VDODISP_CMD *p_cmd);
VDODISP_ER x_vdodisp_cmd_release(const VDODISP_CMD *p_cmd);
VDODISP_ER x_vdodisp_cmd_set_event_cb(const VDODISP_CMD *p_cmd);
VDODISP_ER x_vdodisp_cmd_get_event_cb(const VDODISP_CMD *p_cmd);

#endif

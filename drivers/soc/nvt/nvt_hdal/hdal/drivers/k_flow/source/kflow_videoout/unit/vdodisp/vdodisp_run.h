/**
    vdodisp, Task running operation declare

    @file       vdodisp_run.h
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _VDODISPRUN_H
#define _VDODISPRUN_H

#include "vdodisp_int.h"

VDODISP_ER x_vdodisp_run_open(VDODISP_DEVID device_id);
VDODISP_ER x_vdodisp_run_close(VDODISP_DEVID device_id);
VDODISP_ER x_vdodisp_run_suspend(VDODISP_DEVID device_id);
VDODISP_ER x_vdodisp_run_resume(VDODISP_DEVID device_id);
VDODISP_ER x_vdodisp_run_cmd(VDODISP_DEVID device_id, const VDODISP_CMD *p_cmd);

#endif

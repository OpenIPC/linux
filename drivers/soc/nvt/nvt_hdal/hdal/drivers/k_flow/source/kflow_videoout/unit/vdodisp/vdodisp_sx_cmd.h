/**
    vdodisp, export commands to SxCmd

    @file       vdodisp_sx_cmd.h
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _VDODISPSXCMD_H
#define _VDODISPSXCMD_H

BOOL x_vdodisp_sx_cmd_dump(unsigned char argc, char **argv);
BOOL x_vdodisp_sx_cmd_dump_buf(unsigned char argc, char **argv);
BOOL x_vdodisp_sx_cmd_fps(unsigned char argc, char **argv);
#endif

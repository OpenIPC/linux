/**
    WavStudio, export commands to SxCmd

    @file       wavstudio_sxcmd.h
    @ingroup    mILIBWAVSTUDIO

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _WAVSTUDIOCMD_H
#define _WAVSTUDIOCMD_H

void xWavStudio_InstallCmd(void);
extern int _wavstudio_sxcmd(char* sub_cmd_name, char *cmd_args);
extern int _wavstudio_sxcmd_showhelp(void);

#endif

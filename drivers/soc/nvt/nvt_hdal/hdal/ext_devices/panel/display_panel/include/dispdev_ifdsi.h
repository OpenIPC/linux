/*
    Display object object Golden Sample for driving Memory interface panel

    @file       dispdev_gsinfmi.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _DISPDEV_GSINFDSI_H
#define _DISPDEV_GSINFDSI_H
#include "dispdev_panlcomn.h"
#include "dispdev_platform.h"

#define CMDDELAY_MS         0x80000000
#define CMDDELAY_US         0x40000000
#define CMDTEON             0x20000000
#define CMDTEEXTPIN         0x10000000
#define CMDTEONEACHFRAME    0x04000000
#define CMDBTA              0x08000000
#define CMDTEONEACHFRAMERTY 0x00800000
#define CMDBTAONLY               0x00400000
#define CMDTEONEACHFRAMERTYTWICE 0x00200000

#define DSICMD_CMD          0x01000000
#define DSICMD_DATA         0x02000000




typedef struct {
	DSI_PIXEL_FORMATSEL     pixel_fmt;
} DSI_CONFIG, *PDSI_CONFIG;





T_LCD_PARAM *dispdev_get_config_mode_dsi(UINT32 *mode_number);
T_PANEL_CMD *dispdev_get_standby_cmd_dsi(UINT32 *cmd_number);


T_LCD_ROT *dispdev_get_lcd_rotate_dsi_cmd(UINT32 *mode_number);
void      dispdev_set_dsi_config(DSI_CONFIG *p_dsi_config);

#endif

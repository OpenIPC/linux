/* drivers/video/sunxi/disp2/disp/lcd/lindevb720p.h
 *
 * lindevb720p panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __LINDEVB720P_H__
#define __LINDEVB720P_H__

#include "panels.h"

extern struct __lcd_panel lindevb720p_panel;

extern s32 bsp_disp_get_panel_info(u32 screen_id, struct disp_panel_para *info);

#endif /*End of file*/

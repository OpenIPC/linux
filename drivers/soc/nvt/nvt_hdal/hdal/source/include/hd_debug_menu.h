/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_debug_menu.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _HD_DEBUG_MENU_H_
#define _HD_DEBUG_MENU_H_

#define HD_DEBUG_MENU_ID_QUIT 0xFE
#define HD_DEBUG_MENU_ID_RETURN 0xFF
#define HD_DEBUG_MENU_ID_LAST (-1)

typedef struct _HD_DBG_MENU {
	int            menu_id;          ///< user command value
	const char     *p_name;          ///< command string
	int            (*p_func)(void);  ///< command function
	BOOL           b_enable;         ///< execution option
} HD_DBG_MENU;

void hd_debug_menu_print_p(HD_DBG_MENU *p_menu, const char *p_title);
int hd_debug_menu_exec_p(int menu_id, HD_DBG_MENU *p_menu);
int hd_debug_menu_entry_p(HD_DBG_MENU *p_menu, const char *p_title);

#endif

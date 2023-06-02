/*
	Drvdump header
	Copyright Novatek Microelectronics Corp. 2013.  All rights reserved.
*/

#ifndef __KDRVDUMP_H
#define __KDRVDUMP_H
#include "kwrap/type.h"

void pwm_dump_info(void);
void idec_dump_info(UINT32 id);
void eac_dump_info(void);
BOOL ssenif_debug_dump(CHAR *str_cmd);
void dsi_dump_info(void);
void idetv_dump_info(void);


#endif


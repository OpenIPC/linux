/**
	@brief Source code of logger function.\n
	This file contains the logger function, and logger menu entry point.

	@file hd_debug_menu.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _HD_LOGGER_P_H_
#define _HD_LOGGER_P_H_

void hd_logger_init_p(unsigned int cpu_cfg);
void hd_logger_uninit_p(void);
void hdal_flow_log_p(const char *fmt, ...); //this api name synced from 313
void vprintm_p(const char *fmt, va_list ap);

#endif

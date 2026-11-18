// SPDX-License-Identifier: GPL-2.0
/*
 * u_uac2.h
 *
 * Utility definitions for UAC2 function
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 */

#ifndef U_UAC2_H
#define U_UAC2_H

#include <linux/usb/composite.h>

#define UAC2_DEF_PCHMASK 0x3
#define UAC2_DEF_PSRATE 48000
#define UAC2_DEF_PSSIZE 2
#define UAC2_DEF_CCHMASK 0x3
// ### SIPEED EDIT ###
#define UAC2_DEF_CSRATE 48000
#define UAC2_DEF_CSSIZE 2
#define UAC2_DEF_CSYNC		USB_ENDPOINT_SYNC_ASYNC
#define UAC2_DEF_REQ_NUM 6

#define UAC2_DEF_P_TERM_TYPE 0x301
	/* UAC_OUTPUT_TERMINAL_SPEAKER */
#define UAC2_DEF_C_TERM_TYPE 0x201
	/* UAC_INPUT_TERMINAL_MICROPHONE*/
// ### SIPEED EDIT END ###

struct f_uac2_opts {
	struct usb_function_instance	func_inst;
	int				p_chmask;
	int				p_srate;
	int				p_ssize;
	int				c_chmask;
	int				c_srate;
	int				c_ssize;
	// ### SIPEED EDIT ###
	int				c_sync;
	// ### SIPEED EDIT END ###
	int				req_number;
	bool				bound;

	// ### SIPEED EDIT ###
	char			function_name[32];

	s16				p_terminal_type;
	s16				c_terminal_type;
	// ### SIPEED EDIT END ###

	struct mutex			lock;
	int				refcnt;
};

#endif

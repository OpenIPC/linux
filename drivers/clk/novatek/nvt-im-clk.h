/**
    NVT clock header file
    This file will provide NVT clock related structure & API
    @file       nvt-im-clk.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __NVT_IM_CLK_H__
#define __NVT_IM_CLK_H__

#ifdef CONFIG_COMMON_CLK_NA51068
	#include "nvt-im-clk-bk.h"
#endif

#if defined(CONFIG_COMMON_CLK_NA51055) || defined(CONFIG_COMMON_CLK_NA51089)
	#include "nvt-im-clk-fr.h"
#endif

#endif

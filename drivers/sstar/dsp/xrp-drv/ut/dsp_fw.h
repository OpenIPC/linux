/*
 * dsp_fw.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _dsp_fw_h_
#define _dsp_fw_h_
#include "dsp_ut.h"
int dsp_ut_fw_Request(dsp_ut_core* dspUtCtx, char* fwPath);
int dsp_ut_fw_Load(dsp_ut_core* dspUtCtx);
int dsp_ut_fw_Release(dsp_ut_core* dspUtCtx);

#endif

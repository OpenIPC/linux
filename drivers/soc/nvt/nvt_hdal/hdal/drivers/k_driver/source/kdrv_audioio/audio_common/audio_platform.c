/*
    Audio module driver

    This file is the driver of audio module

    @file       Audio.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifdef __KERNEL__
#include "kwrap/type.h"
#include <linux/clk.h>
#include "audio_int.h"
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"

void _audio_platform_set_samplerate(int sample_rate){

#if _FPGA_EMULATION_
#else
	struct clk *pll7_clk;
	
	
	
			switch (sample_rate) {
			case AUDIO_SR_11025:
			case AUDIO_SR_22050:
			case AUDIO_SR_44100: {
				pll7_clk = clk_get(NULL, "pll7");
				if (IS_ERR(pll7_clk)) {
					DBG_ERR("get pll7 failed\r\n");
				}
				clk_set_rate(pll7_clk, 338688000);

			} break;
	
			case AUDIO_SR_8000:
			case AUDIO_SR_12000:
			case AUDIO_SR_16000:
			case AUDIO_SR_24000:
			case AUDIO_SR_32000:
			case AUDIO_SR_48000:
			default: {
				pll7_clk = clk_get(NULL, "pll7");
				if (IS_ERR(pll7_clk)) {
					DBG_ERR("get pll7 failed\r\n");
				}
				clk_set_rate(pll7_clk, 344064000);
			} break;
	
			}
			clk_put(pll7_clk);
#endif


}


#endif

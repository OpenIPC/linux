/*------------------------------------------------------------------------------
	Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
#ifndef _INFINITY_PCM_H_
#define _INFINITY_PCM_H_

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "infinity.h"

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define DMA_EMPTY 		   0
#define DMA_UNDERRUN		 1
#define DMA_OVERRUN      2
#define DMA_FULL         3
#define DMA_NORMAL 		   4


//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
extern unsigned long long g_nPlayStartTime;
extern unsigned long long g_nCapStartTime;


struct infinity_pcm_dma_data
{
  U8 *name;    /* stream identifier */
  U32 channel;    /* Channel ID */
  dma_addr_t dma_addr;
  S32 dma_size;    /* Size of the DMA transfer */
};

int infinity_audio_clk_enable(void);
int infinity_audio_clk_disable(struct snd_soc_codec *codec, int mode);

#endif /* _INFINITY_PCM_H_ */

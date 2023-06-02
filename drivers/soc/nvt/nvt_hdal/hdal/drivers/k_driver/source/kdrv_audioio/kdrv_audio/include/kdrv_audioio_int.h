/*
    DAI internal header file

    This file is the header file that define register for DAI module


    @file       dai_int.h
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/


#ifndef _KDRV_AUDIO_INT_H
#define _KDRV_AUDIO_INT_H

#ifdef __KERNEL__
#include <linux/delay.h>
#include <linux/list.h>
#include "kdrv_audioio/kdrv_audioio.h"
//#include "../audio_common/include/audio_int.h"
#elif defined(__FREERTOS)
#include "kdrv_audioio/kdrv_audioio.h"
#include "kdrv_audioio_list.h"
#endif
#include "audio_int.h"
#include "AudioCodec.h"
//#include "audio_dbg.h"

#define KDRV_LIST_NODE_NUM 5

#define KDRV_ERRFLAG_TX  (AUDIO_EVENT_BUF_FULL|AUDIO_EVENT_BUF_EMPTY|AUDIO_EVENT_FIFO_ERROR|AUDIO_EVENT_BUF_RTEMPTY|AUDIO_EVENT_BUF_FULL2|AUDIO_EVENT_DONEBUF_FULL2|AUDIO_EVENT_FIFO_ERROR2)
#define KDRV_ERRFLAG_RX  (AUDIO_EVENT_BUF_FULL|AUDIO_EVENT_BUF_EMPTY|AUDIO_EVENT_DONEBUF_FULL|AUDIO_EVENT_FIFO_ERROR|AUDIO_EVENT_BUF_RTEMPTY|AUDIO_EVENT_BUF_FULL2|AUDIO_EVENT_DONEBUF_FULL2|AUDIO_EVENT_FIFO_ERROR2)


typedef struct list_head LIST_HEAD;


typedef struct _KDRV_AUDIO_INFO {
	PAUDTS_OBJ  paud_drv_obj;
	UINT32      sample_rate;
	UINT32      channel;
	UINT32      bit_per_sample;
	UINT32      volume;
	AUDIO_TDMCH tdm_channel;
	BOOL        opened;
}KDRV_AUDIO_INFO;



typedef struct _KDRV_AUDIO_LIST_NODE{
    BOOL                b_used;
    KDRV_BUFFER_INFO    buffer_addr;
	KDRV_CALLBACK_FUNC  cb_func;
    int                 user_data;
	LIST_HEAD           list;
}KDRV_AUDIO_LIST_NODE;

typedef enum
{
    KDRV_HANDLE_RX1,
    KDRV_HANDLE_TXLB,
    KDRV_HANDLE_TX1,
    KDRV_HANDLE_TX2,
    KDRV_HANDLE_MAX,
    ENUM_DUMMY4WORD(KDRV_HANDLE)

}KDRV_HANDLE;

/*
    Audio Internal Flag
*/
typedef enum {
	KDRV_AUDIO_INT_FLAG_NONE             = 0x00000000,
	KDRV_AUDIO_INT_FLAG_LINEPWR_ALWAYSON = 0x00000001,
	KDRV_AUDIO_INT_FLAG_SPKPWR_ALWAYSON  = 0x00000002,
	KDRV_AUDIO_INT_FLAG_ADVCMPWR_ALWAYSON= 0x00000004, 
	KDRV_AUDIO_INT_FLAG_CLKALWAYS_ON     = 0x00000100,

    KDRV_AUDIO_INT_FLAG_RX_EXPAND        = 0x00001000,
    KDRV_AUDIO_INT_FLAG_TX_EXPAND        = 0x00002000,

    KDRV_AUDIO_INT_FLAG_RX_TIMECODE_HIT  = 0x00010000,
    KDRV_AUDIO_INT_FLAG_TX_TIMECODE_HIT  = 0x00020000,

	ENUM_DUMMY4WORD(KDRV_AUDIO_INT_FLAG)
} KDRV_AUDIO_INT_FLAG;



#ifdef __KERNEL__
#define audio_msg printk
#elif defined(__FREERTOS)
#define audio_msg vk_printk
#endif

#endif


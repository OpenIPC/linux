
#ifndef __RAPTOR5_COMMON_H__
#define __RAPTOR5_COMMON_H__

#ifndef NVT_PLATFORM
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#endif


#ifdef NVT_PLATFORM
#include "ad_drv_util.h"
#endif
#include "raptor5_fmt.h"
#include "raptor5_ioctl.h"


typedef struct _nc_decoder_info_s
{
	NC_U8 Total_Chip_Cnt;
	NC_U8 Total_Chn_Cnt;
	NC_U8 chip_id[4];
	NC_U8 chip_rev[4];
	NC_U8 chip_addr[4];
	NC_U8 Total_Port_Num[4];

	NC_U8 chn_alive[16];
	NC_U8 vfc[16];
	NC_FORMAT_FPS_E        fmt_fps[16];
	NC_FORMAT_STANDARD_E   fmt_standard[16];
	NC_FORMAT_RESOLUTION_E fmt_resolution[16];
	NC_VIVO_CH_FORMATDEF_E fmt_video[16];
	NC_CABLE_E             fmt_cable[16];
	NC_U8 eq_stage[16];

	NC_VO_WORK_MODE_E vo_mode[16];

	NC_S8  fmt_name[16][256];
	NC_U8  coax_fw_status[16];
	NC_U32 sleep[16];
	NC_U8  on_video_set[16];
	NC_U8  reserved_e;
	NC_U8  reserved_f;

}nc_decoder_info_s;


extern NC_U8 g_nc_drv_i2c_addr[4];

#define  gpio_i2c_read   __I2CReadByte8
#define  gpio_i2c_write  __I2CWriteByte8

#define NC_DEVICE_DRIVER_BANK_SET(dev, bank) gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xFF, bank );

#define PORTA		0x00
#define PORTB		0x01
#define PORTC		0x02
#define PORTD		0x03
#define PORTAB		0x04
#define PORTCD		0x05

#define FUNC_ON		0x01
#define FUNC_OFF	0x00

#define BIT_MASK_0_1111_1110 0xFE
#define BIT_MASK_1_1111_1101 0xFD
#define BIT_MASK_2_1111_1011 0xFB
#define BIT_MASK_3_1111_0111 0xF7
#define BIT_MASK_4_1110_1111 0xEF
#define BIT_MASK_5_1101_1111 0xDF
#define BIT_MASK_6_1011_1111 0xBF
#define BIT_MASK_7_0111_1111 0x7F

#define BIT_MASK_0_0000_0001 0x01
#define BIT_MASK_1_0000_0010 0x02
#define BIT_MASK_2_0000_0100 0x04
#define BIT_MASK_3_0000_1000 0x08
#define BIT_MASK_4_0001_0000 0x10
#define BIT_MASK_5_0010_0000 0x20
#define BIT_MASK_6_0100_0000 0x40
#define BIT_MASK_7_1000_0000 0x80


#define BANK_0  0x00
#define BANK_1  0x01
#define BANK_2  0x02
#define BANK_3  0x03
#define BANK_4  0x04
#define BANK_5  0x05
#define BANK_9  0x09
#define BANK_A  0x0A
#define BANK_B  0x0B
#define BANK_C  0x0C
#define BANK_11 0x11
#define BANK_12 0x12
#define BANK_13 0x13
#define BANK_20 0x20
#define BANK_21 0x21
#define BANK_22 0x22
#define BANK_23 0x23

#define FW_SUCCESS 0
#define FW_FAILURE -1

#define EOD (-1)

#if 0
#define CHANGEDRIVER 1
#define UNUSED(x) ((void)(x))
#endif

typedef enum _nvp6188_outmode_sel
{
	NVP6188_OUTMODE_1MUX = 0,
	NVP6188_OUTMODE_2MUX,
	NVP6188_OUTMODE_4MUX,
	NVP6188_OUTMODE_BUTT
}NVP6188_OUTMODE_SEL;

extern void __I2CWriteByte8(unsigned char chip_addr, unsigned char reg_addr, unsigned char value);
extern unsigned char __I2CReadByte8(unsigned char chip_addr, unsigned char reg_addr);


/********************************************************************
 *  1. Decoder Information Set
 ********************************************************************/
void nc_drv_common_info_chn_alive_set( NC_U8 chn, NC_U8 alive );
void nc_drv_common_info_chip_data_init_set( int ChipCnt, NC_U8 *id, NC_U8 *rev, NC_U8 *addr, NC_VO_WORK_MODE_E vomux );
void nc_drv_common_info_video_format_set( NC_U8 chn, NC_U8 vfc );
void nc_drv_common_info_video_format_manual_set( NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt );
void nc_drv_common_info_cable_set( NC_U8 chn, NC_CABLE_E cable );
void nc_drv_common_info_eq_stage_set( NC_U8 chn, NC_U8 eq_stage );
void nc_drv_common_info_video_fmt_def_set( NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt );
void nc_drv_common_info_video_fmt_standard_set( NC_U8 chn, NC_FORMAT_STANDARD_E  fmtstd );
void nc_drv_common_info_video_fmt_resolusion_set( NC_U8 chn, NC_FORMAT_RESOLUTION_E fmtres);
void nc_drv_common_info_video_fmt_fps_set( NC_U8 chn, NC_FORMAT_FPS_E fmtfps );

void nc_drv_common_info_coax_fw_status_set( NC_U8 chn, NC_U8 status );
void nc_drv_common_info_vo_mode_set(NC_U8 dev, NC_VO_WORK_MODE_E mode);

/********************************************************************
 *  2. Decoder Information Get
 ********************************************************************/
NC_U8 nc_drv_common_info_chip_id_get(NC_U8 ChipNo);
void  nc_drv_common_info_video_fmt_string_get( NC_U8 chn, char *pstr );
NC_U8 nc_drv_common_info_chn_alive_get( NC_U8 chn );
NC_U8 nc_drv_common_info_vfc_get( NC_U8 chn );
NC_U8 nc_drv_common_info_eq_stage_get( NC_U8 chn );
NC_U8 nc_drv_common_total_chn_count_get( NC_U8 chn );
NC_U8 nc_drv_common_info_coax_fw_update_status_get( NC_U8 chn );
NC_VO_WORK_MODE_E nc_drv_common_info_vo_mode_get(NC_U8 dev);

// Return_Enum Function
NC_CABLE_E 			   nc_drv_common_info_cable_get( NC_U8 chn );
NC_VIVO_CH_FORMATDEF_E nc_drv_common_info_video_fmt_def_get( NC_U8 chn );
NC_FORMAT_STANDARD_E   nc_drv_common_info_video_fmt_standard_get( NC_U8 chn );
NC_FORMAT_RESOLUTION_E nc_drv_common_info_video_fmt_resolusion_get( NC_U8 chn );
NC_FORMAT_FPS_E 	   nc_drv_common_info_video_fmt_fps_get( NC_U8 chn );

void  nc_drv_common_info_status_onvideo_set(NC_U8 chn, NC_U8 status);
NC_U8 nc_drv_common_info_status_onvideo_get(NC_U8 chn);


/**************************************************************************************
 * get enum_string Function
 *
 ***************************************************************************************/
char *nc_drv_common_chip_id_str_get(NC_U8 id);
char *nc_drv_common_mipi_lane_str_get(NC_MIPI_LANE_E lane);
char *nc_drv_common_mipi_clock_str_get(NC_MIPI_BPS_E clk);


/********************************************************************
 *  3. Sleep Time Set
 ********************************************************************/
void nc_drv_common_sleep_set( NC_U8 chn, NC_U8 frame );



#endif


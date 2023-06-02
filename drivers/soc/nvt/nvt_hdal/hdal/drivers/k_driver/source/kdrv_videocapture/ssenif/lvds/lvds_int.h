/*
    LVDS/HiSPi Controller internal header

    LVDS/HiSPi Controller internal header

    @file       lvds.h
    @ingroup    mIDrvIO_LVDS
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifndef _LVDS_INT_H
#define _LVDS_INT_H

//#include "DrvCommon.h"

#include "include/lvds_reg.h"
#include "../senphy.h"
#include "rule_check.h"

#ifdef __KERNEL__
#include "lvds_drv.h"
#include "lvds_dbg.h"
extern PLVDS_MODULE_INFO plvds_mod_info;
#else
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "rcw_macro.h"
#include "io_address.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#endif

/*
    @addtogroup mIDrvIO_LVDS
*/
//@{

//
//  LVDS register access definition
//
#ifdef __KERNEL__
extern UINT32 _LVDS_REG_BASE_ADDR[MODULE_REG_NUM];
#define LVDS_R_REGISTER(ofs)         ioread32((void *)(_LVDS_REG_BASE_ADDR[0] + ofs))
#define LVDS_W_REGISTER(ofs, value)  iowrite32(value, (void *)(_LVDS_REG_BASE_ADDR[0] + ofs))

#define LVDS2_R_REGISTER(ofs)        ioread32((void *)(_LVDS_REG_BASE_ADDR[1] + ofs))
#define LVDS2_W_REGISTER(ofs, value) iowrite32(value, (void *)(_LVDS_REG_BASE_ADDR[1] + ofs))
#else
#define LVDS_R_REGISTER(ofs)             INW( IOADDR_LVDS_REG_BASE+(ofs))
#define LVDS_W_REGISTER(ofs,value)       OUTW(IOADDR_LVDS_REG_BASE+(ofs),(value))

#define LVDS2_R_REGISTER(ofs)             INW( IOADDR_LVDS2_REG_BASE+(ofs))
#define LVDS2_W_REGISTER(ofs,value)       OUTW(IOADDR_LVDS2_REG_BASE+(ofs),(value))

#define wai_sem vos_sem_wait
#define sig_sem vos_sem_sig

#define FLGPTN_LVDS  FLGPTN_BIT(0)
#define FLGPTN_LVDS2 FLGPTN_BIT(1)

#endif


#ifdef _NVT_FPGA_
#define LVDS_DEFAULT_INT            0//(LVDS_INTERRUPT_FIFO_ER|LVDS_INTERRUPT_PIXCNT_ER|LVDS_INTERRUPT_PIXCNT_ER2)
#else
#define LVDS_DEFAULT_INT            (LVDS_INTERRUPT_FIFO_ER|LVDS_INTERRUPT_PIXCNT_ER|LVDS_INTERRUPT_FIFO1_OV|LVDS_INTERRUPT_PIXCNT_ER2)
#endif
#define LVDS_ERR_CHK_INT			(LVDS_INTERRUPT_FIFO_ER|LVDS_INTERRUPT_FIFO2_ER|LVDS_INTERRUPT_FIFO3_ER|LVDS_INTERRUPT_FIFO4_ER| \
									LVDS_INTERRUPT_PIXCNT_ER|LVDS_INTERRUPT_PIXCNT_ER2|LVDS_INTERRUPT_PIXCNT_ER3|LVDS_INTERRUPT_PIXCNT_ER4| \
									LVDS_INTERRUPT_FIFO1_OV|LVDS_INTERRUPT_FIFO2_OV|LVDS_INTERRUPT_FIFO3_OV|LVDS_INTERRUPT_FIFO4_OV)
#ifdef _NVT_EMULATION_
#define LVDS_DEBUG                  ENABLE
#else
#define LVDS_DEBUG                  DISABLE
#endif
#define LVDS_EN_RESET               DISABLE

//
//  Driver internal usage definitions
//

#define LVDS_PXIELOUT_MSK           0xF
#define LVDS_ANALOGDLY_MSK          0xF



void lvds_create_resource(void);
void lvds_release_resource(void);
void lvds2_create_resource(void);
void lvds2_release_resource(void);
void lvds3_create_resource(void);
void lvds3_release_resource(void);
void lvds4_create_resource(void);
void lvds4_release_resource(void);
void lvds5_create_resource(void);
void lvds5_release_resource(void);
void lvds6_create_resource(void);
void lvds6_release_resource(void);
void lvds7_create_resource(void);
void lvds7_release_resource(void);
void lvds8_create_resource(void);
void lvds8_release_resource(void);

#define LVDS_REMAP_WIDTH_1CH(x)     ((x)<<1)
#define LVDS_REMAP_WIDTH_3CH(x)     (((x)<<2)/3+1)

extern void lvds_enable_errchk_timer(LVDS_ID id, BOOL enable);

extern BOOL lvds_print_info_to_uart(CHAR *str_cmd);
extern BOOL lvds2_print_info_to_uart(CHAR *str_cmd);
extern BOOL lvds3_print_info_to_uart(CHAR *str_cmd);
extern BOOL lvds4_print_info_to_uart(CHAR *str_cmd);
extern BOOL lvds5_print_info_to_uart(CHAR *str_cmd);
extern BOOL lvds6_print_info_to_uart(CHAR *str_cmd);
extern BOOL lvds7_print_info_to_uart(CHAR *str_cmd);
extern BOOL lvds8_print_info_to_uart(CHAR *str_cmd);

//extern void lvds_installCmd(void);


extern void lvds_set_padlane_config(LVDS_IN_VALID  lane_select,  LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern void lvds2_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern void lvds3_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern void lvds4_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern void lvds5_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern void lvds6_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern void lvds7_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern void lvds8_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);

void lvds_trigger_relock(void);
void lvds2_trigger_relock(void);



//
//  LVDS Exporting APIs
//
extern ER               lvds_open(void);
extern ER               lvds_close(void);
extern BOOL             lvds_is_opened(void);

extern ER               lvds_set_enable(BOOL benable);
extern BOOL             lvds_get_enable(void);
extern ER               lvds_enable_streaming(void);

extern void             lvds_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds_dump_debug_info(UINT32 clock_frequency);
#ifdef __KERNEL__
extern void				lvds_isr(void);
#else
extern irqreturn_t		lvds_isr(int irq, void *devid);
#endif
extern UINT32			lvds_isr_check(void);

//
//  LVDS2 Exporting APIs
//
extern ER               lvds2_open(void);
extern ER               lvds2_close(void);
extern BOOL             lvds2_is_opened(void);

extern ER               lvds2_set_enable(BOOL benable);
extern BOOL             lvds2_get_enable(void);
extern ER               lvds2_enable_streaming(void);

extern void             lvds2_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds2_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds2_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds2_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds2_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds2_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds2_dump_debug_info(UINT32 clock_frequency);
#ifdef __KERNEL__
extern void				lvds2_isr(void);
#else
extern irqreturn_t		lvds2_isr(int irq, void *devid);
#endif

extern UINT32			lvds2_isr_check(void);


//
//  LVDS3 Exporting APIs
//
extern ER               lvds3_open(void);
extern ER               lvds3_close(void);
extern BOOL             lvds3_is_opened(void);

extern ER               lvds3_set_enable(BOOL benable);
extern BOOL             lvds3_get_enable(void);
extern ER               lvds3_enable_streaming(void);

extern void             lvds3_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds3_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds3_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds3_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds3_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds3_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds3_dump_debug_info(UINT32 clock_frequency);
extern void				lvds3_isr(void);
extern UINT32			lvds3_isr_check(void);

//
//  LVDS4 Exporting APIs
//
extern ER               lvds4_open(void);
extern ER               lvds4_close(void);
extern BOOL             lvds4_is_opened(void);

extern ER               lvds4_set_enable(BOOL benable);
extern BOOL             lvds4_get_enable(void);
extern ER               lvds4_enable_streaming(void);

extern void             lvds4_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds4_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds4_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds4_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds4_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds4_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds4_dump_debug_info(UINT32 clock_frequency);
extern void				lvds4_isr(void);
extern UINT32			lvds4_isr_check(void);


//
//  LVDS5 Exporting APIs
//
extern ER               lvds5_open(void);
extern ER               lvds5_close(void);
extern BOOL             lvds5_is_opened(void);

extern ER               lvds5_set_enable(BOOL benable);
extern BOOL             lvds5_get_enable(void);
extern ER               lvds5_enable_streaming(void);

extern void             lvds5_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds5_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds5_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds5_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds5_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds5_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds5_dump_debug_info(UINT32 clock_frequency);
extern void				lvds5_isr(void);
extern UINT32			lvds5_isr_check(void);


//
//  LVDS6 Exporting APIs
//
extern ER               lvds6_open(void);
extern ER               lvds6_close(void);
extern BOOL             lvds6_is_opened(void);

extern ER               lvds6_set_enable(BOOL benable);
extern BOOL             lvds6_get_enable(void);
extern ER               lvds6_enable_streaming(void);

extern void             lvds6_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds6_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds6_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds6_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds6_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds6_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds6_dump_debug_info(UINT32 clock_frequency);
extern void				lvds6_isr(void);
extern UINT32			lvds6_isr_check(void);

//
//  LVDS7 Exporting APIs
//
extern ER               lvds7_open(void);
extern ER               lvds7_close(void);
extern BOOL             lvds7_is_opened(void);

extern ER               lvds7_set_enable(BOOL benable);
extern BOOL             lvds7_get_enable(void);
extern ER               lvds7_enable_streaming(void);

extern void             lvds7_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds7_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds7_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds7_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds7_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds7_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds7_dump_debug_info(UINT32 clock_frequency);
extern void				lvds7_isr(void);
extern UINT32			lvds7_isr_check(void);

//
//  LVDS7 Exporting APIs
//
extern ER               lvds8_open(void);
extern ER               lvds8_close(void);
extern BOOL             lvds8_is_opened(void);

extern ER               lvds8_set_enable(BOOL benable);
extern BOOL             lvds8_get_enable(void);
extern ER               lvds8_enable_streaming(void);

extern void             lvds8_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds8_get_config(LVDS_CONFIG_ID config_id);

extern void             lvds8_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value);
extern UINT32           lvds8_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id);

extern void             lvds8_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word);
extern LVDS_INTERRUPT   lvds8_wait_interrupt(LVDS_INTERRUPT waited_flag);

extern void             lvds8_dump_debug_info(UINT32 clock_frequency);
extern void				lvds8_isr(void);
extern UINT32			lvds8_isr_check(void);


//@}
#endif

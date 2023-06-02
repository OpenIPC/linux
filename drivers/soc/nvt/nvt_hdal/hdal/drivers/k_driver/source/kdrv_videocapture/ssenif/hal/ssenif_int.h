/*
    Internal Header file of Serial Sensor Interface DAL library

    Internal header file of Serial Sensor Interface DAL library.

    @file       ssenif_int.h
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _SSENIF_INT_H
#define _SSENIF_INT_H

#ifndef __KERNEL__
#include <kwrap/type.h>
#include <kwrap/error_no.h>
#include "dal_ssenif.h"
#include "../senphy.h"
#include "../lvds.h"
#include "../csi.h"
#else
#include <mach/rcw_macro.h>
#include <kwrap/type.h>
#include <kwrap/flag.h>
#include "dal_ssenif.h"
#include "../senphy.h"
#include "../lvds.h"
#include "../csi.h"
#include "ssenif_dbg.h"
#endif


typedef struct {

	DAL_SSENIF_SENSORTYPE   sensor_type;
	DAL_SSENIFVX1_TXTYPE    tansmitter;
	DAL_SSENIFVX1_DATAMUX   data_mux;

	UINT8                   mipi_lane_number;
	UINT8                   vx1_lane_number;
	UINT8                   pixel_depth;
	UINT8                   hdr_enable;
	BOOL                    nack_check;
	UINT32                  sen_slave_addr;
	UINT8                   thcv241pll[8];

} SSENIFSTRUCT_VX1, *PSSENIFSTRUCT_VX1;


extern BOOL             ssenif_list_name_dump;
extern DAL_SSENIF_ID    ssenif_lastest_start;

extern void ssenif_install_cmd(void);

extern ER                   ssenif_init_csi(void);
extern ER                   ssenif_open_csi(void);
extern BOOL                 ssenif_is_opened_csi(void);
extern ER                   ssenif_close_csi(void);
extern ER                   ssenif_start_csi(void);
extern ER                   ssenif_stop_csi(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi(void);
extern void                 ssenif_set_config_csi(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi(DAL_SSENIFCSI_CFGID config_id);

extern ER                   ssenif_init_csi2(void);
extern ER                   ssenif_open_csi2(void);
extern BOOL                 ssenif_is_opened_csi2(void);
extern ER                   ssenif_close_csi2(void);
extern ER                   ssenif_start_csi2(void);
extern ER                   ssenif_stop_csi2(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi2(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi2(void);
extern void                 ssenif_set_config_csi2(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi2(DAL_SSENIFCSI_CFGID config_id);

extern ER                   ssenif_init_csi3(void);
extern ER                   ssenif_open_csi3(void);
extern BOOL                 ssenif_is_opened_csi3(void);
extern ER                   ssenif_close_csi3(void);
extern ER                   ssenif_start_csi3(void);
extern ER                   ssenif_stop_csi3(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi3(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi3(void);
extern void                 ssenif_set_config_csi3(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi3(DAL_SSENIFCSI_CFGID config_id);

extern ER                   ssenif_init_csi4(void);
extern ER                   ssenif_open_csi4(void);
extern BOOL                 ssenif_is_opened_csi4(void);
extern ER                   ssenif_close_csi4(void);
extern ER                   ssenif_start_csi4(void);
extern ER                   ssenif_stop_csi4(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi4(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi4(void);
extern void                 ssenif_set_config_csi4(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi4(DAL_SSENIFCSI_CFGID config_id);

extern ER                   ssenif_init_csi5(void);
extern ER                   ssenif_open_csi5(void);
extern BOOL                 ssenif_is_opened_csi5(void);
extern ER                   ssenif_close_csi5(void);
extern ER                   ssenif_start_csi5(void);
extern ER                   ssenif_stop_csi5(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi5(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi5(void);
extern void                 ssenif_set_config_csi5(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi5(DAL_SSENIFCSI_CFGID config_id);

extern ER                   ssenif_init_csi6(void);
extern ER                   ssenif_open_csi6(void);
extern BOOL                 ssenif_is_opened_csi6(void);
extern ER                   ssenif_close_csi6(void);
extern ER                   ssenif_start_csi6(void);
extern ER                   ssenif_stop_csi6(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi6(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi6(void);
extern void                 ssenif_set_config_csi6(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi6(DAL_SSENIFCSI_CFGID config_id);

extern ER                   ssenif_init_csi7(void);
extern ER                   ssenif_open_csi7(void);
extern BOOL                 ssenif_is_opened_csi7(void);
extern ER                   ssenif_close_csi7(void);
extern ER                   ssenif_start_csi7(void);
extern ER                   ssenif_stop_csi7(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi7(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi7(void);
extern void                 ssenif_set_config_csi7(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi7(DAL_SSENIFCSI_CFGID config_id);

extern ER                   ssenif_init_csi8(void);
extern ER                   ssenif_open_csi8(void);
extern BOOL                 ssenif_is_opened_csi8(void);
extern ER                   ssenif_close_csi8(void);
extern ER                   ssenif_start_csi8(void);
extern ER                   ssenif_stop_csi8(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi8(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_csi8(void);
extern void                 ssenif_set_config_csi8(DAL_SSENIFCSI_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_csi8(DAL_SSENIFCSI_CFGID config_id);



extern ER                   ssenif_init_lvds(void);
extern ER                   ssenif_open_lvds(void);
extern BOOL                 ssenif_is_opened_lvds(void);
extern ER                   ssenif_close_lvds(void);
extern ER                   ssenif_start_lvds(void);
extern ER                   ssenif_stop_lvds(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds(void);
extern void                 ssenif_set_config_lvds(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_lvds2(void);
extern ER                   ssenif_open_lvds2(void);
extern BOOL                 ssenif_is_opened_lvds2(void);
extern ER                   ssenif_close_lvds2(void);
extern ER                   ssenif_start_lvds2(void);
extern ER                   ssenif_stop_lvds2(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds2(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds2(void);
extern void                 ssenif_set_config_lvds2(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds2(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds2(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_lvds3(void);
extern ER                   ssenif_open_lvds3(void);
extern BOOL                 ssenif_is_opened_lvds3(void);
extern ER                   ssenif_close_lvds3(void);
extern ER                   ssenif_start_lvds3(void);
extern ER                   ssenif_stop_lvds3(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds3(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds3(void);
extern void                 ssenif_set_config_lvds3(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds3(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds3(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_lvds4(void);
extern ER                   ssenif_open_lvds4(void);
extern BOOL                 ssenif_is_opened_lvds4(void);
extern ER                   ssenif_close_lvds4(void);
extern ER                   ssenif_start_lvds4(void);
extern ER                   ssenif_stop_lvds4(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds4(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds4(void);
extern void                 ssenif_set_config_lvds4(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds4(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds4(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_lvds5(void);
extern ER                   ssenif_open_lvds5(void);
extern BOOL                 ssenif_is_opened_lvds5(void);
extern ER                   ssenif_close_lvds5(void);
extern ER                   ssenif_start_lvds5(void);
extern ER                   ssenif_stop_lvds5(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds5(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds5(void);
extern void                 ssenif_set_config_lvds5(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds5(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds5(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_lvds6(void);
extern ER                   ssenif_open_lvds6(void);
extern BOOL                 ssenif_is_opened_lvds6(void);
extern ER                   ssenif_close_lvds6(void);
extern ER                   ssenif_start_lvds6(void);
extern ER                   ssenif_stop_lvds6(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds6(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds6(void);
extern void                 ssenif_set_config_lvds6(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds6(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds6(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_lvds7(void);
extern ER                   ssenif_open_lvds7(void);
extern BOOL                 ssenif_is_opened_lvds7(void);
extern ER                   ssenif_close_lvds7(void);
extern ER                   ssenif_start_lvds7(void);
extern ER                   ssenif_stop_lvds7(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds7(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds7(void);
extern void                 ssenif_set_config_lvds7(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds7(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds7(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_lvds8(void);
extern ER                   ssenif_open_lvds8(void);
extern BOOL                 ssenif_is_opened_lvds8(void);
extern ER                   ssenif_close_lvds8(void);
extern ER                   ssenif_start_lvds8(void);
extern ER                   ssenif_stop_lvds8(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds8(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_lvds8(void);
extern void                 ssenif_set_config_lvds8(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_lvds8(DAL_SSENIFLVDS_CFGID config_id);
extern void                 ssenif_set_laneconfig_lvds8(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value);

extern ER                   ssenif_init_slvsec(void);
extern ER                   ssenif_open_slvsec(void);
extern BOOL                 ssenif_is_opened_slvsec(void);
extern ER                   ssenif_close_slvsec(void);
extern ER                   ssenif_start_slvsec(void);
extern ER                   ssenif_stop_slvsec(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_slvsec(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_slvsec(void);
extern void                 ssenif_set_config_slvsec(DAL_SSENIFSLVSEC_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_slvsec(DAL_SSENIFSLVSEC_CFGID config_id);

extern ER                   ssenif_init_vx1(void);
extern ER                   ssenif_open_vx1(void);
extern BOOL                 ssenif_is_opened_vx1(void);
extern ER                   ssenif_close_vx1(void);
extern ER                   ssenif_start_vx1(void);
extern ER                   ssenif_stop_vx1(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_vx1(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_vx1(void);
extern void                 ssenif_set_config_vx1(DAL_SSENIFVX1_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_vx1(DAL_SSENIFVX1_CFGID config_id);
extern DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_write_vx1(UINT32 reg_address, UINT32 reg_data);
extern DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_read_vx1(UINT32 reg_address, UINT32 *reg_data);
extern DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_sequential_write_vx1(UINT32 start_address, UINT32 total_data_length, UINT32 data_buffer_address);
extern void                 ssenif_set_gpio_vx1(DAL_SSENIFVX1_GPIO gpio_pin, BOOL level);
extern BOOL                 ssenif_get_gpio_vx1(DAL_SSENIFVX1_GPIO gpio_pin);

extern ER                   ssenif_init_vx12(void);
extern ER                   ssenif_open_vx12(void);
extern BOOL                 ssenif_is_opened_vx12(void);
extern ER                   ssenif_close_vx12(void);
extern ER                   ssenif_start_vx12(void);
extern ER                   ssenif_stop_vx12(void);
extern DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_vx12(DAL_SSENIF_INTERRUPT waited_flag);
extern void                 ssenif_dump_debug_information_vx12(void);
extern void                 ssenif_set_config_vx12(DAL_SSENIFVX1_CFGID config_id, UINT32 value);
extern UINT32               ssenif_get_config_vx12(DAL_SSENIFVX1_CFGID config_id);
extern DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_write_vx12(UINT32 reg_address, UINT32 reg_data);
extern DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_read_vx12(UINT32 reg_address, UINT32 *reg_data);
extern DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_sequential_write_vx12(UINT32 start_address, UINT32 total_data_length, UINT32 data_buffer_address);
extern void                 ssenif_set_gpio_vx12(DAL_SSENIFVX1_GPIO gpio_pin, BOOL level);
extern BOOL                 ssenif_get_gpio_vx12(DAL_SSENIFVX1_GPIO gpio_pin);
#endif

#ifndef _SEN_COMMON_H_
#define _SEN_COMMON_H_

#if defined(__FREERTOS)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#endif

#include "sen_dbg.h"
#include "sen_cfg.h"
#if defined(__FREERTOS)
#include <libfdt.h>
#endif

//=============================================================================
// struct & definition
//=============================================================================
#define I2C_VD_SYNC 0

#define SEN_PATH_1 0x01
#define SEN_PATH_2 0x02
#define SEN_PATH_3 0x04
#define SEN_PATH_4 0x08
#define SEN_PATH_5 0x10
#define SEN_PATH_6 0x20
#define SEN_PATH_7 0x40
#define SEN_PATH_8 0x80

#define SEN_CMD_DELAY       0xFFFFFFFF // delay
#define SEN_CMD_SETVD       0xFFFFFFFE // set vd period
#define SEN_CMD_PRESET      0xFFFFFFFD // preset expt. time and gain ratio
#define SEN_CMD_DIRECTION   0xFFFFFFFC // preset expt. time and gain ratio

typedef enum _SEN_I2C_ID {
	SEN_I2C_ID_1 = 0,
	SEN_I2C_ID_2 = 1,
	SEN_I2C_ID_3 = 2,
	SEN_I2C_ID_4 = 3,
	SEN_I2C_ID_5 = 4,
	ENUM_DUMMY4WORD(SEN_I2C_ID)
} SEN_I2C_ID;

typedef struct _SEN_PRESET {
	UINT32 expt_time;
	UINT32 gain_ratio;
} SEN_PRESET;

typedef struct _SEN_DIRECTION {
	BOOL mirror;
	BOOL flip;
} SEN_DIRECTION;

typedef struct _SEN_POWER {
	UINT32 mclk;
	UINT32 pwdn_pin;
	UINT32 rst_pin;
	UINT32 rst_time;
	UINT32 stable_time;
} SEN_POWER;

typedef struct _SEN_I2C {
	SEN_I2C_ID id;
	UINT32 addr;
} SEN_I2C;

//=============================================================================
// extern functions
//=============================================================================
extern UINT32 sen_common_calc_log_2(UINT32 devider, UINT32 devident);
extern CFG_FILE_FMT *sen_common_open_cfg(INT8 *pfile_name);
extern void sen_common_close_cfg(CFG_FILE_FMT *pcfg_file);

#if defined(__FREERTOS)
extern BOOL sen_common_check_compatible(CHAR *compatible);
extern void sen_common_load_cfg_preset_compatible(CHAR *compatible, void *param);
extern void sen_common_load_cfg_direction_compatible(CHAR *compatible, void *param);
extern void sen_common_load_cfg_power_compatible(CHAR *compatible, void *param);
extern void sen_common_load_cfg_i2c_compatible(CHAR *compatible, void *param);
extern void sen_common_load_cfg_map(UINT8 *pfdt_addr, CHAR *node_path, void *param);
extern void sen_common_load_cfg_preset(UINT8 *pfdt_addr, CHAR *node_path, void *param);
extern void sen_common_load_cfg_direction(UINT8 *pfdt_addr, CHAR *node_path, void *param);
extern void sen_common_load_cfg_power(UINT8 *pfdt_addr, CHAR *node_path, void *param);
extern void sen_common_load_cfg_i2c(UINT8 *pfdt_addr, CHAR *node_path, void *param);
#else
extern void sen_common_load_cfg_preset_compatible(struct device_node *node, void *param);
extern void sen_common_load_cfg_direction_compatible(struct device_node *node, void *param);
extern void sen_common_load_cfg_power_compatible(struct device_node *node, void *param);
extern void sen_common_load_cfg_i2c_compatible(struct device_node *node, void *param);
extern void sen_common_load_cfg_map(CFG_FILE_FMT *pcfg_file, void *param);
extern void sen_common_load_cfg_preset(CFG_FILE_FMT *pcfg_file, void *param);
extern void sen_common_load_cfg_direction(CFG_FILE_FMT *pcfg_file, void *param);
extern void sen_common_load_cfg_power(CFG_FILE_FMT *pcfg_file, void *param);
extern void sen_common_load_cfg_i2c(CFG_FILE_FMT *pcfg_file, void *param);
#endif
#endif


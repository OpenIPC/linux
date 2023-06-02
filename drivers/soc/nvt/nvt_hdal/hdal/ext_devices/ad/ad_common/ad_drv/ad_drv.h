#ifndef _AD_DRV_H_
#define _AD_DRV_H_

#include "ad.h"
#include "ad_drv_util.h"

#define AD_DRV_VERSION "0.01.019"

#define AD_DRV_CHIP_MAX 8
#define AD_DRV_I2C_ADDR_LEN_MAX 2
#define AD_DRV_I2C_DATA_LEN_MAX 4

/*******************************************************************************
* Register definition                                                          *
*******************************************************************************/
#define AD_DRV_REGDEF_BEGIN(name)	\
typedef union						\
{									\
	UINT32 reg;						\
	struct							\
	{

#define AD_DRV_REGDEF_BIT(field, bits)	\
	UBITFIELD field : bits;

#define AD_DRV_REGDEF_END(name)	\
 	} bit;						\
} name;

/*******************************************************************************
* Built-in debug function                                                      *
*******************************************************************************/
typedef enum {
	// debug function that all decoder could have
	AD_DRV_DBG_FUNC_DUMP_INFO 		= (1 << 0),
	AD_DRV_DBG_FUNC_I2C_WRITE 		= (1 << 1),
	AD_DRV_DBG_FUNC_I2C_READ 		= (1 << 2),
	AD_DRV_DBG_FUNC_HELP 			= (1 << 3),
	AD_DRV_DBG_FUNC_DBGLV 			= (1 << 4),

	// enable all common debug function
	AD_DRV_DBG_FUNC_COMMON			= (1 << 16) - 1,

	// choose these functions depend on which decoder
	AD_DRV_DBG_FUNC_DUMP_BANK_REG 	= (1 << 16),	// AD_DRV_DBG_BANK_REG_DUMP_INFO
	AD_DRV_DBG_FUNC_DUMP_REG 		= (1 << 17),	// AD_DRV_DBG_REG_DUMP_INFO
} AD_DRV_DBG_FUNC;

/*******************************************************************************
* bank_addr: register which used to switch bank number                         *
* bank_num: total banks to be dump                                             *
* bank:                                                                        *
*   id: bank number which will be wrote to bank_addr                           *
*   len: number of registers to be read in this bank                           *
* dump_per_row: number of register to be dump in a row                         *
*******************************************************************************/
typedef struct {
	UINT32 id;
	UINT32 len;
} AD_DRV_DBG_REG_BANK;

typedef struct {
	UINT32 bank_addr;
	UINT32 bank_num;
	AD_DRV_DBG_REG_BANK *bank;
	UINT32 dump_per_row;
} AD_DRV_DBG_BANK_REG_DUMP_INFO;

/*******************************************************************************
* tbl_num: total tables to be dump                                             *
* tbl:                                                                         *
*   addr: start address of this table                                          *
*   len: number of registers to be read from start address                     *
* dump_per_row: number of register to be dump in a row                         *
*******************************************************************************/
typedef struct {
	UINT32 addr;
	UINT32 len;
} AD_DRV_DBG_REG_DUMP_TBL;

typedef struct {
	UINT32 tbl_num;
	AD_DRV_DBG_REG_DUMP_TBL *tbl;
	UINT32 dump_per_row;
} AD_DRV_DBG_REG_DUMP_INFO;

/*******************************************************************************
* module_name: decoder module name                                             *
* version: decoder driver version                                              *
* comm_info: ad module info                                                    *
* i2c_addr_len: i2c address length                                             *
* i2c_data_len: i2c data length                                                *
* i2c_data_msb: i2c data order LSB(0) or MSB(1)                                *
* i2c_retry_num: retry count when i2c r/w fail                                 *
* pwr_ctl: decoder power control callback (Linux & FreeRTOS only)              *
* ext_func: extern function (Type: AD_DRV_EXTFUNC)                             *
* dbg_bank_reg_dump_info: debug dump bank register info                        *
*******************************************************************************/
typedef struct {
	char *module_name;
	char *version;
	AD_INFO *comm_info;
	UINT32 i2c_addr_len;
	UINT32 i2c_data_len;
	UINT32 i2c_data_msb;
	UINT32 i2c_retry_num;
	AD_PWR_CTL pwr_ctl;
	UINT32 dbg_func;	// AD_DRV_DBG_FUNC
	AD_DRV_DBG_BANK_REG_DUMP_INFO *dbg_bank_reg_dump_info;
	AD_DRV_DBG_REG_DUMP_INFO *dbg_reg_dump_info;
} AD_DRV_OPEN_INFO;


#if defined (_BSP_NA51000_) || defined (_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(__UITRON) || defined(__ECOS)
#define ad_drv_open 				AD_DRV_MODEL_BIND(ad_drv_open)
#define ad_drv_close 				AD_DRV_MODEL_BIND(ad_drv_close)
#define ad_drv_i2c_set_i2c_rw_if 	AD_DRV_MODEL_BIND(ad_drv_i2c_set_i2c_rw_if)
#define ad_drv_i2c_write 			AD_DRV_MODEL_BIND(ad_drv_i2c_write)
#define ad_drv_i2c_read 			AD_DRV_MODEL_BIND(ad_drv_i2c_read)
#define ad_drv_pwr_ctrl 			AD_DRV_MODEL_BIND(ad_drv_pwr_ctrl)
#define ad_drv_dbg_parse 			AD_DRV_MODEL_BIND(ad_drv_dbg_parse)
#define ad_drv_dbg 					AD_DRV_MODEL_BIND(ad_drv_dbg)

#if !defined(__UITRON) && !defined(__ECOS)
ER ad_drv_pwr_ctrl(UINT32 chip_id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
#endif
#endif



ER ad_drv_open(UINT32 chip_id, AD_DRV_OPEN_INFO *drv_info);
ER ad_drv_close(UINT32 chip_id);
ER ad_drv_i2c_set_i2c_rw_if(ad_i2c_rw_if i2c_rw_if);
ER ad_drv_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
ER ad_drv_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
UINT32 ad_drv_dbg_parse(char *str_cmd, char **cmd_list, UINT32 cmd_num_max);
ER ad_drv_dbg(char *str_cmd);

#endif //_AD_DRV_H_


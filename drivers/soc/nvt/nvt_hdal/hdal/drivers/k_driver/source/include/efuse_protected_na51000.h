/*
    Novatek protected header file of NT96660's driver.

    The header file for Novatek protected APIs of NT96660's driver.

    @file       efuse_protected.h
    @ingroup    mIDriver
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _NVT_EFUSE_PROTECTED_H
#define _NVT_EFUSE_PROTECTED_H

#include "Type.h"

#define DRV_PKG_IDBIT_MAX   32                      //Total 32 sets package type

typedef enum {
	EFUSE_AUTO_MODE = 0x0,
	EFUSE_MANUAL_MODE,
} EFUSE_MODE;

typedef enum {
	EFUSE_READ = 0x0,                               //A_READ
	EFUSE_MARGIN_READ,                              //Margin_A_READ
	EFUSE_PROGRAM,
} EFUSE_OPERATION_MODE;

typedef enum {
	EFUSE_REPAIR_DISABLE = 0x0,                     //Repair disable
	EFUSE_REPAIR_ENABLE,                            //Repair enable

	ENUM_DUMMY4WORD(EFUSE_REPAIR_EN)
} EFUSE_REPAIR_EN;

typedef enum {
	EFUSE_REPAIR_BANK0 = 0x0,                       //Repair bank0
	EFUSE_REPAIR_BANK1,                             //Repair bank1
	EFUSE_REPAIR_BANK2,                             //Repair bank2
	EFUSE_REPAIR_BANK3,                             //Repair bank3

	EFUSE_REPAIR_BANK_CNT,

	ENUM_DUMMY4WORD(EFUSE_REPAIR_BANK)
} EFUSE_REPAIR_BANK;

STATIC_ASSERT(EFUSE_REPAIR_BANK_CNT <= 4);

typedef enum {
	EFUSE_ARRAY_FUNCTION_SEL = 0x0,                 //Arrary function selected
	EFUSE_REDUNDANCY_FUNCTION_SEL,                  //Redundancy function selected

	ENUM_DUMMY4WORD(EFUSE_ARRAY_REDUNDANCY_SEL)

} EFUSE_ARRAY_REDUNDANCY_SEL;

typedef enum {
	EFUSE_INFO_DEGREE_0 = 0x0,                      // Only display efuse 512 bits
	EFUSE_INFO_DEGREE_1,                            // Display efuse 512 bits and translate field defint
	EFUSE_INFO_DEGREE_CNT,

	ENUM_DUMMY4WORD(EFUSE_INFO_DEGREE)
} EFUSE_INFO_DEGREE;

typedef enum {
	EFUSE_KEY_MANAGER_NORMAL = 0x0,                 // Normal operation
	EFUSE_KEY_MANAGER_CRYPTO,                       // to destination crypto engine
	EFUSE_KEY_MANAGER_RSA,                          // to destination RSA engine
	EFUSE_KEY_MANAGER_HASH,                         // to destination HASH engine

	EFUSE_KEY_MANAGER_DST_CNT,
	ENUM_DUMMY4WORD(EFUSE_KEY_MANAGER_DST)
} EFUSE_KEY_MANAGER_DST;

typedef enum {
#if (_EMULATION_ == ENABLE)
	NT96680_PKG = 0x00,                             // NT96680 package version
	NT96681_PKG,                                    // NT96681 package version
	NT96687_PKG,                                    // NT96687 package version
	NT96683_PKG,                             		// NT96683 package version
	NT96685_PKG,                                    // NT96685 package version
	NT96688_PKG,                                    // NT96688 package version

    NT98680_PKG,                                    // NT98680 package version
	NT98683_PKG,                             		// NT98683 package version
	NT98685_PKG,                                    // NT98685 package version
	NT98687_PKG,                                    // NT98687 package version
#else
	NT96680_PKG = 0x7C3E,                           // NT96680 package version
	NT96681_PKG = 0x7C5D,                           // NT96681 package version
	NT96687_PKG = 0x7C7C,                           // NT96687 package version
  	NT96683_PKG = 0x7C9B,                           // NT96683 package version
  	NT96685_PKG = 0x7CBA,                        	// NT96685 package version
  	NT96688_PKG = 0x7CD9,                        	// NT96688 package version

    NT98680_PKG = 0x7E0F,                           // NT98680 package version
	NT98683_PKG = 0x7E2E,                      		// NT98683 package version
	NT98685_PKG = 0x7E4D,                           // NT98685 package version
	NT98687_PKG = 0x7E6C,                           // NT98687 package version
#endif
	NT9668X_PKG_CNT = 10,
	NT9668X_ENG_VER,                                // NT9668X Engineer version
	UNKNOWN_PKG_VER,                                // Unknow package version

	ENUM_DUMMY4WORD(NT9668X_PKG_UID)

} NT9668X_PKG_UID;

STATIC_ASSERT(NT9668X_PKG_CNT <= DRV_PKG_IDBIT_MAX);

typedef enum {
	EFUSE_MASK_AREA_TYPE = 0x0,
	EFUSE_READ_ONLY_AREA_TYPE,

	ENUM_DUMMY4WORD(EFUSE_USER_AREA_TYPE)

} EFUSE_USER_AREA_TYPE;

typedef enum {
	EFUSE_ETHERNET_TRIM_DATA = 0x100,
	EFUSE_USBC_TRIM_DATA,
	EFUSE_DDRP_V_TRIM_DATA,
	EFUSE_DDRP_H_TRIM_DATA,
	EFUSE_DDRP_ZQ_TRIM_DATA,
	EFUSE_THERMAL_TRIM_DATA,
	EFUSE_IDDQ_TRIM_DATA,

	EFUSE_CYPHER_KEY_DATA_0,
	EFUSE_CYPHER_KEY_DATA_1,
	EFUSE_CYPHER_KEY_DATA_2,
	EFUSE_CYPHER_KEY_DATA_3,
	EFUSE_CYPHER_KEY_DATA_4,
	EFUSE_CYPHER_KEY_DATA_5,
	EFUSE_CYPHER_KEY_DATA_6,
	EFUSE_CYPHER_KEY_DATA_7,

	EFUSE_VER_PKG_UID,
//  EFUSE_VER_DIE_UID,
    EFUSE_VER_ENGINEER_SIM_PKG_UID,
	EFUSE_VER_EXTEND_UID,
	EFUSE_PARAM_CNT,
	ENUM_DUMMY4WORD(EFUSE_PARAM_DATA)
} EFUSE_PARAM_DATA;

typedef enum {
	EFUSE_WRITE_NO_0_KEY_SET_FIELD = 0x0,
	EFUSE_WRITE_NO_1_KEY_SET_FIELD,
	EFUSE_WRITE_NO_2_KEY_SET_FIELD,
	EFUSE_WRITE_NO_3_KEY_SET_FIELD,
	EFUSE_WRITE_NO_4_KEY_SET_FIELD,
	EFUSE_WRITE_NO_5_KEY_SET_FIELD,
	EFUSE_WRITE_NO_6_KEY_SET_FIELD,
	EFUSE_WRITE_NO_7_KEY_SET_FIELD,


	EFUSE_WRITE_NO_8_KEY_SET_FIELD,
	EFUSE_WRITE_NO_9_KEY_SET_FIELD,
	EFUSE_WRITE_NO_10_KEY_SET_FIELD,
	EFUSE_WRITE_NO_11_KEY_SET_FIELD,

	EFUSE_WRITE_NO_12_KEY_SET_FIELD,
	EFUSE_WRITE_NO_13_KEY_SET_FIELD,
	EFUSE_WRITE_NO_14_KEY_SET_FIELD,
	EFUSE_WRITE_NO_15_KEY_SET_FIELD,

	EFUSE_TOTAL_KEY_SET_FIELD,

	EFUSE_CRYPTO_ENGINE_KEY_CNT = EFUSE_WRITE_NO_8_KEY_SET_FIELD,
	EFUSE_RSA_ENGINE_KEY_CNT = EFUSE_TOTAL_KEY_SET_FIELD,
	EFUSE_HASH_ENGINE_KEY_CNT = EFUSE_TOTAL_KEY_SET_FIELD,

	ENUM_DUMMY4WORD(EFUSE_WRITE_KEY_SET_TO_OTP_FIELD)
} EFUSE_WRITE_KEY_SET_TO_OTP_FIELD;


typedef enum {
	EFUSE_WRITE_1ST_KEY_SET_FIELD = EFUSE_WRITE_NO_0_KEY_SET_FIELD,
	EFUSE_WRITE_2ND_KEY_SET_FIELD = EFUSE_WRITE_NO_1_KEY_SET_FIELD,
	EFUSE_WRITE_3RD_KEY_SET_FIELD = EFUSE_WRITE_NO_2_KEY_SET_FIELD,
	EFUSE_WRITE_4TH_KEY_SET_FIELD = EFUSE_WRITE_NO_3_KEY_SET_FIELD,
	EFUSE_TOTAL_KEY_SET,

	ENUM_DUMMY4WORD(EFUSE_WRITE_KEY_SET)
} EFUSE_WRITE_KEY_SET;

STATIC_ASSERT(EFUSE_TOTAL_KEY_SET_FIELD <= 16);

STATIC_ASSERT(EFUSE_RSA_ENGINE_KEY_CNT == 16);
STATIC_ASSERT(EFUSE_HASH_ENGINE_KEY_CNT == 16);
STATIC_ASSERT(EFUSE_CRYPTO_ENGINE_KEY_CNT == 8);


// Capacity of Storage device
typedef struct {
	UINT32  uiWidth;                    // width
	UINT32  uiHeight;                   // height
} CDC_ABILITY, *PCDC_ABILITY;

/**
    @name eFuse Package extended ability

\n  For specific ability chech
*/
//@{
typedef enum {
	EFUSE_ABILITY_CDC_RESOLUTION    =0x1001,        //< Check package resoltion of codec
	///< Context are :
	///< - @b UINT32 * : starting address of CDC_ABILITY

	ENUM_DUMMY4WORD(EFUSE_PKG_ABILITY_LIST)
} EFUSE_PKG_ABILITY_LIST;
//@}




#define EFUSE_SUCCESS                E_OK
#define EFUSE_FREEZE_ERR             -1001          // Programmed already, only can read
#define EFUSE_INACTIVE_ERR           -1002          // This field is empty(not programmed yet)
#define EFUSE_INVALIDATE_ERR         -1003          // This field force invalidate already
#define EFUSE_UNKNOW_PARAM_ERR       -1004          // efuse param field not defined
#define EFUSE_OPS_ERR                -1005          // efuse operation error
#define EFUSE_SECURITY_ERR           -1006          // efuse under security mode => can not read back


// System parameter usage
#define EFUSE_SUCCESS_PRI            EFUSE_SUCCESS  // Operation success from primary field
#define EFUSE_SUCCESS_SEC            1              // Operation success from secondary field

typedef struct {
	UINT32  uiTPDGroup;
	UINT32  uiStrobeWriteGroup;
	UINT32  uiStrobeReadGroup;
	UINT32  uiNormalTimingGroup;

} EFUSE_TIMING_GROUP;

typedef struct {
	UINT32  uiMaskAreaData0;
	UINT32  uiMaskAreaData1;
	UINT32  uiMaskAreaData2;
	UINT32  uiMaskAreaData3;

} EFUSE_MASK_AREA;

typedef struct {
	UINT32  uiReadOnlyAreaData0;
	UINT32  uiReadOnlyAreaData1;
	UINT32  uiReadOnlyAreaData2;
	UINT32  uiReadOnlyAreaData3;

} EFUSE_READ_ONLY_AREA;

extern ER       efuse_close(void);
extern INT32    efuse_read(UINT32 rowAddress);
extern INT32    efuse_readData(UINT32 rowAddress);
extern INT32    efuse_manual_read(UINT32 rowAddress);
extern INT32    efuse_manual_read_data(UINT32 rowAddress);
//extern void   efuse_clear_latch_data_field(void);
extern void     efuse_config(EFUSE_TIMING_GROUP *timCfg);
extern INT32    efuse_redundantReadData(UINT32 rowAddress);
extern void     efuse_dumpWholeBits(EFUSE_INFO_DEGREE degree);
extern INT32    efuse_writeData(UINT32 rowAddress, UINT32 data);
extern ER       efuse_program(UINT32 rowAddress, UINT32 colAddress);
extern INT32    efuse_config_key_set(EFUSE_WRITE_KEY_SET_TO_OTP_FIELD key_set_index);
extern INT32    efuse_write_key(EFUSE_WRITE_KEY_SET_TO_OTP_FIELD key_set_index, UINT8 *ucKey);
extern ER       efuse_repair(EFUSE_REPAIR_BANK uiBank, UINT32 uiFixedData, UINT32 uiRowAddr, UINT32 uiColumnAddr, BOOL bEnable);
extern ER       efuse_open(EFUSE_MODE mode, EFUSE_OPERATION_MODE opMode, EFUSE_REPAIR_EN repair, EFUSE_ARRAY_REDUNDANCY_SEL arraySel);
extern ER       efuse_setConfig(EFUSE_MODE mode, EFUSE_OPERATION_MODE opMode, EFUSE_REPAIR_EN repair, EFUSE_ARRAY_REDUNDANCY_SEL arraySel);



/*
     efuse_readParamOps

     efuse get system parameter (trim data)

     @note for EFUSE_PARAM_DATA

     @param[in]     param   efuse system internal data field
     @param[out]     data   trim data(if success)

     @return
        - @b EFUSE_SUCCESS              success
        - @b EFUSE_UNKNOW_PARAM_ERR     unknow system internal data field
        - @b EFUSE_INVALIDATE_ERR       system internal data field invalidate
        - @b EFUSE_OPS_ERR              efuse operation error
*/
extern INT32    efuse_readParamOps(EFUSE_PARAM_DATA param, UINT16 *data);

/*
     efuse_get_package_version

     efuse get IC package version

     @return IC revision of specific package revision
        - @b   NT96660_PKG      NT96660 package version
        - @b   NT96663_PKG      NT96663 package version
        - @b   NT96665_PKG      NT96665 package version
        - @b   NT9666X_ENG_VER  Engineer sample(This IC only for engineer sample)
        - @b   UNKNOWN_PKG_VER  Unknown IC package version(system must halt)
*/
extern UINT32 efuse_getPkgVersion(void);



/*
     efuse_setPkgVersion

     efuse set IC package version

     @note only support under emulation enable

     @return IC revision of specific package revision
        - @b    EFUSE_SUCCESS           Operation success
        - @b    E_NOSPT                 Not support
        - @b    EFUSE_FREEZE_ERR        Programmed already, only can read
        - @b    EFUSE_INVALIDATE_ERR    This field invalidate already
        - @b    EFUSE_UNKNOW_PARAM_ERR  efuse param field not defined
        - @b    EFUSE_OPS_ERR           efuse operation error

*/
extern INT32  efuse_setPkgVersion(NT9668X_PKG_UID uPKG);

/*
     efuse_check_available

     efuse check specific library can use @ specific package ID or not

     @param[in]   name   library name

     @return IC revision of specific package revision
        - @b   TRUE     Usable library
        - @b   FALSE    Un-usable library
*/
extern BOOL efuse_check_available(const CHAR * name);

extern UINT32 efuse_getDieVersion(void);
extern ER     efuse_setKeyDestination(EFUSE_KEY_MANAGER_DST key_dst, EFUSE_WRITE_KEY_SET_TO_OTP_FIELD key_field_set);

#if 0
extern INT32  efuse_setCypherKey(UINT8 *key, UINT32 keyLen);
#endif
extern INT32  efuse_getCypherKey(UINT8 *key, UINT32 keyLen);

extern INT16 efuse_getFirstPackageUID(void);
extern INT16 efuse_setFirstPackageUID(UINT16 data);
#endif


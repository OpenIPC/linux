/*
	Novatek protected header file of NA51055 driver.

	The header file for Novatek protected APIs of NT96660's driver.

	@file       efuse_protected.h
	@ingroup    mIDriver
	@note       For Novatek internal reference, don't export to agent or customer.

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _NVT_EFUSE_PROTECTED_H
#define _NVT_EFUSE_PROTECTED_H

#include <kwrap/error_no.h>
#include <kwrap/nvt_type.h>
#if defined(__FREERTOS)
#include "plat/pll.h"
#endif

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
 #if defined(_NVT_EMULATION_)
	NT98560_PKG = 0x00,                             // NT98560   package version
	NT98562_PKG,                                    // NT98562   package version
	NT96563_PKG,                                    // NT98563   package version
	NT96565_PKG,                                    // NT96565   package version
	NT98566_PKG,									// NT98566   package version
	NT98565_PKG,
	NT96562_PKG,									// NT96562   package version
	NT96566_PKG,									// NT96566   package version
	NTDump_eFuse,
	NT9856X_PKG_CNT = 9,
#else
	NT98560_PKG = 0x7C3E,                           // NT98560   package version
	NT98562_PKG = 0x7C5D,                           // NT98562   package version
	NT96563_PKG = 0x6C5D,                           // NT98563   package version
	NT96565_PKG = 0x687C,                           // NT96565   package version
  	NT98566_PKG = 0x787C,                           // NT98566   package version
  	NT98565_PKG = 0x487C, 							// NT98565   package version
  	NT96562_PKG = 0x5C5D,							// NT96562   package version
  	NT96566_PKG = 0x587C,							// NT96566   package version
	NT9856X_PKG_CNT = 8,
#endif
	NT9856X_ENG_VER,                                // NT9856X Engineer version
	UNKNOWN_PKG_VER,                                // Unknow package version
	IC_PKG_CNT 		= NT9856X_PKG_CNT,
	ENUM_DUMMY4WORD(NVT_PKG_UID)

} NVT_PKG_UID;


STATIC_ASSERT(IC_PKG_CNT <= DRV_PKG_IDBIT_MAX);

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
	EFUSE_DDRP_LDO_TRIM_DATA,
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

typedef enum {
	EFUSE_OTP_1ST_KEY_SET_FIELD = EFUSE_WRITE_NO_0_KEY_SET_FIELD,        // This if for secure boot
	EFUSE_OTP_2ND_KEY_SET_FIELD,
	EFUSE_OTP_3RD_KEY_SET_FIELD,
	EFUSE_OTP_4TH_KEY_SET_FIELD,
	EFUSE_OTP_5TH_KEY_SET_FIELD,
	EFUSE_OTP_TOTAL_KEY_SET_FIELD,
} EFUSE_OTP_KEY_SET_FIELD;

STATIC_ASSERT(EFUSE_OTP_TOTAL_KEY_SET_FIELD <= 5);
#define EFUSE_OTP_KEY_FIELD_CNT			4


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
	EFUSE_ABILITY_CDC_PLL_FREQ		=0x1002,		//< Check package PLL freq of codec
	///< Context are :
	///< - @b UINT32 * : starting address of CDC_ABILITY

	ENUM_DUMMY4WORD(EFUSE_PKG_ABILITY_LIST)
} EFUSE_PKG_ABILITY_LIST;
//@}

/**
    Crypto engine check
*/
typedef enum {
	SECUREBOOT_SECURE_EN = 0x00,       	///< Quary if secure enable or not
	SECUREBOOT_DATA_AREA_ENCRYPT,       ///< Quary if data area encrypt to cypher text or not
	SECUREBOOT_SIGN_RSA,				///< Quary if Signature methed is RSA or not(AES)
	SECUREBOOT_SIGN_RSA_CHK,			///< Quary if Signature hash checksum RSA key correct or not
	SECUREBOOT_JTAG_DISABLE_EN,			///< Quary if JTAG is disable or not(TRUE : disable)

	SECUREBOOT_1ST_KEY_SET_PROGRAMMED,	///< Quary if 1st key set programmed or not
	SECUREBOOT_2ND_KEY_SET_PROGRAMMED,	///< Quary if 2nd key set programmed or not
	SECUREBOOT_3RD_KEY_SET_PROGRAMMED,	///< Quary if 3rd key set programmed or not
	SECUREBOOT_4TH_KEY_SET_PROGRAMMED,	///< Quary if 4th key set programmed or not
	SECUREBOOT_5TH_KEY_SET_PROGRAMMED,	///< Quary if 5th key set programmed or not

	SECUREBOOT_1ST_KEY_SET_READ_LOCK,	///< Quary if 1st key read lock already or not
	SECUREBOOT_2ND_KEY_SET_READ_LOCK,	///< Quary if 2nd key read lock already or not
	SECUREBOOT_3RD_KEY_SET_READ_LOCK,	///< Quary if 3rd key read lock already or not
	SECUREBOOT_4TH_KEY_SET_READ_LOCK,	///< Quary if 4th key read lock already or not
	SECUREBOOT_5TH_KEY_SET_READ_LOCK,	///< Quary if 5th key read lock already or not

	SECUREBOOT_STATUS_NUM,

} SECUREBOOT_STATUS;

#define EFUSE_SUCCESS                	E_OK
#define EFUSE_FREEZE_ERR             	-1001          // Programmed already, only can read
#define EFUSE_INACTIVE_ERR           	-1002          // This field is empty(not programmed yet)
#define EFUSE_INVALIDATE_ERR         	-1003          // This field force invalidate already
#define EFUSE_UNKNOW_PARAM_ERR       	-1004          // efuse param field not defined
#define EFUSE_OPS_ERR                	-1005          // efuse operation error
#define EFUSE_SECURITY_ERR           	-1006          // efuse under security mode => can not read back
#define EFUSE_PARAM_ERR              	-1007          // efuse param error
#define EFUSE_CONTENT_ERR            	-1008          // efuse operation error
#define OTP_HW_SECURE_EN				(1 << 0)
#define OTP_FW_SECURE_EN				(1 << 5)
#define OTP_DATA_ENCRYPT_EN				(1 << 7)
#define OTP_SIGNATURE_RSA				(1 << 1)
#define OTP_SIGNATURE_RSA_CHK_EN		(1 << 3)
#define OTP_JTAG_DISABLE_EN				(1 << 2)



#define OTP_1ST_KEY_PROGRAMMED_BIT		27
#define OTP_2ND_KEY_PROGRAMMED_BIT		28
#define OTP_3RD_KEY_PROGRAMMED_BIT		29
#define OTP_4TH_KEY_PROGRAMMED_BIT		30
#define OTP_5TH_KEY_PROGRAMMED_BIT		31

#define OTP_1ST_KEY_PROGRAMMED			(1 << OTP_1ST_KEY_PROGRAMMED_BIT)
#define OTP_2ND_KEY_PROGRAMMED			(1 << OTP_2ND_KEY_PROGRAMMED_BIT)
#define OTP_3RD_KEY_PROGRAMMED			(1 << OTP_3RD_KEY_PROGRAMMED_BIT)
#define OTP_4TH_KEY_PROGRAMMED			(1 << OTP_4TH_KEY_PROGRAMMED_BIT)
#define OTP_5TH_KEY_PROGRAMMED			(1 << OTP_5TH_KEY_PROGRAMMED_BIT)

#define OTP_1ST_KEY_READ_LOCK_BIT		22
#define OTP_2ND_KEY_READ_LOCK_BIT		23
#define OTP_3RD_KEY_READ_LOCK_BIT		24
#define OTP_4TH_KEY_READ_LOCK_BIT		25
#define OTP_5TH_KEY_READ_LOCK_BIT		26

#define OTP_1ST_KEY_READ_LOCK			(1 << OTP_1ST_KEY_READ_LOCK_BIT)
#define OTP_2ND_KEY_READ_LOCK			(1 << OTP_2ND_KEY_READ_LOCK_BIT)
#define OTP_3RD_KEY_READ_LOCK			(1 << OTP_3RD_KEY_READ_LOCK_BIT)
#define OTP_4TH_KEY_READ_LOCK			(1 << OTP_4TH_KEY_READ_LOCK_BIT)
#define OTP_5TH_KEY_READ_LOCK			(1 << OTP_5TH_KEY_READ_LOCK_BIT)


#define OTP_1ST_KEY_BIT_START			(0xF << 16)
#define OTP_2ND_KEY_BIT_START			(0xF << 20)
#define OTP_3RD_KEY_BIT_START			(0xF << 24)
#define OTP_4TH_KEY_BIT_START			(0xF << 28)
#define OTP_5TH_KEY_BIT_START			(0xF << 12)


// System parameter usage
#define EFUSE_SUCCESS_PRI            	EFUSE_SUCCESS  // Operation success from primary field
#define EFUSE_SUCCESS_SEC            	1              // Operation success from secondary field

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
extern INT32  efuse_setPkgVersion(NVT_PKG_UID uPKG);

/*
     efuse_check_available

     efuse check specific library can use @ specific package ID or not

     @param[in]   name   library name

     @return IC revision of specific package revision
        - @b   TRUE     Usable library
        - @b   FALSE    Un-usable library
*/
extern BOOL efuse_check_available(const CHAR * name);

/*
     efuse_check_available_extend

     efuse check extend specific functionality

     @param[in]   param1   extend ability list
     @param[in]   param2   specific param of ability

     @return IC revision of specific package revision
        - @b   TRUE     support
        - @b   FALSE    not support
*/
extern BOOL efuse_check_available_extend(EFUSE_PKG_ABILITY_LIST param1, UINT32 param2);

/*
     efuse_get_unique_id

     efuse get unique id (56bits)

     @param[out]     id_L   LSB of ID (32bit)
     @param[out]     id_H   MSB of ID (17bit)

     @return IC success or fail
        - @b   	EFUSE_SUCCESS      	success
        - @b	EFUSE_OPS_ERR		fail
*/
extern ER       efuse_get_unique_id(UINT32 * id_L, UINT32 * id_H);
extern UINT32 	efuse_is_secure_en(void);
extern UINT32 	efuse_getDieVersion(void);

/**
	efuse_setKeyDestination

	Set Destination of efuse get key field value

	@note for EFUSE_KEY_MANAGER_DST EFUSE_WRITE_KEY_SET_TO_OTP_FIELD

	@param[in] key_dst			Destination engine (Crypto/RSA/HASH)
	@param[in] key_field_set	No. of key field (0~15)

	Example: (Optional)
	@code
	{
		// Destination to 1st field of crypto engine
		efuse_setKeyDestination(EFUSE_KEY_MANAGER_CRYPTO, 0);
	}
	@endcode

	@return Success or fail
	- @b   E_OK    	Success
	- @b   E_SYS  	Parameter error
*/
extern ER     	efuse_setKeyDestination(EFUSE_KEY_MANAGER_DST key_dst, EFUSE_WRITE_KEY_SET_TO_OTP_FIELD key_field_set);

/**
    efuse_otp_set_key

    Durung encrypt or decrypt, configure specific key set as AES key(0~3)

    @Note: key set 0 is for secure boot use

    @param[in] key_set_index   key set (0~3)
    @return Description of data returned.
        - @b E_OK:   Success
*/
extern INT32 	efuse_otp_set_key(EFUSE_OTP_KEY_SET_FIELD key_set_index);
#if 0
extern INT32  efuse_setCypherKey(UINT8 *key, UINT32 keyLen);
#endif
extern INT32  	efuse_getCypherKey(UINT8 *key, UINT32 keyLen);

extern INT16 	efuse_getFirstPackageUID(void);
extern INT16 	efuse_setFirstPackageUID(UINT16 data);
/**
    otp_secure_en

    Enable secure boot (Rom treat loader as secure boot flow)
*/
#define otp_secure_en()					enable_secure_boot(SECUREBOOT_SECURE_EN)

/**
    otp_data_area_encrypt_en

    ROM treat loader data area as cypher text enable
*/
#define otp_data_area_encrypt_en()		enable_secure_boot(SECUREBOOT_DATA_AREA_ENCRYPT)

/**
    otp_signature_rsa_en

    ROM treat loader by use RSA as loader's signature
*/
#define otp_signature_rsa_en()			enable_secure_boot(SECUREBOOT_SIGN_RSA)

/**
    otp_signature_rsa_chksum_en

    Once use RSA as signature, enable RSA public checksum(use SHA256) enable
*/

#define otp_signature_rsa_chksum_en()	enable_secure_boot(SECUREBOOT_SIGN_RSA_CHK)

/**
    otp_jtag_dis

    Disable JTAG

    @Note: Can not re enable once disabled
*/
#define otp_jtag_dis()					enable_secure_boot(SECUREBOOT_JTAG_DISABLE_EN)

//Query API collection
#define is_secure_enable()				quary_secure_boot(SECUREBOOT_SECURE_EN)
#define is_data_area_encrypted()		quary_secure_boot(SECUREBOOT_DATA_AREA_ENCRYPT)
#define is_signature_rsa()				quary_secure_boot(SECUREBOOT_SIGN_RSA)
#define is_signature_rsa_chsum_enable()	quary_secure_boot(SECUREBOOT_SIGN_RSA_CHK)
#define is_signature_aes()				!quary_secure_boot(SECUREBOOT_SIGN_RSA)
#define is_JTAG_DISABLE_en()			quary_secure_boot(SECUREBOOT_JTAG_DISABLE_EN)

#define is_1st_key_programmed()			quary_secure_boot(SECUREBOOT_1ST_KEY_SET_PROGRAMMED)
#define is_2nd_key_programmed()			quary_secure_boot(SECUREBOOT_2ND_KEY_SET_PROGRAMMED)
#define is_3rd_key_programmed()			quary_secure_boot(SECUREBOOT_3RD_KEY_SET_PROGRAMMED)
#define is_4th_key_programmed()			quary_secure_boot(SECUREBOOT_4TH_KEY_SET_PROGRAMMED)
#define is_5th_key_programmed()			quary_secure_boot(SECUREBOOT_5TH_KEY_SET_PROGRAMMED)

#define is_1st_key_read_lock()			quary_secure_boot(SECUREBOOT_1ST_KEY_SET_READ_LOCK)
#define is_2nd_key_read_lock()			quary_secure_boot(SECUREBOOT_2ND_KEY_SET_READ_LOCK)
#define is_3rd_key_read_lock()			quary_secure_boot(SECUREBOOT_3RD_KEY_SET_READ_LOCK)
#define is_4th_key_read_lock()			quary_secure_boot(SECUREBOOT_4TH_KEY_SET_READ_LOCK)
#define is_5th_key_read_lock()			quary_secure_boot(SECUREBOOT_5TH_KEY_SET_READ_LOCK)
#endif

#define CNN_VALID0		(0x0 << 8)
#define CNN_VALID1		(0x3 << 8)
#define CNN_INVALID0	(0x1 << 8)
#define CNN_INVALID1	(0x7 << 8)
#define CNN_MASK		(0x7 << 8)



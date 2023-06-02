/**
	@file		shm_info.h
	@ingroup
	@note		THESE STRUCTS ARE VERY VERY IMPORTANT FORMAT DEFINITION OF SYSTEM,
				DO NOT MODIFY ANY ITEM OR INSERT/REMOVE ANY ITEM OF THESE STRUCTS!!!

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __ARCH_COMMON_SHM_INFO_H
#define __ARCH_COMMON_SHM_INFO_H

#if defined(__UITRON)
#include "Type.h"
#else
#include <kwrap/nvt_type.h>
#endif

#define SHM_INFO_VER           0x18060815 ///< YYYY/MM/DD HH

/**
	@name Ld control flag for LDINFO.LdCtrl
*/
//@{
#define LDCF_PARTLOAD_EN    0x0000001 ///< BIT 0.PARTLOAD_EN (0=no,1=yes)
//@}

/**
	@name Ld control flag for LDINFO.LdCtrl2
*/
//@{
#define LDCF_UPDATE_FW      0x0000001 ///< BIT 0.UPDATE_FW (0=no,1=yes)
#define LDCF_UPDATE_LD      0x0000002 ///< BIT 1.UPDATE_LD (0=no,1=yes)
#define LDCF_BOOT_CARD      0x0000004 ///< BIT 2.BOOT_CARD (0=no,1=yes)
#define LDCF_BOOT_FLASH     0x0000008 ///< BIT 3.BOOT_FLASH (0=no,1=yes)
#define LDCF_UPDATE_CAL     0x0000010 ///< BIT 4.UPDATE_CAL (0=no,1=yes)
#define LDCF_UPDATE_FW_DONE 0x0000100 ///< BIT 8.UPDATE_FW_DONE (0=no,1=yes)
#define LDCF_S3_BOOT        0x0000200 ///< BIT 9.BOOT FROM S3 STATE (0=no,1=yes)
//@}

/**
     Loader information

     MUST 28 WORDS
*/
typedef struct _BOOTINFO {
	char LdInfo_1[16];  ///< LD-NAME(16) ------ w by Ld
	UINT32 LdCtrl;      ///< Fw flag (4) ----------- r by Ld
	///<        BIT 0.enable part-load (0=full load,1=part load)
	UINT32 LdCtrl2;     ///< Ld flag (4) ----------- w by Ld
	///<        BIT 0.UPDATE_FW (0=no,1=yes)
	///<        BIT 1.UPDATE_LOADER (0=no,1=yes)
	///<        BIT 2.BOOT_CARD (0=no,1=yes)
	///<        BIT 3.BOOT_FLASH (0=no,1=yes)
	///<        BIT 4.UPDATE_CAL (0=no,1=yes)
	///<        BIT 8.UPDATE_FW_DONE (0=no,1=yes)
	//<         BIT 9.BOOT FROM S3 STATE (0=no,1=yes)
	UINT32 LdLoadSize;  ///< Ld load size (4) ------ w by Ld (NOTE: this value must be block size align)
	UINT32 LdLoadTime;  ///< Ld exec time(us) (4) -- w by Ld
	UINT32 LdResvSize;  ///< Ld size (by bytes, reserved size in partition) (4) ------ w by Ld
	UINT32 FWResvSize;  ///< FW reserved size (4) ------ w by Ld
	UINT16 LdPackage;   ///< IC package expected by Ld (0xFF: ES, 0: 660, 3: 663, 5: 665, etc...)
	UINT16 LdStorage;   ///< Internal storage expected by Ld (0: unkown, 1: nand, 2: spi nand, 3: spi nor)
	UINT32 fdt_addr;
	UINT32 Resv[16];     ///< (4*5) ------------ reserved for project Ld
} BOOTINFO;

STATIC_ASSERT(sizeof(BOOTINFO) == 112);

/**
     communication information

     MUST 54 WORDS
*/
//BOOT_REASON
#define BOOT_REASON_NORMAL   0 // normal or from sd
#define BOOT_REASON_FWUPDFW  1 // Firmware update Firmware
#define BOOT_REASON_FROM_USB 2 // Update from USB
#define BOOT_REASON_FROM_ETH 3 // Update from ethernet
#define BOOT_REASON_FORMAT_ROOTFS 4 // Format rootfs because rootfs broken is detected
#define BOOT_REASON_RECOVERY_SYS	5 // System recovery from eMMC
#define BOOT_REASON_FROM_UART 6 // Update from uart

//COMMINFO::Resv field definition
typedef enum _COMM_RESV_IDX_{
	COMM_RESV_IDX_BOOT_REASON = 0,
	COMM_RESV_IDX_CORE1_START = 1,
	COMM_RESV_IDX_CORE2_START = 2,
	COMM_RESV_IDX_UITRON_COMP_ADDR = 3,
	COMM_RESV_IDX_UITRON_COMP_LEN = 4,
	COMM_RESV_IDX_FW_UPD_ADDR = 5,
	COMM_RESV_IDX_FW_UPD_LEN = 6,
	COMM_RESV_IDX_OTA_NAME_ADDR = 7,
	COMM_RESV_IDX_S3_APB_ADDR = 8,
	COMM_RESV_IDX_S3_APB_LEN = 9,
} COMM_RESV_IDX;

typedef struct _COMMINFO {
	char CommInfo_1[16];///< COMMINFO (16)
	UINT32 Resv[45];///< reversed data for communication between loader, uboot, uitron, linux
	// Resv[0]: COMM_RESV_IDX_BOOT_REASON,
	//     BYTE[0]: BOOT_REASON: use #define COMM_BOOT_REASON_
	//     BYTE[1]: BOOT_DONE: 0:Not yet, 1:Done 2:NG
	//     BYTE[2]: DSP_DONE:  0:Not yet, 1:Done 2:NG
	// Resv[1]: COMM_CORE1_START (used on uboot trigger loader to start uitron)
	// Resv[2]: COMM_CORE2_START (used on uitron trigger linux resume)
	// Resv[3]: COMM_UITRON_COMP_ADDR
	// Resv[4]: COMM_UITRON_COMP_LEN
	// Resv[5]: COMM_FW_UPD_ADDR
	// Resv[6]: COMM_FW_UPD_LEN
	// Resv[7]: COMM_OTA_NAME_ADDR : OTA name
	// Resv[8]: COMM_S3_APB_ADDR : S3-resume's apb register info addr
	// Resv[9]: COMM_S3_APB_LEN : S3-resume's apb register info len
	UINT32 UserDef[5]; ///< can be used for customer
} COMMINFO;

STATIC_ASSERT(sizeof(COMMINFO) == 216);

/**
     Binary file information

     MUST 82 WORDS
*/
typedef struct _SHMINFO {
	BOOTINFO boot;
	COMMINFO comm;
} SHMINFO;

STATIC_ASSERT(sizeof(SHMINFO) == 328);
#endif /* __ARCH_COMMON_SHM_INFO_H */

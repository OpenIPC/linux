/**
    @file       SdmmcDesc.h
    @ingroup    mIDrvStg_SDIO

    @brief      SD/MMC Card Structure define

    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

*/

#ifndef _SDMMCDESC_H
#define _SDMMCDESC_H

/**
    @addtogroup mIDrvStg_SDIO
*/
//@{

/// CID register fields of SD memory card
typedef struct {
	UINT8   ucSDCIDMid;             // manufacture ID
	UINT16  usSDCIDOid;             // OEM/Application ID
	UINT8   ucSDCIDPnm[6];          // product name
	UINT8   ucSDCIDPrv;             // product version
	UINT32  uiSDCIDPsn;             // product serial number
	UINT16  usSDCIDMdt;             // manfacturing date
	UINT8   ucSDCIDCrc;             // CRC7 checksum
} SD_CID_STRUCT, *PSD_CID_STRUCT;

/// CID register fields of MMC memory card
typedef struct {
	UINT32  uiMMCCIDMid;            // manfacture ID
	UINT8   ucMMCCIDPnm[8];         // product name
	UINT8   ucMMCCIDHWRevision;     // hardware revision
	UINT8   ucMMCCIDFWRevision;     // firmware revision
	UINT32  uiMMCCIDPsn;            // card serial number
	UINT8   ucMMCCIDMonth;          // manufacturing date - month
	UINT8   ucMMCCIDYear;           // manufacturing date - year
	UINT8   ucMMCCIDCrc;            // CRC7 checksum
} MMC_CID_STRUCT, *PMMC_CID_STRUCT;

/// CSD register fields of SD/MMC memory card
typedef struct {
	UINT8   ucCSDStructure;          /* CSD structure */
	UINT8   ucMMCCSDProt;            /* MMC protocol version */
	UINT8   ucCSDTaac;               /* data read access time-1 */
	UINT8   ucCSDNsac;               /* data read access time-2 in clk cycles (NSAC * 100) */
	UINT8   ucCSDTranSpeed;          /* max data transfer speed */
	UINT16  usCSDCCC;                /* card command classes */
	UINT16  usCSDReadBlkLen;         /* max read data block length */
	UINT8   ucCSDReadBlkPartial;     /* partial blocks for read allowed */
	UINT8   ucCSDWriteBlkMisalign;   /* write block misalignment */
	UINT8   ucCSDReadBlkMisalign;    /* read block misalignment */
	UINT8   ucCSDDsrImp;             /* DSR implemented */
	UINT32  usCSDCSize;              /* device size */
	UINT8   ucCSDVddRCurrMin;        /* max read current @Vddmin */
	UINT8   ucCSDVddRCurrMax;        /* max read current @Vddmax */
	UINT8   ucCSDVddWCurrMin;        /* max write current @Vddmin */
	UINT8   ucCSDVddWCurrMax;        /* max write current @Vddmax */
	UINT8   ucCSDCSizeMult;          /* device size multiplier */
	UINT8   ucSDCSDEraseBlkEn;       /* erase single block enable */
	UINT8   ucCSDSectorSize;         /* [46~42] erase sector size */
	UINT8   ucMMCCSDEraseGrpSize;    /* [41~37] erase group size */
	UINT8   ucCSDWpGrpSize;          /* [36~32] write protect group size */
	UINT8   ucCSDWpGrpEnable;        /* [31]    write protect group enable */
	UINT8   ucMMCCSDDefaultEcc;      /* manufacturer default ECC */
	UINT8   ucCSDR2WFactor;          /* write speed factor */
	UINT8   ucCSDWriteBlkLen;        /* max write block length */
	UINT8   ucCSDWriteBlkPartial;    /* partial blocks for write allowed */
	UINT8   ucCSDFileFormatGrp;      /* file format group */
	UINT8   ucCSDCopy;               /* copy flag (OTP) */
	UINT8   ucCSDPermWriteProtect;   /* permanent write protection */
	UINT8   ucCSDTmpWriteProtect;    /* temporary write protection */
	UINT8   ucSDCSDFileFormat;       /* file format */
	UINT8   ucMMCCSDEcc;             /* ECC code */
	UINT8   ucCSDCrc;                /* crc */

} SDMMC_CSD_STRUCT, *PSDMMC_CSD_STRUCT;

// SCR register fields of SD memory card
typedef struct {
	UINT8   ucSCRStructure;         // SCR Structure
	UINT8   ucSDSpec;               // SD Memory Card - Spec. Version
	UINT8   ucDataStatAfterErase;   // data_status_after erases
	UINT8   ucSDSecurity;           // SD Security Support
	UINT8   ucSDBusWidths;          // DAT Bus widths supported
	UINT8   ucSDSpec3;              // SD spec 3.0
	UINT8   ucSDCMD20;
	UINT8   ucSDCMD23;
} SD_SCR_STRUCT, *PSD_SCR_STRUCT;

// SD Status fields of SD memory card
typedef struct {
	UINT8   ucDatBusWidth;      // data bus width
	UINT8   ucSecuredMode;      // Card is in Secured Mode of operation or not
	UINT16  usSDCardType;       // sd card type
	UINT32  uiSizeProtectArea;  // Size of protected area
	UINT8   ucSpeedClass;       // Speed Class of the card
	UINT8   ucPerformanceMove;  // Performance of move indicated by 1 MB/s step.
	UINT8   ucAuSize;           // Size of AU
	UINT16  ucEraseSize;        // Number of AUs to be erased at a time
	UINT8   ucEraseTimeout;     // Timeout value for erasing areas specified by UNIT_OF_ERASE_AU
	UINT8   ucEraseOffset;      // Fixed offset value added to erase time.
} SD_STATUS_STRUCT, *PSD_STATUS_STRUCT;

/// SD/MMC card information
typedef struct {
	UINT64  uiTotalSector;
	UINT8   ucHeadNum;
	UINT8   ucSectorNum;
	UINT64  ucRelativeSector;
	UINT16  usSectorPerCluster;
	UINT32  ucResvSec;
} SDMMC_INFO_STRUCT, *PSDMMC_INFO_STRUCT;

// SD Switch Function Status fields of SD memory card
typedef struct {
	UINT16   usMaxCurrentConsump;   // Max current consumption
	UINT16   usFunGroup6Info;       // Function group6 Info.
	UINT16   usFunGroup5Info;       // Function group5 Info.
	UINT16   usFunGroup4Info;       // Function group4 Info.
	UINT16   usFunGroup3Info;       // Function group3 Info.
	UINT16   usFunGroup2Info;       // Function group2 Info.
	UINT16   usFunGroup1Info;       // Function group1 Info.
	UINT8    ucFunGroup6Result;     // Function Group6 Result.
	UINT8    ucFunGroup5Result;     // Function Group5 Result.
	UINT8    ucFunGroup4Result;     // Function Group4 Result.
	UINT8    ucFunGroup3Result;     // Function Group3 Result.
	UINT8    ucFunGroup2Result;     // Function Group2 Result.
	UINT8    ucFunGroup1Result;     // Function Group1 Result.
	UINT8    ucDataStructVer;       // Data Structure Version
	UINT16   usFunGroup6BusySts;    // Function group6 Busy Status.
	UINT16   usFunGroup5BusySts;    // Function group5 Busy Status.
	UINT16   usFunGroup4BusySts;    // Function group4 Busy Status.
	UINT16   usFunGroup3BusySts;    // Function group3 Busy Status.
	UINT16   usFunGroup2BusySts;    // Function group2 Busy Status.
	UINT16   usFunGroup1BusySts;    // Function group1 Busy Status.
} SD_SWFUN_STATUS_STRUCT, *PSD_SWFUN_STATUS_STRUCT;

// MMC EXT CSD structure
typedef struct {
	UINT8   ucCommandSet;                // [504]     command set
	UINT8   ucHCErsGrpSize;              // [224]     High-capacity erase unit size
	UINT8   ucHCProtGrpSize;             // [221]     High-capacity write protect group size
	UINT32  uiSectorCount;               // [215~212] total sector count (512B/sector)
	UINT32  ucSecureProtInfo;            // [211]     Secure write protect information
	UINT8   ucMinPerf_W_8_52;            // [210]     minimum write performance under 8bits, 52Mhz
	UINT8   ucMinPerf_R_8_52;            // [209]     minimum read performance under 8bits, 52Mhz
	UINT8   ucMinPerf_W_8_26_4_52;       // [208]     minimum write performance under 8bits, 26Mhz/4bits, 52Mhz
	UINT8   ucMinPerf_R_8_26_4_52;       // [207]     minimum read performance under 8bits, 26Mhz/4bits, 52Mhz
	UINT8   ucMinPerf_W_4_26;            // [206]     minimum write performance under 4bits, 26Mhz
	UINT8   ucMinPerf_R_4_26;            // [205]     minimum read performance under 4bits, 26Mhz
	UINT8   ucPowerClass_26_360;         // [203]     power class under 26Mhz, 3.6V
	UINT8   ucPowerClass_52_360;         // [202]     power class under 52Mhz, 3.6V
	UINT8   ucPowerClass_26_195;         // [201]     power class under 26Mhz, 1.95V
	UINT8   ucPowerClass_52_195;         // [200]     power class under 52Mhz, 1.95V
	UINT8   ucCardType;                  // [196]     bit0: support 26Mhz, bit1: support 52Mhz
	UINT8   ucCSDStructVer;              // [194]     CSD structure version
	UINT8   ucExtCSDRev;                 // [192]     EXT CSD revision
	UINT8   ucUserProt;                  // [171]     User area write protection register
	UINT8   uiBootMode_Hs;               // [228:2]   Boot mode hs support?
	UINT8   uiBootMode_Ddr;              // [228:1]   Boot mode ddr support?
	UINT8   uiBootMode_Alt;              // [228:0]   Boot mode alternative support?
	UINT32  uiBootMode_Size;             // [226]     Boot mode size support? (uint: byte)
	UINT8   uiBootMode_Ack;              // [179:6]   Boot mode ack pattern support?
	UINT8   uiBootMode_ParEn;            // [179:3]   Boot mode Partition enable?
	UINT8   uiBootMode_ParAccess;        // [179:0]   Boot mode partition access?
	UINT8   uiBootMode_Speed;            // [177:3]   Boot mode support mode? single/high-speed/ddr?
	UINT8   uiBootMode_RstBootBusWidth;  // [177:2]   Boot mode reset boot bus?
	UINT8   uiBootMode_BusWidth;         // [177:1~0] Boot mode support bus width?
	UINT8   uiBootMode_ErsGrpDef;        // [175:0]   Boot mode erase group defition
	UINT8   uiBootMode_ConfigProt;       // [178]     Boot config protection
	UINT8   uiBootMode_BootProt;         // [173]     Boot area protection
	UINT8   uiBootMode_Class6Ctrl;       // [56]
	//UINT32  uiRPMB_Size;            // RPMB size
} MMC_EXT_CSD_STRUCT, *PMMC_EXT_CSD_STRUCT;

#define SD_HOST_BUS_1_BIT           1
#define SD_HOST_BUS_4_BITS          4
#define SD_HOST_BUS_8_BITS          8

typedef struct {
	BOOL    bEnableHS;              // TRUE: if inserted card support HS, then enable high speed
	// FALSE: always run at default mode
	BOOL    bSetDriveSink;          // TRUE: driver may set SDIO pad drive/sink
	// FALSE: driver never set SDIO pad drive/sink
	UINT8   ucMaxBusWidth;          //
} SDMMC_HOST_CONFIG_STRUCT, *PSDMMC_HOST_CONFIG_STRUCT;

//@}

#endif

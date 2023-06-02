/**
    Bin File Information

    Bin file layout

    -----------------------------------------------------
     HEADINFO  0x00 ~ 0x80 = 128 bytes = 32 WORDS
    -----------------------------------------------------

     EXCEPTION_TABLE: 0x180 ~ 0x200 = 128 bytes = 32 WORDS (see exception_MIPS.s)

    -----------------------------------------------------

     VECTOR_TABLE: 0x200 ~ 0x2f0 = 240 bytes = 60 WORDS    (see isr_MIPS.s)

    -----------------------------------------------------

     Code Info: 0x2f0 ~ 0x400 = 272 bytes = 68 WORDS       (see CodeInfo_MIPS.s)

    -----------------------------------------------------

     Code Entry: 0x400                                     (see Loader_MIPS.s)

        :

       PART-1
       PART-2
       PART-3
         :
       PART-N

    -----------------------------------------------------

    @file       bin_info.h
    @ingroup    mMODELEXT
    @note       THESE STRUCTS ARE VERY VERY IMPORTANT FORMAT DEFINITION OF SYSTEM,
                DO NOT MODIFY ANY ITEM OR INSERT/REMOVE ANY ITEM OF THESE STRUCTS!!!

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _BINI_NFO_H
#define _BIN_INFO_H

#define BIN_INFO_VER           0x19062115 ///< YYYY/MM/DD HH

#define BIN_INFO_OFFSET_RTOS   0x00000200 ///< uITRON HEADINFO.o attached to this offset
#define BIN_INFO_OFFSET_UBOOT  0x00000300 ///< uboot HEADINFO.o attached to this

//Ep: Encryption Program (CheckSum version)
//Epcrc: Encryption Program (CRC version)
//Bfc: Bin File Compress
//Ld: Loader
//Fw: Firmware

//Head control flag for HEADINFO.BinCtrl
#define HDCF_LZCOMPRESS_EN  0x0000001 ///< BIT 0.compressed enable (0=no,1=yes)

/**
     @name Ld control flag for LDINFO.LdCtrl
*/
//@{
#define BOOT_FLAG_PARTLOAD_EN    0x0000001 ///< BIT 0.PARTLOAD_EN (0=no,1=yes)
//@}

//HEADINFO::Resv field definition
typedef enum _HEADINFO_RESV_IDX_{
	HEADINFO_RESV_IDX_FDT_ADDR = 0,
	HEADINFO_RESV_IDX_SHM_ADDR = 1,
	HEADINFO_RESV_IDX_BOOT_FLAG = 2,
	HEADINFO_RESV_IDX_COUNTS = 19,
} HEADINFO_RESV_IDX;

/**
     Header information

     0x00 ~ 0x80 = 128 bytes = 32 WORDS
*/
typedef struct HEADINFO {
	unsigned int CodeEntry;   ///< [0x00] fw CODE entry (4) ----- r by Ld
	unsigned int Resv1[HEADINFO_RESV_IDX_COUNTS];
	                          ///< [0x04~0x50] reserved (4*19) -- reserved, its mem value will filled by Ld
	                          ///< Resv1[HEADINFO_RESV_IDX_FDT_ADDR]: store fdt addr for rtos
	                          ///< Resv1[HEADINFO_RESV_IDX_SHM_ADDR]: store shm addr for rtos
	                          ///< Resv1[HEADINFO_RESV_IDX_BOOT_FLAG]: boot flag for rtos
	char BinInfo_1[8];        ///< [0x50~0x58] CHIP-NAME (8) ---- r by Ep
	char BinInfo_2[8];        ///< [0x58~0x60] SDK version (8)
	char BinInfo_3[8];        ///< [0x60~0x68] SDK releasedate (8)
	unsigned int BinLength;   ///< [0x68] Bin File Length (4) --- w by Ep/bfc
	unsigned int Checksum;    ///< [0x6c] Check Sum or CRC (4) ----- w by Ep/Epcrc
	unsigned int CRCLenCheck; ///< [0x70~0x74] Length check for CRC (4) ----- w by Epcrc (total len ^ 0xAA55)
	unsigned int ModelextAddr;///< [0x74~0x78] where modelext data is. w by Ld / u-boot
	unsigned int BinCtrl;     ///< [0x78~0x7C] Bin flag (4) --- w by bfc
	                          ///<             BIT 0.compressed enable (w by bfc)
	                          ///<             BIT 1.Linux SMP enable (1: SMP/VOS, 0: Dual OS)
	unsigned int CRCBinaryTag;///< [0x7C~0x80] Binary Tag for CRC (4) ----- w by Epcrc (0xAA55 + "NT")
}
HEADINFO;

STATIC_ASSERT(sizeof(HEADINFO) == 128);


/**
     Binary file information

     0x00 ~ 0x180 = 384 bytes = 96 WORDS
*/
typedef struct _BININFO {
	HEADINFO head;      ///< 0x00 ~ 0x80 = 128 bytes = 32 WORDS, header information
	unsigned int Resv[64];  ///< 0x80 ~ 0x180 = 384 bytes = 64 WORDS, reserved information
}
BININFO;

STATIC_ASSERT(sizeof(BININFO) == 384);

///////////////////////////////////////////////////////////////////////////////
//Code Info: 0x2f0 ~ 0x400

#define ZI_SECTION_OFFSET   (0x3f0+0x10) ///< ZI area information of starting offset
#define CODE_SECTION_OFFSET (0x3f0+0x18) ///< code section information of starting offset

/*
bfc syntex

bfc.exe p1 p2 p3 p4 p5 p6 p7 p8 p9

p1: compress/decompress
p2: compress method
p3: input file name
p4: output file name
p5: partial load flag ('1' means partial load)
p6: partial load file start [locate offset of bin file]
p7: output binary file length [locate offset of bin file]
p8: partial compress flag [locate offset of bin file]
p9: NAND block size (option)

NOTE: p6~p9 must be hex format (0x****)

@$(BFC) c lz $(BIN_R) tmp 1 0x310 0x68 0x78 $(EMBMEM_BLK_SIZE)
*/

#endif

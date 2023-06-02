#ifndef __ZL380TW_FIRMWARE_H_
#define __ZL380TW_FIRMWARE_H_

#define ZL380XX_FWR_BLOCK_SIZE 16
typedef struct {
    unsigned short buf[ZL380XX_FWR_BLOCK_SIZE];     /*the firmware data block to send to the device*/
    unsigned char numWords;     /*the number of words within the block of data stored in buf[]*/
    unsigned long targetAddr;   /*the target base address to write to register 0x00c of the device*/
    unsigned char useTargetAddr; /*this value is either 0 or 1. When 1 the tarGetAddr must be written to the device*/
} twFwr;

typedef struct {
    twFwr *st_Fwr;
    unsigned char havePrgmBase;
    unsigned long prgmBase;
    unsigned long execAddr;            /*The execution start address of the firmware in RAM*/
    unsigned short twFirmwareStreamLen; /*The number of blocks within the firmware*/
    unsigned long byteCount;           /*The total number of bytes within the firmware - NOT USED*/
} twFirmware;

extern const twFwr st_twFirmware[];
extern const unsigned short firmwareStreamLen;
extern const unsigned long programBaseAddress; 
extern const unsigned long executionAddress;
extern const unsigned char haveProgramBaseAddress;
extern const unsigned short zl_firmwareBlockSize;
#endif

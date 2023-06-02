#ifndef _HAL_JPE_PLATFORM_H_
#define _HAL_JPE_PLATFORM_H_

#if defined(_FPGA_) || defined(_UDMA_) || defined(_HIF_)
//#define REG_BANK_JPD 0x1100
#define REG_BANK_JPE 0x1000
#endif
//#define JPE_BASE_ADDRESS 0x74001A00
#define MIU_BYTES_UNIT 4

#endif


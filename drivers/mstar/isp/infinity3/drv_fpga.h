#ifndef __FPGA_REG__
#define __FPGA_REG__
#include <linux/types.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "ms_platform.h"

unsigned int getTvtoolBankAddrVal(u32 bank, u8 offset);
unsigned int setTvtoolBankAddrVal(u32 bank, u8 offset, u16 val);
void fpgaInit(void);
#endif

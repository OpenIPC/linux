#ifndef __HI_CHIP_REGS_H__
#define __HI_CHIP_REGS_H__

#define GET_SYS_BOOT_MODE(_reg)        (((_reg) >> 4) & 0x3)
#define BOOT_FROM_SPI                  0
#define BOOT_FROM_NAND                 1
#endif /* End of __HI_CHIP_REGS_H__ */

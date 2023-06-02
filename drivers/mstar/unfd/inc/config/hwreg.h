#ifndef __HWREG_H__
#define __HWREG_H__

#include <mach/hardware.h>

#define REG_OFFSET_SHIFT_BITS    2

#define REG(Reg_Addr)                       (*(volatile U16*)(IO_ADDRESS(Reg_Addr)))
//#define GET_REG_ADDR(x, y)                  ((x)+((y) << REG_OFFSET_SHIFT_BITS))

#define NAND_REG(Reg_Addr)                  (*(volatile U16*)(IO_ADDRESS(Reg_Addr)))
#define NAND_GET_REG_ADDR(x, y)             ((x)+((y) << REG_OFFSET_SHIFT_BITS))

#define REG_WRITE_UINT16(reg_addr, val)      NAND_REG((reg_addr)) = (val)
#define REG_READ_UINT16(reg_addr, val)       val = NAND_REG((reg_addr))
#define REG_SET_BITS_UINT16(reg_addr, val)   NAND_REG((reg_addr)) |= (val)
#define REG_CLR_BITS_UINT16(reg_addr, val)   NAND_REG((reg_addr)) &= ~(val)
#define REG_W1C_BITS_UINT16(reg_addr, val)   REG_WRITE_UINT16((reg_addr), NAND_REG((reg_addr))&(val))


#define RIU_BASE                0xA0000000

#define REG_BANK_CHIPTOP        0xF00
#define REG_BANK_PMUTOP         0x1F80
#ifndef CHIPTOP_BASE
#define CHIPTOP_BASE            NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_CHIPTOP)
#endif
#define PMUTOP_BASE             NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_PMUTOP)

#define REG_BANK_FCIE0          0x1000 // TVTool: 20h
#define REG_BANK_FCIE1          0x1080 // TVTool: 21h
#define REG_BANK_FCIE2          0x1100 // TVTool: 22h
#define REG_BANK_FCIE3          0x1180 // TVTool: 23h

#define FCIE0_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE0) /* 0xA0004000 */
#define FCIE1_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE1) /* 0xA0004200 */
#define FCIE2_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE2) /* 0xA0004400 */
#define FCIE3_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE3) /* 0xA0004600 */

#define FCIE_REG_BASE_ADDR      FCIE0_BASE
#define FCIE_REG_BASE1_ADDR     FCIE1_BASE
#define FCIE_NC_CIFD_BASE       FCIE2_BASE
#define FCIE_NC_CIFD_BASE_2     FCIE3_BASE

#endif /* __HWREG_H__ */

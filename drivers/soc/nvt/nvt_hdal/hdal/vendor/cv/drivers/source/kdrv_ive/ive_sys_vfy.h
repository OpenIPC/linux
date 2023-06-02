#ifndef _IVE_SYS_VFY_H
#define _IVE_SYS_VFY_H

typedef enum {
	IVE_NO_FUNC = 0,   // no function
    IVE_RST = 1,       //4 bit input from threshold LUT
    IVE_TERMINATE = 2, //4 bit input from dram directly
	IVE_CLK_EN = 3, //clk en
    ENUM_DUMMY4WORD(IVE_HOOK_MODE)
} IVE_HOOK_MODE;

#endif

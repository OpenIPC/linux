/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/ir.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __MACH_IR_H
#define __MACH_IR_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

enum gk_ir_protocol {
    GK_IR_PROTOCOL_NEC          = 0,
    GK_IR_PROTOCOL_PANASONIC    = 1,
    GK_IR_PROTOCOL_SONY         = 2,
    GK_IR_PROTOCOL_PHILIPS      = 3,
    GK_IR_PROTOCOL_END
};

struct gk_ir_controller {

    int                    protocol;
    int                    debug;
};
#define GK_IR_PARAM_CALL(arg, perm) \
    module_param_cb(ir_protocol, &param_ops_int, &arg.protocol, perm); \
    module_param_cb(ir_debug, &param_ops_int, &arg.debug, perm)

/* ==========================================================================*/
extern struct platform_device            gk_ir;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif /* __MACH_IR_H */


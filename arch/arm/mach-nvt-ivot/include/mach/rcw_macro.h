/*
    Driver Register Cache Word (RCW) operation header file.

    Driver Register Cache Word (RCW) operation header file.
    The macros defined in this header file is used to simply the register
    bit field read and write.  By adapting this way of register access,
    it shall provide more error-proof and more readable way to write
    drivers.  It also does register access in load/store way which allow
    one to accurately control the number of counts the register read and
    write is proceeded from code writing.

    @file       RCWMacro.h
    @ingroup    mIDriver
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef __ASM_ARCH_NVT_IVOT_RCW_MACRO_H
#define __ASM_ARCH_NVT_IVOT_RCW_MACRO_H

#include <linux/types.h>
#include <mach/nvt-io.h>

/**
    @addtogroup mIDriver
*/
//@{

//
// Register word value type
//
// The size in bits of this type should match the RCW type
#define REGVALUE                uint32_t

#define UBITFIELD		unsigned int 	/* Unsigned bit field */
#define BITFIELD 		signed int	/* Signed bit field */


#define FALSE			0
#define TRUE			1

enum {
	//------------------------------------------------------
	//Mnemonic Value      Description
	//------------------------------------------------------
	E_OK     = 0,      // Normal completion
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_SYS    = (-5),   // System error
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_NOMEM  = (-10),  // Insufficient memory
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_NOSPT  = (-17),  // Feature not supported
	E_INOSPT = (-18),  // Feature not supported by ITRON/FILE specification
	E_RSFN   = (-20),  // Reserved function code number
	E_RSATR  = (-24),  // Reserved attribute
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_PAR    = (-33),  // Parameter error
	E_ID     = (-35),  // Invalid ID number
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_NOEXS  = (-52),  // Object does not exist
	E_OBJ    = (-63),  // Invalid object state
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_MACV   = (-65),  // Memory access disabled or memory access violation
	E_OACV   = (-66),  // Object access violation
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_CTX    = (-69),  // Context error
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_QOVR   = (-73),  // Queuing or nesting overflow
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_DLT    = (-81),  // Object being waited for was deleted
	// - - - - - - - - // - - - - - - - - - - - - - - - - - -
	E_TMOUT  = (-85),  // Polling failure or timeout exceeded
	E_RLWAI  = (-86),  // WAIT state was forcibly released
	// - - - - - - - - // - - - - - - - - - - - - - - - - - - -
};
/**
    @name Assert macros
*/
//@{
#define ASSERT_CONCAT_(a, b)    a##b
#define ASSERT_CONCAT(a, b)     ASSERT_CONCAT_(a, b)

#if defined(__COUNTER__)

#define STATIC_ASSERT(expr) \
enum { ASSERT_CONCAT(FAILED_STATIC_ASSERT_, __COUNTER__) = 1/(expr) }

#else

// This might cause compile error when writing STATIC_ASSERT at the same line
// in two (or more) files and one file include another one.
#define STATIC_ASSERT(expr) \
enum { ASSERT_CONCAT(FAILED_STATIC_ASSERT_, __LINE__) = 1/(expr) }

#endif


#define OUTW(addr, value)    	nvt_writel(value, addr)
#define INW(addr)           	nvt_readl(addr)


//
// Macros for Register Cache Word (RCW) type definition
//
// Each RCW type should be exactly the same size with REGVALUE type
// For example, to declare a Register Cache Word type:
//
//     #define rcwname_OFS   0x00   /* the name of RCW corresponding register address offset
//                                     should be in specific format with "_OFS" appended */
//     REGDEF_BEGIN(rcwname)
//         REGDEF_BIT(field1, 8)    /* declare field1 as 8 bits width */
//         REGDEF_BIT(field2, 8)    /* declare field1 as 8 bits width */
//         REGDEF_BIT(, 16)         /* pad reserved (not-used) bits to fill RCW type same as REGVALUE size */
//     REGDEF_END(rcwname)
//
// Register Cache Word type defintion header
#define REGDEF_BEGIN(name)      \
typedef union                   \
{                               \
    REGVALUE    reg;            \
    struct                      \
    {

// Register Cache Word bit defintion
#define REGDEF_BIT(field, bits) \
    UBITFIELD   field : bits;

// Register Cache Word type defintion trailer
#define REGDEF_END(name)        \
    } bit;                      \
} T_##name;                     \
STATIC_ASSERT(sizeof(T_##name) == sizeof(REGVALUE));

// Macro to define register offset
#define REGDEF_OFFSET(name, ofs)        static const uint32_t name##_OFS=(ofs);

//
// Macros for prerequisite stuff initialization for Register Cache Word
// operations
//
// Macro to set register base address for current module
// One must call this macro somewhere to set register I/O base address for
// current module, either globally or locally before any RCW operations is
// invoked
// What this macro set is only effective in current C file
#define REGDEF_SETIOBASE(base)          static const uint32_t _REGIOBASE=(base)

// Macro to set register repeat group offset address in byte
// This macro is optional and only used where register with repeat group exist,
// only one repeat group is allowed in one file
// If registers with repeat group exist, then one only have to define the RCW type
// for the first group, respective group of registers can be accessed through
// RCW_LD2, RCW_ST2, RCW_LD2OF macros
// What this macro set is only effective in current C file
#define REGDEF_SETGRPOFS(grpofs)        static const uint32_t _REGGRPOFS=(grpofs)

//
// Macros for Register Cache Word (RCW) operations
//
// These RCW operations are most likely to be used inside C functions
// It is not recommended to use RCW_DEF(RCW) in global scope which
// declare RCW as global variable
// RCW_LD/RCW_ST/RCW_LDOF pair is for normal single register access while
// RCW_LD2/RCW_ST2/RCW_LD2OF pair is for repeat group register access
// Beware that the RCW_LD/RCW_ST/RCW_LDOF and RCW_LD2/RCW_ST2/RCW_LD2OF
// must not be mixed used. e.g. use RCW_LD then RCW_ST2 or use RCW_LD2 then RCW_ST
//
// Register Cache Word declaration
#define RCW_DEF(RCW)            T##_##RCW t##RCW
// Register Cache Word read from I/O
#define RCW_LD(RCW)             t##RCW.reg = INW(_REGIOBASE+RCW##_OFS)
// Register Cache Word repeat from I/O for group read
#define RCW_LD2(RCW, grpidx)	t##RCW.reg = INW(_REGIOBASE+RCW##_OFS+(grpidx)*_REGGRPOFS)
// Register Cache Word bit reference
#define RCW_OF(RCW)             t##RCW.bit
// Register Cache Word reference
#define RCW_VAL(RCW)            t##RCW.reg
// Register Cache Word write to I/O
#define RCW_ST(RCW)             OUTW(_REGIOBASE+RCW##_OFS, t##RCW.reg)
// Register Cache Word write to I/O for repeat group
#define RCW_ST2(RCW, grpidx)	OUTW(_REGIOBASE+RCW##_OFS+(grpidx)*_REGGRPOFS, t##RCW.reg)
// Register Cache Word read and reference to register cache word bit
// !!!need to check for compiler if support for this syntax
#define RCW_LDOF(RCW)           ((T##_##RCW)(RCW_LD(RCW))).bit
// Register Cache Word read and reference to register cache word bit for repeat group
// !!!need to check for compiler if support for this syntax
#define RCW_LD2OF(RCW, grpidx)	((T##_##RCW)(RCW_LD2(RCW, grpidx))).bit

#define rcw_get_phy_addr(addr)	((addr) & 0x1FFFFFFF)

//@}

#endif /* __ASM_ARCH_NVT_IVOT_RCW_MACRO_H */

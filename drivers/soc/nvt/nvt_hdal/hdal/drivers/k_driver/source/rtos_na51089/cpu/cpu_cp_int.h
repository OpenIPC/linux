/*
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       cpu_cp_int.h
    @ingroup    mIDrvSys_Core

    @brief      NT96650 CPU core internal header file

    @note       Nothing.

*/

#ifndef _CPU_CP_INT_H
#define _CPU_CP_INT_H

#include <io_address.h>
#include "cpu_cp_reg.h"

//#define CPU_OSPREY_SETREG(ofs,value)    	OUTW(IOADDR_CPU_OSPREY_REG_BASE+(ofs),(value))
//#define CPU_OSPREY_GETREG(ofs)          	INW(IOADDR_CPU_OSPREY_REG_BASE+(ofs))


#define CA9_OUTSTANDING_SETREG(ofs,value)  	OUTW(IOADDR_TOP_REG_BASE+(ofs),(value))
#define CA9_OUTSTANDING_GETREG(ofs)         INW(IOADDR_TOP_REG_BASE+(ofs))


#define CPU_CORE_TIMER_SETREG(ofs, value)   OUTW(IOADDR_CA53_SYSCNT_REG_BASE+(ofs),(value))
#define CPU_CORE_TIMER_GETREG(ofs)          INW(IOADDR_CA53_SYSCNT_REG_BASE+(ofs))

/*
     csselrcache level & type selection

     Detailed reference to [trm p4-177 CSSELR]
*/
typedef enum _CACHE_LV_TYPE_ {
	LEVEL_1_DCACHE = 0x0,   //< Level 1 data cache
	LEVEL_1_ICACHE,         //< Level 1 instruction cache
	LEVEL_2_DCACHE,         //< Level 2 data cache

	ENUM_DUMMY4WORD(CACHE_LV_TYPE)
} CACHE_LV_TYPE;


/*
     Values for Ctype fields in CLIDR
*/

typedef enum _CLIDR_CTYPE_ {
	CLIDR_CTYPE_NO_CACHE        = 0x0,
	CLIDR_CTYPE_INSTRUCTION_ONLY = 0x1,
	CLIDR_CTYPE_DATA_ONLY       = 0x2,
	CLIDR_CTYPE_INSTRUCTION_DATA = 0x3,
	CLIDR_CTYPE_UNIFIED         = 0x4,
	ENUM_DUMMY4WORD(CLIDR_CTYPE)
} CLIDR_CTYPE;


//Instruction Synchronization Barrier.
#define _ISB() \
	__asm__ __volatile__("isb\n\t")

//Data Synchronization Barrier
#define _DSB() \
	__asm__ __volatile__("dsb\n\t")

//Data Memory Barrier
#define _DMB() \
	__asm__ __volatile__("dmb\n\t")


/*
 ************************************************************************
 *          Cache Size ID Register, CCSIDR (cp15, 1, c0, c0, 0)         *
 ************************************************************************
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |W|W|R|W|NumSets                      |Associativity      |L    |
 * |T|B|A|A|                             |                   |S    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define S_DC_SETS   8
#define S_DC_W      2
#define S_DC_LS     5

#define _DC_SETS    (1 << S_DC_SETS)    /* 256 sets */
#define _DC_W       (1 << S_DC_W)       /* 4 way */
#define _DC_LS      (1 << S_DC_LS)      /* 32 byte line size */

#define S_IC_SETS   8
#define S_IC_W      2
#define S_IC_LS     5

#define _IC_SETS    (1 << S_IC_SETS)    /* 256 sets */
#define _IC_W       (1 << S_IC_W)       /* 4 way */
#define _IC_LS      (1 << S_IC_LS)      /* 32 byte line size */

#define S_DC_W_L2   4                   /* L2 way shift*/

#define S_SC_SETS	9
#define S_SC_W		3
#define S_SC_LS		5

#define _SC_SETS	(1 << S_SC_SETS)	/* 512 sets */
#define _SC_W 		(1 << S_SC_W)		/* 8 way */
#define _SC_LS 		(1 << S_SC_LS)		/* 32 byte line size */

#define S_CCSIDR_WT     31
#define M_CCSIDR_WT     (0x1  << S_CCSIDR_WT)       /* Support write-through */
#define S_CCSIDR_WB     30
#define M_CCSIDR_WB     (0x1  << S_CCSIDR_WB)       /* Support write-back */
#define S_CCSIDR_RA     29
#define M_CCSIDR_RA     (0x1  << S_CCSIDR_RA)       /* Support read-allocation */
#define S_CCSIDR_WA     28
#define M_CCSIDR_WA     (0x1  << S_CCSIDR_WA)       /* Support write-allocation */
#define S_CCSIDR_SETS   13
#define M_CCSIDR_SETS   (0x7fff  << S_CCSIDR_SETS)  /* Number of sets */
#define S_CCSIDR_A      3
#define M_CCSIDR_A      (0x3ff  << S_CCSIDR_A)      /* Number of associatiovity */
#define S_CCSIDR_LS     0
#define M_CCSIDR_LS     (0x7  << S_CCSIDR_LS)       /* Cache line size */


#define LEVEL_1 (1 - 1)
#define LEVEL_2 (2 - 1)

#define CCSIDR() \
	({ \
		unsigned long val; \
		__asm__ __volatile__( \
							  "mrc p15, 1, %0, c0, c0, 0\n\t" \
							  : "=r" (val)); \
		val; \
	})


#define sel_CSSELR(InD) \
	__asm__ __volatile__(   \
							"mcr p15, 2, %0, c0, c0, 0\n\t" \
							: \
							: "r"(InD));

#define _ICACHE_INV_ALL() _ICIALLU(); \
	_BPIALL()

/*
    ICIALLU, Instruction Cache Invalidate All to PoU
    The ICIALLU characteristics are:

        Invalidate all instruction caches to PoU. If branch predictors are
        architecturally visible, also flush branch predictors.

        This register is part of the Cache maintenance instructions functional group.

    Usage constraints
    If EL3 is implemented and is using AArch32, this operation can be performed at the following
    exception levels:
*/
#define _ICIALLU() \
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" : : "r" (0))

/*
    Branch Predictor Invalidate All
    The BPIALL characteristics are:

        Invalidate all entries from branch predictors.
        This register is part of the Cache maintenance instructions functional group.
    Usage constraints
    If EL3 is implemented and is using AArch32, this operation can be performed at the following
    exception levels:
*/
#define _BPIALL() \
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 6" : : "r" (0))


/*
     ICIMVAU : Instruction Cache line Invalidate by VA to PoU
*/
#define _ICACHE_INV_MVAU(addr) _ICIMVAU(addr)
#define _ICIMVAU(addr) \
	__asm__ __volatile__(   \
							"mcr p15, 0, %0, c7, c5, 1\n\t" \
							: \
							: "r"(addr))

#define _DCACHE_INV_MVAC(addr) _DCIMVAC(addr)
#define _DCIMVAC(addr) \
	__asm__ __volatile__(   \
							"mcr p15, 0, %0, c7, c6, 1\n\t" \
							: \
							: "r"(addr))

//DCCMVAC Clean data cache line by VA to PoC
#define _DCACHE_WBACK_MVAC(addr) _DCCMVAC(addr)
#define _DCCMVAC(addr) \
	__asm__ __volatile__(   \
							"mcr p15, 0, %0, c7, c10, 1\n\t" \
							: \
							: "r"(addr))

//_DCCIMVAC Clean and invalidate data cache line by VA to PoC
#define _DCACHE_WBACK_INV_MVAC(addr) _DCCIMVAC(addr)
#define _DCCIMVAC(addr) \
	__asm__ __volatile__(   \
							"mcr p15, 0, %0, c7, c14, 1\n\t" \
							: \
							: "r"(addr))

//DCCISW Clean and invalidate data cache line by set/way
#define _DCCISW(way_set) \
	__asm__ __volatile__(   \
							"mcr p15, 0, %0, c7 ,c14, 2\n\t"  \
							:   \
							: "r"(way_set))

//DCISW Invalidate data cache line by set/way
/*
    The DCISW input value bit assignments are:

    |31......................4 | 3...1 | 0 |
     Setway                      level  rsv
     bit[3..1] = 0x0 = Level 1
     bit[3..1] = 0x1 = Level 2
     ...
*/
#define _DCISW(way_set) \
	__asm__ __volatile__(   \
							"mcr p15, 0, %0, c7 ,c6, 2\n\t"  \
							:   \
							: "r"(way_set))


//DCCSW Clean data cache line by set/way
/*
    The DCCSW input value bit assignments are:

    |31......................4 | 3...1 | 0 |
     Setway                      level  rsv
     bit[3..1] = 0x0 = Level 1
     bit[3..1] = 0x1 = Level 2
     ...
*/
#define _DCCSW(way_set) \
	__asm__ __volatile__(   \
							"mcr p15, 0, %0, c7 ,c10, 2\n\t"  \
							:   \
							: "r"(way_set))




/* Cache dirty register () */
#define read_CDSR() \
	({ \
		unsigned long val; \
		__asm__ __volatile__(\
							 "mrc p15, 0, %0, c7, c10, 6\n\t" \
							 : "=r"(val) \
							);\
		val;\
	})

/* Get CPU ID */
#define read_MPIDR() \
	({ \
		unsigned long val; \
		__asm__ __volatile__(\
							 "mrc p15, 0, %0, c0, c0, 5\n\t" \
							 : "=r"(val) \
							);\
		val;\
	})

/* ead current CP15 Cache Level ID Register */
#define read_CLIDR() \
	({ \
		unsigned long val; \
		__asm__ __volatile__(\
							 "mrc p15, 1, %0, c0, c0, 1\n\t" \
							 : "=r"(val) \
							);\
		val;\
	})

#define change_property() \
	({ \
		__asm__ __volatile__( \
							  ".arch_extension sec\n\t" \
							  "smc #123\n\t" \
							  : \
							  : \
							); \
	})

#define CLEAR_MMU() \
	__asm__ __volatile__("mcr p15, 0, r0, c8, c7, 0\n\t");


#define DISABLE_MMU()   \
	({ \
		unsigned long val = 0; \
		__asm__ __volatile__( \
							  "mrc p15, 0, %0, c1, c0, 0\n\t" \
							  "bic %0, %0, #0x1\n\t" \
							  "mcr p15, 0, %0, c1, c0, 0\n\t" \
							  : "=r"(val) \
							  : \
							); \
	})

#define DISABLE_DCACHE()    \
	({ \
		unsigned long val = 0; \
		__asm__ __volatile__( \
							  "mrc p15, 0, %0, c1, c0, 0\n\t" \
							  "bic %0, %0, #0x4\n\t" \
							  "mcr p15, 0, %0, c1, c0, 0\n\t" \
							  : "=r"(val) \
							  : \
							); \
	})

#define DISABLE_ICACHE()    \
	({ \
		unsigned long val = 0; \
		__asm__ __volatile__( \
							  "mrc p15, 0, %0, c1, c0, 0\n\t" \
							  "bic %0, %0, #0x1000\n\t" \
							  "mcr p15, 0, %0, c1, c0, 0\n\t" \
							  : "=r"(val) \
							  : \
							); \
	})

#define SETUP_TTBR0(val)    \
	({  \
		__asm__ __volatile__(  \
							   "mcr p15, 0, %0, c2, c0, 0\n\t"    \
							   : \
							   :  "r" (val) \
							); \
	})

#define read_cntfrq() \
	({ \
		unsigned long freq; \
		__asm__ __volatile__(\
							 "mrc p15, 0, %0, c14, c0, 0\n\t" \
							 : "=r"(freq) \
							);\
		freq;\
	})


#define write_cntfrq(freq)\
	({\
		__asm__ __volatile__("mcr p15, 0, %0, c14, c0, 0" : : "r" (freq));\
	})


#define read_PMCR() \
	({ \
		unsigned long cfg; \
		__asm__ __volatile__(\
							 "mrc p15, 0, %0, c9, c12, 0\n\t" \
							 : "=r"(cfg) \
							);\
		cfg;\
	})

#define read_CBAR() \
	({ \
		unsigned long cfg; \
		__asm__ __volatile__(\
							 "mrc p15, 1, %0, c15, c3, 0\n\t" \
							 : "=r"(cfg) \
							);\
		cfg;\
	})


#define write_PMCR(m)\
	({\
		__asm__ __volatile__("mcr p15, 0, %0, c9, c12, 0" : : "r" (m));\
	})

#define write_cntfrq(freq)\
	({\
		__asm__ __volatile__("mcr p15, 0, %0, c14, c0, 0" : : "r" (freq));\
	})

#define read_PMCEID0() \
	({ \
		unsigned long cfg; \
		__asm__ __volatile__(\
							 "mrc p15, 0, %0, c9, c12, 6\n\t" \
							 : "=r"(cfg) \
							);\
		cfg;\
	})

#define read_flags() \
	({ \
		unsigned long cfg; \
		__asm__ __volatile__(\
							 "mrc p15, 0, r0, c9, c12, 3\n\t" \
							 : "=r"(cfg) \
							);\
		cfg;\
	})

#define write_flags(m)\
	({\
		__asm__ __volatile__("mcr p15, 0, %0, c9, c12, 3" : : "r" (m));\
	})

/*
Copyright c 2013 ARM Limited. All rights reserved. ARM DDI 0487A.b
Non-Confidential - Beta ID122413
G5.1.99   MIDR, Main ID Register
The MIDR characteristics are:
Purpose
Provides identification information for the processor, including an implementer code for the device
and a device ID number.
This register is part of the Identification registers functional group.
Usage constraints
This register is accessible as shown below:
If EL2 is implemented, reads of MIDR from EL1(NS) return the value from VPIDR.
Configurations
There is one instance of this register that is used in both Secure and Non-secure states.
MIDR is architecturally mapped to AArch64 register MIDR_EL1.
MIDR is architecturally mapped to external register MIDR_EL1.
Some fields of MIDR are IMPLEMENTATION DEFINED. For details of the values of these fields for a
particular ARMv8 implementation, and any implementation-specific significance of these values,
see the product documentation.
Attributes
MIDR is a 32-bit register.
Field descriptions
The MIDR bit assignments are:
Implementer, bits [31:24]
The Implementer code. This field must hold an implementer code that has been assigned by ARM.
Assigned codes include the following:
EL0 (NS) EL0 (S) EL1 (NS) EL1 (S) EL2 EL3 (SCR.NS=1) EL3 (SCR.NS=0)
- - RO RO RO RO RO
Implementer
31 24
Variant
23 20 19 16
PartNum
15 4
Revision
3 0
Architecture
Hex
representation
ASCII
representation
Implementer
0x41 A ARM Limited
0x42 B Broadcom Corporation
0x43 C Cavium Inc.
0x44 D Digital Equipment CorporationARM can assign codes that are not published in this manual. All values not assigned by ARM are
reserved and must not be used.
Variant, bits [23:20]
An IMPLEMENTATION DEFINED variant number. Typically, this field is used to distinguish between
different product variants, or major revisions of a product.
Architecture, bits [19:16]
The permitted values of this field are:
0001  ARMv4
0010  ARMv4T
0011  ARMv5 (obsolete)
0100  ARMv5T
0101  ARMv5TE
0110  ARMv5TEJ
0111  ARMv6
1111  Defined by CPUID scheme
All other values are reserved.
PartNum, bits [15:4]
An IMPLEMENTATION DEFINED primary part number for the device.
On processors implemented by ARM, if the top four bits of the primary part number are 0x0 or 0x7,
the variant and architecture are encoded differently.
Processors implemented by ARM have an implementer code of 0x41.
Revision, bits [3:0]
An IMPLEMENTATION DEFINED revision number for the device.
Note
  For an ARM implementation, the MIDR is interpreted as follows:
Bits[31:24] Implementer code, must be 0x41
Bits[23:20] Major revision number, rX
Bits[19:16] Architecture code, must be 0xF
Bits[15:4]  ARM part number
Bits[3:0]  Minor revision number, pY
0x49 I Infineon Technologies AG
0x4D M Motorola or Freescale Semiconductor Inc.
0x4E N NVIDIA Corporation
0x50 P Applied Micro Circuits Corporation
0x51 Q Qualcomm Inc.
0x56 V Marvell International Ltd.
0x69 i Intel Corporation
Hex
representation
ASCII
representation
Implementer
ARM DDI 0487A.b_errata_2014_Q2.
Errata updates copyright c 2013, 2014 ARM Limited.
*/
/**
     CPUID MIDR

     Read of CPUID register
     @return Description of data returned.
         - @b CPUID:   Specific cpu id

             Revision, bits [3:0] An IMPLEMENTATION DEFINED revision number for the device.
*/


#define CPUID_MIDR()    \
	({ \
		unsigned long val = 0; \
		__asm__ __volatile__( \
							  "mrc p15, 0, %0, c0, c0, 0\n\t" \
							  : "=r" (val) \
							); \
		val;\
	})


enum {
	HW_BREAKPOINT_EMPTY = 0,
	HW_BREAKPOINT_R     = 1,
	HW_BREAKPOINT_W     = 2,
	HW_BREAKPOINT_RW    = HW_BREAKPOINT_R | HW_BREAKPOINT_W,
	HW_BREAKPOINT_X     = 4,
	HW_BREAKPOINT_INVALID   = HW_BREAKPOINT_RW | HW_BREAKPOINT_X,
};

enum bp_type_idx {
	TYPE_INST   = 0,
	TYPE_DATA   = 1,
	TYPE_MAX
};

/* Debug architecture numbers. */
#define ARM_DEBUG_ARCH_RESERVED 0   /* In case of ptrace ABI updates. */
#define ARM_DEBUG_ARCH_V6   1
#define ARM_DEBUG_ARCH_V6_1 2
#define ARM_DEBUG_ARCH_V7_ECP14 3
#define ARM_DEBUG_ARCH_V7_MM    4
#define ARM_DEBUG_ARCH_V7_1 5


#define ARM_FSR_ACCESS_MASK (1 << 11)

/* Privilege Levels */
#define ARM_BREAKPOINT_PRIV 1
#define ARM_BREAKPOINT_USER 2

/* Lengths */
#define ARM_BREAKPOINT_LEN_1    0x1
#define ARM_BREAKPOINT_LEN_2    0x3
#define ARM_BREAKPOINT_LEN_4    0xf
#define ARM_BREAKPOINT_LEN_8    0xff

/* Limits */
#define ARM_MAX_BRP     16
#define ARM_MAX_WRP     16
#define ARM_MAX_HBP_SLOTS   (ARM_MAX_BRP + ARM_MAX_WRP)

/* DSCR method of entry bits. */
#define ARM_DSCR_MOE(x)         ((x >> 2) & 0xf)
#define ARM_ENTRY_BREAKPOINT        0x1
#define ARM_ENTRY_ASYNC_WATCHPOINT  0x2
#define ARM_ENTRY_SYNC_WATCHPOINT   0xa

/* DSCR monitor/halting bits. */
#define ARM_DSCR_HDBGEN     (1 << 14)
#define ARM_DSCR_MDBGEN     (1 << 15)

/* OSLSR os lock model bits */
#define ARM_OSLSR_OSLM0     (1 << 0)

/* opcode2 numbers for the co-processor instructions. */
#define ARM_OP2_BVR     4
#define ARM_OP2_BCR     5
#define ARM_OP2_WVR     6
#define ARM_OP2_WCR     7

/* Base register numbers for the debug registers. */
#define ARM_BASE_BVR        64
#define ARM_BASE_BCR        80
#define ARM_BASE_WVR        96
#define ARM_BASE_WCR        112

/* Accessor macros for the debug registers. */
#define ARM_DBG_READ(N, M, OP2, VAL) do {\
		asm volatile("mrc p14, 0, %0, " #N "," #M ", " #OP2 : "=r" (VAL));\
	} while (0)

#define ARM_DBG_WRITE(N, M, OP2, VAL) do {\
		asm volatile("mcr p14, 0, %0, " #N "," #M ", " #OP2 : : "r" (VAL));\
	} while (0)

#define L2_MEM_BASE                 IOADDR_CA9_L2_CTRL_REG_BASE

#define reg0_cache_type_ofs			0x004			//reg0_cache_type
#define CACHE_CTRL_REG_OFS			0x100
#define CACHE_AUX_CTRL_REG_OFS		0x104

#define CACHE_TAG_RAM_CTRL_OFS		0x108
#define CACHE_DATA_RAM_CTRL_OFS		0x10C


#define reg2_int_clear_ofs			0x220			//reg2_int_clear
#define reg7_cache_sync				0x730			//reg7_cache_sync
#define reg7_inv_way				0x77C			//reg7_inv_way

#define CACHE_PREFETCH_REG_OFS		0xF60

#define L2_REG0_BASE	            (L2_MEM_BASE + 0x000)	/* Cache ID and Cache Type */
#define L2_REG1_BASE	            (L2_MEM_BASE + 0x100)	/* Control */
#define L2_REG2_BASE	            (L2_MEM_BASE + 0X200)	/* Interrupt and Counter Control Registers */
#define L2_REG7_BASE	            (L2_MEM_BASE + 0x700)	/* Cache Maintenance Operations */
#define L2_REG9_BASE	            (L2_MEM_BASE + 0x900)	/* Cache Lockdown */
#define L2_REG12_BASE	            (L2_MEM_BASE + 0xC00)	/* Address Filtering */
#define L2_REG15_BASE	            (L2_MEM_BASE + 0xF00)	/* Debug, Prefetch and Power */

/**
 * Cache ID and Cache Type
 */
#define L2_REG0_CACHE_ID				(*((volatile unsigned long *)(L2_REG0_BASE + 0x00)))
#define L2_REG0_CACHE_TYPE				(*((volatile unsigned long *)(L2_REG0_BASE + 0x04)))

#define S_L2_REG0_CACHE_TYPE_DB			(31)
#define M_L2_REG0_CACHE_TYPE_DB			(0xf << S_L2_REG0_CACHE_TYPE_DB)
#define S_L2_REG0_CACHE_TYPE_CTYPE		(25)
#define M_L2_REG0_CACHE_TYPE_CTYPE		(0xf << S_L2_REG0_CACHE_TYPE_CTYPE)
#define S_L2_REG0_CACHE_TYPE_H			(24)
#define M_L2_REG0_CACHE_TYPE_H			(0x1 << S_L2_REG0_CACHE_TYPE_H)
#define S_L2_REG0_CACHE_TYPE_DWS		(20)
#define M_L2_REG0_CACHE_TYPE_DWS		(0x7 << S_L2_REG0_CACHE_TYPE_DWS)
#define S_L2_REG0_CACHE_TYPE_DA			(18)
#define M_L2_REG0_CACHE_TYPE_DA			(0x1 << S_L2_REG0_CACHE_TYPE_DA)
#define S_L2_REG0_CACHE_TYPE_DLS		(12)
#define M_L2_REG0_CACHE_TYPE_DLS		(0x3 << S_L2_REG0_CACHE_TYPE_DLS)
#define S_L2_REG0_CACHE_TYPE_IWS		(8)
#define M_L2_REG0_CACHE_TYPE_IWS		(0x7 << S_L2_REG0_CACHE_TYPE_IWS)
#define S_L2_REG0_CACHE_TYPE_IA			(6)
#define M_L2_REG0_CACHE_TYPE_IA			(0x1 << S_L2_REG0_CACHE_TYPE_IA)
#define S_L2_REG0_CACHE_TYPE_ILS		(0)
#define M_L2_REG0_CACHE_TYPE_ILS		(0x3 << S_L2_REG0_CACHE_TYPE_ILS)

#define K_L2_REG0_CACHE_TYPE_DA_16WAY	(1)
#define K_L2_REG0_CACHE_TYPE_DA_8WAY	(0)

/**
 * Control
 */
#define L2_REG1_CONTROL					(*((volatile unsigned long *)(L2_REG1_BASE + 0x00)))
#define L2_REG1_AUX_CTRL				(*((volatile unsigned long *)(L2_REG1_BASE + 0x04)))
#define L2_REG1_TAG_RAM_CTRL			(*((volatile unsigned long *)(L2_REG1_BASE + 0x08)))
#define L2_REG1_DATA_RAM_CTRL			(*((volatile unsigned long *)(L2_REG1_BASE + 0x0C)))

#define S_L2_REG1_CONTROL_EN	(0)
#define M_L2_REG1_CONTROL_EN	(0x1 << S_L2_REG1_CONTROL_EN)

#define K_L2_REG1_CONTROL_EN_ON		1
#define K_L2_REG1_CONTROL_EN_OFF	0

#define S_L2_REG1_AUX_CTRL_BRESP						(30)
#define M_L2_REG1_AUX_CTRL_BRESP						(0x1 << S_L2_REG1_AUX_CTRL_BRESP)
#define S_L2_REG1_AUX_CTRL_INSTR_PREF					(29)
#define M_L2_REG1_AUX_CTRL_INSTR_PREF					(0x1 << S_L2_REG1_AUX_CTRL_INSTR_PREF)
#define S_L2_REG1_AUX_CTRL_DATA_PERF					(28)
#define M_L2_REG1_AUX_CTRL_DATA_PERF					(0x1 << S_L2_REG1_AUX_CTRL_DATA_PERF)
#define S_L2_REG1_AUX_CTRL_NS_INT_CTRL					(27)
#define M_L2_REG1_AUX_CTRL_NS_INT_CTRL					(0x1 << S_L2_REG1_AUX_CTRL_NS_INT_CTRL)
#define S_L2_REG1_AUX_CTRL_NS_LOCK_EN					(26)
#define M_L2_REG1_AUX_CTRL_NS_LOCK_EN					(0x1 << S_L2_REG1_AUX_CTRL_NS_LOCK_EN)
#define S_L2_REG1_AUX_CTRL_CACHE_POLICY					(25)
#define M_L2_REG1_AUX_CTRL_CACHE_POLICY					(0x1 << S_L2_REG1_AUX_CTRL_CACHE_POLICY)
#define S_L2_REG1_AUX_CTRL_FORCE_WA						(23)
#define M_L2_REG1_AUX_CTRL_FORCE_WA						(0x3 << S_L2_REG1_AUX_CTRL_FORCE_WA)
#define S_L2_REG1_AUX_CTRL_SHARED_OVERRIDE_EN			(22)
#define M_L2_REG1_AUX_CTRL_SHARED_OVERRIDE_EN			(0x1 << S_L2_REG1_AUX_CTRL_SHARED_OVERRIDE_EN)
#define S_L2_REG1_AUX_CTRL_PARITY_EN					(21)
#define M_L2_REG1_AUX_CTRL_PARITY_EN					(0x1 << S_L2_REG1_AUX_CTRL_PARITY_EN)
#define S_L2_REG1_AUX_CTRL_EVENT_MON_BUD_EN				(20)
#define M_L2_REG1_AUX_CTRL_EVENT_MON_BUD_EN				(0x1 << S_L2_REG1_AUX_CTRL_EVENT_MON_BUD_EN)
#define S_L2_REG1_AUX_CTRL_WAT_SIZE						(17)
#define M_L2_REG1_AUX_CTRL_WAT_SIZE						(0x7 << S_L2_REG1_AUX_CTRL_WAT_SIZE)
#define S_L2_REG1_AUX_CTRL_ASSOCIATIVITY				(16)
#define M_L2_REG1_AUX_CTRL_ASSOCIATIVITY				(0x1 << S_L2_REG1_AUX_CTRL_ASSOCIATIVITY)
#define S_L2_REG1_AUX_CTRL_SHARED_INV_EN				(13)
#define M_L2_REG1_AUX_CTRL_SHARED_INV_EN				(0x1 << S_L2_REG1_AUX_CTRL_SHARED_INV_EN)
#define S_L2_REG1_AUX_CTRL_EXCLUSIVE_CACHE_CONF			(12)
#define M_L2_REG1_AUX_CTRL_EXCLUSIVE_CACHE_CONF			(0x1 << S_L2_REG1_AUX_CTRL_EXCLUSIVE_CACHE_CONF)
#define S_L2_REG1_AUX_CTRL_STORE_BUD_DEV_LIMIT_EN		(11)
#define M_L2_REG1_AUX_CTRL_STORE_BUD_DEV_LIMIT_EN		(0x1 << S_L2_REG1_AUX_CTRL_STORE_BUD_DEV_LIMIT_EN)
#define S_L2_REG1_AUX_CTRL_HIGH_PRIO_SO_DEV_READS_EN	(10)
#define M_L2_REG1_AUX_CTRL_HIGH_PRIO_SO_DEV_READS_EN	(0x1 << S_L2_REG1_AUX_CTRL_HIGH_PRIO_SO_DEV_READS_EN)
#define S_L2_REG1_AUX_CTRL_FULL_LINE_Z_EN				(0)
#define M_L2_REG1_AUX_CTRL_FULL_LINE_Z_EN				(0x1 << S_L2_REG1_AUX_CTRL_FULL_LINE_Z_EN)

/**
 * Interrupt and Counter Control Registers
 */
#define L2_REG2_EV_CNT_CTRL				(*((volatile unsigned long *)(L2_REG2_BASE + 0x00)))
#define L2_REG2_EV_CNT1_CFG				(*((volatile unsigned long *)(L2_REG2_BASE + 0x04)))
#define L2_REG2_EV_CNT0_CFG				(*((volatile unsigned long *)(L2_REG2_BASE + 0x08)))
#define L2_REG2_EV_CNT1					(*((volatile unsigned long *)(L2_REG2_BASE + 0x0C)))
#define L2_REG2_EV_CNT0					(*((volatile unsigned long *)(L2_REG2_BASE + 0x10)))
#define L2_REG2_INT_MASK				(*((volatile unsigned long *)(L2_REG2_BASE + 0x14)))
#define L2_REG2_INT_MASK_STATUS			(*((volatile unsigned long *)(L2_REG2_BASE + 0x18)))
#define L2_REG2_INT_RAW_STATUS			(*((volatile unsigned long *)(L2_REG2_BASE + 0x1C)))
#define L2_REG2_INT_CLEAR				(*((volatile unsigned long *)(L2_REG2_BASE + 0x20)))

#define S_L2_REG2_INT_DECERR	(8)
#define M_L2_REG2_INT_DECERR	(0x1 << S_L2_REG2_INT_DECERR)	/* Decode error */
#define S_L2_REG2_INT_SLVERR	(7)
#define M_L2_REG2_INT_SLVERR	(0x1 << S_L2_REG2_INT_SLVERR)	/* Slave error */
#define S_L2_REG2_INT_ERRRD		(6)
#define M_L2_REG2_INT_ERRRD		(0x1 << S_L2_REG2_INT_ERRRD)	/* Data RAM read error */
#define S_L2_REG2_INT_ERRRT		(5)
#define M_L2_REG2_INT_ERRRT		(0x1 << S_L2_REG2_INT_ERRRT)	/* Tag RAM read error */
#define S_L2_REG2_INT_ERRWD		(4)
#define M_L2_REG2_INT_ERRWD		(0x1 << S_L2_REG2_INT_ERRWD)	/* Data RAM write error */
#define S_L2_REG2_INT_ERRWT		(3)
#define M_L2_REG2_INT_ERRWT		(0x1 << S_L2_REG2_INT_ERRWT)	/* Tag RAM write error */
#define S_L2_REG2_INT_PARRD		(2)
#define M_L2_REG2_INT_PARRD		(0x1 << S_L2_REG2_INT_PARRD)	/* Parity error on data RAM read */
#define S_L2_REG2_INT_PARRT		(1)
#define M_L2_REG2_INT_PARRT		(0x1 << S_L2_REG2_INT_PARRT)	/* Pariry error on tag RAM read */
#define S_L2_REG2_INT_ECNTR		(0)
#define M_L2_REG2_INT_ECNTR		(0x1 << S_L2_REG2_INT_ECNTR)	/* Event counter1/0 overflow increment */

/**
 * Cache Maintenance Operations
 */
#define L2_REG7_CACHE_SYNC				(*((volatile unsigned long *)(L2_REG7_BASE + 0x30)))
#define L2_REG7_INV_PA					(*((volatile unsigned long *)(L2_REG7_BASE + 0x70)))
#define L2_REG7_INV_WAY					(*((volatile unsigned long *)(L2_REG7_BASE + 0x7C)))
#define L2_REG7_CLEAN_PA				(*((volatile unsigned long *)(L2_REG7_BASE + 0xB0)))
#define L2_REG7_CLEAN_INDEX				(*((volatile unsigned long *)(L2_REG7_BASE + 0xB8)))
#define L2_REG7_CLEAN_WAY				(*((volatile unsigned long *)(L2_REG7_BASE + 0xBC)))
#define L2_REG7_CLEAN_INV_PA			(*((volatile unsigned long *)(L2_REG7_BASE + 0xF0)))
#define L2_REG7_CLEAN_INV_INDEX			(*((volatile unsigned long *)(L2_REG7_BASE + 0xF8)))
#define L2_REG7_CLEAN_INV_WAY			(*((volatile unsigned long *)(L2_REG7_BASE + 0xFC)))


#define K_L2_REG7_CACHE_SYNC_C			(0x1)

#define K_L2_REG7_INV_WAY_8WAY			(0x00ff)
#define K_L2_REG7_INV_WAY_16WAY			(0xffff)

#define K_L2_REG7_CLEAN_WAY_8WAY		(0x00ff)
#define K_L2_REG7_CLEAN_WAY_16WAY		(0xffff)

/**
 * Cache Lockdown
 */
#define L2_REG9_D_LOCKDOWN0				(*((volatile unsigned long *)(L2_REG9_BASE + 0x00)))
#define L2_REG9_I_LOCKDOWN0				(*((volatile unsigned long *)(L2_REG9_BASE + 0x04)))
#define L2_REG9_D_LOCKDOWN1				(*((volatile unsigned long *)(L2_REG9_BASE + 0x08)))
#define L2_REG9_I_LOCKDOWN1				(*((volatile unsigned long *)(L2_REG9_BASE + 0x0C)))
#define L2_REG9_D_LOCKDOWN2				(*((volatile unsigned long *)(L2_REG9_BASE + 0x10)))
#define L2_REG9_I_LOCKDOWN2				(*((volatile unsigned long *)(L2_REG9_BASE + 0x14)))
#define L2_REG9_D_LOCKDOWN3				(*((volatile unsigned long *)(L2_REG9_BASE + 0x18)))
#define L2_REG9_I_LOCKDOWN3				(*((volatile unsigned long *)(L2_REG9_BASE + 0x1C)))
#define L2_REG9_D_LOCKDOWN4				(*((volatile unsigned long *)(L2_REG9_BASE + 0x20)))
#define L2_REG9_I_LOCKDOWN4				(*((volatile unsigned long *)(L2_REG9_BASE + 0x24)))
#define L2_REG9_D_LOCKDOWN5				(*((volatile unsigned long *)(L2_REG9_BASE + 0x28)))
#define L2_REG9_I_LOCKDOWN5				(*((volatile unsigned long *)(L2_REG9_BASE + 0x2C)))
#define L2_REG9_D_LOCKDOWN6				(*((volatile unsigned long *)(L2_REG9_BASE + 0x30)))
#define L2_REG9_I_LOCKDOWN6				(*((volatile unsigned long *)(L2_REG9_BASE + 0x34)))
#define L2_REG9_D_LOCKDOWN7				(*((volatile unsigned long *)(L2_REG9_BASE + 0x38)))
#define L2_REG9_I_LOCKDOWN7				(*((volatile unsigned long *)(L2_REG9_BASE + 0x3C)))
#define L2_REG9_LOCK_LINE_EN			(*((volatile unsigned long *)(L2_REG9_BASE + 0x50)))
#define L2_REG9_UNLOCK_WAY				(*((volatile unsigned long *)(L2_REG9_BASE + 0x54)))

/**
 * Address Filtering
 */
#define L2_REG12_ADDR_FILTERING_START	(*((volatile unsigned long *)(L2_REG12_BASE + 0x00)))
#define L2_REG12_ADDR_FILTERING_END		(*((volatile unsigned long *)(L2_REG12_BASE + 0x04)))

/**
 * Debug, Prefetch and Power
 */
#define L2_REG15_DEBUG_CTRL				(*((volatile unsigned long *)(L2_REG15_BASE + 0x40)))
#define L2_REG15_PREF_CTRL				(*((volatile unsigned long *)(L2_REG15_BASE + 0x60)))
#define L2_REG15_POWER_CTRL				(*((volatile unsigned long *)(L2_REG15_BASE + 0x80)))

#define S_L2_REG15_DEBUG_CTRL_SPNIDEN	(2)
#define M_L2_REG15_DEBUG_CTRL_SPNIDEN	(0x1 << S_L2_REG15_DEBUG_CTRL_SPNIDEN)
#define S_L2_REG15_DEBUG_CTRL_DWB		(1)
#define M_L2_REG15_DEBUG_CTRL_DWB		(0x1 << S_L2_REG15_DEBUG_CTRL_DWB)
#define S_L2_REG15_DEBUG_CTRL_DCL		(0)
#define M_L2_REG15_DEBUG_CTRL_DCL		(0x1 << S_L2_REG15_DEBUG_CTRL_DCL)

#define S_L2_REG15_PREF_CTRL_DL_FILL_EN					(30)
#define M_L2_REG15_PREF_CTRL_DL_FILL_EN					(0x1 << S_L2_REG15_PREF_CTRL_DL_FILL_EN)
#define S_L2_REG15_PREF_CTRL_INST_PREF_EN				(29)
#define M_L2_REG15_PREF_CTRL_INST_PREF_EN				(0x1 << S_L2_REG15_PREF_CTRL_INST_PREF_EN)
#define S_L2_REG15_PREF_CTRL_DATA_PREF_EN				(28)
#define M_L2_REG15_PREF_CTRL_DATA_PREF_EN				(0x1 << S_L2_REG15_PREF_CTRL_DATA_PREF_EN)
#define S_L2_REG15_PREF_CTRL_DL_WRAP_READ_DIS			(27)
#define M_L2_REG15_PREF_CTRL_DL_WRAP_READ_DIS			(0x1 << S_L2_REG15_PREF_CTRL_DL_WRAP_READ_DIS)
#define S_L2_REG15_PREF_CTRL_PREF_DROP_EN				(24)
#define M_L2_REG15_PREF_CTRL_PREF_DROP_EN				(0x1 << S_L2_REG15_PREF_CTRL_PREF_DROP_EN)
#define S_L2_REG15_PREF_CTRL_INCR_DL_FILL_EN			(23)
#define M_L2_REG15_PREF_CTRL_INCR_DL_FILL_EN			(0x1 << S_L2_REG15_PREF_CTRL_INCR_DL_FILL_EN)
#define S_L2_REG15_PREF_CTRL_NOT_SAME_ID_EXCLU_SEQ_EN	(21)
#define M_L2_REG15_PREF_CTRL_NOT_SAME_ID_EXCLU_SEQ_EN	(0x1 << S_L2_REG15_PREF_CTRL_NOT_SAME_ID_EXCLU_SEQ_EN)
#define S_L2_REG15_PREF_CTRL_PREF_OFF					(0)
#define M_L2_REG15_PREF_CTRL_PREF_OFF					(0xf << S_L2_REG15_PREF_CTRL_PREF_OFF)
extern void cpu_inval_tlb(void);
//extern void ca53_set_wfi_source(CPU_WFI_SRC wfi_source);
extern void ca53_set_change_clock_interrupt_enable(BOOL int_en);
extern void ca53_trigger_change_clock_operation(BOOL trigger);

#endif  //#define _CPU_CP_INT_H

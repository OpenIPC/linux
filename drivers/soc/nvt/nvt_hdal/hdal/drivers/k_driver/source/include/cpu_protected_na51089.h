/*
    Novatek protected header file of NT96680's driver.

    The header file for Novatek protected APIs of NT96650's driver.

    @file       cpu_protected.h
    @ingroup    mIDriver
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _NVT_CPU_PROTECTED_H
#define _NVT_CPU_PROTECTED_H

#include <kwrap/nvt_type.h>
#include "comm/timer.h"

#define AES_KEYSIZE_128    16
#define AES_KEYSIZE_256    32

typedef enum {
	CORE_REG_ZERO,
	CORE_REG_AT,
	CORE_REG_V0,
	CORE_REG_V1,

	CORE_REG_A0,
	CORE_REG_A1,
	CORE_REG_A2,
	CORE_REG_A3,

	CORE_REG_T0,
	CORE_REG_T1,
	CORE_REG_T2,
	CORE_REG_T3,

	CORE_REG_T4,
	CORE_REG_T5,
	CORE_REG_T6,
	CORE_REG_T7,

	CORE_REG_S0,
	CORE_REG_S1,
	CORE_REG_S2,
	CORE_REG_S3,

	CORE_REG_S4,
	CORE_REG_S5,
	CORE_REG_S6,
	CORE_REG_S7,

	CORE_REG_T8,
	CORE_REG_T9,

	CORE_REG_K0,
	CORE_REG_K1,

	CORE_REG_GP,
	CORE_REG_SP,
	CORE_REG_FP,
	CORE_REG_RA,

	CORE_REG_ENTRY_HI,
	CORE_REG_ENTRY_LO,
	CORE_REG_STATUS,
	CORE_REG_VECTOR,

	CORE_REG_ENTRY_EPC,
	CORE_REG_ENTRY_CAUSE,
	CORE_REG_ENTRY_BADVADDR,
	CORE_REG_ENTRY_HWRENA,
	CORE_REG_ENTRY_PRID,

	CORE_REG_ENTRY_CNT,

	ENUM_DUMMY4WORD(CORE_CPU_REG)

} CORE_CPU_REG;

typedef enum {
	CA53_WFI_NONE = 0x0,
	CA53_WFI_CORE0 = 0x1,
	CA53_WFI_CORE1 = 0x2,
	CA53_WFI_L2 = 0x4,

	ENUM_DUMMY4WORD(CPU_WFI_SRC)
} CPU_WFI_SRC;

typedef enum {
	CA53_INTERNAL_DIV_DEFAULEE = 0x0,
	CA53_INTERNAL_DIV_1,
	CA53_INTERNAL_DIV_2,
	CA53_INTERNAL_DIV_3,
	CA53_INTERNAL_DIV_4,
	CA53_INTERNAL_DIV_5,
	CA53_INTERNAL_DIV_6,
	CA53_INTERNAL_DIV_7,
	CA53_INTERNAL_DIV_MAX,

	ENUM_DUMMY4WORD(CPU_INTERNAL_CLK_DIV)
} CPU_INTERNAL_CLK_DIV;

/*
     Values for Bridge No
*/

typedef enum _CPU_BRIDGE_ {
	CPU_BRIDGE1  = 0x0,
	CPU_BRIDGE2,

	ENUM_DUMMY4WORD(CPU_BRIDGE)
} CPU_BRIDGE;

/*
     Values for outstanding configuration
*/

typedef enum _OUTSTANDING_TYPE_ {
	CPU_OUTSTANDING_DISABLE  = 0x0,
	CPU_OUTSTANDING_ENABLE,

	ENUM_DUMMY4WORD(OUTSTANDING_TYPE)
} OUTSTANDING_TYPE;

/*
     Values for bufferable type
*/

typedef enum _BUFFERABLE_MODE_ {
	CPU_FIXED_MODE          = 0x0,
	CPU_BY_COMMAND,

	ENUM_DUMMY4WORD(BUFFERABLE_MODE)
} BUFFERABLE_MODE;

/*
     Values for bufferable type
*/

typedef enum _BUFFERABLE_SET_ {
	CPU_NON_BUFFERABLE      = 0x0,
	CPU_BUFFERABLE,

	ENUM_DUMMY4WORD(BUFFERABLE_SET)
} BUFFERABLE_SET;

/* Breakpoint */
#define ARM_BREAKPOINT_EXECUTE      0

/* Watchpoints */
#define ARM_BREAKPOINT_LOAD         1
#define ARM_BREAKPOINT_STORE        2
#define ARM_BREAKPOINT_LOAD_STORE   3

struct arch_hw_breakpoint_ctrl {
	UINT32  __reserved  : 9,
			mismatch            : 1,
			: 9,
			len                 : 8,
			type                : 2,
			privilege           : 2,
			enabled             : 1;
};

typedef struct {
	UINT32  address;
	UINT32  trigger;
	UINT32  size;
	struct  arch_hw_breakpoint_ctrl step_ctrl;
	struct  arch_hw_breakpoint_ctrl ctrl;
} arch_hw_breakpoint, *parch_hw_hreakpoint;

extern ER   ca53_set_aes_key_size(UINT32 uiKeySize);
/*
     AEC ecb encryption.

     AEC ecb encryption by arm v8 instruction

     @param[in]     uiKeySize   AES_KEYSIZE_128 or AES_KEYSIZE_256 bits
     @param[in]     in_key      point of AES key
     @param[out]    out         AES encryption output data
     @param[in]     in          point of AES planeText data

     Example: (Optional)
     @code
     {
        const UINT8 __data[16] = {0x32, 0x88, 0x31, 0xE0, 0x43, 0x5a, 0x31, 0x37, 0xf6, 0x30, 0x98, 0x07, 0xa8, 0x8d, 0xa2, 0x34};
        const UINT8 __key[16]  = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
        UINT8    __cypher[16];
        UINT32  i;
        if(ca53_do_aes_ecb_encrypt(AES_KEYSIZE_128, __key, __cypher, __data) == E_OK)
        {
            emu_msg(("aes ebc ok => "));
            for(i = 0; i < sizeof(__cypher); i++)
            {
                emu_msg(("0x%02x ", __cypher[i]));
            }
            emu_msg(("\r\n"));
        }
        else
        {
            emu_msg(("aes ebc NG\r\n"));
        }
     }
     @endcode
*/
extern ER       ca53_do_aes_ecb_encrypt(UINT32 uiKeySize, const UINT8 *in_key, UINT8 out[], UINT8 const in[]);
extern ER       ca53_do_aes_cbc_encrypt(UINT32 uiKeySize, const UINT8 *in_key, UINT8 out[], UINT8 const in[], UINT8 iv[]);
extern ER       ca53_do_aes_cfb_encrypt(UINT32 uiKeySize, const UINT8 *in_key, UINT8 out[], const UINT8 in[], UINT8 iv[]);
extern ER       ca53_do_aes_ofb_encrypt(UINT32 uiKeySize, const UINT8 *in_key, UINT8 out[], const UINT8 in[], UINT8 iv[]);
extern ER       ca53_do_aes_ctr_encrypt(UINT32 uiKeySize,  const   UINT8 *in_key, UINT8 out[], const UINT8  in[], UINT8  iv[]);
//extern void     ca53_set_cti_halt_enable(CC_CORE_ID uiCore, BOOL bEn);
extern ER       ca9_set_outstanding(CPU_BRIDGE brg, OUTSTANDING_TYPE type, BUFFERABLE_MODE buffer_mode, BUFFERABLE_SET buffer_set);
extern ER       ca9_get_outstanding(CPU_BRIDGE brg, OUTSTANDING_TYPE *type, BUFFERABLE_MODE *buffer_mode, BUFFERABLE_SET *buffer_set);
extern ER 		ca9_set_bus_timeout_enable(BOOL en);
extern void     ca53_cycle_count_start(BOOL do_reset, BOOL enable_divider);
extern void     ca53_cycle_count_stop(void);
extern UINT32   ca53_get_cycle_count(void);
extern void     ca53_core2_start(void);
extern void     ca53_core2_stop(void);
extern UINT32   ca53_get_smpen(void);




extern ER       ca53_change_internal_clock(CPU_INTERNAL_CLK_DIV l2_arm_clk_div, CPU_INTERNAL_CLK_DIV periph_clk_div, CPU_INTERNAL_CLK_DIV coresight_clk_div);
extern ER       ca53_debug_init(void);
extern ER       ca53_install_hw_breakpoint(arch_hw_breakpoint info, UINT32 trigger_type);


/**
    core communication core ID number

    @note For cc_hwLockResource(), cc_hwUnlockResource(), cc_getHwResGrant(),cc_getHwResReqFlag(), cc_startCore(), cc_stopCore(), cc_configCoreOutstanging()
*/
typedef enum {
	CC_CORE_CA53_CORE1 = 0x0,   ///< 1st Core CA53 core1
	CC_CORE_CA53_CORE2,         ///< 2nd Core CA53 core2

	CC_CORE_CA53_CORE_NUM,

	ENUM_DUMMY4WORD(CC_CORE_ID)

} CC_CORE_ID;

extern ER       timer_cfg_core_target(TIMER_ID TimerID, CC_CORE_ID CoreID);
extern ER       timer_cfg_core_target_int_enable(TIMER_ID TimerID, CC_CORE_ID CoreID);
extern void 	pmu_init(void);
extern void 	pmu_start(unsigned int event_array[],unsigned int count);
extern void 	pmu_stop(UINT32 usec);


#endif



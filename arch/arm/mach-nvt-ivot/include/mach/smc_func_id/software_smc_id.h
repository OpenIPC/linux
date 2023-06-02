#ifndef __SOFTWARE_SMC_ID_H__
#define __SOFTWARE_SMC_ID_H__

#include "smc_id_def.h"

#define NVT_SMC_SW_FUNC_ID_VER     0x0001

/* Add new "SW" module type here*/
enum {
	MODULE_SW_SMC_ID = 0,
	MODULE_SW_COMMON_ID,
	MODULE_SW_ID_INVALID = 0x40,
};

/*Get version of software SMC function identifier*/
#define NVT_GET_SMC_SW_FUNC_ID_VER OEM_FAST_FUNC_GEN(SOFTWARE_TYPE, MODULE_SW_SMC_ID, ID_0)
#define NVT_GET_SMC_HW_FUNC_ID_VER OEM_FAST_FUNC_GEN(SOFTWARE_TYPE, MODULE_SW_SMC_ID, ID_1)

/*Command for SMC test*/
#define NVT_HELLO_WORD             OEM_FAST_FUNC_GEN(SOFTWARE_TYPE, MODULE_SW_COMMON_ID, ID_0)
#endif

#ifndef __NVT_SMC_ID_DEF_H__
#define __NVT_SMC_ID_DEF_H__

#include "optee_smc.h"

/* definition for make a function ID */
#define FUNCTION_NUM_GEN(type, module_id, func_num) \
		(((type & TYPE_MASK) << TYPE_SHIFT) | ((module_id & MODULE_MASK) << MODULE_SHIFT) | ((func_num & FUNC_MASK) << FUNC_SHIFT))

#define OEM_FAST_FUNC_GEN(type, module_id, func_num) \
		OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_FAST_CALL, OPTEE_SMC_OWNER_OEM, FUNCTION_NUM_GEN(type, module_id, func_num))

#define OEM_STD_FUNC_GEN(type, module_id, func_num) \
		OPTEE_SMC_CALL_VAL(OPTEE_SMC_32, OPTEE_SMC_STD_CALL, OPTEE_SMC_OWNER_OEM, FUNCTION_NUM_GEN(type, module_id, func_num))

/* Function type */
#define TYPE_SHIFT					14
#define TYPE_MASK					3
#define SOFTWARE_TYPE					0
#define HW_TYPE						1
#define CUSTOM_TYPE					2

/* Module ID define */
#define MODULE_SHIFT					8
#define MODULE_MASK					0x3F

/* Function number for each module */
#define FUNC_MASK					0xFF
#define FUNC_SHIFT					0

typedef enum {
	ID_0  =  0, ID_1 , ID_2 , ID_3 , ID_4 , ID_5 ,	ID_6 , ID_7 , ID_8 , ID_9 ,
	ID_10 = 10, ID_11, ID_12, ID_13, ID_14, ID_15,	ID_16, ID_17, ID_18, ID_19,
	ID_20 = 20, ID_21, ID_22, ID_23, ID_24, ID_25,	ID_26, ID_27, ID_28, ID_29,
	ID_30 = 30, ID_31, ID_32, ID_33, ID_34, ID_35,	ID_36, ID_37, ID_38, ID_39,
	ID_40 = 40, ID_41, ID_42, ID_43, ID_44, ID_45,	ID_46, ID_47, ID_48, ID_49,
	ID_50 = 50, ID_51, ID_52, ID_53, ID_54, ID_55,	ID_56, ID_57, ID_58, ID_59,
	ID_60 = 60, ID_61, ID_62, ID_63, ID_INVALID
} FUNCTION_ID;

#endif

/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_OS_MEM_H__
#define __AX_OS_MEM_H__

#define AX_OS_MEM_MAGIC                    ('o'+'s'+'m')
#define AX_OS_MEM_GET_SHARD_DATA_ADDR  _IOWR(AX_OS_MEM_MAGIC, 1, unsigned long)
#define AX_OS_MEM_RES_MODEL_IMG_MEM  _IOWR(AX_OS_MEM_MAGIC, 2, unsigned long)
#define AX_OS_MEM_GET_MODEL_IMG_INFO  _IOWR(AX_OS_MEM_MAGIC, 3, unsigned long)

/*******************************************************************
|----------------------------------------------------------------|
|   reserved                | SUB_MODULE_ID  |   MODULE_ID       |
|----------------------------------------------------------------|
|<--16bits[31:16]-----------><--8bits[15:8]--><-----8bits[7:0]---->|
*******************************************************************/
typedef enum {
	AX_OS_MEM_POLICY_0 = 0,
	AX_OS_MEM_POLICY_1 = 1,
	AX_OS_MEM_POLICY_2 = 2,
	AX_OS_MEM_POLICY_MAX = 3,
} OS_MEM_POLICY_E;

typedef struct {
	unsigned int magic;
	unsigned int policy;
	unsigned int mod_num;
	unsigned int reserved;
} mem_proc_data_header_t;

typedef struct {
	unsigned int usr_size;
	unsigned int usr_times;
	unsigned int usr_max_total_size;
	unsigned int usr_max_times;
} mem_proc_data_t;

#endif

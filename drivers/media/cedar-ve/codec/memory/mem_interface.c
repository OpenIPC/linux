/*
 * =====================================================================================
 *
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 *       Filename:  ion_mem.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020-04-15
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jilinglin, <jilinglin@allwinnertech.com>
 *        Company:
 *
 * =====================================================================================
 */
#include "mem_interface.h"
#include "ion_mem.h"
#include "cdc_log.h"

struct mem_interface *mem_create(int type, struct mem_param param)
{
	switch (type) {
	case MEM_TYPE_ION:
		return mem_ion_create();
	default:
		logd("mem type unknown!");
		return NULL;
	}
}
EXPORT_SYMBOL(mem_create);

void mem_destory(int type, struct mem_interface *p)
{
	switch (type) {
	case MEM_TYPE_ION:
		mem_ion_destory(p);
		break;
	default:
		logd("mem type unknown!");
	}
}
EXPORT_SYMBOL(mem_destory);

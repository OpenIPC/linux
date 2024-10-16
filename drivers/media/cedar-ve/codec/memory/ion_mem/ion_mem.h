/*
 * =====================================================================================
 *
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 *       Filename:  ion_mem.h
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

#ifndef MEM_ION_H
#define MEM_ION_H

struct mem_interface *mem_ion_create(void);
void mem_ion_destory(struct mem_interface *p);

#endif

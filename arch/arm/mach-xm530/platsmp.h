/******************************************************************************
 *    COPYRIGHT (C) 2013 Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-12-19
 *
******************************************************************************/
#ifndef __PLATSMP__H__
#define __PLATSMP__H__

extern struct smp_operations xm530_smp_ops;

void xm530_scu_power_up(int cpu);
void xm530_secondary_startup(void);

#endif


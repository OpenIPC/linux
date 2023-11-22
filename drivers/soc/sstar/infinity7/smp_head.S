/*
* smp_head.S- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/
#include <linux/linkage.h>
#include <linux/init.h>

/*  __CPUINIT */

#define ACTLR_SMP_ENABLE            0x0040
#define ACTLR_DODMBS_DISABLE        0x0400
#define ACTLR_L2RADIS_DISABLE       0x0800
#define ACTLR_L1RADIS_DISABLE       0x1000
#define ACTLR_L1PCTL_3_PREFETCH     0x6000
#define ACTLR_DDVM_DISABLE          0x8000
#define Mode_MON              0x16
#define Mode_SVC              0x13
#define  TBIT                              0x20
#define  FBIT                              0x40
#define      IBIT                              0x80

#define NSACR_CP11                  0x0800
#define NSACR_CP10                  0x0400

#define CPACR_CP11_FULL             0x00C00000
#define CPACR_CP10_FULL             0x00300000

.extern mercury6_secondary_gic

/*
 * Realview/Versatile Express specific entry point for secondary CPUs.
 * This provides a "holding pen" into which all secondary cores are held
 * until we're ready for them to initialise.
 */
ENTRY(mercury6_secondary_startup)

    // enable smp


    /* Enable VFP */


    /* Native ARMv7 L1 invalide function */


    /*
     * we've been released from the holding pen: secondary_stack
     * should now contain the SVC stack for this core
     */
    b   secondary_startup

ENDPROC(mercury6_secondary_startup)

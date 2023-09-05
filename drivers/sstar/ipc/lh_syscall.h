/*
* lh_syscall.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: Canlet.Lin <Canlet.Lin@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
/*
 * lh_syscall.h
 */
#ifndef	__LH_SYSCALL_H__
#define	__LH_SYSCALL_H__

#include "rlink.h"

#define	RTK_CALL		(0x800)
#define	RTK_CLI			(0x801)
#define	RTK_LOG			(0x802)
#define	RSQ_INIT		(0x100)
#define	RSQ_SNAPSHOT	(0x101)

#define RSQ_VERSION_ID  (0x100)
typedef struct {
    struct rlink_head   root;
    char                name[8];
    char                version[64];
    unsigned int        verid;
    unsigned int        size;
    unsigned int        fiq_cnt;
    unsigned int        ffiq_ts;
    unsigned int        ttff_isp;
    unsigned int        ttff_scl;
    unsigned int        ttff_mfe;
    unsigned int        ldns_ts;
    u64                 start_ts;
    u64	                lifet;
    u64	                spent;
    u64                 linux_idle_in_rtos_time;
    unsigned int        diff;
    unsigned int        linux_idle;
    unsigned int        reserved[2];
    unsigned char       sbox[1024];
} rtkinfo_t;

extern rtkinfo_t	*_rtk;
extern u64	epiod;
extern u64 _getsysts(void);
#define	RTKINFO_FIQCNT(c)	_rtk->fiq_cnt++
#define	RTKINFO_FFIQTS()	_rtk->ffiq_ts = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_LOADNSTS()	_rtk->ldns_ts = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_TTFF_ISP()	if (!_rtk->ttff_isp) _rtk->ttff_isp = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_TTFF_SCL()	if (!_rtk->ttff_scl) _rtk->ttff_scl = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_TTFF_MFE()	if (!_rtk->ttff_mfe) _rtk->ttff_mfe = (unsigned int)(_getsysts() - epiod);
#define RTK_TIME_TO_US(x)   (x / 6)

void init_syscall(void *share, int size);
rtkinfo_t* get_rtkinfo(void);
#endif	// __LH_SYSCALL_H__

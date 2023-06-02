/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#ifndef __DRV_DUALOS_H__
#define __DRV_DUALOS_H__

#include "cam_os_wrapper.h"

#define ENABLE_NBLK_CALL    1

#define INTEROS_SC_L2R_HANDSHAKE        (0x800)
#define INTEROS_SC_L2R_RTK_CLI          (0x801)
#define INTEROS_SC_L2R_RTK_LOG          (0x802)
#define INTEROS_SC_L2R_SCR_FW_ENABLE    (0x803)
#define INTEROS_SC_L2R_SCR_FW_DISABLE   (0x804)
#define INTEROS_SC_L2R_RSQ_INIT         (0x100)
#define INTEROS_SC_L2R_CALL             (0x808)
#define INTEROS_SC_R2L_MI_NOTIFY        (0x809)
#define INTEROS_SC_L2R_SWTOE            (0xfe000000)


#define IPI_NR_RTOS_2_LINUX_CALL_REQ        8
#define IPI_NR_RTOS_2_LINUX_CALL_RESP       9
#define IPI_NR_LINUX_2_RTOS_CALL_REQ        10
#define IPI_NR_LINUX_2_RTOS_CALL_RESP       11
#define IPI_NR_RTOS_2_LINUX_NBLK_CALL_REQ   12
#define IPI_NR_REROUTE_SMC                  14
#define IPI_NR_REROUTE_SMC_BUSY_WAIT        15

#define	RTKINFO_FIQCNT(c)	if (_rtk) _rtk->fiq_cnt++
#define	RTKINFO_FFIQTS()	_rtk->ffiq_ts = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_LOADNSTS()	_rtk->ldns_ts = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_TTFF_ISP()	if (!_rtk->ttff_isp) _rtk->ttff_isp = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_TTFF_SCL()	if (!_rtk->ttff_scl) _rtk->ttff_scl = (unsigned int)(_getsysts() - epiod);
#define	RTKINFO_TTFF_MFE()	if (!_rtk->ttff_mfe) _rtk->ttff_mfe = (unsigned int)(_getsysts() - epiod);
#define RTK_TIME_TO_US(x)   (x / 6)

struct rlink_head {
	struct rlink_head *next, *prev;
	void*           nphys;  // next object physical address
	unsigned int	nsize;	// next object size
	unsigned int	reserved;
};

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
    u64                 spent_hyp;
    u64                 spent_sc;
    u64                 linux_idle_in_rtos_time;
    unsigned int        diff;
    unsigned int        linux_idle;
    u64                 syscall_cnt;

    // sbox must be 16-byte aligned
    unsigned char       sbox[1024];
} rtkinfo_t;

extern rtkinfo_t	*_rtk;
extern u64	epiod;

rtkinfo_t* get_rtkinfo(void);
unsigned long signal_rtos(u32 type, u32 arg1, u32 arg2, u32 arg3);
void handle_interos_nblk_call_req(void);
void handle_reroute_smc(void);

#endif //__DRV_DUALOS_H__

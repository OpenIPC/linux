/*!
********************************************************************************
*  \file    Flag.h
*  \brief
*  \project vos
*  \chip
*  \author Anderson
********************************************************************************
*/
#ifndef _VOS_KER_FLAG_H_
#define _VOS_KER_FLAG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "nvt_type.h"
#include "error_no.h"
#include "consts.h"

//! Init/Exit flag
void  rtos_flag_init(unsigned long max_flag_num);
void  rtos_flag_exit(void);

//! Common api
ER       vos_flag_create(ID *p_flgid, T_CFLG *pk_cflg, char *name);
ER       vos_flag_destroy(ID flgid);
ER       vos_flag_set(ID flgid, FLGPTN setptn);
ER       vos_flag_clr(ID flgid, FLGPTN clrptn);
ER       vos_flag_wait(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode);
ER       vos_flag_wait_timeout(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode, int timeout_tick);
			//note:
			//1. timeout = -1 to wait forever, the effect is the same as vos_flag_wait
			//2. use vos_util_msec_to_tick to convert to ticks
			//3. return 0 if condition is met, or negative value if timeout occurred
ER       vos_flag_wait_interruptible(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode);
			//note: Return E_RLWAI if interrupted, E_OK if conditions are met
FLGPTN   vos_flag_chk(ID flgid, FLGPTN chkptn);
char    *vos_flag_get_name(ID flgid);
void     vos_flag_dump(int (*dump)(const char *fmt, ...));

#if defined(__LINUX)
#define vos_flag_iset   vos_flag_set
#define vos_flag_iclr   vos_flag_clr
#elif defined(__FREERTOS)
ER      vos_flag_iset(ID flgid, FLGPTN setptn); //flag set for ISR (FreeRTOS Only)
ER      vos_flag_iclr(ID flgid, FLGPTN clrptn); //flag clear for ISR (FreeRTOS Only)
#endif

// obsolete uitron-style api, please use vos_flag_xxx directly
#define rel_flg         vos_flag_destroy
#define cre_flg         vos_flag_create
#define del_flg         vos_flag_destroy
#define set_flg         vos_flag_set
#define iset_flg        vos_flag_iset
#define clr_flg         vos_flag_clr
#define iclr_flg        vos_flag_iclr
#define wai_flg         vos_flag_wait
#define kchk_flg        vos_flag_chk
#define vos_flg_dump    vos_flag_dump
#define OS_CONFIG_FLAG(id) vos_flag_create(&id, NULL, #id)

#ifdef __cplusplus
}
#endif

#endif /* _VOS_KER_FLAG_H_ */


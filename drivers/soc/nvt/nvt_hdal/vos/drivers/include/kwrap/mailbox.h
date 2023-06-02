#ifndef _VOS_MAILBOX_H_
#define _VOS_MAILBOX_H_

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>
#include <kwrap/error_no.h>

typedef struct {
	UINT maxmsg; //max msg counts per mbxid
	UINT msgsize; // size in bytes
} VOS_MBX_PARAM;

//! Init for VOS
ER vos_mbx_init(UINT max_mbxid_num);
ER vos_mbx_exit(void);

//! Common api
ER vos_mbx_create(ID *p_mbxid, VOS_MBX_PARAM *p_param);
void vos_mbx_destroy(ID mbxid);

ER vos_mbx_snd(ID mbxid, void *p_data, UINT size);
ER vos_mbx_rcv(ID mbxid, void *p_data, UINT size);
UINT vos_mbx_is_empty(ID mbxid);

#ifdef __cplusplus
}
#endif

#endif /* _VOS_MAILBOX_H_ */


#ifndef _IVE_LL_CMD_H
#define _IVE_LL_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"


UINT64 ive_ll_null_cmd(UINT8 tbl_idx);
UINT64 ive_ll_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val);
UINT64 ive_ll_nextll_cmd(UINT32 addr, UINT8 tbl_idx);
UINT64 ive_ll_nextupd_cmd(UINT32 addr);
VOID ive_setdata_exit_ll(UINT64 **llbuf, UINT32 ll_idx);
VOID ive_setdata_ll(UINT64 **llbuf, UINT32 r_ofs, UINT32 r_value, UINT8 is_bit60);
VOID ive_setdata_end_ll(UINT64 **llbuf, UINT32 ll_idx, UINT8 is_next_update);
VOID ive_setdata_exit_ll(UINT64 **llbuf, UINT32 ll_idx);
VOID ive_set_start_addr_ll(UINT32 start_addr);
VOID ive_flush_buf_ll(UINT64 *s_addr, UINT64 *e_addr);

#ifdef __cplusplus
}
#endif

#endif

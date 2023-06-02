#ifndef _NUE_LL_CMD_H
#define _NUE_LL_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"


UINT64 nue_ll_null_cmd(UINT8 tbl_idx);
UINT64 nue_ll_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val);
UINT64 nue_ll_nextll_cmd(UINT32 addr, UINT8 tbl_idx);
UINT64 nue_ll_nextupd_cmd(UINT32 addr);

#ifdef __cplusplus
}
#endif

#endif

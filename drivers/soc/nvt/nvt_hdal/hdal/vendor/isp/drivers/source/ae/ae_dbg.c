#include "kwrap/type.h"
#include "ae_dbg.h"

#define AE_MAX_CNT 	10
static UINT32 ae_dbg_msg[AE_ID_MAX_NUM] = {0};
static UINT32 ae_err_msg_cnt;

UINT32 ae_dbg_get_dbg_mode(AE_ID id)
{
	if (id >= AE_ID_MAX_NUM) {
		//DBG_ERR("ae id out of range (%d) \r\n", id);
		return ae_dbg_msg[0];
	}

	return ae_dbg_msg[id];
}

void ae_dbg_set_dbg_mode(AE_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= AE_ID_MAX_NUM) {
		for (i = 0; i < AE_ID_MAX_NUM; i++) {
			ae_dbg_msg[i] = 0;
		}
	} else {
		if(cmd == 0) {
			ae_dbg_msg[id] = 0;
		} else {
			ae_dbg_msg[id] |= cmd;
		}
	}

	if (cmd == AE_DBG_NONE) {
		ae_dbg_clr_err_msg();
	}
}

void ae_dbg_clr_dbg_mode(AE_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= AE_ID_MAX_NUM) {
		for (i = 0; i < AE_ID_MAX_NUM; i++) {
			ae_dbg_msg[i] &= ~cmd;
		}
	} else {
		ae_dbg_msg[id] &= ~cmd;
	}
}

BOOL ae_dbg_check_err_msg(BOOL show_dbg_msg)
{
	BOOL rt = TRUE;

	if ((ae_err_msg_cnt < AE_MAX_CNT) && (!show_dbg_msg)) {
		rt = TRUE;
	} else {
		rt = FALSE;
	}
	ae_err_msg_cnt++;

	return rt;
}

void ae_dbg_clr_err_msg(void)
{
	ae_err_msg_cnt = 0;
}

UINT32 ae_dbg_get_err_msg(void)
{
	return ae_err_msg_cnt;
}


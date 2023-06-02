#include "kwrap/type.h"
#include "iq_dbg.h"

#define MAX_CNT 10
static UINT32 iq_dbg_msg[IQ_ID_MAX_NUM] = {0};
static UINT32 wrn_msg_cnt;

UINT32 iq_dbg_get_dbg_mode(IQ_ID id)
{
	if (id >= IQ_ID_MAX_NUM) {
		//DBG_ERR("iq id out of range (%d) \r\n", id);
		return iq_dbg_msg[0];
	}

	return iq_dbg_msg[id];
}

void iq_dbg_set_dbg_mode(IQ_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= IQ_ID_MAX_NUM) {
		for (i = 0; i < IQ_ID_MAX_NUM; i++) {
			iq_dbg_msg[i] = cmd;
		}
	} else {
		iq_dbg_msg[id] = cmd;
	}

	if (cmd == IQ_DBG_NONE) {
		iq_dbg_clr_wrn_msg();
	}
}

void iq_dbg_clr_dbg_mode(IQ_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= IQ_ID_MAX_NUM) {
		for (i = 0; i < IQ_ID_MAX_NUM; i++) {
			iq_dbg_msg[i] &= ~cmd;
		}
	} else {
		iq_dbg_msg[id] &= ~cmd;
	}
}

BOOL iq_dbg_check_wrn_msg(BOOL show_dbg_msg)
{
	BOOL rt = TRUE;

	if ((wrn_msg_cnt < MAX_CNT) && (!show_dbg_msg)) {
		rt = TRUE;
	} else {
		rt = FALSE;
	}
	wrn_msg_cnt++;

	return rt;
}

void iq_dbg_clr_wrn_msg(void)
{
	wrn_msg_cnt = 0;
}

UINT32 iq_dbg_get_wrn_msg(void)
{
	return wrn_msg_cnt;
}

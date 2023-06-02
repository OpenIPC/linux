#include "kwrap/type.h"
#include "isp_dbg.h"

#define MAX_CNT 20
static UINT32 isp_dbg_msg[ISP_ID_MAX_NUM] = {0};
static UINT32 isp_bypass_eng[ISP_ID_MAX_NUM] = {0};
static UINT32 err_msg_cnt;
static UINT32 wrn_msg_cnt;

UINT32 isp_dbg_get_dbg_mode(ISP_ID id)
{
	if (id >= ISP_ID_MAX_NUM) {
		return isp_dbg_msg[0];
	}

	return isp_dbg_msg[id];
}

void isp_dbg_set_dbg_mode(ISP_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= ISP_ID_MAX_NUM) {
		for (i = 0; i < ISP_ID_MAX_NUM; i++) {
			isp_dbg_msg[i] = cmd;
		}
	} else {
		isp_dbg_msg[id] = cmd;
	}

	if (cmd == ISP_DBG_NONE) {
		isp_dbg_clr_err_msg();
	}
}

UINT32 isp_dbg_get_bypass_eng(ISP_ID id)
{
	if (id >= ISP_ID_MAX_NUM) {
		return isp_bypass_eng[0];
	}

	return isp_bypass_eng[id];
}

void isp_dbg_set_bypass_eng(ISP_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= ISP_ID_MAX_NUM) {
		for (i = 0; i < ISP_ID_MAX_NUM; i++) {
			isp_bypass_eng[i] = cmd;
		}
	} else {
		isp_bypass_eng[id] = cmd;
	}
}


BOOL isp_dbg_check_err_msg(BOOL show_dbg_msg)
{
	BOOL rt = TRUE;

	if ((err_msg_cnt < MAX_CNT) && (!show_dbg_msg)) {
		rt = TRUE;
	} else {
		rt = FALSE;
	}
	err_msg_cnt++;

	return rt;
}

void isp_dbg_clr_err_msg(void)
{
	err_msg_cnt = 0;
}

UINT32 isp_dbg_get_err_msg(void)
{
	return err_msg_cnt;
}

BOOL isp_dbg_check_wrn_msg(BOOL show_dbg_msg)
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

void isp_dbg_clr_wrn_msg(void)
{
	wrn_msg_cnt = 0;
}

UINT32 isp_dbg_get_wrn_msg(void)
{
	return wrn_msg_cnt;
}


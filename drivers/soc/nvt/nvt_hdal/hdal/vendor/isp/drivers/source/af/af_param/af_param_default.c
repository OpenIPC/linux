#include "kwrap/type.h"
#include "af_alg.h"

static BOOL af_func_en[AF_ID_MAX_NUM] = {
	TRUE,
#if AF_2ND_ID_PARAM
	FALSE
#endif
};

static AF_ALG_METHOD af_alg_method[AF_ID_MAX_NUM] = {
	AF_ALG_HILL_CLIMB,
#if AF_2ND_ID_PARAM
	AF_ALG_HILL_CLIMB,
#endif
};

static AF_SHOT_MODE af_shot_mode[AF_ID_MAX_NUM] = {
	AF_MODE_SINGLE,
#if AF_2ND_ID_PARAM
	AF_MODE_SINGLE,
#endif
};

static AF_SEARCH_DIR af_search_dir[AF_ID_MAX_NUM] = {
	AF_DIR_2TELE,
#if AF_2ND_ID_PARAM
	AF_DIR_2TELE,
#endif
};

static UINT32 af_skip_frame[AF_ID_MAX_NUM] = {
	1,
#if AF_2ND_ID_PARAM
	1,
#endif
};

static AF_THRES af_thres[AF_ID_MAX_NUM] = {
	{ 20, 80, 95, 60 },
#if AF_2ND_ID_PARAM
	{ 20, 80, 95, 60 },
#endif
};

static AF_STEP_SIZE af_step_size[AF_ID_MAX_NUM] = {
	{ 6, 3, 1 },
#if AF_2ND_ID_PARAM
	{ 6, 3, 1 },
#endif
};

static UINT32 af_max_count[AF_ID_MAX_NUM] = {
	200,
#if AF_2ND_ID_PARAM
	200,
#endif
};

static AF_WIN_WEIGHT af_win_wei[AF_ID_MAX_NUM] = {
	{
		.wei[0] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[1] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[2] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[3] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[4] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[5] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[6] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[7] = { 1, 1, 1, 1, 1, 1, 1, 1 },
	},
#if AF_2ND_ID_PARAM
	{
		.wei[0] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[1] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[2] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[3] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[4] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[5] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[6] = { 1, 1, 1, 1, 1, 1, 1, 1 },
		.wei[7] = { 1, 1, 1, 1, 1, 1, 1, 1 },
	},
#endif
};

static BOOL af_retrigger[AF_ID_MAX_NUM] = {
	FALSE,
#if AF_2ND_ID_PARAM
	FALSE,
#endif
};

static AF_DBG_MSG af_dbg_msg[AF_ID_MAX_NUM] = {
	AF_DBG_NONE,
#if AF_2ND_ID_PARAM
	AF_DBG_NONE,
#endif
};

static AF_PARAM af_param_default[AF_ID_MAX_NUM] = {
	{
		&af_func_en[0],
		&af_alg_method[0],
		&af_shot_mode[0],
		&af_search_dir[0],
		&af_skip_frame[0],
		&af_thres[0],
		&af_step_size[0],
		&af_max_count[0],
		&af_win_wei[0],
		&af_retrigger[0],
		&af_dbg_msg[0],
	},
#if AF_2ND_ID_PARAM
	{
		&af_func_en[1],
		&af_alg_method[1],
		&af_shot_mode[1],
		&af_search_dir[1],
		&af_skip_frame[1],
		&af_thres[1],
		&af_step_size[1],
		&af_max_count[1],
		&af_win_wei[1],
		&af_retrigger[1],
		&af_dbg_msg[1],
	},
#endif
};

void af_get_param_default(AF_ID id, UINT32 *param)
{
	*param = (UINT32)(&af_param_default[id]);
}


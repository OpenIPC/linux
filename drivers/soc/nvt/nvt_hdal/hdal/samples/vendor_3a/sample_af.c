#include "vendor_isp.h"

//=============================================================================
// function declaration
//=============================================================================
void sample_af_alg(unsigned int id, ISP_VA_RSLT *va);

//=============================================================================
// global
//=============================================================================
typedef enum _AF_SEARCH_STEP_ {
	STEP_MIN = 0,
	STEP_MAX,
	STEP_NUM
} AF_SEARCH_STEP;

typedef enum _AF_STATUS_ {
	AF_STATUS_RESET = 0,
	AF_STATUS_SEARCH,
	AF_STATUS_FINISHED,
	AF_STATUS_NUM
} AF_STATUS;

typedef struct _SAMPLE_AF_PARAM_ {
	unsigned int search_step[STEP_NUM];
	unsigned int motor_pos;
	unsigned int motor_pos_final;
	unsigned int focus_value_max;
	AF_STATUS status;
} SAMPLE_AF_PARAM;

static SAMPLE_AF_PARAM sample_af_param;

#define MOTOR_INIT_TIME		(33*10)

static UINT32 af_frm_cnt = 0;

void sample_af_init(unsigned int id)
{
	ISPT_MOTOR_FOCUS motor_focus = {0};
	
	motor_focus.cmd_type = MTR_GET_FOCUS_RANGE;
	vendor_isp_get_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
	sample_af_param.search_step[STEP_MIN] = motor_focus.ctl_cmd.data[0];
	sample_af_param.search_step[STEP_MAX] = motor_focus.ctl_cmd.data[1];
	sample_af_param.motor_pos = sample_af_param.search_step[STEP_MIN];
	sample_af_param.motor_pos_final = sample_af_param.motor_pos;
	sample_af_param.focus_value_max = 0;
	sample_af_param.status = AF_STATUS_RESET;	
	af_frm_cnt = 0;
	
	motor_focus.cmd_type = MTR_SET_FOCUS_INIT;
	vendor_isp_set_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
	
	//printf("search range ===== %d ~ %d\n\n", sample_af_param.search_step[STEP_MIN], sample_af_param.search_step[STEP_MAX]);
}

void sample_af_trig(unsigned int id)
{
	HD_RESULT result = HD_OK;
	unsigned int i;
	unsigned int focus_value;
	static ISPT_VA_DATA va_data = {0};
	ISPT_MOTOR_FOCUS motor_focus = {0};

	af_frm_cnt++;
	
	if(af_frm_cnt < MOTOR_INIT_TIME) {
		return;
	}

	motor_focus.cmd_type = MTR_GET_FOCUS_BUSY_STATUS;
	vendor_isp_get_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
	
	if(motor_focus.ctl_cmd.data[0] == 0x01) {
		printf("get focus busy status (%d)\n", motor_focus.ctl_cmd.data[0]);
		return;
	}

	if(sample_af_param.status == AF_STATUS_RESET) {
		motor_focus.cmd_type = MTR_SET_FOCUS_POSITION_IN_QUEUE;
		motor_focus.ctl_cmd.argu[0] = sample_af_param.motor_pos;
		vendor_isp_set_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
		sample_af_param.status = AF_STATUS_SEARCH;
		return;
	}

	if(sample_af_param.status == AF_STATUS_FINISHED) {
		return;
	}

	va_data.id = id;
	result = vendor_isp_get_common(ISPT_ITEM_VA_DATA, &va_data);

	if(result == HD_OK) {
		//printf("focus_value = %d\r\n", focus_value);
	} else {
		printf("Get VA fail, result = %d \n", result);
		return;
	}

	// get focus value
	focus_value = 0;
	for(i=0; i<ISP_VA_MAX_WINNUM; i++) {
		focus_value += (va_data.va_rslt.g1_h[i]+va_data.va_rslt.g1_v[i]+va_data.va_rslt.g2_h[i]+va_data.va_rslt.g2_v[i]);
	}

	if(focus_value > sample_af_param.focus_value_max) {
		sample_af_param.focus_value_max = focus_value;
		sample_af_param.motor_pos_final = sample_af_param.motor_pos;
	}

	if(sample_af_param.motor_pos < sample_af_param.search_step[STEP_MAX]) {
		sample_af_param.motor_pos += 4;
	} else {
		sample_af_param.status = AF_STATUS_FINISHED;
		sample_af_param.motor_pos = sample_af_param.motor_pos_final;
	}

	if(sample_af_param.status == AF_STATUS_FINISHED) {
		motor_focus.cmd_type = MTR_SET_FOCUS_POSITION_IN_QUEUE;
		motor_focus.ctl_cmd.argu[0] = sample_af_param.motor_pos;
		vendor_isp_set_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
	} else {
		motor_focus.cmd_type = MTR_SET_FOCUS_POSITION;
		motor_focus.ctl_cmd.argu[0] = sample_af_param.motor_pos;
		vendor_isp_set_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
	}
	

	//printf("status = %d, focus_value = {%8d, %8d}, motor_pos = {%3d, %3d}\n", sample_af_param.status, focus_value, sample_af_param.focus_value_max, sample_af_param.motor_pos, sample_af_param.motor_pos_final);
}




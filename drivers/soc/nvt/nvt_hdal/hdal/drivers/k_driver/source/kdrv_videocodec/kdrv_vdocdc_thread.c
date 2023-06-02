
#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_thread.h"

#include "h26x.h"
#include "h26x_common.h"
#include "h26xenc_api.h"
#include "h264enc_api.h"
#include "h264dec_api.h"
#include "h265enc_api.h"
#include "h265dec_api.h"
//#include "comm/ddr_arb.h"

static struct task_struct *run_task;
static struct task_struct *job_task;
static struct task_struct *int_task;

struct mutex mutex_run, mutex_busy, mutex_job, mutex_int;

BOOL do_job_thread, do_run_thread, do_int_thread;

unsigned int job_num[3]= {0};

KDRV_VDOCDC_LLC gst_llc = {0};

static int int_thread(void *usr_data)
{
	int i;

	while(!THREAD_SHOULD_STOP) {

		if (do_int_thread == true) {
			unsigned int interrupt;
			mutex_lock(&mutex_int);
			interrupt = h26x_waitINT();
			if (job_num[2] != 0) {
				for (i = 0; i < job_num[2]; i++) {
					UINT32 idx = (gst_llc.run_st_idx + i)%VDOCDC_JOB_MAX;
					if (gst_llc.codec_mode[idx] == VDOCDC_ENC_MODE) {
						H26XEncResultCfg sResult = {0};
						H26XENC_VAR *p_var = gst_llc.p_var[idx];
						KDRV_VDOENC_PARAM *p_param = gst_llc.p_param[idx];

						if (p_var->eCodecType == VCODEC_H264)
							h264Enc_getResult(p_var, 0, &sResult, interrupt);
						else
							h265Enc_getResult(p_var, 0, &sResult, interrupt);

						p_param->temproal_id = sResult.uiSvcLable;
						p_param->encode_err = ((interrupt & h26x_getIntEn()) != 0x00000001);
						p_param->re_encode_en = (interrupt != H26X_FINISH_INT);
						p_param->nxt_frm_type = sResult.ucNxtPicType;
						p_param->base_qp = sResult.ucQP;
						p_param->bs_size_1 = sResult.uiBSLen;
						p_param->frm_type = sResult.ucPicType;
						p_param->encode_time = sResult.uiHwEncTime;
						if (i == job_num[2]-1) {
							p_param->all_job_done = 1;
						} else {
							p_param->all_job_done = 0;
						}
						gst_llc.callback[idx]->callback(p_param, gst_llc.user_data[idx]);
					}
					else {
						H26XDEC_VAR *p_var = gst_llc.p_var[idx];
						KDRV_VDODEC_PARAM *p_param = gst_llc.p_param[idx];
						KDRV_VDODEC_REFFRMCB pCb = p_param->vRefFrmCb;

						gst_llc.callback[idx]->callback("decode", gst_llc.user_data[idx]);

						if (p_var->eCodecType == VCODEC_H264) {
							if ((h264Dec_getResYAddr(p_var) != 0) && interrupt == 0x00000001)
								pCb.VdoDec_RefFrmDo(gst_llc.id[idx], h264Dec_getResYAddr(p_var), FALSE);
						}
						else {
							if ((h265Dec_getResYAddr(p_var) != 0) && interrupt == 0x00000001)
								pCb.VdoDec_RefFrmDo(gst_llc.id[idx], h265Dec_getResYAddr(p_var), FALSE);
						}
					}
				}
				printk("thread get int:%08x\r\n", interrupt);
				job_num[2] = 0;
				mutex_unlock(&mutex_busy);
			}
		}
	}

	return 0;
}

static int run_thread(void *usr_data)
{

	while(!THREAD_SHOULD_STOP){
		if (do_run_thread == true) {
			if (job_num[1] != 0) {
				mutex_lock(&mutex_busy);
				h26x_setEncLLRegSet(job_num[1], gst_llc.pa[gst_llc.run_st_idx]);
				job_num[2] = job_num[1];
				job_num[1] = 0;
				h26x_start();
				mutex_unlock(&mutex_run);
			}
			else {
				mutex_lock(&mutex_run);
			}
		}
	}

	return 0;
}

static int job_thread(void *usr_data)
{

	while(!THREAD_SHOULD_STOP) {
		if (do_job_thread == true) {
			if (job_num[0] != 0) {
				if (job_num[1] != 0) {
					mutex_lock(&mutex_run);
				}
				gst_llc.run_st_idx = gst_llc.job_st_idx;
				job_num[1] = job_num[0];
				job_num[0] = 0;
				mutex_unlock(&mutex_run);
				mutex_unlock(&mutex_job);
			}
			else {
				mutex_lock(&mutex_job);
			}
		}
	}

	return 0;
}

int kdrv_vdocdc_create_thread(void)
{
	int ret;

	memset(&gst_llc, 0, sizeof(KDRV_VDOCDC_LLC));

	THREAD_CREATE(run_task, run_thread, NULL, "vdocdc_run");

	if (IS_ERR(run_task)) {
		DBG_ERR("run task create fail\r\n");
		ret = PTR_ERR(run_task);
		run_task = NULL;
		return ret;
	}

	THREAD_CREATE(job_task, job_thread, NULL, "vdocdc_job");

	if (IS_ERR(job_task)) {
		DBG_ERR("job task create fail\r\n");
		ret = PTR_ERR(job_task);
		run_task = NULL;
		job_task = NULL;
		return ret;
	}

	THREAD_CREATE(int_task, int_thread, NULL, "vdocdc_int");

	if (IS_ERR(int_task)) {
		DBG_ERR("job task create fail\r\n");
		ret = PTR_ERR(int_task);
		run_task = NULL;
		job_task = NULL;
		int_task = NULL;
		return ret;
	}

	mutex_init(&mutex_run);
	mutex_init(&mutex_busy);
	mutex_init(&mutex_job);
	mutex_init(&mutex_int);

	do_job_thread = true;
	do_run_thread = true;
	do_int_thread = true;

	job_num[0] = job_num[1] = job_num[2] = 0;

	wake_up_process(run_task);
	wake_up_process(job_task);
	wake_up_process(int_task);

	return 0;
}

int kdrv_vdocdc_remove_thread(void)
{
	do_job_thread = false;
	do_run_thread = false;
	do_int_thread = false;

	mutex_unlock(&mutex_run);
	mutex_unlock(&mutex_job);
	mutex_unlock(&mutex_busy);
	mutex_unlock(&mutex_int);

	kthread_stop(run_task);
	kthread_stop(job_task);
	kthread_stop(int_task);

	#if 0
	run_task = NULL;
	job_task = NULL;
	int_task = NULL;
	#endif

	return 0;
}

int kdrv_vdocdc_add_job(KDRV_VDOCDC_MODE codec_mode, UINT32 id, UINT32 uiVaApbAddr, KDRV_CALLBACK_FUNC *p_cb_func, void *user_data, void *p_var, void *p_param)
{
	int ret = 0;

	if (job_num[0] < VDOCDC_JOB_MAX) {
		if (job_num[0] != 0) mutex_lock(&mutex_job);
		if ((h26x_setLLCmd(id, uiVaApbAddr, gst_llc.va[gst_llc.cur_idx], gst_llc.pa[(gst_llc.cur_idx+1)%VDOCDC_JOB_MAX], gst_llc.size)) == 0) {
			if (job_num[0] == 0) gst_llc.job_st_idx = gst_llc.cur_idx;
			gst_llc.id[gst_llc.cur_idx] = id;
			gst_llc.codec_mode[gst_llc.cur_idx] = codec_mode;
			gst_llc.callback[gst_llc.cur_idx] = p_cb_func;
			gst_llc.user_data[gst_llc.cur_idx] = user_data;
			gst_llc.p_var[gst_llc.cur_idx] = p_var;
			gst_llc.p_param[gst_llc.cur_idx] = p_param;
			gst_llc.cur_idx = (gst_llc.cur_idx+1)%VDOCDC_JOB_MAX;
			job_num[0]++;
		}
		else {
			ret = -1;
		}
		mutex_unlock(&mutex_job);
	}
	else
		ret = -1;

	return ret;
}

void kdrv_vdocdc_get_interrupt(UINT32 interrupt)
{
	mutex_unlock(&mutex_int);
}

void kdrv_vdocdc_init_llc(void)
{
	h26x_llc_cb *llc_cb = h26x_llc_cb_init();

	llc_cb->get_interrupt = kdrv_vdocdc_get_interrupt;
}

#if 0
void kdrv_vdocdc_init_wp(void)
{
	h26x_wp_cb *wp_cb = h26x_wp_cb_init();

	wp_cb->get_h26x_wp = arb_enable_wp;
    wp_cb->disable_h26x_wp = arb_disable_wp;
}
#endif
int kdrv_vdocdc_set_llc_mem(UINT32 addr, UINT32 size)
{
	UINT32 llc_va, llc_pa;
	int i;

	if (size < ((h26x_getHwRegSize() + 64) * VDOCDC_JOB_MAX)) {
		DBG_ERR("set link-list command buffer size(%d) < requirement(%d)\r\n", (int)size, (h26x_getHwRegSize() + 64) * VDOCDC_JOB_MAX);
		return -1;
	}

	if (gst_llc.size != 0) {
		DBG_ERR("link-list command buffer set already\r\n");
		return -1;
	}

	llc_va = addr;
	llc_pa = h26x_getPhyAddr(addr);

	gst_llc.size = size;
	gst_llc.va[0] = llc_va;
	gst_llc.pa[0] = llc_pa;
	gst_llc.job_st_idx = 0;
	gst_llc.run_st_idx = 0;
	gst_llc.cur_idx = 0;

	for (i = 1; i < VDOCDC_JOB_MAX; i++) {
		gst_llc.va[i] = gst_llc.va[i-1] + h26x_getHwRegSize() + 64;
		gst_llc.pa[i] = gst_llc.pa[i-1] + h26x_getHwRegSize() + 64;
	}

	return 0;
}

UINT32 kdrv_vdocdc_get_llc_mem(void)
{
	return (gst_llc.size == 0) ? ((h26x_getHwRegSize() + 64) * VDOCDC_JOB_MAX) : 0;
}



#include "isp_dev.h"

//=============================================================================
// function declaration
//=============================================================================
int __init sample_awb_module_init(void);
void __exit sample_awb_module_exit(void);

//=============================================================================
// global
//=============================================================================
ISP_MODULE sample_awb_module;

#define CA_WIN_X    32
#define CA_WIN_Y    32

#if 0
static KDRV_SIE_CA_PARAM sie_ca_param = {
	.enable = TRUE,
	.win_num = {
		.w = 32,
		.h = 32, },
	.th_enable = TRUE,
	.g_th_l = 0,
	.g_th_u = 4095,
	.r_th_l = 0,
	.r_th_u = 4095,
	.b_th_l = 0,
	.b_th_u = 4095,
	.p_th_l = 0,
	.p_th_u = 4095,
	.irsub_r_weight = 0,
	.irsub_g_weight = 0,
	.irsub_b_weight = 0,
	.ca_ob_ofs = 0,
};
#endif

static void sample_awb_alg(unsigned int id, ISP_CA_RSLT *ca, UINT32 *r_gain, UINT32 *b_gain)
{
	UINT32 i, g2r, g2b;
	UINT32 sum_g2r = 0, sum_g2b = 0;
	UINT32 w_cnt = 0;

	for (i = 0; i < CA_WIN_X*CA_WIN_Y; i++) {
		if (ca->r[i] == 0 || ca->g[i] == 0 || ca->b[i] == 0) {
			continue;
		}

		g2r = (ca->g[i] << 8) / ca->r[i];
		g2b = (ca->g[i] << 8) / ca->b[i];
		sum_g2r += g2r;
		sum_g2b += g2b;
		w_cnt++;
	}

	if (w_cnt == 0) {
		sum_g2r = 256;
		sum_g2b = 256;
	} else {
		sum_g2r /= w_cnt;
		sum_g2b /= w_cnt;
	}

	*r_gain = sum_g2r;
	*b_gain = sum_g2b;
}

static void sample_awb_task_trig(unsigned int id, void *arg)
{
	UINT32 r_gain, b_gain;
	UINT32 cgain[3];
	static UINT32 frame_cnt = 0;
	ISP_CA_RSLT *ca_rslt = NULL;    //Get CA from isp device

	ca_rslt = isp_dev_get_ca(id);

	if (frame_cnt < 2) {
		frame_cnt++;
		return;
	} else {
		frame_cnt = 0;
	}

	if (ca_rslt == NULL) {
		r_gain = 512;
		b_gain = 512;
		printk("isp_dev_get_ca(id = %d) NULL!\n", id);
	} else {
		sample_awb_alg(id, ca_rslt, &r_gain, &b_gain);
	}

	// set wb gain to iq
	cgain[0] = r_gain;
	cgain[1] = 256;
	cgain[2] = b_gain;
	isp_dev_set_sync_item(id, ISP_SYNC_SEL_SIE, ISP_SYNC_AWB_CGAIN, &cgain);

	//printk("AWB(%d): WB gain %d %d \n", id, r_gain, b_gain);
}

static INT32 sample_awb_construct(void)
{
	sprintf(sample_awb_module.name, "SAMPLE_AWB");
	sample_awb_module.private   = NULL;
	sample_awb_module.fn_init   = NULL;
	sample_awb_module.fn_uninit = NULL;
	sample_awb_module.fn_trig   = sample_awb_task_trig;
	sample_awb_module.fn_pause  = NULL;
	sample_awb_module.fn_resume = NULL;
	sample_awb_module.fn_ioctl  = NULL;

	// register to isp
	isp_dev_reg_awb_module(&sample_awb_module);

	return 0;
}

static void sample_awb_deconstruct(void)
{
	memset(&sample_awb_module, 0x00, sizeof(ISP_MODULE));
}

static int __init sample_awb_init_module(void)
{
	printk("sample awb\n");

	sample_awb_construct();

	return 0;
}

static void __exit sample_awb_exit_module(void)
{
	sample_awb_deconstruct();
}

module_init(sample_awb_init_module);
module_exit(sample_awb_exit_module);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("sample awb module");
MODULE_LICENSE("GPL");


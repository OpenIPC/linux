
#include "isp_dev.h"

//=============================================================================
// function declaration
//=============================================================================
int __init sample_ae_module_init(void);
void __exit sample_ae_module_exit(void);

//=============================================================================
// global
//=============================================================================
ISP_MODULE sample_ae_module;

#define LA_WIN_X            32
#define LA_WIN_Y            32
#define AE_TARGET_Y         40
#define AE_CONVERGE_RANGE   10
#define AE_CONVRANGE_L      (AE_TARGET_Y - AE_CONVERGE_RANGE)
#define AE_CONVRANGE_H      (AE_TARGET_Y + AE_CONVERGE_RANGE)
#define AE_EXPTIME_L        100
#define AE_EXPTIME_H        33333
#define AE_EXPTIME_STEP     100
#define AE_GAIN_L           100
#define AE_GAIN_H           3200
#define AE_GAIN_STEP        10

static ISP_SENSOR_CTRL ae_expt_setting;
static ISP_SENSOR_CTRL ae_gain_setting;

#if 0
static KDRV_SIE_LA_PARAM sie_la_param = {
	.enable = TRUE,
	.win_num = {
		.w = 32,
		.h = 32, },
	.la_src = KDRV_SIE_LA_HIST_SRC_POST_GMA,
	.la_rgb2y1mod = STCS_LA_RGB,
	.la_rgb2y2mod = STCS_LA_RGB,
	.cg_enable = TRUE,
	.r_gain = 128,
	.g_gain = 128,
	.b_gain = 128,
	.gamma_enable = TRUE,
	.gamma_tbl = {
		0, 73, 139, 198, 241, 273, 304, 333, 360, 386,
		411, 433, 455, 475, 493, 510, 525, 539, 553, 566,
		580, 594, 607, 621, 634, 647, 659, 672, 684, 697,
		709, 721, 733, 744, 756, 767, 777, 787, 797, 806,
		816, 826, 835, 845, 854, 863, 872, 882, 891, 900,
		908, 917, 926, 934, 943, 951, 959, 968, 976, 984,
		992, 1000, 1007, 1015, 1023},
	.histogram_enable = TRUE,
	.histogram_src = KDRV_SIE_LA_HIST_SRC_POST_GMA,
	.irsub_r_weight = 0,
	.irsub_g_weight = 0,
	.irsub_b_weight = 0,
	.la_ob_ofs = 240,
	.lath_enable = TRUE,
	.lathy1lower = 0,
	.lathy1upper = 255,
	.lathy2lower = 0,
	.lathy2upper = 255,
};
#endif

static void sample_ae_task_trig(unsigned int id, void *arg)
{
	UINT32 ix, iy, lum, la_sum;
	static UINT32 exptime = AE_EXPTIME_L;
	static UINT32 gain = AE_GAIN_L;
	static UINT32 frame_cnt = 0;
	ISP_LA_RSLT *la_rslt = NULL;    // NOTE: Get LA from isp device

	la_rslt = isp_dev_get_la(id);

	if (frame_cnt < 2) {
		frame_cnt++;
		return;
	} else {
		frame_cnt = 0;
	}

	la_sum = 0;

	if (la_rslt == NULL) {
		la_sum = 0;
		printk("isp_dev_get_la(id = %d) NULL!\n", id);
	} else {
		for (iy = 0; iy < LA_WIN_Y; iy++) {
			for (ix = 0; ix < LA_WIN_X; ix++) {
				la_sum += (UINT32)la_rslt->lum_1[iy*LA_WIN_X+ix];
			}
		}
	}

	lum = (la_sum / (LA_WIN_X * LA_WIN_Y)) >> 4;    // 8 bit format

	if ((lum <= AE_CONVRANGE_H) && (lum >= AE_CONVRANGE_L)) {
		printk("AE stable.\r\n");
	} else {
		if (lum < AE_CONVRANGE_L) {
			exptime += AE_EXPTIME_STEP;

			if (exptime > AE_EXPTIME_H) {
				exptime = AE_EXPTIME_H;
				gain += AE_GAIN_STEP;
			}
		} else if (lum > AE_CONVRANGE_H) {
			if (gain > AE_GAIN_L) {
				gain -= AE_GAIN_L;
			} else {
				if (exptime > AE_EXPTIME_L) {
					exptime -= AE_EXPTIME_STEP;
				} else {
					exptime = AE_EXPTIME_L;
				}
			}
		}

		exptime = (exptime > AE_EXPTIME_H) ? AE_EXPTIME_H : ((exptime < AE_CONVRANGE_L) ? AE_CONVRANGE_L : exptime);
		gain = (gain > AE_GAIN_H) ? AE_GAIN_H : ((gain < AE_GAIN_L) ? AE_GAIN_L : gain);
	}

	ae_expt_setting.exp_time[0] = exptime;
	ae_gain_setting.gain_ratio[0] = gain;

	isp_api_set_expt(id, &ae_expt_setting);
	isp_api_set_gain(id, &ae_gain_setting);

	//printk("AE(%d): lum = %4d, exptime = %5d, gain = %3d\n", id, lum, exptime, gain);
}

static INT32 sample_ae_construct(void)
{
	sprintf(sample_ae_module.name, "SAMPLE_AE");
	sample_ae_module.private   = NULL;
	sample_ae_module.fn_init   = NULL;
	sample_ae_module.fn_uninit = NULL;
	sample_ae_module.fn_trig   = sample_ae_task_trig;
	sample_ae_module.fn_pause  = NULL;
	sample_ae_module.fn_resume = NULL;
	sample_ae_module.fn_ioctl  = NULL;

	// register to isp
	isp_dev_reg_ae_module(&sample_ae_module);

	return 0;
}

static void sample_ae_deconstruct(void)
{
	memset(&sample_ae_module, 0x00, sizeof(ISP_MODULE));
}

static int __init sample_ae_init_module(void)
{
	printk("sample ae\n");

	sample_ae_construct();

	return 0;
}

static void __exit sample_ae_exit_module(void)
{
	sample_ae_deconstruct();
}

module_init(sample_ae_init_module);
module_exit(sample_ae_exit_module);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("sample ae module");
MODULE_LICENSE("GPL");


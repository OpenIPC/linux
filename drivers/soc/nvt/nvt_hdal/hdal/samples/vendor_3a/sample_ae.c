#include "vendor_isp.h"

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// global
//=============================================================================
#define LA_WIN_X            32
#define LA_WIN_Y            32
#define AE_TARGET_Y         40
#define AE_CONVERGE_RANGE   10
#define AE_CONVRANGE_L      (AE_TARGET_Y - AE_CONVERGE_RANGE)
#define AE_CONVRANGE_H      (AE_TARGET_Y + AE_CONVERGE_RANGE)
#define AE_EXPTIME_DEFAULT  10000
#define AE_EXPTIME_L        500
#define AE_EXPTIME_H        30000
#define AE_EXPTIME_STEP     100
#define AE_GAIN_DEFAULT     1000
#define AE_GAIN_L           1000
#define AE_GAIN_H           4000
#define AE_GAIN_STEP        100

void sample_ae_init(unsigned int id)
{
	HD_RESULT result = HD_OK;
	ISPT_SENSOR_EXPT sensor_expt = {0};
	ISPT_SENSOR_GAIN sensor_gain = {0};

	sensor_expt.id = id;
	sensor_gain.id = id;
	sensor_expt.time[0] = AE_EXPTIME_DEFAULT;
	sensor_gain.ratio[0] = AE_GAIN_DEFAULT;

	result = vendor_isp_set_common(ISPT_ITEM_SENSOR_EXPT, &sensor_expt);
	if (result != HD_OK) {
		printf("SET ISPT_ITEM_SENSOR_EXPT fail, result = %d \n",result);
	}
	result = vendor_isp_set_common(ISPT_ITEM_SENSOR_GAIN, &sensor_gain);
	if (result != HD_OK) {
		printf("SET ISPT_ITEM_SENSOR_GAIN fail, result = %d \n",result);
	}
}

void sample_ae_trig(unsigned int id)
{
	// NOTE: Get LA from isp device
	HD_RESULT result = HD_OK;
	ISPT_SENSOR_EXPT sensor_expt = {0};
	ISPT_SENSOR_GAIN sensor_gain = {0};
	UINT32 ix, iy, iyw;
	UINT32 lum, la_sum;
	static UINT32 exptime = AE_EXPTIME_DEFAULT;
	static UINT32 gain = AE_GAIN_DEFAULT;
	ISPT_LA_DATA la_data = {0};
	static UINT32 frame_cnt = 0;

	la_data.id = id;
	result = vendor_isp_get_common(ISPT_ITEM_LA_DATA, &la_data);

	if(frame_cnt < 2) {
		frame_cnt ++;
		return;
	} else {
		frame_cnt = 0;
	}

	la_sum = 0;

	if (result == HD_OK) {
		//printf("id = %d, no. 528 data = %d, %d \n", la_data.id, la_data.la_rslt.lum_1[528], la_data.la_rslt.lum_2[528]);

		for (iy = 0; iy < LA_WIN_Y; iy ++) {
			iyw = iy * LA_WIN_X;
			for (ix = 0; ix < LA_WIN_X; ix ++) {
				la_sum += (UINT32)la_data.la_rslt.lum_1[iyw + ix];
			}
		}
	} else {
			la_sum = 0;
		printf("GET ISPT_ITEM_LA_DATA fail, result = %d \r\n",result);
	}

	lum = la_sum/(LA_WIN_X * LA_WIN_Y);
	lum = (lum >> 4);

	if((lum <= AE_CONVRANGE_H) && (lum >= AE_CONVRANGE_L)) {
		printf("AE stable.\r\n");
	} else {
		if(lum < AE_CONVRANGE_L) {
			exptime += AE_EXPTIME_STEP;
			if(exptime > AE_EXPTIME_H) {
				exptime = AE_EXPTIME_H;
				gain += AE_GAIN_STEP;
				if(gain > AE_GAIN_H) {
					gain = AE_GAIN_H;
				}
			}
		} else if(lum > AE_CONVRANGE_H){
			if(gain > AE_GAIN_L) {
				gain -= AE_GAIN_STEP;
			} else {
				if(exptime > AE_EXPTIME_L) {
					exptime -= AE_EXPTIME_STEP;
				} else {
					exptime = AE_EXPTIME_L;
				}
			}
		}
	}

	sensor_expt.id = id;
	sensor_gain.id = id;
	sensor_expt.time[0] = exptime;
	sensor_gain.ratio[0] = gain;

	result = vendor_isp_set_common(ISPT_ITEM_SENSOR_EXPT, &sensor_expt);
	if (result != HD_OK) {
		printf("SET ISPT_ITEM_SENSOR_EXPT fail, result = %d \n",result);
	}
	result = vendor_isp_set_common(ISPT_ITEM_SENSOR_GAIN, &sensor_gain);
	if (result != HD_OK) {
		printf("SET ISPT_ITEM_SENSOR_GAIN fail, result = %d \n",result);
	}
	printf("lum = %4d, exptime = %5d, gain = %3d\n", lum, sensor_expt.time[0], sensor_gain.ratio[0]);

}


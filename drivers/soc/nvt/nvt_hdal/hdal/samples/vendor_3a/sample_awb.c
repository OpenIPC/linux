#include "vendor_isp.h"

//=============================================================================
// function declaration
//=============================================================================
void sample_awb_alg(unsigned int id, ISP_CA_RSLT *ca, UINT32 *r_gain, UINT32 *b_gain);

//=============================================================================
// global
//=============================================================================

void sample_awb_init(unsigned int id)
{
	HD_RESULT result = HD_OK;
	ISPT_C_GAIN c_gain = {0};
	c_gain.id = id;
	c_gain.gain[0] = 512;
	c_gain.gain[1] = 256;
	c_gain.gain[2] = 512;
	result = vendor_isp_set_common(ISPT_ITEM_C_GAIN, &c_gain);
	if (result != HD_OK) {
		printf("set ISPT_ITEM_C_GAIN fail, result = %d \n",result);
	}

}

void sample_awb_trig(unsigned int id)
{
	//Get CA from isp device
	HD_RESULT result = HD_OK;
	static ISPT_CA_DATA ca_data = {0};
	ISPT_C_GAIN c_gain = {0};
	UINT32 r_gain;
	UINT32 b_gain;
	static UINT32 frame_cnt = 0;

	result = vendor_isp_get_common(ISPT_ITEM_CA_DATA, &ca_data);

	if(frame_cnt < 2) {
		frame_cnt ++;
		return;
	} else {
		frame_cnt = 0;
	}

	//AWB algorithm
	if (result == HD_OK) {
		//printf("id = %d, no. 528 data = %d, %d, %d \n", ca_data.id, ca_data.ca_rslt.r[528], ca_data.ca_rslt.g[528], ca_data.ca_rslt.b[528]);
		sample_awb_alg(id, &ca_data.ca_rslt, &r_gain, &b_gain);
	} else {
		printf("Get AWBT_ITEM_CA fail, result = %d \n",result);
		r_gain = 2047;
		b_gain = 2047;
	}

	//Set wb gain to iq
	c_gain.id = id;
	c_gain.gain[0] = r_gain;
	c_gain.gain[1] = 256;
	c_gain.gain[2] = b_gain;
	printf("sample_awb_trig, id = %d, WB gain %d %d %d \n", id, c_gain.gain[0], c_gain.gain[1], c_gain.gain[2]);

	result = vendor_isp_set_common(ISPT_ITEM_C_GAIN, &c_gain);
	if (result != HD_OK) {
		printf("set ISPT_ITEM_C_GAIN fail, result = %d \n",result);
	}
}

void sample_awb_alg(unsigned int id, ISP_CA_RSLT *ca, UINT32 *r_gain, UINT32 *b_gain)
{
	UINT32 i;
	UINT32 g2r, g2b;
	UINT32 sum_g2r = 0;
	UINT32 sum_g2b = 0;
	UINT32 w_cnt = 0;

	for (i = 0; i < ISP_CA_MAX_WINNUM; i++) {
		if (ca->r[i] == 0 || ca->g[i] == 0 || ca->b[i] == 0) {
			continue;
		}
		g2r = (ca->g[i] << 8) / ca->r[i];
		g2b = (ca->g[i] << 8) / ca->b[i];
		sum_g2r += g2r;
		sum_g2b += g2b;
		w_cnt ++;
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



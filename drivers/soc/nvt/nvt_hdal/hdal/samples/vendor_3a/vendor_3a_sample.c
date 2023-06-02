#include <string.h>
#include <stdio.h>

#include "hd_type.h"

#include <pthread.h> //for pthread API

#include "vendor_isp.h"
#include "sample_3a.h"

#define GETCHAR()				getchar()

//============================================================================
// global
//============================================================================
pthread_t sample_3a_thread_id;
UINT32 sample_3a_close = 1;

static void *sample_3a_thread(void *arg)
{
	UINT32 id = *(UINT32 *)arg;
	ISPT_WAIT_VD wait_vd = {0};

	wait_vd.id = id;
	wait_vd.timeout = 1000;//timeout (ms)

	while (sample_3a_close){
		vendor_isp_get_common(ISPT_ITEM_WAIT_VD, &wait_vd);
		sample_ae_trig(id);
		sample_awb_trig(id);
		sample_af_trig(id);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	HD_RESULT ret;
	UINT32 id = 0;
	INT key;

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	sample_ae_init(id);
	sample_awb_init(id);
	sample_af_init(id);

	// create sample 3a thread
	ret = pthread_create(&sample_3a_thread_id, NULL, sample_3a_thread, &id);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	// query user key
	printf("Enter q to exit \n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let sample_3a_thread stop loop and exit
			sample_3a_close = 0;
			break;
		}
		if(key == 'f') {
			sample_af_init(id);
		}
	}

	// destroy encode thread
	pthread_join(sample_3a_thread_id, NULL);

	exit:

	ret = vendor_isp_uninit();
	if(ret != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", ret);
	}

	return 0;
}


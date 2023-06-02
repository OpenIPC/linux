
#include <string.h>
#include <stdlib.h>

#include "vendor_isp.h"

#define MEASURE_TIME 0
#if (MEASURE_TIME)
#include "hd_common.h"
#include "hd_util.h"
#endif

//============================================================================
// global
//============================================================================
static int get_choose_int(void)
{
	CHAR buf[256];
	int val = 0, error;

	do {
		error = scanf("%d", &val);
		if (error != 1) {
			printf("Invalid option. Try again.\n");
			clearerr(stdin);
			fgets(buf, sizeof(buf), stdin);
			printf(">> ");
		}
	} while(error != 1);

	return val;
}

int main(int argc, char *argv[])
{
	#if (MEASURE_TIME)
	HD_RESULT ret;
	#endif
	INT rt;
	INT32 option;
	UINT32 trig = 1;
	UINT32 id, dbg_en;
	IQT_NIGHT_MODE night_mode = {0};
	AWBT_TH awb_parameter = {0};
	AWBT_SCENE_MODE awb_scene = {0};
	ISPT_IR_INFO ir_info = {0};
	IQT_RGBIR_PARAM rgbir = {0};

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------\n");
			printf(" 1  Set RGBIR setting \n");
			printf(" 0.  Quit\n");
			printf("----------------------------------------\n");
		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			do {
				printf("Set isp id (0, 1)>> \n");
				id = (UINT32)get_choose_int();
				ir_info.id = id;
				night_mode.id = id;
				awb_scene.id = id;
				rgbir.id = id;
			} while (0);
			do {
				printf("Set dbg en (0, 1)>> \n");
				dbg_en = (UINT32)get_choose_int();
			} while (0);

			while (1){
				rt = vendor_isp_get_common(ISPT_ITEM_IR_INFO, &ir_info);
				if (rt < 0) {
					printf("set ISP_IOC_GET_RGBIR_INFO fail!\n");
					goto _end;
				}
				if (dbg_en == 1) {
					printf("IR LEVEL=%d\n",ir_info.info.ir_level);
				}

				rt = vendor_isp_get_iq(IQT_ITEM_RGBIR_PARAM, &rgbir);
				if (rt < 0) {
					printf("get IQT_ITEM_RGBIR_PARAM fail!\n");
					goto _end;
				}
				if (ir_info.info.ir_level > rgbir.rgbir.auto_param.night_mode_th) {
					night_mode.mode = 1;
					awb_scene.mode = 10;
					rt = vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
					if (rt < 0) {
						printf("set IQT_ITEM_NIGHT_MODE fail!\n");
						goto _end;
					}
					rt = vendor_isp_set_awb(AWBT_ITEM_SCENE, &awb_scene);
					if (rt < 0) {
						printf("set AWBT_ITEM_SCENE fail!\n");
						goto _end;
					}
				} else if (ir_info.info.ir_level < 235) {
					night_mode.mode = 0;
					awb_scene.mode = 0;
					rt = vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
					if (rt < 0) {
						printf("set IQT_ITEM_NIGHT_MODE fail!\n");
						goto _end;
					}
					rt = vendor_isp_set_awb(AWBT_ITEM_SCENE, &awb_scene);
					if (rt < 0) {
						printf("set AWBT_ITEM_SCENE fail!\n");
						goto _end;
					}
				}

				if (ir_info.info.ir_level > 152) {
					rt = vendor_isp_get_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("get AWBT_ITEM_TH fail!\n");
						goto _end;
					}
					awb_parameter.th.rmb_u = 124;
					rt = vendor_isp_set_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("set AWBT_ITEM_TH fail!\n");
						goto _end;
					}
				} else {
					rt = vendor_isp_get_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("get AWBT_ITEM_TH fail!\n");
						goto _end;
					}
					awb_parameter.th.rmb_u = 62;
					rt = vendor_isp_set_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("set AWBT_ITEM_TH fail!\n");
						goto _end;
					}
				}

				usleep(1000*1000);
			}
			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}
	_end:

	#if (MEASURE_TIME)
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}
	#endif

	rt = vendor_isp_uninit();
	if(rt != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", rt);
	}

	return 0;
}


#include <string.h>

#include "vendor_isp.h"

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
	INT32 option;
	UINT32 trig = 1;
	UINT32 version = 0;
	HD_RESULT result = HD_OK;
	AFT_CFG_INFO cfg_info = {0};
	AFT_DTSI_INFO dtsi_info = {0};
	AFT_OPERATION operation = {0};
	AFT_ENABLE enable = {0};
	AFT_ALG_METHOD alg_method = {0};
	AFT_SHOT_MODE shot_mode = {0};
	AFT_SEARCH_DIR search_dir = {0};
	AFT_SKIP_FRAME skip_frame = {0};
	AFT_THRES thres = {0};
	AFT_STEP_SIZE step_size = {0};
	AFT_MAX_COUNT max_count = {0};
	AFT_WIN_WEIGHT win_weight = {0};
	AFT_VA_STA va_sta = {0};
	AFT_RETRIGGER retrigger = {0};

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------\n");
			printf("  1. Get version \n");
			printf("  2. Reload config file \n");
			printf("  3. Reload dtsi file \n");
			printf(" 10. Get operation \n");
			printf(" 31. Get enable \n");
			printf(" 32. Get alg method \n");
			printf(" 33. Get shot mode \n");
			printf(" 34. Get search dir \n");
			printf(" 35. Get skip frame \n");
			printf(" 36. Get thres \n");
			printf(" 37. Get step size \n");
			printf(" 38. Get max count \n");
			printf(" 39. Get win weight \n");
			printf(" 40. Get va sta \n");
			printf(" 60. Set operation \n");
			printf(" 91. Set retrigger \n");
			printf(" 0. Quit\n");
			printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			result = vendor_isp_get_af(AFT_ITEM_VERSION, &version);
			if (result == HD_OK) {
				printf("AF version = 0x%X \n", version);
			} else {
				printf("Get AFT_ITEM_VERSION fail! \n");
			}
			break;

		case 2:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfg_info.id = (UINT32)get_choose_int();
			} while (0);

			strncpy(cfg_info.path, "/mnt/app/isp/isp_imx291_0.cfg", CFG_NAME_LENGTH);

			result = vendor_isp_set_af(AFT_ITEM_RLD_CONFIG, &cfg_info);
			if (result != HD_OK) {
				printf("Set AFT_ITEM_RLD_CONFIG fail! \n");
			}
			break;

		case 3:
			do {
				printf("Set isp id (0, 1)>> \n");
				dtsi_info.id = (UINT32)get_choose_int();
			} while (0);

			strncpy(dtsi_info.node_path, "/isp/af/imx290_af_0", DTSI_NAME_LENGTH);
			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			result = vendor_isp_set_af(AFT_ITEM_RLD_DTSI, &dtsi_info);
			if (result != HD_OK) {
				printf("Set AFT_ITEM_RLD_DTSI fail! \n");
			}
			break;

		case 10:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_OPERATION, &operation);
			if (result == HD_OK) {
				printf("id = %d, operation = %d \n", operation.id, operation.operation);
			} else {
				printf("Get AFT_ITEM_OPERATION fail \n");
			}
			break;

		case 31:
			do {
				printf("Set isp id (0, 1)>> \n");
				enable.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_ENABLE, &enable);
			if (result == HD_OK) {
				printf("id = %d, enable = %d \n", enable.id, enable.func_en);
			} else {
				printf("Get AFT_ITEM_ENABLE fail! \n");
			}
			break;

		case 32:
			do {
				printf("Set isp id (0, 1)>> \n");
				alg_method.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_ALG_METHOD, &alg_method);
			if (result == HD_OK) {
				printf("id = %d, alg method = %d \n", alg_method.id, alg_method.alg_method);
			} else {
				printf("Get AFT_ITEM_ALG_METHOD fail! \n");
			}
			break;

		case 33:
			do {
				printf("Set isp id (0, 1)>> \n");
				shot_mode.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_SHOT_MODE, &shot_mode);
			if (result == HD_OK) {
				printf("id = %d, shot mode = %d \n", shot_mode.id, shot_mode.shot_mode);
			} else {
				printf("Get AFT_ITEM_SHOT_MODE fail! \n");
			}
			break;

		case 34:
			do {
				printf("Set isp id (0, 1)>> \n");
				search_dir.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_SEARCH_DIR, &search_dir);
			if (result == HD_OK) {
				printf("id = %d, shot mode = %d \n", search_dir.id, search_dir.search_dir);
			} else {
				printf("Get AFT_ITEM_SEARCH_DIR fail! \n");
			}
			break;

		case 35:
			do {
				printf("Set isp id (0, 1)>> \n");
				skip_frame.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_SKIP_FRAME, &skip_frame);
			if (result == HD_OK) {
				printf("id = %d, skip frame = %d \n", skip_frame.id, skip_frame.skip_frame);
			} else {
				printf("Get AFT_ITEM_SKIP_FRAME fail! \n");
			}
			break;

		case 36:
			do {
				printf("Set isp id (0, 1)>> \n");
				thres.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_THRES, &thres);
			if (result == HD_OK) {
				printf("id = %d, thres = %d, %d, %d, %d \n", thres.id, thres.thres.thres_rough, thres.thres.thres_fine, thres.thres.thres_final, thres.thres.thres_restart);
			} else {
				printf("Get AFT_ITEM_THRES fail! \n");
			}
			break;

		case 37:
			do {
				printf("Set isp id (0, 1)>> \n");
				step_size.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_STEP_SIZE, &step_size);
			if (result == HD_OK) {
				printf("id = %d, step size = %d, %d, %d \n", step_size.id, step_size.step_size.step_rough, step_size.step_size.step_fine, step_size.step_size.step_final);
			} else {
				printf("Get AFT_ITEM_STEP_SIZE fail! \n");
			}
			break;

		case 38:
			do {
				printf("Set isp id (0, 1)>> \n");
				max_count.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_MAX_COUNT, &max_count);
			if (result == HD_OK) {
				printf("id = %d, max count = %d \n", max_count.id, max_count.max_count);
			} else {
				printf("Get AFT_ITEM_MAX_COUNT fail! \n");
			}
			break;

		case 39:
			do {
				printf("Set isp id (0, 1)>> \n");
				win_weight.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_WIN_WEIGHT, &win_weight);
			if (result == HD_OK) {
				printf("id = %d, win weight(0,0) = %d \n", win_weight.id, win_weight.win_weight.wei[0][0]);
			} else {
				printf("Get AFT_ITEM_WIN_WEIGHT fail! \n");
			}
			break;

		case 40:
			do {
				printf("Set isp id (0, 1)>> \n");
				va_sta.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_af(AFT_ITEM_VA_STA, &va_sta);
			if (result == HD_OK) {
				printf("id = %d, va sta(0,0) = %d \n", va_sta.id, va_sta.va_sta.acc[0][0]);
			} else {
				printf("Get AFT_ITEM_VA_STA fail! \n");
			}
			break;

		case 60:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set operation (0 ~ %d)>> \n", AF_OPERATION_MAX_CNT-1);
				operation.operation = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_af(AFT_ITEM_OPERATION, &operation);
			if (result == HD_OK) {
				printf("id = %d, operation = %d \n", operation.id, operation.operation);
			} else {
				printf("Set AFT_ITEM_OPERATION fail! \n");
			}
			break;

		case 91:
			do {
				printf("Set isp id (0, 1)>> \n");
				retrigger.id = (UINT32)get_choose_int();
			} while (0);

			retrigger.retrigger = 1;
			result = vendor_isp_set_af(AFT_ITEM_RETRIGGER, &retrigger);
			if (result == HD_OK) {
				printf("id = %d retrigger \n", retrigger.id);
			} else {
				printf("Set AFT_ITEM_RETRIGGER fail! \n");
			}
			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}

	result = vendor_isp_uninit();
	if(result != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", result);
	}


	return 0;
}


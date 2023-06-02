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
	int option = 0;
	UINT32 i, j;
	UINT32 trig = 1;
	UINT32 tmp = 0;
	UINT32 version = 0;
	HD_RESULT result = HD_OK;
	AWBT_CFG_INFO cfg_info = {0};
	AWBT_DTSI_INFO dtsi_info = {0};
	AWBT_SCENE_MODE scene = {0};
	AWBT_WB_RATIO wb_ratio = {0};
	AWBT_OPERATION operation = {0};
	AWBT_CA_TH ca_th = {0};
	AWBT_TH th = {0};
	AWBT_LV lv = {0};
	AWBT_CT_WEIGHT ct_weight = {0};
	AWBT_TARGET target = {0};
	AWBT_CT_INFO ct_info = {0};
	AWBT_MWB_GAIN mwb = {0};
	AWBT_CONVERGE converge = {0};
	AWBT_EXPAND_BLOCK expand_blcok = {0};
	AWBT_MANUAL manual = {0};
	AWBT_STATUS status = {0};
	static AWBT_CA ca = {0};
	AWBT_FLAG flag = {0};
	AWBT_LUMA_WEIGHT luma_weight = {0};
	AWBT_KGAIN_RATIO kgain = {0};
	int ret = 0;

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------\n");
			printf("  1. Get version \n");
			printf("  2. Reload config file \n");
			printf("  3. Reload dtsi file \n");
			printf(" 10. Get scene mode \n");
			printf(" 11. Get wb ratio \n");
			printf(" 12. Get operation \n");
			printf(" 30. Get ca th \n");
			printf(" 31. Get th \n");
			printf(" 32. Get lv \n");
			printf(" 33. Get ct weight \n");
			printf(" 34. Get target \n");
			printf(" 35. Get ct info \n");
			printf(" 36. Get mwb \n");
			printf(" 37. Get converge \n");
			printf(" 38. Get expand_blcok \n");
			printf(" 39. Get manual \n");
			printf(" 40. Get status \n");
			printf(" 41. Get ca \n");
			printf(" 42. Get flag \n");
			printf(" 43. Get luma weight \n");
			printf(" 44. Get kgain ratio \n");
			printf(" 60. Set scene mode \n");
			printf(" 61. Set wb ratio \n");
			printf(" 62. Set operation \n");
			printf(" 74. Get kgain ratio \n");
			printf("  0. Quit\n");
			printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			result = vendor_isp_get_awb(AWBT_ITEM_VERSION, &version);
			if (result == HD_OK) {
				printf("version = 0x%X \n", version);
			} else {
				printf("Get AWBT_ITEM_VERSION fail \n");
			}
			break;

		case 2:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfg_info.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Select chg file>> \n");
				printf("1: isp_os02k10_0.cfg \n");
				printf("2: isp_ov2718_0.cfg \n");
				printf("3: isp_imx290_0.cfg \n");
				tmp = (UINT32)get_choose_int();
			} while (0);

			switch (tmp) {
			case 1:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
				break;

			case 2:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ov2718_0.cfg", CFG_NAME_LENGTH);
				break;

			case 3:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
				break;

			default:
				printf("Not support item (%d) \n", tmp);
				break;
			}
			result = vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
			if (result != HD_OK) {
				printf("Set AWBT_ITEM_RLD_CONFIG fail \n");
			}
			break;

		case 3:
			do {
				printf("Set isp id (0, 1)>> \n");
				dtsi_info.id = (UINT32)get_choose_int();
			} while (0);

			strncpy(dtsi_info.node_path, "/isp/awb/imx290_awb_0", DTSI_NAME_LENGTH);
			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			result = vendor_isp_set_awb(AWBT_ITEM_RLD_DTSI, &dtsi_info);
			if (result != HD_OK) {
				printf("Set AWBT_ITEM_RLD_DTSI fail \n");
			}
			break;

		case 10:
			do {
				printf("Set isp id (0, 1)>> \n");
				scene.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_SCENE, &scene);
			if (result == HD_OK) {
				printf("id = %d, scene mode = %d \n", scene.id, scene.mode);
			} else {
				printf("Get AWBT_ITEM_SCENE fail \n");
			}
			break;

		case 11:
			do {
				printf("Set isp id (0, 1)>> \n");
				wb_ratio.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_WB_RATIO, &wb_ratio);
			if (result == HD_OK) {
				printf("id = %d, wb R ratio = %d, wb B ratio = %d  \n", wb_ratio.id, wb_ratio.r, wb_ratio.b);
			} else {
				printf("Get AWBT_ITEM_WB_RATIO fail \n");
			}
			break;

		case 12:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_OPERATION, &operation);
			if (result == HD_OK) {
				printf("id = %d, operation = %d \n", operation.id, operation.operation);
			} else {
				printf("Get AWBT_ITEM_OPERATION fail \n");
			}
			break;

		case 30:
			do {
				printf("Set isp id (0, 1)>> \n");
				ca_th.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CA_TH, &ca_th);
			if (result == HD_OK) {
				printf("id = %d, en = %d, mode = %d  , g(%d,%d), r(%d,%d), b(%d,%d), p(%d,%d) \n", ca_th.id, ca_th.ca_th.enable, ca_th.ca_th.mode,
			ca_th.ca_th.g_l, ca_th.ca_th.g_u, ca_th.ca_th.r_l, ca_th.ca_th.r_u, ca_th.ca_th.b_l, ca_th.ca_th.b_u, ca_th.ca_th.p_l, ca_th.ca_th.p_u);
			} else {
				printf("Get AWBT_ITEM_CA_TH fail \n");
			}
			break;

		case 31:
			do {
				printf("Set isp id (0, 1)>> \n");
				th.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_TH, &th);
			if (result == HD_OK) {
				printf("id = %d \n", th.id);
			} else {
				printf("Get AWBT_ITEM_TH fail \n");
			}
			break;

		case 32:
			do {
				printf("Set isp id (0, 1)>> \n");
				lv.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_LV, &lv);
			if (result == HD_OK) {
				printf("id = %d \n", lv.id);
			} else {
				printf("Get AWBT_ITEM_LV fail \n");
			}
			break;

		case 33:
			do {
				printf("Set isp id (0, 1)>> \n");
				ct_weight.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CT_WEIGHT, &ct_weight);
			if (result == HD_OK) {
				printf("id = %d \n", ct_weight.id);
			} else {
				printf("Get AWBT_ITEM_CT_WEIGHT fail \n");
			}
			break;

		case 34:
			do {
				printf("Set isp id (0, 1)>> \n");
				target.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_TARGET, &target);
			if (result == HD_OK) {
				printf("id = %d \n", target.id);
			} else {
				printf("Get AWBT_ITEM_TARGET fail \n");
			}
			break;

		case 35:
			do {
				printf("Set isp id (0, 1)>> \n");
				ct_info.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CT_INFO, &ct_info);
			if (result == HD_OK) {
				printf("id = %d \n", ct_info.id);
			} else {
				printf("AWBT_ITEM_CT_INFO fail \n");
			}

			break;

		case 36:
			do {
				printf("Set isp id (0, 1)>> \n");
				mwb.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_MWB_GAIN, &mwb);
			if (result == HD_OK) {
				printf("id = %d \n", mwb.id);
			} else {
				printf("Get AWBT_ITEM_MWB_GAIN fail \n");
			}
			break;

		case 37:
			do {
				printf("Set isp id (0, 1)>> \n");
				converge.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CONVERGE, &converge);
			if (result == HD_OK) {
				printf("id = %d \n", converge.id);
			} else {
				printf("Get AWBT_ITEM_CONVERGE fail \n");
			}
			break;

		case 38:
			do {
				printf("Set isp id (0, 1)>> \n");
				expand_blcok.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_EXPAND_BLOCK, &expand_blcok);
			if (result == HD_OK) {
				printf("id = %d \n", expand_blcok.id);
			} else {
				printf("Get AWBT_ITEM_EXPAND_BLOCK fail \n");
			}
			break;

		case 39:
			do {
				printf("Set isp id (0, 1)>> \n");
				manual.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_MANUAL, &manual);
			if (result == HD_OK) {
				printf("id = %d \n", manual.id);
			} else {
				printf("Get AWBT_ITEM_MANUAL fail \n");
			}
			break;

		case 40:
			do {
				printf("Set isp id (0, 1)>> \n");
				status.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_STATUS, &status);
			if (result == HD_OK) {
				printf("id = %d \n", status.id);
			} else {
				printf("Get AWBT_ITEM_STATUS fail \n");
			}
			break;

		case 41:
			do {
				printf("Set isp id (0, 1)>> \n");
				ca.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CA, &ca);
			if (result == HD_OK) {
				printf("id = %d \n", ca.id);
				for (j = 0; j < ca.ca.win_num_y; j++) {
					printf("RGB  \t: ");
					for (i = 0; i < ca.ca.win_num_x; i++) {
						printf("%4d %4d %4d %4d\t", ca.ca.tab[AWBALG_CH_R][j][i], ca.ca.tab[AWBALG_CH_G][j][i], ca.ca.tab[AWBALG_CH_B][j][i], ca.ca.tab[AWBALG_ACC_CNT][j][i]);
					}
					printf("\r\n");
				}
			} else {
				printf("Get AWBT_ITEM_CA fail \n");
			}
			break;

		case 42:
			do {
				printf("Set isp id (0, 1)>> \n");
				flag.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_FLAG, &flag);
			if (result == HD_OK) {
				printf("id = %d \n", flag.id);
				for (j = 0; j < flag.flag.win_num_y; j++) {
					printf("flag \t: ");
					for (i = 0; i < flag.flag.win_num_x; i++) {
						printf("%4d\t", flag.flag.tab[j][i]);
					}
					printf("\r\n");
				}
			} else {
				printf("Get AWBT_ITEM_FLAG fail \n");
			}
			break;

		case 43:
			do {
				printf("Set isp id (0, 1)>> \n");
				luma_weight.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_LUMA_WEIGHT, &luma_weight);
			if (result == HD_OK) {
				printf("id = %d \n", luma_weight.id);
				printf("enable = %d \n", luma_weight.luma_weight.en);
				printf(" y, w\n");
				printf("%d %d\n", luma_weight.luma_weight.y[0], luma_weight.luma_weight.w[0]);
				printf("%d %d\n", luma_weight.luma_weight.y[1], luma_weight.luma_weight.w[1]);
				printf("%d %d\n", luma_weight.luma_weight.y[2], luma_weight.luma_weight.w[2]);
				printf("%d %d\n", luma_weight.luma_weight.y[3], luma_weight.luma_weight.w[3]);
				printf("%d %d\n", luma_weight.luma_weight.y[4], luma_weight.luma_weight.w[4]);
				printf("%d %d\n", luma_weight.luma_weight.y[5], luma_weight.luma_weight.w[5]);
				printf("%d %d\n", luma_weight.luma_weight.y[6], luma_weight.luma_weight.w[6]);
				printf("%d %d\n", luma_weight.luma_weight.y[7], luma_weight.luma_weight.w[7]);
			} else {
				printf("Get AWBT_ITEM_LUMA_WEIGHT fail \n");
			}
			break;

		case 44:
			do {
				printf("Set isp id (0, 1)>> \n");
				kgain.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_KGAIN_RATIO, &kgain);
			if (result == HD_OK) {
				printf("id = %d, Rratio = %d, Bratio = %d \n", kgain.id, kgain.k_gain.r_ratio, kgain.k_gain.b_ratio);
			} else {
				printf("Get AWBT_ITEM_LUMA_WEIGHT fail \n");
			}
			break;

		case 60:
			do {
				printf("Set isp id (0, 1)>> \n");
				scene.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set scene mode (0 ~ %d)>> \n", AWB_SCENE_MAX_CNT-1);
				scene.mode = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_SCENE, &scene);
			if (result == HD_OK) {
				printf("id = %d, scene mode = %d \n", scene.id, scene.mode);
			} else {
				printf("Set AWBT_ITEM_SCENE fail \n");
			}
			break;

		case 61:
			do {
				printf("Set isp id (0, 1)>> \n");
				wb_ratio.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set wb R ratio (1 ~ 400)>> \n");
				wb_ratio.r = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set wb B ratio (1 ~ 400)>> \n");
				wb_ratio.b = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_WB_RATIO, &wb_ratio);
			if (result == HD_OK) {
				printf("id = %d, wb R ratio = %d, wb B ratio = %d  n", wb_ratio.id, wb_ratio.r, wb_ratio.b);
			} else {
				printf("Set AWBT_ITEM_WB_RATIO fail \n");
			}
			break;

		case 62:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set operation (0 ~ %d)>> \n", AWB_OPERATION_MAX_CNT-1);
				operation.operation = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_OPERATION, &operation);
			if (result == HD_OK) {
				printf("id = %d, operation = %d \n", operation.id, operation.operation);
			} else {
				printf("Set AWBT_ITEM_OPERATION fail \n");
			}
			break;

		case 74:
			do {
				printf("Set isp id (0, 1)>> \n");
				kgain.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Rratio (50~150) >> \n");
				kgain.k_gain.r_ratio = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Bratio (50~150) >> \n");
				kgain.k_gain.b_ratio = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_KGAIN_RATIO, &kgain);
			if (result == HD_OK) {
				printf("id = %d, Rratio = %d, Bratio = %d \n", kgain.id, kgain.k_gain.r_ratio, kgain.k_gain.b_ratio);
			} else {
				printf("Get AWBT_ITEM_LUMA_WEIGHT fail \n");
			}
			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}

	ret = vendor_isp_uninit();
	if(ret != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", ret);
	}

	return 0;
}

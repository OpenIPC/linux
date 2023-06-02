
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "vendor_isp.h"

#define MEASURE_TIME 1
#if (MEASURE_TIME)
#include <sys/time.h>
#endif
#define WAIT_VD_PATH2 0

//============================================================================
// global
//============================================================================
pthread_t isp_md_thread_id;
static UINT32 md_conti_run = 1;
pthread_t isp_md_thread_id2;
static UINT32 md_conti_run2 = 1;
pthread_t isp_vd_thread_id;
static UINT32 vd_conti_run = 1;
#if (WAIT_VD_PATH2)
pthread_t isp_vd_thread_id2;
#endif

static INT32 get_choose_int(void)
{
	char buf[256];
	INT val, rt;

	rt = scanf("%d", &val);

	if (rt != 1) {
		printf("Invalid option. Try again.\n");
		clearerr(stdin);
		fgets(buf, sizeof(buf), stdin);
		val = -1;
	}

	return val;
}

static void *isp_md_thread(void *arg)
{
	ISPT_WAIT_VD wait_vd = {0};
	ISPT_MD_DATA md_data = {0};
	UINT32 i, j;

	while (md_conti_run) {
		wait_vd.id = 0;
		wait_vd.timeout = 100;

		for (i = 0; i < 30; i++) {
			vendor_isp_get_common(ISPT_ITEM_WAIT_VD, &wait_vd);
		}

		md_data.id = 0;
		vendor_isp_get_common(ISPT_ITEM_MD_DATA, &md_data);
		printf("md_data, blk_dif_cnt = %d, total_blk_diff = %d \n", md_data.md_rslt.blk_dif_cnt, md_data.md_rslt.total_blk_diff);
		for (i = 0; i < ISP_LA_H_WINNUM; i++) {
			printf("%2d | ", i);
			for (j = 0; j < ISP_LA_W_WINNUM; j++) {
				if (md_data.md_rslt.th[i*ISP_LA_W_WINNUM + j] == 1) {
					printf("\033[1;32;40m %d\033[0m", md_data.md_rslt.th[i*ISP_LA_H_WINNUM + j]);
				} else {
					printf("\033[0m %d\033[0m", md_data.md_rslt.th[i*ISP_LA_H_WINNUM + j]);
				}
			}
			printf("\n");
		}
	};

	return 0;
}

static void *isp_md_thread2(void *arg)
{
	ISPT_WAIT_VD wait_vd = {0};
	ISPT_MD_STA_INFO md_sta = {0};
	UINT32 i;

	while (md_conti_run2) {
		wait_vd.id = 0;
		wait_vd.timeout = 100;

		for (i = 0; i < 30; i++) {
			vendor_isp_get_common(ISPT_ITEM_WAIT_VD, &wait_vd);
		}

		md_sta.id = 0;
		vendor_isp_get_common(ISPT_ITEM_MD_STA, &md_sta);

		if (md_sta.md_sta_info.vaild) {
			printf("\r\n");
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[0],  md_sta.md_sta_info.data[1],  md_sta.md_sta_info.data[2],  md_sta.md_sta_info.data[3],  md_sta.md_sta_info.data[4],  md_sta.md_sta_info.data[5],  md_sta.md_sta_info.data[6],  md_sta.md_sta_info.data[7]);
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[8],  md_sta.md_sta_info.data[9],  md_sta.md_sta_info.data[10], md_sta.md_sta_info.data[11], md_sta.md_sta_info.data[12], md_sta.md_sta_info.data[13], md_sta.md_sta_info.data[14], md_sta.md_sta_info.data[15]);
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[16], md_sta.md_sta_info.data[17], md_sta.md_sta_info.data[18], md_sta.md_sta_info.data[19], md_sta.md_sta_info.data[20], md_sta.md_sta_info.data[21], md_sta.md_sta_info.data[22], md_sta.md_sta_info.data[23]);
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[24], md_sta.md_sta_info.data[25], md_sta.md_sta_info.data[26], md_sta.md_sta_info.data[27], md_sta.md_sta_info.data[28], md_sta.md_sta_info.data[29], md_sta.md_sta_info.data[30], md_sta.md_sta_info.data[31]);
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[32], md_sta.md_sta_info.data[33], md_sta.md_sta_info.data[34], md_sta.md_sta_info.data[35], md_sta.md_sta_info.data[36], md_sta.md_sta_info.data[37], md_sta.md_sta_info.data[38], md_sta.md_sta_info.data[39]);
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[40], md_sta.md_sta_info.data[41], md_sta.md_sta_info.data[42], md_sta.md_sta_info.data[43], md_sta.md_sta_info.data[44], md_sta.md_sta_info.data[45], md_sta.md_sta_info.data[46], md_sta.md_sta_info.data[47]);
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[48], md_sta.md_sta_info.data[49], md_sta.md_sta_info.data[50], md_sta.md_sta_info.data[51], md_sta.md_sta_info.data[52], md_sta.md_sta_info.data[53], md_sta.md_sta_info.data[54], md_sta.md_sta_info.data[55]);
			printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \r\n", md_sta.md_sta_info.data[56], md_sta.md_sta_info.data[57], md_sta.md_sta_info.data[58], md_sta.md_sta_info.data[59], md_sta.md_sta_info.data[60], md_sta.md_sta_info.data[61], md_sta.md_sta_info.data[62], md_sta.md_sta_info.data[63]);
		} else {
			printf("md data not valid. \r\n");
		}
	};

	return 0;
}

static void *isp_vd_thread(void *arg)
{
	ISPT_WAIT_VD *wait_vd = (ISPT_WAIT_VD *)arg;
	#if (MEASURE_TIME)
	UINT64 time_diff;
	struct timeval start, end;
	#endif

	while (vd_conti_run) {
		#if (MEASURE_TIME)
		gettimeofday(&start, NULL);
		#endif

		vendor_isp_get_common(ISPT_ITEM_WAIT_VD, wait_vd);

		#if (MEASURE_TIME)
		gettimeofday(&end, NULL);
		time_diff = 1000000 * (UINT64)(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
		printf("time =  %llu \n", time_diff);
		#endif
	};

	return 0;
}

#if (WAIT_VD_PATH2)
static void *isp_vd_thread2(void *arg)
{
	ISPT_WAIT_VD *wait_vd = (ISPT_WAIT_VD *)arg;
	#if (MEASURE_TIME)
	UINT64 time_diff;
	struct timeval start, end;
	#endif

	while (vd_conti_run) {
		#if (MEASURE_TIME)
		gettimeofday(&start, NULL);
		#endif

		vendor_isp_get_common(ISPT_ITEM_WAIT_VD, wait_vd);

		#if (MEASURE_TIME)
		gettimeofday(&end, NULL);
		time_diff = 1000000 * (UINT64)(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
		printf("time2 = %llu \n", time_diff);
		#endif
	};

	return 0;
}
#endif

int main(int argc, char *argv[])
{
	INT rt;
	INT32 option;
	UINT32 trig = 1;
	INT fz_pos = 0;
	UINT32 tmp = 0;
	UINT32 id;
	UINT32 i, j;
	UINT32 va_opt;
	UINT32 va_sum, va_sum_g1, va_sum_g2;
	CHAR char_tmp[80] = {0};
	UINT32 version = 0;
	ISPT_SENSOR_REG sensor_reg = {0};
	ISPT_SENSOR_INFO sensor_info = {0};
	ISPT_SENSOR_MODE_INFO sensor_mode_info = {0};
	ISPT_WAIT_VD wait_vd = {0};
	#if (WAIT_VD_PATH2)
	ISPT_WAIT_VD wait_vd2 = {0};
	#endif
	ISPT_FUNC func_info = {0};
	ISPT_YUV_INFO yuv_info = {0};
	ISPT_RAW_INFO raw_info = {0};
	ISPT_CHIP_INFO chip_info = {0};
	AET_CFG_INFO cfg_info = {0};
	static ISPT_CA_DATA ca_data = {0};
	ISPT_LA_DATA la_data = {0};
	ISPT_VA_DATA va_data = {0};
	ISPT_VA_INDEP_DATA va_indep_data = {0};
	ISPT_SENSOR_EXPT sensor_expt = {0};
	ISPT_SENSOR_GAIN sensor_gain = {0};
	ISPT_D_GAIN d_gain = {0};
	ISPT_C_GAIN c_gain = {0};
	ISPT_TOTAL_GAIN total_gain = {0};
	ISPT_LV lv = {0};
	ISPT_CT ct = {0};
	ISPT_MOTOR_IRIS motor_iris = {0};
	ISPT_MOTOR_FOCUS motor_focus = {0};
	ISPT_MOTOR_ZOOM motor_zoom = {0};
	ISPT_SENSOR_DIRECTION sensor_direction = {0};
	ISPT_HISTO_DATA histo_data = {0};
	ISPT_3DNR_STA_INFO _3dnr_sta_info = {0};
	IQT_MD_PARAM md_param = {0};

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------\n");
			printf("   1. Get version \n");
			printf("   2. Get isp function \n");
			printf("   3. Get YUV \n");
			printf("   4. Get RAW \n");
			printf("   7. Get sensor info \n");
			printf("   8. Get sensor register \n");
			printf("   9. Get sensor mode info \n");
			printf("  10. Get ca data \n");
			printf("  11. Get la data \n");
			printf("  12. Get va data \n");
			printf("  13. Get va_indep data \n");
			printf("  14. Get wait VD \n");
			printf("  15. Get chip info \n");
			printf("  20. Get motor iris \n");
			printf("  21. Get motor focus \n");
			printf("  22. Get motor zoom \n");
			printf("  23  Get sensor direction \n");
			printf("  24. Get histo data \n");
			printf("  25. Get 3dnr sta info \n");
			printf("  33. Get sensor expt \n");
			printf("  34. Get sensor gain \n");
			printf("  35. Get d gain \n");
			printf("  36. Get c gain \n");
			printf("  37. Get total gain \n");
			printf("  38. Get lv \n");
			printf("  39. Get ct \n");
			printf("  52. Reload cfg \n");
			printf("  53. Set YUV \n");
			printf("  54. Set RAW \n");
			printf("  55. Set sensor sleep \n");
			printf("  56. Set sensor wakeup \n");
			printf("  58. Set sensor register \n");
			printf("  60. Set motor iris \n");
			printf("  61. Set motor focus \n");
			printf("  62. Set motor zoom \n");
			printf("  63  Set sensor direction \n");
			printf("  83. Set sensor expt \n");
			printf("  84. Set sensor gain \n");
			printf("  85. Set d gain \n");
			printf("  86. Set c gain \n");
			printf("  87. Set total gain \n");
			printf("  88. Set lv \n");
			printf("  89. Set ct \n");
			printf("----------------------------------------\n");
			printf("- ONLY NT98528 -------------------------\n");
			printf("----------------------------------------\n");
			printf(" 200. Set md param \n");
			printf(" 201. Display md data \n");
			printf("----------------------------------------\n");
			printf(" 202. Get md sta \n");
			printf(" 0.  Quit\n");
			printf("----------------------------------------\n");
		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			vendor_isp_get_common(ISPT_ITEM_VERSION, &version);
			printf("version = 0x%X \n", version);
			break;

		case 2:
			do {
				printf("Set isp id (0, 1)>> \n");
				func_info.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_FUNC, &func_info);
			printf("id = %d, sie %d, ipp %d, ae %d, af %d, awb %d, defog %d, shdr %d, wdr %d \n", func_info.id, func_info.func_info.sie_valid, func_info.func_info.ipp_valid,
																												func_info.func_info.ae_valid, func_info.func_info.af_valid,
																												func_info.func_info.awb_valid, func_info.func_info.defog_valid,
																												func_info.func_info.shdr_valid, func_info.func_info.wdr_valid);
			for (tmp = 0; tmp < ISP_YUV_OUT_CH; tmp++) {
				printf(" %dx%d", func_info.func_info.yuv_out_ch[tmp].w, func_info.func_info.yuv_out_ch[tmp].h);
			}
			printf(" \n");
			break;

		case 3:
			do {
				printf("Set isp id (0, 1)>> \n");
				yuv_info.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set pid (0 ~ 5)>> \n");
				yuv_info.yuv_info.pid = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_YUV, &yuv_info);
			printf("get yuv, id = %d, pid = %d, %d x %d \n", yuv_info.id, yuv_info.yuv_info.pid, yuv_info.yuv_info.size.w, yuv_info.yuv_info.size.h);
			break;

		case 4:
			do {
				printf("Set isp id (0, 1)>> \n");
				raw_info.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_RAW, &raw_info);
			printf("get raw, id = %d, %d x %d \n", raw_info.id, raw_info.raw_info.size.w, raw_info.raw_info.size.h);
			break;

		case 7:
			vendor_isp_get_common(ISPT_ITEM_SENSOR_INFO, &sensor_info);
			printf("sendor name %s, %s \n", sensor_info.sensor_info.name[0], sensor_info.sensor_info.name[1]);
			printf("id mask %d, %d \n", sensor_info.sensor_info.src_id_mask[0], sensor_info.sensor_info.src_id_mask[1]);
			break;

		case 8:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_reg.id = (UINT32)get_choose_int();
			} while (0);

			printf("Set addr (0x )>> \n");
			rt = scanf("%10s", char_tmp);
			if (rt != 1) {
				printf("read fail. \n");
				break;
			}
			sensor_reg.addr = strtol(char_tmp, NULL, 0);

			vendor_isp_get_common(ISPT_ITEM_SENSOR_REG, &sensor_reg);
			printf("id = %d, addr = 0x%X, data = 0x%X \n", sensor_reg.id, sensor_reg.addr, sensor_reg.data);
			break;

		case 9:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_mode_info.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_SENSOR_MODE_INFO, &sensor_mode_info);
			printf("id = %d, gain: %d %d, act: %d %d %d %d, crop: %d %d \n", sensor_mode_info.id, sensor_mode_info.info.min_gain, sensor_mode_info.info.max_gain,
																			sensor_mode_info.info.act_size[0].x, sensor_mode_info.info.act_size[0].y, sensor_mode_info.info.act_size[0].w, sensor_mode_info.info.act_size[0].h,
																			sensor_mode_info.info.crp_size.w, sensor_mode_info.info.crp_size.h);
			break;

		case 10:
			do {
				printf("Set isp id (0, 1)>> \n");
				ca_data.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_CA_DATA, &ca_data);
			printf("id = %d, no. 528 data = %d, %d, %d \n", ca_data.id, ca_data.ca_rslt.r[528], ca_data.ca_rslt.g[528], ca_data.ca_rslt.b[528]);
			break;

		case 11:
			do {
				printf("Set isp id (0, 1)>> \n");
				la_data.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_LA_DATA, &la_data);
			printf("id = %d, no. 528 data = %d, %d \n", la_data.id, la_data.la_rslt.lum_1[528], la_data.la_rslt.lum_2[528]);
			break;

		case 12:
			do {
				printf("0. [g1h+g1v+g2h+g2v]\n");
				printf("1. [g1h+g1v, g2h+g2v]\n");
				printf("2. [g1h, g1v, g2h, g2v]\n");
				printf("Select display type >> \n");
				va_opt = (UINT32)get_choose_int();
			} while (0);
			vendor_isp_get_common(ISPT_ITEM_VA_DATA, &va_data);

			printf(" \n");
			va_sum_g1 = va_sum_g2 = va_sum = 0;

			if(va_opt == 0) {
				for (i = 0; i < 8; i++) {
					for (j = 0; j < 8; j++) {
						printf("%6d ", (va_data.va_rslt.g1_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g1_v[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g2_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g2_v[i*ISP_VA_W_WINNUM+j]));
						va_sum += (va_data.va_rslt.g1_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g1_v[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g2_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g2_v[i*ISP_VA_W_WINNUM+j]);
					}
					printf("\n");
				}		
				printf("va_sum = %d\n", va_sum);
			} else if(va_opt == 1) {
				for (i = 0; i < 8; i++) {
					for (j = 0; j < 8; j++) {
						printf("[%6d,%6d] ", (va_data.va_rslt.g1_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g1_v[i*ISP_VA_W_WINNUM+j]),(va_data.va_rslt.g2_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g2_v[i*ISP_VA_W_WINNUM+j]));
						va_sum_g1 += (va_data.va_rslt.g1_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g1_v[i*ISP_VA_W_WINNUM+j]);
						va_sum_g2 += (va_data.va_rslt.g2_h[i*ISP_VA_W_WINNUM+j]+va_data.va_rslt.g2_v[i*ISP_VA_W_WINNUM+j]);
					}
					printf("\n");
				}			
				printf("va_sum_g1 = %d\n", va_sum_g1);
				printf("va_sum_g2 = %d\n", va_sum_g2);
			} else {
				for (i = 0; i < 8; i++) {
					for (j = 0; j < 8; j++) {
						printf("[%6d,%6d,%6d,%6d] ", va_data.va_rslt.g1_h[i*ISP_VA_W_WINNUM+j],va_data.va_rslt.g1_v[i*ISP_VA_W_WINNUM+j],va_data.va_rslt.g2_h[i*ISP_VA_W_WINNUM+j],va_data.va_rslt.g2_v[i*ISP_VA_W_WINNUM+j]);
					}
					printf("\n");
				}			
			}
#if 0
			for (tmp = 0; tmp < (8*8); tmp++) {
				printf(" no. (%d) data = %d, %d, %d, %d \n", tmp, va_data.va_rslt.g1_h[tmp], va_data.va_rslt.g1_v[tmp], va_data.va_rslt.g2_h[tmp], va_data.va_rslt.g2_v[tmp]);
			}
#endif

			break;

		case 13:
			vendor_isp_get_common(ISPT_ITEM_VA_INDEP_DATA, &va_indep_data);

			printf(" \n");
			for (tmp = 0; tmp < 5; tmp++) {
				printf(" no. (%d) data = %d, %d, %d, %d \n", tmp, va_indep_data.va_indep_rslt.g1_h[tmp], va_indep_data.va_indep_rslt.g1_v[tmp], va_indep_data.va_indep_rslt.g2_h[tmp], va_indep_data.va_indep_rslt.g2_v[tmp]);
			}

			//printf(" no. 0 data = %d, %d \n", va_indep_data.va_indep_rslt.g1_h[0], va_indep_data.va_indep_rslt.g1_v[0]);
			break;

		case 14:
			do {
				printf("Set isp id (0, 1)>> \n");
				wait_vd.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set timeout (ms)>> \n");
				wait_vd.timeout = (UINT32)get_choose_int();
			} while (0);

			#if (WAIT_VD_PATH2)
			do {
				printf("Set isp id2 (0, 1)>> \n");
				wait_vd2.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set timeout2 (ms)>> \n");
				wait_vd2.timeout = (UINT32)get_choose_int();
			} while (0);
			#endif

			vd_conti_run = 1;
			if (pthread_create(&isp_vd_thread_id, NULL, isp_vd_thread, (void *)&wait_vd) < 0) {
				printf("create encode thread failed");
				break;
			}

			#if (WAIT_VD_PATH2)
			if (pthread_create(&isp_vd_thread_id2, NULL, isp_vd_thread2, (void *)&wait_vd2) < 0) {
				printf("create encode thread failed");
				break;
			}
			#endif

			do {
				printf("Enter 0 to exit >> \n");
				vd_conti_run = (UINT32)get_choose_int();
			} while (0);

			// destroy encode thread
			pthread_join(isp_vd_thread_id, NULL);
			#if (WAIT_VD_PATH2)
			pthread_join(isp_vd_thread_id2, NULL);
			#endif

			break;

		case 15:
			vendor_isp_get_common(ISPT_ITEM_CHIP_INFO, &chip_info);

			if (chip_info.info.id == ISP_CHIP_ID_NT9852X) {
				printf(" NT9852X \n");
			} else {
				printf(" NT98528 \n");
			}
			break;

		case 20:
			motor_iris.cmd_type = MTR_GET_APERTURE_POSITION;
			vendor_isp_get_common(ISPT_ITEM_MOTOR_IRIS, &motor_iris);
			printf("get iris, %d \n", motor_iris.ctl_cmd.data[0]);
			break;

		case 21:
			motor_focus.cmd_type = MTR_GET_FOCUS_POSITION;
			vendor_isp_get_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
			printf("get focus, %d \n", (motor_focus.ctl_cmd.data[0] & 0xFFFF));
			break;

		case 22:
			motor_zoom.cmd_type = MTR_GET_ZOOM_POSITION;
			vendor_isp_get_common(ISPT_ITEM_MOTOR_ZOOM, &motor_zoom);
			printf("get zoom, %d \n", (motor_zoom.ctl_cmd.data[0] & 0xFFFF));
			break;

		case 23:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_direction.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_SENSOR_DIRECTION, &sensor_direction);
			printf("mirror = %d, flip = %d \n", sensor_direction.direction.mirror, sensor_direction.direction.flip);
			break;

		case 24:
			do {
				printf("Set isp id (0, 1)>> \n");
				histo_data.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_HISTO_DATA, &histo_data);
			printf("id = %d,\n", histo_data.id);
			printf("pre data = ,,, %d, %d,,, %d\n", histo_data.histo_rslt.hist_stcs_pre_wdr[10], histo_data.histo_rslt.hist_stcs_pre_wdr[11], histo_data.histo_rslt.hist_stcs_pre_wdr[127]);
			printf("post data = ,,, %d, %d,,, %d\n", histo_data.histo_rslt.hist_stcs_post_wdr[10], histo_data.histo_rslt.hist_stcs_post_wdr[11], histo_data.histo_rslt.hist_stcs_post_wdr[127]);
			break;

		case 25:
			do {
				printf("Set isp id (0, 1)>> \n");
				_3dnr_sta_info.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_3DNR_STA, &_3dnr_sta_info);
			printf("id = %d \n", _3dnr_sta_info.id);
			printf("info_vaild = %d \n", _3dnr_sta_info._3dnr_sta_info.info_vaild);
			printf("enable = %d \n", _3dnr_sta_info._3dnr_sta_info.enable);
			printf("buf_addr = 0x%x \n", _3dnr_sta_info._3dnr_sta_info.buf_addr);
			break;

		case 33:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_expt.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_SENSOR_EXPT, &sensor_expt);
			printf("exposure time (us) = %d, %d \n", sensor_expt.time[0], sensor_expt.time[1]);
			break;

		case 34:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_gain.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_SENSOR_GAIN, &sensor_gain);
			printf("gain ratio (1000 = 1X) = %d, %d \n", sensor_gain.ratio[0], sensor_gain.ratio[1]);
			break;

		case 35:
			do {
				printf("Set isp id (0, 1)>> \n");
				d_gain.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_D_GAIN, &d_gain);
			printf("d gain(128 = 1X) = %d \n", d_gain.gain);
			break;

		case 36:
			do {
				printf("Set isp id (0, 1)>> \n");
				c_gain.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_C_GAIN, &c_gain);
			printf("c gain(256 = 1X) = %d, %d, %d \n", c_gain.gain[0], c_gain.gain[1], c_gain.gain[2]);
			break;

		case 37:
			do {
				printf("Set isp id (0, 1)>> \n");
				total_gain.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_TOTAL_GAIN, &total_gain);
			printf("gain ratio (100 = 1X) = %d \n", total_gain.gain);
			break;

		case 38:
			do {
				printf("Set isp id (0, 1)>> \n");
				lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_LV, &lv);
			printf("lv (100 = 1X) = %d \n", lv.lv);
			break;

		case 39:
			do {
				printf("Set isp id (0, 1)>> \n");
				ct.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_common(ISPT_ITEM_CT, &ct);
			printf("ct = %d \n", ct.ct);
			break;

		case 52:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfg_info.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Select chg file>> \n");
				printf("0: isp_os02k10_0.cfg \n");
				printf("1: isp_ar0237ir_0.cfg \n");
				tmp = (UINT32)get_choose_int();
			} while (0);

			switch (tmp) {
			case 0:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
				break;

			case 1:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ar0237ir_0.cfg", CFG_NAME_LENGTH);
				break;

			default:
				printf("Not support item (%d) \n", tmp);
				break;
			}
			rt = vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
			if (rt < 0) {
				printf("set AET_ITEM_RLD_CONFIG fail!\n");
				break;
			}
			rt = vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
			if (rt < 0) {
				printf("set AWBT_ITEM_RLD_CONFIG fail!\n");
				break;
			}
			rt = vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
			if (rt < 0) {
				printf("set IQT_ITEM_RLD_CONFIG fail!\n");
				break;
			}
			break;

		case 53:
			do {
				printf("Set isp id (0, 1)>> \n");
				id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_YUV, &id);
			printf("set yuv \n");
			break;

		case 54:
			do {
				printf("Set isp id (0, 1)>> \n");
				id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_RAW, &id);
			printf("set raw \n");
			break;

		case 55:
			do {
				printf("Set isp id (0, 1)>> \n");
				id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_SENSOR_SLEEP, &id);
			printf("set sleep \n");
			break;

		case 56:
			do {
				printf("Set isp id (0, 1)>> \n");
				id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_SENSOR_WAKEUP, &id);
			printf("set wakeup \n");
			break;

		case 58:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_reg.id = (UINT32)get_choose_int();
			} while (0);

			printf("Set addr (0x )>> \n");
			rt = scanf("%10s", char_tmp);
			if (rt != 1) {
				printf("read fail. \n");
				break;
			}
			sensor_reg.addr = strtol(char_tmp, NULL, 0);

			printf("Set data (0x )>> \n");
			rt = scanf("%10s", char_tmp);
			if (rt != 1) {
				printf("read fail. \n");
				break;
			}
			sensor_reg.data = strtol(char_tmp, NULL, 0);

			vendor_isp_set_common(ISPT_ITEM_SENSOR_REG, &sensor_reg);
			printf("id = %d, addr = 0x%X, data = 0x%X \n", sensor_reg.id, sensor_reg.addr, sensor_reg.data);
			break;

		case 60:
			motor_iris.cmd_type = MTR_SET_APERTURE_POSITION;
			motor_iris.ctl_cmd.argu[0] = 1;
			vendor_isp_set_common(ISPT_ITEM_MOTOR_IRIS, &motor_iris);
			printf("set iris, %d, %d \n", motor_iris.ctl_cmd.argu[0], motor_iris.ctl_cmd.data[0]);
			break;

		case 61:
			motor_focus.cmd_type = MTR_SET_FOCUS_POSITION_IN_QUEUE;

			printf("Set focus pos >> \n");
			rt = scanf("%d", &fz_pos);
			if (rt != 1) {
				printf("read fail. \n");
				break;
			}

			motor_focus.ctl_cmd.argu[0] = fz_pos;
			vendor_isp_set_common(ISPT_ITEM_MOTOR_FOCUS, &motor_focus);
			printf("set focus, %d, %d \n", motor_focus.ctl_cmd.argu[0], motor_focus.ctl_cmd.data[0]);
			break;

		case 62:
			motor_zoom.cmd_type = MTR_SET_ZOOM_POSITION;

			printf("Set zoom pos >> \n");
			rt = scanf("%d", &fz_pos);
			if (rt != 1) {
				printf("read fail. \n");
				break;
			}

			motor_zoom.ctl_cmd.argu[0] = fz_pos;
			vendor_isp_set_common(ISPT_ITEM_MOTOR_ZOOM, &motor_zoom);
			printf("set zoom, %d, %d \n", motor_zoom.ctl_cmd.argu[0], motor_zoom.ctl_cmd.data[0]);
			break;

		case 63:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_direction.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set mirror (0: disable, 1: enable)>> \n");
				sensor_direction.direction.mirror = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set flip (0: disable, 1: enable)>> \n");
				sensor_direction.direction.flip = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_SENSOR_DIRECTION, &sensor_direction);
			break;

		case 83:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_expt.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set exposure time (us)>> \n");
				sensor_expt.time[0] = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_SENSOR_EXPT, &sensor_expt);
			break;

		case 84:
			do {
				printf("Set isp id (0, 1)>> \n");
				sensor_gain.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set gain ratio (1000 = 1X)>> \n");
				sensor_gain.ratio[0] = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_SENSOR_GAIN, &sensor_gain);
			break;

		case 85:
			do {
				printf("Set isp id (0, 1)>> \n");
				d_gain.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set d gain(128 = 1X)>> \n");
				d_gain.gain = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_D_GAIN, &d_gain);
			break;

		case 86:
			do {
				printf("Set isp id (0, 1)>> \n");
				c_gain.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set r gain(256 = 1X)>> \n");
				c_gain.gain[0] = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set g gain(256 = 1X)>> \n");
				c_gain.gain[1] = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set b gain(256 = 1X)>> \n");
				c_gain.gain[2] = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_C_GAIN, &c_gain);
			break;

		case 87:
			do {
				printf("Set isp id (0, 1)>> \n");
				total_gain.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set total gain(100 = 1X)>> \n");
				total_gain.gain = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_TOTAL_GAIN, &total_gain);
			break;

		case 88:
			do {
				printf("Set isp id (0, 1)>> \n");
				lv.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set lv (100 = 1X)>> \n");
				lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_LV, &lv);
			break;

		case 89:
			do {
				printf("Set isp id (0, 1)>> \n");
				ct.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set ct>> \n");
				ct.ct = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_common(ISPT_ITEM_CT, &ct);
			break;

		case 200:
			md_param.id = 0;
			vendor_isp_get_iq(IQT_ITEM_MD_PARAM, &md_param);
			#if 0
			do {
				printf("Set md enable>> \n");
				md_param.md.enable = (UINT32)get_choose_int();
			} while (0);
			#if 0
			#endif
			do {
				printf("Set sum_frms>> \n");
				md_param.md.sum_frms = (UINT32)get_choose_int();
			} while (0);
			#endif
			do {
				printf("Set blkdiff_thr>> \n");
				md_param.md.blkdiff_thr = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set blkdiff_cnt_thr>> \n");
				md_param.md.blkdiff_cnt_thr = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Set total_blkdiff_thr>> \n");
				md_param.md.total_blkdiff_thr = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_MD_PARAM, &md_param);
			break;

		case 201:
			md_conti_run = 1;
			if (pthread_create(&isp_md_thread_id, NULL, isp_md_thread, NULL) < 0) {
				printf("create md thread failed");
				break;
			}

			do {
				printf("Enter 0 to exit >> \n");
				md_conti_run = (UINT32)get_choose_int();
			} while (0);

			// destroy encode thread
			pthread_join(isp_md_thread_id, NULL);

			break;

		case 202:
			md_conti_run2 = 1;
			if (pthread_create(&isp_md_thread_id2, NULL, isp_md_thread2, NULL) < 0) {
				printf("create md thread 2 failed");
				break;
			}

			do {
				printf("Enter 0 to exit >> \n");
				md_conti_run2 = (UINT32)get_choose_int();
			} while (0);

			// destroy encode thread
			pthread_join(isp_md_thread_id2, NULL);

			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}

	if (vendor_isp_uninit() == HD_ERR_NG) {
		return -1;
	}

	return 0;
}


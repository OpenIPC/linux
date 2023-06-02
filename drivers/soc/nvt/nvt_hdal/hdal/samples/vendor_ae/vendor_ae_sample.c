#include <string.h>

#include "vendor_isp.h"

//============================================================================
// global
//============================================================================
static INT32 get_choose_int(void)
{
	char buf[256];
	INT val, error;

	error = scanf("%d", &val);

	if (error != 1) {
		printf("Invalid option. Try again.\n");
		clearerr(stdin);
		fgets(buf, sizeof(buf), stdin);
		val = -1;
	}

	return val;
}

int main(int argc, char *argv[])
{
	INT32 option;
	UINT32 trig = 1;
	UINT32 version = 0;
	UINT32 ix, iy, iyw;
	AET_CFG_INFO cfg_info = {0};
	AET_DTSI_INFO dtsi_info = {0};
	AET_FREQUENCY_MODE frequency = {0};
	AET_METER_MODE meter = {0};
	AET_EV_OFFSET ev = {0};
	AET_ISO_VALUE iso = {0};
	AET_LONGEXP_MODE longexp = {0};
	AET_LONGEXP_EXPT_VALUE longexp_expt = {0};
	AET_LONGEXP_ISO_VALUE longexp_iso = {0};
	AET_OPERATION operation = {0};
	AET_PRIORITY priority = {0};
	AET_ROI_WIN roi_win = {0};
	AET_LA_WIN la_win = {0};
	AET_SMART_IR smart_ir = {0};
	AET_BASE_ISO base_iso = {0};
	AET_BASE_GAIN_RATIO base_gain_ratio = {0};
	AET_EXPECT_LUM expect_lum = {0};
	AET_LA_CLAMP la_clamp = {0};
	AET_OVER_EXPOSURE over_exposure = {0};
	AET_CONVERGENCE convergence = {0};
	AET_CURVE_GEN_MOVIE curve_gen_movie = {0};
	AET_METER_WINDOW meter_window = {0};
	AET_LUM_GAMMA lum_gamma = {0};
	AET_SHDR shdr = {0};
	AET_IRIS_CFG iris_cfg = {0};
	AET_CURVE_GEN_PHOTO curve_gen_photo = {0};
	AET_MANUAL manual = {0};
	AET_STATUS_INFO status = {0};
	AET_EXPT_BOUND expt_bound = {0};
	AET_GAIN_BOUND gain_bound = {0};
	AET_APERTURE_BOUND aperture_bound = {0};
	AET_STITCH_ID stitch_id = {0};

	//test
	int i=0;

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------\n");
			printf("  1. Get version \n");
			printf("  2. Reload config file \n");
			printf("  3. Reload dtsi file \n");
			printf(" 12. Get frequency mode \n");
			printf(" 13. Get meter mode \n");
			printf(" 14. Get ev offset \n");
			printf(" 15. Get iso value \n");
			printf(" 17. Get longexp mode \n");
			printf(" 18. Get longexp expt value \n");
			printf(" 19. Get longexp iso value \n");
			printf(" 20. Get operation \n");
			printf(" 30. Get base iso \n");
			printf(" 31. Get base gain ratio \n");
			printf(" 32. Get expect lum \n");
			printf(" 33. Get la clamp \n");
			printf(" 34. Get over exposure \n");
			printf(" 35. Get convergence \n");
			printf(" 36. Get curve gen movie \n");
			printf(" 37. Get meter window \n");
			printf(" 38. Get lum gamma \n");
			printf(" 39. Get shdr \n");
			printf(" 40. Get iris cfg \n");
			printf(" 41. Get curve gen photo \n");
			printf(" 42. Get manual \n");
			printf(" 43. Get status \n");
			printf(" 44. Get priority mode \n");
			printf(" 45. Get roi win rect \n");
			printf(" 46. Get smart ir weight \n");
			printf(" 47. Get exptime boundary \n");
			printf(" 48. Get gain boundary \n");
			printf(" 49. Get ae la win \n");
			printf(" 50. Get ae aperture bound \n");
			printf(" 51. Get stitch ID\n");
			printf(" 60. Set over exposure \n");
			printf(" 61. Set convergence \n");
			printf(" 62. Set frequency mode \n");
			printf(" 63. Set meter mode \n");
			printf(" 64. Set ev offset \n");
			printf(" 65. Set iso value \n");
			printf(" 67. Set longexp mode \n");
			printf(" 68. Set longexp expt value \n");
			printf(" 69. Set longexp iso value \n");
			printf(" 70. Set operation \n");
			printf(" 91. Set stitch ID \n");
			printf(" 92. Set ae aperture bound \n");
			printf(" 93. Set ae la win \n");
			printf(" 94. Set priority mode \n");
			printf(" 95. Set roi win rect \n");
			printf(" 96. Set smart ir weight \n");
			printf(" 97. Set exptime boundary \n");
			printf(" 98. Set gain boundary \n");
			printf(" 99. Set manual \n");
			printf(" 0.  Quit\n");
			printf("----------------------------------------\n");
		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			vendor_isp_get_ae(AET_ITEM_VERSION, &version);
			printf("version = 0x%X \n", version);
			break;

		case 2:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfg_info.id = (UINT32)get_choose_int();
			} while (0);

			strncpy(cfg_info.path, "/mnt/app/isp/isp_imx291_0.cfg", CFG_NAME_LENGTH);
			vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
			break;

		case 3:
			do {
				printf("Set isp id (0, 1)>> \n");
				dtsi_info.id = (UINT32)get_choose_int();
			} while (0);

			strncpy(dtsi_info.node_path, "/isp/ae/imx290_ae_0", DTSI_NAME_LENGTH);
			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			vendor_isp_set_ae(AET_ITEM_RLD_DTSI, &dtsi_info);
			break;

		case 12:
			do {
				printf("Set isp id (0, 1)>> \n");
				frequency.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_FREQUENCY, &frequency);
			printf("id = %d, frequency mode = %d \n", frequency.id, frequency.mode);
			break;

		case 13:
			do {
				printf("Set isp id (0, 1)>> \n");
				meter.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_METER, &meter);
			printf("id = %d, meter mode = %d \n", meter.id, meter.mode);
			break;

		case 14:
			do {
				printf("Set isp id (0, 1)>> \n");
				ev.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_EV, &ev);
			printf("id = %d, ev offset = %d \n", ev.id, ev.offset);
			break;

		case 15:
			do {
				printf("Set isp id (0, 1)>> \n");
				iso.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_ISO, &iso);
			printf("id = %d, iso value = %d \n", iso.id, iso.value);
			break;

		case 17:
			do {
				printf("Set isp id (0, 1)>> \n");
				longexp.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_LONGEXP, &longexp);
			printf("id = %d, longexp mode = %d \n", longexp.id, longexp.mode);
			break;

		case 18:
			do {
				printf("Set isp id (0, 1)>> \n");
				longexp_expt.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_LONGEXP_EXPT, &longexp_expt);
			printf("id = %d, longexp_expt value = %d \n", longexp_expt.id, longexp_expt.value);
			break;

		case 19:
			do {
				printf("Set isp id (0, 1)>> \n");
				longexp_iso.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_LONGEXP_ISO, &longexp_iso);
			printf("id = %d, longexp_iso value = %d \n", longexp_iso.id, longexp_iso.value);
			break;

		case 20:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_OPERATION, &operation);
			printf("id = %d, operation = %d \n", operation.id, operation.operation);
			break;

		case 30:
			do {
				printf("Set isp id (0, 1)>> \n");
				base_iso.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_BASE_ISO, &base_iso);
			printf("id = %d, base iso = %d \n", base_iso.id, base_iso.base_iso);
			break;

		case 31:
			do {
				printf("Set isp id (0, 1)>> \n");
				base_gain_ratio.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_BASE_GAIN_RATIO, &base_gain_ratio);
			printf("id = %d, base gain ratio = %d \n", base_gain_ratio.id, base_gain_ratio.base_gain_ratio);
			break;

		case 32:
			do {
				printf("Set isp id (0, 1)>> \n");
				expect_lum.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_EXPECT_LUM, &expect_lum);
			printf("id = %d, param = \n", expect_lum.id);
			printf("movie_mode = %d, photo_mode = %d \n", expect_lum.expect_lum.lum_mov, expect_lum.expect_lum.lum_photo);
			break;

		case 33:
			do {
				printf("Set isp id (0, 1)>> \n");
				la_clamp.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_LA_CLAMP, &la_clamp);
			printf("id = %d, \n", la_clamp.id);
			break;

		case 34:
			do {
				printf("Set isp id (0, 1)>> \n");
				over_exposure.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_OVER_EXPOSURE, &over_exposure);
			printf("id = %d, overexp_en = %d, overexp_lum = %d, overexposure_speed=%d \n", over_exposure.id, over_exposure.over_exposure.enable, over_exposure.over_exposure.lum, over_exposure.over_exposure.speed);
			break;

		case 35:
			do {
				printf("Set isp id (0, 1)>> \n");
				convergence.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_CONVERGENCE, &convergence);
			printf("id = %d, convergence_range = {%d, %d}\n", convergence.id, convergence.convergence.range_conv.l, convergence.convergence.range_conv.h);
			break;

		case 36:
			do {
				printf("Set isp id (0, 1)>> \n");
				curve_gen_movie.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_CURVE_GEN_MOVIE, &curve_gen_movie);
			curve_gen_movie.curve_gen_movie.flicker_mode =1;
			vendor_isp_set_ae(AET_ITEM_CURVE_GEN_MOVIE, &curve_gen_movie);
			printf("id = %d, flicker_mode=%d \n", curve_gen_movie.id, curve_gen_movie.curve_gen_movie.flicker_mode);
			break;

		case 37:
			do {
				printf("Set isp id (0, 1)>> \n");
				meter_window.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_METER_WIN, &meter_window);
			printf("id = %d, \n", meter_window.id);
			for(iy=0; iy<8; iy++) {
				iyw = iy*8;
				for(ix=0; ix<8; ix++) {
					printf("%d, ", meter_window.meter_window.matrix[iyw+ix]);
				}
				printf("\n");
			}
			break;

		case 38:
			do {
				printf("Set isp id (0, 1)>> \n");
				lum_gamma.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_LUM_GAMMA, &lum_gamma);
			printf("id = %d, \n", lum_gamma.id);
			break;

		case 39:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_SHDR, &shdr);
			printf("id = %d, \n", shdr.id);
			break;

		case 40:
			do {
				printf("Set isp id (0, 1)>> \n");
				iris_cfg.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_IRIS_CFG, &iris_cfg);
			printf("id = %d, \n", iris_cfg.id);
			break;

		case 41:
			do {
				printf("Set isp id (0, 1)>> \n");
				curve_gen_photo.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_CURVE_GEN_PHOTO, &curve_gen_photo);
			printf("id = %d, \n", curve_gen_photo.id);
			break;

		case 42:
			do {
				printf("Set isp id (0, 1)>> \n");
				manual.id = (UINT32)get_choose_int();
			} while (0);
			
			vendor_isp_get_ae(AET_ITEM_MANUAL, &manual);
			printf("id = %d, ae_mode = %d, manual_expt = %d, manual_gain = %d\n", manual.id, manual.manual.mode, manual.manual.expotime, manual.manual.iso_gain);
			break;

		case 43:
			do {
				printf("Set isp id (0, 1)>> \n");
				status.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_STATUS, &status);
			printf("id = %d, LV = %d \n", status.id, status.status_info.lv);
			break;

		case 44:
			do {
				printf("Set isp id (0, 1)>> \n");
				priority.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_PRIORITY, &priority);
			printf("id = %d, expt = %d, iso = %d \n", priority.id, priority.expt, priority.iso);
			break;

		case 45:
			do {
				printf("Set isp id (0, 1)>> \n");
				roi_win.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_ROI_WIN, &roi_win);
			printf("id = %d, start = {%d, %d}, end = {%d, %d}, roi_center_w=%d, roi_neighbor_w0=%d, roi_neighbor_w1=%d \n", roi_win.id, roi_win.roi_win.start_x, roi_win.roi_win.start_y, roi_win.roi_win.end_x, roi_win.roi_win.end_y, roi_win.roi_win.roi_center_w, roi_win.roi_win.roi_neighbor_w0, roi_win.roi_win.roi_neighbor_w1);
			break;

		case 46:
			do {
				printf("Set isp id (0, 1)>> \n");
				smart_ir.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_SMART_IR, &smart_ir);
			printf("id = %d, weight = %d, %d, %d  \n", smart_ir.id, smart_ir.weight.center, smart_ir.weight.around, smart_ir.weight.other);
			break;

		case 47:
			do {
				printf("Set isp id (0, 1)>> \n");
				expt_bound.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_EXPT_BOUND, &expt_bound);
			printf("id = %d, expt_bound = %d, %d \n", expt_bound.id, expt_bound.bound.l, expt_bound.bound.h);
			break;

		case 48:
			do {
				printf("Set isp id (0, 1)>> \n");
				gain_bound.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_GAIN_BOUND, &gain_bound);
			printf("id = %d, expt_bound = %d, %d \n", gain_bound.id, gain_bound.bound.l, gain_bound.bound.h);
			break;

		case 49:
			do {
				printf("Set isp id (0, 1)>> \n");
				la_win.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_LA_WIN, &la_win);
			printf("id = %d, la_matrix[22] = %d, la_matrix[100]=%d \n", la_win.id, la_win.la_win.la_matrix[22], la_win.la_win.la_matrix[100]);
			break;

		case 50:
			do {
				printf("Set isp id (0, 1)>> \n");
				aperture_bound.id = (UINT32)get_choose_int();
			} while (0);
		
			vendor_isp_get_ae(AET_ITEM_APERTURE_BOUND, &aperture_bound);
			printf("id = %d, aperture_bound.h = %d, aperture_bound.l=%d \n", aperture_bound.id, aperture_bound.aperture_bound.h, aperture_bound.aperture_bound.l);
			break;

		case 51:

			do {
				printf("Set isp id (0 ~ 7)>> \n");
				stitch_id.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_ae(AET_ITEM_STITCH_ID, &stitch_id);
			printf("id = %d, stitch_id = 0x%x\n", stitch_id.id, stitch_id.stitch_id);
			break;

		case 60:
			do {
				printf("Set isp id (0, 1)>> \n");
				over_exposure.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set enable>> \n");
				over_exposure.over_exposure.enable = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set overexposure speed>> \n");
				over_exposure.over_exposure.speed = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_OVER_EXPOSURE, &over_exposure);
			printf("id = %d, enable=%d, overexposure speed = %d \n", over_exposure.id, over_exposure.over_exposure.enable, over_exposure.over_exposure.speed);
			break;

		case 61:
			do {
				printf("Set isp id (0, 1)>> \n");
				convergence.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set range_conv_l>> \n");
				convergence.convergence.range_conv.l = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set range_conv_h>> \n");
				convergence.convergence.range_conv.h = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set slowshoot enable>> \n");
				convergence.convergence.slowshoot_en = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set stable_counter>> \n");
				convergence.convergence.stable_counter = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_CONVERGENCE, &convergence);
			printf("id = %d, range_conv.l=%d, range_conv.h=%d, slowshoot_en=%d, stable_counter=%d \n", convergence.id, convergence.convergence.range_conv.l, convergence.convergence.range_conv.h, convergence.convergence.slowshoot_en, convergence.convergence.stable_counter);
			break;

		case 62:
			do {
				printf("Set isp id (0, 1)>> \n");
				frequency.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set frequency mode (0 ~ %d)>> \n", AE_FREQUENCY_MAX_CNT-1);
				frequency.mode = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_FREQUENCY, &frequency);
			printf("id = %d, frequency mode = %d \n", frequency.id, frequency.mode);
			break;

		case 63:
			do {
				printf("Set isp id (0, 1)>> \n");
				meter.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set meter mode (0 ~ %d)>> \n", AE_METER_MAX_CNT-1);
				meter.mode = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_METER, &meter);
			printf("id = %d, meter mode = %d \n", meter.id, meter.mode);
			break;

		case 64:
			do {
				printf("Set isp id (0, 1)>> \n");
				ev.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ev offset (0 ~ %d)>> \n", AE_EV_MAX_CNT-1);
				ev.offset = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_EV, &ev);
			printf("id = %d, ev offset = %d \n", ev.id, ev.offset);
			break;

		case 65:
			do {
				printf("Set isp id (0, 1)>> \n");
				iso.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set iso value (0 ~ %d)>> \n", AE_ISO_MAX_CNT-1);
				iso.value = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_ISO, &iso);
			printf("id = %d, iso value = %d \n", iso.id, iso.value);
			break;

		case 67:
			do {
				printf("Set isp id (0, 1)>> \n");
				longexp.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set longexp mode (0 ~ %d)>> \n", AE_CAP_LONGEXP_MAX_CNT-1);
				longexp.mode = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_LONGEXP, &longexp);
			printf("id = %d, longexp mode = %d \n", longexp.id, longexp.mode);
			break;

		case 68:
			do {
				printf("Set isp id (0, 1)>> \n");
				longexp_expt.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set longexp_expt value (us)>> \n");
				longexp_expt.value = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_LONGEXP_EXPT, &longexp_expt);
			printf("id = %d, longexp_expt value = %d \n", longexp_expt.id, longexp_expt.value);
			break;

		case 69:
			do {
				printf("Set isp id (0, 1)>> \n");
				longexp_iso.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set longexp_iso value (gain ratio)>> \n");
				longexp_iso.value = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_LONGEXP_ISO, &longexp_iso);
			printf("id = %d, longexp_iso value = %d \n", longexp_iso.id, longexp_iso.value);
			break;

		case 70:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set operation (0 ~ %d)>> \n", AE_OPERATION_MAX_CNT-1);
				operation.operation = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_OPERATION, &operation);
			printf("id = %d, operation = %d \n", operation.id, operation.operation);
			break;

		case 91:
			do {
				printf("Set isp id (0 ~ 7)>> \n");
				stitch_id.id = (UINT32)get_choose_int();
			} while (0);
		
			do {
				printf("Set stitch id_left (0 ~ 7)>> \n");
				stitch_id.stitch_id |= (UINT32)get_choose_int() << 16;
			} while (0);
					
			do {
				printf("Set stitch id_right (0 ~ 7)>> \n");
				stitch_id.stitch_id |= (UINT32)get_choose_int();
			} while (0);
		
			vendor_isp_set_ae(AET_ITEM_STITCH_ID, &stitch_id);
			printf("id = %d, stitch_id = 0x%x\n", stitch_id.id, stitch_id.stitch_id);
			break;

		case 92:
			do {
				printf("Set isp id (0, 1)>> \n");
				aperture_bound.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set aperture_bound.h >> \n");
				aperture_bound.aperture_bound.h= (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set aperture_bound.l >> \n");
				aperture_bound.aperture_bound.l= (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_APERTURE_BOUND, &aperture_bound);
			printf("id = %d, aperture_bound.h = %d, aperture_bound.h= %d \n", aperture_bound.id, aperture_bound.aperture_bound.h,aperture_bound.aperture_bound.l);
			break;

		case 93:
			do {
				printf("Set isp id (0, 1)>> \n");
				la_win.id = (UINT32)get_choose_int();
			} while (0);

			for(i=0; i<LA_WIN_NUM;i++){
				la_win.la_win.la_matrix[i] = i;
			}

			vendor_isp_set_ae(AET_ITEM_LA_WIN, &la_win);
			printf("id = %d, la_matrix[22] = %d, la_matrix[1000]= %d \n", la_win.id, la_win.la_win.la_matrix[22],la_win.la_win.la_matrix[1000]);
			break;

		case 94:
			do {
				printf("Set isp id (0, 1)>> \n");
				priority.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set expt value (10000 = 10ms)>> \n");
				priority.expt = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set iso value (100 = 1x)>> \n");
				priority.iso = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_PRIORITY, &priority);
			printf("id = %d, expt = %d, iso = %d \n", priority.id, priority.expt, priority.iso);
			break;

		case 95:
			do {
				printf("Set isp id (0, 1)>> \n");
				roi_win.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set x_start (0~99)>> \n");
				roi_win.roi_win.start_x = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set y_start (0~99)>> \n");
				roi_win.roi_win.start_y = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set x_end (1~100)>> \n");
				roi_win.roi_win.end_x = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set y_end (1~100)>> \n");
				roi_win.roi_win.end_y = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set roi_center_w (0~1023)>> \n");
				roi_win.roi_win.roi_center_w = (UINT32)get_choose_int();
			} while (0);
			
			do {
				printf("Set roi_neighbor_w0 (0~1023)>> \n");
				roi_win.roi_win.roi_neighbor_w0 = (UINT32)get_choose_int();
			} while (0);
			
			do {
				printf("Set roi_neighbor_w1 (0~1023)>> \n");
				roi_win.roi_win.roi_neighbor_w1 = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_ROI_WIN, &roi_win);
			printf("id = %d, start = %d %d, end = %d %d \n", roi_win.id, roi_win.roi_win.start_x, roi_win.roi_win.start_y, roi_win.roi_win.end_x, roi_win.roi_win.end_y);
			break;

		case 96:
			do {
				printf("Set isp id (0, 1)>> \n");
				smart_ir.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set smart ir center weight >> \n");
				smart_ir.weight.center = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set smart ir around weight >> \n");
				smart_ir.weight.around = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set smart ir other weight >> \n");
				smart_ir.weight.other = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_SMART_IR, &smart_ir);
			printf("id = %d, weight = %d, %d, %d  \n", smart_ir.id, smart_ir.weight.center, smart_ir.weight.around, smart_ir.weight.other);
			break;

		case 97:
			do {
				printf("Set isp id (0, 1)>> \n");
				expt_bound.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set exptime boundary L >> \n");
				expt_bound.bound.l = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set exptime boundary H >> \n");
				expt_bound.bound.h = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_EXPT_BOUND, &expt_bound);
			printf("id = %d, exptime boundary = %d, %d \n", expt_bound.id, expt_bound.bound.l, expt_bound.bound.h);
			break;

		case 98:
			do {
				printf("Set isp id (0, 1)>> \n");
				gain_bound.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set gain boundary L >> \n");
				gain_bound.bound.l = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set gain boundary H >> \n");
				gain_bound.bound.h = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_ae(AET_ITEM_GAIN_BOUND, &gain_bound);
			printf("id = %d, gain boundary = %d, %d \n", gain_bound.id, gain_bound.bound.l, gain_bound.bound.h);
			break;

		case 99:
			do {
				printf("Set isp id (0, 1)>> \n");
				manual.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set AE_MODE (0: auto mode, 1: manual mode, 2: lock mode)>> \n");
				manual.manual.mode = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set manual exposure (0 for bypass)>> \n");
				manual.manual.expotime = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set manual gain (0 for bypass)>> \n");
				manual.manual.iso_gain = (UINT32)get_choose_int();
			} while (0);
			
			vendor_isp_set_ae(AET_ITEM_MANUAL, &manual);
			printf("id = %d, ae_mode = %d, manual_expt = %d, manual_gain = %d\n", manual.id, manual.manual.mode, manual.manual.expotime, manual.manual.iso_gain);
			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}

	vendor_isp_uninit();

	return 0;
}


#include <string.h>
#include <pthread.h>

#include "vendor_isp.h"

//============================================================================
// global
//============================================================================
pthread_t iq_tonelv_thread_id;
static UINT32 iq_tonelv_conti_run = 1;

BOOL tone_level_en = FALSE;
BOOL tone_level_dbg = FALSE;

static INT32 get_choose_int(void)
{
	CHAR buf[256];
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

static INT32 iq_intpl(INT32 index, INT32 l_value, INT32 h_value, INT32 l_index, INT32 h_index)
{
	INT32 range = h_index - l_index;

	if (l_value == h_value) {
		return l_value;
	} else if (index <= l_index) {
		return l_value;
	} else if (index >= h_index) {
		return h_value;
	}
	if (h_value < l_value) {
		return l_value + ((h_value - l_value) * (index  - l_index) - (range >> 1)) / range;
	} else {
		return l_value + ((h_value - l_value) * (index  - l_index) + (range >> 1)) / range;
	}
}

static void *iq_tonelv_thread(void *arg)
{
	AET_STATUS_INFO ae_status = {0};
	IQT_DR_LEVEL dr_level = {0};
	IQT_SHDR_TONE_LV shdr_tone_lv = {0};
	UINT32 tonelv_max_th_l_iso = 400, tonelv_max_th_l_value = 100;
	UINT32 tonelv_max_th_h_iso = 1600, tonelv_max_th_h_value = 50;
	UINT32 curr_tonelv_max;
	UINT32 tonelv_th_l_dr = 40, tonelv_th_l_value = 50;
	UINT32 tonelv_th_h_dr = 120, tonelv_th_h_value = 100;
	UINT32 smooth_factor = 7;

	static BOOL first_run = TRUE;
	static UINT32 smooth_dr_level = 50;

	UINT32 *id = (UINT32 *)arg;

	ae_status.id = *id;
	dr_level.id = *id;
	shdr_tone_lv.id = *id;

	if (tone_level_en == TRUE) {
		while (iq_tonelv_conti_run) {
			vendor_isp_get_ae(AET_ITEM_STATUS, &ae_status);
			curr_tonelv_max = iq_intpl(ae_status.status_info.iso_gain[1], tonelv_max_th_l_value, tonelv_max_th_h_value, tonelv_max_th_l_iso, tonelv_max_th_h_iso);

			vendor_isp_get_iq(IQT_ITEM_DR_LEVEL, &dr_level);
			if (first_run) {
				smooth_dr_level = dr_level.dr_level;
				first_run = FALSE;
			}
			if (smooth_dr_level <= dr_level.dr_level) {
				smooth_dr_level = (smooth_dr_level * smooth_factor + dr_level.dr_level * 1 + smooth_factor) / (smooth_factor + 1); // Unconditional carry
			} else {
				smooth_dr_level = (smooth_dr_level * smooth_factor + dr_level.dr_level * 1) / (smooth_factor + 1); // Unconditional chop
			}
			shdr_tone_lv.lv = iq_intpl(smooth_dr_level, tonelv_th_l_value, tonelv_th_h_value, tonelv_th_l_dr, tonelv_th_h_dr);
			if (shdr_tone_lv.lv > curr_tonelv_max) {
				shdr_tone_lv.lv = curr_tonelv_max;
			}

			vendor_isp_set_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
			if (tone_level_dbg == TRUE) {
				printf("dr_level smooth = %d (curr = %d) \n", smooth_dr_level, dr_level.dr_level);
				printf("shdr_tone level = %d (max = %d) \n", shdr_tone_lv.lv, curr_tonelv_max);
			}
			sleep(1);
		}
	} else {
		shdr_tone_lv.lv = 50;
		vendor_isp_set_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
		printf("shdr_tone id = %d \n", shdr_tone_lv.id);
		printf("shdr_tone level = %d \n", shdr_tone_lv.lv);
	}

	return 0;
}

static IQT_DPC_PARAM dpc = {0};
static IQT_SHADING_PARAM shading = {0};
static IQT_SHADING_EXT_PARAM shading_ext = {0};
static IQT_LDC_PARAM ldc = {0};
int main(int argc, char *argv[])
{
	INT32 option;
	UINT32 trig = 1;
	UINT32 tmp = 0;
	UINT32 version = 0;
	IQT_CFG_INFO cfg_info = {0};
	IQT_DTSI_INFO dtsi_info = {0};
	IQT_NR_LV nr_lv = {0};
	IQT_3DNR_LV nr3d_lv = {0};
	IQT_SHARPNESS_LV sharpness_lv = {0};
	IQT_SATURATION_LV saturation_lv = {0};
	IQT_CONTRAST_LV contrast_lv = {0};
	IQT_BRIGHTNESS_LV brightness_lv = {0};
	IQT_NIGHT_MODE night_mode = {0};
	IQT_YCC_FORMAT ycc_format = {0};
	IQT_OPERATION operation = {0};
	IQT_IMAGEEFFECT imageeffect = {0};
	IQT_CCID ccid = {0};
	IQT_HUE_SHIFT hue_shift = {0};
	IQT_SHDR_TONE_LV shdr_tone_lv = {0};
	IQT_SHADING_INTER_PARAM shading_inter = {0};
	IQT_CST_PARAM cst = {0};
	IQT_STRIPE_PARAM stripe = {0};
	IQT_YCURVE_PARAM ycurve = {0};
	IQT_OB_PARAM ob = {0};
	IQT_NR_PARAM nr_2d = {0};
	IQT_CFA_PARAM cfa = {0};
	IQT_VA_PARAM va = {0};
	IQT_GAMMA_PARAM gamma = {0};
	IQT_CCM_PARAM ccm = {0};
	IQT_COLOR_PARAM color = {0};
	IQT_CONTRAST_PARAM contrast = {0};
	IQT_EDGE_PARAM edge = {0};
	IQT_3DNR_PARAM nr_3d = {0};
	IQT_WDR_PARAM wdr = {0};
	IQT_SHDR_PARAM shdr = {0};
	IQT_RGBIR_PARAM rgbir = {0};
	IQT_COMPANDING_PARAM companding = {0};
	IQT_SHDR_MODE shdr_mode = {0};
	IQT_3DNR_MISC_PARAM nr_3d_misc = {0};
	IQT_POST_SHARPEN_PARAM post_sharpen = {0};
	IQT_DR_LEVEL dr_level = {0};
	IQT_RGBIR_ENH_PARAM rgbir_enh = {0};
	IQT_RGBIR_ENH_ISO rgbir_enh_iso = {0};

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
		printf("----------------------------------------\r\n");
		printf("   1. Get version \n");
		printf("   2. Reload config file \n");
		printf("   3. Reload dtsi file \n");
		printf("  10. Get nr level \n");
		printf("  11. Get sharpness level \n");
		printf("  12. Get saturation level \n");
		printf("  13. Get contrast level \n");
		printf("  14. Get brightness level \n");
		printf("  15. Get nighe mode \n");
		printf("  16. Get ycc format \n");
		printf("  17. Get operation \n");
		printf("  18. Get imageeffect \n");
		printf("  19. Get ccid \n");
		printf("  20. Get hue_shift \n");
		printf("  21. Get shdr_tone_lv \n");
		printf("  22. Get nr3d level \n");
		printf("  26. Get shading_inter \n");
		printf("  27. Get shading_ext \n");
		printf("  28. Get CST \n");
		printf("  29. Get STRIPE \n");
		printf("  30. Get YCURVE \n");
		printf("  31. Get ob \n");
		printf("  32. Get nr_2d \n");
		printf("  33. Get cfa \n");
		printf("  34. Get va \n");
		printf("  35. Get gamma \n");
		printf("  36. Get ccm \n");
		printf("  37. Get color \n");
		printf("  38. Get contrast \n");
		printf("  39. Get edge \n");
		printf("  40. Get nr_3d \n");
		printf("  41. Get dpc \n");
		printf("  42. Get shading \n");
		printf("  43. Get ldc \n");
		printf("  45. Get wdr \n");
		printf("  47. Get shdr \n");
		printf("  48. Get rgbir \n");
		printf("  49. Get companding \n");
		printf("  52. Get shdr_mode \n");
		printf("  53. Get 3dnr misc param \n");
		printf("  55. Get dr_level \n");
		printf("  56. Get rgbir_enh \n");
		printf("  57. Get rgbir_enh_iso \n");
		printf("  59. Get post_sharpen \n");
		printf(" 110. Set nr level \n");
		printf(" 111. Set sharpness level \n");
		printf(" 112. Set saturation level \n");
		printf(" 113. Set contrast level \n");
		printf(" 114. Set brightness level \n");
		printf(" 115. Set nighe mode \n");
		printf(" 116. Set ycc format \n");
		printf(" 117. Set operation \n");
		printf(" 118. Set imageeffect \n");
		printf(" 119. Set ccid \n");
		printf(" 120. Set hue_shift \n");
		printf(" 121. Set shdr_tone_lv \n");
		printf(" 122. Set nr3d level \n");
		printf(" 128. Set CST \n");
		printf(" 129. Set STRIPE \n");
		printf(" 153. Set 3dnr misc param \n");
		printf(" 500. Set auto tone level enable \n");
		printf("  0. Quit\n");
		printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_int();
		} while (option < 0);

		switch (option) {
		case 1:
			vendor_isp_get_iq(IQT_ITEM_VERSION, &version);
			printf("version = 0x%X \n", version);
			break;

		case 2:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfg_info.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Select chg file>> \n");
				printf("  1: isp_imx290_0.cfg \n");
				printf("  2: isp_os02k10_0.cfg \n");
				printf("  3: isp_os05a10_0.cfg \n");
				tmp = (UINT32)get_choose_int();
			} while (0);

			switch (tmp) {
			case 1:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
				break;

			case 2:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
				break;

			case 3:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0.cfg", CFG_NAME_LENGTH);
				break;

			default:
				printf("Not support item (%d) \n", tmp);
				break;
			}
			vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
			break;

		case 3:
			do {
				printf("Set isp id (0, 1)>> \n");
				dtsi_info.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Select dtsi file>> \n");
				printf("1: imx290_iq_0.dtsi \n");
				printf("2: imx290_iq_0_cap.dtsi \n");
				printf("3: imx290_iq_dpc_0.dtsi \n");
				printf("4: imx290_iq_ldc_0.dtsi \n");
				printf("5: imx290_iq_shading_0.dtsi \n");
				tmp = (UINT32)get_choose_int();
			} while (0);

			switch (tmp) {
			case 1:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0", DTSI_NAME_LENGTH);
				break;

			case 2:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0_cap", DTSI_NAME_LENGTH);
				break;

			case 3:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_dpc_0", DTSI_NAME_LENGTH);
				break;

			case 4:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_ldc_0", DTSI_NAME_LENGTH);
				break;

			case 5:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_shading_0", DTSI_NAME_LENGTH);
				break;

			default:
				printf("Not support item (%d) \n", tmp);
				break;
			}

			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info);
			break;

		case 10:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NR_LV, &nr_lv);
			printf("nr id = %d \n", nr_lv.id);
			printf("nr level = %d \n", nr_lv.lv);
			break;

		case 11:
			do {
				printf("Set isp id (0, 1)>> \n");
				sharpness_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHARPNESS_LV, &sharpness_lv);
			printf("sharpness id = %d \n", sharpness_lv.id);
			printf("sharpness level = %d \n", sharpness_lv.lv);
			break;

		case 12:
			do {
				printf("Set isp id (0, 1)>> \n");
				saturation_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SATURATION_LV, &saturation_lv);
			printf("saturation id = %d \n", saturation_lv.id);
			printf("saturation level = %d \n", saturation_lv.lv);
			break;

		case 13:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CONTRAST_LV, &contrast_lv);
			printf("contrast id = %d \n", contrast_lv.id);
			printf("contrast level = %d \n", contrast_lv.lv);
			break;

		case 14:
			do {
				printf("Set isp id (0, 1)>> \n");
				brightness_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_BRIGHTNESS_LV, &brightness_lv);
			printf("brightness id = %d \n", brightness_lv.id);
			printf("brightness level = %d \n", brightness_lv.lv);
			break;

		case 15:
			do {
				printf("Set isp id (0, 1)>> \n");
				night_mode.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
			printf("night id = %d \n", night_mode.id);
			printf("night mode = %d \n", night_mode.mode);
			break;

		case 16:
			do {
				printf("Set isp id (0, 1)>> \n");
				ycc_format.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_YCC_FORMAT, &ycc_format);
			printf("ycc_format id = %d \n", ycc_format.id);
			printf("ycc_format format = %d \n", ycc_format.format);
			break;

		case 17:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_OPERATION, &operation);
			printf("operation id = %d \n", operation.id);
			printf("operation sel = %d \n", operation.operation);
			break;

		case 18:
			do {
				printf("Set isp id (0, 1)>> \n");
				imageeffect.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_IMAGEEFFECT, &imageeffect);
			printf("imageeffect id = %d \n", imageeffect.id);
			printf("imageeffect effect = %d \n", imageeffect.effect);
			break;

		case 19:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccid.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CCID, &ccid);
			printf("color id = %d \n", ccid.id);
			printf("color ccid = %d \n", ccid.ccid);
			break;

		case 20:
			do {
				printf("Set isp id (0, 1)>> \n");
				hue_shift.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_HUE_SHIFT, &hue_shift);
			printf("hue id = %d \n", hue_shift.id);
			printf("hue shift = %d \n", hue_shift.hue_shift);
			break;

		case 21:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_tone_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
			printf("shdr_tone id = %d \n", shdr_tone_lv.id);
			printf("shdr_tone level = %d \n", shdr_tone_lv.lv);
			break;

		case 22:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr3d_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_3DNR_LV, &nr3d_lv);
			printf("3dnr id = %d \n", nr3d_lv.id);
			printf("3dnr level = %d \n", nr3d_lv.lv);
			break;

		case 26:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading_inter.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHADING_INTER_PARAM, &shading_inter);
			printf("shading_inter id = %d \n", shading_inter.id);
			printf("shading_inter mode = %d \n", shading_inter.shading_inter.mode);
			printf("shading_inter ecs_smooth_l_m_ct_lower = %d \n", shading_inter.shading_inter.ecs_smooth_l_m_ct_lower);
			printf("shading_inter ecs_smooth_l_m_ct_upper = %d \n", shading_inter.shading_inter.ecs_smooth_l_m_ct_upper);
			printf("shading_inter ecs_smooth_m_h_ct_lower = %d \n", shading_inter.shading_inter.ecs_smooth_m_h_ct_lower);
			printf("shading_inter ecs_smooth_m_h_ct_upper = %d \n", shading_inter.shading_inter.ecs_smooth_m_h_ct_upper);
			break;

		case 27:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading_ext.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ecs id (0, 1, 2)>> \n");
				shading_ext.shading_ext_if.ecs_map_idx = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHADING_EXT_PARAM, &shading_ext);
			printf("shading_ext id = %d \n", shading_ext.id);
			printf("shading_ext ecs_map_idx = %d \n", shading_ext.shading_ext_if.ecs_map_idx);
			printf("shading_ext ecs_map_tbl[0] = 0x%x \n", shading_ext.shading_ext_if.ecs_map_tbl[0]);
			break;

		case 28:
			do {
				printf("Set isp id (0, 1)>> \n");
				cst.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CST_PARAM, &cst);
			printf("cst id = %d \n", cst.id);
			printf("cst cst_coef = {%d, %d, %d, %d, %d, %d, %d, %d, %d}, \n"
				, cst.cst_coef[0], cst.cst_coef[1], cst.cst_coef[2], cst.cst_coef[3], cst.cst_coef[4]
				, cst.cst_coef[5], cst.cst_coef[6], cst.cst_coef[7], cst.cst_coef[8]);
			break;

		case 29:
			do {
				printf("Set isp id (0, 1)>> \n");
				stripe.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_STRIPE_PARAM, &stripe);
			printf("stripe id = %d \n", stripe.id);
			printf("stripe cst_coef = %d \n", stripe.stripe_type);
			break;

		case 30:
			do {
				printf("Set isp id (0, 1)>> \n");
				ycurve.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_YCURVE_PARAM, &ycurve);
			printf("ycurve id = %d \n", cst.id);
			printf("ycurve ycurve = {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, ...}, \n"
				, ycurve.ycurve.ycurve_lut[0], ycurve.ycurve.ycurve_lut[1], ycurve.ycurve.ycurve_lut[2]
				, ycurve.ycurve.ycurve_lut[3], ycurve.ycurve.ycurve_lut[4], ycurve.ycurve.ycurve_lut[5]
				, ycurve.ycurve.ycurve_lut[6], ycurve.ycurve.ycurve_lut[7], ycurve.ycurve.ycurve_lut[8]
				, ycurve.ycurve.ycurve_lut[9]);
			break;

		case 31:
			do {
				printf("Set isp id (0, 1)>> \n");
				ob.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_OB_PARAM, &ob);
			printf("ob id = %d \n", ob.id);
			printf("ob mode = %d \n", ob.ob.mode);
			break;

		case 32:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_2d.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NR_PARAM, &nr_2d);
			printf("nr_2d id = %d \n", nr_2d.id);
			printf("nr_2d mode = %d \n", nr_2d.nr.mode);
			break;

		case 33:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfa.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CFA_PARAM, &cfa);
			printf("cfa id = %d \n", cfa.id);
			printf("cfa mode = %d \n", cfa.cfa.mode);
			break;

		case 34:
			do {
				printf("Set isp id (0, 1)>> \n");
				va.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_VA_PARAM, &va);
			printf("va id = %d \n", va.id);
			printf("va mode = %d \n", va.va.mode);
			break;

		case 35:
			do {
				printf("Set isp id (0, 1)>> \n");
				gamma.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_GAMMA_PARAM, &gamma);
			printf("gamma id = %d \n", gamma.id);
			printf("gamma mode = %d \n", gamma.gamma.mode);
			break;

		case 36:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccm.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CCM_PARAM, &ccm);
			printf("ccm id = %d \n", ccm.id);
			printf("ccm mode = %d \n", ccm.ccm.mode);
			break;

		case 37:
			do {
				printf("Set isp id (0, 1)>> \n");
				color.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_COLOR_PARAM, &color);
			printf("color id = %d \n", color.id);
			printf("color mode = %d \n", color.color.mode);
			break;

		case 38:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CONTRAST_PARAM, &contrast);
			printf("contrast id = %d \n", contrast.id);
			printf("contrast mode = %d \n", contrast.contrast.mode);
			break;

		case 39:
			do {
				printf("Set isp id (0, 1)>> \n");
				edge.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_EDGE_PARAM, &edge);
			printf("edge id = %d \n", edge.id);
			printf("edge mode = %d \n", edge.edge.mode);
			break;

		case 40:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_3d.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_3DNR_PARAM, &nr_3d);
			printf("nr_3d id = %d \n", nr_3d.id);
			printf("nr_3d mode = %d \n", nr_3d._3dnr.mode);
			break;

		case 41:
			do {
				printf("Set isp id (0, 1)>> \n");
				dpc.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_DPC_PARAM, &dpc);
			printf("dpc id = %d \n", dpc.id);
			printf("dpc enable = %d \n", dpc.dpc.enable);
			break;

		case 42:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
			printf("shading id = %d \n", shading.id);
			printf("shading ecs_enable = %d \n", shading.shading.ecs_enable);
			printf("shading vig_enable = %d \n", shading.shading.vig_enable);
			break;

		case 43:
			do {
				printf("Set isp id (0, 1)>> \n");
				ldc.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_LDC_PARAM, &ldc);
			printf("ldc id = %d \n", ldc.id);
			printf("ldc geo_enable = %d \n", ldc.ldc.geo_enable);
			printf("ldc lut_2d_enable = %d \n", ldc.ldc.lut_2d_enable);
			break;

		case 45:
			do {
				printf("Set isp id (0, 1)>> \n");
				wdr.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_WDR_PARAM, &wdr);
			printf("wdr id = %d \n", wdr.id);
			printf("wdr mode = %d \n", wdr.wdr.mode);
			break;

		case 47:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHDR_PARAM, &shdr);
			printf("shdr id = %d \n", shdr.id);
			printf("shdr mode = %d \n", shdr.shdr.mode);
			break;

		case 48:
			do {
				printf("Set isp id (0, 1)>> \n");
				rgbir.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_RGBIR_PARAM, &rgbir);
			printf("rgbir id = %d \n", rgbir.id);
			printf("rgbir mode = %d \n", rgbir.rgbir.mode);
			break;

		case 49:
			do {
				printf("Set isp id (0, 1)>> \n");
				companding.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_COMPANDING_PARAM, &companding);
			printf("companding id = %d \n", companding.id);
			printf("companding decomp_knee_pts = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \n"
				, companding.companding.decomp_knee_pts[0], companding.companding.decomp_knee_pts[1], companding.companding.decomp_knee_pts[2]
				, companding.companding.decomp_knee_pts[3], companding.companding.decomp_knee_pts[4], companding.companding.decomp_knee_pts[5]
				, companding.companding.decomp_knee_pts[6], companding.companding.decomp_knee_pts[7], companding.companding.decomp_knee_pts[8]
				, companding.companding.decomp_knee_pts[9], companding.companding.decomp_knee_pts[10]);
			break;

		case 52:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_mode.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHDR_MODE, &shdr_mode);
			printf("shdr_mode id = %d \n", shdr_mode.id);
			printf("shdr_mode mode = %d \n", shdr_mode.shdr_mode);
			break;

		case 53:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_3d_misc.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_3DNR_MISC_PARAM, &nr_3d_misc);
			printf("nr_3d_misc id = %d \n", nr_3d_misc.id);
			printf("nr_3d_misc md_roi = %d, %d \n", nr_3d_misc._3dnr_misc.md_roi[0], nr_3d_misc._3dnr_misc.md_roi[1]);
			printf("nr_3d_misc mc_roi = %d, %d \n", nr_3d_misc._3dnr_misc.mc_roi[0], nr_3d_misc._3dnr_misc.mc_roi[1]);
			printf("nr_3d_misc roi_mv_th = %d \n", nr_3d_misc._3dnr_misc.roi_mv_th);
			printf("nr_3d_misc ds_th_roi = %d \n", nr_3d_misc._3dnr_misc.ds_th_roi);
			break;

		case 55:
			do {
				printf("Set isp id (0, 1)>> \n");
				dr_level.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_DR_LEVEL, &dr_level);
			printf("dr_level id = %d \n", dr_level.id);
			printf("dr_level dr_level = %d \n", dr_level.dr_level);
			break;

		case 56:
			do {
				printf("Set isp id (0, 1)>> \n");
				rgbir_enh.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_RGBIR_ENH_PARAM, &rgbir_enh);
			printf("rgbir_enh id = %d \n", rgbir_enh.id);
			printf("rgbir_enh enable = %d \n", rgbir_enh.rgbir_enh.enable);
			break;

		case 57:
			do {
				printf("Set isp id (0, 1)>> \n");
				rgbir_enh_iso.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_RGBIR_ENH_ISO, &rgbir_enh_iso);
			printf("rgbir_enh_iso id = %d \n", rgbir_enh_iso.id);
			printf("rgbir_enh_iso rgbir_enh_iso = %d \n", rgbir_enh_iso.rgbir_enh_iso);
			break;

		case 59:
			do {
				printf("Set isp id (0, 1)>> \n");
				post_sharpen.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_POST_SHARPEN_PARAM, &post_sharpen);
			printf("post_sharpen id = %d \n", post_sharpen.id);
			printf("post_sharpen enable = %d \n", post_sharpen.post_sharpen.enable);
			printf("post_sharpen mode = %d \n", post_sharpen.post_sharpen.mode);
			break;

		case 110:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				nr_lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NR_LV, &nr_lv);
			printf("nr id = %d \n", nr_lv.id);
			printf("nr level = %d \n", nr_lv.lv);
			break;

		case 111:
			do {
				printf("Set isp id (0, 1)>> \n");
				sharpness_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				sharpness_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SHARPNESS_LV, &sharpness_lv);
			printf("sharpness id = %d \n", sharpness_lv.id);
			printf("sharpness level = %d \n", sharpness_lv.lv);
			break;

		case 112:
			do {
				printf("Set isp id (0, 1)>> \n");
				saturation_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				saturation_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SATURATION_LV, &saturation_lv);
			printf("saturation id = %d \n", saturation_lv.id);
			printf("saturation level = %d \n", saturation_lv.lv);
			break;

		case 113:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				contrast_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_CONTRAST_LV, &contrast_lv);
			printf("contrast id = %d \n", contrast_lv.id);
			printf("contrast level = %d \n", contrast_lv.lv);
			break;

		case 114:
			do {
				printf("Set isp id (0, 1)>> \n");
				brightness_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				brightness_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_BRIGHTNESS_LV, &brightness_lv);
			printf("brightness id = %d \n", brightness_lv.id);
			printf("brightness level = %d \n", brightness_lv.lv);
			break;

		case 115:
			do {
				printf("Set isp id (0, 1)>> \n");
				night_mode.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set mode (0 = OFF ; 1 = ON)>> \n");
				night_mode.mode = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
			printf("night id = %d \n", night_mode.id);
			printf("night mode = %d \n", night_mode.mode);
			break;

		case 116:
			do {
				printf("Set isp id (0, 1)>> \n");
				ycc_format.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set format (0 ~ %d)>> \n", IQ_UI_YCC_OUT_MAX_CNT - 1);
				ycc_format.format = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_YCC_FORMAT, &ycc_format);
			printf("ycc_format id = %d \n", ycc_format.id);
			printf("ycc_format format = %d \n", ycc_format.format);
			break;

		case 117:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set operation (0 ~ %d)>> \n", IQ_UI_OPERATION_MAX_CNT - 1);
				operation.operation = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_OPERATION, &operation);
			printf("operation id = %d \n", operation.id);
			printf("operation sel = %d \n", operation.operation);
			break;

		case 118:
			do {
				printf("Set isp id (0, 1)>> \n");
				imageeffect.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set effect (0 ~ %d)>> \n", IQ_UI_IMAGEEFFECT_MAX_CNT - 1);
				imageeffect.effect = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_IMAGEEFFECT, &imageeffect);
			printf("imageeffect id = %d \n", imageeffect.id);
			printf("imageeffect effect = %d \n", imageeffect.effect);
			break;

		case 119:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccid.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ccid (0 ~ %d)>> \n", IQ_UI_CCID_MAX_CNT - 1);
				ccid.ccid = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_CCID, &ccid);
			printf("color id = %d \n", ccid.id);
			printf("color ccid = %d \n", ccid.ccid);
			break;

		case 120:
			do {
				printf("Set isp id (0, 1)>> \n");
				hue_shift.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set hue_shift (0 ~ %d)>> \n", IQ_UI_HUE_SHIFT_MAX_CNT - 1);
				hue_shift.hue_shift = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_HUE_SHIFT, &hue_shift);
			printf("hue id = %d \n", hue_shift.id);
			printf("hue shift = %d \n", hue_shift.hue_shift);
			break;

		case 121:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_tone_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set shdr_tone_lv (0 ~ %d)>> \n", IQ_UI_SHDR_TONE_LV_MAX_CNT - 1);
				shdr_tone_lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
			printf("shdr_tone id = %d \n", shdr_tone_lv.id);
			printf("shdr_tone level = %d \n", shdr_tone_lv.lv);
			break;

		case 122:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr3d_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				nr3d_lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_3DNR_LV, &nr3d_lv);
			printf("3dnr id = %d \n", nr3d_lv.id);
			printf("3dnr level = %d \n", nr3d_lv.lv);
			break;

		case 128:
			do {
				printf("Set isp id (0, 1)>> \n");
				cst.id = (UINT32)get_choose_int();
			} while (0);

			cst.cst_coef[0] = 77;
			cst.cst_coef[1] = 150;
			cst.cst_coef[2] = 29;
			cst.cst_coef[3] = 0;
			cst.cst_coef[4] = 0;
			cst.cst_coef[5] = 0;
			cst.cst_coef[6] = 0;
			cst.cst_coef[7] = 0;
			cst.cst_coef[8] = 0;

			vendor_isp_set_iq(IQT_ITEM_CST_PARAM, &cst);
			printf("cst id = %d \n", cst.id);
			printf("cst cst_coef = {%d, %d, %d, %d, %d, %d, %d, %d, %d}, \n"
				, cst.cst_coef[0], cst.cst_coef[1], cst.cst_coef[2], cst.cst_coef[3], cst.cst_coef[4]
				, cst.cst_coef[5], cst.cst_coef[6], cst.cst_coef[7], cst.cst_coef[8]);
			break;

		case 129:
			do {
				printf("Set isp id (0, 1)>> \n");
				stripe.id = (UINT32)get_choose_int();
			} while (0);

			stripe.stripe_type = IQT_STRIPE_MANUAL_4STRIPE;

			vendor_isp_set_iq(IQT_ITEM_STRIPE_PARAM, &stripe);
			printf("stripe id = %d \n", stripe.id);
			printf("stripe stripe_type = %d, \n", stripe.stripe_type);
			break;

		case 153:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_3d_misc.id = (UINT32)get_choose_int();
			} while (0);

			nr_3d_misc._3dnr_misc.mc_roi[0] = 1;
			nr_3d_misc._3dnr_misc.mc_roi[1] = 2;
			nr_3d_misc._3dnr_misc.md_roi[0] = 3;
			nr_3d_misc._3dnr_misc.md_roi[1] = 4;
			nr_3d_misc._3dnr_misc.roi_mv_th = 11;
			nr_3d_misc._3dnr_misc.ds_th_roi = 22;

			vendor_isp_set_iq(IQT_ITEM_3DNR_MISC_PARAM, &nr_3d_misc);
			printf("nr_3d_misc id = %d \n", nr_3d_misc.id);
			printf("nr_3d_misc param = %d %d %d %d %d %d \n", nr_3d_misc._3dnr_misc.mc_roi[0], nr_3d_misc._3dnr_misc.mc_roi[1], 
												nr_3d_misc._3dnr_misc.md_roi[0], nr_3d_misc._3dnr_misc.md_roi[1], 
												nr_3d_misc._3dnr_misc.roi_mv_th, nr_3d_misc._3dnr_misc.ds_th_roi);
			break;

		case 500:
			do {
				printf("Set isp id (0, 1)>> \n");
				tmp = (UINT32)get_choose_int();
				printf("Set auto tone level enable (0, 1)>> \n");
				tone_level_en = (BOOL)get_choose_int();
				printf("Set debug enable (0, 1)>> \n");
				tone_level_dbg = (BOOL)get_choose_int();
			} while (0);

			printf("dr_level id = %d \n", dr_level.id);

			iq_tonelv_conti_run = 1;
			if (pthread_create(&iq_tonelv_thread_id, NULL, iq_tonelv_thread, &tmp) < 0) {
				printf("create iq_tonelv thread failed");
				break;
			}

			do {
				printf("Enter 0 to exit >> \n");
				iq_tonelv_conti_run = (UINT32)get_choose_int();
			} while (0);

			// destroy encode thread
			pthread_join(iq_tonelv_thread_id, NULL);

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

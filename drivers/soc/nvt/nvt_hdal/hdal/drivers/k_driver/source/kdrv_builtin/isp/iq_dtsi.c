#if defined(__KERNEL__)

#include <linux/slab.h>
#include <linux/of.h>
#include <plat/top.h>

#include "kwrap/type.h"
#include "unlzma.h"
#include "iq_dtsi.h"
#include "iq_param.h"
#include "isp_builtin.h"

#if (NVT_FAST_ISP_FLOW)
//=============================================================================
// global
//=============================================================================
static IQ_DTSI iq_dtsi[IQ_DTSI_ITEM_MAX_NUM] = {
	{"iq_ob",                   sizeof(IQ_OB_PARAM)            },
	{"iq_nr",                   sizeof(IQ_NR_PARAM)            },
	{"iq_cfa",                  sizeof(IQ_CFA_PARAM)           },
	{"iq_va",                   sizeof(IQ_VA_PARAM)            },
	{"iq_gamma",                sizeof(IQ_GAMMA_PARAM)         },
	{"iq_ccm",                  sizeof(IQ_CCM_PARAM)           },
	{"iq_color",                sizeof(IQ_COLOR_PARAM)         },
	{"iq_contrast",             sizeof(IQ_CONTRAST_PARAM)      },
	{"iq_edge",                 sizeof(IQ_EDGE_PARAM)          },
	{"iq_3dnr",                 sizeof(IQ_3DNR_PARAM)          },
	{"iq_wdr",                  sizeof(IQ_WDR_PARAM)           },
	{"iq_shdr",                 sizeof(IQ_SHDR_PARAM)          },
	{"iq_rgbir",                sizeof(IQ_RGBIR_PARAM)         },
	{"iq_companding",           sizeof(IQ_COMPANDING_PARAM)    },
	{"iq_rgbir_enh",            sizeof(IQ_RGBIR_ENH_PARAM)     },
	{"iq_post_sharpen",         sizeof(IQ_POST_SHARPEN_PARAM)  },
	{"iq_dpc",                  sizeof(IQ_DPC_PARAM)           },
	{"iq_shading",              sizeof(IQ_SHADING_PARAM)       },
	{"iq_shading_inter",        sizeof(IQ_SHADING_INTER_PARAM) },
	{"iq_shading_ext",          sizeof(IQ_SHADING_EXT_PARAM)   },
	{"iq_ldc",                  sizeof(IQ_LDC_PARAM)           },
	{"iq_ycurve",               sizeof(IQ_YCURVE_PARAM)        },
};

//=============================================================================
// internal functions
//=============================================================================
static void iq_dtsi_sub_load(CHAR *node_path, UINT8 *param_ptr, UINT32 index)
{
	CHAR sub_node_name[SUB_NODE_LENGTH];
	UINT32 size = 0, cmp_size = 0;
	UINT8 *cmp_prt = NULL;
	struct device_node* of_node;

	sprintf(sub_node_name, "%s/%s", node_path, iq_dtsi[index].sub_node_name);
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "size", (UINT8 *)&size, sizeof(UINT32)) == 0) {
			if (size != iq_dtsi[index].size) {
				printk("%s/size mismatch!! (dtsi:%d, sdk:%d) \r\n", sub_node_name, size, iq_dtsi[index].size);
				return;
			}
		} else {
			printk("cannot find %s/size \r\n", sub_node_name);
			return;
		}

		if (of_property_read_u8_array(of_node, "cmp_size", (UINT8 *)&cmp_size, sizeof(UINT32)) != 0) {
			cmp_size = 0;
		}

		if (cmp_size != 0) {
			cmp_prt = kzalloc(ALIGN_CEIL(cmp_size+13, 4), GFP_KERNEL);
			if (cmp_prt != NULL) {
				cmp_prt[0] = 0x5D;
				cmp_prt[4] = 0x1;
				cmp_prt[5] = iq_dtsi[index].size & 0xFF;
				cmp_prt[6] = (iq_dtsi[index].size >> 8) & 0xFF;
				cmp_prt[7] = (iq_dtsi[index].size >> 16) & 0xFF;
				cmp_prt[8] = (iq_dtsi[index].size >> 24) & 0xFF;
				if (of_property_read_u8_array(of_node, "data", (UINT8 *)(&cmp_prt[13]), cmp_size) == 0) {
					lzma_inflate(cmp_prt, cmp_size+13, param_ptr, (UINT32)iq_dtsi[index].size);
				} else {
					printk("cannot find %s/cmp_data \r\n", sub_node_name);
				}

				kfree(cmp_prt);
				cmp_prt = NULL;
			} else {
				printk("allocate cmp_prt fail!\n");
			}
		} else {
			if (of_property_read_u8_array(of_node, "data", param_ptr, iq_dtsi[index].size) != 0) {
				printk("cannot find %s/data \r\n", sub_node_name);
			}
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}
}

//=============================================================================
// external functions
//=============================================================================
void iq_dtsi_load(UINT32 id, void *param)
{
	IQ_PARAM_PTR *iq_param = (IQ_PARAM_PTR *)param;
	CHAR node_path[SUB_NODE_LENGTH];
	CHAR node_path2[SUB_NODE_LENGTH];
	CHAR node_path3[SUB_NODE_LENGTH];
	CHAR node_path4[SUB_NODE_LENGTH];

	ISP_BUILTIN_DTSI *isp_builtin_dtsi = NULL;

	if (id < ISP_BUILTIN_DTSI_MAX) {
		isp_builtin_dtsi = isp_builtin_get_dtsi(id);
		if (isp_builtin_dtsi != NULL) {
			sprintf(node_path, isp_builtin_dtsi->iq_node_path);
			sprintf(node_path2, isp_builtin_dtsi->iq_dpc_node_path);
			sprintf(node_path3, isp_builtin_dtsi->iq_shading_node_path);
			sprintf(node_path4, isp_builtin_dtsi->iq_ldc_node_path);
		} else {
			printk("iq_dtsi_load, id = %d, isp_builtin_dtsi = NULL \r\n", id);
		}
	} else {
		printk("iq_dtsi_load, id = %d >= %d \r\n", id, ISP_BUILTIN_DTSI_MAX);
		return;
	}

	// NOTE: OB
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->ob, IQ_DTSI_ITEM_OB_PARAM);

	// NOTE: NR
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->nr, IQ_DTSI_ITEM_NR_PARAM);

	// NOTE: CFA
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->cfa, IQ_DTSI_ITEM_CFA_PARAM);

	// NOTE: VA
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->va, IQ_DTSI_ITEM_VA_PARAM);

	// NOTE: GAMMA
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->gamma, IQ_DTSI_ITEM_GAMMA_PARAM);

	// NOTE: CCM
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->ccm, IQ_DTSI_ITEM_CCM_PARAM);

	// NOTE: COLOR
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->color, IQ_DTSI_ITEM_COLOR_PARAM);

	// NOTE: CONTRAST
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->contrast, IQ_DTSI_ITEM_CONTRAST_PARAM);

	// NOTE: EDGE
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->edge, IQ_DTSI_ITEM_EDGE_PARAM);

	// NOTE: 3DNR
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->_3dnr, IQ_DTSI_ITEM_3DNR_PARAM);

	// NOTE: WDR
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->wdr, IQ_DTSI_ITEM_WDR_PARAM);

	// NOTE: SHDR
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->shdr, IQ_DTSI_ITEM_SHDR_PARAM);

	// NOTE: RGBIR
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->rgbir, IQ_DTSI_ITEM_RGBIR_PARAM);

	// NOTE: COMPANDING
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->companding, IQ_DTSI_ITEM_COMPANDING_PARAM);

	// NOTE: RGBIR ENH
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->rgbir_enh, IQ_DTSI_ITEM_RGBIR_ENH_PARAM);

	// NOTE: POST 3DNR
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->post_sharpen, IQ_DTSI_ITEM_POST_SHARPEN_PARAM);

	// NOTE: DPC
	iq_dtsi_sub_load(node_path2, (UINT8 *)iq_param->dpc, IQ_DTSI_ITEM_DPC_PARAM);

	// NOTE: SHADING
	iq_dtsi_sub_load(node_path3, (UINT8 *)iq_param->shading, IQ_DTSI_ITEM_SHADING_PARAM);

	// NOTE: SHADING INTER
	iq_dtsi_sub_load(node_path3, (UINT8 *)iq_param->shading_inter, IQ_DTSI_ITEM_SHADING_INTER_PARAM);

	// NOTE: SHADING EXT
	iq_dtsi_sub_load(node_path3, (UINT8 *)iq_param->shading_ext, IQ_DTSI_ITEM_SHADING_EXT_PARAM);

	// NOTE: LDC
	iq_dtsi_sub_load(node_path4, (UINT8 *)iq_param->ldc, IQ_DTSI_ITEM_LDC_PARAM);

	// NOTE: YCURVE
	iq_dtsi_sub_load(node_path, (UINT8 *)iq_param->ycurve, IQ_DTSI_ITEM_YCURVE_PARAM);
}

#endif
#endif


#include "display_obj_platform.h"

#if defined __UITRON || defined __ECOS
#elif defined __FREERTOS
unsigned int dispobj_debug_level = NVT_DBG_WRN;
#else
#endif

void display_obj_platform_do_div(UINT64 *dividend, UINT32 divisor)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	*dividend = (*dividend / divisor);
#else
	UINT64 result;

	result = *dividend;
	do_div(result, divisor);
	*dividend = result;
#endif
}

ER display_obj_platform_pll_en(DISPCTRL_SRCCLK clk_src)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS

	if (clk_src == DISPCTRL_SRCCLK_PLL1) {
		if (pll_getPLLEn(PLL_ID_1) != TRUE) {
			pll_setPLLEn(PLL_ID_1, TRUE);
		}
	} else if (clk_src == DISPCTRL_SRCCLK_PLL4) {
		if (pll_getPLLEn(PLL_ID_4) != TRUE) {
			pll_setPLLEn(PLL_ID_4, TRUE);
		}
#if defined(_BSP_NA51000_)
	} else if (clk_src == DISPCTRL_SRCCLK_PLL6) {
		if (pll_getPLLEn(PLL_ID_2) != TRUE) {
			pll_setPLLEn(PLL_ID_2, TRUE);
		}
	} else if (clk_src == DISPCTRL_SRCCLK_PLL9) {
		if (pll_getPLLEn(PLL_ID_14) != TRUE) {
			pll_setPLLEn(PLL_ID_14, TRUE);
		}
#else
	} else if (clk_src == DISPCTRL_SRCCLK_PLL6) {
		if (pll_getPLLEn(PLL_ID_6) != TRUE) {
			pll_setPLLEn(PLL_ID_6, TRUE);
		}
	} else if (clk_src == DISPCTRL_SRCCLK_PLL9) {
		if (pll_getPLLEn(PLL_ID_9) != TRUE) {
			pll_setPLLEn(PLL_ID_9, TRUE);
		}
#endif
	} else {
		DBG_ERR("DISPDEVCTR: SET_CLK_EN error !!\r\n");
		return E_SYS;
	}

#else
#endif
	return E_OK;
}

ER display_obj_platform_set_clk_src(IDE_ID id, DISPCTRL_SRCCLK clk_src)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (clk_src == DISPCTRL_SRCCLK_PLL1) {
		pll_setClockRate(PLL_CLKSEL_IDE_CLKSRC, PLL_CLKSEL_IDE_CLKSRC_480);
	} else if (clk_src == DISPCTRL_SRCCLK_PLL4) {
		pll_setClockRate(PLL_CLKSEL_IDE_CLKSRC, PLL_CLKSEL_IDE_CLKSRC_PLL4);
#if defined(_BSP_NA51000_)
	} else if (clk_src == DISPCTRL_SRCCLK_PLL6) {
		pll_setClockRate(PLL_CLKSEL_IDE_CLKSRC, PLL_CLKSEL_IDE_CLKSRC_PLL2);
	} else if (clk_src == DISPCTRL_SRCCLK_PLL9) {
		pll_setClockRate(PLL_CLKSEL_IDE_CLKSRC, PLL_CLKSEL_IDE_CLKSRC_PLL14);
#else
	} else if (clk_src == DISPCTRL_SRCCLK_PLL6) {
		pll_setClockRate(PLL_CLKSEL_IDE_CLKSRC, PLL_CLKSEL_IDE_CLKSRC_PLL6);
	} else if (clk_src == DISPCTRL_SRCCLK_PLL9) {
		pll_setClockRate(PLL_CLKSEL_IDE_CLKSRC, PLL_CLKSEL_IDE_CLKSRC_PLL9);
#endif
	} else {
		DBG_ERR("DISPDEVCTR: SET_CLK_FREQ error !!\r\n");
		return E_SYS;
	}
#else
	ER er_return;

	er_return = ide_platform_set_clksrc(id, clk_src);
	if (er_return != E_OK) {
		DBG_ERR("DISPDEVCTR: SET_CLK_FREQ error !!\r\n");
		return E_SYS;
	}
#endif
	return E_OK;
}

UINT32 display_obj_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_getPhyAddr(addr);
#else
	return fmem_lookup_pa(addr);
#endif
}

#if !(defined __UITRON || defined __ECOS || defined __FREERTOS)
int display_obj_set_osd_addr(DISP_ID id, unsigned int addr, bool load)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;
	DISPBUFFORMAT format;
	UINT32 w, h;

	if (addr & 0xF) {
		DBG_ERR("addr should be 4 word align 0x%x!!\r\n", (unsigned int)addr);
		return -1;
	}

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_MODE, &disp_lyr);
	format = disp_lyr.SEL.GET_MODE.buf_format;

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);
	w = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs;
	h = disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;

	if (format == DISPBUFFORMAT_ARGB8565) {
		disp_lyr.SEL.SET_OSDBUFADDR.buf_sel = DISPBUFADR_0;
		disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_buf0 = addr;
		disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_alpha0 = A_ADDR(w * h, addr);
		p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_OSDBUFADDR, &disp_lyr);
	} else {
		disp_lyr.SEL.SET_OSDBUFADDR.buf_sel = DISPBUFADR_0;
		disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_buf0 = addr;
		p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_OSDBUFADDR, &disp_lyr);
	}

	if (load) {
		idec_set_callback((IDE_ID)(id), NULL);
		p_disp_obj->load(TRUE);
	}


	return 0;
}

int display_obj_set_video1_blank(DISP_ID id, unsigned int blank_mode)
{
	PDISP_OBJ       p_disp_obj;
	DISPCTRL_PARAM  disp_ctrl;

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	idec_set_callback((IDE_ID)(id), NULL);
	if (blank_mode == 1) {
		disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = FALSE;
	} else if (blank_mode == 0) {
		disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
	} else {
		DBG_ERR("not support blank mode %d !!\r\n", (int)blank_mode);
		return -1;
	}

	disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_VDO1;
	p_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &disp_ctrl);
	p_disp_obj->load(TRUE);

	return 0;
}

int display_obj_set_video2_blank(DISP_ID id, unsigned int blank_mode)
{
	PDISP_OBJ       p_disp_obj;
	DISPCTRL_PARAM  disp_ctrl;

	if (id > DISP_1){
		DBG_ERR("not support disp id %d !!\r\n", (int)id);
		return -1;
	}

	p_disp_obj = disp_get_display_object(id);

	idec_set_callback((IDE_ID)(id), NULL);
	if (blank_mode == 1) {
		disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = FALSE;
	} else if (blank_mode == 0) {
		disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
	} else {
		DBG_ERR("not support blank mode %d !!\r\n", (int)blank_mode);
		return -1;
	}

	disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_VDO2;
	p_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &disp_ctrl);
	p_disp_obj->load(TRUE);

	return 0;
}

int display_obj_set_osd_blank(DISP_ID id, unsigned int blank_mode)
{
	PDISP_OBJ       p_disp_obj;
	DISPCTRL_PARAM  disp_ctrl;

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	idec_set_callback((IDE_ID)(id), NULL);
	if (blank_mode == 1) {
		disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = FALSE;
	} else if (blank_mode == 0) {
		disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
	} else {
		DBG_ERR("not support blank mode %d !!\r\n", (int)blank_mode);
		return -1;
	}

	disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_OSD1;
	p_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &disp_ctrl);
	p_disp_obj->load(TRUE);

	return 0;
}

int display_obj_set_osd_palette(DISP_ID id, unsigned int regno, unsigned int val)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	DBG_IND("rgb val: %x\r\n", (unsigned int)val);

	disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.r_to_y = (val & 0xff);
	disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.g_to_u = ((val >> 8) & 0xff);
	disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.b_to_v = ((val >> 16) & 0xff);

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_CST_FROM_RGB_TO_YUV, &disp_lyr);

	val = (val & 0xff000000) | (disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.r_to_y << 16) | (disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.g_to_u << 8) | disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.b_to_v;

	DBG_IND("yuv val: %x\r\n", (unsigned int)val);

	disp_lyr.SEL.SET_PALETTE.ui_start      = regno;
	disp_lyr.SEL.SET_PALETTE.ui_number     = 1;
	disp_lyr.SEL.SET_PALETTE.p_pale_entry   = &val;

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_PALETTEACRCBY, &disp_lyr);

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_PALETTEACRCBY, &disp_lyr);
	DBG_IND("ui_start: %d, p_pale_entry: %x\r\n", (int)disp_lyr.SEL.SET_PALETTE.ui_start, (unsigned int)*disp_lyr.SEL.SET_PALETTE.p_pale_entry);

	return 0;
}

int display_obj_get_video1_buf_addr(DISP_ID id, unsigned long *phys_addr, char __iomem **virt_addr, unsigned int *buffer_len)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;
	DISPBUFFORMAT format;

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_VDOBUFADDR, &disp_lyr);

	*virt_addr = (char *)disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_y0;
	*phys_addr = (fmem_lookup_pa(disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_y0));

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_MODE, &disp_lyr);
	format = disp_lyr.SEL.GET_MODE.buf_format;

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);

	if (format == DISPBUFFORMAT_YUV422PACK) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height * 2;
	} else if (format == DISPBUFFORMAT_YUV420PACK) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height * 3 / 2;
	} else {
		DBG_WRN("buffer format not set!\r\n");
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height * 2;
	}


	DBG_IND("virt_addr: %x, phys_addr: %lx, buffer_size: %d\r\n", (unsigned int)*virt_addr, (unsigned long)*phys_addr, (int)*buffer_len);

	// UV addr?
	//disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_cb0;
	return 0;
}

int display_obj_get_video2_buf_addr(DISP_ID id, unsigned long *phys_addr, char __iomem **virt_addr, unsigned int *buffer_len)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;
	DISPBUFFORMAT format;

	if (id > DISP_1){
		DBG_ERR("not support disp id %d !!\r\n", (int)id);
		return -1;
	}

	p_disp_obj = disp_get_display_object(id);

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_GET_VDOBUFADDR, &disp_lyr);

	*virt_addr = (char *)disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_y0;
	phys_addr = (unsigned long *)(fmem_lookup_pa(disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_y0));

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_GET_MODE, &disp_lyr);
	format = disp_lyr.SEL.GET_MODE.buf_format;

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);

	if (format == DISPBUFFORMAT_YUV422PACK) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height * 2;
	} else if (format == DISPBUFFORMAT_YUV420PACK) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height * 3 / 2;
	} else {
		DBG_WRN("buffer format not set!\r\n");
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height * 2;
	}
	DBG_IND("virt_addr: %x, phys_addr: %lx, buffer_size: %d\r\n", (unsigned int)*virt_addr, (unsigned long)phys_addr, (int)*buffer_len);
	// UV addr?
	//disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_cb0;
	return 0;
}

int display_obj_get_osd_buf_addr(DISP_ID id, unsigned long *phys_addr, char __iomem **virt_addr, unsigned int *buffer_len)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;
	DISPBUFFORMAT format;

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_OSDBUFADDR, &disp_lyr);

	*virt_addr = (char *)disp_lyr.SEL.GET_OSDBUFADDR.ui_addr_buf0;
	phys_addr = (unsigned long *)(fmem_lookup_pa(disp_lyr.SEL.GET_OSDBUFADDR.ui_addr_buf0));

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_MODE, &disp_lyr);
	format = disp_lyr.SEL.GET_MODE.buf_format;

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);

	if (format == DISPBUFFORMAT_ARGB8565) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height * 3 / 2;
	} else if (format == DISPBUFFORMAT_ARGB8888) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;
	} else if (format == DISPBUFFORMAT_PAL8) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;
	} else if (format == DISPBUFFORMAT_ARGB4444 || format == DISPBUFFORMAT_ARGB1555) {
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;
	} else {
		DBG_WRN("buffer format not set!\r\n");
		*buffer_len = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs * disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;
	}

	DBG_IND("virt_addr: %x, phys_addr: %lx, buffer_size: %d\r\n", (unsigned int)*virt_addr, (unsigned long)phys_addr, (int)*buffer_len);

	// alpha addr?
	//disp_lyr.SEL.GET_OSDBUFADDR.ui_addr_cb0;
	return 0;
}

int display_obj_get_video1_param(DISP_ID id, struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;
	UINT32 h_sync, h_total, h_valid_st, h_valid_ed;
	UINT32 v_sync, v_total, v_valid_oddst, v_valid_odded, v_valid_evenst, v_valid_evened;

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_WINSIZE, &disp_lyr);

	if (var != NULL) {
		var->xres = disp_lyr.SEL.GET_WINSIZE.ui_win_width;
		var->yres = disp_lyr.SEL.GET_WINSIZE.ui_win_height;

		p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);
		var->xres_virtual = disp_lyr.SEL.GET_BUFSIZE.ui_buf_width;
		var->yres_virtual = disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;

		var->xoffset = 0;
		var->yoffset = 0;

		DBG_IND("xres: %d, yres: %d, xres_virtual: %d, yres_virtual: %d\r\n", (int)var->xres, var->yres, (int)var->xres_virtual, (int)var->yres_virtual);

		idec_get_hor_timing((IDE_ID)id, &h_sync, &h_total, &h_valid_st, &h_valid_ed);

		var->left_margin = h_valid_st + 1;
		var->right_margin = h_total - h_valid_ed;
		var->hsync_len = h_sync + 1;

		DBG_IND("left_margin: %d, right_margin: %d, hsync_len: %d\r\n", (int)var->left_margin, (int)var->right_margin, (int)var->hsync_len);

		idec_get_ver_timing((IDE_ID)id, &v_sync, &v_total, &v_valid_oddst, &v_valid_odded, &v_valid_evenst, &v_valid_evened);

		var->upper_margin = v_valid_oddst + 1;
		var->lower_margin = v_total - v_valid_odded;
		var->vsync_len = v_sync + 1;

		DBG_IND("upper_margin: %d, lower_margin: %d, vsync_len: %d\r\n", (int)var->upper_margin, (int)var->lower_margin, (int)var->vsync_len);
	}

	if (fix != NULL) {
		fix->line_length = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs;
		DBG_IND("line_length: %d\r\n", (int)fix->line_length);
	}

	return 0;

}

int display_obj_get_video2_param(DISP_ID id, struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;
	UINT32 h_sync, h_total, h_valid_st, h_valid_ed;
	UINT32 v_sync, v_total, v_valid_oddst, v_valid_odded, v_valid_evenst, v_valid_evened;

	if (id > DISP_1){
		DBG_ERR("not support disp id %d !!\r\n", (int)id);
		return -1;
	}

	if (var != NULL) {
		p_disp_obj = disp_get_display_object(id);

		p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_GET_WINSIZE, &disp_lyr);
		var->xres = disp_lyr.SEL.GET_WINSIZE.ui_win_width;
		var->yres = disp_lyr.SEL.GET_WINSIZE.ui_win_height;

		p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);
		var->xres_virtual = disp_lyr.SEL.GET_BUFSIZE.ui_buf_width;
		var->yres_virtual = disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;

		var->xoffset = 0;
		var->yoffset = 0;

		DBG_IND("xres: %d, yres: %d, xres_virtual: %d, yres_virtual: %d\r\n", (int)var->xres, (int)var->yres, (int)var->xres_virtual, (int)var->yres_virtual);

		idec_get_hor_timing((IDE_ID)id, &h_sync, &h_total, &h_valid_st, &h_valid_ed);

		var->left_margin = h_valid_st + 1;
		var->right_margin = h_total - h_valid_ed;
		var->hsync_len = h_sync;

		DBG_IND("left_margin: %d, right_margin: %d, hsync_len: %d\r\n", (int)var->left_margin, (int)var->right_margin, (int)var->hsync_len);

		idec_get_ver_timing((IDE_ID)id, &v_sync, &v_total, &v_valid_oddst, &v_valid_odded, &v_valid_evenst, &v_valid_evened);

		var->upper_margin = v_valid_oddst + 1;
		var->lower_margin = v_total - v_valid_odded;
		var->vsync_len = v_sync;

		DBG_IND("upper_margin: %d, lower_margin: %d, vsync_len: %d\r\n", (int)var->upper_margin, (int)var->lower_margin, (int)var->vsync_len);
	}

	if (fix != NULL) {
		fix->line_length = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs;
		DBG_IND("line_length: %d\r\n", (int)fix->line_length);
	}

	return 0;
}

int display_obj_get_osd_param(DISP_ID id, struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix)
{
	PDISP_OBJ       p_disp_obj;
	DISPLAYER_PARAM disp_lyr;
	UINT32 h_sync, h_total, h_valid_st, h_valid_ed;
	UINT32 v_sync, v_total, v_valid_oddst, v_valid_odded, v_valid_evenst, v_valid_evened;
	IDE_COLOR_FORMAT format;

	p_disp_obj = disp_get_display_object(id);

	if (p_disp_obj == NULL) {
		DBG_ERR("get disp obj err\r\n");
		return -1;
	}

	p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_WINSIZE, &disp_lyr);
	if (var != NULL) {

		p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);
		var->xres = disp_lyr.SEL.GET_BUFSIZE.ui_buf_width;
		var->yres = disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;

		var->xres_virtual = disp_lyr.SEL.GET_BUFSIZE.ui_buf_width;
		var->yres_virtual = disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;

		var->xoffset = 0;
		var->yoffset = 0;

		DBG_IND("xres: %d, yres: %d, xres_virtual: %d, yres_virtual: %d\r\n", (int)var->xres, (int)var->yres, (int)var->xres_virtual, (int)var->yres_virtual);

		idec_get_hor_timing((IDE_ID)id, &h_sync, &h_total, &h_valid_st, &h_valid_ed);

		var->left_margin = h_valid_st + 1;
		var->right_margin = h_total - h_valid_ed;
		var->hsync_len = h_sync;

		DBG_IND("left_margin: %d, right_margin: %d, hsync_len: %d\r\n", (int)var->left_margin, (int)var->right_margin, (int)var->hsync_len);

		idec_get_ver_timing((IDE_ID)id, &v_sync, &v_total, &v_valid_oddst, &v_valid_odded, &v_valid_evenst, &v_valid_evened);

		var->upper_margin = v_valid_oddst + 1;
		var->lower_margin = v_total - v_valid_odded;
		var->vsync_len = v_sync;

		DBG_IND("upper_margin: %d, lower_margin: %d, vsync_len: %d\r\n", (int)var->upper_margin, (int)var->lower_margin, (int)var->vsync_len);

		idec_get_o1_fmt((IDE_ID)id, &format);

		if (format == COLOR_8_BIT) {
			var->bits_per_pixel = 8;

			var->red.offset = 16;
			var->red.length = 8;
			var->red.msb_right = 0;

			var->green.offset = 8;
			var->green.length = 8;
			var->green.msb_right = 0;

			var->blue.offset = 0;
			var->blue.length = 8;
			var->blue.msb_right = 0;

			var->transp.offset = 24;
			var->transp.length = 8;
			var->transp.msb_right = 0;
		} else if (format == COLOR_ARGB8565) {
			var->bits_per_pixel = 24;

			var->red.offset = 11;
			var->red.length = 5;
			var->red.msb_right = 0;

			var->green.offset = 5;
			var->green.length = 6;
			var->green.msb_right = 0;

			var->blue.offset = 0;
			var->blue.length = 5;
			var->blue.msb_right = 0;

			var->transp.offset = 16;
			var->transp.length = 8;
			var->transp.msb_right = 0;
		} else if (format == COLOR_ARGB8888) {
			var->bits_per_pixel = 32;

			var->red.offset = 16;
			var->red.length = 8;
			var->red.msb_right = 0;

			var->green.offset = 8;
			var->green.length = 8;
			var->green.msb_right = 0;

			var->blue.offset = 0;
			var->blue.length = 8;
			var->blue.msb_right = 0;

			var->transp.offset = 24;
			var->transp.length = 8;
			var->transp.msb_right = 0;
		} else if (format == COLOR_ARGB1555) {
			var->bits_per_pixel = 16;

			var->red.offset = 10;
			var->red.length = 5;
			var->red.msb_right = 0;

			var->green.offset = 5;
			var->green.length = 5;
			var->green.msb_right = 0;

			var->blue.offset = 0;
			var->blue.length = 5;
			var->blue.msb_right = 0;

			var->transp.offset = 15;
			var->transp.length = 1;
			var->transp.msb_right = 0;
		} else if (format == COLOR_ARGB4444) {
			var->bits_per_pixel = 16;

			var->red.offset = 8;
			var->red.length = 4;
			var->red.msb_right = 0;

			var->green.offset = 4;
			var->green.length = 4;
			var->green.msb_right = 0;

			var->blue.offset = 0;
			var->blue.length = 4;
			var->blue.msb_right = 0;

			var->transp.offset = 12;
			var->transp.length = 4;
			var->transp.msb_right = 0;
	}

	DBG_IND("bits_per_pixel: %d\r\n", (int)var->bits_per_pixel);
	DBG_IND("R => ofs: %d, length: %d, msb_right: %d\r\n", (int)var->red.offset, (int)var->red.length, (int)var->red.msb_right);
	DBG_IND("G => ofs: %d, length: %d, msb_right: %d\r\n", (int)var->green.offset, (int)var->green.length, (int)var->green.msb_right);
	DBG_IND("B => ofs: %d, length: %d, msb_right: %d\r\n", (int)var->blue.offset, (int)var->blue.length, (int)var->blue.msb_right);
	DBG_IND("A => ofs: %d, length: %d, msb_right: %d\r\n", (int)var->transp.offset, (int)var->transp.length, (int)var->transp.msb_right);
	}

	if (fix != NULL) {
		fix->line_length = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs;
		DBG_IND("line_length: %d\r\n", (int)fix->line_length);
	}
	return 0;
}

int display_obj_get_layer_support(DISP_ID id, DISPLAYER layer)
{
	int ret = 0;

	if (id == DISP_2) {
		if ((layer == DISPLAYER_OSD1) || (layer == DISPLAYER_VDO1))
			;
		else
			ret = -1;

	} else {
		if ((layer == DISPLAYER_OSD1) || (layer == DISPLAYER_VDO1) || (layer == DISPLAYER_VDO2) || (layer == DISPLAYER_FD))
			;
		else
			ret = -1;
	}

	return ret;
}
EXPORT_SYMBOL(display_obj_set_video1_blank);
EXPORT_SYMBOL(display_obj_set_video2_blank);
EXPORT_SYMBOL(display_obj_set_osd_blank);
EXPORT_SYMBOL(display_obj_set_osd_palette);
EXPORT_SYMBOL(display_obj_get_video1_buf_addr);
EXPORT_SYMBOL(display_obj_get_video2_buf_addr);
EXPORT_SYMBOL(display_obj_get_osd_buf_addr);
EXPORT_SYMBOL(display_obj_get_video1_param);
EXPORT_SYMBOL(display_obj_get_video2_param);
EXPORT_SYMBOL(display_obj_get_osd_param);
EXPORT_SYMBOL(display_obj_set_osd_addr);
EXPORT_SYMBOL(display_obj_get_layer_support);
#endif

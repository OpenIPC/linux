/*
    KDRV of Video output

    Exported KDRV of Video output

    @file       kdrv_vdoout.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "display.h"
#include "display_obj_dbg.h"
#include "kdrv_videoout/kdrv_vdoout.h"

KDRV_VDDO_DEV_TYPE last_active_dev[DISP_MAX] = {VDDO_DEV_NULL};

INT32 kdrv_vddo_open(UINT32 chip, UINT32 engine)
{
	PDISP_OBJ       p_disp_obj;

	if(!((engine >= KDRV_VDOOUT_ENGINE0) && (engine <= KDRV_VDOOUT_ENGINE1))) {
		DBG_ERR("Err engine\r\n");
		return -1;
	}
	p_disp_obj = disp_get_display_object((DISP_ID)(engine&0xFF));

	if (p_disp_obj == NULL) {
		DBG_ERR("disp get obj error\r\n");
		return -1;
	}

	if (!p_disp_obj->is_opened()) {
		p_disp_obj->open();
	}

	return 0;
}

INT32 kdrv_vddo_close(UINT32 chip, UINT32 engine)
{
	PDISP_OBJ       p_disp_obj;

	if(!((engine >= KDRV_VDOOUT_ENGINE0) && (engine <= KDRV_VDOOUT_ENGINE1))) {
		DBG_ERR("Err engine\r\n");
		return -1;
	}

	p_disp_obj = disp_get_display_object((DISP_ID)(engine&0xFF));

	if (p_disp_obj == NULL) {
		DBG_ERR("disp get obj error\r\n");
		return -1;
	}

	if (p_disp_obj->is_opened()) {
		p_disp_obj->close();
	}

	return 0;
}

INT32 kdrv_vddo_trigger(UINT32 handler, KDRV_CALLBACK_FUNC *p_cb_func)
{
	PDISP_OBJ       p_disp_obj;
	UINT32			engine;

	engine = KDRV_DEV_ID_ENGINE(handler);
	if(!((engine >= KDRV_VDOOUT_ENGINE0) && (engine <= KDRV_VDOOUT_ENGINE1))) {
		DBG_ERR("Err engine\r\n");
		return -1;
	}

	p_disp_obj = disp_get_display_object((DISP_ID)(engine&0xFF));

	if (p_disp_obj) {

		if (!p_disp_obj->is_opened()) {
			DBG_ERR("no open 0x%08X\r\n", (unsigned int)handler);
			return -1;
		}

		idec_set_callback((IDE_ID)(engine&0xFF), p_cb_func);
		if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
			p_disp_obj->load(TRUE);
		} else {
			p_disp_obj->load(FALSE);
		}

	} else {
		DBG_ERR("Err handler\r\n");
		return -1;
	}

	return 0;
}

INT32 kdrv_vddo_set(UINT32 handler, KDRV_VDDO_PARAM_ID id, VOID *p_param)
{
	PDISP_OBJ       p_disp_obj;
	DISPDEV_PARAM   disp_dev;
	DISPCTRL_PARAM  disp_ctrl;
	DISPLAYER_PARAM disp_lyr;
	KDRV_VDDO_DISPCTRL_PARAM *p_kdrv_disp_ctrl;
	KDRV_VDDO_DISPDEV_PARAM *p_kdrv_disp_dev;
	KDRV_VDDO_DISPLAYER_PARAM *p_kdrv_disp_layer;
	KDRV_CALLBACK_FUNC *p_cb_func;

	UINT32			group;
	UINT32			engine;

	engine = KDRV_DEV_ID_ENGINE(handler);
	if (!((engine >= KDRV_VDOOUT_ENGINE0) && (engine <= KDRV_VDOOUT_ENGINE1))) {
		DBG_ERR("Err engine\r\n");
		return -1;
	}

	p_disp_obj = disp_get_display_object((DISP_ID)(engine&0xFF));

	if (p_disp_obj) {

		if (!p_disp_obj->is_opened()) {
			DBG_ERR("no open 0x%08X\r\n", (unsigned int)handler);
			return -1;
		}

	} else {
		DBG_ERR("Err handler\r\n");
		return -1;
	}

	group = id & 0xFF000000;

	if (group == VDDO_DISPCTRL_BASE) {
		/*
			dispaly control group
		*/
		switch(id) {
		case VDDO_DISPCTRL_WAIT_FRM_END: {
			p_cb_func = p_param;
			idec_set_callback((IDE_ID)(engine&0xFF), p_cb_func);
			if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
				p_disp_obj->wait_frm_end(TRUE);
			} else {
				p_disp_obj->wait_frm_end(FALSE);
			}
		}
		break;
		case VDDO_DISPCTRL_WAIT_DRAM_OUT_DONE: {
			p_disp_obj->wait_out_dram_done();
		}
		break;
		case VDDO_DISPCTRL_ENABLE: {
			p_kdrv_disp_ctrl = p_param;
			disp_ctrl.SEL.SET_ENABLE.b_en = p_kdrv_disp_ctrl->SEL.KDRV_VDDO_ENABLE.en;
			p_disp_obj->disp_ctrl(DISPCTRL_SET_ENABLE, &disp_ctrl);
		}
		break;
		case VDDO_DISPCTRL_ALL_LYR_EN: {
			p_kdrv_disp_ctrl = p_param;
			disp_ctrl.SEL.SET_ALL_LYR_EN.b_en = p_kdrv_disp_ctrl->SEL.KDRV_VDDO_ALL_LYR_EN.en;
			disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr = (DISPLAYER)p_kdrv_disp_ctrl->SEL.KDRV_VDDO_ALL_LYR_EN.disp_lyr;
			p_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &disp_ctrl);
		}
		break;
		case VDDO_DISPCTRL_BACKGROUND: {
			p_kdrv_disp_ctrl = p_param;
			disp_ctrl.SEL.SET_BACKGROUND.ui_color_y = p_kdrv_disp_ctrl->SEL.KDRV_VDDO_BACKGROUND.color_y;
			disp_ctrl.SEL.SET_BACKGROUND.ui_color_cb = p_kdrv_disp_ctrl->SEL.KDRV_VDDO_BACKGROUND.color_cb;
			disp_ctrl.SEL.SET_BACKGROUND.ui_color_cr = p_kdrv_disp_ctrl->SEL.KDRV_VDDO_BACKGROUND.color_cr;
			p_disp_obj->disp_ctrl(DISPCTRL_SET_BACKGROUND, &disp_ctrl);
		}
		break;
		case VDDO_DISPCTRL_DMA_ABORT: {
			p_kdrv_disp_ctrl = p_param;
			disp_ctrl.SEL.SET_DMA_ABORT.b_en = p_kdrv_disp_ctrl->SEL.KDRV_VDDO_DMAABORT.en;
			p_disp_obj->disp_ctrl(DISPCTRL_SET_DMA_ABORT, &disp_ctrl);
		}
		break;
		default:
			DBG_ERR("Err param_id 0x%08X\r\n",id);
			return -1;
		}

	} else if (group == VDDO_DISPDEV_BASE) {
		/*
			display device group
		*/
		p_kdrv_disp_dev = p_param;
		switch(id) {
		case VDDO_DISPDEV_OPEN_DEVICE: {
			if ((p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id == VDDO_DEV_TV_NTSC) || (p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id == VDDO_DEV_TV_PAL) || (p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id == VDDO_DEV_HDMI)) {
				DBG_ERR("device ID 0x%08X not support\r\n",p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id);
				return -1;

			} else if (p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id == VDDO_DEV_PANEL) {
				if (p_disp_obj->dev_callback == NULL) {
					DBG_ERR("Panel Driver not loaded\r\n");

					disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_NULL;
					return -1;
				} else {
					disp_dev.SEL.HOOK_DEVICE_OBJECT.dev_id         = DISPDEV_ID_PANEL;
					disp_dev.SEL.HOOK_DEVICE_OBJECT.p_disp_dev_obj   = p_disp_obj->dev_callback();
					if (p_disp_obj->dev_ctrl(DISPDEV_HOOK_DEVICE_OBJECT, &disp_dev) != 0)
						return RE_INIT;

					disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_PANEL;
				}

			} else if (p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id == VDDO_DEV_MIPIDSI) {
				if (p_disp_obj->dev_callback == NULL) {
					DBG_ERR("Panel Driver(DSI) not loaded\r\n");

					disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_NULL;
					return -1;
				} else {
					disp_dev.SEL.HOOK_DEVICE_OBJECT.dev_id         = DISPDEV_ID_MIPIDSI;
					disp_dev.SEL.HOOK_DEVICE_OBJECT.p_disp_dev_obj   = p_disp_obj->dev_callback();
					if (p_disp_obj->dev_ctrl(DISPDEV_HOOK_DEVICE_OBJECT, &disp_dev) != 0)
						return RE_INIT;

					disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_MIPIDSI;
				}

			} else {
				DBG_ERR("Err device ID 0x%08X\r\n",p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id);
				last_active_dev[engine&0xff] = VDDO_DEV_NULL;
				return -1;
			}
			if (p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.user_data_en) {
				disp_dev.SEL.OPEN_DEVICE.user_data_en = p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.user_data_en;
				disp_dev.SEL.OPEN_DEVICE.user_data = p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.user_data;
			} else {
				disp_dev.SEL.OPEN_DEVICE.user_data_en = 0;
				disp_dev.SEL.OPEN_DEVICE.user_data = 0;
			}
			if (p_disp_obj->dev_ctrl(DISPDEV_OPEN_DEVICE, &disp_dev) != 0)
				return -1;

			last_active_dev[engine&0xff] = p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id;

		}
		break;
		case VDDO_DISPDEV_CLOSE_DEVICE: {
			last_active_dev[engine&0xff] = VDDO_DEV_NULL;
			p_disp_obj->dev_ctrl(DISPDEV_CLOSE_DEVICE, NULL);
		}
		break;
		case VDDO_DISPDEV_REG_IF: {
			if (p_disp_obj->dev_callback == NULL) {
				DBG_ERR("Panel Driver not loaded\r\n");

				disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_NULL;
				return -1;
			} else {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = (DISPDEV_LCDCTRL)p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.lcd_ctrl;
				disp_dev.SEL.SET_REG_IF.ui_sif_ch = (SIF_CH)p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_sif_ch;
				disp_dev.SEL.SET_REG_IF.ui_gpio_sen = p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_gpio_sen;
				disp_dev.SEL.SET_REG_IF.ui_gpio_clk = p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_gpio_clk;
				disp_dev.SEL.SET_REG_IF.ui_gpio_data = p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_gpio_data;
				p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);
			}
		}
		break;
		case VDDO_DISPDEV_HDMIMODE: {
			DBG_ERR("not support hdmi\r\n");
			return -1;
			//disp_dev.SEL.SET_HDMIMODE.video_id = (HDMI_VIDEOID)p_kdrv_disp_dev->SEL.KDRV_VDDO_HDMIMODE.video_id;
			//disp_dev.SEL.SET_HDMIMODE.audio_id = (HDMI_AUDIOFMT)p_kdrv_disp_dev->SEL.KDRV_VDDO_HDMIMODE.audio_id;
			//p_disp_obj->dev_ctrl(DISPDEV_SET_HDMIMODE, &disp_dev);
		}
		break;
		case VDDO_DISPDEV_ROTATE: {
			disp_dev.SEL.SET_ROTATE.rot = (DISPDEV_LCD_ROTATE)p_kdrv_disp_dev->SEL.KDRV_VDDO_ROTATE.rot;
			p_disp_obj->dev_ctrl(DISPDEV_SET_ROTATE, &disp_dev);
		}
		break;
		case VDDO_DISPDEV_LCDMODE: {
			disp_dev.SEL.SET_LCDMODE.mode = (DISPDEV_LCDMODE)p_kdrv_disp_dev->SEL.KDRV_VDDO_LCDMODE.mode;
			p_disp_obj->dev_ctrl(DISPDEV_SET_LCDMODE, &disp_dev);
		}
		break;
		case VDDO_DISPDEV_POWERDOWN: {
			disp_dev.SEL.SET_POWERDOWN.b_power_down = p_kdrv_disp_dev->SEL.KDRV_VDDO_POWERDOWN.power_down;
			p_disp_obj->dev_ctrl(DISPDEV_SET_POWERDOWN, &disp_dev);
		}
		break;
		case VDDO_DISPDEV_OUTPUT_DRAM: {
			disp_dev.SEL.SET_OUTPUT_DRAM_PAR.y_addr = p_kdrv_disp_dev->SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.y_addr;
			disp_dev.SEL.SET_OUTPUT_DRAM_PAR.uv_addr = p_kdrv_disp_dev->SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.uv_addr;
			disp_dev.SEL.SET_OUTPUT_DRAM_PAR.output_width = p_kdrv_disp_dev->SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.output_width;
			disp_dev.SEL.SET_OUTPUT_DRAM_PAR.output_height = p_kdrv_disp_dev->SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.output_height;
			disp_dev.SEL.SET_OUTPUT_DRAM_PAR.output_line_ofs= p_kdrv_disp_dev->SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.output_line_ofs;
			disp_dev.SEL.SET_OUTPUT_DRAM_PAR.is_yuv422 = p_kdrv_disp_dev->SEL.KDRV_VDDO_OUTPUT_DRAM_PAR.is_yuv422;
			p_disp_obj->dev_ctrl(DISPDEV_SET_OUTPUT_DRAM, &disp_dev);
		}
		break;
		case VDDO_DISPDEV_TVFULL: {
			disp_dev.SEL.SET_TVFULL.b_en_full = p_kdrv_disp_dev->SEL.KDRV_VDDO_TVFULL.en_full;
			p_disp_obj->dev_ctrl(DISPDEV_SET_TVFULL, &disp_dev);
		}
		break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n",id);
			return -1;
		}
	} else if (group == VDDO_DISPLAYER_BASE) {
		/*
			display layer group
		*/
		p_kdrv_disp_layer = p_param;
		switch(id) {
		case VDDO_DISPLAYER_ENABLE: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.layer > VDDO_DISPLAYER_VDO2) {
				disp_lyr.SEL.SET_FDEN.b_en = p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.en;
				disp_lyr.SEL.SET_FDEN.fd_num = (DISPFDNUM)p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.fd_num;
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_FD, DISPLAYER_OP_SET_FDEN, &disp_lyr);
			} else {
				disp_lyr.SEL.SET_ENABLE.b_en = p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.en;
				p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.layer, DISPLAYER_OP_SET_ENABLE, &disp_lyr);
			}
		}
		break;
		case VDDO_DISPLAYER_BUFADDR: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer == VDDO_DISPLAYER_OSD1) {
				disp_lyr.SEL.SET_OSDBUFADDR.buf_sel = DISPBUFADR_0;
				disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_buf0 = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_y;
				disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_alpha0 = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_cbcr;
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_OSDBUFADDR, &disp_lyr);
			} else if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer == VDDO_DISPLAYER_VDO1) {
				disp_lyr.SEL.SET_VDOBUFADDR.buf_sel = DISPBUFADR_0;
				disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0 = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_y;
				disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_cbcr;
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_VDOBUFADDR, &disp_lyr);
			} else if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer == VDDO_DISPLAYER_VDO2) {
				disp_lyr.SEL.SET_VDOBUFADDR.buf_sel = DISPBUFADR_0;
				disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0 = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_y;
				disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_cbcr;
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_SET_VDOBUFADDR, &disp_lyr);
			} else {
				DBG_WRN("no buffer needed for layer 0x%08X\r\n",p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer);
			}
		}
		break;
		case VDDO_DISPLAYER_OUTDIR: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.layer <= VDDO_DISPLAYER_VDO2) {
				disp_lyr.SEL.SET_OUTDIR.buf_out_dir = (DISPOUTDIR)p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.buf_out_dir;
				p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.layer, DISPLAYER_OP_SET_OUTDIR, &disp_lyr);
			} else {
				DBG_WRN("layer %d not support\r\n", p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.layer);
			}
		}
		break;
		case VDDO_DISPLAYER_PALETTE: {
			disp_lyr.SEL.SET_PALETTE.ui_start = p_kdrv_disp_layer->SEL.KDRV_VDDO_PALETTE.start;
			disp_lyr.SEL.SET_PALETTE.ui_number = p_kdrv_disp_layer->SEL.KDRV_VDDO_PALETTE.number;
			disp_lyr.SEL.SET_PALETTE.p_pale_entry = p_kdrv_disp_layer->SEL.KDRV_VDDO_PALETTE.p_pale_entry;
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_PALETTE, &disp_lyr);
		}
		break;
		case VDDO_DISPLAYER_BLEND: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.layer == VDDO_DISPLAYER_OSD1) {
				disp_lyr.SEL.SET_BLEND.type = (DISPBLENDTYPE)p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.type;
				disp_lyr.SEL.SET_BLEND.ui_global_alpha = p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.global_alpha;
				disp_lyr.SEL.SET_BLEND.b_global_alpha5 = p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.global_alpha5;
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_BLEND, &disp_lyr);
			} else {
				disp_lyr.SEL.SET_BLEND.type = (DISPBLENDTYPE)p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.type;
				disp_lyr.SEL.SET_BLEND.ui_global_alpha = p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.global_alpha;
				p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.layer, DISPLAYER_OP_SET_BLEND, &disp_lyr);
			}
		}
		break;
		case VDDO_DISPLAYER_VDOCOLORKEY_SRC: {
			disp_lyr.SEL.SET_VDOCOLORKEY_SRC.ck_src = (DISPVDOCKCMPSOURCE)p_kdrv_disp_layer->SEL.KDRV_VDDO_VDOCOLORKEY_SRC.colorkey_src;
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_VDOCOLORKEY_SRC, &disp_lyr);
		}
		break;
		case VDDO_DISPLAYER_COLORKEY: {
			disp_lyr.SEL.SET_COLORKEY.ck_op = (DISPCKOP)p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.colorkey_op;
			disp_lyr.SEL.SET_COLORKEY.ui_ck_y = p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.key_y;
			disp_lyr.SEL.SET_COLORKEY.ui_ck_cb = p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.key_cb;
			disp_lyr.SEL.SET_COLORKEY.ui_ck_cr = p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.key_cr;
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.layer, DISPLAYER_OP_SET_COLORKEY, &disp_lyr);
		}
		break;
		case VDDO_DISPLAYER_FD_CONFIG: {
			disp_lyr.SEL.SET_FDSIZE.fd_num = (DISPFDNUM)p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_num;
			disp_lyr.SEL.SET_FDSIZE.ui_fdx = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_x;
			disp_lyr.SEL.SET_FDSIZE.ui_fdy = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_y;
			disp_lyr.SEL.SET_FDSIZE.ui_fdw = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_w;
			disp_lyr.SEL.SET_FDSIZE.ui_fdh = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_h;
			disp_lyr.SEL.SET_FDSIZE.ui_fd_bord_w = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_bord_w;
			disp_lyr.SEL.SET_FDSIZE.ui_fd_bord_h = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_bord_h;
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_FD, DISPLAYER_OP_SET_FDSIZE, &disp_lyr);

			disp_lyr.SEL.SET_FDCOLOR.fd_num = (DISPFDNUM)p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_num;
			disp_lyr.SEL.SET_FDCOLOR.ui_fd_cr_y = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_color_y;
			disp_lyr.SEL.SET_FDCOLOR.ui_fd_cr_cb = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_color_cb;
			disp_lyr.SEL.SET_FDCOLOR.ui_fd_cr_cr = p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_color_cr;
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_FD, DISPLAYER_OP_SET_FDCOLOR, &disp_lyr);
		}
		break;
		case VDDO_DISPLAYER_OP_BUFXY: {
			disp_lyr.SEL.SET_BUFXY.ui_buf_ofs_x = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFXY.buf_ofs_x;
			disp_lyr.SEL.SET_BUFXY.ui_buf_ofs_y = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFXY.buf_ofs_y;
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFXY.layer, DISPLAYER_OP_SET_BUFXY, &disp_lyr);
		}
		break;
		case VDDO_DISPLAYER_BUFWINSIZE: {
			disp_lyr.SEL.SET_MODE.buf_format = (DISPBUFFORMAT)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.format;
			disp_lyr.SEL.SET_MODE.buf_mode = DISPBUFMODE_BUFFER_REPEAT;
			disp_lyr.SEL.SET_MODE.buf_number = DISPBUFNUM_1;
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.layer, DISPLAYER_OP_SET_MODE, &disp_lyr);
			disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_width = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.buf_width;
			disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_height = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.buf_height;
			disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_line_ofs = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs;
			disp_lyr.SEL.SET_BUFWINSIZE.ui_win_width = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_width;
			disp_lyr.SEL.SET_BUFWINSIZE.ui_win_height = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_height;
			disp_lyr.SEL.SET_BUFWINSIZE.i_win_ofs_x = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x;
			disp_lyr.SEL.SET_BUFWINSIZE.i_win_ofs_y = p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y;
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.layer, DISPLAYER_OP_SET_BUFWINSIZE, &disp_lyr);
		}
		break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n",id);
			return -1;
		}
	} else {
		DBG_ERR("Err param_id 0x%08X\r\n",id);
		return -1;
	}

	return 0;
}

INT32 kdrv_vddo_get(UINT32 handler, KDRV_VDDO_PARAM_ID id, VOID *p_param)
{
	PDISP_OBJ       p_disp_obj;
	DISPDEV_PARAM   disp_dev;
	DISPCTRL_PARAM  disp_ctrl;
	DISPLAYER_PARAM disp_lyr;
	KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl;
	KDRV_VDDO_DISPDEV_PARAM *p_kdrv_disp_dev;
	KDRV_VDDO_DISPLAYER_PARAM *p_kdrv_disp_layer;

	UINT32			group;
	UINT32			engine;

	engine = KDRV_DEV_ID_ENGINE(handler);
	if (!((engine >= KDRV_VDOOUT_ENGINE0) && (engine <= KDRV_VDOOUT_ENGINE1))) {
		DBG_ERR("Err engine\r\n");
		return -1;
	}

	if (id != VDDO_DISPDEV_HDMI_ABI) {
		p_disp_obj = disp_get_display_object((DISP_ID)(engine&0xFF));

		if (p_disp_obj) {

			if (!p_disp_obj->is_opened()) {
				DBG_ERR("no open 0x%08X\r\n", (unsigned int)handler);
				return -1;
			}

		} else {
			DBG_ERR("Err handler\r\n");
			return -1;
		}
	}
	group = id & 0xFF000000;

	if (group == VDDO_DISPCTRL_BASE) {
		/*
			dispaly control group
		*/
		switch(id) {
		case VDDO_DISPCTRL_ENABLE: {
			p_disp_obj->disp_ctrl(DISPCTRL_GET_ENABLE, &disp_ctrl);
			kdrv_disp_ctrl.SEL.KDRV_VDDO_ENABLE.en = disp_ctrl.SEL.GET_ENABLE.b_en;
		}
		break;
		case VDDO_DISPCTRL_ALL_LYR_EN: {
			p_disp_obj->disp_ctrl(DISPCTRL_GET_ALL_LYR_EN, &disp_ctrl);
			kdrv_disp_ctrl.SEL.KDRV_VDDO_ALL_LYR_EN.disp_lyr = (KDRV_VDDO_DISPLAYER)disp_ctrl.SEL.GET_ALL_LYR_EN.disp_lyr;
		}
		break;
		case VDDO_DISPCTRL_BACKGROUND: {
			p_disp_obj->disp_ctrl(DISPCTRL_GET_BACKGROUND, &disp_ctrl);
			kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_y = disp_ctrl.SEL.GET_BACKGROUND.ui_color_y;
			kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cb = disp_ctrl.SEL.GET_BACKGROUND.ui_color_cb;
			kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cr = disp_ctrl.SEL.GET_BACKGROUND.ui_color_cr;
		}
		break;
		default:
			DBG_ERR("Err param_id 0x%08X\r\n",id);
			return -1;
		}
		p_param = &kdrv_disp_ctrl;

	} else if (group == VDDO_DISPDEV_BASE) {
		/*
			display device group
		*/
		p_kdrv_disp_dev = p_param;
		switch(id) {
		case VDDO_DISPDEV_OPEN_DEVICE: {
			//p_disp_obj->dev_ctrl(DISPDEV_GET_ACT_DEVICE, &disp_dev);
			p_kdrv_disp_dev->SEL.KDRV_VDDO_OPEN_DEVICE.dev_id = last_active_dev[engine&0xff];
		}
		break;
		case VDDO_DISPDEV_REG_IF: {
			p_disp_obj->dev_ctrl(DISPDEV_GET_REG_IF, &disp_dev);
			p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.lcd_ctrl = (KDRV_VDDO_DISPDEV_LCDCTRL)disp_dev.SEL.GET_REG_IF.lcd_ctrl;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_sif_ch = (KDRV_VDDO_SIF_CH)disp_dev.SEL.GET_REG_IF.ui_sif_ch;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_gpio_sen = disp_dev.SEL.GET_REG_IF.ui_gpio_sen;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_gpio_clk = disp_dev.SEL.GET_REG_IF.ui_gpio_clk;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_REG_IF.ui_gpio_data = disp_dev.SEL.GET_REG_IF.ui_gpio_data;

		}
		break;
		case VDDO_DISPDEV_HDMIMODE: {
			p_disp_obj->dev_ctrl(DISPDEV_GET_HDMIMODE, &disp_dev);
			p_kdrv_disp_dev->SEL.KDRV_VDDO_HDMIMODE.video_id = (KDRV_VDDO_HDMI)disp_dev.SEL.GET_HDMIMODE.video_id;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_HDMIMODE.audio_id = (KDRV_VDDO_HDMI_AUDIOFMT)disp_dev.SEL.GET_HDMIMODE.audio_id;

		}
		break;
		case VDDO_DISPDEV_LCDMODE: {
			p_disp_obj->dev_ctrl(DISPDEV_GET_LCDMODE, &disp_dev);
			p_kdrv_disp_dev->SEL.KDRV_VDDO_LCDMODE.mode = (KDRV_VDDO_DISPDEV_LCDMODE)disp_dev.SEL.GET_LCDMODE.mode;

		}
		break;
		case VDDO_DISPDEV_POWERDOWN: {
			p_disp_obj->dev_ctrl(DISPDEV_GET_POWERDOWN, &disp_dev);
			p_kdrv_disp_dev->SEL.KDRV_VDDO_POWERDOWN.power_down = disp_dev.SEL.GET_POWERDOWN.b_power_down;

		}
		break;
		case VDDO_DISPDEV_DISPSIZE: {
			disp_dev.SEL.GET_PREDISPSIZE.dev_id = p_kdrv_disp_dev->SEL.KDRV_VDDO_DISPSIZE.dev_id;
			p_disp_obj->dev_ctrl(DISPDEV_GET_PREDISPSIZE, &disp_dev);
			p_kdrv_disp_dev->SEL.KDRV_VDDO_DISPSIZE.win_width = disp_dev.SEL.GET_PREDISPSIZE.ui_win_width;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_DISPSIZE.win_height = disp_dev.SEL.GET_PREDISPSIZE.ui_win_height;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_DISPSIZE.buf_width = disp_dev.SEL.GET_PREDISPSIZE.ui_buf_width;
			p_kdrv_disp_dev->SEL.KDRV_VDDO_DISPSIZE.buf_height = disp_dev.SEL.GET_PREDISPSIZE.ui_buf_height;
		}
		break;

		case VDDO_DISPDEV_HDMI_ABI: {

			DBG_ERR("HDMI not support\r\n");
			return -1;
		}
		break;
		default:
			DBG_ERR("Err param_id 0x%08X\r\n",id);
			return -1;
		}
	} else if (group == VDDO_DISPLAYER_BASE) {
		/*
			display layer group
		*/
		p_kdrv_disp_layer = p_param;
		switch(id) {
		case VDDO_DISPLAYER_ENABLE: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.layer > VDDO_DISPLAYER_VDO2) {
				disp_lyr.SEL.GET_FDEN.fd_num = (DISPFDNUM)p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.fd_num;
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_FD, DISPLAYER_OP_GET_FDEN, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.en = disp_lyr.SEL.GET_FDEN.b_en;

			} else {
				p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.layer, DISPLAYER_OP_GET_ENABLE, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_ENABLE.en = disp_lyr.SEL.GET_ENABLE.b_en;

			}
		}
		break;
		case VDDO_DISPLAYER_BUFADDR: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer == VDDO_DISPLAYER_OSD1) {
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_OSDBUFADDR, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_y = disp_lyr.SEL.GET_OSDBUFADDR.ui_addr_buf0;
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_cbcr = disp_lyr.SEL.GET_OSDBUFADDR.ui_addr_alpha0;
			} else if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer == VDDO_DISPLAYER_VDO1) {
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_VDOBUFADDR, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_y = disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_y0;
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_cbcr = disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_cb0;
			} else if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer == VDDO_DISPLAYER_VDO2) {
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_GET_VDOBUFADDR, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_y = disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_y0;
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.addr_cbcr = disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_cb0;
			} else {
				DBG_WRN("no buffer used by layer 0x%08X\r\n",p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFADDR.layer);
			}
		}
		break;
		case VDDO_DISPLAYER_OUTDIR: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.layer <= VDDO_DISPLAYER_VDO2) {
				p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.layer, DISPLAYER_OP_GET_OUTDIR, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.buf_out_dir = disp_lyr.SEL.GET_OUTDIR.buf_out_dir;
			} else {
				DBG_WRN("layer %d not support\r\n", p_kdrv_disp_layer->SEL.KDRV_VDDO_OUTDIR.layer);
			}
		}
		break;
		case VDDO_DISPLAYER_PALETTE: {
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_PALETTE, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_PALETTE.start = disp_lyr.SEL.GET_PALETTE.ui_start;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_PALETTE.number = disp_lyr.SEL.GET_PALETTE.ui_number;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_PALETTE.p_pale_entry = disp_lyr.SEL.GET_PALETTE.p_pale_entry;
		}
		break;
		case VDDO_DISPLAYER_BLEND: {
			if (p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.layer == VDDO_DISPLAYER_OSD1) {
				p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_BLEND, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.type = (KDRV_VDDO_DISPBLENDTYPE)disp_lyr.SEL.GET_BLEND.type;
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.global_alpha = disp_lyr.SEL.GET_BLEND.ui_global_alpha;
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.global_alpha5 = disp_lyr.SEL.GET_BLEND.b_global_alpha5;
			} else {
				p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.layer, DISPLAYER_OP_GET_BLEND, &disp_lyr);
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.type = (KDRV_VDDO_DISPBLENDTYPE)disp_lyr.SEL.GET_BLEND.type;
				p_kdrv_disp_layer->SEL.KDRV_VDDO_BLEND.global_alpha = disp_lyr.SEL.GET_BLEND.ui_global_alpha;
			}
		}
		break;
		case VDDO_DISPLAYER_VDOCOLORKEY_SRC: {
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_VDOCOLORKEY_SRC, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_VDOCOLORKEY_SRC.colorkey_src = (KDRV_VDDO_DISPVDOCKCMPSOURCE)disp_lyr.SEL.GET_VDOCOLORKEY_SRC.ck_src;
		}
		break;
		case VDDO_DISPLAYER_COLORKEY: {
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.layer, DISPLAYER_OP_GET_COLORKEY, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.colorkey_op = (KDRV_VDDO_DISPCKOP)disp_lyr.SEL.GET_COLORKEY.ck_op;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.key_y = disp_lyr.SEL.GET_COLORKEY.ui_ck_y;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.key_cb = disp_lyr.SEL.GET_COLORKEY.ui_ck_cb;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_COLORKEY.key_cr = disp_lyr.SEL.GET_COLORKEY.ui_ck_cr;
		}
		break;
		case VDDO_DISPLAYER_FD_CONFIG: {
			disp_lyr.SEL.GET_FDSIZE.fd_num = (DISPFDNUM)p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_num;
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_FD, DISPLAYER_OP_GET_FDSIZE, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_x = disp_lyr.SEL.GET_FDSIZE.ui_fdx;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_y = disp_lyr.SEL.GET_FDSIZE.ui_fdy;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_w = disp_lyr.SEL.GET_FDSIZE.ui_fdw;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_h = disp_lyr.SEL.GET_FDSIZE.ui_fdh;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_bord_w = disp_lyr.SEL.GET_FDSIZE.ui_fd_bord_w;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_bord_h = disp_lyr.SEL.GET_FDSIZE.ui_fd_bord_h;

			disp_lyr.SEL.GET_FDCOLOR.fd_num = (DISPFDNUM)p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_num;
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_FD, DISPLAYER_OP_GET_FDCOLOR, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_color_y = disp_lyr.SEL.GET_FDCOLOR.ui_fd_cr_y;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_color_cb = disp_lyr.SEL.GET_FDCOLOR.ui_fd_cr_cb;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_FDSIZE.fd_color_cr = disp_lyr.SEL.GET_FDCOLOR.ui_fd_cr_cr;
		}
		break;
		case VDDO_DISPLAYER_OP_BUFXY: {
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFXY.layer, DISPLAYER_OP_GET_BUFXY, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFXY.buf_ofs_x = disp_lyr.SEL.GET_BUFXY.ui_buf_ofs_x;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFXY.buf_ofs_y = disp_lyr.SEL.GET_BUFXY.ui_buf_ofs_y;
		}
		break;
		case VDDO_DISPLAYER_BUFWINSIZE: {
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.layer, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.buf_width = disp_lyr.SEL.GET_BUFSIZE.ui_buf_width;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.buf_height = disp_lyr.SEL.GET_BUFSIZE.ui_buf_height;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs;
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.layer, DISPLAYER_OP_GET_WINSIZE, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_width = disp_lyr.SEL.GET_WINSIZE.ui_win_width;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_height = disp_lyr.SEL.GET_WINSIZE.ui_win_height;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = disp_lyr.SEL.GET_WINSIZE.i_win_ofs_x;
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = disp_lyr.SEL.GET_WINSIZE.i_win_ofs_y;
			p_disp_obj->disp_lyr_ctrl((DISPLAYER)p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.layer, DISPLAYER_OP_GET_MODE, &disp_lyr);
			p_kdrv_disp_layer->SEL.KDRV_VDDO_BUFWINSIZE.format = (KDRV_VDDO_DISPBUFFORMAT)disp_lyr.SEL.GET_MODE.buf_format;
		}
		break;

    	case VDDO_DISPLAYER_CST_FROM_RGB_TO_YUV: {
            disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.r_to_y = p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.r_to_y;
            disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.g_to_u = p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.g_to_u;
            disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.b_to_v = p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.b_to_v;
            disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.alpha = p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.alpha;
			p_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_CST_FROM_RGB_TO_YUV, &disp_lyr);
            p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.r_to_y = disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.r_to_y;
            p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.g_to_u = disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.g_to_u;
            p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.b_to_v = disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.b_to_v;
            p_kdrv_disp_layer->SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.alpha = disp_lyr.SEL.GET_CST_OF_RGB_TO_YUV.alpha;
    	}
    	break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n",id);
			return -1;
		}

	} else {
		DBG_ERR("Err param_id 0x%08X\r\n",id);
		return -1;
	}


	return 0;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(kdrv_vddo_open);
EXPORT_SYMBOL(kdrv_vddo_close);
EXPORT_SYMBOL(kdrv_vddo_trigger);
EXPORT_SYMBOL(kdrv_vddo_set);
EXPORT_SYMBOL(kdrv_vddo_get);
#endif


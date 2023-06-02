/*
    Display device object Golden Sample for driving Memory interface panel

    @file       dispdev_gsinfmi.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dispdev_ifdsi.h"

#ifdef __KERNEL__
static DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   spin_unlock_irqrestore(&my_lock, flags)
#else
#if defined(__FREERTOS)
#include <kwrap/spinlock.h>
static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)
//#define DISPDEV_IFDSI_NT35410  1
//#define DISPDEV_IFDSI_NT35510  2
#define DISPDEV_IFDSI_TYPE     DISPDEV_IFDSI_NT35510
#else
#define DISPDEV_IFDSI_TYPE     DISPDEV_IFDSI_NT35510
#endif
#endif

#define PORCH_ALIGN  20
static BOOL b_te_on = FALSE;
static BOOL b_te_ext_pin = FALSE;

static void     disp_device_init(T_LCD_PARAM *p_mode);

static ER       dispdev_open_if_dsi(void);
static ER       dispdev_close_if_dsi(void);
static ER       dispdev_set_device_rotate(DISPDEV_LCD_ROTATE rot);
static void     dispdev_write_lcd_reg(UINT32 ui_addr, UINT32 ui_value);
static UINT32   dispdev_read_lcd_reg(UINT32 ui_addr);
static void     dispdev_set_if_dsi_ioctrl(FP p_io_ctrl_func);
static ER       dispdev_get_lcd_dsi_size(PDISPDEV_GET_PRESIZE t_lcd_size);

#define MAX_DSI_CMD         200
#define IDE_SRC_CLK_480     5242880
static UINT8    dsi_cmd_param[MAX_DSI_CMD];

#define LOG_DSI_CFG_MSG DISABLE

#define DSI_READ_TEST	DISABLE


#if LOG_DSI_CFG_MSG == ENABLE
#define DSI_CFG_MSG(...)      DBG_IND(__VA_ARGS__)
#define DSI_ERR_MSG(...)      DBG_ERR(__VA_ARGS__)
#define DSI_IND_MSG(...)      DBG_IND(__VA_ARGS__)
#define DSI_CMD_MSG(...)
#else
#define DSI_CFG_MSG(...)
#define DSI_ERR_MSG(...)
#define DSI_IND_MSG(...)
#define DSI_CMD_MSG(...)
#endif

/*
    Exporting display device object
*/
//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
static DISPDEV_OBJ     disp_dev_ifdsi_obj_lcd1 = { dispdev_open_if_dsi, dispdev_close_if_dsi, dispdev_set_device_rotate, dispdev_write_lcd_reg, dispdev_read_lcd_reg, dispdev_get_lcd_dsi_size, dispdev_set_if_dsi_ioctrl, {NULL, NULL, NULL} };
static volatile DISPDEV_IOCTRL  p_ifdsi_io_control1;
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
//static DISPDEV_OBJ     disp_dev_ifdsi_obj_lcd2 = { dispdev_open_if_dsi, dispdev_close_if_dsi, dispdev_set_device_rotate, dispdev_write_lcd_reg, dispdev_read_lcd_reg, dispdev_get_lcd_dsi_size, dispdev_set_if_dsi_ioctrl, {NULL, NULL, NULL} };
//static volatile DISPDEV_IOCTRL  p_ifdsi_io_control2 = NULL;
//#endif

#ifndef __KERNEL__
static UINT32   disp_dev_src_freq;
#endif

#define _CFG_USE_LP_ 1
#if _CFG_USE_LP_
static UINT8 test_pat2[210];
static UINT32 ecc_poly[6] = {0xF12CB7, 0xF2555B, 0x749A6D, 0xB8E38E, 0xDF03F0, 0xEFFC00};

static UINT32 dsi_gen_ecc(UINT32 ui_pkt_hdr)
{
	UINT32  csi_pkt_hdr;
	UINT32  i, j;
	BOOL    tmp;

	csi_pkt_hdr = 0;

	ui_pkt_hdr &= 0xFFFFFF;


	for (j = 0; j < 6; j++) {
		tmp = 0;
		for (i = 0; i < 24; i++) {
			tmp ^= ((ui_pkt_hdr >> i) & 0x1 & (ecc_poly[j] >> i));
		}
#if 1
		csi_pkt_hdr |= (tmp << (/*24+*/j));
#else
		csi_pkt_hdr |= (tmp << (31 - j));
#endif
	}

	//emu_msg(("0x%08X\r\n",csi_pkt_hdr));
	return csi_pkt_hdr;
}

#define DSI_CRCPOLY 0x8408
static UINT16 dsi_gen_crc(char *data_p, UINT16 length)
{
	unsigned char   i;
	unsigned int    data;
	unsigned int    crc = 0xffff;

	if (length == 0) {
		return (UINT16)(crc);
	}

	do {
		for (i = 0, data = (unsigned int)0xff & *data_p++; i < 8; i++, data >>= 1) {
			if ((crc & 0x0001) ^ (data & 0x0001)) {
				crc = (crc >> 1) ^ DSI_CRCPOLY;
			} else {
				crc >>= 1;
			}
		}

	} while (--length);

	return (UINT16)((crc) & 0xFFFF);

}


static ER dsi_lp_write_cmd(DSI_LANESEL ui_data_lane, UINT32 ui_dsc_cmd, UINT8 *param, UINT32 param_no, DSI_PACKET_TYPE pkt_type, BOOL b_eot)
{
	UINT32 ui_pkt_hd = 0;
	//UINT32 ui_packet_header;
	UINT32 ui_trans_data;
	UINT32 ui_param_cnt;
	UINT32 ui_ecc_code;

	DBG_DUMP("[%d]Low power Cmd => 0x%02x\r\n", (int)pkt_type, (unsigned int)ui_dsc_cmd);
	if (pkt_type == DSI_SHORT_PACKET) {
		if (param_no >= 2) {
			DBG_ERR("Low power DT param exceed 1 => %d\r\n", (int)param_no);
			return E_SYS;
		}

		if (param_no == 0) {
			ui_pkt_hd = DATA_TYPE_SHORT_WRITE_NO_PARAM;
		} else {
			ui_pkt_hd = DATA_TYPE_SHORT_WRITE_1_PARAM;
		}

#if (DISPDEV_IFDSI_TYPE == DISPDEV_IFDSI_NT35410)
		if (ui_dsc_cmd == 0x32) {
			ui_pkt_hd = 0x32;
		}
#elif (DISPDEV_IFDSI_TYPE == DISPDEV_IFDSI_NT35510)
		//if(ui_dsc_cmd == 0x35 && b_te_on == FALSE)
		//    ui_pkt_hd = 0x32;
#endif
		ui_pkt_hd |= (ui_dsc_cmd << 8);
		for (ui_param_cnt = 0; ui_param_cnt < param_no; ui_param_cnt++) {
			ui_pkt_hd |= (param[ui_param_cnt] << 16);
		}
		//DSI_IND_MSG("emu_dsiLPWriteCmd = 0x%08x\r\n", (uint)ui_pkt_hd);

		ui_ecc_code = (dsi_gen_ecc(ui_pkt_hd) & 0xFF);

		//EMU_DSI_LOG_MSG("emu_dsiLPWriteCmd ui_ecc_code = 0x%08x\r\n", (uint)ui_ecc_code);

		ui_pkt_hd |= (ui_ecc_code << 24);

		DSI_IND_MSG("emu_dsiLPWriteCmd = 0x%08x\r\n", (uint)ui_pkt_hd);

		dsi_set_escape_entry(ui_data_lane, (UINT32)ULPS_ENTRY_CMD_LPDT, FALSE);

		for (ui_param_cnt = 0; ui_param_cnt < 3; ui_param_cnt++) {
			ui_trans_data = ((ui_pkt_hd >> (ui_param_cnt * 8)) & 0xFF);
			dsi_set_escape_transmission(ui_data_lane, ui_trans_data, DSI_SET_ESC_NOT_STOP);
		}
		ui_trans_data = ((ui_pkt_hd >> 24) & 0xFF);

		if (b_eot == FALSE) {
			dsi_set_escape_transmission(ui_data_lane, ui_trans_data, DSI_SET_ESC_STOP_WITH_ESC_CMD);
		} else {
			dsi_set_escape_transmission(ui_data_lane, ui_trans_data, DSI_SET_ESC_NOT_STOP);
			//keep sending EoT packet
			dsi_set_escape_transmission(ui_data_lane, 0x08, DSI_SET_ESC_NOT_STOP);
			dsi_set_escape_transmission(ui_data_lane, 0x0F, DSI_SET_ESC_NOT_STOP);
			dsi_set_escape_transmission(ui_data_lane, 0x0F, DSI_SET_ESC_NOT_STOP);
			ui_pkt_hd = 0x000F0F08;
			ui_ecc_code = (dsi_gen_ecc(ui_pkt_hd) & 0xFF);
			//debug_msg("EoT ECC = 0x%02x\r\n", (uint)ui_ecc_code);
			dsi_set_escape_transmission(ui_data_lane, ui_ecc_code, DSI_SET_ESC_STOP_WITH_ESC_CMD);
		}
	} else {
		ui_pkt_hd = DATA_TYPE_LONG_WRITE;
		ui_pkt_hd |= ((param_no + 1) << 8);
		ui_ecc_code = (dsi_gen_ecc(ui_pkt_hd) & 0xFF);

		//EMU_DSI_LOG_MSG("emu_dsiLPWriteCmd ui_ecc_code = 0x%08x\r\n", (uint)ui_ecc_code);

		ui_pkt_hd |= (ui_ecc_code << 24);

		DSI_IND_MSG("emu_dsiLPWriteCmd Packet header = 0x%08x\r\n", (uint)ui_pkt_hd);

		DSI_IND_MSG("   [ DI] [0x%02x]\r\n", (uint)(ui_pkt_hd & 0xFF));
		DSI_IND_MSG("   [WC0] [0x%02x]\r\n", (uint)((ui_pkt_hd & 0xFF00) >> 8));
		DSI_IND_MSG("   [WC1] [0x%02x]\r\n", (uint)((ui_pkt_hd & 0xFF0000) >> 16));
		DSI_IND_MSG("   [ECC] [0x%02x]\r\n", (uint)((ui_pkt_hd & 0xFF000000) >> 24));
		dsi_set_escape_entry(ui_data_lane, (UINT32)ULPS_ENTRY_CMD_LPDT, FALSE);

		//1. Packet Header
		//("Start send long packet via Low power => ");
		for (ui_param_cnt = 0; ui_param_cnt <= 3; ui_param_cnt++) {
			ui_trans_data = ((ui_pkt_hd >> (ui_param_cnt * 8)) & 0xFF);
			dsi_set_escape_transmission(ui_data_lane, ui_trans_data, DSI_SET_ESC_NOT_STOP);
			//EMU_DSI_LOG_MSG("[0x%02x] ", (uint)ui_trans_data);
		}

		//2. DCS command always be 1st byte of packet data in long packet
		dsi_set_escape_transmission(ui_data_lane, ui_dsc_cmd, DSI_SET_ESC_NOT_STOP);

		DSI_IND_MSG(" [Data00] : [0x%02x]\r\n", (uint)ui_dsc_cmd);

		test_pat2[0] = ui_dsc_cmd;

		//3. Send remain data 1 ~ n
		for (ui_param_cnt = 0; ui_param_cnt < param_no; ui_param_cnt++) {
			dsi_set_escape_transmission(ui_data_lane, param[ui_param_cnt], DSI_SET_ESC_NOT_STOP);
			DSI_IND_MSG(" [Data%02d] : [0x%02x]\r\n", (uint)ui_param_cnt + 1, param[ui_param_cnt]);
			test_pat2[ui_param_cnt + 1] = param[ui_param_cnt];
		}

		//4. Final generate 2 bytes Checksum data
		ui_trans_data = dsi_gen_crc((CHAR *)&test_pat2[0], (UINT16)(param_no + 1));

		DSI_IND_MSG("    CRC   : [0x%04x]\r\n", (uint)ui_trans_data);

		//debug_msg("Generate CRC = 0x%04x\r\n", ui_trans_data);

		dsi_set_escape_transmission(ui_data_lane, (ui_trans_data & 0xFF), DSI_SET_ESC_NOT_STOP);

		dsi_set_escape_transmission(ui_data_lane, ((ui_trans_data >> 8) & 0xFF), DSI_SET_ESC_STOP_WITH_ESC_CMD);
	}

	return E_OK;
}

static ER dsi_lp_read_cmd(DSI_LANESEL ui_data_lane, UINT32 ui_dsc_cmd, UINT8 * param, UINT32 param_no, DSI_PACKET_TYPE pkt_type, BOOL b_eot)
{
    UINT32 ui_pkt_hd = 0;
    //UINT32 uiPacketHeader;
    UINT32 ui_trans_data;
    UINT32 ui_param_cnt;
    UINT32 ui_ecc_code;

    DSI_IND_MSG("[%d]Low power Cmd => 0x%02x\r\n", (int)pkt_type, (uint)ui_dsc_cmd);
    if(pkt_type == DSI_SHORT_PACKET)
    {
        if(param_no >= 2)
        {
            DSI_ERR_MSG("Low power DT param exceed 1 => %d\r\n", (int)param_no);
            return E_SYS;
        }

        if(param_no == 0)
            ui_pkt_hd = DATA_TYPE_SHORT_READ_NO_PARAM;

        ui_pkt_hd |= (ui_dsc_cmd<<8);
        for(ui_param_cnt = 0; ui_param_cnt < param_no; ui_param_cnt++)
        {
            ui_pkt_hd |= (param[ui_param_cnt] << 16);
        }
        //DSI_IND_MSG("emu_dsiLPWriteCmd = 0x%08x\r\n", (uint)ui_pkt_hd);

        ui_ecc_code = (dsi_gen_ecc(ui_pkt_hd) & 0xFF);

        //DSI_IND_MSG("emu_dsiLPWriteCmd uiECCCode = 0x%08x\r\n", (uint)ui_ecc_code);

        ui_pkt_hd |= (ui_ecc_code << 24);

        DSI_IND_MSG("dsi_LPWReadCmd = 0x%08x\r\n", (uint)ui_pkt_hd);

        dsi_set_escape_entry(ui_data_lane, (UINT32)ULPS_ENTRY_CMD_LPDT, FALSE);

        for(ui_param_cnt = 0; ui_param_cnt < 3; ui_param_cnt++)
        {
            ui_trans_data = ((ui_pkt_hd >> (ui_param_cnt * 8))& 0xFF);
            dsi_set_escape_transmission(ui_data_lane, ui_trans_data, DSI_SET_ESC_NOT_STOP);
        }
        ui_trans_data = ((ui_pkt_hd >> 24)& 0xFF);

        if(b_eot == FALSE)
        {
            dsi_set_escape_transmission(ui_data_lane, ui_trans_data, DSI_SET_ESC_STOP_WITH_ESC_CMD);
        }
        else
        {
            dsi_set_escape_transmission(ui_data_lane, ui_trans_data, DSI_SET_ESC_NOT_STOP);
            //keep sending EoT packet
            dsi_set_escape_transmission(ui_data_lane, 0x08, DSI_SET_ESC_NOT_STOP);
            dsi_set_escape_transmission(ui_data_lane, 0x0F, DSI_SET_ESC_NOT_STOP);
            dsi_set_escape_transmission(ui_data_lane, 0x0F, DSI_SET_ESC_NOT_STOP);
            ui_pkt_hd = 0x000F0F08;
            ui_ecc_code = (dsi_gen_ecc(ui_pkt_hd) & 0xFF);
            //debug_msg("EoT ECC = 0x%02x\r\n", (uint)ui_ecc_code);
            dsi_set_escape_transmission(ui_data_lane, ui_ecc_code, DSI_SET_ESC_STOP_WITH_ESC_CMD);
        }
    }
#if 0
    else
    {
        uiPKTHD = DATA_TYPE_LONG_WRITE;
        uiPKTHD |= ((paramNo + 1)<<8);
        uiECCCode = (dsi_GenECC(uiPKTHD) & 0xFF);

        //EMU_DSI_LOG_MSG("emu_dsiLPWriteCmd uiECCCode = 0x%08x\r\n", uiECCCode);

        uiPKTHD |= (uiECCCode << 24);

        DSI_IND_MSG("emu_dsiLPWriteCmd Packet header = 0x%08x\r\n", uiPKTHD);

        DSI_IND_MSG("   [ DI] [0x%02x]\r\n", (uiPKTHD & 0xFF));
        DSI_IND_MSG("   [WC0] [0x%02x]\r\n", ((uiPKTHD & 0xFF00)>>8));
        DSI_IND_MSG("   [WC1] [0x%02x]\r\n", ((uiPKTHD & 0xFF0000)>>16));
        DSI_IND_MSG("   [ECC] [0x%02x]\r\n", ((uiPKTHD & 0xFF000000)>>24));
        dsi_setEscapeEntry(uiDataLane, (UINT32)ULPS_ENTRY_CMD_LPDT, FALSE);

        //1. Packet Header
        //("Start send long packet via Low power => ");
        for(uiParamCnt = 0; uiParamCnt <= 3; uiParamCnt++)
        {
            uiTransData = ((uiPKTHD >> (uiParamCnt * 8))& 0xFF);
            dsi_setEscapeTransmission(uiDataLane, uiTransData, DSI_SET_ESC_NOT_STOP);
            //EMU_DSI_LOG_MSG("[0x%02x] ", uiTransData);
        }

        //2. DCS command always be 1st byte of packet data in long packet
        dsi_setEscapeTransmission(uiDataLane, uiDCSCmd, DSI_SET_ESC_NOT_STOP);

        DSI_IND_MSG(" [Data00] : [0x%02x]\r\n", uiDCSCmd);

        testPat2[0] = uiDCSCmd;

        //3. Send remain data 1 ~ n
        for(uiParamCnt = 0; uiParamCnt < paramNo; uiParamCnt++)
        {
            dsi_setEscapeTransmission(uiDataLane, param[uiParamCnt], DSI_SET_ESC_NOT_STOP);
            DSI_IND_MSG(" [Data%02d] : [0x%02x]\r\n", uiParamCnt+1, param[uiParamCnt]);
            testPat2[uiParamCnt+1] = param[uiParamCnt];
        }

        //4. Final generate 2 bytes Checksum data
        uiTransData = dsi_GenCRC((CHAR *)&testPat2[0], (UINT16)(paramNo+1));

        DSI_IND_MSG("    CRC   : [0x%04x]\r\n", uiTransData);

        //debug_msg("Generate CRC = 0x%04x\r\n", uiTransData);

        dsi_setEscapeTransmission(uiDataLane, (uiTransData & 0xFF), DSI_SET_ESC_NOT_STOP);

        dsi_setEscapeTransmission(uiDataLane, ((uiTransData >> 8)&0xFF), DSI_SET_ESC_STOP_WITH_ESC_CMD);
    }
#endif
    return E_OK;
}

#endif

static void disp_device_init(T_LCD_PARAM *p_mode)
{
	DSI_CMD_RW_CTRL_PARAM   dcs_context;
	DSI_CMD_CTRL_PARAM      dcs_ctrl_context;
	UINT32                  i, data_cnt;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control1;
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
//	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control2;
//#endif
	BOOL                    b_eopt = TRUE;

	b_te_on = FALSE;
	b_te_ext_pin = FALSE;
	dcs_context.b_bta_en = FALSE;
	dcs_context.b_bta_only = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_GET_PANEL_ADJUST, &dev_io_ctrl);

	dcs_ctrl_context.ui_param_cnt = 0;

	if (dev_io_ctrl.SEL.GET_PANEL_ADJUST.pfp_adjust != NULL) {
		dev_io_ctrl.SEL.GET_PANEL_ADJUST.pfp_adjust();
	} else if (p_mode->p_cmd_queue != NULL) {
		for (i = 0; i < p_mode->n_cmd;) {
			if (p_mode->p_cmd_queue[i].ui_address == CMDDELAY_MS) {
				dispdev_platform_delay_ms(p_mode->p_cmd_queue[i].ui_value);
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDDELAY_US) {
				dispdev_platform_delay_us(p_mode->p_cmd_queue[i].ui_value);
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDBTA) {
				DSI_CFG_MSG(" Issue BTA\r\n");
				dcs_context.b_bta_en = TRUE;
				dcs_context.b_bta_only = TRUE;
				dcs_context.ui_cmd_no = 1;
				dcs_context.ui_sram_ofs = 0x80;
				dcs_context.b_eot_en = FALSE;

				dcs_ctrl_context.ui_data_type = DATA_TYPE_SHORT_READ_NO_PARAM;
				dcs_ctrl_context.ui_packet_type = DSI_SHORT_PACKET;
				dcs_ctrl_context.ui_virtual_channel = 0;
				dcs_ctrl_context.ui_dcs_cmd = 0x00;
				dcs_ctrl_context.ui_param = NULL;
				dcs_ctrl_context.ui_param_cnt = 0;

				dcs_context.p_dsi_cmd_ctx = &dcs_ctrl_context;
				dsi_set_hs_dcs_command(&dcs_context);
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDTEON) {
				if (dsi_set_config(DSI_CONFIG_ID_SYNC_EN, (BOOL)p_mode->p_cmd_queue[i].ui_value) != E_OK) {
					DSI_ERR_MSG(" Configure pixel TE fail\r\n");
				} else {
					DSI_CFG_MSG(" Configure pixel TE %d success\r\n", (BOOL)p_mode->p_cmd_queue[i].ui_value);
				}
				b_te_on = TRUE;
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDTEEXTPIN) {
				if (dsi_set_config(DSI_CONFIG_ID_SYNC_SRC, (BOOL)p_mode->p_cmd_queue[i].ui_value) != E_OK) {
					DSI_ERR_MSG(" Configure pixel TE ext pin fail\r\n");
				} else {
					DSI_CFG_MSG(" Configure pixel TE ext pin %d success\r\n", (BOOL)p_mode->p_cmd_queue[i].ui_value);
				}
				b_te_ext_pin = TRUE;
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDTEONEACHFRAME) {
				if (dsi_set_config(DSI_CONFIG_ID_SYNC_WITH_SETTEON, (BOOL)p_mode->p_cmd_queue[i].ui_value) != E_OK) {
					DSI_ERR_MSG(" Configure pixel TE ON each frame fail\r\n");
				} else {
					DSI_CFG_MSG(" Configure pixel TE TE ON each frame %d success\r\n", (BOOL)p_mode->p_cmd_queue[i].ui_value);
				}
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDTEONEACHFRAMERTY) {
				if (dsi_set_config(DSI_CONFIG_ID_SYNC_WITH_SETTEON_RTY, (BOOL)p_mode->p_cmd_queue[i].ui_value) != E_OK) {
					DSI_ERR_MSG(" Configure pixel TE ON each frame retry fail\r\n");
				} else {
					DSI_CFG_MSG(" Configure pixel TE TE ON each frame %d retry success\r\n", (BOOL)p_mode->p_cmd_queue[i].ui_value);
				}
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDTEONEACHFRAMERTYTWICE) {
				if (dsi_set_config(DSI_CONFIG_ID_SYNC_WITH_SETTEON_RTY_TWICEBTA, (BOOL)p_mode->p_cmd_queue[i].ui_value) != E_OK) {
					DSI_ERR_MSG(" Configure pixel TE ON each frame retry twice fail\r\n");
				} else {
					DSI_CFG_MSG(" Configure pixel TE TE ON each frame %d retry twice success\r\n", (BOOL)p_mode->p_cmd_queue[i].ui_value);
				}
				i++;
			} else if (p_mode->p_cmd_queue[i].ui_address == DSICMD_CMD) {
				dcs_context.ui_cmd_no = 1;
				dcs_context.b_eot_en = FALSE;
				dcs_context.b_bta_only = FALSE;
				dcs_ctrl_context.ui_param_cnt = 0;
				dcs_ctrl_context.ui_virtual_channel = 0;
				dcs_context.ui_sram_ofs = 128;
				dcs_ctrl_context.ui_dcs_cmd = p_mode->p_cmd_queue[i].ui_value;

				DSI_CMD_MSG("     [Cmd]=[0x%02x]\r\n", (uint)p_mode->p_cmd_queue[i].ui_value);

				for (data_cnt = i + 1; data_cnt < p_mode->n_cmd; data_cnt++) {
					/*if (p_mode->p_cmd_queue[data_cnt].ui_address == DSICMD_DATA) {
						//data cnt++;
						if (p_mode->p_cmd_queue[data_cnt].ui_value > 0xFF) {
							DSI_ERR_MSG("MIPI command exceed 1 bytes error 0x%08x\r\n", p_mode->p_cmd_queue[data_cnt].ui_value);
						} else {
							DSI_CFG_MSG("  [Data%02d]=[0x%02x]\r\n", (data_cnt - (i + 1)), p_mode->p_cmd_queue[data_cnt].ui_value);
						}
						if (dcs_ctrl_context.ui_param_cnt > (MAX_DSI_CMD - 1)) {
							DSI_ERR_MSG("DSI data exceed [%03d] %03d\r\n", MAX_DSI_CMD, dcs_ctrl_context.ui_param_cnt);
							break;
						}
						dsi_cmd_param[dcs_ctrl_context.ui_param_cnt] = p_mode->p_cmd_queue[data_cnt].ui_value;
						dcs_ctrl_context.ui_param_cnt++;
					} else {
						break;
					}*/
					if (p_mode->p_cmd_queue[data_cnt].ui_address != DSICMD_DATA) {
						break;
					}
					if (p_mode->p_cmd_queue[data_cnt].ui_value > 0xFF) {
						DSI_ERR_MSG("MIPI command exceed 1 bytes error 0x%08x\r\n", (uint)p_mode->p_cmd_queue[data_cnt].ui_value);
					} else {
						DSI_CFG_MSG("  [Data%02d]=[0x%02x]\r\n", (int)(data_cnt - (i + 1)), (uint)p_mode->p_cmd_queue[data_cnt].ui_value);
					}
					if (dcs_ctrl_context.ui_param_cnt > (MAX_DSI_CMD - 1)) {
						DSI_ERR_MSG("DSI data exceed [%03d] %03d\r\n", MAX_DSI_CMD, (int)dcs_ctrl_context.ui_param_cnt);
						break;
					}
					dsi_cmd_param[dcs_ctrl_context.ui_param_cnt] = p_mode->p_cmd_queue[data_cnt].ui_value;
					dcs_ctrl_context.ui_param_cnt++;

				}
				if (dcs_ctrl_context.ui_param_cnt == 0) {
					dcs_ctrl_context.ui_packet_type = DSI_SHORT_PACKET;
					dcs_ctrl_context.ui_data_type = DATA_TYPE_SHORT_WRITE_NO_PARAM;
					dcs_ctrl_context.ui_param = NULL;
#if _CFG_USE_LP_
					dsi_cmd_param[0] = 0x0;
					dsi_lp_write_cmd(DSI_DATA_LANE_0, dcs_ctrl_context.ui_dcs_cmd, dsi_cmd_param, 0, DSI_SHORT_PACKET, b_eopt);
#endif
				} else if (dcs_ctrl_context.ui_param_cnt == 1) {
					dcs_ctrl_context.ui_packet_type = DSI_SHORT_PACKET;
					dcs_ctrl_context.ui_data_type = DATA_TYPE_SHORT_WRITE_1_PARAM;
					dcs_ctrl_context.ui_param = (UINT8 *)&dsi_cmd_param;
#if _CFG_USE_LP_
					dsi_lp_write_cmd(DSI_DATA_LANE_0, dcs_ctrl_context.ui_dcs_cmd, dsi_cmd_param, 1, DSI_SHORT_PACKET, b_eopt);
#endif
				} else {
					dcs_ctrl_context.ui_packet_type = DSI_LONG_PACKET;
					dcs_ctrl_context.ui_data_type = DATA_TYPE_LONG_WRITE;
					dcs_ctrl_context.ui_param = (UINT8 *)&dsi_cmd_param[0];
#if _CFG_USE_LP_
					dsi_lp_write_cmd(DSI_DATA_LANE_0, dcs_ctrl_context.ui_dcs_cmd, dsi_cmd_param, dcs_ctrl_context.ui_param_cnt, DSI_LONG_PACKET, b_eopt);
#endif
				}
#if !_CFG_USE_LP_
				dcs_context.p_dsi_cmd_ctx = &dcs_ctrl_context;
				dsi_set_hs_dcs_command(&dcs_context);
#endif

				//dispdev_write_lcd_reg(p_mode->p_cmd_queue[i].ui_address, p_mode->p_cmd_queue[i].ui_value);
				i += (dcs_ctrl_context.ui_param_cnt + 1);
			} else {
				//dispdev_write_lcd_reg(p_mode->p_cmd_queue[i].ui_address, p_mode->p_cmd_queue[i].ui_value);
				i += (dcs_ctrl_context.ui_param_cnt + 1);
			}
		}
	}
}


UINT32 dispdev_readDsiReg(UINT32 addr);
UINT32 dispdev_readDsiReg(UINT32 addr)
{
	UINT32 test;
	dispdev_read_lcd_reg(addr);
	test = dsi_get_config(DSI_CONFIG_ID_BTA_VALUE);
	DBG_DUMP("DSI read = 0x%08x\r\n",(int)test);
	return test;
}
/*
    Display Device API of open LCD with DSI interface

    Display Device API of open LCD with DSI interface

    @param[in] p_devif   Please refer to dispdevctrl.h for details.

    @return
     - @b E_PAR:    Error p_devif content.
     - @b E_NOSPT:  Driver without supporting the specified LCD mode.
     - @b E_OK:     LCD Open done and success.
*/
//static UINT8 testPat[300];

static ER dispdev_open_if_dsi(void)
{
	//UINT32                  uiErrorRpt;
	T_LCD_PARAM              *p_mode      = NULL;
	T_LCD_PARAM              *p_config_mode = NULL;
	UINT32                  mode_number;
	DSI_CONFIG              dsi_config;
	PINMUX_FUNC_ID          pin_func_id;
//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control1;
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
//	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control2;
//#endif
#ifndef __KERNEL__
	PINMUX_LCDINIT          lcd_mode;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#endif

	DSI_PIXEL_FORMATSEL     ui_pixel_fmt          = DSI_PIXEL_RGB_565;
	DSI_VIDEOPKT_TYPESEL    ui_video_sync_mode     = DSI_VIDEOPKT_TYPE_SYNC_EVENT;
	DSI_PIXPKT_MODESEL      ui_pixel_pkt_mode_sel   = DSI_PIXMODE_VIDEO_MODE;
	UINT32                  ui_lane_cnt;



	UINT32                  ui_hact_byte_count;
	UINT32                  ui_hfp_byte_count = 0;
	UINT32                  ui_hbp_byte_count = 0;
	UINT32                  ui_bllp_byte_count;
	UINT32                  ui_hsa, ui_vsa;

#ifndef __KERNEL__
	UINT32                  cur_freq;

	PLL_ID                  src_pll;
#endif
	//UINT32                  THS_TRAIL;
	//UINT32                  THS_EXIT;
	//UINT32                  THS_CLK_POST;
	//UINT32                  THS_CLK_TRAIL;
	//UINT32                  THS_CLK_PREPARE;
	//UINT32                  THS_CLK_ZERO;
	//UINT32                  THS_CLK_PRE;
	//UINT32                  TLPX;
	//UINT32                  THS_PREPARE;
	//UINT32                  THS_ZERO;
	//These two param always be 1
	//UINT32                  THS_SYNC = 1;
	//UINT32                  TLP_transition_time = 1;
	//UINT32                  TLP_TOTAL;
	//UINT32                  uiVIDEData;
	//UINT32                  uiVtotal_Sub_Vactive;

	dispanl_debug(("^GopenIFDsi START\r\n"));

	ui_hsa = 0;

	if (p_disp_dev_control == NULL) {
		return E_PAR;
	}

	//
	//  Sortig the SysInit Config mode support
	//

//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
//	pin_func_id = PINMUX_FUNC_ID_LCD2;
//#endif
#ifndef __KERNEL__
	lcd_mode = pinmux_get_dispmode(pin_func_id);
#endif
	p_config_mode = dispdev_get_config_mode_dsi(&mode_number);

#if 0
	if ((p_config_mode != NULL) && (mode_number > 0)) {
		for (i = 0; i < mode_number; i++) {
			if (p_config_mode[i].panel.lcd_mode == lcd_mode) {
				p_mode = (T_LCD_PARAM *)&p_config_mode[i];
				break;
			}
		}
	}
#else
	if (p_config_mode != NULL) {
		p_mode = (T_LCD_PARAM *)&p_config_mode[0];
	}
#endif
	if (p_mode == NULL) {
#ifndef __KERNEL__
		DBG_ERR("lcd_mode=%d not support\r\n", lcd_mode);
#endif
		return E_NOSPT;
	}

	if (dsi_is_opened()) {
		dsi_close();
	}

	dsi_open();

#if 0
//#if _FPGA_EMULATION_
#if defined (_NVT_FPGA_)
	dsi_config.dsi_src_clk = 24.0;
#else
	// Max clock = 480
	dsi_config.dsi_src_clk = 480.0;
#endif
#endif
	dispdev_set_dsi_config(&dsi_config);

#if 1
	if (dsi_set_config(DSI_CONFIG_ID_MODE, DSI_MODE_MANUAL_MODE) != E_OK) {
		DSI_ERR_MSG("Configure manual mode fail\r\n");
	}


	if ((p_mode->panel.lcd_mode >= PINMUX_DSI_1_LANE_CMD_MODE_RGB565) && (p_mode->panel.lcd_mode <= PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB888)) {
		if (p_mode->panel.lcd_mode <= PINMUX_DSI_1_LANE_CMD_MODE_RGB888) {
			ui_pixel_pkt_mode_sel = DSI_PIXMODE_COMMAND_MODE;
			DSI_CFG_MSG(" panel is command mode\r\n");
		} else {
			ui_pixel_pkt_mode_sel = DSI_PIXMODE_VIDEO_MODE;
			DSI_CFG_MSG(" panel is video mode\r\n");
		}
		DSI_CFG_MSG(" panel is one lane configuration\r\n");
		dsi_set_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_0);
		ui_lane_cnt = 1;
	}

	// Config & Enable display device, send manual mode command
	disp_device_init(p_mode);

#if (DSI_READ_TEST == ENABLE)
	{
		UINT32 reg;
		DBG_DUMP("read 0x0C:\r\n");
		reg = dispdev_readDsiReg(0x0C);
		DBG_DUMP("read reg= 0x%08x\r\n", (uint)reg);
	}

#endif

#if 0
	if (dsi_set_config(DSI_CFG_PIXEL_FMT, ui_pixel_fmt) != E_OK) {
		DBG_DUMP("Configure pixel format fail\r\n");
	}
#endif
	DSI_CFG_MSG("=====ide configuration====== \r\n");
	DSI_CFG_MSG("=VSyncBackPorch      = %04d=\r\n", (int)p_mode->panel.ui_vsync_back_porch_odd);
	DSI_CFG_MSG("=VSyncTotalPeriod    = %04d=\r\n", (int)p_mode->panel.ui_vsync_total_period);
	DSI_CFG_MSG("=HSyncTotalPeriod    = %04d=\r\n", (int)p_mode->panel.ui_hsync_total_period);
	DSI_CFG_MSG("=HSyncBackPorch      = %04d=\r\n", (int)p_mode->panel.ui_hsync_back_porch);
	DSI_CFG_MSG("=HSyncActivePeriod   = %04d=\r\n", (int)p_mode->panel.ui_hsync_active_period);
	DSI_CFG_MSG("=VSyncActivePeriod   = %04d=\r\n", (int)p_mode->panel.ui_vsync_active_period);
	DSI_CFG_MSG("=====ide configuration====== \r\n");


#endif
	//dsi_set_config(DSI_CFG_RGB_SWAP, TRUE);



	if ((p_mode->panel.lcd_mode >= PINMUX_DSI_2_LANE_CMD_MODE_RGB565) && (p_mode->panel.lcd_mode <= PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888)) {
		if (p_mode->panel.lcd_mode <= PINMUX_DSI_2_LANE_CMD_MODE_RGB888) {
			ui_pixel_pkt_mode_sel = DSI_PIXMODE_COMMAND_MODE;
			DSI_CFG_MSG(" panel is command mode\r\n");
		} else {
			ui_pixel_pkt_mode_sel = DSI_PIXMODE_VIDEO_MODE;
			DSI_CFG_MSG(" panel is video mode\r\n");
		}
		DSI_CFG_MSG(" panel is two lane configuration\r\n");
		dsi_set_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_1);
		ui_lane_cnt = 2;
	}

	if ((p_mode->panel.lcd_mode >= PINMUX_DSI_4_LANE_CMD_MODE_RGB565) && (p_mode->panel.lcd_mode <= PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888)) {
		if (p_mode->panel.lcd_mode <= PINMUX_DSI_4_LANE_CMD_MODE_RGB888) {
			ui_pixel_pkt_mode_sel = DSI_PIXMODE_COMMAND_MODE;
			DSI_CFG_MSG(" panel is command mode\r\n");
		} else {
			ui_pixel_pkt_mode_sel = DSI_PIXMODE_VIDEO_MODE;
			DSI_CFG_MSG(" panel is video mode\r\n");
		}
		DSI_CFG_MSG(" panel is four lane configuration\r\n");
		dsi_set_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_3);
		ui_lane_cnt = 4;
	}

	switch (p_mode->panel.lcd_mode) {
	case PINMUX_DSI_1_LANE_CMD_MODE_RGB565:
	case PINMUX_DSI_2_LANE_CMD_MODE_RGB565:
	case PINMUX_DSI_4_LANE_CMD_MODE_RGB565:
	case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB565:
	case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB565:
	case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB565:
	case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB565:
	case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB565:
	case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB565:
		ui_pixel_fmt = DSI_PIXEL_RGB_565;
		DSI_CFG_MSG(" panel is RGB565 configuration\r\n");
		break;

	case PINMUX_DSI_1_LANE_CMD_MODE_RGB666P:
	case PINMUX_DSI_2_LANE_CMD_MODE_RGB666P:
	case PINMUX_DSI_4_LANE_CMD_MODE_RGB666P:
	case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P:
	case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P:
	case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666P:
	case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666P:
	case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666P:
	case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666P:
		if (dsi_get_config(DSI_CONFIG_ID_CHIP_VER) == DSI_DRV_CHIPVER_A) {
			if ((p_mode->panel.ui_hsync_active_period % 16) != 0) {
				DBG_ERR("RGB666P must be 16-pixel alignment\r\n");
			}
		}
		ui_pixel_fmt = DSI_PIXEL_RGB_666_PACKETED;
		DSI_CFG_MSG(" panel is RGB666P configuration\r\n");
		break;

	case PINMUX_DSI_1_LANE_CMD_MODE_RGB666L:
	case PINMUX_DSI_2_LANE_CMD_MODE_RGB666L:
	case PINMUX_DSI_4_LANE_CMD_MODE_RGB666L:
	case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L:
	case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L:
	case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666L:
	case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666L:
	case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666L:
	case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666L:
		ui_pixel_fmt = DSI_PIXEL_RGB_666_LOOSELY;
		DSI_CFG_MSG(" panel is RGB666L configuration\r\n");
		break;

	case PINMUX_DSI_1_LANE_CMD_MODE_RGB888:
	case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB888:
	case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB888:
	case PINMUX_DSI_2_LANE_CMD_MODE_RGB888:
	case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB888:
	case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888:
	case PINMUX_DSI_4_LANE_CMD_MODE_RGB888:
	case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB888:
	case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888:
		ui_pixel_fmt = DSI_PIXEL_RGB_888;
		DSI_CFG_MSG(" panel is RGB888 configuration\r\n");
		break;
	default:
		DSI_CFG_MSG(" Unknow DSI pixel format mode\r\n");
		break;
	}

	if (ui_pixel_pkt_mode_sel == DSI_PIXMODE_VIDEO_MODE) {
		switch (p_mode->panel.lcd_mode) {
		case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB565:
		case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB565:
		case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB565:
		case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P:
		case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666P:
		case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666P:
		case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L:
		case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666L:
		case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666L:
		case PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB888:
		case PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB888:
		case PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB888:
			ui_video_sync_mode = DSI_VIDEOPKT_TYPE_SYNC_PULSE;
			DSI_CFG_MSG(" panel is sync pulse configuration\r\n");
			break;

		case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB565:
		case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB565:
		case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB565:
		case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P:
		case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666P:
		case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666P:
		case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L:
		case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666L:
		case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666L:
		case PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB888:
		case PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888:
		case PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888:
			ui_video_sync_mode = DSI_VIDEOPKT_TYPE_SYNC_EVENT;
			DSI_CFG_MSG(" panel is sync event configuration\r\n");
			break;
		default:
			DSI_CFG_MSG(" Unknow DSI sync mode\r\n");
			break;
		}
	}

	if (dsi_set_config(DSI_CONFIG_ID_PIXEL_FMT, ui_pixel_fmt) != E_OK) {
		DSI_ERR_MSG(" Configure pixel format fail\r\n");
	} else {
		DSI_CFG_MSG(" Configure pixel format 0x%02x success\r\n", (uint)ui_pixel_fmt);
	}


	if (ui_pixel_pkt_mode_sel == DSI_PIXMODE_COMMAND_MODE) {
		dsi_set_config(DSI_CONFIG_ID_BLANK_CTRL, TRUE);

		dsi_set_config(DSI_CONFIG_ID_INTER_PKT_LP, TRUE);
#if 0
		if (dsi_set_config(DSI_CONFIG_ID_SYNC_EN, p_mode->panel.b_te_on) != E_OK) {
			DSI_ERR_MSG(" Configure pixel TE fail\r\n");
		} else {
			DSI_CFG_MSG(" Configure pixel TE %d success\r\n", (int) p_mode->panel.b_te_on);
		}

		if (dsi_set_config(DSI_CONFIG_ID_SYNC_SRC, p_mode->panel.b_ext_pin) != E_OK) {
			DSI_ERR_MSG(" Configure pixel TE sync src\r\n");
		} else {
			DSI_CFG_MSG(" Configure pixel TE sync src %d success\r\n", (int) p_mode->panel.b_ext_pin);
		}

		if (dsi_set_config(DSI_CONFIG_ID_SYNC_WITH_SETTEON, p_mode->panel.b_te_on_each_frame) != E_OK) {
			DSI_ERR_MSG(" Configure pixel TE sync each frame\r\n");
		} else {
			DSI_CFG_MSG(" Configure pixel TE sync each frame %d success\r\n", (int) p_mode->panel.b_te_on_each_frame);
		}
#endif
		if (ui_pixel_fmt == DSI_PIXEL_RGB_666_PACKETED) {
			ui_hact_byte_count = (p_mode->panel.ui_hsync_active_period * 9) / 4 ;
			ui_hfp_byte_count = (p_mode->panel.ui_hsync_back_porch * 9) / 4;
			ui_hbp_byte_count = (p_mode->panel.ui_hsync_back_porch * 9) / 4;
			ui_bllp_byte_count = (p_mode->panel.ui_hsync_total_period * 9) / 4;

			//ui_bllp_byte_count = p_mode->panel.ui_hsync_total_period * 2;
		} else if (ui_pixel_fmt == DSI_PIXEL_RGB_565) {
			ui_hact_byte_count = (p_mode->panel.ui_hsync_active_period) * 2;
			ui_hfp_byte_count = p_mode->panel.ui_hsync_back_porch * 2;
			ui_hbp_byte_count = p_mode->panel.ui_hsync_back_porch * 2;
			//ui_bllp_byte_count = p_mode->panel.ui_hsync_total_period * 2;

			ui_bllp_byte_count = ui_hact_byte_count + ui_hfp_byte_count + ui_hbp_byte_count;
		}
		// DSI_PIXEL_RGB_666_LOOSELY & DSI_PIXEL_RGB_888
		//else if(ui_pixel_fmt == DSI_PIXEL_RGB_666_LOOSELY)
		//{
		//}
		else {
			ui_hact_byte_count = (p_mode->panel.ui_hsync_active_period) * 3;
			ui_hfp_byte_count = p_mode->panel.ui_hsync_back_porch * 3;
			ui_hbp_byte_count = p_mode->panel.ui_hsync_back_porch * 3;
			ui_bllp_byte_count = ui_hact_byte_count + ui_hfp_byte_count + ui_hbp_byte_count;
		}

#if 0
		if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_MODE, DSI_PIXMODE_COMMAND_MODE) != E_OK) {
			DBG_DUMP("Configure pixel mode command mode fail\r\n");
		}
#endif
		if (dsi_set_config(DSI_CONFIG_ID_HFP, ui_hfp_byte_count) != E_OK) {
			DSI_ERR_MSG(" Configure HFP as 0x%08x error\r\n", (uint)ui_hfp_byte_count);
		} else {
			DSI_CFG_MSG(" Configure HFP as %03d success\r\n", (int)ui_hfp_byte_count);
		}

		if (dsi_set_config(DSI_CONFIG_ID_HBP, ui_hbp_byte_count) != E_OK) {
			DSI_ERR_MSG(" Configure HPB as 0x%08x error\r\n", (uint)ui_hbp_byte_count);
		} else {
			DSI_CFG_MSG(" Configure HPB as %03d success\r\n", (int)ui_hbp_byte_count);
		}

		if (dsi_set_config(DSI_CONFIG_ID_HACT, ui_hact_byte_count) != E_OK) {
			DSI_ERR_MSG(" Configure HACT as %3d fail\r\n", (int)ui_hact_byte_count);
		} else {
			DSI_CFG_MSG(" Configure HACT as %03d success\r\n", (int)ui_hact_byte_count);
		}

		if (dsi_set_config(DSI_CONFIG_ID_VSA, p_mode->panel.ui_vsync_back_porch_even) != E_OK) {
			DSI_ERR_MSG(" Configure VSA as 0x%08x\r\n", (uint)p_mode->panel.ui_vsync_back_porch_even);
		}

		if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_PH_DT, DATA_TYPE_LONG_WRITE) != E_OK) {
			DSI_ERR_MSG(" Configure pkt packet header data type fail\r\n");
		}

		if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_PH_VC, 0) != E_OK) {
			DSI_ERR_MSG(" Configure pkt packet header vc fail\r\n");
		}

		if (dsi_set_config(DSI_CONFIG_ID_DCS_CT0, DCS_CMD_MEMORY_WRITE) != E_OK) {
			DSI_ERR_MSG(" Configure CT0 as mem write fail\r\n");
		}

		if (dsi_set_config(DSI_CONFIG_ID_DCS_CT1, DCS_CMD_MEMORY_WRITE_CONT) != E_OK) {
			DSI_ERR_MSG(" Configure CT1 as mem write cont fail\r\n");
		}

		if (dsi_set_config(DSI_CONFIG_ID_VTOTAL, p_mode->panel.ui_vsync_total_period + 10 * PORCH_ALIGN) != E_OK) {
			DSI_ERR_MSG(" Configure VTotal as %03d\r\n", (int)p_mode->panel.ui_vsync_total_period);
		} else {
			DSI_CFG_MSG(" Configure VTotal as %03d success\r\n", (int)p_mode->panel.ui_vsync_total_period);
		}

		if (dsi_set_config(DSI_CONFIG_ID_VVALID_START, p_mode->panel.ui_vsync_back_porch_even + PORCH_ALIGN) != E_OK) {
			DSI_ERR_MSG(" Configure VStart as 0x%08x error\r\n", (uint)p_mode->panel.ui_vsync_back_porch_even);
		} else {
			DSI_CFG_MSG(" Configure VStart as %03d success\r\n", (int)p_mode->panel.ui_vsync_back_porch_even);
		}

		if (dsi_set_config(DSI_CONFIG_ID_VVALID_END, (p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + PORCH_ALIGN) - 1) != E_OK) {
			DSI_ERR_MSG(" Configure VEnd as %03d error\r\n", (int)((p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + PORCH_ALIGN) - 1));
		} else {
			DSI_CFG_MSG(" Configure VEnd as %03d success\r\n", (int)((p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + PORCH_ALIGN) - 1));
		}
	}


	else {
		if ((p_mode->panel.lcd_mode >= PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB565) &&
			((p_mode->panel.lcd_mode <= PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888))) {
			dsi_set_config(DSI_CONFIG_ID_BLANK_CTRL, TRUE);
		} else {
			dsi_set_config(DSI_CONFIG_ID_BLANK_CTRL, FALSE);
		}

		dsi_set_config(DSI_CONFIG_ID_INTER_PKT_LP, FALSE);

		ui_vsa = p_mode->panel.ui_vsync_back_porch_odd / 2;

		if (ui_pixel_fmt == DSI_PIXEL_RGB_666_PACKETED) {
			if (ui_video_sync_mode == DSI_VIDEOPKT_TYPE_SYNC_PULSE) {
				ui_hact_byte_count = (p_mode->panel.ui_hsync_active_period * 9) / 4 ;
				ui_hfp_byte_count = (p_mode->panel.ui_hsync_back_porch * 9) / 4;
				ui_hbp_byte_count = (p_mode->panel.ui_hsync_back_porch * 9) / 4;
				ui_bllp_byte_count = ui_hact_byte_count + ui_hfp_byte_count + ui_hbp_byte_count + 12;
				if (ui_hbp_byte_count % 2 != 0) {
					ui_hsa = ui_hbp_byte_count / 2;
					ui_hbp_byte_count = ui_hbp_byte_count / 2 + 1;
				} else {
					ui_hsa = ui_hbp_byte_count / 2;
					ui_hbp_byte_count = ui_hbp_byte_count / 2;
				}
			} else {
				ui_hact_byte_count = (p_mode->panel.ui_hsync_active_period * 9) / 4 ;
				ui_hfp_byte_count = (p_mode->panel.ui_hsync_back_porch * 9) / 4;
				ui_hbp_byte_count = (p_mode->panel.ui_hsync_back_porch * 9) / 4;
				ui_bllp_byte_count = ui_hact_byte_count + ui_hfp_byte_count + ui_hbp_byte_count + 12;
			}
			if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_PH_DT, DATA_TYPE_PACKET_STREAM_RGB_666_PACKED) != E_OK) {
				DSI_ERR_MSG(" Configure pixel packet header error [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_565_PACKED);
			} else {
				DSI_CFG_MSG(" Configure pixel packet header success [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_565_PACKED);
			}
		} else if (ui_pixel_fmt == DSI_PIXEL_RGB_565) {
			if (ui_video_sync_mode == DSI_VIDEOPKT_TYPE_SYNC_PULSE) {
				ui_hact_byte_count = p_mode->panel.ui_hsync_active_period * 2;
				ui_hfp_byte_count = (p_mode->panel.ui_hsync_back_porch + PORCH_ALIGN) * 2;
				ui_hbp_byte_count = (p_mode->panel.ui_hsync_back_porch + PORCH_ALIGN) * 2;

				ui_hsa = ui_hbp_byte_count / 2;
				ui_hbp_byte_count = ui_hbp_byte_count / 2;

				ui_bllp_byte_count = ui_hact_byte_count + ui_hfp_byte_count + ui_hbp_byte_count + 12;
			} else {
				ui_hact_byte_count = p_mode->panel.ui_hsync_active_period * 2;
				ui_hfp_byte_count = (p_mode->panel.ui_hsync_back_porch + PORCH_ALIGN) * 2;
				ui_hbp_byte_count = (p_mode->panel.ui_hsync_back_porch + PORCH_ALIGN) * 2;
				ui_bllp_byte_count = ui_hact_byte_count + ui_hfp_byte_count + ui_hbp_byte_count + 12;
			}
			if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_PH_DT, DATA_TYPE_PACKET_STREAM_RGB_565_PACKED) != E_OK) {
				DSI_ERR_MSG(" Configure pixel packet header error [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_565_PACKED);
			} else {
				DSI_CFG_MSG(" Configure pixel packet header success [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_565_PACKED);
			}
		} else {
			UINT32 ui_hbp_hfp_sum;
			//UINT32 X = 5;
			//UINT32 Y = 0;

			//*********************************************************************************************************
			//((Htotal + X) * DSI_CLK)/IDE_CLK = (22+HBP+HFP+HACT)/2
			//((ui_hsync_total_period + X) * DSI_CLK)/IDE_CLK * LANENUM - 22 - HACT(byteCount) = HBP+HFP
			//*********************************************************************************************************

			//DSI_VIDEOPKT_TYPE_SYNC_EVENT
			ui_hact_byte_count = p_mode->panel.ui_hsync_active_period * 3;             //HACT


			//ui_hbp_hfp_sum = (((p_mode->panel.ui_hsync_total_period + X) * 3) / 2) * ui_lane_cnt - 22 - ui_hact_byte_count;

			ui_hbp_hfp_sum = ((p_mode->panel.ui_hsync_total_period) * 3) - ui_hact_byte_count - 22;

			DSI_CFG_MSG("ui_hsync_total_period = %d\r\n", (int)p_mode->panel.ui_hsync_total_period * 3);
			DSI_CFG_MSG("ui_hact_byte_count= %d, ui_hbp_hfp_sum = %d\r\n", (int)ui_hact_byte_count, (int)ui_hbp_hfp_sum);

			if (ui_hbp_hfp_sum % 2) {
				DBG_ERR("ui_hbp_hfp_sum = %d MUST be even number\r\n", (int)ui_hbp_hfp_sum);
				//??return E_SYS; //??Shirley remove for NT35410
			}
			//else
			//{
			ui_hbp_byte_count = (ui_hbp_hfp_sum >> 1) /*- 10*/;
			ui_hfp_byte_count = (ui_hbp_hfp_sum >> 1) /*+ 10*/;
			//}

			if (ui_hbp_byte_count % 4) {
				DBG_ERR("ui_hbp_byte_count = %d MUST be 4x number\r\n", (int)ui_hbp_byte_count);
				ui_hbp_byte_count = (ui_hbp_byte_count >> 2) << 2;
				ui_hfp_byte_count = (ui_hfp_byte_count >> 2) << 2;
			}

			//Calculate LP timing
			//THS_TRAIL           = dsi_get_config(DSI_CONFIG_ID_THS_TRAIL) + 1;
			//THS_EXIT            = dsi_get_config(DSI_CONFIG_ID_THS_EXIT) + 1;
			//THS_CLK_POST        = dsi_get_config(DSI_CONFIG_ID_TCLK_POST) + 1;
			//THS_CLK_TRAIL       = dsi_get_config(DSI_CONFIG_ID_TCLK_TRAIL) + 1;
			//THS_CLK_PREPARE     = dsi_get_config(DSI_CONFIG_ID_TCLK_PREPARE) + 1;
			//THS_CLK_ZERO        = dsi_get_config(DSI_CONFIG_ID_TCLK_ZERO) + 1;
			//THS_CLK_PRE         = dsi_get_config(DSI_CONFIG_ID_TCLK_PRE) + 1;
			//TLPX                = dsi_get_config(DSI_CONFIG_ID_TLPX) + 1;
			//THS_PREPARE         = dsi_get_config(DSI_CONFIG_ID_THS_PREPARE) + 1;
			//THS_ZERO            = dsi_get_config(DSI_CONFIG_ID_THS_ZERO) + 1;
			/*
						TLP_TOTAL         = THS_TRAIL + \
											THS_EXIT + \
											TLP_transition_time + \
											THS_CLK_POST + \
											THS_CLK_TRAIL + \
											THS_EXIT + \
											TLPX + \
											THS_CLK_PREPARE + \
											THS_CLK_ZERO + \
											THS_CLK_PRE + \
											TLP_transition_time +\
											TLPX + \
											THS_PREPARE + \
											THS_ZERO + \
											THS_SYNC;

			*/
			//uiVtotal_Sub_Vactive= p_mode->panel.ui_vsync_total_period - p_mode->panel.ui_vsync_active_period;
//          uiVIDEData = (Vtotal - VACT       ) * Htotal                          * DSI_CLK/IDE_CLK
//          uiVIDEData = (uiVtotal_Sub_Vactive) * p_mode->panel.ui_hsync_total_period * 3 / 2;

			//ui_bllp_byte_count = (((uiVIDEData - TLP_TOTAL - Y -3) * ui_lane_cnt) / (uiVtotal_Sub_Vactive)) - 10;
			//ui_bllp_byte_count = (((uiVIDEData - TLP_TOTAL -3) * ui_lane_cnt) / (uiVtotal_Sub_Vactive)) - 10;
			ui_bllp_byte_count = ui_hbp_byte_count + ui_hfp_byte_count + ui_hact_byte_count + 22 - 10;


			//DBG_DUMP("ui_hfp_byte_count = 0x%08x\r\n", ui_hfp_byte_count);
			//DBG_DUMP("ui_bllp_byte_count = 0x%08x LP = %d\r\n", ui_bllp_byte_count, TLP_TOTAL);
			//DBG_DUMP("uiBHBPByteCount = 0x%08x\r\n", ui_hbp_byte_count);


			if (ui_video_sync_mode == DSI_VIDEOPKT_TYPE_SYNC_PULSE) {
				/*if(ui_hbp_byte_count % 2 != 0)
				{
				    ui_hsa = ui_hbp_byte_count / 2;
				    ui_hbp_byte_count = ui_hbp_byte_count / 2 + 1;
				}
				else
				{
				    ui_hsa = ui_hbp_byte_count / 2;
				    ui_hbp_byte_count = ui_hbp_byte_count / 2;
				}*/
				ui_hsa = ui_hbp_byte_count / 2;
				ui_hbp_byte_count = ui_hbp_byte_count - ui_hsa - 10;
			}


			if (ui_pixel_fmt == DSI_PIXEL_RGB_666_LOOSELY) {
				if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_PH_DT, DATA_TYPE_PACKET_STREAM_RGB_666_LOOSELY) != E_OK) {
					DSI_ERR_MSG(" Configure pixel packet header error [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_666_LOOSELY);
				} else {
					DSI_CFG_MSG(" Configure pixel packet header success [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_666_LOOSELY);
				}
			} else {
				if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_PH_DT, DATA_TYPE_PACKET_STREAM_RGB_888_PACKED) != E_OK) {
					DSI_ERR_MSG(" Configure pixel packet header error [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_888_PACKED);
				} else {
					DSI_CFG_MSG(" Configure pixel packet header success [0x%02x]\r\n", (uint)DATA_TYPE_PACKET_STREAM_RGB_888_PACKED);
				}
			}
			if ((p_mode->panel.lcd_mode >= PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB565) &&
				((p_mode->panel.lcd_mode <= PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888))) {
				//if (dsi_get_config(DSI_CONFIG_ID_BLANK_CTRL) == TRUE)
				{
					ui_hbp_byte_count = ui_hbp_byte_count / ui_lane_cnt;
					ui_hfp_byte_count = ui_hfp_byte_count / ui_lane_cnt;
					ui_bllp_byte_count = ui_bllp_byte_count / ui_lane_cnt;
				}
				//DBG_DUMP("2.ui_hfp_byte_count = 0x%08x\r\n", ui_hfp_byte_count);
				//DBG_DUMP("2.ui_bllp_byte_count = 0x%08x LP = %d\r\n", ui_bllp_byte_count, TLP_TOTAL);
				//DBG_DUMP("2.uiBHBPByteCount = 0x%08x\r\n", ui_hbp_byte_count);
			}
		}

		if (dsi_set_config(DSI_CONFIG_ID_VTOTAL, p_mode->panel.ui_vsync_total_period) != E_OK) {
			DSI_ERR_MSG(" Configure VTotal as %03d error\r\n", (int)p_mode->panel.ui_vsync_total_period);
		} else {
			DSI_CFG_MSG(" Configure VTotal as %03d success\r\n", (int)p_mode->panel.ui_vsync_total_period);
		}

		if (dsi_set_config(DSI_CONFIG_ID_VVALID_START, p_mode->panel.ui_vsync_back_porch_even + 2) != E_OK) {
			DSI_ERR_MSG(" Configure VStart as 0x%08x error\r\n", (uint) p_mode->panel.ui_vsync_back_porch_even + 2);
		} else {
			DSI_CFG_MSG(" Configure VStart as %03d success\r\n", (int)p_mode->panel.ui_vsync_back_porch_even + 2);
		}

		if (dsi_set_config(DSI_CONFIG_ID_VVALID_END, (p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + 2) - 1) != E_OK) {
			DSI_ERR_MSG(" Configure VEnd as %03d error\r\n", (int)(p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + 2) - 1);
		} else {
			DSI_CFG_MSG(" Configure VEnd as %03d success\r\n", (int)(p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + 2) - 1);
		}

#if 0
		if (dsi_set_config(DSI_CFG_PIXPKT_MODE, DSI_PIXMODE_VIDEO_MODE) != E_OK) {
			DBG_DUMP("^R Configure pixel mode video mode fail\r\n");
		} else {
			DBG_DUMP("^G Configure pixel mode video mode success\r\n");
		}
#endif
		if (dsi_set_config(DSI_CONFIG_ID_VDOPKT_TYPE, ui_video_sync_mode) != E_OK) {
			DSI_ERR_MSG(" Configure video sync mode [%d] fail\r\n", (int)ui_video_sync_mode);
		} else {
			DSI_CFG_MSG(" Configure video sync mode [%d] success\r\n", (int)ui_video_sync_mode);
		}
		if (ui_video_sync_mode == DSI_VIDEOPKT_TYPE_SYNC_PULSE) {
			if (dsi_set_config(DSI_CONFIG_ID_VSA, ui_vsa) != E_OK) {
				DSI_ERR_MSG(" Configure VSA as %03d fail\r\n", (int)ui_vsa);
			} else {
				DSI_CFG_MSG(" Configure VSA as %03d success\r\n", (int)ui_vsa);
			}

			if (dsi_set_config(DSI_CONFIG_ID_HSA, ui_hsa) != E_OK) {
				DSI_CFG_MSG(" Configure HSA as %3d fail\r\n", (int)ui_hsa);
			} else {
				DSI_CFG_MSG(" Configure HSA as %03d success\r\n", (int)ui_hsa);
			}
		} else {
			dsi_set_config(DSI_CONFIG_ID_SYNCEVT_NULL_LEN, 0x20);

			dsi_set_config(DSI_CONFIG_ID_SYNCEVT_SLICE_NO, 0);

			//ui_hact_byte_count = ui_hact_byte_count / 2;

			//ui_bllp_byte_count += (0x20 + 18);
		}

		if (dsi_set_config(DSI_CONFIG_ID_HBP, ui_hbp_byte_count) != E_OK) {
			DSI_ERR_MSG(" Configure HPB as 0x%08x error\r\n", (uint)ui_hbp_byte_count);
		} else {
			DSI_CFG_MSG(" Configure HPB as %03d success\r\n", (int)ui_hbp_byte_count);
		}


		if (dsi_set_config(DSI_CONFIG_ID_HFP, ui_hfp_byte_count) != E_OK) {
			DSI_ERR_MSG(" Configure HFP as 0x%08x error\r\n", (uint)ui_hfp_byte_count);
		} else {
			DSI_CFG_MSG(" Configure HFP as %03d success\r\n", (int)ui_hfp_byte_count);
		}

		if (dsi_set_config(DSI_CONFIG_ID_HACT, ui_hact_byte_count) != E_OK) {
			DSI_ERR_MSG(" Configure HACT as %3d fail\r\n", (int)ui_hact_byte_count);
		} else {
			DSI_CFG_MSG(" Configure HACT as %03d success\r\n", (int)ui_hact_byte_count);
		}
	}

	if (dsi_set_config(DSI_CONFIG_ID_IDEHVALID, p_mode->panel.ui_hsync_active_period) != E_OK) {
		DSI_ERR_MSG(" Configure IDEHVALID as %03d error\r\n", (int)(p_mode->panel.ui_hsync_active_period));
	} else {
		DSI_CFG_MSG(" Configure IDEHVALID as %03d success\r\n", (int)(p_mode->panel.ui_hsync_active_period));
	}

	if (dsi_set_config(DSI_CONFIG_ID_PIXPKT_MODE, ui_pixel_pkt_mode_sel) != E_OK) {
		DSI_ERR_MSG(" Configure pixel mode [%d] mode fail => (0:video/1:command)\r\n", (int)ui_pixel_pkt_mode_sel);
	} else {
		DSI_CFG_MSG(" Configure pixel mode [%d] mode success => (0:video/1:command)\r\n", (int)ui_pixel_pkt_mode_sel);
	}

	if (dsi_set_config(DSI_CONFIG_ID_BLLP, ui_bllp_byte_count) != E_OK) {
		DSI_ERR_MSG(" Configure BLLP as 0x%08x error\r\n", (uint)ui_bllp_byte_count);
	} else {
		DSI_CFG_MSG(" Configure BLLP as %03d success\r\n", (int)ui_bllp_byte_count);
	}

#if 0
	if (dsi_set_config(DSI_CONFIG_ID_VTOTAL, p_mode->panel.ui_vsync_total_period + 10 * PORCH_ALIGN) != E_OK) {
		DSI_ERR_MSG(" Configure VTotal as %03d error\r\n", (int) p_mode->panel.ui_vsync_total_period);
	} else {
		DSI_CFG_MSG(" Configure VTotal as %03d success\r\n", (int) p_mode->panel.ui_vsync_total_period);
	}

	if (dsi_set_config(DSI_CONFIG_ID_VVALID_START, p_mode->panel.ui_vsync_back_porch_even + 2) != E_OK) {
		DSI_ERR_MSG(" Configure VStart as 0x%08x error\r\n", (uint) p_mode->panel.ui_vsync_back_porch_even + 2);
	} else {
		DSI_CFG_MSG(" Configure VStart as %03d success\r\n", (int) p_mode->panel.ui_vsync_back_porch_even + 2);
	}

	if (dsi_set_config(DSI_CONFIG_ID_VVALID_END, (p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + 2) - 1) != E_OK) {
		DSI_ERR_MSG(" Configure VEnd as %03d error\r\n", (int) (p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + 2) - 1);
	} else {
		DSI_CFG_MSG(" Configure VEnd as %03d success\r\n", (int) (p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_even + 2) - 1);
	}
#endif
#if 0
	if (dsi_set_config(DSI_CONFIG_ID_MODE, DSI_MODE_AUTO_MODE1) != E_OK) {
		DSI_ERR_MSG(" Configure auto mode 1 fail\r\n");
	}
#else
	if (dsi_set_config(DSI_CONFIG_ID_MODE, DSI_MODE_AUTO_MODE2) != E_OK) {
		DSI_ERR_MSG(" Configure auto mode 2 fail\r\n");
	}
#endif

#ifndef __KERNEL__
	p_disp_dev_control(DISPDEV_IOCTRL_GET_SRCCLK, &dev_io_ctrl);

	switch (dev_io_ctrl.SEL.GET_SRCCLK.src_clk) {
	case DISPCTRL_SRCCLK_PLL1:
	/*case DISPCTRL_SRCCLK_IDE2_PLL1:*/ {
			src_pll = PLL_ID_1;
		}
		break;

	/*case DISPCTRL_SRCCLK_PLL2:
	case DISPCTRL_SRCCLK_IDE2_PLL2: {
			src_pll = PLL_ID_2;
		}
		break;*/

	case DISPCTRL_SRCCLK_PLL4:
	/*case DISPCTRL_SRCCLK_IDE2_PLL4:*/ {
			src_pll = PLL_ID_4;
		}
		break;

	case DISPCTRL_SRCCLK_PLL6:
	/*case DISPCTRL_SRCCLK_IDE2_PLL6:*/ {
			src_pll = PLL_ID_6;
		}
		break;

	/*case DISPCTRL_SRCCLK_PLL14:
	case DISPCTRL_SRCCLK_IDE2_PLL14: {
			src_pll = PLL_ID_14;
		}
		break;*/

	case DISPCTRL_SRCCLK_PLL9:
	/*case DISPCTRL_SRCCLK_IDE2_PLL9:*/ {
			src_pll = PLL_ID_9;
		}
		break;

	default: {
			DBG_ERR("Error source clock %d\r\n", dev_io_ctrl.SEL.GET_SRCCLK.src_clk);
			return E_SYS;
		}

	}


	cur_freq = pll_getPLLFreq(src_pll) / 1000000;

	if (src_pll != PLL_ID_1) {
		DBG_IND("Original src_clk(%d)Mhz\r\n", (int)cur_freq);
		//pll_setPLLEn(src_pll,    FALSE);
		//pll_setPLL(src_pll,      IDE_SRC_CLK_480);
		//pll_setPLLEn(src_pll,    TRUE);
		disp_dev_src_freq = cur_freq;
		DBG_IND("DEVDSI: Chg PLL%d to(%d)Mhz\r\n", (int)(src_pll + 1), (int)(pll_getPLLFreq(src_pll) / 1000000));
	} else {
		disp_dev_src_freq = 0;
		DBG_IND("Original src_clk already PLL1(%d)Mhz\r\n", (int)cur_freq);
	}

	//enable PLL for DSI (PLL11)
	pll_setPLLEn(PLL_ID_11,    TRUE);
#endif

	// Config & Enable Display physical engine (ide engine)
	dispdev_set_display(p_disp_dev_control, T_LCD_INF_MIPI_DSI, p_mode, pin_func_id);

	if (b_te_on == TRUE && b_te_ext_pin == TRUE) {
#ifndef __KERNEL__
		//switch PINMUX
		UINT32 ui_reg;

		ui_reg = *(UINT32 *)0xF0010008;
		ui_reg &= ~(0x40F);
		ui_reg |= 0x409;

		*(UINT32 *)0xF0010008 = ui_reg;
#endif
	}

	dsi_set_tx_en(TRUE, TRUE);

	dispanl_debug(("^GopenIFDsi Done\r\n"));
#if 0
	// Test panel Frame Rate
	{
		UINT32 i;

		Perf_Open();
		Perf_Mark();
		for (i = 0; i < 1000; i++) {
			p_devif->wait_frm_end();
		}
		DBG_DUMP("Time tick = %d us\r\n", Perf_GetDuration());
		Perf_Close();
	}
#endif

	return E_OK;
}

/*
    Close LCD display panel.

    Call this api to close the LCD panel device.

    @param[in] p_devif Please refer to dispdevctrl.h for details.

    @return
     - @b E_OK:     LCD Close done and success.
*/
static ER dispdev_close_if_dsi(void)
{
	UINT32                  i, cmd_number, data_cnt;
	T_PANEL_CMD              *p_standby = NULL;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#if !_CFG_USE_LP_
	DSI_CMD_RW_CTRL_PARAM   dcs_context;
#endif
	DSI_CMD_CTRL_PARAM      dcs_ctrl_context;
//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control1;
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
//	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control2;
//#endif


	BOOL                    b_eopt = TRUE;
#ifndef __KERNEL__
	UINT32                  src_pll;
#endif
	unsigned long           flags;


	dispanl_debug(("^GcloseIFDsi START\r\n"));

	dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
	/*    if(dsi_get_config(DSI_CONFIG_ID_PIXPKT_MODE) == DSI_PIXMODE_VIDEO_MODE)
	    {
			p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
			p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl); //Disable ide
			dsi_wait_frame_end();
			dsi_set_tx_en(FALSE, TRUE);                               //Disable DSI
		}
		else
		{
			dsi_wait_frame_end();
			p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl); //Disable ide
			dsi_set_tx_en(FALSE, TRUE);
			p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
			p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		}*/
	{
		//DBG_ERR("wait dsi_wait_frame_end\r\n");
		dsi_wait_frame_end();
		loc_cpu(flags);
		p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl); //Disable ide
		dsi_set_tx_en(FALSE, FALSE);                              //Disable DSI
		unl_cpu(flags);
		//DBG_ERR("wait ide frame end .....\r\n");
		p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		//DBG_ERR("wait frame 1\r\n");
		p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		//DBG_ERR("wait frame 2\r\n");

		dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGB_SERIAL; //switch to display format 0
		p_disp_dev_control(DISPDEV_IOCTRL_SET_DEVICE, &dev_io_ctrl);

	}
//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
//#endif

	p_standby = dispdev_get_standby_cmd_dsi(&cmd_number);

	dcs_ctrl_context.ui_param_cnt = 0;

	if (p_standby != NULL) {
		if (dsi_set_config(DSI_CONFIG_ID_MODE, DSI_MODE_MANUAL_MODE) != E_OK) {
			DSI_ERR_MSG("^R Configure manual mode fail\r\n");
		}

		// Set display device into Standby or powerdown.
		for (i = 0; i < cmd_number;) {
			if (p_standby[i].ui_address == CMDDELAY_MS) {
				dispdev_platform_delay_ms(p_standby[i].ui_value);
				i++;
			} else if (p_standby[i].ui_address == CMDDELAY_US) {
				dispdev_platform_delay_us(p_standby[i].ui_value);
				i++;
			} else if (p_standby[i].ui_address == DSICMD_CMD) {
#if !_CFG_USE_LP_
				dcs_context.b_bta_en = FALSE;
				dcs_context.b_bta_only = FALSE;
				dcs_context.ui_cmd_no = 1;
				dcs_context.b_eot_en = FALSE;
				dcs_context.ui_sram_ofs = 128;
#endif
				dcs_ctrl_context.ui_param_cnt = 0;
				dcs_ctrl_context.ui_virtual_channel = 0;

				dcs_ctrl_context.ui_dcs_cmd = p_standby[i].ui_value;

				for (data_cnt = i + 1; data_cnt < cmd_number; data_cnt++) {
					/*if (p_standby[data_cnt].ui_address == DSICMD_DATA) {
						//data cnt++;
						if (p_standby[data_cnt].ui_value > 0xFF) {
							DSI_ERR_MSG("MIPI command exceed 1 bytes error 0x%08x\r\n", p_standby[data_cnt].ui_value);
						}
						dsi_cmd_param[dcs_ctrl_context.ui_param_cnt] = p_standby[data_cnt].ui_value;
						dcs_ctrl_context.ui_param_cnt++;
					} else {
						break;
					}*/
					if (p_standby[data_cnt].ui_address != DSICMD_DATA) {
						break;
					}
					if (p_standby[data_cnt].ui_value > 0xFF) {
						DSI_ERR_MSG("MIPI command exceed 1 bytes error 0x%08x\r\n", (uint)p_standby[data_cnt].ui_value);
					}
					dsi_cmd_param[dcs_ctrl_context.ui_param_cnt] = p_standby[data_cnt].ui_value;
					dcs_ctrl_context.ui_param_cnt++;

				}
				if (dcs_ctrl_context.ui_param_cnt == 0) {
					dcs_ctrl_context.ui_packet_type = DSI_SHORT_PACKET;
					dcs_ctrl_context.ui_data_type = DATA_TYPE_SHORT_WRITE_NO_PARAM;
					dcs_ctrl_context.ui_param = NULL;
#if _CFG_USE_LP_
					dsi_cmd_param[0] = 0x0;
					dsi_lp_write_cmd(DSI_DATA_LANE_0, dcs_ctrl_context.ui_dcs_cmd, dsi_cmd_param, 0, DSI_SHORT_PACKET, b_eopt);
#endif

				} else if (dcs_ctrl_context.ui_param_cnt == 1) {
					dcs_ctrl_context.ui_packet_type = DSI_SHORT_PACKET;
					dcs_ctrl_context.ui_data_type = DATA_TYPE_SHORT_WRITE_1_PARAM;
					dcs_ctrl_context.ui_param = (UINT8 *)&dsi_cmd_param;
#if _CFG_USE_LP_
					dsi_cmd_param[0] = 0x0;
					dsi_lp_write_cmd(DSI_DATA_LANE_0, dcs_ctrl_context.ui_dcs_cmd, dsi_cmd_param, 1, DSI_SHORT_PACKET, b_eopt);
#endif
				} else {
					dcs_ctrl_context.ui_packet_type = DSI_LONG_PACKET;
					dcs_ctrl_context.ui_data_type = DATA_TYPE_LONG_WRITE;
					dcs_ctrl_context.ui_param = (UINT8 *)&dsi_cmd_param;
#if _CFG_USE_LP_
					dsi_cmd_param[0] = 0x0;
					dsi_lp_write_cmd(DSI_DATA_LANE_0, dcs_ctrl_context.ui_dcs_cmd, dsi_cmd_param, dcs_ctrl_context.ui_param_cnt, DSI_LONG_PACKET, b_eopt);
#endif
				}
				/*
				else
				{
				    DSI_ERR_MSG("Max 8 param \r\n");
				}
				*/
#if !_CFG_USE_LP_
				dcs_context.p_dsi_cmd_ctx = &dcs_ctrl_context;
				dsi_set_hs_dcs_command(&dcs_context);
#endif

				//dispdev_write_lcd_reg(p_mode->p_cmd_queue[i].ui_address, p_mode->p_cmd_queue[i].ui_value);
				i += (dcs_ctrl_context.ui_param_cnt + 1);
			} else {
				//dispdev_write_lcd_reg(p_mode->p_cmd_queue[i].ui_address, p_mode->p_cmd_queue[i].ui_value);
				i += (dcs_ctrl_context.ui_param_cnt + 1);
			}
		}
	}

	dsi_close();

#ifndef __KERNEL__
	p_disp_dev_control(DISPDEV_IOCTRL_GET_SRCCLK, &dev_io_ctrl);

	switch (dev_io_ctrl.SEL.GET_SRCCLK.src_clk) {
	case DISPCTRL_SRCCLK_PLL1:
	/*case DISPCTRL_SRCCLK_IDE2_PLL1:*/ {
			src_pll = PLL_ID_1;
		}
		break;

	/*case DISPCTRL_SRCCLK_PLL2:
	case DISPCTRL_SRCCLK_IDE2_PLL2: {
			src_pll = PLL_ID_2;
		}
		break;*/

	case DISPCTRL_SRCCLK_PLL4:
	/*case DISPCTRL_SRCCLK_IDE2_PLL4:*/ {
			src_pll = PLL_ID_4;
		}
		break;

	case DISPCTRL_SRCCLK_PLL6:
	/*case DISPCTRL_SRCCLK_IDE2_PLL6:*/ {
			src_pll = PLL_ID_6;
		}
		break;

	/*case DISPCTRL_SRCCLK_PLL14:
	case DISPCTRL_SRCCLK_IDE2_PLL14: {
			src_pll = PLL_ID_14;
		}
		break;*/

	case DISPCTRL_SRCCLK_PLL9:
	/*case DISPCTRL_SRCCLK_IDE2_PLL9:*/ {
			src_pll = PLL_ID_9;
		}
		break;

	default:
		DBG_ERR("Unknow src clock = %d\r\n", (int)(dev_io_ctrl.SEL.GET_SRCCLK.src_clk));
		return E_SYS;
		//break;
	}

	if (disp_dev_src_freq != 0) {
		DBG_IND("DEVDSI: Chg PLL%d from (%d)MHz to(%d)MHz\r\n", (int)(src_pll + 1), (int)(pll_getPLLFreq(src_pll) / 1000000), (int)disp_dev_src_freq);
		pll_setPLLEn(src_pll, FALSE);
		pll_setPLL(src_pll, (disp_dev_src_freq * 131072 / 12));
		pll_setPLLEn(src_pll,    TRUE);
		DBG_IND("DEVDSI: Chg PLL%d to(%d)Mhz done\r\n", (int)(src_pll + 1), (int)(pll_getPLLFreq(src_pll) / 1000000));
	}
#endif
	dispanl_debug(("^GcloseIFDsi Done\r\n"));
	return E_OK;
}

/*
    Write LCD Register

    Write LCD Register

    @param[in] p_devif   Please refer to dispdevctrl.h for details.
    @param[in] ui_addr   LCD Register Offset.
    @param[in] ui_value  Assigned LCD Register value

    @return void
*/
static void dispdev_write_lcd_reg(UINT32 ui_addr, UINT32 ui_value)
{
	//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	//DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control1;
	//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
	//DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control2;
	//#endif
	//dispdev_writeToLcdMi(p_disp_dev_control, ui_addr, ui_value);
}

//#define OUTW(addr,value)    (*(UINT32 volatile *)(addr) = (UINT32)(value))

/*
    Read LCD Register

    Read LCD Register

    @param[in] p_devif   Please refer to dispdevctrl.h for details.
    @param[in] ui_addr   LCD Register Offset.

    @return Read back LCD Register value
*/
static UINT32  dispdev_read_lcd_reg(UINT32 ui_addr)
{
#if 0
	DSI_CMD_RW_CTRL_PARAM	dcsContext;
	DSI_CMD_CTRL_PARAM		dcsCtrlContext;
	UINT32					uiValue=0x0;

	dcsContext.b_bta_en = TRUE;
	dcsContext.b_bta_only = FALSE;
	dcsContext.ui_cmd_no = 1;
	dcsContext.ui_sram_ofs = 0x80;

	dcsCtrlContext.ui_data_type = DATA_TYPE_SHORT_READ_NO_PARAM;
	dcsCtrlContext.ui_packet_type = DSI_SHORT_PACKET;
	dcsCtrlContext.ui_virtual_channel = 0;
	dcsCtrlContext.ui_dcs_cmd = ui_addr;
	dcsCtrlContext.ui_param = NULL;
	dcsCtrlContext.ui_param_cnt = 0;

	dcsContext.p_dsi_cmd_ctx = &dcsCtrlContext;
	dsi_set_hs_dcs_command_BTA(&dcsContext);

	uiValue = dsi_get_config(DSI_CONFIG_ID_BTA_VALUE);

	return uiValue;
#else
	DSI_CMD_RW_CTRL_PARAM	dcsContext = {0};
	DSI_CMD_CTRL_PARAM		dcsCtrlContext = {0};
	UINT32					uiValue=0x0;
#if _CFG_USE_LP_
	BOOL                    bEoPT = TRUE;
//	UINT32					uiDsiCmdCtrl;
//	UINT32					SRAM_RD_CNT;
//	UINT32					uiReadCnt;
//	UINT32					uiRdCnt;
//	UINT32					SRAM_RD_OFS;
#endif
	dcsContext.b_bta_en = FALSE;
	dcsContext.b_bta_only = FALSE;
	dcsContext.ui_cmd_no = 1;
	dcsContext.ui_sram_ofs = 0x80;
    dcsContext.b_eot_en = 0;

	dcsCtrlContext.ui_data_type = DATA_TYPE_SHORT_READ_NO_PARAM;
	dcsCtrlContext.ui_packet_type = DSI_SHORT_PACKET;
	dcsCtrlContext.ui_virtual_channel = 0;
	dcsCtrlContext.ui_dcs_cmd = ui_addr;
	dcsCtrlContext.ui_param = NULL;
	dcsCtrlContext.ui_param_cnt = 0;

#if !_CFG_USE_LP_
	dcsContext.p_dsi_cmd_ctx = &dcsCtrlContext;
	dsi_set_hs_dcs_command(&dcsContext);
#else
//	if(dcsCtrlContext.uiParamCnt == 0)
//    {
        dcsCtrlContext.ui_packet_type = DSI_SHORT_PACKET;
        dcsCtrlContext.ui_data_type = DATA_TYPE_SHORT_READ_NO_PARAM;
        dcsCtrlContext.ui_param = NULL;
        dsi_cmd_param[0] = 0x0;
        dsi_lp_read_cmd(DSI_DATA_LANE_0, dcsCtrlContext.ui_dcs_cmd, dsi_cmd_param, 0, DSI_SHORT_PACKET, bEoPT);
//    }
/*
    else if(dcsCtrlContext.uiParamCnt == 1)
    {
        dcsCtrlContext.uiPacketType = DSI_SHORT_PACKET;
        dcsCtrlContext.ui_data_type = DATA_TYPE_SHORT_WRITE_1_PARAM;
        dcsCtrlContext.uiParam = (UINT8 *)&dsiCmdParam;
        dsi_LPWriteCmd(DSI_DATA_LANE_0, dcsCtrlContext.uiDCSCmd, dsiCmdParam, 1, DSI_SHORT_PACKET, bEoPT);
    }
    else
    {
        dcsCtrlContext.uiPacketType = DSI_LONG_PACKET;
        dcsCtrlContext.ui_data_type = DATA_TYPE_LONG_WRITE;
        dcsCtrlContext.uiParam = (UINT8 *)&dsiCmdParam[0];
        dsi_LPWriteCmd(DSI_DATA_LANE_0, dcsCtrlContext.uiDCSCmd, dsiCmdParam, dcsCtrlContext.uiParamCnt, DSI_LONG_PACKET, bEoPT);
    }
*/
	dcsContext.b_bta_en = TRUE;
	dcsContext.b_bta_only = TRUE;
	dcsContext.ui_cmd_no = 1;
	dcsContext.ui_sram_ofs = 0x80;
    dcsContext.b_eot_en = 0;

	dcsCtrlContext.ui_data_type = DATA_TYPE_SHORT_READ_NO_PARAM;
	dcsCtrlContext.ui_packet_type = DSI_SHORT_PACKET;
	dcsCtrlContext.ui_virtual_channel = 0;
	dcsCtrlContext.ui_dcs_cmd = 0x0;
	dcsCtrlContext.ui_param = NULL;
	dcsCtrlContext.ui_param_cnt = 0;

	dcsContext.p_dsi_cmd_ctx = &dcsCtrlContext;

	dsi_set_hs_dcs_command(&dcsContext);

#endif
	return uiValue;
#endif
}

/*
    PreGet LCD size

    PreGet LCD size

    @param[in] p_devif   Please refer to dispdevctrl.h for details.
    @param[in] ui_addr   LCD Register Offset.
    @param[in] ui_value  Assigned LCD Register value

    @return void
*/
static ER dispdev_get_lcd_dsi_size(DISPDEV_GET_PRESIZE *t_size)
{
	//UINT32                  i;
	T_LCD_PARAM              *p_mode      = NULL;
	T_LCD_PARAM              *p_config_mode = NULL;
#ifndef __KERNEL__
	PINMUX_LCDINIT          lcd_mode;
#endif
	UINT32                  mode_number;
	PINMUX_FUNC_ID          pin_func_id;
	//DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control1;
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
//	DISPDEV_IOCTRL          p_disp_dev_control = p_ifdsi_io_control2;
//#endif


	dispanl_debug(("^GgetLcdSize START\r\n"));

	if (p_disp_dev_control == NULL) {
		return E_PAR;
	}

	t_size->ui_buf_width = 0;
	t_size->ui_buf_height = 0;
	t_size->ui_win_width = 0;
	t_size->ui_win_height = 0;

	//
	//  Sortig the SysInit Config mode support
	//
//#if (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
//#elif ((DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2))
//	pin_func_id = PINMUX_FUNC_ID_LCD2;
//#endif
#ifndef __KERNEL__
	lcd_mode = pinmux_get_dispmode(pin_func_id);
#endif
	p_config_mode = dispdev_get_config_mode_dsi(&mode_number);
#if 0
	if ((p_config_mode != NULL) && (mode_number > 0)) {
		for (i = 0; i < mode_number; i++) {
			if (p_config_mode[i].panel.lcd_mode == lcd_mode) {
				p_mode = (T_LCD_PARAM *)&p_config_mode[i];
				break;
			}
		}
	}
#else
	if (p_config_mode != NULL) {
		p_mode = (T_LCD_PARAM *)&p_config_mode[0];
	}
#endif
	if (p_mode == NULL) {
#ifndef __KERNEL__
		DBG_ERR("lcd_mode=%d not support\r\n", lcd_mode);
#endif
		return E_NOSPT;
	}

	t_size->ui_buf_width = p_mode->panel.ui_buffer_width;
	t_size->ui_buf_height = p_mode->panel.ui_buffer_height;
	t_size->ui_win_width = p_mode->panel.ui_window_width;
	t_size->ui_win_height = p_mode->panel.ui_window_height;

	return E_OK;
}



/*
    Set TV Display Device IOCTRL function pointer
*/
static void dispdev_set_if_dsi_ioctrl(FP p_io_ctrl_func)
{
//#if (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	p_ifdsi_io_control1 = (DISPDEV_IOCTRL)p_io_ctrl_func;
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
//	p_ifdsi_io_control2 = (DISPDEV_IOCTRL)p_io_ctrl_func;
//#endif
}

/*
    Set LCD rotation
*/
static ER dispdev_set_device_rotate(DISPDEV_LCD_ROTATE rot)
{
	//UINT32          mode_no,index;
	//T_LCD_ROT        *p_cmd_rot=NULL;
	//BOOL            b_valid = FALSE;

	return E_NOSPT;
}



//#if   (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
/**
    Get LCD 1 Device Object

    Get LCD 1 Device Object. This Object is used to control the LCD Device.

    @return The pointer of LCD Display Device object.
*/
PDISPDEV_OBJ dispdev_get_lcd1_dev_obj(void)
{
	return &disp_dev_ifdsi_obj_lcd1;
}
//#elif (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD2)
/**
    Get LCD 2 Device Object

    Get LCD 2 Device Object. This Object is used to control the LCD Device.

    @return The pointer of LCD Display Device object.
*/
//PDISPDEV_OBJ dispdev_get_lcd2_dev_obj(void)
//{
//	return &disp_dev_ifdsi_obj_lcd2;
//}
//#endif


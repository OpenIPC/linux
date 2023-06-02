
/* ----------------------------------------------------------------------------------
 * 1. Header file include -----------------------------------------------------------
 * --------------------------------------------------------------------------------*/
#include "raptor5_common.h"
#include "raptor5_table.h"
#include "raptor5_function.h"


/* ----------------------------------------------------------------------------------
 * 2. Define ------------------------------------------------------------------------
 * --------------------------------------------------------------------------------*/
#define OPT_DISABLE 0
#define OPT_ENABLE  1

/* ----------------------------------------------------------------------------------
 * 3. Define variable ---------------------------------------------------------------
 * --------------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------------
 * 4. External variable & function --------------------------------------------------
 * --------------------------------------------------------------------------------*/
extern NC_U8 g_nc_drv_i2c_addr[4];


/* ----------------------------------------------------------------------------------
 * 5. Function prototype ------------------------------------------------------------
 * --------------------------------------------------------------------------------*/
NC_U32 internal_nc_drv_video_distance_hsync_distortion_get( NC_U8 dev, NC_U8 chn, NC_U32 *Sam1Frame, NC_U32 *Sam8Frame );
void   internal_nc_drv_video_sd_format_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt);
void internal_nc_drv_video_input_format_init_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt, NC_VO_WORK_MODE_E mux_mode, NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo);
void   internal_nc_drv_video_input_format_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt, NC_VO_WORK_MODE_E mux_mode, NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo);
void   internal_nc_drv_video_output_hide_set( NC_U8 Dev, NC_U8 Chn, NC_U8 Val );

void nc_drv_video_auto_manual_mode_set(void *pParam);

void nc_drv_video_setting_info_get( void *pParam )
{
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 info_chn = pVdInfo->Chn;

	pVdInfo->VideoFormat      = nc_drv_common_info_video_fmt_def_get(info_chn);
	pVdInfo->VideoCable       = nc_drv_common_info_cable_get(info_chn);
	pVdInfo->FormatStandard   = nc_drv_common_info_video_fmt_standard_get(info_chn);
	pVdInfo->FormatResolution = nc_drv_common_info_video_fmt_resolusion_get(info_chn);
	pVdInfo->FormatFps        = nc_drv_common_info_video_fmt_fps_get(info_chn);
	pVdInfo->EqStage          = nc_drv_common_info_eq_stage_get(info_chn);
	nc_drv_common_info_video_fmt_string_get(info_chn, pVdInfo->FmtStr);
}

NC_VIVO_CH_FORMATDEF_E nc_drv_video_h_v_cnt_check_get(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E vi_fmt)
{
	NC_VIVO_CH_FORMATDEF_E fmt = vi_fmt;

	int H_CNT = 0;
	int V_CNT = 0;
	int AHD_VBI = 0;
	int CVI_VBI = 0;
	int SyncMax = 0;

	NC_U8 HighReg = 0;
	NC_U8 LowReg  = 0;

	/*********************************************************************
	 * H, V Count Check
	 *********************************************************************/
	printk("============= Signal Information =============\n");
	NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF2);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF3);
	H_CNT = (HighReg<<8)|LowReg;
	printk("H_Cnt[%x + %x = %x/%d]\n", HighReg, LowReg, H_CNT, H_CNT);

	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF4);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF5);
	V_CNT = (HighReg<<8)|LowReg;
	printk("V_Cnt[%x + %x = %x/%d]\n", HighReg, LowReg, V_CNT, V_CNT);

	/*********************************************************************
	 * LE Pulse Check
	 *********************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, 0x13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B, chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa4);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa5);
	AHD_VBI = (HighReg<<8)|LowReg;
	printk("AHD_VBI[%x + %x = %x/%d]\n", HighReg, LowReg, AHD_VBI, AHD_VBI);

	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa6);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa7);
	CVI_VBI = (HighReg<<8)|LowReg;
	printk("CVI_VBI[%x + %x = %x/%d]\n", HighReg, LowReg, CVI_VBI, CVI_VBI);

	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xaa);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xab);
	SyncMax = (HighReg<<8)|LowReg;
	printk("SyncMax[%x + %x = %x/%d]\n", HighReg, LowReg, SyncMax, SyncMax);

	if(H_CNT == 0 && V_CNT == 0)
	{
		printk("[%s::%d]NC_VI_NO_SIGNAL\n", __func__, __LINE__);
		return NC_VI_SIGNAL_OFF;
	}
	else
	{
		printk("[%s::%d]NC_VI_SIGNAL_ON\n", __func__, __LINE__);

		return fmt;
	}

	printk("==============================================\n");

	if(vi_fmt == TVI_FHD_30P)
	{
		if(AHD_VBI > 6000)
			fmt = AHD_1080P_30P;
		else
			fmt = TVI_FHD_30P;
	}
	else if(vi_fmt == CVI_5M_20P)
	{

	}

	return fmt;

}


int nc_drv_video_h_cnt_get(NC_U8 dev, NC_U8 chn)
{
	int H_CNT = 0;

	NC_U8 HighReg = 0;
	NC_U8 LowReg  = 0;

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF2);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF3);
	H_CNT = (HighReg<<8)|LowReg;
//	printk("H_Cnt[%x + %x = %x/%d]\n", HighReg, LowReg, H_CNT, H_CNT);

	return H_CNT;
}

int nc_drv_video_v_cnt_get(NC_U8 dev, NC_U8 chn)
{
	int V_CNT = 0;

	NC_U8 HighReg = 0;
	NC_U8 LowReg  = 0;

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF4);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF5);
	V_CNT = (HighReg<<8)|LowReg;
//	printk("V_Cnt[%x + %x = %x/%d]\n", HighReg, LowReg, V_CNT, V_CNT);

	return V_CNT;
}

int nc_drv_video_sync_max_get(NC_U8 dev, NC_U8 chn)
{
	int SyncMax = 0;

	NC_U8 HighReg = 0;
	NC_U8 LowReg  = 0;

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B, chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xaa);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xab);
	SyncMax = (HighReg<<8)|LowReg;
//	printk("SyncMax[%x + %x = %x/%d]\n", HighReg, LowReg, SyncMax, SyncMax);

	return SyncMax;
}

int nc_drv_video_ahd_vbi_get(NC_U8 dev, NC_U8 chn)
{
	int AHD_VBI = 0;

	NC_U8 HighReg = 0;
	NC_U8 LowReg  = 0;

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B, chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa4);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa5);
	AHD_VBI = (HighReg<<8)|LowReg;
//	printk("AHD_VBI[%x + %x = %x/%d]\n", HighReg, LowReg, AHD_VBI, AHD_VBI);

	return AHD_VBI;
}

int nc_drv_video_cvi_vbi_get(NC_U8 dev, NC_U8 chn)
{
	int CVI_VBI = 0;

	NC_U8 HighReg = 0;
	NC_U8 LowReg  = 0;

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B, chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa6);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xa7);
	CVI_VBI = (HighReg<<8)|LowReg;
//	printk("CVI_VBI[%x + %x = %x/%d]\n", HighReg, LowReg, CVI_VBI, CVI_VBI);
//	printk("AHD_VBI[%x + %x = %x/%d]\n", HighReg, LowReg, AHD_VBI, AHD_VBI);

	return CVI_VBI;
}

NC_VIVO_CH_FORMATDEF_E nc_drv_novideo_h_v_cnt_get(NC_U8 dev, NC_U8 chn)
{
	NC_U8 HighReg = 0;
	NC_U8 LowReg  = 0;

	int H_CNT = 0;
	int V_CNT = 0;

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF2);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF3);
	H_CNT = (HighReg<<8)|LowReg;
//	printk("H_Cnt[%x + %x = %x/%d]\n", HighReg, LowReg, H_CNT, H_CNT);

	LowReg  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF4);
	HighReg = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF5);
	V_CNT = (HighReg<<8)|LowReg;
//	printk("V_Cnt[%x + %x = %x/%d]\n", HighReg, LowReg, V_CNT, V_CNT);

	if(H_CNT == 0 && V_CNT == 0)
	{
//		printk("[%s::%d]NC_VI_NO_SIGNAL\n", __func__, __LINE__);
		return NC_VI_SIGNAL_OFF;
	}
	else
	{
//		printk("[%s::%d]NC_VI_SIGNAL_ON\n", __func__, __LINE__);
		return NC_VI_SIGNAL_ON;
	}
}

NC_U8 nc_drv_video_input_vfc_adjust_format_get(NC_U8 dev, NC_U8 chn, NC_U8 vfc)
{
	NC_U8 org_vfc 	 = 0;
	NC_U8 adjust_vfc = 0;

	int check_ahd = 0;
	int check_cvi = 0;

	int v_cnt = 0;
	int h_cnt = 0;
	int sync_max = 0;

	org_vfc 	= vfc;
	adjust_vfc  = vfc;

	check_ahd = nc_drv_video_ahd_vbi_get(dev, chn);
	check_cvi = nc_drv_video_cvi_vbi_get(dev, chn);
	h_cnt	  = nc_drv_video_h_cnt_get(dev, chn);
	v_cnt	  = nc_drv_video_v_cnt_get(dev, chn);
	sync_max  = nc_drv_video_sync_max_get(dev, chn);
	printk("check_ahd(%d), check_cvi(%d), sync_max(%d) h(%d) v(%d)\n", check_ahd, check_cvi, sync_max, h_cnt, v_cnt);

	if(check_ahd > 0)
	{
		if(org_vfc == 0x33)
		{
			/* adjust 0x57(TVI_2M_30P) -> 0x50(AHD_2M_30P) */
			adjust_vfc = 0x30;
		}
		else if(org_vfc == 0x57)
		{
			/* adjust 0x57(TVI_4M_30P) -> 0x50(AHD_4M_30P) */
			adjust_vfc = 0x50;
		}
		else
		{
			adjust_vfc = org_vfc;
		}
	}
	else
	{
		adjust_vfc = org_vfc;
	}

	/**********************************************
	 * AHD 4M 30P = 200M 1501, 400M 1497
	 * TVI 4M 30P = 200M 1506, 400M 1506
	 **********************************************/
	if(org_vfc == 0x57)
	{
		if( 1503 >= v_cnt && v_cnt >= 1496  ) // AHD 200M 1501
		{
			adjust_vfc = 0x50;
		}
	}

	return adjust_vfc;
}

NC_U8 nc_drv_video_input_vfc_cvi_5M_20p_adjust_format_get(NC_U8 dev, NC_U8 chn, NC_U8 vfc)
{
	NC_U8 org_vfc 	 = 0;
	NC_U8 adjust_vfc = 0;

#ifndef NVT_PLATFORM
	int check_ahd = 0;
	int check_cvi = 0;
#endif

	int v_cnt = 0;
#ifndef NVT_PLATFORM
	int h_cnt = 0;
	int sync_max = 0;
#endif

	org_vfc 	= vfc;
	adjust_vfc  = vfc;

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2b, 0x10);
	msleep(40);
//	check_ahd = nc_drv_video_ahd_vbi_get(dev, chn);
//	check_cvi = nc_drv_video_cvi_vbi_get(dev, chn);
#ifndef NVT_PLATFORM
	h_cnt	  = nc_drv_video_h_cnt_get(dev, chn);
#endif
	v_cnt	  = nc_drv_video_v_cnt_get(dev, chn);
//	sync_max  = nc_drv_video_sync_max_get(dev, chn);
//	printk("[%s::%d]chn(%d), check_ahd(%d), check_cvi(%d), sync_max(%d) h(%d) v(%d)\n", __func__, __LINE__, chn, check_ahd, check_cvi, sync_max, h_cnt, v_cnt);
//	printk("[%s::%d]chn(%d), h(%d) v(%d)\n", __func__, __LINE__, chn, h_cnt, v_cnt);
	/****************************************************************************************
	 * CVI 5M 20P : check_ahd(0), check_cvi(0), sync_max(15286) h(1875) v(1988)
	 * TVI 5M 20P : check_ahd(0), check_cvi(0), sync_max(30535) h(1875) v(1981)
	 ****************************************************************************************/
	if(org_vfc == 0x74)
	{
		if(v_cnt > 1986){
			adjust_vfc = 0x75;
		}
		else{
			adjust_vfc = 0x74;
		}
	}


	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2b, 0x00);

	return adjust_vfc;
}

void nc_drv_embedded_line_opt_set(NC_U8 chn, int opt)
{
	NC_U8 dev = chn/4;

	if(opt == 0)
	{
		/* No Video  */
        NC_U8 chipid =  nc_drv_common_info_chip_id_get(dev);

    	if(chipid == NC_DECODER_6188)
    	{
    		NC_U8 arb_dev = chn/2;
    		NC_U8 arb_chn = chn%2;
    		NC_U8 emb_arb = 0x00;
    		NC_U8 emb_arb_mask = ~(0x03<<(4*arb_chn));

    		/* Set 0 */
    		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*arb_dev));
    		emb_arb = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x1c);
    		emb_arb = emb_arb&emb_arb_mask;
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, emb_arb);

    	}
    	else if(chipid == NC_DECODER_6188C)
    	{
    		NC_U8 arb_dev = chn/4;
    		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*arb_dev));
    	}
	}
	else if(opt == 1)
	{
		/* On Video  */
        NC_U8 chipid =  nc_drv_common_info_chip_id_get(dev);

    	if(chipid == NC_DECODER_6188)
    	{
    		NC_U8 arb_dev = chn/2;
    		NC_U8 arb_chn = chn%2;
    		NC_U8 emb_arb = 0x00;
    		NC_U8 emb_arb_mask = ~(0x03<<(4*arb_chn));

    		/* Set 1 */
    		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*arb_dev));
    		emb_arb = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x1c);
    		emb_arb = emb_arb&emb_arb_mask;
    		emb_arb = emb_arb|(0x01<<(4*arb_chn));
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, emb_arb);

    	}
    	else if(chipid == NC_DECODER_6188C)
    	{
    		NC_U8 arb_dev = chn/4;
    		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*arb_dev));
    	}
	}

}

void nc_drv_video_input_arb_enable_set(NC_U8 dev, NC_U8 chn, int opt)
{
	NC_U8 chip_id = nc_drv_common_info_chip_id_get(dev);

	NC_U8 arb_dev = 0;
	NC_U8 arb_chn = 0;
	NC_U8 mask 	  = 0;


	if(opt == 0) // No_video
	{
		if(chip_id == NC_DECODER_6188)
		{
			arb_dev = chn/2;
			arb_chn = chn%2;
			mask	= 0;

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20 + (0x10*arb_dev));
			mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x00);
			mask = mask & ~(0x10<<(arb_chn*2));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, mask);
		}
		else if(chip_id == NC_DECODER_6188C)
		{
			arb_dev = chn/4;
			arb_chn = chn%4;

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20);
			mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x00);
			mask = mask & ~(0x10<<arb_chn);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, mask);
		}
		else
		{
			printk("[%s::%d]chip id get error!! %x\n", __FILE__, __LINE__, chip_id);
		}
	}
	else if(opt == 1) // On_video
	{
		if(chip_id == NC_DECODER_6188)
		{
			arb_dev = chn/2;
			arb_chn = chn%2;
			mask 	= 0;

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20 + (0x10*arb_dev));
			mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x00);
			mask = mask | (0x10<<(arb_chn*2));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, mask);
		}
		else if(chip_id == NC_DECODER_6188C)
		{
			arb_dev = chn/4;
			arb_chn = chn%4;

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20);
			mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x00);
			mask = mask | (0x10<<arb_chn);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, mask);
		}
		else
		{
			printk("[%s::%d]chip id get error!! %x\n", __FILE__, __LINE__, chip_id);
		}
	}
}

void nc_drv_video_input_vfc_status_get( void *pParam )
{
	NC_S32 ret = 0;
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 info_chn = pVdInfo->Chn;
	NC_U8 dev = pVdInfo->Chn/4;
	NC_U8 chn = pVdInfo->Chn%4;

	NC_U8 cur_vfc = 0;
	NC_U8 pre_vfc = 0;
	NC_U8 fmt_change = 0; // 0 : Not Change, 1 : Change

	/* common - device&channel number check  */
	if( (ret = nc_drv_common_total_chn_count_get(info_chn)) != 0 )
	{
		printk("[%s]nc_drv_common_total_chn_count_get error!!TotalChn>>>%d\n", __func__, ret);
		return;
	}

	/* Check if Coax firmware is being updated  */
	if(nc_drv_common_info_coax_fw_update_status_get(info_chn))
	{
		pVdInfo->FmtChange   = 0;
		return;
	}

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
	cur_vfc = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xF0);
	pre_vfc = nc_drv_common_info_vfc_get( info_chn );

	/**********************************************************
	 * No Video Status
	 * 0xFF, 0xF0, 0x0F
	 **********************************************************/
	if( cur_vfc == 0xFF || (cur_vfc&0xF0) == 0xF0 || (cur_vfc&0x0F) == 0x0F )
	{
		/****************************************************************
		 * 1. No Video Sequence
		 ****************************************************************/
		if( pre_vfc != 0xFF )
		{
			/**************************************************************************************
			 * DATA_OUT_MODE :: 0001  :  Background color output, Black Pattern
			 **************************************************************************************/
			NC_U8 mask = 0;
			NC_U8 output = 0x0F;
			NC_DEVICE_DRIVER_BANK_SET(dev, 0x00);
			mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7a + (chn/2));
			if(chn%2)
			{
				// Channel 1, 3, 5, 7, 9, 11, 13, 15
				output = (output<<4)|mask;
			}
			else
			{
				// Channel 0, 2, 4, 6, 8, 10, 12, 14
				output = output|mask;
			}
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a + (chn/2), output);

			/* On -> No video -----------*/
			NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0xB8);

			/* Raptor5_Add ***/
            NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
            mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x70);
            mask = mask &~(0x01<<chn);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x70, mask);  // FORMAT_SET_DONE_OFF

            /* DEC PRE/POST Clock ******************
             * Post 148.5,    PRE 74.25
             ***************************************/
            NC_DEVICE_DRIVER_BANK_SET(dev, 0x01);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x8C+chn, 0x40);

    		/* CVI 5M 20P *************************************************************/
    		mask = 0x00;
    		NC_DEVICE_DRIVER_BANK_SET(dev, 0x11);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0a+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0b+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14+(0x20 * chn), 0x00);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x15+(0x20 * chn), 0x00);

    		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb7, 0xfc);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xbb, 0x0f);

    		NC_DEVICE_DRIVER_BANK_SET(dev, 0x09);
    		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x44);
    		mask = mask &~(0x01<<chn);
    		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x44, mask);
    		/************************************************************************/

			nc_drv_common_info_chn_alive_set(info_chn, 0);
			fmt_change = 1;
			printk("[NC_DRV_VFC]NoVideo >>> Chn::%d[pre(0x%02X)->cur(0x%02X)]\n", info_chn, pre_vfc, cur_vfc);
		}

		cur_vfc = 0xff;
	}
	else
	{
		/****************************************************************
		 * 2. On Video Sequence
		 ****************************************************************/
		if( pre_vfc != cur_vfc )
		{
			if(pre_vfc==0x75 && cur_vfc==0x74)
			{
#ifndef NVT_PLATFORM
				NC_U8 mask = 0;
#endif
				NC_U8 adjust_vfc = 0;

				adjust_vfc = nc_drv_video_input_vfc_cvi_5M_20p_adjust_format_get(dev, chn, cur_vfc);

				if(adjust_vfc == 0x74)
				{
					/* Change CVI 5M 20P -> TVI 5M 20P */
					NC_U8 mask = 0;
#ifndef NVT_PLATFORM
					NC_U8 adjust_vfc = 0;
#endif

					NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
					mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x70);
					mask = mask | (0x01<<chn);
					gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x70, mask);  // FORMAT_SET_DONE_ON

					msleep(100);

					mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x71);
					mask = mask | (0x01<<chn);
					gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x71, mask);  // ACC_GET_DONE

					/* On Video Format Change  */
					fmt_change = 1;
					printk("[NC_DRV_VFC]OnVideo >>> Chn::%d[pre(0x%02X)->cur(0x%02X)]\n", info_chn, pre_vfc, cur_vfc);
				}
				else if(adjust_vfc == 0x75)
				{
					cur_vfc = 0x75;
					fmt_change = 0;
				}
			}
			else
			{
				/* Sequence 2.1 : No -> On video -----------------------*/
				/* Sequence 2.2 : On -> On video Format Changed --------*/
				NC_U8 mask = 0;
				NC_U8 adjust_vfc = 0;

				NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
				mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x70);
				mask = mask | (0x01<<chn);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x70, mask);  // FORMAT_SET_DONE_ON

				msleep(100);

				mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x71);
				mask = mask | (0x01<<chn);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x71, mask);  // ACC_GET_DONE

				/* Check CVI 5M 20P *****/
				if(cur_vfc == 0x74)
				{
					adjust_vfc = nc_drv_video_input_vfc_cvi_5M_20p_adjust_format_get(dev, chn, cur_vfc);
					if(adjust_vfc != cur_vfc)
					{
						printk("[NC_DRV_VFC]OnVideo >>> Chn::%d adjust[cur_vfc(0x%02X)->adjust_vfc(0x%02X)]\n", info_chn, cur_vfc, adjust_vfc);
						cur_vfc = adjust_vfc;
					}
				}

				/* On Video Format Change  */
				fmt_change = 1;
				printk("[NC_DRV_VFC]OnVideo >>> Chn::%d[pre(0x%02X)->cur(0x%02X)]\n", info_chn, pre_vfc, cur_vfc);
			}
		}
	}

	/***************************************************************************************
	 * Decoder channel information save
	 * 1. Save - VFC
	 * 2. Save - Video Format Define_enum, Standard_enum, Resolution_enum, Fps_enum
	 * 3. Save - Video Format String
	 * 4. Save - Sleep Time
	 * 5. Save - Video Cable - Currently fixed
	 ***************************************************************************************/
	nc_drv_common_info_video_format_set( info_chn, cur_vfc );
	nc_drv_common_info_cable_set(info_chn, CABLE_3C2V);

	/***************************************************************************************
	 * Driver -> Application information send
	 * 1. Format Change : 1 - Changed  /  0 - Not Changed
	 * 2. VFC Value
	 * 3. Format Define_enum
	 * 4. Format Character String
	 ***************************************************************************************/
	pVdInfo->FmtChange   = fmt_change;
	pVdInfo->Vfc 	     = cur_vfc;
	pVdInfo->VideoFormat = nc_drv_common_info_video_fmt_def_get(info_chn);
	nc_drv_common_info_video_fmt_string_get(info_chn, pVdInfo->FmtStr);

//	printk("[%s::%d]chn%d (%02x), (%d), (%s)\n", __func__, __LINE__, chn, pVdInfo->Vfc , pVdInfo->VideoFormat, pVdInfo->FmtStr);
}


NC_U8 nc_drv_int_eq_stage_max_get(NC_U8 chn, NC_U8 *pSrc, NC_U8 Size)
{
	int ii = 0;
	NC_U8 max = 0;
	NC_U8 max_eq_stage = 0;

	for(ii=0; ii<Size; ii++)
	{
//		printk("chn[%d] pSrc[%d]::%d\n", chn, ii, pSrc[ii]);
		if(max <= pSrc[ii])
		{
			max = pSrc[ii];
			max_eq_stage = ii;
		}
	}

	return max_eq_stage;
}

void nc_drv_video_input_eq_stage_get( void *pParam )
{
	NC_S32 ret = 0;

	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 info_chn = pVdInfo->Chn;
	NC_U8 dev 	   = pVdInfo->Chn/4;
	NC_U8 chn 	   = pVdInfo->Chn%4;

	NC_U8  eq_stage 	 = 0;
	NC_S32 sam_check_cnt = 0;
	NC_S32 sam_check_sel = 5;

	NC_U8  Stage_max 	 = 0;
	NC_U32 sam_value[10] = { 0, };
	NC_U8  Stage[10] 	 = { 0, };

	int ii = 0;

	NC_U32 pre_sam_update = 0;
	NC_U32 time_out_cnt   = 0;
	NC_U32 sam_avg 		  = 0;

	NC_VIVO_CH_FORMATDEF_E fmt  = nc_drv_common_info_video_fmt_def_get(info_chn);
	NC_CABLE_E cable 	 	    = nc_drv_common_info_cable_get(info_chn);
	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;


	/* common - device&channel number check */
	if( (ret = nc_drv_common_total_chn_count_get(info_chn)) != 0 )
	{
		printk("[%s]nc_drv_common_total_chn_count_get error!!TotalChn>>>%d\n", __func__, ret);
		return;
	}

	/***************************************************************************************
	 * 2. hsync_distortion get
	 ***************************************************************************************/
	while(1)
	{
		NC_U32 sam_value_cur = 0;

		sam_value_cur = internal_nc_drv_video_distance_hsync_distortion_get(dev, chn, 0, 0);
		if( (sam_value_cur > 0) && ( pre_sam_update != sam_value_cur))
		{
			NC_U8 sam_update = 0;

			sam_value[sam_check_cnt++] = sam_value_cur;
//			printk("[%s::%d]%d::%x\n", __FILE__, __LINE__, sam_check_cnt, sam_value[sam_check_cnt]);
//			sam_check_cnt++;

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
			sam_update = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7a);
			sam_update = sam_update|(0x01<<chn);	// Set 1[3:0]
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a, sam_update);

			sam_update = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7a);
			sam_update = sam_update&~(0x01<<chn);	// set 0[3:0]
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a, sam_update);

			pre_sam_update = sam_value_cur;
		}
		else
		{
			time_out_cnt++;
			if(time_out_cnt > 70)
			{
				printk("[%s::%d]Chn%d - EQ Stage Get time_out_cnt Error!!(%d / %x)\n", __func__, __LINE__, info_chn, sam_value_cur, sam_value_cur);
				return;
			}
		}

		if(sam_check_cnt >= sam_check_sel)
		{
#if 0
			int ii = 0;
			for(ii=0; ii<20; ii++)
			{
				sam_avg += sam_value[ii];
			}
			sam_avg = sam_avg/10;
//			sam_avg = (sam_value[0] + sam_value[1] + sam_value[2])/3;
#endif
			break;
		}

		msleep( 10 );
	}

	if( cable == CABLE_3C2V )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else if( cable == CABLE_UTP )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else
	{
		printk("[%s::%d]Video Cable Error!!\n", __FILE__, __LINE__);
		return;
	}

	for(ii=0; ii<sam_check_sel; ii++)
	{
		for( eq_stage=0; eq_stage<10; eq_stage++ )
		{
			if(sam_value[ii] >= stTableVideo->nc_table_eq_distance_value[eq_stage])
			{
//				printk("[%s::%d]Chn%d - %d_sam_value[%d, %x] > Stage[%d]\n", __func__, __LINE__, info_chn, ii, sam_value[ii], sam_value[ii], eq_stage);
				Stage[eq_stage]++;
				break;
			}
		}
	}

	Stage_max = nc_drv_int_eq_stage_max_get(chn, Stage, 10);
	pVdInfo->EqStage = Stage_max;

	/* Test Purpose */
	pVdInfo->SamVal = sam_avg;

#ifndef NVT_PLATFORM
	printk("[%s::%d]Chn%d - EqStage::%d\n", __func__, __LINE__, info_chn, Stage_max);
#else
	AD_IND("[%s::%d]Chn%d - EqStage::%d\n", __func__, __LINE__, info_chn, Stage_max);
#endif

	/********************************************************************************
	 * Save Common Information
	 ********************************************************************************/
	nc_drv_common_info_eq_stage_set(info_chn, Stage_max);
}

void nc_drv_video_input_add_8M_Ratio_4_3(void *pParam)
{
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pVdInfo->Chn/4;

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_11);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x0f);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02, 0x10);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03, 0x0c);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0xc0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x0f);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x78);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08, 0x60);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0a, 0x09);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0b, 0x90);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c, 0x09);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, 0xc4);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0e, 0x09);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0f, 0x90);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10, 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11, 0x60);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12, 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13, 0x90);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14, 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x15, 0x10);
}

void nc_drv_video_input_eq_stage_set( void *pParam )
{
	NC_S32 ret = 0;
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 info_chn = pVdInfo->Chn;
	NC_U8 dev = pVdInfo->Chn/4;
	NC_U8 chn = pVdInfo->Chn%4;

	NC_VIVO_CH_FORMATDEF_E fmt   = 0;
	NC_CABLE_E cable 			 = 0;
	NC_U8 eq_stage 			   	 = 0;

	NC_U8 mask = 0;
	NC_U8 output = 0x01;

	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;

	/* common - device&channel number check */
	if( (ret = nc_drv_common_total_chn_count_get(info_chn)) != 0 )
	{
		printk("[%s]nc_drv_common_total_chn_count_get error!!TotalChn>>>%d\n", __func__, ret);
		return;
	}

	if(nc_drv_common_info_chn_alive_get(pVdInfo->Chn) != 1)
	{
		printk("[%s]Video input is not set yet.!! chn(%d)\n", __func__, chn);
		return;
	}

	cable 	 = nc_drv_common_info_cable_get(info_chn);
	fmt      = nc_drv_common_info_video_fmt_def_get(info_chn);
	eq_stage = pVdInfo->EqStage;

	if( cable == CABLE_3C2V )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else if( cable == CABLE_UTP )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else
	{
		printk("\n");
		return;
	}

//	printk("[%s]info_chn(%d), Dev(%d), Chn(%d), Fmt(%d::%s), EqStage(%d), Cable(%d)\n", __func__, info_chn, dev, chn, fmt, stTableVideo->name, eq_stage, cable );

	/**************************************************************************************
	 * eq_base_set_value
	 **************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5+chn);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, stTableVideo->nc_table_eq_base_5x01_bypass[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58, stTableVideo->nc_table_eq_base_5x58_band_sel[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5c, stTableVideo->nc_table_eq_base_5x5c_gain_sel[eq_stage]);

	NC_DEVICE_DRIVER_BANK_SET(dev,  (chn < 2 ? BANK_A : BANK_B));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3d + (chn%2 * 0x80), stTableVideo->nc_table_eq_base_ax3d_deq_on[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3c + (chn%2 * 0x80), stTableVideo->nc_table_eq_base_ax3c_deq_a_sel[eq_stage]);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_9);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80 + (chn * 0x20), stTableVideo->nc_table_eq_base_9x80_deq_b_sel[eq_stage]);

	/**************************************************************************************
	 * eq_coeff_set_value
	 **************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev,  (chn < 2 ? BANK_A : BANK_B));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax30_deq_01[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax31_deq_02[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x32 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax32_deq_03[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x33 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax33_deq_04[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x34 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax34_deq_05[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x35 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax35_deq_06[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x36 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax36_deq_07[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x37 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax37_deq_08[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x38 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax38_deq_09[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x39 + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax39_deq_10[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3a + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax3a_deq_11[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3b + (chn%2 * 0x80), stTableVideo->nc_table_eq_coeff_ax3b_deq_12[eq_stage]);

	/**************************************************************************************
	 * eq_color_set_value
	 **************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10 + chn,   stTableVideo->nc_table_eq_color_0x10_contrast[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x18 + chn,   stTableVideo->nc_table_eq_color_0x18_h_peaking[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x21 + chn*4, stTableVideo->nc_table_eq_color_0x21_c_filter[eq_stage]);

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40 + chn, stTableVideo->nc_table_eq_color_0x40_hue[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x44 + chn, stTableVideo->nc_table_eq_color_0x44_u_gain[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48 + chn, stTableVideo->nc_table_eq_color_0x48_v_gain[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x4c + chn, stTableVideo->nc_table_eq_color_0x4c_u_offset[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50 + chn, stTableVideo->nc_table_eq_color_0x50_v_offset[eq_stage]);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x20, stTableVideo->nc_table_eq_color_5x20_black_level[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x27, stTableVideo->nc_table_eq_color_5x27_acc_ref[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28, stTableVideo->nc_table_eq_color_5x28_cti_delay[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2b, stTableVideo->nc_table_eq_color_5x2b_sub_saturation[eq_stage]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xd5, stTableVideo->nc_table_eq_color_5xd5_c_option[eq_stage]);

	/**************************************************************************************
	 * eq_timing_a_set_value
	 **************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58 + chn, stTableVideo->nc_table_eq_timing_0x58_h_delay_a[eq_stage]);

	/**************************************************************************************
	 * DATA_OUT_MODE :: 0001  :  Y_(001~254), Cb_(001~254), Cr_(001~254)
	 **************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, 0x00);
	mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7a + (chn/2));
	if(chn%2)
	{
		// Channel 1, 3, 5, 7, 9, 11, 13, 15
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7a + (chn/2));
		mask = mask&0x0F;
		output = (output<<4)|mask;
	}
	else
	{
		// Channel 0, 2, 4, 6, 8, 10, 12, 14
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7a + (chn/2));
		mask = mask&0xF0;
		output = output|mask;
	}
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a + (chn/2), output);

	nc_drv_common_info_eq_stage_set(chn, eq_stage);
}

void nc_drv_video_set_chn_x_mode_4mux(NC_U8 dev, NC_U8 chn, NC_U8 onoff)
{
#ifndef NVT_PLATFORM
	NC_U8 val_1xC0, val_20x0d, val_20x12, val_20x13, val_1xcd;
#else
	NC_U8 val_20x12, val_20x13;
#endif
	NC_U32 add_tmp, devide_tmp;
	NC_U8 arb_dev = 0; //mipi0
	NC_U8 arb_chn = chn%4;
	NC_DEVICE_DRIVER_BANK_SET(dev, 0x20 + (0x10*arb_dev));
	val_20x12 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x12+arb_chn*4);
	val_20x13 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x13+arb_chn*4);
	add_tmp = (val_20x13<<8) + val_20x12;

	if(1 == onoff)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x20 + (0x10*arb_dev));
		devide_tmp = add_tmp >> 1;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+arb_chn*2, (devide_tmp & 0xFF));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+arb_chn*2, ((devide_tmp>>8) & 0xFF));
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x01 + (0x30*arb_dev));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0+arb_chn*2, 0x88+0x11*(chn%4));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1+arb_chn*2, 0x88+0x11*(chn%4));
	}
	else
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x20 + (0x10*arb_dev));
		devide_tmp = add_tmp << 1;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+arb_chn*2, (devide_tmp & 0xFF));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+arb_chn*2, ((devide_tmp>>8) & 0xFF));
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x01 + (0x30*arb_dev));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0+arb_chn*2, 0x00+0x11*(chn%4));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1+arb_chn*2, 0x00+0x11*(chn%4));
	}
}

void nc_drv_video_set_chn_x_mode_2mx(NC_U8 dev, NC_U8 chn, NC_U8 onoff)
{
#ifndef NVT_PLATFORM
	NC_U8 val_1xC0, val_20x0d, val_20x12, val_20x13;
#else
	NC_U8 val_20x0d, val_20x12, val_20x13;
#endif

	NC_U32 add_tmp, devide_tmp;
	NC_U8 arb_dev = chn/2;
	NC_U8 arb_chn = chn%2;
	NC_DEVICE_DRIVER_BANK_SET(dev, 0x20 + (0x10*arb_dev));
	val_20x0d = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x0d);
	val_20x12 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x12+arb_chn*4);
	val_20x13 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x13+arb_chn*4);
	add_tmp = (val_20x13<<8) + val_20x12;

	if(1 == onoff)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x20 + (0x10*arb_dev));
		devide_tmp = add_tmp >> 1;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+arb_chn*4, (devide_tmp & 0xFF));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+arb_chn*4, ((devide_tmp>>8) & 0xFF));
		val_20x0d &= ~(0x04<<arb_chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, val_20x0d);
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x01 + (0x30*arb_dev));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0+arb_chn*4, 0x88+0x11*(chn%4));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1+arb_chn*4, 0x88+0x11*(chn%4));
	}
	else
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x20 + (0x10*arb_dev));
		devide_tmp = add_tmp << 1;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+arb_chn*4, (devide_tmp & 0xFF));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+arb_chn*4, ((devide_tmp>>8) & 0xFF));
		val_20x0d |= (0x04<<arb_chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, val_20x0d);
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x01 + (0x30*arb_dev));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0+arb_chn*4, 0x44+0x11*(chn%4));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1+arb_chn*4, 0x44+0x11*(chn%4));
	}
}

void nc_drv_video_arbiter_set(NC_U8 dev, NC_U8 chn, NC_VIDEO_FMT_INIT_TABLE_S *pstTableVideo )
{
	NC_U8 ChipId = nc_drv_common_info_chip_id_get(dev);
	NC_VO_WORK_MODE_E vo_mux = nc_drv_common_info_vo_mode_get(dev);
	if(ChipId != NC_DECODER_6188 && ChipId != NC_DECODER_6188C)
	{
		printk("[%s::%d]Error!!Unknown Chip ID - %x\n", __func__, __LINE__, ChipId);
		return;
	}

//	printk("[%s::%d]dev(%d), chn(%d), chip(%x::%s)\n", __func__, __LINE__, dev, chn, ChipId, nc_drv_common_chip_id_str_get(ChipId));

	//if(ChipId == NC_DECODER_6188)
	if(vo_mux  == NC_VO_WORK_MODE_2MUX)
	{
		NC_U8 bit_mask = 0x00;
		NC_U8 arb_scale_rd = 0x00;
		NC_U8 arb_scale_wr = 0x00;

		NC_U8 arb_dev = chn/2;
		NC_U8 arb_chn = chn%2;

		NC_DEVICE_DRIVER_BANK_SET(dev, 0x20 + (0x10*arb_dev));

		/* Arbiter Scale - Chn_1, Chn_3 */
		arb_scale_wr = pstTableVideo->nc_table_arb_20x01_scale_mode;
		arb_scale_wr = arb_scale_wr<<(arb_chn*4);

		arb_scale_rd = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x01);
		bit_mask = ~(0x03<<(arb_chn*4));
		arb_scale_rd = arb_scale_rd&bit_mask;
		arb_scale_wr = arb_scale_wr|arb_scale_rd;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, arb_scale_wr);

		/* Channel Merge - [4]Chn34, [3]Chn12   2M Under - 0, 2M Over - 1  */
		bit_mask 	 = 0x00;
		arb_scale_rd = 0x00;
		arb_scale_wr = 0x00;

		arb_scale_rd = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x0d);
		arb_scale_rd = arb_scale_rd&(~(0x04<<arb_chn));

		if(pstTableVideo->nc_table_arb_20x0d_ch_merge)
		{
			arb_scale_wr = arb_scale_rd|(0x04<<arb_chn);
		}
		else
		{
			arb_scale_wr = arb_scale_rd&(~(0x04<<arb_chn));
		}

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, arb_scale_wr);

		/* Arbiter Read Packet Size */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+(arb_chn*4), pstTableVideo->nc_table_arb_20x13_rd_packet_15_8);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+(arb_chn*4), pstTableVideo->nc_table_arb_20x12_rd_packet_7_0);
//		printk("[%s::%d]chn_rd_packet(%d)\n", __func__, __LINE__, (pstTableVideo->nc_table_arb_20x13_rd_packet_15_8<<8)|pstTableVideo->nc_table_arb_20x12_rd_packet_7_0);
	}
	//else if(ChipId == NC_DECODER_6188C)
	else if(vo_mux  == NC_VO_WORK_MODE_4MUX)
	{
		NC_U8 bit_mask 	   = 0x00;
		NC_U8 arb_scale_rd = 0x00;
		NC_U8 arb_scale_wr = 0x00;
		NC_U8 arb_dev; // 0x00:CSI 0; 0x01:CSI 1

		for (arb_dev = 0; arb_dev <= 1; arb_dev++) {

			NC_DEVICE_DRIVER_BANK_SET(dev, 0x20+0x10*arb_dev);

			/* Arbiter Scale */
			arb_scale_wr = pstTableVideo->nc_table_arb_20x01_scale_mode;
			arb_scale_wr = arb_scale_wr<<(chn*2);

			arb_scale_rd = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x01);
			bit_mask = ~(0x03<<(chn*2));
			arb_scale_rd = arb_scale_rd&bit_mask;
			arb_scale_wr = arb_scale_wr|arb_scale_rd;
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, arb_scale_wr);

			/* Channel Merge - 20x0d fixed 0x01  */
//			printk("[%s::%d]chn_merge(%02X)\n", __func__, __LINE__, gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x0d));

			/* Arbiter Read Packet Size */
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+(chn*2), pstTableVideo->nc_table_arb_20x13_rd_packet_15_8);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+(chn*2), pstTableVideo->nc_table_arb_20x12_rd_packet_7_0);
//			printk("[%s::%d]chn_rd_packet(%d)\n", __func__, __LINE__, (pstTableVideo->nc_table_arb_20x13_rd_packet_15_8<<8)|pstTableVideo->nc_table_arb_20x12_rd_packet_7_0);
		}
	}
	else
	{
		printk("[%s::%d]Error\n", __func__, __LINE__);
	}
}


void nc_drv_video_input_initialize_set(NC_U8 dev)
{
	//NC_U8 dev 		= 0;
	NC_U8 chn 		= 0;

	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;

	NC_VIVO_CH_FORMATDEF_E fmt = AHD_1080P_30P;
	NC_CABLE_E cable 	 	   = CABLE_3C2V;

	if( cable == CABLE_3C2V )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else if( cable == CABLE_UTP )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else
	{
		printk("\n");
		return;
	}

	if(stTableVideo == NULL)
	{
		printk("[%s::%d]Can not find Video Table!! Chn(%d)\n", __func__, __LINE__, chn);
		return;
	}
	else if( fmt == AHD_8M_CIF_25P || fmt == AHD_8M_CIF_30P ||\
			 fmt == CVI_HD_25P     || fmt == CVI_HD_30P     ||\
			 fmt == TVI_HD_B_25P   || fmt == TVI_HD_B_30P   || fmt == TVI_960P_25P   || fmt == TVI_960P_30P )
	{
		printk("[%s::%d]It is not yet ready Video Format!! Chn(%d), fmt(%s)\n", __func__, __LINE__, chn, stTableVideo->name);
		return;
	}

	for(chn=0; chn<4; chn++)
	{
		/****************************************************************
		 * video format setting
		 ****************************************************************/
		internal_nc_drv_video_input_format_init_set(dev, chn, fmt, 0, stTableVideo);

		nc_drv_video_arbiter_set(dev, chn, stTableVideo);
	}

}

void nc_drv_motion_over_size_format_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt)
{
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_2);

	if(fmt ==  AHD_1080P_12_5P || fmt == AHD_1080P_15P)
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+(0x07 * chn), 0x23);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28+(0x06 * chn), 0x11);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29+(0x06 * chn), 0xf0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2a+(0x06 * chn), 0x2d);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2b+(0x06 * chn), 0x06);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2c+(0x06 * chn), 0xea);
	}
	else if(fmt ==  AHD_720P_12_5P_EX_Btype || fmt == AHD_720P_15P_EX_Btype)
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+(0x07 * chn), 0x23);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28+(0x06 * chn), 0x11);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29+(0x06 * chn), 0xa0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2a+(0x06 * chn), 0x1e);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2b+(0x06 * chn), 0x06);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2c+(0x06 * chn), 0x9a);
	}
}

void internal_nc_drv_video_input_nrt_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt)
{
	NC_U8 mask = 0x00;

	if(fmt == AHD_1080P_12_5P || fmt == AHD_1080P_15P)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_11);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x00 + (chn*0x20));
		// exp_mode_1 0 -> 1
		mask = mask | 0x10;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00 + (chn*0x20), mask);

		nc_drv_motion_over_size_format_set(dev, chn, fmt);
	}
	else if(fmt == AHD_720P_12_5P_EX_Btype || fmt == AHD_720P_15P_EX_Btype)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_11);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x00 + (chn*0x20));
		// exp_mode_1 0 -> 1
		mask = mask | 0x10;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00 + (chn*0x20), mask);

		nc_drv_motion_over_size_format_set(dev, chn, fmt);
	}
	else
	{
		printk("[%s::%d]Not supported video format\n", __func__, __LINE__);
	}
}


void nc_drv_video_input_set(void *pParam)
{
	NC_S32 ret = 0;
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 info_chn 	= pVdInfo->Chn;
	NC_U8 dev 		= pVdInfo->Chn/4;
	NC_U8 chn 		= pVdInfo->Chn%4;

	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;

	NC_VIVO_CH_FORMATDEF_E fmt   = nc_drv_common_info_video_fmt_def_get(info_chn);
	NC_FORMAT_STANDARD_E fmt_std = nc_drv_common_info_video_fmt_standard_get(info_chn);
	NC_CABLE_E cable 	 	     = nc_drv_common_info_cable_get(info_chn);

	NC_VO_WORK_MODE_E mux_mode = nc_drv_common_info_vo_mode_get(chn/4);

	/* common - device&channel number check  */
	if( (ret = nc_drv_common_total_chn_count_get(info_chn)) != 0 )
	{
		printk("[%s]nc_drv_common_total_chn_count_get error!!TotalChn(%d), Chn(%d)\n", __func__, ret, info_chn);
		return;
	}

	if( cable == CABLE_3C2V )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else if( cable == CABLE_UTP )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else
	{
		printk("\n");
		return;
	}

	if(stTableVideo == NULL)
	{
		printk("[%s::%d]Can not find Video Table!! Chn(%d)\n", __func__, __LINE__, chn);
		return;
	}
	else if( fmt == AHD_8M_CIF_25P || fmt == AHD_8M_CIF_30P ||\
			 fmt == CVI_HD_25P     || fmt == CVI_HD_30P     ||\
			 fmt == TVI_HD_B_25P   || fmt == TVI_HD_B_30P   || fmt == TVI_960P_25P   || fmt == TVI_960P_30P )
	{
		printk("[%s::%d]It is not yet ready Video Format!! Chn(%d), fmt(%s)\n", __func__, __LINE__, chn, stTableVideo->name);
		return;
	}

//	printk("[%s::%d]Chn(%d), fmt(%s)\n", __func__, __LINE__, chn, stTableVideo->name);
	/****************************************************************
	 * Common Information
	 ****************************************************************/
	nc_drv_common_info_chn_alive_set(info_chn, 1);

	/****************************************************************
	 * video format setting
	 ****************************************************************/
	internal_nc_drv_video_input_format_set(dev, chn, fmt, mux_mode, stTableVideo);

	/****************************************************************
	 * NRT Setting - AHD 1M 12.5/15P, AHD 2M 12.5/15P
	 ****************************************************************/
	if(fmt == AHD_720P_15P_EX_Btype || fmt == AHD_720P_12_5P_EX_Btype || fmt == AHD_1080P_12_5P || fmt == AHD_1080P_15P)
	{
		internal_nc_drv_video_input_nrt_set(dev, chn, fmt);
	}
	else
	{
		NC_U8 mask = 0x00;
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_11);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x00 + (chn*0x20));
		// exp_mode_1 1 -> 0
		mask = mask&~(0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00 + (chn*0x20), mask);

		/* Motion Default*/
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_2);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+(0x07 * chn), 0x23);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28+(0x06 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29+(0x06 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2a+(0x06 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2b+(0x06 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2c+(0x06 * chn), 0x00);
	}

	/****************************************************************
	 * arbiter setting
	 ****************************************************************/
	nc_drv_video_arbiter_set(dev, chn, stTableVideo);

	/****************************************************************
	 * video eq_stage default setting -> stage 0
	 * setting sequence : video setting -> eq_stage 0 setting -> eq_stage get -> eq_stage set
	 ****************************************************************/
	pVdInfo->EqStage = 0;
	nc_drv_video_input_eq_stage_set(pVdInfo);

	/****************************************************************
	 * Coaxial Initialize
	 ****************************************************************/
	pVdInfo->Coax_ver = COAX_COAX_NORMAL;
	nc_drv_coax_initialize_set(pVdInfo);


	/****************************************************************
	 * AOC Initialize
	 ****************************************************************/
	if(nc_drv_audio_mode_get() == NC_AD_AOC && fmt_std == FMT_AHD)
	{
		nc_drv_audio_video_format_set(pVdInfo);
	}

	/****************************************************************
	 * Turn on the HPLL again, when finished video settings7
	 ****************************************************************/
	if( fmt_std == FMT_SD )
	{
		internal_nc_drv_video_sd_format_set(dev, chn, fmt);
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x04);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50, 0x84);

	}
	else if(fmt == CVI_5M_20P)
	{
#ifdef NVT_PLATFORM
		NC_U8 mask = 0x00;
#endif
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0x39);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0xee);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50, 0xc6);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62, 0x00);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb5, 0x40); // 200319 added

		/* H_ZOOM Off */
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x93 + chn, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98 + chn, 0x00);

#ifndef NVT_PLATFORM
		NC_U8 mask = 0x00;
#endif
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x11);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+(0x20 * chn), 0x0f);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01+(0x20 * chn), 0x01);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+(0x20 * chn), 0x60);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03+(0x20 * chn), 0x0a);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04+(0x20 * chn), 0x20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05+(0x20 * chn), 0x0e);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06+(0x20 * chn), 0xa6);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08+(0x20 * chn), 0x78);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0a+(0x20 * chn), 0x07);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0b+(0x20 * chn), 0x98);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c+(0x20 * chn), 0x07);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d+(0x20 * chn), 0xbc);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11+(0x20 * chn), 0xa0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+(0x20 * chn), 0xba);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x15+(0x20 * chn), 0x85);

		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb7, 0xf4);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xbb, 0x10);

		NC_DEVICE_DRIVER_BANK_SET(dev, 0x09);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x44);
		mask = mask | (0x01<<chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x44, mask);
	}
	else
	{
#ifdef NVT_PLATFORM
		NC_U8 mask = 0x00;
#endif
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0x39);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0xee);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50, 0xc6);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62, 0x00);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb5, 0x40); // 200319 added

		/* H_ZOOM Off */
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x93 + chn, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98 + chn, 0x00);

		/* CVI 5M 20P  */
#ifndef NVT_PLATFORM
		NC_U8 mask = 0x00;
#endif
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x11);
		if((fmt == AHD_1080P_15P) || (fmt == AHD_1080P_12_5P))
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+(0x20 * chn), 0x10);
		else
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0a+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0b+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14+(0x20 * chn), 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x15+(0x20 * chn), 0x00);

		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb7, 0xfc);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xbb, 0x0f);

		NC_DEVICE_DRIVER_BANK_SET(dev, 0x09);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x44);
		mask = mask &~(0x01<<chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x44, mask);

	}

}

void nc_drv_video_input_manual_set(void * pParam)
{
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 info_chn = pVdInfo->Chn;
	NC_U8 dev = pVdInfo->Chn/4;
	NC_U8 chn = pVdInfo->Chn%4;

	NC_FORMAT_STANDARD_E Fmt_std   = pVdInfo->FormatStandard;
	NC_FORMAT_RESOLUTION_E Fmt_res = pVdInfo->FormatResolution;
	NC_FORMAT_FPS_E	Fmt_fps		   = pVdInfo->FormatFps;
	NC_CABLE_E cable 			   = pVdInfo->VideoCable;
	NC_U8 eq_stage 				   = 0;

	NC_VIVO_CH_FORMATDEF_E fmt = nc_drv_table_video_format_get(Fmt_std, Fmt_res, Fmt_fps);

	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;

	if( cable == CABLE_3C2V )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else if( cable == CABLE_UTP )
	{
		stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	}
	else
	{
		printk("\n");
		return;
	}

	printk("[%s]info_chn(%d), Dev(%d), Chn(%d) Video Fmt(%d::%s)\n", __func__, info_chn, dev, chn, fmt, stTableVideo->name);

	/********************************************************************************
	 * Common Information Set
	 ********************************************************************************/
	nc_drv_common_info_video_format_manual_set(info_chn, fmt);
	nc_drv_common_info_eq_stage_set(info_chn, eq_stage);
	nc_drv_common_info_cable_set(info_chn, cable);

	/********************************************************************************
	 * Video Input Set
	 ********************************************************************************/
	nc_drv_video_input_set(pVdInfo);
}

void nc_drv_video_output_port_set( void *pParam )
{
	int ii = 0, time_mux;
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;
	NC_U8 dev = pVdInfo->Chn/4;
	NC_VO_WORK_MODE_E vo_mux = nc_drv_common_info_vo_mode_get(dev);

	NC_DECODER_CHIP_MODEL_E chip_id = nc_drv_common_info_chip_id_get(dev);
	printk("[%s::%d]chip[%d] vo_mux == %d\n", __func__, __LINE__, dev, vo_mux);
	/* Video Output Setting  */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x55, 0x10);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x56, 0x10);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xcc, 0x58);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xcd, 0x58);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xce, 0x58);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xcf, 0x58);

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC8, 0x00); // [7:4] B Port, [3:0] A Port
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC9, 0x00); // [7:4] D Port, [3:0] C Port
	//gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xCA, 0xFF); // [7:4] Clk_EN  [3:0] VD Out EN [4,3,2,1], [4,3,2,1]

	if(chip_id == NC_DECODER_6188)
		time_mux = 2;
	else
		time_mux = 1;

	if(vo_mux == NC_VO_WORK_MODE_2MUX)
	{
		for(ii=0; ii<time_mux; ii++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1 + (ii*0x30));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0, 0x44 + (ii*0x22));// Port A - chn1
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1, 0x44 + (ii*0x22));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC2, 0xcc + (ii*0x22));// Port B - chn2
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC3, 0xcc + (ii*0x22));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC4, 0x55 + (ii*0x22));// Port C - chn3
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC5, 0x55 + (ii*0x22));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC6, 0xdd + (ii*0x22));// Port D - chn4
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC7, 0xdd + (ii*0x22));

			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xed, 0x0f);

			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa4, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa5, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa6, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa7, 0x00);

			/* Arbiter 16bit mode */
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20 + (ii*0x10));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0xaa); // Channel 1, 3
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x0a); // Channel 1, 3
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, 0x6d);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40, 0x01);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40, 0x00);
		}
	}
	//else if(chip_id == NC_DECODER_6188C)
	else if(vo_mux  == NC_VO_WORK_MODE_4MUX)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0, 0x00); // Port A - chn1
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC2, 0x11); // Port B - chn2
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC3, 0x11);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC4, 0x22); // Port C - chn3
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC5, 0x22);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC6, 0x33); // Port D - chn4
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC7, 0x33);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xed, 0x00);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa4, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa5, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa6, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa7, 0x00);
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, 0x01);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40, 0x01);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40, 0x00);
	}
	else
	{
		printk("[%s::%d]Error\n", __func__, __LINE__);
		return;
	}
}

void nc_drv_video_output_port_manual_set( void *pParam )
{
	NC_S32 ret = 0;
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pVdInfo->Chn/4;
	NC_U8 chn = pVdInfo->Chn%4;

	NC_VO_WORK_MODE_E mux		= pVdInfo->VO_MuxMode;
	NC_VO_INTF_MODE_E interface = pVdInfo->VO_Interface;

	NC_U8 mux_mask = 0;
	NC_U8 set_val  = 0;

	NC_U8 ChnSeq_21 = pVdInfo->VO_ChnSeq[1]<<4 | pVdInfo->VO_ChnSeq[0];
	NC_U8 ChnSeq_43 = pVdInfo->VO_ChnSeq[3]<<4 | pVdInfo->VO_ChnSeq[2];


	printk("[%s]Chn(%d), Intf(%d), Mux(%d), ChnSeq(%02X, %02X)\n", __func__, chn, interface, mux, ChnSeq_21, ChnSeq_43);


	// common - device&channel number check
	if( (ret = nc_drv_common_total_chn_count_get(pVdInfo->Chn)) != 0 )
	{
		printk("[%s]nc_drv_common_total_chn_count_get error!!TotalChn>>>%d\n", __func__, ret);
		return;
	}

	/**************************************************************************************************
	 * Interface Mode Setting
	 * 1. Post YC Merge
	 * 	  - 5x69[4] Post YC Merge
	 *   	0 : Y, C_Merge      : BT656
	 *   	1 : Y, C_Seperation : BT1120
	 * 2. TM Mux_YC Merge
	 * 	  - 1xED[3:0] -> [1 : 2 : 3 : 4]
	 *   	0 : Y+C, Y+C	: BT656
	 *   	1 : Y, C_Merge : BT1120
	 **************************************************************************************************/
	if( interface == NC_VO_MODE_BT656 )
	{
		NC_U8 mask = 0;

		/* 1. Post YC Merge */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x69);
		mask = mask&0xEF; // Set 0[4] 0xEF 1110 1111
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x69, mask);

		/* 2. TM Mux_YC Merge */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xed);
		mask = mask&~(0x01<<chn);	// set 0[3:0]
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xed, mask);
	}
	else if( interface == NC_VO_MODE_BT1120_STANDARD )
	{
		NC_U8 mask = 0;

		/* 1. Post YC Merge */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x69);
		mask = mask|(0x01<<4);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x69, mask);

		/* 2. TM Mux_YC Merge */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xed);
		mask = mask|(0x01<<chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xed, mask);
	}

	/**************************************************************************************************
	 * Mux_Mode Setting
	 * 1. VPORT_CH_OUT_SEL
	 *    - 1xC8 [7:4], [3:0] -> [2 : 1]
	 *      1xC9 [7:4], [3:0] -> [4 : 3]
	 * 2. VCLK, VDO EN
	 *    - 1xCA [7:4] - VCLK[4, 3, 2, 1]
	 *   		 [3:0] -  VDO[4, 3, 2, 1]
	 **************************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
	mux_mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xc8 + (chn/2));

	if( mux == NC_VO_WORK_MODE_1MUX )
	{
		set_val = 0x0;

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x55, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x56, 0x32);
	}
	else if( mux == NC_VO_WORK_MODE_2MUX )
	{
		set_val = 0x2;

		if(ChnSeq_21 == 0x32)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x55, 0x10);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x56, 0x10);
		}
		else if(ChnSeq_21 == 0x21)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x55, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x56, 0x11);
		}
		else if(ChnSeq_21 == 0x10)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x55, 0x10);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x56, 0x32);
		}
	}
	else if( mux == NC_VO_WORK_MODE_4MUX )
	{
		set_val = 0x8;

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x55, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x56, 0x32);
	}
	else
		printk("[%s::%d]%d::Error!!\n", __func__, __LINE__, mux);

	if( chn%2 )
	{
		/* Channel 1, 3 */
		mux_mask = mux_mask&0x0F;
		mux_mask = mux_mask|(set_val<<4);
	}
	else
	{
		/* Channel 0, 2 */
		mux_mask = mux_mask&0xF0;
		mux_mask = mux_mask|(set_val);
	}

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC8 + (chn/2), mux_mask);

	mux_mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xCA);
	mux_mask = mux_mask|(0x11<<chn);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xCA, mux_mask);

	/**************************************************************************************************
	 * Video Output Port Sequence Mode
	 * Example Port A
	 * 		1xC0 : VPORT1_SEQ2[7:4] VPORT1_SEQ1[3:0]
	 * 		1xC0 : VPORT1_SEQ4[7:4] VPORT1_SEQ3[3:0]
	 *
	 * Normal Display of Channel 1 ~ 4 ( 0x00 ~ 0x04 )
	 * Only Y Display of Channel 1 ~ 4 ( 0x04 ~ 0x07 )
	 * H_CIF Display of Channel  1 ~ 4 ( 0x08 ~ 0x0B )
	 * Only C Display of Channel 1 ~ 4 ( 0x0C ~ 0x0F )
	 **************************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0 + (chn*2), ChnSeq_21);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1 + (chn*2), ChnSeq_43);

	nc_drv_common_info_vo_mode_set(pVdInfo->Chn, mux);
}

void nc_drv_video_output_color_pattern_set( void *pParam )
{
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pVdInfo->Chn/4;
	NC_U8 chn = pVdInfo->Chn%4;
	NC_U8 Set = pVdInfo->Value;

	NC_U8 bgdcol   = 0;
	NC_U8 chn_mask = 0;

	if( chn%2 )
		chn_mask = 0x0F; // chn_1_3
	else
		chn_mask = 0xF0; // chn_0_2

	if(Set)
	{
		/* Color Pattern ON */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		bgdcol = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x78 + (chn/2));
		bgdcol = bgdcol & chn_mask;
		if( chn%2 )
			bgdcol = bgdcol | (0x0A<<4);
		else
			bgdcol = bgdcol | 0x0A;

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x78 + (chn/2), bgdcol);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2c, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6a, 0x80);
	}
	else
	{
		/* Color Pattern OFF */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		bgdcol = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x78 + (chn/2));
		bgdcol = bgdcol & chn_mask;
		if( chn%2 )
			bgdcol = bgdcol | (0x08<<4);
		else
			bgdcol = bgdcol | 0x08;

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x78 + (chn/2), bgdcol);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2c, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6a, 0x00);
	}
}

void nc_drv_video_auto_manual_mode_set(void *pParam)
{
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;
	NC_U8 dev = pVdInfo->Chn/4;
	NC_VIDEO_SET_MODE_E OpMode = pVdInfo->OP_Mode;

	NC_U8 Chn = 0;

	if(OpMode == NC_VIDEO_SET_MODE_AUTO)
	{
		for(Chn=0; Chn <16; Chn++)
		{
			nc_drv_common_info_video_format_set(Chn, 0xee);
			nc_drv_common_info_coax_fw_status_set(Chn, 0);
		}

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x7f);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x70, 0xf0);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x18);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0x18);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02, 0x18);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03, 0x18);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03, 0x10);
	}
	else if(OpMode == NC_VIDEO_SET_MODE_MANUAL)
	{
		for(Chn=0; Chn <16; Chn++)
		{
			nc_drv_common_info_coax_fw_status_set(Chn, 1);
		}

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x70, 0x00);
	}
	else
	{
		printk("[%s::%d]Unknown Video Set Mode!!! %d\n", __FILE__, __LINE__, OpMode);
	}
}

void nc_drv_video_color_tune_get(void *pParam)
{
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;
	NC_U8 dev = pVdInfo->Chn/4;
	NC_U8 chn = pVdInfo->Chn%4;

	NC_VIDEO_COLOR_TUNE_E sel = pVdInfo->VD_Color_Tune_Sel;
	NC_U8 val = 0;

	if( sel == NC_VIDEO_BRIGHTNESS )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		val = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x78 + chn);
	}
	else if( sel == NC_VIDEO_CONTRAST )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		val = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x10 + chn);
	}
	else if( sel == NC_VIDEO_HUE )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		val = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x40 + chn);
	}
	else if( sel == NC_VIDEO_SATURATION )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		val = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x3c + chn);
	}
	else if( sel == NC_VIDEO_H_DELAY )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		val = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x58 + chn);
	}
	else
	{
		printk("[%s]Unknown Color Tuning Selection!!\n", __func__ );
	}

	 pVdInfo->Value = val;
}

void nc_drv_video_color_tune_set(void *pParam)
{
	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;
	NC_U8 dev = pVdInfo->Chn/4;
	NC_U8 chn = pVdInfo->Chn%4;

	NC_VIDEO_COLOR_TUNE_E sel = pVdInfo->VD_Color_Tune_Sel;
	NC_U8 val = pVdInfo->Value;

	if( sel == NC_VIDEO_BRIGHTNESS )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c + chn, val);
	}
	else if( sel == NC_VIDEO_CONTRAST )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10 + chn, val);
	}
	else if( sel == NC_VIDEO_HUE )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40 + chn, val);
	}
	else if( sel == NC_VIDEO_SATURATION )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3c + chn, val);
	}
	else if( sel == NC_VIDEO_H_DELAY )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58 + chn, val);
	}
	else
	{
		printk("[%s]Unknown Color Tuning Selection!!\n", __func__ );
	}
}

/***************************************************************************************
 * Internal Function
 *
 ***************************************************************************************/
void internal_nc_drv_video_input_format_init_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt, NC_VO_WORK_MODE_E mux_mode, NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo)
{
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x81 + chn, stTableVideo->nc_table_fmt_std_mode_set_0x81_sd_ahd);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x85 + chn, stTableVideo->nc_table_fmt_std_mode_set_0x85_special);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x89 + chn, stTableVideo->nc_table_fmt_std_timing_0x89_h_delay_b);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x8e + chn, stTableVideo->nc_table_fmt_std_timing_0x8e_h_delay_c);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa0 + chn, stTableVideo->nc_table_fmt_std_timing_0xa0_y_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x64 + chn, stTableVideo->nc_table_fmt_std_timing_0x64_v_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08 + chn, stTableVideo->nc_table_fmt_std_0x08_video_format);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c + chn, stTableVideo->nc_table_fmt_std_color_0x0c_brightness);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14 + chn, stTableVideo->nc_table_fmt_std_0x14_h_v_sharpness);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x23 + (chn*0x04), stTableVideo->nc_table_fmt_std_0x23_novideo_det);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x34 + chn, stTableVideo->nc_table_fmt_std_0x34_linemem_md);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3c + chn, stTableVideo->nc_table_fmt_std_0x3c_saturation);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5c + chn, stTableVideo->nc_table_fmt_std_0x5c_v_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30 + chn, stTableVideo->nc_table_fmt_std_0x30_y_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa4 + chn, stTableVideo->nc_table_fmt_std_0xa4_y_c_delay2);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x84 + chn, stTableVideo->nc_table_fmt_std_clock_1x84_vadc);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x8c + chn, stTableVideo->nc_table_fmt_std_clock_1x8c_post_pre);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x24, stTableVideo->nc_table_fmt_std_color_5x24_burst_dec_a);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5f, stTableVideo->nc_table_fmt_std_color_5x5f_burst_dec_b);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xd1, stTableVideo->nc_table_fmt_std_color_5xd1_burst_dec_c);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x90, stTableVideo->nc_table_fmt_std_timing_5x90_comb_mode);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb9, stTableVideo->nc_table_fmt_std_timing_5xb9_h_pll_op_a);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x57, stTableVideo->nc_table_fmt_std_timing_5x57_mem_path);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x25, stTableVideo->nc_table_fmt_std_timing_5x25_fsc_lock_speed);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, stTableVideo->nc_table_fmt_std_5x00_a_cmp);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, stTableVideo->nc_table_fmt_std_5x1b_comb_th_a);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1f, stTableVideo->nc_table_fmt_std_5x1f_y_c_gain);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x22, stTableVideo->nc_table_fmt_std_5x22_u_v_offset2);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x26, stTableVideo->nc_table_fmt_std_5x26_fsc_lock_sense);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50, stTableVideo->nc_table_fmt_std_5x50_none_);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5b, stTableVideo->nc_table_fmt_std_5x5b_vafe);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x64, stTableVideo->nc_table_fmt_std_5x64_mem_rdp_01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa9, stTableVideo->nc_table_fmt_std_5xa9_adv_stp_delay1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xc8, stTableVideo->nc_table_fmt_std_5xc8_y_sort_sel);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6e, stTableVideo->nc_table_fmt_std_5x6e_v_delay_ex_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6f, stTableVideo->nc_table_fmt_std_5x6f_v_delay_val);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7b, stTableVideo->nc_table_fmt_std_5x7b_v_rst_point);
	/* Raptor5 Added */
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6c, stTableVideo->nc_table_fmt_std_timing_5x6c_v_delay_manual_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6d, stTableVideo->nc_table_fmt_std_timing_5x6d_v_delay_manual_val);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6e, stTableVideo->nc_table_fmt_std_timing_5x6e_v_blk_end_manual_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6f, stTableVideo->nc_table_fmt_std_timing_5x6f_v_blk_end_manual_val);


	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_9);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x96 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x96_h_scaler1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x98_h_scaler3);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x99 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x99_h_scaler4);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9a + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9a_h_scaler5);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9b + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9b_h_scaler6);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9c + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9c_h_scaler7);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9d + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9d_h_scaler8);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9e + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9e_h_scaler9);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x97 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x97_h_scaler2);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40 + chn, stTableVideo->nc_table_fmt_std_timing_9x40_pn_auto);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x50_fsc_ext_val_1_7_0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x51 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x51_fsc_ext_val_1_15_8);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x52 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x52_fsc_ext_val_1_23_16);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x53 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x53_fsc_ext_val_1_31_24);

	NC_DEVICE_DRIVER_BANK_SET(dev,  (chn < 2 ? BANK_A : BANK_B));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x25 + (chn%2 * 0x80), stTableVideo->nc_table_fmt_std_color_ax25_y_filter_b);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x27 + (chn%2 * 0x80), stTableVideo->nc_table_fmt_std_color_ax27_y_filter_b_sel);
}

void internal_nc_drv_video_input_format_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt, NC_VO_WORK_MODE_E mux_mode, NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo)
{
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x81 + chn, stTableVideo->nc_table_fmt_std_mode_set_0x81_sd_ahd);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x85 + chn, stTableVideo->nc_table_fmt_std_mode_set_0x85_special);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x89 + chn, stTableVideo->nc_table_fmt_std_timing_0x89_h_delay_b);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x8e + chn, stTableVideo->nc_table_fmt_std_timing_0x8e_h_delay_c);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa0 + chn, stTableVideo->nc_table_fmt_std_timing_0xa0_y_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x64 + chn, stTableVideo->nc_table_fmt_std_timing_0x64_v_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08 + chn, stTableVideo->nc_table_fmt_std_0x08_video_format);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c + chn, stTableVideo->nc_table_fmt_std_color_0x0c_brightness);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14 + chn, stTableVideo->nc_table_fmt_std_0x14_h_v_sharpness);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x23 + (chn*0x04), stTableVideo->nc_table_fmt_std_0x23_novideo_det);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x34 + chn, stTableVideo->nc_table_fmt_std_0x34_linemem_md);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3c + chn, stTableVideo->nc_table_fmt_std_0x3c_saturation);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5c + chn, stTableVideo->nc_table_fmt_std_0x5c_v_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30 + chn, stTableVideo->nc_table_fmt_std_0x30_y_delay);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa4 + chn, stTableVideo->nc_table_fmt_std_0xa4_y_c_delay2);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x84 + chn, stTableVideo->nc_table_fmt_std_clock_1x84_vadc);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x8c + chn, stTableVideo->nc_table_fmt_std_clock_1x8c_post_pre);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x24, stTableVideo->nc_table_fmt_std_color_5x24_burst_dec_a);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5f, stTableVideo->nc_table_fmt_std_color_5x5f_burst_dec_b);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xd1, stTableVideo->nc_table_fmt_std_color_5xd1_burst_dec_c);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x90, stTableVideo->nc_table_fmt_std_timing_5x90_comb_mode);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb9, stTableVideo->nc_table_fmt_std_timing_5xb9_h_pll_op_a);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x57, stTableVideo->nc_table_fmt_std_timing_5x57_mem_path);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x25, stTableVideo->nc_table_fmt_std_timing_5x25_fsc_lock_speed);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, stTableVideo->nc_table_fmt_std_5x00_a_cmp);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, stTableVideo->nc_table_fmt_std_5x1b_comb_th_a);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1f, stTableVideo->nc_table_fmt_std_5x1f_y_c_gain);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x22, stTableVideo->nc_table_fmt_std_5x22_u_v_offset2);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x26, stTableVideo->nc_table_fmt_std_5x26_fsc_lock_sense);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50, stTableVideo->nc_table_fmt_std_5x50_none_);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5b, stTableVideo->nc_table_fmt_std_5x5b_vafe);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x64, stTableVideo->nc_table_fmt_std_5x64_mem_rdp_01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa9, stTableVideo->nc_table_fmt_std_5xa9_adv_stp_delay1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xc8, stTableVideo->nc_table_fmt_std_5xc8_y_sort_sel);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6e, stTableVideo->nc_table_fmt_std_5x6e_v_delay_ex_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6f, stTableVideo->nc_table_fmt_std_5x6f_v_delay_val);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7b, stTableVideo->nc_table_fmt_std_5x7b_v_rst_point);

	/* Raptor5 Added */
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6c, stTableVideo->nc_table_fmt_std_timing_5x6c_v_delay_manual_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6d, stTableVideo->nc_table_fmt_std_timing_5x6d_v_delay_manual_val);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6e, stTableVideo->nc_table_fmt_std_timing_5x6e_v_blk_end_manual_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x6f, stTableVideo->nc_table_fmt_std_timing_5x6f_v_blk_end_manual_val);

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_9);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x96 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x96_h_scaler1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x98_h_scaler3);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x99 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x99_h_scaler4);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9a + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9a_h_scaler5);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9b + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9b_h_scaler6);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9c + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9c_h_scaler7);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9d + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9d_h_scaler8);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x9e + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x9e_h_scaler9);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x97 + (chn * 0x20), stTableVideo->nc_table_fmt_std_timing_9x97_h_scaler2);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40 + chn, stTableVideo->nc_table_fmt_std_timing_9x40_pn_auto);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x50_fsc_ext_val_1_7_0);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x51 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x51_fsc_ext_val_1_15_8);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x52 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x52_fsc_ext_val_1_23_16);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x53 + (chn*0x04), stTableVideo->nc_table_fmt_std_9x53_fsc_ext_val_1_31_24);

	NC_DEVICE_DRIVER_BANK_SET(dev,  (chn < 2 ? BANK_A : BANK_B));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x25 + (chn%2 * 0x80), stTableVideo->nc_table_fmt_std_color_ax25_y_filter_b);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x27 + (chn%2 * 0x80), stTableVideo->nc_table_fmt_std_color_ax27_y_filter_b_sel);
}

NC_U32 internal_nc_drv_video_distance_hsync_distortion_get( NC_U8 dev, NC_U8 chn, NC_U32 *Sam1Frame, NC_U32 *Sam8Frame )
{
	NC_U32 hsync_distortion_sum = 0;
	NC_U32 hsync_distortion_falling = 0;
	NC_U32 hsync_distortion_rising = 0;
	NC_U8 hsync_7_0 = 0;
	NC_U8 hsync_15_8 = 0;
	NC_U8 hsync_23_16 = 0;
	NC_U8 hsync_31_24 = 0;

	/***************************************************************************************
	 * hsync_falling & rising distortion read - 1 frame
	 ***************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B, chn);

	/* 1. falling distortion read */
	hsync_7_0   = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD0);
	hsync_15_8  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD1);
	hsync_23_16 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD2);
	hsync_31_24 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD3);
	hsync_distortion_falling = ((hsync_31_24 << 24) | (hsync_23_16 << 16) | (hsync_15_8 << 8) | hsync_7_0);

	/* 2. rising distortion read */
	hsync_7_0   = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD4);
	hsync_15_8  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD5);
	hsync_23_16 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD6);
	hsync_31_24 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD7);
	hsync_distortion_rising = ((hsync_31_24 << 24) | (hsync_23_16 << 16) | (hsync_15_8 << 8) | hsync_7_0);

	/* 3. falling - rising */
	hsync_distortion_sum = hsync_distortion_falling - hsync_distortion_rising;

#if 1
	if(hsync_distortion_sum != 0)
#ifndef NVT_PLATFORM
		printk("f(%x) - r(%x) = s(%x)\n", hsync_distortion_falling, hsync_distortion_rising, hsync_distortion_sum);
#else
		AD_IND("f(%x) - r(%x) = s(%x)\n", hsync_distortion_falling, hsync_distortion_rising, hsync_distortion_sum);
#endif
#endif

	return hsync_distortion_sum;
}

NC_U32 internal_nc_drv_video_5chfbt_distance_hsync_distortion_get( NC_U8 dev, NC_U8 chn, NC_U32 *Sam1Frame, NC_U32 *Sam8Frame )
{
	NC_U32 hsync_distortion_sum = 0;
	NC_U32 hsync_distortion_falling = 0;
	NC_U32 hsync_distortion_rising = 0;
	NC_U8 hsync_7_0 = 0;
	NC_U8 hsync_15_8 = 0;
	NC_U8 hsync_23_16 = 0;
	NC_U8 hsync_31_24 = 0;

	/***************************************************************************************
	 * hsync_falling & rising distortion read - 1 frame
	 ***************************************************************************************/
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B, chn);

	/* 1. falling distortion read */
	hsync_7_0   = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD0);
	hsync_15_8  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD1);
	hsync_23_16 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD2);
	hsync_31_24 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD3);
	hsync_distortion_falling = ((hsync_31_24 << 24) | (hsync_23_16 << 16) | (hsync_15_8 << 8) | hsync_7_0);

	/* 2. rising distortion read */
	hsync_7_0   = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD4);
	hsync_15_8  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD5);
	hsync_23_16 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD6);
	hsync_31_24 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xD7);
	hsync_distortion_rising = ((hsync_31_24 << 24) | (hsync_23_16 << 16) | (hsync_15_8 << 8) | hsync_7_0);

	/* 3. falling - rising */
	hsync_distortion_sum = hsync_distortion_falling - hsync_distortion_rising;

	return hsync_distortion_sum;
}

void internal_nc_drv_video_sd_format_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt)
{
	NC_U8 chn_mask = 0;

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
	if( fmt == SD_H960_2EX_Btype_NT || fmt == SD_H960_2EX_NT || fmt == SD_H960_NT )
	{
		chn_mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x54);
		if(chn_mask & 0x01<<(4+chn))
		{
			/* if chn_bit 1 -> 0 */
			chn_mask = chn_mask ^ (0x01<<(4+chn));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x54, chn_mask);
		}
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08 + chn, 0xa0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5c + chn, 0xbc);
	}
	else if( fmt == SD_H960_2EX_Btype_PAL || fmt == SD_H960_2EX_PAL || fmt == SD_H960_EX_PAL || fmt == SD_H960_PAL )
	{
		chn_mask = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x54);

		/* chn_bit 0 -> 1 */
		chn_mask = chn_mask|(0x01<<(4+chn));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x54, chn_mask);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08 + chn, 0xdd);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5c + chn, 0xbe);
	}

#if 0
	if( fmt == SD_H960_NT  || fmt == SD_H960_EX_NT  || fmt == SD_H960_2EX_NT ||\
		fmt == SD_H960_PAL || fmt == SD_H960_EX_PAL || fmt == SD_H960_2EX_PAL )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0xb9);
	}
	else
#endif
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0x39);
	}


	if( fmt == SD_H960_2EX_NT  )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62, 0x00);
	}
	else
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62, 0x20);
	}

	/* 200319 Added */
	if( fmt == SD_H960_2EX_NT || fmt == SD_H960_2EX_PAL  )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb5, 0x00);

		/* H_ZOOM ON */
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x93 + chn, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98 + chn, 0x07);
	}
	else
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x05 + chn);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb5, 0x20);

		/* H_ZOOM Off */
		NC_DEVICE_DRIVER_BANK_SET(dev, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x93 + chn, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98 + chn, 0x00);
	}
}

void internal_nc_drv_video_output_hide_set( NC_U8 Dev, NC_U8 Chn, NC_U8 Val )
{
	NC_U8 dev = Dev;
	NC_U8 chn = Chn;
	NC_U8 Set = Val;

	NC_U8 output_mode = 0;
	NC_U8 chn_mask    = 0;

	/****************************************************************
	 * 0x01 : Y(001~254), Cb_(001~254), Cr_(001~254)
	 * 0x0F : Background color output  <<<<<  BGDCOL 0x78, 0x79
	 ****************************************************************/

	if( chn%2 )
		chn_mask = 0x0F; // chn_1_3
	else
		chn_mask = 0xF0; // chn_0_2

	NC_DEVICE_DRIVER_BANK_SET(dev, 0x00);

	if( chn == 0 || chn == 1 )
	{
		output_mode = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7A);
		output_mode = output_mode & chn_mask;
		output_mode = output_mode | Set;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7A, output_mode);
	}
	else if( chn == 2 || chn == 3 )
	{
		output_mode = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x7B);
		output_mode = output_mode & chn_mask;
		output_mode = output_mode | Set;
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7B, output_mode);
	}
	else
	{
		printk("[%s]Error!!\n", __func__);
		return;
	}
}

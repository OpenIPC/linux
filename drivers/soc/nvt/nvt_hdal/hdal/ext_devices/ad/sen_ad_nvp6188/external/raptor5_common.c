
/* ----------------------------------------------------------------------------------
 * 1. Header file include -----------------------------------------------------------
 * --------------------------------------------------------------------------------*/
#include "raptor5_common.h"
#include "raptor5_table.h"

/* ----------------------------------------------------------------------------------
 * 2. Define ------------------------------------------------------------------------
 * --------------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------------
 * 3. Define variable ---------------------------------------------------------------
 * --------------------------------------------------------------------------------*/
static nc_decoder_info_s s_nc_decoder_info;


/* ----------------------------------------------------------------------------------
 * 4. External variable & function --------------------------------------------------
 * --------------------------------------------------------------------------------*/



/* ----------------------------------------------------------------------------------
 * 5. Function prototype ------------------------------------------------------------
 * --------------------------------------------------------------------------------*/


/******************************************************************************
 * Decoder Each Channel information Control
 * VFC, distance
 ******************************************************************************/
void nc_drv_common_info_status_onvideo_set(NC_U8 chn, NC_U8 status)
{
	s_nc_decoder_info.on_video_set[chn] = status;
}

NC_U8 nc_drv_common_info_status_onvideo_get(NC_U8 chn)
{
	return s_nc_decoder_info.on_video_set[chn];
}

void nc_drv_common_info_chip_data_init_set( int ChipCnt, NC_U8 *id, NC_U8 *rev, NC_U8 *addr, NC_VO_WORK_MODE_E vomux )
{
	NC_S32 ii = 0;

	memset( &s_nc_decoder_info, 0, sizeof(nc_decoder_info_s));

	s_nc_decoder_info.Total_Chip_Cnt = ChipCnt;
	s_nc_decoder_info.Total_Chn_Cnt  = ChipCnt * 4;

	for(ii=0; ii<4; ii++)
	{
		s_nc_decoder_info.chip_id[ii]   = id[ii];
		s_nc_decoder_info.chip_rev[ii]  = rev[ii];
		s_nc_decoder_info.chip_addr[ii] = addr[ii];

		if(s_nc_decoder_info.chip_id[ii]==NC_DECODER_6188C)
			nc_drv_common_info_vo_mode_set(ii, NC_VO_WORK_MODE_4MUX);
		else
			nc_drv_common_info_vo_mode_set(ii, vomux);
	}

	/* Channel VFC Value Initialize */
	for(ii=0; ii<16; ii++)
	{
		s_nc_decoder_info.vfc[ii] = 0xEE;
	}

}



void nc_drv_common_info_video_format_set( NC_U8 chn, NC_U8 vfc )
{
	NC_S8 fmt_name[256] = {0, };
	NC_VIVO_CH_FORMATDEF_E fmt = 0;
	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;
#ifndef NVT_PLATFORM
	NC_VO_WORK_MODE_E mux_mode  = 0;
#endif

	/***************************************************************************************
	 * 1. VFC Status - Save
	 ***************************************************************************************/
	s_nc_decoder_info.vfc[chn] = vfc;

	/***************************************************************************************
	 * 2. Video format character string - Save
	 ***************************************************************************************/
	fmt = nc_drv_table_vfc_to_formatdef_get( vfc, (char *)fmt_name);
	memcpy(s_nc_decoder_info.fmt_name[chn], fmt_name, sizeof(fmt_name));

#ifndef NVT_PLATFORM
	mux_mode = nc_drv_common_info_vo_mode_get(chn);
#endif

	/***************************************************************************************
	 * 3. Video format enum_ - Save
	 ***************************************************************************************/
	s_nc_decoder_info.fmt_video[chn] = fmt;
	stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);

	s_nc_decoder_info.fmt_standard[chn]   = stTableVideo->fmt_standard;
	s_nc_decoder_info.fmt_resolution[chn] = stTableVideo->fmt_resolution;
	s_nc_decoder_info.fmt_fps[chn]        = stTableVideo->fmt_fps;

	/***************************************************************************************
	 * 4. Sleep time - Save
	 ***************************************************************************************/
	switch( s_nc_decoder_info.fmt_fps[chn] )
	{
		case    FMT_PAL  	: 	s_nc_decoder_info.sleep[chn] = 33;  	break;
		case    FMT_NT  	: 	s_nc_decoder_info.sleep[chn] = 33;  	break;
		case    FMT_7_5P  	: 	s_nc_decoder_info.sleep[chn] = 133;  	break;
		case    FMT_12_5P  	: 	s_nc_decoder_info.sleep[chn] = 80;  	break;
		case    FMT_15P  	: 	s_nc_decoder_info.sleep[chn] = 66;  	break;
		case    FMT_18P  	: 	s_nc_decoder_info.sleep[chn] = 55;  	break;
		case    FMT_18_75P  : 	s_nc_decoder_info.sleep[chn] = 53;  	break;
		case    FMT_20P  	: 	s_nc_decoder_info.sleep[chn] = 50;  	break;
		case    FMT_25P  	: 	s_nc_decoder_info.sleep[chn] = 40;  	break;
		case    FMT_30P  	: 	s_nc_decoder_info.sleep[chn] = 33;  	break;
		case    FMT_50P  	: 	s_nc_decoder_info.sleep[chn] = 20;  	break;
		case    FMT_60P  	: 	s_nc_decoder_info.sleep[chn] = 16;  	break;
		default : 				s_nc_decoder_info.sleep[chn] = 40;  	break;
	}

}

void nc_drv_common_info_video_format_manual_set( NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt )
{
	NC_U8 vfc = 0;
	NC_S8 fmt_name[256] = {0, };
	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;

	/***************************************************************************************
	 * 1. VFC - Save
	 ***************************************************************************************/
	vfc = nc_drv_table_formatdef_to_vfc_get(fmt ,(char *)fmt_name);
	s_nc_decoder_info.vfc[chn] = vfc;

	/***************************************************************************************
	 * 2. Video format character string - Save
	 ***************************************************************************************/
	memcpy(s_nc_decoder_info.fmt_name[chn], fmt_name, sizeof(fmt_name));

	/***************************************************************************************
	 * 3. Video format enum_ - Save
	 ***************************************************************************************/
	stTableVideo = (NC_VIDEO_FMT_INIT_TABLE_S*)nc_drv_table_video_init_vlaue_get(fmt);
	s_nc_decoder_info.fmt_video[chn] = fmt;
	s_nc_decoder_info.fmt_standard[chn]   = stTableVideo->fmt_standard;
	s_nc_decoder_info.fmt_resolution[chn] = stTableVideo->fmt_resolution;
	s_nc_decoder_info.fmt_fps[chn]        = stTableVideo->fmt_fps;

	/***************************************************************************************
	 * 4. Sleep time - Save
	 ***************************************************************************************/
	switch( s_nc_decoder_info.fmt_fps[chn] )
	{
		case    FMT_50P  	:
		case    FMT_60P  	:
				s_nc_decoder_info.sleep[chn] = 20;  	break;
		case    FMT_PAL  	:
		case    FMT_NT  	:
		case    FMT_25P  	:
		case    FMT_30P  	:
				s_nc_decoder_info.sleep[chn] = 40;  	break;
		case    FMT_20P  	:
				s_nc_decoder_info.sleep[chn] = 50;  	break;
		case    FMT_18P  	:
		case    FMT_18_75P  :
				s_nc_decoder_info.sleep[chn] = 60;  	break;
		case    FMT_15P  	:
				s_nc_decoder_info.sleep[chn] = 70;  	break;
		case    FMT_12_5P  	:
				s_nc_decoder_info.sleep[chn] = 80;  	break;
		case    FMT_7_5P  	:
				s_nc_decoder_info.sleep[chn] = 150;  	break;
		default :
				s_nc_decoder_info.sleep[chn] = 40;  	break;
	}

}

void nc_drv_common_info_chn_alive_set( NC_U8 chn, NC_U8 alive )
{
	s_nc_decoder_info.chn_alive[chn] = alive;
}

void nc_drv_common_info_cable_set( NC_U8 chn, NC_CABLE_E cable )
{
	s_nc_decoder_info.fmt_cable[chn] = cable;
}

void nc_drv_common_info_eq_stage_set( NC_U8 chn, NC_U8 eq_stage )
{
	s_nc_decoder_info.eq_stage[chn] = eq_stage;
}

void nc_drv_common_info_video_fmt_def_set( NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt )
{
	s_nc_decoder_info.fmt_video[chn] = fmt;
}

void nc_drv_common_info_video_fmt_standard_set( NC_U8 chn, NC_FORMAT_STANDARD_E  fmtstd )
{
	s_nc_decoder_info.fmt_standard[chn] = fmtstd;
}

void nc_drv_common_info_video_fmt_resolusion_set( NC_U8 chn, NC_FORMAT_RESOLUTION_E fmtres )
{
	s_nc_decoder_info.fmt_resolution[chn] = fmtres;
}

void nc_drv_common_info_video_fmt_fps_set( NC_U8 chn, NC_FORMAT_FPS_E fmtfps )
{
	s_nc_decoder_info.fmt_fps[chn] = fmtfps;
}

void nc_drv_common_info_coax_fw_status_set( NC_U8 chn, NC_U8 status )
{
	s_nc_decoder_info.coax_fw_status[chn] = status;
}

void nc_drv_common_info_vo_mode_set(NC_U8 dev, NC_VO_WORK_MODE_E mode)
{
	s_nc_decoder_info.vo_mode[dev] = mode;
}

/*************************************************************************************************
 * Decoder Information Get
 *************************************************************************************************/
NC_VO_WORK_MODE_E nc_drv_common_info_vo_mode_get(NC_U8 dev)
{
	return s_nc_decoder_info.vo_mode[dev];
}

NC_U8 nc_drv_common_info_chip_id_get(NC_U8 ChipNo)
{
	return s_nc_decoder_info.chip_id[ChipNo];
}

NC_U8 nc_drv_common_info_chn_alive_get( NC_U8 chn )
{
	return s_nc_decoder_info.chn_alive[chn];
}

void nc_drv_common_info_video_fmt_string_get( NC_U8 chn, char *pstr )
{
	NC_S8 str[256];
	memcpy(pstr, s_nc_decoder_info.fmt_name[chn], sizeof(str));
}

NC_U8 nc_drv_common_total_chn_count_get( NC_U8 chn )
{
	NC_U8 ret = 0;

	if( s_nc_decoder_info.Total_Chn_Cnt <= chn )
		return s_nc_decoder_info.Total_Chn_Cnt;

	return ret;
}

NC_U8 nc_drv_common_info_vfc_get( NC_U8 chn )
{
	return s_nc_decoder_info.vfc[chn];
}

NC_U8 nc_drv_common_info_eq_stage_get( NC_U8 chn )
{
	return s_nc_decoder_info.eq_stage[chn];
}


NC_CABLE_E nc_drv_common_info_cable_get( NC_U8 chn )
{
	return s_nc_decoder_info.fmt_cable[chn];
}

NC_VIVO_CH_FORMATDEF_E nc_drv_common_info_video_fmt_def_get( NC_U8 chn )
{
	return s_nc_decoder_info.fmt_video[chn];
}

NC_FORMAT_STANDARD_E nc_drv_common_info_video_fmt_standard_get( NC_U8 chn )
{
	return s_nc_decoder_info.fmt_standard[chn];
}

NC_FORMAT_RESOLUTION_E nc_drv_common_info_video_fmt_resolusion_get( NC_U8 chn )
{
	return s_nc_decoder_info.fmt_resolution[chn];
}

NC_FORMAT_FPS_E nc_drv_common_info_video_fmt_fps_get( NC_U8 chn )
{
	return s_nc_decoder_info.fmt_fps[chn];
}

NC_U8 nc_drv_common_info_coax_fw_update_status_get( NC_U8 chn )
{
	return s_nc_decoder_info.coax_fw_status[chn];
}


/**************************************************************************************
 * get enum_string Function
 *
 ***************************************************************************************/
char *nc_drv_common_chip_id_str_get(NC_U8 id)
{
	if(id == 0xd0) 		return "NVP6188";
	else if(id == 0xd2) return "NVP6188C";
	else if(id == 0xd3) return "NVP6188D";
	else				return "Unknown ID";

	return "Unknown ID";
}

char *nc_drv_common_mipi_lane_str_get(NC_MIPI_LANE_E lane)
{
	if(lane == NC_MIPI_LANE_1) 		return "NC_MIPI_LANE_1";
	else if(lane == NC_MIPI_LANE_2) return "NC_MIPI_LANE_2";
	else if(lane == NC_MIPI_LANE_4) return "NC_MIPI_LANE_4";
	else				return "Unknown LANE";

	return "Unknown ID";
}

char *nc_drv_common_mipi_clock_str_get(NC_MIPI_BPS_E bps)
{
	if(bps == NC_MIPI_BPS_594Mbps) 			return "NC_MIPI_BPS_594Mbps";
	else if(bps == NC_MIPI_BPS_756Mbps)	return "NC_MIPI_BPS_756Mbps";
	else if(bps == NC_MIPI_BPS_972Mbps) 	return "NC_MIPI_BPS_972Mbps";
	else if(bps == NC_MIPI_BPS_1242Mbps) 	return "NC_MIPI_BPS_1242Mbps";
	else if(bps == NC_MIPI_BPS_1458Mbps) 	return "NC_MIPI_BPS_1458Mbps";
	else				return "Unknown CLOCK";

	return "Unknown ID";
}


/**************************************************************************************
 * Timer Function
 *
 *
 ***************************************************************************************/
void nc_drv_common_sleep_set( NC_U8 chn, NC_U8 frame )
{
	NC_U32 sleep = 0;

	sleep = s_nc_decoder_info.sleep[chn] * frame;

	msleep(sleep);
}


/*******************************************************************************
 *	End of file
 *******************************************************************************/

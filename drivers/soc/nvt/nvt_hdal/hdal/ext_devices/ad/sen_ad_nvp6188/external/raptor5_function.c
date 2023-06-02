
/* ----------------------------------------------------------------------------------
 * 1. Header file include -----------------------------------------------------------
 * --------------------------------------------------------------------------------*/
#include "raptor5_common.h"
#include "raptor5_function.h"
#include "raptor5_table.h"

/* ----------------------------------------------------------------------------------
 * 2. Define ------------------------------------------------------------------------
 * --------------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------------
 * 3. Define variable ---------------------------------------------------------------
 * --------------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------------
 * 4. External variable & function --------------------------------------------------
 * --------------------------------------------------------------------------------*/


/* ----------------------------------------------------------------------------------
 * 5. Function prototype ------------------------------------------------------------
 * --------------------------------------------------------------------------------*/


/*=======================================================================================================
 * Coaxial Protocol function
 ========================================================================================================*/
/*=======================================================================================================
 * Coaxial protocol up stream Flow
 * Coaxial protocol up stream register(example: channel 0)
 * (3x00) tx_baud               : 1 bit duty
 * (3x02) tx_pel_baud           : 1 bit duty of pelco(SD)
 * (3x03) tx_line_pos0          : up stream line position(low)
 * (3x04) tx_line_pos1          : up stream line position(high)
 * (3x05) tx_line_count         : up stream output line number in 1 frame
 * (3x07) tx_pel_line_pos0      : up stream line position of pelco(low)
 * (3x08) tx_pel_line_pos1      : up stream line position of pelco(high)
 * (3x0A) tx_line_count_max     : up stream output total line
 * (3x0B) tx_mode               : up stream Mode set (ACP, CCP, TCP)
 * (3x0D) tx_sync_pos0          : up stream sync start position(low)
 * (3x0E) tx_sync_pos1          : up stream sync start position(high)
 * (3x2F) tx_even               : up stream SD..Interlace
 * (3x0C) tx_zero_length        : Only CVI 4M
 *
 * Coaxial protocol down stream register(example: channel 0)
 * (3x63) rx_comm_on         : Coaxial Down Stream Mode ON/OFF ( 0: OFF / 1: ON )
 * (3x62) rx_area            : Down Stream Read Line Number
 * (3x66) rx_signal_enhance  : Signal Enhance ON/OFF ( 0: OFF / 1: ON )
 * (3x69) rx_manual_duty     : 1 Bit Duty Setting ( HD@25, 30P 0x32  /  HD@50, 60P, FHD@25, 30P 0x64 )
 * (3x60) rx_head_matching   : Same Header Read (EX. 0x48)
 * (3x61) rx_data_rz         : The lower 2 bits set Coax Mode.. ( 0 : A-CP ), ( 1 : C-CP ), ( 2 : T-CP )
 * (3x68) rx_sz              : Down stream size setting
 * (3x3A)                    : Down stream buffer clear
 ========================================================================================================*/
int __NC_VD_COAX_Command_Each_Copy( unsigned char *Dst, int *Src )
{
	NC_S32 items = 0;

	while( Src[items] != EOD )
	{
		Dst[items] = Src[items];
		items++;
	}

	return items;
}

int __NC_VD_COAX_Command_Copy( NC_FORMAT_STANDARD_E format, NC_VIVO_CH_FORMATDEF_E vivofmt, unsigned char *Dst, NC_COAX_CMD_TABLE_S *pCMD )
{
	NC_S32 cmd_cnt = 0;

	if( format == FMT_SD )
	{
		cmd_cnt = __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->sd );
	}
	else if( format == FMT_AHD )
	{
		if( vivofmt == AHD_5M_20P || vivofmt == AHD_5M_12_5P || vivofmt == AHD_5_3M_20P  ||\
			vivofmt == AHD_4M_30P || vivofmt == AHD_4M_25P || vivofmt == AHD_4M_15P || vivofmt == AHD_8M_12_5P || vivofmt == AHD_8M_15P )
		{
			cmd_cnt = __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->ahd_4_5m );
		}
		else
		{
			cmd_cnt = __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->ahd_8bit );
		}
	}
	else if( format == FMT_CVI )
	{
		cmd_cnt= __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->cvi_cmd );
	}
	else if( format == FMT_TVI )
	{
		if( (vivofmt == TVI_4M_30P) || (vivofmt == TVI_4M_25P) || (vivofmt == TVI_4M_15P) )
		{
			cmd_cnt = __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->tvi_v2_0 );
		}
		else
		{
			cmd_cnt = __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->tvi_v1_0 );
		}
	}
	else
		printk("NC_VD_COAX_Tx_Command_Send::Command Copy Error!!\n");


	return cmd_cnt;
}

int __NC_VD_COAX_16bit_Command_Copy( NC_FORMAT_STANDARD_E format, NC_VIVO_CH_FORMATDEF_E vivofmt, unsigned char *Dst, NC_COAX_CMD_TABLE_S *pCMD )
{
	NC_S32 cmd_cnt = 0;

	if( (vivofmt == AHD_720P_25P) || (vivofmt == AHD_720P_30P) ||\
			(vivofmt == AHD_720P_25P_EX) || (vivofmt == AHD_720P_30P_EX) ||\
			(vivofmt == AHD_720P_25P_EX_Btype) || (vivofmt == AHD_720P_30P_EX_Btype) )
	{
		cmd_cnt = __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->ahd_16bit );
	}
	else if( (vivofmt == CVI_4M_25P) || (vivofmt == CVI_4M_30P) ||\
			(vivofmt == CVI_8M_15P) || (vivofmt == CVI_8M_12_5P) )
	{
		cmd_cnt = __NC_VD_COAX_Command_Each_Copy( Dst, pCMD->cvi_new_cmd );
	}
	else
	{
		printk("[drv_coax] Can not send commands!! Unsupported format!!\n" );
		return 0;
	}

	return cmd_cnt;
}

void nc_drv_coax_initialize_set(void *pParam)
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;
	NC_COAX_ATTR_TABLE_S *pCoaxInitTable;

	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;
	NC_U8 distance = 0;

	NC_COAX_CMD_VER_E      coax_ver  = pCoaxInfo->Coax_ver;
	NC_VIVO_CH_FORMATDEF_E video_fmt = nc_drv_common_info_video_fmt_def_get(pCoaxInfo->Chn);  // pCoaxInfo->VideoFormat;

	if( coax_ver == COAX_AHD_16BIT || coax_ver == COAX_CVI_NEW )
	{
		if( video_fmt == AHD_720P_25P || video_fmt == AHD_720P_30P || video_fmt == AHD_720P_25P_EX || video_fmt == AHD_720P_30P_EX ||\
		    video_fmt == AHD_720P_25P_EX_Btype || video_fmt == AHD_720P_30P_EX_Btype ||\
		    video_fmt == CVI_4M_25P || video_fmt == CVI_4M_25P || video_fmt == CVI_8M_12_5P || video_fmt == CVI_8M_15P)
		{
			pCoaxInitTable = nc_drv_table_coax_special_initialize_info_get( video_fmt ); // AHD 16bit, CVI New Format
		}
		else
		{
			printk("[%s::%d]AHD16bit or CVI New Command Not Supported Format::%d!!\n", __func__, __LINE__, video_fmt);
			return;
		}

	}
	else
	{
		pCoaxInitTable = nc_drv_table_coax_normal_initialize_info_get( video_fmt ); // Normal Case
	}

//	printk( "[drv_coax]dev(%d), chn(%d), fmt(%s)\n", dev, chn, pCoaxInitTable->name );

	/* Coaxial each mode set */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn)
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2F, 0x00);       // MPP_H_INV, MPP_V_INV, MPP_F_INV
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0xE0);       // MPP_H_S[7~4], MPP_H_E[3:0]
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0x43);       // MPP_H_S[7:0]
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x32, 0xA2);       // MPP_H_E[7:0]
 	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7C, pCoaxInitTable->rx_src);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7D, pCoaxInitTable->rx_slice_lev);

	/* UP Stream Initialize */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+((chn%2)*0x80), pCoaxInitTable->tx_baud[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+((chn%2)*0x80), pCoaxInitTable->tx_pel_baud[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03+((chn%2)*0x80), pCoaxInitTable->tx_line_pos0[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04+((chn%2)*0x80), pCoaxInitTable->tx_line_pos1[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), pCoaxInitTable->tx_line_count);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07+((chn%2)*0x80), pCoaxInitTable->tx_pel_line_pos0[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08+((chn%2)*0x80), pCoaxInitTable->tx_pel_line_pos1[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), pCoaxInitTable->tx_line_count_max);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0B+((chn%2)*0x80), pCoaxInitTable->tx_mode);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0D+((chn%2)*0x80), pCoaxInitTable->tx_sync_pos0[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0E + ((chn%2)*0x80), pCoaxInitTable->tx_sync_pos1[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2F+((chn%2)*0x80), pCoaxInitTable->tx_even);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0C+((chn%2)*0x80), pCoaxInitTable->tx_zero_length);

	/* Down Stream Initialize */
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x63+((chn%2)*0x80), pCoaxInitTable->rx_comm_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62+((chn%2)*0x80), pCoaxInitTable->rx_area);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x66+((chn%2)*0x80), pCoaxInitTable->rx_signal_enhance);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x69+((chn%2)*0x80), pCoaxInitTable->rx_manual_duty);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), pCoaxInitTable->rx_head_matching);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x61+((chn%2)*0x80), pCoaxInitTable->rx_data_rz);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x68+((chn%2)*0x80), pCoaxInitTable->rx_sz);
}

/*******************************************************************************************************
**************************** Coaxial protocol up stream function ***************************************
********************************************************************************************************
* Coaxial protocol up stream Flow
* 1. Up stream initialize       -  coax_tx_init
* 2. Fill upstream data & Send  -  coax_tx_cmd_send
*
* Coaxial protocol up stream register(example: channel 0)
* (3x00) tx_baud               : 1 bit duty
* (3x02) tx_pel_baud           : 1 bit duty of pelco(SD)
* (3x03) tx_line_pos0          : up stream line position(low)
* (3x04) tx_line_pos1          : up stream line position(high)
* (3x05) tx_line_count         : up stream output line number in 1 frame
* (3x07) tx_pel_line_pos0      : up stream line position of pelco(low)
* (3x08) tx_pel_line_pos1      : up stream line position of pelco(high)
* (3x0A) tx_line_count_max     : up stream output total line
* (3x0B) tx_mode               : up stream Mode set (ACP, CCP, TCP)
* (3x0D) tx_sync_pos0          : up stream sync start position(low)
* (3x0E) tx_sync_pos1          : up stream sync start position(high)
* (3x2F) tx_even               : up stream SD..Interlace
* (3x0C) tx_zero_length        : Only CVI 4M
========================================================================================================*/
void nc_drv_coax_initialize_manual_set(void *pParam)
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;
	NC_COAX_ATTR_TABLE_S *pCoaxInitTable;

	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;
	NC_U8 distance = 0;

	NC_COAX_CMD_VER_E      coax_ver  = pCoaxInfo->Coax_ver;
	NC_VIVO_CH_FORMATDEF_E video_fmt = pCoaxInfo->VideoFormat;

	nc_drv_common_info_video_format_manual_set(chn, video_fmt);


	if( coax_ver == COAX_AHD_16BIT || coax_ver == COAX_CVI_NEW )
	{
		if( video_fmt == AHD_720P_25P || video_fmt == AHD_720P_30P || video_fmt == AHD_720P_25P_EX || video_fmt == AHD_720P_30P_EX ||\
		    video_fmt == AHD_720P_25P_EX_Btype || video_fmt == AHD_720P_30P_EX_Btype ||\
		    video_fmt == CVI_4M_25P || video_fmt == CVI_4M_25P || video_fmt == CVI_8M_12_5P || video_fmt == CVI_8M_15P)
		{
			pCoaxInitTable = nc_drv_table_coax_special_initialize_info_get( video_fmt );  // AHD 16bit, CVI New Format
		}
		else
		{
			printk("[%s::%d]AHD16bit or CVI New Command Not Supported Format::%d!!\n", __func__, __LINE__, video_fmt);
			return;
		}

	}
	else
	{
		pCoaxInitTable = nc_drv_table_coax_normal_initialize_info_get( video_fmt ); // Normal Case
	}

	printk( "[drv_coax]dev(%d), chn(%d), fmt(%s)\n", dev, chn, pCoaxInitTable->name );

	/* MPP Coaxial mode select Ch1~4 */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xA8, 0x08);  // MPP_TST_SEL1
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xA9, 0x09);  // MPP_TST_SEL2
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xAA, 0x0A);  // MPP_TST_SEL3
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xAB, 0x0B);  // MPP_TST_SEL4

	/* Coaxial each mode set */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn)
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2F, 0x00);		// MPP_H_INV, MPP_V_INV, MPP_F_INV
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0xE0);  	// MPP_H_S[7~4], MPP_H_E[3:0]
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0x43);     // MPP_H_S[7:0]
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x32, 0xA2);     // MPP_H_E[7:0]
 	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7C, pCoaxInitTable->rx_src);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7D, pCoaxInitTable->rx_slice_lev);

	/* UP Stream Initialize */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+((chn%2)*0x80), pCoaxInitTable->tx_baud[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02+((chn%2)*0x80), pCoaxInitTable->tx_pel_baud[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03+((chn%2)*0x80), pCoaxInitTable->tx_line_pos0[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04+((chn%2)*0x80), pCoaxInitTable->tx_line_pos1[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), pCoaxInitTable->tx_line_count);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07+((chn%2)*0x80), pCoaxInitTable->tx_pel_line_pos0[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08+((chn%2)*0x80), pCoaxInitTable->tx_pel_line_pos1[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), pCoaxInitTable->tx_line_count_max);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0B+((chn%2)*0x80), pCoaxInitTable->tx_mode);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0D+((chn%2)*0x80), pCoaxInitTable->tx_sync_pos0[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0E + ((chn%2)*0x80), pCoaxInitTable->tx_sync_pos1[distance]);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2F+((chn%2)*0x80), pCoaxInitTable->tx_even);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0C+((chn%2)*0x80), pCoaxInitTable->tx_zero_length);

	/* Down Stream Initialize */
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x63+((chn%2)*0x80), pCoaxInitTable->rx_comm_on);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62+((chn%2)*0x80), pCoaxInitTable->rx_area);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x66+((chn%2)*0x80), pCoaxInitTable->rx_signal_enhance);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x69+((chn%2)*0x80), pCoaxInitTable->rx_manual_duty);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), pCoaxInitTable->rx_head_matching);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x61+((chn%2)*0x80), pCoaxInitTable->rx_data_rz);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x68+((chn%2)*0x80), pCoaxInitTable->rx_sz);

}

void nc_drv_coax_up_stream_command_set( void *pParam )
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;

	NC_S32 ii;
	NC_S32 cmd_cnt = 0;

	NC_U8 command[32]          = { 0, };
	NC_U8 TCP_CMD_Stop_v10[10] = { 0xb5, 0x00, 0x14, 0x00, 0x80, 0x00, 0x00, 0x00, 0xc9, 0x80 };
#ifndef NVT_PLATFORM
	NC_U8 TCP_CMD_Stop_v20[10] = { 0xb5, 0x01, 0x14, 0x00, 0x80, 0x00, 0x00, 0x00, 0xc5, 0x80 };
#endif

	NC_U8 info_chn = pCoaxInfo->Chn;
	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;

	NC_COAX_CMD_DEF_E      cmd             = pCoaxInfo->Cmd;
	NC_COAX_CMD_VER_E      coax_ver        = pCoaxInfo->Coax_ver;
	NC_VIVO_CH_FORMATDEF_E vivofmt 	       = nc_drv_common_info_video_fmt_def_get(info_chn);      	// pCoaxInfo->VideoFormat;
	NC_FORMAT_STANDARD_E   format_standard = nc_drv_common_info_video_fmt_standard_get(info_chn); 	// pCoaxInfo->FormatStandard;
	NC_COAX_CMD_TABLE_S    *pCMD  		   = nc_drv_table_coax_up_stream_command_get(cmd);   		// Get From Coax_Tx_Command Table

//	printk("[%s::%d]cmd(%d) coax_ver(%d) vivofmt(%d) format_standard(%d) [0x%02X] \n", __FILE__, __LINE__, cmd, coax_ver, vivofmt, format_standard, g_nc_drv_i2c_addr[dev]);

	if( coax_ver == COAX_AHD_16BIT || coax_ver == COAX_CVI_NEW )
	{
		/* UP Stream command copy in coax command table */
		cmd_cnt = __NC_VD_COAX_16bit_Command_Copy(format_standard, vivofmt, command, pCMD);
		printk("[drv_coax]Dev(%d), Chn(%d) Command >>>>> %s\n", dev, chn, pCMD->name);

		if(vivofmt == AHD_720P_25P || vivofmt == AHD_720P_30P || \
		   vivofmt == AHD_720P_25P_EX || vivofmt == AHD_720P_30P_EX ||\
		   vivofmt == AHD_720P_25P_EX_Btype || vivofmt == AHD_720P_30P_EX_Btype)
		{
			/* Adjust Bank */
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));

			/* fill Reset */
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x20+((chn%2)*0x80)+ii, 0);
			}

			/* Command Shot  */
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c+((chn%2)*0x80), 0x01);
			msleep(30);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c+((chn%2)*0x80), 0x00);

			/* fill command  */
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x20+((chn%2)*0x80)+ii, command[ii]);
			}

			/* Command Shot  */
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c+((chn%2)*0x80), 0x01);
			msleep(30);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c+((chn%2)*0x80), 0x00);

		}
		else if(vivofmt == CVI_4M_25P || vivofmt == CVI_4M_25P || vivofmt == CVI_8M_12_5P || vivofmt == CVI_8M_15P)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_C+chn);
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0C + ii, command[ii]);
			}

			/* Shot  */
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x01);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);
		}
		else
		{
			printk("[%s::%d]AHD16bit or CVI New Command Not Supported Format::%d!!\n", __func__, __LINE__, vivofmt);
			return;
		}

	}
	else
	{
		/* UP Stream command copy in coax command table */
		cmd_cnt = __NC_VD_COAX_Command_Copy( format_standard, vivofmt, command, pCMD );
		printk("[drv_coax]Dev(%d), Chn(%d) fmt(%d), Command >>>>> %s\n", dev, chn, vivofmt, pCMD->name );

		/* fill command + shot */
		if( format_standard == FMT_SD )
		{
			/* Reset -> Real */
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));

			/* Reset Command */
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x20+((chn%2)*0x80))+ii, 0);
			}

			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0C+((chn%2)*0x80), 0x01);
			msleep(35);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0C+((chn%2)*0x80), 0x00);

			msleep(35);

			/* Real Command */
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x20+((chn%2)*0x80))+ii, command[ii]);
			}

			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0C+((chn%2)*0x80), 0x01);
			msleep(35);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0C+((chn%2)*0x80), 0x00);

		}
		else if(format_standard == FMT_AHD)
		{
			/* Reset -> Real */
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x10+((chn%2)*0x80))+ii, command[ii]);
			}
			if(cmd == COAX_CMD_SPECIAL_3DNR_REQUEST)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);
				msleep(150);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);
			}
			else
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x01);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);
			}
		}
		else if(format_standard == FMT_CVI)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_C+chn)
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+ii, command[ii]);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+ii, 0xff);
			}

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x01);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);
		}
		else if(format_standard == FMT_TVI)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
			for(ii=0; ii<cmd_cnt; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x10+((chn%2)*0x80))+ii, command[ii]);
			}

			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);
			msleep(30);
			#if 0
			if( (vivofmt == TVI_4M_30P) || (vivofmt == TVI_4M_25P) || (vivofmt == TVI_4M_15P) )
			{
				NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
				for(ii=0; ii<10; ii++)
				{
					gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80)+ii, TCP_CMD_Stop_v20[ii]);
				}
			}
			else
			#endif
			{
				NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
				for(ii=0; ii<10; ii++)
				{
					gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80)+ii, TCP_CMD_Stop_v10[ii]);
				}
			}

			/* shot */
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);
		}
		else
		{
			printk("[%s::%d]Unknown format standard::%d \n", __func__, __LINE__, format_standard);
		}

	}

}

void nc_drv_coax_rt_nrt_mode_change_set(void *pParam)
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;
	NC_U8 param = pCoaxInfo->Param;

	NC_U8 tx_line 	  = 0;
	NC_U8 tx_line_max = 0;


	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	tx_line     = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80) );
	tx_line_max = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80) );

	/* Adjust_Tx */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), 0x03);       // Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), 0x04);       // Tx_max line set

	if( !param ) // RT Mode
	{
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x60);   // Register write
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), 0xb1);   // Output command
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), 0x00);   // RT Mode
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x14+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x15+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x16+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x17+((chn%2)*0x80), 0x00);
	}
	else // NRT Mode
	{
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x60);   // Register write
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), 0xb1);   // Output command
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), 0x01);   // RT Mode
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x14+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x15+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x16+((chn%2)*0x80), 0x00);
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x17+((chn%2)*0x80), 0x00);
	}

	/* Tx_Command Shot */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);	 		// trigger on
	msleep(200);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x10);	 		// reset
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);	 		// trigger Off

	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), tx_line);		// Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), tx_line_max);  // Tx_max line set

}

void nc_drv_coax_tx_reg_ctrl_set(void *pParam)
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;

	int whileCnt = 0;

	NC_U8 tx_header	  = 0;
	NC_U8 tx_line 	  = 0;
	NC_U8 tx_line_max = 0;

	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;

	NC_U8 addr_h  = pCoaxInfo->rx_pelco_data[0];
	NC_U8 addr_l  = pCoaxInfo->rx_pelco_data[1];
	NC_U8 reg_set = pCoaxInfo->rx_pelco_data[2];

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	tx_header   = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80));
	tx_line     = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80));
	tx_line_max = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80));

	/* Adjust_Tx */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), 0x60);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), 0x03);     // Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), 0x04);     // Tx_max line set

	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x60);   	// Register write
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), addr_h);   // Output command
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), addr_l);	// RT Mode
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), reg_set);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x14+((chn%2)*0x80), 0x00);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x15+((chn%2)*0x80), 0x00);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x16+((chn%2)*0x80), 0x00);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x17+((chn%2)*0x80), 0x00);

	/* Tx Command Shot */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);	 // trigger on
	msleep(200);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x10);	 // reset
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);	 // trigger Off

	while(whileCnt<10)
	{
		if( 0x60 == gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x50+((chn%2)*0x80))))
		{
			addr_h  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x51+((chn%2)*0x80)));
			addr_l  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x52+((chn%2)*0x80)));
			reg_set = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x53+((chn%2)*0x80)));
			pCoaxInfo->Value = reg_set;
			printk("tx_reg_set_valid_Check::0x%02x%02x >> %02X\n", addr_h, addr_l, reg_set);
			break;
		}
		else
		{
			msleep(10);
			whileCnt++;
		}
	}

	if(whileCnt >= 10)
	{
		printk("nc_drv_coax_tx_reg_ctrl_set error!!\n");
		printk("0x50 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x50+((chn%2)*0x80))));
		printk("0x51 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x51+((chn%2)*0x80))));
		printk("0x52 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x52+((chn%2)*0x80))));
		printk("0x53 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x53+((chn%2)*0x80))));
		pCoaxInfo->Value = 0xFF;
	}

	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), tx_header);    // Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), tx_line);      // Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), tx_line_max);	// Tx_max line set
}

void nc_drv_coax_tx_reg_ctrl_get(void *pParam)
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;

	int whileCnt = 0;

	NC_U8 tx_header	  = 0;
	NC_U8 tx_line 	  = 0;
	NC_U8 tx_line_max = 0;

	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;

	NC_U8 addr_h  = pCoaxInfo->rx_pelco_data[0];
	NC_U8 addr_l  = pCoaxInfo->rx_pelco_data[1];

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	tx_header   = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80));
	tx_line     = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80));
	tx_line_max = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80));

	/* Adjust_Tx */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), 0x61);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), 0x03);     // Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), 0x04);     // Tx_max line set

	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x61);   	// Register write
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), addr_h);	// Output command
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), addr_l);   // RT Mode
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0x00);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x14+((chn%2)*0x80), 0x00);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x15+((chn%2)*0x80), 0x00);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x16+((chn%2)*0x80), 0x00);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x17+((chn%2)*0x80), 0x00);

	/* Tx_Command Shot */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);	 // trigger on
	msleep(200);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x10);	 // reset
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);	 // trigger Off

	while(whileCnt<10)
	{
		if( 0x61 == gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x50+((chn%2)*0x80))))
		{
			addr_h = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x51+((chn%2)*0x80)));
			addr_l = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x52+((chn%2)*0x80)));
			pCoaxInfo->Value = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x53+((chn%2)*0x80)));
			printk("tx_reg_get_valid_Check::0x%02x%02x >> %02X\n", addr_h, addr_l, gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x53+((chn%2)*0x80))));
			break;
		}
		else
		{
			msleep(10);
			whileCnt++;
		}
	}

	if(whileCnt >= 10)
	{
		printk("nc_drv_coax_tx_reg_ctrl_set error!!\n");
		printk("0x50 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x50+((chn%2)*0x80))));
		printk("0x51 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x51+((chn%2)*0x80))));
		printk("0x52 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x52+((chn%2)*0x80))));
		printk("0x53 : %x\n", gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x53+((chn%2)*0x80))));
		pCoaxInfo->Value = 0xFF;
	}

	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), tx_header);    // Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), tx_line);      // Tx_line set
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), tx_line_max);	// Tx_max line set
}

void nc_drv_coax_down_stream_data_get(void *pParam)
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;

	NC_S32 ii = 0;

	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;
	NC_FORMAT_STANDARD_E fmt_standard = nc_drv_common_info_video_fmt_standard_get(chn);;
	pCoaxInfo->FormatStandard = fmt_standard;


	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));

	if( (fmt_standard == FMT_CVI) || (fmt_standard == FMT_TVI) )
	{
		for(ii=0;ii<5;ii++)
		{
			pCoaxInfo->rx_data1[ii] = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x40+((chn%2)*0x80))+ii);   // ChX_Rx_Line_1 : 0x40 ~ 0x44 5byte
			pCoaxInfo->rx_data2[ii] = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x45+((chn%2)*0x80))+ii);   // ChX_Rx_Line_2 : 0x45 ~ 0x49 5byte
			pCoaxInfo->rx_data3[ii] = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x4A+((chn%2)*0x80))+ii);   // ChX_Rx_Line_3 : 0x4A ~ 0x4E 5byte
			pCoaxInfo->rx_data4[ii] = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x6C+((chn%2)*0x80))+ii);   // ChX_Rx_Line_4 : 0x6C ~ 0x70 5byte
			pCoaxInfo->rx_data5[ii] = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x71+((chn%2)*0x80))+ii);   // ChX_Rx_Line_5 : 0x71 ~ 0x75 5byte
			pCoaxInfo->rx_data6[ii] = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x76+((chn%2)*0x80))+ii);   // ChX_Rx_Line_6 : 0x76 ~ 0x7A 5byte
		}
	}
	else if( (fmt_standard ==FMT_AHD) || (fmt_standard ==FMT_SD) )
	{
		for(ii=0;ii<8;ii++)
		{
			pCoaxInfo->rx_pelco_data[ii] = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x50+((chn%2)*0x80))+ii);   // ChX_PELCO_Rx_Line_1 ~ 8 : 0x50 ~ 0x57 8byte
		}
	}
	else
	{
		printk("[%s::%d]Unknown format standard::%d \n", __func__, __LINE__, fmt_standard);
	}

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3A+((chn%2)*0x80), 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3A+((chn%2)*0x80), 0x00);

}

void nc_drv_coax_down_stream_deinit_set(void *pParam)
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;
	NC_U8 dev = pCoaxInfo->Chn/4;
	NC_U8 chn = pCoaxInfo->Chn%4;

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+((chn%4)/2))
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x63+((chn%2)*0x80), 0x00);

}

/*=======================================================================================================
 ********************************************************************************************************
 ************************** Coaxial protocol firmware upgrade function **********************************
 ********************************************************************************************************
 *
 * Coaxial protocol firmware upgrade Flow
 * 1. ACP Check - Down Stream Header 0x55  - nc_drv_coax_fwseq_1_ready_header_check_from_isp_recv
 * 2.1 FW ready send                       - coax_fw_ready_cmd_to_isp_send
 * 2.2 FW ready ACK receive                - coax_fw_ready_cmd_ack_from_isp_recv
 * 3.1 FW start send                       - coax_fw_start_cmd_to_isp_send
 * 3.2 FW start ACK receive                - coax_fw_start_cmd_ack_from_isp_recv
 * 4.1 FW data send - 139byte         	   - coax_fw_one_packet_data_to_isp_send
 * 4.2 FW data ACK receive - offset        - coax_fw_one_packet_data_ack_from_isp_recv
 * 5.1 FW end send                         - coax_fw_end_cmd_to_isp_send
 * 5.2 FW end ACK receive                  - coax_fw_end_cmd_ack_from_isp_recv
 ========================================================================================================*/
void nc_drv_coax_fwseq_1_ready_header_check_from_isp_recv(void *pParam)
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_S32 ret = FW_FAILURE;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;

	NC_U8 readval = 0;

	/* set register */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x50+((chn%2)*0x80), 0x05 );  // PELCO Down Stream Read 1st Line
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), 0x55 );  // Header Matching

	/* If the header is (0x50=>0x55) and chip information is (0x51=>0x3X, 0x4X, 0x5X ), it can update firmware */
	if( gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x50+((chn%2)*0x80) ) == 0x55 )
	{
		printk(">>>>> DRV[%s:%d] CH:%d, this camera can update, please, wait! = 0x%x\n", __func__, __LINE__, chn, gpio_i2c_read(g_nc_drv_i2c_addr[chn/4], 0x51+((chn%2)*0x80)));
		ret = FW_SUCCESS;
	}
	else
	{
		readval= gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x50+((chn%2)*0x80) );
		printk(">>>>> DRV[%s:%d] check ACP_STATUS_MODE::0x%x\n", __func__, __LINE__, readval);
		ret = FW_FAILURE;
	}

	pFileInfo->result = ret;
}

void nc_drv_coax_fwseq_2_1_ready_cmd_to_isp_send(void *pParam) // FW Ready
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev      = pFileInfo->Chn/4;
	NC_U8 chn      = pFileInfo->Chn%4;
	NC_U8 info_chn = pFileInfo->Chn;

	NC_S32 cp_mode = nc_drv_common_info_video_fmt_standard_get(info_chn);

	NC_S32 ret = FW_FAILURE;

	/* Adjust UP Stream */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), 0x04);  // Tx Line count max

	/* change video mode FHD@25P Command Send */
	if( cp_mode == FMT_AHD )
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x60);	// Register Write Control 				 - 17th line
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), 0xB0);	// table(Mode Change Command) 			 - 18th line
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), 0x02);	// Flash Update Mode(big data)			 - 19th line
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0x02);	// Init_Value(FW Information Check Mode) - 20th line

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);	// trigger on
		msleep(400);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x10);	// reset
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);	// trigger Off
		printk(">>>>> DRV[%s:%d] CH:%d, coax_fw_ready_cmd_to_isp_send!!- AHD\n", __func__, __LINE__, chn );
		ret = FW_SUCCESS;
	}
	else if( (cp_mode == FMT_CVI) || (cp_mode == FMT_TVI) )
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x55);	// 0x55(header)          				 - 16th line
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), 0x60);	// Register Write Control 				 - 17th line
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), 0xB0);	// table(Mode Change Command) 			 - 18th line
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0x02);	// Flash Update Mode         			 - 19th line
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14+((chn%2)*0x80), 0x00);	// Init_Value(FW Information Check Mode) - 20th line

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);	// trigger on
		msleep(1000);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x10);	// reset
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);	// trigger Off
		printk(">>>>> DRV[%s:%d] CH:%d, coax_fw_ready_cmd_to_isp_send!!- AHD\n", __func__, __LINE__, chn );
		ret = FW_SUCCESS;
	}
	else
	{
		printk(">>>> DRV[%s:%d] CH:%d, FMT:%d > Unknown Format!!! \n", __func__, __LINE__, chn, cp_mode );
		ret = FW_FAILURE;
	}

//	nc_drv_common_info_coax_fw_status_set(chn, 1);
	pFileInfo->result = ret;
}

void nc_drv_coax_fwseq_2_2_ready_cmd_ack_from_isp_recv(void *pParam)
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;

	NC_U8 retval  = 0x00;
	NC_U8 retval2 = 0x00;

	NC_S32 ret = FW_FAILURE;

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	#if 0
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x63+((chn%2)*0x80), 0x01 );	// Ch_X Rx ON
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62+((chn%2)*0x80), 0x05 );	// Ch_X Rx Area
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x66+((chn%2)*0x80), 0x81 );	// Ch_X Rx Signal enhance
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x69+((chn%2)*0x80), 0x2D );	// Ch_X Rx Manual duty
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), 0x55 );	// Ch_X Rx Header matching
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x61+((chn%2)*0x80), 0x00 );	// Ch_X Rx data_rz
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x68+((chn%2)*0x80), 0x80 );	// Ch_X Rx SZ
	#endif
	if( gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x57+((chn%2)*0x80) ) == 0x02 )
	{
		/* get status, If the status is 0x00(Camera information), 0x01(Firmware version) */
		if( gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x56+((chn%2)*0x80) ) == 0x00 )
		{
			printk(">>>>> DRV[%s:%d]CH:%d Receive ISP status : [READY]\n", __func__, __LINE__, chn );
			ret = FW_SUCCESS;
		}
	}
	else
	{
		retval  = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x56+((chn%2)*0x80) );
		retval2 = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x57+((chn%2)*0x80) );
		printk(">>>>> DRV[%s:%d]CH:%d retry : Receive ISP status[READY], [0x56-true[0x00]:0x%x], [0x57-true[0x02]:0x%x]\n",	__func__, __LINE__, chn, retval, retval2 );
		ret = FW_FAILURE;
	}

	/* Down Stream Buffer clear */
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3A+((chn%2)*0x80), 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3A+((chn%2)*0x80), 0x00);

	pFileInfo->result = ret;

}

void nc_drv_coax_fwseq_3_1_start_cmd_to_isp_send(void *pParam)
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;

	/* Tx_Command set */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x60);	 // Register Write Control 				 - 17th line
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), 0xB0);	 // table(Mode Change Command) 			 - 18th line
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), 0x02);	 // Flash Update Mode(big data)			 - 19th line
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0x40);	 // Start firmware update                - 20th line

	/* Tx_Command Shot */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);	 // trigger on
	msleep(400);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x10);	 // reset
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);	 // trigger Off

	printk(">>>>> DRV[%s:%d]CH:%d >> Send command[START]\n", __func__, __LINE__, chn );
}

void nc_drv_coax_fwseq_3_2_start_cmd_ack_from_isp_recv( void *pParam )
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;
	NC_S32 ret   = FW_FAILURE;

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	if( gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x57+((chn%2)*0x80) ) == 0x02 )
	{
		if( gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x56+((chn%2)*0x80) ) == 0x02 )
		{
			printk(">>>>> DRV[%s:%d]CH:%d Receive ISP status : [START]\n", __func__, __LINE__, chn );
			ret = FW_SUCCESS;
		}
		else
		{
			unsigned char retval1;
			unsigned char retval2;

			retval1 = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x56+((chn%2)*0x80) );
			retval2 = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x57+((chn%2)*0x80) );
			ret = FW_FAILURE;
			printk(">>>>> DRV[%s:%d]CH:%d retry : Receive ISP status[START], [0x56-true[0x02]:0x%x], [0x57-true[0x02]:0x%x]\n",	__func__, __LINE__, chn, retval1, retval2 );
		}
	}

	/* Rx_Buffer clear */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3A+((chn%2)*0x80), 0x01);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3A+((chn%2)*0x80), 0x00);

	pFileInfo->result = ret;
}

void nc_drv_coax_fwseq_4_1_one_packet_data_to_isp_send( void *pParam )
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;
	NC_U32 readsize = pFileInfo->readsize;

	NC_U32 low = 0x00;
	NC_U32 mid = 0x00;
	NC_U32 high = 0x00;

	NC_S32 byteNumOfPacket = 0;

	int ii = 0;

	/* fill packet(139bytes), end packet is filled with 0xff */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_C+chn)
	for( ii = 0; ii < 139; ii++ )
	{
		if( byteNumOfPacket < (NC_S32)readsize)
		{
			gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x00+ii, pFileInfo->onepacketbuf[ii] );
			byteNumOfPacket++;
		}
		else if( byteNumOfPacket >= (NC_S32)readsize ) // end packet : fill 0xff
		{
			gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x00+ii, 0xff );
			byteNumOfPacket++;
		}

		if( ii == 0 )
			low = pFileInfo->onepacketbuf[ii];
		else if( ii == 1 )
			mid = pFileInfo->onepacketbuf[ii];
		else if( ii == 2 )
			high = pFileInfo->onepacketbuf[ii];
	}

	/* offset */
	pFileInfo->currentFileOffset = (unsigned int)((high << 16 )&(0xFF0000))| (unsigned int)((mid << 8 )&(0xFF00)) | (unsigned char)(low);

	/* Tx_Change mode to use Big data */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0B+((chn%2)*0x80), 0x30);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), 0x8A);

	/* Tx_Shot */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);	// trigger on
}

void nc_drv_coax_fwseq_4_2_one_packet_data_ack_from_isp_recv( void *pPparam )
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pPparam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;
	NC_U32 onepacketaddr = pFileInfo->currentFileOffset;

	NC_U32 receive_addr       = 0;
	NC_U32 receive_addr_7_0   = 0;
	NC_U32 receive_addr_15_8  = 0;
	NC_U32 receive_addr_23_16 = 0;
	NC_S32 ret = FW_FAILURE;


	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	#if 0
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x63+((chn%2)*0x80), 0x01 );	// Ch_X Rx ON
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62+((chn%2)*0x80), 0x05 );	// Ch_X Rx Area
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x66+((chn%2)*0x80), 0x81 );	// Ch_X Rx Signal enhance
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x69+((chn%2)*0x80), 0x2D );	// Ch_X Rx Manual duty
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), 0x55 );	// Ch_X Rx Header matching
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x61+((chn%2)*0x80), 0x00 );	// Ch_X Rx data_rz
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x68+((chn%2)*0x80), 0x80 );	// Ch_X Rx SZ
	#endif
	if( gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x57+((chn%2)*0x80) ) == 0x02 )
	{
		/* check ISP status - only check first packet */
		if( pFileInfo->currentpacketnum == 0 )
		{
			if( gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x56+((chn%2)*0x80) ) == 0x03 )
			{
				pFileInfo->result = FW_FAILURE;
				printk(">>>>> DRV[%s:%d] CH:%d, Failed, error status, code=3..................\n", __func__, __LINE__, chn );
				return;
			}
		}

		receive_addr_23_16 = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x53+((chn%2)*0x80));
		receive_addr_15_8  = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x54+((chn%2)*0x80));
		receive_addr_7_0   = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0x55+((chn%2)*0x80));

		/* check offset */
		receive_addr = (receive_addr_23_16 << 16) | (receive_addr_15_8 << 8) | (receive_addr_7_0);

		if( onepacketaddr == receive_addr )
		{
			gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x10);	// Reset
			gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);	// trigger off
			ret = FW_SUCCESS;
			pFileInfo->receive_addr = receive_addr;
			pFileInfo->result = ret;
		}
	}

	pFileInfo->result = ret;
}

void nc_drv_coax_fwseq_5_1_end_cmd_to_isp_send(void *pParam)
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;
	NC_S32 send_success = pFileInfo->result;


	/* adjust_Tx line */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0B+((chn%2)*0x80), 0x10);  // Tx_Mode
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), 0x03);	 // Tx_Line Count       3 line number
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), 0x03);	 // Tx_Total Line Count 3 line number

	/* Fill end command */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x10+((chn%2)*0x80), 0x60);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x11+((chn%2)*0x80), 0xb0);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x12+((chn%2)*0x80), 0x02);
	if( send_success == FW_FAILURE )
	{
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0xE0/*0xC0*/);
		printk(">>>>> DRV[%s:%d] CH:%d, Camera UPDATE error signal. send Abnormal ending!\n", __func__, __LINE__, chn );
	}
	else
	{
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x13+((chn%2)*0x80), 0x80/*0x60*/);
		printk(">>>>> DVR[%s:%d] CH:%d, Camera UPDATE ending signal. wait please!\n", __func__, __LINE__, chn );
	}

	/* Shot */
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);
	msleep(400);
	gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);

}

void nc_drv_coax_fwseq_5_2_end_cmd_ack_from_isp_recv(void *pParam)
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;

	NC_U8 ack_return = 0x00;
	NC_U8 isp_status = 0x00;

	/* get status, If the ack_return(0x56) is 0x05(completed writing f/w file to isp's flash) */
	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+(chn/2));
	ack_return = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x56+((chn%2)*0x80) );
	isp_status = gpio_i2c_read( g_nc_drv_i2c_addr[dev], 0x57+((chn%2)*0x80) );
	if( isp_status == 0x02 && ack_return == 0x05 )
	{
		printk(">>>>> DRV[%s:%d]CH:%d Receive ISP status : [END]\n", __func__, __LINE__, chn );
		pFileInfo->result = FW_SUCCESS;
		return;
	}
	else
	{
		printk(">>>>> DRV[%s:%d]CH:%d retry : Receive ISP status[END], [0x56-true[0x05]:0x%x], [0x57-true[0x02]:0x%x]\n", __func__, __LINE__, chn, ack_return, isp_status );
		pFileInfo->result = FW_FAILURE;
		return;
	}

}

void nc_drv_coax_test_isp_init_set(NC_U8 mode)
{
	NC_U8 dev = 0;
	NC_U8 chn = 0;

	if(mode)
	{
		/* UP Stream Adjust FirmUP Mode */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+((chn%4)/2));
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x00+((chn%2)*0x80), 0x2D);   // Duty
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x03+((chn%2)*0x80), 0x0D);   // line
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0B+((chn%2)*0x80), 0x30);	  // Use Bank C
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x05+((chn%2)*0x80), 0x8A);	  // tx_line_count
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x0A+((chn%2)*0x80), 0x8A);   // tx_line_count_max
	}
	else
	{
		/* UP Stream Adjust FHD@25P */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+((chn%4)/2))
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x63+((chn%2)*0x80), 0x01 );    // Ch_X Down Stream ON
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62+((chn%2)*0x80), 0x05 );    // Ch_X Down Stream Area
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x66+((chn%2)*0x80), 0x81 );    // Ch_X Down Stream Signal enhance
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x69+((chn%2)*0x80), 0x2D );    // Ch_X Down Stream Manual duty
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+((chn%2)*0x80), 0x55 );    // Ch_X Down Stream Header matching
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x61+((chn%2)*0x80), 0x00 );    // Ch_X Down Stream data_rz
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x68+((chn%2)*0x80), 0x80 );    // Ch_X Down Stream SZ
	}

}

void nc_drv_coax_test_isp_data_set(void *pParam)
{
	nc_decoder_s *pFileInfo = (nc_decoder_s*)pParam;

	NC_U8 dev = pFileInfo->Chn/4;
	NC_U8 chn = pFileInfo->Chn%4;

	NC_U8 SignalOn 			  = pFileInfo->Param;
	NC_COAX_CMD_DEF_E TestCmd = pFileInfo->Cmd;
	int ii = 0;

	if(SignalOn)
	{
		nc_drv_coax_test_isp_init_set(1);

		if( TestCmd == COAX_CMD_ISP_TEST_LOW_TO_HIGH )
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_C+chn);
			for(ii=0; ii<255; ii++)
			{
				gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x00+ii, 0x00 + ii);
			}
		}
		else if( TestCmd == COAX_CMD_ISP_TEST_HIGH_TO_LOW )
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_C+chn);
			for(ii=0; ii<255; ii++)
			{
				gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x00+ii, 0xFE - ii);
			}
		}
		else
			printk("[%s::%d]Unknown ISP test Command!!\n", __FILE__, __LINE__);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+((chn%4)/2));
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x08);
	}
	else
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_3+((chn%4)/2));
		gpio_i2c_write( g_nc_drv_i2c_addr[dev], 0x09+((chn%2)*0x80), 0x00);
		nc_drv_coax_test_isp_init_set(0);
	}
}

void nc_drv_coax_down_stream_detection_info_get( void *pParam )
{
	nc_decoder_s *pCoaxInfo = (nc_decoder_s*)pParam;

	NC_U8 Dev   = pCoaxInfo->Dev;
	NC_U8 ch    = pCoaxInfo->Chn;
	NC_U8 val_1 = 0;
#ifndef NVT_PLATFORM
	NC_U8 val_2 = 0;
#endif

	gpio_i2c_write(g_nc_drv_i2c_addr[Dev], 0xFF, 0x02+ch%4);
	val_1 = gpio_i2c_read(g_nc_drv_i2c_addr[Dev], 0x5c+((ch%2)*0x80));


	gpio_i2c_write(g_nc_drv_i2c_addr[Dev], 0xFF, 0x00);
#ifndef NVT_PLATFORM
	val_2 = gpio_i2c_read(g_nc_drv_i2c_addr[Dev], 0xa8 + ch);
#endif

	pCoaxInfo->Param = val_1;
}


/*=======================================================================================================
 * Motion
 * 1. nc_drv_motion_detection_info_get
 * 2. nc_drv_motion_onoff_set
 * 3. nc_drv_motion_all_block_onoff_set
 * 4. nc_drv_motion_each_block_onoff_set
 * 5. nc_drv_motion_each_block_onoff_get
 * 6. nc_drv_motion_motion_tsen_set
 * 7. nc_drv_motion_motion_psen_set
 ========================================================================================================*/
void nc_drv_motion_detection_info_get(void *pParam)
{
	nc_decoder_s *pMotion =  (nc_decoder_s*)pParam;

	NC_U8 dev = pMotion->Chn/4;
	NC_U8 Chn = pMotion->Chn%4;

	NC_U8 ReadVal = 0;
	NC_U8 Chn_mask = 1;

	NC_U8 ret = 0;

	Chn_mask = Chn_mask<<Chn;

	NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
	ReadVal = gpio_i2c_read(g_nc_drv_i2c_addr[dev], 0xA9);

	ret = ReadVal&Chn_mask;
	pMotion->Param = ret;
}

void nc_drv_motion_onoff_set(void *pParam)
{
	nc_decoder_s *pMotion =  (nc_decoder_s*)pParam;

	NC_U8 dev = pMotion->Chn/4;
	NC_U8 chn = pMotion->Chn%4;

	NC_U8 select = pMotion->Select;
	NC_VIVO_CH_FORMATDEF_E vivofmt = nc_drv_common_info_video_fmt_def_get(chn);

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xFF, 0x02);

	if(vivofmt == TVI_3M_18P || vivofmt == TVI_5M_12_5P || vivofmt == TVI_5M_12_5P)
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00 + (0x07 * chn), 0x0C);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02 + (0x07 * chn), 0x23);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28 + (0x06 * chn), 0x11);

		if(vivofmt == TVI_3M_18P)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29 + (0x06 * chn), 0x78);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2A + (0x06 * chn), 0x40);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2C + (0x06 * chn), 0x72);
		}
		else if(vivofmt == TVI_5M_12_5P)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29 + (0x06 * chn), 0xA2);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2A + (0x06 * chn), 0x51);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2C + (0x06 * chn), 0x96);
		}

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B + (0x06 * pMotion->Chn), 0x6);

		printk("[DRV_Motion_OnOff]Dev(%d) Chn(%d) fmtdef(%d)\n", dev, chn, pMotion->VideoFormat);
	}
	else
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28 + (0x06 * chn), 0x00);
	}

	if(select<0 || select>1)
	{
		printk("[DRV_Motion_OnOff]Error!!Dev(%d) Chn(%d) Setting Value Over:%x!! Only 0 or 1\n", dev, chn, pMotion->Select);
		return;
	}

	switch(select)
	{
		case FUNC_OFF : gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x00 + (0x07 * chn)), 0x0D);
						break;
		case FUNC_ON : gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x00 + (0x07 * chn)), 0x0C);
					   break;
	}

}

void nc_drv_motion_all_block_onoff_set(void *pParam)
{
	nc_decoder_s *pMotion =  (nc_decoder_s*)pParam;

	NC_U8 dev = pMotion->Chn/4;
	NC_U8 chn = pMotion->Chn%4;

	NC_U8 select = pMotion->Select;
	NC_VIVO_CH_FORMATDEF_E vivofmt = nc_drv_common_info_video_fmt_def_get(chn);

	NC_S32 ii   = 0;
#ifndef NVT_PLATFORM
	NC_U8  addr = 0;
#endif

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xFF, 0x02);

	if(vivofmt == TVI_3M_18P || vivofmt == TVI_5M_12_5P || vivofmt == TVI_5M_12_5P/*TVI_5M_20P*/)
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00 + (0x07 * chn), 0x0C);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02 + (0x07 * chn), 0x23);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28 + (0x06 * chn), 0x11);

		if(vivofmt == TVI_3M_18P)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29 + (0x06 * chn), 0x78);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2A + (0x06 * chn), 0x40);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2C + (0x06 * chn), 0x72);
		}
		else if(vivofmt == TVI_5M_12_5P)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29 + (0x06 * chn), 0xA2);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2A + (0x06 * chn), 0x51);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2C + (0x06 * chn), 0x96);
		}

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B + (0x06 * chn), 0x6);

		printk("[DRV_Motion_OnOff]Dev(%d) Chn(%d) fmtdef(%d)\n", dev, chn, vivofmt);
	}
	else
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28 + (0x06 * chn), 0x00);
	}

	for(ii=0; ii<24; ii++)
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x40 +(0x18 *chn)) + ii, select);
#ifndef NVT_PLATFORM
		addr = (0x40 +(0x18 *chn)) + ii;
#endif
	}
}

void nc_drv_motion_each_block_onoff_set(void *pParam)
{
	nc_decoder_s *pMotion =  (nc_decoder_s*)pParam;

	NC_U8 dev = pMotion->Chn/4;
	NC_U8 chn = pMotion->Chn%4;

	NC_VIVO_CH_FORMATDEF_E vivofmt 	       = nc_drv_common_info_video_fmt_def_get(chn);

	NC_U8 val = 0x80;
	NC_U8 ReadVal;
	NC_U8 on;
	NC_U8 SetPix  = pMotion->Select/8;
	NC_U8 SetVal  = pMotion->Select%8;

	val = val >> SetVal;

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xFF, 0x02);

	if(vivofmt == TVI_3M_18P || vivofmt == TVI_5M_12_5P || vivofmt == TVI_5M_12_5P)
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00 + (0x07 * chn), 0x0C);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02 + (0x07 * chn), 0x23);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28 + (0x06 * chn), 0x11);

		if(vivofmt == TVI_3M_18P)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29 + (0x06 * chn), 0x78);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2A + (0x06 * chn), 0x40);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2C + (0x06 * chn), 0x72);
		}
		else if(vivofmt == TVI_5M_12_5P)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x29 + (0x06 * chn), 0xA2);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2A + (0x06 * chn), 0x51);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2C + (0x06 * chn), 0x96);
		}

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2B + (0x06 * chn), 0x6);

		printk("[DRV_Motion_OnOff]Dev(%d) Chn(%d) fmtdef(%d)\n", dev, chn, vivofmt);
	}
	else
	{
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x28 + (0x06 * chn), 0x00);
	}

	ReadVal = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x40 +(0x18 * chn)) + SetPix);
	on = val&ReadVal;
	if(on)
	{
		val = ~val;
		val = val&ReadVal;
	}
	else
	{
		val = val|ReadVal;
	}
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x40 +(0x18 * chn)) + SetPix, val);
}

void nc_drv_motion_each_block_onoff_get(void *pParam)
{
	nc_decoder_s *pMotion =  (nc_decoder_s*)pParam;

	NC_U8 dev = pMotion->Chn/4;
	NC_U8 chn = pMotion->Chn%4;

	NC_U8 val 	  = 0x80;
	NC_U8 on 	  = 0;
	NC_U8 ReadVal = 0;

	NC_U8 SetPix  = pMotion->Select/8;
	NC_U8 SetVal  = pMotion->Select%8;

	val = val >> SetVal;

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xFF, 0x02);
	ReadVal = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x40 +(0x18 * chn)) + SetPix);

	on = val&ReadVal;

	if(on)
	{
		pMotion->Param = 1;
	}
	else
	{
		pMotion->Param = 0;
	}
}

void nc_drv_motion_motion_tsen_set(void *pParam)
{
	nc_decoder_s *pMotion =  (nc_decoder_s*)pParam;

	NC_U8 dev = pMotion->Chn/4;
	NC_U8 chn = pMotion->Chn%4;

	NC_U8 SetVal = pMotion->Select;

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xFF, 0x02);
	gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x01 +(0x07 * chn)), SetVal);
	printk("[%s::%d]Dev(%d) Chn(%d), TSEN Val(%x)\n", __FILE__, __LINE__, dev, chn, SetVal);
}

void nc_drv_motion_motion_psen_set(void *pParam)
{
	nc_decoder_s *pMotion =  (nc_decoder_s*)pParam;

	NC_U8 dev = pMotion->Chn/4;
	NC_U8 chn = pMotion->Chn%4;

	NC_U8 msb_mask = 0xF0;
	NC_U8 lsb_mask = 0x07;
	NC_U8 SetVal = pMotion->Select;
	NC_U8 ReadVal;

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xFF, 0x02);
	ReadVal = gpio_i2c_read(g_nc_drv_i2c_addr[dev], (0x02 +(0x07 * chn)));

	msb_mask = msb_mask&ReadVal;
	SetVal = lsb_mask&SetVal;

	SetVal = SetVal|msb_mask;

	gpio_i2c_write(g_nc_drv_i2c_addr[dev], (0x02 +(0x07 * chn)), SetVal);
	printk("[%s::%d]Dev(%d), Chn(%d), readVal(%x), SetVal(%x)\n", __FILE__, __LINE__, dev, chn, ReadVal, SetVal);
}

/**************************************************************************************
 * Audio Function
 *
 *
 ***************************************************************************************/
static NC_U32 sample_rate = 0;
static NC_U32 audio_mode  = 0;

NC_U32 nc_drv_audio_mode_get(void)
{
	return audio_mode;
}

void nc_drv_audio_init_set(void *pParam)
{
	nc_decoder_s *pAdInfo =  (nc_decoder_s*)pParam;

	NC_U8 dev = pAdInfo->Chn/4;
	NC_U8 chn = 0;

	printk("[%s::%d]mode(%d), smp_rate(%d)", __func__, __LINE__, pAdInfo->audio_mode, pAdInfo->audio_sample_rate);

	if(pAdInfo->audio_mode == NC_AD_AOC)
	{
		audio_mode = NC_AD_AOC;
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62, 0xFF);

		/* AIGAIN *******************/
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x41, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x42, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x43, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x08);


		if( pAdInfo->audio_sample_rate == NC_AD_SAMPLE_RATE_8000 )
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x80);
			sample_rate = 0;
		}
		else if(pAdInfo->audio_sample_rate == NC_AD_SAMPLE_RATE_16000)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x88);
			sample_rate = 1;
		}
		else
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x80);
			sample_rate = 0;
			printk("[%s]Unknown AOC Sampling Rate!!, set 8K mode [%d]\n", __func__, pAdInfo->audio_sample_rate);
		}

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_12);
		for(chn=0; chn<4; chn++)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00 + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04 + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05 + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06 + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0a + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0b + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0c + (chn*0x40), 0x01);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0e + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0f + (chn*0x40), 0x30);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10 + (chn*0x40), 0x04);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x17 + (chn*0x40), 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x19 + (chn*0x40), 0xa0);
		}
	}
	else if(pAdInfo->audio_mode == NC_AD_AI)
	{
		audio_mode = NC_AD_AI;
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x02);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0x0A);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58, 0x0f);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62, 0x00);
#if 0
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x02); // 32Khz: 03, 48Khz: 02
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x34, 0x0c); // 32Khz: 00, 48Khz: 0c
#endif
		if( pAdInfo->audio_sample_rate == NC_AD_SAMPLE_RATE_8000 )
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x80);
			sample_rate = 0;
		}
		else if(pAdInfo->audio_sample_rate == NC_AD_SAMPLE_RATE_16000)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x88);
			sample_rate = 1;
		}
		else
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x80);
			sample_rate = 0;
			printk("[%s]Unknown AOC Sampling Rate!!, set 8K mode [%d]\n", __func__, pAdInfo->audio_sample_rate);
		}
	}
	else
	{
		/* Default ADC Set */
		audio_mode = NC_AD_AI;
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x02);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0x0A);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58, 0x0f);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x62, 0x00);

		if( pAdInfo->audio_sample_rate == NC_AD_SAMPLE_RATE_8000 )
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x80);
			sample_rate = 0;
		}
		else if(pAdInfo->audio_sample_rate == NC_AD_SAMPLE_RATE_16000)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x88);
			sample_rate = 1;
		}
		else
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x80);
			sample_rate = 0;
			printk("[%s]Unknown AOC Sampling Rate!!, set 8K mode [%d]\n", __func__, pAdInfo->audio_sample_rate);
		}
	}
}

void nc_drv_audio_video_format_set(void *pParam)
{
	NC_S32 ret = 0;

	nc_decoder_s *pVdInfo = (nc_decoder_s*)pParam;

	NC_U8 info_chn 	= pVdInfo->Chn;
	NC_U8 dev 		= pVdInfo->Chn/4;
	NC_U8 chn 		= pVdInfo->Chn%4;
	NC_CABLE_E cable 	       = nc_drv_common_info_cable_get(info_chn);
	NC_VIVO_CH_FORMATDEF_E fmt = nc_drv_common_info_video_fmt_def_get(info_chn);

	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;

	if( (ret = nc_drv_common_total_chn_count_get(info_chn)) != 0 )
	{
		printk("[%s]nc_drv_common_total_chn_count_get error!!TotalChn>>>%d\n", __func__, ret);
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
		printk("[%s::%d]Cable Error!!\n", __func__, __LINE__);
		return;
	}

	if(stTableVideo->nc_table_aoc)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_12);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01 + (chn*0x40), stTableVideo->nc_table_aoc_12x01_audio_mode[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x02 + (chn*0x40), stTableVideo->nc_table_aoc_12x02_aoc_clk_mode[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x18 + (chn*0x40), stTableVideo->nc_table_aoc_12x18_aoc_16k_mode[sample_rate]);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07 + (chn*0x40), stTableVideo->nc_table_aoc_12x07_aoc_start_line_high[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08 + (chn*0x40), stTableVideo->nc_table_aoc_12x08_aoc_start_line_low[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x09 + (chn*0x40), stTableVideo->nc_table_aoc_12x09_aoc_cat_whd[sample_rate]);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11 + (chn*0x40), stTableVideo->nc_table_aoc_12x11_aoc_saa_value[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12 + (chn*0x40), stTableVideo->nc_table_aoc_12x12_aoc_data_value_high[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x13 + (chn*0x40), stTableVideo->nc_table_aoc_12x13_aoc_data_value_low[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x14 + (chn*0x40), stTableVideo->nc_table_aoc_12x14_aoc_base_unit[sample_rate]);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x15 + (chn*0x40), stTableVideo->nc_table_aoc_12x15_aoc_catch_point[sample_rate]);
	}
	else
	{
		printk("[%s::%d]Error!! It is a video format without a aoc table!!\n", __func__, __LINE__);
	}
}


/**************************************************************************************
 * Direct Application Setting Function
 *
 *
 ***************************************************************************************/
void nc_drv_common_bank_data_get(nc_decoder_s *data)
{
	int ii = 0;

	printk("[%s::%d]Slave(0x%02X), Bank(0x%02X)\n", __func__, __LINE__, data->Dev, data->Bank);

	gpio_i2c_write(data->Dev, 0xFF, data->Bank);

	for(ii=0; ii<256; ii++)
	{
		data->DumpData[ii] = gpio_i2c_read(data->Dev, 0x00+ii);
	}
}

void nc_drv_common_register_data_set(nc_decoder_s *data)
{
	gpio_i2c_write( data->Dev, 0xFF, data->Bank );
	gpio_i2c_write( data->Dev, data->Addr, data->Param );
}

void nc_drv_common_register_data_get(nc_decoder_s *data)
{
	gpio_i2c_write( data->Dev, 0xFF, data->Bank );
	data->Param = gpio_i2c_read( data->Dev, data->Addr );
}

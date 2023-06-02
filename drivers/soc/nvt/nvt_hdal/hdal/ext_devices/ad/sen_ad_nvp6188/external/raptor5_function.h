
#ifndef _RAPTOR5_FUNCTION_
#define _RAPTOR5_FUNCTION_

#include "raptor5_common.h"

/********************************************************************
 *  1. Coaxial Protocol Function
 ********************************************************************/
void nc_drv_coax_initialize_set(void *pParam);
void nc_drv_coax_initialize_manual_set(void *pParam);
void nc_drv_coax_up_stream_command_set( void *pParam );
void nc_drv_coax_down_stream_data_get(void *pParam);
void nc_drv_coax_down_stream_deinit_set(void *pParam);
void nc_drv_coax_rt_nrt_mode_change_set(void *pParam);

void nc_drv_coax_tx_reg_ctrl_set(void *pParam);
void nc_drv_coax_tx_reg_ctrl_get(void *pParam);

void nc_drv_coax_fwseq_1_ready_header_check_from_isp_recv(void *pParam);
void nc_drv_coax_fwseq_2_1_ready_cmd_to_isp_send(void *pParam);
void nc_drv_coax_fwseq_2_2_ready_cmd_ack_from_isp_recv(void *pParam);
void nc_drv_coax_fwseq_3_1_start_cmd_to_isp_send(void *pParam);
void nc_drv_coax_fwseq_3_2_start_cmd_ack_from_isp_recv(void *pParam);
void nc_drv_coax_fwseq_4_1_one_packet_data_to_isp_send(void *pParam);
void nc_drv_coax_fwseq_4_2_one_packet_data_ack_from_isp_recv(void *pPparam);
void nc_drv_coax_fwseq_5_1_end_cmd_to_isp_send(void *pParam);
void nc_drv_coax_fwseq_5_2_end_cmd_ack_from_isp_recv(void *pParam);

/* Only ISP Test */
void nc_drv_coax_test_isp_data_set(void *pParam);

/********************************************************************
 *  2. Audio of Coaxial Function
 ********************************************************************/
NC_U32 nc_drv_audio_mode_get(void);
void   nc_drv_audio_init_set(void *pParam);
void   nc_drv_audio_video_format_set(void *pParam);

/********************************************************************
 *  3. Motion Function
 ********************************************************************/
void nc_drv_motion_onoff_set(void *pParam);
void nc_drv_motion_each_block_onoff_set(void *pParam);
void nc_drv_motion_each_block_onoff_get(void *pParam);
void nc_drv_motion_motion_tsen_set(void *pParam);
void nc_drv_motion_motion_psen_set(void *pParam);
void nc_drv_motion_all_block_onoff_set(void *pParam);
void nc_drv_motion_detection_info_get(void *pParam);
void nc_drv_motion_over_size_format_set(NC_U8 dev, NC_U8 chn, NC_VIVO_CH_FORMATDEF_E fmt);

/********************************************************************
 *  4. For Test Function
 ********************************************************************/
void nc_drv_common_bank_data_get(nc_decoder_s *data);
void nc_drv_common_register_data_set(nc_decoder_s *data);
void nc_drv_common_register_data_get(nc_decoder_s *data);


#endif
/********************************************************************
 *  End of file
 ********************************************************************/

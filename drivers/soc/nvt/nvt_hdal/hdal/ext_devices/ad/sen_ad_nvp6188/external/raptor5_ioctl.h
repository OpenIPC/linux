
#ifndef __RAPTOR5_IOCTL_H__
#define __RAPTOR5_IOCTL_H__

/******************************************************************
 * Allocate ioctl space
 * 1. Information   0x90_00 ~ 0x9F_FF
 * 2. VIDEO    		0x10_00 ~ 0x1F_FF
 * 3. AOC      		0x20_00 ~ 0x2F_FF
 * 4. COAXIAL  		0x30_00 ~ 0x3F_FF
 * 5. MOTION   		0x40_00 ~ 0x4F_FF
 * 6. For Test 		0x70_00 ~ 0x7F_FF
 ******************************************************************/

/******************************************************************
 * 1. Information      0x00_00 ~ 0x0F_FF
 ******************************************************************/
#define IOC_VDEC_CHIP_INFO_GET	    0x9001

#define IOC_VDEC_REG_BANK_DUMP_GET  0x9060
#define IOC_VDEC_REG_DATA_SET 		0x9061
#define IOC_VDEC_REG_DATA_GET       0x9062

/******************************************************************
 * 2. VIDEO    0x10_00 ~ 0x1F_FF
 ******************************************************************/
// Information
#define IOC_VDEC_VIDEO_INPUT_INFO_GET         0x1000

// VIDEO Input
#define IOC_VDEC_VIDEO_INPUT_VFC_GET         		0x1001
#define IOC_VDEC_VIDEO_INPUT_EQ_STAGE_GET	0x1002
#define IOC_VDEC_VIDEO_INPUT_SET			0x1003
#define IOC_VDEC_VIDEO_INPUT_EQ_STAGE_SET	0x1004
#define IOC_VDEC_VIDEO_INPUT_MANUAL_SET     0x1005
#define IOC_VDEC_VIDEO_OUTPUT_SET  			0x1100
#define IOC_VDEC_VIDEO_OUTPUT_MANUAL_SET  	0x1101
#define IOC_VDEC_VIDEO_OUTPUT_PATTERN_SET   0x1102
#define IOC_VDEC_VIDEO_COLOR_TUNE_GET   	0x1110
#define IOC_VDEC_VIDEO_COLOR_TUNE_SET   	0x1111
#define IOC_VDEC_VIDEO_AOC_FORMAT_SET 		0x1200
#define IOC_VDEC_VIDEO_AUTO_MANUAL_MODE_SET		0x1201

/******************************************************************
 * 4. COAXIAL  0x30_00 ~ 0x3F_FF
 ******************************************************************/
#define IOC_VDEC_COAX_INIT_SET             		0x3001
#define IOC_VDEC_COAX_INIT_MANUAL_SET			0x3101
#define IOC_VDEC_COAX_UP_COMMAMD_SET       		0x3002
#define IOC_VDEC_COAX_DOWN_DATA_GET        		0x3003
#define IOC_VDEC_COAX_RT_NRT_MODE_CHANGE_SET	0x3004

#define IOC_VDEC_COAX_FW_ACP_HEADER_GET     	0x3010
#define IOC_VDEC_COAX_FW_READY_CMD_SET      	0x3011
#define IOC_VDEC_COAX_FW_READY_ACK_GET  		0x3012
#define IOC_VDEC_COAX_FW_START_CMD_SET  		0x3013
#define IOC_VDEC_COAX_FW_START_ACK_GET  		0x3014
#define IOC_VDEC_COAX_FW_SEND_DATA_SET  		0x3015
#define IOC_VDEC_COAX_FW_SEND_ACK_GET   		0x3016
#define IOC_VDEC_COAX_FW_END_CMD_SET    		0x3017
#define IOC_VDEC_COAX_FW_END_ACK_GET   			0x3018


/******************************************************************
 * 5. MOTION   0x40_00 ~ 0x4F_FF
 ******************************************************************/
#define IOC_VDEC_MOTION_ON_OFF_SET	  0x4050
#define IOC_VDEC_MOTION_BLOCK_SET     0x4051
#define IOC_VDEC_MOTION_BLOCK_GET     0x4052
#define IOC_VDEC_MOTION_TSEN_SET      0x4053
#define IOC_VDEC_MOTION_PSEN_SET      0x4054
#define IOC_VDEC_MOTION_ALL_BLOCK_SET 0x4055
#define IOC_VDEC_MOTION_DETECTION_GET 0x4056


/******************************************************************
 * 6. AUDIO   0x50_00 ~ 0x5F_FF
 ******************************************************************/
#define IOC_VDEC_AUDIO_INIT_SET	  0x5000
#define IOC_VDEC_AUDIO_VIDEO_AOC_FORMAT_SET  0x5001

/******************************************************************
 * 7. For TEST  0x70_00 ~ 0x7F_FF
 ******************************************************************/
#define IOC_VDEC_COAX_TEST_ISP_DATA_SET				0x7000

#define IOC_VDEC_COAX_TX_REG_CTRL_SET 0x7001
#define IOC_VDEC_COAX_TX_REG_CTRL_GET 0x7002

#endif

#ifndef _H264_DEF_H_
#define _H264_DEF_H_

#define H264ENC_HEADER_MAXSIZE  512
#define H264ENC_NAL_MAXSIZE  320 //5120/16

#define NALU_TYPE_SLICE    1
#define NALU_TYPE_DPA      2
#define NALU_TYPE_DPB      3
#define NALU_TYPE_DPC      4
#define NALU_TYPE_IDR      5
#define NALU_TYPE_SEI      6
#define NALU_TYPE_SPS      7
#define NALU_TYPE_PPS      8
#define NALU_TYPE_AUD      9
#define NALU_TYPE_EOSEQ    10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL     12
#define NALU_TYPE_SPS_EXT  13
#define NALU_TYPE_PREFIX   14

#define NALU_PRIORITY_HIGHEST     3
#define NALU_PRIORITY_HIGH        2
#define NALU_PRIORITY_LOW         1
#define NALU_PRIORITY_DISPOSABLE  0

typedef enum
{
    CAVLC = 0,
    CABAC = 1,
}H264_ENTROPY;

typedef enum
{
    PROFILE_BASELINE      =  66,
    PROFILE_MAIN          =  77,    
    PROFILE_HIGH          = 100,
    PROFILE_HIGH10        = 110,
}H264_PROFILE;

typedef enum {
	BUFFERING_PERIOD                      = 0,
	PIC_TIMING                            = 1,
	SCENE_INFO_SEI                        = 9,
	SUB_SEQ_INFO                          = 10,
	MOTION_SEI                            = 18,
	SCALABLE_SEI                          = 24,
	SUB_PIC_SEI                           = 25,
	NON_REQUIRED_SEI                      = 26,
	PRIORITYLEVEL_SEI                     = 27,
	SCALABLE_SEI_LAYERS_NOT_PRESENT       = 28,
	SCALABLE_SEI_DEPENDENCY_CHANGE        = 29,
	SCALABLE_NESTING_SEI                  = 30,
	AVC_COMPATIBLE_HRD_SEI                = 31,
	INTEGRITY_CHECK_SEI                   = 32,
	REDUNDANT_PIC_SEI                     = 33,
	TL0_DEP_REP_IDX_SEI                   = 34,
	TL_SWITCHING_POINT_SEI                = 35,
	RESERVED_SEI                          = 36
} SEI_TYPE;

#endif

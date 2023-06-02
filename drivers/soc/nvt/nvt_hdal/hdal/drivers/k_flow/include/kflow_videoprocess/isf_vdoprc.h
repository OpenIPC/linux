/*
    Copyright   Novatek Microelectronics Corp. 2005~2014.  All rights reserved.

    @file       isf_vdoprc.h
    @ingroup    mvdoprc

    @note       Nothing.

    @date       2014/10/01
*/

#ifndef ISF_VDOPRC_H
#define ISF_VDOPRC_H
/*
Note: NT96680
  each IPL support 5 outputs
     Output path 1: support raw encode for H264/H265 only
  Dzoom | SIE1~8
  AE/AWB | SIE1~8
  AF | SIE1 only
  for CCIR sensor, IPL also support Dzoom/SensorRatio/Crop/5-Output!
*/

/*
For isf_vdoprc1~8

Support these input port(s): IN1

Support these output port(s): OUT1,OUT2,OUT3,OUT4,OUT5,OUTC

Support these port config API(s):

============================================================
port                        IN1         OUT1,OUT2,OUT3,OUT4,OUT5
------------------------------------------------------------
ImageUnit_CfgImgSize()                  [Y]
ImageUnit_CfgImgAspect()    [Y]
ImageUnit_CfgImgWindow()    [Y](*)      [Y](**)
ImageUnit_CfgImgDirect()    [Y]
============================================================
(*) set SIE input crop window
(**) set IME output crop window

*/

#ifdef __KERNEL__
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#endif

#if defined(CONFIG_NVT_SMALL_HDAL)
//#define VDOPRC_MAX_NUM 	1 //reduce for small version
#define VDOPRC_MAX_NUM 	2 //reduce for small version
#else

#if defined(_BSP_NA51000_) || defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#define VDOPRC_MAX_NUM	16
#endif
#if defined(_BSP_NA51023_)
#define VDOPRC_MAX_NUM	2
#endif

#endif

#define VDOPRC_MAX_IN_NUM 		1
#if defined(CONFIG_NVT_SMALL_HDAL)
#define VDOPRC_MAX_OUT_NUM 		8 //reduce for small version
#else
#define VDOPRC_MAX_OUT_NUM 		16
#endif
#define VDOPRC_MAX_PHY_OUT_NUM 	5
#define VDOPRC_MAX_VPE_OUT_NUM 	4

typedef struct {
	UINT32 path_mask[VDOPRC_MAX_NUM]; ///< path to check
	INT32  wait_ms;         ///< timeout, unit: ms
} VDOPRC_POLL_LIST, *PVDOPRC_POLL_LIST;

typedef struct {
	UINT32 max_in_count; ///< path to check
	UINT32 max_out_count; ///< timeout, unit: ms
} VDOPRC_SYS_CAPS, *PVDOPRC_SYS_CAPS;

enum {
	VDOPRC_PARAM_START = 0x80001010,
	VDOPRC_PARAM_PIPE = VDOPRC_PARAM_START, ///< ISF_CTRL: Set pipe mode of an ImagePipe device. #VDOPRC_PIPE
	VDOPRC_PARAM_VPORT_START, ///< ISF_CTRL: begin / Set virtual port source of an ImagePipe device. #VDOPRC_PARAM_VPORT
	VDOPRC_PARAM_VPORT_END = VDOPRC_PARAM_VPORT_START + 10 - 1, ///< ISF_CTRL: end / Set virtual port source of an ImagePipe device. #VDOPRC_PARAM_VPORT
	VDOPRC_PARAM_IPL_CB, ///< ISF_CTRL: Set general callback of an ImagePipe device.
	VDOPRC_PARAM_EVENT_CB, ///< ISF_CTRL: Set event callback of an ImagePipe device.
	VDOPRC_PARAM_FUNC, ///< ISF_CTRL: Enable basic function of an ImagePipe device. #VDOPRC_FUNC
#if _TODO
	VDOPRC_PARAM_FUNC2, ///< ISF_CTRL: Enable advance function of an ImagePipe device. #VDOPRC_FUNC2
#else
	VDOPRC_PARAM_RESV1, ///< reserved
#endif
	VDOPRC_PARAM_MAX_FUNC, ///< ISF_CTRL: Prepare buffer for enable basic function of an ImagePipe device. #VDOPRC_FUNC
#if _TODO
	VDOPRC_PARAM_MAX_FUNC2, ///< ISF_CTRL: Prepare buffer for enable advance function of an ImagePipe device. #VDOPRC_FUNC2
#else
	VDOPRC_PARAM_RESV2, ///< reserved
#endif
	VDOPRC_PARAM_SCALEMATHOD_CTRL, ///< ISF_CTRL: Set ratio to decide small scaling or large scaling, threshold format = [31:16]/[15:0]
	VDOPRC_PARAM_SCALEMATHOD_SMALL, ///< ISF_CTRL: Set scaling mathod for small scaling. #IPL_IME_SCALER
	VDOPRC_PARAM_SCALEMATHOD_LARGE, ///< ISF_CTRL: Set scaling mathod for large scaling. #IPL_IME_SCALER
	VDOPRC_PARAM_NVX_CODEC, ///< ISF_CTRL: Set target codec for yuv compress mode of an ImagePipe device. #MAKEFOURCC()
#if 0
	VDOPRC_PARAM_SNAPSHOT_IMM,    ///< ISF_OUTx: Do snapshot of an ImagePipe device.
	VDOPRC_PARAM_SENSORID_IMM,  ///< ISF_IN1: Get sensor map (RO) of an ImagePipe device.
	VDOPRC_PARAM_ACTUAL_INSIZE_IMM,    ///< ISF_IN1: Get real input size (RO) of an ImagePipe device.
	VDOPRC_PARAM_ACTUAL_INFPS_IMM,    ///< ISF_IN1: Get real input fps (RO) of an ImagePipe device.
	VDOPRC_PARAM_DEFAULT_INFPS_IMM,  ///< ISF_IN1: Get default input fps (RO) of an ImagePipe device.
	VDOPRC_PARAM_LIVE_INFPS_IMM,  ///< ISF_IN1: Get live input fps (RO) of an ImagePipe device.
#endif
#if defined(_BSP_NA51023_)
	VDOPRC_PARAM_MOTIONDETECT_CB, ///< ISF_CTRL: Set motion-detect callback of an ImagePipe device.
#endif
	VDOPRC_PARAM_CODEC, ///< ISF_OUTx: Set target codec for this output of an ImagePipe device. #MAKEFOURCC()
#if defined(_BSP_NA51023_)
	VDOPRC_PARAM_VIEWTRACKING_CB, ///< ISF_CTRL: Set general view-tracking callback of an ImagePipe device.
#endif
	VDOPRC_PARAM_COLORSPACE,  ///< ISF_CTRL: Set color space of an ImagePipe device.
	VDOPRC_PARAM_IQ_ID,  ///< ISF_CTRL: Set IQ id of an ImagePipe device.
	VDOPRC_PARAM_3DNR_REFPATH,  ///< ISF_CTRL: Set 3DNR reference path of an ImagePipe device.
	VDOPRC_PARAM_OUT_FRC_IMM, ///< ISF_OUTx: Set target FRC for this output of an ImagePipe device.
	VDOPRC_PARAM_IN_FRC_IMM, ///< ISF_INx: Set target FRC for this output of an ImagePipe device.
	VDOPRC_PARAM_POLL_LIST,  ///< poll multi path to check frame buffer available
	VDOPRC_PARAM_IN_CFG_FUNC, ///< ISF_INx: Enable advance function of an ImagePipe device. #VDOPRC_IN_CFG_FUNC
	VDOPRC_PARAM_OUT_CFG_FUNC, ///< ISF_OUTx: Enable advance function of an ImagePipe device. #VDOPRC_OUT_CFG_FUNC
	VDOPRC_PARAM_LOWLATENCY_TRIG,  ///< ISF_CTRL: Set delay trigger of LOWLATENCY output path of an ImagePipe device.
	VDOPRC_PARAM_VPE_PRE_SCL_CROP, ///< ISF_OUTx: crop before scaling, for VPE only
	VDOPRC_PARAM_VPE_OUT_REGION,   ///< ISF_OUTx: specified output region, for VPE only, referring to ISF_VDO_WIN, imgaspect for bg size
	VDOPRC_PARAM_USER_OUT_BUF,     ///< ISF_CTRL: Set specified output buffer of an ImagePipe device.
	VDOPRC_PARAM_HEIGHT_ALIGN,     ///< ISF_OUTx: specified image height alignment value
	VDOPRC_PARAM_IN_DEPTH,     ///< ISF_INx: in depth value
	VDOPRC_PARAM_SYS_CAPS,  ///< ISF_CTRL: sys caps
	VDOPRC_PARAM_DMA_ABORT,  ///< ISF_CTRL: dma abort
	VDOPRC_PARAM_OUT_ORDER,  ///< ISF_OUTx: set ime out order
	VDOPRC_PARAM_DIRECT_CB = VDOPRC_PARAM_START + 0x00000100,  ///< get dir cb (direct mode)
};

#define VDOPRC_PIPE_OFF			0
#define VDOPRC_PIPE_DIRTY			0x00008000
#define VDOPRC_PIPE_CAPTURE		0x00000001 //do capture flow (WDR and DEFOG flow with small first PASS)
#define VDOPRC_PIPE_INPROC			0x00000002 //do RAW-domain input crop/mirror/flip process (RHE)
#define VDOPRC_PIPE_EFFECT			0x00000004 //do RAW-domain WDR/SHDR/Defog process (RHE)
#define VDOPRC_PIPE_NOISE			0x00000008 //do RAW-domain NR/3DNR filtering process (IFE)
#define VDOPRC_PIPE_WARP			0x00000010 //do YUY-domain warping process (DCE)
#define VDOPRC_PIPE_COLOR			0x00000020 //do YUY-domain gamma/tone/color/sharpness process (IPE) (if only select it, support 1 out only)
#define VDOPRC_PIPE_SCALE			0x00000040 //do YUV-domain scale process (IME)
#define VDOPRC_PIPE_OUTPROC		0x00000080 //do YUV-domain output crop process (IME)
#define VDOPRC_PIPE_VPE    		0x000000F2 //do YUV-domain scale process (VPE)
#define VDOPRC_PIPE_WARP_360		0x00000100 //do YUY-domain warping process (DCE) with panorama 360 flow
#define VDOPRC_PIPE_WARP_360_4V	0x00000200 //do YUY-domain warping process (DCE) with panorama 360 qual view flow
#define VDOPRC_PIPE_ALL			(VDOPRC_PIPE_INPROC|VDOPRC_PIPE_EFFECT|VDOPRC_PIPE_NOISE|VDOPRC_PIPE_WARP|VDOPRC_PIPE_COLOR|VDOPRC_PIPE_SCALE|VDOPRC_PIPE_OUTPROC)
#define VDOPRC_PIPE_RAWALL			VDOPRC_PIPE_ALL
#define VDOPRC_PIPE_RAWCAP			(VDOPRC_PIPE_RAWALL|VDOPRC_PIPE_CAPTURE)
#define VDOPRC_PIPE_YUVALL			(VDOPRC_PIPE_COLOR|VDOPRC_PIPE_SCALE|VDOPRC_PIPE_OUTPROC)
#define VDOPRC_PIPE_YUVCAP			(VDOPRC_PIPE_YUVALL|VDOPRC_PIPE_CAPTURE)
#define VDOPRC_PIPE_YUVAUX			(VDOPRC_PIPE_INPROC|VDOPRC_PIPE_SCALE|VDOPRC_PIPE_OUTPROC)
#define VDOPRC_PIPE_ISE    		VDOPRC_PIPE_YUVAUX //do YUV-domain scale process (ISE)
#define VDOPRC_PIPE_DEFAULT		VDOPRC_PIPE_ALL

//used from VDOPRC_PARAM_VPORT_START to VDOPRC_PARAM_VPORT_END
#define VDOPRC_PARAM_VPORT_SRC(oPort)	(VDOPRC_PARAM_VPORT_START+(oPort))       ///< virtual port: set share from src oPort

//isp effect
//used by VDOPRC_PARAM_FUNC
#define VDOPRC_FUNC_3DNR			0x00000008 //3DNR effect
#define VDOPRC_FUNC_MD				IPL_FUNC_MD //motion detect
#define VDOPRC_FUNC_WDR     		0x00000001 //wdr effect, (conflict with VDOPRC_FUNC_SHDR and VDOPRC_FUNC_STITCH)
#define VDOPRC_FUNC_SHDR     		0x00000002 //sensor HDR effect, (conflict with VDOPRC_FUNC_WDR and VDOPRC_FUNC_STITCH)
//#define VDOPRC_FUNC_STITCH     		IPL_FUNC_STITCH //stitch effect, (conflict with VDOPRC_FUNC_WDR and VDOPRC_FUNC_SHDR)
#define VDOPRC_FUNC_DEFOG     		0x00000004 //defog effect, (conflict with VDOPRC_FUNC_STITCH)
#define VDOPRC_FUNC_3DNR_STA		0x00000200
#define VDOPRC_FUNC_YUV_SUBOUT 	0x00000080
#define VDOPRC_FUNC_VA_SUBOUT 		0x00000100

//osg effect
#define VDOPRC_FUNC_DATASTAMP		0x00000010
#define VDOPRC_FUNC_PRIMASK 		0x00000020
#define VDOPRC_FUNC_PM_PIXELIZTION 	0x00000040

//in~out distort correction
#define VDOPRC_FUNC_GDC			0x00000400

////////////////////////////////////////////////////////////
//in binding
#define VDOPRC_IFUNC_DIRECT		0x00000001 //in direct from src
#define VDOPRC_IFUNC_ONEBUF		0x00000002 //in one-buf from src
//in crop/dir
#define VDOPRC_IFUNC_CROP			0x00010000
#define VDOPRC_IFUNC_MIRRORX		0x00020000
#define VDOPRC_IFUNC_MIRRORY		0x00040000
#define VDOPRC_IFUNC_ROTATE		0x00080000
//out scale
#define VDOPRC_IFUNC_SCALEUP		0x00100000
#define VDOPRC_IFUNC_SCALEDN		0x00200000
#define VDOPRC_IFUNC_SCALEFIX		0x00400000
//in pxlfmt
#define VDOPRC_IFUNC_Y			0x01000000
#define VDOPRC_IFUNC_YUV			0x02000000
#define VDOPRC_IFUNC_RGB			0x04000000
#define VDOPRC_IFUNC_RAW			0x08000000
#define VDOPRC_IFUNC_NRX			0x10000000
#define VDOPRC_IFUNC_YUVP			0x20000000
#define VDOPRC_IFUNC_EXTFMT		0x40000000
#define VDOPRC_IFUNC_ALLFMT		0xff000000

////////////////////////////////////////////////////////////
//out binding
#define VDOPRC_OFUNC_MD         0x00000100 //out md
#define VDOPRC_OFUNC_DIS        0x00000200 //out dis
#define VDOPRC_OFUNC_LOWLATENCY	0x00000400 //out low-latency to dest
#define VDOPRC_OFUNC_ONEBUF		0x00000800 //out one-buf to dest
//out crop/dir
#define VDOPRC_OFUNC_CROP			0x00010000
#define VDOPRC_OFUNC_MIRRORX		0x00020000
#define VDOPRC_OFUNC_MIRRORY		0x00040000
#define VDOPRC_OFUNC_ROTATE		0x00080000
//out scale
#define VDOPRC_OFUNC_SCALEUP		0x00100000
#define VDOPRC_OFUNC_SCALEDN		0x00200000
#define VDOPRC_OFUNC_SCALEFIX		0x00400000
//out pxlfmt
#define VDOPRC_OFUNC_Y			0x01000000
#define VDOPRC_OFUNC_YUV			0x02000000
#define VDOPRC_OFUNC_RGB			0x04000000
#define VDOPRC_OFUNC_RAW			0x08000000
#define VDOPRC_OFUNC_NVX			0x10000000
#define VDOPRC_OFUNC_YUVP			0x20000000
#define VDOPRC_OFUNC_YUVP2		0x40000000
#define VDOPRC_OFUNC_SAMEFMT		0x80000000
#define VDOPRC_OFUNC_ALLFMT		0xff000000


//used by VDOPRC_PARAM_FUNC2
//#define VDOPRC_FUNC2_YUVCOMPRESS	0x00000001 //enable YUV Compress of ImagePipe output 1
#define VDOPRC_FUNC2_TRIGGERSINGLE 	0x00000002
#define VDOPRC_FUNC2_MOTIONDETECT1	0x00000010 //enable Motion Detect of ImagePipe output 1
#define VDOPRC_FUNC2_MOTIONDETECT2	0x00000020 //enable Motion Detect of ImagePipe output 2
#define VDOPRC_FUNC2_MOTIONDETECT3	0x00000040 //enable Motion Detect of ImagePipe output 3
#define VDOPRC_FUNC2_VIEWTRACKING1	0x00000100 //enable View Tracking of ImagePipe output 1,2,3,4
#define VDOPRC_FUNC2_VIEWTRACKING2	0x00000200 //enable View Tracking of ImagePipe output 1,2,3,4
#define VDOPRC_FUNC2_VIEWTRACKING3	0x00000400 //enable View Tracking of ImagePipe output 1,2,3,4
#define VDOPRC_FUNC2_VIEWTRACKING4	0x00000500 //enable View Tracking of ImagePipe output 1,2,3,4
#define VDOPRC_FUNC2_DIRECT		0x00008000 //enable direct input from VdoIn to ImagePipe
#define VDOPRC_FUNC2_DROPOLDSIZE	0x00010000 //enable drop old-size of ImagePipe output

//for VDOPRC_PARAM_SCALEMATHOD_CTRL
#define VDOPRC_CTRL_SCALEMATHOD_RATIO(dest,src)	MAKE_UINT16_UINT16(dest,src) ///< ratio to select small or large

//for VDOPRC_PARAM_SCALEMATHOD_SMALL, VDOPRC_PARAM_SCALEMATHOD_LARGE
#define VDOPRC_CTRL_SCALEMATHOD_BICUBIC		0 ///< bicubic interpolation
#define VDOPRC_CTRL_SCALEMATHOD_BILINEAR	1 ///< bilinear interpolation
#define VDOPRC_CTRL_SCALEMATHOD_NEAREST		2 ///< nearest interpolation
#define VDOPRC_CTRL_SCALEMATHOD_INTEGRATION	3 ///< integration interpolation,support only scale down
#define VDOPRC_CTRL_SCALEMATHOD_AUTO		4 ///< automatical calculation

//for VDOPRC_PARAM_COLOR_SPACE
#define VDOPRC_CTRL_COLORSPACE_FULL		0 ///< full range
#define VDOPRC_CTRL_COLORSPACE_BT601	1 ///< BT.601
#define VDOPRC_CTRL_COLORSPACE_BT709	2 ///< BT.709


typedef struct _VDOPRC_DIS_PLUGIN {
    INT32 (*open)(UINT32 isp_id, UINT32* buffer_size); //calc working buffer size, vprc will alloc working buffer for start
    INT32 (*start)(UINT32 isp_id, UINT32 buffer_addr, UINT32 subsample_level, UINT32 scale_up_ratio); //start calc MV
    INT32 (*proc)(UINT32 isp_id, UINT32 frm_cnt, UINT32* mv_valid, INT32* mv_dx, INT32* mv_dy); //match MV by frame count
    INT32 (*stop)(UINT32 isp_id); //stop calc MV (do not access working buffer anymore)
    INT32 (*close)(UINT32 isp_id); //vprc will free working buffer
} VDOPRC_DIS_PLUGIN;

extern INT32 isf_vdoprc_reg_dis_plugin(VDOPRC_DIS_PLUGIN* p_dis_plugin);


extern ISF_UNIT isf_vdoprc0;
extern ISF_UNIT isf_vdoprc1;
extern ISF_UNIT isf_vdoprc2;
extern ISF_UNIT isf_vdoprc3;
extern ISF_UNIT isf_vdoprc4;
extern ISF_UNIT isf_vdoprc5;
extern ISF_UNIT isf_vdoprc6;
extern ISF_UNIT isf_vdoprc7;
extern ISF_UNIT isf_vdoprc8;
extern ISF_UNIT isf_vdoprc9;
extern ISF_UNIT isf_vdoprc10;
extern ISF_UNIT isf_vdoprc11;
extern ISF_UNIT isf_vdoprc12;
extern ISF_UNIT isf_vdoprc13;
extern ISF_UNIT isf_vdoprc14;
extern ISF_UNIT isf_vdoprc15;


#if _TODO

typedef void (*IPL_CB_FP)(IPL_CBMSG uiMsgID, void *Data);

#if defined(_BSP_NA51023_)
typedef UINT32 (*IPL_VIEWTRACKING_CB)(ISF_UNIT *p_thisunit, UINT32 frame_cnt, INT32* pOffx, INT32* pOffy);
#endif

typedef IPL_POST_PROC_FUNC_EN ISF_VDOPRC_FUNC;

typedef struct _ISF_VDOPRC_EVENT_CB {
	IPL_ISR_EVENT_FP ISR_CB_FP[IPL_ISR_MAX_CNT];    ///< ISR CB for photo mode
}
ISF_VDOPRC_EVENT_CB;

#endif

extern ISF_UNIT *uiIsfIPLMapping[VDOPRC_MAX_NUM];
#define ISF_IPL(id)  uiIsfIPLMapping[id]

#endif //ISF_VDOPRC_H


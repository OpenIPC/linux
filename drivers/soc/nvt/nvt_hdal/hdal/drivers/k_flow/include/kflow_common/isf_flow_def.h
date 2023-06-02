/**
    Stream Framework.

    A framework to integrate image data flow of all related application tasks.

    @file       isf_stream_def.h
    @ingroup    mIisf_stream
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
/*

    Concepts:

    1. stream is a data "directed acyclic graph" defined by many port-connected units.
    2. unit could have many input ports, and many output ports.
    3. unit could be connect togather if any source unit's output port is binding to destination unit's input port.
    4. each port is assigned to connected with PUSH, PULL, or NOTIFY-PULL connect-type.
    5. after open the stream, data will passing between units through their connected ports with behavior of its connect-type.


    Usage:

    1. Connect port to build a data "directed acyclic graph" by these APIs:

        isf_stream_begin(p_stream);
            isf_unit_set_output(rport, p_dest, state);
        isf_stream_end();

        isf_stream_begin(p_stream);
	        isf_unit_begin(p_unit, flag);
	            isf_unit_set_output(oport, p_dest, state);
	        isf_unit_end();
        isf_stream_end();

        isf_stream_begin(p_stream);
	        isf_unit_begin(p_unit, flag);
	            isf_unit_set_output(oport, p_dest, state);
	        isf_unit_end();
        isf_stream_end();

    2. Change state of port by these APIs:

        isf_unit_begin(p_unit, flag);
            isf_unit_set_output(oport, p_dest, state);
        isf_unit_end();

    3. Config or change parameters of units by these APIs:

        isf_unit_begin(p_unit, flag);
            isf_unit_set_connecttype(nport, ISF_CONNECT_xxxx);
            isf_unit_set_bufdepth(nport, depth);
            isf_unit_set_attr(nport, attr);
            isf_unit_set_vdosize(nport, VDO_SIZE(img_w, img_h));
            isf_unit_set_vdoformat(nport, VDO_PXLFMT_xxxx);
            isf_unit_set_vdoframectrl(nport, VDO_FRC(dest, src));
            isf_unit_set_vdoaspect(nport, VDO_AR(ar_x, ar_y));
            isf_unit_set_vdowinpose(nport, VDO_POSE(win_x, win_y));
            isf_unit_set_vdowinsize(nport, VDO_SIZE(win_w, win_h));
            isf_unit_set_vdodirect(nport, VDO_DIR_xxxx);
            isf_unit_set_audsize(nport, AUD_BITWIDTH_xxxx);
            isf_unit_set_audformat(nport, AUD_SNDMODE_xxxx);
            isf_unit_set_audframectrl(nport, AUD_FRC(dest, src));
            isf_unit_set_param(nport, param, value);
        isf_unit_end(void);
        isf_unit_get_param(p_unit, nport, param);


    4. Start to control graph by these APIs:

        isf_stream_open(p_stream);
        isf_stream_update_all(p_stream);
        isf_stream_close(p_stream);
        isf_stream_is_open(p_stream);


    5. Get port of units by these APIs:

        isf_unit_in(p_unit, iport);
        isf_unit_out(p_unit, oport);


    6. Get data from port and send data to port of units by these APIs:

        isf_unit_begin(p_unit, flag);
	        isf_unit_is_allow_push(p_destport);
	        isf_unit_push(p_destport, p_data, 0);
	        isf_unit_is_allow_pull(p_srcport);
	        isf_unit_pull(p_srcport, p_data, 0);
	        isf_unit_is_allow_notify(p_srcport);
	        isf_unit_notify(p_srcport, 0);
	        addr = isf_unit_new_data(size, p_data);
	        isf_unit_release_data(p_data);
        isf_unit_end(void);

*/


#ifndef _ISF_FLOW_DEF_H_
#define _ISF_FLOW_DEF_H_

#include "kwrap/platform.h"
#include "kwrap/type.h"
#include "kflow_common/type_vdo.h"
#include "kflow_common/type_aud.h"

#ifdef __KERNEL__
#else
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#endif

/**
    @addtogroup mIImgStm
*/
//@{

#define _ISF_BIND(a, b)	a ## b

///////////////////////////////////////////////////////////////////////////////
/**
     @name result value
*/
//@{
typedef enum {
	ISF_OK                      = 0, ///< ok
	ISF_ENTER                   = 1, ///< ok

	ISF_ERR_BEGIN               = -1, ///< begin
	
	// return error /////////////////////////////////////////////////////////////// 
	
	///< general error
	ISF_ERR_NOT_SUPPORT         = -1, 		///< not support
	///< execute error
    	//HD_ERR_NG                 = -10,     	///< general failure
	ISF_ERR_FAILED              = -11, 		///< general failure
	ISF_ERR_DRV                 = -12,    	///< driver call failure
	ISF_ERR_NOT_ALLOW           = -13,    	///< not allow in this case
	ISF_ERR_IGNORE              = -14, 		///< ignored or bypassed
	ISF_ERR_WAIT_TIMEOUT        = -15, 		///< wait timeout
	ISF_ERR_PROCESS_FAIL        = -19, 		///< data process, general fail
	///< module state error
    	ISF_ERR_INIT                = -20,     	///< already initialised 
    	ISF_ERR_UNINIT              = -21,     	///< not initialised yet
	ISF_ERR_INVALID_STATE       = -22, 		///< invalid state
    	//ISF_ERR_TERM              = -23,     	///< module has terminated 
	ISF_ERR_NOT_OPEN_YET        = -24, 		///< already close, or not open yet
	ISF_ERR_ALREADY_OPEN        = -25, 		///< already open, or not close yet
    	ISF_ERR_NOT_START           = -26,     	///< path is not start yet
    	ISF_ERR_ALREADY_START       = -27,     	///< path is already start
	ISF_ERR_NOT_CONNECT_YET     = -28, 		///< not connect yet
	ISF_ERR_ALREADY_CONNECT     = -29, 		///< already connect
	///< parameter error
    	//ISF_ERR_UNIQUE            = -30,     	///< unique id needed 
	ISF_ERR_INVALID_UNIT_ID     = -31, 		///< invalid unit
	ISF_ERR_INVALID_PORT_ID     = -32, 		///< invalid input port id
    	ISF_ERR_PATH                = -33,     	///< invalid path id
	ISF_ERR_INVALID_PARAM       = -34, 		///< invalid parameter id
	ISF_ERR_INVALID_VALUE       = -35, 		///< parameter value is invalid
	ISF_ERR_PARAM_EXCEED_LIMIT  = -36, 		///< parameter value is out of range
	ISF_ERR_NULL_POINTER        = -37, 		///< parameter pointer is NULL
    	ISF_ERR_NO_CONFIG           = -38,     	///< module device config or path config is not set
    	ISF_ERR_NO_DRV_CONFIG       = -39,     	///< module driver config is not set
	///< object or resource error
    	//ISF_ERR_RESOURCE          = -40,     	///< insufficient resources for request 
	ISF_ERR_NOT_AVAIL           = -41,     	///< resources not available
	//ISF_ERR_NOT_FOUND         = -42, 		///< not found
    	//ISF_ERR_EOL               = -43,     	///< a list traversal has ended
	ISF_ERR_NOT_REL_YET         = -44,     	///< object is in use. 
    	ISF_ERR_ALREADY_REL         = -45,     	///< object is not in use. 
    	//ISF_ERR_LOCKED            = -46,     	///< object locked 
    	//ISF_ERR_DELETED           = -47,     	///< object has been deleted
	///< data or buffer error
	ISF_ERR_HEAP                = -50,     	///< heap full (application)
	ISF_ERR_BUFFER_CREATE       = -51, 		///< buffer create pool fail
	ISF_ERR_NEW_FAIL            = -52, 		///< blk new fail
	ISF_ERR_INVALID_SIGN        = -53, 		///< invalid signature
	ISF_ERR_INVALID_DATA        = -54, 		///< invalid data format or content
	ISF_ERR_QUEUE_FULL          = -55, 		///< data input, push to input queue, but queue is full
	ISF_ERR_QUEUE_EMPTY         = -56, 		///< data output, pull from output queue, but queue is empty

	ISF_ERR_INTERMEDIATE        = -91, 		///< intermediate return - successful but not complete

	ISF_ERR_END                 = -1000, 	///< end

	// internal error /////////////////////////////////////////////////////////////// 
	
	ISF_ERR_COPY_FROM_USER      = -1001, ///< copy cmd failed
	ISF_ERR_COPY_TO_USER        = -1002, ///< copy cmd failed

	//for general operation
	ISF_ERR_INACTIVE_STATE      = -1003, ///< inactive state
	//ISF_ERR_WAIT_START          = -1004, ///< wait for start
	//ISF_ERR_WAIT_STOP           = -1005, ///< wait for stop
	ISF_ERR_FATAL               = -1006, ///< general fatal fail
	//for general value
	ISF_ERR_VER_NOTMATCH        = -1010, ///< version not match
	ISF_ERR_NULL_OBJECT         = -1011, ///< object pointer is NULL
	ISF_ERR_NULL_FUNCCB         = -1012, ///< the object has null function pointer
	ISF_ERR_INVALID_NAME        = -1013, ///< invalid name
	ISF_ERR_PARAM_NOT_SUPPORT   = -1014, ///< param enum out of range
	ISF_ERR_ADD_FAIL            = -1015, ///< blk add fail
	ISF_ERR_RELEASE_FAIL        = -1016, ///< blk release fail
	//for stream and unit
	ISF_ERR_NAME_ISNOTMATCH     = -2000, ///< cannot find stream or unit by match name!
	ISF_ERR_COUNT_OVERFLOW      = -2001, ///< total count of connected unit is overflow!
	ISF_ERR_INVALID_STREAM_ID   = -2002, ///< invalid stream
	//for port
	ISF_ERR_CONNECT_NOTMATCH    = -2003, ///< build connect step1: match connect type is failed!
	ISF_ERR_CONNECT_BINDDEST    = -2004, ///< build connect step2: bind to dest unit is failed!
	ISF_ERR_CONNECT_BINDSRC     = -2005, ///< build connect step3: bind to src unit is failed!
	ISF_ERR_CONNECT_CHANGED     = -2006, ///< connect is changed
	ISF_ERR_CONNECT_NOTBUILD    = -2007, ///< connect is not build
	ISF_ERR_INACTIVE_PORT       = -2008, ///< inactive port
	ISF_ERR_NULL_PORT           = -2009, ///< port is NULL
	//for data new, release
	ISF_ERR_BUFFER_DESTROY      = -2010, ///< buffer destroy pool fail
	ISF_ERR_BUFFER_GET          = -2011, ///< buffer get new block fail (reference_count = 1)
	ISF_ERR_BUFFER_ADD          = -2012, ///< buffer add block fail (reference_count ++)
	ISF_ERR_BUFFER_RELEASE      = -2013, ///< buffer release block fail (reference_count --)
	ISF_ERR_INVALID_DATA_ID     = -2014, ///< invalid data id
	//for data drop
	ISF_ERR_FLUSH_DROP          = -3000, ///< data input, internal is going to stop or force clean
	ISF_ERR_FRC_DROP            = -3001, ///< data input or output, drop by video frame control
	ISF_ERR_QUEUE_DROP          = -3002, ///< data output, drop by clean queue
	ISF_ERR_OUT_DROP            = -3003, ///< data output, but no bind and no queue
	//for data param
	ISF_ERR_DATA_EXCEED_LIMIT   = -3004, ///< data value out of range
	ISF_ERR_DATA_NOT_SUPPORT    = -3005, ///< data enum out of range
	ISF_ERR_INCOMPLETE_DATA     = -3006, ///< in-complete data
	//for data queue
	ISF_ERR_QUEUE_ERROR         = -3007, ///< data input, push to input queue, but queue is full
	//for data proc
	ISF_ERR_START_FAIL          = -3008, ///< data input, internal start fail
	ISF_ERR_STOP_FAIL           = -3006, ///< data input, internal stop fail


	ENUM_DUMMY4WORD(ISF_RV)
} ISF_RV;
//@}


///////////////////////////////////////////////////////////////////////////////
/**
     unit id.
*/
#define ISF_MAX_UNIT		0xff ///< end

#define ISF_UNIT_BASE		0x81

#define ISF_UNIT_VDOCAP	ISF_UNIT_BASE ///< video in
#define ISF_MAX_VDOCAP		8

#define ISF_UNIT_VDOOUT	(ISF_UNIT_VDOCAP+ISF_MAX_VDOCAP) ///< video out
#define ISF_MAX_VDOOUT	2

#define ISF_UNIT_VDOPRC	(ISF_UNIT_VDOOUT+ISF_MAX_VDOOUT) ///< video proc
#define ISF_MAX_VDOPRC	128

#define ISF_UNIT_VDOENC	(ISF_UNIT_VDOPRC+ISF_MAX_VDOPRC) ///< video encode
#define ISF_MAX_VDOENC	16

#define ISF_UNIT_VDODEC	(ISF_UNIT_VDOENC+ISF_MAX_VDOENC) ///< video decode
#define ISF_MAX_VDODEC	16

#define ISF_UNIT_AUDCAP	(ISF_UNIT_VDODEC+ISF_MAX_VDODEC) ///< audio cap
#define ISF_MAX_AUDCAP		2

#define ISF_UNIT_AUDOUT	(ISF_UNIT_AUDCAP+ISF_MAX_AUDCAP) ///< audio out
#define ISF_MAX_AUDOUT	2

#define ISF_UNIT_AUDENC	(ISF_UNIT_AUDOUT+ISF_MAX_AUDOUT) ///< audio encode
#define ISF_MAX_AUDENC	16

#define ISF_UNIT_AUDDEC	(ISF_UNIT_AUDENC+ISF_MAX_AUDENC) ///< audio decode
#define ISF_MAX_AUDDEC	16

#define ISF_UNIT_DUMMY	(ISF_UNIT_AUDDEC+ISF_MAX_AUDDEC) ///< audio decode
#define ISF_MAX_DUMMY		1

#define ISF_UNIT_MAX		(ISF_UNIT_DUMMY+ISF_MAX_DUMMY-1)

#ifdef __KERNEL__
#define ISF_UNIT_ID(uid,dev)	(_ISF_BIND(ISF_UNIT_,uid) + (dev))
#else
#define ISF_UNIT(uid,dev)		(_ISF_BIND(ISF_UNIT_,uid) + (dev))
#endif


///////////////////////////////////////////////////////////////////////////////
/**
     output port id.
*/
#define ISF_OUT_BASE  	0
#define ISF_OUT_MAX		(ISF_OUT_BASE+128-1)

#define ISF_OUT(i)		(ISF_OUT_BASE+(i))


/**
     input port id.
*/
#define ISF_IN_BASE  		128
#define ISF_IN_MAX		(ISF_IN_BASE+128-1)

#define ISF_IN(i)			(ISF_IN_BASE+(i))

/**
     root port id.
*/
#define ISF_ROOT_BASE  	256
#define ISF_ROOT_MAX		(ISF_ROOT_BASE+128-1)

#define ISF_ROOT(i)		(ISF_ROOT_BASE+(i))

/**
     ctrl port id.
*/
#define ISF_CTRL    		0xffff ///< unit control

/**
     combine of unit id and port id.

     or,
     combine of stream id and root id.
*/
#ifdef __KERNEL__
#define ISF_PORT_ID(uid,pid)	(((uid)<<16) | ((pid)&0xffff))
#else
#define ISF_PORT(uid,pid)	(((uid)<<16) | ((pid)&0xffff))
#endif

/**
     special port id
*/
#define ISF_PORT_NULL     (0xfff0) ///< disconnect output of port
#define ISF_PORT_EOS      (0xfff1) ///< connect output of port as "end of stream"
#define ISF_PORT_KEEP     (0xfffe) ///< keep output of port



///////////////////////////////////////////////////////////////////////////////
// param
typedef enum {

	///< using by ISF_FLOW_IOCTL_PARAM_ITEM

	ISF_UNIT_PARAM_CHN_BEGIN = 		0x00001001,
		ISF_UNIT_PARAM_CONNECTTYPE =	0x00001001, //config before start
		ISF_UNIT_PARAM_ATTR =			0x00001002, //config before start
		ISF_UNIT_PARAM_BUFDEPTH =		0x00001003, //config before start
		ISF_UNIT_PARAM_MULTI =		0x0000100f,
		ISF_UNIT_PARAM_VDOMAX = 		0x00001010, //config before open
		ISF_UNIT_PARAM_VDOFRAME = 		0x00001011,
		ISF_UNIT_PARAM_VDOWIN = 		0x00001012,
		ISF_UNIT_PARAM_VDOFPS = 		0x00001013,
		ISF_UNIT_PARAM_VDOFUNC = 		0x0000101f,
		ISF_UNIT_PARAM_AUDMAX = 		0x00001020, //config before open
		ISF_UNIT_PARAM_AUDFRAME = 		0x00001021,
		ISF_UNIT_PARAM_AUDFPS = 		0x00001023,
		ISF_UNIT_PARAM_AUDFUNC = 		0x0000102f,
	ISF_UNIT_PARAM_CHN_END =			0x00001fff,

	ISF_UNIT_PARAM_OTHER_BEGIN =		0x00002000,

	ISF_UNIT_PARAM_OTHER_END =			0x00008fff

} ISF_PARAM;


///////////////////////////////////////////////////////////////////////////////
// value
/**
     @name VALUE for keep last value
*/
#define ISF_VALUE_KEEP              0xffffffff ///< keep last value


/**
     @name attr of port
*/
//#define ISF_PORT_ATTR_READYSRCBUF   0x00000001      ///< if enable this flag => never auto lock data when Push(),Pull()
//#define ISF_PORT_ATTR_KEEPSRCBUF    0x00000002      ///< if enable this flag => never auto unlock data when Push(),Pull()


/**
     connect type of port.
*/
typedef enum {
	ISF_CONNECT_NONE = 0,  ///< dummy connect type
	ISF_CONNECT_PUSH = 0x01,  ///< push connect type
	ISF_CONNECT_PULL = 0x02,  ///< pull connect type
	ISF_CONNECT_NOTIFY = 0x04,  ///< notify connect type, support ISF_CONNECT_NOTIFY|ISF_CONNECT_PUSH
	ISF_CONNECT_SYNC = 0x08,  ///< sync connect type, support ISF_CONNECT_PUSH|ISF_CONNECT_SYNC
	ISF_CONNECT_ALL = ISF_CONNECT_PUSH | ISF_CONNECT_PULL | ISF_CONNECT_NOTIFY | ISF_CONNECT_SYNC, ///< all connect type
	ENUM_DUMMY4WORD(ISF_CONNECT_TYPE)
} ISF_CONNECT_TYPE;


/**
     port state
*/
typedef enum {
	ISF_PORT_STATE_OFF      = 0x00, ///< off
	ISF_PORT_STATE_READY    = 0x01, ///< ready
	ISF_PORT_STATE_RUN      = 0x02, ///< run
#if 0
	ISF_PORT_STATE_WAIT     = 0x03, ///< wait
	ISF_PORT_STATE_SLEEP    = 0x04, ///< sleep
#endif
	ISF_PORT_STATE_ALL      = 0xff, ///< any state for compare
	ENUM_DUMMY4WORD(ISF_PORT_STATE)
} ISF_PORT_STATE;


/**
    event message.
*/
typedef struct _ISF_EVENT_MSG {
	UINT32 sign;					///< signature = MAKEFOURCC('E','V','N','T')
	UINT32 event; ///< default 0
	UINT32 param1; ///< default 0
	UINT32 param2; ///< default 0
	UINT32 param3; ///< default 0
} ISF_EVENT_MSG; ///< size = 4 words


/**
    performance record.
*/
typedef struct _ISF_PERF_RECORD {
	UINT32 sign;					///< signature = MAKEFOURCC('P','E','R','F')
	UINT64 timestamp_n; 			///< time-stamp new
	UINT64 timestamp_ib; 			///< time-stamp vdocap process start
	UINT64 timestamp_ie; 			///< time-stamp vdocap process end (push)
	UINT64 timestamp_pb; 			///< time-stamp vdoprc process start
	UINT64 timestamp_pe; 			///< time-stamp vdoprc process end (push)
	UINT64 timestamp_eb; 			///< time-stamp vdoenc process start
	UINT64 timestamp_ee; 			///< time-stamp vdoenc process end (sync)
	UINT64 timestamp_r; 			///< time-stamp release
} ISF_PERF_RECORD;


///////////////////////////////////////////////////////////////////////////////
// param struct

typedef struct _ISF_SET_ {
	UINT32 param; ///< param
	UINT32 value; ///< value
}
ISF_SET;

typedef struct _ISF_GET_ {
	UINT32 param; ///< param
	UINT32* p_value; ///< value
}
ISF_GET;

typedef struct _ISF_ATTR_ {
	UINT32 attr; ///< attr
}
ISF_ATTR;

typedef struct _ISF_VDO_FUNC_ {
	UINT32 src; ///< src
	UINT32 func; ///< func
}
ISF_VDO_FUNC;

typedef struct _ISF_VDO_MAX_ {
	UINT32 max_frame; ///< frame buffer count, assign before start
	UINT32 max_imgfmt; ///< maximum image format, assign before start
	USIZE max_imgsize; ///< maximum image size, assign before start
}
ISF_VDO_MAX;

typedef struct _ISF_VDO_FRAME_ {
	UINT32 direct; ///< direction, set by isf_unit_set_vdodirection()
	UINT32 imgfmt; ///< image format, set by isf_unit_set_vdoimgformat()
	USIZE imgsize; ///< image size, set by isf_unit_set_vdoimgsize()
}
ISF_VDO_FRAME;

typedef struct _ISF_VDO_FPS_ {
	UINT32 framepersecond; ///< frame per second, set by isf_unit_set_vdoframerate()
}
ISF_VDO_FPS;

typedef struct _ISF_VDO_WIN_ {
	USIZE imgaspect; ///< image aspect size, set by isf_unit_set_vdoaspect()
	URECT window; ///< crop or cast window range, set by isf_unit_set_vdopostwindow()
}
ISF_VDO_WIN;

typedef struct _ISF_AUD_MAX_ {
	UINT32 max_frame; ///< frame buffer count, assign before start
	UINT32 max_bitpersample; ///< maximum, assign before start
	UINT32 max_channelcount; ///< maximum, assign before start
	UINT32 max_samplepersecond; ///< maximum, assign before start
} ISF_AUD_MAX;

typedef struct _ISF_AUD_FRAME_ {
	UINT32 bitpersample; ///< audio sample bits
	UINT32 channelcount; ///< audio channel count
	UINT32 samplecount; ///< audio sample count
} ISF_AUD_FRAME;

typedef struct _ISF_AUD_FPS_ {
	UINT32 samplepersecond; ///< audio sample rate, for PULL connect type, this is TargetSampleRate
} ISF_AUD_FPS;

///////////////////////////////////////////////////////////////////////////////
// data
//#define ISF_TS //using timestamp
//#define ISF_DUMP_TS //dump timestamp

#define ISF_DATA_DESC_SIZE  38 ///< size of data desc in WORDS

#ifdef ISF_TS
#define ISF_DATA_SIZE       (44+8+8+8) ///< size of data in WORDS
#else
#define ISF_DATA_SIZE       44 ///< size of data in WORDS
#endif

/**
    General data and its descriptor.
*/
typedef struct _ISF_DATA {
	UINT32 sign; ///< signature, equal to ISF_SIGN_DATA
	UINT32 size; ///< data size
	UINT32 h_data;  ///< for common blk = "nvtmpp blk_id"; for private blk = "custom data handle";
	UINT32 src; ///< [private blk] src port, [common blk] for in, it is src's oport, for out, it is this oport
	UINT32 func; ///< [private blk] pool id, [common blk] ;
	void* p_class;  ///< func of "custom data handle", or NULL if h_data is "nvtmpp blk_id"
	/*
	ISF_EVENT_MSG event; ///< default 0
	UINT64 serial; ///< serial id
	UINT64 timestamp; ///< time-stamp ImagePipe process start
	*/
	#ifdef ISF_TS
	UINT64 ts_vprc_in[4];
	UINT64 ts_vprc_out[4];
	UINT64 ts_venc_in[4];
	#endif
	UINT32 desc[ISF_DATA_DESC_SIZE]; ///< data descriptor, the first word must be FOURCC
} ISF_DATA;
STATIC_ASSERT(sizeof(ISF_DATA) / sizeof(UINT32) == ISF_DATA_SIZE);


#define ISF_FLOW_MAX	8	//maximum isf_flow process


#ifdef __KERNEL__
#else
#ifdef __cplusplus
}
#endif
#endif

#endif //_ISF_FLOW_DEF_H_

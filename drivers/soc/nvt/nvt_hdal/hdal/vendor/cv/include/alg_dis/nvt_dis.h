#ifndef _NVT_DIS_H_
#define _NVT_DIS_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "vendor_dis.h"


#define DIS_BLKHMAX                 4
#define DIS_BLKVMAX                 64
#define DIS_MDSNUMMAX               32
#define DIS_MVNUMMAX                (DIS_BLKHMAX*DIS_BLKVMAX*DIS_MDSNUMMAX)

#define DIS_VECTOR_PERCENT          50
#define DIS_NUM_CAND_FILTER         8
#define DIS_TBUFNUM                 8   ///< temporal filter buffer number

/**
    DIS Motion Vector.

    Structure of directional motion vector elements.
*/
//@{
typedef struct _DIS_MOTION_VECTOR {
	INT32 ix; ///< x component
	INT32 iy; ///< y component
} DIS_MOTION_VECTOR;
//@}


/**
    DIS Motion Information.

    Structure of information in a motion vector.
*/
//@{
typedef struct _DIS_MOTION_INFOR {
	INT32   ix;          ///< x component
	INT32   iy;          ///< y component
	UINT32  ui_sad;      ///< sum of absolute difference
	UINT32  ui_cnt;      ///< number of edge pixels (src block)
	UINT32  ui_idx;      ///< block index
	BOOL    bvalid;      ///< reliability of MV
} DIS_MOTION_INFOR;
//@}

/**
    DIS Motion vector information

    Structure of DIS motion vector information.
*/
//@{
typedef struct _DIS_MV_INFO_SIZE {
	DIS_MOTION_INFOR *motvec;
	UINT32 ui_num_h;
	UINT32 ui_num_v;
} DIS_MV_INFO_SIZE;
//@}


/**
    Struct DIS MDS dimension.

    Structure of dimension elements in a motion detecting section.
*/
//@{
typedef struct _DIS_MDS_DIM {
	UINT8 ui_blknum_h;    ///< horizontal block number in use
	UINT8 ui_blknum_v;    ///< vertical block number in use
	UINT8 ui_mdsnum;     ///< total MDS number in use
} DIS_MDS_DIM;
//@}

/**
   DIS Block Size Selection
*/
//@{
typedef enum {
	DIS_BLKSZ_64x48 = 0,  ///< positive LUT
	DIS_BLKSZ_32x32 = 1,  ///< linear LUT
	ENUM_DUMMY4WORD(DIS_BLKSZ)
} DIS_BLKSZ;
//@}


/**
    DIS IPC init buffer.
*/
typedef struct {
	UINT32 addr;                             /* IPC buffer addr */
	UINT32 size;                             /* IPC buffer size */
} DIS_IPC_INIT;


/**
    DIS ALG Entire Configuration

    Structure of DIS ALG parameters when one wants to configure this module.
*/
//@{
typedef struct _DIS_PARAM_STRUCT {
	UINT32 in_size_h;                    ///< input horizontal pixels
	UINT32 in_size_v;                    ///< input vertical pixels
	UINT32 in_lineofs;                   ///< input line offset
	UINT32 in_add0;                      ///< input starting address 0
	UINT32 in_add1;                      ///< input starting address 1
	UINT32 in_add2;                      ///< input starting address 2
	UINT32 frame_cnt;                    ///< frame count
} DIS_PARAM;
//@}

//----------------------------------------------------------------------
/**
	ETH input parameter structure
*/
typedef struct _ETH_IN_PARAM {
	BOOL enable;		///< eth enable
	BOOL out_bit_sel;	///< 0 --> output 2 bit per pixel, 1 --> output 8 bit per pixel
	BOOL out_sel;		///< 0 --> output all pixel, 1 --> output pixel select by b_h_out_sel/b_v_out_sel
	BOOL h_out_sel; 	///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	BOOL v_out_sel; 	///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	
	UINT16 th_low;		///< edge threshold
	UINT16 th_mid;
	UINT16 th_high;

} DIS_ETH_IN_PARAM;

/**
	ETH output parameter structure
*/

typedef struct _DIS_ETH_OUT_PARAM {
	UINT32	         w;		        ///< eth output width
	UINT32	         h;		        ///< eth output height
	UINT32           out_lofs;      ///< eth output lineoffset;
} DIS_ETH_OUT_PARAM;

/**
	ETH output buffer structure
*/
typedef struct _DIS_ETH_BUFFER_INFO {
	UINT32 ui_inadd;        ///< output buffer address
	UINT32 buf_size;	    ///< output buffer size [Set SIE]kflow will force disable eth when buffer size < eth out size, [Get] none
	UINT32 frame_cnt;
} DIS_ETH_BUFFER_INFO;
//----------------------------------------------------------------------

/**
    Get the working buffer size that dis_alg need
    @return UINT32: the working buffer size that dis_alg need.
*/
extern UINT32      dis_get_prvmaxBuffer(void);

/**
    Get the working buffer size that dis_eth need
    @return UINT32: the working buffer size that dis_eth need.
*/
extern UINT32      dis_eth_get_prvmaxBuffer(void);

/**
    init buff size need by dis_alg.
    @param p_buf: the address for buff.
    @return VOID.
*/
extern void        dis_initialize(DIS_IPC_INIT *p_buf);

/**
    init buff size need by dis_eth, and enable eth.
    @param p_buf: the address for buff.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
HD_RESULT          dis_eth_initialize(DIS_IPC_INIT *p_buf);

/**
    end dis_alg.
    @return VOID.
*/
extern void        dis_end(void);

/**
    uninit dis_eth.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
HD_RESULT          dis_eth_uninitialize(void);

/**
    process the dis_alg with compensation, need set disinfor firstly.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
extern HD_RESULT   dis_process(void);
/**
    process the dis_alg without compensation, need set disinfor firstly.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
extern HD_RESULT   dis_process_no_compen_info(void);

/**
    set dis infor.
    @param[in] p_disinfo: dis infor.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
extern HD_RESULT   dis_set_disinfor(DIS_PARAM *p_disinfo);

/**
    set dis blksize.
    @param[in] blksz: dis block size.
    @return BOOL: true for ok , false for error.
*/
extern BOOL        dis_set_blksize(DIS_BLKSZ blksz);

/**
    get dis inh.
    @param[in] VOID.
    @return UINT32: height.
*/
extern UINT32      dis_get_cur_dis_inh(void);

/**
    get dis inv.
    @param[in] VOID.
    @return UINT32: width.
*/
extern UINT32      dis_get_cur_dis_inv(void);

/**
    get dis input info.
    @param[in&out]  p_info: dis infor.
    @return VOID.
*/
extern void        dis_get_input_info(DIS_PARAM *p_info);

/**
    get dis blksz.
    @param[in]  VOID.
    @return DIS_BLKSZ: dis block size.
*/
extern DIS_BLKSZ   dis_get_blksize_info(void);

/**
    get dis blksz.
    @param[in]  VOID.
    @return DIS_MDS_DIM: dis mds dimension.
*/
extern DIS_MDS_DIM dis_get_mds_dim(void);

/**
    get dis trigger status.
    @param[in]  VOID.
    @return UINT32: 0 for not trigger; 1 for trigger.
*/
extern UINT32      dis_get_trigger_status(void);

//#NT#2018/07/23#joshua liu -begin
//#NT#Support time stamp
/**
    push dis time stamp.
    @param[out]  time_stamp: time stamp.
    @param[in]   frame_count: index.
    @return VOID.
*/
extern void       dis_push_time_stamp(UINT64 time_stamp, UINT32 frame_count); // Set time_stamp using frame_count as the index. Pushing if there is a matched counter else discarding the time-stamp.

/**
    get dis motion vector.
    @param[out]  p_mvinfo_size: dis motion vector infor.
    @return UINT64: time stamp.
*/
extern UINT64     dis_get_ready_motionvec(DIS_MV_INFO_SIZE *p_mvinfo_size);	  // Retrieve the newest valid motion vectors and time stamp.  Return time stamp (-1 if failed)
//#NT#2018/07/23#joshua liu -end

/**
    get dis eth input infor.
    @param[out]  p_dis_input_param: input infor.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
HD_RESULT         dis_eth_get_input_info(DIS_ETH_IN_PARAM* p_dis_input_param);

/**
    get dis eth output infor.
    @param[out]  p_eth_out_info: output infor.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
HD_RESULT         dis_eth_get_out_info(DIS_ETH_OUT_PARAM* p_eth_out_info);

/**
    get dis eth input addr.
    @param[out]  p_eth_out_addr: output addr.
    @return HD_RESULT,HD_OK for ok. other for error.
*/
HD_RESULT         dis_eth_get_out_addr(DIS_ETH_BUFFER_INFO* p_eth_out_addr);



#endif

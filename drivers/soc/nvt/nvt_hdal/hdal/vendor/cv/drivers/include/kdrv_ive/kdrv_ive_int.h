#ifndef _KDRV_IVE_INT_H_
#define _KDRV_IVE_INT_H_

#include "kwrap/type.h"
#include "kdrv_ive.h"
#include "ive_lib.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"


typedef ER (*KDRV_IVE_SET_FP)(UINT32, void* );
typedef ER (*KDRV_IVE_GET_FP)(UINT32, void* );
typedef INT32(*KDRV_IPP_ISRCB)(UINT32, UINT32, void *, void *);

/**
	max ive kdrv handle number, current max is 8 due to flag config
*/
#if !defined(CONFIG_NVT_SMALL_HDAL)
#define KDRV_IVE_HANDLE_MAX_NUM (8)
#else
#define KDRV_IVE_HANDLE_MAX_NUM (4)
#endif

#define KDRV_IVE_LOCK			FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IVE_FMD			FLGPTN_BIT(8)
#define KDRV_IVE_TIMEOUT		FLGPTN_BIT(9)
#define KDRV_IVE_HDL_UNLOCK		FLGPTN_BIT(10)
#define KDRV_IVE_LOCK_BITS		0x000000ff
#define KDRV_IVE_INIT_BITS		(KDRV_IVE_LOCK_BITS | KDRV_IVE_HDL_UNLOCK)


typedef struct _KDRV_IVE_PRAM {
	KDRV_IVE_IN_IMG_INFO			in_img_info;
	KDRV_IVE_IMG_IN_DMA_INFO		in_dma_info;
	KDRV_IVE_IMG_OUT_DMA_INFO		out_dma_info;
	KDRV_IPP_ISRCB			  		isr_cb;
	KDRV_IVE_GENERAL_FILTER_PARAM 	gen_filt;
	KDRV_IVE_MEDIAN_FILTER_PARAM 	medn_filt;
	KDRV_IVE_EDGE_FILTER_PARAM 		edge_filt;
	KDRV_IVE_NON_MAX_SUP_PARAM 		non_max_sup;
	KDRV_IVE_THRES_LUT_PARAM 		thres_lut;
	KDRV_IVE_MORPH_FILTER_PARAM		morph_filt;
	KDRV_IVE_INTEGRAL_IMG_PARAM		integral_img;
	KDRV_IVE_OUTSEL_PARAM           outsel_info;
	KDRV_IVE_LL_DMA_INFO            ll_dma_info;      ///< IVE LinkedList address
 	KDRV_IVE_IRV_PARAM	            irv;
	KDRV_IVE_DMA_ABORT_PARAM	    ive_dma_abort_data;
	KDRV_IVE_FLOW_CT_PARAM          flow_ct;
} KDRV_IVE_PRAM, *pKDRV_IVE_PRAM;


/**
	IVE KDRV handle structure
*/
#define KDRV_IVE_HANDLE_LOCK	0x00000001

typedef struct _KDRV_IVE_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_IPP_ISRCB isrcb_fp;
} KDRV_IVE_HANDLE;

typedef enum {
	KDRV_IVE_IDLE = 0,
	KDRV_IVE_TRIG = 1,
	KDRV_IVE_WAIT = 2,
	ENUM_DUMMY4WORD(KDRV_IVE_HANDLE_NONBLOCK_STAT)
} KDRV_IVE_HANDLE_NONBLOCK_STAT;

typedef struct _KDRV_IVE_HANDLE_NONBLOCK {
	UINT8 is_change;
	KDRV_IVE_HANDLE_NONBLOCK_STAT stat; //0:idle, 1:trigger, 2:waitdone
} KDRV_IVE_HANDLE_NONBLOCK;

typedef enum {
	FLG_ID_KDRV_IVE,
	KDRV_IVE_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_IVE_FLAG)
} KDRV_IVE_FLAG;

typedef enum {
	SEMID_KDRV_IVE,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_IVE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_IVE_SEM)
} KDRV_IVE_SEM;

// IVE
typedef struct _KDRV_IVE_SEM_INFO_ {
	SEM_HANDLE  semphore_id;				///< Semaphore ID

#if defined __UITRON || defined __ECOS || defined(__FREERTOS)
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_IVE_SEM_TABLE;

#if defined(__FREERTOS)
extern ID kdrv_ive_flag_id[KDRV_IVE_FLAG_COUNT];
#else
extern UINT32 kdrv_ive_flag_id[KDRV_IVE_FLAG_COUNT];
#endif
extern KDRV_IVE_SEM_TABLE kdrv_ive_semtbl[KDRV_IVE_SEMAPHORE_COUNT];
extern void kdrv_ive_install_id(void);
extern void kdrv_ive_uninstall_id(void);
extern ID kdrv_ive_get_flag_id(KDRV_IVE_FLAG idx);
extern SEM_HANDLE* kdrv_ive_get_sem_id(KDRV_IVE_SEM idx);
extern void kdrv_ive_init(void);

#endif //_KDRV_IVE_INT_H_

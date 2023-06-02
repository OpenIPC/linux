#ifndef _KDRV_DIS_INT_H_
#define _KDRV_DIS_INT_H_


#include "dis_platform.h"
#include "kdrv_dis.h"
#include "dis_lib.h"

typedef ER (*KDRV_DIS_SET_FP)(UINT32, void* );
typedef ER (*KDRV_DIS_GET_FP)(UINT32, void* );

/**
    max dis kdrv handle number, current max is 8 due to flag config
*/
#define KDRV_DIS_HANDLE_MAX_NUM (8)

#define KDRV_DIS_LOCK			FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_DIS_FMD			FLGPTN_BIT(8)
#define KDRV_DIS_TIMEOUT		FLGPTN_BIT(9)
#define KDRV_DIS_HDL_UNLOCK		FLGPTN_BIT(10)
#define KDRV_DIS_LOCK_BITS		0x000000ff
#define KDRV_DIS_INIT_BITS		(KDRV_DIS_LOCK_BITS | KDRV_DIS_HDL_UNLOCK)

typedef INT32(*KDRV_DIS_ISRCB)(UINT32, UINT32, void *, void *);



typedef struct _KDRV_DIS_PRAM {
	KDRV_DIS_IN_IMG_INFO			in_img_info;
	KDRV_DIS_IN_DMA_INFO			in_dma_info;
	KDRV_DIS_OUT_DMA_INFO		    out_dma_info;
	UINT32                          dis_int_en; 
	KDRV_MV_OUT_DMA_INFO            out_mv_info;
} KDRV_DIS_PRAM, *pKDRV_DIS_PRAM;


/**
	DIS KDRV handle structure
*/
#define KDRV_DIS_HANDLE_LOCK	0x00000001

typedef struct _KDRV_DIS_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *p_sem_id;
	UINT32 sts;
	KDRV_DIS_ISRCB isrcb_fp;
} KDRV_DIS_HANDLE;

typedef enum {
	FLG_ID_KDRV_DIS,
	KDRV_DIS_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_DIS_FLAG)
} KDRV_DIS_FLAG;

typedef enum {
	SEMID_KDRV_DIS,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_DIS_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_DIS_SEM)
} KDRV_DIS_SEM;

// DIS
typedef struct _KDRV_DIS_SEM_INFO_ {
	SEM_HANDLE  semphore_id;				///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_DIS_SEM_TABLE;

#if defined(__FREERTOS)
extern ID 		 kdrv_dis_flag_id[KDRV_DIS_FLAG_COUNT];
#else
extern UINT32    kdrv_dis_flag_id[KDRV_DIS_FLAG_COUNT];
#endif
extern KDRV_DIS_SEM_TABLE kdrv_dis_semtbl[KDRV_DIS_SEMAPHORE_COUNT];
extern void kdrv_dis_install_id(void);
extern void kdrv_dis_uninstall_id(void);
extern ID kdrv_dis_get_flag_id(KDRV_DIS_FLAG idx);
extern SEM_HANDLE* kdrv_dis_get_sem_id(KDRV_DIS_SEM idx);
extern void kdrv_dis_init(void);


#endif //_KDRV_DIS_INT_H_

#ifndef _KDRV_MD_INT_H_
#define _KDRV_MD_INT_H_

#include "md_platform.h"
#include "kdrv_md.h"
#include "mdbc_lib.h"

typedef ER (*KDRV_MD_SET_FP)(UINT32, void* );
typedef ER (*KDRV_MD_GET_FP)(UINT32, void* );

/**
	max md kdrv handle number, current max is 8 due to flag config
*/
#define KDRV_MD_HANDLE_MAX_NUM (8)

#define KDRV_MD_LOCK			FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_MD_FMD				FLGPTN_BIT(8)
#define KDRV_MD_TIMEOUT			FLGPTN_BIT(9)
#define KDRV_MD_HDL_UNLOCK		FLGPTN_BIT(10)
#define KDRV_MD_LOCK_BITS		0x000000ff
#define KDRV_MD_INIT_BITS		(KDRV_MD_LOCK_BITS | KDRV_MD_HDL_UNLOCK)

/**
    IPL process ID
*/
typedef enum _KDRV_MD_ID {
	KDRV_MD_ID_0 = 0,                                  ///< MDBC
	KDRV_MD_ID_MAX_NUM,
	ENUM_DUMMY4WORD(KDRV_MD_ID)
} KDRV_MD_ID;

#define KDRV_MD_HANDLE_LOCK	0x00000001

/**
	MD KDRV handle structure
*/
typedef struct _KDRV_MD_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_MD_ISRCB isrcb_fp;
} KDRV_MD_HANDLE;

typedef enum {
	FLG_ID_KDRV_MD,
	KDRV_MD_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_MD_FLAG)
} KDRV_MD_FLAG;

typedef enum {
	SEMID_KDRV_MD,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_MD_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_MD_SEM)
} KDRV_MD_SEM;

// AI
typedef struct _KDRV_MD_SEM_INFO_ {
	SEM_HANDLE  semphore_id;				///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_MD_SEM_TABLE;

#if defined(__FREERTOS)
extern ID 	   kdrv_md_flag_id[KDRV_MD_FLAG_COUNT];
#else
extern UINT32 kdrv_md_flag_id[KDRV_MD_FLAG_COUNT];
#endif
extern KDRV_MD_SEM_TABLE kdrv_md_semtbl[KDRV_MD_SEMAPHORE_COUNT];
extern void kdrv_md_install_id(void);
extern void kdrv_md_uninstall_id(void);
extern ID kdrv_md_get_flag_id(KDRV_MD_FLAG idx);
extern SEM_HANDLE* kdrv_md_get_sem_id(KDRV_MD_SEM idx);
extern void kdrv_md_init(void);

#endif //_KDRV_MD_INT_H_
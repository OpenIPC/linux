#ifndef _NVTMPP_INIT_H
#define _NVTMPP_INIT_H
#include "kwrap/type.h"

#define FBOOT_COMNBLK_MAX_CNT       128


//#define FBOOT_VCAP_COMNBLK_MAX_CNT 10
//#define FBOOT_VPRC_COMNBLK_MAX_CNT 2

//#define FBOOT_VCAP_COMNBLK_MIN_CNT 2
//#define FBOOT_VPRC_COMNBLK_MIN_CNT 2

//#define FBOOT_VPRC_MAX_PATH        4

#define FBOOT_MISC_CPOOL_MAX        3
#define FBOOT_MISC_CPOOLBCK_MAX_CNT 3

typedef enum {
	FBOOT_POOL_VPRC_CTRL         = 0,
	//FBOOT_POOL_VENC_CTRL            ,
	FBOOT_POOL_VENC_0_MAX           ,
	FBOOT_POOL_ACAP_CTRL            ,
	FBOOT_POOL_VCAP_SHDR_BUF        ,
	FBOOT_POOL_VENC_1_MAX           ,
	FBOOT_POOL_VENC_2_MAX           ,
	FBOOT_POOL_VENC_3_MAX           ,
	FBOOT_POOL_VENC_4_MAX		,
	FBOOT_POOL_VENC_5_MAX		,
	FBOOT_POOL_VPRC1_CTRL       ,
	FBOOT_POOL_CNT,
	ENUM_DUMMY4WORD(NVTMPP_FBOOT_PRV_POOL)
} NVTMPP_FBOOT_PRV_POOL;


typedef int (*NVTMPP_LOCK_BLK_CB)(UINT32 blk_addr);
typedef int (*NVTMPP_UNLOCK_BLK_CB)(UINT32 blk_addr);

typedef struct {
	UINT32 va;    				        ///< Memory buffer virtual starting address
	UINT32 pa;    				        ///< Memory buffer physical starting address
	UINT32 size;     					///< Memory buffer size
} NVTMPP_MMZ_DDR_S;

typedef struct {
	NVTMPP_MMZ_DDR_S  ddr_mem[3];       ///< Memory buffer virtual starting address
	UINT32            max_pools_cnt;
} NVTMPP_MMZ_INFO_S;


typedef struct {
	UINT32 addr;    				    ///< Memory buffer virtual starting address
	UINT32 size;     					///< Memory buffer size
	UINT32 ref_cnt;                     ///< block reference count
} NVTMPP_FASTBOOT_BLK_S;

typedef struct {
	UINT32 pool_type;                   ///< The common pool type
	UINT32 blk_cnt;
	NVTMPP_FASTBOOT_BLK_S  blk[FBOOT_MISC_CPOOLBCK_MAX_CNT];
} NVTMPP_FBOOT_MISC_CPOOL_S;

typedef struct {
	const CHAR *pool_name;              ///< The private pool name on nvtmpp
	const CHAR *dts_node;               ///< The dts node name
	BOOL        optional;               ///< The pool is optional or not
} NVTMPP_FBOOT_POOL_DTS_INFO_S;

typedef struct {
	UINT32                 init_tag;
	// common blks
	UINT32                 comn_blk_cnt;
	NVTMPP_FASTBOOT_BLK_S  comn_blk[FBOOT_COMNBLK_MAX_CNT];
	//UINT32                 vcap_blk_cnt;
	//NVTMPP_FASTBOOT_BLK_S  vcap_blk[FBOOT_VCAP_COMNBLK_MAX_CNT];
	//UINT32                 vprc_blk_cnt[FBOOT_VPRC_MAX_PATH];
	//NVTMPP_FASTBOOT_BLK_S  vprc_blk[FBOOT_VPRC_MAX_PATH][FBOOT_VPRC_COMNBLK_MAX_CNT];
	// misc common pool type
	NVTMPP_FBOOT_MISC_CPOOL_S misc_cpool[FBOOT_MISC_CPOOL_MAX];
	// private pools
	NVTMPP_FASTBOOT_BLK_S  pv_pools[FBOOT_POOL_CNT];
} NVTMPP_FASTBOOT_MEM_S;

extern int  nvtmpp_init_mmz(void);
extern void nvtmpp_exit_mmz(void);
extern NVTMPP_MMZ_INFO_S *nvtmpp_get_mmz(void);
extern int  nvtmpp_parse_fastboot_mem_dt(void);
extern NVTMPP_FASTBOOT_MEM_S *nvtmpp_get_fastboot_mem(void);
extern void nvtmpp_reg_fastboot_lock_cb(NVTMPP_LOCK_BLK_CB lock_cb);
extern void nvtmpp_reg_fastboot_unlock_cb(NVTMPP_UNLOCK_BLK_CB unlock_cb);
extern int  nvtmpp_lock_fastboot_blk(UINT32 blk_addr);
extern int  nvtmpp_unlock_fastboot_blk(UINT32 blk_addr);
extern UINT32 nvtmpp_fastboot_spin_lock(void);
extern void   nvtmpp_fastboot_spin_unlock(UINT32 flags);
extern NVTMPP_FBOOT_POOL_DTS_INFO_S *nvtmpp_get_fastboot_pvpool_dts_info(void);
extern UINT32  nvtmpp_sys_pa2va(UINT32 phys_addr);
extern UINT32  nvtmpp_sys_va2pa(UINT32 virt_addr);

// return 0 if get blk fail
extern UINT32  nvtmpp_get_fastboot_blk(UINT32 blk_size);

#endif //_NVTMPP_INIT_H

#ifndef _AUDCAP_BUILTIN_H
#define _AUDCAP_BUILTIN_H

#include "kwrap/type.h"

#define AUDCAP_BUILTIN_BSQ_MAX             75             ///< same value as NMR_VDOTRIG_BSQ_MAX

#define AUDCAP_BUILTIN_LOOPBACK 1

typedef enum {
	BUILTIN_AUDCAP_PARAM_INFO,
	BUILTIN_AUDCAP_PARAM_FIRST_QUEUE,
	BUILTIN_AUDCAP_PARAM_TEMP_QUEUE,
	BUILTIN_AUDCAP_PARAM_ISR_CB,
	BUILTIN_AUDCAP_PARAM_DONE_QUEUE,
	BUILTIN_AUDCAP_PARAM_INIT_DONE,
	BUILTIN_AUDCAP_PARAM_TRIGGER_CB,
#if AUDCAP_BUILTIN_LOOPBACK
	BUILTIN_AUDCAP_PARAM_FIRST_QUEUE_TXLB,
	BUILTIN_AUDCAP_PARAM_TEMP_QUEUE_TXLB,
	BUILTIN_AUDCAP_PARAM_DONE_QUEUE_TXLB,
#endif
	BUILTIN_AUDCAP_PARAM_MAX,
} BUILTIN_AUDCAP_PARAM;

typedef struct {
	/* acap info */
	UINT32 aud_sr;                ///< Sample Rate
	UINT32 aud_ch;                ///< Channel
	UINT32 aud_bitpersample;      ///< bit Per Sample
	UINT32 aud_ch_num;			  ///< Channel number
	UINT32 aud_vol;
	UINT32 aud_que_cnt;
	UINT32 aud_buf_sample_cnt;    ///< buffer sample count
	/* acap internal buffer info */
	UINT32 ctrl_blk_addr;
	UINT32 ctrl_blk_size;
} AUDCAP_BUILTIN_INIT_INFO;

typedef struct {
	UINT32 bs_start_addr;                               ///< [w] output bit-stream start address if enc space not enough
	UINT32 bs_addr_1;                                   ///< [w] 1st output bit-stream address
	UINT32 bs_size_1;                                   ///< [w] 1st output bit-stream size
	UINT32 bs_end_addr;                                 ///< [w] output bit-stream end address
	UINT32 interrupt;
	UINT32 timestamp;
} AUDCAP_BUILTIN_PARAM;

typedef struct {
	UINT32 addr;    							//BS buffer starting address
	UINT32 size;    							//BS buffer size
	UINT32 timestamp;
} AUDCAP_BUILTIN_BS_INFO, *PAUDCAP_BUILTIN_BS_INFO;

typedef struct {
	UINT32                      Front;                  ///< Front pointer
	UINT32                      Rear;                   ///< Rear pointer
	UINT32                      bFull;                  ///< Full flag
	AUDCAP_BUILTIN_BS_INFO      Queue[AUDCAP_BUILTIN_BSQ_MAX];
} AUDCAP_BUILTIN_BSQ, *PAUDCAP_BUILTIN_BSQ;

typedef struct {
	AUDCAP_BUILTIN_BSQ          bsQueue;
	AUDCAP_BUILTIN_PARAM        acap_param;
} AUDCAP_BUILTIN_OBJ, *PAUDCAP_BUILTIN_OBJ;

typedef void (*AUDCAP_BUILTIN_TRIGGER_CB)(UINT32);

int audcap_builtin_start(void);
int audcap_builtin_stop(void);
void audcap_builtin_install_id(void);
void audcap_builtin_uninstall_id(void);
int  audcap_builtin_init(AUDCAP_BUILTIN_INIT_INFO *p_info);

extern UINT32 audcap_builtin_get_param(UINT32 param, UINT32 *value);
extern UINT32 audcap_builtin_set_param(UINT32 param, UINT32 *value);
#endif //_AUDCAP_BUILTIN_H
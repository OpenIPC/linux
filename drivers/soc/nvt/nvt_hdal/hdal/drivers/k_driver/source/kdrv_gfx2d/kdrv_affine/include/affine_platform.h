#ifndef __MODULE_PLATFORM_H__
#define __MODULE_PLATFORM_H__

//#include "affine.h"
#if (defined __UITRON || defined __ECOS)
#include "frammap/frammap_if.h"

#define AFFINE_BOTTOMHALF_KTHREAD	(0)
#define AFFINE_BOTTOMHALF_TASKLET	(1)
#define AFFINE_BOTTOMHALF_SEL		(AFFINE_BOTTOMHALF_TASKLET)

extern UINT8 grph_ll_buffer[AFFINE_LL_BUF_SIZE];

#elif defined(__FREERTOS)
#include "kwrap/type.h"
#include "affine_compatible.h"
#include "io_address.h"
#define EXPORT_SYMBOL(x)
			
#define FLGPTN_AFFINE  0x01

#define AFFINE_BOTTOMHALF_KTHREAD	(0)
#define AFFINE_BOTTOMHALF_TASKLET	(1)
#define AFFINE_BOTTOMHALF_SEL		(AFFINE_BOTTOMHALF_KTHREAD)


//extern UINT32 IOADDR_AFFINE_REG_BASE;
extern UINT8* affine_ll_buffer;

#else
#include "kwrap/type.h"
#include "affine_compatible.h"
//#include "affine_drv.h"
#define _EMULATION_             (0)	// no emu macro in linux
					// manually defined here
			
#define FLGPTN_AFFINE  0x01

#define AFFINE_BOTTOMHALF_KTHREAD	(0)
#define AFFINE_BOTTOMHALF_TASKLET	(1)
#define AFFINE_BOTTOMHALF_SEL		(AFFINE_BOTTOMHALF_TASKLET)


extern UINT32 IOADDR_AFFINE_REG_BASE;
extern UINT8* affine_ll_buffer;
#endif

extern void affine_platform_clk_enable(AFFINE_ID id);
extern void affine_platform_clk_disable(AFFINE_ID id);
extern void affine_platform_clk_set_freq(AFFINE_ID id, UINT32 freq);
extern void affine_platform_clk_get_freq(AFFINE_ID id, UINT32 *p_freq);
extern void affine_platform_flg_clear(AFFINE_ID id, FLGPTN flg);
extern void affine_platform_flg_set(AFFINE_ID id, FLGPTN flg);
extern void affine_platform_flg_wait(AFFINE_ID id, FLGPTN flg);
extern ER affine_platform_sem_wait(AFFINE_ID id);
extern ER affine_platform_sem_signal(AFFINE_ID id);
extern ER affine_platform_oc_sem_wait(AFFINE_ID id);
extern ER affine_platform_oc_sem_signal(AFFINE_ID id);
extern UINT32 affine_platform_spin_lock(AFFINE_ID id);
extern void affine_platform_spin_unlock(AFFINE_ID id, UINT32 flags);
extern void affine_platform_sram_enable(AFFINE_ID id);
extern void affine_platform_int_enable(AFFINE_ID id);
extern void affine_platform_int_disable(AFFINE_ID id);
extern UINT32 affine_platform_dma_is_cacheable(UINT32 addr);
extern UINT32 affine_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size);
extern UINT32 affine_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 affine_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
extern BOOL affine_platform_is_valid_va(UINT32 addr);
extern UINT32 affine_platform_va2pa(UINT32 addr);
#if !(defined __UITRON || defined __ECOS)
extern void affine_isr(void);
#endif

extern BOOL affine_platform_list_empty(AFFINE_ID id);
extern ER affine_platform_add_list(AFFINE_ID id,
				KDRV_AFFINE_TRIGGER_PARAM *p_param,
                        	KDRV_CALLBACK_FUNC *p_callback);
extern struct _AFFINE_REQ_LIST_NODE* affine_platform_get_head(AFFINE_ID id);
extern ER affine_platform_del_list(AFFINE_ID id);

extern void affine_platform_set_ist_event(AFFINE_ID id);
#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_TASKLET)
extern int  affine_platform_ist(AFFINE_ID id, UINT32 event);
#else
extern void  affine_platform_ist(void);
#endif

#if defined __FREERTOS
extern void affine_platform_init(void);
#else
//extern void affine_platform_init(MODULE_INFO *pmodule_info);
#endif
extern void affine_platform_uninit(void);

#endif

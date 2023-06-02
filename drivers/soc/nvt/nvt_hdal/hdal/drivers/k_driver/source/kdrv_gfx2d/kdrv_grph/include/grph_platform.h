#ifndef __MODULE_PLATFORM_H_
#define __MODULE_PLATFORM_H__
//#include "graphic_main.h"

//#include "grph.h"
#if (defined __UITRON || defined __ECOS)
//#include "grph.h"
#include "frammap/frammap_if.h"

extern UINT8 grph_ll_buffer[GRPH_ID_2 + 1][GRPH_LL_BUF_SIZE];

#else

#if defined(__FREERTOS)
//#define ARRAY_SIZE(X)	(sizeof((X)) / sizeof((X[0])))
//#define EXPORT_SYMBOL(x)

#else
//#include <linux/slab.h>
//#include "graphic_drv.h"
#endif

#include "kwrap/type.h"
#include "comm/hwclock.h"
#include "grph_compatible.h"
#include <plat/nvt-sramctl.h>
#define _EMULATION_             (0)	// no emu macro in linux
					// manually defined here

#define FLGPTN_GRAPHIC  0x01
#define FLGPTN_GRAPHIC2 0x02

#define IST_EVENT_NORMAL	0
#define IST_EVENT_ABORT		1

#define ACC_BUF_UNIT    32          // unit size of acc buffer (count by byte)

#if defined(__FREERTOS)
#else
extern UINT32 IOADDR_GRAPHIC_REG_BASE;
extern UINT32 IOADDR_GRAPHIC2_REG_BASE;
#endif

extern UINT8* grph_ll_buffer[GRPH_ID_2 + 1];
extern UINT8 *grph_acc_buffer;
#endif
//#include "grph_int.h"

extern KDRV_CALLBACK_FUNC v_grph_callback[GRPH_ID_2+1];

extern void grph_platform_clk_enable(GRPH_ID id);
extern void grph_platform_clk_disable(GRPH_ID id);
extern void grph_platform_clk_set_freq(GRPH_ID id, UINT32 freq);
extern void grph_platform_clk_get_freq(GRPH_ID id, UINT32 *p_freq);
extern void grph_platform_flg_clear(GRPH_ID id, FLGPTN flg);
extern void grph_platform_flg_set(GRPH_ID id, FLGPTN flg);
extern void grph_platform_flg_wait(GRPH_ID id, FLGPTN flg);
extern ER grph_platform_sem_wait(GRPH_ID id);
extern ER grph_platform_sem_signal(GRPH_ID id);
extern ER grph_platform_oc_sem_wait(GRPH_ID id);
extern ER grph_platform_oc_sem_signal(GRPH_ID id);
extern UINT32 grph_platform_spin_lock(GRPH_ID id);
extern void grph_platform_spin_unlock(GRPH_ID id, UINT32 flags);
extern void grph_platform_sram_enable(GRPH_ID id);
extern void grph_platform_int_enable(GRPH_ID id);
extern void grph_platform_int_disable(GRPH_ID id);
extern UINT32 grph_platform_dma_is_cacheable(UINT32 addr);
extern UINT32 grph_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size);
extern UINT32 grph_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 grph_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 grph_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
extern BOOL grph_platform_is_valid_va(UINT32 addr);
extern UINT32 grph_platform_va2pa(UINT32 addr);
#if !(defined __UITRON || defined __ECOS)
//extern int DBG_ERR(const char *fmt, ...);
//extern int DBG_WRN(const char *fmt, ...);
//extern int DBG_IND(const char *fmt, ...);
extern void graph_isr(void);
extern void graph2_isr(void);
#endif

extern BOOL grph_platform_list_empty(GRPH_ID id);
extern ER grph_platform_add_list(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param,
                        KDRV_CALLBACK_FUNC *p_callback);
extern struct _GRPH_REQ_LIST_NODE* grph_platform_get_head(GRPH_ID id);
//extern ER grph_platform_get_list(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param);
extern ER grph_platform_del_list(GRPH_ID id);
extern void grph_jobs_init(void);
//extern ER grph_platform_del_list(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param);

extern void grph_platform_set_ist_event(GRPH_ID id, UINT32 events);
#if defined(__FREERTOS)
extern void  grph_platform_ist(void);
#else
extern int  grph_platform_ist(GRPH_ID id, UINT32 event);
#endif

#if defined(__FREERTOS)
extern void grph_platform_init(void);
#else
//extern void grph_platform_init(MODULE_INFO *pmodule_info);
#endif
extern void grph_platform_uninit(void);

#endif

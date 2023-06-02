#ifndef __MODULE_PLATFORM_H_
#define __MODULE_PLATFORM_H__



#if (defined __UITRON || defined __ECOS)
#include  "Type.h"
#include "dma_protected.h"

#define JPGDBG_ERR  DBG_ERR
#define JPGDBG_WRN  DBG_WRN
#define JPGDBG_IND  DBG_IND

#elif defined __KERNEL__
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include <linux/clk.h>

#define JPGDBG_ERR(fmtstr, args...) nvt_dbg(ERR, fmtstr, ##args)
#define JPGDBG_WRN(fmtstr, args...) nvt_dbg(WARN, fmtstr, ##args)
#define JPGDBG_IND(fmtstr, args...) nvt_dbg(INFO, fmtstr, ##args)

extern UINT32 IOADDR_JPEG_REG_BASE;
#else
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "io_address.h"
#include "pll_protected.h"
#include "dma_protected.h"

#include "jpeg_dbg.h"

#define JPGDBG_ERR  DBG_ERR
#define JPGDBG_WRN  DBG_WRN
#define JPGDBG_IND  DBG_IND
#endif

#define JPEG_BRC2     1

//VBR control
#define VBR_NORAMAL   0
#define VBR_TO_CBR    1
//BRC def
#define  DEFAULT_BRC_QF  500

#define  IMAGE_RETRY_COUNTS    4
//VBR mode
#define  DEFAULT_BRC_VBR_QUALITY       256

#define JPEG_SETREG(ofs, value)  OUTW(IOADDR_JPEG_REG_BASE+(ofs), (value))
#define JPEG_GETREG(ofs)        INW(IOADDR_JPEG_REG_BASE+(ofs))

extern UINT32 jpeg_platform_dma_is_cacheable(UINT32 addr);
extern BOOL jpeg_platform_dma_flush_dev2mem(BOOL decmode, UINT32 addr, UINT32 size);
extern BOOL jpeg_platform_dma_flush_mem2dev(BOOL decmode, UINT32 addr, UINT32 size);
extern BOOL jpeg_platform_dma_flush_dev2mem_width_neq_loff(BOOL decmode, UINT32 addr, UINT32 size);
extern UINT32 jpeg_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size);


extern int jpeg_platform_ist(UINT32 event);
extern void jpeg_isr_bottom(UINT32 events);


extern void jpeg_create_resource(void);
extern void jpeg_release_resource(void);


extern void jpeg_platform_clear_flg(void);
extern void jpeg_platform_set_flg(void);
extern BOOL jpeg_platform_check_flg(void);
extern void jpeg_platform_wait_flg(void);

extern ER jpeg_platform_sem_wait(void);
extern ER jpeg_platform_sem_signal(void);

extern void jpeg_platform_clk_get_freq(UINT32 *p_freq);
extern void jpeg_platform_clk_set_freq(UINT32 freq);

extern void jpeg_platform_triginfo_init(void);
extern UINT32 jpeg_platform_spin_lock(void);
extern void jpeg_platform_spin_unlock(UINT32 flags);

#endif

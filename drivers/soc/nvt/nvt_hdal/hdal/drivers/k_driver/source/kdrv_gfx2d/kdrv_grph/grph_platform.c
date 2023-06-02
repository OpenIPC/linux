
//#include "grph.h"
#include "kwrap/type.h"
//#include "grph_compatible.h"
#include "grph_platform.h"
#include "grph_int.h"

#if defined(__FREERTOS)
#define ARRAY_SIZE(X)   (sizeof((X)) / sizeof((X[0])))
#define EXPORT_SYMBOL(x)

#else
#include <linux/slab.h>
#include "graphic_drv.h"
#endif


#if defined __UITRON || defined __ECOS

#include "interrupt.h"
#include "top.h"
#include "dma.h"


static const DRV_INT_NUM v_inten[] = {DRV_INT_GRAPHIC, DRV_INT_GRAPHIC2};
static UINT32 v_clksel[] = {PLL_CLKSEL_GRAPHIC_80, PLL_CLKSEL_GRAPHIC2_80};
static const ID v_flg_id[] = {FLG_ID_GRAPHIC, FLG_ID_GRAPHIC2};
static const DRV_SEM v_sem[] = {SEMID_GRAPHIC, SEMID_GRAPHIC2};
static const CG_EN v_clken[] = {GRAPH_CLK, GRAPH2_CLK};

UINT8 grph_ll_buffer[GRPH_ID_2 + 1][GRPH_LL_BUF_SIZE];
UINT8 grph_acc_buffer[GRPH_LL_MAX_COUNT][32];   // only GRPH_ID_1 support ACC

#elif defined(__FREERTOS)

#include "kwrap/type.h"//a header for basic variable type
#include "kwrap/semaphore.h"
#include "kwrap/spinlock.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "kwrap/task.h"
#include "pll.h"
#include "pll_protected.h"
#include "interrupt.h"
#include <stdlib.h>

static ID v_flg_id[GRPH_ID_2+1];

static SEM_HANDLE *v_sem[GRPH_ID_2+1];
static SEM_HANDLE *v_sem_oc[GRPH_ID_2+1];

static SEM_HANDLE SEMID_GRAPHIC;
static SEM_HANDLE SEMID_GRAPHIC2;

static SEM_HANDLE SEMID_GRAPHIC_OPEN_CLOSE;
static SEM_HANDLE SEMID_GRAPHIC2_OPEN_CLOSE;

static ID		FLG_ID_GRAPHIC;
static ID		FLG_ID_GRPH_IST;
static const FLGPTN	v_flag_ptn[GRPH_ID_2+1] = {FLGPTN_BIT(0), FLGPTN_BIT(1)};

static vk_spinlock_t	v_grph_spin_locks[GRPH_ID_2+1];

static THREAD_HANDLE	grph_ist_tsk_id          = 0;
//static struct tasklet_struct * v_p_timer_tasklet[GRPH_ID_2+1];

static const CG_EN v_clken[] = {GRAPH_CLK, GRAPH2_CLK};
static UINT32 v_clksel[] = {PLL_CLKSEL_GRAPHIC_240, PLL_CLKSEL_GRAPHIC2_240};
//static struct clk *p_grph_clk[GRPH_ID_2+1];

//static UINT32 v_ist_events[GRPH_ID_2 + 1];

UINT8* grph_ll_buffer[GRPH_ID_2 + 1];
UINT8 *grph_acc_buffer;


#else

#include <linux/spinlock.h>
#include "kwrap/type.h"//a header for basic variable type
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include  <mach/fmem.h>


UINT32 IOADDR_GRAPHIC_REG_BASE;
UINT32 IOADDR_GRAPHIC2_REG_BASE;

static ID v_flg_id[GRPH_ID_2+1];

static SEM_HANDLE *v_sem[GRPH_ID_2+1];
static SEM_HANDLE *v_sem_oc[GRPH_ID_2+1];

static SEM_HANDLE SEMID_GRAPHIC;
static SEM_HANDLE SEMID_GRAPHIC2;

static SEM_HANDLE SEMID_GRAPHIC_OPEN_CLOSE;
static SEM_HANDLE SEMID_GRAPHIC2_OPEN_CLOSE;

static FLGPTN     FLG_ID_GRAPHIC;
//static FLGPTN     FLG_ID_GRAPHIC2;

static spinlock_t v_grph_spin_locks[GRPH_ID_2+1];

static struct tasklet_struct * v_p_timer_tasklet[GRPH_ID_2+1];

static struct clk *p_grph_clk[GRPH_ID_2+1];

UINT8* grph_ll_buffer[GRPH_ID_2 + 1];
UINT8 *grph_acc_buffer;

#endif

static UINT32 v_grph_ist_event[GRPH_ID_2+1];

KDRV_CALLBACK_FUNC v_grph_callback[GRPH_ID_2+1];

unsigned int kdrv_grph_debug_level = NVT_DBG_WRN;

void grph_platform_clk_enable(GRPH_ID id)
{
#if defined __UITRON || defined __ECOS
	pll_enableClock(v_clken[id]);
#elif defined(__FREERTOS)
	pll_enableClock(v_clken[id]);
#else
	if (IS_ERR(p_grph_clk[id])) {
		printk("%s: get clk fail 0x%p\r\n", __func__, p_grph_clk[id]);
	} else {
		clk_enable(p_grph_clk[id]);
	}
#endif
}

void grph_platform_clk_disable(GRPH_ID id)
{
#if defined __UITRON || defined __ECOS
	pll_disableClock(v_clken[id]);
#elif defined(__FREERTOS)
	pll_disableClock(v_clken[id]);
#else
	if (IS_ERR(p_grph_clk[id])) {
		printk("%s: get clk fail 0x%p\r\n", __func__, p_grph_clk[id]);
	} else {
		clk_disable(p_grph_clk[id]);
	}
#endif
}

void grph_platform_clk_set_freq(GRPH_ID id, UINT32 freq)
{
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)

#if defined(_BSP_NA51089_)
	const UINT32 v_clk_rate[2][4] = {
		{
			PLL_CLKSEL_GRAPHIC_240,     PLL_CLKSEL_GRAPHIC_320,
			PLL_CLKSEL_GRAPHIC_480,     PLL_CLKSEL_GRAPHIC_480
		},
		{
			PLL_CLKSEL_GRAPHIC2_240,    PLL_CLKSEL_GRAPHIC2_320,
			PLL_CLKSEL_GRAPHIC2_480,    PLL_CLKSEL_GRAPHIC2_480
		}
	};

	if (freq < 240) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", id, (unsigned int)freq);
		freq = 0;
	} else if (freq == 240) {
		freq = 0;
	} else if (freq < 320) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", id, (unsigned int)freq);
		freq = 0;
	} else if (freq == 320) {
		freq = 1;
	} else if (freq < 480) {
		DBG_WRN("%d: input frequency %d round to 320MHz\r\n", id, (unsigned int)freq);
		freq = 1;
	} else if (freq == 480) {
		freq = 3;
	} else {
		DBG_WRN("%d: input frequency %d round to 480MHz\r\n", id, (unsigned int)freq);
		freq = 3;
	}

	v_clksel[id] = v_clk_rate[id][freq];
	if (id == GRPH_ID_1)
		pll_set_clock_rate(PLL_CLKSEL_GRAPHIC, v_clk_rate[id][freq]);
	else
		pll_set_clock_rate(PLL_CLKSEL_GRAPHIC2, v_clk_rate[id][freq]);
#else	// else of #if defined(_BSP_NA51089_)
	// NA51000
	const UINT32 v_clk_rate[2][4] = {
		{
			PLL_CLKSEL_GRAPHIC_240,     PLL_CLKSEL_GRAPHIC_PLL6,
			PLL_CLKSEL_GRAPHIC_PLL13,   PLL_CLKSEL_GRAPHIC_480
		},
		{
			PLL_CLKSEL_GRAPHIC2_240,    PLL_CLKSEL_GRAPHIC2_PLL6,
			PLL_CLKSEL_GRAPHIC2_PLL13,  PLL_CLKSEL_GRAPHIC2_480
		}
	};

	if (freq < 240) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", id, (unsigned int)freq);
		freq = 0;
	} else if (freq == 240) {
		freq = 0;
	} else if (freq < 380) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", id, (unsigned int)freq);
		freq = 0;
	} else if (freq == 380) {
		freq = 1;
		if (pll_get_pll_enable(PLL_ID_6) == FALSE) {
			pll_set_pll_enable(PLL_ID_6, TRUE);
		}
	} else if (freq < 420) {
		DBG_WRN("%d: input frequency %d round to 380MHz(PLL6)\r\n", id, (unsigned int)freq);
		freq = 1;
		if (pll_get_pll_enable(PLL_ID_6) == FALSE) {
			pll_set_pll_enable(PLL_ID_6, TRUE);
		}
	} else if (freq == 420) {
		freq = 2;
		if (pll_get_pll_enable(PLL_ID_13) == FALSE) {
			pll_set_pll_enable(PLL_ID_13, TRUE);
		}
	} else if (freq < 480) {
		DBG_WRN("%d: input frequency %d round to 420MHz(PLL13)\r\n", id, (unsigned int)freq);
		freq = 2;
		if (pll_get_pll_enable(PLL_ID_13) == FALSE) {
			pll_set_pll_enable(PLL_ID_13, TRUE);
		}
	} else if (freq == 480) {
		freq = 3;
	} else {
		DBG_WRN("%d: input frequency %d round to 480MHz\r\n", id, (unsigned int)freq);
		freq = 3;
	}

	v_clksel[id] = v_clk_rate[id][freq];
#endif

#else
	struct clk *parent_clk;

	if (IS_ERR(p_grph_clk[id])) {
		printk("%s: get clk fail 0x%p\r\n", __func__, p_grph_clk[id]);
		return;
	}

	if (freq < 240) {
		printk("%d: input frequency %d round to 240MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq == 240) {
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq < 320) {
		printk("%d: input frequency %d round to 240MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq == 320) {
		parent_clk = clk_get(NULL, "pllf320");
	} else if (freq < 480) {
		printk("%d: input frequency %d round to 320MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "pllf320");
	} else if (freq == 480) {
		parent_clk = clk_get(NULL, "fix480m");
	} else {
		printk("%d: input frequency %d round to 480MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "fix480m");
	}

	clk_set_parent(p_grph_clk[id], parent_clk);
	clk_put(parent_clk);
#endif
}

void grph_platform_clk_get_freq(GRPH_ID id, UINT32 *p_freq)
{
#if defined __UITRON || defined __ECOS
	if (id == GRPH_ID_1) {
		switch (pll_getClockRate(PLL_CLKSEL_GRAPHIC)) {
		case PLL_CLKSEL_GRAPHIC_120:
			*p_freq = 120;
			break;
		case PLL_CLKSEL_GRAPHIC_160:
			*p_freq = 160;
			break;
		case PLL_CLKSEL_GRAPHIC_192:
			*p_freq = 192;
			break;
		case PLL_CLKSEL_GRAPHIC_240:
		default:
			*p_freq = 240;
			break;
		}
	} else {
		switch (pll_getClockRate(PLL_CLKSEL_GRAPHIC2)) {
		case PLL_CLKSEL_GRAPHIC2_120:
			*p_freq = 120;
			break;
		case PLL_CLKSEL_GRAPHIC2_160:
			*p_freq = 160;
			break;
		case PLL_CLKSEL_GRAPHIC2_192:
			*p_freq = 192;
			break;
		case PLL_CLKSEL_GRAPHIC2_240:
		default:
			*p_freq = 240;
			break;
		}
	}

#elif defined(__FREERTOS)

#if defined(_BSP_NA51089_)
	if (id == GRPH_ID_1) {
		switch (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC)) {
		case PLL_CLKSEL_GRAPHIC_240:
		default:
			*p_freq = 240;
			break;
		case PLL_CLKSEL_GRAPHIC_320:
			*p_freq = 320;
			break;
		case PLL_CLKSEL_GRAPHIC_480:
			*p_freq = 480;
			break;
		}
	} else {
		switch (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC2)) {
		case PLL_CLKSEL_GRAPHIC2_240:
		default:
			*p_freq = 240;
			break;
		case PLL_CLKSEL_GRAPHIC2_320:
			*p_freq = 320;
			break;
		case PLL_CLKSEL_GRAPHIC2_480:
			*p_freq = 480;
			break;
		}
	}
#else	// else of #if defined(_BSP_NA51089_)
	// NA51000
	if (id == GRPH_ID_1) {
		switch (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC)) {
		case PLL_CLKSEL_GRAPHIC_240:
		default:
			*p_freq = 240;
			break;
		case PLL_CLKSEL_GRAPHIC_PLL6:
			*p_freq = pll_get_pll_freq(PLL_ID_6) / 1000000;;
			break;
		case PLL_CLKSEL_GRAPHIC_PLL13:
			*p_freq = pll_get_pll_freq(PLL_ID_13) / 1000000;
			break;
		case PLL_CLKSEL_GRAPHIC_480:
			*p_freq = 480;
			break;
		}
	} else {
		switch (pll_get_clock_rate(PLL_CLKSEL_GRAPHIC2)) {
		case PLL_CLKSEL_GRAPHIC2_240:
		default:
			*p_freq = 240;
			break;
		case PLL_CLKSEL_GRAPHIC2_PLL6:
			*p_freq = pll_get_pll_freq(PLL_ID_6) / 1000000;
			break;
		case PLL_CLKSEL_GRAPHIC2_PLL13:
			*p_freq = pll_get_pll_freq(PLL_ID_13) / 1000000;
			break;
		case PLL_CLKSEL_GRAPHIC2_480:
			*p_freq = 480;
			break;
		}
	}
#endif

#else
	if (IS_ERR(p_grph_clk[id])) {
		printk("%s: get clk fail 0x%p\r\n", __func__, p_grph_clk[id]);
		return;
	}
	*p_freq = clk_get_rate(p_grph_clk[id]);
#endif
}

void grph_platform_flg_clear(GRPH_ID id, FLGPTN flg)
{
	clr_flg(v_flg_id[id], flg);
}

void grph_platform_flg_set(GRPH_ID id, FLGPTN flg)
{
	 iset_flg(v_flg_id[id], flg);
}

void grph_platform_flg_wait(GRPH_ID id, FLGPTN flg)
{
	FLGPTN              ptn;

//	printk("%s: flg id %d\r\n", __func__, v_flg_id[id]);
	wai_flg(&ptn, v_flg_id[id], flg, TWF_ORW | TWF_CLR);
//	printk("%s: done\r\n", __func__);
}

ER grph_platform_sem_wait(GRPH_ID id)
{
	if (id > GRPH_ID_2)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return wai_sem(v_sem[id]);
#else
	return SEM_WAIT(*v_sem[id]);
#endif
}

ER grph_platform_oc_sem_wait(GRPH_ID id)
{
	if (id > GRPH_ID_2)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return wai_sem(v_sem[id]);
#else
	return SEM_WAIT(*v_sem_oc[id]);
#endif
}

ER grph_platform_oc_sem_signal(GRPH_ID id)
{
	if (id > GRPH_ID_2)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[id]);
#else
//	printk("%s: id %d, sem 0x%p\r\n", __func__, id, v_sem[id]);
	SEM_SIGNAL(*v_sem_oc[id]);
	return E_OK;
#endif
}

ER grph_platform_sem_signal(GRPH_ID id)
{
	if (id > GRPH_ID_2)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[id]);
#else
//	printk("%s: id %d, sem 0x%p\r\n", __func__, id, v_sem[id]);
	SEM_SIGNAL(*v_sem[id]);
	return E_OK;
#endif
}

UINT32 grph_platform_spin_lock(GRPH_ID id)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
#elif defined(__FREERTOS)
	unsigned long flags;
	vk_spin_lock_irqsave(&v_grph_spin_locks[id], flags);
	return flags;
#else
	unsigned long flags;
	spin_lock_irqsave(&v_grph_spin_locks[id], flags);
	return flags;
#endif
}

void grph_platform_spin_unlock(GRPH_ID id, UINT32 flags)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#elif defined(__FREERTOS)
	vk_spin_unlock_irqrestore(&v_grph_spin_locks[id], flags);
#else
	spin_unlock_irqrestore(&v_grph_spin_locks[id], flags);
#endif
}

void grph_platform_sram_enable(GRPH_ID id)
{
#if defined __UITRON || defined __ECOS
	if (id == GRPH_ID_1)
		pll_disableSramShutDown(GRAPH_RSTN);
	else
		pll_disableSramShutDown(GRAPH2_RSTN);
#else
#if defined(_BSP_NA51089_)
    if (id == GRPH_ID_1)
        nvt_disable_sram_shutdown(GRAPH_SD);
    else
        nvt_disable_sram_shutdown(GRAPH2_SD);
#endif
#endif
}

void grph_platform_int_enable(GRPH_ID id)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(v_inten[id]);
#else
#endif
}

void grph_platform_int_disable(GRPH_ID id)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(v_inten[id]);
#else
#endif
}

UINT32 grph_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}

UINT32 grph_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(addr, size);
#elif defined(__FREERTOS)
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_BIDIRECTIONAL);
	return 0;
#else
/*
	if (size > 256*1024)
	{
		flush_cache_all();
		return 1;
	}
	else
*/
	{
		vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_BIDIRECTIONAL_NON_ALIGN);
		return 0;
	}
#endif
}

UINT32 grph_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWithoutCheck(addr, size);

#elif defined(__FREERTOS)
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE_NON_ALIGN);
	return 0;

#else
	UINT32 ret;

#if 0
	if (size > 256*1024) {
		flush_cache_all();
		ret = 1;
	} else
#endif
	{
		vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE_NON_ALIGN);
		ret = 0;
	}

	return ret;
#endif
}

UINT32 grph_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWithoutCheck(addr, size);

#elif defined(__FREERTOS)
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE_NON_ALIGN);
	return 0;

#else
	UINT32 ret;

	{
		vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE_NON_ALIGN);
		ret = 0;
	}

	return ret;
#endif
}

UINT32 grph_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	UINT32 ret;

	ret = dma_flushWriteCacheWithoutCheck(addr, size);
	wmb();

	return ret

#elif defined(__FREERTOS)

	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_TO_DEVICE_NON_ALIGN);
	return 0;

#else
	UINT32 ret;

#if 0
	if (size > 256*1024) {
		flush_cache_all();
		ret = 1;
	} else
#endif
	{
		vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_TO_DEVICE_NON_ALIGN);
		ret = 0;
	}

	return ret;
#endif
}

BOOL grph_platform_is_valid_va(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
#define IS_DRAM_ADDR(x)      (((x) < 0xC0000000UL)?1:0)
	return IS_DRAM_ADDR(addr);
#elif defined(__FREERTOS)
    #define IS_DRAM_ADDR(x)      (((x) < 0xC0000000UL)?1:0)
	return IS_DRAM_ADDR(addr);
#else
	return TRUE;	// assume user always fed valid va
#endif
}

UINT32 grph_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_getPhyAddr(addr);
#elif defined(__FREERTOS)
	return vos_cpu_get_phy_addr(addr);
#else
	return fmem_lookup_pa(addr);
#endif
}

void grph_platform_set_ist_event(GRPH_ID id, UINT32 events)
{
	v_grph_ist_event[id] |= events;
#if defined(__FREERTOS)
	iset_flg(FLG_ID_GRPH_IST, v_flag_ptn[id]);
#else
	/*  Tasklet for bottom half mechanism */
        tasklet_schedule(v_p_timer_tasklet[id]);
#endif
}

#if defined(__FREERTOS)
void grph_platform_ist(void)
{
	THREAD_ENTRY();

	// coverity[loop_top]
	while (1) {
		FLGPTN ptn = 0;
		UINT32 flg = v_flag_ptn[0]|v_flag_ptn[1];
		UINT32 ist_events;
		UINT32 spin_flags;

		vos_flag_wait(&ptn, FLG_ID_GRPH_IST, flg, TWF_ORW | TWF_CLR);

		if (ptn & v_flag_ptn[0]) {
			spin_flags = grph_platform_spin_lock(GRPH_ID_1);
			ist_events = v_grph_ist_event[GRPH_ID_1];
			v_grph_ist_event[GRPH_ID_1] = 0;
			grph_platform_spin_unlock(GRPH_ID_1, spin_flags);

			grph_isr_bottom(GRPH_ID_1, ist_events);
		}
		if (ptn & v_flag_ptn[1]) {
			spin_flags = grph_platform_spin_lock(GRPH_ID_2);
			ist_events = v_grph_ist_event[GRPH_ID_2];
			v_grph_ist_event[GRPH_ID_2] = 0;
			grph_platform_spin_unlock(GRPH_ID_2, spin_flags);

			grph_isr_bottom(GRPH_ID_2, ist_events);
		}
	}

	vos_task_destroy(grph_ist_tsk_id);
}
#else
int grph_platform_ist(GRPH_ID id, UINT32 event)
{
	UINT32 ist_events;
	UINT32 spin_flags;

	spin_flags = grph_platform_spin_lock(id);
	ist_events = v_grph_ist_event[id];
	v_grph_ist_event[id] = 0;
	grph_platform_spin_unlock(id, spin_flags);

	grph_isr_bottom(id, ist_events);

	return 0;
}
#endif

#if 0
void grph_platform_isr_bottom(GRPH_ID id, UINT32 events)
{
	GRPH_REQ_LIST_NODE *p_node;

	p_node = grph_platform_get_head(id);
	if (p_node == NULL) {
		printk("%s: get head fail\r\n", __func__);
		return;
	}

	if (p_node->callback.callback == NULL) {
		grph_platform_flg_set(id, v_isr_flag[id]);
	} else {
		p_node->cb_info.timestamp = jiffies;
		p_node->cb_info.acc_result;
		p_node->callback.callback(&p_node->cb_info, NULL);
	}

	grph_platform_del_list(id);

	if (grph_platform_list_empty(id) == FALSE) {
		// Not empty => process next node

#if 1		// debug
		GRPH_REQ_LIST_NODE *p_node = NULL;

		p_node = grph_platform_get_head(id);
		if (p_node->callback.callback != NULL) {
			printk("%s: callback not NULL\r\n", __func__);
		}
#endif

		graph_trigger(id);
	}
}
#endif

#if defined(__FREERTOS)
void grph_platform_init(void)
{
	OS_CONFIG_FLAG(FLG_ID_GRAPHIC);
	OS_CONFIG_FLAG(FLG_ID_GRPH_IST);
	SEM_CREATE(SEMID_GRAPHIC, 1);
	SEM_CREATE(SEMID_GRAPHIC2, 1);
	SEM_CREATE(SEMID_GRAPHIC_OPEN_CLOSE, 1);
	SEM_CREATE(SEMID_GRAPHIC2_OPEN_CLOSE, 1);
	v_sem[0] = &SEMID_GRAPHIC;
	v_sem[1] = &SEMID_GRAPHIC2;
	v_sem_oc[0] = &SEMID_GRAPHIC_OPEN_CLOSE;
	v_sem_oc[1] = &SEMID_GRAPHIC2_OPEN_CLOSE;
	v_flg_id[0] = FLG_ID_GRAPHIC;
	v_flg_id[1] = FLG_ID_GRAPHIC;
	grph_ist_tsk_id = vos_task_create(grph_platform_ist,  0, "GrphIst",   5,  4096);
	if (grph_ist_tsk_id == 0) {
		DBG_ERR("%s: create grph ist fail\r\n", __func__);
	} else {
		vos_task_resume(grph_ist_tsk_id);
	}
	grph_ll_buffer[0] = malloc(GRPH_LL_BUF_SIZE);
	grph_ll_buffer[1] = malloc(GRPH_LL_BUF_SIZE);
	grph_acc_buffer = malloc(GRPH_LL_MAX_COUNT*ACC_BUF_UNIT);
	memset(v_grph_ist_event, 0, sizeof(v_grph_ist_event));
	grph_jobs_init();
	vk_spin_lock_init(&v_grph_spin_locks[GRPH_ID_1]);
	vk_spin_lock_init(&v_grph_spin_locks[GRPH_ID_2]);

	request_irq(INT_ID_GRAPHIC, (irq_handler_t)graph_isr, IRQF_TRIGGER_HIGH, "grph1", 0);
	request_irq(INT_ID_GRAPHIC2, (irq_handler_t)graph2_isr, IRQF_TRIGGER_HIGH, "grph2", 0);
}
#else
void grph_platform_init(MODULE_INFO *pmodule_info)
{
	IOADDR_GRAPHIC_REG_BASE = (UINT32)pmodule_info->io_addr[0];
	IOADDR_GRAPHIC2_REG_BASE = (UINT32)pmodule_info->io_addr[1];
	p_grph_clk[0] = pmodule_info->pclk[0];
	p_grph_clk[1] = pmodule_info->pclk[1];
	OS_CONFIG_FLAG(FLG_ID_GRAPHIC);
	SEM_CREATE(SEMID_GRAPHIC, 1);
	SEM_CREATE(SEMID_GRAPHIC2, 1);
	SEM_CREATE(SEMID_GRAPHIC_OPEN_CLOSE, 1);
	SEM_CREATE(SEMID_GRAPHIC2_OPEN_CLOSE, 1);
	v_sem[0] = &SEMID_GRAPHIC;
	v_sem[1] = &SEMID_GRAPHIC2;
	v_sem_oc[0] = &SEMID_GRAPHIC_OPEN_CLOSE;
	v_sem_oc[1] = &SEMID_GRAPHIC2_OPEN_CLOSE;
	v_flg_id[0] = FLG_ID_GRAPHIC;
	v_flg_id[1] = FLG_ID_GRAPHIC;
	grph_ll_buffer[0] = kmalloc(GRPH_LL_BUF_SIZE, GFP_KERNEL);
	grph_ll_buffer[1] = kmalloc(GRPH_LL_BUF_SIZE, GFP_KERNEL);
	grph_acc_buffer = kmalloc(GRPH_LL_MAX_COUNT*ACC_BUF_UNIT, GFP_KERNEL);
	memset(v_grph_ist_event, 0, sizeof(v_grph_ist_event));
//	INIT_LIST_HEAD(&v_grph_req_list[GRPH_ID_1]);
//	INIT_LIST_HEAD(&v_grph_req_list[GRPH_ID_2]);
	v_p_timer_tasklet[GRPH_ID_1] = &pmodule_info->xxx_tasklet[GRPH_ID_1];
	v_p_timer_tasklet[GRPH_ID_2] = &pmodule_info->xxx_tasklet[GRPH_ID_2];
	grph_jobs_init();
	// coverity[side_effect_free]: spin_lock_init is kernel API, do not change
	spin_lock_init(&v_grph_spin_locks[GRPH_ID_1]);
	// coverity[side_effect_free]: spin_lock_init is kernel API, do not change
	spin_lock_init(&v_grph_spin_locks[GRPH_ID_2]);

#if 0
	DBG_ERR("GRPH addr 0x%x, GRPH2 addr 0x%x\r\n",
		IOADDR_GRAPHIC_REG_BASE,
		IOADDR_GRAPHIC2_REG_BASE);
#endif
//	printk("SEM ID: 0x%p, 0x%p\r\n", v_sem[0], v_sem[1]);
}
#endif

#if defined(__FREERTOS)
void grph_platform_uninit(void)
{
	rel_flg(FLG_ID_GRAPHIC);
	rel_flg(FLG_ID_GRPH_IST);
	SEM_DESTROY(SEMID_GRAPHIC);
	SEM_DESTROY(SEMID_GRAPHIC2);
	SEM_DESTROY(SEMID_GRAPHIC_OPEN_CLOSE);
	SEM_DESTROY(SEMID_GRAPHIC2_OPEN_CLOSE);
	free(grph_ll_buffer[0]);
	free(grph_ll_buffer[1]);
	free(grph_acc_buffer);
}
#else
void grph_platform_uninit(void)
{
	rel_flg(FLG_ID_GRAPHIC);
	SEM_DESTROY(SEMID_GRAPHIC);
	SEM_DESTROY(SEMID_GRAPHIC2);
	SEM_DESTROY(SEMID_GRAPHIC_OPEN_CLOSE);
	SEM_DESTROY(SEMID_GRAPHIC2_OPEN_CLOSE);
	kfree(grph_ll_buffer[0]);
	kfree(grph_ll_buffer[1]);
	kfree(grph_acc_buffer);
}
#endif

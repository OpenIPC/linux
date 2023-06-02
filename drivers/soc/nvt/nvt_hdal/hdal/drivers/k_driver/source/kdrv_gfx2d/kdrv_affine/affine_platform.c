#include "kwrap/type.h"
#include "affine_platform.h"
//#include "affine_drv.h"
//#include "affine_reg.h"
#include "affine_dbg.h"
#include "llc.h"

#if defined __UITRON || defined __ECOS

#include "interrupt.h"
#include "top.h"
#include "dma.h"


static const DRV_INT_NUM v_inten[] = {DRV_INT_AFFINE};
static const ID v_flg_id[] = {FLG_ID_AFFINE};
static const DRV_SEM v_sem[] = {SEMID_AFFINE};
static const CG_EN v_clken[] = {AFFINE_CLK};
UINT8 affine_ll_buffer[AFFINE_LL_BUF_SIZE];

#elif defined(__FREERTOS)

#include "kwrap/type.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "kwrap/task.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include <string.h>
#include <stdlib.h>

static ID v_flg_id[AFFINE_ID_1+1];

static SEM_HANDLE *v_sem[AFFINE_ID_1+1];

static SEM_HANDLE *v_sem_oc[AFFINE_ID_1+1];

static SEM_HANDLE SEMID_AFFINE;

static SEM_HANDLE SEMID_AFFINE_OPEN_CLOSE;

static const UINT32 v_clken[] = {AFFINE_CLK};
static UINT32 v_clksel[] = {PLL_CLKSEL_AFFINE_240};

static ID FLG_ID_AFFINE;
static ID FLG_ID_AFFINE_IST;

static vk_spinlock_t v_affine_spin_locks[AFFINE_ID_1+1];

unsigned long affine_spin_flags;

UINT8* affine_ll_buffer;

#else

#include <linux/spinlock.h>
#include <linux/slab.h>
#include "kwrap/type.h"//a header for basic variable type
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "kwrap/task.h"
#include "affine_drv.h"
#include  <mach/fmem.h>


UINT32 IOADDR_AFFINE_REG_BASE;

static ID v_flg_id[AFFINE_ID_1+1];

static SEM_HANDLE *v_sem[AFFINE_ID_1+1];

static SEM_HANDLE *v_sem_oc[AFFINE_ID_1+1];

static FLGPTN FLG_ID_AFFINE;
static FLGPTN FLG_ID_AFFINE_IST;


static SEM_HANDLE SEMID_AFFINE;

static SEM_HANDLE SEMID_AFFINE_OPEN_CLOSE;

static spinlock_t v_affine_spin_locks[AFFINE_ID_1+1];

unsigned long affine_spin_flags;

static struct tasklet_struct * v_p_affine_tasklet[AFFINE_ID_1+1];

UINT8* affine_ll_buffer;

#endif

#define AFFINE_IST_FLG_TRIG	(FLGPTN_BIT(0))
#define AFFINE_IST_FLG_EXIT	(FLGPTN_BIT(31))

THREAD_HANDLE m_affine_tsk_id = 0;

#define AFFINE_REQ_POLL_SIZE	16
static AFFINE_REQ_LIST_NODE v_req_pool[AFFINE_ID_1+1][AFFINE_REQ_POLL_SIZE];
static UINT32 v_req_front[AFFINE_ID_1+1];
static UINT32 v_req_tail[AFFINE_ID_1+1];

//KDRV_CALLBACK_FUNC v_affine_callback[AFFINE_ID_1+1];

unsigned int rtos_afn_debug_level = NVT_DBG_WRN;

void affine_platform_clk_enable(AFFINE_ID id)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(v_clken[id]);
#else
	struct clk * affine_clk;
	affine_clk = clk_get(NULL, "f0ca0000.affine");
	if (IS_ERR(affine_clk)) {
		DBG_ERR("%s: get affine clk fail\r\n", __func__);
	}
	clk_enable(affine_clk);
	clk_put(affine_clk);
#endif
}

void affine_platform_clk_disable(AFFINE_ID id)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(v_clken[id]);
#else
	struct clk * affine_clk;
	affine_clk = clk_get(NULL, "f0ca0000.affine");
	if (IS_ERR(affine_clk)) {
		DBG_ERR("%s: get affine clk fail\r\n", __func__);
	}
	clk_disable(affine_clk);
	clk_put(affine_clk);
#endif
}

void affine_platform_clk_set_freq(AFFINE_ID id, UINT32 freq)
{
#if defined __UITRON || defined __ECOS
	const UINT32 v_affine_clk_rate[1][4] = {
		{
			PLL_CLKSEL_AFFINE_240,   PLL_CLKSEL_AFFINE_320,
			PLL_CLKSEL_AFFINE_480,   PLL_CLKSEL_AFFINE_PLL13
		},
	};

	if (freq < 240) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", id, freq);
		freq = 0;
	} else if (freq == 240) {
		freq = 0;
	} else if (freq < 320) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", id, freq);
		freq = 0;
	} else if (freq == 320) {
		freq = 1;
	} else if (freq < 360) {
		DBG_WRN("%d: input frequency %d round to 320MHz\r\n", id, freq);
		freq = 1;
	} else if (freq == 360) {
		freq = 2;
	} else if (freq < 480) {
		DBG_WRN("%d: input frequency %d round to 360MHz\r\n", id, freq);
		freq = 2;
	} else if (freq == 480) {
		freq = 3;
	} else {
		DBG_WRN("%d: input frequency %d round to 480MHz\r\n", id, freq);
		freq = 3;
	}

	pll_setClockRate(vAffineClkId[id], v_affine_clk_rate[id][freq]);
#elif defined(__FREERTOS)
	const UINT32 v_affine_clk_rate[1][4] = {
		{
			PLL_CLKSEL_AFFINE_240,	 PLL_CLKSEL_AFFINE_320,
			PLL_CLKSEL_AFFINE_480,	 PLL_CLKSEL_AFFINE_PLL13
		},
	};

	if (freq < 240) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", (int)id, (int)freq);
		freq = 0;
	} else if (freq == 240) {
		freq = 0;
	} else if (freq < 320) {
		DBG_WRN("%d: input frequency %d round to 240MHz\r\n", (int)id, (int)freq);
		freq = 0;
	} else if (freq == 320) {
		freq = 1;
		if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}
	} else if (freq < 360) {
		DBG_WRN("%d: input frequency %d round to 320MHz\r\n", (int)id, (int)freq);
		freq = 1;
		if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}
	} else if (freq == 360) {
		freq = 2;
		if (pll_get_pll_enable(PLL_ID_13) == FALSE) {
			pll_set_pll_enable(PLL_ID_13, TRUE);
		}
	} else if (freq < 480) {
		DBG_WRN("%d: input frequency %d round to 360MHz\r\n", (int)id, (int)freq);
		freq = 2;
		if (pll_get_pll_enable(PLL_ID_13) == FALSE) {
			pll_set_pll_enable(PLL_ID_13, TRUE);
		}
	} else if (freq == 480) {
		freq = 3;
	} else {
		DBG_WRN("%d: input frequency %d round to 480MHz\r\n", (int)id, (int)freq);
		freq = 3;
	}

	v_clksel[id] = v_affine_clk_rate[id][freq];

#else
	struct clk *affine_clk, *parent_clk;
	affine_clk = clk_get(NULL, "f0ca0000.affine");
	if (IS_ERR(affine_clk)) {
		DBG_ERR("%s: get affine clk fail\r\n", __func__);
	}

	if (freq < 240) {
		DBG_ERR("%d: input frequency %d round to 240MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq == 240) {
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq < 320) {
		DBG_ERR("%d: input frequency %d round to 240MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq == 320) {
		parent_clk = clk_get(NULL, "pllf320");
	} else if (freq < 360) {
		DBG_ERR("%d: input frequency %d round to 320MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "pllf320");
	} else if (freq == 360) {
		parent_clk = clk_get(NULL, "pll13");
	} else if (freq < 480) {
		DBG_ERR("%d: input frequency %d round to 360MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "pll13");
	} else if (freq == 480) {
		parent_clk = clk_get(NULL, "fix480m");
	} else {
		DBG_ERR("%d: input frequency %d round to 360MHz\r\n", id, freq);
		parent_clk = clk_get(NULL, "pll13");
	}

	clk_set_parent(affine_clk, parent_clk);
	clk_put(parent_clk);
	clk_put(affine_clk);
#endif
}

void affine_platform_clk_get_freq(AFFINE_ID id, UINT32 *p_freq)
{
#if defined __UITRON || defined __ECOS
	if (id == AFFINE_ID_1) {
		switch (pll_getClockRate(PLL_CLKSEL_AFFINE)) {
		case PLL_CLKSEL_AFFINE_240:
			*p_freq = 240;
			break;
		case PLL_CLKSEL_AFFINE_320:
			*p_freq = 320;
			break;
		case PLL_CLKSEL_AFFINE_480:
			*p_freq = 480;
			break;
		case PLL_CLKSEL_AFFINE_PLL13:
			*p_freq = 360;
			break;
		}
	}
#elif defined(__FREERTOS)
	if (id == AFFINE_ID_1) {
		switch (pll_get_clock_rate(PLL_CLKSEL_AFFINE)) {
		case PLL_CLKSEL_AFFINE_240:
			*p_freq = 240;
			break;
		case PLL_CLKSEL_AFFINE_320:
			*p_freq = 320;
			break;
		case PLL_CLKSEL_AFFINE_480:
			*p_freq = 480;
			break;
		case PLL_CLKSEL_AFFINE_PLL13:
			*p_freq = 360;
			break;
		}
	}

#else
	struct clk * affine_clk;
	affine_clk = clk_get(NULL, "f0ca0000.affine");
	*p_freq = clk_get_rate(affine_clk);
	clk_put(affine_clk);
#endif
}

void affine_platform_flg_clear(AFFINE_ID id, FLGPTN flg)
{
	clr_flg(v_flg_id[id], flg);
}

void affine_platform_flg_set(AFFINE_ID id, FLGPTN flg)
{
	iset_flg(v_flg_id[id], flg);
}

void affine_platform_flg_wait(AFFINE_ID id, FLGPTN flg)
{
	FLGPTN              ptn;

	wai_flg(&ptn, v_flg_id[id], flg, TWF_ORW | TWF_CLR);
}

ER affine_platform_sem_wait(AFFINE_ID id)
{
	if (id > AFFINE_ID_1)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return wai_sem(v_sem[id]);
#else
	return SEM_WAIT(*v_sem[id]);
#endif
}

ER affine_platform_oc_sem_wait(AFFINE_ID id)
{
	if (id > AFFINE_ID_1)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return wai_sem(v_sem[id]);
#else
	return SEM_WAIT(*v_sem_oc[id]);
#endif
}

ER affine_platform_sem_signal(AFFINE_ID id)
{
	if (id > AFFINE_ID_1)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[id]);
#else
//	DBG_ERR("%s: id %d, sem 0x%p\r\n", __func__, id, v_sem[id]);
	SEM_SIGNAL(*v_sem[id]);
	return E_OK;
#endif
}

ER affine_platform_oc_sem_signal(AFFINE_ID id)
{
	if (id > AFFINE_ID_1)
		return E_NOSPT;

#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[id]);
#else
//	printk("%s: id %d, sem 0x%p\r\n", __func__, id, v_sem[id]);
	SEM_SIGNAL(*v_sem_oc[id]);
	return E_OK;
#endif
}

UINT32 affine_platform_spin_lock(AFFINE_ID id)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
#elif defined(__FREERTOS)
	vk_spin_lock_irqsave(&v_affine_spin_locks[id], affine_spin_flags);
#else
	spin_lock_irqsave(&v_affine_spin_locks[id], affine_spin_flags);
#endif

	return affine_spin_flags;
}

void affine_platform_spin_unlock(AFFINE_ID id, UINT32 flags)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#elif defined(__FREERTOS)
	vk_spin_unlock_irqrestore(&v_affine_spin_locks[id], flags);
#else
	spin_unlock_irqrestore(&v_affine_spin_locks[id], flags);
#endif
}

void affine_platform_sram_enable(AFFINE_ID id)
{
#if defined __UITRON || defined __ECOS
	if (id == AFFINE_ID_1)
		pll_disableSramShutDown(AFFINE_RSTN);
#else
#endif
}

void affine_platform_int_enable(AFFINE_ID id)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(v_inten[id]);
#else
#endif
}

void affine_platform_int_disable(AFFINE_ID id)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(v_inten[id]);
#else
#endif
}

UINT32 affine_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}

UINT32 affine_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(addr, size);
#elif defined (__FREERTOS)
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

UINT32 affine_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWithoutCheck(addr, size);
#elif defined (__FREERTOS)
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

UINT32 affine_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushWriteCacheWithoutCheck(addr, size);
#elif defined (__FREERTOS)
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

BOOL affine_platform_is_valid_va(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
#define IS_DRAM_ADDR(x)      (((x) < 0xC0000000UL)?1:0)
	return IS_DRAM_ADDR(addr);
#else
	return TRUE;	// assume user always fed valid va
#endif
}

UINT32 affine_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_getPhyAddr(addr);
#elif defined (__FREERTOS)
	return vos_cpu_get_phy_addr(addr);
#else
	return fmem_lookup_pa(addr);
#endif
}

/*
	Cp contents from KDRV_AFFINE_TRIGGER_PARAM to AFFINE_REQ_LIST_NODE

	Because pointer in AFFINE_REQ_LIST_NODE may points to stack of caller,
	these data may be not accessed by affine ISR.
	Thus we should copy data pointed by pointer.

	KDRV_AFFINE_TRIGGER_PARAM: structure passed from
					kdrv_affine_trigger()
	AFFINE_REQ_LIST_NODE: internal structure to queue jobs
*/
static ER param_to_queue_element(KDRV_AFFINE_TRIGGER_PARAM *p_param,
				AFFINE_REQ_LIST_NODE *p_node)
{
	memcpy(&p_node->trig_param, p_param,
		sizeof(KDRV_AFFINE_TRIGGER_PARAM));

	return E_OK;
}

/*
        Check if service queue is empty
*/
BOOL affine_platform_list_empty(AFFINE_ID id)
{
	if (id > AFFINE_ID_1) {
                DBG_ERR("%s: invalid id %d\r\n", __func__, id);
                return E_SYS;
        }

	if (v_req_front[id] == v_req_tail[id]) {
		// queue empty
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    Add request descriptor to service queue
*/
ER affine_platform_add_list(AFFINE_ID id,
			KDRV_AFFINE_TRIGGER_PARAM *p_param,
			KDRV_CALLBACK_FUNC *p_callback)
{
	UINT32 next;
	const UINT32 tail = v_req_tail[id];

	if (id > AFFINE_ID_1) {
		DBG_ERR("%s: invalid id %d\r\n", __func__, (int)id);
		return E_SYS;
	}

	next = (tail+1) % AFFINE_REQ_POLL_SIZE;
//	DBG_ERR("%s: next %d\r\n", __func__, (int)next);

	if (next == v_req_front[id]) {
		// queue full
		DBG_ERR("%s: queue full, front %d, tail %d\r\n", __func__,
			(int)v_req_front[id], (int)tail);
		return E_SYS;
	}

	if (param_to_queue_element(p_param, &v_req_pool[id][tail]) != E_OK)
		return E_SYS;

	if (p_callback) {
		memcpy(&v_req_pool[id][tail].callback,
			p_callback,
			sizeof(KDRV_CALLBACK_FUNC));
	} else {
		memset(&v_req_pool[id][tail].callback,
				0, sizeof(KDRV_CALLBACK_FUNC));
	}

	v_req_tail[id] = next;

        return E_OK;
}

/*
	Get head request descriptor from service queue
*/
AFFINE_REQ_LIST_NODE* affine_platform_get_head(AFFINE_ID id)
{
	AFFINE_REQ_LIST_NODE *p_node;

	p_node = &v_req_pool[id][v_req_front[id]];

	if (id > AFFINE_ID_1) {
		DBG_ERR("%s: invalid id %d\r\n", __func__, (int)id);
		return NULL;
	}

	if (v_req_front[id] == v_req_tail[id]) {
		// queue empty
		DBG_ERR("%s: queue empty\r\n", __func__);
		return NULL;
	}

	return p_node;
}

/*
	Delete request descriptor from service queue
*/
ER affine_platform_del_list(AFFINE_ID id)
{
	if (id > AFFINE_ID_1) {
		DBG_ERR("%s: invalid id %d\r\n", __func__, (int)id);
		return E_SYS;
	}

	if (v_req_front[id] == v_req_tail[id]) {
		DBG_ERR("%s: queue already empty, front %d, tail %d\r\n",
			__func__, (int)v_req_front[id], (int)v_req_tail[id]);
		return E_SYS;
	}

	v_req_front[id] = (v_req_front[id]+1) % AFFINE_REQ_POLL_SIZE;

	return E_OK;
}

void affine_platform_set_ist_event(AFFINE_ID id)
{
#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_KTHREAD)
		iset_flg(FLG_ID_AFFINE_IST, AFFINE_IST_FLG_TRIG);
#else
		/*	Tasklet for bottom half mechanism */
			tasklet_schedule(v_p_affine_tasklet[id]);
#endif
}

#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_KTHREAD)
void affine_platform_ist(void)
{
//	THREAD_ENTRY();

	while (!THREAD_SHOULD_STOP) {
		FLGPTN ptn = 0;
		UINT32 flg = AFFINE_IST_FLG_EXIT|AFFINE_IST_FLG_TRIG;

		vos_flag_wait(&ptn, FLG_ID_AFFINE_IST, flg, TWF_ORW | TWF_CLR);
//		DBG_ERR("%s: ptn 0x%x\r\n", __func__, (unsigned int)ptn);

		if (ptn & AFFINE_IST_FLG_TRIG) {
			affine_isr_bottom(AFFINE_ID_1, 0);
		}

		if (ptn & AFFINE_IST_FLG_EXIT) {
			DBG_ERR("%s: thread exit\r\n", __func__);
			break;
		}
	}

	THREAD_RETURN(0);
}
#else //if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_TASKLET)
int affine_platform_ist(AFFINE_ID id, UINT32 event)
{
	affine_isr_bottom(id, event);

	return 0;
}
#endif

#if !(defined __UITRON || defined __ECOS)
#if 0
int DBG_ERR(const char *fmt, ...)
{
}


int DBG_WRN(const char *fmt, ...)
{
}

int DBG_IND(const char *fmt, ...)
{
}
#endif
#endif

#if !(defined __UITRON || defined __ECOS)
#if defined (__FREERTOS)
static int is_create = 0;
irqreturn_t affine_platform_isr(int irq, void *devid)
{
	affine_isr();
	return IRQ_HANDLED;
}
void affine_platform_init(void)
{
	if (!is_create) {
		OS_CONFIG_FLAG(FLG_ID_AFFINE);
		OS_CONFIG_FLAG(FLG_ID_AFFINE_IST);
		SEM_CREATE(SEMID_AFFINE, 1);
		SEM_CREATE(SEMID_AFFINE_OPEN_CLOSE, 1);
		v_sem[0] = &SEMID_AFFINE;
		v_sem_oc[0] = &SEMID_AFFINE_OPEN_CLOSE;
		v_flg_id[0] = FLG_ID_AFFINE;
		affine_ll_buffer = malloc(AFFINE_LL_BUF_SIZE);
#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_TASKLET)
		v_p_affine_tasklet[AFFINE_ID_1] =
				&pmodule_info->affine_tasklet[AFFINE_ID_1];
#endif
		v_req_front[AFFINE_ID_1] = 0;
		v_req_tail[AFFINE_ID_1] = 0;
		vk_spin_lock_init(&v_affine_spin_locks[AFFINE_ID_1]);

		request_irq(INT_ID_AFFINE, affine_platform_isr, IRQF_TRIGGER_HIGH, "affine", 0);

		is_create = 1;

#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_KTHREAD)
		m_affine_tsk_id = vos_task_create(affine_platform_ist, 0, "affine_ist", 10, 4096);
		if (m_affine_tsk_id == 0) {
			DBG_ERR("%s: create affine ist fail\r\n", __func__);
		} else {
			vos_task_resume(m_affine_tsk_id);
		}
#endif

	}

}
#else
void affine_platform_init(MODULE_INFO *pmodule_info)
{
	IOADDR_AFFINE_REG_BASE = (UINT32)pmodule_info->io_addr[0];
	OS_CONFIG_FLAG(FLG_ID_AFFINE);
	SEM_CREATE(SEMID_AFFINE, 1);
	SEM_CREATE(SEMID_AFFINE_OPEN_CLOSE, 1);
	v_sem[0] = &SEMID_AFFINE;
	v_sem_oc[0] = &SEMID_AFFINE_OPEN_CLOSE;
	v_flg_id[0] = FLG_ID_AFFINE;
	affine_ll_buffer = kmalloc(AFFINE_LL_BUF_SIZE, GFP_KERNEL);
#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_TASKLET)
	v_p_affine_tasklet[AFFINE_ID_1] =
			&pmodule_info->affine_tasklet[AFFINE_ID_1];
#endif
	v_req_front[AFFINE_ID_1] = 0;
	v_req_tail[AFFINE_ID_1] = 0;
	spin_lock_init(&v_affine_spin_locks[AFFINE_ID_1]);

#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_KTHREAD)
	m_affine_tsk_id = vos_task_create(affine_platform_ist, 0, "affine_ist", 10, 4096);
	if (m_affine_tsk_id == 0) {
		DBG_ERR("%s: create affine ist fail\r\n", __func__);
	} else {
		vos_task_resume(m_affine_tsk_id);
	}
#endif

	printk("AFFINE addr 0x%x\r\n", (unsigned int)IOADDR_AFFINE_REG_BASE);
	//printk("SEM ID: 0x%x\r\n", (unsigned int)v_sem[0]);
}
#endif

#if defined (__FREERTOS)
void affine_platform_uninit(void)
{
#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_KTHREAD)
	set_flg(FLG_ID_AFFINE_IST, AFFINE_IST_FLG_EXIT);
	vos_task_destroy(m_affine_tsk_id);
#endif
	rel_flg(FLG_ID_AFFINE);
	rel_flg(FLG_ID_AFFINE_IST);
	SEM_DESTROY(SEMID_AFFINE);
	SEM_DESTROY(SEMID_AFFINE_OPEN_CLOSE);
	free(affine_ll_buffer);
}
#else
void affine_platform_uninit(void)
{
#if (AFFINE_BOTTOMHALF_SEL == AFFINE_BOTTOMHALF_KTHREAD)
	set_flg(FLG_ID_AFFINE_IST, AFFINE_IST_FLG_EXIT);
	vos_task_destroy(m_affine_tsk_id);
#endif
	rel_flg(FLG_ID_AFFINE);
	rel_flg(FLG_ID_AFFINE_IST);
	SEM_DESTROY(SEMID_AFFINE);
	SEM_DESTROY(SEMID_AFFINE_OPEN_CLOSE);
	kfree(affine_ll_buffer);
}
#endif
#endif

#if defined(__FREERTOS)
#include "include/timer_platform.h"
#else
#include "timer_platform.h"
#endif

#if defined __UITRON || defined __ECOS

#include "timer.h"
#include "interrupt.h"
#include "top.h"
#include "dma.h"


static const DRV_INT_NUM v_inten[] = {DRV_INT_TIMER};
static const ID v_flg_id[] = {FLG_ID_TIMER};
static const DRV_SEM v_sem[] = {SEMID_TIMER};
static const CG_EN v_clken[] = {TMR_CLK};

#elif defined(__FREERTOS)
#include <stdio.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include <kwrap/nvt_type.h>
#include "pll_protected.h"
#include "rcw_macro.h"

#define dma_getPhyAddr(parm) rcw_get_phy_addr(parm)
#define wai_sem vos_sem_wait
#define sig_sem vos_sem_sig

static ID FLG_ID_TIMER;
static ID timer_semaphore;
static VK_DEFINE_SPINLOCK(timer_spinlock);
#define loc_cpu(spin_flags) vk_spin_lock_irqsave(&timer_spinlock, spin_flags)
#define unl_cpu(spin_flags) vk_spin_unlock_irqrestore(&timer_spinlock, spin_flags)
static ID v_flg_id[1];
static ID v_sem[1];
static CG_EN v_clken[1] = {TMR_CLK};

volatile DRV_CB          pf_ist_cbs_timer[TIMER_NUM];

static UINT32 ist_timer_events;
#else

#include "timer_int.h"
#include "comm/timer.h"
#include <linux/spinlock.h>
#include "kwrap/type.h"//a header for basic variable type
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include <mach/fmem.h>


UINT32 _REGIOBASE;
UINT32 IOADDR_TIMER_REG_BASE;

static ID v_flg_id[1];

static SEM_HANDLE *v_sem[1];

static SEM_HANDLE SEMID_TIMER;

static FLGPTN     FLG_ID_TIMER;
static FLGPTN     FLG_ID_TMR_IST;

static spinlock_t v_spin_lock[1];

THREAD_HANDLE m_timer_tsk_id = 0;

static struct tasklet_struct *p_timer_tasklet;

volatile DRV_CB          pf_ist_cbs_timer[TIMER_NUM];

//static UINT32 ist_timer_log;

static UINT32 ist_timer_events;

#endif

#define TIMER_IST_FLG_TRIG	(FLGPTN_BIT(0))
#define TIMER_IST_FLG_EXIT	(FLGPTN_BIT(31))

//static const FLGPTN vGraphFlag[] = {FLGPTN_GRAPHIC, FLGPTN_GRAPHIC2};

void timer_platform_clk_enable(void)
{
#if defined __UITRON || defined __ECOS
	pll_enableClock(v_clken[0]);
#elif defined __FREERTOS
	pll_enable_clock(v_clken[0]);
#else
#endif
}

void timer_platform_clk_disable(void)
{
#if defined __UITRON || defined __ECOS
	pll_disableClock(v_clken[0]);
#elif defined __FREERTOS
	pll_disable_clock(v_clken[0]);
#else
#endif
}

void timer_platform_clk_set_freq(UINT32 freq)
{
#if defined __UITRON || defined __ECOS
#else
#endif
}

void timer_platform_clk_get_freq(UINT32 *p_freq)
{
#if defined __UITRON || defined __ECOS
#else
#endif
}

void timer_platform_flg_clear(FLGPTN flg)
{
	clr_flg(v_flg_id[0], flg);
}

void timer_platform_flg_set(FLGPTN flg)
{
	iset_flg(v_flg_id[0], flg);
}

void timer_platform_flg_wait(FLGPTN flg)
{
	FLGPTN              ptn;

//	printk("%s: flg id %d\r\n", __func__, v_flg_id[id]);
	wai_flg(&ptn, v_flg_id[0], flg, TWF_ORW | TWF_CLR);
//	printk("%s: done\r\n", __func__);
}

FLGPTN timer_platform_flg_check(FLGPTN chkptn)
{
	return kchk_flg(FLG_ID_TIMER, chkptn);
}

ER timer_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return wai_sem(v_sem[0]);
#else
	return SEM_WAIT(*v_sem[0]);
#endif
}

ER timer_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[0]);
#elif defined __FREERTOS
	sig_sem(v_sem[0]);
	return E_OK;
#else
//	printk("%s: id %d, sem 0x%p\r\n", __func__, id, v_sem[id]);
	SEM_SIGNAL(*v_sem[0]);
	return E_OK;
#endif
}

UINT32 timer_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	unsigned long flags;
	loc_cpu(flags);
	return flags;
#else
	unsigned long flags;
	spin_lock_irqsave(&v_spin_lock[0], flags);
	return flags;
#endif
}

void timer_platform_spin_unlock(UINT32 flags)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	unl_cpu(flags);
#else
	spin_unlock_irqrestore(&v_spin_lock[0], flags);
#endif
}

void timer_platform_sram_enable(void)
{
#if defined __UITRON || defined __ECOS
#else
#endif
}

void timer_platform_int_enable(void)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(v_inten[0]);
#else
#endif
}

void timer_platform_int_disable(void)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(v_inten[0]);
#else
#endif
}

UINT32 timer_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}

UINT32 timer_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(addr, size);
#elif defined __FREERTOS
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
		fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
		return 0;
	}
#endif
}

UINT32 timer_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWithoutCheck(addr, size);
#elif defined __FREERTOS
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
		fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
		ret = 0;
	}

	return ret;
#endif
}

UINT32 timer_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushWriteCacheWithoutCheck(addr, size);
#elif defined __FREERTOS
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
		fmem_dcache_sync((void *)addr, size, DMA_BIDIRECTIONAL);
		ret = 0;
	}

	return ret;
#endif
}

BOOL timer_platform_is_valid_va(UINT32 addr)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
#define IS_DRAM_ADDR(x)      (((x) < 0xC0000000UL)?1:0)
	return IS_DRAM_ADDR(addr);
#else
	if (fmem_lookup_pa(addr) == 0xFFFFFFFF)
		return FALSE;
	else
		return TRUE;
#endif
}

UINT32 timer_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_getPhyAddr(addr);
#else
	return fmem_lookup_pa(addr);
#endif
}

void timer_platform_set_ist_event(UINT32 events)
{
#if defined __UITRON || defined __ECOS
	drv_setIstEvent(DRV_IST_MODULE_TIMER, events);
#elif defined __FREERTOS
	kick_bh(INT_ID_TIMER, events, NULL);
#else
	UINT32 losing_event, i = 0;

//	printk("%s: 0x%x\r\n", __func__, events);

	losing_event = ist_timer_events & events;

	if (losing_event) {
		if ((losing_event & nvt_get_silence_timer()) == 0) {
			nvt_dbg(ERR, "losing event: now 0x%x, last 0x%x\r\n",
				events, ist_timer_events);
		}

		for (i = 0; i < 20; i++) {
			if ((losing_event >> i) & 0x1)
				nvt_set_losing_event_count(i);
		}
	}

	ist_timer_events |= events;
//	ist_timer_log = timer_get_current_count(timer_get_sys_timer_id());
#if (TIMER_BOTTOMHALF_SEL == TIMER_BOTTOMHALF_KTHREAD)
	iset_flg(FLG_ID_TMR_IST, TIMER_IST_FLG_TRIG);
#else
	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(p_timer_tasklet);
#endif

#endif
}

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

#if (TIMER_BOTTOMHALF_SEL == TIMER_BOTTOMHALF_KTHREAD)
static THREAD_DECLARE(timer_platform_ist, arglist)
{
//	THREAD_ENTRY();

	while (!THREAD_SHOULD_STOP) {
		FLGPTN ptn = 0x0;
		UINT32 flg = TIMER_IST_FLG_EXIT|TIMER_IST_FLG_TRIG;

		wai_flg(&ptn, FLG_ID_TMR_IST, flg, TWF_ORW | TWF_CLR);
//		printk("%s: ptn 0x%x\r\n", __func__, ptn);

		if (ptn & TIMER_IST_FLG_TRIG) {
			UINT32 i, events, flags;

			flags = timer_platform_spin_lock();
			events = ist_timer_events;
			ist_timer_events = 0;
			timer_platform_spin_unlock(flags);

			do {
				i = __builtin_ctz(events);
				events &= ~(1 << i);

#if 0
				printk("%s: id %d, ptr 0x%p\r\n", __func__, i,
					 &pf_ist_cbs_timer[i]);
				printk("%s: id %d, cb 0x%p\r\n", __func__, i,
					 pf_ist_cbs_timer[i]);
#endif
				if (i < TIMER_NUM) {
					if (pf_ist_cbs_timer[i] != NULL) {
						pf_ist_cbs_timer[i](i);
					} else {
						printk("%s: %d is NULL, event 0x%x\r\n",
							__func__,
							i, (int)events);
					}
				}
			} while (events);
		}

		if (ptn & TIMER_IST_FLG_EXIT) {
			printk("%s: thread exit\r\n", __func__);
			break;
		}
	}

	THREAD_RETURN(0);
}
#else //if (TIMER_BOTTOMHALF_SEL == TIMER_BOTTOMHALF_TASKLET)
int timer_platform_ist(UINT32 event)
{
	UINT32 i, events;
	UINT32 spin_flags;

//	i = timer_get_current_count(timer_get_sys_timer_id());
//	printk("%s: itick %d, ntick %d\r\n", __func__,
//			ist_timer_log, i);

	spin_flags = timer_platform_spin_lock();
#if defined __FREERTOS
	events = event;
#else
	events = ist_timer_events;
#endif
	ist_timer_events = 0;
	timer_platform_spin_unlock(spin_flags);

	do {
		i = __builtin_ctz(events);
		events &= ~(1 << i);

		if (i < TIMER_NUM) {
				if (pf_ist_cbs_timer[i] != NULL) {
				pf_ist_cbs_timer[i](i);
			} else {
#if defined __FREERTOS
				printf("%s: %ld is NULL, event 0x%lx\r\n",
					__func__,
					i, events);
#else
				printk("%s: %d is NULL, event 0x%x\r\n",
					__func__,
					i, events);
#endif
			}
		}
	} while (events);


	return 0;
}
#endif

#if defined __FREERTOS
void timer_platform_create_resource(void)
{
	cre_flg(&FLG_ID_TIMER, NULL, "timer_flag");
	vos_sem_create(&timer_semaphore, 20, "timer_semaphore");
	v_sem[0] = timer_semaphore;
	v_flg_id[0] = FLG_ID_TIMER;
	timer_open_sys_timer();
}
#else
void timer_platform_create_resource(MODULE_INFO *pmodule_info)
{
	_REGIOBASE = (UINT32)pmodule_info->io_addr[0];
	IOADDR_TIMER_REG_BASE = (UINT32)pmodule_info->io_addr[0];
	p_timer_tasklet = &pmodule_info->tasklet;
	OS_CONFIG_FLAG(FLG_ID_TIMER);
	OS_CONFIG_FLAG(FLG_ID_TMR_IST);
	SEM_CREATE(SEMID_TIMER, TIMER_LAST_ID+1);
	v_sem[0] = &SEMID_TIMER;
	v_flg_id[0] = FLG_ID_TIMER;
#ifdef CONFIG_NOVATEK_TIMER
	v_spin_lock[0] = nvttmr_lock;
#else
	spin_lock_init(&v_spin_lock[0]);
#endif

	// coverity[side_effect_free]: spin_lock_init is kernel API, do not change

	ist_timer_events = 0;

#if (TIMER_BOTTOMHALF_SEL == TIMER_BOTTOMHALF_KTHREAD)
	THREAD_CREATE(m_timer_tsk_id, timer_platform_ist, NULL, "timer_ist");
	THREAD_SET_PRIORITY(m_timer_tsk_id, 2);
	THREAD_RESUME(m_timer_tsk_id);
#endif

	timer_open_sys_timer();
}
#endif

void timer_platform_release_resource(void)
{
#if defined __FREERTOS
	rel_flg(FLG_ID_TIMER);
	SEM_DESTROY(timer_semaphore);
#else
#if (TIMER_BOTTOMHALF_SEL == TIMER_BOTTOMHALF_KTHREAD)
	set_flg(FLG_ID_TMR_IST, TIMER_IST_FLG_EXIT);
	THREAD_DESTROY(m_timer_tsk_id);
#endif
	rel_flg(FLG_ID_TIMER);
	rel_flg(FLG_ID_TMR_IST);
	SEM_DESTROY(SEMID_TIMER);
#endif
}

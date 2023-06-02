
#include "ddr_arb_platform.h"

#if defined __UITRON || defined __ECOS

#include "ddr_arb.h"
#include "interrupt.h"
#include "top.h"
#include "dma.h"


static const DRV_INT_NUM v_inten[] = {DRV_INT_ARB};
static const ID v_flg_id[] = {FLG_ID_ARB};
static const DRV_SEM v_sem[] = {SEMID_ARB};
static const CG_EN v_clken[] = {ARB_CLK};

#elif defined(__FREERTOS)
#include "ddr_arb_int.h"

static ID v_flg_id[1];

static SEM_HANDLE *v_sem[1];

static SEM_HANDLE SEMID_DDR_ARB;

static ID     FLG_ID_DDR_ARB;
static ID     FLG_ID_DDR_ARB_IST;

static vk_spinlock_t v_spin_lock[1];

#else

#include "ddr_arb_int.h"
#include "comm/ddr_arb.h"
#include <linux/spinlock.h>
#include <linux/delay.h>
#include "kwrap/type.h"//a header for basic variable type
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include <mach/fmem.h>


UINT32 IOADDR_DDR_ARB_REG_BASE;
UINT32 IOADDR_DDR_ARB2_REG_BASE;

UINT32 IOADDR_DDR_ARB_PROT_REG_BASE;
UINT32 IOADDR_DDR_ARB2_PROT_REG_BASE;


static ID v_flg_id[1];

static SEM_HANDLE *v_sem[1];

static SEM_HANDLE SEMID_DDR_ARB;

static FLGPTN     FLG_ID_DDR_ARB;
static FLGPTN     FLG_ID_DDR_ARB_IST;

static spinlock_t v_spin_lock[1];

THREAD_HANDLE m_ddr_arb_tsk_id = 0;

static struct tasklet_struct *p_ddr_arb_tasklet;

volatile DRV_CB          pf_ist_cbs_ddr_arb[DDR_ARB_COUNT];

static UINT32 ist_ddr_arb_events;

#endif

unsigned int rtos_mau_debug_level = NVT_DBG_ERR;

void ddr_arb_platform_clk_enable(void)
{
#if defined __UITRON || defined __ECOS
	pll_enableClock(v_clken[0]);
#else
#endif
}

void ddr_arb_platform_clk_disable(void)
{
#if defined __UITRON || defined __ECOS
	pll_disableClock(v_clken[0]);
#else
#endif
}

void ddr_arb_platform_clk_get_freq(UINT32 *p_freq)
{
#if defined __KERNEL__

	struct clk *pll3_clk;
	pll3_clk = clk_get(NULL, "pll3");
	if (IS_ERR(pll3_clk)) {
		DBG_ERR("get pll3 failed\r\n");
	}

	*p_freq = clk_get_rate(pll3_clk) * 2;

#else

	*p_freq = pll_get_pll_freq(PLL_ID_3) * 2;
#endif
}

void ddr_arb_platform_flg_clear(FLGPTN flg)
{
	clr_flg(v_flg_id[0], flg);
}

void ddr_arb_platform_flg_set(FLGPTN flg)
{
	 iset_flg(v_flg_id[0], flg);
}

void ddr_arb_platform_flg_wait(FLGPTN flg)
{
	FLGPTN              ptn;

//	printk("%s: flg id %d\r\n", __func__, v_flg_id[id]);
	wai_flg(&ptn, v_flg_id[0], flg, TWF_ORW | TWF_CLR);
//	printk("%s: done\r\n", __func__);
}

FLGPTN ddr_arb_platform_flg_check(FLGPTN chkptn)
{
	return kchk_flg(FLG_ID_DDR_ARB, chkptn);
}

ER ddr_arb_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(v_sem[0]);
#else
	return SEM_WAIT(*v_sem[0]);
#endif
}

ER ddr_arb_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[0]);
#else
//	printk("%s: id %d, sem 0x%p\r\n", __func__, id, v_sem[id]);
	SEM_SIGNAL(*v_sem[0]);
	return E_OK;
#endif
}

UINT32 ddr_arb_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#elif defined(__FREERTOS)
	unsigned long flags;
	vk_spin_lock_irqsave(&v_spin_lock[0], flags);
	return flags;
#else
	unsigned long flags;
	spin_lock_irqsave(&v_spin_lock[0], flags);
	return flags;
#endif
}

void ddr_arb_platform_spin_unlock(UINT32 flags)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#elif defined(__FREERTOS)
	vk_spin_unlock_irqrestore(&v_spin_lock[0], flags);
#else
	spin_unlock_irqrestore(&v_spin_lock[0], flags);
#endif
}

void ddr_arb_platform_int_enable(void)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(v_inten[0]);
#else
#endif
}

void ddr_arb_platform_int_disable(void)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(v_inten[0]);
#else
#endif
}

UINT32 ddr_arb_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}

UINT32 ddr_arb_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(addr, size);
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

UINT32 ddr_arb_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_flushReadCacheWithoutCheck(addr, size);
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

UINT32 ddr_arb_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_flushWriteCacheWithoutCheck(addr, size);
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

BOOL ddr_arb_platform_is_valid_va(UINT32 addr)
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

UINT32 ddr_arb_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_getPhyAddr(addr);
#else
	return fmem_lookup_pa(addr);
#endif
}

void ddr_arb_platform_delay_ms(UINT32 ms)
{
#if defined __UITRON || defined __ECOS
	Delay_DelayMs(ms);
#elif defined __FREERTOS
#if defined(_NVT_FPGA_) && defined(_CPU2_RTOS_)
	vos_task_delay_us_polling(ms*1000);
#else
	vos_task_delay_ms(ms);
#endif
#else
	msleep(ms);
#endif
}



#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "comm/hwclock.h"
#define FLGDBGUT_DMA_HVYLOAD  FLGPTN_BIT(4)   //notify task there's command coming
#define FLGDBGUT_QUIT_HVY     FLGPTN_BIT(6)

static DRAM_CONSUME_ATTR nvt_heavyload_cfg = {0};
static ID nvt_dram1_heavyload_flag_id;
static THREAD_HANDLE nvt_dram1_heavyload_tsk_id;
static int is_create = 0;

void dram_consume_task(void)
{
	UINT32 count[1];
	UINT32 test1_start, test2_start, test3_start;
	UINT32 test1_size, test2_size, test3_size;
	UINT32 dma_grp = 0, ch_bit = 0;
	UINT32 dma_ch;
	UINT32 dis_count;
	UINT32 ch_no;
	DMA_HEAVY_LOAD_PARAM hvy_param0, hvy_param1, hvy_param2;
	BOOL err = FALSE;
	FLGPTN flg_ptn = 0;
	ID flag_id = nvt_dram1_heavyload_flag_id;
	FLGPTN mask = FLGDBGUT_DMA_HVYLOAD | FLGDBGUT_QUIT_HVY;

	dis_count = 0;
	while (1) {
		wai_flg(&flg_ptn, flag_id, mask, TWF_ORW | TWF_CLR);

		if ((flg_ptn & FLGDBGUT_DMA_HVYLOAD) != 0U) {
			if (nvt_heavyload_cfg.load_degree == DRAM_CONSUME_CH_DISABLE) {
				// Scan which DMA channel is required to do disable/enable test
				for (dma_grp = 0; dma_grp < DMA_CH_GROUP_CNT; dma_grp++) {
					ch_bit = nvt_heavyload_cfg.dma_channel.channel_group[dma_grp];

					while (ch_bit) {
						dma_ch = __builtin_ctz(ch_bit);
						ch_no = (dma_ch + (dma_grp * 32));
						if ((dis_count & 0x1) == 0) {

							DBG_DUMP("Channel No.%d => disable\r\n", ch_no);
							dma_disableChannel(ch_no);
						} else {
							DBG_DUMP("Channel No.%d => enable\r\n", ch_no);
							dma_enableChannel(ch_no);
						}

						ch_bit &= ~(1 << dma_ch);
					}
				}
				dis_count++;
				srand((unsigned) hwclock_get_counter());
				vos_task_delay_ms(rand()&0x3FF);
				if (nvt_heavyload_cfg.is_start == TRUE) {
					set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_DMA_HVYLOAD);
				} else {
					if ((dis_count & 0x1) == 1) {
						set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_DMA_HVYLOAD);
					}
				}


			} else {
				test1_size =
					test2_size =
						test3_size = ALIGN_FLOOR_32(nvt_heavyload_cfg.size / 3);

				test1_start = nvt_heavyload_cfg.addr;
				test2_start = test1_start + test1_size;
				test3_start = test2_start + test2_size;

				srand((unsigned) hwclock_get_counter());
				count[0] = rand();
				count[0] = count[0] & 0xffff;

				hvy_param0.burst_len    = 127;
				hvy_param0.dma_size     = test1_size;
				hvy_param0.test_method  = DMA_HEAVY_LOAD_RW_BURST_INTERLEAVE;
				hvy_param0.start_addr   = test1_start;
				hvy_param0.test_times   = count[0];

				hvy_param1.burst_len    = 126;
				hvy_param1.dma_size     = test2_size;
				hvy_param1.test_method  = DMA_HEAVY_LOAD_READ_AFTER_WRITE_ALL_BUF;
				hvy_param1.start_addr   = test2_start;
				hvy_param1.test_times   = count[0];

				hvy_param2.burst_len    = 128;
				hvy_param2.dma_size     = test3_size;
				hvy_param2.test_method  = DMA_HEAVY_LOAD_READ_AFTER_WRITE_ALL_BUF;
				hvy_param2.start_addr   = test3_start;
				hvy_param2.test_times   = count[0];

				DBG_DUMP("*************ConsumeTsk Information*******************\r\n");
				DBG_DUMP("*   (@)Loading Degree ==================>[     %05d]*\r\n", (int)nvt_heavyload_cfg.load_degree);


				// Easy
				if (nvt_heavyload_cfg.load_degree == DRAM_CONSUME_EASY_LOADING) {
					if (hvy_param0.test_times > 2000) {
						hvy_param0.test_times = 2000;
					}
					DBG_DUMP("*   (@)Channel0 Burst Length============>[     %05d]*\r\n", (int)hvy_param0.burst_len);
					DBG_DUMP("*   (@)Channel0 Test DMA addr ==========>[0x%08x]*\r\n", (unsigned int)hvy_param0.start_addr);
					DBG_DUMP("*   (@)Channel0 Test DMA size ==========>[0x%08x]*\r\n", (unsigned int)hvy_param0.dma_size);
					DBG_DUMP("*   (@)Channel0 Test count  ============>[     %05d]*\r\n", (int)hvy_param0.test_times);
					DBG_DUMP("*************ConsumeTsk Information*******************\r\n");


					if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH0, &hvy_param0) != E_OK) {
						DBG_ERR("Consume task open fail degree[%d]-1\r\n", (int)nvt_heavyload_cfg.load_degree);
						err = TRUE;
					}

					dma_trig_heavyload(DDR_ARB_1, 0x1);
				}
				// Normal
				else if (nvt_heavyload_cfg.load_degree == DRAM_CONSUME_NORMAL_LOADING) {
					DBG_DUMP("*   (@)Channel0 Burst Length============>[     %05d]*\r\n", (int)hvy_param0.burst_len);
					DBG_DUMP("*   (@)Channel0 Test DMA addr ==========>[0x%08x]*\r\n", (unsigned int)hvy_param0.start_addr);
					DBG_DUMP("*   (@)Channel0 Test DMA size ==========>[0x%08x]*\r\n", (unsigned int)hvy_param0.dma_size);
					DBG_DUMP("*   (@)Channel0 Test count  ============>[     %05d]*\r\n", (int)hvy_param0.test_times);
					DBG_DUMP("*   (@)Channel1 Burst Length============>[     %05d]*\r\n", (int)hvy_param1.burst_len);
					DBG_DUMP("*   (@)Channel1 Test DMA addr ==========>[0x%08x]*\r\n", (unsigned int)hvy_param1.start_addr);
					DBG_DUMP("*   (@)Channel1 Test DMA size ==========>[0x%08x]*\r\n", (unsigned int)hvy_param1.dma_size);
					DBG_DUMP("*   (@)Channel1 Test count  ============>[     %05d]*\r\n", (int)hvy_param1.test_times);
					DBG_DUMP("*************ConsumeTsk Information*******************\r\n");
					if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH0, &hvy_param1) != E_OK) {
						DBG_ERR("Consume task open fail degree[%d]-1\r\n", (int)nvt_heavyload_cfg.load_degree);
						err = TRUE;
					}
					if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH1, &hvy_param1) != E_OK) {
						DBG_ERR("Consume task open fail degree[%d]-2\r\n", (int)nvt_heavyload_cfg.load_degree);
						err = TRUE;
					}

					dma_trig_heavyload(DDR_ARB_1, 0x3);
				}
				// Heavy
				else {
					DBG_DUMP("*   (@)Channel0 Burst Length============>[     %05d]*\r\n", (int)hvy_param0.burst_len);
					DBG_DUMP("*   (@)Channel0 Test DMA addr ==========>[0x%08x]*\r\n", (unsigned int)hvy_param0.start_addr);
					DBG_DUMP("*   (@)Channel0 Test DMA size ==========>[0x%08x]*\r\n", (unsigned int)hvy_param0.dma_size);
					DBG_DUMP("*   (@)Channel0 Test count  ============>[     %05d]*\r\n", (int)hvy_param0.test_times);
					DBG_DUMP("*   (@)Channel1 Burst Length============>[     %05d]*\r\n", (int)hvy_param1.burst_len);
					DBG_DUMP("*   (@)Channel1 Test DMA addr ==========>[0x%08x]*\r\n", (unsigned int)hvy_param1.start_addr);
					DBG_DUMP("*   (@)Channel1 Test DMA size ==========>[0x%08x]*\r\n", (unsigned int)hvy_param1.dma_size);
					DBG_DUMP("*   (@)Channel1 Test count  ============>[     %05d]*\r\n", (int)hvy_param1.test_times);
					DBG_DUMP("*   (@)Channel2 Burst Length============>[     %05d]*\r\n", (int)hvy_param2.burst_len);
					DBG_DUMP("*   (@)Channel2 Test DMA addr ==========>[0x%08x]*\r\n", (unsigned int)hvy_param2.start_addr);
					DBG_DUMP("*   (@)Channel2 Test DMA size ==========>[0x%08x]*\r\n", (unsigned int)hvy_param2.dma_size);
					DBG_DUMP("*   (@)Channel2 Test count  ============>[     %05d]*\r\n", (int)hvy_param2.test_times);
					DBG_DUMP("*************ConsumeTsk Information*******************\r\n");


					//dma_setChannelPriority(DMA_CH_CPU, DMA_PRIORITY_HIGH);
					//dma_setChannelPriority(DMA_CH_CPU2, DMA_PRIORITY_HIGH);
					//dma_setChannelPriority(DMA_CH_HLOAD_0, DMA_PRIORITY_MIDDLE);
					//dma_setChannelPriority(DMA_CH_HLOAD_1, DMA_PRIORITY_MIDDLE);
					//dma_setChannelPriority(DMA_CH_HLOAD_2, DMA_PRIORITY_MIDDLE);

					if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH0, &hvy_param0) != E_OK) {
						DBG_ERR("Consume task open fail degree[%d]-1\r\n", (int)nvt_heavyload_cfg.load_degree);
						err = TRUE;
					}
					if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH1, &hvy_param1) != E_OK) {
						DBG_ERR("Consume task open fail degree[%d]-2\r\n", (int)nvt_heavyload_cfg.load_degree);
						err = TRUE;
					}
					if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH2, &hvy_param2) != E_OK) {              // Enable maximun 3 channel at the same time
						DBG_ERR("Consume task open fail degree[%d]-3\r\n", (int)nvt_heavyload_cfg.load_degree);
						err = TRUE;
					}

					dma_trig_heavyload(DDR_ARB_1, 0x7);
				}

				if (err == FALSE) {
					if (dma_wait_heavyload_done_polling(DDR_ARB_1, NULL) == TRUE) {
						DBG_IND("SUCCESS\r\n");

						if (nvt_heavyload_cfg.is_start == TRUE)
							set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_DMA_HVYLOAD);
						else
							;//fmem_release_from_cma(handle2, 0);
					} else {
						DBG_ERR("FAIL \r\n");
						DBG_ERR("Terminate Dram consume task... \r\n");
					}
				} else {
					nvt_heavyload_cfg.is_start = FALSE;
					//fmem_release_from_cma(handle2, 0);
				}
			}
		}
		if ((flg_ptn & FLGDBGUT_QUIT_HVY) != 0U) {
			break;
		}
	}
	THREAD_RETURN(0);
}



int dram_consume_cfg(PDRAM_CONSUME_ATTR attr)
{
	DRAM_CONSUME_ATTR *dram_consume_cfg = &nvt_heavyload_cfg;

	dram_consume_cfg->load_degree = attr->load_degree;
	dram_consume_cfg->size = attr->size;
	dram_consume_cfg->dma_channel = attr->dma_channel;

	dram_consume_cfg->addr = attr->addr;
	dram_consume_cfg->is_start = FALSE;

	return 0;
}

int dram_consume_start(void)
{
	DRAM_CONSUME_ATTR *dram_consume_cfg = &nvt_heavyload_cfg;

	if (dram_consume_cfg->is_start) {
		DBG_WRN( "DRAM HEAVYLOAD IS START\r\n");
		return 0;
	}

	dram_consume_cfg->is_start = TRUE;

	set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_DMA_HVYLOAD);
    return 0;
}

int dram_consume_stop(void)
{
	DRAM_CONSUME_ATTR *dram_consume_cfg = &nvt_heavyload_cfg;

	if (!dram_consume_cfg->is_start) {
		DBG_WRN("DRAM HEAVYLOAD IS NOT START\r\n");
		return 0;
	}
	dma_stop_heavyload(DDR_ARB_1);
	dram_consume_cfg->is_start = FALSE;
    return 0;
}

/*
void ddr_arb_platform_create_resource(void)
{
	if (!is_create) {
		OS_CONFIG_FLAG(FLG_ID_DDR_ARB);
		OS_CONFIG_FLAG(FLG_ID_DDR_ARB_IST);
		SEM_CREATE(SEMID_DDR_ARB, 1);
		v_sem[0] = &SEMID_DDR_ARB;
		v_flg_id[0] = FLG_ID_DDR_ARB;
		vk_spin_lock_init(&v_spin_lock[0]);

		//dma_set_monitor_period();

		OS_CONFIG_FLAG(nvt_dram1_heavyload_flag_id);
		THREAD_CREATE(nvt_dram1_heavyload_tsk_id, dram_consume_task, NULL, "dram_consume_task");
		THREAD_RESUME(nvt_dram1_heavyload_tsk_id);
		request_irq(INT_ID_MAU, ddr_platform_isr, IRQF_TRIGGER_HIGH, "dma", 0);
		//dma_set_system_priority(TRUE);

		is_create = 1;
	}
}
*/
irqreturn_t ddr_platform_isr(int irq, void *devid)
{
	arb_isr();
	return IRQ_HANDLED;
}

void ddr_arb_platform_create_resource(void)
{
	if (!is_create) {
		OS_CONFIG_FLAG(FLG_ID_DDR_ARB);
		OS_CONFIG_FLAG(FLG_ID_DDR_ARB_IST);
		SEM_CREATE(SEMID_DDR_ARB, 1);
		v_sem[0] = &SEMID_DDR_ARB;
		v_flg_id[0] = FLG_ID_DDR_ARB;
		vk_spin_lock_init(&v_spin_lock[0]);

		dma_set_monitor_period();

		OS_CONFIG_FLAG(nvt_dram1_heavyload_flag_id);
		THREAD_CREATE(nvt_dram1_heavyload_tsk_id, dram_consume_task, NULL, "dram_consume_task");
		THREAD_RESUME(nvt_dram1_heavyload_tsk_id);


		request_irq(INT_ID_DMA, ddr_platform_isr, IRQF_TRIGGER_HIGH, "dma", 0);

		dma_set_system_priority(TRUE);

		is_create = 1;
	}
}
void ddr_arb_platform_release_resource(void)
{
	rel_flg(FLG_ID_DDR_ARB);
	rel_flg(FLG_ID_DDR_ARB_IST);
	SEM_DESTROY(SEMID_DDR_ARB);
}

#else

void ddr_arb_platform_set_ist_event(UINT32 events)
{
#if defined __UITRON || defined __ECOS
	drv_setIstEvent(DRV_IST_MODULE_DDR_ARB, events);
#else
	UINT32 losing_event;

//	printk("%s: 0x%x\r\n", __func__, events);

	losing_event = ist_ddr_arb_events & events;

	if (losing_event) {
		nvt_dbg(ERR, "losing event: now 0x%x, last 0x%x\r\n",
				events, ist_ddr_arb_events);
	}

	ist_ddr_arb_events |= events;
#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_KTHREAD)
	iset_flg(FLG_ID_TMR_IST, DDR_ARB_IST_FLG_TRIG);
#else
	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(p_ddr_arb_tasklet);
#endif

#endif
}

#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_KTHREAD)
static THREAD_DECLARE(ddr_arb_platform_ist, arglist)
{
//	THREAD_ENTRY();

	while (!THREAD_SHOULD_STOP) {
		FLGPTN ptn;
		UINT32 flg = DDR_ARB_IST_FLG_EXIT|DDR_ARB_IST_FLG_TRIG;

		wai_flg(&ptn, FLG_ID_TMR_IST, flg, TWF_ORW | TWF_CLR);
//		printk("%s: ptn 0x%x\r\n", __func__, ptn);

		if (ptn & DDR_ARB_IST_FLG_TRIG) {
			UINT32 i, events;

			ddr_arb_platform_spin_lock();
			events = ist_ddr_arb_events;
			ist_ddr_arb_events = 0;
			ddr_arb_platform_spin_unlock();

			do {
				i = __builtin_ctz(events);
				events &= ~(1 << i);

				if (pf_ist_cbs_ddr_arb[i] != NULL) {
					pf_ist_cbs_ddr_arb[i](0);
				} else {
					printk("%s: %d is NULL, event 0x%x\r\n",
						__func__,
						i, events);
				}
			} while (events);
		}

		if (ptn & DDR_ARB_IST_FLG_EXIT) {
			printk("%s: thread exit\r\n", __func__);
			break;
		}
	}

	THREAD_RETURN(0);
}
#else //if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_TASKLET)
int ddr_arb_platform_ist(UINT32 event)
{
	UINT32 i, events;
	UINT32 spin_flags;

	spin_flags = ddr_arb_platform_spin_lock();
	events = ist_ddr_arb_events;
	ist_ddr_arb_events = 0;
	ddr_arb_platform_spin_unlock(spin_flags);

	do {
		i = __builtin_ctz(events);
		events &= ~(1 << i);

		if (pf_ist_cbs_ddr_arb[i] != NULL) {
			pf_ist_cbs_ddr_arb[i](0);
		} else {
			printk("%s: %d is NULL, event 0x%x\r\n",
				__func__,
				i, events);
		}
	} while (events);


	return 0;
}
#endif


void ddr_arb_platform_create_resource(MODULE_INFO *pmodule_info)
{
	IOADDR_DDR_ARB_REG_BASE = (UINT32)pmodule_info->io_addr[0];
	IOADDR_DDR_ARB_PROT_REG_BASE = (UINT32)pmodule_info->io_addr[1];
	p_ddr_arb_tasklet = &pmodule_info->xxx_tasklet;
	OS_CONFIG_FLAG(FLG_ID_DDR_ARB);
	OS_CONFIG_FLAG(FLG_ID_DDR_ARB_IST);
	SEM_CREATE(SEMID_DDR_ARB, 1);
	v_sem[0] = &SEMID_DDR_ARB;
	v_flg_id[0] = FLG_ID_DDR_ARB;
	spin_lock_init(&v_spin_lock[0]);

	ist_ddr_arb_events = 0;

	dma_set_monitor_period();

#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_KTHREAD)
	THREAD_CREATE(m_ddr_arb_tsk_id, ddr_arb_platform_ist, NULL, "ddr_arb_ist");
	THREAD_RESUME(m_ddr_arb_tsk_id);
#endif

	//printk("Arbiter addr 0x%x 0x%x\r\n",
	//	IOADDR_DDR_ARB_REG_BASE,
	//	IOADDR_DDR_ARB2_REG_BASE
	//	);
	//printk("SEM ID: 0x%p\r\n", v_sem[0]);

	//printk("%s: exit\r\n", __func__);
}

void ddr_arb_platform_release_resource(void)
{
#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_KTHREAD)
	set_flg(FLG_ID_TMR_IST, DDR_ARB_IST_FLG_EXIT);
	THREAD_DESTROY(m_ddr_arb_tsk_id);
#endif
	rel_flg(FLG_ID_DDR_ARB);
	rel_flg(FLG_ID_DDR_ARB_IST);
	SEM_DESTROY(SEMID_DDR_ARB);
}
#endif
#endif

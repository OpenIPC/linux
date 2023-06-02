

#ifdef __KERNEL__
#include <mach/fmem.h>
#include "kwrap/cpu.h"
#elif defined(__FREERTOS)

#include <kwrap/spinlock.h>
#include "kwrap/cpu.h"
#include <stdio.h>
#include <string.h>
#endif
#include "jpeg_platform.h"
#include "jpeg.h"


struct clk *p_jpeg_clk;
KDRV_JPEG_TRIG_INFO g_jpeg_trig_info;


#if defined(__FREERTOS)
unsigned int jpeg_debug_level = NVT_DBG_WARN;
static int is_create = 0;

#endif



#ifdef __KERNEL__
//static DEFINE_SPINLOCK(my_lock);

//#define loc_cpu(myflags)   spin_lock_irqsave(&my_lock, myflags)
//#define unl_cpu(myflags)   spin_unlock_irqrestore(&my_lock, myflags)

static spinlock_t jpeg_spin_lock;
#else
//static  VK_DEFINE_SPINLOCK(my_lock);
//#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
//#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

static vk_spinlock_t jpeg_spin_lock;
#endif



// semaphore & interrupt flag id
static SEM_HANDLE SEMID_JPEG;
//static FLGPTN     FLG_ID_JPEG;

static ID FLG_ID_JPEG;

#define FLGPTN_JPEG  FLGPTN_BIT(0)

//BRC control
extern UINT32 g_uiBrcTargetSize[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_uiBrcStdQTableQuality[KDRV_VDOENC_ID_MAX+1];


BOOL jpeg_platform_dma_flush_dev2mem(BOOL decmode, UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWithoutCheck(addr, size);

#elif defined(__FREERTOS)
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);
	return 0;

#elif defined __KERNEL__
	BOOL ret;

	{
		if(decmode) {
			vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);
		} else {
			vos_cpu_dcache_sync_vb((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);
		}
		ret = 0;
	}

	return ret;
#else
    return 0;
#endif
}



BOOL jpeg_platform_dma_flush_mem2dev(BOOL decmode, UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushWriteCacheWithoutCheck(addr, size);

#elif defined(__FREERTOS)

	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_TO_DEVICE);
	return 0;

#elif defined __KERNEL__
	BOOL ret;

	{
		if(decmode) {
			vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_TO_DEVICE);
		} else {
			vos_cpu_dcache_sync_vb((VOS_ADDR)addr, size, VOS_DMA_TO_DEVICE);
		}

		ret = 0;
	}

	return ret;

#else
    return 0;
#endif
}

BOOL jpeg_platform_dma_flush_dev2mem_width_neq_loff(BOOL decmode, UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(addr, size);
#elif defined(__FREERTOS)
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_BIDIRECTIONAL);
	return 0;

#elif defined __KERNEL__
	{
		if(decmode) {
			vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_BIDIRECTIONAL);
		} else {
			vos_cpu_dcache_sync_vb((VOS_ADDR)addr, size, VOS_DMA_BIDIRECTIONAL);
		}

		return 0;
	}

#else
    return 0;

#endif
}

UINT32 jpeg_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size)
{
#if defined __UITRON || defined __ECOS
	return dma_flushReadCacheWithoutCheck(addr, size);

#elif defined(__FREERTOS)
	vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);
	return 0;

#else
	UINT32 ret;

	{
		vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);
		ret = 0;
	}

	return ret;
#endif
}


UINT32 jpeg_platform_dma_is_cacheable(UINT32 addr)
{
#if defined __UITRON || defined __ECOS ||defined __FREERTOS
	return dma_isCacheAddr(addr);
#else
	return 1;
#endif
}


void jpeg_platform_clk_set_freq(UINT32 freq)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
// Set JPEG clock
	switch (freq) {
#if defined(_BSP_NA51055_)
	case PLL_CLKSEL_JPEG_320:
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		}
		break;

	case PLL_CLKSEL_JPEG_PLL9:
		if (pll_getPLLEn(PLL_ID_9) == FALSE) {
			pll_setPLLEn(PLL_ID_9, TRUE);
		}
		break;
#elif defined(_BSP_NA51000_)
	case PLL_CLKSEL_JPEG_PLL6:
		if (pll_getPLLEn(PLL_ID_6) == FALSE) {
			pll_setPLLEn(PLL_ID_6, TRUE);
		}
		break;

	case PLL_CLKSEL_JPEG_PLL13:
		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);
		}
		break;

#endif
	default:
		break;
	}

	pll_setClockRate(PLL_CLKSEL_JPEG, freq);

#elif defined __KERNEL__
	struct clk *parent_clk;

	if (IS_ERR(p_jpeg_clk)) {
		//printk("%s: get clk fail %d\r\n", __func__, (int)(freq));
		printk("%s: get clk fail \r\n", __func__);
		return;
	}

	if (freq < 240) {
		printk("input frequency %d round to 240MHz\r\n", (int)(freq));
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq == 240) {
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq < 320) {
		printk("input frequency %d round to 240MHz\r\n", (int)(freq));
		parent_clk = clk_get(NULL, "fix240m");
	} else if (freq == 320) {
		parent_clk = clk_get(NULL, "pllf320");
	} else if (freq < 480) {
		printk("input frequency %d round to 320MHz\r\n", (int)(freq));
		parent_clk = clk_get(NULL, "pllf320");
	} else if (freq == 480) {
		parent_clk = clk_get(NULL, "fix480m");
	} else {
		printk("input frequency %d round to 320MHz\r\n", (int)(freq));
		parent_clk = clk_get(NULL, "pllf320");
	}

	clk_set_parent(p_jpeg_clk, parent_clk);
	clk_put(parent_clk);
#else

#endif
}

void jpeg_platform_clk_get_freq(UINT32 *p_freq)
{
#if defined __UITRON || defined __ECOS
    UINT32 freq;

		switch (pll_getClockRate(PLL_CLKSEL_JPEG)) {
		case PLL_CLKSEL_JPEG_240:
			return 240;

		case PLL_CLKSEL_JPEG_480:
			return 480;

		case PLL_CLKSEL_JPEG_PLL6:
			freq = pll_getPLLFreq(PLL_ID_6);
			freq += 500000;
			freq /= 1000000;
			return freq;

		default:
		case PLL_CLKSEL_JPEG_PLL13:
			freq = pll_getPLLFreq(PLL_ID_13);
			freq += 500000;
			freq /= 1000000;

			return freq;
		}
#elif defined __KERNEL__
	if (IS_ERR(p_jpeg_clk)) {
		//printk("%s: get clk fail %d\r\n", __func__, (int)(p_jpeg_clk));
		printk("%s: get clk fail \r\n", __func__);
		*p_freq = 0;
		return;
	}
	*p_freq = clk_get_rate(p_jpeg_clk);
#else

#endif
}


void jpeg_platform_clear_flg(void)
{
	clr_flg(FLG_ID_JPEG, FLGPTN_JPEG);
}

void jpeg_platform_set_flg(void)
{
	iset_flg(FLG_ID_JPEG, FLGPTN_JPEG);
}

BOOL jpeg_platform_check_flg(void)
{
	if (kchk_flg(FLG_ID_JPEG, FLGPTN_JPEG) & FLGPTN_JPEG)
		return TRUE;
	else
		return FALSE;
}

void jpeg_platform_wait_flg(void)
{
	FLGPTN              ptn;

//	printk("%s: flg id %d\r\n", __func__, (int)(v_flg_id[id]));
	wai_flg(&ptn, FLG_ID_JPEG, FLGPTN_JPEG, TWF_ORW | TWF_CLR);
//	printk("%s: done\r\n", __func__);
}



ER jpeg_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_JPEG);
#else
	return SEM_WAIT(SEMID_JPEG);
#endif

}

ER jpeg_platform_sem_signal(void)
{

#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_JPEG);
#else
//	printk("%s: id %d, sem 0x%p\r\n", __func__, id, v_sem[id]);
	SEM_SIGNAL(SEMID_JPEG);
	return E_OK;
#endif

}


KDRV_JPEG_TRIG_INFO *kdrv_jpeg_get_triginfo_by_coreid(void)
{
	return &g_jpeg_trig_info;
}


void jpeg_platform_triginfo_init(void)
{
	// init trig info
	memset(&g_jpeg_trig_info, 0x00, sizeof(g_jpeg_trig_info));
    g_jpeg_trig_info.p_queue = kdrv_jpeg_get_queue_by_coreid();
	g_jpeg_trig_info.tri_func = (JPEG_TRI_FUNC) jpeg_trigger;
}



void jpeg_create_resource(void)
{
#ifdef __KERNEL__
	OS_CONFIG_FLAG(FLG_ID_JPEG);
	SEM_CREATE(SEMID_JPEG, 1);
	spin_lock_init(&jpeg_spin_lock);
#else
    if (!is_create) {
		//OS_CONFIG_FLAG(FLG_ID_JPEG);
		cre_flg(&FLG_ID_JPEG,		NULL,	"FLG_ID_JPEG");
		vos_sem_create(&SEMID_JPEG, 1, "SEMID_JPEG");
		is_create = 1;

		vk_spin_lock_init(&jpeg_spin_lock);

        jpeg_platform_triginfo_init();
	}

#endif
}

void jpeg_release_resource(void)
{
#ifdef __KERNEL__
	rel_flg(FLG_ID_JPEG);
	SEM_DESTROY(SEMID_JPEG);

#else
	vos_sem_destroy(SEMID_JPEG);
	is_create = 0;
#endif
}

void jpeg_platform_set_ist_event(void)
{
	/*  Tasklet for bottom half mechanism */
	//tasklet_schedule(jpeg_module_info->jpg_tasklet);
}


void jpeg_isr_bottom(UINT32 events)
{
	KDRV_JPEG_TRIG_INFO *p_trig;
	KDRV_JPEG_QUEUE_ELEMENT   element;
	//unsigned long flag;
	UINT32 spin_flags;
	UINT32 bufsize;

	// Enter critical section
	spin_flags = jpeg_platform_spin_lock();
	p_trig = kdrv_jpeg_get_triginfo_by_coreid();

	if (p_trig->cb == NULL) {
		//printk("callback == NULL\r\n");
	} else {
		//printk("callback != NULL\r\n");
		if(p_trig->jpeg_dec_param == NULL)
		{
			p_trig->jpeg_enc_param->bs_size_1 = jpeg_get_bssize() + p_trig->jpeg_enc_param->svc_hdr_size;
			jpeg_flush_bsbuf(p_trig->jpeg_enc_param->bs_addr_1, p_trig->jpeg_enc_param->bs_size_1);

			if (g_uiBrcTargetSize[p_trig->jpeg_enc_param->temproal_id])
			{
#if 1//JPEG_BRC2
				p_trig->jpeg_enc_param->base_qp = g_uiBrcStdQTableQuality[p_trig->jpeg_enc_param->temproal_id];//g_uiBrcQF[id];
#else
				p_trig->jpeg_enc_param->base_qp = g_uiBrcQF[p_trig->jpeg_enc_param->temproal_id];
#endif
			}

			//Count frame sequence size
			if(p_trig->jpeg_enc_param->vbr_mode)
				jpeg_frame_window_count(p_trig->jpeg_enc_param->temproal_id, p_trig->jpeg_enc_param->bs_size_1);
#if 0//JPEG_DEBUG
#if defined __KERNEL__
			printk("p_enc_param->bs_size_1 = 0x%x\r\n", (unsigned int)(p_trig->jpeg_enc_param->bs_size_1));
#endif
#endif
			p_trig->cb->callback(p_trig->jpeg_enc_param, p_trig->user_data);
		} else {
			bufsize = p_trig->jpeg_dec_param->uiWidth*p_trig->jpeg_dec_param->uiHeight;
			vos_cpu_dcache_sync((UINT32)p_trig->jpeg_dec_param->y_addr, (bufsize*3)>>1, VOS_DMA_FROM_DEVICE);

			if(events==1)
			{
				//CB return not decoded
				p_trig->jpeg_dec_param->errorcode = 0xFFFF;
			}
			p_trig->cb->callback(p_trig->jpeg_dec_param, NULL);
		}
	}

	//loc_cpu(flag);
	if (kdrv_jpeg_queue_is_empty_p(p_trig->p_queue)) {
		p_trig->is_busy = FALSE;
		//unl_cpu(flag);
		jpeg_platform_spin_unlock(spin_flags);
	} else {
		//KDRV_JPEG_QUEUE_ELEMENT   element;

		kdrv_jpeg_queue_del_p(p_trig->p_queue, &element);
		p_trig->cb        = element.p_cb_func;
		//unl_cpu(flag);
		jpeg_platform_spin_unlock(spin_flags);

		if (element.jpeg_mode)
			p_trig->tri_func(1, element.jpeg_dec_param, element.p_cb_func);
		else
			p_trig->tri_func(0, element.jpeg_enc_param, element.p_cb_func);
	}

}



int jpeg_platform_ist(UINT32 event)
{
	jpeg_isr_bottom(event);

	return 0;
}



UINT32 jpeg_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
#elif defined(__FREERTOS)
	unsigned long flags;
	vk_spin_lock_irqsave(&jpeg_spin_lock, flags);
	return flags;
#else
	unsigned long flags;
	spin_lock_irqsave(&jpeg_spin_lock, flags);
	return flags;
#endif
}

void jpeg_platform_spin_unlock(UINT32 flags)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#elif defined(__FREERTOS)
	vk_spin_unlock_irqrestore(&jpeg_spin_lock, flags);
#else
	spin_unlock_irqrestore(&jpeg_spin_lock, flags);
#endif
}

#include "ide_platform.h"
#include "ide2_int.h"

#if defined __UITRON || defined __ECOS

static const DRV_INT_NUM v_ide_int_en[] = {DRV_INT_IDE, DRV_INT_IDE2};
static ID v_ide_flg_id[] = {FLG_ID_IDE, FLG_ID_IDE2};

#elif defined(__FREERTOS)
static ID v_ide_flg_id[IDE_ID_1+1];

static vk_spinlock_t v_ide_spin_locks[IDE_ID_1+1];

static SEM_HANDLE SEMID_IDE[IDE_ID_1+1];

unsigned int ide_debug_level = NVT_DBG_WRN;

UINT32 IOADDR_IDE2_REG_BASE;

#else

UINT32 IOADDR_IDE_REG_BASE;
UINT32 IOADDR_IDE2_REG_BASE;

static ID v_ide_flg_id[IDE_ID_1+1];

static spinlock_t v_ide_spin_locks[IDE_ID_1+1];

static struct clk *ide_clk[IDE_ID_1+1];
static struct clk *ide_if_clk[IDE_ID_1+1];
static SEM_HANDLE SEMID_IDE[IDE_ID_1+1];
static struct tasklet_struct * v_p_ide_tasklet[IDE_ID_1+1];

#endif

#define IDE_REQ_POLL_SIZE	16
static IDE_REQ_LIST_NODE v_req_pool[IDE_ID_1+1][IDE_REQ_POLL_SIZE];
static UINT32 v_req_front[IDE_ID_1+1];
static UINT32 v_req_tail[IDE_ID_1+1];

PINMUX_LCDINIT ide_platform_get_disp_mode(UINT32 pin_func_id)
{
#if defined __UITRON || defined __ECOS
	return pinmux_getDispMode((PINMUX_FUNC_ID)pin_func_id);
#else
	return pinmux_get_dispmode((PINMUX_FUNC_ID) pin_func_id);
#endif
}

void ide_platform_delay_ms(UINT32 ms)
{
#if defined __UITRON || defined __ECOS
	Delay_DelayMs(ms);
#elif defined __FREERTOS
	delay_us(ms * 1000);
#else
	msleep(ms);
	//mdelay(ms);
#endif
}

void ide_platform_delay_us(UINT32 us)
{
#if defined __UITRON || defined __ECOS
	Delay_DelayUs(us);
#elif defined __FREERTOS
	delay_us(us);
#else
	ndelay(1000 * us);
#endif
}

ER ide_platform_flg_clear(IDE_ID id, FLGPTN flg)
{
	return clr_flg(v_ide_flg_id[id], flg);
}

ER ide_platform_flg_set(IDE_ID id, FLGPTN flg)
{
	 return iset_flg(v_ide_flg_id[id], flg);
}

ER ide_platform_flg_wait(IDE_ID id, FLGPTN flg)
{
	FLGPTN              ui_flag;

	return wai_flg(&ui_flag, v_ide_flg_id[id], flg, TWF_ORW | TWF_CLR);
}

ER ide_platform_sem_set(IDE_ID id)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_IDE[id]);
#else
	SEM_SIGNAL(SEMID_IDE[id]);
	return E_OK;
#endif
}

ER ide_platform_sem_wait(IDE_ID id)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_IDE[id]);
#else
	return SEM_WAIT(SEMID_IDE[id]);
#endif
}


UINT32 ide_platform_spin_lock(IDE_ID id)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
	return 0;
#elif defined __FREERTOS
	unsigned long flag;
	vk_spin_lock_irqsave(&v_ide_spin_locks[id], flag);
	return flag;
#else
	unsigned long flag;
	spin_lock_irqsave(&v_ide_spin_locks[id], flag);
	return flag;
#endif
}

void ide_platform_spin_unlock(IDE_ID id, UINT32 flag)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#elif defined __FREERTOS
	vk_spin_unlock_irqrestore(&v_ide_spin_locks[id], flag);
#else
	spin_unlock_irqrestore(&v_ide_spin_locks[id], flag);
#endif
}

void ide_platform_sram_enable(IDE_ID id)
{
#if defined __UITRON || defined __ECOS
	if (id == IDE_ID_1)
		pinmux_disable_sram_shutdown(IDE_SD);
	else
		;//pll_disableSramShutDown(IDE2_RSTN);
#elif defined __FREERTOS
	nvt_enable_sram_shutdown(IDE_SD);
#else
#endif
}

void ide_platform_sram_disable(IDE_ID id)
{
#if defined __UITRON || defined __ECOS
	if (id == IDE_ID_1)
		pinmux_enable_sram_shutdown(IDE_SD);
	else
		;//pll_enableSramShutDown(IDE2_RSTN);
#elif defined __FREERTOS
	nvt_disable_sram_shutdown(IDE_SD);
#else
#endif
}

void ide_platform_int_enable(IDE_ID id)
{
#if defined __UITRON || defined __ECOS
	drv_enableInt(v_ide_int_en[id]);
#else
#endif
}

void ide_platform_int_disable(IDE_ID id)
{
#if defined __UITRON || defined __ECOS
	drv_disableInt(v_ide_int_en[id]);
#else
#endif
}

UINT32 ide_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return dma_getPhyAddr(addr);
#else
	return fmem_lookup_pa(addr);
#endif
}

UINT32 factor_caculate(UINT16 x, UINT16 y, BOOL h)
{
	UINT32 a, b, c;
	UINT64 temp;

	if (h == TRUE) {

		a = (x - 1) << 15;
		b = (y - 1);

		temp = (UINT64) a;
#if defined __UITRON || defined __ECOS || defined __FREERTOS
		temp = temp/b;
#else
		do_div(temp, b);
#endif
		c = (UINT32) temp;
	} else {
		a = (x - 1) << 12;
		b = (y - 1);
		temp = (UINT64) a;
#if defined __UITRON || defined __ECOS || defined __FREERTOS
		temp = temp/b;
#else
		do_div(temp, b);
#endif
		c = (UINT32) temp;
	}

	//c = temp - (1 << 15);

	return c;
}

void ide_platform_clk_en(IDE_ID id)
{
	DBG_IND("ide clk enable\n");
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id == IDE_ID_1)
		pll_enable_clock(IDE1_CLK);
	else
		DBG_ERR("not support IDE%d\n", id);
#else
	clk_prepare(ide_clk[id]);

	clk_enable(ide_clk[id]);
#endif
	DBG_IND("ide clk enable finished\n");
}

void ide_platform_clk_dis(IDE_ID id)
{
	DBG_IND("ide clk disable\n");
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id == IDE_ID_1)
		pll_disable_clock(IDE1_CLK);
	else
		DBG_ERR("not support IDE%d\n", id);
#else
	clk_disable(ide_clk[id]);

	clk_unprepare(ide_clk[id]);
#endif
	DBG_IND("ide clk disable finished\n");
}

void ide_platform_set_iffreq(IDE_ID id, UINT32 freq)
{
	DBG_IND("ide if clk %d Hz\n", (int)freq);
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id == IDE_ID_1)
		pll_set_clock_freq(IDEOUTIFCLK_FREQ, freq);
	else
		DBG_ERR("not support IDE%d\n", id);
#else
	clk_set_rate(ide_if_clk[id], freq);
#endif
}

void ide_platform_set_freq(IDE_ID id, UINT32 freq)
{
	DBG_IND("ide clk %d Hz\n", (int)freq);
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id == IDE_ID_1)
		pll_set_clock_freq(IDECLK_FREQ, freq);
	else
		DBG_ERR("not support IDE%d\n", id);
#else
	clk_set_rate(ide_clk[id], freq);
#endif
}

UINT32 ide_platform_get_iffreq(IDE_ID id)
{
	UINT32 rate = 0;

#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id == IDE_ID_1)
		pll_get_clock_freq(IDEOUTIFCLK_FREQ, &rate);
	else
		DBG_ERR("not support IDE%d\n", id);

#else
	rate = clk_get_rate(ide_if_clk[id]);
#endif
	DBG_IND("ide if clk %d Hz\n", (int)rate);

	return rate;
}

UINT32 ide_platform_get_freq(IDE_ID id)
{
	UINT32 rate = 0;

#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id == IDE_ID_1)
		pll_get_clock_freq(IDECLK_FREQ, &rate);
	else
		DBG_ERR("not support IDE%d\n", id);
#else
	rate = clk_get_rate(ide_clk[id]);
#endif
	DBG_IND("ide clk %d Hz\n", (int)rate);

	return rate;
}

ER ide_platform_set_clksrc(IDE_ID id, UINT32 src)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
#else
	struct clk *source_clk;

	if (src == 0) {
		source_clk = clk_get(NULL, "fix480m");
		if (IS_ERR(source_clk)) {
			DBG_ERR("ide get clk source err\n");
			return E_SYS;
		}
		clk_set_parent(ide_clk[id], source_clk);
		clk_set_parent(ide_if_clk[id], source_clk);
	} else if (src == 1) {
		source_clk = clk_get(NULL, "pll6");
		if (IS_ERR(source_clk)) {
			DBG_ERR("ide get clk source err\n");
			return E_SYS;
		}
		clk_set_parent(ide_clk[id], source_clk);
		clk_set_parent(ide_if_clk[id], source_clk);
	} else if (src == 2) {
		source_clk = clk_get(NULL, "pll4");
		if (IS_ERR(source_clk)) {
			DBG_ERR("ide get clk source err\n");
			return E_SYS;
		}
		clk_set_parent(ide_clk[id], source_clk);
		clk_set_parent(ide_if_clk[id], source_clk);
	} else if (src == 3) {
		source_clk = clk_get(NULL, "pll9");
		if (IS_ERR(source_clk)) {
			DBG_ERR("ide get clk source err\n");
			return E_SYS;
		}
		clk_set_parent(ide_clk[id], source_clk);
		clk_set_parent(ide_if_clk[id], source_clk);
	} else {
		DBG_ERR("ide clk source %d not support\n", src);
		return E_SYS;
	}
#endif
	return E_OK;
}

/*
        Check if service queue is empty
*/
BOOL ide_platform_list_empty(IDE_ID id)
{
	if (id > IDE_ID_1) {
                DBG_ERR("invalid id %d\r\n", id);
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
ER ide_platform_add_list(IDE_ID id, KDRV_CALLBACK_FUNC *p_callback)
{
	UINT32 next;
	const UINT32 tail = v_req_tail[id];

	if (id > IDE_ID_1) {
		DBG_ERR("invalid id %d\r\n", id);
		return E_SYS;
	}

	next = (tail+1) % IDE_REQ_POLL_SIZE;
	//printk("%s: next %d\r\n", __func__, next);

	if (next == v_req_front[id]) {
		// queue full
		DBG_ERR("queue full, front %d, tail %d\r\n", (int)v_req_front[id], (int)tail);
		return E_SYS;
	}

	if (p_callback) {
		memcpy(&v_req_pool[id][tail].callback,
			p_callback,
			sizeof(KDRV_CALLBACK_FUNC));
	} else {
		memset(&v_req_pool[id][tail].callback,
                        0,
                        sizeof(KDRV_CALLBACK_FUNC));
	}

	v_req_tail[id] = next;

	return E_OK;
}

/*
	Get head request descriptor from service queue
*/
IDE_REQ_LIST_NODE* ide_platform_get_head(IDE_ID id)
{
	IDE_REQ_LIST_NODE *p_node;

	p_node = &v_req_pool[id][v_req_front[id]];

	if (id > IDE_ID_1) {
		DBG_ERR("invalid id %d\r\n", id);
		return NULL;
	}

	if (v_req_front[id] == v_req_tail[id]) {
		// queue empty
		DBG_ERR("queue empty\r\n");
		return NULL;
	}

	return p_node;

//	memcpy(p_param, &p_node->trig_param, sizeof(KDRV_GRPH_TRIGGER_PARAM));

//	return E_OK;
}

/*
	Delete request descriptor from service queue
*/
ER ide_platform_del_list(IDE_ID id)
{
	if (id > IDE_ID_1) {
		DBG_ERR("invalid id %d\r\n", id);
		return E_SYS;
	}

	if (v_req_front[id] == v_req_tail[id]) {
		DBG_ERR("queue already empty, front %d, tail %d\r\n", (int)v_req_front[id], (int)v_req_tail[id]);
		return E_SYS;
	}

	v_req_front[id] = (v_req_front[id]+1) % IDE_REQ_POLL_SIZE;

	return E_OK;
}


void ide_platform_set_ist_event(IDE_ID id)
{
	/*  Tasklet for bottom half mechanism */
#if defined __KERNEL__
        tasklet_schedule(v_p_ide_tasklet[id]);
#endif
}

int ide_platform_ist(IDE_ID id, UINT32 event)
{
	idec_isr_bottom(id, event);

	return 0;
}

#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
static int is_create = 0;
irqreturn_t ide_platform_isr(int irq, void *devid)
{
	ide_isr();
	return IRQ_HANDLED;
}
void ide_platform_create_resource(void)
{
	if (!is_create) {
		OS_CONFIG_FLAG(v_ide_flg_id[0]);
		SEM_CREATE(SEMID_IDE[0], 1);
		vk_spin_lock_init(&v_ide_spin_locks[0]);

		request_irq(INT_ID_IDE, ide_platform_isr, IRQF_TRIGGER_HIGH, "ide", 0);
		is_create = 1;
	}
}
void ide_platform_release_resource(void)
{
	is_create = 0;
	rel_flg(v_ide_flg_id[0]);
	SEM_DESTROY(SEMID_IDE[0]);
}
#else
void ide_platform_create_resource(MODULE_INFO *pmodule_info)
{
	IOADDR_IDE_REG_BASE = (UINT32)pmodule_info->io_addr[0];
	OS_CONFIG_FLAG(v_ide_flg_id[0]);
	SEM_CREATE(SEMID_IDE[0], 1);
	spin_lock_init(&v_ide_spin_locks[0]);

	ide_clk[0] = pmodule_info->pclk[0];
	ide_if_clk[0] = pmodule_info->ifclk[0];

	v_p_ide_tasklet[IDE_ID_1] = &pmodule_info->ide_tasklet[IDE_ID_1];
	v_req_front[IDE_ID_1] = 0;
	v_req_tail[IDE_ID_1] = 0;

	//printk("ide addr 0x%x\r\n",
	//	IOADDR_IDE_REG_BASE);
}

void ide_platform_release_resource(void)
{
	rel_flg(v_ide_flg_id[0]);
	SEM_DESTROY(SEMID_IDE[0]);
}

EXPORT_SYMBOL(ide_platform_set_clksrc);
EXPORT_SYMBOL(ide_platform_set_freq);
EXPORT_SYMBOL(ide_platform_set_iffreq);
EXPORT_SYMBOL(ide_platform_get_freq);
EXPORT_SYMBOL(ide_platform_get_iffreq);
EXPORT_SYMBOL(ide_platform_clk_en);
EXPORT_SYMBOL(ide_platform_clk_dis);
#endif
#endif



#include "sie_platform.h"

// debug level
#if defined(__FREERTOS)
unsigned int sie_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
#else
static struct clk* sie_clk[MODULE_CLK_NUM];
static struct clk* sie_mclk[MODULE_CLK_NUM];
static struct clk* sie_pxclk[MODULE_CLK_NUM];
static struct clk* sie_io_pxclk[MODULE_CLK_NUM];
#endif

static VK_DEFINE_SPINLOCK(sie_spin_lock);
static VK_DEFINE_SPINLOCK(sie_spin_lock2);
//static unsigned long sie_spin_flags = 0, sie_spin_flags2 = 0;
static SIE_CLKSRC_SEL g_sieClkSrc[MODULE_CLK_NUM]={SIE_CLKSRC_480,SIE_CLKSRC_480,SIE_CLKSRC_480};
static UINT32 g_sieClkRate[MODULE_CLK_NUM]={240000000,240000000,240000000};

UINT32 _SIE_REG_BASE_ADDR_SET[MODULE_CLK_NUM];

static SEM_HANDLE SEMID_SIE;
static ID     FLG_ID_SIE;
static SEM_HANDLE SEMID_SIE2;
static ID     FLG_ID_SIE2;
static SEM_HANDLE SEMID_SIE3;
static ID     FLG_ID_SIE3;

void sie_platform_disable_sram_shutdown(SIE_ENGINE_ID id)
{
	switch (id) {
	case SIE_ENGINE_ID_1:
		nvt_disable_sram_shutdown(SIE_SD);
		break;

	case SIE_ENGINE_ID_2:
		nvt_disable_sram_shutdown(SIE2_SD);
		break;

	case SIE_ENGINE_ID_3:
		nvt_disable_sram_shutdown(SIE3_SD);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}
}

UINT32 sie_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size, UINT32 type)
{
	if (type == 0)
		vos_cpu_dcache_sync(addr, ALIGN_CEIL(size, VOS_ALIGN_BYTES), VOS_DMA_FROM_DEVICE);
	else
		vos_cpu_dcache_sync_vb(addr, ALIGN_CEIL(size, VOS_ALIGN_BYTES), VOS_DMA_FROM_DEVICE);
	return 0;
}

UINT32 sie_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync(addr, ALIGN_CEIL(size, VOS_ALIGN_BYTES), VOS_DMA_TO_DEVICE);
    //nvt_dbg(ERR, "size = %d, ALIGN_CEIL(size, VOS_ALIGN_BYTES) = %d\r\n",size, ALIGN_CEIL(size, VOS_ALIGN_BYTES));
	return 0;
}

UINT32 sie_platform_va2pa(UINT32 addr)
{
	VOS_ADDR phy_adr;

	phy_adr = vos_cpu_get_phy_addr(addr);
	if (phy_adr == VOS_ADDR_INVALID) {
		nvt_dbg(ERR,"addr(0x%.8x) conv to phy addr fail\r\n", (int)addr);
		return 0;
	}
	return phy_adr;
}

void sie_platform_enable_sram_shutdown(SIE_ENGINE_ID id)
{
	switch (id) {
	case SIE_ENGINE_ID_1:
		nvt_enable_sram_shutdown(SIE_SD);
		break;

	case SIE_ENGINE_ID_2:
		nvt_enable_sram_shutdown(SIE2_SD);
		break;

	case SIE_ENGINE_ID_3:
		nvt_enable_sram_shutdown(SIE3_SD);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}
}

void sie_platform_prepare_clk(SIE_ENGINE_ID id)
{
#if defined(__FREERTOS)
#else
	clk_prepare(sie_clk[id]);
	if (id <= 3) { // sie5 not support sie_pxclk
		clk_prepare(sie_pxclk[id]);
	}
	if ((id <= 1) || (id ==3)) { // sie3 & sie5 not support sie_io_pxclk
		clk_prepare(sie_io_pxclk[id]);
	}
	if (id <= 2) {
		clk_prepare(sie_mclk[id]);
	}
#endif
}

void sie_platform_unprepare_clk(SIE_ENGINE_ID id)
{
#if defined(__FREERTOS)
#else
	//clk_unprepare(sie_clk[id]);
	//clk_unprepare(sie_pxclk[id]);
	//if (id<=1)
	//	clk_unprepare(sie_io_pxclk[id]);
#endif
}

void sie_platform_enable_clk(SIE_ENGINE_ID id)
{
#if defined(__FREERTOS)
	switch (id) {
	case SIE_ENGINE_ID_1:
		pll_enableClock(SIE_CLK);
		break;

	case SIE_ENGINE_ID_2:
		pll_enableClock(SIE2_CLK);
		break;

	case SIE_ENGINE_ID_3:
		pll_enableClock(SIE3_CLK);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}
#else
	clk_enable(sie_clk[id]);
	if (id<=3) {
		clk_enable(sie_pxclk[id]);
	}
	//clk_enable(sie_mclk[id]);// move to sie_setmclock
#endif
}

void sie_platform_disable_clk(SIE_ENGINE_ID id)
{
#if defined(__FREERTOS)
	SIE_CLKSRC_SEL clksrc = SIE_CLKSRC_480;
	UINT32 clkrate;

	switch (id) {
	case SIE_ENGINE_ID_1:
		pll_disableClock(SIE_CLK);
		sie_getClock(SIE_ENGINE_ID_1,&clksrc, &clkrate);
		break;

	case SIE_ENGINE_ID_2:
		pll_disableClock(SIE2_CLK);
		sie_getClock(SIE_ENGINE_ID_2,&clksrc, &clkrate);
		break;

	case SIE_ENGINE_ID_3:
		pll_disableClock(SIE3_CLK);
		sie_getClock(SIE_ENGINE_ID_3,&clksrc, &clkrate);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}

	switch (clksrc) {
	case SIE_CLKSRC_PLL5:
		pll_setPLLEn(PLL_ID_5, FALSE);
		break;

	case SIE_CLKSRC_PLL12:
		pll_setPLLEn(PLL_ID_12, FALSE);
		break;

	default:
		break;
	}
#else
	clk_disable(sie_clk[id]);
	if (id<=3) {
		clk_disable(sie_pxclk[id]);
	}
#endif
}

ER sie_platform_sem_wait(SIE_ENGINE_ID id)
{
	ER erReturn = E_OK;

	switch (id) {
	case SIE_ENGINE_ID_1:
		erReturn = SEM_WAIT(SEMID_SIE);
		break;

	case SIE_ENGINE_ID_2:
		erReturn = SEM_WAIT(SEMID_SIE2);
		break;

	case SIE_ENGINE_ID_3:
		erReturn = SEM_WAIT(SEMID_SIE3);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}

	return erReturn;
}

ER sie_platform_sem_signal(SIE_ENGINE_ID id)
{
	switch (id) {
	case SIE_ENGINE_ID_1:
		SEM_SIGNAL(SEMID_SIE);
		break;

	case SIE_ENGINE_ID_2:
		SEM_SIGNAL(SEMID_SIE2);
		break;

	case SIE_ENGINE_ID_3:
		SEM_SIGNAL(SEMID_SIE3);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}
	return E_OK;
}

ER sie_platform_flg_clear(SIE_ENGINE_ID id, FLGPTN flg)
{
	switch (id) {
	case SIE_ENGINE_ID_1:
		vos_flag_clr(FLG_ID_SIE, flg);
		break;

	case SIE_ENGINE_ID_2:
		vos_flag_clr(FLG_ID_SIE2, flg);
		break;

	case SIE_ENGINE_ID_3:
		vos_flag_clr(FLG_ID_SIE3, flg);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\r\n", (int)id);
		break;
	}

	return E_OK;
}

ER sie_platform_flg_set(SIE_ENGINE_ID id, FLGPTN flg)
{
	switch (id) {
	case SIE_ENGINE_ID_1:
		vos_flag_iset(FLG_ID_SIE, flg);
		break;

	case SIE_ENGINE_ID_2:
		vos_flag_iset(FLG_ID_SIE2, flg);
		break;

	case SIE_ENGINE_ID_3:
		vos_flag_iset(FLG_ID_SIE3, flg);
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}

	return E_OK;
}

ER sie_platform_flg_wait(SIE_ENGINE_ID id, PFLGPTN p_flgptn, FLGPTN flg)
{
	switch (id) {
	case SIE_ENGINE_ID_1:
		if (vos_flag_wait_timeout(p_flgptn, FLG_ID_SIE, flg, TWF_CLR|TWF_ORW, vos_util_msec_to_tick(SIE_DRVWAIT_FLG_TIMEOUT_MS)) != 0) {
			nvt_dbg(ERR,"id: %d, wait flag timeout (0x%08x,0x%08x)\r\n",id,(int)p_flgptn,(int)flg);
		}
		break;

	case SIE_ENGINE_ID_2:
		if (vos_flag_wait_timeout(p_flgptn, FLG_ID_SIE2, flg, TWF_CLR|TWF_ORW, vos_util_msec_to_tick(SIE_DRVWAIT_FLG_TIMEOUT_MS)) != 0) {
			nvt_dbg(ERR,"id: %d, wait flag timeout (0x%08x,0x%08x)\r\n",id,(int)p_flgptn,(int)flg);
		}
		break;

	case SIE_ENGINE_ID_3:
		if (vos_flag_wait_timeout(p_flgptn, FLG_ID_SIE3, flg, TWF_CLR|TWF_ORW, vos_util_msec_to_tick(SIE_DRVWAIT_FLG_TIMEOUT_MS)) != 0) {
			nvt_dbg(ERR,"id: %d, wait flag timeout (0x%08x,0x%08x)\r\n",id,(int)p_flgptn,(int)flg);
		}
		break;

	default:
		nvt_dbg(ERR,"id = %d overflow\r\n", (int)id);
		break;
	}
	return E_OK;
}

unsigned long sie_platform_spin_lock(UINT32 type)
{
	unsigned long sie_spin_flags = 0;
	
	switch (type) {
	case 0:	//sie_int 1
		vk_spin_lock_irqsave(&sie_spin_lock, sie_spin_flags);
		break;

	case 1:	//sie_int 2
		vk_spin_lock_irqsave(&sie_spin_lock2, sie_spin_flags);
		break;

	default:
		nvt_dbg(ERR,"type = %d overflow\r\n", (int)type);
		break;
	}

	return sie_spin_flags;
}

void sie_platform_spin_unlock(UINT32 type, unsigned long flags)
{
	//unsigned long sie_spin_flags, sie_spin_flags2;
	
	switch (type) {
	case 0:	//sie_int 1
		vk_spin_unlock_irqrestore(&sie_spin_lock, flags);
		break;

	case 1:	//sie_int 2
		vk_spin_unlock_irqrestore(&sie_spin_lock2, flags);
		break;

	default:
		nvt_dbg(ERR,"type = %d overflow\r\n", (int)type);
		break;
	}
}

#if defined(__FREERTOS)
irqreturn_t sie1_platform_isr(int irq, void *devid)
{
	sie_isr(SIE_ENGINE_ID_1);
	return IRQ_HANDLED;
}
irqreturn_t sie2_platform_isr(int irq, void *devid)
{
	sie_isr(SIE_ENGINE_ID_2);
	return IRQ_HANDLED;
}
irqreturn_t sie3_platform_isr(int irq, void *devid)
{
	sie_isr(SIE_ENGINE_ID_3);
	return IRQ_HANDLED;
}

void sie_platform_create_resource(void)
{
	vos_flag_create(&FLG_ID_SIE, NULL, "sie");
	vos_sem_create(&SEMID_SIE, 1, "SEM_ID_SIE");

	vos_flag_create(&FLG_ID_SIE2, NULL, "sie2");
	vos_sem_create(&SEMID_SIE2, 1, "SEM_ID_SIE2");

	vos_flag_create(&FLG_ID_SIE3, NULL, "sie3");
	vos_sem_create(&SEMID_SIE3, 1, "SEM_ID_SIE3");


	_SIE_REG_BASE_ADDR_SET[SIE_ENGINE_ID_1] = IOADDR_SIE_REG_BASE;
	_SIE_REG_BASE_ADDR_SET[SIE_ENGINE_ID_2] = 0xF0D20000;
	_SIE_REG_BASE_ADDR_SET[SIE_ENGINE_ID_3] = 0xF0D30000;

	request_irq(INT_ID_SIE, sie1_platform_isr, IRQF_TRIGGER_HIGH, "sie1", 0);
	request_irq(INT_ID_SIE2, sie2_platform_isr, IRQF_TRIGGER_HIGH, "sie2", 0);
	request_irq(INT_ID_SIE3, sie3_platform_isr, IRQF_TRIGGER_HIGH, "sie3", 0);
}
#else
void sie_platform_create_resource(SIE_MODULE_INFO *pmodule_info)
{
	vos_flag_create(&FLG_ID_SIE, NULL, "sie");
	vos_sem_create(&SEMID_SIE, 1, "SEM_ID_SIE");

	vos_flag_create(&FLG_ID_SIE2, NULL, "sie2");
	vos_sem_create(&SEMID_SIE2, 1, "SEM_ID_SIE2");

	vos_flag_create(&FLG_ID_SIE3, NULL, "sie3");
	vos_sem_create(&SEMID_SIE3, 1, "SEM_ID_SIE3");


	_SIE_REG_BASE_ADDR_SET[SIE_ENGINE_ID_1] = (UINT32)pmodule_info->io_addr[SIE_ENGINE_ID_1];
	_SIE_REG_BASE_ADDR_SET[SIE_ENGINE_ID_2] = (UINT32)pmodule_info->io_addr[SIE_ENGINE_ID_2];
	_SIE_REG_BASE_ADDR_SET[SIE_ENGINE_ID_3] = (UINT32)pmodule_info->io_addr[SIE_ENGINE_ID_3];


	sie_clk[SIE_ENGINE_ID_1] = clk_get(NULL, "f0c00000.sie");
	sie_clk[SIE_ENGINE_ID_2] = clk_get(NULL, "f0d20000.sie");
	sie_clk[SIE_ENGINE_ID_3] = clk_get(NULL, "f0d30000.sie");

	sie_mclk[SIE_ENGINE_ID_1] = clk_get(NULL, "f0c00000.siemck");
	sie_mclk[SIE_ENGINE_ID_2] = clk_get(NULL, "f0c00000.siemk2");
	sie_mclk[SIE_ENGINE_ID_3] = clk_get(NULL, "f0c00000.siemk3");

	sie_pxclk[SIE_ENGINE_ID_1] = clk_get(NULL, "sie1_pxclk");
	sie_pxclk[SIE_ENGINE_ID_2] = clk_get(NULL, "sie2_pxclk");
	sie_pxclk[SIE_ENGINE_ID_3] = clk_get(NULL, "sie3_pxclk");

	sie_io_pxclk[SIE_ENGINE_ID_1] = clk_get(NULL, "sie_io_pxclk");
	sie_io_pxclk[SIE_ENGINE_ID_2] = clk_get(NULL, "sie2_io_pxclk");

	//nvt_dbg(IND, "CLK GET done\r\n");
}
#endif
void sie_platform_release_resource(void)
{
	vos_sem_destroy(FLG_ID_SIE);
	vos_sem_destroy(SEMID_SIE);

	vos_sem_destroy(FLG_ID_SIE2);
	vos_sem_destroy(SEMID_SIE2);

	vos_sem_destroy(FLG_ID_SIE3);
	vos_sem_destroy(SEMID_SIE3);
}

void sie_platform_set_clk_rate(SIE_ENGINE_ID id, SIE_OPENOBJ *pObjCB)
{
    sie_setClock(id, pObjCB->SieClkSel, pObjCB->uiSieClockRate);
	sie_setPxClock(id, pObjCB->PxClkSel);
}

static UINT32 sensor_lcm(UINT32 a, UINT32 b)
{
	UINT64 input_a, input_b;
	UINT64 tmp;

	if (a == 0) {
		a++;
	}

	if (b == 0) {
		b++;
	}

	input_a = a;
	input_b = b;

	while (b != 0) {
		tmp = a % b;
		a = b;
		b = tmp;
	}

	tmp = input_a * input_b;

#if defined(__FREERTOS)
	tmp = tmp/a;
	tmp = (UINT32)tmp;
#else
	do_div(tmp, a);
#endif

	return tmp;
}

void sensor_set_pll5_rate(UINT32 rate)
{
#if defined(__FREERTOS)
	UINT64 tmp, rate_64bit = rate;
	tmp = rate_64bit*131072/12000000;
	pll_setPLL(PLL_ID_5, (UINT32)tmp);
#else
	struct clk *pll_clk;

	pll_clk = clk_get(NULL, "pll5");

	if (!IS_ERR(pll_clk)) {
		clk_set_rate(pll_clk, rate);
	}

	clk_put(pll_clk);
#endif
}

UINT32 sensor_get_pll5_rate(void)
{
#if defined(__FREERTOS)
	pll_getPLLFreq(PLL_ID_5);
	return 0;
#else
	struct clk *pll_clk;
	UINT32 ret;
	pll_clk = clk_get(NULL, "pll5");

	if (!IS_ERR(pll_clk)) {
		ret = clk_get_rate(pll_clk);
		clk_put(pll_clk);
		return ret;
	} else {
		return 0;
	}
#endif
}
void sensor_set_pll10_rate(UINT32 rate)
{
#if defined(__FREERTOS)
	UINT64 tmp, rate_64bit = rate;
	tmp = rate_64bit*131072/12000000;
	pll_setPLL(PLL_ID_10, (UINT32)tmp);
#else
	struct clk *pll_clk;

	pll_clk = clk_get(NULL, "pll10");

	if (!IS_ERR(pll_clk)) {
		clk_set_rate(pll_clk, rate);
	}

	clk_put(pll_clk);
#endif
}

UINT32 sensor_get_pll10_rate(void)
{
#if defined(__FREERTOS)
	pll_getPLLFreq(PLL_ID_10);
	return 0;
#else
	struct clk *pll_clk;
	UINT32 ret;
	pll_clk = clk_get(NULL, "pll10");

	if (!IS_ERR(pll_clk)) {
		ret = clk_get_rate(pll_clk);
		clk_put(pll_clk);
		return ret;
	} else {
		return 0;
	}
#endif
}
void sensor_set_pll12_rate(UINT32 rate)
{
#if defined(__FREERTOS)
	UINT64 tmp, rate_64bit = rate;
	tmp = rate_64bit*131072/12000000;
	pll_setPLL(PLL_ID_12, (UINT32)tmp);
#else
	struct clk *pll_clk;

	pll_clk = clk_get(NULL, "pll12");

	if (!IS_ERR(pll_clk)) {
		clk_set_rate(pll_clk, rate);
	}

	clk_put(pll_clk);
#endif
}

UINT32 sensor_get_pll12_rate(void)
{
#if defined(__FREERTOS)
	pll_getPLLFreq(PLL_ID_12);
	return 0;
#else
	struct clk *pll_clk;
	UINT32 ret;
	pll_clk = clk_get(NULL, "pll12");

	if (!IS_ERR(pll_clk)) {
		ret = clk_get_rate(pll_clk);
		clk_put(pll_clk);
		return ret;
	} else {
		return 0;
	}
#endif
}

ER sensor_register_pll5_rate(UINT32 rate, UINT32 min_sie_freq)
{
	UINT32 pll5_rate, tmp_rate;
	UINT32 i = 0, j = 2;
	static UINT32 sensor_rate_record[8], count = 0;
	static UINT32 prev_pll5_rate = 0, largest_sie_freq = 0;

	//nvt_dbg(ERR,"sensor_register_pll5_rate, rate = %d, min_sie_freq = %d\r\n",rate, min_sie_freq);

	if (rate >= PLL5_DRV_MAX_FREQ) {
		sensor_set_pll5_rate(PLL5_DRV_MAX_FREQ);
		count = 0;
		return E_OK;
	}

	/*At most 8 sensors are supported at the same time*/
	if (count >= 8) {
		nvt_dbg(ERR,"At most %d sensors are supported\r\n", (int)8);
		return E_NOSPT;
	}

	/*Only different rate are acceptable*/
	if (count) {
		for (i = 0; i < count; i++) {
			if (sensor_rate_record[i] == rate)
				break;
		}
	}

	if (i == count) {
		sensor_rate_record[count] = rate;
		count++;
	}

	pll5_rate = sensor_rate_record[0];

	for (i = 1; i < count; i++) {
		pll5_rate = sensor_lcm(pll5_rate, sensor_rate_record[i]);
	}

	if (IGNORE_SIE_DRV_MIN_FREQ == min_sie_freq) {
		min_sie_freq = largest_sie_freq;
	}

	/*If the LCM of sensor rate is 1, it means the only thing required
	is to update pll5 rate, and which the correct value should be min_sie_freq.
	This state only happens with the first and all elements of sensor rate are 1*/
	if (min_sie_freq) {
		if (pll5_rate != 1) {
			tmp_rate = pll5_rate;

			while ((pll5_rate < min_sie_freq) || (pll5_rate < prev_pll5_rate)) {
				pll5_rate = tmp_rate * j;
				j++;
			}
		} else {
			if (prev_pll5_rate) {
				pll5_rate = prev_pll5_rate;
			} else {
				pll5_rate = min_sie_freq;
			}
		}
	}

	if (pll5_rate > PLL5_DRV_MAX_FREQ) {
		nvt_dbg(ERR,"pll_5 rate overflow %d\r\n", (int)PLL5_DRV_MAX_FREQ);
		return E_OBJ;
	}

	sensor_set_pll5_rate(pll5_rate);

	prev_pll5_rate = pll5_rate;

	if (min_sie_freq > largest_sie_freq) {
		largest_sie_freq = min_sie_freq;
	}

	return E_OK;
}
ER sensor_register_pll10_rate(UINT32 rate, UINT32 min_sie_freq)
{
	UINT32 pll10_rate, tmp_rate;
	UINT32 i = 0, j = 2;
	static UINT32 sensor_rate_record[8], count = 0;
	static UINT32 prev_pll10_rate = 0, largest_sie_freq = 0;

	if (rate >= PLL10_DRV_MAX_FREQ) {
		sensor_set_pll10_rate(PLL10_DRV_MAX_FREQ);
		count = 0;
		return E_OK;
	}

	/*At most 8 sensors are supported at the same time*/
	if (count >= 8) {
		nvt_dbg(ERR,"At most %d sensors are supported\r\n", (int)8);
		return E_NOSPT;
	}

	/*Only different rate are acceptable*/
	if (count) {
		for (i = 0; i < count; i++) {
			if (sensor_rate_record[i] == rate)
				break;
		}
	}

	if (i == count) {
		sensor_rate_record[count] = rate;
		count++;
	}

	pll10_rate = sensor_rate_record[0];

	for (i = 1; i < count; i++) {
		pll10_rate = sensor_lcm(pll10_rate, sensor_rate_record[i]);
	}

	if (IGNORE_SIE_DRV_MIN_FREQ == min_sie_freq) {
		min_sie_freq = largest_sie_freq;
	}

	/*If the LCM of sensor rate is 1, it means the only thing required
	is to update pll10 rate, and which the correct value should be min_sie_freq.
	This state only happens with the first and all elements of sensor rate are 1*/
	if (min_sie_freq) {
		if (pll10_rate != 1) {
			tmp_rate = pll10_rate;

			while ((pll10_rate < min_sie_freq) || (pll10_rate < prev_pll10_rate)) {
				pll10_rate = tmp_rate * j;
				j++;
			}
		} else {
			if (prev_pll10_rate) {
				pll10_rate = prev_pll10_rate;
			} else {
				pll10_rate = min_sie_freq;
			}
		}
	}

	if (pll10_rate > PLL10_DRV_MAX_FREQ) {
		nvt_dbg(ERR,"pll_10 rate overflow %d\r\n", (int)PLL10_DRV_MAX_FREQ);
		return E_OBJ;
	}

	sensor_set_pll10_rate(pll10_rate);

	prev_pll10_rate = pll10_rate;

	if (min_sie_freq > largest_sie_freq) {
		largest_sie_freq = min_sie_freq;
	}

	return E_OK;
}
ER sensor_register_pll12_rate(UINT32 rate, UINT32 min_sie_freq)
{
	UINT32 pll12_rate, tmp_rate;
	UINT32 i = 0, j = 2;
	static UINT32 sensor_rate_record[8], count = 0;
	static UINT32 prev_pll12_rate = 0, largest_sie_freq = 0;

	if (rate >= PLL12_DRV_MAX_FREQ) {
		sensor_set_pll12_rate(PLL12_DRV_MAX_FREQ);
		count = 0;
		return E_OK;
	}

	/*At most 8 sensors are supported at the same time*/
	if (count >= 8) {
		nvt_dbg(ERR,"At most %d sensors are supported\r\n", (int)8);
		return E_NOSPT;
	}

	/*Only different rate are acceptable*/
	if (count) {
		for (i = 0; i < count; i++) {
			if (sensor_rate_record[i] == rate) {
				break;
			}
		}
	}

	if (i == count) {
		sensor_rate_record[count] = rate;
		count++;
	}

	pll12_rate = sensor_rate_record[0];

	for (i = 1; i < count; i++) {
		pll12_rate = sensor_lcm(pll12_rate, sensor_rate_record[i]);
	}

	if (IGNORE_SIE_DRV_MIN_FREQ == min_sie_freq) {
		min_sie_freq = largest_sie_freq;
	}

	/*If the LCM of sensor rate is 1, it means the only thing required
	is to update pll12 rate, and which the correct value should be min_sie_freq.
	This state only happens with the first and all elements of sensor rate are 1*/
	if (min_sie_freq) {
		if (pll12_rate != 1) {
			tmp_rate = pll12_rate;

			while ((pll12_rate < min_sie_freq) || (pll12_rate < prev_pll12_rate)) {
				pll12_rate = tmp_rate * j;
				j++;
			}
		} else {
			if (prev_pll12_rate) {
				pll12_rate = prev_pll12_rate;
			} else {
				pll12_rate = min_sie_freq;
			}
		}
	}

	if (pll12_rate > PLL12_DRV_MAX_FREQ) {
		nvt_dbg(ERR,"pll_12 rate overflow %d\r\n", (int)PLL12_DRV_MAX_FREQ);
		return E_OBJ;
	}

	sensor_set_pll12_rate(pll12_rate);

	prev_pll12_rate = pll12_rate;

	if (min_sie_freq > largest_sie_freq) {
		largest_sie_freq = min_sie_freq;
	}

	return E_OK;
}
#if 0
ER sensor_register_rate(UINT32 rate, UINT32 min_sie_freq, KDRV_SIE_PLL_SEL pll_sel)
{
	if (pll_sel == KDRV_SIE_PLL_SRC_PLL5) {
		sensor_register_pll5_rate(rate, min_sie_freq);
	} else if (pll_sel == KDRV_SIE_PLL_SRC_PLL10) {
		sensor_register_pll10_rate(rate, min_sie_freq);
	} else if (pll_sel == KDRV_SIE_PLL_SRC_PLL12) {
		sensor_register_pll12_rate(rate, min_sie_freq);
	}

	return E_OK;
}
#endif
ER sie_setmclock(SIE_MCLKSRC_SEL MClkSrc, UINT32 uiClkRate, BOOL MClkEn)
{
#if defined(__FREERTOS)
	if (MClkEn)	{
		pll_enableClock(SIE_MCLK);
		switch (MClkSrc) {
		case SIE_MCLKSRC_CURR:
			break;

		case SIE_MCLKSRC_480:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_480);
			break;

		case SIE_MCLKSRC_PLL5:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL5);
		    pll_setPLLEn(PLL_ID_5, TRUE);
			break;

		case SIE_MCLKSRC_PLL10:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL10);
			pll_setPLLEn(PLL_ID_10, TRUE);
			break;

		case SIE_MCLKSRC_PLL12:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL12);
			pll_setPLLEn(PLL_ID_12, TRUE);
			break;

		default:
			nvt_dbg(ERR,"not support, mclk src = %d\r\n",(int)MClkSrc);
			pll_setClockRate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_480);
			break;
		}

		if (uiClkRate != 0) {
			pll_setClockFreq(SIEMCLK_FREQ, uiClkRate);
		}
	}
#else
	struct clk *parent_clk = NULL;

	if (MClkEn) {
		#if defined(CONFIG_NVT_FPGA_EMULATION)
		parent_clk = clk_get(NULL, "fix480m");
		nvt_dbg(ERR,"Mclk src always 480MHz @ fpga platform\r\n");
		#else
		switch (MClkSrc) {
		case SIE_MCLKSRC_CURR:
			break;

		case SIE_MCLKSRC_480:
			parent_clk = clk_get(NULL, "fix480m");
			break;

		case SIE_MCLKSRC_PLL5:
			parent_clk = clk_get(NULL, "pll5");
			break;

		case SIE_MCLKSRC_PLL10:
			parent_clk = clk_get(NULL, "pll10");
			break;

		case SIE_MCLKSRC_PLL12:
			parent_clk = clk_get(NULL, "pll12");
			break;

		default:
			parent_clk = clk_get(NULL, "fix480m");
		    nvt_dbg(ERR,"not support, mclkSrc = %d\r\n",MClkSrc);
			break;
		}
		#endif

		if (parent_clk == NULL) {
			nvt_dbg(ERR,"set mclock  fail!\r\n");
			return E_SYS;
		}

		#if 1
		//if(!(__clk_is_enabled(sie_mclk[0]))) {
			clk_enable(sie_mclk[0]);

		//}
		#endif

		if (!IS_ERR(parent_clk)) {
			clk_set_parent(sie_mclk[0], parent_clk);
		}

		clk_put(parent_clk);
		if (uiClkRate != 0) {
			clk_set_rate(sie_mclk[0], uiClkRate);
		}
	}else {
		//printk("check3 mclk dis\r\n");
		clk_disable(sie_mclk[0]);
	}
#endif

	return E_OK;
}

ER sie_setmclock2(SIE_MCLKSRC_SEL MClkSrc, UINT32 uiClkRate, BOOL MClkEn)
{
#if defined(__FREERTOS)
	if (MClkEn)	{
		pll_enableClock(SIE_MCLK2);
		switch (MClkSrc) {
		case SIE_MCLKSRC_CURR:
			break;

		case SIE_MCLKSRC_480:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_480);
			break;

		case SIE_MCLKSRC_PLL5:
		    pll_setPLLEn(PLL_ID_5, TRUE);
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL5);
			break;

		case SIE_MCLKSRC_PLL10:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL10);
			pll_setPLLEn(PLL_ID_10, TRUE);
			break;

		case SIE_MCLKSRC_PLL12:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL12);
			pll_setPLLEn(PLL_ID_12, TRUE);
			break;

		default:
			nvt_dbg(ERR,"not support, mclk2 src = %d\r\n",(int)MClkSrc);
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_480);
			break;
		}

		if (uiClkRate != 0) {
			pll_setClockFreq(SIEMCLK2_FREQ, uiClkRate);
		}
	}
#else
	struct clk *parent_clk = NULL;

	if (MClkEn) {
		switch (MClkSrc) {
		case SIE_MCLKSRC_CURR:
			break;

		case SIE_MCLKSRC_480:
			parent_clk = clk_get(NULL, "fix480m");
			break;

		case SIE_MCLKSRC_PLL5:
			parent_clk = clk_get(NULL, "pll5");
			break;

		case SIE_MCLKSRC_PLL10:
			parent_clk = clk_get(NULL, "pll10");
			break;

		case SIE_MCLKSRC_PLL12:
			parent_clk = clk_get(NULL, "pll12");
			break;

		default:
			nvt_dbg(ERR,"not support, mclk2 src = %d\r\n",(int)MClkSrc);
			parent_clk = clk_get(NULL, "fix480m");
			break;
		}

		if (parent_clk == NULL) {
			nvt_dbg(ERR,"set mclock2  fail!\r\n");
			return E_SYS;
		}
#if 1
		//if(!(__clk_is_enabled(sie_mclk[1]))) {
			clk_enable(sie_mclk[1]);

		//}
#endif
		if (!IS_ERR(parent_clk))
			clk_set_parent(sie_mclk[1], parent_clk);

		clk_put(parent_clk);

		if (uiClkRate != 0)
			clk_set_rate(sie_mclk[1], uiClkRate);
	}else {
		//printk("check3 mclk dis\r\n");
		clk_disable(sie_mclk[1]);
	}
#endif

	return E_OK;
}

ER sie_setmclock3(SIE_MCLKSRC_SEL MClkSrc, UINT32 uiClkRate, BOOL MClkEn)
{
#if defined(__FREERTOS)
	if (MClkEn) {
		pll_enableClock(SIE_MCLK3);
		switch (MClkSrc) {
		case SIE_MCLKSRC_CURR:
			break;

		case SIE_MCLKSRC_480:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_480);
			break;

		case SIE_MCLKSRC_PLL5:
		    pll_setPLLEn(PLL_ID_5, TRUE);
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL5);
			break;

		case SIE_MCLKSRC_PLL10:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL10);
			pll_setPLLEn(PLL_ID_10, TRUE);
			break;

		case SIE_MCLKSRC_PLL12:
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL12);
			pll_setPLLEn(PLL_ID_12, TRUE);
			break;

		default:
			nvt_dbg(ERR,"not support, mclk3 src = %d\r\n",(int)MClkSrc);
			pll_setClockRate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_480);
			break;
		}

		if (uiClkRate != 0) {
			pll_setClockFreq(SIEMCLK3_FREQ, uiClkRate);
		}
	}
#else
	struct clk *parent_clk = NULL;

	if (MClkEn) {
		switch (MClkSrc) {
		case SIE_MCLKSRC_CURR:
			break;

		case SIE_MCLKSRC_480:
			parent_clk = clk_get(NULL, "fix480m");
			break;

		case SIE_MCLKSRC_PLL5:
			parent_clk = clk_get(NULL, "pll5");
			break;

		case SIE_MCLKSRC_PLL10:
			parent_clk = clk_get(NULL, "pll10");
			break;

		case SIE_MCLKSRC_PLL12:
			parent_clk = clk_get(NULL, "pll12");
			break;

		default:
			nvt_dbg(ERR,"not support, mclk3 src = %d\r\n",(int)MClkSrc);
			parent_clk = clk_get(NULL, "fix480m");
			break;
		}

		if (parent_clk == NULL){
			nvt_dbg(ERR,"sie_setmclock3  fail!\r\n");
			return E_SYS;
		}
#if 1
		//if(!(__clk_is_enabled(sie_mclk[2]))) {
			clk_enable(sie_mclk[2]);

		//}
#endif
		if (!IS_ERR(parent_clk))
			clk_set_parent(sie_mclk[2], parent_clk);

		clk_put(parent_clk);

		if (uiClkRate != 0)
			clk_set_rate(sie_mclk[2], uiClkRate);
	}else {
		//printk("check3 mclk dis\r\n");
		clk_disable(sie_mclk[2]);
	}
#endif

	return E_OK;
}

ER sie1_setClock(SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate)
{
#if defined(__FREERTOS)
    //nvt_dbg(ERR, "sieclk, src %d, uiClkRate = %d\r\n",ClkSrc,uiClkRate);
	//nvt_dbg(ERR, "pll5 rate %d, pll12 rate %d\r\n",pll_get_pll_freq(PLL_ID_5), pll_get_pll_freq(PLL_ID_12),pll_get_pll_freq(PLL_ID_10));
	#if (defined(_NVT_FPGA_))
	pll_setClockRate(PLL_CLKSEL_SIE_CLKSRC, PLL_CLKSEL_SIE_CLKSRC_PLL5);
	g_sieClkSrc[SIE_ENGINE_ID_1] = SIE_CLKSRC_PLL5;
	g_sieClkRate[SIE_ENGINE_ID_1] = pll_get_pll_freq(PLL_ID_5)>>1;
	pll_setPLLEn(PLL_ID_5, TRUE);
	#else
	pll_setClockRate(PLL_CLKSEL_SIE_CLKSRC, PLL_CLKSEL_SIE_CLKSRC_320);
	g_sieClkSrc[SIE_ENGINE_ID_1] = SIE_CLKSRC_320;
	g_sieClkRate[SIE_ENGINE_ID_1] = SIE_MAX_CLK_FREQ;
	#endif

	//nvt_dbg(ERR, "SIE ClkSrc = %d, ClkRate = %d\r\n",g_sieClkSrc[SIE_ENGINE_ID_1],g_sieClkRate[SIE_ENGINE_ID_1]);

	// Enable PLL clock source
	if (g_sieClkRate[SIE_ENGINE_ID_1] != 0) {
		pll_setClockFreq(SIECLK_FREQ, g_sieClkRate[SIE_ENGINE_ID_1]);
	}
#else
	struct clk *parent_clk=NULL;
	UINT32 sie_src_freq;
	#if defined(CONFIG_NVT_FPGA_EMULATION)
	struct clk *parent2_clk=NULL;
	#endif

	if (sie_power_saving_mode==1) {
		parent_clk = clk_get(NULL, "fix480m");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_1] = SIE_CLKSRC_480;
		g_sieClkRate[SIE_ENGINE_ID_1] = sie_src_freq>>1;
	}else if (sie_power_saving_mode==2) {
		parent_clk = clk_get(NULL, "fix480m");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_1] = SIE_CLKSRC_480;
		g_sieClkRate[SIE_ENGINE_ID_1] = sie_src_freq>>2;
	}else {
		#if defined(CONFIG_NVT_FPGA_EMULATION)
		parent_clk = clk_get(NULL, "fix480m");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_1] = SIE_CLKSRC_480;
		g_sieClkRate[SIE_ENGINE_ID_1] = sie_src_freq>>1;
		// enable pll5
		parent2_clk = clk_get(NULL, "pll5");
		if (parent2_clk==NULL) {
			nvt_dbg(ERR, "pll5 nod null\r\n");
		}
		clk_prepare(parent2_clk);
		clk_enable(parent2_clk);
		#else
		parent_clk = clk_get(NULL, "pllf320");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_1] = SIE_CLKSRC_320;
		if (sie_src_freq==SIE_MAX_CLK_FREQ)
			g_sieClkRate[SIE_ENGINE_ID_1] = sie_src_freq;
		else if (sie_src_freq==(2*SIE_MAX_CLK_FREQ))
			g_sieClkRate[SIE_ENGINE_ID_1] = sie_src_freq>>1;
		#endif
	}
	
	//nvt_dbg(ERR, "SIE ClkSrc = %d, ClkRate = %d\r\n",g_sieClkSrc[SIE_ENGINE_ID_1],g_sieClkRate[SIE_ENGINE_ID_1]);

	if (!IS_ERR(parent_clk)) {
		clk_set_parent(sie_clk[SIE_ENGINE_ID_1], parent_clk);
	}

	clk_put(parent_clk);

	if (g_sieClkRate[SIE_ENGINE_ID_1] != 0) {
		clk_set_rate(sie_clk[SIE_ENGINE_ID_1], g_sieClkRate[SIE_ENGINE_ID_1]);
	}
#endif

	return 0;
}

ER sie2_setClock(SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate)
{
#if defined(__FREERTOS)

	#if (defined(_NVT_FPGA_))
	pll_setClockRate(PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_PLL5);
	g_sieClkSrc[SIE_ENGINE_ID_2] = SIE_CLKSRC_PLL5;
	g_sieClkRate[SIE_ENGINE_ID_2] = pll_get_pll_freq(PLL_ID_5)>>1;
	pll_setPLLEn(PLL_ID_5, TRUE);
	#else
	pll_setClockRate(PLL_CLKSEL_SIE2_CLKSRC, PLL_CLKSEL_SIE2_CLKSRC_320);
	g_sieClkSrc[SIE_ENGINE_ID_2] = SIE_CLKSRC_320;
	g_sieClkRate[SIE_ENGINE_ID_2] = SIE_MAX_CLK_FREQ;
	#endif

	//nvt_dbg(ERR, "SIE ClkSrc = %d, ClkRate = %d\r\n",g_sieClkSrc[SIE_ENGINE_ID_1],g_sieClkRate[SIE_ENGINE_ID_1]);

	// Enable PLL clock source
	if (g_sieClkRate[SIE_ENGINE_ID_2] != 0) {
		pll_setClockFreq(SIE2CLK_FREQ, g_sieClkRate[SIE_ENGINE_ID_2]);
	}
#else
	struct clk *parent_clk=NULL;
	UINT32 sie_src_freq;

	if (sie_power_saving_mode==1) {
		parent_clk = clk_get(NULL, "fix480m");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_2] = SIE_CLKSRC_480;
		g_sieClkRate[SIE_ENGINE_ID_2] = sie_src_freq>>1;
	}else if (sie_power_saving_mode==2) {
		parent_clk = clk_get(NULL, "fix480m");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_2] = SIE_CLKSRC_480;
		g_sieClkRate[SIE_ENGINE_ID_2] = sie_src_freq>>2;
	}else {
		parent_clk = clk_get(NULL, "pllf320");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_2] = SIE_CLKSRC_320;
		if (sie_src_freq==SIE_MAX_CLK_FREQ)
			g_sieClkRate[SIE_ENGINE_ID_2] = sie_src_freq;
		else if (sie_src_freq==(2*SIE_MAX_CLK_FREQ))
			g_sieClkRate[SIE_ENGINE_ID_2] = sie_src_freq>>1;
	}
		

	//nvt_dbg(ERR, "SIE2 ClkSrc = %d, ClkRate = %d\r\n",g_sieClkSrc[SIE_ENGINE_ID_2],g_sieClkRate[SIE_ENGINE_ID_2]);

	if (!IS_ERR(parent_clk)) {
		clk_set_parent(sie_clk[SIE_ENGINE_ID_2], parent_clk);
	}

	clk_put(parent_clk);

	if (g_sieClkRate[SIE_ENGINE_ID_2] != 0) {
		clk_set_rate(sie_clk[SIE_ENGINE_ID_2], g_sieClkRate[SIE_ENGINE_ID_2]);
	}
	
#endif

	return 0;
}

ER sie3_setClock(SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate)
{
#if defined(__FREERTOS)
	#if (defined(_NVT_FPGA_))
	pll_setClockRate(PLL_CLKSEL_SIE3_CLKSRC, PLL_CLKSEL_SIE3_CLKSRC_PLL5);
	g_sieClkSrc[SIE_ENGINE_ID_3] = SIE_CLKSRC_PLL5;
	g_sieClkRate[SIE_ENGINE_ID_3] = pll_get_pll_freq(PLL_ID_5)>>1;
	pll_setPLLEn(PLL_ID_5, TRUE);
	#else
	pll_setClockRate(PLL_CLKSEL_SIE3_CLKSRC, PLL_CLKSEL_SIE3_CLKSRC_320);
	g_sieClkSrc[SIE_ENGINE_ID_3] = SIE_CLKSRC_320;
	g_sieClkRate[SIE_ENGINE_ID_3] = SIE_MAX_CLK_FREQ;
	#endif

	//nvt_dbg(ERR, "SIE ClkSrc = %d, ClkRate = %d\r\n",g_sieClkSrc[SIE_ENGINE_ID_1],g_sieClkRate[SIE_ENGINE_ID_1]);

	// Enable PLL clock source
	if (g_sieClkRate[SIE_ENGINE_ID_3] != 0) {
		pll_setClockFreq(SIE3CLK_FREQ, g_sieClkRate[SIE_ENGINE_ID_3]);
	}
#else
	struct clk *parent_clk=NULL;
	UINT32 sie_src_freq;

	if (sie_power_saving_mode==1) {
		parent_clk = clk_get(NULL, "fix480m");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_3] = SIE_CLKSRC_480;
		g_sieClkRate[SIE_ENGINE_ID_3] = sie_src_freq>>1;
	}else if (sie_power_saving_mode==2) {
		parent_clk = clk_get(NULL, "fix480m");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_3] = SIE_CLKSRC_480;
		g_sieClkRate[SIE_ENGINE_ID_3] = sie_src_freq>>2;
	}else {
		parent_clk = clk_get(NULL, "pllf320");
		sie_src_freq = clk_get_rate(parent_clk);
		g_sieClkSrc[SIE_ENGINE_ID_3] = SIE_CLKSRC_320;
		if (sie_src_freq==SIE_MAX_CLK_FREQ)
			g_sieClkRate[SIE_ENGINE_ID_3] = sie_src_freq;
		else if (sie_src_freq==(2*SIE_MAX_CLK_FREQ))
			g_sieClkRate[SIE_ENGINE_ID_3] = sie_src_freq>>1;
	}
		

	//nvt_dbg(ERR, "SIE3 ClkSrc = %d, ClkRate = %d\r\n",g_sieClkSrc[SIE_ENGINE_ID_3],g_sieClkRate[SIE_ENGINE_ID_3]);

	if (!IS_ERR(parent_clk)) {
		clk_set_parent(sie_clk[SIE_ENGINE_ID_3], parent_clk);
	}

	clk_put(parent_clk);

	if (g_sieClkRate[SIE_ENGINE_ID_3] != 0) {
		clk_set_rate(sie_clk[SIE_ENGINE_ID_3], g_sieClkRate[SIE_ENGINE_ID_3]);
	}
#endif

	return 0;
}

ER sie_setClock(SIE_ENGINE_ID id, SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate)
{
#if 0
#if !defined(__FREERTOS)
	struct clk *pll10_clk=NULL;
	UINT32 pll10_freq;
#endif
#endif
    if (uiClkRate > SIE_MAX_CLK_FREQ) {
		nvt_dbg(ERR, "id %d, Input Clk rate %d > MAX_CLK_FREQ, clamp to %d\r\n",id, uiClkRate,SIE_MAX_CLK_FREQ);
		uiClkRate = SIE_MAX_CLK_FREQ;
	}
	#if 0
	#if defined(__FREERTOS)
	if (pll_get_pll_freq(PLL_ID_10)<600000000) {
		nvt_dbg(ERR,"PLL10 is %d not 600MHz\r\n",pll_get_pll_freq(PLL_ID_10));
    }
	#else
	pll10_clk = clk_get(NULL, "pll10");
	pll10_freq = clk_get_rate(pll10_clk);
	if (pll10_freq<600000000) {
		nvt_dbg(ERR,"PLL10 is %d not 600MHz\r\n",pll10_freq);
    }
	#endif
	#endif
	switch (id) {
	case SIE_ENGINE_ID_1:
		sie1_setClock(ClkSrc, uiClkRate);
		break;

	case SIE_ENGINE_ID_2:
		sie2_setClock(ClkSrc, uiClkRate);
		break;

	case SIE_ENGINE_ID_3:
		sie3_setClock(ClkSrc, uiClkRate);
		break;

	default:
		nvt_dbg(ERR,"id = %d out of range !!\r\n", id);
	return E_SYS;
	}
	return E_OK;
}

ER sie_getClock(SIE_ENGINE_ID id, SIE_CLKSRC_SEL *ClkSrc, UINT32 *uiClkRate)
{
	*ClkSrc = g_sieClkSrc[id];
	*uiClkRate = g_sieClkRate[id];

	return E_OK;
}

ER sie1_setPxClock(SIE_PXCLKSRC PxClkSel)
{
#if defined(__FREERTOS)
	//#if defined(_BSP_NA51055_)
	switch (PxClkSel) {
	case SIE_PXCLKSRC_OFF:
		break;

	case SIE_PXCLKSRC_PAD:
		pll_setClockRate(PLL_CLKSEL_SIE_PXCLKSRC, PLL_CLKSEL_SIE_PXCLKSRC_PXCLKPAD);
		pll_setClockRate(PLL_CLKSEL_SIE_IO_PXCLKSRC, PLL_CLKSEL_SIE_IO_PXCLKSRC_PXCLKPAD);//
		pll_enableClock(SIE_PXCLK);
		break;

	case SIE_PXCLKSRC_PAD_AHD:
		pll_setClockRate(PLL_CLKSEL_SIE_PXCLKSRC, PLL_CLKSEL_SIE_PXCLKSRC_PXCLKPAD);//
		pll_setClockRate(PLL_CLKSEL_SIE_IO_PXCLKSRC, PLL_CLKSEL_SIE_IO_PXCLKSRC_PXCLKPAD2);
		pll_enableClock(SIE_PXCLK);
		break;

	case SIE_PXCLKSRC_MCLK:
		pll_setClockRate(PLL_CLKSEL_SIE_PXCLKSRC, PLL_CLKSEL_SIE_PXCLKSRC_MCLK);
		pll_enableClock(SIE_PXCLK);
		break;

	default:
		nvt_dbg(ERR,"SIE PX_SRC %d out of range !!\r\n", PxClkSel);
		break;
	}
	//#endif
	return 0;
#else
	struct clk *parent_clk = NULL;
	struct clk *parent_clk2 = NULL;

	switch (PxClkSel) {
	case SIE_PXCLKSRC_OFF:
		break;

	case SIE_PXCLKSRC_PAD:
		parent_clk = clk_get(NULL, "sie_io_pxclk");
		parent_clk2 = clk_get(NULL, "sie1_pxclkpad");
		break;

	case SIE_PXCLKSRC_PAD_AHD:
		parent_clk = clk_get(NULL, "sie_io_pxclk");
		parent_clk2 = clk_get(NULL, "sie2_pxclkpad");
		break;

	case SIE_PXCLKSRC_MCLK:
		parent_clk = clk_get(NULL, "f0c00000.siemck");
		break;

	default:
		nvt_dbg(ERR,"SIE PX_SRC %d out of range !!\r\n", PxClkSel);
		break;
	}

	//nvt_dbg(IND, "PxClkSel = %d\r\n",PxClkSel);

	if (!IS_ERR(parent_clk)) {
		clk_set_parent(sie_pxclk[SIE_ENGINE_ID_1], parent_clk);
	}

	clk_put(parent_clk);

	if (parent_clk2 != NULL) {
		if (!IS_ERR(parent_clk2)) {
			clk_set_parent(sie_io_pxclk[SIE_ENGINE_ID_1], parent_clk2);
		}

		//clk_put(parent_clk2);
	}
#endif

	return 0;
}

ER sie2_setPxClock(SIE_PXCLKSRC PxClkSel)
{
#if defined(__FREERTOS)
	//#if defined(_BSP_NA51055_)
	switch (PxClkSel) {
	case SIE_PXCLKSRC_OFF:
		break;

	case SIE_PXCLKSRC_PAD:
		pll_setClockRate(PLL_CLKSEL_SIE2_PXCLKSRC, PLL_CLKSEL_SIE2_PXCLKSRC_PXCLKPAD);
		pll_setClockRate(PLL_CLKSEL_SIE2_IO_PXCLKSRC, PLL_CLKSEL_SIE2_IO_PXCLKSRC_PXCLKPAD2);//
		pll_enableClock(SIE2_PXCLK);
		break;

	case SIE_PXCLKSRC_PAD_AHD:
		pll_setClockRate(PLL_CLKSEL_SIE2_PXCLKSRC, PLL_CLKSEL_SIE2_PXCLKSRC_PXCLKPAD);//
		pll_setClockRate(PLL_CLKSEL_SIE2_IO_PXCLKSRC, PLL_CLKSEL_SIE2_IO_PXCLKSRC_PXCLKPAD);
		pll_enableClock(SIE2_PXCLK);
		break;

	case SIE_PXCLKSRC_MCLK:
		pll_setClockRate(PLL_CLKSEL_SIE2_PXCLKSRC, PLL_CLKSEL_SIE2_PXCLKSRC_MCLK);
		pll_enableClock(SIE2_PXCLK);
		break;

	default:
		nvt_dbg(ERR,"SIE2 PX_SRC %d out of range !!\r\n", PxClkSel);
		break;
	}
	//#endif
#else
	struct clk *parent_clk = NULL;
	struct clk *parent_clk2 = NULL;

	switch (PxClkSel) {
	case SIE_PXCLKSRC_OFF:
		break;

	case SIE_PXCLKSRC_PAD:
		parent_clk = clk_get(NULL, "sie2_io_pxclk");
		parent_clk2 = clk_get(NULL, "sie2_pxclkpad");
		break;

	case SIE_PXCLKSRC_PAD_AHD:
		parent_clk = clk_get(NULL, "sie2_io_pxclk");
		parent_clk2 = clk_get(NULL, "sie1_pxclkpad");
		break;

	case SIE_PXCLKSRC_MCLK:
		parent_clk = clk_get(NULL, "f0c00000.siemck");
		break;

	default:
		nvt_dbg(ERR,"SIE2 PX_SRC %d out of range !!\r\n", PxClkSel);
		break;
	}

	if (!IS_ERR(parent_clk)) {
		clk_set_parent(sie_pxclk[SIE_ENGINE_ID_2], parent_clk);
	}

	clk_put(parent_clk);

	if (parent_clk2 != NULL) {
		if (!IS_ERR(parent_clk2)) {
			clk_set_parent(sie_io_pxclk[SIE_ENGINE_ID_2], parent_clk2);
		}

		//clk_put(parent_clk2);
	}
#endif

	return 0;
}

ER sie3_setPxClock(SIE_PXCLKSRC PxClkSel)
{
#if defined(__FREERTOS)
	//#if defined(_BSP_NA51055_)
	switch (PxClkSel) {
	case SIE_PXCLKSRC_OFF:
		break;

	case SIE_PXCLKSRC_PAD:
		pll_setClockRate(PLL_CLKSEL_SIE3_PXCLKSRC, PLL_CLKSEL_SIE3_PXCLKSRC_CCIR2_PXCLK);
		pll_enableClock(SIE3_PXCLK);
		break;

	case SIE_PXCLKSRC_PAD_AHD:
		pll_setClockRate(PLL_CLKSEL_SIE3_PXCLKSRC, PLL_CLKSEL_SIE3_PXCLKSRC_CCIR2_PXCLK);
		pll_enableClock(SIE3_PXCLK);
		break;

	case SIE_PXCLKSRC_MCLK:
		pll_setClockRate(PLL_CLKSEL_SIE3_PXCLKSRC, PLL_CLKSEL_SIE3_PXCLKSRC_MCLK);
		pll_enableClock(SIE3_PXCLK);
		break;

	default:
		nvt_dbg(ERR,"SIE3 PX_SRC %d out of range !!\r\n", PxClkSel);
		break;
	}
	//#endif
#else
	struct clk *parent_clk = NULL;
	struct clk *parent_clk2 = NULL;

	switch (PxClkSel) {
	case SIE_PXCLKSRC_OFF:
		break;

	case SIE_PXCLKSRC_PAD:
		parent_clk = clk_get(NULL, "sie2_io_pxclk");
		parent_clk2 = clk_get(NULL, "sie2_pxclkpad");
		break;

	case SIE_PXCLKSRC_PAD_AHD:
		parent_clk = clk_get(NULL, "sie2_io_pxclk");
		parent_clk2 = clk_get(NULL, "sie1_pxclkpad");
		break;

	case SIE_PXCLKSRC_MCLK:
		parent_clk = clk_get(NULL, "f0c00000.siemck");
		break;

	default:
		nvt_dbg(ERR,"SIE3 PX_SRC %d out of range !!\r\n", PxClkSel);
		break;
	}

	if (!IS_ERR(parent_clk)) {
		clk_set_parent(sie_pxclk[SIE_ENGINE_ID_3], parent_clk);
	}

	clk_put(parent_clk);

	if (parent_clk2 != NULL) {
		if (!IS_ERR(parent_clk2)) {
			clk_set_parent(sie_io_pxclk[SIE_ENGINE_ID_3], parent_clk2);
		}
		//clk_put(parent_clk2);
	}
#endif

	return 0;
}

ER sie_setPxClock(SIE_ENGINE_ID id, SIE_PXCLKSRC PxClkSel)
{
	switch (id) {
	case SIE_ENGINE_ID_1:
		sie1_setPxClock(PxClkSel);
		break;

	case SIE_ENGINE_ID_2:
		sie2_setPxClock(PxClkSel);
		break;

	case SIE_ENGINE_ID_3:
		sie3_setPxClock(PxClkSel);
		break;

	default:
		nvt_dbg(ERR,"id = %d out of range !!\r\n", id);
		return E_SYS;
	}
	return E_OK;
}

/**
	register log
*/

#define SIE_REG_LOG_NUM 512
static SIE_REG_INFO *reg_log[MODULE_CLK_NUM]={NULL,NULL,NULL};
static UINT32 reg_log_cnt[MODULE_CLK_NUM]={0};
static UINT32 b_reg_log_en[MODULE_CLK_NUM] = {DISABLE,DISABLE,DISABLE};

void sie_platform_set_reg(UINT32 addr, UINT32 ofs, UINT32 val)
{
    UINT32 i,sie_id=0;

    for (i=0;i<MODULE_CLK_NUM;i++) {
		if(addr==_SIE_REG_BASE_ADDR_SET[i]) {
			sie_id = i;
		}
    }

	if (b_reg_log_en[sie_id] && reg_log_cnt[sie_id] < SIE_REG_LOG_NUM) {
		//if (sie_id == 1)
		//	nvt_dbg(ERR,"record sie%d register\r\n",(int)sie_id);
		reg_log[sie_id][reg_log_cnt[sie_id]].ofs = ofs;
		reg_log[sie_id][reg_log_cnt[sie_id]].val = val;
		reg_log_cnt[sie_id]++;

		if (reg_log_cnt[sie_id] >= SIE_REG_LOG_NUM) {
			nvt_dbg(ERR,"record register buffer overflow %d\r\n",(int)SIE_REG_LOG_NUM);
		}
	}
#if defined(__FREERTOS)
	OUTW((addr + ofs), (val));
#else
	iowrite32(val, (void *)(addr + ofs));
#endif
}

INT32 sie_platform_set_reg_log_enable(UINT32 id, UINT32 en)
{
#if defined(__FREERTOS)
#else
	void* p_buf;

	if (b_reg_log_en[id] != 0) {
		DBG_ERR("SIE %d log already enable\r\n",id);
		return E_SYS;
	}

	//pr_err("SIE alloc register log buffer OK\r\n");
	//pr_err("SIE alloc register log buffer OK\r\n");
	//pr_err("SIE alloc register log buffer OK\r\n");


	#if 0
	for (count=0;count<MODULE_CLK_NUM;count++) {
		p_buf = kmalloc(sizeof(SIE_REG_INFO) * SIE_REG_LOG_NUM, GFP_KERNEL);
		if (p_buf == NULL) {
			DBG_ERR("SIE %d alloc register log buffer failed\r\n", count);
			return E_NOMEM;
		}
		reg_log_cnt[count] = 0;
		reg_log[count] = p_buf;
		//pr_err("SIE%d register log buffer 0x%08x\r\n",count,(int)reg_log[count]);
    }

	b_reg_log_en = en;
	#else
	if (id<MODULE_CLK_NUM) {
		p_buf = kmalloc(sizeof(SIE_REG_INFO) * SIE_REG_LOG_NUM, GFP_KERNEL);
		if (p_buf == NULL) {
			DBG_ERR("SIE %d alloc register log buffer failed\r\n", id);
			return E_NOMEM;
		}
		reg_log_cnt[id] = 0;
		reg_log[id] = p_buf;


		b_reg_log_en[id] = en;
	}
	#endif
#endif
	return E_OK;
}

INT32 sie_platform_set_reg_log_disable(UINT32 id, UINT32 b_freebuf)
{
#if defined(__FREERTOS)
#else
	b_reg_log_en[id] = DISABLE;

	//pr_err("SIE disable register log buffer OK,log_en %d, free_en %d, id %d\r\n",b_reg_log_en,b_freebuf, id);
	//pr_err("SIE disable register log buffer OK,log_en %d, free en %d, id %d\r\n",b_reg_log_en,b_freebuf, id);
	//pr_err("SIE disable register log buffer OK,log_en %d, free en %d, id %d\r\n",b_reg_log_en,b_freebuf, id);

	if (b_freebuf) {
		//pr_err("id = %d\r\n",id);
		if ((reg_log[id] != NULL) && (id<MODULE_CLK_NUM)) {
			//pr_err("free %d buf 0x%08x\r\n",id, (int)reg_log[id]);
			kfree((void *)reg_log[id]);
			reg_log[id] = NULL;
		}

	}
#endif
	return E_OK;
}

SIE_REG_LOG_RESULT sie_platform_get_reg_log(UINT32 id)
{
	SIE_REG_LOG_RESULT rst;

	//need support multi sie
	rst.cnt = reg_log_cnt[id];
	rst.p_reg = reg_log[id];
	//pr_err("get_reg_log, id = %d, cnt = %d\r\n",id, reg_log_cnt[id]);

	return rst;
}

EXPORT_SYMBOL(sie_platform_release_resource);
EXPORT_SYMBOL(sie_setmclock);
EXPORT_SYMBOL(sie_setmclock2);
EXPORT_SYMBOL(sie_setmclock3);
EXPORT_SYMBOL(sensor_register_pll5_rate);
EXPORT_SYMBOL(sensor_register_pll10_rate);
EXPORT_SYMBOL(sensor_register_pll12_rate);
EXPORT_SYMBOL(sie_setClock);
EXPORT_SYMBOL(sie_getClock);
EXPORT_SYMBOL(sie_setPxClock);
EXPORT_SYMBOL(sie_platform_set_reg_log_enable);
EXPORT_SYMBOL(sie_platform_get_reg_log);
EXPORT_SYMBOL(sie_platform_set_reg_log_disable);
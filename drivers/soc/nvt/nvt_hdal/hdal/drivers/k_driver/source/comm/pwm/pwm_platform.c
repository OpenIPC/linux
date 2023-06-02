#include "pwm_platform.h"

#if defined __UITRON || defined __ECOS
static ID v_pwm_flg_id[] = {FLG_ID_PWM, FLG_ID_PWM2, FLG_ID_PWM3};
static UINT32 gPWMPinmuxTable[PWMID_NO_11 + 1] = {
	PINMUX_FUNC_ID_PWM_0,
	PINMUX_FUNC_ID_PWM_1,
	PINMUX_FUNC_ID_PWM_2,
	PINMUX_FUNC_ID_PWM_3,
	PINMUX_FUNC_ID_PWM_4,
	PINMUX_FUNC_ID_PWM_5,
	PINMUX_FUNC_ID_PWM_6,
	PINMUX_FUNC_ID_PWM_7,
	PINMUX_FUNC_ID_PWM_8,
	PINMUX_FUNC_ID_PWM_9,
	PINMUX_FUNC_ID_PWM_10,
	PINMUX_FUNC_ID_PWM_11,
};

static UINT32 v_pwm_clk_en[PWM_PWMCH_BITS] = {
	PWM0_CLK,   PWM1_CLK,   PWM2_CLK,   PWM3_CLK,
	PWM4_CLK,   PWM5_CLK,   PWM6_CLK,   PWM7_CLK,
	PWM8_CLK,   PWM9_CLK,   PWM10_CLK,  PWM11_CLK,
};

static UINT32 v_pwm_sem[PWM_PWMCH_BITS] = {
	SEMID_PWM_0,     SEMID_PWM_1,    SEMID_PWM_2,    SEMID_PWM_3,
	SEMID_PWM_4,     SEMID_PWM_5,    SEMID_PWM_6,    SEMID_PWM_7,
	SEMID_PWM_8,     SEMID_PWM_9,    SEMID_PWM_10,   SEMID_PWM_11,
};

static FLGPTN v_ccnt_sem[PWM_CCNTCH_BITS] = {
	SEMID_PWM_CCNT0,
	SEMID_PWM_CCNT1,
	SEMID_PWM_CCNT2,
};
#elif defined(__FREERTOS)
static ID v_pwm_flg_id[] = {FLG_ID_PWM, FLG_ID_PWM2, FLG_ID_PWM3};

static UINT32 v_pwm_clk_en[PWM_PWMCH_BITS] = {
	PWM0_CLK,   PWM1_CLK,   PWM2_CLK,   PWM3_CLK,
	PWM4_CLK,   PWM5_CLK,   PWM6_CLK,   PWM7_CLK,
	PWM8_CLK,   PWM9_CLK,   PWM10_CLK,  PWM11_CLK,
};

static SEM_HANDLE v_pwm_sem[PWM_PWMCH_BITS];

static SEM_HANDLE v_ccnt_sem[PWM_CCNTCH_BITS];


unsigned int pwm_debug_level = NVT_DBG_WRN;

#else

UINT32 IOADDR_PWM_REG_BASE;

static ID v_pwm_flg_id[3];

static struct clk *v_pwm_clk_en[PWM_PWMCH_BITS + PWM_CCNTCH_BITS];

static SEM_HANDLE v_pwm_sem[PWM_PWMCH_BITS];

static SEM_HANDLE v_ccnt_sem[PWM_CCNTCH_BITS];

#endif

static vk_spinlock_t v_pwm_spin_locks;


ER pwm_platform_flg_clear(ID id, FLGPTN flg)
{
	return clr_flg(v_pwm_flg_id[id], flg);
}

ER pwm_platform_flg_set(ID id, FLGPTN flg)
{
	 return iset_flg(v_pwm_flg_id[id], flg);
}

ER pwm_platform_flg_wait(PFLGPTN p_flgptn, ID id, FLGPTN flg)
{
	return wai_flg(p_flgptn, v_pwm_flg_id[id], flg, TWF_ORW);
}

ER pwm_platform_sem_wait(PWM_TYPE pwm_type, UINT32 id)
{
#if defined __UITRON || defined __ECOS
	if (pwm_type == PWM_TYPE_PWM)
		return wai_sem(v_pwm_sem[id]);
	else
		return wai_sem(v_ccnt_sem[id]);
#else
	if (pwm_type == PWM_TYPE_PWM)
		return SEM_WAIT(v_pwm_sem[id]);
	else
		return SEM_WAIT(v_ccnt_sem[id]);
#endif
}

ER pwm_platform_sem_signal(PWM_TYPE pwm_type, UINT32 id)
{
#if defined __UITRON || defined __ECOS
	if (pwm_type == PWM_TYPE_PWM)
		return sig_sem(v_pwm_sem[id]);
	else
		return sig_sem(v_ccnt_sem[id]);
#else
	if (pwm_type == PWM_TYPE_PWM)
		SEM_SIGNAL(v_pwm_sem[id]);
	else
		SEM_SIGNAL(v_ccnt_sem[id]);
	return E_OK;
#endif
}

UINT32 pwm_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
#else
	unsigned long flags;
	vk_spin_lock_irqsave(&v_pwm_spin_locks, flags);
	return flags;
#endif
}

void pwm_platform_spin_unlock(UINT32 flags)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&v_pwm_spin_locks, flags);
#endif
}

void pwm_platform_int_enable(void)
{
#if defined __UITRON || defined __ECOS
#if (_EMULATION_ON_CPU2_ == ENABLE)
		drv_enableInt(DRV_INT_PWM2);
#else
		drv_enableInt(DRV_INT_PWM);
#endif
#else
#endif
}

void pwm_platform_int_disable(void)
{
#if defined __UITRON || defined __ECOS
#if (_EMULATION_ON_CPU2_ == ENABLE)
		drv_disableInt(DRV_INT_PWM2);
#else
		drv_disableInt(DRV_INT_PWM);
#endif
#else
#endif
}

void pwm_platform_enable_clk(UINT32 id)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id >= PWMID_CCNT_NO_0 && id <= PWMID_CCNT_NO_2) {
		pll_enable_clock(PWM_CCNT0_CLK + id - PWMID_CCNT_NO_0);
	} else if (id < PWMID_CCNT_NO_0){
		pll_enable_clock(v_pwm_clk_en[id]);
	} else {
		;
	}

#else
	clk_prepare(v_pwm_clk_en[id]);

	clk_enable(v_pwm_clk_en[id]);
#endif
}

void pwm_platform_disable_clk(UINT32 id)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	if (id >= PWMID_CCNT_NO_0 && id <= PWMID_CCNT_NO_2) {
		pll_disable_clock(PWM_CCNT0_CLK + id - PWMID_CCNT_NO_0);
	} else if (id < PWMID_CCNT_NO_0){
		pll_disable_clock(v_pwm_clk_en[id]);
	} else {
		;
	}

#else
	clk_disable(v_pwm_clk_en[id]);

	clk_unprepare(v_pwm_clk_en[id]);
#endif
}

void pwm_platform_set_clk_rate(UINT32 id, UINT32 div)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_set_pwm_clock_rate(id, div);
#else
	UINT32 rate = 120000000;

	if (div < 3) {
		nvt_dbg(WRN, "divider should >= 3, but div = %d\r\n", div);
		div = 3;
	}

	rate = rate / (div + 1);

	nvt_dbg(IND, "clk rate = %d Hz\r\n", rate);

	clk_set_rate(v_pwm_clk_en[id], rate);
#endif
}


void pwm_platform_set_pinmux(UINT32 id, BOOL en)
{
#if defined __UITRON || defined __ECOS
	if (en)
		pinmux_setPinmux(gPWMPinmuxTable[id], PINMUX_PWM_SEL_ACTIVE);
	else
		pinmux_setPinmux(gPWMPinmuxTable[id], PINMUX_PWM_SEL_INACTIVE);
#else
#endif
}

#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
static int is_create = 0, is_create_isr = 0, is_create_isr2 = 0;
irqreturn_t pwm_platform_isr(int irq, void *devid)
{
	pwm_isr();
	return IRQ_HANDLED;
}
irqreturn_t pwm_platform_isr2(int irq, void *devid)
{
	pwm_isr2();
	return IRQ_HANDLED;
}
void pwm_platform_create_resource(void)
{
	int i = 0;

	if (!is_create) {

		OS_CONFIG_FLAG(v_pwm_flg_id[0]);
		OS_CONFIG_FLAG(v_pwm_flg_id[1]);
		OS_CONFIG_FLAG(v_pwm_flg_id[2]);

		for (i = 0; i < PWM_PWMCH_BITS; i++) {
			SEM_CREATE(v_pwm_sem[i], 1);
		}

		for (i = 0; i < PWM_CCNTCH_BITS; i++) {
			SEM_CREATE(v_ccnt_sem[i], 1);
		}

		vk_spin_lock_init(&v_pwm_spin_locks);

		is_create = 1;
	}

	if (!is_create_isr && (pwm_open_dest == PWM_DEST_TO_CPU1)) {
		pwm_platform_request_irq(pwm_open_dest);
	}

	if (!is_create_isr2 && (pwm_open_dest == PWM_DEST_TO_CPU2)) {
		pwm_platform_request_irq(pwm_open_dest);
	}
}
void pwm_platform_request_irq(PWM_DEST pwm_dest)
{
	if (pwm_dest == PWM_DEST_TO_CPU1) {
		request_irq(INT_ID_PWM, pwm_platform_isr, IRQF_TRIGGER_HIGH, "pwm", 0);
		is_create_isr = 1;
	} else if (pwm_dest == PWM_DEST_TO_CPU2) {
		request_irq(INT_ID_PWM2, pwm_platform_isr2, IRQF_TRIGGER_HIGH, "pwm2", 0);
		is_create_isr2 = 1;
	}
}
void pwm_platform_free_irq(PWM_DEST pwm_dest)
{
	if (pwm_dest == PWM_DEST_TO_CPU1) {
		free_irq(INT_ID_PWM, 0);
		is_create_isr = 0;
	} else if (pwm_dest == PWM_DEST_TO_CPU2) {
		free_irq(INT_ID_PWM2, 0);
		is_create_isr2 = 0;
	}
}
#else
void pwm_platform_create_resource(MODULE_INFO *pmodule_info)
{
	int i = 0;

	IOADDR_PWM_REG_BASE = (UINT32)pmodule_info->io_addr[0];

	OS_CONFIG_FLAG(v_pwm_flg_id[0]);
	OS_CONFIG_FLAG(v_pwm_flg_id[1]);
	OS_CONFIG_FLAG(v_pwm_flg_id[2]);

	for (i = 0; i < PWM_PWMCH_BITS; i++) {
		SEM_CREATE(v_pwm_sem[i], 1);
	}

	for (i = 0; i < PWM_CCNTCH_BITS; i++) {
		SEM_CREATE(v_ccnt_sem[i], 1);
	}

	for (i = 0; i < (PWM_PWMCH_BITS + PWM_CCNTCH_BITS); i++) {
		v_pwm_clk_en[i] = pmodule_info->pclk[i];
	}

	vk_spin_lock_init(&v_pwm_spin_locks);

	//printk("pwm addr 0x%x\r\n",
	//	IOADDR_PWM_REG_BASE);
}
#endif
void pwm_platform_release_resource(void)
{
	int i = 0;

	rel_flg(v_pwm_flg_id[0]);
	rel_flg(v_pwm_flg_id[1]);
	rel_flg(v_pwm_flg_id[2]);

	for (i = 0; i < PWM_PWMCH_BITS; i++) {
		SEM_DESTROY(v_pwm_sem[i]);
	}

	for (i = 0; i < PWM_CCNTCH_BITS; i++) {
		SEM_DESTROY(v_ccnt_sem[i]);
	}

}
#endif

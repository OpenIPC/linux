/*
    Digital Audio Interface(DAI) module driver

    @file       dai.c
    @ingroup    mIDrvAud_DAI
    @brief      DAI module driver
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/
#ifdef __KERNEL__
#include <mach/rcw_macro.h>
#include <linux/clk.h>
//#include <mach/top.h>
//#include "kwrap/type.h"//a header for basic variable type
//#include "kwrap/semaphore.h"
//#include "kwrap/flag.h"
//#include "dai_dbg.h"
//#include "dai_drv.h"
#include "dai_reg.h"
#include "dai_int.h"
//static ID     FLG_ID_DAI;
//static SEM_HANDLE SEMID_DAI;
static DEFINE_SPINLOCK(my_lock);
#define loc_cpu(myflags)   spin_lock_irqsave(&my_lock, myflags)
#define unl_cpu(myflags)   spin_unlock_irqrestore(&my_lock, myflags)


#define DBG_WRN(fmt, args...) printk(fmt, ##args)
#define DBG_ERR(fmt, args...) printk(fmt, ##args)

UINT32 _DAI_REG_BASE_ADDR[1];

#define OS_CONFIG_FLAG(x)
#define SEM_CREATE(x, y)
#define rel_flg(x)
#define SEM_DESTROY(x)
#define iset_flg(x,y)
#define SEM_WAIT(x) 0
#define clr_flg(x,y)
#define wai_flg(a,b,c,d)
#define SEM_SIGNAL(x)
#define FLGPTN UINT32

#elif defined(__FREERTOS)

#define __MODULE__ rtos_dai
#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/debug.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/type.h"

#include "include/dai.h"
#include "include/dai_reg.h"
#include "include/dai_int.h"
#include "pll_protected.h"
#include "dma_protected.h"
#include "cache_protected.h"
#include "io_address.h"

static ID	     FLG_ID_DAI;
static SEM_HANDLE SEMID_DAI;

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

unsigned int rtos_dai_debug_level = NVT_DBG_WRN;

static BOOL rtos_init = 0;


#endif
/**
    @addtogroup mIDrvAud_DAI
*/
//@{

#ifdef __KERNEL__
void dai_create_resource(void)
{
	OS_CONFIG_FLAG(FLG_ID_DAI);
	SEM_CREATE(SEMID_DAI, 1);
}

void dai_release_resource(void)
{
	rel_flg(FLG_ID_DAI);
	SEM_DESTROY(SEMID_DAI);
}
#endif

#ifdef __KERNEL__
void dai_enableclk(void)
{
	struct clk *dai_clk, *source_clk;

	dai_clk = clk_get(NULL, "f0630000.dai");
	if (IS_ERR(dai_clk)) {
		DBG_ERR("failed to get dai clk\n");
	}
	clk_enable(dai_clk);

	source_clk = clk_get(NULL, "pll7");
	if (IS_ERR(source_clk)) {
		DBG_ERR("failed to get pll7 clk\n");
	}
	clk_set_parent(dai_clk, source_clk);

	clk_put(dai_clk);
	clk_put(source_clk);

}

void dai_disableclk(void)
{
	struct clk *dai_clk;

	dai_clk = clk_get(NULL, "f0630000.dai");
	if (IS_ERR(dai_clk)) {
		DBG_ERR("failed to get dai clk\n");
	}
	clk_disable(dai_clk);
	clk_put(dai_clk);
}

void dai_setclkrate(unsigned long clkrate)
{
	struct clk *dai_clk;

	dai_clk = clk_get(NULL, "f0630000.dai");
	if (IS_ERR(dai_clk)) {
		DBG_ERR("failed to get dai clk\n");
	}
	clk_set_rate(dai_clk, clkrate);
	clk_put(dai_clk);
}
#else
void dai_enableclk(void)
{
	pll_enableClock(DAI_CLK);
}

void dai_disableclk(void)
{

}

void dai_setclkrate(unsigned long clkrate)
{

}
#endif

static DRV_CB   dai_cb_funciton;

static BOOL     b_dai_opened  = FALSE;

static UINT32	size_error[2] = {0, 0};

//static UINT32   dai_pinmux_flag = PIN_AUDIO_CFG_NONE;

static UINT32 dai_tasklet_event;
void dai_tasklet(void)
{
	UINT32 evt;
	unsigned long flag;

	loc_cpu(flag);
	evt = dai_tasklet_event;
	dai_tasklet_event = 0;
	unl_cpu(flag);

	if (dai_cb_funciton != NULL) {
		dai_cb_funciton(evt);
	}
}


/*
    DAI ISt

    It's DAI Interrupt Service Routine

    @param void
    @return void
*/
#if defined(__FREERTOS)
irq_bh_handler_t dai_bh_ist(int irq, unsigned long event, void *data)
{
	dai_tasklet();

	return (irq_bh_handler_t) IRQ_HANDLED;
}
#endif


/*
    DAI ISR

    It's DAI Interrupt Service Routine

    @param void
    @return void
*/
#if defined (__FREERTOS)
irqreturn_t dai_isr(int irq, void *devid)
#else
void dai_isr(void)
#endif
{
	T_DAI_INTSTATUS_REG     reg_status;
	T_DAI_INTSTATUS2_REG    reg_status_2;

	//DBG_IND("\r\n");

	// Get interrupt status
	reg_status.reg = DAI_GETREG(DAI_INTSTATUS_REG_OFS);
	reg_status_2.reg = DAI_GETREG(DAI_INTSTATUS2_REG_OFS);

	// Only handle interrupts which are enabled
	reg_status.reg  &= DAI_GETREG(DAI_INTEN_REG_OFS);
	reg_status_2.reg &= DAI_GETREG(DAI_INTEN2_REG_OFS);


	if ((reg_status.reg == 0) && (reg_status_2.reg == 0)) {
#if defined(__FREERTOS)
		return IRQ_NONE;
#else
		return;
#endif
	}


	// Clear interrupt status
	DAI_SETREG(DAI_INTSTATUS_REG_OFS,  reg_status.reg);
	DAI_SETREG(DAI_INTSTATUS2_REG_OFS, reg_status_2.reg);


	// INT2 remap
	if (reg_status_2.bit.TXLB_DMABFI) {
		reg_status.reg |= DAI_TXLBDMADONE_INT;
	}
	if (reg_status_2.bit.TXLB_DMALOAD) {
		reg_status.reg |= DAI_TXLBDMALOAD_INT;
	}
	if (reg_status_2.bit.TXLB_STOP) {
		reg_status.reg |= DAI_TXLBSTOP_INT;
	}
	if (reg_status_2.bit.TXLB_BWERR) {
		reg_status.reg |= DAI_TXLBBWERR_INT;
	}

	// Call the isr handler

	dai_tasklet_event |= reg_status.reg;
	#if defined(__FREERTOS)
	kick_bh(INT_ID_DAI, reg_status.reg, NULL);
	#endif

	// Set DAI Flag
	iset_flg(FLG_ID_DAI, reg_status.reg);


#if defined(__FREERTOS)
		return IRQ_HANDLED;
#endif

}

#if 1

/*
    Lock DAI module

    Use semaphore lock for the DAI module

    @return
	@b E_OK: success
	@b Else: fail
*/
ER dai_lock(void)
{
	ER er_ret;

	er_ret        = SEM_WAIT(SEMID_DAI);
	if (er_ret != E_OK) {
		DBG_ERR("wait semaphore fail\r\n");
		return er_ret;
	}

	return E_OK;
}

/*
    Unlock DAI module

    Release semaphore lock for the DAI module

    @return
	@b E_OK: success
	@b Else: fail
*/
ER dai_unlock(void)
{
	SEM_SIGNAL(SEMID_DAI);
	return E_OK;
}

/*
    Wait DAI Interrupt Event
*/
DAI_INTERRUPT dai_wait_interrupt(DAI_INTERRUPT waited_flag)
{
	FLGPTN              ui_flag = 0;

	wai_flg(&ui_flag, FLG_ID_DAI, waited_flag, TWF_ORW | TWF_CLR);

	return ui_flag;
}

/*
    Select DAI I2S pinmux

    @return void
*/
void dai_select_pinmux(BOOL b_en)
{/*
	PIN_GROUP_CONFIG pinmux_i2s[1];
	int				 ret = 0;
	unsigned long flag;

	pinmux_i2s->pin_function = PIN_FUNC_AUDIO;


	if (b_en) {
		loc_cpu(flag);
		dai_pinmux_flag |= PIN_AUDIO_CFG_I2S;
		unl_cpu(flag);
	} else {
		loc_cpu(flag);
		dai_pinmux_flag &= ~PIN_AUDIO_CFG_I2S;
		unl_cpu(flag);
	}

	pinmux_i2s->config = dai_pinmux_flag;

	ret = nvt_pinmux_update(pinmux_i2s, 1);

	if (ret) {
		DBG_WRN("pinmux_mclk update error! \r\n");
	}*/
}


/*
    Select DAI I2S MCLK pinmux

    @return void
*/
void dai_select_mclk_pinmux(BOOL b_en)
{/*
	PIN_GROUP_CONFIG pinmux_mclk[1];
	int				 ret = 0;
	unsigned long flag;

	pinmux_mclk->pin_function = PIN_FUNC_AUDIO;


	if (b_en) {
		loc_cpu(flag);
		dai_pinmux_flag |= PIN_AUDIO_CFG_MCLK;
		unl_cpu(flag);
	} else {
		loc_cpu(flag);
		dai_pinmux_flag &= ~PIN_AUDIO_CFG_MCLK;
		unl_cpu(flag);
	}

	pinmux_mclk->config = dai_pinmux_flag;

	ret = nvt_pinmux_update(pinmux_mclk, 1);

	if (ret) {
		DBG_WRN("pinmux_mclk update error! \r\n");
	}
*/

}

#endif
#if 1

/**
    Open digital audio controller driver.

    Open digital auiod controller.

    @param[in] p_isr_handler      Callback function registered for interrupt notification.

    @return void
*/
void dai_open(DRV_CB p_isr_handler)
{

#if defined(__FREERTOS)
		if (!rtos_init) {
			rtos_init = 1;
			cre_flg(&FLG_ID_DAI, NULL, "FLG_ID_DAI");
			vos_sem_create(&SEMID_DAI, 1, "SEMID_DAI");
		}
#else
	dai_create_resource();
#endif
	if (b_dai_opened) {
		return;
	}

	// log driver opened
	b_dai_opened = TRUE;

	//set dai interrupt handler

	dai_cb_funciton  = p_isr_handler;

	// Clear Interrupt Flag
	clr_flg(FLG_ID_DAI, DAI_INTERRUPT_ALL);

	// Disable DAI Reset
	//pll_disableSystemReset(DAI_RSTN);

	// Enable DAI clock
	dai_enableclk();

	dai_tasklet_event = 0;
#if defined(__FREERTOS)
	// Enable dai interrupt
	request_irq(INT_ID_DAI, dai_isr ,IRQF_TRIGGER_HIGH, "dai", 0);
	request_irq_bh(INT_ID_DAI, (irq_bh_handler_t) dai_bh_ist, IRQF_BH_PRI_HIGH);
#endif


}


/**
    Close digital audio controller driver.

    Close digital auiod controller.

    @return void
*/
void dai_close(void)
{
	if (!b_dai_opened) {
		return;
	}

	// Disable dai interrupt
	//drv_disableInt(DRV_INT_DAI);linux no need

	// Disable DAI clock
	dai_disableclk();
#if defined(__FREERTOS)
	rtos_init = 0;
	rel_flg(FLG_ID_DAI);
	vos_sem_destroy(SEMID_DAI);

#else
	dai_release_resource();
#endif
	//clear dai interrupt handler

	dai_cb_funciton  = NULL;


	// log driver closed
	b_dai_opened = FALSE;
}

/**
    Set Digital Audio interface(DAI) General Configurations.

    Set Digital Audio interface(DAI) general configuration.
    Use DAI_CONFIG_ID as configuration selection and config_value is the configuration parameter.

    @param[in] config_id     Configuration selection. Please refer to DAI_CONFIG_ID for details.
    @param[in] config_value   configuration parameter. Please refer to DAI_CONFIG_ID for details.

    @return void
*/
void dai_set_config(DAI_CONFIG_ID config_id, UINT32 config_value)
{
	T_DAI_CONFIG_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	reg_ctrl.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);

	switch (config_id) {
	case DAI_CONFIG_ID_EXTCODEC_EN: {
			if (config_value == TRUE) {
				reg_ctrl.bit.EXCODEC_EN = 1;
			} else {
				reg_ctrl.bit.EXCODEC_EN = 0;

				// embedded codec fix using CH0/1
				reg_ctrl.bit.HDMI_CH_SEL = DAI_I2SHDMI_SEL_CH01;
			}
		}
		break;

	case DAI_CONFIG_ID_ISRCB: {
			if (!b_dai_opened) {
				break;
			}

			//set dai interrupt handler
			dai_cb_funciton  = (DRV_CB)config_value;

		}
		break;

	case DAI_CONFIG_ID_HDMI_TXEN: {
			if (config_value == TRUE) {
				reg_ctrl.bit.TX_MUX_SEL = 1;     // playback to EAC/HDMI
			} else {
				reg_ctrl.bit.TX_MUX_SEL = 0;     // playback to EAC only
			}
		}
		break;

	case DAI_CONFIG_ID_AVSYNC_EN: {
			if (config_value == TRUE) {
				reg_ctrl.bit.AVSYNC = 1;
			} else {
				reg_ctrl.bit.AVSYNC = 0;
			}
		}
		break;

	case DAI_CONFIG_ID_AVSYNC_SRC: {
			reg_ctrl.bit.AVSYNC_SRC = config_value;
		}
		break;

	case DAI_CONFIG_ID_SET_INTEN: {
			T_DAI_INTEN_REG     reg_int_en;
			T_DAI_INTEN2_REG    reg_int_en_2;
#if defined (__FREERTOS) // wait for rtos flag issue fixed can be removed.
			if (config_value & DAI_TXLBDMADONE_INT){
				reg_int_en_2.reg = DAI_GETREG(DAI_INTEN2_REG_OFS);
				reg_int_en_2.reg |= 0x1;
				DAI_SETREG(DAI_INTEN2_REG_OFS, reg_int_en_2.reg);
			}
			if (config_value & DAI_TXLBDMALOAD_INT){
				reg_int_en_2.reg = DAI_GETREG(DAI_INTEN2_REG_OFS);
				reg_int_en_2.reg |= 0x10;
				DAI_SETREG(DAI_INTEN2_REG_OFS, reg_int_en_2.reg);
			}
			if (config_value & DAI_TXLBSTOP_INT){
				reg_int_en_2.reg = DAI_GETREG(DAI_INTEN2_REG_OFS);
				reg_int_en_2.reg |= 0x100;
				DAI_SETREG(DAI_INTEN2_REG_OFS, reg_int_en_2.reg);
			}
			if (config_value & DAI_TXLBBWERR_INT){
				reg_int_en_2.reg = DAI_GETREG(DAI_INTEN2_REG_OFS);
				reg_int_en_2.reg |= 0x1000;
				DAI_SETREG(DAI_INTEN2_REG_OFS, reg_int_en_2.reg);
			}
#else
			UINT32              i;
			if (config_value & DAI_INTERRUPT_TXLB_ALL) {
				reg_int_en_2.reg = DAI_GETREG(DAI_INTEN2_REG_OFS);
				for (i = 0; i < 4; i++) {
					if (config_value & (0x1 << (i + 24))) {
						reg_int_en_2.reg |= (0x1 << (i << 2));
					}
				}
				DAI_SETREG(DAI_INTEN2_REG_OFS, reg_int_en_2.reg);

				config_value &= ~DAI_INTERRUPT_TXLB_ALL;
			}
#endif
			if (config_value & DAI_INTERRUPT_ALL) {
				reg_int_en.reg = DAI_GETREG(DAI_INTEN_REG_OFS);
				reg_int_en.reg |= (config_value & DAI_INTERRUPT_ALL);
				DAI_SETREG(DAI_INTEN_REG_OFS, reg_int_en.reg);
			}
		}
		break;

	case DAI_CONFIG_ID_CLR_INTEN: {
			T_DAI_INTEN_REG     reg_int_en;
			T_DAI_INTEN2_REG    reg_int_en_2;
			UINT32              i;

			if (config_value & DAI_INTERRUPT_TXLB_ALL) {
				reg_int_en_2.reg = DAI_GETREG(DAI_INTEN2_REG_OFS);
				for (i = 0; i < 4; i++) {
					if (config_value & (0x1 << (i + 24))) {
						reg_int_en_2.reg &= ~(0x1 << (i << 2));
					}
				}
				DAI_SETREG(DAI_INTEN2_REG_OFS, reg_int_en_2.reg);

				config_value &= ~DAI_INTERRUPT_TXLB_ALL;
			}


			if (config_value & DAI_INTERRUPT_ALL) {
				reg_int_en.reg = DAI_GETREG(DAI_INTEN_REG_OFS);
				reg_int_en.reg &= ~(config_value & DAI_INTERRUPT_ALL);
				DAI_SETREG(DAI_INTEN_REG_OFS, reg_int_en.reg);
			}
		}
		break;

	case DAI_CONFIG_ID_CLR_INTSTS: {
			T_DAI_INTSTATUS_REG     reg_status;
			T_DAI_INTSTATUS2_REG   reg_status_2;


			// Get interrupt status 2
			reg_status_2.reg = DAI_GETREG(DAI_INTSTATUS2_REG_OFS);

			// Only allow to clear interrupt status which are disabled
			reg_status_2.reg &= ~(DAI_GETREG(DAI_INTEN2_REG_OFS));

			// Clear interrupt status 2
			DAI_SETREG(DAI_INTSTATUS2_REG_OFS, reg_status_2.reg);



			// Get interrupt status
			reg_status.reg = DAI_GETREG(DAI_INTSTATUS_REG_OFS);

			// Only allow to clear interrupt status which are disabled
			reg_status.reg &= ~(DAI_GETREG(DAI_INTEN_REG_OFS));

			// Clear interrupt status
			DAI_SETREG(DAI_INTSTATUS_REG_OFS, reg_status.reg);

		}
		break;









	case DAI_CONFIG_ID_CLKSRC: {
			if (config_value != DAI_CODESCK_INT) {
				unl_cpu(flag);
				DBG_WRN("Only support internal audio MCLK\r\n");
				return;
			}
		}
		break;

	case DAI_CONFIG_ID_RX_SRC_SEL: {
			if (config_value == DAI_RX_SRC_I2S) {
				reg_ctrl.bit.RX_SRC_MUX_SEL = 1;
			} else {
				reg_ctrl.bit.RX_SRC_MUX_SEL = 0;
			}
		}
		break;

	default:
		unl_cpu(flag);
		DBG_WRN("CfgID Err = %d\r\n", (int)config_id);
		return;

	}

	DAI_SETREG(DAI_CONFIG_REG_OFS, reg_ctrl.reg);
	unl_cpu(flag);
}


/**
    Get Digital Audio interface(DAI) General Configurations.

    Get Digital Audio interface(DAI) general configuration.
    Use DAI_CONFIG_ID as configuration selection and config_value is the configuration parameter.

    @param[in] config_id     Configuration selection. Please refer to DAI_CONFIG_ID for details.

    @return void
*/
UINT32 dai_get_config(DAI_CONFIG_ID config_id)
{
	UINT32              ret = 0;
	T_DAI_CONFIG_REG    reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);

	switch (config_id) {
	case DAI_CONFIG_ID_EXTCODEC_EN: {
			ret = reg_ctrl.bit.EXCODEC_EN;
		}
		break;

	case DAI_CONFIG_ID_RX_SRC_SEL: {
			ret = reg_ctrl.bit.RX_SRC_MUX_SEL;
		}
		break;

	case DAI_CONFIG_ID_ISRCB: {
			//set dai interrupt handler

			ret = (UINT32)dai_cb_funciton;

		}
		break;

	case DAI_CONFIG_ID_HDMI_TXEN: {
			ret = reg_ctrl.bit.TX_MUX_SEL;
		}
		break;

	case DAI_CONFIG_ID_AVSYNC_EN: {
			ret = reg_ctrl.bit.AVSYNC;
		}
		break;

	case DAI_CONFIG_ID_AVSYNC_SRC: {
			ret = reg_ctrl.bit.AVSYNC_SRC;
		}
		break;

	default:
		DBG_WRN("config_id Err = %d\r\n", (int)config_id);
		break;

	}

	return ret;
}


/**
    Set Digital Audio I2S interface configurations

    Use DAI_I2SCONFIG_ID as configuration selection and config_value is the configuration parameter.

    @param[in] config_id     Configuration selection. Please refer to DAI_I2SCONFIG_ID for details.
    @param[in] config_value   configuration parameter. Please refer to DAI_I2SCONFIG_ID for details.

    @return void
*/
void dai_set_i2s_config(DAI_I2SCONFIG_ID config_id, UINT32 config_value)
{
	T_DAI_I2SCONFIG_REG reg_i2s_ctrl;
	T_DAI_I2S_TDM_ORDER_REG reg_i2s_order;
	unsigned long flag;
	UINT32 		ret = 0;

	loc_cpu(flag);

	reg_i2s_ctrl.reg = DAI_GETREG(DAI_I2SCONFIG_REG_OFS);
	reg_i2s_order.reg = DAI_GETREG(DAI_I2S_TDM_ORDER_REG_OFS);

	switch (config_id) {
	case DAI_I2SCONFIG_ID_CLKRATIO: {
			if (config_value == DAI_I2SCLKR_256FS_32BIT) {
				reg_i2s_ctrl.bit.CKRATIO = 0;
			} else if (config_value == DAI_I2SCLKR_256FS_64BIT) {
				reg_i2s_ctrl.bit.CKRATIO = 1;
			} else if (config_value == DAI_I2SCLKR_256FS_128BIT) {
				reg_i2s_ctrl.bit.CKRATIO = 2;
			} else if (config_value == DAI_I2SCLKR_256FS_256BIT) {
				reg_i2s_ctrl.bit.CKRATIO = 3;
			} else {
				unl_cpu(flag);
				DBG_ERR("f/m clk ratio 0x%x not support\r\n", (unsigned int)config_value);
				return;
			}
		}
		break;

	case DAI_I2SCONFIG_ID_FORMAT: {
			if (config_value != DAI_I2SFMT_STANDARD) {
				unl_cpu(flag);
				DBG_ERR("only supports I2S standard format\r\n");
				return;
			}
		}
		break;

	case DAI_I2SCONFIG_ID_OPMODE: {
			if (config_value == DAI_OP_SLAVE) {
				reg_i2s_ctrl.bit.SLAVE = 1;
			} else {
				reg_i2s_ctrl.bit.SLAVE = 0;
			}
		}
		break;

	case DAI_I2SCONFIG_ID_CHANNEL_LEN: {
			reg_i2s_ctrl.bit.CHANNEL_LEN = config_value > 0;
		}
		break;

	case DAI_I2SCONFIG_ID_HDMICH_SEL: {
			T_DAI_CONFIG_REG reg_ctrl;

			reg_ctrl.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);

			if (reg_ctrl.bit.EXCODEC_EN == 0) {
				// When embedded codec, fix using CH0/1
				reg_ctrl.bit.HDMI_CH_SEL = DAI_I2SHDMI_SEL_CH01;
			} else {
				reg_ctrl.bit.HDMI_CH_SEL = config_value;
			}

			DAI_SETREG(DAI_CONFIG_REG_OFS, reg_ctrl.reg);
		}
		break;

	case DAI_I2SCONFIG_ID_CLK_INV: {
			if (config_value == 0) {
				reg_i2s_ctrl.bit.I2S_ASFCK_INV = 0;
			} else {
				reg_i2s_ctrl.bit.I2S_ASFCK_INV = 1;
			}
		}break;

	case DAI_I2SCONFIG_ID_DATA_ORDER: {
			if (config_value == DAI_I2S_DATAORDER_TYPE1) {
				switch ((32 << reg_i2s_ctrl.bit.CKRATIO) / (16 << reg_i2s_ctrl.bit.CHANNEL_LEN)) {
						case 2: reg_i2s_order.bit.I2S_TDM_ORDER = 0x00000010; break;
						case 4: reg_i2s_order.bit.I2S_TDM_ORDER = 0x00003120; break;
						case 6:	reg_i2s_order.bit.I2S_TDM_ORDER = 0x00531420; break;
						case 8: reg_i2s_order.bit.I2S_TDM_ORDER = 0x75316420; break;
						default: reg_i2s_order.bit.I2S_TDM_ORDER = 0x75316420; break;
						}
			} else if (config_value == DAI_I2S_DATAORDER_TYPE2){
				switch ((32 << reg_i2s_ctrl.bit.CKRATIO) / (16 << reg_i2s_ctrl.bit.CHANNEL_LEN)) {
						case 2: reg_i2s_order.bit.I2S_TDM_ORDER = 0x00000010; break;
						case 4: reg_i2s_order.bit.I2S_TDM_ORDER = 0x00003210; break;
						case 6:	reg_i2s_order.bit.I2S_TDM_ORDER = 0x00543210; break;
						case 8: reg_i2s_order.bit.I2S_TDM_ORDER = 0x76543210; break;
						default: reg_i2s_order.bit.I2S_TDM_ORDER = 0x76543210; break;
						}
			} else {
				reg_i2s_order.bit.I2S_TDM_ORDER = config_value;
			}
		}break;

	case DAI_I2SCONFIG_ID_CLK_OFS: {
			if(config_value > DAI_I2S_CLK_OFS_MAX) {
				config_value = DAI_I2S_CLK_OFS_MAX;
			}
			ret = reg_i2s_ctrl.bit.CKRATIO; // make sure ofs is at least one bit clk.
			if (config_value < (UINT32)(0x7>>ret)) {
				config_value = (UINT32)(0x7>>ret);
			}
			reg_i2s_ctrl.bit.I2S_ASFCK_OFS = config_value;
		}break;


	default:
		unl_cpu(flag);
		DBG_WRN("CfgID Err = %d\r\n", (int)config_id);
		return;

	}

	DAI_SETREG(DAI_I2SCONFIG_REG_OFS, reg_i2s_ctrl.reg);
	DAI_SETREG(DAI_I2S_TDM_ORDER_REG_OFS, reg_i2s_order.reg);
	unl_cpu(flag);
}



/**
    Get Digital Audio I2S interface configurations

    Use DAI_I2SCONFIG_ID as configuration selection and config_value is the configuration parameter.

    @param[in] config_id     Configuration selection. Please refer to DAI_I2SCONFIG_ID for details.

    @return void
*/
UINT32 dai_get_i2s_config(DAI_I2SCONFIG_ID config_id)
{
	UINT32              ret = 0;
	T_DAI_I2SCONFIG_REG reg_i2s_ctrl;
	T_DAI_I2S_TDM_ORDER_REG reg_i2s_order;

	reg_i2s_ctrl.reg = DAI_GETREG(DAI_I2SCONFIG_REG_OFS);

	switch (config_id) {
	case DAI_I2SCONFIG_ID_CLKRATIO: {
			ret = reg_i2s_ctrl.bit.CKRATIO;
		}
		break;

	case DAI_I2SCONFIG_ID_FORMAT: {
			ret = DAI_I2SFMT_STANDARD;
		}
		break;

	case DAI_I2SCONFIG_ID_OPMODE: {
			ret = !reg_i2s_ctrl.bit.SLAVE;
		}
		break;

	case DAI_I2SCONFIG_ID_CHANNEL_LEN: {
			ret = reg_i2s_ctrl.bit.CHANNEL_LEN;
		}
		break;

	case DAI_I2SCONFIG_ID_HDMICH_SEL: {
			T_DAI_CONFIG_REG reg_ctrl;

			reg_ctrl.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);
			ret = reg_ctrl.bit.HDMI_CH_SEL;
		}
		break;

	case DAI_I2SCONFIG_ID_SLAVEMATCH: {
			ret = reg_i2s_ctrl.bit.I2SCKR_MATCH;
		}
		break;

	case DAI_I2SCONFIG_ID_CURRENT_CLKRATIO: {
			ret = reg_i2s_ctrl.bit.I2SCKR_CUR;
		}
		break;

	case DAI_I2SCONFIG_ID_CLK_INV: {
			ret = reg_i2s_ctrl.bit.I2S_ASFCK_INV;
		}break;

	case DAI_I2SCONFIG_ID_DATA_ORDER: {
			ret = reg_i2s_order.bit.I2S_TDM_ORDER;
		}break;
	case DAI_I2SCONFIG_ID_CLK_OFS: {
			ret = reg_i2s_ctrl.bit.I2S_ASFCK_OFS;
		}break;


	default:
		DBG_WRN("CfgID Err = %d\r\n", (int)config_id);
		break;

	}

	return ret;

}

#endif

#if 1

/**
    Set DAI Playback Chaneels Configurations.

    Set DAI Playback Chaneels(TX1 and TX2) Configurations.

    @param[in]  channel        playback Channel selection
    @param[in]  config_id    playback config ID selection
    @param[in]  config_value  configuration value

    @return void
*/
void dai_set_tx_config(DAI_TXCH channel, DAI_TXCFG_ID config_id, UINT32 config_value)
{
	T_DAI_FMTCFG0_REG reg_fmt_0;
	unsigned long flag;

	loc_cpu(flag);

	reg_fmt_0.reg = DAI_GETREG(DAI_FMTCFG0_REG_OFS);

	switch (config_id) {
	case DAI_TXCFG_ID_CHANNEL: {
			if (config_value != DAI_CH_DUAL_MONO) {
				if (channel == DAI_TXCH_TX1) {
					reg_fmt_0.bit.TX1_SOUNDM = config_value;
				} else if (channel == DAI_TXCH_TX2) {
					reg_fmt_0.bit.TX2_SOUNDM = config_value;
				}
			} else {
				unl_cpu(flag);
				DBG_WRN("DAI TX no support dual mono\r\n");
				return;
			}
		}
		break;

	case DAI_TXCFG_ID_TOTAL_CH: {
			T_DAI_CONFIG_REG reg_ctrl;

			reg_ctrl.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);

			if ((reg_ctrl.bit.EXCODEC_EN == 0) && (config_value > DAI_TOTCH_2CH)) {
				DBG_WRN("Embedded codec 2CH only\r\n");
				config_value = DAI_TOTCH_2CH;
			}

			if (channel == DAI_TXCH_TX1) {
				reg_fmt_0.bit.TX1_SOUNDCH = config_value;
			} else if (channel == DAI_TXCH_TX2) {
				reg_fmt_0.bit.TX2_SOUNDCH = config_value;
			}
		}
		break;

	case DAI_TXCFG_ID_PCMLEN: {
			if (channel == DAI_TXCH_TX1) {
				reg_fmt_0.bit.TX1_PCMLEN = config_value;
			} else if (channel == DAI_TXCH_TX2) {
				reg_fmt_0.bit.TX2_PCMLEN = config_value;
			}
		}
		break;

	case DAI_TXCFG_ID_DRAMCH: {
			if (channel == DAI_TXCH_TX1) {
				reg_fmt_0.bit.TX1_DRAMCH = config_value;
			} else if (channel == DAI_TXCH_TX2) {
				reg_fmt_0.bit.TX2_DRAMCH = config_value;
			}
		}
		break;

	case DAI_TXCFG_ID_TIMECODE_TRIG: {
			if (channel == DAI_TXCH_TX1) {
				DAI_SETREG(DAI_TX1TCTRIGGER_REG_OFS, config_value);
			} else {
				unl_cpu(flag);
				DBG_ERR("Only TX CH1 has TimeCode function (1)\r\n");
				return;
			}
		}
		break;

	case DAI_TXCFG_ID_TIMECODE_OFS: {
			if (channel == DAI_TXCH_TX1) {
				DAI_SETREG(DAI_TX1TCOFFSET_REG_OFS, config_value);
			} else {
				unl_cpu(flag);
				DBG_ERR("Only TX CH1 has TimeCode function (2)\r\n");
				return;
			}
		}
		break;




	default:
		unl_cpu(flag);
		DBG_WRN("CfgID Err = %d\r\n", (int)config_id);
		return;

	}

	DAI_SETREG(DAI_FMTCFG0_REG_OFS, reg_fmt_0.reg);
	unl_cpu(flag);
}


/**
    Get DAI Playback Chaneels Configurations.

    Get DAI Playback Chaneels(TX1 and TX2) Configurations.

    @param[in]  channel        playback Channel selection
    @param[in]  config_id    playback config ID selection

    @return configuration value
*/
UINT32 dai_get_tx_config(DAI_TXCH channel, DAI_TXCFG_ID config_id)
{
	T_DAI_FMTCFG0_REG   reg_fmt_0;
	UINT32              ret = 0;

	reg_fmt_0.reg = DAI_GETREG(DAI_FMTCFG0_REG_OFS);

	switch (config_id) {
	case DAI_TXCFG_ID_CHANNEL: {
			if (channel == DAI_TXCH_TX1) {
				ret = reg_fmt_0.bit.TX1_SOUNDM;
			} else if (channel == DAI_TXCH_TX2) {
				ret = reg_fmt_0.bit.TX2_SOUNDM;
			}
		}
		break;

	case DAI_TXCFG_ID_TOTAL_CH: {
			if (channel == DAI_TXCH_TX1) {
				ret = reg_fmt_0.bit.TX1_SOUNDCH;
			} else if (channel == DAI_TXCH_TX2) {
				ret = reg_fmt_0.bit.TX2_SOUNDCH;
			}
		}
		break;

	case DAI_TXCFG_ID_PCMLEN: {
			if (channel == DAI_TXCH_TX1) {
				ret = reg_fmt_0.bit.TX1_PCMLEN;
			} else if (channel == DAI_TXCH_TX2) {
				ret = reg_fmt_0.bit.TX2_PCMLEN;
			}
		}
		break;

	case DAI_TXCFG_ID_DRAMCH: {
			if (channel == DAI_TXCH_TX1) {
				ret = reg_fmt_0.bit.TX1_DRAMCH;
			} else if (channel == DAI_TXCH_TX2) {
				ret = reg_fmt_0.bit.TX2_DRAMCH;
			}
		}
		break;

	case DAI_TXCFG_ID_TIMECODE_TRIG: {
			if (channel == DAI_TXCH_TX1) {
				ret = DAI_GETREG(DAI_TX1TCTRIGGER_REG_OFS);
			}
		}
		break;

	case DAI_TXCFG_ID_TIMECODE_OFS: {
			if (channel == DAI_TXCH_TX1) {
				ret = DAI_GETREG(DAI_TX1TCOFFSET_REG_OFS);
			}
		}
		break;

	case DAI_TXCFG_ID_TIMECODE_VAL: {
			if (channel == DAI_TXCH_TX1) {
				ret = DAI_GETREG(DAI_TX1TCVALUE_REG_OFS);
			}
		}
		break;



	default:
		break;

	}

	return ret;
}



/**
    Set DAI Playback Loopback Chaneel Configurations.

    Set DAI Playback Loopback Chaneels(TXLB) Configurations.

    @param[in]  channel        playback Loopback Channel selection
    @param[in]  config_id    playback Loopback config ID selection
    @param[in]  config_value  configuration value

    @return void
*/
void dai_set_txlb_config(DAI_TXLBCFG_ID config_id, UINT32 config_value)
{
	T_DAI_FMTCFG2_REG reg_fmt_2;
	unsigned long flag;

	loc_cpu(flag);

	reg_fmt_2.reg = DAI_GETREG(DAI_FMTCFG2_REG_OFS);

	switch (config_id) {
	case DAI_TXLBCFG_ID_CHANNEL: {
			if (config_value != DAI_CH_DUAL_MONO) {
				reg_fmt_2.bit.TXLB_SOUNDM = config_value;
			} else {
				unl_cpu(flag);
				DBG_WRN("DAI TXLB no support dual mono\r\n");
				return;
			}
		}
		break;

	case DAI_TXLBCFG_ID_TOTAL_CH: {
			T_DAI_CONFIG_REG reg_ctrl;

			reg_ctrl.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);

			if ((reg_ctrl.bit.EXCODEC_EN == 0) && (config_value > DAI_TOTCH_2CH)) {
				DBG_WRN("Embedded codec 2CH only\r\n");
				config_value = DAI_TOTCH_2CH;
			}

			reg_fmt_2.bit.TXLB_SOUNDCH = config_value;

		}
		break;

	case DAI_TXLBCFG_ID_PCMLEN: {

			reg_fmt_2.bit.TXLB_PCMLEN = config_value;

		}
		break;

	case DAI_TXLBCFG_ID_DRAMCH: {
			reg_fmt_2.bit.TXLB_DRAMCH = config_value;
		}
		break;

	case DAI_TXLBCFG_ID_RXSYNC: {
			T_DAI_CONFIG_REG    reg_cfg;

			reg_cfg.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);
			reg_cfg.bit.TXLB_SYNC = config_value > 0;
			DAI_SETREG(DAI_CONFIG_REG_OFS, reg_cfg.reg);
			unl_cpu(flag);
		}
		return;

	default:
		unl_cpu(flag);
		DBG_WRN("CfgID Err = %d\r\n", (int)config_id);
		return;

	}

	DAI_SETREG(DAI_FMTCFG2_REG_OFS, reg_fmt_2.reg);
	unl_cpu(flag);
}



/**
    Set DAI Playback Loopback Chaneel Configurations.

    Set DAI Playback Loopback Chaneels(TXLB) Configurations.

    @param[in]  channel        playback Loopback Channel selection
    @param[in]  config_id    playback Loopback config ID selection
    @param[in]  config_value  configuration value

    @return void
*/
UINT32 dai_get_txlb_config(DAI_TXLBCFG_ID config_id)
{
	T_DAI_FMTCFG2_REG   reg_fmt_2;
	UINT32              ret = 0;

	reg_fmt_2.reg = DAI_GETREG(DAI_FMTCFG2_REG_OFS);

	switch (config_id) {
	case DAI_TXLBCFG_ID_CHANNEL: {
			ret = reg_fmt_2.bit.TXLB_SOUNDM;
		}
		break;

	case DAI_TXLBCFG_ID_TOTAL_CH: {
			ret = reg_fmt_2.bit.TXLB_SOUNDCH;
		}
		break;

	case DAI_TXLBCFG_ID_PCMLEN: {
			ret = reg_fmt_2.bit.TXLB_PCMLEN;
		}
		break;

	case DAI_TXLBCFG_ID_DRAMCH: {
			ret = reg_fmt_2.bit.TXLB_DRAMCH;
		}
		break;

	case DAI_TXLBCFG_ID_RXSYNC: {
			T_DAI_CONFIG_REG    reg_cfg;

			reg_cfg.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);
			ret = reg_cfg.bit.TXLB_SYNC;
		}
		break;


	default:
		break;

	}

	return ret;
}

/**
    Set DAI Record Channels Configurations.

    Set DAI Record Channels (RX1 & RX2) Configurations.

    @param[in]  config_id    record config ID selection
    @param[in]  config_value  configuration value

    @return void
*/
void dai_set_rx_config(DAI_RXCFG_ID config_id, UINT32 config_value)
{
	T_DAI_FMTCFG1_REG reg_fmt_1;
	unsigned long flag;

	loc_cpu(flag);

	reg_fmt_1.reg = DAI_GETREG(DAI_FMTCFG1_REG_OFS);

	switch (config_id) {
	case DAI_RXCFG_ID_CHANNEL: {
			reg_fmt_1.bit.RX_SOUNDM = config_value;
		}
		break;

	case DAI_RXCFG_ID_TOTAL_CH: {
			T_DAI_CONFIG_REG reg_ctrl;

			reg_ctrl.reg = DAI_GETREG(DAI_CONFIG_REG_OFS);

			if ((reg_ctrl.bit.EXCODEC_EN == 0) && (config_value > DAI_TOTCH_2CH)) {
				DBG_WRN("Embedded codec 2CH only\r\n");
				config_value = DAI_TOTCH_2CH;
			}

			reg_fmt_1.bit.RX_SOUNDCH = config_value;
		}
		break;

	case DAI_RXCFG_ID_PCMLEN: {
			reg_fmt_1.bit.RX_PCMLEN = config_value;
		}
		break;

	case DAI_RXCFG_ID_DRAMCH: {
			reg_fmt_1.bit.RX_DRAMCH = config_value;
		}
		break;

	case DAI_RXCFG_ID_TIMECODE_TRIG: {
			DAI_SETREG(DAI_RXTCTRIGGER_REG_OFS, config_value);
		}
		break;

	case DAI_RXCFG_ID_TIMECODE_OFS: {
			DAI_SETREG(DAI_RXTCOFFSET_REG_OFS, config_value);
		}
		break;






	default:
		unl_cpu(flag);
		DBG_WRN("CfgID Err = %d\r\n", (int)config_id);
		return;

	}

	DAI_SETREG(DAI_FMTCFG1_REG_OFS, reg_fmt_1.reg);
	unl_cpu(flag);
}


/**
    Get DAI Record Channels Configurations.

    Get DAI Record Channels (RX1 & RX2) Configurations.

    @param[in]  config_id    record config ID selection

    @return configuration value
*/
UINT32 dai_get_rx_config(DAI_RXCFG_ID config_id)
{
	T_DAI_FMTCFG1_REG   reg_fmt_1;
	UINT32              ret = 0;

	reg_fmt_1.reg = DAI_GETREG(DAI_FMTCFG1_REG_OFS);

	switch (config_id) {
	case DAI_RXCFG_ID_CHANNEL: {
			ret = reg_fmt_1.bit.RX_SOUNDM;
		}
		break;

	case DAI_RXCFG_ID_TOTAL_CH: {
			ret = reg_fmt_1.bit.RX_SOUNDCH;
		}
		break;

	case DAI_RXCFG_ID_PCMLEN: {
			ret = reg_fmt_1.bit.RX_PCMLEN;
		}
		break;

	case DAI_RXCFG_ID_DRAMCH: {
			ret = reg_fmt_1.bit.RX_DRAMCH;
		}
		break;

	case DAI_RXCFG_ID_TIMECODE_TRIG: {
			ret = DAI_GETREG(DAI_RXTCTRIGGER_REG_OFS);
		}
		break;

	case DAI_RXCFG_ID_TIMECODE_OFS: {
			ret = DAI_GETREG(DAI_RXTCOFFSET_REG_OFS);
		}
		break;

	case DAI_RXCFG_ID_TIMECODE_VAL: {
			ret = DAI_GETREG(DAI_RXTCVALUE_REG_OFS);
		}
		break;


	default:
		break;

	}

	return ret;
}


/**
    Enable/Disable DAI playback DMA Channel

    Enable/Disable DAI playback DMA Channel

    @param[in]  channel        playback Channel selection
    @param[in]  b_en         TRUE is Enable. FALSE is Disable.

    @return void
*/
void dai_enable_tx_dma(DAI_TXCH channel, BOOL b_en)
{
	T_DAI_CTRL_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (channel == DAI_TXCH_TX1) {
		if (b_en == TRUE) {
			reg_ctrl.bit.DMA_TX1_EN = 1;
		} else {
			reg_ctrl.bit.DMA_TX1_EN = 0;
		}
	} else if (channel == DAI_TXCH_TX2) {
		if (b_en == TRUE) {
			reg_ctrl.bit.DMA_TX2_EN = 1;
		} else {
			reg_ctrl.bit.DMA_TX2_EN = 0;
		}
	}


	DAI_SETREG(DAI_CTRL_REG_OFS, reg_ctrl.reg);

	unl_cpu(flag);
}

/**
    Enable/Disable DAI record DMA Channel

    Enable/Disable DAI record DMA Channel

    @param[in]  b_en         TRUE is Enable. FALSE is Disable.

    @return void
*/
void dai_enable_rx_dma(BOOL b_en)
{
	T_DAI_CTRL_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (b_en == TRUE) {
		reg_ctrl.bit.DMA_RX_EN = 1;
	} else {
		reg_ctrl.bit.DMA_RX_EN = 0;
	}

	DAI_SETREG(DAI_CTRL_REG_OFS, reg_ctrl.reg);

	unl_cpu(flag);
}


/**
    Enable/Disable DAI playback loopback DMA Channel

    Enable/Disable DAI playback loopback DMA Channel

    @param[in]  b_en         TRUE is Enable. FALSE is Disable.

    @return void
*/
void dai_enable_txlb_dma(BOOL b_en)
{
	T_DAI_CTRL_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (b_en == TRUE) {
		reg_ctrl.bit.DMA_TXLB_EN = 1;
	} else {
		reg_ctrl.bit.DMA_TXLB_EN = 0;
	}

	DAI_SETREG(DAI_CTRL_REG_OFS, reg_ctrl.reg);

	unl_cpu(flag);
}


/**
    Start/Stop DAI Playback

    Start/Stop DAI Specified Channel Playback

    @param[in]  channel        playback Channel selection
    @param[in]  b_en         TRUE is Start. FALSE is Stop.

    @return void
*/
void dai_enable_tx(DAI_TXCH channel, BOOL b_en)
{
	T_DAI_CTRL_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (channel == DAI_TXCH_TX1) {
		if (b_en == TRUE) {
			reg_ctrl.bit.TX1_EN = 1;
		} else {
			reg_ctrl.bit.TX1_EN = 0;
			size_error[0] = 0;
		}
	} else if (channel == DAI_TXCH_TX2) {
		if (b_en == TRUE) {
			reg_ctrl.bit.TX2_EN = 1;
		} else {
			reg_ctrl.bit.TX2_EN = 0;
			size_error[1] = 0;
		}
	}

	DAI_SETREG(DAI_CTRL_REG_OFS, reg_ctrl.reg);

	unl_cpu(flag);
}


/**
    Start/Stop DAI Record

    Start/Stop DAI Specified Channel Record

    @param[in]  b_en         TRUE is Start. FALSE is Stop.

    @return void
*/
void dai_enable_rx(BOOL b_en)
{
	T_DAI_CTRL_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (b_en == TRUE) {
		reg_ctrl.bit.RX_EN = 1;
	} else {
		reg_ctrl.bit.RX_EN = 0;
	}

	DAI_SETREG(DAI_CTRL_REG_OFS, reg_ctrl.reg);

	unl_cpu(flag);
}

/**
    Start/Stop DAI playback loopback

    Start/Stop DAI Specified Channel Record

    @param[in]  b_en         TRUE is Start. FALSE is Stop.

    @return void
*/
void dai_enable_txlb(BOOL b_en)
{
	T_DAI_CTRL_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (b_en == TRUE) {
		reg_ctrl.bit.TXLB_EN = 1;
	} else {
		reg_ctrl.bit.TXLB_EN = 0;
	}

	DAI_SETREG(DAI_CTRL_REG_OFS, reg_ctrl.reg);

	unl_cpu(flag);
}

/**
    Set DAI module enable/disable

    Set DAI module enable/disable

    @param[in] b_en      DAI module enable/disable
     - @b TRUE:     module enable
     - @b FALSE:    module disable

    @return void
*/
void dai_enable_dai(BOOL b_en)
{
	T_DAI_CTRL_REG reg_ctrl;
	unsigned long flag;

	loc_cpu(flag);


	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (b_en == TRUE) {
		reg_ctrl.bit.DAIEN = 1;
	} else {
		reg_ctrl.bit.DAIEN = 0;
	}

	DAI_SETREG(DAI_CTRL_REG_OFS, reg_ctrl.reg);

	unl_cpu(flag);
}

#endif

#if 1

/**
    Check whether DAI is enabled or not

    If DAI is enabled this function will return TRUE.

    @return
	@b TRUE:  DAI is enabled
	@b FALSE: DAI is disabled

*/
BOOL dai_is_dai_enable(void)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	return reg_ctrl.bit.DAIEN;
}

/**
    Check if DAI Playback Channel is enabled

    Check if DAI Playback Channel is enabled

    @param[in]  channel        playback Channel selection

    @return
	@b TRUE:  Specified Playback channel is enabled
	@b FALSE: Specified Playback channel is disabled
*/
BOOL dai_is_tx_enable(DAI_TXCH channel)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (channel == DAI_TXCH_TX2) {
		return reg_ctrl.bit.TX2_EN;
	} else {
		return reg_ctrl.bit.TX1_EN;
	}
}

/**
    Check if DAI record Channel is enabled

    Check if DAI record Channel is enabled

    @return
	@b TRUE:  Record is enabled
	@b FALSE: Record is disabled
*/
BOOL dai_is_rx_enable(void)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	return reg_ctrl.bit.RX_EN;
}

/**
    Check if DAI playback loopback Channel is enabled

    Check if DAI playback loopback Channel is enabled

    @return
	@b TRUE:  playback loopback is enabled
	@b FALSE: playback loopback is disabled
*/
BOOL dai_is_txlb_enable(void)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	return reg_ctrl.bit.TXLB_EN;
}

/**
    Check whether DAI is under tx/rx

    If DAI is under tx/rx, this function will return TRUE.

    @return
	@b TRUE:  DAI is under tx/rx.
	@b FALSE: DAI is not under tx/rx.

*/
BOOL dai_is_txrx_enable(void)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	return (reg_ctrl.bit.TX1_EN || reg_ctrl.bit.TX2_EN || reg_ctrl.bit.RX_EN || reg_ctrl.bit.TXLB_EN);
}

/**
    Check if playback DMA Channel is Enabled

    Check if playback DMA Channel is Enabled

    @param[in]  channel        playback Channel selection

    @return
	@b TRUE:  Specified Playback DMA channel is enabled
	@b FALSE: Specified Playback DMA channel is disabled
*/
BOOL dai_is_tx_dma_enable(DAI_TXCH channel)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	if (channel == DAI_TXCH_TX2) {
		return reg_ctrl.bit.DMA_TX2_EN;
	} else {
		return reg_ctrl.bit.DMA_TX1_EN;
	}
}


/**
    Check if Record DMA Channel is Enabled

    Check if Record DMA Channel is Enabled

    @return
	@b TRUE:  Specified Record DMA channel is enabled
	@b FALSE: Specified Record DMA channel is disabled
*/
BOOL dai_is_rx_dma_enable(void)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	return reg_ctrl.bit.DMA_RX_EN;
}


/**
    Check if Playback Loopback DMA Channel is Enabled

    Check if Playback Loopback DMA Channel is Enabled

    @return
	@b TRUE:  Specified Playback Loopback DMA channel is enabled
	@b FALSE: Specified Playback Loopback DMA channel is disabled
*/
BOOL dai_is_txlb_dma_enable(void)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	return reg_ctrl.bit.DMA_TXLB_EN;
}


/**
    Check whether Tx or Rx DMA is enabled or not

    If Tx or Rx DMA is enabled this function will return TRUE.

    @return
	@b TRUE:  DMA is enabled
	@b FALSE: DMA is disabled
*/
BOOL dai_is_dma_enable(void)
{
	T_DAI_CTRL_REG reg_ctrl;

	reg_ctrl.reg = DAI_GETREG(DAI_CTRL_REG_OFS);

	return (reg_ctrl.bit.DMA_RX_EN || reg_ctrl.bit.DMA_TX1_EN || reg_ctrl.bit.DMA_TX2_EN || reg_ctrl.bit.DMA_TXLB_EN);
}



#endif

#if 1

/**
    Set Playback DMA parameter

    Set Playback DMA starting address, buffer size.

    @param[in] dma_channel       Playback DMA Channel Selection. Valid value is 0 or 1 for NT96680.
    @param[in] dma_start_addr      DMA start address. (unit: byte, should be word-alignment)
    @param[in] dma_buffer_size      DMA buffer size (unit: 16 words aligned)

    @return void
*/
void dai_set_tx_dma_para(UINT32 dma_channel, UINT32 dma_start_addr, UINT32 dma_buffer_size)
{
	T_DAI_TX1DMASTART_REG   reg_dma_start = {0};
	T_DAI_TX1DMABUFSIZE_REG reg_dma_bufsz  = {0};

	if (dma_start_addr & 0x03) {
		DBG_WRN("DMA address should be word align, but 0x%x, align it to 0x%x\r\n", (unsigned int)dma_start_addr, (unsigned int)(dma_start_addr & (~0x03)));
	}
	if (size_error[dma_channel]) {
		DBG_WRN("Previous DMA size must be multiples of 2 words (%d)(%d)\r\n", (int)dma_channel, (int)size_error[dma_channel]);
	}
	if (dma_buffer_size & 0x0F) {
		size_error[dma_channel] = dma_buffer_size;
	}

//#ifdef __KERNEL__
//	fmem_dcache_sync((void *)dma_start_addr, dma_buffer_size * 4, DMA_BIDIRECTIONAL);
//#elif defined(__FREERTOS)
//	dma_flushWriteCache(dma_start_addr, dma_buffer_size * 4);
//#endif

	reg_dma_start.bit.TX1DMASTADR = dma_get_phy_addr(dma_start_addr);
	reg_dma_bufsz.bit.TX1DMABUFSZ = dma_buffer_size;



	if (dma_channel == 0) {
		DAI_SETREG(DAI_TX1DMASTART_REG_OFS,   reg_dma_start.reg);
		DAI_SETREG(DAI_TX1DMABUFSIZE_REG_OFS, reg_dma_bufsz.reg);
	} else if (dma_channel == 1) {
		DAI_SETREG(DAI_TX2DMASTART_REG_OFS,   reg_dma_start.reg);
		DAI_SETREG(DAI_TX2DMABUFSIZE_REG_OFS, reg_dma_bufsz.reg);
	}

#if DAI_DBG_MSG
	DBG_WRN("[TX%d] Addr=0x%08X Size=0x%08X(words)\r\n", (int)dma_channel + 1, (unsigned int)dma_start_addr, (int)dma_buffer_size);
#endif
}

/**
    Set Record DMA parameter

    Set Record DMA starting address, buffer size.

    @param[in] dma_channel       Record DMA Channel Selection. Valid value is 0 or 1 for NT96680.
    @param[in] dma_start_addr      DMA start address. (unit: byte, should be word-alignment)
    @param[in] dma_buffer_size      DMA buffer size (unit: 16 words aligned)

    @return void
*/
void dai_set_rx_dma_para(UINT32 dma_channel, UINT32 dma_start_addr, UINT32 dma_buffer_size)
{
	T_DAI_RX1DMASTART_REG   reg_dma_start = {0};
	T_DAI_RX1DMABUFSIZE_REG reg_dma_bufsz = {0};

	if (dma_start_addr & 0x03) {
		DBG_WRN("DMA address should be word align, but 0x%x, align it to 0x%x\r\n", (unsigned int)dma_start_addr, (unsigned int)(dma_start_addr & (~0x03)));
	}
	if (dma_buffer_size & 0x0F) {
		DBG_WRN("DMA size must be multiples of 2 words (%d)\r\n", (int)dma_buffer_size);
	}

//#ifdef __KERNEL__
//	fmem_dcache_sync((void *)dma_start_addr, dma_buffer_size * 4, DMA_BIDIRECTIONAL);
//#elif defined(__FREERTOS)
//	dma_flushReadCache(dma_start_addr, dma_buffer_size * 4);
//#endif

	reg_dma_start.bit.RX1DMASTADR  = dma_get_phy_addr(dma_start_addr);
	reg_dma_bufsz.bit.RX1DMABUFSZ   = dma_buffer_size;



	if (dma_channel == 0) {
		DAI_SETREG(DAI_RX1DMASTART_REG_OFS,   reg_dma_start.reg);
		DAI_SETREG(DAI_RX1DMABUFSIZE_REG_OFS, reg_dma_bufsz.reg);
	} else if (dma_channel == 1) {
		DAI_SETREG(DAI_RX2DMASTART_REG_OFS,   reg_dma_start.reg);
		DAI_SETREG(DAI_RX2DMABUFSIZE_REG_OFS, reg_dma_bufsz.reg);
	}

#if DAI_DBG_MSG
	DBG_WRN("[RX%d] Addr=0x%08X Size=0x%08X(words)\r\n", (int)dma_channel + 1, (unsigned int)dma_start_addr, (int)dma_buffer_size);
#endif

}

/**
    Set Playback Loopback DMA parameter

    Set Playback Loopback DMA starting address, buffer size.

    @param[in] dma_start_addr      DMA start address. (unit: byte, should be word-alignment)
    @param[in] dma_buffer_size      DMA buffer size (unit: 16 words aligned)

    @return void
*/
void dai_set_txlb_dma_para(UINT32 dma_start_addr, UINT32 dma_buffer_size)
{
	T_DAI_TXLBDMASTART_REG   reg_dma_start = {0};
	T_DAI_TXLBDMABUFSIZE_REG reg_dma_bufsz  = {0};

	if (dma_start_addr & 0x03) {
		DBG_WRN("DMA address should be word align, but 0x%x, align it to 0x%x\r\n", (unsigned int)dma_start_addr, (unsigned int)(dma_start_addr & (~0x03)));
	}
	if (dma_buffer_size & 0x0F) {
		DBG_WRN("DMA size must be multiples of 2 words (%d)\r\n", (int)dma_buffer_size);
	}

//#ifdef __KERNEL__
//	fmem_dcache_sync((void *)dma_start_addr, dma_buffer_size * 4, DMA_BIDIRECTIONAL);
//#elif defined(__FREERTOS)
//	dma_flushReadCache(dma_start_addr, dma_buffer_size * 4);
//#endif

	reg_dma_start.bit.TXLBDMASTADR = dma_get_phy_addr(dma_start_addr);
	reg_dma_bufsz.bit.TXLBDMABUFSZ  = dma_buffer_size;



	DAI_SETREG(DAI_TXLBDMASTART_REG_OFS,   reg_dma_start.reg);
	DAI_SETREG(DAI_TXLBDMABUFSIZE_REG_OFS, reg_dma_bufsz.reg);

#if DAI_DBG_MSG
	DBG_WRN("[TXLB] Addr=0x%08X Size=0x%08X(words)\r\n", (unsigned int)dma_start_addr, (int)dma_buffer_size);
#endif
}


/**
    Get Playback DMA parameter

    Get Playback DMA starting address, buffer size.

    @param[in] dma_channel       Playback DMA Channel Selection. Valid value is 0 or 1 for NT96680.
    @param[out] dma_start_addr     DMA start address. (unit: byte, should be word-alignment)
    @param[out] dma_buffer_size     DMA buffer size (unit: word)

    @return void
*/
void dai_get_tx_dma_para(UINT32 dma_channel, UINT32 *p_start_addr, UINT32 *p_buffer_size)
{
	if (dma_channel == 0) {
		if (p_start_addr != NULL) {
			*p_start_addr = dma_get_noncache_addr(DAI_GETREG(DAI_TX1DMASTART_REG_OFS));
		}

		if (p_buffer_size != NULL) {
			*p_buffer_size = DAI_GETREG(DAI_TX1DMABUFSIZE_REG_OFS);
		}
	} else if (dma_channel == 1) {
		if (p_start_addr != NULL) {
			*p_start_addr = dma_get_noncache_addr(DAI_GETREG(DAI_TX2DMASTART_REG_OFS));
		}

		if (p_buffer_size != NULL) {
			*p_buffer_size = DAI_GETREG(DAI_TX2DMABUFSIZE_REG_OFS);
		}
	}
}


/**
    Get Record DMA parameter

    Get Record DMA starting address, buffer size.

    @param[in] dma_channel       Record DMA Channel Selection. Valid value is 0 or 1 for NT96680.
    @param[out] dma_start_addr      DMA start address. (unit: byte, should be word-alignment)
    @param[out] dma_buffer_size      DMA buffer size (unit: word)

    @return void
*/
void dai_get_rx_dma_para(UINT32 dma_channel, UINT32 *p_start_addr, UINT32 *p_buffer_size)
{
	if (dma_channel == 0) {
		if (p_start_addr != NULL) {
			*p_start_addr = dma_get_noncache_addr(DAI_GETREG(DAI_RX1DMASTART_REG_OFS));
		}

		if (p_buffer_size != NULL) {
			*p_buffer_size = DAI_GETREG(DAI_RX1DMABUFSIZE_REG_OFS);
		}
	} else if (dma_channel == 1) {
		if (p_start_addr != NULL) {
			*p_start_addr = dma_get_noncache_addr(DAI_GETREG(DAI_RX2DMASTART_REG_OFS));
		}

		if (p_buffer_size != NULL) {
			*p_buffer_size = DAI_GETREG(DAI_RX2DMABUFSIZE_REG_OFS);
		}
	}
}

/**
    Get Playback Loopback DMA parameter

    Get Playback Loopback DMA starting address, buffer size.

    @param[in] dma_start_addr      DMA start address. (unit: byte, should be word-alignment)
    @param[in] dma_buffer_size      DMA buffer size (unit: word)

    @return void
*/
void dai_get_txlb_dma_para(UINT32 *p_start_addr, UINT32 *p_buffer_size)
{
	if (p_start_addr != NULL) {
		*p_start_addr = dma_get_noncache_addr(DAI_GETREG(DAI_TXLBDMASTART_REG_OFS));
	}

	if (p_buffer_size != NULL) {
		*p_buffer_size = DAI_GETREG(DAI_TXLBDMABUFSIZE_REG_OFS);
	}
}


/**
    Get Playback DMA currrent address

    Get Playback DMA currrent address

    @param[in] dma_channel   Playback DMA Channel Selection. Valid value is 0 or 1 for NT96660.

    @return Playback DMA current address
*/
UINT32 dai_get_tx_dma_curaddr(UINT32 dma_channel)
{
	if (dma_channel == 1) {
		return dma_get_noncache_addr(DAI_GETREG(DAI_TX2DMACURRENT_REG_OFS));
	} else {
		return dma_get_noncache_addr(DAI_GETREG(DAI_TX1DMACURRENT_REG_OFS));
	}
}

/**
    Get Record DMA currrent address

    Get Record DMA currrent address

    @param[in] dma_channel   Record DMA Channel Selection. Valid value is 0 or 1 for NT96660.

    @return Record DMA current address
*/
UINT32 dai_get_rx_dma_curaddr(UINT32 dma_channel)
{
	if (dma_channel == 1) {
		return dma_get_noncache_addr(DAI_GETREG(DAI_RX2DMACURRENT_REG_OFS));
	} else {
		return dma_get_noncache_addr(DAI_GETREG(DAI_RX1DMACURRENT_REG_OFS));
	}
}


/**
    Get Playback Loopback DMA currrent address

    Get Playback Loopback DMA currrent address

    @return Playback Loopback DMA current address
*/
UINT32 dai_get_txlb_dma_curaddr(void)
{
	return dma_get_noncache_addr(DAI_GETREG(DAI_TXLBDMACURRENT_REG_OFS));
}

/**
    Clear DAI flag

    Clear DAI flag

    @param[in] int_flag  interrupt flags to clear.

    @return void
*/
void dai_clr_flg(DAI_INTERRUPT int_flag)
{
	// Clear Interrupt Flag

	clr_flg(FLG_ID_DAI, int_flag);
}


#endif
//@}

/*
    DAI self Debug Mode Enable

    After DAI Debug enabled, the DAI RX channel's input is switched from CODEC interface to DAI Playback Mixer output.
*/
void dai_debug(BOOL b_en)
{
	T_DAI_DBG_CONFIG_REG    reg_dbg;

	reg_dbg.reg = DAI_GETREG(DAI_DBG_CONFIG_REG_OFS);
	reg_dbg.bit.DBGEN = b_en > 0;
	DAI_SETREG(DAI_DBG_CONFIG_REG_OFS, reg_dbg.reg);
}

/*
    DAI Debug Channel Enable for EAC

    b_mode_ad is TRUE for AD. b_mode_ad is FALSE for DA.
*/
void dai_debug_eac(BOOL b_en, BOOL b_mode_ad)
{
	T_DAI_DBG_CONFIG_REG    reg_dbg;

	reg_dbg.reg = DAI_GETREG(DAI_DBG_CONFIG_REG_OFS);
	reg_dbg.bit.DBGEN        = 0;
	reg_dbg.bit.EAC_DBG_EN   = b_en > 0;
	reg_dbg.bit.EAC_DBG_MODE = b_mode_ad > 0;
	DAI_SETREG(DAI_DBG_CONFIG_REG_OFS, reg_dbg.reg);
}

/*
    Set Debug Channel DMA parameter

    Set Debug Channel DMA starting address, buffer size.
*/
void dai_set_debug_dma_para(UINT32 dma_start_addr, UINT32 dma_buffer_size)
{
	T_DAI_DBG_ADDR_REG   reg_dma_start = {0};
	T_DAI_DBG_SIZE_REG   reg_dma_bufsz  = {0};

	reg_dma_start.bit.DMASTADR     = dma_get_phy_addr(dma_start_addr);
	reg_dma_bufsz.bit.DMABUFSZ      = dma_buffer_size;

	if (dma_start_addr) {
		DAI_SETREG(DAI_DBG_ADDR_REG_OFS,    reg_dma_start.reg);
	}

	if (dma_buffer_size) {
		DAI_SETREG(DAI_DBG_SIZE_REG_OFS,    reg_dma_bufsz.reg);
	}
}

/*
    Get Debug Done Status parameter
*/
BOOL dai_get_debug_status(void)
{
	T_DAI_DBG_STS_REG   reg_status = {0};

	reg_status.reg = DAI_GETREG(DAI_DBG_STS_REG_OFS);
	return reg_status.bit.DONE;
}

/*
    Get Debug Done Status parameter
*/
void dai_clr_debug_status(void)
{
	T_DAI_DBG_STS_REG   reg_status;

	reg_status.reg = DAI_GETREG(DAI_DBG_STS_REG_OFS);
	reg_status.bit.DONE = 1;
	DAI_SETREG(DAI_DBG_STS_REG_OFS, reg_status.reg);
}


#if 0//def __KERNEL__
EXPORT_SYMBOL(dai_create_resource);
EXPORT_SYMBOL(dai_release_resource);
EXPORT_SYMBOL(dai_enableclk);
EXPORT_SYMBOL(dai_disableclk);
EXPORT_SYMBOL(dai_setclkrate);
EXPORT_SYMBOL(dai_isr);
EXPORT_SYMBOL(dai_lock);
EXPORT_SYMBOL(dai_unlock);
EXPORT_SYMBOL(dai_wait_interrupt);
EXPORT_SYMBOL(dai_select_pinmux);
EXPORT_SYMBOL(dai_select_mclk_pinmux);
EXPORT_SYMBOL(dai_open);
EXPORT_SYMBOL(dai_close);
EXPORT_SYMBOL(dai_set_config);
EXPORT_SYMBOL(dai_get_config);
EXPORT_SYMBOL(dai_set_i2s_config);
EXPORT_SYMBOL(dai_get_i2s_config);
EXPORT_SYMBOL(dai_set_tx_config);
EXPORT_SYMBOL(dai_get_tx_config);
EXPORT_SYMBOL(dai_set_txlb_config);
EXPORT_SYMBOL(dai_get_txlb_config);
EXPORT_SYMBOL(dai_set_rx_config);
EXPORT_SYMBOL(dai_get_rx_config);
EXPORT_SYMBOL(dai_enable_tx_dma);
EXPORT_SYMBOL(dai_enable_rx_dma);
EXPORT_SYMBOL(dai_enable_txlb_dma);
EXPORT_SYMBOL(dai_enable_tx);
EXPORT_SYMBOL(dai_enable_rx);
EXPORT_SYMBOL(dai_enable_txlb);
EXPORT_SYMBOL(dai_enable_dai);
EXPORT_SYMBOL(dai_is_dai_enable);
EXPORT_SYMBOL(dai_is_tx_enable);
EXPORT_SYMBOL(dai_is_rx_enable);
EXPORT_SYMBOL(dai_is_txlb_enable);
EXPORT_SYMBOL(dai_is_txrx_enable);
EXPORT_SYMBOL(dai_is_tx_dma_enable);
EXPORT_SYMBOL(dai_is_rx_dma_enable);
EXPORT_SYMBOL(dai_is_txlb_dma_enable);
EXPORT_SYMBOL(dai_is_dma_enable);
EXPORT_SYMBOL(dai_set_tx_dma_para);
EXPORT_SYMBOL(dai_set_rx_dma_para);
EXPORT_SYMBOL(dai_set_txlb_dma_para);
EXPORT_SYMBOL(dai_get_tx_dma_para);
EXPORT_SYMBOL(dai_get_rx_dma_para);
EXPORT_SYMBOL(dai_get_txlb_dma_para);
EXPORT_SYMBOL(dai_get_tx_dma_curaddr);
EXPORT_SYMBOL(dai_get_rx_dma_curaddr);
EXPORT_SYMBOL(dai_get_txlb_dma_curaddr);
EXPORT_SYMBOL(dai_clr_flg);
EXPORT_SYMBOL(dai_debug);
EXPORT_SYMBOL(dai_debug_eac);
EXPORT_SYMBOL(dai_set_debug_dma_para);
EXPORT_SYMBOL(dai_get_debug_status);
EXPORT_SYMBOL(dai_clr_debug_status);
#endif



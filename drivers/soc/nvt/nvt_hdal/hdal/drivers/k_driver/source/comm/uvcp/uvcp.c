#include <kwrap/util.h>
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>

#include "uvcp.h"
#include "uvcp_reg.h"
#include "uvcp_drv.h"
#include "uvcp_main.h"
#include "uvcp_dbg.h"


#include "uvcp_int.h"


#define __MODULE__ nvt_uvcp
#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"

unsigned int nvt_uvcp_debug_level = NVT_DBG_WRN;


static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

extern UINT32 _UVCP_REG_BASE_ADDR[MODULE_REG_NUM];
#define UVCP_SETREG(_OFS, value)          OUTW(_UVCP_REG_BASE_ADDR[0]+(_OFS), (value))
#define UVCP_GETREG(_OFS)                 INW(_UVCP_REG_BASE_ADDR[0]+(_OFS))


static BOOL _uvcp_opened;

static UINT32 uvcp_max_packet_size = 1024;
static UINT32 uvcp_reference_clk = 30; // 30 Mhz
static UINT32 uvcp_counter_start;
static BOOL   uvcp_fid;
static UINT32 uvcp_last_counter_val;
static UINT32 uvcp_last_sof_val;
static UINT32 uvcp_output_address;

//static ID SEMID_UVCP;
static ID FLG_ID_UVCP;

#ifdef _NVT_EMULATION_
static UINT32 uvcp_lastout;
#endif

irqreturn_t uvcp_isr(int irq, void *devid)
{
	T_UVCP_INTSTS_REG regintsts;

	//DBG_DUMP("uvcp_isr 1\r\n");

	regintsts.reg  = UVCP_GETREG(UVCP_INTSTS_REG_OFS);
	regintsts.reg &= UVCP_GETREG(UVCP_INTEN_REG_OFS);
	UVCP_SETREG(UVCP_INTSTS_REG_OFS, regintsts.reg);


	if (regintsts.bit.BREAKPOINT) {
		//DBG_DUMP("B");
		iset_flg(FLG_ID_UVCP, FLGPTN_UVCP_BRK);
	}

	if(regintsts.bit.COMPLETE) {
#ifdef _NVT_EMULATION_
		UINT8 *pbuf;
		UINT32 bound;

		bound = UVCP_GETREG(UVCP_OUTSIZE_REG_OFS);
		if((bound%uvcp_max_packet_size) > 0)
			bound = bound - (bound%uvcp_max_packet_size);
		else
			bound = bound - uvcp_max_packet_size;

		pbuf = (UINT8 *) (uvcp_output_address+bound);
		bound = (pbuf[9]<<24)+(pbuf[8]<<16)+(pbuf[7]<<8)+(pbuf[6]<<0);
		uvcp_last_counter_val = bound;
		//DBG_DUMP("LSTCNT=0x%08X\r\n", (int)uvcp_last_counter_val);

		bound = (pbuf[11]<<8)+(pbuf[10]<<0);
		uvcp_last_sof_val = bound & 0x7FF;

//#ifdef _NVT_EMULATION_
		uvcp_lastout = UVCP_GETREG(UVCP_OUTSIZE_REG_OFS);
#endif

		iset_flg(FLG_ID_UVCP, FLGPTN_UVCP);
	}

	return IRQ_HANDLED;
}

#if 1//ndef __KERNEL__
static BOOL uvcp_init_done = 0;
void uvcp_init(void)
{
	vos_flag_create(&FLG_ID_UVCP, NULL, "FLG_ID_UVCP");
	//vos_sem_create(&SEMID_UVCP, 1, "SEMID_UVCP");
}

void uvcp_uninit(void)
{
	//vos_sem_destroy(SEMID_UVCP);
	vos_flag_destroy(FLG_ID_UVCP);
}
#endif

#if UVCP_POLLING
static ER uvcp_lock(void)
{
#ifdef __KERNEL__
	SEM_WAIT(SEMID_UVCP);
#else
	vos_sem_wait(SEMID_UVCP);
#endif
	return E_OK;
}

/*
    LVDS ReSource Un-Lock
*/
static ER uvcp_unlock(void)
{
#ifdef __KERNEL__
	SEM_SIGNAL(SEMID_UVCP);
#else
	vos_sem_sig(SEMID_UVCP);
#endif
	return E_OK;
}
#endif


ER uvcp_open(void)
{
	if(_uvcp_opened) {
		DBG_ERR("driver already opened\r\n");
		return E_OACV;
	}

	_uvcp_opened = TRUE;

	if(!uvcp_init_done) {
		uvcp_init();
		uvcp_init_done = 1;
	}

#ifndef __KERNEL__
	// open clock
	pll_enable_clock(UVCP_CLKEN);

	// open irq
	request_irq(INT_ID_UVCP, uvcp_isr ,IRQF_TRIGGER_HIGH, "uvcp", 0);
#else


#endif

	#if !UVCP_POLLING
	UVCP_SETREG(UVCP_INTEN_REG_OFS, 0x3);
	#endif

	// controller init
	UVCP_SETREG(UVCP_BREAKSIZE_REG_OFS, 4);

	return E_OK;
}


ER uvcp_close(void)
{
	if(!_uvcp_opened) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

#ifndef __KERNEL__
	free_irq(INT_ID_UVCP, 0);

	pll_disable_clock(UVCP_CLKEN);
#endif

	_uvcp_opened = FALSE;
	return E_OK;
}

BOOL uvcp_is_opened(void)
{
	return _uvcp_opened;
}

ER uvcp_trigger(BOOL eof, BOOL wait_break)
{
	T_UVCP_OP_REG		regop;
	T_UVCP_CTRL2_REG	regctrl2;
	unsigned long		flags;
	FLGPTN              uiflag;

	loc_cpu(flags);

	regop.reg = UVCP_GETREG(UVCP_OP_REG_OFS);
	if(regop.bit.START) {
		unl_cpu(flags);
		DBG_ERR("\r\n");
		return E_OACV;
	}

	#if UVCP_POLLING
	UVCP_SETREG(UVCP_INTSTS_REG_OFS, 0x3);
	#endif

	regctrl2.reg = UVCP_GETREG(UVCP_CTRL2_REG_OFS);
	regctrl2.bit.EOF_SET = eof;
	regctrl2.bit.FID_VAL = uvcp_fid;
	UVCP_SETREG(UVCP_CTRL2_REG_OFS, regctrl2.reg);

	clr_flg(FLG_ID_UVCP, FLGPTN_UVCP|FLGPTN_UVCP_BRK);

	regop.bit.START = 1;
	UVCP_SETREG(UVCP_OP_REG_OFS, regop.reg);

	unl_cpu(flags);

	if(wait_break) {
		vos_flag_wait_timeout(&uiflag, FLG_ID_UVCP, FLGPTN_UVCP_BRK, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(2000));
		if (uiflag == 0) {
			DBG_ERR("wait_break timeout\r\n");
			return E_SYS;
		}
	}

	if (eof) {
		uvcp_fid = !uvcp_fid;
	}

	return E_OK;
}


ER uvcp_wait_complete(void)
{
#if UVCP_POLLING
	UINT32 timeot = 0;
	UINT32 regval;

	uvcp_lock();

	do {
		regval = UVCP_GETREG(UVCP_INTSTS_REG_OFS);
		timeot++;
		if(timeot > 10000000)
			break;
	} while(!(regval & 0x1));

	if(regval & 0x1) {
		UVCP_SETREG(UVCP_INTSTS_REG_OFS, 0x1);
	} else {
		uvcp_unlock();
		DBG_ERR("uvcp_wait_complete timeout\r\n");
		return E_SYS;
	}

	uvcp_unlock();

#else

	FLGPTN              uiflag;

	vos_flag_wait_timeout(&uiflag, FLG_ID_UVCP, FLGPTN_UVCP, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(2000));

	if (uiflag == 0) {
		DBG_ERR("uvcp_wait_complete timeout\r\n");
		return E_SYS;
	}
#endif

	return E_OK;
}

ER uvcp_wait_break(void)
{
	FLGPTN              uiflag;

	vos_flag_wait_timeout(&uiflag, FLG_ID_UVCP, FLGPTN_UVCP_BRK, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(2000));

	if (uiflag == 0) {
		DBG_ERR("uvcp_wait_break timeout\r\n");
		return E_SYS;
	}

	return E_OK;
}


ER uvcp_set_config(UVCP_CONFIG_ID configID, UINT32 configContext)
{
	unsigned long	flags;

	loc_cpu(flags);

	switch (configID) {
	case UVCP_CONFIG_ID_HEADER_LENGTH:
	{
		T_UVCP_CTRL0_REG regctrl0;
		T_UVCP_CTRL2_REG regctrl2;

		if ((configContext == 2)||(configContext == 12)) {
			regctrl2.reg = UVCP_GETREG(UVCP_CTRL2_REG_OFS);
			regctrl2.bit.HEADER_LEN_VALUE 	= configContext;
			regctrl2.bit.PTS_VAL 			= (configContext>2);
			regctrl2.bit.SCR_VAL 			= (configContext>2);
			#if 0
			regctrl2.bit.RES_VAL 			= 1;
			regctrl2.bit.STI_VAL 			= 1;
			regctrl2.bit.ERR_VAL 			= 1;
			regctrl2.bit.EOH_VAL 			= 0;
			DBG_DUMP("TESTONLY\r\n");
			#endif
			UVCP_SETREG(UVCP_CTRL2_REG_OFS, regctrl2.reg);

			regctrl0.reg = UVCP_GETREG(UVCP_CTRL0_REG_OFS);
			regctrl0.bit.HEADER_LEN_CTRL = (configContext>2);
			regctrl0.bit.PAYLOAD_SLICE_SIZE = uvcp_max_packet_size - regctrl2.bit.HEADER_LEN_VALUE;
			UVCP_SETREG(UVCP_CTRL0_REG_OFS, regctrl0.reg);

		} else {
			DBG_ERR("err header length %d\r\n", configContext);
		}
	} break;

	case UVCP_CONFIG_ID_DATALEN_PER_UF:
	{
		T_UVCP_CTRL0_REG regctrl0;
		T_UVCP_CTRL2_REG regctrl2;

		uvcp_max_packet_size = configContext;

		regctrl2.reg = UVCP_GETREG(UVCP_CTRL2_REG_OFS);

		regctrl0.reg = UVCP_GETREG(UVCP_CTRL0_REG_OFS);
		regctrl0.bit.PAYLOAD_SLICE_SIZE = uvcp_max_packet_size - regctrl2.bit.HEADER_LEN_VALUE;
		UVCP_SETREG(UVCP_CTRL0_REG_OFS, regctrl0.reg);
	} break;

	case UVCP_CONFIG_ID_FID:
	{
		uvcp_fid = (configContext>0);
	} break;

	case UVCP_CONFIG_ID_REFERENCE_CLK:
	{
		T_UVCP_CTRL1_REG regctrl1;
		T_UVCP_CTRL4_REG regctrl4;
		UINT32 count;

		uvcp_reference_clk = configContext;

		count = (UINT32)(125* uvcp_reference_clk);

		regctrl1.reg = UVCP_GETREG(UVCP_CTRL1_REG_OFS);
		regctrl1.bit.SCR_INCRE31_0 = count;
		#if 0
		regctrl1.bit.SCR_INCRE42_32 = 0x2;
		regctrl1.bit.SCR_REPEAT     = 0x1;
		DBG_DUMP("TESTONLY\r\n");
		#endif
		UVCP_SETREG(UVCP_CTRL1_REG_OFS, regctrl1.reg);

		regctrl4.reg = UVCP_GETREG(UVCP_CTRL4_REG_OFS);
		regctrl4.bit.SCR31_0_START_VAL = uvcp_counter_start - regctrl1.bit.SCR_INCRE31_0;
		UVCP_SETREG(UVCP_CTRL4_REG_OFS, regctrl4.reg);
	}
	break;

	case UVCP_CONFIG_ID_SOF_START_VAL:
	{
		T_UVCP_CTRL5_REG regctrl5;

		regctrl5.reg = UVCP_GETREG(UVCP_CTRL5_REG_OFS);
		regctrl5.bit.SCR42_32_START_VAL = configContext-1;
		UVCP_SETREG(UVCP_CTRL5_REG_OFS, regctrl5.reg);
	}
	break;

	case UVCP_CONFIG_ID_COUNTER_START_VAL:
	{
		T_UVCP_CTRL4_REG regctrl4;
		T_UVCP_CTRL1_REG regctrl1;

		uvcp_counter_start = configContext;

		regctrl1.reg = UVCP_GETREG(UVCP_CTRL1_REG_OFS);
		regctrl4.reg = UVCP_GETREG(UVCP_CTRL4_REG_OFS);
		regctrl4.bit.SCR31_0_START_VAL = uvcp_counter_start - regctrl1.bit.SCR_INCRE31_0;
		UVCP_SETREG(UVCP_CTRL4_REG_OFS, regctrl4.reg);
	}
	break;

	case UVCP_CONFIG_ID_BREAK_SIZE:
	{
		if(configContext == 0)
			configContext = 2;

		UVCP_SETREG(UVCP_BREAKSIZE_REG_OFS, configContext);
	}
	break;

	case UVCP_CONFIG_ID_PTS:
	{
		UVCP_SETREG(UVCP_CTRL3_REG_OFS, configContext);
	}
	break;

	default:
		break;
	}


	unl_cpu(flags);

	return E_OK;
}



UINT32 uvcp_get_config(UVCP_CONFIG_ID configID)
{
	UINT32 ret = 0;

	switch (configID) {
	case UVCP_CONFIG_ID_GETOUT_SIZE:
	{
		ret = UVCP_GETREG(UVCP_OUTSIZE_REG_OFS);
#ifdef _NVT_EMULATION_
		if(ret == 0)
			ret = uvcp_lastout;
#endif
	} break;

	case UVCP_CONFIG_ID_FID:
	{
		ret = uvcp_fid;
	} break;

	case UVCP_CONFIG_ID_GET_COUNTER_END:
	{
		ret = uvcp_last_counter_val;
	} break;

	case UVCP_CONFIG_ID_GET_SOF_END:
	{
		ret = uvcp_last_sof_val;
	} break;

	case UVCP_CONFIG_ID_PTS:
	{
		ret = UVCP_GETREG(UVCP_CTRL3_REG_OFS);
	}
	break;

	case UVCP_CONFIG_ID_GET_CNTSTEP:
	{
		T_UVCP_CTRL1_REG regctrl1;

		regctrl1.reg = UVCP_GETREG(UVCP_CTRL1_REG_OFS);
		ret = regctrl1.bit.SCR_INCRE31_0;
	}
	break;

	case UVCP_CONFIG_ID_COUNTER_START_VAL:
	{
		ret = uvcp_counter_start;
	}
	break;

	case UVCP_CONFIG_ID_SOF_START_VAL:
	{
		T_UVCP_CTRL5_REG regctrl5;

		regctrl5.reg = UVCP_GETREG(UVCP_CTRL5_REG_OFS);
		ret = (regctrl5.bit.SCR42_32_START_VAL + 1)&0x7FF;
	}
	break;

	default:
		DBG_ERR("id err %d\r\n", (int)configID);
		break;

	}

	return ret;
}



void uvcp_config_dma(UINT32 input_address, UINT32 input_size, UINT32 output_address, UINT32 *output_size)
{
	uvcp_output_address = output_address;

	dma_flushWriteCache((UINT32)input_address, input_size);
#if _UVCP_KERN_SELFTEST
	dma_flushReadCache((UINT32)input_address, input_size);
	dma_flushWriteCache((UINT32)output_address, *output_size);
#endif
	dma_flushReadCache((UINT32)output_address, *output_size);

	UVCP_SETREG(UVCP_INADDR_REG_OFS, dma_getPhyAddr((UINT32)input_address));
	UVCP_SETREG(UVCP_INSIZE_REG_OFS, input_size);
	UVCP_SETREG(UVCP_OUTADDR_REG_OFS,dma_getPhyAddr((UINT32)output_address));

}





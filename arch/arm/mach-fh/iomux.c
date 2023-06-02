#include <mach/iomux.h>
#include <mach/pmu.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/io.h>


Iomux_Pad fh_iomux_cfg[] = {
	{
		.func_name = { "RESETN", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = -1,
	},
	{
		.func_name = { "TEST", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = -1,
	},
	{
		.func_name = { "CIS_CLK", "", "", "", },
		.reg_type  = 5,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_HSYNC", "GPIO20", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_VSYNC", "GPIO21", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_PCLK", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 0,
	},
	{
		.func_name = { "CIS_D0", "GPIO22", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D1", "GPIO23", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D2", "GPIO24", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D3", "GPIO25", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D4", "GPIO26", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D5", "GPIO27", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D6", "GPIO28", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D7", "GPIO29", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D8", "GPIO30", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D9", "GPIO31", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D10", "GPIO32", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_D11", "GPIO33", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_REF_CLK", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 3,
	},
	{
		.func_name = { "MAC_MDC", "GPIO34", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 0,
	},
	{
		.func_name = { "MAC_MDIO", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_COL", "GPIO35", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_CRS", "GPIO36", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_RXCK", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = -1,
	},
	{
		.func_name = { "MAC_RXD0", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = -1,
	},

	{
		.func_name = { "MAC_RXD1", "GPIO38", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_RXD2", "GPIO39", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_RXD3", "GPIO40", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_RXDV", "GPIO41", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_TXCK", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = -1,
	},
	{
		.func_name = { "MAC_TXD0", "GPIO42", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_TXD1", "GPIO43", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_TXD2", "GPIO44", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_TXD3", "GPIO45", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_TXEN", "GPIO46", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "MAC_RXER", "GPIO47", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO0", "ARC_JTAG_TCK", "GPIO0", "CIS_SSI0_CSN1", },
		.reg_type  = 21,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO1", "ARC_JTAG_TRSTN", "GPIO1", "CIS_SSI0_RXD", },
		.reg_type  = 21,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO2", "ARC_JTAG_TMS", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO3", "ARC_JTAG_TDI", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO4", "ARC_JTAG_TDO", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "JTAG_TCK", "GPIO5", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "JTAG_TRSTN", "GPIO6", "PWM_OUT3", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "JTAG_TMS", "GPIO7", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "JTAG_TDI", "GPIO8", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "JTAG_TDO", "GPIO9", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO10", "UART1_OUT", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO11", "UART1_IN", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO12", "PWM_OUT0", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO13", "PWM_OUT1", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "GPIO14", "PWM_OUT2", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "UART0_IN", "GPIO48", "UART0_IN", " I2S_WS", },
		.reg_type  = 21,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 1,
	},
	{
		.func_name = { "UART0_OUT", "GPIO49", "UART0_OUT", "I2S_CLK", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_SCL", "GPIO56", "CIS_SCL", "CIS_SSI0_CLK", },
		.reg_type  = 13,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "CIS_SDA", "GPIO57", "CIS_SDA", "CIS_SSI0_TXD", },
		.reg_type  = 13,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SCL1", "GPIO50", "SCL1", "I2S_DI", },
		.reg_type  = 21,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SDA1", "GPIO51", "I2S_DO", "", },
		.reg_type  = 21,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SSI0_CLK", "", "", "", },
		.reg_type  = 5,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SSI0_TXD", "", "", "", },
		.reg_type  = 5,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SSI0_CSN0", "GPIO54", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SSI0_CSN1", "GPIO55", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SSI0_RXD", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = -1,
	},
	{
		.func_name = { "SD0_CD", "GPIO52", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SD0_WP", "GPIO53", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SD0_CLK", "", "", "", },
		.reg_type  = 5,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD0_CMD_RSP", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD0_DATA0", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD0_DATA1", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 2,
	},
	{
		.func_name = { "SD0_DATA2", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD0_DATA3", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD1_CLK", "SSI1_CLK", "", "", },
		.reg_type  = 8,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_NONE,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SD1_CD", "GPIO_58", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SD1_WP", "GPIO_59", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
	{
		.func_name = { "SD1_DATA0", "SSI1_TXD", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD1_DATA1", "SSI1_CSN0", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD1_DATA2", "SSI1_CSN1", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD1_DATA3", "", "", "", },
		.reg_type  = 17,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "SD1_CMD_RSP", "SSI1_RXD", "", "", },
		.reg_type  = 20,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = 3,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "CLK_SW0", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = -1,
	},
	{
		.func_name = { "CLK_SW1", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = -1,
	},
	{
		.func_name = { "CLK_SW2", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = -1,
	},
	{
		.func_name = { "CLK_SW3", "", "", "", },
		.reg_type  = 9,
		.func_sel  = 0,
		.pupd 	   = IOMUX_PUPD_UP,
		.drv_cur   = -1,
	},
	{
		.func_name = { "RESERVED", "", "", "", },
		.reg_type  = 20,
		.func_sel  = -1,
	},
	{
		.func_name = { "MAC_TXER", "GPIO37", "", "", },
		.reg_type  = 20,
		.func_sel  = 1,
		.pupd 	   = IOMUX_PUPD_DOWN,
		.drv_cur   = 1,
	},
};


static void fh_iomux_setmfs(Iomux_Object *iomux_obj, Iomux_Pad *pad)
{
	switch (pad->reg_type) {
	case 8:
		(IOMUX_PADTYPE(8)pad->reg)->bit.mfs = pad->func_sel;
		break;
	case 13:
		(IOMUX_PADTYPE(13)pad->reg)->bit.mfs = pad->func_sel;
		break;
	case 20:
		(IOMUX_PADTYPE(20)pad->reg)->bit.mfs = pad->func_sel;
		break;
	case 21:
		(IOMUX_PADTYPE(21)pad->reg)->bit.mfs = pad->func_sel;
		break;
	default:
		break;
	}

}

#ifdef IOMUX_DEBUG

static int fh_iomux_getmfs(Iomux_Object *iomux_obj, Iomux_Pad *pad)
{
	int mfs;

	switch (pad->reg_type) {
	case 8:
		mfs = (IOMUX_PADTYPE(8)pad->reg)->bit.mfs;
		break;
	case 13:
		mfs = (IOMUX_PADTYPE(13)pad->reg)->bit.mfs;
		break;
	case 20:
		mfs = (IOMUX_PADTYPE(20)pad->reg)->bit.mfs;
		break;
	case 21:
		mfs = (IOMUX_PADTYPE(21)pad->reg)->bit.mfs;
		break;
	default:
		mfs = -1;
		break;

	}
	return mfs;
}


static void fh_iomux_print(Iomux_Object *iomux_obj)
{
	int i;
	u32 reg;

	printk("\tPad No.\t\tFunction Select\t\tRegister\n");

	for (i = 0; i < ARRAY_SIZE(fh_iomux_cfg); i++) {
		int curr_func;

		curr_func = fh_iomux_getmfs(iomux_obj, &iomux_obj->pads[i]);
		reg = readl((u32)iomux_obj->pads[i].reg);

		if (curr_func < 0)
			printk("\t%d\t\t%-8s(no mfs)\t0x%08x\n", i, iomux_obj->pads[i].func_name[0],
			       reg);
		else
			printk("\t%d\t\t%-16s\t0x%08x\n", i, iomux_obj->pads[i].func_name[curr_func],
			       reg);

	}

}

#endif

static void fh_iomux_setcur(Iomux_Object *iomux_obj, Iomux_Pad *pad)
{
	switch (pad->reg_type) {
	case 5:
		(IOMUX_PADTYPE(5)pad->reg)->bit.e8_e4 = pad->drv_cur;
		break;
	case 8:
		(IOMUX_PADTYPE(8)pad->reg)->bit.e8_e4 = pad->drv_cur;
		break;
	case 13:
		(IOMUX_PADTYPE(13)pad->reg)->bit.e4_e2 = pad->drv_cur;
		break;
	case 17:
		(IOMUX_PADTYPE(17)pad->reg)->bit.e8_e4 = pad->drv_cur;
		break;
	case 20:
		(IOMUX_PADTYPE(20)pad->reg)->bit.e4_e2 = pad->drv_cur;
		break;
	case 21:
		(IOMUX_PADTYPE(21)pad->reg)->bit.e4_e2 = pad->drv_cur;
		break;
	default:
		break;
	}

}

static void fh_iomux_setpupd(Iomux_Object *iomux_obj, Iomux_Pad *pad)
{
	switch (pad->reg_type) {
	case 9:
		(IOMUX_PADTYPE(9)pad->reg)->bit.pu_pd = pad->pupd;
		break;
	case 17:
		(IOMUX_PADTYPE(17)pad->reg)->bit.pu_pd = pad->pupd;
		break;
	case 20:
		(IOMUX_PADTYPE(20)pad->reg)->bit.pu_pd = pad->pupd;
		break;
	case 21:
		(IOMUX_PADTYPE(21)pad->reg)->bit.pu_pd = pad->pupd;
		break;
	default:
		break;
	}
}

static void fh_iomux_setrest(Iomux_Object *iomux_obj, Iomux_Pad *pad)
{
	switch (pad->reg_type) {
	case 5:
		(IOMUX_PADTYPE(5)pad->reg)->bit.sr = 0;
		break;
	case 8:
		(IOMUX_PADTYPE(8)pad->reg)->bit.sr = 0;
		break;
	case 9:
		(IOMUX_PADTYPE(9)pad->reg)->bit.ie = 1;
		(IOMUX_PADTYPE(9)pad->reg)->bit.smt = 1;
		break;
	case 13:
		(IOMUX_PADTYPE(13)pad->reg)->bit.ie = 1;
		(IOMUX_PADTYPE(13)pad->reg)->bit.smt = 1;
		break;
	case 17:
		(IOMUX_PADTYPE(17)pad->reg)->bit.sr = 0;
		(IOMUX_PADTYPE(17)pad->reg)->bit.ie = 1;
		(IOMUX_PADTYPE(17)pad->reg)->bit.e = 1;
		(IOMUX_PADTYPE(17)pad->reg)->bit.smt = 1;
		break;
	case 20:
		(IOMUX_PADTYPE(20)pad->reg)->bit.sr = 0;
		(IOMUX_PADTYPE(20)pad->reg)->bit.ie = 1;
		(IOMUX_PADTYPE(20)pad->reg)->bit.smt = 1;
		break;
	case 21:
		(IOMUX_PADTYPE(21)pad->reg)->bit.sr = 0;
		(IOMUX_PADTYPE(21)pad->reg)->bit.ie = 1;
		(IOMUX_PADTYPE(21)pad->reg)->bit.smt = 1;
		break;
	default:
		break;
	}

}

void fh_iomux_init(Iomux_Object *iomux_obj)
{
	int i;
	u32 reg = 0;

	iomux_obj->pads = fh_iomux_cfg;

	for (i = 0; i < ARRAY_SIZE(fh_iomux_cfg); i++) {
		iomux_obj->pads[i].reg_offset = i * 4;
		iomux_obj->pads[i].reg = &reg;

#if defined(CONFIG_FH_PWM_NUM) && CONFIG_FH_PWM_NUM == 4
	//for pwm3 only
	if(fh_iomux_cfg[i].func_sel == 2
		&& iomux_obj->pads[i].reg_offset == 0xa8)
	{
		fh_pmu_set_reg(0x128, 0x00101110);
		fh_iomux_cfg[i].func_sel = 1;
	}
#endif

        if(iomux_obj->pads[i].func_sel < 0)
            continue;

		fh_iomux_setmfs(iomux_obj, &fh_iomux_cfg[i]);
		fh_iomux_setcur(iomux_obj, &fh_iomux_cfg[i]);
		fh_iomux_setpupd(iomux_obj, &fh_iomux_cfg[i]);
		fh_iomux_setrest(iomux_obj, &fh_iomux_cfg[i]);
		fh_pmu_set_reg(0x5c + iomux_obj->pads[i].reg_offset, reg);
	}
#ifdef CONFIG_FH_GMAC_RMII
	//(IOMUX_PADTYPE(17)(iomux_obj->pads[18]).reg)->bit.e = 1;
	reg = fh_pmu_get_reg(REG_PMU_PAD_MAC_REF_CLK_CFG);
	reg |= (1 << 13);
	fh_pmu_set_reg(REG_PMU_PAD_MAC_REF_CLK_CFG, reg);
#else
	//(IOMUX_PADTYPE(17)(iomux_obj->pads[18]).reg)->bit.e = 0;
	reg = fh_pmu_get_reg(REG_PMU_PAD_MAC_REF_CLK_CFG);
	reg &= ~(1 << 13);
	fh_pmu_set_reg(REG_PMU_PAD_MAC_REF_CLK_CFG, reg);
#endif
#ifdef IOMUX_DEBUG
	fh_iomux_print(iomux_obj);
#endif
}

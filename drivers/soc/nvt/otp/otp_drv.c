#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "otp_drv.h"
//#include "otp_ioctl.h"
#include "otp_dbg.h"
#include "otp_platform.h"
#include <plat/top.h>
#include <plat/hardware.h>
#include <plat/efuse_protected.h>

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ddr_arb_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_ddr_arb_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_ddr_arb_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_ddr_arb_drv_isr(int irq, void *devid);
irqreturn_t nvt_ddr_arb2_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

static UINT32	ETH_BASE_ADDR = 0;
static UINT32	ARB1_BASE_ADDR = 0;
static UINT32	ARB2_BASE_ADDR = 0;

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
#ifndef EFUSE_DDRP_LDO_TRIM_DATA
#define EFUSE_DDRP_LDO_TRIM_DATA	EFUSE_DDRP_ZQ_TRIM_DATA
#endif
int nvt_otp_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_otp_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_otp_drv_init(MODULE_INFO *pmodule_info)
{
	int err = 0;

//	init_waitqueue_head(&pmodule_info->xxx_wait_queue);
//	spin_lock_init(&pmodule_info->xxx_spinlock);
//	sema_init(&pmodule_info->xxx_sem, 1);
//	init_completion(&pmodule_info->xxx_completion);
//	tasklet_init(&pmodule_info->xxx_tasklet, nvt_ddr_arb_drv_do_tasklet, (unsigned long)pmodule_info);

	/* allocate graphic resource here */
    otp_platform_create_resource(pmodule_info);

	//printk("%s: resource done\r\n", __func__);


	/* initial clock here */

	/* Add HW Module initialization here when driver loaded */



	/* Add error handler here */

	return err;
}

int nvt_otp_drv_remove(MODULE_INFO *pmodule_info)
{


	/* Add HW Moduel release operation here*/

	/* release OS resources */
    otp_platform_release_resource();


	return 0;
}

int nvt_otp_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_otp_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_otp_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg)
{
	int err = 0;
#if 0
	int __user *argp = (int __user *)arg;

	//nvt_dbg(IND, "IF-%d cmd:%x\n =>%08X", if_id, cmd,DDR_ARB_IOC_CHKSUM);

	switch (cmd) {
	case DDR_ARB_IOC_CHKSUM: {
			ARB_CHKSUM param = {0};
			if (unlikely(copy_from_user(&param, argp, sizeof(param)))) {
				DBG_ERR("failed to copy_from_user\r\n");
				return -EFAULT;
			}
			if (param.version != DDR_ARB_IOC_VERSION) {
				DBG_ERR("version not matched kernel(%08X), user(%08X)\r\n", DDR_ARB_IOC_VERSION, param.version);
				return -EFAULT;
			}
			param.sum = arb_chksum(param.ddr_id, param.phy_addr, param.len);
			if (unlikely(copy_to_user(argp, &param, sizeof(param)))) {
                                DBG_ERR("failed to copy_to_user\r\n");
                                return -EFAULT;
                        }
		} break;
	}
#endif
	return err;
}
#define REG_PHY_PAGE_SEL	0xFE

#define ZQK_OFFSET			0x110

#define TRIM_CLK_LDO_0_95_V		0xA8
#define TRIM_CLK_LDO_0_90_V		0xFC

#define TRIM_IOPAD_LDO_0_95_V	0x0A
#define TRIM_IOPAD_LDO_0_90_V	0x0F

#define LDO_STEP_CNT			16
#define RZQ_STEP_CNT			32
//ETH trim related
#define TX_TRIM_ROUT_CNT		8
#define TX_TRIM_DAC_CNT			32

#define TX_TRIM_RX_TX_SEC_CNT	8

const static CHAR   *TX_TRIM_ROUT_DEFINE[TX_TRIM_ROUT_CNT] = {
	"       100ohm",
	" 106ohm (+6%)",
	"113ohm (+13%)",
	"124ohm (+24%)",
	"  95ohm (-5%)",
	"  92ohm (-8%)",
	" 89ohm (-11%)",
	" 86ohm (-14%)",
};

const static BOOL   TX_TRIM_ROUT_AVAILABLE[TX_TRIM_ROUT_CNT] = {
	1,
	1,
	1,
	1,

	1,
	1,
	1,
	1,
};


const static CHAR   *TX_TRIM_DAC_DEFINE[TX_TRIM_DAC_CNT] = {
	" 50u (+0%)",
	" 51u (+2%)",
	" 52u (+4%)",
	" 53u (+6%)",
	" 54u (+8%)",
	"55u (+10%)",
	"56u (+12%)",
	"57u (+14%)",
	"58u (+16%)",

	"59u (+18%)",
	"60u (+20%)",
	"61u (+22%)",
	"62u (+24%)",
	"63u (+26%)",
	"64u (+28%)",
	"65u (+30%)",
	"34u (-32%)",
	"35u (-30%)",
	"36u (-28%)",
	"37u (-26%)",
	"38u (-24%)",
	"39u (-22%)",
	"40u (-20%)",
	"41u (-18%)",

	"42u (-16%)",
	"43u (-14%)",
	"44u (-12%)",
	"45u (-10%)",
	" 46u (-8%)",
	" 47u (-6%)",
	" 48u (-4%)",
	" 49u (-2%)",
};


const static BOOL   TX_TRIM_DAC_AVAILABLE[TX_TRIM_DAC_CNT] = {
	1,
	1,
	1,
	1,

	1,
	1,
	1,
	1,

	1,
	1,
	1,
	1,

	1,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	1,
	1,
	1,
	1,

	1,
	1,
	1,
	1,

	1,
	1,
	1,
	1,
};
//Reverse Notice
const static CHAR   *TX_TRIM_RX_TX_SE_DEFINE[TX_TRIM_RX_TX_SEC_CNT] = {
	"+20%",
	"+13%",
	" +6%",
	" +0%",
	" -5%",
	"-10%",
	"-15%",
	"-20%",
};

const static BOOL   TX_TRIM_RX_TX_SEL_AVAILABLE[TX_TRIM_RX_TX_SEC_CNT] = {
	0,
	1,
	1,
	1,

	1,
	1,
	1,
	1,
};
STATIC_ASSERT(sizeof(TX_TRIM_RX_TX_SE_DEFINE) / sizeof(UINT32) == (TX_TRIM_RX_TX_SEC_CNT));
STATIC_ASSERT(sizeof(TX_TRIM_RX_TX_SEL_AVAILABLE) / sizeof(UINT32) == (TX_TRIM_RX_TX_SEC_CNT));
STATIC_ASSERT(sizeof(TX_TRIM_ROUT_DEFINE) / sizeof(UINT32) == (TX_TRIM_ROUT_CNT));
STATIC_ASSERT(sizeof(TX_TRIM_ROUT_AVAILABLE) / sizeof(UINT32) == (TX_TRIM_ROUT_CNT));
STATIC_ASSERT(sizeof(TX_TRIM_DAC_DEFINE) / sizeof(UINT32) == (TX_TRIM_DAC_CNT));
STATIC_ASSERT(sizeof(TX_TRIM_DAC_AVAILABLE) / sizeof(UINT32) == (TX_TRIM_DAC_CNT));


const static CHAR   *RZQ_STEP_NAME[RZQ_STEP_CNT] = {
	"null",
	"-15",
	"-14",
	"-13",
	"-12",
	"-11",
	"-10",
	" -9",
	" -8",
	" -7",
	" -6",
	" -5",
	" -4",
	" -3",
	" -2",
	" -1",
	"default",
	" +1",
	" +2",
	" +3",
	" +4",
	" +5",
	" +6",
	" +7",
	" +8",
	" +9",
	"+10",
	"+11",
	"+12",
	"+13",
	"+14",
	"+15",
};

const static UINT32   RZQ_STEP_EFUSE_VALUE[RZQ_STEP_CNT] = {
	0x20,
	0x1F,
	0x1E,
	0x1D,
	0x1C,
	0x1B,
	0x1A,
	0x19,
	0x18,
	0x17,
	0x16,
	0x15,
	0x14,
	0x13,
	0x12,
	0x11,
	0x00,
	0x01,
	0x02,
	0x03,
	0x04,
	0x05,
	0x06,
	0x07,
	0x08,
	0x09,
	0x0A,
	0x0B,
	0x0C,
	0x0D,
	0x0E,
	0x0F,
};
STATIC_ASSERT(sizeof(RZQ_STEP_NAME) / sizeof(UINT32) == (RZQ_STEP_CNT));
STATIC_ASSERT(sizeof(RZQ_STEP_EFUSE_VALUE) / sizeof(UINT32) == (RZQ_STEP_CNT));
#define INREG32(x)                      (*((volatile UINT32*)(x)))
#define OUTREG32(x, y)                  (*((volatile UINT32*)(x)) = (y))

static UINT32 io_read32(uintptr_t addr)
{
	const volatile uintptr_t *ptr = (volatile uintptr_t *) addr;
	const volatile UINT32 *ptr32 = (volatile UINT32 *)ptr;

	return *ptr32;
}

static void io_write32(uintptr_t addr, UINT32 val)
{
	volatile uintptr_t *ptr = (volatile uintptr_t *) addr;
	volatile UINT32 *ptr32 = (volatile UINT32 *)ptr;

	*ptr32 = val;
}

static uintptr_t phy_ahb_addr(int phy_id, UINT8 real_offset)
{
	//const uintptr_t phy_base[] = {PHY_BASE_0, PHY_BASE_1};
	if(phy_id == 0) {
		return (ARB1_BASE_ADDR + ((uintptr_t)real_offset << 2));
	} else {
		return (ARB2_BASE_ADDR + ((uintptr_t)real_offset << 2));
	}
}

static void phy_change_page(int phy_id, UINT8 target_page)
{
//	static u8 current_page[PHY_NUMBER] = { -1 };
//	static u8 current_page[PHY_NUMBER] = { -1, -1};
	/* change page only if current page is different then target page */
	//if (current_page[phy_id] != target_page) {
	io_write32(phy_ahb_addr(phy_id, REG_PHY_PAGE_SEL), target_page);
		//current_page[phy_id] = target_page;
	//}
}

static UINT8 phy_page(UINT16 offset)
{
	return (offset >> 8) & 0xF;
}

static UINT8 phy_real_offset(UINT16 offset)
{
	return (UINT8)offset;
}

static UINT8 _phy_read(int phy_id, UINT8 page, UINT8 real_offset)
{
	uintptr_t ahb_addr = phy_ahb_addr(phy_id, real_offset);
	phy_change_page(phy_id, page);

	return io_read32(ahb_addr);
}

static UINT32 bitCount(UINT32 data)
{
    UINT32 count;

    count = 0;
    while(data)
    {
        data &= (data - 1);
        count++;
    }

    return count;
}


static void emu_disp_bit_mask(UINT32 value, UINT32 mask)
{
	UINT32 	uiData = value & mask;
	UINT32 	uiBits;
	UINT32	bitCnt = bitCount(mask);

	for (uiBits = 0; uiBits < bitCnt; uiBits++) {
		if (uiData & (1 << uiBits)) {
			pr_cont("1");
		} else {
			pr_cont("0");
		}
	}
}

static UINT8 phy_mask_get(int phy_id, UINT16 offset, UINT8 mask)
{
	UINT8 val;
	UINT8 page = phy_page(offset);
	UINT8 real_offset = phy_real_offset(offset);

//	ddr_fatal(!phy_id_valid(phy_id), "phy_mask_get error: phy_id=%d\n",
//		  phy_id);

	val = _phy_read(phy_id, page, real_offset) & mask;

//	phy_io_dbg("phy_mask_get %d [0x%03x]=0x%02x\n", phy_id, offset, val);

	return val;
}


//rx_tx_sel 0 -> RX, 1 -> TX
static void emu_disp_RX_TX_SEL_trim(UINT32 rx_tx_sel, UINT32 table_cnt, UINT32 trim, UINT32 reg)
{
	UINT32	index;
	UINT32	real_trim;
	if(rx_tx_sel == 0) {
		pr_info("RX_TRIM_SEL_RX reg       [mention] trim\n");
	} else {
		pr_info("RX_TRIM_SEL_TX reg       [mention] trim\n");
	}
	pr_info("=============================================\n");
	for(index = 0; index < table_cnt; index ++) {
		real_trim = table_cnt - index - 1;
		if(reg == real_trim) {
			pr_cont("---->");
		} else {
			pr_cont("     ");
		}
		emu_disp_bit_mask(index, 0x7);
		pr_cont("                   ");
		pr_cont("%s", TX_TRIM_RX_TX_SE_DEFINE[index]);
		if(TX_TRIM_RX_TX_SEL_AVAILABLE[index])
			pr_cont("---------[O]");
		else
			pr_cont("---------[X]");

		if(trim == real_trim) {
			pr_cont("<=====");
		} else {
			pr_cont("      ");
		}
		pr_cont("\r\n");
	}
	pr_cont("=============================================\r\n");
}

//TX trim ROUT
static void emu_disp_TX_trim_DAC(UINT32 table_cnt, UINT32 trim, UINT32 reg)
{
	UINT32	index;
	pr_info("TX_trim_DAC reg       [mention] trim\n");

	pr_info("=============================================\n");
	for(index = 0; index < table_cnt; index ++) {

		if(reg == index) {
			pr_cont("---->");
		} else {
			pr_cont("     ");
		}
		emu_disp_bit_mask(index, 0x1F);
		pr_cont("                 ");
		pr_cont("%s", TX_TRIM_DAC_DEFINE[index]);
		if(TX_TRIM_DAC_AVAILABLE[index])
			pr_cont("---[O]");
		else
			pr_cont("---[X]");

		if(trim == index) {
			pr_cont("<=====");
		} else {
			pr_cont("      ");
		}

		pr_cont("\r\n");
	}
	pr_cont("=============================================\r\n");
}



//TX trim ROUT
static void emu_disp_TX_trim_ROUT(UINT32 table_cnt, UINT32 trim, UINT32 reg)
{
	UINT32	index;
	pr_info("=============================================\n");
	pr_info("TX_trim_ROUT reg       [mention] trim\r\n");
	pr_info("=============================================\n");
	for(index = 0; index < table_cnt; index ++) {
		if(reg == index) {
			pr_cont("---->");
		} else {
			pr_cont("     ");
		}
		emu_disp_bit_mask(index, 0x7);
		pr_cont("                 ");
		pr_cont("%s", TX_TRIM_ROUT_DEFINE[index]);
		if(TX_TRIM_ROUT_AVAILABLE[index])
			pr_cont("---[O]");
		else
			pr_cont("---[X]");

		if(trim == index) {
			pr_cont("<=====");
		} else {
			pr_cont("      ");
		}

		pr_cont("\r\n");
	}
	pr_cont("=============================================\r\n");
}

static void emu_disp_zqk_trim_table(UINT32 phyID, UINT32 DDR_value, UINT32 trim, UINT32 table_cnt)
{
//	UINT32 	uiBits;
	UINT32	index;
	pr_cont("[DDR[%d] register]  [rzq efuse trim]    [Mention]\r\n", phyID);
	pr_cont("==================================================\r\n");
	for(index = 1; index < table_cnt; index ++) {
		pr_cont("       ");
		emu_disp_bit_mask(index, 0x1F);
		if(DDR_value == index)
			pr_cont("<=====      ");
		else
			pr_cont("            ");
		emu_disp_bit_mask(RZQ_STEP_EFUSE_VALUE[index], 0x1F);
		if(trim == RZQ_STEP_EFUSE_VALUE[index])
			pr_cont("<=====      ");
		else
			pr_cont("            ");
		pr_cont("%s", RZQ_STEP_NAME[index]);

		pr_cont("\r\n");
	}
	pr_cont("==================================================\r\n");

}

static void emu_disp_ldo_trim(UINT32 phyID, UINT32 table_cnt, UINT32 trim)
{
//	UINT32 	uiBits;
	UINT32	index;
	pr_cont("[DDR[%d] LDO PLL  trim]    [LDO PLL]\r\n", phyID);
	pr_cont("=======================================\r\n");
	for(index = 0; index < table_cnt; index ++) {
		if(index < 4) {
			pr_cont("            %02x", index);
			pr_cont("             ");
			pr_cont("1.00V");
		} else if(index >=4 && index <= 9) {
			pr_cont("            %02x", index);
			pr_cont("             ");
			pr_cont("0.95V");
		} else {
			pr_cont("            %02x", index);
			pr_cont("             ");
			pr_cont("0.90V");
		}
		if(trim == index)
		{
			pr_cont("<======");
		}
		pr_cont("\r\n");
	}
	pr_cont("=======================================\r\n");
}

static void emu_disp_ldo_clk_iopd_trim(UINT32 phyID, UINT32 table_cnt, UINT32 trim)
{
	UINT32	index;
	pr_cont("[DDR[%d]LDO CLK(IO)trim]    [LDO PLL]\r\n", phyID);
	pr_cont("=======================================\r\n");
	for(index = 0; index < table_cnt; index ++) {
		if(index < 4) {
			pr_cont("            %02x", index);
			pr_cont("             ");
			pr_cont("0.95V");
		} else {
			pr_cont("            %02x", index);
			pr_cont("             ");
			pr_cont("0.90V");
		}
		if(trim == index)
		{
			pr_cont("<======");
		}
		pr_cont("\r\n");
	}
	pr_cont("=======================================\r\n");
}

void trim_drvdump(void) {
	INT32	trim;
	UINT16  value;
	UINT32 	TX_TRIM_ROUT, TX_TRIM_DAC, TX_TRIM_SEL_RX, TX_TRIM_SEL_TX;
    UINT32  REG_TX_TRIM_ROUT, REG_TX_TRIM_DAC, REG_TX_TRIM_SEL_RX, REG_TX_TRIM_SEL_TX;
	UINT32 	zqk;
	UINT8   sum;
	UINT8	page;
	UINT8	real_offset;

	UINT32	DDR2_EN;
 	UINT32	LDO_TRIM_VALUE;

 	UINT8	H_LDO, V_LDO;
	UINT8	PLL_LDO;
	UINT8	CLK_LDO;
	UINT8	IOPAD_LDO;

	ETH_BASE_ADDR = (UINT32)ioremap_nocache(NVT_ETH_BASE_PHYS, 0x4000);
	ARB1_BASE_ADDR= (UINT32)ioremap_nocache(NVT_DDR1_PHY_BASE_PHYS, 0x800);
	ARB2_BASE_ADDR= (UINT32)ioremap_nocache(NVT_DDR2_PHY_BASE_PHYS, 0x800);
	if( nvt_get_chip_id() == CHIP_NA51084) {
		trim = efuse_readParamOps(EFUSE_ETHERNET_TRIM_DATA, &value);
		pr_info("####### Ethernet ######\n");
		if(trim == E_OK) {
			pr_info("  0x3B74 = [0x%08x]\n", (int)INREG32(ETH_BASE_ADDR + 0x3B74));
            pr_info("  0x3B78 = [0x%08x]\n", (int)INREG32(ETH_BASE_ADDR + 0x3B78));
            pr_info("  0x3B68 = [0x%08x]\n", (int)INREG32(ETH_BASE_ADDR + 0x3B68));

			TX_TRIM_ROUT    = value & 0x7;            	// bit[2..0] all available
	        TX_TRIM_DAC     = ((value >> 3) & 0x1F);    // bit[7..3]
	        TX_TRIM_SEL_RX  = ((value >> 8) & 0x7);     // bit[10..8]
	        TX_TRIM_SEL_TX  = ((value >>11) & 0x7);     // bit[13..11]

	        //0xF02B3B74 bit[5..3]
			REG_TX_TRIM_ROUT	= (INREG32(ETH_BASE_ADDR + 0x3B74) >> 3) & 0x7;
			//0xF02B3B78 bit[4..0]
			REG_TX_TRIM_DAC		= (INREG32(ETH_BASE_ADDR + 0x3B78) >> 0) & 0x1F;
			//0xF02B3B68 bit[7..5]
			REG_TX_TRIM_SEL_RX	= (INREG32(ETH_BASE_ADDR + 0x3B68) >> 5) & 0x7;
			//0xF02B3B68 bit[4..2]
			REG_TX_TRIM_SEL_TX	= (INREG32(ETH_BASE_ADDR + 0x3B68) >> 2) & 0x7;

			emu_disp_TX_trim_ROUT(TX_TRIM_ROUT_CNT, TX_TRIM_ROUT, REG_TX_TRIM_ROUT);
			emu_disp_TX_trim_DAC(TX_TRIM_DAC_CNT, TX_TRIM_DAC, REG_TX_TRIM_DAC);
			emu_disp_RX_TX_SEL_trim(0, TX_TRIM_RX_TX_SEC_CNT, TX_TRIM_SEL_RX, REG_TX_TRIM_SEL_RX);
			emu_disp_RX_TX_SEL_trim(1, TX_TRIM_RX_TX_SEC_CNT, TX_TRIM_SEL_TX, REG_TX_TRIM_SEL_TX);


			if((TX_TRIM_ROUT == REG_TX_TRIM_ROUT) && (TX_TRIM_DAC == REG_TX_TRIM_DAC) && (TX_TRIM_SEL_RX == REG_TX_TRIM_SEL_RX) && (TX_TRIM_SEL_TX == REG_TX_TRIM_SEL_TX)) {
				pr_info("  TX_TRIM_ROUT[0x%02x]=REG[0x%02x]\n", (int)TX_TRIM_ROUT, (int)REG_TX_TRIM_ROUT);
				pr_info("   TX_TRIM_DAC[0x%02x]=REG[0x%02x]\n", (int)TX_TRIM_DAC, (int)REG_TX_TRIM_DAC);
				pr_info("TX_TRIM_SEL_RX[0x%02x]=REG[0x%02x]\n", (int)TX_TRIM_SEL_RX, (int)REG_TX_TRIM_SEL_RX);
				pr_info("TX_TRIM_SEL_TX[0x%02x]=REG[0x%02x]\n", (int)TX_TRIM_SEL_TX, (int)REG_TX_TRIM_SEL_TX);
				pr_info("  *Eth Trim data range success 0x%08x\n", (int)value);
			} else {
				pr_info("  TX_TRIM_ROUT[0x%02x]!=REG[0x%02x]\n", (int)TX_TRIM_ROUT, (int)REG_TX_TRIM_ROUT);
				pr_info("   TX_TRIM_DAC[0x%02x]!=REG[0x%02x]\n", (int)TX_TRIM_DAC, (int)REG_TX_TRIM_DAC);
				pr_info("TX_TRIM_SEL_RX[0x%02x]!=REG[0x%02x]\n", (int)TX_TRIM_SEL_RX, (int)REG_TX_TRIM_SEL_RX);
				pr_info("TX_TRIM_SEL_TX[0x%02x]!=REG[0x%02x]\n", (int)TX_TRIM_SEL_TX, (int)REG_TX_TRIM_SEL_TX);
			}

			pr_info("####### ZQK ######\n");

			trim = efuse_readParamOps(EFUSE_DDRP_H_TRIM_DATA, &value);
            if(trim == EFUSE_SUCCESS)
            {
				page = phy_page(ZQK_OFFSET);
				real_offset = phy_real_offset(ZQK_OFFSET);
				zqk = _phy_read(0, page, real_offset);
				emu_disp_zqk_trim_table(0, zqk, value, RZQ_STEP_CNT);
				pr_info("zqk trim[0] = [0x%08x]\r\n", (int)value);
				pr_info(" zqk reg[0] = [0x%08x]\r\n", (int)zqk);
				if(zqk & 0x10) {
					if((zqk - value) != 0x10) {
						pr_info(" !!!!rzq reg read back check error[1]!!!!\n");
					} else {
						pr_info("	rzq reg read back check success[1]\n");
					}
				} else {
					sum = (value + zqk);
					if(sum != 0x20) {
						pr_info(" !!!!rzq reg read back check error[0] sum = 0x%02x!!!!\r\n", (int)sum);
					} else {
						pr_info("	rzq reg read back check success[0]\r\n");
					}
				}
            }
            else
            {
                pr_info(" !!!!RZQ DDR1 trim read error [%d]!!!!\r\n", trim);
            }
			DDR2_EN = *(volatile UINT32 *)(ARB2_BASE_ADDR - 0x1000 + 4);	//bit[5] = 1?
			if((DDR2_EN & (1<<5)) == (1<<5)) {
	            trim = efuse_readParamOps(EFUSE_DDRP_V_TRIM_DATA, &value);
	            if(trim == EFUSE_SUCCESS)
	            {
					page = phy_page(ZQK_OFFSET);
					real_offset = phy_real_offset(ZQK_OFFSET);
					zqk = _phy_read(1, page, real_offset);
					emu_disp_zqk_trim_table(1, zqk, value, RZQ_STEP_CNT);
					pr_info("zqk trim[1] = [0x%08x]\r\n", (int)value);
					pr_info(" zqk reg[1] = [0x%08x]\r\n", (int)zqk);
					if(zqk & 0x10) {
						if((zqk - value) != 0x10) {
							pr_info("!!!!rzq reg read back check error[1]!!!!\n");
						} else {
							pr_info("	rzq reg read back check success[1]\n");
						}
					} else {
						sum = (value + zqk);
						if(sum != 0x20) {
							pr_info("!!!!rzq reg read back check error[0] sum = 0x%02x!!!!\n", (int)sum);
						} else {
							pr_info("	rzq reg read back check success[0]\n");
						}
					}
	            }
	            else
	            {
	                pr_info("!!!!RZQ DDR2 trim read error [%d]!!!!\n", trim);
	            }
            }else {
				pr_info("No DDR2 init\n");
            }

		}
		pr_info("####### LDO ######\n");
		trim = efuse_readParamOps(EFUSE_DDRP_LDO_TRIM_DATA, &value);
        if(trim == EFUSE_SUCCESS)
        {
            pr_info("LDO Trim data read success\r\n");

            LDO_TRIM_VALUE = (value & 0xFF);
            H_LDO = (LDO_TRIM_VALUE & 0xF);
            V_LDO = ((LDO_TRIM_VALUE & 0xF0) >> 4);

            DDR2_EN = *(volatile UINT32 *)(ARB2_BASE_ADDR - 0x1000 + 4);	//bit[5] = 1?

            pr_info("    LDO Trim data range success LDO [%d] 0~31 H_LDO[%d], V_LDO[%d]\r\n", (int)LDO_TRIM_VALUE, (int)H_LDO, (int)V_LDO);

            PLL_LDO = phy_mask_get(0, 0x0BF, 0x30); //get bit[5..4]
            CLK_LDO = phy_mask_get(0, 0x0D8, 0xfc); //get bit[7..2]
			IOPAD_LDO = phy_mask_get(0, 0x0D9, 0x0f); //get bit[3..0]

            pr_info("    PLL_LDO register[0] = 0x%02x\r\n", (int)PLL_LDO);
			if(PLL_LDO == 0x00) {
				pr_info("    PLL_LDO = 1.05V => Error\r\n");
			} else if(PLL_LDO == 0x10) {
				pr_info("    PLL_LDO = 1.00V\r\n");
				if(H_LDO > 4) {
					pr_info("    PLL_LDO = 1.00V but trim > 4\r\n");
				}
			} else if(PLL_LDO == 0x20) {
				pr_info("    PLL_LDO = 0.95V\r\n");
				if(H_LDO < 4 || H_LDO > 9) {
					pr_info("    PLL_LDO = 0.95V but trim < 4 or > 9\r\n");
				}
			} else if(PLL_LDO == 0x30) {
				pr_info("    PLL_LDO = 0.90V\r\n");
				if(H_LDO < 0xa) {
					pr_info("    PLL_LDO = 0.90V but H_LDO < 0xa\r\n");
				}
			}
			pr_info("    CLK_LDO register[0] = 0x%02x\r\n", (int)CLK_LDO);

			if(CLK_LDO == TRIM_CLK_LDO_0_95_V) {
				pr_info("    CLK_LDO = 0.95V\r\n");
				if(H_LDO >= 4) {
					pr_info("    CLK_LDO = 0.95V but trim >= 4\r\n");
				}
			} else if(CLK_LDO == TRIM_CLK_LDO_0_90_V) {
				pr_info("    CLK_LDO = 0.90V\r\n");
				if(H_LDO < 4) {
					pr_info("    CLK_LDO = 0.90V but H_LDO < 4\r\n");
				}
			} else {
				pr_info("    UNKNOW CLK_LDO\r\n");
			}

			pr_info("  IOPAD_LDO register[0] = 0x%02x\r\n", (int)IOPAD_LDO);
			if(IOPAD_LDO == TRIM_IOPAD_LDO_0_95_V) {
				pr_info("  IOPAD_LDO = 0.95V\r\n");
				if(H_LDO >= 4) {
					pr_info("  IOPAD_LDO = 0.95V but trim >= 4\r\n");
				}
			} else if(IOPAD_LDO == TRIM_IOPAD_LDO_0_90_V) {
				pr_info("  IOPAD_LDO = 0.90V\r\n");
				if(H_LDO < 4) {
					pr_info("  IOPAD_LDO = 0.90V but H_LDO < 4\r\n");
				}
			} else {
				pr_info("    UNKNOW IOPAD_LDO\r\n");
			}

			emu_disp_ldo_trim(0, LDO_STEP_CNT, H_LDO);
			emu_disp_ldo_clk_iopd_trim(0, LDO_STEP_CNT, H_LDO);

			if((DDR2_EN & (1<<5)) == (1<<5)) {
				pr_info("DDR2 EN [0x%08x]\r\n", (int)DDR2_EN);
				PLL_LDO = phy_mask_get(1, 0x0BF, 0x30); //get bit[5..4]
				CLK_LDO = phy_mask_get(1, 0x0D8, 0xfc); //get bit[7..2]
				IOPAD_LDO = phy_mask_get(1, 0x0D9, 0x0f); //get bit[3..0]

                pr_info("    PLL_LDO register[1] = 0x%02x\r\n", (int)PLL_LDO);
				if(PLL_LDO == 0x00) {
					pr_info("    PLL_LDO = 1.05V => Error\r\n");
				} else if(PLL_LDO == 0x10) {
					pr_info("    PLL_LDO = 1.00V\r\n");
					if(V_LDO > 4) {
						pr_info("    PLL_LDO = 1.00V but trim > 4\r\n");
					}
				} else if(PLL_LDO == 0x20) {
					pr_info("    PLL_LDO = 0.95V\r\n");
					if(V_LDO < 4 || V_LDO > 9) {
						pr_info("    PLL_LDO = 0.95V but trim < 4 or > 9\r\n");
					}
				} else if(PLL_LDO == 0x30) {
					pr_info("    PLL_LDO = 0.90V\r\n");
					if(V_LDO < 0xa) {
						pr_info("    PLL_LDO = 0.90V but H_LDO < 0xa\r\n");
					}
				}

				pr_info("    CLK_LDO register[1] = 0x%02x\r\n", (int)CLK_LDO);
				if(CLK_LDO == TRIM_CLK_LDO_0_95_V) {
					pr_info("    CLK_LDO = 0.95V\r\n");
					if(V_LDO >= 4) {
						pr_info("    CLK_LDO = 0.95V but trim >= 4\r\n");
					}
				} else if(CLK_LDO == TRIM_CLK_LDO_0_90_V) {
					pr_info("    CLK_LDO = 0.90V\r\n");
					if(V_LDO < 4) {
						pr_info("    CLK_LDO = 0.90V but H_LDO < 4\r\n");
					}
				} else {
					pr_info("    UNKNOW CLK_LDO\r\n");
				}

				pr_info("  IOPAD_LDO register[1] = 0x%02x\r\n", (int)IOPAD_LDO);
				if(IOPAD_LDO == TRIM_IOPAD_LDO_0_95_V) {
					pr_info("  IOPAD_LDO = 0.95V\r\n");
					if(V_LDO >= 4) {
						pr_info("  IOPAD_LDO = 0.95V but trim >= 4\r\n");
					}
				} else if(IOPAD_LDO == TRIM_IOPAD_LDO_0_90_V) {
					pr_info("  IOPAD_LDO = 0.90V\r\n");
					if(V_LDO < 4) {
						pr_info("    IOPAD_LDO = 0.90V but H_LDO < 4\r\n");
					}
				} else {
					pr_info("    UNKNOW IOPAD_LDO\r\n");
				}
				emu_disp_ldo_trim(1, LDO_STEP_CNT, V_LDO);
				emu_disp_ldo_clk_iopd_trim(1, LDO_STEP_CNT, V_LDO);
			}
        }
	}

	iounmap((volatile void __iomem *)ETH_BASE_ADDR);
	iounmap((volatile void __iomem *)ARB1_BASE_ADDR);
	iounmap((volatile void __iomem *)ARB2_BASE_ADDR);
}

EXPORT_SYMBOL(trim_drvdump);

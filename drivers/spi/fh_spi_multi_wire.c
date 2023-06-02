/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include "fh_spi_common.h"

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/

/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type
 *  that only be used in this file  here
 ***************************************************************************/

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/


/*****************************************************************************

 *  static fun;
 *****************************************************************************/
static void reg_bit_process(u32 *data, u32 value, u32 mask);
static int Spi_SetApbReadWireMode(struct fh_spi *dw, spi_read_wire_mode_e mode);
static int Spi_SetXip(struct fh_spi *dw, spi_xip_config_e value);
static int Spi_SetDPI(struct fh_spi *dw, spi_dpi_config_e value);
static int Spi_SetSwap(struct fh_spi *dw, unsigned int value);
static int Spi_SetWidth(struct fh_spi *dw, unsigned int value);
static int Spi_SetQPI(struct fh_spi *dw, spi_qpi_config_e value);
static int Spi_TimingConfigure(struct fh_spi *dw, u32 value);
static void fh_spic_check_idle(struct fh_spi *dw);
static void spic_bus_change_1_wire(struct spi_master *p_master);
static void spic_bus_change_2_wire(struct spi_master *p_master, u32 dir);
static void spic_bus_change_4_wire(struct spi_master *p_master, u32 dir);
static void spic_swap_data_width(struct fh_spi_controller *fh_spi, u32 reg_width);
static void spic_fix_wire_mode(struct fh_spi_controller *fh_spi);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

/*add spic new code here below..*/
static void reg_bit_process(u32 *data, u32 value, u32 mask)
{
	(*data) &= ~mask;
	(*data) |= value;
}

static int Spi_SetApbReadWireMode(struct fh_spi *dw, spi_read_wire_mode_e mode)
{
	u32 data = dw_readl(dw, ccfgr);

	switch (mode){
	case STANDARD_READ:
		reg_bit_process(&data, 0 << 8, 7 << 8);
		reg_bit_process(&data, 0 << 2, 3 << 2);
		reg_bit_process(&data, 0 << 0, 3 << 0);
		reg_bit_process(&data, 0 << 4, 7 << 4);
		break;

	case DUAL_OUTPUT:
		reg_bit_process(&data, 0 << 8, 7 << 8);
		reg_bit_process(&data, 3 << 2, 3 << 2);
		reg_bit_process(&data, 1 << 0, 3 << 0);
		reg_bit_process(&data, 0 << 4, 7 << 4);
		break;

	case QUAD_OUTPUT:
		reg_bit_process(&data, 0 << 8, 7 << 8);
		reg_bit_process(&data, 3 << 2, 3 << 2);
		reg_bit_process(&data, 2 << 0, 3 << 0);
		reg_bit_process(&data, 0 << 4, 7 << 4);
		break;
	default:
		printk("wrong mode now....\n");
	}
	data |= 1<<13;
	dw_writel(dw, ccfgr, data);
	return CONFIG_OK;
}

static int Spi_SetXip(struct fh_spi *dw, spi_xip_config_e value)
{
	u32 data = dw_readl(dw, ccfgr);
	u32 data1 = dw_readl(dw, opcr);

	if (value == XIP_ENABLE) {
		reg_bit_process(&data, XIP_ENABLE << 11, 1 << 11);
		reg_bit_process(&data1, 0x20 << 20, 0xff << 20);
	} else if (value == XIP_DISABLE) {
		reg_bit_process(&data, XIP_DISABLE << 11, 1 << 11);
		reg_bit_process(&data1, 0xff << 20, 0xff << 20);
	}
	dw_writel(dw, ccfgr, data);
	dw_writel(dw, opcr, data1);

	return 0;
}

static int Spi_SetDPI(struct fh_spi *dw, spi_dpi_config_e value)
{
	u32 data = dw_readl(dw, opcr);

	reg_bit_process(&data, value << 16, 1 << 16);
	dw_writel(dw, opcr, data);
	return 0;
}

static int Spi_SetSwap(struct fh_spi *dw, unsigned int value)
{
	u32 data;

	Spi_Enable(dw, SPI_DISABLE);
	data = dw_readl(dw, ccfgr);
	data &= ~(1<<12);
	data |= (value<<12);
	dw_writel(dw, ccfgr, data);
	Spi_Enable(dw, SPI_ENABLE);
	return 0;
}

static int Spi_SetWidth(struct fh_spi *dw, unsigned int value)
{
	u32 data;
	u32 ret;

	Spi_Enable(dw, SPI_DISABLE);
	data = dw_readl(dw, ctrl0);
	ret = dw_readl(dw, ccfgr);
	if (value == 32) {
		ret |= 1<<16;
		data &= ~(0x1f << 16);
		data |= ((value - 1) << 16);
	} else {
		ret &= ~(1<<16);
		data &= ~(0x0f << 0);
		data |= ((value - 1) << 0);
	}

	dw_writel(dw, ctrl0, data);
	dw_writel(dw, ccfgr, ret);
	Spi_Enable(dw, SPI_ENABLE);
	return 0;
}

static int Spi_SetQPI(struct fh_spi *dw, spi_qpi_config_e value)
{
	u32 data = dw_readl(dw, opcr);

	reg_bit_process(&data, value << 17, 1 << 17);
	dw_writel(dw, opcr, data);

	return 0;
}

static int Spi_TimingConfigure(struct fh_spi *dw, u32 value)
{
	dw_writel(dw, timcr, value);
	return 0;
}

static void fh_spic_check_idle(struct fh_spi *dw)
{
	u32 status;

	status = Spi_ReadStatus(dw);
	/*ahb rx fifo not empty..*/
	BUG_ON((status & 1<<10) != 0);
	/*ahb tx fifo empty..*/
	BUG_ON((status & 1<<9) != 1<<9);
	/*apb rx fifo*/
	BUG_ON((status & 1<<3) != 0);
	/*apb tx fifo*/
	BUG_ON((status & 1<<2) != 1<<2);
	/*shift not busy..*/
	BUG_ON((status & 1) != 0);
}


static void spic_bus_change_1_wire(struct spi_master *p_master)
{
	struct fh_spi_controller *fh_spi;

	fh_spi = spi_master_get_devdata(p_master);
	fh_spi->dwc.multi_para.active_wire_width = ONE_WIRE_SUPPORT;
	fh_spi->dwc.multi_para.dir = SPI_DATA_DIR_DUOLEX;
	fh_spic_check_idle(&fh_spi->dwc);
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetApbReadWireMode(&fh_spi->dwc,STANDARD_READ);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
}

static void spic_bus_change_2_wire(struct spi_master *p_master, u32 dir)
{
	struct fh_spi_controller *fh_spi;

	fh_spi = spi_master_get_devdata(p_master);
	fh_spi->dwc.multi_para.active_wire_width = DUAL_WIRE_SUPPORT;
	fh_spi->dwc.multi_para.dir = dir;
	fh_spic_check_idle(&fh_spi->dwc);
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetApbReadWireMode(&fh_spi->dwc,DUAL_OUTPUT);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);

}

static void spic_bus_change_4_wire(struct spi_master *p_master, u32 dir)
{
	struct fh_spi_controller *fh_spi;

	fh_spi = spi_master_get_devdata(p_master);
	fh_spi->dwc.multi_para.active_wire_width = QUAD_WIRE_SUPPORT;
	fh_spi->dwc.multi_para.dir = dir;
	fh_spic_check_idle(&fh_spi->dwc);
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetApbReadWireMode(&fh_spi->dwc,QUAD_OUTPUT);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
}

static void spic_swap_data_width(struct fh_spi_controller *fh_spi, u32 reg_width)
{
	switch(reg_width) {
		case 32:
		Spi_SetSwap(&fh_spi->dwc, 1);
		Spi_SetWidth(&fh_spi->dwc, 32);
		break;

		case 16:
		Spi_SetSwap(&fh_spi->dwc, 1);
		Spi_SetWidth(&fh_spi->dwc, 16);
		break;

		case 8:
		Spi_SetSwap(&fh_spi->dwc, 0);
		Spi_SetWidth(&fh_spi->dwc, 8);
		break;
		default :
		BUG_ON(1);
	}
}

static void spic_fix_wire_mode(struct fh_spi_controller *fh_spi)
{
	struct spi_master *spi_master;
	struct spi_device *p_spi_dev;
	struct spi_transfer *t;

	p_spi_dev = fh_spi->active_spi_dev;
	spi_master = p_spi_dev->master;
	t = fh_spi->active_transfer;
	/*first check tx and rx can't be have multi wire the same time...*/
	if((t->tx_nbits & (SPI_NBITS_DUAL | SPI_NBITS_QUAD)) && ((t->rx_nbits & (SPI_NBITS_DUAL | SPI_NBITS_QUAD)))){
		dev_err(&p_spi_dev->dev, "%s, tx_nbits : rx_nbits = %x : %x\n",
		__func__,t->tx_nbits,t->rx_nbits);
		BUG_ON(1);
	}
	if(t->tx_buf){
		switch(t->tx_nbits){
			case SPI_NBITS_DUAL:
			//set dual out...
			spic_bus_change_2_wire(spi_master,SPI_DATA_DIR_OUT);
			break;
			case SPI_NBITS_QUAD:
			spic_bus_change_4_wire(spi_master,SPI_DATA_DIR_OUT);
			//set quad out...
			break;

			default:
			break;
		}
	}
	if(t->rx_buf) {
		switch(t->rx_nbits){
			case SPI_NBITS_DUAL:
			//set dual out...
			spic_bus_change_2_wire(spi_master, SPI_DATA_DIR_IN);
			break;
			case SPI_NBITS_QUAD:
			spic_bus_change_4_wire(spi_master, SPI_DATA_DIR_IN);
			//set quad out...
			break;

			default:
			break;
		}
	}

	/* first check if spi slave open as multi wire */
	if (p_spi_dev->mode & (SPI_TX_DUAL | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)) {
		/* check spi control hw multi info */
		if((fh_spi->dwc.multi_para.active_wire_width &
		(DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT))
		&& (fh_spi->dwc.multi_para.dir == SPI_DATA_DIR_OUT)) {
			fh_spi->dwc.transfer_mode = SPI_ONLY_TX_MODE;
		}
		else if((fh_spi->dwc.multi_para.active_wire_width &
		(DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT)) &&
		(fh_spi->dwc.multi_para.dir == SPI_DATA_DIR_IN)) {
			fh_spi->dwc.transfer_mode = SPI_ONLY_RX_MODE;
		}
		/*do not parse one wire..*/
	}
}

u32 Spi_SetClk_Masken(struct fh_spi *dw, unsigned int value)
{
	u32 data = dw_readl(dw, ccfgr);

	data &= ~(1<<15);
	data |= (value<<15);
	dw_writel(dw, ccfgr, data);
	return 0;
}

void spic_multi_wire_init(struct spi_master * p_master)
{
	struct fh_spi_controller *fh_spi;

	fh_spi = spi_master_get_devdata(p_master);
	fh_spi->dwc.multi_para.max_wire_size = fh_spi->dwc.board_info->ctl_wire_support;
	//multi support
	/* the spi->mode bits understood by this driver: */
	if (fh_spi->dwc.multi_para.max_wire_size == 4)
		p_master->mode_bits = SPI_CPOL | SPI_CPHA |\
		SPI_RX_QUAD | SPI_RX_DUAL;
	else if (fh_spi->dwc.multi_para.max_wire_size == 2)
		p_master->mode_bits = SPI_CPOL | SPI_CPHA |\
		SPI_RX_DUAL;

	fh_spi->dwc.multi_para.swap_max_size = CONFIG_SPI_SWAP_MAX_DATA_WIDTH;
	fh_spi->dwc.multi_para.change_1_wire = spic_bus_change_1_wire;
	fh_spi->dwc.multi_para.change_2_wire = spic_bus_change_2_wire;
	fh_spi->dwc.multi_para.change_4_wire = spic_bus_change_4_wire;
	fh_spi->dwc.multi_para.fix_wire_mode = spic_fix_wire_mode;
	fh_spi->dwc.multi_para.swap_data_width = spic_swap_data_width;
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetXip(&fh_spi->dwc, XIP_DISABLE);
	Spi_SetDPI(&fh_spi->dwc, DPI_DISABLE);
	Spi_SetQPI(&fh_spi->dwc, QPI_DISABLE);
	Spi_TimingConfigure(&fh_spi->dwc, 0x0);
	Spi_SetApbReadWireMode(&fh_spi->dwc,STANDARD_READ);
	Spi_SetClk_Masken(&fh_spi->dwc, 1);
	Spi_SetRxlevlel(&fh_spi->dwc, fh_spi->dwc.fifo_len - 2);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);	
}

u32 check_spi_sup_multi_wire(struct fh_spi_controller *fh_spi)
{
	if (fh_spi->dwc.id == 0)
		return 1;
	else
		return 0;
}

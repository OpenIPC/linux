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
static u32 Spi_DisableIrq(struct fh_spi *dw, u32 irq);
static u32 Spi_SetPolarity(struct fh_spi *dw, spi_polarity_e polarity);
static u32 Spi_SetPhase(struct fh_spi *dw, spi_phase_e phase);
static u32 Spi_SetFrameFormat(struct fh_spi *dw, spi_format_e format);
static u32 Spi_SetTransferMode(struct fh_spi *dw, spi_transfer_mode_e mode);
static u32 Spi_SetBaudrate(struct fh_spi *dw, u32 baudrate);
static u32 Spi_ReadTxfifolevel(struct fh_spi *dw);
static u32 Spi_ReadRxfifolevel(struct fh_spi *dw);
static u32 Spi_ReadRxlevlel(struct fh_spi *dw);
static u32 Spi_EnableSlaveen(struct fh_spi *dw, spi_slave_e port);
static u32 Spi_DisableSlaveen(struct fh_spi *dw, spi_slave_e port);
static u32 Spi_ReadData(struct fh_spi *dw);
static u32 Spi_SetRxdelay(struct fh_spi *dw, u32 data);
static inline u32 tx_max_tx_only(struct fh_spi_controller *fh_spi);
static inline u32 tx_max(struct fh_spi_controller *fh_spi);
static inline u32 rx_max(struct fh_spi_controller *fh_spi);
static void fh_spi_check_rxfifo_depth(struct fh_spi_controller *fh_spi);
static void spi_wait_tx_only_done(struct fh_spi_controller *fh_spi);
static void fh_spi_plat_info_get(struct fh_spi_platform_data *p_info,
struct platform_device *pdev);

static int poll_pump_tx_only_data(struct fh_spi_controller *fh_spi);
static int poll_rx_only_with_regwidth(struct fh_spi_controller *fh_spi,
u8 *rxbuf, u32 size, u32 reg_width);

static int poll_pump_rx_only_data(struct fh_spi_controller *fh_spi);
static int poll_pump_tx_rx_data(struct fh_spi_controller *fh_spi);
static void spi_ctl_fix_pump_data_mode(struct fh_spi_controller *fh_spi);
static int poll_pump_data(struct fh_spi_controller *fh_spi);
static irqreturn_t fh_spi_irq(int irq, void *dev_id);
static int fh_spi_init_hw(struct fh_spi_controller *fh_spi,
struct fh_spi_platform_data *board_info,struct spi_master *p_master);

static int fh_spi_handle_message(struct fh_spi_controller *fh_spi,
struct spi_message *m);

static int fh_spi_transfer(struct fh_spi_controller *spi,
struct spi_message *m);

static int spi_transfer_one_message(struct spi_master *master,
struct spi_message *msg);

static int fh_spi_setup(struct spi_device *spi);
static void fh_spi_cleanup(struct spi_device *spi);
static int fh_spi_setup_transfer(struct spi_device *spi,
struct spi_transfer *t);

static int  fh_spi_probe(struct platform_device *dev);
static int  fh_spi_remove(struct platform_device *dev);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/


u32 Spi_Enable(struct fh_spi *dw, spi_enable_e enable)
{
	dw_writel(dw, ssienr, enable);
	return CONFIG_OK;
}

static u32 Spi_DisableIrq(struct fh_spi *dw, u32 irq)
{
	u32 data = 0;

	data = dw_readl(dw, imr);
	data &= ~irq;
	dw_writel(dw, imr, data);
	return CONFIG_OK;
}

u32 Spi_ContinueReadNum(struct fh_spi *dw, u32 num)
{
	dw_writel(dw, ctrl1, (num-1));
	return CONFIG_OK;
}

static u32 Spi_SetPolarity(struct fh_spi *dw, spi_polarity_e polarity)
{
	u32 data;

	data = dw_readl(dw, ctrl0);
	data &= ~(u32) SPI_POLARITY_RANGE;
	data |= polarity;
	dw_writel(dw, ctrl0, data);
	return CONFIG_OK;
}

static u32 Spi_SetPhase(struct fh_spi *dw, spi_phase_e phase)
{
	u32 data;

	data = dw_readl(dw, ctrl0);
	data &= ~(u32) SPI_PHASE_RANGE;
	data |= phase;
	dw_writel(dw, ctrl0, data);
	return CONFIG_OK;
}

static u32 Spi_SetFrameFormat(struct fh_spi *dw, spi_format_e format)
{
	u32 data = 0;

	data = dw_readl(dw, ctrl0);
	data &= ~(u32) SPI_FRAME_FORMAT_RANGE;
	data |= format;
	dw_writel(dw, ctrl0, data);
	return CONFIG_OK;
}

static u32 Spi_SetTransferMode(struct fh_spi *dw, spi_transfer_mode_e mode)
{
	u32 data = 0;

	data = dw_readl(dw, ctrl0);
	data &= ~(u32) SPI_TRANSFER_MODE_RANGE;
	data |= mode;
	dw_writel(dw, ctrl0, data);
	return CONFIG_OK;
}

static u32 Spi_SetBaudrate(struct fh_spi *dw, u32 baudrate)
{
	dw_writel(dw, baudr, baudrate);
	return CONFIG_OK;
}

static u32 Spi_ReadTxfifolevel(struct fh_spi *dw)
{
	return dw_readl(dw, txflr);
}

static u32 Spi_ReadRxfifolevel(struct fh_spi *dw)
{
	return (u32) dw_readl(dw, rxflr);
}

static u32 Spi_ReadRxlevlel(struct fh_spi *dw)
{
	return dw_readl(dw, rxfltr);
}

u32 Spi_SetRxlevlel(struct fh_spi *dw, u32 level)
{
	dw_writel(dw, rxfltr, level);
	return CONFIG_OK;
}

u32 Spi_ReadStatus(struct fh_spi *dw)
{
	return (u32) dw_readl(dw, sr);
}

static u32 Spi_EnableSlaveen(struct fh_spi *dw, spi_slave_e port)
{
	u32 data = 0;

	gpio_direction_output(dw->active_cs_pin, 0);
	data = dw_readl(dw, ser);
	data |= port;
	dw_writel(dw, ser, data);
	return CONFIG_OK;
}

static u32 Spi_DisableSlaveen(struct fh_spi *dw, spi_slave_e port)
{
	u32 data = 0;

	gpio_direction_output(dw->active_cs_pin, 1);
	data = dw_readl(dw, ser);
	data &= ~port;
	dw_writel(dw, ser, data);
	return CONFIG_OK;
}

u32 Spi_WriteData(struct fh_spi *dw, u32 data)
{
	dw_writel(dw, dr, data);
	return WRITE_ONLY_OK;
}

static u32 Spi_ReadData(struct fh_spi *dw)
{
	return dw_readl(dw, dr);
}

static u32 Spi_SetRxdelay(struct fh_spi *dw, u32 data)
{
	dw_writel(dw, rx_sample_dly, data);
	return CONFIG_OK;
}

u32 Spi_RawIsrstatus(struct fh_spi *dw)
{
	u32 data = dw_readl(dw, risr);
	return data;
}

static inline u32 tx_max_tx_only(struct fh_spi_controller *fh_spi)
{
	u32 hw_tx_level;

	hw_tx_level = Spi_ReadTxfifolevel(&fh_spi->dwc);
	hw_tx_level = fh_spi->dwc.fifo_len - hw_tx_level;

	return min(hw_tx_level, fh_spi->dwc.tx_len);
}

static inline u32 tx_max(struct fh_spi_controller *fh_spi)
{

	u32 hw_tx_level, hw_rx_level;
	u32 temp_tx_lev;

	temp_tx_lev = Spi_ReadTxfifolevel(&fh_spi->dwc);
	hw_rx_level = temp_tx_lev + Spi_ReadRxfifolevel(&fh_spi->dwc);
	if (hw_rx_level >= fh_spi->dwc.fifo_len)
		return 0;
	/* add shift data... maybe should add apb bus delay */
	hw_rx_level++;

	hw_tx_level = temp_tx_lev;
	hw_tx_level = fh_spi->dwc.fifo_len - hw_tx_level;
	hw_rx_level = fh_spi->dwc.fifo_len - hw_rx_level;
	/* min(hw_tx_level, fh_spi->dwc.tx_len); */
	return min(min(hw_tx_level, fh_spi->dwc.tx_len), hw_rx_level);
}


/* Return the max entries we should read out of rx fifo */
static inline u32 rx_max(struct fh_spi_controller *fh_spi)
{
	u32 hw_rx_level;

	hw_rx_level = Spi_ReadRxfifolevel(&fh_spi->dwc);
	return hw_rx_level;
}

static void fh_spi_check_rxfifo_depth(struct fh_spi_controller *fh_spi)
{
	u32 w_i, r_i, ori;

	ori = Spi_ReadRxlevlel(&fh_spi->dwc);
	for (r_i = w_i = RX_FIFO_MIN_LEN; w_i < RX_FIFO_MAX_LEN; r_i++) {
		Spi_SetRxlevlel(&fh_spi->dwc, ++w_i);
		if (r_i == Spi_ReadRxlevlel(&fh_spi->dwc))
			break;
	}
	Spi_SetRxlevlel(&fh_spi->dwc, ori);
	fh_spi->dwc.fifo_len = r_i + 1;
}


static void spi_wait_tx_only_done(struct fh_spi_controller *fh_spi)
{
	u32 status;

	do {
		status = Spi_ReadStatus(&fh_spi->dwc);
	} while ((status & 0x01) || (!(status & 0x04)));
}


static void fh_spi_plat_info_get(struct fh_spi_platform_data *p_info,
struct platform_device *pdev)
{

#ifdef CONFIG_USE_OF
	struct device_node *np = pdev->dev.of_node;
	int temp, i;
	char spi_cs_name[20] = { 0 };

	if (!of_property_read_u32(np, "clk_in", &temp))
		p_info->apb_clock_in = temp;
	if (!of_property_read_u32(np, "num-cs", &temp))
		p_info->slave_max_num = temp;
	if (!of_property_read_u32(np, "bus_no", &temp))
		p_info->bus_no = temp;
	if (!of_property_read_u32(np, "multi_wire_size", &temp))
		p_info->ctl_wire_support = temp;
	if (!of_property_read_u32(np, "clock_source_num", &temp))
		p_info->clock_source_num = temp;
	if (p_info->clock_source_num)
		of_property_read_u32_array(np, "clock_source", p_info->clock_source, p_info->clock_source_num);
	
	p_info->clk_name = (char *)of_get_property(np, "clk_name", &temp);
	p_info->hclk_name = (char *)of_get_property(np, "hclk_name", &temp);
	p_info->pclk_name = (char *)of_get_property(np, "pclk_name", &temp);

	/* dma para >>*/

	if (strcmp("enable", of_get_property(np, "dma_enable", &temp)) == 0)
		p_info->dma_transfer_enable = SPI_TRANSFER_USE_DMA;
	if (!of_property_read_u32(np, "rx_hs_no", &temp))
		p_info->rx_handshake_num = temp;
	if (!of_property_read_u32(np, "rx_dma_channel", &temp))
		p_info->rx_dma_channel = temp;
	if (strcmp("enable", of_get_property(np, "dma_protctl_enable", &temp)) == 0)
		p_info->dma_protctl_enable = SPI_DMA_PROTCTL_ENABLE;
	if (!of_property_read_u32(np, "dma_protctl_data", &temp))
		p_info->dma_protctl_data = temp;
	if (strcmp("enable", of_get_property(np, "dma_master_sel_enable", &temp)) == 0)
		p_info->dma_master_sel_enable = SPI_DMA_MASTER_SEL_ENABLE;
	if (!of_property_read_u32(np, "dma_master_ctl_sel", &temp))
		p_info->dma_master_ctl_sel = temp;
	if (!of_property_read_u32(np, "dma_master_mem_sel", &temp))
		p_info->dma_master_mem_sel = temp;

	/* <<dma para */

	for (i = 0; i < p_info->slave_max_num; i++) {
		sprintf(spi_cs_name, "%s%d%s", "cs", i, "_gpio");
		if (!of_property_read_u32(np, spi_cs_name, &temp))
			p_info->cs_data[i].GPIO_Pin = temp;
		else
			BUG_ON(1);
	}
#else
	/* just cpy plat data.*/
	memcpy((void *)p_info, (void *)pdev->dev.platform_data,
	sizeof(struct fh_spi_platform_data));
	if(p_info->ctl_wire_support & QUAD_WIRE_SUPPORT)
		p_info->ctl_wire_support = 4;
	else if(p_info->ctl_wire_support & DUAL_WIRE_SUPPORT)
		p_info->ctl_wire_support = 2;
	else
		p_info->ctl_wire_support = 1;
#endif	

	if (p_info->slave_max_num >
	SPI_MASTER_CONTROLLER_MAX_SLAVE) {
		dev_err(&pdev->dev, "plat set slave gpio too much 0x%x\n", p_info->slave_max_num);
		BUG_ON(1);
	}
}


static int poll_pump_tx_only_data(struct fh_spi_controller *fh_spi)
{
	register u32 tx_fifo_capability;
	u8 *txbuf;

	txbuf = (u8 *) fh_spi->dwc.tx_buff;
	/*tx_max_tx_only*/
	while (fh_spi->dwc.tx_len) {
		tx_fifo_capability = tx_max_tx_only(fh_spi);
		fh_spi->dwc.tx_len -= tx_fifo_capability;
		while (tx_fifo_capability) {
			Spi_WriteData(&fh_spi->dwc, *txbuf++);
			fh_spi->dwc.tx_buff++;
			tx_fifo_capability--;
		}
	}
	/*wait idle and tx fifo empty..*/
	spi_wait_tx_only_done(fh_spi);
	return 0;
}



u32 rx_only_fix_data_width(struct fh_spi_controller *fh_spi, u32 size)
{
	u32 data_width = 0;
#ifdef CONFIG_SPI_USE_MULTI_WIRE
	if(check_spi_sup_multi_wire(fh_spi))
	{
		if (!(((int)fh_spi->dwc.rx_buff % 4)  || (size % 4)))
			data_width = 32;
		else if (!(((int)fh_spi->dwc.rx_buff % 2)  || (size % 2)))
			data_width = 16;
		else
			data_width = 8;
		data_width = min(data_width, fh_spi->dwc.multi_para.swap_max_size);
	}
	else
		data_width = 8;
#else
	data_width = 8;
#endif

	return data_width;
}

static int poll_rx_only_with_regwidth(struct fh_spi_controller *fh_spi,
u8 *rxbuf, u32 size, u32 reg_width)
{
	register u32 rx_fifo_capability;
	u32 otp_xfer_size;
	u8 *rxbuf_8;
	u16 *rxbuf_16;
	u32 *rxbuf_32;

	rxbuf_8 = (u8 *)rxbuf; 
	rxbuf_16  = (u16 *)rxbuf;
	rxbuf_32 = (u32 *)rxbuf;
#ifdef CONFIG_SPI_USE_MULTI_WIRE
	if (reg_width > 8)
		fh_spi->dwc.multi_para.swap_data_width(fh_spi, reg_width);
#endif

	/* refix size div reg_width */
	size = size / (reg_width / 8);
	/* printk("reg width is %d\n",reg_width); */
start:
	/* or rx fifo error.. */
	if (size == 0) {
#ifdef CONFIG_SPI_USE_MULTI_WIRE
		if (reg_width > 8)
			fh_spi->dwc.multi_para.swap_data_width(fh_spi, 8);
#endif
		return 0;
	}

	otp_xfer_size = min((u32)fh_spi->dwc.fifo_len, size);
	size -= otp_xfer_size;
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_ContinueReadNum(&fh_spi->dwc, otp_xfer_size);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
	Spi_WriteData(&fh_spi->dwc, 0xffffffff);
	do {
		rx_fifo_capability = rx_max(fh_spi);
		otp_xfer_size -= rx_fifo_capability;
		while (rx_fifo_capability) {
			if (reg_width == 32)
				*rxbuf_32++ = (u32)Spi_ReadData(&fh_spi->dwc);
			else if (reg_width == 16)
				*rxbuf_16++ = (u16)Spi_ReadData(&fh_spi->dwc);
			else
				*rxbuf_8++ = (u8)Spi_ReadData(&fh_spi->dwc);
			rx_fifo_capability--;
		}
	}
	while (otp_xfer_size);
	goto start;
}

static int poll_pump_rx_only_data(struct fh_spi_controller *fh_spi)
{
	u32 size;
	int ret;
	u32 data_width = 0;

	size = fh_spi->dwc.tx_len;
	data_width = rx_only_fix_data_width(fh_spi, size);
	ret = poll_rx_only_with_regwidth(fh_spi,
	fh_spi->dwc.rx_buff, size, data_width);

	return ret;

}

static int poll_pump_tx_rx_data(struct fh_spi_controller *fh_spi)
{
	register u32 rx_fifo_capability, tx_fifo_capability;
	u8 *txbuf;
	u8 *rxbuf;
	u16 data;
	u32 size;

	rxbuf = (u8 *)fh_spi->dwc.rx_buff;
	txbuf = (u8 *)fh_spi->dwc.tx_buff;
	size = fh_spi->active_transfer->len;

	goto first;
start:
	rx_fifo_capability = rx_max(fh_spi);
	fh_spi->dwc.rx_len += rx_fifo_capability;
	if (rxbuf != NULL) {
		fh_spi->dwc.rx_buff += rx_fifo_capability;
		while (rx_fifo_capability) {
			*rxbuf++ = Spi_ReadData(&fh_spi->dwc);
			rx_fifo_capability--;
		}

	} else {
		while (rx_fifo_capability) {
			data = Spi_ReadData(&fh_spi->dwc);
			rx_fifo_capability--;
		}
	}
	if (fh_spi->dwc.rx_len == size)
		return 0;
first:
	tx_fifo_capability = tx_max(fh_spi);
	fh_spi->dwc.tx_len -= tx_fifo_capability;
	if (txbuf != NULL) {
		fh_spi->dwc.tx_buff += tx_fifo_capability;
		while (tx_fifo_capability) {
			Spi_WriteData(&fh_spi->dwc, *txbuf++);
			tx_fifo_capability--;
		}

	} else {
		while (tx_fifo_capability) {
			Spi_WriteData(&fh_spi->dwc, 0xff);
			tx_fifo_capability--;
		}
	}

	goto start;
}

static void spi_ctl_fix_pump_data_mode(struct fh_spi_controller *fh_spi)
{

	if ((fh_spi->dwc.rx_buff == NULL) && (fh_spi->dwc.tx_buff != NULL))
		fh_spi->dwc.transfer_mode = SPI_ONLY_TX_MODE;
	else if ((fh_spi->dwc.rx_buff != NULL) && (fh_spi->dwc.tx_buff == NULL))
		fh_spi->dwc.transfer_mode = SPI_ONLY_RX_MODE;
	else
		fh_spi->dwc.transfer_mode = SPI_TX_RX_MODE;
}

static int poll_pump_data(struct fh_spi_controller *fh_spi)
{

	if (fh_spi->dwc.transfer_mode == SPI_ONLY_TX_MODE)
		poll_pump_tx_only_data(fh_spi);
	else if (fh_spi->dwc.transfer_mode == SPI_ONLY_RX_MODE)
		poll_pump_rx_only_data(fh_spi);
	else
		poll_pump_tx_rx_data(fh_spi);

	return 0;
}


static irqreturn_t fh_spi_irq(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}


static int fh_spi_init_hw(struct fh_spi_controller *fh_spi,
struct fh_spi_platform_data *board_info,struct spi_master *p_master)
{
	int status, i;

	fh_spi->dwc.id = board_info->bus_no;
	fh_spi_check_rxfifo_depth(fh_spi);
	fh_spi->dwc.num_cs = board_info->slave_max_num;
	for (i = 0; i < fh_spi->dwc.num_cs; i++) {
		fh_spi->dwc.cs_data[i].GPIO_Pin =
		                board_info->cs_data[i].GPIO_Pin;
		fh_spi->dwc.cs_data[i].name = board_info->cs_data[i].name;
	}
	fh_spi->dwc.transfer_mode = SPI_TX_RX_MODE;
	do {
		status = Spi_ReadStatus(&fh_spi->dwc);
	} while (status & 0x01);

	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_DisableIrq(&fh_spi->dwc, SPI_IRQ_ALL);
	Spi_SetFrameFormat(&fh_spi->dwc, SPI_MOTOROLA_MODE);
	Spi_SetTransferMode(&fh_spi->dwc, fh_spi->dwc.transfer_mode);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
	return 0;
}


static int fh_spi_handle_message(struct fh_spi_controller *fh_spi,
struct spi_message *m)
{
	bool first, last;
	struct spi_transfer *t, *tmp_t;
	int status = 0;
	int cs_change;
	struct spi_device	*spi_dev;
	struct spi_master	*spi_master;
	u8 pump_mode = PUMP_DATA_POLL_MODE;

	cs_change = 1;
	m->actual_length = 0;
	fh_spi->active_message = m;
	/*get spi slave from the message bind on it.*/
	spi_dev = m->spi;
	spi_master = spi_dev->master;
	/*bind the active spi dev to the controller..*/
	fh_spi->active_spi_dev = spi_dev;
	list_for_each_entry_safe(t, tmp_t, &m->transfers, transfer_list)
	{
		first = (&t->transfer_list == m->transfers.next);
		last = (&t->transfer_list == m->transfers.prev);
		if (first || t->speed_hz || t->bits_per_word)
			fh_spi_setup_transfer(m->spi, t);

		if (cs_change)
			Spi_EnableSlaveen(&fh_spi->dwc, fh_spi->dwc.slave_port);
		cs_change = t->cs_change;
		fh_spi->active_transfer = t;
		fh_spi->dwc.tx_len = t->len;
		fh_spi->dwc.rx_len = 0;
		fh_spi->dwc.tx_buff = (void *) t->tx_buf;
		fh_spi->dwc.rx_buff = t->rx_buf;

		/* here we don't need to check multi wire tobe legal...
		the spi core 'spi_setup' will make sure the spi dev and core have right open mode.*/

		spi_ctl_fix_pump_data_mode(fh_spi);
#ifdef CONFIG_SPI_USE_MULTI_WIRE
		if(check_spi_sup_multi_wire(fh_spi))
			fh_spi->dwc.multi_para.fix_wire_mode(fh_spi);
#endif


#ifdef CONFIG_SPI_USE_DMA
		if (!!fh_spi->dwc.board_info->dma_transfer_enable){
			if (fh_spi->dwc.transfer_mode == SPI_ONLY_RX_MODE && t->len > DMA_TRANS_GATE_LEVEL){
				if(((unsigned int)t->rx_buf % 4 == 0) && (t->len % 4 == 0)){
					pump_mode = PUMP_DATA_DMA_MODE;
				}
			}
		}
#endif

		Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
		Spi_SetTransferMode(&fh_spi->dwc, fh_spi->dwc.transfer_mode);
		Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
		switch(pump_mode){
#ifdef CONFIG_SPI_USE_DMA
			case PUMP_DATA_DMA_MODE:
				status = fh_spi->dwc.dma_para.dma_read(fh_spi);
			break;
#endif
			case PUMP_DATA_POLL_MODE:
				status = poll_pump_data(fh_spi);
			break;

			default:
			BUG_ON(1);
			break;

		}

#ifdef CONFIG_SPI_USE_MULTI_WIRE
		/*back to onewire.*/
		if(check_spi_sup_multi_wire(fh_spi))
			fh_spi->dwc.multi_para.change_1_wire(fh_spi->active_spi_dev->master);
#endif

		if (!cs_change && last)
			Spi_DisableSlaveen(&fh_spi->dwc,
					fh_spi->dwc.slave_port);

		m->actual_length += t->len;
		if (status)
			break;
	}

	return status;
}


static int fh_spi_transfer(struct fh_spi_controller *spi,
struct spi_message *m)
{
	struct fh_spi_controller *fh_spi = spi;

	mutex_lock(&fh_spi->lock);
	m->status = -EINPROGRESS;
	m->status = fh_spi_handle_message(fh_spi, m);
	mutex_unlock(&fh_spi->lock);
	return 0;

}

static int spi_transfer_one_message(struct spi_master *master,
struct spi_message *msg)
{
	struct fh_spi_controller *fh_spi = spi_master_get_devdata(master);
	/*here may be i should start a queue work.*/
	fh_spi_transfer(fh_spi,msg);
	/* call core to set msg done */
	spi_finalize_current_message(master);
	return 0;
}


static int fh_spi_setup(struct spi_device *spi)
{
	/* spi_setup() does basic checks,
	 * stmp_spi_setup_transfer() does more later
	 */
	struct fh_spi_controller *fh_spi = spi_master_get_devdata(spi->master);

	fh_spi->dwc.active_cs_pin =
	fh_spi->dwc.cs_data[spi->chip_select].GPIO_Pin;

	if (spi->chip_select >= fh_spi->dwc.num_cs) {
		dev_err(&spi->dev, "%s, unsupported chip select no=%d\n",
	__func__, spi->chip_select);
		return -EINVAL;
	}
	fh_spi->dwc.slave_port = 1 << spi->chip_select;

	if (spi->bits_per_word != 8) {
		dev_err(&spi->dev, "%s, unsupported bits_per_word=%d\n",
	__func__, spi->bits_per_word);
		return -EINVAL;
	}
	return 0;
}

/*free the controller malloc data for the spi_dev*/
static void fh_spi_cleanup(struct spi_device *spi)
{

}

static u32 fh_spi_clock_adjust(u32 *div,
struct fh_spi_controller *fh_spi, u32 speed)
{
	u32 rate;
	struct clk *clk;

	u32 delta = 0xFFFFFFFF;
	u32 i     = 0;
	u32 j     = 1;
	u32 selected_rate = 0;
	u32 selected_div  = 0;
	u32 *clock_source = fh_spi->dwc.board_info->clock_source;
	u32 source_num = fh_spi->dwc.board_info->clock_source_num;

	if (fh_spi->dwc.board_info->clock_in_use == speed)
		return 0;

	for (; i < source_num; i++) {
		for (j = 1; j < 0xFF; j++) {
			rate = clock_source[i] / (j * 2);

			if (rate <= speed) {
				if (delta > (speed - rate)) {
					delta = speed - rate;
					selected_rate = clock_source[i];
					selected_div  = j * 2;
				}
			}
		}
	}

	clk = clk_get(&fh_spi->p_dev->dev, fh_spi->dwc.board_info->clk_name);
	clk_set_rate(clk, selected_rate);
	*div = selected_div;
	fh_spi->dwc.board_info->clock_in_use = speed;

	return 1;
}

static int fh_spi_setup_transfer(struct spi_device *spi,
struct spi_transfer *t)
{

	u8 bits_per_word;
	u32 hz;
	u32 div;
	u32 res;
	u32 cpol,cpha;
	struct fh_spi_controller *fh_spi = spi_master_get_devdata(spi->master);

	bits_per_word = spi->bits_per_word;
	if (t && t->bits_per_word)
		bits_per_word = t->bits_per_word;

	/*
	 * Calculate speed:
	 *  - by default, use maximum speed from ssp clk
	 *  - if device overrides it, use it
	 *  - if transfer specifies other speed, use transfer's one
	 */

	hz = fh_spi->dwc.max_freq;
	if (spi->max_speed_hz)
		hz = min(hz, spi->max_speed_hz);

	res = fh_spi_clock_adjust(&div, fh_spi, hz);

	if (bits_per_word != 8) {
		dev_err(&spi->dev, "%s, unsupported bits_per_word=%d\n",
				__func__, bits_per_word);
		return -EINVAL;
	}

	if (spi->mode & SPI_CPOL)
		cpol = SPI_POLARITY_HIGH;
	else
		cpol = SPI_POLARITY_LOW;

	if (spi->mode & SPI_CPHA)
		cpha = SPI_PHASE_TX_FIRST;
	else
		cpha = SPI_PHASE_RX_FIRST;

	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetPolarity(&fh_spi->dwc, cpol);
	Spi_SetPhase(&fh_spi->dwc, cpha);
	if (res) 
		Spi_SetBaudrate(&fh_spi->dwc, div);
	Spi_SetRxdelay(&fh_spi->dwc, spi->sample_delay);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
	fh_spi_setup(spi);

	return 0;
}

static int  fh_spi_probe(struct platform_device *dev)
{

	int err = 0;
	struct spi_master *master;
	struct fh_spi_controller *fh_spi;
	int ret, i, j;
	struct clk *clk;
#ifdef CONFIG_USE_OF
	struct device_node *np;
	struct resource res;
#else
	struct resource *p_res;
#endif
	/*board info below*/
	struct fh_spi_platform_data *spi_platform_info;

	spi_platform_info = kzalloc(sizeof(struct fh_spi_platform_data), GFP_KERNEL);
	if (!spi_platform_info) {
		err = -ENOMEM;
		dev_err(&dev->dev, "%s, master malloc failed.\n", __func__);
		goto out0;
	}

	master = spi_alloc_master(&dev->dev, sizeof(struct fh_spi_controller));
	if (master == NULL) {
		err = -ENOMEM;
		kfree(spi_platform_info);
		dev_err(&dev->dev, "%s, master malloc failed.\n", __func__);
		goto out0;
	}

	fh_spi_plat_info_get(spi_platform_info, dev);

	fh_spi = spi_master_get_devdata(master);
	if (!fh_spi) {
		dev_err(&dev->dev, "%s, master dev data is null.\n", __func__);
		err = -ENOMEM;
		//free the spi master data
		goto out_put_master;
	}
	/* controller's master dev is platform dev~~ */
	fh_spi->master_dev = &dev->dev;
	/* bind the platform dev */
	fh_spi->p_dev = dev;

	/* set the platform dev private data */
	platform_set_drvdata(dev, master);
#ifdef CONFIG_USE_OF
	int id;
	np = dev->dev.of_node;
	fh_spi->dwc.regs = of_iomap(np, 0);
	of_address_to_resource(np, 0, &res);
	fh_spi->dwc.paddr = res.start;
	fh_spi->dwc.irq = irq_of_parse_and_map(np, 0);
	id = of_alias_get_id(np, "spi");
	sprintf(fh_spi->dwc.isr_name, "spi-%d", id);
#else
	p_res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!p_res)
		BUG_ON(1);
	fh_spi->dwc.regs = ioremap(p_res->start, resource_size(p_res));
	fh_spi->dwc.paddr = p_res->start;
	fh_spi->dwc.irq = platform_get_irq(dev, 0);
	fh_spi->dwc.irq = irq_create_mapping(NULL, fh_spi->dwc.irq);
	sprintf(fh_spi->dwc.isr_name, "spi-%d", dev->id);
#endif

	if (!fh_spi->dwc.irq) {
		pr_err("%s: ERROR: getting resource failed"
		       "cannot get IORESOURCE_IRQ\n", __func__);
		ret = -ENXIO;
		goto out_put_master;
	}
	err = request_irq(fh_spi->dwc.irq, fh_spi_irq, 0, fh_spi->dwc.isr_name,
	                fh_spi);
	if (err) {
		dev_dbg(&dev->dev, "request_irq failed, %d\n", err);
		goto out_set_plat_drvdata_null;
	}

	mutex_init(&fh_spi->lock);
	master->bus_num = spi_platform_info->bus_no;
	master->transfer_one_message = spi_transfer_one_message;
	master->setup = fh_spi_setup;
	master->cleanup = fh_spi_cleanup;

	master->max_speed_hz = spi_platform_info->apb_clock_in / 2;
	fh_spi->dwc.max_freq =  spi_platform_info->apb_clock_in / 2;
	master->dev.of_node = dev->dev.of_node;
	/* support 8bit one work.. */
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->dev.of_node = dev->dev.of_node;
	master->num_chipselect = spi_platform_info->slave_max_num;
	master->mode_bits = SPI_CPOL | SPI_CPHA;

	clk = clk_get(&fh_spi->p_dev->dev, spi_platform_info->clk_name);
	if (IS_ERR(clk)) {
		dev_err(&fh_spi->p_dev->dev, "cannot find the spi%d clk.\n",
		fh_spi->dwc.id);
		err = PTR_ERR(clk);
		goto out_free_irq;
	}
	clk_set_rate(clk, SPI_DEFAULT_CLK);
	clk_prepare_enable(clk);

	if (spi_platform_info->hclk_name) {
		clk = clk_get(&fh_spi->p_dev->dev,
		spi_platform_info->hclk_name);
		if (IS_ERR(clk)) {
			dev_err(&fh_spi->p_dev->dev, "cannot find the spi%d hclk.\n",
			fh_spi->dwc.id);
			err = PTR_ERR(clk);
			goto out_free_irq;
		}
		clk_prepare_enable(clk);
	}

	if (spi_platform_info->pclk_name) {
		clk = clk_get(&fh_spi->p_dev->dev,
		spi_platform_info->pclk_name);
		if (IS_ERR(clk)) {
			dev_err(&fh_spi->p_dev->dev, "cannot find the spi%d pclk.\n",
			fh_spi->dwc.id);
			err = PTR_ERR(clk);
			goto out_free_irq;
		}
		clk_prepare_enable(clk);
	}
	fh_spi->dwc.board_info = spi_platform_info;
	ret = fh_spi_init_hw(fh_spi, spi_platform_info, master);

	if (ret) {
		err = ret;
		goto out_free_irq;
	}
#ifdef CONFIG_SPI_USE_MULTI_WIRE
	if(check_spi_sup_multi_wire(fh_spi))
		spic_multi_wire_init(master);
#endif

#ifdef CONFIG_SPI_USE_DMA
	fh_spi_dma_init(fh_spi);
#endif

	for (i = 0; i < fh_spi->dwc.num_cs; i++) {
		ret = gpio_request(fh_spi->dwc.cs_data[i].GPIO_Pin,
		                fh_spi->dwc.cs_data[i].name);
		if (ret) {
			dev_err(&dev->dev,
			                "spi failed to request the gpio:%d\n",
			                fh_spi->dwc.cs_data[i].GPIO_Pin);

			if (i != 0) {
				for (j = 0; j < i; j++)
					gpio_free(fh_spi->dwc.cs_data[j].GPIO_Pin);
			}
			err = ret;

			goto out_clk_disable;
		}

		gpio_direction_output(fh_spi->dwc.cs_data[i].GPIO_Pin,
		                GPIOF_OUT_INIT_HIGH);
	}
	err = spi_register_master(master);
	if (err) {
		dev_dbg(&dev->dev, "cannot register spi master, %d\n", err);
		goto out_gpio_tree;
	}

	return 0;

out_gpio_tree:
	for (i = 0; i < fh_spi->dwc.num_cs; i++)
		gpio_free(fh_spi->dwc.cs_data[i].GPIO_Pin);

out_clk_disable:
out_free_irq:
	free_irq(fh_spi->dwc.irq, fh_spi);
out_set_plat_drvdata_null:
	memset(fh_spi, 0, sizeof(struct fh_spi_controller));
	platform_set_drvdata(dev, NULL);
out_put_master:
	kfree(spi_platform_info);
	spi_master_put(master);
out0:
	return err;

}


static int  fh_spi_remove(struct platform_device *dev)
{
	struct resource *r;
	struct spi_master *master;
	struct fh_spi_controller *fh_spi;
	int i;

	master = platform_get_drvdata(dev);
	if (master == NULL)
		goto out0;

	fh_spi = spi_master_get_devdata(master);
	spi_unregister_master(master);
	/*gpio free*/
	for (i = 0; i < fh_spi->dwc.num_cs; i++)
		gpio_free(fh_spi->dwc.cs_data[i].GPIO_Pin);
	/*clk disable*/
	/*queue free*/
#ifdef CONFIG_SPI_USE_DMA
	if (fh_spi->dwc.board_info->dma_transfer_enable == SPI_TRANSFER_USE_DMA) {
		if(fh_spi->dwc.dma_para.rx.chan) {
			dma_release_channel(fh_spi->dwc.dma_para.rx.chan);
			fh_spi->dwc.dma_para.rx.chan->private = NULL;
		}
	}
#endif
	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	/*io map free*/
	iounmap(fh_spi->dwc.regs);
	/*mem region free*/
	release_mem_region(r->start, resource_size(r));
	/*irq free*/
	free_irq(fh_spi->dwc.irq, fh_spi);
	/*clear the spi master dev data*/
	memset(fh_spi, 0, sizeof(struct fh_spi_controller));
	/*put master*/
	platform_set_drvdata(dev, NULL);
	spi_master_put(master);

out0:
	return 0;


}

static const struct of_device_id fh_spi_of_match[] = {
	{.compatible = "fh,fh-spi",},
	{},
};

MODULE_DEVICE_TABLE(of, fh_spi_of_match);

static struct platform_driver fh_spi_driver = {
	.driver = {
		.name = "fh_spi",
		.of_match_table = fh_spi_of_match,
	},
	.probe = fh_spi_probe,
	.remove = fh_spi_remove,
};

#ifdef CONFIG_DEFERRED_INIICALLS_SPI
deferred_module_platform_driver(fh_spi_driver);
#else
module_platform_driver(fh_spi_driver);
#endif

MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_DESCRIPTION("Fullhan SPI device driver");
MODULE_LICENSE("GPL");

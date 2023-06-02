
#include "fh_spi_common.h"

static void Spi_SetDmaRxDataLevel(struct fh_spi *dw, u32 level)
{
	dw_writel(dw, dmardlr, level);
}

static void Spi_SetDmaControlEnable(struct fh_spi *dw,
	spi_dma_control_mode_e enable_pos)
{
	u32 data;
	data = dw_readl(dw, dmacr);
	data |= enable_pos;
	dw_writel(dw, dmacr, data);
}

static void Spi_SetDmaControlDisable(struct fh_spi *dw,
	spi_dma_control_mode_e enable_pos)
{
	u32 data;
	data = dw_readl(dw, dmacr);
	data &= ~enable_pos;
	dw_writel(dw, dmacr, data);
}

static bool fh_spi_dma_chan_filter(struct dma_chan *chan, void *param)
{
	int dma_channel = *(int *) param;
	bool ret = false;

	if (chan->chan_id == dma_channel)
		ret = true;
	return ret;
}

unsigned int fh_spi_dma_set_rx_config(struct fh_spi_controller *fh_spi,
struct dma_slave_config *rx_config)
{
	unsigned int ret;

	rx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	ret = SG_ONE_TIME_MAX_SIZE;
	rx_config->src_addr = fh_spi->dwc.paddr + SPI_DATA_REG_OFFSET;
	rx_config->slave_id = fh_spi->dwc.dma_para.rx.hs_no;
	rx_config->src_maxburst = 8;
	rx_config->dst_maxburst = 8;
	rx_config->device_fc = false;
	rx_config->direction = DMA_DEV_TO_MEM;
#ifdef CONFIG_SPI_USE_MULTI_WIRE
	if(check_spi_sup_multi_wire(fh_spi)){
		rx_config->src_addr = fh_spi->dwc.paddr + 0x1000;
		ret = min(SG_ONE_TIME_MAX_SIZE, 0x1000);
		rx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	}
#endif
	return ret;
}

void fh_spi_dma_set_rx_src_width(struct dma_slave_config *rx_config, unsigned int data_width)
{
	if (data_width == 8)
		rx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	else if (data_width == 16)
		rx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	else if (data_width == 32)
		rx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	else
		BUG_ON(1);
}

void fh_spi_dma_set_rx_extra_para(struct fh_spi_controller *fh_spi,
struct fh_dma_extra *p_ext_para)
{
#if 0
	if (!!fh_spi->dwc.board_info->dma_protctl_enable) {
		p_ext_para->protctl_flag = PROTCTL_ENABLE;
		p_ext_para->protctl_data =
                  fh_spi->dwc.board_info->dma_protctl_data;
	}

	if (!!fh_spi->dwc.board_info->dma_master_sel_enable) {
		p_ext_para->master_flag = MASTER_SEL_ENABLE;
		p_ext_para->src_master =
		fh_spi->dwc.board_info->dma_master_ctl_sel;
		p_ext_para->dst_master =
		fh_spi->dwc.board_info->dma_master_mem_sel;
	}
#endif
	p_ext_para->dinc = FH_DMA_SLAVE_INC;
	p_ext_para->sinc = FH_DMA_SLAVE_FIX;
#ifdef CONFIG_SPI_USE_MULTI_WIRE
	if(check_spi_sup_multi_wire(fh_spi)){
		p_ext_para->sinc = FH_DMA_SLAVE_INC;
	}
#endif

}

static int dma_set_rx_para(struct fh_spi_controller *fh_spi,
void (*call_back)(void *arg), unsigned int rx_para_size, unsigned int data_width)
{
	struct fh_dma_extra ext_para;
	struct dma_slave_config *rx_config;
	struct spi_transfer *t;
	struct dma_chan *rxchan;
	struct scatterlist *p_sca_list;
	unsigned int sg_size = 0;
	int i, xfer_len, one_sg_data_len;
	unsigned char *temp_buf;
	unsigned int one_time_size;
	struct dma_async_tx_descriptor *p_desc = 0;

	t = fh_spi->active_transfer;
	rxchan = fh_spi->dwc.dma_para.rx.chan;
	rx_config = &fh_spi->dwc.dma_para.rx.cfg;
	memset(rx_config, 0, sizeof(struct dma_slave_config));
	memset(&ext_para, 0, sizeof(struct fh_dma_extra));

	one_time_size = fh_spi_dma_set_rx_config(fh_spi, rx_config);
	fh_spi_dma_set_rx_src_width(rx_config, data_width);
	xfer_len = rx_para_size;
	temp_buf = (unsigned char *)t->rx_buf;
	fh_spi_dma_set_rx_extra_para(fh_spi, &ext_para);

	if (xfer_len >= one_time_size)
		sg_size =  xfer_len / one_time_size;

	if (xfer_len % one_time_size)
		sg_size++;

	if (sg_size > MAX_SG_LEN) {
		printk("%s_%d :: too large sg size:0x%x\n",
		__func__, __LINE__, sg_size);
		return -1;
	}

	p_sca_list = &fh_spi->dwc.dma_para.rx.sgl[0];
	for (i = 0; i < sg_size; i++, p_sca_list++)
	{
		one_sg_data_len = min((u32)xfer_len, one_time_size);
		xfer_len -= one_sg_data_len;
		p_sca_list->dma_address = dma_map_single(
			rxchan->dev->device.parent,
			(void *)temp_buf, one_sg_data_len,
			DMA_FROM_DEVICE);
		fh_spi->dwc.dma_para.rx.sgl_data_size[i] = one_sg_data_len;
		temp_buf += one_sg_data_len;
		p_sca_list->length = one_sg_data_len;
	}

	dmaengine_slave_config(rxchan, rx_config);
	p_desc = rxchan->device->device_prep_slave_sg(rxchan,
	&fh_spi->dwc.dma_para.rx.sgl[0], sg_size, DMA_DEV_TO_MEM,
	DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
	&ext_para);
	BUG_ON(!p_desc);
	fh_spi->dwc.dma_para.rx.actual_sgl_size = sg_size;
	p_desc->callback = call_back;
	p_desc->callback_param = fh_spi;
	/*write back to host para*/
	fh_spi->dwc.dma_para.rx.desc = p_desc;
	return 0;
}

void unmap_dma_rx_sg(struct fh_spi_controller *fh_spi){
	struct dma_chan *rxchan;
	struct scatterlist *p_sca_list;
	int i;

	rxchan = fh_spi->dwc.dma_para.rx.chan;
	p_sca_list = &fh_spi->dwc.dma_para.rx.sgl[0];
	for(i = 0; i < fh_spi->dwc.dma_para.rx.actual_sgl_size; i++, p_sca_list++){
		dma_unmap_single(rxchan->dev->device.parent,
		p_sca_list->dma_address,
		fh_spi->dwc.dma_para.rx.sgl_data_size[i], DMA_FROM_DEVICE);
	}
}

static void fh_spi_rx_only_dma_done(void *arg)
{
	struct fh_spi_controller *fh_spi = (struct fh_spi_controller *) arg;
	complete(&(fh_spi->dwc.dma_para.done));

}

static int dma_pump_rx_only_data(struct fh_spi_controller *fh_spi)
{

	struct spi_transfer *t;
	struct dma_chan *rxchan;
	int ret;
	unsigned int temp_size;
	unsigned int reg_width;
	struct dma_async_tx_descriptor *p_desc;
	u32 raw_isr;

	t = fh_spi->active_transfer;
	rxchan = fh_spi->dwc.dma_para.rx.chan;
	
	init_completion(&fh_spi->dwc.dma_para.done);

	temp_size = min(fh_spi->dwc.tx_len, (u32)SPI_RX_ONLY_ONE_TIME_SIZE);
	reg_width = rx_only_fix_data_width(fh_spi, temp_size);

#ifdef CONFIG_SPI_USE_MULTI_WIRE
	if (reg_width > 8)
		fh_spi->dwc.multi_para.swap_data_width(fh_spi, reg_width);
#endif

	ret = dma_set_rx_para(fh_spi, fh_spi_rx_only_dma_done, temp_size, reg_width);
	if (ret != 0)
		BUG_ON(1);
	p_desc = fh_spi->dwc.dma_para.rx.desc;
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetDmaRxDataLevel(&fh_spi->dwc, 7);

	Spi_ContinueReadNum(&fh_spi->dwc, temp_size / (reg_width / 8));
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);

	p_desc->tx_submit(p_desc);
	Spi_SetDmaControlEnable(&fh_spi->dwc, SPI_DMA_RX_POS);
	Spi_WriteData(&fh_spi->dwc, 0xffffffff);

	if (!(wait_for_completion_timeout(&fh_spi->dwc.dma_para.done, 20*HZ))) {
		raw_isr = Spi_RawIsrstatus(&fh_spi->dwc);
		pr_err("%s %d time out..spi raw status is %x\n",
		__func__, __LINE__, raw_isr);
		BUG_ON(1);
		return -1;
	}
	if (t->rx_buf != NULL)
		unmap_dma_rx_sg(fh_spi);

	Spi_SetDmaControlDisable(&fh_spi->dwc, SPI_DMA_RX_POS);
	/*here need to cal the data has been transfered ...
	if we need to start a new transfer
	then maybe i could recall the rx only func  :)*/
	fh_spi->dwc.rx_len += temp_size;
	fh_spi->dwc.tx_len -= temp_size;
	fh_spi->active_transfer->rx_buf += temp_size;

#ifdef CONFIG_SPI_USE_MULTI_WIRE
		if (reg_width > 8)
			fh_spi->dwc.multi_para.swap_data_width(fh_spi, 8);
#endif
	
	return 0;
}


static int dma_pump_data_read(struct fh_spi_controller *fh_spi){
	BUG_ON(fh_spi->dwc.transfer_mode != SPI_ONLY_RX_MODE);
	
	do {
		dma_pump_rx_only_data(fh_spi);
	} while (fh_spi->dwc.tx_len != 0);

	return 0;
}


void fh_spi_dma_init(struct fh_spi_controller *fh_spi){
	int filter_no;
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	if (fh_spi->dwc.board_info->dma_transfer_enable != SPI_TRANSFER_USE_DMA)
		return;

	filter_no = fh_spi->dwc.board_info->rx_dma_channel;
	fh_spi->dwc.dma_para.rx.chan = dma_request_channel(mask,
					fh_spi_dma_chan_filter, &filter_no);

	if (!fh_spi->dwc.dma_para.rx.chan) {
		dev_err(&fh_spi->p_dev->dev,
						"spi%d request dma channel error....\n",
						fh_spi->dwc.id);
		fh_spi->dwc.board_info->dma_transfer_enable = 0;
		return;
	}
	fh_spi->dwc.dma_para.rx.hs_no = fh_spi->dwc.board_info->rx_handshake_num;
	fh_spi->dwc.dma_para.dma_read = dma_pump_data_read;
	fh_spi->dwc.dma_para.dma_write = NULL;
	fh_spi->dwc.dma_para.dma_write_read = NULL;

}

#include "fh_axi_dma_adapt.h"

static inline struct fh_axi_dma_adapt *to_fh_axi_dma(struct dma_device *ddev);
static inline struct fh_axi_dma_channel_adapt *to_fh_axi_dma_chan(struct dma_chan *chan);
static dma_cookie_t fh_axi_dma_adapt_submit(struct dma_async_tx_descriptor *tx);
static int adapt_alloc_chan_resources(struct dma_chan *chan);
static void adapt_free_chan_resources(struct dma_chan *chan);
static FH_UINT32 cal_data_width(dma_addr_t dest, dma_addr_t src, size_t len);
static void adapt_prep_callback(void *p);
static struct dma_async_tx_descriptor *adapt_prep_dma_memcpy(struct dma_chan *chan,
dma_addr_t dest, dma_addr_t src, size_t len, unsigned long flags);
static FH_UINT32 width_from_core_to_driver(FH_UINT32 core_width);
static FH_UINT32 burst_from_core_to_driver(FH_UINT32 core_width);

static struct dma_async_tx_descriptor *
adapt_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context);

static int adapt_device_config(struct dma_chan *chan, struct dma_slave_config *config);
static int adapt_device_pause(struct dma_chan *chan);
static int adapt_device_resume(struct dma_chan *chan);

static enum dma_status
adapt_tx_status(struct dma_chan *chan,
dma_cookie_t cookie, struct dma_tx_state *txstate);

static irqreturn_t adapt_dma_interrupt(int irq, void *dev_id);
static void adapt_axi_dma_tasklet(unsigned long data);
static struct fh_axi_dma * adapt_to_driver_handle(struct fh_axi_dma_adapt *p_adapt);
static void adapt_device_issue_pending(struct dma_chan *chan);
static int adapt_device_terminate_all(struct dma_chan *chan);
static int fh_axi_dma_adapt_probe(struct platform_device *pdev);
static int fh_axi_dma_adapt_remove(struct platform_device *pdev);
static int __init fh_axi_dma_adapt_init(void);
static void __exit fh_axi_dma_adapt_exit(void);

static inline struct fh_axi_dma_adapt *to_fh_axi_dma(struct dma_device *ddev)
{
	return container_of(ddev, struct fh_axi_dma_adapt, dma);
}

static inline struct fh_axi_dma_channel_adapt *to_fh_axi_dma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct fh_axi_dma_channel_adapt, core_chan);
}

static struct fh_axi_dma * adapt_to_driver_handle(struct fh_axi_dma_adapt *p_adapt)
{
	return p_adapt->driver_pri;
}

static void fh_axi_adapt_isr_prep_func_callback(void *p)
{
	dma_cookie_complete((struct dma_async_tx_descriptor *)p);
}

static dma_cookie_t fh_axi_dma_adapt_submit(struct dma_async_tx_descriptor *tx)
{
	dma_cookie_t		cookie;
	struct axi_dma_ops *p_ops;
	struct dma_transfer *p_trans;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct dma_chan *chan = tx->chan;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	cookie = dma_cookie_assign(tx);
	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	p_trans->isr_prepare_callback = fh_axi_adapt_isr_prep_func_callback;
	p_trans->isr_prepare_para = (void *)tx;
	p_trans->complete_callback = tx->callback;
	p_trans->complete_para = tx->callback_param;
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_SINGLE_TRANSFER, p_trans);
	return cookie;
}


static int adapt_alloc_chan_resources(struct dma_chan *chan)
{
	
	struct axi_dma_ops *p_ops;
	struct dma_transfer *p_trans;
	int i;
	struct dma_async_tx_descriptor *p_async;
	FH_UINT32 desc_no;
	FH_UINT32 each_desc_size;
	FH_UINT32 driver_desc_phy_base;
	FH_UINT32 desc_cap;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	dma_cookie_init(chan);
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_trans->dma_number = fhd->dma.dev_id;
	p_trans->channel_number = chan->chan_id;
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_REQUEST_CHANNEL, p_trans);
	p_ops->axi_dma_get_desc_para(fhd->driver_pri, p_trans, &desc_no, &driver_desc_phy_base, &each_desc_size, &desc_cap);
	fhc_adapt->ch_adapt_desc_base = (struct dma_async_tx_descriptor *)devm_kzalloc(&fhd->pdev->dev,
	sizeof(struct dma_async_tx_descriptor) * desc_no, GFP_KERNEL);
	for(i = 0, p_async = fhc_adapt->ch_adapt_desc_base; i < desc_no; i++, p_async++){
		dma_async_tx_descriptor_init(p_async, chan);
		p_async->tx_submit = fh_axi_dma_adapt_submit;
		p_async->flags = DMA_CTRL_ACK;
		p_async->phys = driver_desc_phy_base + (i * each_desc_size);
	}

	return desc_no;
}


static void adapt_free_chan_resources(struct dma_chan *chan)
{
	struct axi_dma_ops *p_ops;
	struct dma_transfer *p_trans;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_RELEASE_CHANNEL, p_trans);
	if(fhc_adapt->ch_adapt_desc_base)
		devm_kfree(&fhd->pdev->dev, fhc_adapt->ch_adapt_desc_base);
}

static FH_UINT32 cal_data_width(dma_addr_t dest, dma_addr_t src, size_t len)
{
	FH_UINT32 data_width = 0;

	if (!((src % 4) || (dest % 4)  || (len % 4)))
		data_width = DW_DMA_SLAVE_WIDTH_32BIT;
	else if (!((src % 2) || (dest % 2)  || (len % 2)))
		data_width = DW_DMA_SLAVE_WIDTH_16BIT;
	else
		data_width = DW_DMA_SLAVE_WIDTH_8BIT;

	return data_width;
}

static void adapt_prep_callback(void *p)
{
	struct dma_chan *chan = (struct dma_chan *)p;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	devm_kfree(&fhd->pdev->dev, fhc_adapt->desc_head);
}


static struct dma_async_tx_descriptor *
adapt_prep_dma_memcpy(struct dma_chan *chan,
dma_addr_t dest, dma_addr_t src,
size_t len, unsigned long flags)
{
	struct axi_dma_ops *p_ops;
	struct dma_transfer *p_trans;
	int i;
	FH_UINT32 desc_no;
	FH_UINT32 each_desc_size;
	FH_UINT32 driver_desc_phy_base;
	FH_UINT32 width;
	FH_UINT32 ot_len;
	FH_UINT32 need_trans_size;
	FH_UINT32 desc_cap;
	struct dma_transfer_desc *p_desc;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_trans = &fhc_adapt->trans;
	width = cal_data_width(dest, src, len);

	p_ops->axi_dma_get_desc_para(fhd->driver_pri, p_trans, &desc_no, &driver_desc_phy_base, &each_desc_size, &desc_cap);

	//cal size with width
	len = len / ( 1 << width );
	need_trans_size = len / desc_cap;
	if(len % desc_cap)
		need_trans_size++;

	fhc_adapt->desc_size = need_trans_size;
	fhc_adapt->desc_head = devm_kzalloc(&fhd->pdev->dev,
	sizeof(struct dma_transfer_desc) * need_trans_size, GFP_KERNEL);

	BUG_ON(!fhc_adapt->desc_head);
	p_desc = fhc_adapt->desc_head;

	p_trans->fc_mode = DMA_M2M;
	i = 0;
	while(len != 0){
		ot_len = min_t(unsigned int, desc_cap, len);
		p_desc->src_add = src + i * desc_cap * (1 << width);
		p_desc->src_width = width;
		p_desc->src_msize = DW_DMA_SLAVE_MSIZE_32;
		p_desc->src_inc_mode = DW_DMA_SLAVE_INC;

		p_desc->dst_add = dest + i * desc_cap * (1 << width);
		p_desc->dst_width = width;
		p_desc->dst_msize = DW_DMA_SLAVE_MSIZE_32;
		p_desc->dst_inc_mode = DW_DMA_SLAVE_INC;
		p_desc->size = ot_len;
		p_desc++;
		i++;
		len -= ot_len;
	}
	p_trans->p_desc = fhc_adapt->desc_head;
	p_trans->desc_size = need_trans_size;

	p_trans->prepare_callback = adapt_prep_callback;
	p_trans->prepare_para = chan;

	return fhc_adapt->ch_adapt_desc_base;
}


static FH_UINT32 width_from_core_to_driver(FH_UINT32 core_width)
{
	FH_UINT32 ret;

	switch(core_width){
		case DMA_SLAVE_BUSWIDTH_1_BYTE :
		ret = DW_DMA_SLAVE_WIDTH_8BIT;
		break;

		case DMA_SLAVE_BUSWIDTH_2_BYTES :
		ret = DW_DMA_SLAVE_WIDTH_16BIT;
		break;

		case DMA_SLAVE_BUSWIDTH_4_BYTES :
		ret = DW_DMA_SLAVE_WIDTH_32BIT;
		break;

		default :
		BUG_ON(1);
	}
	return ret;
}


static FH_UINT32 burst_from_core_to_driver(FH_UINT32 core_width)
{
	FH_UINT32 ret;
	switch(core_width){
		case 1 :
		ret = DW_DMA_SLAVE_MSIZE_1;
		break;

		case 4 :
		ret = DW_DMA_SLAVE_MSIZE_4;
		break;

		case 8 :
		ret = DW_DMA_SLAVE_MSIZE_8;
		break;

		case 16 :
		ret = DW_DMA_SLAVE_MSIZE_16;
		break;

		case 32 :
		ret = DW_DMA_SLAVE_MSIZE_32;
		break;

		default :
		BUG_ON(1);
	}
	return ret;
}


static struct dma_async_tx_descriptor *
adapt_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{

	struct dma_transfer *p_trans;
	int i;
	struct scatterlist	*sg;
	FH_UINT32 dev_width, dev_add;
	FH_UINT32 mem_width, mem_add;
	FH_UINT32 ot_len;
	struct fh_dma_extra *ext_para = (struct fh_dma_extra *)context;
	struct dma_slave_config	*sconfig;
	struct dma_transfer_desc *p_desc;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	fhc_adapt->desc_size = sg_len;

	fhc_adapt->desc_head = devm_kzalloc(&fhd->pdev->dev,
	sizeof(struct dma_transfer_desc) * sg_len, GFP_KERNEL);
	BUG_ON(!fhc_adapt->desc_head);
	p_desc = fhc_adapt->desc_head;
	sconfig = &fhc_adapt->sconfig;

	if (ext_para)
		memcpy(&fhc_adapt->ext_para, ext_para, sizeof(struct fh_dma_extra));


	switch (direction) {
		case DMA_MEM_TO_DEV:
			p_trans->fc_mode = DMA_M2P;
			p_trans->dst_per = sconfig->slave_id;
			dev_width = width_from_core_to_driver(sconfig->dst_addr_width);
			dev_add = sconfig->dst_addr;
			mem_width = width_from_core_to_driver(sconfig->src_addr_width);


			for_each_sg(sgl, sg, sg_len, i) {
				mem_add = sg_dma_address(sg);
				ot_len = sg_dma_len(sg);
				p_desc->src_add = mem_add;
				p_desc->src_width = mem_width;
				ot_len = ot_len >> p_desc->src_width;
				p_desc->src_msize = burst_from_core_to_driver(sconfig->src_maxburst);
				p_desc->src_inc_mode = DW_DMA_SLAVE_INC;
				p_desc->dst_add = dev_add;
				p_desc->dst_width = dev_width;
				p_desc->dst_msize = burst_from_core_to_driver(sconfig->dst_maxburst);
				p_desc->dst_inc_mode = DW_DMA_SLAVE_FIX;
				if (ext_para){
					p_desc->dst_inc_mode = ext_para->dinc;
				}
				p_desc->size = ot_len;
				p_desc++;
			}
		break;

		case DMA_DEV_TO_MEM:
			p_trans->fc_mode = DMA_P2M;
			p_trans->src_per = sconfig->slave_id;
			dev_width = width_from_core_to_driver(sconfig->src_addr_width);
			dev_add = sconfig->src_addr;
			mem_width = width_from_core_to_driver(sconfig->dst_addr_width);

			for_each_sg(sgl, sg, sg_len, i) {
				mem_add = sg_dma_address(sg);
				ot_len = sg_dma_len(sg);
				
				p_desc->src_add = dev_add;
				p_desc->src_width = dev_width;
				ot_len = ot_len >> p_desc->src_width;
				p_desc->src_msize = burst_from_core_to_driver(sconfig->src_maxburst);
				p_desc->src_inc_mode = DW_DMA_SLAVE_FIX;
				if (ext_para)
					p_desc->src_inc_mode = ext_para->sinc;
				p_desc->dst_add = mem_add;
				p_desc->dst_width = mem_width;
				p_desc->dst_msize = burst_from_core_to_driver(sconfig->dst_maxburst);
				p_desc->dst_inc_mode = DW_DMA_SLAVE_INC;
				p_desc->size = ot_len;

				p_desc++;
			}

		break;

		default:
			return NULL;
	}
	p_trans->p_desc = fhc_adapt->desc_head;
	p_trans->desc_size = fhc_adapt->desc_size;
	p_trans->prepare_callback = adapt_prep_callback;
	p_trans->prepare_para = chan;

	return fhc_adapt->ch_adapt_desc_base;
}


static int adapt_device_config(struct dma_chan *chan, struct dma_slave_config *config)
{
	struct fh_axi_dma_channel_adapt *fhc_adapt;

	fhc_adapt = to_fh_axi_dma_chan(chan);
	memcpy(&fhc_adapt->sconfig, config, sizeof(struct dma_slave_config));
	return 0;
}


static int adapt_device_pause(struct dma_chan *chan)
{
	struct axi_dma_ops *p_ops;
	struct dma_transfer *p_trans;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;

	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_PAUSE, p_trans);


	return 0;
}

static int adapt_device_resume(struct dma_chan *chan)
{
	struct axi_dma_ops *p_ops;
	struct dma_transfer *p_trans;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;

	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_RESUME, p_trans);


	return 0;
}



static enum dma_status
adapt_tx_status(struct dma_chan *chan,
dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	enum dma_status	ret;

	ret = dma_cookie_status(chan, cookie, txstate);
	return ret;
}


static irqreturn_t adapt_dma_interrupt(int irq, void *dev_id)
{
	struct fh_axi_dma_adapt *fh_axi_adapt_obj;
	struct axi_dma_ops *p_ops;
	struct fh_axi_dma *p_driver_handle;

	fh_axi_adapt_obj = (struct fh_axi_dma_adapt *)dev_id;
	p_driver_handle = adapt_to_driver_handle(fh_axi_adapt_obj);
	p_ops = get_fh_axi_dma_ops(p_driver_handle);
	// close dma isr .tasklet will reopen
	p_ops->axi_dma_isr_enable_set(p_driver_handle, DMA_COMMON_ISR_CLOSE);
	tasklet_schedule(&fh_axi_adapt_obj->tasklet);

	return IRQ_HANDLED;
}


static void adapt_axi_dma_tasklet(unsigned long data)
{
	struct axi_dma_ops *p_ops;
	struct fh_axi_dma *p_driver_handle;
	struct fh_axi_dma_adapt *p_adapt = (struct fh_axi_dma_adapt *)data;

	p_driver_handle = adapt_to_driver_handle(p_adapt);
	p_ops = get_fh_axi_dma_ops(p_driver_handle);
	p_ops->axi_dma_isr_process(p_driver_handle);
	p_ops->axi_dma_isr_enable_set(p_driver_handle, DMA_COMMON_ISR_OPEN);
}

/****
 * 
 * adapt cyclic
 * 
****/

inline dma_addr_t fh_dma_get_src_addr(struct dma_chan *chan)
{
	struct dma_transfer *p_trans;
	struct axi_dma_ops * p_ops;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	return (dma_addr_t)p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_GET_SAR, p_trans);

}
EXPORT_SYMBOL(fh_dma_get_src_addr);

inline dma_addr_t fh_dma_get_dst_addr(struct dma_chan *chan)
{
	struct dma_transfer *p_trans;
	struct axi_dma_ops * p_ops;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	return (dma_addr_t)p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_GET_DAR, p_trans);
}
EXPORT_SYMBOL(fh_dma_get_dst_addr);


int fh_dma_cyclic_start(struct dma_chan *chan)
{
	struct dma_transfer *p_trans;
	struct axi_dma_ops * p_ops;
	struct fh_cyclic_desc *p_cyclic;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_cyclic = &fhc_adapt->cyclic;
	p_trans = &fhc_adapt->trans;

	p_trans->complete_callback = p_cyclic->period_callback;
	p_trans->complete_para = p_cyclic->period_callback_param;
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_CYCLIC_START, p_trans);

	return 0;
}
EXPORT_SYMBOL(fh_dma_cyclic_start);


void fh_dma_cyclic_stop(struct dma_chan *chan)
{
	struct dma_transfer *p_trans;
	struct axi_dma_ops * p_ops;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_CYCLIC_STOP, p_trans);
}
EXPORT_SYMBOL(fh_dma_cyclic_stop);


struct fh_cyclic_desc *fh_dma_cyclic_prep(struct dma_chan *chan,
		dma_addr_t buf_addr, size_t buf_len, size_t period_len,
		enum dma_transfer_direction direction)
{
	struct dma_transfer *p_trans;
	int i;
	FH_UINT32 ot_len;
	FH_UINT32 periods;
	struct axi_dma_ops * p_ops;
	struct fh_dma_slave *fhs = chan->private;
	struct dma_transfer_desc *p_desc;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);

	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;
	periods = buf_len / period_len;
	fhc_adapt->desc_size = periods;
	fhc_adapt->desc_head = devm_kzalloc(&fhd->pdev->dev,
	sizeof(struct dma_transfer_desc) * periods, GFP_KERNEL);

	BUG_ON(!fhc_adapt->desc_head);
	p_desc = fhc_adapt->desc_head;

	switch (direction) {
		case DMA_MEM_TO_DEV:
			p_trans->fc_mode = DMA_M2P;
			p_trans->dst_per = fhs->cfg_hi;
			for (i = 0; i < periods; i++){
				p_desc->src_add = buf_addr + (period_len * i);
				p_desc->src_width = DW_DMA_SLAVE_WIDTH_32BIT;
				ot_len = period_len >> p_desc->src_width;
				p_desc->src_msize = fhs->src_msize;
				p_desc->src_inc_mode = DW_DMA_SLAVE_INC;

				p_desc->dst_add = fhs->tx_reg;
				p_desc->dst_width = fhs->reg_width;
				p_desc->dst_msize = fhs->dst_msize;
				p_desc->dst_inc_mode = DW_DMA_SLAVE_FIX;
				p_desc->size = ot_len;
				p_desc++;
			}
		break;

		case DMA_DEV_TO_MEM:

			p_trans->fc_mode = DMA_P2M;
			p_trans->src_per = fhs->cfg_hi;
			
			for (i = 0; i < periods; i++){
				p_desc->src_add = fhs->rx_reg;
				p_desc->src_width = fhs->reg_width;
				ot_len = period_len >> p_desc->src_width;
				p_desc->src_msize = fhs->src_msize;
				p_desc->src_inc_mode = DW_DMA_SLAVE_FIX;

				p_desc->dst_add = buf_addr + (period_len * i);
				p_desc->dst_width = DW_DMA_SLAVE_WIDTH_32BIT;
				p_desc->dst_msize = fhs->dst_msize;
				p_desc->dst_inc_mode = DW_DMA_SLAVE_INC;
				p_desc->size = ot_len;
				p_desc++;
			}
		break;
		default:
			return NULL;
	}

	p_trans->p_desc = fhc_adapt->desc_head;
	p_trans->desc_size = fhc_adapt->desc_size;
	p_trans->prepare_callback = adapt_prep_callback;
	p_trans->cyclic_periods = periods;
	p_trans->prepare_para = chan;
	p_ops = get_fh_axi_dma_ops(fhd->driver_pri);
	p_ops->axi_dma_control(fhd->driver_pri, AXI_DMA_CTRL_DMA_CYCLIC_PREPARE, p_trans);

	return &fhc_adapt->cyclic;

}
EXPORT_SYMBOL(fh_dma_cyclic_prep);



void fh_dma_cyclic_free(struct dma_chan *chan)
{

	struct dma_transfer *p_trans;
	struct fh_axi_dma_channel_adapt *fhc_adapt;
	struct fh_axi_dma_adapt *fhd = to_fh_axi_dma(chan->device);
	fhc_adapt = to_fh_axi_dma_chan(chan);
	p_trans = &fhc_adapt->trans;

	if(fhc_adapt->ch_adapt_desc_base)
		devm_kfree(&fhd->pdev->dev, fhc_adapt->ch_adapt_desc_base);
}
EXPORT_SYMBOL(fh_dma_cyclic_free);


static void adapt_device_issue_pending(struct dma_chan *chan)
{

}

static int adapt_device_terminate_all(struct dma_chan *chan)
{
	return 0;
}

static void adapt_plat_info_get(struct fh_axi_dma_platform_data *p_info,
struct platform_device *pdev){
#ifndef CONFIG_OF
	/* just cpy plat data.*/
	memcpy((void *)p_info, (void *)pdev->dev.platform_data,
	sizeof(struct fh_axi_dma_platform_data));
#endif
}


static int fh_axi_dma_adapt_probe(struct platform_device *pdev)
{
	struct fh_axi_dma_platform_data p_axi_plat;
	struct dma_transfer trans;
	size_t			size;
	void __iomem	*regs;
	int			irq;
	int			err;
	int			i;
	int channel_no;
	struct clk	*clk;
	struct resource *res;
	struct fh_axi_dma_adapt *fh_axi_adapt_obj;
	struct axi_dma_platform_data driver_plat_data;
	struct axi_dma_ops * p_ops;
	struct device_node *np = pdev->dev.of_node;

	if (np && !IS_ERR(np)) {
		regs = of_iomap(np, 0);
		if (!regs) {
			err = -ENOMEM;
			goto err_release_r;
		}
		irq = irq_of_parse_and_map(np, 0);
		clk = of_clk_get(np, 0);
	} else {
		adapt_plat_info_get(&p_axi_plat, pdev);
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			dev_err(&pdev->dev, "can't fetch device resource info\n");
			goto err_release_r;
		}
		regs = ioremap(res->start, resource_size(res));
		if (regs == NULL) {
			dev_err(&pdev->dev, "ioremap resource error\n");
			goto err_release_r;
		}
		irq = platform_get_irq(pdev, 0);
		irq = irq_create_mapping(NULL, irq);
		clk = clk_get(&pdev->dev, p_axi_plat.clk_name);
		if (IS_ERR(clk)) {
			err = -EINVAL;
			goto err_release_r;
		}
	}
	clk_prepare_enable(clk);
	channel_no = cal_axi_dma_channel(regs);
	size = sizeof(struct fh_axi_dma_adapt) + channel_no * sizeof(struct fh_axi_dma_channel_adapt);
	fh_axi_adapt_obj = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);

	if (!pdev->dev.dma_mask) {
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	}
	fh_axi_adapt_obj->pdev = pdev;
	fh_axi_adapt_obj->clk = clk;

	INIT_LIST_HEAD(&fh_axi_adapt_obj->dma.channels);
	for (i = 0; i < channel_no; i++) {
		struct dma_chan *dma_channel;
		spin_lock_init(&fh_axi_adapt_obj->chan[i].lock);
		dma_channel = &fh_axi_adapt_obj->chan[i].core_chan;
		dma_channel->device = &fh_axi_adapt_obj->dma;
		dma_cookie_init(dma_channel);
		list_add_tail(&dma_channel->device_node,
		&fh_axi_adapt_obj->dma.channels);
	}
	
	driver_plat_data.id = pdev->id;
	driver_plat_data.name = "axi_dma_driver";
	driver_plat_data.base = (unsigned int)regs;
	driver_plat_data.kernel_pri = (void *)fh_axi_adapt_obj->dma.dev;
	driver_plat_data.adapt_pri = fh_axi_adapt_obj;


	fh_axi_adapt_obj->driver_pri = fh_axi_dma_probe(&driver_plat_data);
	if(!fh_axi_adapt_obj->driver_pri)
		BUG_ON(1);

	platform_set_drvdata(pdev, fh_axi_adapt_obj);
	fh_axi_adapt_obj->dma.dev = &pdev->dev;
	fh_axi_adapt_obj->dma.device_alloc_chan_resources = adapt_alloc_chan_resources;
	fh_axi_adapt_obj->dma.device_free_chan_resources = adapt_free_chan_resources;
	fh_axi_adapt_obj->dma.device_prep_dma_memcpy = adapt_prep_dma_memcpy;
	fh_axi_adapt_obj->dma.device_prep_slave_sg = adapt_prep_slave_sg;
	fh_axi_adapt_obj->dma.device_config = adapt_device_config;
	fh_axi_adapt_obj->dma.device_pause = adapt_device_pause;
	fh_axi_adapt_obj->dma.device_resume = adapt_device_resume;
	fh_axi_adapt_obj->dma.device_terminate_all = adapt_device_terminate_all;
	fh_axi_adapt_obj->dma.device_tx_status = adapt_tx_status;
	fh_axi_adapt_obj->dma.device_issue_pending = adapt_device_issue_pending;

	tasklet_init(&fh_axi_adapt_obj->tasklet, adapt_axi_dma_tasklet, (unsigned long)fh_axi_adapt_obj);
	err = devm_request_irq(&pdev->dev, irq, adapt_dma_interrupt, 0,
			       "fh-axi-dmac", fh_axi_adapt_obj);
	BUG_ON(err);

	dma_cap_set(DMA_MEMCPY, fh_axi_adapt_obj->dma.cap_mask);
	dma_cap_set(DMA_SLAVE, fh_axi_adapt_obj->dma.cap_mask);

	trans.dma_number = pdev->id;
	p_ops = get_fh_axi_dma_ops(fh_axi_adapt_obj->driver_pri);
	p_ops->axi_dma_control(fh_axi_adapt_obj->driver_pri, AXI_DMA_CTRL_DMA_OPEN, &trans);
	err = dma_async_device_register(&fh_axi_adapt_obj->dma);
	if (err)
		pr_err("dma register failed, ret %d\n", err);

	dev_info(&pdev->dev, "FH DMA Controller, %d channels\n", channel_no);
	return 0;

err_release_r:
	return err;
}

static int fh_axi_dma_adapt_remove(struct platform_device *pdev)
{

	struct fh_axi_dma_adapt *fh_axi_adapt_obj;
	struct dma_transfer trans;
	struct axi_dma_ops * p_ops;

	//dma off 
	trans.dma_number = pdev->id;
	fh_axi_adapt_obj = platform_get_drvdata(pdev);
	p_ops = get_fh_axi_dma_ops(fh_axi_adapt_obj->driver_pri);
	p_ops->axi_dma_control(fh_axi_adapt_obj->driver_pri, AXI_DMA_CTRL_DMA_CLOSE, &trans);
	//kill tasklet
	tasklet_kill(&fh_axi_adapt_obj->tasklet);
	//unregister dma
	dma_async_device_unregister(&fh_axi_adapt_obj->dma);
	devm_kfree(&pdev->dev, fh_axi_adapt_obj);
	return 0;
}

void fh_axi_dma_adapt_shutdown(struct platform_device *pdev){
	pr_err("%s : %d.....\n",__func__,__LINE__);
}




static const struct of_device_id fh_axi_dma_adapt_of_id_table[] = {
	{ .compatible = "fh,fh-axi-dmac" },
	{}
};

MODULE_DEVICE_TABLE(of, fh_axi_dma_adapt_of_id_table);

static struct platform_driver fh_dma_axi_adapt_driver = {
	.probe		= fh_axi_dma_adapt_probe,
	.remove		= fh_axi_dma_adapt_remove,
	.shutdown	= fh_axi_dma_adapt_shutdown,
	.driver = {
		.name	= "fh_axi_dmac",
		.of_match_table = fh_axi_dma_adapt_of_id_table,
	},
};

static int __init fh_axi_dma_adapt_init(void)
{
	return platform_driver_register(&fh_dma_axi_adapt_driver);
}
subsys_initcall(fh_axi_dma_adapt_init);

static void __exit fh_axi_dma_adapt_exit(void)
{
	platform_driver_unregister(&fh_dma_axi_adapt_driver);
}
module_exit(fh_axi_dma_adapt_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("fullhan AXI-DMA devive driver");



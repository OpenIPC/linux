/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include "fh_spi_common.h"
#include <linux/kfifo.h>
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define KFIFO_SIZE				2048
#define SPI_SLAVE_MAX_FIFO_SIZE	256
#define SLAVE_SET_PHASE			1
#define SLAVE_SET_POLARITY		(SLAVE_SET_PHASE + 1)
#define SLAVE_INIT_RX_FIFO		(SLAVE_SET_POLARITY + 1)
#define SLAVE_INIT_TX_FIFO		(SLAVE_INIT_RX_FIFO + 1)
#define SLAVE_GET_ERROR_STATUS	(SLAVE_INIT_TX_FIFO + 1)
#define MAX_SPI_SLAVES			(8)
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type
 *  that only be used in this file  here
 ***************************************************************************/
struct fh_spi_slave_controller {
	spinlock_t lock;
	struct platform_device *p_dev;
	struct fh_spi dwc;
	u32 cur_rx_len;
	u32 cur_tx_len;
	/* dev interface */
	int major;
	struct class *psClass;
	struct device *psDev;

	/* kfifo interface */
	struct kfifo kfifo_in;
	struct kfifo kfifo_out;
};

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/
static int fh_spi_slave_init_hw(struct fh_spi_slave_controller *fh_spi_slave,
                struct fh_spi_platform_data *board_info);
static u32 Spi_SetFrameFormat(struct fh_spi *dw, spi_format_e format);
static u32 Spi_SetTransferMode(struct fh_spi *dw, spi_transfer_mode_e mode);
static u32 Spi_SetPolarity(struct fh_spi *dw, spi_polarity_e polarity);
static u32 Spi_SetPhase(struct fh_spi *dw, spi_phase_e phase);
static u32 Spi_DisableIrq(struct fh_spi *dw, u32 irq);
static u32 Spi_EnableIrq(struct fh_spi *dw, u32 irq);
static u32 Spi_SetSlaveMode(struct fh_spi *dw, spi_slave_mode_e format);
/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/
static struct fh_spi_slave_controller *priv_array[MAX_SPI_SLAVES] = { NULL,
NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/*****************************************************************************

 *  static fun;
 *****************************************************************************/
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

static u32 Spi_SetPhase(struct fh_spi *dw, spi_phase_e phase)
{
	u32 data;

	data = dw_readl(dw, ctrl0);
	data &= ~(u32) SPI_PHASE_RANGE;
	data |= phase;
	dw_writel(dw, ctrl0, data);

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

static u32 Spi_EnableIrq(struct fh_spi *dw, u32 irq)
{
	u32 data = 0;

	data = dw_readl(dw, imr);
	data |= irq;
	dw_writel(dw, imr, data);

	return CONFIG_OK;
}

static u32 Spi_SetTxlevlel(struct fh_spi *dw, u32 level)
{
	dw_writel(dw, txfltr, level);
	return CONFIG_OK;
}

static u32 Spi_ReadTxfifolevel(struct fh_spi *dw)
{
	return dw_readl(dw, txflr);
}

static u32 Spi_ReadRxfifolevel(struct fh_spi *dw)
{
	return dw_readl(dw, rxflr);
}

static u32 Spi_ReadRxlevlel(struct fh_spi *dw)
{
	return dw_readl(dw, rxfltr);
}

static u32 Spi_SetSlaveMode(struct fh_spi *dw, spi_slave_mode_e format)
{
	u32 data = 0;
	data = dw_readl(dw, ctrl0);
	data &= ~(u32) SPI_SLAVE_MODE_RANGE;
	data |= format;
	dw_writel(dw, ctrl0, data);

	return CONFIG_OK;
}

static u16 Spi_ReadData(struct fh_spi *dw)
{
	return dw_readw(dw, dr);
}

static u32 Spi_Isrstatus(struct fh_spi *dw)
{
	u32 data = dw_readl(dw, isr);
	return data;
}

static int spi_slave_open(struct inode *inode, struct file *filp)
{
	int i, ret = 0;
	struct fh_spi_slave_controller *fh_spi_slave;

	for (i = 0; i < MAX_SPI_SLAVES; i++) {
		if (priv_array[i] && MKDEV(priv_array[i]->major, 0) == inode->i_rdev) {
			filp->private_data = priv_array[i];
			break;
		}
	}

	if (i == MAX_SPI_SLAVES)
		return -ENXIO;

	fh_spi_slave = priv_array[i];
	kfifo_reset(&fh_spi_slave->kfifo_in);
	kfifo_reset(&fh_spi_slave->kfifo_out);
	return ret;
}

static ssize_t spi_slave_read(struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{
	int ret;
	unsigned int copied;
	struct fh_spi_slave_controller *fh_spi_slave;
	fh_spi_slave = (struct fh_spi_slave_controller *)filp->private_data;

	if (kfifo_is_empty(&fh_spi_slave->kfifo_out))
		return -EFAULT;

	ret = kfifo_to_user(&fh_spi_slave->kfifo_out, buf, count, &copied);
	if (ret == 0) {
		fh_spi_slave->cur_rx_len = copied;
		return copied;
	}else
		return ret;
}

static void spi_slave_isr_tx_data(struct fh_spi_slave_controller *fh_spi_slave)
{
	Spi_SetTxlevlel(&fh_spi_slave->dwc, fh_spi_slave->dwc.fifo_len - 5);
	Spi_EnableIrq(&fh_spi_slave->dwc, SPI_IRQ_TXEIM);
}

static ssize_t spi_slave_write(struct file *filp, const char __user *buf,
                size_t count, loff_t *f_pos)
{
	int ret;
	unsigned int copied;
	struct fh_spi_slave_controller *fh_spi_slave;
	fh_spi_slave = (struct fh_spi_slave_controller *) filp->private_data;

	if (kfifo_is_full(&fh_spi_slave->kfifo_in))
		return -EFAULT;

	ret = kfifo_from_user(&fh_spi_slave->kfifo_in, buf, count, &copied);
	if (ret == 0) {
		fh_spi_slave->cur_tx_len = copied;
		spi_slave_isr_tx_data(fh_spi_slave);
		return copied;
	} else
		return ret;
}

long spi_slave_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = -ENOIOCTLCMD;

	switch (cmd) {
	case SLAVE_SET_PHASE:
		break;
	case SLAVE_SET_POLARITY:
		break;
	case SLAVE_INIT_RX_FIFO:
		break;
	case SLAVE_INIT_TX_FIFO:
		break;
	case SLAVE_GET_ERROR_STATUS:
		break;
	default:
		break;
	}

	return err;
}

static int spi_slave_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	return ret;
}

static struct file_operations spi_slave_fops = {
	.open = spi_slave_open,
	.read = spi_slave_read,
	.write = spi_slave_write,
	.unlocked_ioctl = spi_slave_ioctl,
	.release = spi_slave_release,
};

static inline u32 tx_max(struct fh_spi_slave_controller *fh_spi_slave)
{
	u32 hw_tx_level;

	hw_tx_level = Spi_ReadTxfifolevel(&fh_spi_slave->dwc);
	hw_tx_level = fh_spi_slave->dwc.fifo_len - hw_tx_level;

	return hw_tx_level;
}

static inline u32 rx_max(struct fh_spi_slave_controller *fh_spi)
{
	u32 hw_rx_level;

	hw_rx_level = Spi_ReadRxfifolevel(&fh_spi->dwc);

	return hw_rx_level;
}

static void fh_spi_slave_check_fifo_depth(struct fh_spi_slave_controller *fh_spi)
{
	u32 w_i, r_i, ori;

	ori = Spi_ReadRxlevlel(&fh_spi->dwc);
	for (r_i = w_i = RX_FIFO_MIN_LEN; w_i < RX_FIFO_MAX_LEN; r_i++) {
		Spi_SetRxlevlel(&fh_spi->dwc, ++w_i);
		if (r_i == Spi_ReadRxlevlel(&fh_spi->dwc))
			break;
	}

	fh_spi->dwc.fifo_len = r_i + 1;
}

static void spi_slave_process_tx_isr(struct fh_spi_slave_controller *fh_spi_slave)
{

	u8 tx_buff[SPI_SLAVE_MAX_FIFO_SIZE] = { 0 };
	int kfifo_tx_size, hw_tx_size, trans_size;
	u16 data;
	int i;
	int temp;

	kfifo_tx_size = kfifo_len(&fh_spi_slave->kfifo_in);
	hw_tx_size = tx_max(fh_spi_slave);
	trans_size = min(kfifo_tx_size, hw_tx_size);
	temp = kfifo_out(&fh_spi_slave->kfifo_in, tx_buff, trans_size);

	for (i = 0; i < trans_size; i++) {
		data = tx_buff[i];
		Spi_WriteData(&fh_spi_slave->dwc, data);
	}
	fh_spi_slave->cur_tx_len -= trans_size;
	if (fh_spi_slave->cur_tx_len == 0)
		Spi_DisableIrq(&fh_spi_slave->dwc, SPI_IRQ_TXEIM);

}

static void spi_slave_process_rx_isr(
			struct fh_spi_slave_controller *fh_spi_slave)
{
	int hw_rx_size;
	int i;
	u16 data;

	hw_rx_size = rx_max(fh_spi_slave);
	for (i = 0; i < hw_rx_size; i++) {
		data = Spi_ReadData(&fh_spi_slave->dwc);
		kfifo_in(&fh_spi_slave->kfifo_out, &data, 1);
	}
}

static irqreturn_t fh_spi_slave_irq(int irq, void *dev_id)
{
	struct fh_spi_slave_controller *fh_spi_slave;
	u32 isr_status;
	u32 raw_status;

	fh_spi_slave = (struct fh_spi_slave_controller *) dev_id;
	isr_status = Spi_Isrstatus(&fh_spi_slave->dwc);
	raw_status = Spi_RawIsrstatus(&fh_spi_slave->dwc);
	if (raw_status & (1<<3))
		pr_err("[FH_SPI_S_ERROR]: rx overflow....\n");

	if (isr_status & SPI_IRQ_TXEIM)
		spi_slave_process_tx_isr(fh_spi_slave);

	if (isr_status & SPI_IRQ_RXFIM)
		spi_slave_process_rx_isr(fh_spi_slave);

	return IRQ_HANDLED;
}

static int fh_spi_slave_init_hw(struct fh_spi_slave_controller *fh_spi_slave,
	struct fh_spi_platform_data *board_info)
{
	int status;

	fh_spi_slave_check_fifo_depth(fh_spi_slave);

	fh_spi_slave->dwc.transfer_mode = SPI_TX_RX_MODE;
	do {
		status = Spi_ReadStatus(&fh_spi_slave->dwc);
	} while (status & 0x01);

	Spi_Enable(&fh_spi_slave->dwc, SPI_DISABLE);
	Spi_SetFrameFormat(&fh_spi_slave->dwc, SPI_MOTOROLA_MODE);
	Spi_SetTransferMode(&fh_spi_slave->dwc, fh_spi_slave->dwc.transfer_mode);
	Spi_SetPolarity(&fh_spi_slave->dwc, SPI_POLARITY_HIGH);
	Spi_SetPhase(&fh_spi_slave->dwc, SPI_PHASE_RX_FIRST);
	Spi_SetRxlevlel(&fh_spi_slave->dwc, 0);
	Spi_SetSlaveMode(&fh_spi_slave->dwc, SPI_SLAVE_EN);
	Spi_DisableIrq(&fh_spi_slave->dwc, SPI_IRQ_ALL);
	Spi_EnableIrq(&fh_spi_slave->dwc, SPI_IRQ_RXFIM);
	Spi_Enable(&fh_spi_slave->dwc, SPI_ENABLE);

	return 0;
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

static int fh_spi_slave_probe(struct platform_device *dev)
{
	int err = 0;
	int major_id;
	struct resource *p_res;
	struct clk *clk;
	char spi_slave_name[32] = {0};
	char spi_slave_class_name[32] = {0};
	struct fh_spi_slave_controller *fh_spi_slave;
	struct fh_spi_platform_data *spi_platform_info;

	spi_platform_info = (struct fh_spi_platform_data *)dev->dev.platform_data;
	if (spi_platform_info == NULL) {
		dev_err(&dev->dev, "%s, spi slave platform data null.\n",
			__func__);
		err = -ENODEV;
		return err;
	}

	fh_spi_slave = kzalloc(sizeof(struct fh_spi_slave_controller), GFP_KERNEL);
	if (!fh_spi_slave) {
		dev_err(&dev->dev, "malloc spi slave control mem not enough\n");
		err = -ENOMEM;
		return err;
	}

	p_res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!p_res) {
		dev_err(&dev->dev, "%s, spi slave ioresource error. \n",
			__func__);
		err = -ENODEV;
		goto dev_free;
	}

	fh_spi_slave->dwc.id = spi_platform_info->bus_no;
	fh_spi_slave->dwc.regs = ioremap(p_res->start, resource_size(p_res));
	fh_spi_slave->dwc.paddr = p_res->start;
	fh_spi_slave->dwc.irq = platform_get_irq(dev, 0);
	fh_spi_slave->dwc.irq = irq_create_mapping(NULL, fh_spi_slave->dwc.irq);
	sprintf(fh_spi_slave->dwc.isr_name, "spi-slave-%d", dev->id);

	if (!fh_spi_slave->dwc.irq) {
		pr_err("%s: ERROR: getting resource failed"
				"cannot get IORESOURCE_IRQ\n", __func__);
		err = -ENXIO;
		goto dev_free;
	}

	err = request_irq(fh_spi_slave->dwc.irq, fh_spi_slave_irq, 0, fh_spi_slave->dwc.isr_name,
				fh_spi_slave);
	if (err) {
		dev_dbg(&dev->dev, "request_irq failed, %d\n", err);
		goto irqmapping_free;
	}

	spi_platform_info->bus_no = dev->id;
	priv_array[dev->id] = fh_spi_slave;
	spin_lock_init(&fh_spi_slave->lock);

	clk = clk_get(NULL, spi_platform_info->clk_name);
	if (IS_ERR(clk)) {
		dev_err(&fh_spi_slave->p_dev->dev, "cannot find the spi-slave%d clk.\n",
		fh_spi_slave->dwc.id);
		err = PTR_ERR(clk);
		goto irqmapping_free;
	}
	clk_set_rate(clk, SPI_DEFAULT_CLK);
	clk_prepare_enable(clk);

	fh_spi_slave->dwc.board_info = spi_platform_info;
	err = fh_spi_slave_init_hw(fh_spi_slave, spi_platform_info);
	if (err)
		goto irqmapping_free;

	sprintf(spi_slave_name, "fh_spi_slave_%d", dev->id);
	sprintf(spi_slave_class_name, "fh_spi_slave_class_%d", dev->id);

	major_id = register_chrdev(0, spi_slave_name, &spi_slave_fops);
	if (major_id <= 0) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev, "cannot register spi slave_%d char dev..\n",
				fh_spi_slave->dwc.id);
		goto irqmapping_free;
	} else
		fh_spi_slave->major = major_id;


	fh_spi_slave->psClass = class_create(THIS_MODULE, spi_slave_class_name);
	if (IS_ERR(fh_spi_slave->psClass)) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev, "%s: Unable to create class\n", __FILE__);
		goto irqmapping_free;
	}

	fh_spi_slave->psDev = device_create(fh_spi_slave->psClass,
										NULL,
										MKDEV(major_id, 0),
										fh_spi_slave,
										spi_slave_name);
	if (IS_ERR(fh_spi_slave->psDev)) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev,"Error: %s: Unable to create device\n", __FILE__);
		goto irqmapping_free;
	}

	if (kfifo_alloc(
		&fh_spi_slave->kfifo_in,
		KFIFO_SIZE,
		GFP_KERNEL)) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev,"Error: %s: Unable to alloc kfifo..\n", __FILE__);
		goto irqmapping_free;
	}

	if (kfifo_alloc(&fh_spi_slave->kfifo_out,
		KFIFO_SIZE,
		GFP_KERNEL)) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev,"Error: %s: Unable to alloc kfifo..\n", __FILE__);
		goto irqmapping_free;
	}

	dev_info(&dev->dev, "FH Spi Slave probe successfully\n");

	return 0;

irqmapping_free:
	free_irq(fh_spi_slave->dwc.irq, fh_spi_slave);
dev_free:
	kfree(fh_spi_slave);

	return err;
}

static int fh_spi_slave_remove(struct platform_device *dev)
{
	return 0;
}

static const struct of_device_id fh_spi_slave_of_match[] = {
	{.compatible = "fh,fh-spi-slave",},
	{},
};

MODULE_DEVICE_TABLE(of, fh_spi_slave_of_match);

static struct platform_driver fh_spi_slave_driver = {
	.driver = {
		.name = "fh_spi_slave",
		.of_match_table = fh_spi_slave_of_match,
	},
	.probe = fh_spi_slave_probe,
	.remove = fh_spi_slave_remove,
};

static int fh_spi_slave_init(void)
{
	return platform_driver_register(&fh_spi_slave_driver);
}

static void fh_spi_slave_exit(void)
{
	platform_driver_unregister(&fh_spi_slave_driver);
}

module_init(fh_spi_slave_init);
module_exit(fh_spi_slave_exit);
MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_DESCRIPTION("DUOBAO SPI SLAVE driver");
MODULE_LICENSE("GPL");

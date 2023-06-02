/*
 * fh_slave_spi.c
 *
 *  Created on: Sep 19, 2016
 *      Author: duobao
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/bug.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <mach/fh_dmac.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/io.h>
#include <linux/kfifo.h>
#include <mach/spi.h>

#define lift_shift_bit_num(bit_num)	(1<<bit_num)
//read spi irq, only useful if you set which is masked
#define SPI_IRQ_TXEIS	(lift_shift_bit_num(0))
#define SPI_IRQ_TXOIS	(lift_shift_bit_num(1))
#define SPI_IRQ_RXUIS	(lift_shift_bit_num(2))
#define SPI_IRQ_RXOIS	(lift_shift_bit_num(3))
#define SPI_IRQ_RXFIS	(lift_shift_bit_num(4))
#define SPI_IRQ_MSTIS	(lift_shift_bit_num(5))
//spi status
#define SPI_STATUS_BUSY         (lift_shift_bit_num(0))
#define SPI_STATUS_TFNF         (lift_shift_bit_num(1))
#define SPI_STATUS_TFE          (lift_shift_bit_num(2))
#define SPI_STATUS_RFNE         (lift_shift_bit_num(3))
#define SPI_STATUS_RFF          (lift_shift_bit_num(4))
#define SPI_STATUS_TXE          (lift_shift_bit_num(5))
#define SPI_STATUS_DCOL         (lift_shift_bit_num(6))
#define CACHE_LINE_SIZE		(32)
#define PUMP_DATA_NONE_MODE	(0x00)
#define PUMP_DATA_DMA_MODE	(0x11)
#define PUMP_DATA_ISR_MODE	(0x22)
#define PUMP_DATA_POLL_MODE	(0x33)
#define SPI_DIV_TRANSFER_SIZE	(256)
#define SPI_DATA_REG_OFFSET	(0x60)
#define KFIFO_SIZE		2048
#define DUMY_DATA		0xff
#define SPI_SLAVE_MAX_FIFO_SIZE	256
#define SLAVE_SET_PHASE		1
#define SLAVE_SET_POLARITY	SLAVE_SET_PHASE + 1
#define SLAVE_INIT_RX_FIFO 	SLAVE_SET_POLARITY + 1
#define SLAVE_INIT_TX_FIFO 	SLAVE_INIT_RX_FIFO + 1
#define SLAVE_GET_ERROR_STATUS	SLAVE_INIT_TX_FIFO + 1

//#define FH_SPI_SLAVE_DEBUG
#define MAX_SPI_SLAVES			8
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
enum {
	CONFIG_OK = 0, CONFIG_PARA_ERROR = lift_shift_bit_num(0),
	//only for the set slave en/disable
	CONFIG_BUSY = lift_shift_bit_num(1),
	//only for write_read mode
	WRITE_READ_OK = 0,
	WRITE_READ_ERROR = lift_shift_bit_num(2),
	WRITE_READ_TIME_OUT = lift_shift_bit_num(3),
	//only for write only mode
	WRITE_ONLY_OK = 0,
	WRITE_ONLY_ERROR = lift_shift_bit_num(4),
	WRITE_ONLY_TIME_OUT = lift_shift_bit_num(5),
	//only for read only mode
	READ_ONLY_OK = 0,
	READ_ONLY_ERROR = lift_shift_bit_num(6),
	READ_ONLY_TIME_OUT = lift_shift_bit_num(7),
	//eeprom mode
	EEPROM_OK = 0,
	EEPROM_ERROR = lift_shift_bit_num(8),
	EEPROM_TIME_OUT = lift_shift_bit_num(9),
	//if read/write/eeprom error,the error below could give you more info by reading the 'Spi_ReadTransferError' function
	MULTI_MASTER_ERROR = lift_shift_bit_num(10),
	TX_OVERFLOW_ERROR = lift_shift_bit_num(11),
	RX_OVERFLOW_ERROR = lift_shift_bit_num(12),
};

//enable spi
typedef enum enum_spi_enable {
	SPI_DISABLE = 0,
	SPI_ENABLE = (lift_shift_bit_num(0)),
} spi_enable_e;

//polarity
typedef enum enum_spi_polarity {
	SPI_POLARITY_LOW = 0,
	SPI_POLARITY_HIGH = (lift_shift_bit_num(7)),
	//bit pos
	SPI_POLARITY_RANGE = (lift_shift_bit_num(7)),
} spi_polarity_e;

//phase
typedef enum enum_spi_phase {
	SPI_PHASE_RX_FIRST = 0,
	SPI_PHASE_TX_FIRST = (lift_shift_bit_num(6)),
	//bit pos
	SPI_PHASE_RANGE = (lift_shift_bit_num(6)),
} spi_phase_e;

//frame format
typedef enum enum_spi_format {
	SPI_MOTOROLA_MODE = 0x00,
	SPI_TI_MODE = 0x10,
	SPI_MICROWIRE_MODE = 0x20,
	//bit pos
	SPI_FRAME_FORMAT_RANGE = 0x30,
} spi_format_e;

//data size
typedef enum enum_spi_data_size {
	SPI_DATA_SIZE_4BIT = 0x03,
	SPI_DATA_SIZE_5BIT = 0x04,
	SPI_DATA_SIZE_6BIT = 0x05,
	SPI_DATA_SIZE_7BIT = 0x06,
	SPI_DATA_SIZE_8BIT = 0x07,
	SPI_DATA_SIZE_9BIT = 0x08,
	SPI_DATA_SIZE_10BIT = 0x09,
	SPI_DATA_SIZE_16BIT = 0x0f,
	//bit pos
	SPI_DATA_SIZE_RANGE = 0x0f,
} spi_data_size_e;

//transfer mode
typedef enum enum_spi_transfer_mode {
	SPI_TX_RX_MODE = 0x000,
	SPI_ONLY_TX_MODE = 0x100,
	SPI_ONLY_RX_MODE = 0x200,
	SPI_EEPROM_MODE = 0x300,
	//bit pos
	SPI_TRANSFER_MODE_RANGE = 0x300,
} spi_transfer_mode_e;

//spi baudrate
typedef enum enum_spi_baudrate {
	SPI_SCLKIN = 50000000,
	SPI_SCLKOUT_27000000 = (SPI_SCLKIN / 27000000), //27M
	SPI_SCLKOUT_13500000 = (SPI_SCLKIN / 13500000),  //13.5M
	SPI_SCLKOUT_6750000 = (SPI_SCLKIN / 6750000),  //6.75M
	SPI_SCLKOUT_4500000 = (SPI_SCLKIN / 4500000),	 //4.5M
	SPI_SCLKOUT_3375000 = (SPI_SCLKIN / 3375000),  //3.375M
	SPI_SCLKOUT_2700000 = (SPI_SCLKIN / 2700000),	 //2.7M
	SPI_SCLKOUT_1500000 = (SPI_SCLKIN / 1500000),  //1.5M
	SPI_SCLKOUT_500000 = (SPI_SCLKIN / 500000),  //0.1M
	SPI_SCLKOUT_100000 = (SPI_SCLKIN / 100000),  //0.1M
} spi_baudrate_e;

//spi_irq
typedef enum enum_spi_irq {
	SPI_IRQ_TXEIM = (lift_shift_bit_num(0)),
	SPI_IRQ_TXOIM = (lift_shift_bit_num(1)),
	SPI_IRQ_RXUIM = (lift_shift_bit_num(2)),
	SPI_IRQ_RXOIM = (lift_shift_bit_num(3)),
	SPI_IRQ_RXFIM = (lift_shift_bit_num(4)),
	SPI_IRQ_MSTIM = (lift_shift_bit_num(5)),
	SPI_IRQ_ALL = 0x3f,
} spi_irq_e;

//spi_slave_port
typedef enum enum_spi_slave {
	SPI_SLAVE_PORT0 = (lift_shift_bit_num(0)),
	SPI_SLAVE_PORT1 = (lift_shift_bit_num(1)),
} spi_slave_e;

//dma control
typedef enum enum_spi_dma_control_mode {
	SPI_DMA_RX_POS = (lift_shift_bit_num(0)),
	SPI_DMA_TX_POS = (lift_shift_bit_num(1)),
	//bit pos
	SPI_DMA_CONTROL_RANGE = 0x03,
} spi_dma_control_mode_e;

//frame format
typedef enum enum_spi_slave_mode {
	SPI_SLAVE_EN = 0x00,
	SPI_SLAVE_DIS = 1 << 10,
	//bit pos
	SPI_SLAVE_MODE_RANGE = 1 << 10,
} spi_slave_mode_e;

#ifdef FH_SPI_SLAVE_DEBUG
#define SPI_SLAVE_PRINT_DBG(fmt, args...) \
	printk("[FH_SPI_S_DEBUG]: "); \
	printk(fmt, ## args)
#else
#define SPI_SLAVE_PRINT_DBG(fmt, args...)  do { } while (0)
#endif

struct fh_spi_reg {
	u32 ctrl0;
	u32 ctrl1;
	u32 ssienr;
	u32 mwcr;
	u32 ser;
	u32 baudr;
	u32 txfltr;
	u32 rxfltr;
	u32 txflr;
	u32 rxflr;
	u32 sr;
	u32 imr;
	u32 isr;
	u32 risr;
	u32 txoicr;
	u32 rxoicr;
	u32 rxuicr;
	u32 msticr;
	u32 icr;
	u32 dmacr;
	u32 dmatdlr;
	u32 dmardlr;
	u32 idr;
	u32 version;
	u32 dr; /* Currently oper as 32 bits,
	 though only low 16 bits matters */
};

#define dw_readl(dw, name) \
	__raw_readl(&(((struct fh_spi_reg *)dw->regs)->name))
#define dw_writel(dw, name, val) \
	__raw_writel((val), &(((struct fh_spi_reg *)dw->regs)->name))
#define dw_readw(dw, name) \
	__raw_readw(&(((struct fh_spi_reg *)dw->regs)->name))
#define dw_writew(dw, name, val) \
	__raw_writew((val), &(((struct fh_spi_reg *)dw->regs)->name))

struct _fh_spi_dma_transfer {
	//bind to dma channel
	struct dma_chan *chan;
	struct dma_slave_config *cfg;
	struct scatterlist sgl;
	struct dma_async_tx_descriptor *desc;
};

struct fh_spi {
	void * __iomem regs; /* vaddr of the control registers */
	u32 id;
	u32 paddr;
	u32 slave_port;
	u32 irq; /* irq no */
	u32 fifo_len; /* depth of the FIFO buffer */
	u32 cpol;
	u32 cpha;
	u32 isr_flag;
	//clk
	u32 apb_clock_in;
	u32 max_freq; /* max bus freq supported */
	u32 div;
	/*use id u32 bus_num;*//*which bus*/
	u32 num_cs; /* supported slave numbers */
	u32 data_width;
	u32 frame_mode;
	u32 transfer_mode;
	u32 active_cs_pin;
	//copy from the user...
	u32 tx_len;
	u32 rx_len;
	void *rx_buff;
	void *tx_buff;
	u32 tx_dma_add;
	u32 rx_dma_add;
	u32 tx_hs_no;			//tx handshaking number
	u32 rx_hs_no;			//rx handshaking number
	u32 *tx_dumy_buff;
	u32 *rx_dumy_buff;
	struct fh_spi_cs cs_data[SPI_MASTER_CONTROLLER_MAX_SLAVE];
	u32 pump_data_mode;
	struct _fh_spi_dma_transfer dma_rx;
	struct _fh_spi_dma_transfer dma_tx;
	u32 complete_times;
	struct fh_spi_platform_data *board_info;
};

//this file private
struct fh_spi_slave_controller {
	struct clk *clk;
	spinlock_t lock;
	//message queue
	struct platform_device *p_dev;
	struct fh_spi dwc;
	struct completion tx_done;

	u32 cur_rx_len;
	u32 cur_tx_len;

	//dev interface
	int major;
	struct class *psClass;
	struct device *psDev;

	//kfifo interface
	struct kfifo kfifo_in;
	struct kfifo kfifo_out;
};

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

static int fh_spi_slave_init_hw(struct fh_spi_slave_controller *fh_spi_slave,
                struct fh_spi_platform_data *board_info);
static u32 Spi_RawIsrstatus(struct fh_spi *dw);

/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
static struct fh_spi_slave_controller *priv_array[MAX_SPI_SLAVES] = { NULL,
NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/* function body */

static u32 Spi_Enable(struct fh_spi *dw, spi_enable_e enable)
{
	dw_writel(dw, ssienr, enable);
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

static u32 Spi_SetRxlevlel(struct fh_spi *dw, u32 level)
{
	dw_writel(dw, rxfltr, level);
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

static u32 Spi_ReadStatus(struct fh_spi *dw)
{
	return (uint8_t) dw_readl(dw, sr);
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

static u32 Spi_WriteData(struct fh_spi *dw, u16 data)
{
	dw_writew(dw, dr, data);
	return WRITE_ONLY_OK;
}

static u16 Spi_ReadData(struct fh_spi *dw)
{
	return dw_readw(dw, dr);
}

#if(0)
static void Spi_Clearallerror(struct fh_spi *dw)
{
	u32 data = dw_readl(dw, icr);
	data = 0;
}
#endif

static u32 Spi_Isrstatus(struct fh_spi *dw)
{
	u32 data = dw_readl(dw, isr);
	return data;
}

static u32 Spi_RawIsrstatus(struct fh_spi *dw)
{
	u32 data = dw_readl(dw, risr);
	return data;
}

#if(0)
static void Spi_SetDmaTxDataLevel(struct fh_spi *dw, u32 level)
{
	dw_writel(dw, dmatdlr, level);
}

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
#endif

static int spi_slave_open(struct inode *inode, struct file *filp)
{
	int i, ret = 0;
	struct fh_spi_slave_controller *fh_spi_slave;
	SPI_SLAVE_PRINT_DBG("%s\n", __func__);
	//bind the pri to the spi slave control...
	SPI_SLAVE_PRINT_DBG("inode id is %x..\n", inode->i_rdev);
	for (i = 0; i < MAX_SPI_SLAVES; i++) {
		SPI_SLAVE_PRINT_DBG("register id is %x..\n",
		                MKDEV(priv_array[i]->major, 0));
		if (priv_array[i]
		                && MKDEV(priv_array[i]->major, 0)
		                                == inode->i_rdev) {
			//SPI_SLAVE_PRINT_DBG();
			filp->private_data = priv_array[i];
			break;
		}
	}
	if (i == MAX_SPI_SLAVES)
		return -ENXIO;
	//reset kfifo...
	fh_spi_slave = priv_array[i];
	kfifo_reset(&fh_spi_slave->kfifo_in);
	kfifo_reset(&fh_spi_slave->kfifo_out);
	return ret;
}

static ssize_t spi_slave_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	int ret;
	unsigned int copied;
	struct fh_spi_slave_controller *fh_spi_slave;
	fh_spi_slave = (struct fh_spi_slave_controller *) filp->private_data;
	//write data to fifo_out
	if (kfifo_is_empty(&fh_spi_slave->kfifo_out)) {
		return -EFAULT;
	}
	ret = kfifo_to_user(&fh_spi_slave->kfifo_out, buf, count, &copied);
	//start spi hw work...
	if (ret == 0) {
		fh_spi_slave->cur_rx_len = copied;
		return copied;
	}
	//error..
	else {

		return ret;
	}

	//hw isr pump fifo_out data..

}

#if(0)
static void wait_spi_idle(struct fh_spi_slave_controller *fh_spi_slave) {
	int status;
	do {
		status = Spi_ReadStatus(&fh_spi_slave->dwc);
		SPI_SLAVE_PRINT_DBG("status is %x\n",status);
	}while (status & 0x01);
}
#endif

static void spi_slave_isr_tx_data(struct fh_spi_slave_controller *fh_spi_slave)
{
	//fh_spi_slave->dwc.isr_flag = SPI_IRQ_TXEIM;
	//Spi_SetTxlevlel(&fh_spi_slave->dwc, fh_spi_slave->dwc.fifo_len / 2);
	Spi_SetTxlevlel(&fh_spi_slave->dwc, fh_spi_slave->dwc.fifo_len - 5);
	SPI_SLAVE_PRINT_DBG("open spi slave isr tx..\n");
	Spi_EnableIrq(&fh_spi_slave->dwc, SPI_IRQ_TXEIM);
	//wait_for_completion(&fh_spi_slave->tx_done);
}

static ssize_t spi_slave_write(struct file *filp, const char __user *buf,
                size_t count, loff_t *f_pos)
{
	int ret;
	unsigned int copied;
	struct fh_spi_slave_controller *fh_spi_slave;
	fh_spi_slave = (struct fh_spi_slave_controller *) filp->private_data;
	if (kfifo_is_full(&fh_spi_slave->kfifo_in)) {
		//spin_unlock_irqrestore(&fh_spi_slave->lock, flags);
		return -EFAULT;
	}
	ret = kfifo_from_user(&fh_spi_slave->kfifo_in, buf, count, &copied);
	//start spi hw work...
	if (ret == 0) {
		//start spi hw work...
		//here we could start a back work to process the hw write data....
		fh_spi_slave->cur_tx_len = copied;
		spi_slave_isr_tx_data(fh_spi_slave);
		return copied;
	}
	//error..
	else {
		return ret;
	}



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
	hw_tx_level /= 2;
	return hw_tx_level;	//min(hw_tx_level, fh_spi_slave->dwc.tx_len);
}

/* Return the max entries we should read out of rx fifo */
static inline u32 rx_max(struct fh_spi_slave_controller *fh_spi_slave)
{
	u32 hw_rx_level;

	hw_rx_level = Spi_ReadRxfifolevel(&fh_spi_slave->dwc);
	return hw_rx_level;
}

static void spi_slave_process_tx_isr(
                struct fh_spi_slave_controller *fh_spi_slave)
{

	u8 tx_buff[SPI_SLAVE_MAX_FIFO_SIZE] = { 0 };
	int kfifo_tx_size, hw_tx_size, trans_size;
	u16 data;
	int i;
	int temp;
	//Spi_DisableIrq(&fh_spi_slave->dwc, SPI_IRQ_TXEIM);
	//Spi_DisableIrq(&fh_spi_slave->dwc, SPI_IRQ_RXFIM);

	kfifo_tx_size = kfifo_len(&fh_spi_slave->kfifo_in);
	//kfifo_tx_size = fh_spi_slave->cur_tx_len;
	hw_tx_size = tx_max(fh_spi_slave);
	//read MIN(hw tx fifo avail size , tx kfifo size)
	trans_size = min(kfifo_tx_size, hw_tx_size);
	temp = kfifo_out(&fh_spi_slave->kfifo_in, tx_buff, trans_size);
	//transfer data to hw.. and reduce the actual trans data size..
	SPI_SLAVE_PRINT_DBG("kfifo size :%d,  hw size:%d..\n",kfifo_tx_size,hw_tx_size);
	SPI_SLAVE_PRINT_DBG("tx isr size is %d..\n",trans_size);
	//printk("**0d%d\n",trans_size);
	for (i = 0; i < trans_size; i++) {
		data = tx_buff[i];
		//SPI_SLAVE_PRINT_DBG("tx data is %x\n",data);
		Spi_WriteData(&fh_spi_slave->dwc, data);
	}
	//SPI_SLAVE_PRINT_DBG("\n");
	fh_spi_slave->cur_tx_len -= trans_size;
	if (fh_spi_slave->cur_tx_len == 0) {
		Spi_DisableIrq(&fh_spi_slave->dwc, SPI_IRQ_TXEIM);
		//complete(&(fh_spi_slave->tx_done));
	} else {
		//Spi_EnableIrq(&fh_spi_slave->dwc, SPI_IRQ_TXEIM);
	}

}

static void spi_slave_process_rx_isr(
                struct fh_spi_slave_controller *fh_spi_slave)
{

	int hw_rx_size;
	int i;
	u16 data;
	int status;
	//here we try to get more data when the the clk is too high...
	//do {
	hw_rx_size = rx_max(fh_spi_slave);
	SPI_SLAVE_PRINT_DBG("rx get size is 0x%d\n",hw_rx_size);
	for (i = 0; i < hw_rx_size; i++) {
		data = Spi_ReadData(&fh_spi_slave->dwc);
		//rx_buff[i] = (u8) data;
		kfifo_in(&fh_spi_slave->kfifo_out, &data, 1);
	}
	status = Spi_ReadStatus(&fh_spi_slave->dwc);
	//} while (status & (1 << 3));

}

static irqreturn_t fh_spi_slave_irq(int irq, void *dev_id)
{
	struct fh_spi_slave_controller *fh_spi_slave;
	u32 isr_status;
	u32 raw_status;

	fh_spi_slave = (struct fh_spi_slave_controller *) dev_id;
	isr_status = Spi_Isrstatus(&fh_spi_slave->dwc);
	raw_status = Spi_RawIsrstatus(&fh_spi_slave->dwc);
	//printk("raw irq status is 0x%x..\n",raw_status);
	SPI_SLAVE_PRINT_DBG("irq status is 0x%x..\n",isr_status);
	if(raw_status & (1<<3)){
		printk("[FH_SPI_S_ERROR]: rx overflow....\n");
	}
	if (isr_status & SPI_IRQ_TXEIM) {
		spi_slave_process_tx_isr(fh_spi_slave);
	}
	if (isr_status & SPI_IRQ_RXFIM) {
		spi_slave_process_rx_isr(fh_spi_slave);
	}

	return IRQ_HANDLED;
}

static int __devinit fh_spi_slave_probe(struct platform_device *dev)
{
	int err = 0;
	struct resource *r,*ioarea;
	int ret;
	char spi_slave_name[32] = {0};
	char spi_slave_class_name[32] = {0};
	int major_id;
	struct fh_spi_slave_controller *fh_spi_slave;
	struct fh_spi_platform_data * spi_platform_info;
	spi_platform_info = (struct fh_spi_platform_data *)dev->dev.platform_data;
	if(spi_platform_info == NULL){
		err = -ENODEV;
		dev_err(&dev->dev, "%s, spi slave platform data null.\n",
			__func__);
		BUG();

	}
	fh_spi_slave =kzalloc(sizeof(struct fh_spi_slave_controller), GFP_KERNEL);
	if (!fh_spi_slave) {
		dev_err(&dev->dev, "malloc spi slave control mem not enough\n");
		BUG();
	}
	fh_spi_slave->dwc.irq = platform_get_irq(dev, 0);
	if (fh_spi_slave->dwc.irq < 0) {
		dev_err(&dev->dev, "%s, spi slave irq no error.\n",
			__func__);
		err = fh_spi_slave->dwc.irq;
		BUG();
	}
	err = request_irq(fh_spi_slave->dwc.irq , fh_spi_slave_irq, 0,
			  dev_name(&dev->dev), fh_spi_slave);
	if (err) {
		dev_dbg(&dev->dev, "request_irq failed, %d\n", err);BUG();
	}
	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&dev->dev, "%s, spi slave ioresource error. \n",
			__func__);
		err = -ENODEV;
		BUG();
	}
	fh_spi_slave->dwc.paddr = r->start;
	ioarea = request_mem_region(r->start,
                resource_size(r), dev->name);if(!ioarea) {
		dev_err(&dev->dev, "spi slave region already claimed\n");
		err = -EBUSY;
		BUG();

	}
	fh_spi_slave->dwc.regs = ioremap(r->start, resource_size(r));
	if (!fh_spi_slave->dwc.regs) {
		dev_err(&dev->dev, "spi slave region already mapped\n");
		err = -EINVAL;
		BUG();
	}
	spi_platform_info->bus_no = dev->id;
	priv_array[dev->id] = fh_spi_slave;
	init_completion(&fh_spi_slave->tx_done);
	spin_lock_init(
                &fh_spi_slave->lock);

	fh_spi_slave->clk = clk_get(NULL, spi_platform_info->clk_name);
	if (IS_ERR(fh_spi_slave->clk)) {
		dev_err(&fh_spi_slave->p_dev->dev, "cannot find the spi%d clk.\n",
				fh_spi_slave->dwc.id);
		err = PTR_ERR(fh_spi_slave->clk);
		BUG();
	}
	clk_enable(fh_spi_slave->clk);
	clk_set_rate(fh_spi_slave->clk,spi_platform_info->apb_clock_in);

	ret = fh_spi_slave_init_hw(fh_spi_slave,spi_platform_info);
	if(ret) {
		err = ret;
		BUG();
	}
	init_completion(&fh_spi_slave->tx_done);
	sprintf(spi_slave_name, "fh_spi_slave_%d", dev->id);
	sprintf(spi_slave_class_name, "fh_spi_slave_class_%d", dev->id);
	major_id = register_chrdev(0, spi_slave_name, &spi_slave_fops);
	if (major_id <= 0) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev, "cannot register spi slave_%d char dev..\n",
				fh_spi_slave->dwc.id);
		BUG();
	} else {
		fh_spi_slave->major = major_id;
	}

	fh_spi_slave->psClass = class_create(THIS_MODULE, spi_slave_class_name);if (IS_ERR(fh_spi_slave->psClass)) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev, "%s: Unable to create class\n", __FILE__);
		BUG();
	}

	fh_spi_slave->psDev = device_create(fh_spi_slave->psClass, NULL, MKDEV(major_id, 0),
			fh_spi_slave, spi_slave_name);
	if (IS_ERR(fh_spi_slave->psDev)) {
		err = -EIO;
		dev_err(&fh_spi_slave->p_dev->dev,"Error: %s: Unable to create device\n", __FILE__);
		BUG();
	}

	if(kfifo_alloc(
                &fh_spi_slave->kfifo_in, KFIFO_SIZE, GFP_KERNEL)){
		dev_err(&fh_spi_slave->p_dev->dev,"Error: %s: Unable to alloc kfifo..\n", __FILE__);
		BUG();
	}

	if(kfifo_alloc(&fh_spi_slave->kfifo_out, KFIFO_SIZE, GFP_KERNEL)) {
		dev_err(&fh_spi_slave->p_dev->dev,"Error: %s: Unable to alloc kfifo..\n", __FILE__);
		BUG();
	}

	#if(0)

	//1 :empty 		0:not empty
	//1 :full		0:not full
	int empty,full,avail;
	char test_buf_out[20] = {0};

	empty = kfifo_is_empty(&fh_spi_slave->kfifo_in);
	full = kfifo_is_full(&fh_spi_slave->kfifo_in);
	avail = kfifo_avail(&fh_spi_slave->kfifo_in);
	printk("empty:  %x,  full:  %x,  avail:  %x\n",empty,full,avail);
	printk(KERN_INFO "queue len: %u\n", kfifo_len(&fh_spi_slave->kfifo_in));
	kfifo_in(&fh_spi_slave->kfifo_in, "hello", 5);
	printk(KERN_INFO "queue len: %u\n", kfifo_len(&fh_spi_slave->kfifo_in));

	empty = kfifo_is_empty(&fh_spi_slave->kfifo_in);
	full = kfifo_is_full(&fh_spi_slave->kfifo_in);
	avail = kfifo_avail(&fh_spi_slave->kfifo_in);
	printk("empty:  %x,  full:  %x,  avail:  %x\n",empty,full,avail);

	/* put values into the fifo */
	for (i = 0; i !=5; i++)
	kfifo_put(&fh_spi_slave->kfifo_in, &i);

	i = kfifo_out(&fh_spi_slave->kfifo_in, test_buf_out, 5);
	printk("data len is %d\n",i);
	printk(KERN_INFO "buf: %.*s\n", i, test_buf_out);

	printk(KERN_INFO "queue len: %u\n", kfifo_len(&fh_spi_slave->kfifo_in));
	i = kfifo_out(&fh_spi_slave->kfifo_in, test_buf_out, 10);
	printk("data len is %d\n",i);
	printk(KERN_INFO "buf: %.*s\n", i, test_buf_out);
	#endif

	return err;
}

static int __devexit fh_spi_slave_remove(struct platform_device *dev)
{
	return 0;
}

static int fh_spi_slave_init_hw(struct fh_spi_slave_controller *fh_spi_slave,
        struct fh_spi_platform_data *board_info)
{
	int status;
	fh_spi_slave->dwc.id = board_info->bus_no;
	fh_spi_slave->dwc.fifo_len = board_info->fifo_len;
	fh_spi_slave->dwc.rx_hs_no = board_info->rx_handshake_num;
	fh_spi_slave->dwc.tx_hs_no = board_info->tx_handshake_num;
	memset(&fh_spi_slave->dwc.dma_rx, 0, sizeof(struct _fh_spi_dma_transfer));
	memset(&fh_spi_slave->dwc.dma_tx, 0, sizeof(struct _fh_spi_dma_transfer));
	fh_spi_slave->dwc.pump_data_mode = PUMP_DATA_ISR_MODE;
	//bind the platform data here....
	fh_spi_slave->dwc.board_info = board_info;

	fh_spi_slave->dwc.isr_flag = SPI_IRQ_RXFIM;
	fh_spi_slave->dwc.frame_mode = SPI_MOTOROLA_MODE;
	fh_spi_slave->dwc.transfer_mode = SPI_TX_RX_MODE;
	fh_spi_slave->dwc.cpol = SPI_POLARITY_HIGH;
	fh_spi_slave->dwc.cpha = SPI_PHASE_RX_FIRST;
	do {
		status = Spi_ReadStatus(&fh_spi_slave->dwc);
	} while (status & 0x01);
	//add spi disable
	Spi_Enable(&fh_spi_slave->dwc, SPI_DISABLE);
	//add spi frame mode & transfer mode
	Spi_SetFrameFormat(&fh_spi_slave->dwc, fh_spi_slave->dwc.frame_mode);
	Spi_SetTransferMode(&fh_spi_slave->dwc, fh_spi_slave->dwc.transfer_mode);
	Spi_SetPolarity(&fh_spi_slave->dwc, fh_spi_slave->dwc.cpol);
	Spi_SetPhase(&fh_spi_slave->dwc, fh_spi_slave->dwc.cpha);
	//Spi_SetRxlevlel(&fh_spi_slave->dwc, fh_spi_slave->dwc.fifo_len / 2);
	Spi_SetRxlevlel(&fh_spi_slave->dwc, 0);
	Spi_SetSlaveMode(&fh_spi_slave->dwc, SPI_SLAVE_EN);
	//add spi disable all isr
	Spi_DisableIrq(&fh_spi_slave->dwc, SPI_IRQ_ALL);
	Spi_EnableIrq(&fh_spi_slave->dwc, fh_spi_slave->dwc.isr_flag);
	//add spi enable
	Spi_Enable(&fh_spi_slave->dwc, SPI_ENABLE);

	return 0;

}

static struct platform_driver fh_spi_slave_driver = {
	.probe = fh_spi_slave_probe,
        .remove = __devexit_p(fh_spi_slave_remove),
        .driver = {
        		.name = "fh_spi_slave",
        		.owner = THIS_MODULE,
	},
        .suspend =NULL,
        .resume = NULL,
};

static int __init fh_spi_slave_init(void)
{
	return platform_driver_register(&fh_spi_slave_driver);
}

static void __exit fh_spi_slave_exit(void)
{
	platform_driver_unregister(&fh_spi_slave_driver);
}

module_init(fh_spi_slave_init);
module_exit(fh_spi_slave_exit);
MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_DESCRIPTION("DUOBAO SPI SLAVE driver");
MODULE_LICENSE("GPL");

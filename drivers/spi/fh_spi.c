/** @file fh_spi.c
 *  @note ShangHai FullHan Co., Ltd. All Right Reserved.
 *  @brief fh driver
 *  @author     yu.zhang
 *  @date       2015/1/11
 *  @note history
 *  @note 2014-1-11 V1.0.0 create the file.
 */
/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/bug.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/dmaengine.h>
#include <mach/spi.h>
#include <mach/fh_dmac.h>
#include <linux/dma-mapping.h>
#include <mach/fh_dmac_regs.h>
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define lift_shift_bit_num(bit_num)     (1<<bit_num)
/* read spi irq, only useful if you set which is masked */
#define SPI_IRQ_TXEIS           (lift_shift_bit_num(0))
#define SPI_IRQ_TXOIS           (lift_shift_bit_num(1))
#define SPI_IRQ_RXUIS           (lift_shift_bit_num(2))
#define SPI_IRQ_RXOIS           (lift_shift_bit_num(3))
#define SPI_IRQ_RXFIS           (lift_shift_bit_num(4))
#define SPI_IRQ_MSTIS           (lift_shift_bit_num(5))
/* spi status */
#define SPI_STATUS_BUSY         (lift_shift_bit_num(0))
#define SPI_STATUS_TFNF         (lift_shift_bit_num(1))
#define SPI_STATUS_TFE          (lift_shift_bit_num(2))
#define SPI_STATUS_RFNE         (lift_shift_bit_num(3))
#define SPI_STATUS_RFF          (lift_shift_bit_num(4))
#define SPI_STATUS_TXE          (lift_shift_bit_num(5))
#define SPI_STATUS_DCOL         (lift_shift_bit_num(6))
#define CACHE_LINE_SIZE         (32)
#define PUMP_DATA_NONE_MODE     (0x00)
#define PUMP_DATA_DMA_MODE      (0x11)
#define PUMP_DATA_ISR_MODE      (0x22)
#define PUMP_DATA_POLL_MODE     (0x33)
#define DMA_TRANS_GATE_LEVEL    1024
#define SPI_DATA_REG_OFFSET     (0x60)
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
enum {
	CONFIG_OK = 0, CONFIG_PARA_ERROR = lift_shift_bit_num(0),
	/* only for the set slave en/disable */
	CONFIG_BUSY = lift_shift_bit_num(1),
	/* only for write_read mode */
	WRITE_READ_OK = 0,
	WRITE_READ_ERROR = lift_shift_bit_num(2),
	WRITE_READ_TIME_OUT = lift_shift_bit_num(3),
	/* only for write only mode */
	WRITE_ONLY_OK = 0,
	WRITE_ONLY_ERROR = lift_shift_bit_num(4),
	WRITE_ONLY_TIME_OUT = lift_shift_bit_num(5),
	/* only for read only mode */
	READ_ONLY_OK = 0,
	READ_ONLY_ERROR = lift_shift_bit_num(6),
	READ_ONLY_TIME_OUT = lift_shift_bit_num(7),
	/* eeprom mode */
	EEPROM_OK = 0,
	EEPROM_ERROR = lift_shift_bit_num(8),
	EEPROM_TIME_OUT = lift_shift_bit_num(9),
	MULTI_MASTER_ERROR = lift_shift_bit_num(10),
	TX_OVERFLOW_ERROR = lift_shift_bit_num(11),
	RX_OVERFLOW_ERROR = lift_shift_bit_num(12),
};

/* enable spi */
typedef enum enum_spi_enable {
	SPI_DISABLE = 0,
	SPI_ENABLE = (lift_shift_bit_num(0)),
} spi_enable_e;

/* polarity */
typedef enum enum_spi_polarity {
	SPI_POLARITY_LOW = 0,
	SPI_POLARITY_HIGH = (lift_shift_bit_num(7)),
	SPI_POLARITY_RANGE = (lift_shift_bit_num(7)),
} spi_polarity_e;

typedef enum enum_spi_phase {
	SPI_PHASE_RX_FIRST = 0,
	SPI_PHASE_TX_FIRST = (lift_shift_bit_num(6)),
	SPI_PHASE_RANGE = (lift_shift_bit_num(6)),
} spi_phase_e;

typedef enum enum_spi_format {
	SPI_MOTOROLA_MODE = 0x00,
	SPI_TI_MODE = 0x10,
	SPI_MICROWIRE_MODE = 0x20,
	SPI_FRAME_FORMAT_RANGE = 0x30,
} spi_format_e;

typedef enum enum_spi_data_size {
	SPI_DATA_SIZE_4BIT = 0x03,
	SPI_DATA_SIZE_5BIT = 0x04,
	SPI_DATA_SIZE_6BIT = 0x05,
	SPI_DATA_SIZE_7BIT = 0x06,
	SPI_DATA_SIZE_8BIT = 0x07,
	SPI_DATA_SIZE_9BIT = 0x08,
	SPI_DATA_SIZE_10BIT = 0x09,
	SPI_DATA_SIZE_16BIT = 0x0f,
	SPI_DATA_SIZE_RANGE = 0x0f,
} spi_data_size_e;

typedef enum enum_spi_transfer_mode {
	SPI_TX_RX_MODE = 0x000,
	SPI_ONLY_TX_MODE = 0x100,
	SPI_ONLY_RX_MODE = 0x200,
	SPI_EEPROM_MODE = 0x300,
	SPI_TRANSFER_MODE_RANGE = 0x300,
} spi_transfer_mode_e;

typedef enum enum_spi_baudrate {
	SPI_SCLKIN = 50000000, 
	SPI_SCLKOUT_27000000 = (SPI_SCLKIN / 27000000), //27M
	SPI_SCLKOUT_13500000 = (SPI_SCLKIN / 13500000),  //13.5M
	SPI_SCLKOUT_6750000 = (SPI_SCLKIN / 6750000),  //6.75M
	SPI_SCLKOUT_4500000 = (SPI_SCLKIN / 4500000),    //4.5M
	SPI_SCLKOUT_3375000 = (SPI_SCLKIN / 3375000),  //3.375M
	SPI_SCLKOUT_2700000 = (SPI_SCLKIN / 2700000),    //2.7M
	SPI_SCLKOUT_1500000 = (SPI_SCLKIN / 1500000),  //1.5M
	SPI_SCLKOUT_500000 = (SPI_SCLKIN / 500000),  //0.1M
	SPI_SCLKOUT_100000 = (SPI_SCLKIN / 100000),  //0.1M
} spi_baudrate_e;

typedef enum enum_spi_irq {
	SPI_IRQ_TXEIM = (lift_shift_bit_num(0)),
	SPI_IRQ_TXOIM = (lift_shift_bit_num(1)),
	SPI_IRQ_RXUIM = (lift_shift_bit_num(2)),
	SPI_IRQ_RXOIM = (lift_shift_bit_num(3)),
	SPI_IRQ_RXFIM = (lift_shift_bit_num(4)),
	SPI_IRQ_MSTIM = (lift_shift_bit_num(5)),
	SPI_IRQ_ALL = 0x3f,
} spi_irq_e;

typedef enum enum_spi_slave {
	SPI_SLAVE_PORT0 = (lift_shift_bit_num(0)),
	SPI_SLAVE_PORT1 = (lift_shift_bit_num(1)),
} spi_slave_e;

typedef enum enum_spi_dma_control_mode {
	SPI_DMA_RX_POS = (lift_shift_bit_num(0)),
	SPI_DMA_TX_POS = (lift_shift_bit_num(1)),
	SPI_DMA_CONTROL_RANGE = 0x03,
} spi_dma_control_mode_e;

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
	u32 rev[35];
	u32 rx_sample_dly;
};

#define dw_readl(dw, name) \
__raw_readl(&(((struct fh_spi_reg *)dw->regs)->name))
#define dw_writel(dw, name, val) \
__raw_writel((val), &(((struct fh_spi_reg *)dw->regs)->name))
#define dw_readw(dw, name) \
 __raw_readw(&(((struct fh_spi_reg *)dw->regs)->name))
#define dw_writew(dw, name, val) \
__raw_writew((val), &(((struct fh_spi_reg *)dw->regs)->name))
#define MAX_SG_LEN		32
#define SG_ONE_TIME_MAX_SIZE (4096)
struct _fh_spi_dma_transfer {
	struct dma_chan *chan;
	struct dma_slave_config cfg;
	struct scatterlist sgl[MAX_SG_LEN];
	u32 sgl_data_size[MAX_SG_LEN];
	u32 actual_sgl_size;
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
	u32 apb_clock_in;
	u32 max_freq; /* max bus freq supported */
	u32 div;
	/*use id u32 bus_num;*//*which bus*/
	u32 num_cs; /* supported slave numbers */
	u32 data_width;
	u32 frame_mode;
	u32 transfer_mode;
	u32 active_cs_pin;
	u32 tx_len;
	u32 rx_len;
	void *rx_buff;
	void *tx_buff;
	u32 tx_dma_add;
	u32 rx_dma_add;
	u32 tx_hs_no;
	u32 rx_hs_no;
	u32 tx_dumy_buff[4];
	u32 rx_dumy_buff[4];
	u32 tx_dumy_dma_add;
	u32 rx_dumy_dma_add;
	struct fh_spi_cs cs_data[SPI_MASTER_CONTROLLER_MAX_SLAVE];
	u32 pump_data_mode;
	struct _fh_spi_dma_transfer dma_rx;
	struct _fh_spi_dma_transfer dma_tx;
	u32 complete_times;
	struct fh_spi_platform_data *board_info;
};

struct fh_spi_controller {
	struct device *master_dev;
	struct clk *clk;
	spinlock_t lock;
	struct list_head queue;
	struct platform_device *p_dev;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	struct spi_message *active_message;
	struct spi_transfer *active_transfer;
	struct fh_spi dwc;
	struct completion done;
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

static u32 Spi_Enable(struct fh_spi *dw, spi_enable_e enable);
static u32 Spi_SetPolarity(struct fh_spi *dw, spi_polarity_e polarity);
static u32 Spi_SetPhase(struct fh_spi *dw, spi_phase_e phase);
static u32 Spi_SetFrameFormat(struct fh_spi *dw, spi_format_e format);
static u32 Spi_SetBaudrate(struct fh_spi *dw, spi_baudrate_e baudrate);
static u32 Spi_DisableIrq(struct fh_spi *dw, u32 irq);
static u32 Spi_ReadStatus(struct fh_spi *dw);
static u32 Spi_EnableSlaveen(struct fh_spi *dw, spi_slave_e port);
static u32 Spi_DisableSlaveen(struct fh_spi *dw, spi_slave_e port);
static u32 Spi_EnableIrq(struct fh_spi *dw, u32 irq);
static u32 Spi_SetTxlevlel(struct fh_spi *dw, u32 level);
static u32 Spi_ReadTxfifolevel(struct fh_spi *dw);
static u32 Spi_ReadRxfifolevel(struct fh_spi *dw);
static u32 Spi_WriteData(struct fh_spi *dw, u16 data);
static u16 Spi_ReadData(struct fh_spi *dw);
static u32 Spi_Isrstatus(struct fh_spi *dw);
static void Spi_SetDmaTxDataLevel(struct fh_spi *dw, u32 level);
static void Spi_SetDmaRxDataLevel(struct fh_spi *dw, u32 level);
static void Spi_SetDmaControlEnable(struct fh_spi *dw,
spi_dma_control_mode_e enable_pos);
static bool fh_spi_dma_chan_filter(struct dma_chan *chan, void *param);
static int fh_spi_setup(struct spi_device *spi);
static u32 Spi_SetRxdelay(struct fh_spi *dw, u8 data);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

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

static u32 Spi_SetBaudrate(struct fh_spi *dw, spi_baudrate_e baudrate)
{
	dw_writel(dw, baudr, baudrate);
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
	return (u32) dw_readl(dw, rxflr);
}

static u32 Spi_ReadStatus(struct fh_spi *dw)
{
	return (uint8_t) dw_readl(dw, sr);
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

static u32 Spi_WriteData(struct fh_spi *dw, u16 data)
{
	dw_writew(dw, dr, data);
	return WRITE_ONLY_OK;
}

static u16 Spi_ReadData(struct fh_spi *dw)
{
	return dw_readw(dw, dr);
}

static void Spi_Clearallerror(struct fh_spi *dw)
{
	u32 data = dw_readl(dw, icr);
	data = 0;
}

static u32 Spi_Isrstatus(struct fh_spi *dw)
{
	u32 data = dw_readl(dw, isr);
	return data;
}

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

static void Spi_SetDmaControlDisable(struct fh_spi *dw,
spi_dma_control_mode_e enable_pos)
{
	u32 data;
	data = dw_readl(dw, dmacr);
	data &= ~enable_pos;
	dw_writel(dw, dmacr, data);
}

static u32 Spi_SetRxdelay(struct fh_spi *dw, u8 data)
{
	dw_writel(dw, rx_sample_dly, data);
	return CONFIG_OK;
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

static int fh_spi_init_hw(struct fh_spi_controller *fh_spi,
struct fh_spi_platform_data *board_info)
{
	int status, i;

	fh_spi->dwc.id = board_info->bus_no;

	fh_spi->dwc.fifo_len = board_info->fifo_len;
	fh_spi->dwc.num_cs = board_info->slave_max_num;
	for (i = 0; i < fh_spi->dwc.num_cs; i++) {
		fh_spi->dwc.cs_data[i].GPIO_Pin =
		                board_info->cs_data[i].GPIO_Pin;
		fh_spi->dwc.cs_data[i].name = board_info->cs_data[i].name;
	}

	fh_spi->dwc.rx_hs_no = board_info->rx_handshake_num;
	fh_spi->dwc.tx_hs_no = board_info->tx_handshake_num;
	memset(&fh_spi->dwc.dma_rx, 0, sizeof(struct _fh_spi_dma_transfer));
	memset(&fh_spi->dwc.dma_tx, 0, sizeof(struct _fh_spi_dma_transfer));
	fh_spi->dwc.complete_times = 0;
	fh_spi->dwc.pump_data_mode = PUMP_DATA_POLL_MODE;
	/* bind the platform data here.... */
	fh_spi->dwc.board_info = board_info;

	fh_spi->dwc.isr_flag = SPI_IRQ_TXEIM | SPI_IRQ_TXOIM | SPI_IRQ_RXUIM
	                | SPI_IRQ_RXOIM;
	fh_spi->dwc.frame_mode = SPI_MOTOROLA_MODE;
	fh_spi->dwc.transfer_mode = SPI_TX_RX_MODE;

	do {
		status = Spi_ReadStatus(&fh_spi->dwc);
	} while (status & 0x01);
	/* add spi disable */
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	/* add spi frame mode & transfer mode */
	Spi_SetFrameFormat(&fh_spi->dwc, fh_spi->dwc.frame_mode);
	Spi_SetTransferMode(&fh_spi->dwc, fh_spi->dwc.transfer_mode);
	/* add spi disable all isr */
	Spi_DisableIrq(&fh_spi->dwc, SPI_IRQ_ALL);
	/* add spi enable */
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
	return 0;
}

static irqreturn_t fh_spi_irq(int irq, void *dev_id)
{
	u8* txbuf;
	struct fh_spi_controller *fh_spi;
	u32 isr_status;
	u32 rx_fifo_capability, tx_fifo_capability;
	u16 data;
	unsigned size;
	fh_spi = (struct fh_spi_controller *) dev_id;
	data = 0x00;
	txbuf = (u8*) fh_spi->dwc.tx_buff;
	isr_status = Spi_Isrstatus(&fh_spi->dwc);
	/* this transfer total size. */
	size = fh_spi->active_transfer->len;
	if (isr_status & (SPI_IRQ_TXOIM | SPI_IRQ_RXUIM | SPI_IRQ_RXOIM)) {
		Spi_Clearallerror(&fh_spi->dwc);
		/* error handle */
		dev_err(&fh_spi->p_dev->dev, "spi isr status:%x\n", isr_status);
		WARN_ON(1);
	}

	Spi_DisableIrq(&fh_spi->dwc, fh_spi->dwc.isr_flag);

	if (fh_spi->dwc.transfer_mode == SPI_TX_RX_MODE) {

		tx_fifo_capability = tx_max(fh_spi);
		rx_fifo_capability = rx_max(fh_spi);
		fh_spi->dwc.rx_len += rx_fifo_capability;
		while (rx_fifo_capability) {
			data = Spi_ReadData(&fh_spi->dwc);
			if (fh_spi->dwc.rx_buff != NULL) {
				*(u8 *) fh_spi->dwc.rx_buff++ = (u8) data;
			}
			rx_fifo_capability--;
		}

		if (fh_spi->dwc.rx_len == size) {
			complete(&(fh_spi->done));
			return IRQ_HANDLED;
		}

		fh_spi->dwc.tx_len -= tx_fifo_capability;
		while (tx_fifo_capability) {
			data = 0x0;

			if (fh_spi->dwc.tx_buff != NULL) {
				data = *(u8*) fh_spi->dwc.tx_buff++;
			}
			Spi_WriteData(&fh_spi->dwc, data);
			tx_fifo_capability--;
		}
		Spi_EnableIrq(&fh_spi->dwc, fh_spi->dwc.isr_flag);

	} else if (fh_spi->dwc.transfer_mode == SPI_ONLY_TX_MODE) {
		tx_fifo_capability = tx_max(fh_spi);

		fh_spi->dwc.tx_len -= tx_fifo_capability;
		while (tx_fifo_capability) {
			/* data = 0x0; */
			Spi_WriteData(&fh_spi->dwc, *txbuf++);
			fh_spi->dwc.tx_buff++;
			tx_fifo_capability--;
		}
		if (fh_spi->dwc.tx_len == 0) {
			complete(&(fh_spi->done));
			return IRQ_HANDLED;
		}
		/* reopen tx isr... */
		Spi_EnableIrq(&fh_spi->dwc, fh_spi->dwc.isr_flag);
	}
	return IRQ_HANDLED;
}

static int fh_spi_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	u8 bits_per_word;
	u32 hz;
	u32 div;
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
	if (t && t->speed_hz)
		hz = min(hz, t->speed_hz);

	div = fh_spi->dwc.apb_clock_in / hz;
	fh_spi->dwc.div = div;

	if (hz == 0) {
		dev_err(&spi->dev, "Cannot continue with zero clock\n");
		WARN_ON(1);
		return -EINVAL;
	}

	if (bits_per_word != 8) {
		dev_err(&spi->dev, "%s, unsupported bits_per_word=%d\n",
		                __func__, bits_per_word);
		return -EINVAL;
	}

	if (spi->mode & SPI_CPOL)
		fh_spi->dwc.cpol = SPI_POLARITY_HIGH;
	else
		fh_spi->dwc.cpol = SPI_POLARITY_LOW;

	if (spi->mode & SPI_CPHA)
		fh_spi->dwc.cpha = SPI_PHASE_TX_FIRST;
	else
		fh_spi->dwc.cpha = SPI_PHASE_RX_FIRST;

	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetPolarity(&fh_spi->dwc, fh_spi->dwc.cpol);
	Spi_SetPhase(&fh_spi->dwc, fh_spi->dwc.cpha);
	Spi_SetBaudrate(&fh_spi->dwc, fh_spi->dwc.div);
	Spi_SetRxdelay(&fh_spi->dwc, 1);
	Spi_DisableIrq(&fh_spi->dwc, SPI_IRQ_ALL);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
	fh_spi_setup(spi);
	return 0;
}

static int isr_pump_data(struct fh_spi_controller *fh_spi)
{
	u32 status;
	/* first clear isr... */
	fh_spi->dwc.isr_flag &= ~(SPI_IRQ_TXEIM | SPI_IRQ_RXFIM);
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);

	if ((fh_spi->dwc.rx_buff == NULL) && (fh_spi->dwc.tx_buff != NULL)) {

		fh_spi->dwc.isr_flag |= SPI_IRQ_TXEIM;
		Spi_SetTxlevlel(&fh_spi->dwc, fh_spi->dwc.fifo_len - 1);
		Spi_EnableIrq(&fh_spi->dwc, fh_spi->dwc.isr_flag);
		fh_spi->dwc.transfer_mode = SPI_ONLY_TX_MODE;

	} else {
		/* tx & rx.. */
		fh_spi->dwc.isr_flag |= SPI_IRQ_TXEIM;
		Spi_SetTxlevlel(&fh_spi->dwc, fh_spi->dwc.fifo_len - 1);
		Spi_EnableIrq(&fh_spi->dwc, fh_spi->dwc.isr_flag);
		fh_spi->dwc.transfer_mode = SPI_TX_RX_MODE;
	}

	Spi_SetTransferMode(&fh_spi->dwc, fh_spi->dwc.transfer_mode);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
	wait_for_completion(&fh_spi->done);
	/* add wait spi idle.. */
	do {
		status = Spi_ReadStatus(&fh_spi->dwc);
	} while (status & 0x01);

	return 0;
}

static bool fh_spi_dma_chan_filter(struct dma_chan *chan, void *param)
{
	int dma_channel = *(int *) param;
	bool ret = false;

	if (chan->chan_id == dma_channel) {
		ret = true;
	}
	return ret;
}

static void fh_spi_rx_dma_done(void *arg)
{
	struct fh_spi_controller *fh_spi = (struct fh_spi_controller *) arg;
	fh_spi->dwc.complete_times++;
	if (fh_spi->dwc.complete_times == 2) {
		fh_spi->dwc.complete_times = 0;
		complete(&(fh_spi->done));
	}
}



static int dma_set_tx_para(struct fh_spi_controller *fh_spi)
{
	struct fh_dma_pri fh_pri;
	struct dma_slave_config *tx_config;
	struct spi_transfer *t;
	struct dma_chan *txchan;
	struct scatterlist *p_sca_list;
	unsigned int sg_size = 0;
	int i, xfer_len, one_sg_data_len;
	unsigned char *temp_buf;
	t = fh_spi->active_transfer;
	memset(&fh_spi->dwc.dma_tx.cfg, 0, sizeof(struct dma_slave_config));
	txchan = fh_spi->dwc.dma_tx.chan;
	tx_config = &fh_spi->dwc.dma_tx.cfg;
	tx_config->dst_addr = fh_spi->dwc.paddr + SPI_DATA_REG_OFFSET;
	/* set the spi data tx reg */
	tx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	tx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	tx_config->slave_id = fh_spi->dwc.tx_hs_no;
	tx_config->src_maxburst = 4;
	tx_config->dst_maxburst = 4;
	tx_config->direction = DMA_MEM_TO_DEV;
	tx_config->device_fc = FALSE;
	xfer_len = t->len;
	temp_buf = (unsigned char*)t->tx_buf;
	sg_size =  t->len /SG_ONE_TIME_MAX_SIZE; 
	if(t->len % SG_ONE_TIME_MAX_SIZE){
		sg_size++;
	}
	if(sg_size > MAX_SG_LEN) {
		printk("%s_%d :: too large sg size:0x%x\n",__func__,__LINE__,sg_size);
		return -1;
	}
	p_sca_list = &fh_spi->dwc.dma_tx.sgl[0];
	for(i=0;i<sg_size;i++,p_sca_list++){
		one_sg_data_len = min(xfer_len,SG_ONE_TIME_MAX_SIZE);
		xfer_len -=one_sg_data_len;

		if (t->tx_buf == NULL){
			fh_pri.sinc = FH_DMA_SLAVE_FIX;
			p_sca_list->dma_address =  fh_spi->dwc.tx_dumy_dma_add;
		}
		else{
			fh_pri.sinc = FH_DMA_SLAVE_INC;
			p_sca_list->dma_address = dma_map_single(
		                txchan->dev->device.parent,
		                (void*) temp_buf, one_sg_data_len,
		                DMA_TO_DEVICE);
			fh_spi->dwc.dma_tx.sgl_data_size[i] = one_sg_data_len;
			temp_buf += one_sg_data_len;

		}
		p_sca_list->length = one_sg_data_len;
	}
	fh_pri.dinc = FH_DMA_SLAVE_FIX;
	dmaengine_slave_config(txchan, tx_config);

	fh_spi->dwc.dma_tx.desc = txchan->device->device_prep_slave_sg(txchan,
				&fh_spi->dwc.dma_tx.sgl[0], sg_size, DMA_MEM_TO_DEV,
				DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_DEST_UNMAP,
				&fh_pri);
	fh_spi->dwc.dma_tx.actual_sgl_size = sg_size;
	fh_spi->dwc.dma_tx.desc->callback = fh_spi_rx_dma_done;
	fh_spi->dwc.dma_tx.desc->callback_param = fh_spi;
	return 0;

}

static int dma_set_rx_para(struct fh_spi_controller *fh_spi)
{

	struct fh_dma_pri fh_pri;
	struct dma_slave_config *rx_config;
	struct spi_transfer *t;
	struct dma_chan *rxchan;
	struct scatterlist *p_sca_list;
	unsigned int sg_size = 0;
	int i, xfer_len, one_sg_data_len;
	unsigned char *temp_buf;

	t = fh_spi->active_transfer;
	rxchan = fh_spi->dwc.dma_rx.chan;
	memset(&fh_spi->dwc.dma_rx.cfg, 0, sizeof(struct dma_slave_config));
	rx_config = &fh_spi->dwc.dma_rx.cfg;
	rx_config->src_addr = fh_spi->dwc.paddr + SPI_DATA_REG_OFFSET;
	rx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_config->slave_id = fh_spi->dwc.rx_hs_no;
	rx_config->src_maxburst = 4;
	rx_config->dst_maxburst = 4;
	rx_config->device_fc = FALSE;
	rx_config->direction = DMA_DEV_TO_MEM;
	xfer_len = t->len;
	temp_buf = (unsigned char*)t->rx_buf;
	sg_size =  t->len /SG_ONE_TIME_MAX_SIZE; 
	if(t->len % SG_ONE_TIME_MAX_SIZE){
		sg_size++;
	}
	if(sg_size > MAX_SG_LEN) {
		printk("%s_%d :: too large sg size:0x%x\n",__func__,__LINE__,sg_size);
		return -1;
	}
	p_sca_list = &fh_spi->dwc.dma_rx.sgl[0];
	for(i=0;i<sg_size;i++,p_sca_list++){
		one_sg_data_len = min(xfer_len,SG_ONE_TIME_MAX_SIZE);
		xfer_len -=one_sg_data_len;
		if (t->rx_buf == NULL){
			fh_pri.dinc = FH_DMA_SLAVE_FIX;
			p_sca_list->dma_address =  fh_spi->dwc.rx_dumy_dma_add;
		}
		else{
			fh_pri.dinc = FH_DMA_SLAVE_INC;
			p_sca_list->dma_address = dma_map_single(
		                rxchan->dev->device.parent,
		                (void*) temp_buf, one_sg_data_len,
		                DMA_FROM_DEVICE);
			fh_spi->dwc.dma_rx.sgl_data_size[i] = one_sg_data_len;
			temp_buf += one_sg_data_len;
		}
		p_sca_list->length = one_sg_data_len;
	}

	fh_pri.sinc = FH_DMA_SLAVE_FIX;
	dmaengine_slave_config(rxchan, rx_config);
	fh_spi->dwc.dma_rx.desc = rxchan->device->device_prep_slave_sg(rxchan,
	                &fh_spi->dwc.dma_rx.sgl[0], sg_size, DMA_DEV_TO_MEM,
	                DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_DEST_UNMAP,
	                &fh_pri);
	fh_spi->dwc.dma_rx.actual_sgl_size = sg_size;
	fh_spi->dwc.dma_rx.desc->callback = fh_spi_rx_dma_done;
	fh_spi->dwc.dma_rx.desc->callback_param = fh_spi;

	return 0;

}

void unmap_dma_tx_sg(struct fh_spi_controller *fh_spi){
	struct dma_chan *txchan;
	struct scatterlist *p_sca_list;
	int i;
	txchan = fh_spi->dwc.dma_tx.chan;
	p_sca_list = &fh_spi->dwc.dma_tx.sgl[0];
	for(i=0;i<fh_spi->dwc.dma_tx.actual_sgl_size;i++,p_sca_list++){
		dma_unmap_single(txchan->dev->device.parent,
				p_sca_list->dma_address,
				fh_spi->dwc.dma_tx.sgl_data_size[i], DMA_MEM_TO_DEV);
	}
}

void unmap_dma_rx_sg(struct fh_spi_controller *fh_spi){
	struct dma_chan *rxchan;
	struct scatterlist *p_sca_list;
	int i;
	
	rxchan = fh_spi->dwc.dma_rx.chan;
	p_sca_list = &fh_spi->dwc.dma_rx.sgl[0];
	for(i=0;i<fh_spi->dwc.dma_rx.actual_sgl_size;i++,p_sca_list++){
		dma_unmap_single(rxchan->dev->device.parent,
				p_sca_list->dma_address,
				fh_spi->dwc.dma_rx.sgl_data_size[i], DMA_FROM_DEVICE);
	}
}


static int dma_pump_data(struct fh_spi_controller *fh_spi)
{
	struct spi_transfer *t;
	struct dma_chan *rxchan;
	struct dma_chan *txchan;
	int ret;
	t = fh_spi->active_transfer;
	txchan = fh_spi->dwc.dma_tx.chan;
	rxchan = fh_spi->dwc.dma_rx.chan;
	init_completion(&fh_spi->done);
	ret = dma_set_tx_para(fh_spi);
	if (ret != 0) {
		return ret;
	}
	ret = dma_set_rx_para(fh_spi);
	if (ret != 0) {
		unmap_dma_tx_sg(fh_spi);
		return ret;
	}
	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	Spi_SetDmaRxDataLevel(&fh_spi->dwc, 3);
	Spi_SetDmaControlEnable(&fh_spi->dwc, SPI_DMA_RX_POS);
	Spi_SetDmaTxDataLevel(&fh_spi->dwc, 4);
	Spi_SetDmaControlEnable(&fh_spi->dwc, SPI_DMA_TX_POS);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);
	fh_spi->dwc.dma_rx.desc->tx_submit(fh_spi->dwc.dma_rx.desc);
	fh_spi->dwc.dma_tx.desc->tx_submit(fh_spi->dwc.dma_tx.desc);
	wait_for_completion(&fh_spi->done);
	if (t->tx_buf != NULL) {
		unmap_dma_tx_sg(fh_spi);
	}
	if (t->rx_buf != NULL) {
		unmap_dma_rx_sg(fh_spi);
	}
	Spi_SetDmaControlDisable(&fh_spi->dwc, SPI_DMA_RX_POS);
	Spi_SetDmaControlDisable(&fh_spi->dwc, SPI_DMA_TX_POS);
	return 0;

}

static int poll_pump_data(struct fh_spi_controller *fh_spi)
{
	register u32 rx_fifo_capability, tx_fifo_capability;
	u8* txbuf;
	u8* rxbuf;
	u16 data;
	u32 size;
	rxbuf = (u8*) fh_spi->dwc.rx_buff;
	txbuf = (u8*) fh_spi->dwc.tx_buff;
	size = fh_spi->active_transfer->len;

	Spi_Enable(&fh_spi->dwc, SPI_DISABLE);
	fh_spi->dwc.transfer_mode = SPI_TX_RX_MODE;
	Spi_SetTransferMode(&fh_spi->dwc, fh_spi->dwc.transfer_mode);
	Spi_Enable(&fh_spi->dwc, SPI_ENABLE);

	goto first;
	start: rx_fifo_capability = rx_max(fh_spi);
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

	if (fh_spi->dwc.rx_len == size) {
		return 0;
	}

	first: tx_fifo_capability = tx_max(fh_spi);
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

	return 0;
}

static int fh_spi_handle_message(struct fh_spi_controller *fh_spi,
                struct spi_message *m)
{
	bool first, last;
	struct spi_transfer *t, *tmp_t;
	int status = 0;
	int cs_change;
	cs_change = 1;
	m->actual_length = 0;
	fh_spi->active_message = m;

	list_for_each_entry_safe(t, tmp_t, &m->transfers, transfer_list)
	{
		first = (&t->transfer_list == m->transfers.next);
		last = (&t->transfer_list == m->transfers.prev);

		if (first || t->speed_hz || t->bits_per_word)
		fh_spi_setup_transfer(m->spi, t);

		if (cs_change) {
			Spi_EnableSlaveen(&fh_spi->dwc, fh_spi->dwc.slave_port);
		}
		cs_change = t->cs_change;

		fh_spi->active_transfer = t;
		fh_spi->dwc.tx_len = t->len;
		fh_spi->dwc.rx_len = 0;
		fh_spi->dwc.tx_buff = (void *) t->tx_buf;
		fh_spi->dwc.rx_buff = t->rx_buf;

		if (fh_spi->dwc.board_info->dma_transfer_enable
				== SPI_TRANSFER_USE_DMA) {

			if(fh_spi->dwc.tx_len < DMA_TRANS_GATE_LEVEL) {
				fh_spi->dwc.pump_data_mode = PUMP_DATA_POLL_MODE;
				goto pump_data;
			}
			fh_spi->dwc.pump_data_mode = PUMP_DATA_DMA_MODE;
		}
		else {
			fh_spi->dwc.pump_data_mode = PUMP_DATA_POLL_MODE;
		}

		pump_data:

		switch (fh_spi->dwc.pump_data_mode) {
			case PUMP_DATA_DMA_MODE:
			status = dma_pump_data(fh_spi);
			if (status == 0) {
				break;
			} else {
				WARN_ON(1);
				dev_err(&fh_spi->p_dev->dev,
						"spi dma pump data error\n");
				fh_spi->dwc.pump_data_mode =
				PUMP_DATA_POLL_MODE;
			}

			case PUMP_DATA_ISR_MODE:
			status = isr_pump_data(fh_spi);
			break;

			case PUMP_DATA_POLL_MODE:
			status = poll_pump_data(fh_spi);
			break;
			default:
			status = -1;
			WARN_ON(1);
			dev_err(&fh_spi->p_dev->dev,
					"spi pump data mode error..\n");
		}

		if (!cs_change && last) {
			Spi_DisableSlaveen(&fh_spi->dwc,
					fh_spi->dwc.slave_port);
		}
/* 
*      if (t->delay_usecs)
*		udelay(t->delay_usecs);
*/
		m->actual_length += t->len;
		if (status)
			break;

	}

	return status;

}

static void fh_spi_handle(struct work_struct *w)
{
	struct fh_spi_controller
	*fh_spi = container_of(w, struct fh_spi_controller, work);
	unsigned long flags;
	struct spi_message *m;
	spin_lock_irqsave(&fh_spi->lock, flags);
	while (!list_empty(&fh_spi->queue)) {
		m = list_entry(fh_spi->queue.next, struct spi_message, queue);
		list_del_init(&m->queue);
		spin_unlock_irqrestore(&fh_spi->lock, flags);
		m->status = fh_spi_handle_message(fh_spi, m);
		if (m->complete)
			m->complete(m->context);

		spin_lock_irqsave(&fh_spi->lock, flags);
	}
	spin_unlock_irqrestore(&fh_spi->lock, flags);

}

static int fh_spi_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct fh_spi_controller *fh_spi = spi_master_get_devdata(spi->master);
	unsigned long flags;
	m->status = -EINPROGRESS;
	spin_lock_irqsave(&fh_spi->lock, flags);
	list_add_tail(&m->queue, &fh_spi->queue);
	spin_unlock_irqrestore(&fh_spi->lock, flags);
	queue_work(fh_spi->workqueue, &fh_spi->work);
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

static void fh_spi_cleanup(struct spi_device *spi)
{

}

static int __devinit fh_spi_probe(struct platform_device *dev)
{
	int err = 0;
	struct spi_master *master;
	struct fh_spi_controller *fh_spi;
	struct resource *r, *ioarea;
	int ret, i, j;
	dma_cap_mask_t mask;
	int filter_no;
	struct fh_spi_platform_data * spi_platform_info;

	spi_platform_info =
	                (struct fh_spi_platform_data *) dev->dev.platform_data;
	if (spi_platform_info == NULL) {

		err = -ENODEV;
		goto out0;
	}

	master = spi_alloc_master(&dev->dev, sizeof(struct fh_spi_controller));
	if (master == NULL) {
		err = -ENOMEM;
		dev_err(&dev->dev, "%s, master malloc failed.\n", __func__);
		goto out0;
	}

	fh_spi = spi_master_get_devdata(master);
	if (!fh_spi) {
		dev_err(&dev->dev, "%s, master dev data is null.\n", __func__);
		err = -ENOMEM;
		goto out_put_master;
	}
	fh_spi->master_dev = &dev->dev;
	fh_spi->p_dev = dev;
	platform_set_drvdata(dev, master);

	fh_spi->dwc.irq = platform_get_irq(dev, 0);
	if (fh_spi->dwc.irq < 0) {
		dev_err(&dev->dev, "%s, spi irq no error.\n", __func__);
		err = fh_spi->dwc.irq;
		goto out_set_plat_drvdata_null;
	}

	err = request_irq(fh_spi->dwc.irq, fh_spi_irq, 0, dev_name(&dev->dev),
	                fh_spi);
	if (err) {
		dev_dbg(&dev->dev, "request_irq failed, %d\n", err);
		goto out_set_plat_drvdata_null;
	}

	/* Get resources(memory, IRQ) associated with the device */
	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&dev->dev, "%s, spi ioresource error. \n", __func__);
		err = -ENODEV;
		goto out_free_irq;
	}

	fh_spi->dwc.paddr = r->start;
	ioarea = request_mem_region(r->start, resource_size(r), dev->name);
	if (!ioarea) {
		dev_err(&dev->dev, "spi region already claimed\n");
		err = -EBUSY;
		goto out_free_irq;
	}

	fh_spi->dwc.regs = ioremap(r->start, resource_size(r));
	if (!fh_spi->dwc.regs) {
		dev_err(&dev->dev, "spi region already mapped\n");
		err = -EINVAL;
		goto out_relase_mem_region;
	}

	INIT_WORK(&fh_spi->work, fh_spi_handle);
	init_completion(&fh_spi->done);
	INIT_LIST_HEAD(&fh_spi->queue);
	spin_lock_init(&fh_spi->lock);

	fh_spi->workqueue = create_singlethread_workqueue(dev_name(&dev->dev));
	if (!fh_spi->workqueue) {
		err = -ENXIO;
		goto out_iounmap;
	}
	master->transfer = fh_spi_transfer;
	master->setup = fh_spi_setup;
	master->cleanup = fh_spi_cleanup;

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA;
	master->bus_num = dev->id;
	spi_platform_info->bus_no = dev->id;

	master->num_chipselect = spi_platform_info->slave_max_num;
	/* parse the controller board info~~~ */
	/* clk enable in the func */
	ret = fh_spi_init_hw(fh_spi, spi_platform_info);
	if (ret) {
		err = ret;
		goto out_destroy_queue;
	}
	fh_spi->clk = clk_get(&fh_spi->p_dev->dev, spi_platform_info->clk_name);

	if (IS_ERR(fh_spi->clk)) {
		dev_err(&fh_spi->p_dev->dev, "cannot find the spi%d clk.\n",
		                fh_spi->dwc.id);
		err = PTR_ERR(fh_spi->clk);
		goto out_destroy_queue;
	}

	clk_enable(fh_spi->clk);
	fh_spi->dwc.apb_clock_in = clk_get_rate(fh_spi->clk);
	if (spi_platform_info->apb_clock_in > fh_spi->dwc.apb_clock_in) {
		clk_set_rate(fh_spi->clk, spi_platform_info->apb_clock_in);
		fh_spi->dwc.apb_clock_in = spi_platform_info->apb_clock_in;
	}
	fh_spi->dwc.max_freq = fh_spi->dwc.apb_clock_in / 2;

	/* request the cs gpio */
	for (i = 0; i < fh_spi->dwc.num_cs; i++) {
		ret = gpio_request(fh_spi->dwc.cs_data[i].GPIO_Pin,
		                fh_spi->dwc.cs_data[i].name);
		if (ret) {
			dev_err(&dev->dev,
			                "spi failed to request the gpio:%d\n",
			                fh_spi->dwc.cs_data[i].GPIO_Pin);
			/* release the gpio already request.. */
			if (i != 0) {
				for (j = 0; j < i; j++) {
					gpio_free(
					                fh_spi->dwc.cs_data[j].GPIO_Pin);
				}
			}
			err = ret;
			/* clk disable */
			goto out_clk_disable;
		}
		/* set the dir */
		gpio_direction_output(fh_spi->dwc.cs_data[i].GPIO_Pin,
		                GPIOF_OUT_INIT_HIGH);
	}
	/* fix:need use the platform dma channel.. not 0 and 1.... */
	if (fh_spi->dwc.board_info->dma_transfer_enable == SPI_TRANSFER_USE_DMA) {
		filter_no = fh_spi->dwc.board_info->tx_dma_channel;
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		fh_spi->dwc.dma_tx.chan = dma_request_channel(mask,
		                fh_spi_dma_chan_filter, &filter_no);

		if (!fh_spi->dwc.dma_tx.chan) {
			dev_err(&fh_spi->p_dev->dev,
			                "spi%d request dma channel error....\n",
			                fh_spi->dwc.id);
			fh_spi->dwc.board_info->dma_transfer_enable = 0;
			goto step_register_master;
		}
		filter_no = fh_spi->dwc.board_info->rx_dma_channel;
		fh_spi->dwc.dma_rx.chan = dma_request_channel(mask,
		                fh_spi_dma_chan_filter, &filter_no);

		if (!fh_spi->dwc.dma_rx.chan) {
			dev_err(&fh_spi->p_dev->dev,
			                "spi%d request dma channel error....\n",
			                fh_spi->dwc.id);
			dma_release_channel(fh_spi->dwc.dma_tx.chan);
			fh_spi->dwc.board_info->dma_transfer_enable = 0;
			goto step_register_master;
		}

		fh_spi->dwc.tx_dumy_dma_add = dma_map_single(
		                fh_spi->dwc.dma_tx.chan->dev->device.parent,
		                (void*) fh_spi->dwc.tx_dumy_buff,
		                sizeof(fh_spi->dwc.tx_dumy_buff),
		                DMA_TO_DEVICE);

		fh_spi->dwc.rx_dumy_dma_add = dma_map_single(
		                fh_spi->dwc.dma_rx.chan->dev->device.parent,
		                (void*) fh_spi->dwc.rx_dumy_buff,
		                sizeof(fh_spi->dwc.rx_dumy_buff),
		                DMA_TO_DEVICE);

	}

step_register_master:
	err = spi_register_master(master);
	if (err) {
		dev_dbg(&dev->dev, "cannot register spi master, %d\n", err);
		goto out_gpio_tree;
	}

	return 0;

out_gpio_tree:
	for (i = 0; i < fh_spi->dwc.num_cs; i++) {
		gpio_free(fh_spi->dwc.cs_data[i].GPIO_Pin);
	}
out_clk_disable:
	clk_disable(fh_spi->clk);
out_destroy_queue:
	destroy_workqueue(fh_spi->workqueue);
out_iounmap:
	iounmap(fh_spi->dwc.regs);
out_relase_mem_region:
	release_mem_region(r->start, resource_size(r));
out_free_irq:
	free_irq(fh_spi->dwc.irq, fh_spi);
out_set_plat_drvdata_null:
	memset(fh_spi, 0,sizeof(struct fh_spi_controller));
	platform_set_drvdata(dev, NULL);
out_put_master:
	spi_master_put(master);
out0:
	return err;

}

static int __devexit fh_spi_remove(struct platform_device *dev)
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
	for(i = 0;i<fh_spi->dwc.num_cs;i++) {
		gpio_free(fh_spi->dwc.cs_data[i].GPIO_Pin);
	}
	clk_disable(fh_spi->clk);
	if(fh_spi->dwc.pump_data_mode == PUMP_DATA_DMA_MODE) {
		if(fh_spi->dwc.dma_rx.chan) {
			dma_release_channel(fh_spi->dwc.dma_rx.chan);
			fh_spi->dwc.dma_rx.chan->private = NULL;
		}
		if(fh_spi->dwc.dma_tx.chan) {
			dma_release_channel(fh_spi->dwc.dma_tx.chan);
			fh_spi->dwc.dma_tx.chan->private = NULL;
		}
	}
	destroy_workqueue(fh_spi->workqueue);
	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	iounmap(fh_spi->dwc.regs);
	release_mem_region(r->start, resource_size(r));
	free_irq(fh_spi->dwc.irq, fh_spi);
	memset(fh_spi,0,sizeof(struct fh_spi_controller));
	platform_set_drvdata(dev, NULL);
	spi_master_put(master);

out0:
	return 0;

}

static struct platform_driver fh_spi_driver = {
		.probe = fh_spi_probe,
		.remove = __devexit_p(fh_spi_remove),
		.driver = {
				.name = "fh_spi",
				.owner = THIS_MODULE,
		},
		.suspend = NULL,
		.resume = NULL,
};

static int __init fh_spi_init(void)
{
	return platform_driver_register(&fh_spi_driver);
}

static void __exit fh_spi_exit(void)
{
	platform_driver_unregister(&fh_spi_driver);
}

module_init(fh_spi_init);
module_exit(fh_spi_exit);
MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_DESCRIPTION("DUOBAO SPI driver");
MODULE_LICENSE("GPL");

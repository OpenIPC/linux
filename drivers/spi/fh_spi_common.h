#ifndef __FH_SPI_COMMON_H__
#define __FH_SPI_COMMON_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/in.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/irqreturn.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <linux/crc32.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/fh_dmac.h>
#include <mach/fh_dma_plat.h>
#include <mach/fh_spi_plat.h>

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define SPI_DEFAULT_CLK         (100000000)

#define lift_shift_bit_num(bit_num)     (1<<bit_num)
//read spi irq, only useful if you set which is masked
#define SPI_IRQ_TXEIS           (lift_shift_bit_num(0))
#define SPI_IRQ_TXOIS           (lift_shift_bit_num(1))
#define SPI_IRQ_RXUIS           (lift_shift_bit_num(2))
#define SPI_IRQ_RXOIS           (lift_shift_bit_num(3))
#define SPI_IRQ_RXFIS           (lift_shift_bit_num(4))
#define SPI_IRQ_MSTIS           (lift_shift_bit_num(5))
//spi status
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
#define SPI_RX_ONLY_ONE_TIME_SIZE	(0x10000)
#define MAX_SG_LEN		128
#if (SPI_RX_ONLY_ONE_TIME_SIZE > 2048)
#define SG_ONE_TIME_MAX_SIZE 2048
#else
#define SG_ONE_TIME_MAX_SIZE SPI_RX_ONLY_ONE_TIME_SIZE
#endif

#define SPI_DATA_DIR_IN		(0xaa)
#define SPI_DATA_DIR_OUT	(0xbb)
#define SPI_DATA_DIR_DUOLEX		(0xcc)
#define RX_FIFO_MAX_LEN 256
#define RX_FIFO_MIN_LEN 2
/****************************************************************************
 * ADT section
 * add definition of user defined Data Type
 * that only be used in this file here
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
	/* if read/write/eeprom error,the error below
	 * could give you more info by reading the
	 * 'Spi_ReadTransferError' function
	 */
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

//slave control
typedef enum enum_spi_slave_mode {
	SPI_SLAVE_EN = 0x00,
	SPI_SLAVE_MODE_RANGE = 1 << 10,
} spi_slave_mode_e;

/*read wire mode*/
typedef enum enum_spi_read_wire_mode {
	STANDARD_READ = 0x00,
	DUAL_OUTPUT   = 0x01,
	DUAL_IO       = 0x02,
	QUAD_OUTPUT   = 0x03,
	QUAD_IO       = 0x04,
} spi_read_wire_mode_e;

/*program wire mode*/
typedef enum enum_spi_prog_wire_mode {
	STANDARD_PROG = 0x00,
	QUAD_INPUT    = 0x01,
} spi_prog_wire_mode_e;

/*ahb Xip config*/
typedef enum enum_spi_xip_config {
	XIP_DISABLE = 0,
	XIP_ENABLE  = 1,
} spi_xip_config_e;

/*ahb DPI config*/
typedef enum enum_spi_dpi_config {
	DPI_DISABLE = 0,
	DPI_ENABLE  = 1,
} spi_dpi_config_e;

/*ahb QPI config*/
typedef enum enum_spi_qpi_config {
	QPI_DISABLE = 0,
	QPI_ENABLE  = 1,
} spi_qpi_config_e;

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
	u32 ccfgr;
	u32 opcr;
	u32 timcr;
	u32 bbar0;
	u32 bbar1;
};

#define dw_readl(dw, name) \
    __raw_readl(&(((struct fh_spi_reg *)dw->regs)->name))
#define dw_writel(dw, name, val) \
    __raw_writel((val), &(((struct fh_spi_reg *)dw->regs)->name))
#define dw_readw(dw, name) \
    __raw_readw(&(((struct fh_spi_reg *)dw->regs)->name))
#define dw_writew(dw, name, val) \
    __raw_writew((val), &(((struct fh_spi_reg *)dw->regs)->name))
struct fh_spi_controller;
#ifdef CONFIG_SPI_USE_MULTI_WIRE
struct fh_spi_multi {
	/*add multi*/
	u32 max_wire_size;
	u32 swap_max_size;
	u32 active_wire_width;
	u32 dir;
	void (*fix_wire_mode)(struct fh_spi_controller *fh_spi);
	void (*change_1_wire)(struct spi_master *p_master);
	void (*change_2_wire)(struct spi_master *p_master, u32 dir);
	void (*change_4_wire)(struct spi_master *p_master, u32 dir);
	void (*swap_data_width)(struct fh_spi_controller *fh_spi, u32 reg_width);
};
void spic_multi_wire_init(struct spi_master * p_master);
#endif

#ifdef CONFIG_SPI_USE_DMA
struct fh_spi_dma_channel_para {
	u32 hs_no;
	struct dma_chan *chan;
	struct dma_slave_config cfg;
	struct scatterlist sgl[MAX_SG_LEN];
	u32 sgl_data_size[MAX_SG_LEN];
	u32 actual_sgl_size;
	struct dma_async_tx_descriptor *desc;
	u32 complete_times;
};

struct fh_spi_dma {
	/*u32 dumy_buff[4];*/
	struct fh_spi_dma_channel_para rx;
	struct completion done;
	int (*dma_read)(struct fh_spi_controller *fh_spi);
	int (*dma_write)(struct fh_spi_controller *fh_spi);
	int (*dma_write_read)(struct fh_spi_controller *fh_spi);
};
void fh_spi_dma_init(struct fh_spi_controller *fh_spi);
#endif

struct fh_spi {
	void * __iomem regs; /* vaddr of the control registers */
	u32 id;
	u32 irq;
	u32 paddr;
	u32 slave_port;
	u32 fifo_len; /* depth of the FIFO buffer */
	u32 max_freq; /* max bus freq supported */

	/*use id u32 bus_num;*//*which bus*/
	u32 num_cs; /* supported slave numbers */
	u32 transfer_mode;
	u32 active_cs_pin;
	//copy from the user...
	u32 tx_len;
	u32 rx_len;
	void *rx_buff;
	void *tx_buff;
	struct fh_spi_cs cs_data[SPI_MASTER_CONTROLLER_MAX_SLAVE];
	struct fh_spi_platform_data *board_info;
	char   isr_name[16];
#ifdef CONFIG_SPI_USE_MULTI_WIRE
	struct fh_spi_multi multi_para;
#endif
#ifdef CONFIG_SPI_USE_DMA
	struct fh_spi_dma dma_para;
#endif
};

struct fh_spi_controller {
	//bind to master class
	struct device *master_dev;

	struct mutex lock;
	//message queue
	struct list_head queue;
	struct platform_device *p_dev;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	struct spi_message *active_message;
	struct spi_transfer *active_transfer;
	struct fh_spi dwc;
	struct spi_device *active_spi_dev;
};


/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/
u32 Spi_Enable(struct fh_spi *dw, spi_enable_e enable);
u32 Spi_ReadStatus(struct fh_spi *dw);
u32 check_spi_sup_multi_wire(struct fh_spi_controller *fh_spi);
u32 rx_only_fix_data_width(struct fh_spi_controller *fh_spi, u32 size);
u32 Spi_RawIsrstatus(struct fh_spi *dw);
u32 Spi_ContinueReadNum(struct fh_spi *dw, u32 num);
u32 Spi_WriteData(struct fh_spi *dw, u32 data);
u32 Spi_SetRxlevlel(struct fh_spi *dw, u32 level);
/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/

#endif

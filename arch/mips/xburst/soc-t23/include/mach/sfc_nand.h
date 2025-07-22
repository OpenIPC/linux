#ifndef __SFC_NAND_H__
#define __SFC_NAND_H__

#include <linux/list.h>
#include <linux/mtd/mtd.h>
#include <linux/irqreturn.h>
/*#include "sfc_params.h"*/

#define SPIFLASH_PARAMER_OFFSET (26 * 1024)
struct cmd_info{
	int cmd;
	int cmd_len;/*reserved; not use*/
	int dataen;
	int sta_exp;
	int sta_msk;
};

struct sfc_transfer {
	int direction;

	struct cmd_info *cmd_info;

	int addr_len;
	unsigned int addr;
	unsigned int addr_plus;
	int addr_dummy_bits;/*cmd + addr_dummy_bits + addr*/

	const unsigned char *data;
	int data_dummy_bits;/*addr + data_dummy_bits + data*/
	unsigned int len;
	unsigned int cur_len;

	int sfc_mode;
	int ops_mode;
	int phase_format;/*we just use default value;phase1:cmd+dummy+addr... phase0:cmd+addr+dummy...*/

	struct list_head transfer_list;
};

struct sfc_message {
	struct list_head    transfers;
	unsigned        actual_length;
	int         status;

};

struct sfc{

	void __iomem		*iomem;
	struct resource     *ioarea;
	int			irq;
	struct clk		*clk;
	struct clk		*clk_gate;
	unsigned long src_clk;
	struct completion	done;
	int			threshold;
	irqreturn_t (*irq_callback)(struct sfc *);
	unsigned long		phys;

	struct sfc_transfer *transfer;
};


struct sfc_flash;

struct spi_nor_flash_ops {
	int (*set_4byte_mode)(struct sfc_flash *flash);
	int (*set_quad_mode)(struct sfc_flash *flash);
};


struct sfc_flash {
	struct mtd_info     mtd;
	struct device		*dev;
	struct resource		*resource;
	struct sfc			*sfc;
	struct jz_sfc_info *pdata;
	void *flash_info;
	unsigned int flash_info_num;

	unsigned int chip_id;
	struct mutex        lock;

	int			status;
	spinlock_t		lock_status;

	int quad_succeed;
	struct spi_nor_info *g_nor_info;
	struct spi_nor_flash_ops	*nor_flash_ops;
	struct spi_nor_cmd_info	 *cur_r_cmd;
	struct spi_nor_cmd_info	 *cur_w_cmd;
	struct burner_params *params;
	struct norflash_partitions *norflash_partitions;

	unsigned char *swap_buf;
};


/* SFC register */

#define	SFC_GLB				(0x0000)
#define	SFC_DEV_CONF			(0x0004)
#define	SFC_DEV_STA_EXP			(0x0008)
#define	SFC_DEV_STA_RT			(0x000c)
#define	SFC_DEV_STA_MSK			(0x0010)
#define	SFC_TRAN_CONF(n)		(0x0014 + (n * 4))
#define	SFC_TRAN_LEN			(0x002c)
#define	SFC_DEV_ADDR(n)			(0x0030 + (n * 4))
#define	SFC_DEV_ADDR_PLUS(n)		(0x0048 + (n * 4))
#define	SFC_MEM_ADDR			(0x0060)
#define	SFC_TRIG			(0x0064)
#define	SFC_SR				(0x0068)
#define	SFC_SCR				(0x006c)
#define	SFC_INTC			(0x0070)
#define	SFC_FSM				(0x0074)
#define	SFC_CGE				(0x0078)
#define	SFC_RM_DR			(0x1000)

/* For SFC_GLB */
#define	GLB_TRAN_DIR			(1 << 13)
#define GLB_TRAN_DIR_WRITE		(1)
#define GLB_TRAN_DIR_READ		(0)
#define	GLB_THRESHOLD_OFFSET		(7)
#define GLB_THRESHOLD_MSK		(0x3f << GLB_THRESHOLD_OFFSET)
#define GLB_OP_MODE			(1 << 6)
#define SLAVE_MODE			(0x0)
#define DMA_MODE			(0x1)
#define GLB_PHASE_NUM_OFFSET		(3)
#define GLB_PHASE_NUM_MSK		(0x7  << GLB_PHASE_NUM_OFFSET)
#define GLB_WP_EN			(1 << 2)
#define GLB_BURST_MD_OFFSET		(0)
#define GLB_BURST_MD_MSK		(0x3  << GLB_BURST_MD_OFFSET)

/* For SFC_DEV_CONF */
#define	DEV_CONF_ONE_AND_HALF_CYCLE_DELAY	(3)
#define	DEV_CONF_ONE_CYCLE_DELAY	(2)
#define	DEV_CONF_HALF_CYCLE_DELAY	(1)
#define	DEV_CONF_NO_DELAY	        (0)
#define	DEV_CONF_SMP_DELAY_OFFSET	(16)
#define	DEV_CONF_SMP_DELAY_MSK		(0x3 << DEV_CONF_SMP_DELAY_OFFSET)
#define DEV_CONF_CMD_TYPE		(0x1 << 15)
#define DEV_CONF_STA_TYPE_OFFSET	(13)
#define DEV_CONF_STA_TYPE_MSK		(0x1 << DEV_CONF_STA_TYPE_OFFSET)
#define DEV_CONF_THOLD_OFFSET		(11)
#define	DEV_CONF_THOLD_MSK		(0x3 << DEV_CONF_THOLD_OFFSET)
#define DEV_CONF_TSETUP_OFFSET		(9)
#define DEV_CONF_TSETUP_MSK		(0x3 << DEV_CONF_TSETUP_OFFSET)
#define DEV_CONF_TSH_OFFSET		(5)
#define DEV_CONF_TSH_MSK		(0xf << DEV_CONF_TSH_OFFSET)
#define DEV_CONF_CPHA			(0x1 << 4)
#define DEV_CONF_CPOL			(0x1 << 3)
#define DEV_CONF_CEDL			(0x1 << 2)
#define DEV_CONF_HOLDDL			(0x1 << 1)
#define DEV_CONF_WPDL			(0x1 << 0)

/* For SFC_TRAN_CONF */
#define	TRAN_CONF_TRAN_MODE_OFFSET	(29)
#define	TRAN_CONF_TRAN_MODE_MSK		(0x7)
#define	TRAN_CONF_ADDR_WIDTH_OFFSET	(26)
#define	TRAN_CONF_ADDR_WIDTH_MSK	(0x7 << ADDR_WIDTH_OFFSET)
#define TRAN_CONF_POLLEN		(1 << 25)
#define TRAN_CONF_CMDEN			(1 << 24)
#define TRAN_CONF_FMAT			(1 << 23)
#define TRAN_CONF_FMAT_OFFSET		(23)
#define TRAN_CONF_DMYBITS_OFFSET	(17)
#define TRAN_CONF_DMYBITS_MSK		(0x3f << DMYBITS_OFFSET)
#define TRAN_CONF_DATEEN		(1 << 16)
#define TRAN_CONF_DATEEN_OFFSET		(16)
#define	TRAN_CONF_CMD_OFFSET		(0)
#define	TRAN_CONF_CMD_MSK		(0xffff << CMD_OFFSET)
#define	TRAN_CONF_CMD_LEN		(1 << 15)

/* For SFC_TRIG */
#define TRIG_FLUSH			(1 << 2)
#define TRIG_STOP			(1 << 1)
#define TRIG_START			(1 << 0)

/* For SFC_SCR */
#define	CLR_END			(1 << 4)
#define CLR_TREQ		(1 << 3)
#define CLR_RREQ		(1 << 2)
#define CLR_OVER		(1 << 1)
#define CLR_UNDER		(1 << 0)

/* For SFC_TRAN_CONFx */
#define	TRAN_MODE_OFFSET	(29)
#define	TRAN_MODE_MSK		(0x7 << TRAN_MODE_OFFSET)
#define TRAN_SPI_STANDARD   (0x0)
#define TRAN_SPI_DUAL   (0x1 )
#define TRAN_SPI_QUAD   (0x5 )
#define TRAN_SPI_IO_QUAD   (0x6 )


#define	ADDR_WIDTH_OFFSET	(26)
#define	ADDR_WIDTH_MSK		(0x7 << ADDR_WIDTH_OFFSET)
#define POLLEN			(1 << 25)
#define CMDEN			(1 << 24)
#define FMAT			(1 << 23)
#define DMYBITS_OFFSET		(17)
#define DMYBITS_MSK		(0x3f << DMYBITS_OFFSET)
#define DATEEN			(1 << 16)
#define	CMD_OFFSET		(0)
#define	CMD_MSK			(0xffff << CMD_OFFSET)

#define N_MAX				6
#define MAX_SEGS        128

#define CHANNEL_0		0
#define CHANNEL_1		1
#define CHANNEL_2		2
#define CHANNEL_3		3
#define CHANNEL_4		4
#define CHANNEL_5		5

#define ENABLE			1
#define DISABLE			0

#define COM_CMD			1	// common cmd
#define POLL_CMD		2	// the cmd will poll the status of flash,ext: read status

#define DMA_OPS			1
#define CPU_OPS			0

#define TM_STD_SPI		0
#define TM_DI_DO_SPI	1
#define TM_DIO_SPI		2
#define TM_FULL_DIO_SPI	3
#define TM_QI_QO_SPI	5
#define TM_QIO_SPI		6
#define	TM_FULL_QIO_SPI	7


#define DEFAULT_ADDRSIZE	3


#define THRESHOLD		32

#define SFC_NOR_RATE    110
#define DEF_ADDR_LEN    3
#define DEF_TCHSH       5
#define DEF_TSLCH       5
#define DEF_TSHSL_R     20
#define DEF_TSHSL_W     50



#endif



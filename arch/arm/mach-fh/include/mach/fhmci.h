#ifndef _FH_MCI_H_
#define _FH_MCI_H_

extern int trace_level;
#define FHMCI_TRACE_LEVEL 5
/*
   0 - all message
   1 - dump all register read/write
   2 - flow trace
   3 - timeout err and protocol err
   */

#define FHMCI_TRACE_FMT KERN_INFO
#define ID_SD0        0
#define ID_SD1        1

#define POWER_ON	1
#define POWER_OFF	0

#define CARD_UNPLUGED	1
#define CARD_PLUGED	0

#define ENABLE		1
#define DISABLE		0

#define FH_MCI_DETECT_TIMEOUT	(HZ/4)

#define FH_MCI_REQUEST_TIMEOUT	(5 * HZ)

#define MAX_RETRY_COUNT   100
#define MAX_MCI_HOST	(2)	/* max num of host on soc */

#define fhmci_trace(level, msg...) do { \
	if ((level) >= trace_level) { \
		printk(FHMCI_TRACE_FMT "%s:%d: ", __func__, __LINE__); \
		printk(msg); \
		printk("\n"); \
	} \
} while (0)

#define fhmci_assert(cond) do { \
	if (!(cond)) {\
		printk(KERN_ERR "Assert:fhmci:%s:%d\n", \
				__func__, \
				__LINE__); \
		BUG(); \
	} \
} while (0)

#define fhmci_error(s...) do { \
	printk(KERN_ERR "fhmci:%s:%d: ", __func__, __LINE__); \
	printk(s); \
	printk("\n"); \
} while (0)

#define fhmci_readl(addr) ({unsigned int reg = readl((unsigned int)addr); \
	fhmci_trace(1, "readl(0x%04X) = 0x%08X", (unsigned int)addr, reg); \
	reg; })

#define fhmci_writel(v, addr) do { \
	writel(v, (unsigned int)addr); \
	fhmci_trace(1, "writel(0x%04X) = 0x%08X",\
			(unsigned int)addr, (unsigned int)(v)); \
} while (0)


struct fhmci_des {
	unsigned long idmac_des_ctrl;
	unsigned long idmac_des_buf_size;
	unsigned long idmac_des_buf_addr;
	unsigned long idmac_des_next_addr;
};

struct fhmci_host {
	struct mmc_host		*mmc;
	spinlock_t		lock;
	struct mmc_request	*mrq;
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	void __iomem		*base;
	unsigned int		card_status;
	struct scatterlist	*dma_sg;
	unsigned int		dma_sg_num;
	unsigned int		dma_alloc_size;
	unsigned int		dma_dir;
	dma_addr_t		dma_paddr;
	unsigned int		*dma_vaddr;
	struct timer_list	timer;
	unsigned int		irq;
	unsigned int		irq_status;
	unsigned int		is_tuning;
	wait_queue_head_t	intr_wait;
	unsigned long		pending_events;
	unsigned int		id;
	struct fh_mci_board *pdata;
	unsigned int (*get_cd)(struct fhmci_host *host);
	unsigned int (*get_ro)(struct fhmci_host *host);
#define	FHMCI_PEND_DTO_b	(0)
#define	FHMCI_PEND_DTO_m	(1 << FHMCI_PEND_DTO_b)
};

/* Board platform data */
struct fh_mci_board {
    unsigned int num_slots;

    unsigned int quirks; /* Workaround / Quirk flags */
    unsigned int bus_hz; /* Bus speed */

    unsigned int caps;  /* Capabilities */

    /* delay in mS before detecting cards after interrupt */
    unsigned int detect_delay_ms;

    int (*init)(unsigned int slot_id,void* irq_handler_t , void *);
    unsigned int (*get_ro)(struct fhmci_host *host);
    unsigned int (*get_cd)(struct fhmci_host *host);
    int (*get_ocr)(unsigned int slot_id);
    int (*get_bus_wd)(unsigned int slot_id);
    /*
     * Enable power to selected slot and set voltage to desired level.
     * Voltage levels are specified using MMC_VDD_xxx defines defined
     * in linux/mmc/host.h file.
     */
    void (*setpower)(unsigned int slot_id, unsigned int volt);
    void (*exit)(unsigned int slot_id);
    void (*select_slot)(unsigned int slot_id);

    struct dw_mci_dma_ops *dma_ops;
    struct dma_pdata *data;
    struct block_settings *blk_settings;
    int fifo_depth;
};

union cmd_arg_s {
	unsigned int cmd_arg;
	struct cmd_bits_arg {
		unsigned int cmd_index:6;
		unsigned int response_expect:1;
		unsigned int response_length:1;
		unsigned int check_response_crc:1;
		unsigned int data_transfer_expected:1;
		unsigned int read_write:1;
		unsigned int transfer_mode:1;
		unsigned int send_auto_stop:1;
		unsigned int wait_prvdata_complete:1;
		unsigned int stop_abort_cmd:1;
		unsigned int send_initialization:1;
		unsigned int card_number:5;
		unsigned int update_clk_reg_only:1; /* bit 21 */
		unsigned int read_ceata_device:1;
		unsigned int ccs_expected:1;
		unsigned int enable_boot:1;
		unsigned int expect_boot_ack:1;
		unsigned int disable_boot:1;
		unsigned int boot_mode:1;
		unsigned int volt_switch:1;
		unsigned int use_hold_reg:1;
		unsigned int reserved:1;
		unsigned int start_cmd:1; /* HSB */
	} bits;
};

struct mmc_ctrl {
	unsigned int slot_idx;       /*0: mmc0;  1: mmc1*/
	unsigned int mmc_ctrl_state; /*0: enable mmc_rescan;  1: disable mmc_rescan*/
};

enum mmc_ctrl_state {
	RESCAN_ENABLE = 0,
	RESCAN_DISABLE
};

struct platform_device *get_mci_device(unsigned int index);
int storage_dev_set_mmc_rescan(struct mmc_ctrl *m_ctrl);
int read_mci_ctrl_states(int id_mmc_sd);

#endif


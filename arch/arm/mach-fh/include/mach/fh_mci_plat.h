#ifndef __FH_MCI_PLAT_H__
#define __FH_MCI_PLAT_H__

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
	int fifo_depth;
	unsigned int bus_hz;
	unsigned int (*get_cd)(struct fhmci_host *host);
	unsigned int (*get_ro)(struct fhmci_host *host);
#define	FHMCI_PEND_DTO_b	(0)
#define	FHMCI_PEND_DTO_m	(1 << FHMCI_PEND_DTO_b)
	char isr_name[10];
};

/* Board platform data */
struct fh_mci_board {
    unsigned int num_slots;

    unsigned int quirks; /* Workaround / Quirk flags */
    unsigned int bus_hz; /* Bus speed */

    unsigned int caps;  /* Capabilities */

    /* delay in mS before detecting cards after interrupt */
    unsigned int detect_delay_ms;

    int (*init)(unsigned int slot_id, void *irq_handler_t, void *);
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
	int drv_degree;
	int sam_degree;
	/*if rescan_max_num = 0 scan all times*/
	unsigned int rescan_max_num;
};

#endif


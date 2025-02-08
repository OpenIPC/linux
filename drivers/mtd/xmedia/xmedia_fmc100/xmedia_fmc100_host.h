/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef XMEDIA_FMC100_HOSTH
#define XMEDIA_FMC100_HOSTH

#include <linux/flash_stats.h>
#include "../xmedia_flash.h"
#include "../flash_cache.h"
#include "../syncnand/nand_sync.h"

struct xmedia_fmc_host;

struct xmedia_fmc_reg {
	u32 op_cfg;
	u32 cmd;
	u32 op_ctrl;

	int clock;
	int dummy;
};

struct xmedia_fmc_xnand {
	int ifmode;
	char *name;
	struct mtd_info *mtd;
	struct nand_chip chip[1];
	struct device *dev;

	struct cmdfunc_data cmdfunc[1];   /* mtd operation function parameter */
	struct mtd_func mtd_func[1];      /* mtd call function */

	struct xmedia_fmc_host *host;

	struct nand_ctrl_info *ctrl_info;   /* nand controller information */

	struct nand_read_retry *read_retry; /* read retry function */

	struct flash_regop regop[1];

	struct spinand_driver *spinand_drv;

	int ecc_strength;
	int pagesize;
	int oobsize;
	int blocksize;
	int page_per_block;

	struct {
		u32 fmc_cfg_ecc_type;
		u32 fmc_cfg_page_size;
		struct xmedia_fmc_reg read;
		struct xmedia_fmc_reg write;
		u32 fmc_pnd_pwidth_cfg;
	} reg;

	bool randomizer_en;

	bool is_empty_page;
	bool is_bad_block;
	bool is_uc_err;

	char *pagebuf, *org_pagebuf;
	char *oobbuf;

	ulong clk_rate;

	int erase_addr_cycle; /* erase addr cycle */
	int read_addr_cycle;  /* read/write addr cycle */
	union {
		u8 manuf;
		u8 id[8];
	};

	u8 *(*read_raw_bbm)(struct xmedia_fmc_xnand *xnand);
	void (*cmd_readid)(struct xmedia_fmc_xnand *xnand);
	void (*cmd_reset)(struct xmedia_fmc_xnand *xnand);
	void (*cmd_erase)(struct xmedia_fmc_xnand *xnand);

	void (*shutdown)(struct xmedia_fmc_xnand *xnand);
	void (*suspend)(struct xmedia_fmc_xnand *xnand);
	void (*resume)(struct xmedia_fmc_xnand *xnand);

	struct flash_regop_intf regop_intf;

	struct flash_stats *stats;

	struct flash_cache *cache;
};

struct xmedia_fmc_spinor {
	struct mtd_info mtd[1];
	struct device *dev;

	char *name;
	int nr_chips;
	u32 chipsize; /* one chip size, when nr_chips > 1 */
	int chipsize_shift;
	int chipsize_mask;
	u32 erasesize;

	u32 addr_cycle;

	int chipselect[XMEDIA_FMC100_OP_CFG_NUM_CS];

#define XMEDIA_FMC100_SPINOR_DMA_SIZE                (4096)
#define XMEDIA_FMC100_SPINOR_DMA_SIZE_MASK           (XMEDIA_FMC100_SPINOR_DMA_SIZE - 1)
	void *dma_vaddr;
	dma_addr_t dma_paddr;

	struct flash_regop regop[1];

	struct spinor_driver *drv;

	struct {
		struct xmedia_fmc_reg read;
		struct xmedia_fmc_reg write;
		struct xmedia_fmc_reg erase;
	} xfer;

	struct xmedia_fmc_host *host;

	void (*shutdown)(struct xmedia_fmc_spinor *spinor);
	void (*suspend)(struct xmedia_fmc_spinor *spinor);
	void (*resume)(struct xmedia_fmc_spinor *spinor);

	struct flash_regop_intf regop_intf;
};

#define NAND_MODE_SYNC		(1 << 0)

struct xmedia_fmc_host {
	void __iomem *regbase;
	void __iomem *iobase;
	int sz_iobase;

	u32 fmc_crg_value;
	void __iomem *fmc_crg_addr;
	struct mutex lock;
	struct device *dev;
	unsigned int irq;
	struct completion dma_done;
	struct clk *clk;

	u32 intr_status;

	int ifmode;
	void (*set_ifmode)(struct xmedia_fmc_host *host, int ifmode);
	void (*irq_enable)(struct xmedia_fmc_host *host);
	int (*wait_dma_finish)(struct xmedia_fmc_host *host);
	int (*wait_host_ready)(struct xmedia_fmc_host *host);
	int chipselect;

	/* for sync nand only. */
	int flags;

	int caps;

	struct {
		u32 fmc_cfg_ecc_type;
		u32 fmc_cfg_page_size;
	} reg;

	struct xmedia_fmc_xnand *nand;
	struct xmedia_fmc_xnand *spinand;
	struct xmedia_fmc_spinor *spinor;
};

#define XMEDIA_FMC_IFMODE_INVALID         0
#define XMEDIA_FMC_IFMODE_SPINOR          1
#define XMEDIA_FMC_IFMODE_NAND                2
#define XMEDIA_FMC_IFMODE_SPINAND             3

int xmedia_fmc100_write_reg(struct flash_regop_info *info);

int xmedia_fmc100_read_reg(struct flash_regop_info *info);

int xmedia_fmc100_spinor_probe(struct platform_device *pdev, struct xmedia_fmc_host *host);

int xmedia_fmc100_xnand_probe(struct platform_device *pdev, struct xmedia_fmc_host *host,
			 int ifmode);

u32 xmedia_fmc100_syncmode_reg(u32 value, u32 _to_reg);

void xmedia_fmc100_set_sync_timing(struct nand_sync_timing *nand_sync_timing);

extern struct nand_chip_clock g_fmc100_nand_rwlatch[];

extern struct spinand_chip_xfer g_fmc100_spinand_chip_xfer[];

extern struct spinor_chip_xfer xmedia_fmc100_spinor_chip_xfer[];

extern struct nand_sync_timing xmedia_fmc100_nand_sync_timing[];

#endif /* XMEDIA_FMC100_HOSTH */

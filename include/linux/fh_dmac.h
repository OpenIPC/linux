#ifndef FH_DMAC_H
#define FH_DMAC_H

#include <linux/dmaengine.h>
#include <mach/fh_dma_plat.h>

#ifdef CONFIG_FH_DMAC
/* Platform-configurable bits in CFG_HI */
#define FHC_CFGH_FCMODE		(1 << 0)
#define FHC_CFGH_FIFO_MODE	(1 << 1)
#define FHC_CFGH_PROTCTL(x)	((x) << 2)
#define FHC_CFGH_SRC_PER(x)	((x) << 7)
#define FHC_CFGH_DST_PER(x)	((x) << 11)
#endif

enum fh_dma_slave_increment {
	FH_DMA_SLAVE_INC,
	FH_DMA_SLAVE_DEC,
	FH_DMA_SLAVE_FIX,
};

enum fh_dmac_flags {
	FH_DMA_IS_CYCLIC = 0,
	FH_DMA_IS_SOFT_LLP = 1,
};

/**
 * enum fh_dma_slave_width - DMA slave register access width.
 * @DMA_SLAVE_WIDTH_8BIT: Do 8-bit slave register accesses
 * @DMA_SLAVE_WIDTH_16BIT: Do 16-bit slave register accesses
 * @DMA_SLAVE_WIDTH_32BIT: Do 32-bit slave register accesses
 */
enum fh_dma_slave_width {
	FH_DMA_SLAVE_WIDTH_8BIT,
	FH_DMA_SLAVE_WIDTH_16BIT,
	FH_DMA_SLAVE_WIDTH_32BIT,
};

/* bursts size */
enum fh_dma_msize {
	FH_DMA_MSIZE_1,
	FH_DMA_MSIZE_4,
	FH_DMA_MSIZE_8,
	FH_DMA_MSIZE_16,
	FH_DMA_MSIZE_32,
	FH_DMA_MSIZE_64,
	FH_DMA_MSIZE_128,
	FH_DMA_MSIZE_256,
};

/* flow controller */
enum fh_dma_fc {
	FH_DMA_FC_D_M2M,
	FH_DMA_FC_D_M2P,
	FH_DMA_FC_D_P2M,
	FH_DMA_FC_D_P2P,
	FH_DMA_FC_P_P2M,
	FH_DMA_FC_SP_P2P,
	FH_DMA_FC_P_M2P,
	FH_DMA_FC_DP_P2P,
};

/**
 * struct fh_dma_slave - Controller-specific information about a slave
 *
 * @dma_dev: required DMA master device
 * @tx_reg: physical address of data register used for
 *	memory-to-peripheral transfers
 * @rx_reg: physical address of data register used for
 *	peripheral-to-memory transfers
 * @reg_width: peripheral register width
 * @cfg_hi: Platform-specific initializer for the CFG_HI register
 * @cfg_lo: Platform-specific initializer for the CFG_LO register
 * @src_master: src master for transfers on allocated channel.
 * @dst_master: dest master for transfers on allocated channel.
 * @src_msize: src burst size.
 * @dst_msize: dest burst size.
 * @fc: flow controller for DMA transfer
 */
struct fh_dma_slave {
	struct device		*dma_dev;
	dma_addr_t		tx_reg;
	dma_addr_t		rx_reg;
	enum fh_dma_slave_width	reg_width;
	u32			cfg_hi;
	u32			cfg_lo;
	u8			src_master;
	u8			dst_master;
	u8			src_msize;
	u8			dst_msize;
	u8			fc;
};

/* DMA API extensions */
struct fh_cyclic_desc {
	struct fh_desc	**desc;
	unsigned long	periods;
	void	(*period_callback)(void *param);
	void		*period_callback_param;
};

struct fh_dma_extra {
	u32 sinc;
	u32 dinc;
	u32 protctl_flag;
	u32 protctl_data;
	u32 master_flag;
	u32 src_master;
	u32 dst_master;
};

struct fh_dma_chan {
	struct dma_chan			chan;
	void __iomem			*ch_regs;
	u8				mask;
	u8				priority;
	enum dma_transfer_direction	direction;
	bool				paused;
	bool				initialized;

	/* software emulation of the LLP transfers */
	struct list_head	*tx_node_active;

	spinlock_t		lock;

	/* these other elements are all protected by lock */
	unsigned long		flags;
	struct list_head	active_list;
	struct list_head	queue;
	struct list_head	free_list;
	u32			residue;
	struct fh_cyclic_desc	*cdesc;

	unsigned int		descs_allocated;

	/* hardware configuration */
	unsigned int		block_size;
	bool			nollp;

	/* custom slave configuration */
	unsigned int		request_line;
	unsigned char		src_master;
	unsigned char		dst_master;
	struct fh_dma_extra	ext_para;
	/* configuration passed via DMA_SLAVE_CONFIG */
	struct dma_slave_config dma_sconfig;
#ifdef CONFIG_CHANNEL_ALLOC_MEM_CLASSICS
	void *dma_vaddr;
	dma_addr_t dma_paddr;
#endif
};

struct fh_dma {
	struct dma_device	dma;
	void __iomem		*regs;
#ifndef CONFIG_CHANNEL_ALLOC_MEM_CLASSICS
	struct dma_pool		*desc_pool;
#endif
	struct tasklet_struct	tasklet;
	struct clk		*clk;

	u8			all_chan_mask;

	/* hardware configuration */
	unsigned char		nr_masters;
	unsigned char		data_width[4];

	struct fh_dma_chan	chan[0];
};

/* LLI == Linked List Item; a.k.a. DMA block descriptor */
struct fh_lli {
	/* values that are not changed by hardware */
	u32		sar;
	u32		dar;
	u32		llp;		/* chain to next lli */
	u32		ctllo;
	/* values that may get written back: */
	u32		ctlhi;
	/* sstat and dstat can snapshot peripheral register state.
	 * silicon config may discard either or both...
	 */
	u32		sstat;
	u32		dstat;
};

struct fh_desc {
	/* FIRST values the hardware uses */
	struct fh_lli			lli;

	/* THEN values for driver housekeeping */
	struct list_head		desc_node;
	struct list_head		tx_list;
	struct dma_async_tx_descriptor	txd;
	size_t				len;
	size_t				total_len;
};


#define to_fh_desc(h)	list_entry(h, struct fh_desc, desc_node)


static inline int axi_dma_lock(axi_dma_lock_t *p, FH_UINT32 time_out)
{
	_axi_dma_lock(p, time_out);
	return 0;
}

static inline void* axi_dma_malloc_desc(void* pri, FH_UINT32 size, FH_UINT32* phy_back){
	// struct fh_axi_dma_adapt *fh_axi_adapt_obj;
	// fh_axi_adapt_obj = (struct fh_axi_dma_adapt *)pri;
	return dma_alloc_coherent(pri, size, (dma_addr_t *)phy_back, GFP_KERNEL);
}

static inline void axi_dma_free_desc(void* pri, FH_UINT32 size, void *vir, FH_UINT32 phy_add){
	// struct fh_axi_dma_adapt *fh_axi_adapt_obj;
	// fh_axi_adapt_obj = (struct fh_axi_dma_adapt *)pri;
	dma_free_coherent(pri, size, vir, phy_add);
}
#define axi_dma_free_desc(core_pri, size, vir, phy)	dma_free_coherent(core_pri, size, vir, phy)

static inline void axi_dma_clean_invalidated_desc(FH_UINT32 buffer, FH_UINT32 size)
{

}

static inline void axi_dma_clean_desc(FH_UINT32 buffer, FH_UINT32 size)
{
	
}

static inline void axi_dma_invalidate_desc(FH_UINT32 buffer, FH_UINT32 size)
{
	
}

/* data cache flush.. if data buff just in no cache mem; no need to care*/
static inline void axi_dma_clean_invalidated_dcache(FH_UINT32 buffer, FH_UINT32 size)
{

}

static inline void axi_dma_clean_dcache(FH_UINT32 buffer, FH_UINT32 size)
{
	
}

static inline void axi_dma_invalidate_dcache(FH_UINT32 buffer, FH_UINT32 size)
{
	
}

static inline struct fh_dma_chan *to_fh_dma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct fh_dma_chan, chan);
}

static inline struct fh_dma *to_fh_dma(struct dma_device *ddev)
{
	return container_of(ddev, struct fh_dma, dma);
}

static inline struct fh_desc *
txd_to_fh_desc(struct dma_async_tx_descriptor *txd)
{
	return container_of(txd, struct fh_desc, txd);
}

struct fh_cyclic_desc *fh_dma_cyclic_prep(struct dma_chan *chan,
		dma_addr_t buf_addr, size_t buf_len, size_t period_len,
		enum dma_transfer_direction direction);
void fh_dma_cyclic_free(struct dma_chan *chan);
int fh_dma_cyclic_start(struct dma_chan *chan);
void fh_dma_cyclic_stop(struct dma_chan *chan);

dma_addr_t fh_dma_get_src_addr(struct dma_chan *chan);
dma_addr_t fh_dma_get_dst_addr(struct dma_chan *chan);

#endif /* FH_DMAC_H */

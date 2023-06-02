#include "fh_axi_dma_adapt.h"


#ifndef BIT
#define BIT(x) (1 << (x))
#endif

//#define AXI_DMA_DEBUG
#define AXI_DESC_ALLIGN                64  /*DO NOT TOUCH!!!*/
#define AXI_DESC_ALLIGN_BIT_MASK     0x7f  /*DO NOT TOUCH!!!*/
#define lift_shift_bit_num(bit_num) (1 << bit_num)
#define AXI_DMA_MAX_NR_CHANNELS 8
#ifndef GENMASK
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (sizeof(long) * 8 - 1 - (h))))
#endif
#ifndef GENMASK_ULL
#define GENMASK_ULL(h, l)                                                      \
	(((~0ULL) << (l)) & (~0ULL >> (sizeof(long long) * 8 - 1 - (h))))
#endif

/* DMAC_CFG */
#define DMAC_EN_POS			0
#define DMAC_EN_MASK		BIT(DMAC_EN_POS)
#define INT_EN_POS			1
#define INT_EN_MASK			BIT(INT_EN_POS)
/* CH_CTL_H */
#define CH_CTL_H_ARLEN_EN(n) ((n) << 6)	/* 32+6 = 38 */
#define CH_CTL_H_ARLEN(n) ((n) << 7)	/* 32+7 = 39 */
#define CH_CTL_H_AWLEN_EN(n) ((n) << 15) /* 32+15 = 47 */
#define CH_CTL_H_AWLEN(n) ((n) << 16)	/* 32+16 = 48 */
#define CH_CTL_H_IOC_BLKTFR		BIT(26)
#define CH_CTL_H_LLI_LAST		BIT(30)
#define CH_CTL_H_LLI_VALID		BIT(31)

/* CH_CTL_L */
#define CH_CTL_L_LAST_WRITE_EN		BIT(30)
#define CH_CTL_L_AR_CACHE(n) ((n) << 22)
#define CH_CTL_L_AW_CACHE(n) ((n) << 26)
#define CH_CTL_L_DST_WIDTH_POS		11
#define CH_CTL_L_SRC_WIDTH_POS		8
#define CH_CTL_L_DST_INC_POS		6
#define CH_CTL_L_SRC_INC_POS		4
#define CH_CTL_L_DST_MAST		BIT(2)
#define CH_CTL_L_SRC_MAST		BIT(0)

/* CH_CFG_H */
#define CH_CFG_H_PRIORITY_POS		17
#define CH_CFG_H_HS_SEL_DST_POS		4
#define CH_CFG_H_HS_SEL_SRC_POS		3
#define CH_CFG_H_TT_FC_POS		0

#define CH_CFG_H_DST_OSR_LMT(n) ((n) << 27)
#define CH_CFG_H_SRC_OSR_LMT(n) ((n) << 23)
/* CH_CFG_L */
#define CH_CFG_L_DST_MULTBLK_TYPE_POS	2
#define CH_CFG_L_SRC_MULTBLK_TYPE_POS	0

#define AXI_DMA_CTLL_DST_WIDTH(n) ((n) << 11) /* bytes per element */
#define AXI_DMA_CTLL_SRC_WIDTH(n) ((n) << 8)
#define AXI_DMA_CTLL_DST_INC_MODE(n) ((n) << 6)
#define AXI_DMA_CTLL_SRC_INC_MODE(n) ((n) << 4)

#define AXI_DMA_CTLL_DST_MSIZE(n) ((n) << 18)
#define AXI_DMA_CTLL_SRC_MSIZE(n) ((n) << 14)


#define AXI_DMA_CTLL_DMS(n) ((n) << 2)
#define AXI_DMA_CTLL_SMS(n) ((n) << 0)
/* caution ,diff with ahb dma */
#define AXI_DMA_CFGH_FC(n) ((n) << 0)
#define AXI_DMA_CFGH_DST_PER(n) ((n) << 3)

#define DW_REG(name)  \
    FH_UINT32 name; \
    FH_UINT32 __pad_##name


#define __dma_raw_writeb(v, a) (*(volatile FH_UINT8 *)(a) = (v))
#define __dma_raw_writew(v, a) (*(volatile FH_UINT16 *)(a) = (v))
#define __dma_raw_writel(v, a) (*(volatile FH_UINT32 *)(a) = (v))

#define __dma_raw_readb(a) (*(volatile FH_UINT8 *)(a))
#define __dma_raw_readw(a) (*(volatile FH_UINT16 *)(a))
#define __dma_raw_readl(a) (*(volatile FH_UINT32 *)(a))

#define dw_readl(dw, name) \
    __dma_raw_readl(&(((struct dw_axi_dma_regs *)dw->regs)->name))
#define dw_writel(dw, name, val) \
    __dma_raw_writel((val), &(((struct dw_axi_dma_regs *)dw->regs)->name))
#define dw_readw(dw, name) \
    __dma_raw_readw(&(((struct dw_axi_dma_regs *)dw->regs)->name))
#define dw_writew(dw, name, val) \
    __dma_raw_writew((val), &(((struct dw_axi_dma_regs *)dw->regs)->name))

#define CONTROLLER_STATUS_CLOSED (0)
#define CONTROLLER_STATUS_OPEN (1)

#define CHANNEL_STATUS_CLOSED (0)
#define CHANNEL_STATUS_OPEN (1)
#define CHANNEL_STATUS_IDLE (2)
#define CHANNEL_STATUS_BUSY (3)
#define SINGLE_TRANSFER (0)
#define CYCLIC_TRANSFER (1)
#define DEFAULT_TRANSFER SINGLE_TRANSFER
#define CHANNEL_REAL_FREE (0)
#define CHANNEL_NOT_FREE (1)

#define axi_dma_list_entry(node, type, member) \
    ((type *)((char *)(node) - (unsigned long)(&((type *)0)->member)))

#define axi_dma_list_for_each_entry_safe(pos, n, head, member)                \
    for (pos = axi_dma_list_entry((head)->next, typeof(*pos), member),     \
        n    = axi_dma_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                      \
         pos = n, n = axi_dma_list_entry(n->member.next, typeof(*n), member))
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
enum {
	DWAXIDMAC_ARWLEN_1		= 0,
	DWAXIDMAC_ARWLEN_2		= 1,
	DWAXIDMAC_ARWLEN_4		= 3,
	DWAXIDMAC_ARWLEN_8		= 7,
	DWAXIDMAC_ARWLEN_16		= 15,
	DWAXIDMAC_ARWLEN_32		= 31,
	DWAXIDMAC_ARWLEN_64		= 63,
	DWAXIDMAC_ARWLEN_128		= 127,
	DWAXIDMAC_ARWLEN_256		= 255,
	DWAXIDMAC_ARWLEN_MIN		= DWAXIDMAC_ARWLEN_1,
	DWAXIDMAC_ARWLEN_MAX		= DWAXIDMAC_ARWLEN_256
};

enum {
	DWAXIDMAC_BURST_TRANS_LEN_1	= 0,
	DWAXIDMAC_BURST_TRANS_LEN_4,
	DWAXIDMAC_BURST_TRANS_LEN_8,
	DWAXIDMAC_BURST_TRANS_LEN_16,
	DWAXIDMAC_BURST_TRANS_LEN_32,
	DWAXIDMAC_BURST_TRANS_LEN_64,
	DWAXIDMAC_BURST_TRANS_LEN_128,
	DWAXIDMAC_BURST_TRANS_LEN_256,
	DWAXIDMAC_BURST_TRANS_LEN_512,
	DWAXIDMAC_BURST_TRANS_LEN_1024
};

enum {
	DWAXIDMAC_CH_CTL_L_INC		= 0,
	DWAXIDMAC_CH_CTL_L_NOINC
};

enum {
	DWAXIDMAC_HS_SEL_HW		= 0,
	DWAXIDMAC_HS_SEL_SW
};

enum {
	DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC	= 0,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_SRC,
	DWAXIDMAC_TT_FC_PER_TO_PER_SRC,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DST,
	DWAXIDMAC_TT_FC_PER_TO_PER_DST
};

enum {
	DWAXIDMAC_MBLK_TYPE_CONTIGUOUS	= 0,
	DWAXIDMAC_MBLK_TYPE_RELOAD,
	DWAXIDMAC_MBLK_TYPE_SHADOW_REG,
	DWAXIDMAC_MBLK_TYPE_LL
};


/**
 * DW AXI DMA channel interrupts
 *
 * @DWAXIDMAC_IRQ_NONE: Bitmask of no one interrupt
 * @DWAXIDMAC_IRQ_BLOCK_TRF: Block transfer complete
 * @DWAXIDMAC_IRQ_DMA_TRF: Dma transfer complete
 * @DWAXIDMAC_IRQ_SRC_TRAN: Source transaction complete
 * @DWAXIDMAC_IRQ_DST_TRAN: Destination transaction complete
 * @DWAXIDMAC_IRQ_SRC_DEC_ERR: Source decode error
 * @DWAXIDMAC_IRQ_DST_DEC_ERR: Destination decode error
 * @DWAXIDMAC_IRQ_SRC_SLV_ERR: Source slave error
 * @DWAXIDMAC_IRQ_DST_SLV_ERR: Destination slave error
 * @DWAXIDMAC_IRQ_LLI_RD_DEC_ERR: LLI read decode error
 * @DWAXIDMAC_IRQ_LLI_WR_DEC_ERR: LLI write decode error
 * @DWAXIDMAC_IRQ_LLI_RD_SLV_ERR: LLI read slave error
 * @DWAXIDMAC_IRQ_LLI_WR_SLV_ERR: LLI write slave error
 * @DWAXIDMAC_IRQ_INVALID_ERR: LLI invalid error or Shadow register error
 * @DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR: Slave Interface Multiblock type error
 * @DWAXIDMAC_IRQ_DEC_ERR: Slave Interface decode error
 * @DWAXIDMAC_IRQ_WR2RO_ERR: Slave Interface write to read only error
 * @DWAXIDMAC_IRQ_RD2RWO_ERR: Slave Interface read to write only error
 * @DWAXIDMAC_IRQ_WRONCHEN_ERR: Slave Interface write to channel error
 * @DWAXIDMAC_IRQ_SHADOWREG_ERR: Slave Interface shadow reg error
 * @DWAXIDMAC_IRQ_WRONHOLD_ERR: Slave Interface hold error
 * @DWAXIDMAC_IRQ_LOCK_CLEARED: Lock Cleared Status
 * @DWAXIDMAC_IRQ_SRC_SUSPENDED: Source Suspended Status
 * @DWAXIDMAC_IRQ_SUSPENDED: Channel Suspended Status
 * @DWAXIDMAC_IRQ_DISABLED: Channel Disabled Status
 * @DWAXIDMAC_IRQ_ABORTED: Channel Aborted Status
 * @DWAXIDMAC_IRQ_ALL_ERR: Bitmask of all error interrupts
 * @DWAXIDMAC_IRQ_ALL: Bitmask of all interrupts
 */
enum {
	DWAXIDMAC_IRQ_NONE		= 0,
	DWAXIDMAC_IRQ_BLOCK_TRF		= BIT(0),
	DWAXIDMAC_IRQ_DMA_TRF		= BIT(1),
	DWAXIDMAC_IRQ_SRC_TRAN		= BIT(3),
	DWAXIDMAC_IRQ_DST_TRAN		= BIT(4),
	DWAXIDMAC_IRQ_SRC_DEC_ERR	= BIT(5),
	DWAXIDMAC_IRQ_DST_DEC_ERR	= BIT(6),
	DWAXIDMAC_IRQ_SRC_SLV_ERR	= BIT(7),
	DWAXIDMAC_IRQ_DST_SLV_ERR	= BIT(8),
	DWAXIDMAC_IRQ_LLI_RD_DEC_ERR	= BIT(9),
	DWAXIDMAC_IRQ_LLI_WR_DEC_ERR	= BIT(10),
	DWAXIDMAC_IRQ_LLI_RD_SLV_ERR	= BIT(11),
	DWAXIDMAC_IRQ_LLI_WR_SLV_ERR	= BIT(12),
	DWAXIDMAC_IRQ_INVALID_ERR	= BIT(13),
	DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR	= BIT(14),
	DWAXIDMAC_IRQ_DEC_ERR		= BIT(16),
	DWAXIDMAC_IRQ_WR2RO_ERR		= BIT(17),
	DWAXIDMAC_IRQ_RD2RWO_ERR	= BIT(18),
	DWAXIDMAC_IRQ_WRONCHEN_ERR	= BIT(19),
	DWAXIDMAC_IRQ_SHADOWREG_ERR	= BIT(20),
	DWAXIDMAC_IRQ_WRONHOLD_ERR	= BIT(21),
	DWAXIDMAC_IRQ_LOCK_CLEARED	= BIT(27),
	DWAXIDMAC_IRQ_SRC_SUSPENDED	= BIT(28),
	DWAXIDMAC_IRQ_SUSPENDED		= BIT(29),
	DWAXIDMAC_IRQ_DISABLED		= BIT(30),
	DWAXIDMAC_IRQ_ABORTED		= BIT(31),
	DWAXIDMAC_IRQ_ALL_ERR		= (GENMASK(21, 16) | GENMASK(14, 5)),
	DWAXIDMAC_IRQ_ALL		= GENMASK(31, 0)
};

struct  axi_dma_lli {
	FH_UINT32		sar_lo;
	FH_UINT32		sar_hi;
	FH_UINT32		dar_lo;
	FH_UINT32		dar_hi;
	FH_UINT32		block_ts_lo;
	FH_UINT32		block_ts_hi;
	FH_UINT32		llp_lo;
	FH_UINT32		llp_hi;
	FH_UINT32		ctl_lo;
	FH_UINT32		ctl_hi;
	FH_UINT32		sstat;
	FH_UINT32		dstat;
	FH_UINT32		status_lo;
	FH_UINT32		status_hi;
	FH_UINT32		reserved_lo;
	FH_UINT32		reserved_hi;
};

/* Hardware register definitions. */
struct dw_axi_dma_chan_regs {
	DW_REG(SAR);			/* 0x0 ~ 0x7*/
	DW_REG(DAR);			/* 0x8 ~ 0xf*/
	DW_REG(BLOCK_TS);		/* 0x10 ~ 0x17*/
	FH_UINT32 CTL_LO;			
	FH_UINT32 CTL_HI;				/* 0x18 ~ 0x1f*/
	FH_UINT32 CFG_LO;
	FH_UINT32 CFG_HI;				/* 0x20 ~ 0x27*/
	DW_REG(LLP);			/* 0x28 ~ 0x2f*/
	FH_UINT32 STATUS_LO;
	FH_UINT32 STATUS_HI;			/* 0x30 ~ 0x37*/
	DW_REG(SWHS_SRC);		/* 0x38 ~ 0x3f*/
	DW_REG(SWHS_DST);		/* 0x40 ~ 0x47*/
	DW_REG(BLK_TFR_RESU);
	DW_REG(ID);
	DW_REG(QOS);
	DW_REG(SSTAT);
	DW_REG(DSTAT);
	DW_REG(SSTATAR);
	DW_REG(DSTATAR);
	FH_UINT32 INTSTATUS_EN_LO;
	FH_UINT32 INTSTATUS_EN_HI;
	FH_UINT32 INTSTATUS_LO;
	FH_UINT32 INTSTATUS_HI;
	FH_UINT32 INTSIGNAL_LO;
	FH_UINT32 INTSIGNAL_HI;
	FH_UINT32 INTCLEAR_LO;
	FH_UINT32 INTCLEAR_HI;
	FH_UINT32 rev[24];
};

struct dw_axi_dma_regs {
	DW_REG(ID);					/* 0x0 */
	DW_REG(COMPVER);			/* 0x8 */
	DW_REG(CFG);				/* 0x10 */
	FH_UINT32 CHEN_LO;				/* 0x18 */
	FH_UINT32 CHEN_HI;				/* 0x1c */
	DW_REG(reserved_20_27);		/* 0x20 */
	DW_REG(reserved_28_2f);		/* 0x28 */
	DW_REG(INTSTATUS);			/* 0x30 */
	DW_REG(COM_INTCLEAR);		/* 0x38 */
	DW_REG(COM_INTSTATUS_EN);	/* 0x40 */
	DW_REG(COM_INTSIGNAL_EN);	/* 0x48 */
	DW_REG(COM_INTSTATUS);		/* 0x50 */
	DW_REG(RESET);				/* 0x58 */
	FH_UINT32 reserved[40];			/* 0x60 */
	struct dw_axi_dma_chan_regs CHAN[AXI_DMA_MAX_NR_CHANNELS];/* 0x100 */
};

struct dw_axi_dma {
	/* vadd */
	void *regs;
	FH_UINT32 channel_max_number;
	FH_UINT32 controller_status;
	FH_UINT32 id;
	char name[20];
};

struct dma_channel {
	FH_UINT32 channel_status;  /* open, busy ,closed */
	FH_UINT32 desc_trans_size;
	axi_dma_lock_t channel_lock;
	axi_dma_list queue;
	struct dma_transfer *active_trans;
	FH_UINT32 open_flag;
	FH_UINT32 desc_total_no;
	FH_UINT32 free_index;
	FH_UINT32 used_index;
	FH_UINT32 desc_left_cnt;
	/*malloc maybe not allign; driver will malloc (size + cache line) incase*/
	FH_UINT32 allign_malloc;
	FH_UINT32 allign_phy;
	struct axi_dma_lli *base_lli;
	FH_UINT32 base_lli_phy;
	FH_UINT32 int_sig;
	FH_UINT32 status;
};

struct fh_axi_dma {
	/* myown */
	void *kernel_pri;// used for malloc........
	void *adapt_pri;// use for call adapt driver
	struct axi_dma_ops ops;
	struct dw_axi_dma dwc;
	/* channel obj */
	struct dma_channel dma_channel[AXI_DMA_MAX_NR_CHANNELS];
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
static void fh_axi_dma_cyclic_stop(struct dma_transfer *p);
static void fh_axi_dma_cyclic_start(struct dma_transfer *p);
static void fh_axi_dma_cyclic_prep(struct fh_axi_dma *fh_dma_p,
struct dma_transfer *p);
static void fh_axi_dma_cyclic_free(struct dma_transfer *p);
static void fh_axi_dma_cyclic_pause(struct dma_transfer *p);
static void fh_axi_dma_cyclic_resume(struct dma_transfer *p);
static FH_UINT32 vir_lli_to_phy_lli(struct axi_dma_lli *base_lli,
FH_UINT32 base_lli_phy, struct axi_dma_lli *cal_lli);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

/* function body */
/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/

static void dump_lli(struct axi_dma_lli *p_lli, struct axi_dma_lli *base_lli, FH_UINT32 base_lli_phy)
{
	FH_UINT32 phy_add;

	phy_add = vir_lli_to_phy_lli(base_lli, base_lli_phy, p_lli);

	FH_AXI_DMA_PRINT("SAR: 0x%08x DAR: 0x%08x LLP: 0x%08x BTS 0x%08x CTL: 0x%08x:%08x LLI_PHY_ADD: 0x%x\n",
	(p_lli->sar_lo),
	(p_lli->dar_lo),
	(p_lli->llp_lo),
	(p_lli->block_ts_lo),
	(p_lli->ctl_hi),
	(p_lli->ctl_lo),
	(FH_UINT32)phy_add);

}

static void dump_channel_reg(struct fh_axi_dma *p_dma, struct dma_transfer *p_transfer)
{
	struct dw_axi_dma *temp_dwc;

	FH_UINT32 chan_no = p_transfer->channel_number;

	temp_dwc = &p_dma->dwc;

	FH_AXI_DMA_PRINT("[CHAN : %d]SAR: 0x%x DAR: 0x%x LLP: 0x%x CTL: 0x%x:%08x CFG: 0x%x:%08x INTEN :0x%x INTSTATUS: 0x%x\n",
	chan_no,
	dw_readl(temp_dwc, CHAN[chan_no].SAR),
	dw_readl(temp_dwc, CHAN[chan_no].DAR),
	dw_readl(temp_dwc, CHAN[chan_no].LLP),
	dw_readl(temp_dwc, CHAN[chan_no].CTL_HI),
	dw_readl(temp_dwc, CHAN[chan_no].CTL_LO),
	dw_readl(temp_dwc, CHAN[chan_no].CFG_HI),
	dw_readl(temp_dwc, CHAN[chan_no].CFG_LO),
	dw_readl(temp_dwc, CHAN[chan_no].INTSTATUS_EN_LO),
	dw_readl(temp_dwc, CHAN[chan_no].INTSTATUS_LO)
	);

}

static void dump_dma_common_reg(struct fh_axi_dma *p_dma)
{
	struct dw_axi_dma *temp_dwc;

	temp_dwc = &p_dma->dwc;
	if (!temp_dwc->regs)
		return;
	FH_AXI_DMA_PRINT("ID: 0x%x COMPVER: 0x%x CFG: 0x%x CHEN: 0x%x:%08x INTSTATUS: 0x%x\
	COM_INTSTATUS_EN: 0x%x COM_INTSIGNAL_EN: %x COM_INTSTATUS: %x\n",
		dw_readl(temp_dwc, ID),
		dw_readl(temp_dwc, COMPVER),
		dw_readl(temp_dwc, CFG),
		dw_readl(temp_dwc, CHEN_HI),
		dw_readl(temp_dwc, CHEN_LO),
		dw_readl(temp_dwc, INTSTATUS),
		dw_readl(temp_dwc, COM_INTSTATUS_EN),
		dw_readl(temp_dwc, COM_INTSIGNAL_EN),
		dw_readl(temp_dwc, COM_INTSTATUS)
	);

}


static void dump_chan_xfer_info(struct fh_axi_dma *p_dma, struct dma_transfer *p_transfer)
{
	FH_UINT32 i;
	struct axi_dma_lli *p_lli;

	p_lli = (struct axi_dma_lli *)p_transfer->first_lli;
	dump_dma_common_reg(p_dma);
	dump_channel_reg(p_dma, p_transfer);
	for (i = 0; i < p_transfer->desc_size; i++) {
		dump_lli(&p_lli[i], p_dma->dma_channel[p_transfer->channel_number].base_lli,
		p_dma->dma_channel[p_transfer->channel_number].base_lli_phy);
	}
}

static FH_UINT32 allign_func(FH_UINT32 in_addr, FH_UINT32 allign_size, FH_UINT32 *phy_back_allign)
{
	*phy_back_allign = (*phy_back_allign + allign_size - 1) & (~(allign_size - 1));
	return (in_addr + allign_size - 1) & (~(allign_size - 1));
}

static FH_UINT32 vir_lli_to_phy_lli(struct axi_dma_lli *base_lli,
FH_UINT32 base_lli_phy, struct axi_dma_lli *cal_lli)
{
	FH_UINT32 ret;

	ret = base_lli_phy + ((FH_UINT32)cal_lli - (FH_UINT32)base_lli);
	return ret;
}

struct axi_dma_lli *get_desc(struct fh_axi_dma *p_dma,
struct dma_transfer *p_transfer, FH_UINT32 lli_size)
{
	struct axi_dma_lli *ret_lli;
	FH_UINT32 free_index;
	FH_UINT32 allign_left;
	FH_UINT32 totoal_desc;
	FH_UINT32 actual_get_desc = 0;
	FH_UINT32 totoal_free_desc;

	totoal_free_desc =
	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt;
	free_index  = p_dma->dma_channel[p_transfer->channel_number].free_index;
	totoal_desc = p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	allign_left = totoal_desc - free_index;

	/* check first.. */
	if (totoal_free_desc < lli_size) {
		FH_AXI_DMA_PRINT("not enough desc to get...\n");
		FH_AXI_DMA_PRINT("get size is %d,left is %d\n", lli_size, totoal_free_desc);
		return AXI_DMA_NULL;
	}

	if (lli_size > allign_left) {
		/* if allign desc not enough...just reset null.... */
		if ((totoal_free_desc - allign_left) < lli_size) {
			FH_AXI_DMA_PRINT("not enough desc to get...\n");
			FH_AXI_DMA_PRINT(
				"app need size is %d, totoal left is %d, allign left is %d\n",
				lli_size, totoal_free_desc, allign_left);
			FH_AXI_DMA_PRINT("from head to get desc size is %d, actual get is %d\n",
			(totoal_free_desc - allign_left),
			(allign_left + lli_size));
			return AXI_DMA_NULL;
		} else {
			actual_get_desc = allign_left + lli_size;
			free_index = 0;
		}
	}

	ret_lli = &p_dma->dma_channel[p_transfer->channel_number].base_lli[free_index];
	p_dma->dma_channel[p_transfer->channel_number].free_index +=
	actual_get_desc;
	p_dma->dma_channel[p_transfer->channel_number].free_index %=
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt -=
	actual_get_desc;
	p_transfer->lli_size = lli_size;
	p_transfer->actual_lli_size = actual_get_desc;
	return ret_lli;
}

FH_UINT32 put_desc(struct fh_axi_dma *p_dma, struct dma_transfer *p_transfer)
{
	FH_UINT32 lli_size;

	lli_size = p_transfer->actual_lli_size;
	p_dma->dma_channel[p_transfer->channel_number].used_index += lli_size;
	p_dma->dma_channel[p_transfer->channel_number].used_index %=
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt += lli_size;
	p_transfer->lli_size = 0;
	p_transfer->actual_lli_size = 0;
	return AXI_DMA_RET_OK;
}

#if defined CONFIG_ARCH_FH8636 || defined CONFIG_ARCH_FH8852V101
#define axi_dma_reset(...)
#else
static void axi_dma_reset(struct dw_axi_dma *axi_dma_obj)
{
	FH_UINT32 ret;
	FH_UINT32 time_out = 0xffffff;

	dw_writel(axi_dma_obj, RESET, 1);
	do {
		ret = dw_readl(axi_dma_obj, RESET);
		time_out--;
	} while (ret && time_out);

	if (!time_out)
		FH_AXI_DMA_PRINT("%s : time out..\n", __func__);
}
#endif

static void axi_dma_enable(struct dw_axi_dma *axi_dma_obj)
{
	FH_UINT32 ret;

	ret = dw_readl(axi_dma_obj, CFG);
	ret |= BIT(DMAC_EN_POS);
	dw_writel(axi_dma_obj, CFG, ret);
}

static void axi_dma_isr_common_enable(struct dw_axi_dma *axi_dma_obj)
{
	FH_UINT32 ret;

	ret = dw_readl(axi_dma_obj, CFG);
	ret |= BIT(INT_EN_POS);
	dw_writel(axi_dma_obj, CFG, ret);
}

static void axi_dma_isr_common_disable(struct dw_axi_dma *axi_dma_obj)
{
	FH_UINT32 ret;

	ret = dw_readl(axi_dma_obj, CFG);
	ret &= ~(BIT(INT_EN_POS));
	dw_writel(axi_dma_obj, CFG, ret);
}

static void handle_dma_open(struct fh_axi_dma *p_dma)
{
	struct dw_axi_dma *temp_dwc;

	temp_dwc = &p_dma->dwc;
	axi_dma_enable(temp_dwc);
	axi_dma_isr_common_enable(temp_dwc);
	p_dma->dwc.controller_status = CONTROLLER_STATUS_OPEN;
}

static void handle_dma_close(struct fh_axi_dma *p_dma)
{
	FH_UINT32 i;
	struct dw_axi_dma *temp_dwc;

	temp_dwc = &p_dma->dwc;
	/* take lock */
	for (i = 0; i < p_dma->dwc.channel_max_number; i++)
		p_dma->dma_channel[i].channel_status = CHANNEL_STATUS_CLOSED;

	axi_dma_reset(temp_dwc);
	p_dma->dwc.controller_status = CONTROLLER_STATUS_CLOSED;

}


static FH_UINT32 check_channel_real_free(struct fh_axi_dma *p_dma,
FH_UINT32 channel_number)
{
	struct dw_axi_dma *temp_dwc;
	FH_UINT32 ret_status;

	temp_dwc = &p_dma->dwc;
	AXI_DMA_ASSERT(channel_number < p_dma->dwc.channel_max_number);

	ret_status = dw_readl(temp_dwc, CHEN_LO);
	if (ret_status & lift_shift_bit_num(channel_number)) {
		FH_AXI_DMA_PRINT("channel_number : %d is busy....\n", channel_number);
		return CHANNEL_NOT_FREE;
	}
	return CHANNEL_REAL_FREE;
}

static FH_ERR handle_request_channel(struct fh_axi_dma *p_dma,
struct dma_transfer *p_transfer)
{
	FH_UINT32 i;
	FH_ERR ret_status = AXI_DMA_RET_OK;

	if (p_transfer->channel_number == AUTO_FIND_CHANNEL) {
		/* check each channel lock,find a free channel... */
		for (i = 0; i < p_dma->dwc.channel_max_number; i++) {
			ret_status = axi_dma_trylock(&p_dma->dma_channel[i].channel_lock);
			if (ret_status == AXI_DMA_RET_OK)
				break;
		}

		if (i < p_dma->dwc.channel_max_number) {
			ret_status = check_channel_real_free(p_dma, i);
			if (ret_status != CHANNEL_REAL_FREE) {
				FH_AXI_DMA_PRINT("auto request channel error\n");
				AXI_DMA_ASSERT(ret_status == CHANNEL_REAL_FREE);
			}
			/* caution : channel is already locked here.... */
			p_transfer->channel_number = i;
			/* bind to the controller. */
			/* p_transfer->dma_controller = p_dma; */
			p_dma->dma_channel[i].channel_status = CHANNEL_STATUS_OPEN;
		} else {
			FH_AXI_DMA_PRINT("[dma]: auto request err, no free channel\n");
			return -AXI_DMA_RET_NO_MEM;
		}
	}
    /* request channel by user */
	else {
		AXI_DMA_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);
		ret_status = axi_dma_lock(
			&p_dma->dma_channel[p_transfer->channel_number].channel_lock,
			AXI_DMA_TICK_PER_SEC * 2);
		if (ret_status != AXI_DMA_RET_OK) {
			FH_AXI_DMA_PRINT("[dma]: request %d channel err.\n", p_transfer->channel_number);
			return -AXI_DMA_RET_NO_MEM;
		}

		/*enter_critical();*/
		ret_status = check_channel_real_free(p_dma, p_transfer->channel_number);
		if (ret_status != CHANNEL_REAL_FREE) {
			FH_AXI_DMA_PRINT("user request channel error\n");
			AXI_DMA_ASSERT(ret_status == CHANNEL_REAL_FREE);
		}
		/* bind to the controller */
		/* p_transfer->dma_controller = p_dma; */
		p_dma->dma_channel[p_transfer->channel_number].channel_status =
		CHANNEL_STATUS_OPEN;
		/* exit_critical(); */
	}

	/* malloc desc for this one channel... */
	/* fix me.... */

	p_dma->dma_channel[p_transfer->channel_number].allign_malloc =
	(FH_UINT32)axi_dma_malloc_desc(p_dma->kernel_pri,
	(p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
	sizeof(struct axi_dma_lli)) +
	AXI_DESC_ALLIGN, &p_dma->dma_channel[p_transfer->channel_number].allign_phy);

	if (!p_dma->dma_channel[p_transfer->channel_number].allign_malloc) {
		/* release channel */
		FH_AXI_DMA_PRINT("[dma]: no mem to malloc channel%d desc..\n",
		p_transfer->channel_number);
		p_dma->dma_channel[p_transfer->channel_number].channel_status =
		CHANNEL_STATUS_CLOSED;
		axi_dma_unlock(
			&p_dma->dma_channel[p_transfer->channel_number].channel_lock);
		return -AXI_DMA_RET_NO_MEM;
	}

	p_dma->dma_channel[p_transfer->channel_number].base_lli_phy =
	p_dma->dma_channel[p_transfer->channel_number].allign_phy;
	p_dma->dma_channel[p_transfer->channel_number].base_lli =
	(struct axi_dma_lli *)allign_func(
		p_dma->dma_channel[p_transfer->channel_number].allign_malloc,
		AXI_DESC_ALLIGN, &p_dma->dma_channel[p_transfer->channel_number].base_lli_phy);

	if (!p_dma->dma_channel[p_transfer->channel_number].base_lli) {
		FH_AXI_DMA_PRINT("request desc failed..\n");
		AXI_DMA_ASSERT(p_dma->dma_channel[p_transfer->channel_number].base_lli !=
		AXI_DMA_NULL);
	}

	if ((FH_UINT32)p_dma->dma_channel[p_transfer->channel_number].base_lli %
	AXI_DESC_ALLIGN) {
		FH_AXI_DMA_PRINT("malloc is not cache allign..");
	}

	/* axi_dma_memset((void *)dma_trans_desc->first_lli, 0, lli_size * sizeof(struct */
	/* axi_dma_lli)); */
	axi_dma_memset((void *)p_dma->dma_channel[p_transfer->channel_number].base_lli,
	0, p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
	sizeof(struct axi_dma_lli));

	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt =
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	p_dma->dma_channel[p_transfer->channel_number].free_index = 0;
	p_dma->dma_channel[p_transfer->channel_number].used_index = 0;


	return AXI_DMA_RET_OK;
}

static void fhc_chan_able_set(struct fh_axi_dma *p_dma,
struct dma_transfer *p_transfer, FH_SINT32 enable)
{
	struct dw_axi_dma *temp_dwc;
	FH_SINT32 ret, ret_read;
	FH_UINT32 time_out = 0xffffff;
	FH_UINT32 chan_mask = lift_shift_bit_num(p_transfer->channel_number);

	temp_dwc = &p_dma->dwc;
	ret = dw_readl(temp_dwc, CHEN_LO);
	ret &= ~(chan_mask);
	ret |= enable ? ((chan_mask) | (chan_mask << 8)) :  (chan_mask << 8);

	dw_writel(temp_dwc, CHEN_LO, ret);
	/*enable == 1 do not check. maybe just isr break. the bit dma will self clear*/
	if (enable == 0) {
		do {
			ret_read = dw_readl(temp_dwc, CHEN_LO);
			time_out--;
		} while ((!!(ret_read & chan_mask) != enable) && time_out);
	}

	if (!time_out)
		dump_chan_xfer_info(p_dma, p_transfer);
}

static void fhc_chan_susp_set(struct fh_axi_dma *p_dma,
struct dma_transfer *p_transfer, FH_SINT32 enable)
{
	struct dw_axi_dma *temp_dwc;
	FH_SINT32 ret;
	FH_UINT32 time_out = 0xffffff;
	FH_SINT32 chan_no = p_transfer->channel_number;
	FH_UINT32 chan_mask = lift_shift_bit_num(p_transfer->channel_number);

	temp_dwc = &p_dma->dwc;
	ret = dw_readl(temp_dwc, CHEN_LO);
	ret &= ~(chan_mask << 16);
	ret |= enable ? ((chan_mask << 16) | (chan_mask << 24)) :  (chan_mask << 24);
	dw_writel(temp_dwc, CHEN_LO, ret);

	if (enable) {
		do {
			ret = dw_readl(temp_dwc, CHAN[chan_no].INTSTATUS_LO);
			time_out--;
		} while ((!!(ret & DWAXIDMAC_IRQ_SUSPENDED) != enable) && time_out);

		if (!time_out) {
			dump_chan_xfer_info(p_dma, p_transfer);
			return;
		}
		/*clear susp irp*/
		dw_writel(temp_dwc, CHAN[chan_no].INTCLEAR_LO, DWAXIDMAC_IRQ_SUSPENDED);
	}
}

static FH_UINT32 handle_release_channel(struct fh_axi_dma *p_dma,
struct dma_transfer *p_transfer)
{
	FH_UINT32 ret_status;
	/* enter_critical(); */
	ret_status = p_dma->dma_channel[p_transfer->channel_number].channel_status;

	AXI_DMA_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);

	if (ret_status == CHANNEL_STATUS_CLOSED) {
		FH_AXI_DMA_PRINT(
			"release channel error,reason: release a closed channel!!\n");
		AXI_DMA_ASSERT(ret_status != CHANNEL_STATUS_CLOSED);
	}
	fhc_chan_able_set(p_dma, p_transfer, 0);

	axi_dma_unlock(
		&p_dma->dma_channel[p_transfer->channel_number].channel_lock);
	/* p_transfer->dma_controller = AXI_DMA_NULL; */
	p_dma->dma_channel[p_transfer->channel_number].channel_status =
	CHANNEL_STATUS_CLOSED;
	p_dma->dma_channel[p_transfer->channel_number].open_flag =
	DEFAULT_TRANSFER;
	/* exit_critical(); */

	axi_dma_free_desc(p_dma->kernel_pri,
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
	sizeof(struct axi_dma_lli) + AXI_DESC_ALLIGN,
	(void *)p_dma->dma_channel[p_transfer->channel_number].allign_malloc,
	p_dma->dma_channel[p_transfer->channel_number].allign_phy);

	p_dma->dma_channel[p_transfer->channel_number].allign_malloc = AXI_DMA_NULL;
	p_dma->dma_channel[p_transfer->channel_number].base_lli = AXI_DMA_NULL;
	p_dma->dma_channel[p_transfer->channel_number].allign_phy = AXI_DMA_NULL;
	p_dma->dma_channel[p_transfer->channel_number].base_lli_phy = AXI_DMA_NULL;

	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt =
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	p_dma->dma_channel[p_transfer->channel_number].free_index = 0;
	p_dma->dma_channel[p_transfer->channel_number].used_index = 0;

	return AXI_DMA_RET_OK;
}

void axi_dma_ctl_init(FH_UINT32 *low, FH_UINT32 *high)
{
	/*cache*/
	*low |= CH_CTL_L_AR_CACHE(3) | CH_CTL_L_AW_CACHE(0);
	/*burst len*/
	*high |= CH_CTL_H_ARLEN_EN(1) | CH_CTL_H_ARLEN(0xf) | CH_CTL_H_AWLEN_EN(0) | CH_CTL_H_AWLEN(0xf);
}

void axi_dma_cfg_init(FH_UINT32 *low, FH_UINT32 *high)
{
	/*out standing*/
	*high |= CH_CFG_H_DST_OSR_LMT(7) | CH_CFG_H_SRC_OSR_LMT(7);
}

void handle_transfer(struct fh_axi_dma *p_dma,
struct dma_transfer *p_transfer)
{
	FH_UINT32 desc_size;
	FH_UINT32 i;
	FH_UINT32 lli_phy_add;
	FH_UINT32 dst_inc_mode;
	FH_UINT32 src_inc_mode;
	FH_UINT32 ret_status;
	FH_UINT32 cfg_low;
	FH_UINT32 cfg_high;
	struct dma_transfer *dma_trans_desc;
	struct axi_dma_lli *p_lli_base;
	FH_UINT32 lli_phy_base;
	struct axi_dma_lli *p_lli = AXI_DMA_NULL;
	struct dw_axi_dma *temp_dwc;
	struct dma_transfer_desc *p_head_desc;

	FH_UINT32 isr = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_BLOCK_TRF | DWAXIDMAC_IRQ_SUSPENDED;
	FH_UINT32 isr_sig = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_BLOCK_TRF;

	dma_trans_desc = p_transfer;
	temp_dwc = &p_dma->dwc;
	p_head_desc = p_transfer->p_desc;
	desc_size = p_transfer->desc_size;
	p_transfer->dma_controller->dma_channel[p_transfer->channel_number].active_trans = dma_trans_desc;

	dma_trans_desc->first_lli = get_desc(p_dma, p_transfer, desc_size);

	/* not enough mem.. */
	if (dma_trans_desc->first_lli == AXI_DMA_NULL) {
		FH_AXI_DMA_PRINT("transfer error,reason: not enough mem..\n");
		AXI_DMA_ASSERT(dma_trans_desc->first_lli != AXI_DMA_NULL);
	}
	axi_dma_invalidate_desc((FH_UINT32)dma_trans_desc->first_lli,
	desc_size * sizeof(struct axi_dma_lli));
	axi_dma_memset((void *)dma_trans_desc->first_lli, 0,
	desc_size * sizeof(struct axi_dma_lli));
	p_lli = dma_trans_desc->first_lli;
	lli_phy_base = p_dma->dma_channel[p_transfer->channel_number].base_lli_phy;
	p_lli_base = p_dma->dma_channel[p_transfer->channel_number].base_lli;

	AXI_DMA_ASSERT(((FH_UINT32)p_lli & AXI_DESC_ALLIGN_BIT_MASK) == 0);
	for (i = 0; i < desc_size; i++, p_head_desc++) {
		/*parse each desc*/
		dst_inc_mode = (p_head_desc->dst_inc_mode == DW_DMA_SLAVE_INC) ? 0 : 1;
		src_inc_mode = (p_head_desc->src_inc_mode == DW_DMA_SLAVE_INC) ? 0 : 1;
		p_lli[i].sar_lo = p_head_desc->src_add;
		p_lli[i].dar_lo = p_head_desc->dst_add;
		p_lli[i].ctl_lo =
		AXI_DMA_CTLL_DST_WIDTH(p_head_desc->dst_width) |
		AXI_DMA_CTLL_SRC_WIDTH(p_head_desc->src_width) |
		AXI_DMA_CTLL_DST_MSIZE(p_head_desc->dst_msize) |
		AXI_DMA_CTLL_SRC_MSIZE(p_head_desc->src_msize) |
		AXI_DMA_CTLL_DST_INC_MODE(dst_inc_mode) |
		AXI_DMA_CTLL_SRC_INC_MODE(src_inc_mode);
		p_lli[i].ctl_hi = CH_CTL_H_LLI_VALID;
		if (p_dma->dma_channel[p_transfer->channel_number].open_flag == CYCLIC_TRANSFER)
			p_lli[i].ctl_hi |= CH_CTL_H_IOC_BLKTFR;

		axi_dma_ctl_init(&p_lli[i].ctl_lo, &p_lli[i].ctl_hi);

		p_lli[i].block_ts_lo = p_head_desc->size - 1;

		if ((i + 1) < desc_size) {
			lli_phy_add = vir_lli_to_phy_lli(p_lli_base, lli_phy_base, &p_lli[i + 1]);
			p_lli[i].llp_lo = lli_phy_add;
		} else {
			if (p_dma->dma_channel[p_transfer->channel_number].open_flag == SINGLE_TRANSFER)
				p_lli[i].ctl_hi |= CH_CTL_H_LLI_LAST;
			else {
				/*cyclic make a ring..*/
				lli_phy_add = vir_lli_to_phy_lli(p_lli_base, lli_phy_base, &p_lli[0]);
				p_lli[i].llp_lo = lli_phy_add;
			}
		}
		axi_dma_clean_desc((FH_UINT32)&p_lli[i],
		sizeof(struct axi_dma_lli));
#ifdef AXI_DMA_DEBUG
		dump_lli(&p_lli[i], p_lli_base, lli_phy_base);
#endif
	}

	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO,
	3 << CH_CFG_L_DST_MULTBLK_TYPE_POS | 3 << CH_CFG_L_SRC_MULTBLK_TYPE_POS);
	/* set flow mode */
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI,
	AXI_DMA_CFGH_FC(dma_trans_desc->fc_mode));
	/* set base link add */
	lli_phy_add = vir_lli_to_phy_lli(p_lli_base, lli_phy_base, &p_lli[0]);
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].LLP, lli_phy_add);
	/* clear isr */
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTCLEAR_LO, 0xffffffff);
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTCLEAR_HI, 0xffffffff);
	/* open isr */
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTSTATUS_EN_LO, isr);
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTSIGNAL_LO, isr_sig);
	AXI_DMA_ASSERT(dma_trans_desc->dst_hs <= DMA_SW_HANDSHAKING);
	AXI_DMA_ASSERT(dma_trans_desc->src_hs <= DMA_SW_HANDSHAKING);
	/* only hw handshaking need this.. */
	switch (dma_trans_desc->fc_mode) {
	case DMA_M2M:
		break;
	case DMA_M2P:
		ret_status = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
		ret_status &= ~0x18;
		ret_status |= dma_trans_desc->dst_per << 12;
		dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, ret_status);
		break;
	case DMA_P2M:
		ret_status = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
		ret_status &= ~0x18;
		ret_status |= dma_trans_desc->src_per << 7;
		dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, ret_status);
		break;
	default:
		break;
	}

	/*rewrite cfg..*/
	cfg_low = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO);
	cfg_high = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
	axi_dma_cfg_init(&cfg_low, &cfg_high);
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO, cfg_low);
	dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, cfg_high);

	dma_trans_desc->dma_controller
	->dma_channel[dma_trans_desc->channel_number]
	.channel_status = CHANNEL_STATUS_BUSY;

	if (dma_trans_desc->prepare_callback)
		dma_trans_desc->prepare_callback(dma_trans_desc->prepare_para);

#ifdef AXI_DMA_DEBUG
	dump_dma_common_reg(p_dma);
	dump_channel_reg(p_dma, dma_trans_desc);
#endif
	fhc_chan_able_set(p_dma, dma_trans_desc, 1);
}

static void handle_single_transfer(struct fh_axi_dma *p_dma,
struct dma_transfer *p_transfer)
{
	FH_UINT32 ret_status;
	axi_dma_list *p_controller_list;
	FH_UINT32 max_trans_size;
	struct dma_transfer *dma_trans_desc;
	struct dma_transfer *_dma_trans_desc;

	AXI_DMA_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);
	AXI_DMA_ASSERT(p_transfer->dma_number < DMA_CONTROLLER_NUMBER);
	/* when the dma transfer....the lock should be 0!!!! */
	/* or user may not request the channel... */

	ret_status = p_dma->dma_channel[p_transfer->channel_number].channel_status;
	if (ret_status == CHANNEL_STATUS_CLOSED) {
		FH_AXI_DMA_PRINT("transfer error,reason: use a closed channel..\n");
		AXI_DMA_ASSERT(ret_status != CHANNEL_STATUS_CLOSED);
	}
	p_transfer->dma_controller = p_dma;

	axi_dma_list_init(&p_transfer->transfer_list);
	max_trans_size =
	p_transfer->dma_controller->dma_channel[p_transfer->channel_number]
	.desc_trans_size;
	if (p_transfer->period_len != 0)
		max_trans_size = axi_dma_min(max_trans_size, p_transfer->period_len);

	/* add transfer to the controller's queue list */
	/* here should  insert before and handle after....this could be a fifo... */
	axi_dma_list_insert_before(&p_dma->dma_channel[p_transfer->channel_number].queue,
	&p_transfer->transfer_list);

	p_controller_list = &p_dma->dma_channel[p_transfer->channel_number].queue;

	/* here the driver could make a queue to cache the transfer and kick a */
	/* thread to handle the queue~~~ */
	/* but now,this is a easy version...,just handle the transfer now!!! */
	axi_dma_list_for_each_entry_safe(dma_trans_desc, _dma_trans_desc,
	p_controller_list, transfer_list) {
		handle_transfer(p_dma, dma_trans_desc);
	}

}

FH_SINT32 axi_dma_driver_control(struct fh_axi_dma *dma,
FH_UINT32 cmd, struct dma_transfer *arg)
{
	struct fh_axi_dma *my_own = dma;
	struct dw_axi_dma *temp_dwc = &my_own->dwc;
	FH_ERR ret = AXI_DMA_RET_OK;
	struct dma_transfer *p_dma_transfer = arg;

	switch (cmd) {
	case AXI_DMA_CTRL_DMA_OPEN:
		/* open the controller.. */
		handle_dma_open(my_own);
		break;
	case AXI_DMA_CTRL_DMA_CLOSE:
		/* close the controller.. */
		handle_dma_close(my_own);
		break;
	case AXI_DMA_CTRL_DMA_REQUEST_CHANNEL:
		/* request a channel for the user */
		ret = handle_request_channel(my_own, p_dma_transfer);
		break;
	case AXI_DMA_CTRL_DMA_RELEASE_CHANNEL:
		/* release a channel */
		AXI_DMA_ASSERT(p_dma_transfer != AXI_DMA_NULL);
		ret = handle_release_channel(my_own, p_dma_transfer);
		break;
	case AXI_DMA_CTRL_DMA_SINGLE_TRANSFER:
		/* make a channel to transfer data. */
		AXI_DMA_ASSERT(p_dma_transfer != AXI_DMA_NULL);
		/* check if the dma channel is open,or return error. */
		my_own->dma_channel[p_dma_transfer->channel_number].open_flag =
		SINGLE_TRANSFER;
		handle_single_transfer(my_own, p_dma_transfer);
		/* then wait for the channel is complete.. */
		/* caution that::we should be in the "enter_critical()"when set the */
		/* dma to work. */
		break;

	case AXI_DMA_CTRL_DMA_CYCLIC_PREPARE:
		AXI_DMA_ASSERT(p_dma_transfer != AXI_DMA_NULL);
		my_own->dma_channel[p_dma_transfer->channel_number].open_flag =
		CYCLIC_TRANSFER;
		fh_axi_dma_cyclic_prep(my_own, p_dma_transfer);
		break;

	case AXI_DMA_CTRL_DMA_CYCLIC_START:
		fh_axi_dma_cyclic_start(p_dma_transfer);
		break;

	case AXI_DMA_CTRL_DMA_CYCLIC_STOP:
		fh_axi_dma_cyclic_stop(p_dma_transfer);
		break;

	case AXI_DMA_CTRL_DMA_CYCLIC_FREE:
		fh_axi_dma_cyclic_free(p_dma_transfer);
		break;
	case AXI_DMA_CTRL_DMA_PAUSE:
		fh_axi_dma_cyclic_pause(p_dma_transfer);
		break;
	case AXI_DMA_CTRL_DMA_RESUME:
		fh_axi_dma_cyclic_resume(p_dma_transfer);
		break;
	case AXI_DMA_CTRL_DMA_GET_DAR:
		return dw_readl(temp_dwc, CHAN[p_dma_transfer->channel_number].DAR);
	case AXI_DMA_CTRL_DMA_GET_SAR:
		return dw_readl(temp_dwc, CHAN[p_dma_transfer->channel_number].SAR);

	default:
		break;
	}

	return ret;
}

static void fh_isr_single_process(struct fh_axi_dma *my_own,
struct dma_transfer *p_transfer)
{
	if (p_transfer->isr_prepare_callback)
		p_transfer->isr_prepare_callback(p_transfer->isr_prepare_para);

	if (p_transfer->complete_callback)
		p_transfer->complete_callback(p_transfer->complete_para);
	p_transfer->dma_controller
	->dma_channel[p_transfer->channel_number]
	.channel_status = CHANNEL_STATUS_IDLE;
	put_desc(my_own, p_transfer);
	axi_dma_list_remove(&p_transfer->transfer_list);
}

static void fh_isr_cyclic_process(struct fh_axi_dma *my_own,
struct dma_transfer *p_transfer)
{
	FH_UINT32 index;
	struct dw_axi_dma *temp_dwc;
	struct axi_dma_lli *p_lli;

	temp_dwc = &my_own->dwc;
	p_lli = AXI_DMA_NULL;

	if (p_transfer->isr_prepare_callback)
		p_transfer->isr_prepare_callback(p_transfer->isr_prepare_para);

	if (p_transfer->complete_callback)
		p_transfer->complete_callback(p_transfer->complete_para);
	p_lli = p_transfer->first_lli;
	/*invaild desc mem to cache...*/
	axi_dma_invalidate_desc((uint32_t)p_lli,
	sizeof(struct axi_dma_lli) * p_transfer->cyclic_periods);
	for (index = 0; index < p_transfer->cyclic_periods; index++) {
		if (!(p_lli[index].ctl_hi & CH_CTL_H_LLI_VALID))
			p_lli[index].ctl_hi |= CH_CTL_H_LLI_VALID;
	}
	/*flush cache..*/
	axi_dma_clean_desc((uint32_t)p_lli,
	sizeof(struct axi_dma_lli) * p_transfer->cyclic_periods);
	/*kick dma again.*/
	dw_writel(temp_dwc, CHAN[p_transfer->channel_number].BLK_TFR_RESU, 1);
}

static void dma_channel_isr_enable(struct dw_axi_dma *dwc, int chan, int val)
{
	dw_writel(dwc, CHAN[chan].INTSIGNAL_LO, val);
}

static int dma_channel_isr_disable(struct dw_axi_dma *dwc, int chan)
{
	int raw = dw_readl(dwc, CHAN[chan].INTSIGNAL_LO);

	dw_writel(dwc, CHAN[chan].INTSIGNAL_LO, 0);
	return raw;
}

void fh_axi_dma_isr_enable_set(struct fh_axi_dma *param, FH_UINT32 enable)
{
	struct fh_axi_dma *my_own = param;
	struct dw_axi_dma *temp_dwc = &my_own->dwc;
	FH_UINT32 i;

	/* enable channel int */
	for (i = 0; i < my_own->dwc.channel_max_number; i++) {
		if (enable) {
			int int_sig = my_own->dma_channel[i].int_sig;

			if (int_sig) {
				dma_channel_isr_enable(temp_dwc, i, int_sig);
				my_own->dma_channel[i].int_sig = 0;
			}
		} else {
			int status = dw_readl(temp_dwc, CHAN[i].INTSTATUS_LO)
				& (~DWAXIDMAC_IRQ_SUSPENDED);
			if (!status)
				continue;
			/* backup channel int, then disable it */
			my_own->dma_channel[i].status = status;
			my_own->dma_channel[i].int_sig =
				dma_channel_isr_disable(temp_dwc, i);
			/* clear int */
			dw_writel(temp_dwc, CHAN[i].INTCLEAR_LO, status);
			if (status & DWAXIDMAC_IRQ_ALL_ERR) {
				FH_AXI_DMA_PRINT("bug status is %x\n", status);
				AXI_DMA_ASSERT(0);
			}
		}
	}
}

void fh_axi_dma_isr(struct fh_axi_dma *param)
{
	FH_UINT32 status;
	FH_UINT32 i;
	struct dw_axi_dma *temp_dwc;
	struct fh_axi_dma *my_own = param;
	temp_dwc = &my_own->dwc;

	for (i = 0; i < my_own->dwc.channel_max_number; i++) {
		if (!my_own->dma_channel[i].status)
			continue;
		status = my_own->dma_channel[i].status;
		if (my_own->dma_channel[i].open_flag == SINGLE_TRANSFER) {
			if (status & DWAXIDMAC_IRQ_DMA_TRF) {
				fh_isr_single_process(my_own,
				my_own->dma_channel[i].active_trans);
			}
		}
		if (my_own->dma_channel[i].open_flag == CYCLIC_TRANSFER) {
			if (status & DWAXIDMAC_IRQ_BLOCK_TRF) {
				fh_isr_cyclic_process(my_own,
				my_own->dma_channel[i].active_trans);
			}
		}
		my_own->dma_channel[i].status = 0;
	}
}

const char *channel_lock_name[AXI_DMA_MAX_NR_CHANNELS] = {
	"channel_0_lock", "channel_1_lock", "channel_2_lock", "channel_3_lock",
	"channel_4_lock", "channel_5_lock", "channel_6_lock", "channel_7_lock",
};

static void fh_axi_dma_cyclic_pause(struct dma_transfer *p)
{
	struct fh_axi_dma *my_own = p->dma_controller;

	fhc_chan_susp_set(my_own, p, 1);
}

static void fh_axi_dma_cyclic_resume(struct dma_transfer *p)
{
	struct fh_axi_dma *my_own = p->dma_controller;

	fhc_chan_susp_set(my_own, p, 0);
}

static void fh_axi_dma_cyclic_stop(struct dma_transfer *p)
{
	struct fh_axi_dma *my_own = p->dma_controller;

	fhc_chan_susp_set(my_own, p, 1);
	fhc_chan_able_set(my_own, p, 0);
}

static void fh_axi_dma_cyclic_start(struct dma_transfer *p)
{
	struct fh_axi_dma *my_own = p->dma_controller;
	struct dw_axi_dma *dwc;
	struct axi_dma_lli *p_lli = AXI_DMA_NULL;

	dwc = &my_own->dwc;
	p_lli = p->first_lli;

	/* warnning!!!!must check if the add is 64Byte ally... */
	AXI_DMA_ASSERT(((FH_UINT32)p_lli & AXI_DESC_ALLIGN_BIT_MASK) == 0);
	handle_transfer(my_own, p);
}

static void fh_axi_dma_cyclic_prep(struct fh_axi_dma *fh_dma_p,
struct dma_transfer *p)
{
	/* bind the controller to the transfer */
	p->dma_controller = fh_dma_p;
	/* bind active transfer */
	fh_dma_p->dma_channel[p->channel_number].active_trans = p;
}

static void fh_axi_dma_cyclic_free(struct dma_transfer *p)
{
	struct fh_axi_dma *my_own = p->dma_controller;

	put_desc(my_own, p);
}

FH_UINT32 cal_axi_dma_channel(void *regs)
{
#ifdef DMA_FIXED_CHANNEL_NUM
	return DMA_FIXED_CHANNEL_NUM;
#else
	FH_UINT32 ret;
	FH_UINT32 i;
	FH_UINT32 time_out = 0xffffff;

	/*reset first..*/
	__dma_raw_writel(1, &(((struct dw_axi_dma_regs *)regs)->RESET));
	do {
		ret = __dma_raw_readl(&(((struct dw_axi_dma_regs *)regs)->RESET));
		time_out--;
	} while (ret && time_out);

	if (!time_out)
		FH_AXI_DMA_PRINT("%s :: %d : time out..\n", __func__, __LINE__);
	time_out = 0xffffff;
	/*enable dma.*/
	__dma_raw_writel(1, &(((struct dw_axi_dma_regs *)regs)->CFG));

	/*write channel.*/
	for (i = 0; i < AXI_DMA_MAX_NR_CHANNELS; i++) {
		__dma_raw_writel(1 << i | 1 << (8+i), &(((struct dw_axi_dma_regs *)regs)->CHEN_LO));
		ret = __dma_raw_readl(&(((struct dw_axi_dma_regs *)regs)->CHEN_LO));
		if (ret < 1 << i)
			break;
	}

	/*reset again*/
	__dma_raw_writel(1, &(((struct dw_axi_dma_regs *)regs)->RESET));
	do {
		ret = __dma_raw_readl(&(((struct dw_axi_dma_regs *)regs)->RESET));
		time_out--;
	} while (ret && time_out);

	if (!time_out)
		FH_AXI_DMA_PRINT("%s :: %d : time out..\n", __func__, __LINE__);

	return i;
#endif
}


struct axi_dma_ops *get_fh_axi_dma_ops(struct fh_axi_dma *p_axi_dma)
{
	AXI_DMA_ASSERT(p_axi_dma != AXI_DMA_NULL);
	return &p_axi_dma->ops;
}


void get_desc_para(struct fh_axi_dma *dma, struct dma_transfer *p_transfer,
FH_UINT32 *desc_size, FH_UINT32 *base_phy_add,
FH_UINT32 *each_desc_size, FH_UINT32 *each_desc_cap)
{
	*desc_size = dma->dma_channel[p_transfer->channel_number]
	.desc_total_no;
	*base_phy_add = dma->dma_channel[p_transfer->channel_number]
	.base_lli_phy;
	*each_desc_size = sizeof(struct axi_dma_lli);
	*each_desc_cap = dma->dma_channel[p_transfer->channel_number]
	.desc_trans_size;
}

struct fh_axi_dma *fh_axi_dma_probe(struct axi_dma_platform_data *priv_data)
{
	FH_UINT32 i;
	struct fh_axi_dma *fh_dma_p;
	struct axi_dma_platform_data *plat = priv_data;

	fh_dma_p = (struct fh_axi_dma *)axi_dma_malloc(sizeof(struct fh_axi_dma));
	if (!fh_dma_p) {
		FH_AXI_DMA_PRINT("ERROR: %s, malloc failed\n", __func__);
		return AXI_DMA_NULL;
	}
	axi_dma_memset(fh_dma_p, 0, sizeof(struct fh_axi_dma));
	axi_dma_scanf(fh_dma_p->dwc.name, "%s%d",
	plat->name, (FH_SINT32)plat->id);

	fh_dma_p->dwc.regs = (void *)plat->base;
	fh_dma_p->dwc.channel_max_number =
	cal_axi_dma_channel(fh_dma_p->dwc.regs);
	fh_dma_p->dwc.controller_status = CONTROLLER_STATUS_CLOSED;
	fh_dma_p->dwc.id = plat->id;
	fh_dma_p->kernel_pri = plat->kernel_pri;
	fh_dma_p->adapt_pri = plat->adapt_pri;
	fh_dma_p->ops.axi_dma_isr_process = fh_axi_dma_isr;
	fh_dma_p->ops.axi_dma_control = axi_dma_driver_control;
	fh_dma_p->ops.axi_dma_get_desc_para = get_desc_para;
	fh_dma_p->ops.axi_dma_isr_enable_set = fh_axi_dma_isr_enable_set;

	/* channel set */
	for (i = 0; i < fh_dma_p->dwc.channel_max_number; i++) {
		fh_dma_p->dma_channel[i].channel_status = CHANNEL_STATUS_CLOSED;
		fh_dma_p->dma_channel[i].desc_total_no  = DESC_MAX_SIZE;
		fh_dma_p->dma_channel[i].int_sig = 0;
		fh_dma_p->dma_channel[i].status = 0;
		axi_dma_list_init(&(fh_dma_p->dma_channel[i].queue));
		fh_dma_p->dma_channel[i].desc_trans_size =
		FH_CHANNEL_MAX_TRANSFER_SIZE;
		axi_dma_lock_init(&fh_dma_p->dma_channel[i].channel_lock,
		channel_lock_name[i]);
	}
	return fh_dma_p;
}

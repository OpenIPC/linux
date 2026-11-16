#ifndef __VO__H__
#define __VO__H__

#define VO_ADDR_BASE	       0xB30C0000
#define VO_ADDR_VER			   0x000
#define VO_ADDR_TOP_CON	       0x004
#define VO_ADDR_TOP_STATUS     0x008
#define VO_ADDR_INT_EN         0x00C
#define VO_ADDR_INT_VAL        0x010
#define VO_ADDR_INT_CLC        0x014
#define VO_ADDR_VC0_SIZE       0x100
#define VO_ADDR_VC0_STRIDE     0x104
#define VO_ADDR_VC0_Y_ADDR     0x108
#define VO_ADDR_VC0_UV_ADDR    0x10C
#define VO_ADDR_VC0_BLK_INFO   0x110
#define VO_ADDR_VC0_CON_PAR    0x114
#define VO_ADDR_VC0_VBLK_PAR   0x118
#define VO_ADDR_VC0_HBLK_PAR   0x11C
#define VO_ADDR_VC0_TRAN_EN    0x120
#define VO_ADDR_VC0_TIME_OUT   0x124
#define VO_ADDR_VC0_ADDR_INFO  0x128
#define VO_ADDR_VC0_YUV_CLIP   0x130


#define u32 unsigned int

struct vo_buf_info {
	unsigned int vaddr_alloc;
	unsigned int paddr;
	unsigned int paddr_align;
	unsigned int size;
};

struct vo_reg_struct {
	char *name;
	unsigned int addr;
};
struct jz_vo {
	int irq;
	char name[16];

	struct clk *clk;
	struct clk *clk_mux;
	struct clk *clk_div;
	struct clk *ahb0_gate; /* T31 IPU mount at AHB1*/
	void __iomem *iomem;
	struct device *dev;
	struct resource *res;
	struct miscdevice misc_dev;

	struct mutex mutex;
	struct mutex irq_mutex;
	struct completion done_vo;
	struct completion done_buf;
	struct vo_buf_info pbuf;
};

struct vo_flush_cache_para
{
	void *addr;
	unsigned int size;
};

struct vo_param
{
	unsigned int 		src_w;
	unsigned int 		src_h;
	unsigned int 		bt_mode;
    unsigned int        line_full_thr;

    unsigned int        vfb_num;
    unsigned int        vbb_num;
    unsigned int        hfb_num;

    unsigned int		src_addr_y;
	unsigned int		src_addr_uv;

	unsigned int		src_y_strid;
	unsigned int		src_uv_strid;

};

static inline unsigned int reg_read(struct jz_vo *jzvo, int offset)
{
	return readl(jzvo->iomem + offset);
}

static inline void reg_write(struct jz_vo *jzvo, int offset, unsigned int val)
{
	writel(val, jzvo->iomem + offset);
}

#define __vo_irq_clear(value)					reg_bit_set(vo,VO_ADDR_INT_CLC, value)
#define __release_vo_irq_clear(value)				reg_bit_clr(vo,VO_ADDR_INT_CLC, value)
#define __start_vo()						reg_bit_set(vo, VO_ADDR_VC0_TRAN_EN, 1<<0)
#define __reset_vo()						reg_bit_set(vo, VO_ADDR_TOP_CON, 1<<1)
#define __release_reset_vo()					reg_bit_clr(vo, VO_ADDR_TOP_CON, 1<<1)
#define __reset_vo_dma()					reg_bit_set(vo, VO_ADDR_TOP_CON, 1<<0)
#define __release_vo_dma()					reg_bit_clr(vo, VO_ADDR_TOP_CON, 1<<0)

#define JZVO_IOC_MAGIC  'D'
#define IOCTL_VO_INIT			_IO(JZVO_IOC_MAGIC, 106)
#define IOCTL_VO_START	    	_IO(JZVO_IOC_MAGIC, 107)
#define IOCTL_VO_END		    _IO(JZVO_IOC_MAGIC, 108)
#define IOCTL_VO_BUF_FLUSH_CACHE	_IO(JZVO_IOC_MAGIC, 109)
#endif

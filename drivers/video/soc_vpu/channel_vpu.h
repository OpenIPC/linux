#ifndef __CHANNEL_VPU_H__
#define __CHANNEL_VPU_H__

#include <stdbool.h>

enum workphase {
	OPEN	= 0,
	WORKING,
	CLOSE,
};

enum codecdir {
	HWH264ENC = 0,
	HWH264DEC,
	HWH265ENC = (1 << 4),
	HWH265DEC,
	HWJPEGENC = (1 << 16),
	HWJPEGDEC,
};

#define RANDOM_ID	    (-1)
#define JZ_NVPU_ID      (0x1 << 24)
#define VPU_HELIX_ID    (0x1 << 25)
#define VPU_RADIX_ID    (0x1 << 26)
#define VPU_RADOM_BM    (0x7f << 24)

#define UNUSED_TID	    (-1)

struct channel_node {
	struct list_head	*clist;	/* channel list node */
	struct list_head	*vlist;	/* vpu list node */
	unsigned int		mdelay;	/* delay num ms when request channel or vpu */
	unsigned int		channel_id; /* requested channel id which is used to set thread id */
	int					vpu_id; /* request vpu id */
	unsigned int		codecdir; /* encoder(jpeg or h264 compress) or decoder request */
	enum workphase		workphase; /* need be set accoding to you work */
	unsigned int		status;	/* vpu finish status */
	unsigned int		output_len; /* vpu finish status */
	unsigned int		dma_addr; /* vconfig dma space addr, virtual or physical */
	int					thread_id; /* used in h264 chain mode, if key frame: >=0, else < 0
									* if not chain mode, must be < 0 */
	unsigned int		cmpx;   /* complex */

	unsigned int		n_flag; /*ncu start flag*/
	void *				ncu_addr; /* ncu config addr, virtual*/
	unsigned int        frame_type;
};

#if 0
enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};
#endif

struct flush_cache_info {
	unsigned int	addr;
	unsigned int	len;
#define WBACK		DMA_TO_DEVICE
#define INV		DMA_FROM_DEVICE
#define WBACK_INV	DMA_BIDIRECTIONAL
	unsigned int	dir;
};

struct buf_info {
	unsigned int	alloc_vaddr;
	unsigned int	alloc_paddr;
	unsigned int	alloc_size;
	unsigned int	align_value;
	unsigned int	align_vaddr;
	unsigned int	align_paddr;
	unsigned int	align_size;
	bool		tlb_needed;
	bool		tlb_maped;
};

struct init_buf_info {
	struct list_head	*clist;
	struct buf_info		buf_info;
};

enum reg_dir {
	READ_DIR = 0,
	WRITE_DIR,
};

struct reg_info {
	unsigned int	paddr;	/* physical reg addr */
	unsigned int	value;	/* read: read value, write: write value */
	enum reg_dir	dir;	/* read or write command */
};

/* IOCTL MACRO DEFINE PLACE */
#define SOC_VPU_MAGIC			'c'
#define IOCTL_CHANNEL_REQ		_IOWR(SOC_VPU_MAGIC, 0, struct channel_node)
#define IOCTL_CHANNEL_REL		_IOWR(SOC_VPU_MAGIC, 1, struct channel_node)
#define IOCTL_CHANNEL_RUN		_IOWR(SOC_VPU_MAGIC, 2, struct channel_node)
#define IOCTL_CHANNEL_START		_IOWR(SOC_VPU_MAGIC, 3, struct channel_node)
#define IOCTL_CHANNEL_WAIT_COMPLETE	_IOWR(SOC_VPU_MAGIC, 4, struct channel_node)
#define IOCTL_CHANNEL_FLUSH_CACHE	_IOWR(SOC_VPU_MAGIC, 5, struct channel_node)
#define IOCTL_CHANNEL_BUF_INIT		_IOWR(SOC_VPU_MAGIC, 6, struct channel_node)
#define IOCTL_CHANNEL_WOR_VPU_REG	_IOWR(SOC_VPU_MAGIC, 7, struct channel_node)
#define IOCTL_CHANNEL_VPU_SUSPEND	_IOWR(SOC_VPU_MAGIC, 8, struct channel_node)
#define IOCTL_CHANNEL_VPU_RESUME	_IOWR(SOC_VPU_MAGIC, 9, struct channel_node)
#define IOCTL_CHANNEL_PRIVATE_TLB	_IOWR(SOC_VPU_MAGIC, 10, struct channel_node)

#endif	//__CHANNEL_VPU_H__

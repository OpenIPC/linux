#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <linux/types.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include "channel_vpu.h"

#define SOC_VPU_VERSION     "1.0.0-03203fd46d"
#define FRAME_TYPE_IVDC     (3)
#define IVDC_BASE_ADDR      (0x13200000)

extern int vpu_register(struct list_head *vlist);
extern int vpu_unregister(struct list_head *vlist);

enum channel_phase {
	INIT_CHANNEL	= -1,
	REQUEST_CHANNEL = 0,
	FLUSH_CACHE,
	RUN_CHANNEL,
	RELEASE_CHANNEL,
	CHANNEL_FINISHED,
};

struct channel_tlb_vaddrmanager {
	struct list_head vaddr_entry;
	unsigned int vaddr;
	unsigned int size;
};

struct channel_tlb_pidmanager {
	pid_t tgid;
	unsigned int tlbbase;
	unsigned int private_tlbbase;
	struct list_head vaddr_list;
};


struct channel_list {
	struct list_head		list;
	int				id;
	enum channel_phase		phase;
	spinlock_t			slock;
	pid_t				tgid;		/* current process id */
	pid_t				pid;		/* current thread id */
	bool				tlb_flag;
	bool				private_tlb_flag;
	struct channel_tlb_pidmanager	*tlb_pidmanager;
};

struct free_channel_list {
	struct list_head	fclist_head;
	struct completion	cdone;		/* channel complete */
	spinlock_t		slock;		/* done lock */
};

enum vpu_phase {
	INIT_VPU = -1,
	OPEN_VPU = 0,
	REQUEST_VPU,
	RUN_VPU,
	COMPLETE_VPU,
	RELASE_VPU,
};

#define FIND_VPU_TRY_TIME_THRESHOLD (100)

struct vpu_list {
	struct list_head	list;		/* the list of this struct list */
	struct list_head	*vlist;		/* the list of vpu list to be register */
	enum vpu_phase		phase;
	int			user_cnt;	/* the child vpu use times */
	pid_t			tgid;		/* current process id */
	pid_t			pid;		/* current thread id */
	spinlock_t		slock;
};

struct free_vpu_list {
	struct list_head	fvlist_head;
	struct completion	vdone;		/* channel complete */
	spinlock_t		slock;		/* done lock */
};

struct soc_channel {
	struct list_head	*fclist_head;	/* free channel list head */
	struct list_head	*fvlist_head;	/* free vpu list head */
	struct miscdevice	mdev;		/* miscdevice */
	spinlock_t		cnt_slock;		/* done lock */
	int			user_cnt;
};

struct vpu_ops {
	struct module *owner;
	long (*open)(struct device *dev);
	long (*release)(struct device *dev);
	long (*start_vpu)(struct device *dev, const struct channel_node * const cnode);
	long (*wait_complete)(struct device *dev, struct channel_node * const cnode);
	long (*reset)(struct device *dev);
	long (*suspend)(struct device *dev);
	long (*resume)(struct device *dev);
};

enum jz_vpu_status {
	VPU_STATUS_CLOSE,
	VPU_STATUS_OPEN,
};

struct vpu {
	struct list_head	vlist;
	int                 vpu_id;
    int                 idx;
	struct device		*dev;
	struct vpu_ops		*ops;
};

#define vpu_readl(vpu, offset)		__raw_readl((vpu)->iomem + offset)

#define vpu_writel(vpu, offset, value)	__raw_writel((value), (vpu)->iomem + offset)

#define CLEAR_VPU_BIT(vpu,offset,bm)				\
	do {							\
		unsigned int stat;				\
		stat = vpu_readl(vpu,offset);			\
		vpu_writel(vpu,offset,stat & ~(bm));		\
	} while(0)

#define SET_VPU_BIT(vpu,offset,bm)				\
	do {							\
		unsigned int stat;				\
		stat = vpu_readl(vpu,offset);			\
		vpu_writel(vpu,offset,stat | (bm));		\
	} while(0)

#define check_vpu_status(STAT, fmt, args...) do {		\
		if(vpu_stat & STAT)				\
			dev_err(vpu->vpu.dev, fmt, ##args);	\
	}while(0)

#endif //__CHANNEL_H__

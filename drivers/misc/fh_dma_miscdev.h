
#ifndef FH_DMA_MISCDEV_H_
#define FH_DMA_MISCDEV_H_

#include <linux/dmaengine.h>


#define DEVICE_NAME                 "fh_dma_misc"
#define PROC_FILE                   "driver/dma_misc"

#define DMA_IOCTL_MAGIC             'd'
#define RESERVERD                   _IO(DMA_IOCTL_MAGIC, 0)
#define REQUEST_CHANNEL             _IOWR(DMA_IOCTL_MAGIC, 1, __u32)
#define DMA_MEMCOPY                 _IOWR(DMA_IOCTL_MAGIC, 2, __u32)

#define DMA_IOCTL_MAXNR             14

#define DMA_MEMCPY_TIMEOUT          5000 //msec

struct dma_memcpy
{
    int chan_id;
    void *src_addr_vir;
    void *dst_addr_vir;
    unsigned int size;
    unsigned int src_addr_phy;
    unsigned int dst_addr_phy;
};



#endif /* FH_DMA_MISCDEV_H_ */

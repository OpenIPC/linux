#ifndef __DMAC_H__
#define __DMAC_H__

#define DMAC_ERROR_BASE     0x64

#define DMAC_CHN_SUCCESS            (DMAC_ERROR_BASE+0x10)
#define DMAC_CHN_ERROR              (DMAC_ERROR_BASE+0x11)
#define DMAC_CHN_TIMEOUT            (DMAC_ERROR_BASE+0x12)
#define DMAC_CHN_ALLOCAT            (DMAC_ERROR_BASE+0x13)
#define DMAC_CHN_VACANCY            (DMAC_ERROR_BASE+0x14)

#ifdef CONFIG_HIEDMAC
extern int dma_driver_init(void);
extern int dmac_channelclose(unsigned int channel);
extern int dmac_channelstart(unsigned int u32channel);
extern int dmac_channel_allocate(void);

extern int dmac_start_m2p(unsigned int channel, unsigned int pmemaddr,
                          unsigned int uwperipheralid,
                          unsigned int uwnumtransfers,
                          unsigned int next_lli_addr);
extern int dmac_m2p_transfer(unsigned long long memaddr, unsigned int uwperipheralid,
                             unsigned int length);
extern int dmac_channel_free(unsigned int channel);

extern int do_dma_m2p(unsigned long long memaddr, unsigned int peripheral_addr,
                      unsigned int length);
extern int do_dma_p2m(unsigned long mem_addr, unsigned int peripheral_addr,
                      unsigned int length);
extern int dmac_wait(int channel);

extern int dmac_start_m2m(unsigned int  channel, unsigned long psource,
                          unsigned long pdest, unsigned int uwnumtransfers);
extern int dmac_m2m_transfer(unsigned long source, unsigned long dest,
                             unsigned int length);
extern int dmac_register_isr(unsigned int channel, void *pisr);
extern int free_dmalli_space(unsigned int *ppheadlli, unsigned int page_num);
extern int dmac_start_llim2p(unsigned int channel, unsigned int *pfirst_lli,
                             unsigned int uwperipheralid);
extern int dmac_buildllim2m(unsigned long *ppheadlli,
                            unsigned long psource,
                            unsigned long pdest,
                            unsigned int totaltransfersize,
                            unsigned int uwnumtransfers);

extern int dmac_start_llim2m(unsigned int channel, unsigned long *pfirst_lli);

extern int allocate_dmalli_space(struct device *dev, unsigned long *ppheadlli,
                                 unsigned int page_num);
#endif /* CONFIG_HIEDMAC*/


/*structure for LLI*/
typedef struct dmac_lli {
    //must be 64Byte aligned
    unsigned long next_lli;
    unsigned int reserved[5];
    unsigned int count;
    unsigned long src_addr;
    unsigned long dest_addr;
    unsigned int config;
    unsigned int pad[51];
} dmac_lli;
#endif

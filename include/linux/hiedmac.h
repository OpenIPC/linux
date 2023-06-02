#ifndef __DMAC_H__
#define __DMAC_H__

#define DMAC_ERROR_BASE		0x64

#define DMAC_CHN_SUCCESS            (DMAC_ERROR_BASE+0x10)
#define DMAC_CHN_ERROR              (DMAC_ERROR_BASE+0x11)
#define DMAC_CHN_TIMEOUT            (DMAC_ERROR_BASE+0x12)
#define DMAC_CHN_ALLOCAT            (DMAC_ERROR_BASE+0x13)
#define DMAC_CHN_VACANCY            (DMAC_ERROR_BASE+0x14)

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

typedef void DMAC_ISR(unsigned int channel, int status);

int do_dma_llim2m(unsigned long source,
		unsigned long dest,
		unsigned long length);
int dmac_m2m_transfer(unsigned long source, unsigned long dest,
						unsigned int length);

#endif

#ifndef NNA_DRIVER_H
#define NNA_DRIVER_H
typedef enum nna_cmd {
    NNA_CMD_ENABLE_IRQ   = 0x100,
    NNA_CMD_DISABLE_IRQ  = 0x101,
    NNA_CMD_SET_FREQ     = 0x102,
    NNA_CMD_RESET_NNA    = 0x103,
	NNA_CMD_QUERY        = 0x104,
	NNA_CMD_MAP_DMA_FD   = 0x105,
	NNA_CMD_UNMAP_DMA_FD = 0x106,
} nna_cmd;

typedef enum clock_freq {
    NNA_CLOCK_300M   = 300,
    NNA_CLOCK_400M   = 400,
    NNA_CLOCK_600M   = 600,
    NNA_CLOCK_1200M  = 1200,
} clock_freq;

struct user_dma_param {
	int				fd;
	unsigned int	phy_addr;
};

#endif

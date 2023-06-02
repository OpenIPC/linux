#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <comm/ddr_arb.h>
#include <kwrap/examsys.h>
#if !defined(__FREERTOS)
#include <sys/ioctl.h>
#include "ddr_arb_ioctl.h"
#endif

#if !defined(__FREERTOS)
unsigned short arb_chksum(DDR_ARB id, unsigned int addr, unsigned int len)
{
	ARB_CHKSUM chksum = {
		.version = DDR_ARB_IOC_VERSION,
		.ddr_id = (unsigned int)id,
		.phy_addr = addr,
		.len = len,
		.sum = 0,
	};


	int fd = open("/dev/nvt_ddr_arb0", O_RDWR);
	if (fd < 0) {
		printf("Error to open /dev/nvt_ddr_arb0\n");
		return 0;
	}
	if (ioctl(fd, DDR_ARB_IOC_CHKSUM, (void *)&chksum) < 0) {
		printf("DDR_ARB_IOC_CHKSUM failed.\n");
		return 0;
	}
	close(fd);
	return chksum.sum;
}
#endif

EXAMFUNC_ENTRY(vendor_chksum, argc, argv)
{
	unsigned int phy_addr = 0x100000; // must be word alignment
	unsigned int len = 0x100000; // must be word alignment
	unsigned short sum = arb_chksum(0, phy_addr, len);
	printf("the sum value on phy_addr(0x%08X) with len(0x%08X) is 0x%08X.\n", phy_addr, len, sum);
	return 0;
}

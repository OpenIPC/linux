#ifndef __MODULE_IOCTL_CMD_H_
#define __MODULE_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define DDR_ARB_IOC_VERSION 0x19100409

typedef struct _ARB_CHKSUM {
	unsigned int version;  ///< [IN] must be DDR_ARB_IOC_VERSION for check header version
	unsigned int ddr_id;   ///< [IN] 0 or 1 for DDR[0], or DDR[1]
	unsigned int phy_addr; ///< [IN] must be word alignment
	unsigned int len;      ///< [IN] must be word alignment
	unsigned short sum;   ///< [OUT] the result of sum
} ARB_CHKSUM;

//============================================================================
// IOCTL command
//============================================================================
#define DDR_ARB_IOC_COMMON_TYPE 'M'
#define DDR_ARB_IOC_CHKSUM    _IOWR(DDR_ARB_IOC_COMMON_TYPE, 1, void*)

/* Add other command ID here*/


#endif

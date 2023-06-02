#ifndef __eth_lib_h_
#define __eth_lib_h_

#include "kflow_videoprocess/ctl_ipp_isp.h"
#include "kflow_videocapture/ctl_sie_isp.h"


#define ETH_BUFFER_NUM 8

//#define SIE_ETH
//#define ETH_DEBUG
//#define DUMP_ETH


/**
	ETH input buffer structure
*/
typedef struct _ETH_IN_BUFFER_INFO {
	UINT32 buf_addr[ETH_BUFFER_NUM];    ///< output buffer address
	UINT32 buf_size;	   ///< output buffer size [Set SIE]kflow will force disable eth when buffer size < eth out size, [Get] none
} ETH_IN_BUFFER_INFO;


int dis_eth_api_reg(void);
int dis_eth_api_unreg(void);

#endif

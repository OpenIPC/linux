/**
	@brief Header file of IO control definition of vendor net flow sample.

	@file net_flow_sample_ioctl.h

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KFLOW_AI_NET_IOCTL_H_
#define _KFLOW_AI_NET_IOCTL_H_

/********************************************************************
	TYPE DEFINITION
********************************************************************/

/**
	Memory buffer
*/
typedef struct {
	int addr;                           ///< memory buffer starting address
	int size;                           ///< memory buffer size
} vendor_ais_mem_t;

/**
	Memory information
*/
typedef struct {
	int ddr_id;
	int bufsize;
	int type;
	void *va;                           ///< need to free
	dma_addr_t pa;
} vendor_ais_ddr_t;

#endif  /* _KFLOW_AI_NET_IOCTL_H_ */

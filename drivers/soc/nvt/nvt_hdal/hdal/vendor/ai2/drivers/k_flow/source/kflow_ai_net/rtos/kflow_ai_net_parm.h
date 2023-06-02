/**
	@brief Header file of parameters definition of vendor net flow sample.

	@file net_flow_sample_parm.h

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NET_FLOW_SAMPLE_PARM_H_
#define _NET_FLOW_SAMPLE_PARM_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define PARAMETER_CNT           3
#define VENDOR_AIS_NC_MEM_SZ    0x01200000
#define VENDOR_AIS_MEM_SZ       0x00000000

static int vendor_ais_nc_mem_size = VENDOR_AIS_NC_MEM_SZ;
module_param(vendor_ais_nc_mem_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vendor_ais_nc_mem_size, "net flow nocache memory size");

static int vendor_ais_cache_size = VENDOR_AIS_MEM_SZ;
module_param(vendor_ais_cache_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vendor_ais_cache_size, "net flow cache memory size");

static uint vendor_ais_init_array[PARAMETER_CNT] = {[0 ... (PARAMETER_CNT - 1)] = -1};
module_param_array(vendor_ais_init_array, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vendor_ais_init_array, "net flow init parameter");

#endif  /* _NET_FLOW_SAMPLE_PARM_H_ */

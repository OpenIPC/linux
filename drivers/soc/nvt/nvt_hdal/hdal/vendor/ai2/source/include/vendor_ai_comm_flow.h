/**
	@brief Header file of definition of vendor user-space net flow sample.

	@file net_flow_user_sample.h

	@ingroup net_flow_user_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_COMM_FLOW_H_
#define _VENDOR_AI_COMM_FLOW_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

#include "kflow_ai_net/kflow_ai_net.h"
#define DEFAULT_DEVICE  	"/dev/" VENDOR_AIS_FLOW_DEV_NAME

#if defined(__LINUX_USER__)
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#endif
#if defined(__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

#if defined(__LINUX_USER__)
#define KFLOW_AI_OPEN  open
#define KFLOW_AI_IOCTL ioctl
#define KFLOW_AI_CLOSE close
#endif
#if defined (__FREERTOS)
#define KFLOW_AI_OPEN(...) 1
#define KFLOW_AI_IOCTL vendor_ais_flow_miscdev_ioctl
#define KFLOW_AI_CLOSE(...) 1
#endif


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

extern VOID vendor_ai_errno_location(VOID);

extern HD_RESULT vendor_ais_init_all(void);
extern HD_RESULT vendor_ais_uninit_all(void);
extern HD_RESULT vendor_ais_cfgschd(UINT32 schd);
extern HD_RESULT vendor_ais_cfgchk(UINT32 chk_interval);

extern HD_RESULT vendor_ais_init_net(void);
extern HD_RESULT vendor_ais_uninit_net(void);

extern HD_RESULT vendor_ais_lock_net(UINT32 net_id);
extern HD_RESULT vendor_ais_unlock_net(UINT32 net_id);




#endif  /* _VENDOR_AI_COMM_FLOW_H_ */

/**
	@brief Source file of kflow_ai_net.

	@file kflow_cpu_platform.h

	@ingroup kflow_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#if defined(__FREERTOS)
#define EXPORT_SYMBOL(a)
#define MODULE_AUTHOR(a)
#define MODULE_LICENSE(a)
#define MODULE_VERSION(a)
#include <string.h>         // for memset, strncmp
#include <stdio.h>          // sscanf
#else
//#include <linux/delay.h>
#include <linux/module.h>
#endif

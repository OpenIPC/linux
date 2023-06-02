/*!
********************************************************************************
*  \file    task.h
*  \brief
*  \project vos
*  \chip
*  \author Anderson
********************************************************************************
*/
#ifndef _VOS_KER_CONSTS_H_
#define _VOS_KER_CONSTS_H_

#ifdef __ECOS
// eCos system

#include <cyg/infra/cyg_type.h>
#include <cyg/compat/uitron/uit_type.h>
#else

#ifdef __cplusplus
extern "C" {
#endif



/*-----------------------------------------------------------------------------*/
/* Including Files                                                                                                                 */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                                                                         */
/*-----------------------------------------------------------------------------*/
/* Timeout related constants */

#define TMO_POL     ((TMO)0)            /* Polling option           */
#define TMO_FEVR    ((TMO)-1)           /* Endless wait option      */

/* Event flags related constants */

#define TWF_ANDW    0x00                /* AND condition            */
#define TWF_ORW     0x02                /* OR condition             */
#define TWF_CLR     0x01                /* Clear condition          */

#ifdef __cplusplus
}
#endif

#endif

#endif /* _VOS_KER_CONSTS_H_ */


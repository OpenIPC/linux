/**
	@brief Source file of kflow_ai_net.

	@file kflow_ai_core.h

	@ingroup kflow_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#ifndef _KFLOW_AI_NUE_H_
#define _KFLOW_AI_NUE_H_

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/error_no.h"
#include "kflow_ai_net/kflow_ai_core.h" //for LIST_HEAD

//current kflow
extern KFLOW_AI_ENGINE_CTX* kflow_nue_get_engine(void);
extern int kflow_nue_set_output_path(char *path);

#endif //_KFLOW_AI_NUE_H_

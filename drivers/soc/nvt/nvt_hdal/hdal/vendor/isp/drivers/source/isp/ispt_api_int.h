#ifndef _ISPT_API_INT_H_
#define _ISPT_API_INT_H_

#include "ispt_api.h"

//=============================================================================
// extern functions
//=============================================================================
extern UINT32 ispt_api_get_item_size(ISPT_ITEM item);
extern ER ispt_api_get_cmd(ISPT_ITEM item, UINT32 addr);
extern ER ispt_api_set_cmd(ISPT_ITEM item, UINT32 addr);

#endif


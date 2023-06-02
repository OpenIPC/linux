/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef TOOLS_H
#define TOOLS_H

#include "base_type.h"
#include "queue.h"
i32 log2i(i32 x, i32 *result);
i32 check_range(i32 x, i32 min, i32 max);
void **malloc_array(struct queue *q, i32 r, i32 c, i32 size);
void *qalloc(struct queue *q, i32 nmemb, i32 size);
void qfree(struct queue *q);

#endif

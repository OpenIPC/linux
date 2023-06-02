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

#ifndef TEST_DATA_DEFINE_H
#define TEST_DATA_DEFINE_H
/* INF3: remove include files
#include <stdio.h>
*/
#include "queue.h"
#include "base_type.h"

#ifndef TEST_DATA
#define COMMENT(b,...)
#else
#define COMMENT(b,...)  if ((b)->stream_trace) { \
        char buffer[128]; \
        snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
        ASSERT(strlen((b)->stream_trace->comment) + strlen(buffer) < \
          sizeof((b)->stream_trace->comment)); \
        strcat((b)->stream_trace->comment, buffer); \
      }
#endif
/* INF3: remove structure declaration
struct stream_trace
{
  struct node *next;
  char *buffer;
  char comment[256];
  size_t size;
  FILE *fp;
  u32 cnt;
};
*/
#endif

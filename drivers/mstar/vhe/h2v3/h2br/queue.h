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

#ifndef QUEUE_H
#define QUEUE_H

struct node
{
  struct node *next;
};

struct queue
{
  struct node *head;
  struct node *tail;
};

void queue_init(struct queue *);
void queue_put(struct queue *, struct node *);
void queue_put_tail(struct queue *, struct node *);
void queue_remove(struct queue *, struct node *);
void free_nodes(struct node *tail);
struct node *queue_get(struct queue *);

#endif

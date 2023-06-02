/**
	@brief Header file of vendor ai net group.

	@file vendor_ai_net_group.h

	@ingroup vendor_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_NET_GROUP_H_
#define _VENDOR_AI_NET_GROUP_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kflow_ai_net/kflow_ai_net.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/
struct list_head {
	struct list_head *next, *prev;
};

#define LIST_POISON1  ((struct list_head *) 0x00100100)
#define LIST_POISON2  ((struct list_head *) 0x00200200)

#define INIT_LIST_HEAD(ptr) do { (ptr)->next = (ptr); (ptr)->prev = (ptr); } while (0)

#define offset_of(TYPE, MEMBER)  (unsigned int)(&(((TYPE *)0)->MEMBER))
#define container_of(ptr,type,member)		(type *)((unsigned char *)ptr  - offset_of(type, member))
#define list_entry(ptr, type, member)		container_of(ptr, type, member)
#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)
#define list_prev_entry(pos, member)		list_entry((pos)->member.prev, typeof(*(pos)), member)
#define list_next_entry(pos, member) 		list_entry((pos)->member.next, typeof(*(pos)), member)
#define list_last_entry(ptr, type, member) 	list_entry((ptr)->prev, type, member)
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
		&pos->member != (head);					\
		pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_next_entry(pos, member);			\
		&pos->member != (head); 					\
		pos = n, n = list_next_entry(n, member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_last_entry(head, typeof(*pos), member);		\
		&pos->member != (head); 					\
		pos = list_prev_entry(pos, member))

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void __list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void list_move_tail(struct list_head *list, struct list_head *head)
{
	__list_del_entry(list);
	list_add_tail(list, head);
}

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#define VENDOR_AI_NET_SET_BMP(bmp, idx)		do { (bmp) |= (1 << (idx)); } while(0)
#define VENDOR_AI_NET_CLR_BMP(bmp, idx)		do { (bmp) &= (~(1 << (idx))); } while(0)
#define VENDOR_AI_NET_IS_BMP(bmp, idx)      ((bmp) & (1 << (idx)))
#define VENDOR_AI_NET_MAX(a, b)				((a)>(b)?(a):(b))

#define VENDOR_AI_NET_LLCMD_NULL		0x0
#define VENDOR_AI_NET_LLCMD_NEXT_LL		0x2
#define VENDOR_AI_NET_LLCMD_NEXT_UPD	0x4
#define VENDOR_AI_NET_LLCMD_UPD			0x8

typedef enum {
	VENDOR_AI_NET_MCTRL_HEAD = 0,
	VENDOR_AI_NET_MCTRL_MIDDLE = 1,
	VENDOR_AI_NET_MCTRL_TAIL = 2,
	ENUM_DUMMY4WORD(VENDOR_AI_NET_MCTRL_POS)
} VENDOR_AI_NET_MCTRL_POS;

typedef struct _VENDOR_AI_NET_MCTRL_ENTRY {
	struct list_head list;
	UINT32 mc_idx;
	NN_GEN_MODE_CTRL *p_data;
	VENDOR_AI_NET_MCTRL_POS pos_bmp;
	VOID *p_group;
} VENDOR_AI_NET_MCTRL_ENTRY;

typedef struct _VENDOR_AI_NET_MCTRL_LIST {
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry;
	UINT32 mctrl_num;
} VENDOR_AI_NET_MCTRL_LIST;

typedef struct _VENDOR_AI_NET_LLGROUP {
	UINT32 addr;
	UINT32 cnt;
	UINT32 step;
	UINT32 step_end;
	UINT32 prev_num;
	UINT32 next_num;
	UINT32 idea_cycle;
	UINT32 g_idx;
	NN_GEN_ENG_TYPE eng;
	NN_GEN_TRIG_SRC trig_src;
	struct list_head list;
	struct list_head mctrl_listhead;
	NN_GEN_MODE_CTRL *p_head;
	NN_GEN_MODE_CTRL *p_tail;
} VENDOR_AI_NET_LLGROUP;

typedef struct _VENDOR_AI_NET_LLGROUP_LIST {
	VENDOR_AI_NET_LLGROUP *p_group;
	UINT32 group_num;
} VENDOR_AI_NET_LLGROUP_LIST;

typedef struct _VENDOR_AI_NET_MEM_ENTRY {
	VENDOR_AI_NET_LLGROUP *p_group;
	UINT32 is_alloc;
} VENDOR_AI_NET_MEM_ENTRY;

typedef struct _VENDOR_AI_NET_MEM_LIST {
	VENDOR_AI_NET_MEM_ENTRY *p_mem;
	UINT32 mem_num;
} VENDOR_AI_NET_MEM_LIST;

typedef struct _VENDOR_AI_NET_GROUP_MEM {
	UINT32 user_parm_addr;
	UINT32 mctrl_entry_addr;
	UINT32 mctrl_entry_size;
	UINT32 llgroup_addr;
	UINT32 llgroup_size;
	UINT32 mem_entry_addr;
	UINT32 mem_entry_size;
	VENDOR_AI_NET_MCTRL_LIST mctrl_list;
	VENDOR_AI_NET_LLGROUP_LIST group_list;
	VENDOR_AI_NET_MEM_LIST mem_list;
	VENDOR_AI_NET_JOB_OPT method;
} VENDOR_AI_NET_GROUP_MEM;


UINT32 vendor_ai_net_group_calcbuffersize(UINT32 proc_id, UINT32 user_parm_addr);
HD_RESULT vendor_ai_net_group_setbuffer(UINT32 proc_id, UINT32 working_buf_addr, UINT32 user_parm_addr);
HD_RESULT vendor_ai_net_group_proc(UINT32 proc_id, VENDOR_AI_NET_JOB_OPT job_opt_method);
HD_RESULT vendor_ai_net_group_llcmd_fix(UINT32 proc_id);
VENDOR_AI_NET_MCTRL_LIST *vendor_ai_net_group_getmctrlresult(UINT32 proc_id);
VENDOR_AI_NET_LLGROUP_LIST *vendor_ai_net_group_getgroupresult(UINT32 proc_id);
VENDOR_AI_NET_MEM_LIST *vendor_ai_net_group_getmemresult(UINT32 proc_id);
HD_RESULT vendor_ai_net_group_init(VOID);
HD_RESULT vendor_ai_net_group_uninit(VOID);

#endif  /* _VENDOR_AI_NET_GROUP_H_ */

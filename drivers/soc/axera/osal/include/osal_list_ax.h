/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_LIST_AX__H__
#define __OSAL_LIST_AX__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_type_ax.h"


//list api
typedef struct AX_LIST_HEAD {
    struct AX_LIST_HEAD *next, *prev;
} AX_LIST_HEAD_T;
#define AX_OSAL_LIST_HEAD_INIT(name) { &(name), &(name) }

#define AX_OSAL_LIST_HEAD(name) \
    struct AX_LIST_HEAD name = AX_OSAL_LIST_HEAD_INIT(name)

static inline void AX_OSAL_LIB_init_list_head(struct AX_LIST_HEAD *list)
{
    list->next = list;
    list->prev = list;
}


/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void AX_OSAL___list_add(struct AX_LIST_HEAD *_new, struct AX_LIST_HEAD *_prev, struct AX_LIST_HEAD *_next)
{
    _next->prev = _new;
    _new->next = _next;
    _new->prev = _prev;
    _prev->next = _new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void AX_OSAL_LIB_list_add(struct AX_LIST_HEAD *_new, struct AX_LIST_HEAD *_head)
{
    AX_OSAL___list_add(_new, _head, _head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void AX_OSAL_LIB_list_add_tail(struct AX_LIST_HEAD *_new, struct AX_LIST_HEAD *_head)
{
    AX_OSAL___list_add(_new, _head->prev, _head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void AX_OSAL___list_del(struct AX_LIST_HEAD *_prev, struct AX_LIST_HEAD *_next)
{
    _next->prev = _prev;
    _prev->next = _next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void AX_OSAL___list_del_entry(struct AX_LIST_HEAD *_entry)
{
    AX_OSAL___list_del(_entry->prev, _entry->next);
}

#define AX_OSAL_LIST_POISON1  ((void *) 0x00100100)
#define AX_OSAL_LIST_POISON2  ((void *) 0x00200200)


static inline void AX_OSAL_LIB_list_del(struct AX_LIST_HEAD *_entry)
{
    AX_OSAL___list_del(_entry->prev, _entry->next);
    _entry->next = (struct AX_LIST_HEAD *)AX_OSAL_LIST_POISON1;
    _entry->prev = (struct AX_LIST_HEAD *)AX_OSAL_LIST_POISON2;
}


/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void AX_OSAL_LIB_list_replace(struct AX_LIST_HEAD *_old, struct AX_LIST_HEAD *_new)
{
    _new->next = _old->next;
    _new->next->prev = _new;
    _new->prev = _old->prev;
    _new->prev->next = _new;
}

static inline void AX_OSAL_LIB_list_replace_init(struct AX_LIST_HEAD *_old, struct AX_LIST_HEAD *_new)
{
    AX_OSAL_LIB_list_replace(_old, _new);
    AX_OSAL_LIB_init_list_head(_old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void AX_OSAL_LIB_list_del_init(struct AX_LIST_HEAD *_entry)
{
    AX_OSAL___list_del_entry(_entry);
    AX_OSAL_LIB_init_list_head(_entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void AX_OSAL_LIB_list_move(struct AX_LIST_HEAD *_list, struct AX_LIST_HEAD *_head)
{
    AX_OSAL___list_del_entry(_list);
    AX_OSAL_LIB_list_add(_list, _head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void AX_OSAL_LIB_list_move_tail(struct AX_LIST_HEAD *_list, struct AX_LIST_HEAD *_head)
{
    AX_OSAL___list_del_entry(_list);
    AX_OSAL_LIB_list_add_tail(_list, _head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int AX_OSAL_LIB_list_is_last(struct AX_LIST_HEAD *_list, struct AX_LIST_HEAD *_head)
{
    return _list->next == _head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int AX_OSAL_LIB_list_empty(struct AX_LIST_HEAD *_head)
{
    return _head->next == _head;
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void AX_OSAL_LIB_list_rotate_left(struct AX_LIST_HEAD *_head)
{
    struct AX_LIST_HEAD *first;

    if (!AX_OSAL_LIB_list_empty(_head)) {
        first = _head->next;
        AX_OSAL_LIB_list_move_tail(first, _head);
    }
}

static inline void AX_OSAL___list_splice(const struct AX_LIST_HEAD *_list,
        struct AX_LIST_HEAD *_prev,
        struct AX_LIST_HEAD *_next)
{
    struct AX_LIST_HEAD *first = _list->next;
    struct AX_LIST_HEAD *last = _list->prev;

    first->prev = _prev;
    _prev->next = first;

    last->next = _next;
    _next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void AX_OSAL_LIB_list_splice(struct AX_LIST_HEAD *_list, struct AX_LIST_HEAD *_head)
{
    if (!AX_OSAL_LIB_list_empty(_list))
        AX_OSAL___list_splice(_list, _head, _head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void AX_OSAL_LIB_list_splice_tail(struct AX_LIST_HEAD *_list, struct AX_LIST_HEAD *_head)
{
    if (!AX_OSAL_LIB_list_empty(_list))
        AX_OSAL___list_splice(_list, _head->prev, _head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void AX_OSAL_LIB_list_splice_init(struct AX_LIST_HEAD *_list, struct AX_LIST_HEAD *_head)
{
    if (!AX_OSAL_LIB_list_empty(_list)) {
        AX_OSAL___list_splice(_list, _head, _head->next);
        AX_OSAL_LIB_init_list_head(_list);
    }
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void AX_OSAL_LIB_list_splice_tail_init(struct AX_LIST_HEAD *_list, struct AX_LIST_HEAD *_head)
{
    if (!AX_OSAL_LIB_list_empty(_list)) {
        AX_OSAL___list_splice(_list, _head->prev, _head);
        AX_OSAL_LIB_init_list_head(_list);
    }
}

#define AX_OSAL_offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)

#define AX_OSAL_container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - AX_OSAL_offsetof(type,member) );})


/**
 * list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:    the type of the struct this is embedded in.
 * @member:    the name of the list_struct within the struct.
 */
#define AX_OSAL_LIB_list_entry(ptr, type, member) \
    AX_OSAL_container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:    the list head to take the element from.
 * @type:    the type of the struct this is embedded in.
 * @member:    the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define AX_OSAL_LIB_list_first_entry(ptr, type, member) \
    AX_OSAL_LIB_list_entry((ptr)->next, type, member)

/**
 * list_for_each    -    iterate over a list
 * @pos:    the &struct list_head to use as a loop cursor.
 * @head:    the head for your list.
 */
#define AX_OSAL_LIB_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)


/**
 * list_for_each_prev    -    iterate over a list backwards
 * @pos:    the &struct list_head to use as a loop cursor.
 * @head:    the head for your list.
 */
#define AX_OSAL_LIB_list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop cursor.
 * @n:        another &struct list_head to use as temporary storage
 * @head:    the head for your list.
 */
#define AX_OSAL_LIB_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop cursor.
 * @n:        another &struct list_head to use as temporary storage
 * @head:    the head for your list.
 */
#define AX_OSAL_LIB_list_for_each_prev_safe(pos, n, head) \
    for (pos = (head)->prev, n = pos->prev; \
         pos != (head); \
         pos = n, n = pos->prev)

/**
 * list_for_each_entry    -    iterate over list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 */
#define AX_OSAL_LIB_list_for_each_entry(pos, head, member) \
    for (pos = AX_OSAL_LIB_list_entry((head)->next, typeof(*pos), member);    \
         &pos->member != (head);     \
         pos = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:    the type * to use as a loop cursor.
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 */
#define AX_OSAL_LIB_list_for_each_entry_reverse(pos, head, member) \
    for (pos = AX_OSAL_LIB_list_entry((head)->prev, typeof(*pos), member);    \
         &pos->member != (head);     \
         pos = AX_OSAL_LIB_list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_for_each_entry_continue()
 * @pos:    the type * to use as a start point
 * @head:    the head of the list
 * @member:    the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in list_for_each_entry_continue().
 */
#define AX_OSAL_LIB_list_prepare_entry(pos, head, member) \
    ((pos) ? : AX_OSAL_LIB_list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define AX_OSAL_LIB_list_for_each_entry_continue(pos, head, member) \
    for (pos = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member);    \
         &pos->member != (head);    \
         pos = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:    the type * to use as a loop cursor.
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define AX_OSAL_LIB_list_for_each_entry_continue_reverse(pos, head, member) \
    for (pos = AX_OSAL_LIB_list_entry(pos->member.prev, typeof(*pos), member);    \
         &pos->member != (head);    \
         pos = AX_OSAL_LIB_list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:    the type * to use as a loop cursor.
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define AX_OSAL_LIB_list_for_each_entry_from(pos, head, member) \
    for (; &pos->member != (head);    \
         pos = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop cursor.
 * @n:        another type * to use as temporary storage
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 */
#define AX_OSAL_LIB_list_for_each_entry_safe(pos, n, head, member) \
    for (pos = AX_OSAL_LIB_list_entry((head)->next, typeof(*pos), member),    \
        n = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member);    \
         &pos->member != (head);                     \
         pos = n, n = AX_OSAL_LIB_list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:    the type * to use as a loop cursor.
 * @n:        another type * to use as temporary storage
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define AX_OSAL_LIB_list_for_each_entry_safe_continue(pos, n, head, member) \
    for (pos = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member),         \
        n = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member);        \
         &pos->member != (head);                        \
         pos = n, n = AX_OSAL_LIB_list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:    the type * to use as a loop cursor.
 * @n:        another type * to use as temporary storage
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define AX_OSAL_LIB_list_for_each_entry_safe_from(pos, n, head, member) \
    for (n = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member);        \
         &pos->member != (head);                        \
         pos = n, n = AX_OSAL_LIB_list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:    the type * to use as a loop cursor.
 * @n:        another type * to use as temporary storage
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define AX_OSAL_LIB_list_for_each_entry_safe_reverse(pos, n, head, member) \
    for (pos = AX_OSAL_LIB_list_entry((head)->prev, typeof(*pos), member),    \
        n = AX_OSAL_LIB_list_entry(pos->member.prev, typeof(*pos), member);    \
         &pos->member != (head);                     \
         pos = n, n = AX_OSAL_LIB_list_entry(n->member.prev, typeof(*n), member))

/**
 * list_safe_reset_next - reset a stale list_for_each_entry_safe loop
 * @pos:    the loop cursor used in the list_for_each_entry_safe loop
 * @n:        temporary storage used in list_for_each_entry_safe
 * @member:    the name of the list_struct within the struct.
 *
 * list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define AX_OSAL_LIB_list_safe_reset_next(pos, n, member) \
    n = AX_OSAL_LIB_list_entry(pos->member.next, typeof(*pos), member)


#ifdef __cplusplus
}
#endif

#endif /*__OSAL_LIST_AX__H__*/

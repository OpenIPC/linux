#ifndef _VOS_LIST_H_
#define _VOS_LIST_H_

#define VOS_LIST_POISON1    ((void *) 0x100)
#define VOS_LIST_POISON2    ((void *) 0x200)

#define vos_offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)

/**
 *  vos_container_of - cast a member of a structure out to the containing structure
 *  @ptr:	the pointer to the member.
 *  @type:	the type of the container struct this is embedded in.
 *  @member:	the name of the member within the struct.
 */
#define vos_container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - vos_offsetof(type,member) );})

struct vos_list_head {
	struct vos_list_head *next, *prev;
};

#define VOS_LIST_HEAD_INIT(name) { &(name), &(name) }

#define VOS_LIST_HEAD(name) \
	struct vos_list_head name = VOS_LIST_HEAD_INIT(name)

static inline void VOS_INIT_LIST_HEAD(struct vos_list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline int __vos_list_add_valid(struct vos_list_head *new,
				struct vos_list_head *prev,
				struct vos_list_head *next)
{
	return 1;
}
static inline int __vos_list_del_entry_valid(struct vos_list_head *entry)
{
	return 1;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __vos_list_add(struct vos_list_head *new,
			      struct vos_list_head *prev,
			      struct vos_list_head *next)
{
	if (!__vos_list_add_valid(new, prev, next))
		return;

	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void vos_list_add(struct vos_list_head *new, struct vos_list_head *head)
{
	__vos_list_add(new, head, head->next);
}


/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void vos_list_add_tail(struct vos_list_head *new, struct vos_list_head *head)
{
	__vos_list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __vos_list_del(struct vos_list_head * prev, struct vos_list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __vos_list_del_entry(struct vos_list_head *entry)
{
	if (!__vos_list_del_entry_valid(entry))
		return;

	__vos_list_del(entry->prev, entry->next);
}

static inline void vos_list_del(struct vos_list_head *entry)
{
	__vos_list_del_entry(entry);
	entry->next = VOS_LIST_POISON1;
	entry->prev = VOS_LIST_POISON2;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void vos_list_replace(struct vos_list_head *old,
				struct vos_list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void vos_list_replace_init(struct vos_list_head *old,
					struct vos_list_head *new)
{
	vos_list_replace(old, new);
	VOS_INIT_LIST_HEAD(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void vos_list_del_init(struct vos_list_head *entry)
{
	__vos_list_del_entry(entry);
	VOS_INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void vos_list_move(struct vos_list_head *list, struct vos_list_head *head)
{
	__vos_list_del_entry(list);
	vos_list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void vos_list_move_tail(struct vos_list_head *list,
				  struct vos_list_head *head)
{
	__vos_list_del_entry(list);
	vos_list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int vos_list_is_last(const struct vos_list_head *list,
				const struct vos_list_head *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int vos_list_empty(const struct vos_list_head *head)
{
	return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static inline int vos_list_empty_careful(const struct vos_list_head *head)
{
	struct vos_list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void vos_list_rotate_left(struct vos_list_head *head)
{
	struct vos_list_head *first;

	if (!vos_list_empty(head)) {
		first = head->next;
		vos_list_move_tail(first, head);
	}
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int vos_list_is_singular(const struct vos_list_head *head)
{
	return !vos_list_empty(head) && (head->next == head->prev);
}

static inline void __vos_list_cut_position(struct vos_list_head *list,
		struct vos_list_head *head, struct vos_list_head *entry)
{
	struct vos_list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void vos_list_cut_position(struct vos_list_head *list,
		struct vos_list_head *head, struct vos_list_head *entry)
{
	if (vos_list_empty(head))
		return;
	if (vos_list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		VOS_INIT_LIST_HEAD(list);
	else
		__vos_list_cut_position(list, head, entry);
}

/**
 * list_cut_before - cut a list into two, before given entry
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *
 * This helper moves the initial part of @head, up to but
 * excluding @entry, from @head to @list.  You should pass
 * in @entry an element you know is on @head.  @list should
 * be an empty list or a list you do not care about losing
 * its data.
 * If @entry == @head, all entries on @head are moved to
 * @list.
 */
static inline void vos_list_cut_before(struct vos_list_head *list,
				   struct vos_list_head *head,
				   struct vos_list_head *entry)
{
	if (head->next == entry) {
		VOS_INIT_LIST_HEAD(list);
		return;
	}
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry->prev;
	list->prev->next = list;
	head->next = entry;
	entry->prev = head;
}

static inline void __vos_list_splice(const struct vos_list_head *list,
				 struct vos_list_head *prev,
				 struct vos_list_head *next)
{
	struct vos_list_head *first = list->next;
	struct vos_list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void vos_list_splice(const struct vos_list_head *list,
				struct vos_list_head *head)
{
	if (!vos_list_empty(list))
		__vos_list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void vos_list_splice_tail(struct vos_list_head *list,
				struct vos_list_head *head)
{
	if (!vos_list_empty(list))
		__vos_list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void vos_list_splice_init(struct vos_list_head *list,
				    struct vos_list_head *head)
{
	if (!vos_list_empty(list)) {
		__vos_list_splice(list, head, head->next);
		VOS_INIT_LIST_HEAD(list);
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
static inline void vos_list_splice_tail_init(struct vos_list_head *list,
					 struct vos_list_head *head)
{
	if (!vos_list_empty(list)) {
		__vos_list_splice(list, head->prev, head);
		VOS_INIT_LIST_HEAD(list);
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct vos_list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the vos_list_head within the struct.
 */
#define vos_list_entry(ptr, type, member) \
	vos_container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define vos_list_first_entry(ptr, type, member) \
	vos_list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define vos_list_last_entry(ptr, type, member) \
	vos_list_entry((ptr)->prev, type, member)

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define vos_list_first_entry_or_null(ptr, type, member) ({ \
	struct vos_list_head *head__ = (ptr); \
	struct vos_list_head *pos__ = head__->next; \
	pos__ != head__ ? vos_list_entry(pos__, type, member) : NULL; \
})

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the vos_list_head within the struct.
 */
#define vos_list_next_entry(pos, member) \
	vos_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the vos_list_head within the struct.
 */
#define vos_list_prev_entry(pos, member) \
	vos_list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct vos_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define vos_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct vos_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define vos_list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct vos_list_head to use as a loop cursor.
 * @n:		another &struct vos_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define vos_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct vos_list_head to use as a loop cursor.
 * @n:		another &struct vos_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define vos_list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 */
#define vos_list_for_each_entry(pos, head, member)				\
	for (pos = vos_list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = vos_list_next_entry(pos, member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 */
#define vos_list_for_each_entry_reverse(pos, head, member)			\
	for (pos = vos_list_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head); 					\
	     pos = vos_list_prev_entry(pos, member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the vos_list_head within the struct.
 *
 * Prepares a pos entry for use as a start point in list_for_each_entry_continue().
 */
#define vos_list_prepare_entry(pos, head, member) \
	((pos) ? : vos_list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define vos_list_for_each_entry_continue(pos, head, member) 		\
	for (pos = vos_list_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = vos_list_next_entry(pos, member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define vos_list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = vos_list_prev_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = vos_list_prev_entry(pos, member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define vos_list_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);					\
	     pos = vos_list_next_entry(pos, member))

/**
 * list_for_each_entry_from_reverse - iterate backwards over list of given type
 *                                    from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Iterate backwards over list of given type, continuing from current position.
 */
#define vos_list_for_each_entry_from_reverse(pos, head, member)		\
	for (; &pos->member != (head);					\
	     pos = vos_list_prev_entry(pos, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 */
#define vos_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = vos_list_first_entry(head, typeof(*pos), member),	\
		n = vos_list_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = vos_list_next_entry(n, member))

/**
 * list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define vos_list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = vos_list_next_entry(pos, member), 				\
		n = vos_list_next_entry(pos, member);				\
	     &pos->member != (head);						\
	     pos = n, n = vos_list_next_entry(n, member))

/**
 * list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define vos_list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = vos_list_next_entry(pos, member);					\
	     &pos->member != (head);						\
	     pos = n, n = vos_list_next_entry(n, member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the vos_list_head within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define vos_list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = vos_list_last_entry(head, typeof(*pos), member),		\
		n = vos_list_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = vos_list_prev_entry(n, member))

/**
 * list_safe_reset_next - reset a stale list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the list_for_each_entry_safe loop
 * @n:		temporary storage used in list_for_each_entry_safe
 * @member:	the name of the vos_list_head within the struct.
 *
 * list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define vos_list_safe_reset_next(pos, n, member)				\
	n = vos_list_next_entry(pos, member)

#endif //_VOS_LIST_H_

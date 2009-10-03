/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   Linux kernel header adapted for user-mode
 *   The 2.6.17-rt1 version was used.
 *
 *   Original copyright holders of this code are unknown, they were not
 *   mentioned in the original file.
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

#ifndef __LINUX_LIST_H__
#define __LINUX_LIST_H__

#include <stddef.h>

#if !defined(offsetof)
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/** Cast a member of a structure out to the containing structure.
 * @param ptr The pointer to the member.
 * @param type The type of the container struct this is embedded in.
 * @param member The name of the member within the struct.
 */
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

/** Prefetch a pointer to cache.
 * @param x Pointer to prefetch.
 */
#define prefetch(x) (x = x)

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

/** The list structure. */
struct list_head {
  struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
  struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
  list->next = list;
  list->prev = list;
}

/** Insert a new entry between two known consecutive entries.
 * <br />This is only for internal list manipulation where we know
 * the prev/next entries already!
 * @param new New entry to be added.
 * @param prev List entry before the one to add.
 * @param next List entry after the one to add.
 */
static inline void __list_add(struct list_head *new,
            struct list_head *prev,
            struct list_head *next)
{
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

/** Add a new entry.
 * <br />Insert a new entry after the specified head.
 * This is good for implementing stacks.
 * @param new New entry to be added.
 * @param head List head to add it after.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
  __list_add(new, head, head->next);
}

/** Add a new entry.
 * <br />Insert a new entry before the specified head.
 * This is useful for implementing queues.
 * @param new New entry to be added.
 * @param list Head to add it before.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
  __list_add(new, head->prev, head);
}

/** Delete a list entry by making the prev/next entries
 * point to each other.
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 * @param prev List entry before the one to delete.
 * @param next List entry after the one to delete.
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
  next->prev = prev;
  prev->next = next;
}

/** Deletes entry from list.
 * <br />Note: @ref list_empty() on entry does not return true after this, the
 * entry is in an undefined state.
 * @param entry The element to delete from the list.
 */
static inline void list_del(struct list_head *entry)
{
  __list_del(entry->prev, entry->next);
  entry->next = LIST_POISON1;
  entry->prev = LIST_POISON2;
}

/** Deletes entry from list and reinitialize it.
 * @param entry The element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
  __list_del(entry->prev, entry->next);
  INIT_LIST_HEAD(entry);
}

/** Tests whether a list is empty.
 * @param head The list to test.
 */
static inline int list_empty(const struct list_head *head)
{
  return head->next == head;
}

static inline void __list_splice(struct list_head *list,
         struct list_head *head)
{
  struct list_head *first = list->next;
  struct list_head *last = list->prev;
  struct list_head *at = head->next;

  first->prev = head;
  head->next = first;

  last->next = at;
  at->prev = last;
}

/** Join two lists.
 * @param list The new list to add.
 * @param head The place to add it in the first list.
 */
static inline void list_splice(struct list_head *list, struct list_head *head)
{
  if (!list_empty(list))
    __list_splice(list, head);
}

/** Join two lists and reinitialise the emptied list.
 * The list at @a list is reinitialised
 * @param list The new list to add.
 * @param head The place to add it in the first list.
 */
static inline void list_splice_init(struct list_head *list,
            struct list_head *head)
{
  if (!list_empty(list)) {
    __list_splice(list, head);
    INIT_LIST_HEAD(list);
  }
}

/** Get the struct for this entry.
 * @param ptr The struct @ref list_head pointer.
 * @param type The type of the struct this is embedded in.
 * @param member The name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
  container_of(ptr, type, member)

/** Iterate over a list.
 * @param pos The struct @ref list_head to use as a loop counter.
 * @param head The head for your list.
 */
#define list_for_each(pos, head) \
  for (pos = (head)->next; prefetch(pos->next), pos != (head); \
          pos = pos->next)

/** Iterate over a list.
 * <br />This variant differs from @ref list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 * @param pos The struct @ref list_head to use as a loop counter.
 * @param head The head for your list.
 */
#define __list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

/** Iterate over a list backwards.
 * @param pos The struct @ref list_head to use as a loop counter.
 * @param head The head for your list.
 */
#define list_for_each_prev(pos, head) \
  for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
          pos = pos->prev)

/** Iterate over a list safe against removal of list entry.
 * @param pos The &struct list_head to use as a loop counter.
 * @param n Another &struct list_head to use as temporary storage
 * @param head The head for your list.
 */
#define list_for_each_safe(pos, n, head) \
  for (pos = (head)->next, n = pos->next; pos != (head); \
    pos = n, n = pos->next)

/** Iterate over list of given type.
 * @param pos  The type * to use as a loop counter.
 * @param head The head for your list.
 * @param member The name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)        \
  for (pos = list_entry((head)->next, typeof(*pos), member);  \
       prefetch(pos->member.next), &pos->member != (head);  \
       pos = list_entry(pos->member.next, typeof(*pos), member))

/** Iterate over list of given type safe against removal of list entry.
 * @param pos  The type * to use as a loop counter.
 * @param n Another type * to use as temporary storage
 * @param head The head for your list.
 * @param member The name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)        \
  for (pos = list_entry((head)->next, typeof(*pos), member),  \
       n = list_entry(pos->member.next, typeof(*pos), member);  \
       prefetch(pos->member.next), &pos->member != (head);  \
       pos = n, n = list_entry(pos->member.next, typeof(*pos), member))

/** Iterate backwards over list of given type.
 * @param pos  The type * to use as a loop counter.
 * @param head The head for your list.
 * @param member The name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)      \
  for (pos = list_entry((head)->prev, typeof(*pos), member);  \
       prefetch(pos->member.prev), &pos->member != (head);  \
       pos = list_entry(pos->member.prev, typeof(*pos), member))

#endif /* __LINUX_LIST_H__ */

#ifndef __LIST_HEAD_H__
#define __LIST_HEAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/**
 * Simple  circular doubly linked list implementation
 */
typedef struct __list TList;
struct __list{
	TList *prev, *next;
	void *data;
};

static void
init_list(TList *list) {
	list->next = list;
	list->prev = list;
}

static TList *
tlist_new(void *data) {
	TList *list;

	list = (TList *)malloc(sizeof(TList));
	init_list(list);
	list->data = data;

	return list;
}

/**
 * add a new entry after the given list.
 * @param list list to add it after.
 * @param new new entry to be added.
 * @return the added entry.
 */
static TList *
list_append(TList *head, void *data) {
	TList *new = tlist_new(data);
	
	new->next = head->next;
	head->next->prev = new;
	new->prev = head;
	head->next = new;

	return new;
}

/**
 * add a new entry after the given list.
 * @param list list to add it after.
 * @param new new entry to be added.
 * @return the added entry.
 */
static TList *
list_prepend(TList *head, void *data) {
	TList *new = tlist_new(data);
	
	new->next = head;
	head->prev = new;
	new->prev = head->prev;
	head->prev->next = new;

	return new;
}

/**
 * list_replace - replace old entry by new one
 * @param old : the element to be replaced
 * @param new : the new element to insert
 * @return the updated entry.
 * If @old was empty, it will be overwritten.
 */
static TList *
list_replace(TList *old, void *data)
{
	TList *new = tlist_new(data);
	
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;

	return new;
}

/**
 * Delete a list entry by making the entry's prev/next entries point to each other.
 * @param entry the element to delete from the list.
 */
static void
list_del(TList *entry) {
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
}

/**
 * Tests whether a list is empty.
 * @list the list to test.
 */
static int
list_empty(TList *list) {
	return list->next == list;
}

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &TList to use as a loop cursor.
 * @list:	the head for your list.
 */
#define list_for_each(pos, list) \
	for (pos = (list)->next;  pos != (list); \
        	pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &TList to use as a loop cursor.
 * @list:	the head for your list.
 */
#define list_for_each_prev(pos, list) \
	for (pos = (list)->prev;  pos != (list); \
        	pos = pos->prev)

#endif

#include "list.h"


void init_list_head(struct list_head *list)
{
	list->next = list;
	list->prev = list;
	list->owner = list;
}

static void _list_add(struct list_head *new,struct list_head *prev,struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

void list_add(struct list_head *new, struct list_head *head)
{
    _list_add(new, head, head->next);
}

void list_add_tail(struct list_head *new, struct list_head *head)
{
	_list_add(new, head->prev, head);
}

static void _list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

void list_del(struct list_head *entry)
{
	_list_del(entry->prev, entry->next);
	entry->next = (void *)0;
	entry->prev = (void *)0;
}

int list_empty(const struct list_head *head)
{
	return head->next == head;
}

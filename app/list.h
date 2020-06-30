#ifndef LIST_H
#define LIST_H

struct list_head
{
	struct list_head *next;
	struct list_head *prev;
	void *owner;
};

void init_list_head(struct list_head *list);

void list_add(struct list_head *new, struct list_head *head);

void list_add_tail(struct list_head *new, struct list_head *head);

void list_del(struct list_head *entry);

int list_empty(const struct list_head *head);

#endif

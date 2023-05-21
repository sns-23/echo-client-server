#include "list.h"

static void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

void list_del(struct list_head *entry)
{
    struct list_head *next = entry->next;
    struct list_head *prev = entry->prev;

    next->prev = prev;
    prev->next = next;
}

size_t list_count_nodes(struct list_head *head)
{
	struct list_head *pos;
	size_t count = 0;

	list_for_each(pos, head)
		count++;

	return count;
}
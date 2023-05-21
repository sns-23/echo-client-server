
#include <stddef.h>

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_entry_is_head(pos, head, member)               \
    (&pos->member == (head))

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each_entry(pos, head, member)              \
    for (pos = list_first_entry(head, typeof(*pos), member);    \
         !list_entry_is_head(pos, head, member);            \
         pos = list_next_entry(pos, member))

#define list_for_each(pos, head) \
    for (pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next)

struct list_head {
    struct list_head *prev, *next;
};

static inline int list_is_head(const struct list_head *list, const struct list_head *head)
{
    return list == head;
}

void list_add(struct list_head *new, struct list_head *head);
void list_add_tail(struct list_head *new, struct list_head *head);
void list_del(struct list_head *entry);
size_t list_count_nodes(struct list_head *head);
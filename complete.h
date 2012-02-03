#ifndef COMPLETE_H
#define COMPLETE_H

struct node {
	char *name;
	struct node *next;
};

struct list {
	struct node *head;
	struct node *tail;
	struct node *cur;
};

void free_list(struct list *list);
struct list *extern_complete(const char *cmd, const char *input);
static inline struct node *next(struct list *list) {
	if (list == NULL)
		return NULL;
	if (list->cur)
		list->cur = list->cur->next;
	if (list->cur == NULL)
		list->cur = list->head;
	return list->cur;
}

#endif


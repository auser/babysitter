#ifndef _LIST_H_
#define _LIST_H_ 

struct list_node {
  struct list_node * next;
  struct list_node * prev;
  size_t count;
} list_node;

#define LIST_INIT(el) struct list_node el = {&el, &el, NULL }
#define LIST_INIT_DATA(el, dataa) struct list_node el = {&el, &el, dataa}
#define LIST_DATA(el, type) ((type *) el->data)

#define list_for(head, it) for (it = (head)->next; it != (head); it = it->next)

static inline void list_init(struct list_node * head, void * data) {
  head->next = head;
  head->prev = head;
  head->data = data;
}

static inline void list_add_tail(struct list_node * head, struct list_node * el) {
  head->prev->next = el;
  el->prev = head->prev;
  el->next = head;
  head->prev = el;
}

static inline void list_add(struct list_node * head, struct list_node * el) {
  head->next->prev = el;
  el->next = head->next;
  el->prev = head;
  head->next = el;
}

static inline int list_is_empty(struct list_node * s) {
  return (s->next == s);
}

static inline void list_del(struct list_node * el) {
  el->prev->next = el->next;
  el->next->prev = el->prev;
}

static inline void list_nodewap(struct list_node * head1, struct list_node * head2) {
  *head2 = *head1;
  head1->next->prev = head2;
  head1->prev->next = head2;
}

#endif
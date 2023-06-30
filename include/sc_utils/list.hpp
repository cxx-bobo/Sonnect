#ifndef _LIST_HPP_
#define _LIST_HPP_

#define LIST_HEAD_INIT(name) { &(name), &(name) }

/*!
 * \brief   the link list entry
 */
struct list_entry {
    struct list_entry *next;
    struct list_entry *prev;
};

/*!
 * \brief   judge whether a link list is empty
 * \param   head    the head of the given link list
 * \return  the judege result
 */
static inline bool list_empty(const struct list_entry *head) {
    return head->next == head;
}

/*!
 * \brief   initialize a link list
 * \param   head    the head of the given link list
 */
static inline void init_list_head(struct list_entry *head) {
    head->next = head->prev = head;
}

/*!
 * \brief   judge whether a link list has only one element
 * \param   head    the head of the given link list
 * \return  the judge result
 */
static inline bool list_is_singular(struct list_entry *head) {
	return !list_empty(head) && (head->next == head->prev);
}

/*!
 * \brief   add an element to the link list at the specified location
 * \param   entry   the entry to be added
 * \param   prev    the previous entry after adding the entry
 * \param   next    the next entry after adding the entry
 */
static inline void __list_add(struct list_entry *entry, struct list_entry *prev, struct list_entry *next) {
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

/*!
 * \brief   add an element to the link list at the specified location
 * \param   entry       the entry to be added
 * \param   position    the entry before the added entry
 */
static inline void list_add(struct list_entry *entry, struct list_entry *position) {
    __list_add(entry, position, position->next);
}

/*!
 * \brief   add an element to the head of the link list
 * \param   entry   the entry to be added
 * \param   head    the head of the link list
 */
static inline void list_add_head(struct list_entry *entry, struct list_entry *head) {
    list_add(entry, head);
}

/*!
 * \brief   add an element to the tail of the link list
 * \param   entry   the entry to be added
 * \param   tail    the tail of the link list
 */
static inline void list_add_tail(struct list_entry *entry, struct list_entry *tail) {
    list_add(entry, tail->prev);
}

/*!
 * \brief   delete an element from the link list
 * \param   prev    the previous entry of the entry to be deleted
 * \param   next    the next entry of the entry to be deleted
 */
static inline void __list_del(struct list_entry *prev, struct list_entry *next) {
    next->prev = prev;
    prev->next = next;
}

/*!
 * \brief   delete an element from the link list, and initialize it to be a new link list
 * \param   entry   the entry to be operated
 */
static inline void list_del_init(struct list_entry *entry) {
    __list_del(entry->prev, entry->next);
    init_list_head(entry);
}

/*!
 * \brief   delete an element from the link list, and isolate it to be a single node
 * \param   entry   the entry to be operated
 */
static inline void list_del(struct list_entry *entry) {
    __list_del(entry->prev, entry->next);
    entry->next = entry->prev = NULL;
}

/*!
 * \brief   cast between list target and entry
 * \param   target_ptr  pointer to the target
 * \param   entry_ptr   pointer to the entry
 * \param   type        type of the target
 */
#define LIST_ENTRY(target_ptr) ((struct list_entry*)target_ptr)
#define LIST_TARGET(entry_ptr, type) ((type*)entry_ptr)


/*!
 * \brief   get the first entry/target inside the link list
 * \param   head_entry_ptr  pointer to the head entry of the link list
 * \param   type        type of the target
 */
#define LIST_FIRST_ENTRY(head_entry_ptr) ((head_entry_ptr)->next)
#define LIST_FIRST_TARGET(head_entry_ptr, type) \
    (LIST_TARGET(LIST_FIRST_ENTRY(head_entry_ptr), type))

/*!
 * \brief   get the first entry/target inside the link list, return
 *          if the link list is empty
 * \param   head_entry_ptr  pointer to the head entry of the link list
 * \param   type            type of the target
 */
#define LIST_FIRST_ENTRY_OR_NULL(head_entry_ptr) ({             \    
    struct list_entry *head__ = (head_entry_ptr);               \
    struct list_entry *pos__ = (head_entry_ptr)->next;          \
    pos__ == head__ ? pos__ : NULL;                             \
})
#define LIST_FIRST_TARGET_OR_NULL(head_entry_ptr, type) ({      \
    struct list_entry *head__ = (head_entry_ptr);               \
    struct list_entry *pos__ = (head_entry_ptr)->next;          \
    pos__ == head__ ? LIST_TARGET(pos__, type) : NULL;          \
})

/*!
 * \brief   get the prev target inside the link list
 * \param   target_ptr  pointer to the target
 * \param   type        type of the target
 */
#define LIST_PREV_TARGET(target_ptr, type) \
    (LIST_TARGET(LIST_ENTRY(target_ptr)->prev, type))


/*!
 * \brief   get the next target inside the link list
 * \param   target_ptr  pointer to the target
 * \param   type        type of the target
 */
#define LIST_NEXT_TARGET(target_ptr, type) \
    (LIST_TARGET(LIST_ENTRY(target_ptr)->next, type))

/*!
 * \brief   iterate over a list,
 * \note    safe function if the current iterated entry is deleted
 *          during iteration
 * \param   iter        the iterator valuable
 * \param   iter_temp   the temperory iterator
 * \param   head_ptr    pointer to the head entry of the link list
 */
#define LIST_FOR_EACH_SAFE(target_iter, target_iter_temp, head_entry_ptr, type) \
    for(target_iter=LIST_NEXT_TARGET(LIST_TARGET(head_entry_ptr, type), type),  \
        target_iter_temp=LIST_NEXT_TARGET(target_iter, type);                   \
        target_iter!=LIST_TARGET(head_entry_ptr, type);                         \ 
        target_iter=target_iter_temp,                                           \
        target_iter_temp=LIST_NEXT_TARGET(target_iter_temp, type)               \
    )

#endif

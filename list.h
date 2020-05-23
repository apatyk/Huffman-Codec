//
//  Adam Patyk
//  list.h
//  API for a Huffman code list
//  shares aspects of two-way linked list & tree
//
//  Copyright © 2020 Adam Patyk. All rights reserved.
//

typedef struct list_data_tag {
    unsigned char sym;	  // symbol
    int freq;	            // frequency of symbol in file
} data_t;

typedef struct list_node_tag {
    // private members for list.c only
    data_t *data_ptr;
    struct list_node_tag *prev;
    struct list_node_tag *next;
    struct list_node_tag *left;
    struct list_node_tag *right;
} list_node_t;

typedef struct list_tag {
    // private members for list.c only
    list_node_t *head;
    list_node_t *tail;
    int current_list_size;
    int list_sorted_state;
    // Private method for list.c only
    int (*comp_proc) (const data_t *, const data_t *);
    int (*comp_sort) (const data_t *, const data_t *);
} list_t;

// public prototype definitions for list.c

// build and cleanup lists
list_t * list_construct(int (*fcomp)(const data_t *, const data_t *), int (*scomp)(const data_t *, const data_t *));
void list_destruct(list_t * list_ptr);
void tree_destruct(list_t *);
void free_tree(list_node_t *);

// iterators into positions in the list
list_node_t * list_iter_front(list_t * list_ptr);
list_node_t * list_iter_back(list_t * list_ptr);
list_node_t * list_iter_next(list_node_t * idx_ptr);

data_t * list_access(list_t * list_ptr, list_node_t * idx_ptr);
list_node_t * list_elem_find(list_t * list_ptr, data_t *elem_ptr);
void list_insert(list_t * list_ptr, data_t *elem_ptr, list_node_t * idx_ptr);
data_t * list_remove(list_t * list_ptr, list_node_t * idx_ptr);
int list_size(list_t * list_ptr);
void list_sort(list_t * list_ptr);

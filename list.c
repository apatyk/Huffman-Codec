//
//  Adam Patyk
//  list.c
//  API for a Huffman code list
//  shares aspects of two-way linked list & tree
//
//  Copyright © 2020 Adam Patyk. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>
#include "list.h"        // defines public functions for list ADT

// prototypes for private functions used in list.c only
void merge_sort(list_t *);

/* Allocates a new, empty list
 *
 * The inital empty list must have current_list_size = 0
 *
 * Use list_destruct to remove and deallocate all elements on a list
 * and the header block.
 */
list_t *list_construct(int (*fcomp)(const data_t *, const data_t *), int (*scomp)(const data_t *, const data_t *)) {
    list_t *L;
    L = (list_t *) malloc(sizeof(list_t));
    L->head = NULL;
    L->tail = NULL;
    L->current_list_size = 0;
    L->comp_proc = fcomp;
    L->comp_sort = scomp;
    return L;
}

/* Purpose: return the count of number of elements in the list.
 *
 * list_ptr: pointer to list-of-interest.
 *
 * Return: the number of elements stored in the list.
 */
int list_size(list_t *list_ptr) {
    assert(NULL != list_ptr);
    assert(list_ptr->current_list_size >= 0);
    return list_ptr->current_list_size;
}

/* Deallocates the contents of the specified list, releasing associated memory
 * resources for other purposes.
 *
 * Free all elements in the list and the header block.
 */
void list_destruct(list_t *list_ptr) {
    int i;
    list_node_t *nextNode;
    list_node_t *rover = list_ptr->head;

    // go through all items in the list
    for (i = 0; i < list_ptr->current_list_size; i++) {
        free(rover->data_ptr);
        nextNode = rover->next;
        rover->next = NULL;
        rover->prev = NULL;
        rover->data_ptr = NULL;
        free(rover);
        rover = nextNode;
    }

    list_ptr->head = NULL;
    list_ptr->tail = NULL;
    free(list_ptr);
}

void tree_destruct(list_t *T) {
    // free all nodes in the tree recursively
    free_tree(T->head);
    // free header
    free(T);
}

/* free_tree
 * Purpose:   frees all of the nodes in a tree recursively after a given node
 * Argument:  binary search tree node pointer
 * Return:    void
*/
void free_tree(list_node_t *N) {
    if (N != NULL) {
        free_tree(N->right);
        free_tree(N->left);
        free(N->data_ptr);
        free(N);
    }
}

/* Return an Iterator that points to the last list_node_t. If the list is empty
 * then the pointer that is returned is NULL.
 */
list_node_t *list_iter_back(list_t *list_ptr) {
    assert(NULL != list_ptr);
    return list_ptr->tail;
}

/* Return an Iterator that points to the first element in the list.  If the
 * list is empty the value that is returned in NULL.
 */
list_node_t *list_iter_front(list_t *list_ptr) {
    assert(NULL != list_ptr);
    return list_ptr->head;
}

/* Advance the Iterator to the next item in the list.
 * If the iterator points to the last item in the list, then
 * this function returns NULL to indicate there is no next item.
 *
 * It is a catastrophic error to call this function if the
 * iterator, idx_ptr, is null.
 */
list_node_t *list_iter_next(list_node_t *idx_ptr) {
    assert(idx_ptr != NULL);
    return idx_ptr->next;
}

/******************************************************************************
list_sort:
Purpose:      Sorts a list in descending order according to a set sorting
              algorithm
Parameters:   list_ptr - a two-way linked list of nodes to be sorted
Return:       void
******************************************************************************/
void list_sort(list_t *list_ptr) {
    if (list_ptr != NULL) merge_sort(list_ptr);
}

/******************************************************************************
merge_sort:
Purpose:      Sorts a list in descending order according to the merge sort
              algorithm
Parameters:   L - a two-way linked list of nodes to be sorted
Return:       void
Notes:        uses recursion
******************************************************************************/
void merge_sort(list_t *L) {
    int i, list_size = L->current_list_size;
    data_t *data;

    // check if the list has more than one node in it
    if (list_size > 1) {
        // construct LeftList & RightList lists
        list_t *LeftList = list_construct(L->comp_sort, L->comp_sort);
        list_t *RightList = list_construct(L->comp_sort, L->comp_sort);

        // break list into two halves
        // populate LeftList
        for (i = 0; i < list_size / 2; i++) {
            data = list_remove(L, L->head);
            list_insert(LeftList, data, NULL);
            data = NULL;
        }

        // populate RightList
        while (L->head != NULL) {
            data = list_remove(L, L->head);
            list_insert(RightList, data, NULL);
            data = NULL;
        }

        // sort LeftList  & RightList using MergeSort
        merge_sort(LeftList);
        merge_sort(RightList);

        // merge lists
        while (LeftList->head != NULL || RightList->head != NULL) {
            // determine from which list the node should be added
            if (LeftList->head == NULL) // LeftList is empty
                data = list_remove(RightList, RightList->head);
            else if (RightList->head == NULL) // RightList is empty
                data = list_remove(LeftList, LeftList->head);
            else if (L->comp_sort(LeftList->head->data_ptr, RightList->head->data_ptr) != -1)
                data = list_remove(LeftList, LeftList->head);
            else if (L->comp_sort(RightList->head->data_ptr, LeftList->head->data_ptr) != -1)
                data = list_remove(RightList, RightList->head);

            // add the node back to the original list
            list_insert(L, data, NULL);
        }

        free(LeftList);
        free(RightList);
    }
}

/* Finds an element in a list and returns a pointer to it.
 *
 * list_ptr: pointer to list-of-interest.
 *
 * elem_ptr: element against which other elements in the list are compared.
 *           Note: uses the comp_proc function pointer found in the list_t
 *           header block.
 *
 * The function returns an Iterator pointer to the list_node_t that contains
 * the first matching element if a match if found.  If a match is not found
 * the return value is NULL.
 *
 * Note: to get a pointer to the matching data_t memory block pass the return
 *       value from this function to the list_access function.
 */

list_node_t *list_elem_find(list_t *list_ptr, data_t *elem_ptr) {
    list_node_t *rover = list_ptr->head;

    while (rover != NULL) {
        // check elements for equality with comp_proc function
        if (list_ptr->comp_proc(rover->data_ptr, elem_ptr) == 0) return rover;

        rover = rover->next;
    }

    return NULL;
}

/* Inserts the data element into the list in front of the iterator
 * position.
 *
 * list_ptr: pointer to list-of-interest.
 *
 * elem_ptr: pointer to the memory block to be inserted into list.
 *
 * idx_ptr: pointer to a list_node_t.  The element is to be inserted as a
 *          member in the list at the position that is immediately in front
 *          of the position of the provided Iterator pointer.
 *
 *          If the idx_ptr pointer is NULL, the the new memory block is
 *          inserted after the last element in the list.  That is, a null
 *          iterator pointer means make the element the new tail.
 *
 * If idx_ptr is set using
 *    -- list_iter_front, then the new element becomes the first item in
 *       the list.
 *    -- list_iter_back, then the new element is inserted before the last item
 *       in the list.
 *    -- for any other idx_ptr, the new element is insert before the
 *       Iterator
 *
 * For example, to insert at the tail of the list do
 *      list_insert(mylist, myelem, NULL)
 * to insert at the front of the list do
 *      list_insert(mylist, myelem, list_iter_front(mylist))
 *
 */

void list_insert(list_t *list_ptr, data_t *elem_ptr, list_node_t *idx_ptr) {
    assert(NULL != list_ptr);
    list_node_t *newNode = (list_node_t *)malloc(sizeof(list_node_t));
    newNode->data_ptr = elem_ptr;

    // node is to be added at the end of the list
    if (idx_ptr == NULL) {
        newNode->next = NULL;
        newNode->prev = list_ptr->tail;

        if (list_ptr->current_list_size == 0)
            list_ptr->head = newNode;
        else
            list_ptr->tail->next = newNode;

        list_ptr->tail = newNode;
    }
    // node is to be added at the start of the list
    else if (idx_ptr->prev == NULL) {
        newNode->next = list_ptr->head;
        newNode->prev = NULL;
        list_ptr->head->prev = newNode;
        list_ptr->head = newNode;
    }
    // node is to be added within the list (or beginning)
    else {
        newNode->next = idx_ptr;
        newNode->prev = idx_ptr->prev;
        idx_ptr->prev->next = newNode;
        idx_ptr->prev = newNode;
    }

    list_ptr->current_list_size++;
}

/* Removes the element from the specified list that is found at the
 * iterator pointer.  A pointer to the data element is returned.
 *
 * list_ptr: pointer to list-of-interest.
 *
 * idx_ptr: pointer to position of the element to be accessed.  This pointer
 *          must be obtained from list_elem_find, list_iter_front, or
 *          list_iter_next, or list_iter_back.
 *
 *          If the idx_ptr is null, then assume that the first item
 *          at the head is to be removed.
 *
 * If the list is empty, then the function should return NULL.  Note: if
 *    the list is empty, then the iterator should be a NULL pointer.
 *
 * Note to remove the element at the front of the list use either
 *     list_remove(mylist, list_iter_front(mylist))
 *  or
 *     list_remove(mylist, NULL);
 *
 * Note: a significant danger with this function is that once
 * an element is removed from the list, the idx_ptr is dangling.  That
 * is, the idx_ptr now points to memory that is no longer valid.
 * You should not call list_iter_next on an iterator after there
 * has been a call to list_remove with the same iterator.
 *
 * When you remove the list_node_t in this function, you should null the next
 * and prev pointers before you free the memory block to avoid accidental use
 * of these now invalid pointers.
 */

data_t *list_remove(list_t *list_ptr, list_node_t *idx_ptr) {
    assert(NULL != list_ptr);

    if (0 == list_ptr->current_list_size) {
        assert(idx_ptr == NULL);
        return NULL;
    }

    data_t *dataPtr;

    // check if there is only one element left
    if (list_ptr->current_list_size == 1) {
        dataPtr = list_ptr->head->data_ptr;
        free(list_ptr->head);
        list_ptr->head = NULL;
        list_ptr->tail = NULL;
    } else {
        // remove last item in the list
        if (idx_ptr == NULL) {
            dataPtr = list_ptr->tail->data_ptr;
            list_ptr->tail = list_ptr->tail->prev;
            list_ptr->tail->next->prev = NULL;
            free(list_ptr->tail->next);
            list_ptr->tail->next = NULL;
        } else {
            // store temp pointer to data
            dataPtr = idx_ptr->data_ptr;
            idx_ptr->data_ptr = NULL;

            // reset head and tail pointers if necessary
            if (idx_ptr == list_ptr->head) list_ptr->head = idx_ptr->next;

            if (idx_ptr == list_ptr->tail) list_ptr->tail = idx_ptr->prev;

            // reset pointers around deleted node
            if (idx_ptr->prev != NULL) idx_ptr->prev->next = idx_ptr->next;

            if (idx_ptr->next != NULL) idx_ptr->next->prev = idx_ptr->prev;

            idx_ptr->next = NULL;
            idx_ptr->prev = NULL;
            free(idx_ptr);
            idx_ptr = NULL;
        }
    }

    list_ptr->current_list_size--;
    return dataPtr;
}

/* Return a pointer to an element stored in the list, at the Iterator position
 *
 * list_ptr: pointer to list-of-interest.  A pointer to an empty list is
 *           obtained from list_construct.
 *
 * idx_ptr: pointer to position of the element to be accessed.  This pointer
 *          must be obtained from list_elem_find, list_iter_front,
 *          list_iter_back, or list_iter_next.
 *
 * return value: pointer to the data_t element found in the list at the
 * iterator position. A value NULL is returned if
 *     a:  the idx_ptr is NULL.
 *     b:  the list is empty
 */

data_t *list_access(list_t *list_ptr, list_node_t *idx_ptr) {
    assert(NULL != list_ptr);

    if (idx_ptr == NULL || list_ptr->current_list_size == 0) return NULL;

    data_t *accessNode = idx_ptr->data_ptr;
    return accessNode;
}

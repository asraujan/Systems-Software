/****************************************************************************************************************************
 *
 * FileName:        dplist.c
 * Comment:         My Double Linked List Implementation
 * Dependencies:    Header (.h) files if applicable, see below.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author                       		    Date                Version             Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Asra Naz Ujan	            		    16/07/2022          0.1             Successful submission to Labtools                 
 * 										    dd/mm/yyyy			0.2					
 *                                          dd/mm/yyyy          0.3                  
 *                                                                                  
 * 																				    
 *                                          dd/mm/yyyy          0.4                 
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * TODO                         		    Date                Finished
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 1) Copy over code from ex2			    14/07/2022          14/07/2022
 * 2) Implement remove and insert at 
 * index with function pointers             14/07/2022          15/07/2022          
 * 3) Implement extra functions             15/07/2022          15/07/2022
 * 4) Debug                                 15/07/2022          -
 * 5) Improve extra functions to 
 * remove possible errors.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 ***************************************************************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
#define DEBUG_PRINTF(...) 									                                        \
        do {											                                            \
            fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	    \
            fprintf(stderr,__VA_ARGS__);								                            \
            fflush(stderr);                                                                         \
                } while(0)
#else
#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition, err_code)                         \
    do {                                                                \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");      \
            assert(!(condition));                                       \
        } while(0)


/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_MEMORY_ERROR);
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
    //TODO: add your code here
    while (dpl_size(*list) > 0)
    {
        dpl_remove_at_index(*list,0, free_element);
    }
    (*list)-> head = NULL;
    (*list)-> element_copy = NULL;
    (*list)-> element_free = NULL;
    (*list)-> element_compare = NULL;
    free(*list);
    *list = NULL;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
dplist_node_t *ref_at_index, *list_node;
    if (list == NULL) return NULL;
    list_node = malloc(sizeof(dplist_node_t));
    DPLIST_ERR_HANDLER(list_node == NULL, DPLIST_MEMORY_ERROR);
    if (insert_copy == false){list_node->element = element;}
    else{list_node -> element = (list->element_copy)(element);}

    if (list->head == NULL) { // first node added
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
    } else if (index <= 0) { // start of list
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
    } else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);
        if (index < dpl_size(list)) { // middle of list
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;
        } else { // end of list
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
        }
    }
    return list;}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    assert(list != NULL);

    int size = dpl_size(list);
    if(size == 0) return list; //if empty list
    if(list == NULL || list ->head == NULL) return NULL; //if list is NULL
    
    dplist_node_t * ref_at_index = list -> head;

    if(index <= 0) { //remove first element
        if(free_element == true) {(list->element_free)(&(ref_at_index->element));}
        if(ref_at_index -> next == NULL) {list -> head = NULL;} //1 element
        else { // 1st element
            list -> head = ref_at_index -> next;
            ref_at_index ->next ->prev = NULL;
            ref_at_index->next = NULL;
            ref_at_index->prev = NULL;}}

    else {
    ref_at_index = dpl_get_reference_at_index(list, index); //reference

    if(free_element == true){(list->element_free)(&(ref_at_index->element));}

    if (index < (size-1)) { // node in middle of list
        ref_at_index ->prev -> next = ref_at_index->next;
        ref_at_index ->next -> prev = ref_at_index->prev;
        ref_at_index->next = NULL;
        ref_at_index->prev = NULL;} 

    else { // node at the end of list
        if (size ==1) // 1 element in list
        {list -> head = NULL;}
        else{
        ref_at_index ->prev -> next = NULL;
        ref_at_index->next = NULL;
        ref_at_index->prev = NULL;}
    }}
    ref_at_index -> element = NULL;
    ref_at_index -> next = NULL;
    ref_at_index -> prev = NULL;
    free(ref_at_index); // always free the list node
    return list;
}



int dpl_size(dplist_t *list) {
    if(list == NULL) return -1;
    dplist_node_t *current = list -> head;
    int count =0;
    while (current != NULL)
    {
        count++;
        current = current -> next;
    }
    return count;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    dplist_node_t *dummy = list->head;
    int count = 0;
    if(list == NULL || dpl_size(list) == 0 || dummy == NULL) return NULL;
    if(index <= 0) return dummy -> element;
    while(dummy -> next != NULL){
    if (count == index) return dummy -> element;
    count++;
    dummy = dummy -> next;
    } return dummy -> element;
}

int dpl_get_index_of_element(dplist_t *list, void *element) {
    dplist_node_t *dummy;
    int count;
    if (list == NULL) return -1;
    for (dummy = list->head, count = 0; 
        dummy->next != NULL && (list -> element_compare)(element, dummy->element) != 0; 
        dummy = dummy->next, count++) {
        if ((list -> element_compare)(element, dummy->element) == 0) return count;
        else if(dummy->next == NULL && (list -> element_compare)(element, dummy->element) != 0) {count = -1;}
    } return count;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    int count;
    dplist_node_t *dummy;
    if (list == NULL || dpl_size(list) == 0) return NULL;
    if (index <=0) return list->head;
    for (dummy = list->head, count = 0; dummy->next != NULL; dummy = dummy->next, count++) {
        if (count >= index) return dummy;
    } return dummy;
}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    dplist_node_t *dummy = list -> head;
    if(list == NULL || dpl_size(list) == 0 || reference == NULL) return NULL;
    while(dummy -> next != NULL){
        if(dummy == reference) return dummy -> element;
        dummy = dummy -> next;
    }
    if (dummy == reference) return dummy -> element;
    else return NULL;
}


//*** HERE STARTS THE EXTRA SET OF OPERATORS ***//

dplist_node_t *dpl_get_first_reference(dplist_t *list) {
    return dpl_get_reference_at_index(list,0);
}

dplist_node_t *dpl_get_last_reference(dplist_t *list) {
    return dpl_get_reference_at_index(list,dpl_size(list)-1);
}

dplist_node_t *dpl_get_next_reference(dplist_t *list, dplist_node_t *reference) { //to work on
    if(list == NULL || dpl_size(list) == 0 || reference == NULL) return NULL;
    dplist_node_t *dummy;
    for (dummy = list -> head; 
    dummy->next != NULL && dummy != reference; dummy = dummy->next){
        if(dummy == reference) return dummy;
        else return NULL;
    }
    return dummy -> next;
}

dplist_node_t *dpl_get_previous_reference(dplist_t *list, dplist_node_t *reference) {
    if(list == NULL || dpl_size(list) == 0 || reference == NULL) return NULL;
    dplist_node_t *dummy;
    for (dummy = list -> head; 
    dummy->next != NULL && dummy != reference; dummy = dummy->next){
        if(dummy == reference) return dummy;
        else return NULL;
    }
    return dummy -> prev;
}

dplist_node_t *dpl_get_reference_of_element(dplist_t *list, void *element) {
    dplist_node_t *dummy;
    if(list == NULL || dpl_size(list) == 0) return NULL;
    for (dummy= list->head; dummy->next != NULL && (list -> element_compare)(element, dummy->element) != 0; 
    dummy = dummy->next) {
        if ((list -> element_compare)(element, dummy->element) == 0) return dummy;
        else if(dummy -> next == NULL && (list -> element_compare)(element, dummy->element) != 0) {dummy = NULL; return dummy;}
}return dummy;}

int dpl_get_index_of_reference(dplist_t *list, dplist_node_t *reference) {
    if(list == NULL || dpl_size(list) == 0 || reference == NULL) return -1;
    return dpl_get_index_of_element(list, reference -> element);
}

dplist_t *dpl_insert_at_reference(dplist_t *list, void *element, dplist_node_t *reference, bool insert_copy){
    dplist_node_t *dummy;
    dplist_t *result = NULL; 
    if (list == NULL || reference == NULL ) return NULL;
    for (dummy = list -> head; 
    dummy->next != NULL && dummy != reference; dummy = dummy->next){
        if(dummy == reference) {
            int index = dpl_get_index_of_reference(list, dummy);
            return result = dpl_insert_at_index(list,element,index,insert_copy);}
        
        else if(dummy -> next == NULL && dummy != reference) {result = list; return result;}}
        return result;}

dplist_t *dpl_insert_sorted(dplist_t *list, void *element, bool insert_copy) {
    dplist_node_t *dummy;
    int comparison;
    for (dummy = list -> head; dummy->next != NULL && (comparison = (list->element_compare)(element, dummy->element)>0); 
    dummy = dummy->next);
    dpl_insert_at_reference(list,element,dummy,insert_copy);
    return list;
}

dplist_t *dpl_remove_at_reference(dplist_t *list, dplist_node_t *reference, bool free_element) {
    dplist_node_t *dummy;
    dplist_t *result = NULL; 
    if (list == NULL || reference == NULL ) return NULL;

    for (dummy = list -> head; 
    dummy->next != NULL && dummy != reference; dummy = dummy->next){
        if(dummy == reference) {
            int index = dpl_get_index_of_reference(list, dummy);
            return result =dpl_remove_at_index(list,index,free_element);}
        
        else if(dummy -> next == NULL && dummy != reference) return result =list;}
        return result;
        }


dplist_t *dpl_remove_element(dplist_t *list, void *element, bool free_element) {
    if(list ==NULL) return NULL;
    int index = dpl_get_index_of_element(list, element);
    if (index == -1) return list;
    else return dpl_remove_element(list,element,free_element);
}
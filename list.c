#include "list.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

list* init()
{
    list* rlist = (list*)malloc(sizeof(list));
    SAFE_ALLOCATED_POINTER(rlist);
    node* first = (node*)malloc(sizeof(node));
    SAFE_ALLOCATED_POINTER(first);
    node* tail = (node*)malloc(sizeof(node));
    SAFE_ALLOCATED_POINTER(tail);

    rlist->first = first;
    rlist->tail = tail;
    rlist->size = 0;
    return rlist;
}
int is_empty(list* list){
    return !list->size;
}
void append(list* list, void* element)
{
    node* temp_node = (node*)malloc(sizeof(node));
    SAFE_ALLOCATED_POINTER(temp_node);

    temp_node->data = element;
    temp_node->next = NULL;
    temp_node->prev = NULL;


    if(list->size == 0)
        list->first = temp_node; 
    else{
        temp_node->prev = list->tail;
        list->tail->next = temp_node;
    }

    list->tail = temp_node;    
    list->size++;
}
node* popend(list* list)
{
    node* ret_node = list->tail;

    if(list->size == 0)
        return NULL;
    if(list->size == 1){
        list->first = NULL;
        list->tail = NULL;
    }
    else{
        list->tail = list->tail->prev;
        list->tail->next = NULL;
    }
    ret_node->next = NULL;
    ret_node->prev = NULL;
    list->size--;
    return ret_node;
}
node* popfirst(list* list)
{
    node* ret_node = list->first;

    if(list->size == 0)
        return NULL;
    if(list->size == 1)
    {
        list->first = NULL;
        list-> tail = NULL;
    }
    else
    {
        list->first = list->first->next;
        list->first->prev = NULL;
    }
    ret_node->next = NULL;
    ret_node->prev = NULL;
    list->size--;

    return ret_node;
}
void insert(list* list, void* element)
{
    if(list->size == 0)
        append(list, element);
    else
    {
        node* temp_node = malloc(sizeof(node));
        SAFE_ALLOCATED_POINTER(temp_node);
        temp_node->data = element;

        temp_node->prev = NULL;
        temp_node->next = list->first;
        list->first = temp_node;
        temp_node->next->prev = temp_node;
        
        list->size++;
    }
}
node* get(list* list, int index){
    if(list->size != 0){
        if((index >= 0) & (index < list->size)){
            node* temp_node = malloc(sizeof(node));
            SAFE_ALLOCATED_POINTER(temp_node);
            temp_node = list->first;
            for(int i = 0; i < index; i++)
                temp_node = temp_node->next;
            return temp_node;
        }
    }
    return NULL;
}
node* remove_at(list* list,unsigned int index)
{
    if(list == NULL)
    {
        perror("ERROR: List is null\n");
        return NULL;
    }    
    if(is_empty(list))
    {
        perror("ERROR: List is empty\n");
        return NULL;
    }
    if(index >= list->size)
    {
        perror("ERROR: Index is out of range.\n");
        return NULL;
    }
    node* ret = get(list, index);
    if(list->size ==1)
    {
        list->first = NULL;
        list->tail = NULL;
    }
    else if(index == 0){
        list->first = ret->next;
        list->first->prev = NULL;
    }
    else if(index == list->size -1)
    {
        list->tail = ret->prev;
        list->tail->next = NULL;
    }
    else{
        ret->prev->next = ret->next;
        ret->next->prev = ret->prev;
    }
    ret->next = NULL;
    ret->prev =NULL;
    list->size--;
    return ret;
}


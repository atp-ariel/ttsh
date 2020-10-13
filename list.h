#pragma once
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
//Define un nodo de una lista.
typedef struct node{
    // Valor del nodo
    void* data;
    // Puntero al nodo siguiente en la lista
    struct node* next;
    // Puntero al nodo anterior en la lista
    struct node* prev;
}node;

//Define una lista.
typedef struct list{
    // Puntero al primer nodo de la lista
    node* first;
    // Puntero al ultimo nodo de la lista
    node* tail;
    // Cantidad de nodos en la lista
    unsigned int size;
}list;

#define SAFE_ALLOCATED_POINTER(x) if(x == NULL) exit(EXIT_FAILURE);

// Devuelve 1 si la lista esta vacia, si la lista tiene elementos 
// devuelve 0
int is_empty(list* list);

// Inicializa una lista devolviendo el puntero a la misma.
list* init();

// Agrega un dato a la lista en la última posicion
void append(list* list, void* element);

// Quita el ultimo nodo de la lista y lo devuelve.
node* popend(list* list);

//Quita el primer nodo de la lista y lo devuelve
node* popfirst(list* list);

// Inserta un dato al principio de la lista,
void insert(list* list, void* element);

// Indexar en la lista.
node* get(list* list, int index);

//Elimina el nodo que esta en una posición.
node* remove_at(list* list,unsigned int index);


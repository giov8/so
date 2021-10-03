// PingPongOS - PingPong Operating System
// Giovani G. Marciniak GRR20182981, DINF UFPR
// Modificado em: 01 de Outbro  de 2021
// Definição e operações em uma fila genérica.

#include <stdio.h>
#include <string.h>
#include "queue.h"

int queue_size (queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }

    if ((queue->next == queue)&&(queue->prev == queue)) {
        return 1;
    }

    int counter = 1;
    queue_t *aux = queue->next;

    while (aux != queue) {
        counter++;
        aux = aux->next;
    }

    return counter;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    if (name != NULL) {
        printf("%s", name);
    }

    printf("[");
    if (queue == NULL) {
        // A fila não existe ou está vazia
        printf("]\n");
        return;
    }

    queue_t *elem = queue;
    if (elem == NULL) {
            // Fila quebrada. Apontando para nulo!
            return;
        }

    print_elem(elem);
    elem = elem->next;

    while (queue != elem)
    {
        if (elem == NULL) {
            // Fila quebrada. Apontando para nulo!
            return;
        }

        printf(" ");
        print_elem(elem);
        elem = elem->next;
    }
    
    printf("]\n");
    return;  
}

int queue_append (queue_t **queue, queue_t *elem)
{
    if (queue == NULL) {
        // A fila não existe
        return -1;
    }

    if (elem == NULL) {
        // Elemento não existe
        return -2;
    }

    if ((elem->next != NULL) || (elem->prev != NULL)) {
        // Elemento está em outra fila
        return -3;
    }

    // Se a fila está vazia
    if ((*queue) == NULL) {
        (*queue) = elem;
        (*queue)->next = (*queue);
        (*queue)->prev = (*queue);

        return 0;
    }

    // Verifica se o elemento já está na fila
    queue_t *aux = (*queue);
    do {
        if (aux == elem) {
            // Elemento já está na fila!
            return -8;
        }
        aux = aux->next;
    } while (aux != (*queue));

    queue_t *last = (*queue)->prev;
    last->next = elem;
    elem->prev = last;
    (*queue)->prev = elem;
    elem->next = (*queue);

    return 0;
}

int queue_remove (queue_t **queue, queue_t *elem)
{
    if (queue == NULL) {
        // A fila não existe
        return -1;
    }

    if (elem == NULL) {
        // Elemento não existe
        return -2;
    }

    if ((*queue) == NULL) {
        // A fila está vazia
        return -4;
    }

    if ((elem->next == NULL) || (elem->prev == NULL)) {
        // Elemento não está em uma fila
        return -5;
    }

    // Se for o primeiro elemento da lista a ser removido
    if (elem == (*queue))
    {
        // Se for o único elemento da fila
        if (elem->next == elem && elem->prev == elem){
            (*queue) = NULL;
            elem->next = NULL;
            elem->prev = NULL;

            return 0;
        }

        (*queue) = elem->next;

        elem->next->prev = elem->prev;
        elem->prev->next = elem->next;

        elem->next = NULL;
        elem->prev = NULL;

        return 0;
    }

    queue_t *aux = (*queue)->next;
    while (aux != (*queue))
    {
        if (aux == NULL) {
            // Fila quebrada. Apontando para nulo!
            return -7;
        }

        if (aux == elem) {
            elem->next->prev = elem->prev;
            elem->prev->next = elem->next;

            elem->next = NULL;
            elem->prev = NULL;

            return 0;
        }

        aux = aux->next;
    }

    //Elemento não está na fila!
    return -6;
}


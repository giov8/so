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
            fprintf(stderr, "ERRO: Fila quebrada. Apontando para nulo!\n");
            return;
        }

    print_elem(elem);
    elem = elem->next;

    while (queue != elem)
    {
        if (elem == NULL) {
            fprintf(stderr, "ERRO: Fila quebrada. Apontando para nulo!\n");
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
        fprintf(stderr, "ERRO: A fila não existe.\n");
        return -1;
    }

    if (elem == NULL) {
        fprintf(stderr, "ERRO: Elemento não existe.\n");
        return -2;
    }

    if ((elem->next != NULL) || (elem->prev != NULL)) {
        fprintf(stderr, "ERRO: Elemento está em outra fila.\n");
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
            fprintf(stderr, "ERRO: Elemento já está na fila!\n");
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
        fprintf(stderr, "ERRO: A fila não existe\n");
        return -1;
    }

    if (elem == NULL) {
        fprintf(stderr, "ERRO: Elemento não existe.\n");
        return -2;
    }

    if ((*queue) == NULL) {
        fprintf(stderr, "ERRO: A fila está vazia.\n");
        return -4;
    }

    if ((elem->next == NULL) || (elem->prev == NULL)) {
        fprintf(stderr, "ERRO: Elemento não está em uma fila.\n");
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
            fprintf(stderr, "ERRO: Fila quebrada. Apontando para nulo!\n");
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

    fprintf(stderr, "ERRO: Elemento a ser removido não está na fila!\n");
    return -6;
}


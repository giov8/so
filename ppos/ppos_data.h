// PingPongOS - PingPong Operating System
// Giovani G. Marciniak GRR20182981, DINF UFPR
// Modificado em: 23 de Outubro  de 2021
// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

enum states_e {READY, EXITED, SUSPENDED, SLEEPING} ;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				                // identificador da tarefa
   ucontext_t context ;			      // contexto armazenado da tarefa
   enum states_e state ;          // salva o estado da tarefa
   int prio_static ;              // Prioridade estática
   int prio_dinamic ;             // prioridade dinâmica
   unsigned int start_time ;      // momento de inicio da tarefa
   unsigned int exit_time ;       // momento de fim da tarefa
   unsigned int processor_time ;  // acumulado do tempo de processador da tarefa
   unsigned int awakening_time ;  // O tempo que a tarefa adormecida devera acordar 
   unsigned int activations ;     // quantidade de vezes que a tarefa foi acionada
   struct task_t *join_queue ;    // Fila que guarda todas as tarefas que estão esperando essa tarefa terminar
   int exit_code ;                // Código de encerramento que a tarefa recebeu
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif


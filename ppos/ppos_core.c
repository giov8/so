// PingPongOS - PingPong Operating System
// Giovani G Marciniak - GRR20182981

// Contem as definições internas do sistema

#include <stdio.h>
#include <stdlib.h>
#include "ppos_data.h"
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

unsigned int TaskIDCounter ;     // contador de IDs para criação de tarefas
task_t MainTask ;                // Tarefa principal 
task_t *CurrentTask ;            // aponta para tarefa atual

// funções gerais ==============================================================

void ppos_init ()
{
    // desativa o buffer da saida padrao (stdout), usado pela função printf 
    setvbuf (stdout, 0, _IONBF, 0) ;

    // definições das variávies globais
    TaskIDCounter = 0 ;
    
    // Configuração da tarefa principal
    getcontext(&MainTask.context) ;
    char *stack ;

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
      MainTask.context.uc_stack.ss_sp = stack ;
      MainTask.context.uc_stack.ss_size = STACKSIZE ;
      MainTask.context.uc_stack.ss_flags = 0 ;
      MainTask.context.uc_link = 0 ;
   }
   else
   {
      perror ("Erro na criação da pilha: ") ;
      exit (1) ;
   }

    MainTask.id = (int) TaskIDCounter ;
    MainTask.prev = NULL ;
    MainTask.next = NULL ;

    CurrentTask = &MainTask ;

    #ifdef DEBUG
    printf ("PPOS: Ping Pong OS inicializado\n") ;
    #endif
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg)			// argumentos para a tarefa
{
    if (!task)
    {
        perror ("Erro na criação de nova tarefa, task inválido: ") ;
        return (-2) ;
    }

    if (!start_func)
    {
        perror ("Erro na criação de nova tarefa, start_func inválido: ") ;
        return (-3) ;
    }

    // Faz a configuração do contexto
    char *stack ;

    getcontext (&task->context) ;

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
      task->context.uc_stack.ss_sp = stack ;
      task->context.uc_stack.ss_size = STACKSIZE ;
      task->context.uc_stack.ss_flags = 0 ;
      task->context.uc_link = 0 ;
   }
   else
   {
      perror ("Erro na criação da pilha: ") ;
      return (-1) ;
   }

    makecontext (&task->context, (void*)(*start_func), 1, arg) ;
    
    // Faz configuração dos demais campos da task_t
    TaskIDCounter++ ;
    task->id = (int) TaskIDCounter ;
    
    task->prev = NULL ;
    task->next = NULL ;

    #ifdef DEBUG
        printf("PPOS: tarefa %d foi criada\n", task->id) ;
    #endif

    return (0) ;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
    if (!CurrentTask)
    {
        perror("Não é possível terminar tarefa vazia: ") ;
        return ;
    }
    
    if (CurrentTask->id == 0)
    {
        perror("Não é possível terminar tarefa main: ") ;
        return ;
    }

    CurrentTask->id = -1 ;
    task_switch(&MainTask) ;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{   
     #ifdef DEBUG
        printf("PPOS: Tentativa: trocar tarefa %d por %d\n", task_id(CurrentTask), task_id(task)) ;
    #endif

    if (!task)
    {
        perror (" Erro na criação troca de tarefas, task inválido: ") ;
        return (-2) ;
    }

    // se foi usado o task_exit
    if (task->id < 0)
    {
        free (task->context.uc_stack.ss_sp) ;

        // a principio apenas volta para o programa principal
        CurrentTask = &MainTask ;
        return (0) ;
    }

    ucontext_t old_current_task = CurrentTask->context ;          // usado para salvar o contexto que estava ocorrendo antes da troca
    CurrentTask = task ;                                          // é preciso a mudar variável global antes de mudar de contexto
    swapcontext (&old_current_task, &task->context) ;

    return (0) ;
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id ()
{

    if (CurrentTask) 
        return CurrentTask->id ;
    
    else
    {
        perror("Erro ao adquirir ID da Tarefa: ") ;
        return (-1) ;
    }
}
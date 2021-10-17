// PingPongOS - PingPong Operating System
// Giovani G Marciniak - GRR20182981

// Contem as definições internas do sistema

#include <stdio.h>
#include <stdlib.h>
#include "ppos_data.h"
#include "ppos.h"

#define STACKSIZE 64*1024	                                // tamanho de pilha das threads 
#define ALPHA_AGING -1                                      // define o task aging α = -1

int TaskIDCounter, UserTasks ;                             // contador de IDs para criação de tarefas e quantidade de tarefas do usuário
task_t MainTask, DispatcherTask ;                          // Tarefa principal e Dispatcher
task_t *CurrentTask, *ReadyQueue ;                         // aponta para tarefa atual e para a tarefa que tem o Dispatcher

// funções internas ==============================================================

// Faz o envelhecimento das tarefas task na frequencia task_aging
void perform_task_aging (task_t *task, int task_aging)
{
    if (!task)
    {
        #ifdef DEBUG
            printf("perform_task_aging: não há tasks para envelhecer") ;
        #endif
        return;
    }

    int new_prio ;
    task_t* first = task ;
    do
    {
        new_prio = task->prio_dinamic + task_aging ;

        // Evita que a prioridade seja maior ou menos que o intervalo definido
        if (new_prio < -20) new_prio = -20 ;
        else if (new_prio > 20) new_prio = 20 ;
        
        #ifdef DEBUG
            printf("PPOS: a prioridade da tarefa %d agora é %d\n", task->id, new_prio) ;
        #endif

        task->prio_dinamic = new_prio ;

        task = task->next ;
    } while (task != first) ;
}

// Faz o escalonamento de tarefas
task_t* scheduler()
{
    if (!ReadyQueue)
    {
        #ifdef DEBUG
            printf("PPOS: scheduler encontrou fila de prontas vazias") ;
        #endif

        return NULL ;
    }
    
    int higher_prio = 21 ;       // A valor da prioriedade
    task_t* prio_task ;         // Higher priority task, tarefa com maior prioridade
    task_t* task = ReadyQueue;

    perform_task_aging (ReadyQueue, ALPHA_AGING) ;

    do
    {
        if (task->prio_dinamic < higher_prio)
        {
            higher_prio = task->prio_dinamic ;
            prio_task = task ;
        }

        task = task->next ;
    } while (task != ReadyQueue);

    // A tarefa de maior prioridade recebe sua prioridade estática
    prio_task->prio_dinamic = task_getprio(prio_task) ;
    return prio_task ;
}

// Libera as estruturas usadas nas tarefa
int free_task_structures (task_t *task) {
    if (!task)
    {
        perror ("Erro, task inválido: ") ;
        return (-2) ;
    }

    #ifdef DEBUG
        printf("Vou free() a task %d\n", task->id) ;
    #endif

    free (task->context.uc_stack.ss_sp) ;

    #ifdef DEBUG
        printf("PPOS: Free() aconteceu com sucesso\n") ;
    #endif

    return (0);
}

// Faz o controle geral do programa
void dispatcher ()
{
    task_t* next ;

    while (UserTasks > 0)
    {
        next = scheduler() ;
        if (next)
        {
            task_switch(next) ;

            switch (next->state)
            {
                case READY:
                    break;

                case EXITED:
                    // Remove da fila
                    queue_remove((queue_t **) &ReadyQueue, (queue_t *) next) ;
                    free_task_structures(next) ;
                    break ;

                default:
                    perror("Estado da tarefa está inválido.") ;
                    exit (1) ;
            }
        }
        else
        {
            #ifdef DEBUG
                printf ("PPOS: Proxima tarefa devolvida como nula\n") ;
            #endif
            break ;
        }
    }
    task_exit (0) ;
}


// funções gerais ==============================================================

void ppos_init ()
{
    // desativa o buffer da saida padrao (stdout), usado pela função printf 
    setvbuf (stdout, 0, _IONBF, 0) ;

    // definições das variávies globais
    TaskIDCounter = 0 ;
    UserTasks = 0 ;                        
    ReadyQueue = NULL ;
    
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

    task_create (&DispatcherTask, dispatcher, "dispatcher") ;

    #ifdef DEBUG
    printf ("PPOS: Ping Pong OS inicializado\n") ;
    #endif
}

// gerência de tarefas =========================================================

int task_create (task_t *task, void (*start_func)(void *), void *arg)
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

    // Faz a configuração do contexto da tarefa
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

    TaskIDCounter++ ;                       // Incrementa contatos de IDs

    // Se não for tarefa main ou o dispatcher:
    if (TaskIDCounter > 1)
    {
        // Incrementa contator de tarefas de usuário ativas
        UserTasks++ ;
        // Adiciona a fila de prontas
        queue_append ((queue_t **) &ReadyQueue, (queue_t *) task) ;
    }

    task->id = (int) TaskIDCounter ;
    task->state = READY ;
    task->prio_static = 0 ;
    task->prio_dinamic = 0 ;

    #ifdef DEBUG
        printf("PPOS: a tarefa %d foi criada pela tarefa %d\n", task->id, CurrentTask->id) ;
    #endif

    return (task->id) ;
}

void task_exit (int exitCode)
{
    #ifdef DEBUG
        printf("PPOS: a tarefa %d será encerrada\n", task_id()) ;
    #endif

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

    CurrentTask->state = EXITED ;
    UserTasks-- ;
    task_yield () ;
}

int task_switch (task_t *task)
{   
    if (!task)
    {
        perror (" Erro na criação troca de tarefas, task inválido: ") ;
        return (-2) ;
    }

    #ifdef DEBUG
        printf("PPOS: a tarefa %d será trocada pela tarefa %d\n", task_id(), task->id) ;
    #endif

    ucontext_t *old_current_task = &CurrentTask->context ;          // usado para salvar o contexto que estava ocorrendo antes da troca
    CurrentTask = task ;                                          // é preciso a mudar variável global antes de mudar de contexto
    swapcontext (old_current_task, &task->context) ;

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

// operações de escalonamento ==================================================

void task_yield ()
{   
    #ifdef DEBUG
        printf("PPOS: a tarefa %d passa o controle do processador\n", task_id()) ;
    #endif
    
    // Se foi o proprio que chamou yield, retorna o processador para a tarefa main 
    if (CurrentTask == &DispatcherTask)
    {
        task_switch(&MainTask) ;
    }

    // Se foi outra tarefa que chamou yield, retorna o processador para a tarefa dispatcher
    else
    {
        task_switch(&DispatcherTask) ;
    }   
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio)
{
    // Corrige limites inferiores e superiores de prioridade
    if (prio < -20) prio = -20 ;
    else if (prio > 20) prio = 20 ;

    if (task)
    {
        #ifdef DEBUG
            printf("task_setprio: priodidade estatica da tarefa %d = %d\n", task->id, prio) ;
        #endif
        task->prio_static = prio ;
        task->prio_dinamic = prio ;
        return ;
    }

    if (CurrentTask)
    {
        #ifdef DEBUG
            printf("task_setprio: priodidade estatica da tarefa (corrente) %d = %d", task->id, prio) ;
        #endif
        CurrentTask->prio_static = prio ;
        task->prio_dinamic = prio ;
        return ;
    }

    perror("task_setprio: task e CurrentTask são nulas!") ;
    exit (1) ;
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task)
{
    if (task)
    {
        return task->prio_static ;
    }

    if (CurrentTask)
    {
        return CurrentTask->prio_static ;
    }

    perror("task_getprio: task e CurrentTask são nulas!") ;
    exit (1) ;
}
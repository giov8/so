// PingPongOS - PingPong Operating System
// Giovani G. Marciniak GRR20182981, DINF UFPR
// Modificado em: 23 de Outubro  de 2021
// Contem as definições internas do sistema

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos_data.h"
#include "ppos.h"

#define STACKSIZE 64*1024	                                // tamanho de pilha das threads 
#define ALPHA_AGING -1                                      // define o task aging α = -1
#define QUANTUM_SIZE 20                                     // quantidade de ticks que cada tarefa recebe para ser executada

int TaskIDCounter, UserTasks ;                             // contador de IDs para criação de tarefas e quantidade de tarefas do usuário
task_t MainTask, DispatcherTask ;                          // Tarefa principal e Dispatcher
task_t *CurrentTask, *ReadyQueue ;                         // aponta para tarefa atual e para a tarefa que tem o Dispatcher
struct sigaction action ;                                  // define um tratador de sinal 
struct itimerval timer ;                                   // estrutura de inicialização do timer
unsigned int TicksRemaining, TicksTimer ;                  // ticks que restam para a tarefa atual executar e o total de ticks que aconteceram no programa
unsigned int ProcessorYieldTime ;                          // momento que o processador foi entregue a tarefa atual
int CoreFunctionAtivated;                                  // booleano que define se uma função do core está ativada, então não deve ser feira a preempção

// funções timer =================================================================

// trata o sinal recebido do timer
void handler (int signum)
{
    TicksTimer++ ;

    // não faz preempção se estiver rodando uma tarefa do sistema/core
    if(CoreFunctionAtivated)
        return ;
    
    if (TicksRemaining > 0)
    {
        TicksRemaining-- ;
    }
    else
    {
        task_yield() ;
    }
} 

// retorna o relógio atual (em milisegundos)
unsigned int systime ()
{
    return TicksTimer ;
}


// funções dispatcher/scheduler ==================================================

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
    CoreFunctionAtivated = 1 ;

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
    CoreFunctionAtivated = 1 ;
    
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

    UserTasks++ ;                                                               // Incrementa contator de tarefas de usuário ativas
    queue_append ((queue_t **) &ReadyQueue, (queue_t *) &MainTask) ;             // Adiciona a fila de prontas

    // =================================================

    // Criação do Dispatcher (tarefa 1)
    task_create (&DispatcherTask, dispatcher, "dispatcher") ;

    // Configuração de sinais e tempo:
    // registra a ação para o sinal de timer SIGALRM
    TicksTimer = 0 ;
    ProcessorYieldTime = 0 ;

    action.sa_handler = handler ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;             // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;                // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;          // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;             // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }


    #ifdef DEBUG
    printf ("PPOS: Ping Pong OS inicializado\n") ;
    #endif

    CoreFunctionAtivated = 0 ;
    task_yield () ;
}

// gerência de tarefas =========================================================

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
    CoreFunctionAtivated = 1 ;                            // para evitar que funções de nucleo sofram preempção

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
        UserTasks++ ;                                                       // Incrementa contator de tarefas de usuário ativas
        queue_append ((queue_t **) &ReadyQueue, (queue_t *) task) ;         // Adiciona a fila de prontas
    }

    task->id = (int) TaskIDCounter ;
    task->state = READY ;
    task->prio_static = 0 ;
    task->prio_dinamic = 0 ;
    task->processor_time = 0 ;
    task->activations = 0 ;
    task->start_time = systime() ;

    #ifdef DEBUG
        printf("PPOS: a tarefa %d foi criada pela tarefa %d\n", task->id, CurrentTask->id) ;
    #endif

    CoreFunctionAtivated = 0 ;
    return (task->id) ;
}

void task_exit (int exitCode)
{
    CoreFunctionAtivated = 1 ;                            // para evitar que funções de nucleo sofram preempção

    #ifdef DEBUG
        printf("PPOS: a tarefa %d será encerrada\n", task_id()) ;
    #endif

    if (!CurrentTask)
    {
        perror("Não é possível terminar tarefa vazia: ") ;
        exit(1) ;
    }
    
    CurrentTask->state = EXITED ;
    CurrentTask->exit_time = systime () ;
    UserTasks-- ;

    printf("Task %d exit: execution time %u ms, processor time %u ms, %u ativations\n",
        CurrentTask->id, CurrentTask->exit_time - CurrentTask->start_time, CurrentTask->processor_time, CurrentTask->activations) ;

    task_yield () ;
}

int task_switch (task_t *task)
{   
    CoreFunctionAtivated = 1 ;                             // para evitar que funções de nucleo sofram preempção

    if (!task)
    {
        perror (" Erro na criação troca de tarefas, task inválido: ") ;
        exit (1) ;
    }

    #ifdef DEBUG
        printf("PPOS: a tarefa %d será trocada pela tarefa %d\n", task_id(), task->id) ;
    #endif

    CurrentTask->processor_time += systime () - ProcessorYieldTime ;    // acrescenta o tempo de processador para a tarefa que estava executando
    ucontext_t *old_current_task = &CurrentTask->context ;              // usado para salvar o contexto que estava ocorrendo antes da troca

    CurrentTask = task ;                                                // é preciso a mudar variável global antes de mudar de contexto
    CurrentTask->activations++;                                         // aciona ativação da tarefa
    ProcessorYieldTime = systime ();                                    // guarda o momento que o processador foi entregue para a tarefa
    TicksRemaining = QUANTUM_SIZE ;                                     // a tarefa recebera um quantum para executar
    CoreFunctionAtivated = 0 ;
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
    CoreFunctionAtivated = 1;             // para evitar que funções de nucleo sofram preempção
    
    #ifdef DEBUG
        printf("PPOS: a tarefa %d passa o controle do processador\n", task_id()) ;
    #endif
    
    if (CurrentTask == &DispatcherTask) {   // Dispatcher agora será a ultima task, então ela que sairá da tarefa.
        exit(0) ;
    }

    else {  
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
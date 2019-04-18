/*
***
* NOME: LUCA
*
* COGNOME: MAURI
*
* MATRICOLA: 518136
*
* MAIL: LUCA.MAURI95@TISCALI.IT
*
* DICHIARO CHE IL PROGRAMMA è IN OGNI SUA PARTE, OPERA ORIGINALE DEL SUO AUTORE: LUCA MAURI
***
*/

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
 
#include "threadpool.h"
#include "hash.h"
 
//funzione per la creazione della coda di task
static coda_t *crea_coda_task();
 
//funzione per la coda
static int libera_coda_task(coda_t *coda);
 
//worker
static void* thread_worker(void* arg);

int threadpool_free(threadpool_t *tp);

threadpool_t *crea_threadpool(int numero_threads){
 
  //controllo iniziale dei parametri
  if(numero_threads <= 0){
    errno = EINVAL;
    return NULL;
  }
 
  int err;
  threadpool_t *tp;
 
  //alloco il threadpool
  tp = (threadpool_t *)malloc(sizeof(threadpool_t));
  //alloco la coda di task
  tp->coda_task = crea_coda_task(tp);
  //controllo esito creazione coda
  if(tp->coda_task == NULL){
      free(tp);
      return NULL;
  }

  tp->coda_back =  crea_coda(sizeof(long));
  if(tp->coda_back == NULL){
      libera_coda_task(tp->coda_task);
      free(tp);
      return NULL;
  }

  //inizializzo i parametri del threadpool
  tp->size_coda = 0;
  tp->shutdown = 0;
   
  //alloco i thread
  tp->threads = (pthread_t*)malloc(sizeof(pthread_t) *numero_threads);
  //controllo esito crezione
  if(tp->threads == NULL){
      libera_coda_task(tp->coda_task);
      libera_coda_task(tp->coda_back);
      free(tp);
      return NULL;
  }
   
  for(int i=0; i < numero_threads; i++){
      //creazione di un thread
    err = pthread_create(&tp->threads[i], NULL, thread_worker, (void*) tp);

    //controllo l'esito della creazione del thread
    if(err){
      remove_threadpool(tp);
      return NULL;    
    }
    //thread inserito con successo, aumento il numero di thread nel threadpool
    tp->size_coda++;
  }
 
  return tp;
 
}
 
int add_task(threadpool_t *tp, int (*function)(void*), void* arg){
  
  //controllo iniziale dei parametri
  if((tp == NULL) || (function == NULL)){
    errno = EINVAL;
    return -1;
  }
  //controllo che non sia in fase di shutdown
  if(tp->shutdown != 0){
    return -1;
  }
  void *taaa;
 
  //in questo caso non sto chiudendo, inizializzo il task
  task_t task;
  task.function = function;
  task.arg = arg;

  //aggiungo il task alla coda
  taaa = add_node(tp->coda_task, (void*)&task);
  //controllo l'esito
  if(taaa == NULL){
    return -1;
  }
  return 0;
}
 
int remove_threadpool(threadpool_t *tp){
  
  //controllo iniziale dei parametri
  if(tp == NULL){
    return -1;
  }
 
  //flag per l'esito delle operazioni
  int err = 0;
  
  //setto lo shutdown
  tp->shutdown = 1;
  tp->coda_task->shutdown = 1;
  //sveglio tutti i task della coda
  if((pthread_cond_broadcast(&(tp->coda_task->cond)) != 0)){
    return -1;
  }

  //effettuo la join dei threads
  for(int i=0; i<tp->size_coda; i++){
    if(pthread_join(tp->threads[i], NULL) != 0){
      err = -1;
    }
  }

  //a questo punto cancello completamente il threadpool
  if(!err){
    threadpool_free(tp);
  }
  return err;
  
}

//funzione di supporto alla rimozione del threadpool
int threadpool_free(threadpool_t *tp){
  
  if(tp == NULL) {
    return -1;
  }

  if(tp->threads) {
    free(tp->threads);
    remove_coda(tp->coda_task);
    remove_coda(tp->coda_back);
    tp->coda_task = NULL;
    tp->coda_back = NULL;        
  }
  free(tp);    
  return 0;
}
 
static coda_t *crea_coda_task(){
 
  //coda per i task 
  coda_t *coda; 
   
  //alloco la coda
  coda = crea_coda(sizeof(task_t));
 
  return coda;
 
}
 
static int libera_coda_task(coda_t *coda){

  //distruzione coda
  remove_coda(coda);
 
  return 0;
 
}
 
static void* thread_worker(void* arg){
 
  //inizializzo le variabili
  threadpool_t *tp = (threadpool_t *)arg;
  task_t *task = NULL;
  int res; 
  //fino a quando lo shutdown non viene chiamato
  while(!tp->shutdown){
    //prendo il primo elemento della coda dei task
    task = remove_node(tp->coda_task);
    //controllo che il task non sia null
    if(task == NULL){
      break;
    }

    res = task->function(task->arg);
    //addnode solo su successo
    if(res == 0){
      add_node(tp->coda_back, &task->arg); //ritorno del fd
    }
    free(task);
  }
  
  return NULL;
}
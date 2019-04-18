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

#if !defined(THREADPOOL_H)
#define THREADPOOL_H

#include <pthread.h>
#include "coda.h"
#include "hash.h"

typedef struct task{
  int (*function)(void*); //funzione da passare al thread
  void *arg; //argomento della funzione
}task_t;

typedef struct threadpool{
  coda_t *coda_task; //coda di task
  coda_t *coda_back; //coda ritorno fd 
  pthread_t *threads;  //thread del pool
  size_t size_coda; //dimensione della coda di threadpool
  int shutdown; //flag per segnalare se il pool si sta spegnendo
}threadpool_t;

//funzione per la creazione del threadpool
threadpool_t *crea_threadpool(int numero_threads);

//funzione per la rimozione del threadpool
int remove_threadpool(threadpool_t *tp);

//funzione per l'aggiunta di un nuovo thread al pool
int add_task(threadpool_t *tp, int (*function)(void*), void*arg);

#endif /* THREADPOOL_H */

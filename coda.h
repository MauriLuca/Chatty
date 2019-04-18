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


#if !defined(CODA_)
#define CODA_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

//struttura di un elemento della coda
typedef struct node{
  void *data; //puntatore al dato del nodo
  struct node *next; //puntatore all'elemento successivo
}node_t;

//struttura dati della coda
typedef struct coda{
  node_t *head; //puntatore alla testa
  node_t *tail; //puntatore alla coda
  size_t size; //dimensione della coda
  size_t nodesize; //dimensione di ogni nodo della coda
  pthread_mutex_t mutex;//mutex per le operazioni sulla coda
  pthread_cond_t cond;
  int shutdown;
}coda_t;

//funzione di creazione della coda
coda_t *crea_coda(size_t nodesize);

//funzione di aggiunta di un nuovo nodo alla coda
void* add_node(coda_t *coda, void *data);

//funzione di rimozione di un nodo dalla coda
void* remove_node(coda_t *coda);

void* remove_node_nowait(coda_t *coda);

//funzione di rimozione della coda
int remove_coda(coda_t *coda);


#endif /* CODA_H */

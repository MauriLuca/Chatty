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


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "coda.h"

//funzione di test che stampa la coda

void print_coda(coda_t *coda);

//funzione di creazione della coda
coda_t *crea_coda(size_t nodesize){

  //alloco la coda
  coda_t *coda;
  coda = (coda_t*)malloc(sizeof(coda_t));
  //controllo errori nella creazione
  if(coda == NULL){
    perror("Errore nella creazione della coda");
    return NULL;
  }

  //controllo errori nella mutex
  if(pthread_mutex_init(&coda->mutex, NULL) != 0){
    perror("Errore nell'inizializzazione della mutex");
    return NULL;
  }

  if(pthread_cond_init(&coda->cond, NULL) != 0){
    perror("Errore nell'inizializzazione della condition variable");
    return NULL;
  }

  coda->head = NULL;
  coda->tail = NULL;
  coda->size = 0;
  coda->nodesize = nodesize;
  coda->shutdown = 0;
  
  return coda;

}

//funzione di aggiunta di un nuovo nodo alla coda
void* add_node(coda_t *coda, void *data){
  
  //controllo iniziale dei paramentri
  if (coda == NULL || data == NULL){
    errno = EINVAL;
    return NULL;
  }

  node_t *nodo;
  nodo = (node_t*)malloc(sizeof(node_t));

  //controllo allocazione 
  if(nodo == NULL) {
    perror("Errore nella creazione del nodo");
	  return NULL;
  }
  nodo->data = malloc(coda->nodesize);
  nodo->next = NULL;
  memcpy(nodo->data, data, coda->nodesize);

  //controllo che la coda sia stata creata
  if(coda == NULL){
    perror("Errore coda non creata");
    free(nodo);
    return NULL;
  }

  pthread_mutex_lock(&coda->mutex);

  //caso inserisco il primo elemento
  if(coda->head == NULL && coda->tail == NULL){
	  coda->head = coda->tail = nodo;
  }
  //caso ho una coda che non è una coda
  else if(coda->head == NULL || coda->tail == NULL){
	  free(nodo);
    pthread_mutex_unlock(&coda->mutex);
    return NULL;

	}
  //caso inserisco in una lista con almeno un elemento
  else{
	  coda->tail->next = nodo;
	  coda->tail = nodo;
  }
  coda->size = (coda->size)+1;
  pthread_cond_signal(&coda->cond);
  pthread_mutex_unlock(&coda->mutex);

  return (void*)1;

}

//funzione di rimozione di un nodo dalla testa della coda
void* remove_node(coda_t *coda){
  void* data = NULL;
  node_t *old_head;
  
  if(coda == NULL){
    perror("Errore coda non creata");
    return NULL;
  }

  pthread_mutex_lock(&coda->mutex);
  while(coda->head == NULL && !coda->shutdown) pthread_cond_wait(&coda->cond,&coda->mutex);

  if(coda->shutdown){
    pthread_mutex_unlock(&coda->mutex);
    return NULL;
  } 

  old_head = coda->head;
  data = malloc(coda->nodesize);
  memcpy(data,old_head->data,coda->nodesize);

  if (coda->size == 1) {
    free(old_head->data);
    free(old_head);
    coda->head = coda->tail = NULL;
  }
  else {
    coda->head = coda->head->next;
    free(old_head->data);		
    free(old_head);
  }
  coda->size = (coda->size)-1;

  
  pthread_mutex_unlock(&coda->mutex);
  return data;

}

//funzione di rimozione di un nodo dalla testa della coda
//ritorna NULL quando la coda e' vuota senza aspettare
void* remove_node_nowait(coda_t *coda){
  void* data = NULL;
  node_t *old_head;
  if(coda == NULL){
    perror("Errore coda non creata");
    return NULL;
  }

  pthread_mutex_lock(&coda->mutex);
  while(coda->head == NULL && !coda->shutdown) {
    pthread_mutex_unlock(&coda->mutex);
    return NULL; //differenza da remove_node
  }

  if(coda->shutdown){
    pthread_mutex_unlock(&coda->mutex);
    return NULL;
  } 

  old_head = coda->head;
  data = malloc(coda->nodesize);
  memcpy(data,old_head->data,coda->nodesize);

  if (coda->size == 1) {
    free(old_head->data);
    free(old_head);
    coda->head = coda->tail = NULL;
  }
  else {
    coda->head = coda->head->next;
    free(old_head->data);		
    free(old_head);
  }
  coda->size = (coda->size)-1;

  
  pthread_mutex_unlock(&coda->mutex);
  return data;

}
		
//funzione di rimozione della coda
int remove_coda(coda_t *coda){

  node_t *curr;
  node_t *succ;

  //Controllo la coda
  if(coda == NULL){
    perror("Errore coda non creata");
    return -1;
  }
  //controllo errori nel distrugger la mutex
  if(pthread_mutex_destroy(&coda->mutex) != 0){
    perror("Errore distruzione mutex");
    return -1;
  }

  curr = coda->head;
  succ = NULL;
  while(curr != NULL){
	succ = curr->next;
	free(curr->data);
	free(curr);
	curr = succ;
	}
  free(coda);
  coda = NULL;

  return 0;

}

void print_coda(coda_t *coda){

  node_t *tmp;
  tmp = coda->head;
  while(tmp!=NULL){
	printf("%d\n" ,*(int*) tmp->data);
  tmp = tmp->next;
	}

}
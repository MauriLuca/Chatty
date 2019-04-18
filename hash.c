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
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "config.h"
#include "hash.h"
#include "configurazione.h"

 
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

/**
 * Una semplice hash di stringhe.
 *
 * @param[in] key -- la stringa su cui effettuare l'hash
 *
 * @returns l'indice hash
 *  
 */
static unsigned int hash_pjw(void* key)
{
    char *datum = (char *)key;
    unsigned int hash_value, i;
 
    if(!datum) return 0;
 
    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value);
}
 
static int string_compare(void* a, void* b) 
{
    return (strcmp( (char*)a, (char*)b ) == 0);
}
 
/**
 * Crea l'hash table
 *
 * @param nutenti -- dimensione iniziale dell'hash
 *
 * @returns l'hash table 
 */

hash_t *crea_hash(int nutenti, struct statistics *stat){
   
  hash_t *ht;
  int i = 0;
 
  ht = (hash_t*)malloc(sizeof(hash_t));
  if(!ht) return NULL;

  ht -> utenti = (entry_t**)malloc(nutenti * sizeof(utenti_t*));
  if(!ht->utenti) return NULL;
 
  ht->numero_buckets = nutenti;
  for(i=0;i<ht->numero_buckets;i++)
    ht->utenti[i] = NULL;
 
  ht->hash_function = hash_pjw;
  ht->hash_key_compare = string_compare;

  ht->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * 20);
	for(int i=0; i<20;i++){
		if(pthread_mutex_init(&(ht->mutex[i]), NULL) != 0){
			perror("errore inizializzazione mutex array");
			return NULL;
		}
  }
  ht->stat = stat;
  return ht;
 
}
 
/**
 * Cerca l'utente nell'hash table
 *
 * @param ht -- l'hash table dove cerco l'utente
 * @param nick -- l'utente da cercare
 *
 * @returns la struttura utente 
 *   Se l'utente non è presente nell'hash table, ritorno NULL.
 */
 
utenti_t* find_user(hash_t *ht, char *nick){
 
  entry_t* curr;
  unsigned int hash_val;
 
  if(!ht || !nick) return NULL;

  hash_val = (* ht->hash_function)(nick) % ht->numero_buckets;
 
  for (curr=ht->utenti[hash_val]; curr != NULL;){
        if ( ht->hash_key_compare(curr->nick, nick))
            return(curr->utente);
        curr=curr->next;
  }
 
  return NULL;
}
 
/**
 * Inserisce un utente nell'hash table
 *
 * @param ht -- l'hash table
 * @param nick -- il nickname del nuovo utente da inserire
 * @param data -- puntatore alla struttura utente
 *
 * @returns file descriptor dell'utente appena aggiunto. 
 *          ritorno -1 in caso di errore.
 */
 
int add_user(hash_t *ht, char *nick, int fd){

  entry_t *curr;
  unsigned int hash_val;

  //controllo parametri di input
  if(!ht || !nick || !fd) return -1;
  
  hash_val = (* ht->hash_function)(nick) % ht->numero_buckets;
 
  for (curr=ht->utenti[hash_val]; curr != NULL; curr=curr->next)
      if ( ht->hash_key_compare(curr->nick, nick))
          return -1; /* l'utente già esiste */
 
  /* se l'utente non viene trovato */
  curr = (entry_t*)malloc(sizeof(entry_t));
  utenti_t *utente = (utenti_t*)malloc(sizeof(utenti_t));
  message_t **messaggi = (message_t**)malloc(sizeof(message_t)*(configurazione.MaxHistMsgs));
  if(!curr || !utente || !messaggi) return -1;

  //configuro i parametri utente
  utente->fd = fd;
  for(int i=0;i<configurazione.MaxHistMsgs; i++)
    messaggi[i]=NULL;
  utente->messages = messaggi;
  utente->next = 0;
  
  //configuro i parametri entry
  strncpy(curr->nick, nick, MAX_NAME_LENGTH + 1);
  curr->utente = utente;
  curr->next = ht->utenti[hash_val]; //aggiungo in testa
  ht->utenti[hash_val] = curr;
 
  return fd;
}
  
/**
 * Connette l'utente 
 *
 * @param ht -- l'hash table dove è memorizzato l'utente
 * @param nick -- il nickname dell'utente da connettere
 * @param fd -- il file descriptor della connesione
 *
 * @returns fd se ha successo, -1 altrimenti
 */  
int connect_user(hash_t *ht, char *nick, int fd){

  utenti_t *utente;
  int res = -1;

  if(!ht || !nick || !fd) return -1;
  utente = find_user(ht, nick);
  //controllo che utente non sia null
  if (utente == NULL) return -1;

  if(utente->fd < 0){
      utente->fd = fd;
      res = fd;
  }
  return res;
}
/**
 * Disconnette l'utente dall'hash table
 *
 * @param ht -- l'hash table dove è memorizzato l'utente
 * @param nick -- il nickname dell'utente disconnettere
 *
 * @returns 0 se ha successo, -1 altrimenti
 */
int disconnect_user(hash_t *ht, char *nick){

  utenti_t *utente;
  unsigned int hash_val;

  if(!ht || !nick) return -1;
  
  hash_val = (* ht->hash_function)(nick) % ht->numero_buckets;
  int mutexvar = hash_val % 20;

  pthread_mutex_lock(&(ht->mutex[mutexvar]));

  if((utente = find_user(ht, nick)) == NULL){
      pthread_mutex_unlock(&(ht->mutex[mutexvar]));
      return -1;
  }

  utente->fd = -1;
  pthread_mutex_unlock(&(ht->mutex[mutexvar]));
  return 0;
}
/**
 * Rimuove l'utente dall'hash table
 *
 * @param ht -- l'hash table dove è memorizzato l'utente
 * @param nick -- il nickname dell'utente da rimuovere
 * @param free_key -- puntatore alla funzione che rimuove l'utente
 * @param free_data -- puntatore alla funzione che rimuove i dati utente
 *
 * @returns 0 se ha successo, -1 altrimenti
 */
int remove_user(hash_t *ht, char* nick, void (*free_key)(void*), void (*free_data)(void*)){   
  entry_t *curr, *prev;
  unsigned int hash_val;
 
  if(!ht || !nick) return -1;
  hash_val = (* ht->hash_function)(nick) % ht->numero_buckets;
 
  //cerco l'utente e lo rimuovo dividendo i casi di rimozione in testa e non
  prev = NULL;
  for (curr=ht->utenti[hash_val]; curr != NULL;)  {
      if ( ht->hash_key_compare(curr->nick, nick)) {
          if (prev == NULL) {
              ht->utenti[hash_val] = curr->next;
          } else {
              prev->next = curr->next;
          }
          if (*free_key && curr->nick) (*free_key)(curr->nick);
          if (*free_data && curr->utente) (*free_data)(curr->utente);

          //svuoto la history di messaggi dell'utente
          for(int i=0; i<configurazione.MaxHistMsgs ;i++)
            if(curr->utente->messages[i]!=NULL)
                free(curr->utente->messages[i]);
          free(curr->utente->messages);
          free(curr->utente);
          free(curr);
          return 0;
      }
      prev = curr;
      curr = curr->next;
  }
  return -1;
}
/**
 * Free hash table structures (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int remove_hash(hash_t *ht, void (*free_key)(void*), void (*free_data)(void*))
{
    entry_t *bucket, *curr, *next;
    int i;

    if(!ht) return -1;

    for (i=0; i<ht->numero_buckets; i++) {
        bucket = ht->utenti[i];
        for (curr=bucket; curr!=NULL; ) {
            next=curr->next;
            
                if(curr->utente != NULL){
                
                //libero la history di messaggi
                for(int i=0;i<configurazione.MaxHistMsgs;i++){
                    if(curr->utente->messages[i] != NULL){
                    free((*curr->utente->messages[i]).data.buf);
                    free(curr->utente->messages[i]);
                    }
                }
                free(curr->utente->messages);
                free(curr->utente);
            }
            free(curr);
            curr=next;
        }
    }
    if(ht->utenti != NULL) free(ht->utenti);

    for(int i=0;i<20;i++){
        if(pthread_mutex_destroy(&(ht->mutex[i])) != 0){
            printf("distruzione mutex non riuscita numero: %d\n", i);
            fflush(stdout);
            return -1; 
        }
    }
    if(ht->mutex) free(ht->mutex);

    if(ht) free(ht);

    return 0;
}

/**
 * Dump the hash table's contents to the given file pointer.
 *
 * @param stream -- the file to which the hash table should be dumped
 * @param ht -- the hash table to be dumped
 *
 * @returns 0 on success, -1 on failure.
 */

int icl_hash_dump(FILE* stream, hash_t * ht)
{
    entry_t *bucket, *curr;
    int i;

    if(!ht) return -1;

    for(i=0; i<ht->numero_buckets; i++) {
        bucket = ht->utenti[i];
        for(curr=bucket; curr!=NULL; ) {
            if(curr->nick)
                fprintf(stream, "icl_hash_dump: %s: %p\n", (char *)curr->nick, (void*)curr->utente);
            curr=curr->next;
        }
    }

    return 0;
}
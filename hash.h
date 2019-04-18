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

#if !defined(HASH_H_)
#define HASH_H_

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "message.h"
#include "config.h"
#include "configurazione.h"
#include "stats.h"

//struttura dati utente
typedef struct utente{
	int fd;//file descriptor
  	message_t **messages;//array di messaggi dell'utente
  	int next;//indice della prima posizione libera dell'array di messaggi
} utenti_t;

//entry della tabella hash
typedef struct entry{
	char nick[MAX_NAME_LENGTH + 1];//nickname dell'utente 
	utenti_t *utente;//puntatore al tipo utente
  	struct entry *next;//puntatore all'elemento successivo
} entry_t;

//struttura tabella hash
typedef struct hash{
  	int numero_buckets;//numero di buckets
  	entry_t **utenti; //struttura utenti
  	unsigned int (*hash_function)(void*);
  	int (*hash_key_compare)(void*, void*);
  	pthread_mutex_t *mutex;//array di mutex 
	struct statistics *stat;
} hash_t;

//funzione di creazione hash
hash_t *crea_hash(int nutenti, struct statistics *stat);

//funzione di ricerca utente, restituisce l'utente
utenti_t *find_user(hash_t *ht, char *nick);

//funzione di aggiunta utente, restituisce 0 se viene aggiunto
int add_user(hash_t *ht, char *nick, int fd);

//funzione che connette l'utente, restituisce il file descriptor della connessione, -1 errore
int connect_user(hash_t *ht, char *nick, int fd);

//funzione che disconnette l'utente, restituisce 0 se viene disconnesso, -1 errore
int disconnect_user(hash_t *ht, char *nick);

//funzione di rimozione utente, restituisce 0 se viene rimosso, -1 errore
int remove_user(hash_t *ht, char *nick, void (*free_key)(void*), void (*free_data)(void*));

//funzione di rimozione dell'hash table
int remove_hash(hash_t *, void (*)(void*), void (*)(void*)),
    icl_hash_dump(FILE *, hash_t *);

#define icl_hash_foreach(ht, tmpint, tmpent, kp, dp)    \
    for (tmpint=0;tmpint<ht->nbuckets; tmpint++)        \
        for (tmpent=ht->buckets[tmpint];                                \
             tmpent!=NULL&&((kp=tmpent->key)!=NULL)&&((dp=tmpent->data)!=NULL); \
             tmpent=tmpent->next)

#endif /* HASH_H */

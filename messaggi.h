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


#if !defined(MESSAGGI_H)
#define MESSAGGI_H

#include <stdlib.h>
#include "ops.h"
#include "hash.h"
#include "message.h"
#include "connections.h"


typedef enum {
    TEXT_TYPE,
    FILE_TYPE,
}tipo_messaggio_t;

//funzione di supporto di invio messaggio anche offline 
int invia(utenti_t *rcvr, message_t msg, hash_t *ht);

//funzione di invio messaggio
int inviaMessaggio(char *sndr, char *rcvr, char* msg, int size_msg, tipo_messaggio_t tipo, hash_t *utenti);

//funzione di invio messaggio broadcast
int inviaBroadcast(char *sndr, char * msg, int size_msg, hash_t *ht);

//funzione che restituisce il file di un utente
int getFile (char *sndr, char *file_id, hash_t *ht);

//funzione che controlla che il mittente sia registrato ed online
utenti_t *checkSndr(char *sndr, hash_t *ht);

//funzione che dice al client che l'invio è ok
int msgOk(int fd, char *buffer, int len);

//funzione che comunica al client che l'invio è non ok
int msgErr(int fd, op_t OP_FAIL, hash_t *ht);

//funzione che invia al client la lista degli utenti online
int OnlineUsers(int fd, hash_t *ht);

//funzione che invia ha history di messaggi dell'utente
int sendHistory(char *sndr, hash_t * ht);

#endif /* MESSAGGI_H */
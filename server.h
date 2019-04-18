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

#if !defined(SERVER_H_)
#define SERVER_H_

#include "threadpool.h"
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "stats.h"
#include "config.h"

//array delle connessioni
typedef struct connections{
    int fd;
    char nick[MAX_NAME_LENGTH + 1];
}connections_t;

connections_t *conn;

int worker(void *arg);

void listener(struct statistics *stat);




#endif /* SERVER_H_ */
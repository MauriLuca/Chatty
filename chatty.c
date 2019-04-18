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

/*
 * membox Progetto del corso di LSO 2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "configurazione.h"
#include "stats.h"
#include "server.h"

/* inserire gli altri include che servono */

/* struttura che memorizza le statistiche del server, struct statistics 
 * e' definita in stats.h.
 *
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };
extern pthread_mutex_t mutex_stat;

volatile sig_atomic_t toExit = 0;

static void gestoreTERM(){
	toExit = 1;
}

//funzione di handling con statistiche
static void gestoreSIGUSR1 (){

	FILE *file;
	file = fopen(configurazione.StatFileName, "a+");
	
	if(file == NULL){
		perror("errore nella gestione file statistiche segnale");
	}
	pthread_mutex_lock(&mutex_stat);
	printStats(file);
	pthread_mutex_unlock(&mutex_stat);
	fclose(file);
}



static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

int main(int argc, char *argv[]) {

	//parsing dei file di configurazione

	int parse = getopt(argc, argv, "f:");

	switch(parse){
		case 'f': fill(&configurazione, argv[2]);
		break;

		default: {
			usage(argv[0]);
			return -1;
		}
	}
	
	//signal
	sigset_t set;
	struct sigaction s;

	//creo la maschera per tutti i segnali
	sigfillset(&set);
	//maschero tutti i segnali intanto che non ho registrato tutte le gestioni
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	//inizializzo il segnale
	memset(&s, 0, sizeof(s));

	//assegno l'handler
	s.sa_handler = gestoreTERM;
	//devo gestire SIGINT, SIGTERM e SIGQUIT
	sigaction(SIGINT, &s, NULL);
	sigaction(SIGTERM, &s, NULL);
	sigaction(SIGQUIT, &s, NULL);

	//devo ignorare sigpipe per evitare che il server chiuda se un client riattacca
	s.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &s, NULL);

	//tolgo dal set tutti i segnali
	sigemptyset(&set);
	//devo aggiungere SIGUSR1
	sigaddset(&set, SIGUSR1);
	//setto la maschera solo a SIGUSR1
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	//gestisco SIGUSR1 con la terminazione + statistiche
	//se statFileName è vuoto o non specificato ignoro il segnale
	if(configurazione.StatFileName != NULL){
	s.sa_handler = gestoreSIGUSR1;
	sigaction(SIGUSR1, &s, NULL);
	}

	sigemptyset(&set);
	pthread_sigmask(SIG_SETMASK, &set, NULL);

	//avvio il listener	
	listener(&chattyStats);

	return 0;

}
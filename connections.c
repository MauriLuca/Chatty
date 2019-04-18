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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/un.h>
#include <string.h>
#include "connections.h"
#include "config.h"

//funzione di supporto alla read
static inline int nread(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
        if ((r=read((int)fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;   // gestione chiusura socket
        left    -= r;
        bufptr  += r;
    }
    return 1;
}

//funzione di supporto alla write
static inline int nwrite(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
        if ((r=write((int)fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;
        left    -= r;
        bufptr  += r;
    }
    return 1;
}

int openConnection(char* path, unsigned int ntimes, unsigned int secs){
 
  //controllo iniziale dei parametri
  if(path == NULL || ntimes > MAX_RETRIES || secs > MAX_SLEEPING){
  	errno = EINVAL;
  	return -1;
  }
  
  int fd; //file descriptor
  struct sockaddr_un sa; //socket
  int number_retry = 0; //variabile per il numero ti tentativi effettuati

  //assegnazione dell'indirizzo
  strncpy(sa.sun_path,path,UNIX_PATH_MAX);
  sa.sun_family = AF_UNIX;

  
  //inizializzo il file descriptor per comunicare col server
  fd = socket(AF_UNIX, SOCK_STREAM,0);

  //tentativo di connessione al server
  while(connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1 && number_retry <= ntimes){

  	sleep(secs);
  	number_retry++;
  }
  
  //numero ti tentativi di connessione superato
  if(number_retry > ntimes)
  	return -1;
  //connessione aperta, restituisco il file descriptor per la connesione	
  else return fd;

}

int readHeader(long connfd, message_hdr_t *hdr){
  
  //controllo iniziale dei parametri
  if(connfd < 0 || hdr == NULL){
  	errno = EINVAL;
  	return -1;
  }

  int res;

  //leggo l'header del messaggio
  res = nread(connfd,hdr,sizeof(message_hdr_t));

  return res;

}

int readData(long fd, message_data_t *data){

  //controllo iniziale dei parametri
  if(fd < 0 || data == NULL){
	errno = EINVAL;
	return -1;
  }

  int res;
  res = nread(fd, &(data->hdr), sizeof(message_data_hdr_t));
  if(res <= 0){
    return res;
  }
  size_t len = (data->hdr.len);//lunghezza del buffer
  data->buf = NULL;
  if(len!=0){
    char *buffer = malloc(sizeof(char)*len);
    memset(buffer,0,sizeof(char)*len);
    data->buf = buffer;
    
    //leggo il body del messaggio
    res = nread(fd,data->buf,sizeof(char)*len);
  }
  return res;
}

int readMsg(long fd, message_t *msg){
  
  //controllo iniziale dei parametri
  if(fd < 0 || msg == NULL){
	errno = EINVAL;
	return -1;
  }

  int res;
  
  //leggo l'header 
  res = readHeader(fd, &(msg->hdr));
  
  if (res<=0){
    return res;
  }
  //leggo il body del messaggio
  res = readData(fd, &(msg->data));

  return res;

}

int sendRequest(long fd, message_t *msg){

  //controllo iniziale dei parametri
  if(fd < 0 || msg == NULL){
	errno = EINVAL;
	return -1;
  }

  int res;
  res = sendHeader(fd, &(msg->hdr));
  if (res<=0){
    return res;
  } 
  res = sendData(fd, &(msg->data));
  if(res <=0){
    return res;
  }
  return res;
  
}

int sendData(long fd, message_data_t *msg){

  //controllo iniziale dei parametri
  if(fd < 0 || msg == NULL){
	errno = EINVAL;
	return -1;
  }

  int res = 0;

  res = nwrite(fd, &(msg->hdr), sizeof(message_data_hdr_t));

  if(res <= 0){
    return res;
  }
  
  res = nwrite(fd, msg->buf, (msg->hdr.len) * sizeof(char));
  
  return res;

}

int sendHeader(long fd, message_hdr_t *hdr){

  //controllo iniziale dei parametri
  if(fd < 0 || hdr == NULL){
    errno = EINVAL;
    return -1;
  }

  int res;

  res = nwrite(fd, hdr, sizeof(message_hdr_t));

  return res;
  
}
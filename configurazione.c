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
#include "configurazione.h"

t_param configurazione = { NULL, 0,0,0,0,0,NULL,NULL };

void fill(t_param *list, char *pas){
  FILE *fd;
  char *s;
  char *r;
  char del[]="= \n";
  int count=0;
  
  /*alloco lo spazio per s*/
  s=malloc(sizeof(char)*1000);
  /*apro il file*/
  fd=fopen(pas, "r");
  if(fd==NULL){
    perror("Errore in lettura file");
    exit(1);
  }
  /*leggo e stampo la riga che matcha*/ 
  while(1){
    count = 0;
    r = fgets(s,1000,fd);
    if(r == NULL) break;
    /*se la stringa inizia con un # non è la stringa che cerco*/
    if(s[0]=='#'){}
    /*ho trovato la stringa*/
    else if((r = (strstr(s, "UnixPath")))){
      /*divido la stringa in 3 parti: prima dell'= dopo l'= e dopo il valore che cerco*/
      r=strtok(s, del);
      /*scorro i tre sottosegmenti ed esco dal ciclo quando ho trovato quello che mi serve*/
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->UnixPath = (char*)malloc(sizeof(char)*strlen(r)+1);
      memset(list->UnixPath,0,sizeof(char)*strlen(r)+1);
      list->UnixPath = strncpy(list->UnixPath,r,strlen(r));
    }
    /*caso Threadsinpool*/
    else if((r= (strstr(s, "ThreadsInPool")))){
      r=strtok(s, del);
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->ThreadsInPool=atoi(r);
    }
    /*caso MaxConnections*/
    else if((r= (strstr(s, "MaxConnections")))){
      r=strtok(s, del);
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->MaxConnections=atoi(r);
    }
    /*caso MaxMsgsSize*/
    else if((r = (strstr(s, "MaxMsgSize")))){
      r=strtok(s, del);
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->MaxMsgSize=atoi(r);
    }
    /*caso MaxFileSize*/
    else if((r = (strstr(s, "MaxFileSize")))){
      r=strtok(s, del);
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->MaxFileSize=atoi(r);
    }
    /*caso MaxHistMsgs*/
    else if((r = (strstr(s, "MaxHistMsgs")))){
      r=strtok(s, del);
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->MaxHistMsgs = atoi(r);
    }
    /*caso DirName*/
    else if((r = (strstr(s, "DirName")))){
      r=strtok(s, del);
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->DirName = (char*)malloc(sizeof(char)*strlen(r)+1);
      memset(list->DirName,0,sizeof(char)*strlen(r)+1);
      list->DirName = strncpy(list->DirName,r,strlen(r));
    }
    /*caso StatFileName*/
    else if((r = (strstr(s, "StatFileName")))){
      r=strtok(s, del);
      while(r!=NULL){
	count++;
	if (count!=2){}
	else break;
	r=strtok(NULL, del);
      }
      list->StatFileName = (char*)malloc(sizeof(char)*strlen(r)+1);
      memset(list->StatFileName,0,sizeof(char)*strlen(r)+1);
      list->StatFileName = strncpy(list->StatFileName,r,strlen(r));    }
  }
  /*chiudo il file*/
  fclose(fd);
  /*dealloco la memoria*/
  free(s);
}
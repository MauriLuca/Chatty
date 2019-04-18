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


#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hash.h"
#include "message.h"
#include "connections.h"
#include "messaggi.h"
#include "configurazione.h"
#include "stats.h"
#include "server.h"

extern pthread_mutex_t mutex_stat;
//mutex per accedere all'array delle connessioni
extern pthread_mutex_t mutex_connections;

extern pthread_mutex_t mutex_writes;

//funzione che controlla che il mittente sia registrato ed online
utenti_t *checkSndr(char *sndr, hash_t *ht){
    
    utenti_t *sender;
    int mutex_idx_sender = ((*ht->hash_function)(sndr) % ht ->numero_buckets) % 20;

    
    //controllo inziale dei parametri
    if(sndr == NULL || ht == NULL){
        perror("Errore nel parametri iniziali");
        return NULL;
    }

    pthread_mutex_lock(&(ht->mutex[mutex_idx_sender]));
    //cerco l'utente nell'hash table
    sender = find_user(ht, sndr);

    //caso in cui l'utente non venga trovato
    if(sender == NULL){
        pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));
        return NULL;
    } 
        
    //caso in cui l'utente non sia online
    if(sender->fd == 0){
        pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));
        printf("Utente non online o registrato\n");
        return NULL;
    }
    
    pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));
    return sender;
}

//funzione di supporto di invio messaggio anche offline 
int invia(utenti_t *rcvr, message_t msg, hash_t *ht){
    
    //controllo iniziale dei parametri
    if(rcvr == NULL || ht == NULL){
        errno = EINVAL;
        return -1;
    }
    
    int res;
    //l'utente è online
    if(rcvr->fd > 0){
        
        pthread_mutex_lock(&mutex_writes);
        //invio il messaggio direttamente
        res = sendRequest(rcvr->fd, &msg);
        pthread_mutex_unlock(&mutex_writes);
        //controllo eventtuali errori
         if(res >0 )
         {
            if(msg.hdr.op == TXT_MESSAGE){
                pthread_mutex_lock(&mutex_stat);
                ht->stat->ndelivered++;
                pthread_mutex_unlock(&mutex_stat);
            }
            return 0;
         }
        //caso messaggio testuale
    }
    //l'utente non è online
  
    
    message_t* messaggio = malloc(sizeof(message_t));
    *messaggio = msg;
    (*messaggio).data.buf = NULL;
    (*messaggio).data.buf = strdup(msg.data.buf);

    rcvr->messages[rcvr->next] = messaggio;
    rcvr->next = (rcvr->next +1) % configurazione.MaxHistMsgs;
    //se uguale a base free del msg e del data->buf, controlla che buf != NULL


    //aumento le statistiche per i messaggi non consegnati se il messaggio è di testo

    if(msg.hdr.op == TXT_MESSAGE){
        pthread_mutex_lock(&mutex_stat);
        ht->stat->nnotdelivered++;
        //rilascio la lock
        pthread_mutex_unlock(&mutex_stat);
    }
    


    return 0;
}

//carico il file
int postFile(int fd, char *file_id, hash_t * ht){
    int res;

    message_data_t file_dati;
    FILE *file;
    char path[UNIX_PATH_MAX];

    //prendo il file
    res = readData(fd, &file_dati);
    //controllo errori
    if(res == -1){
        return -1;
    }
    //controllo dimensione file
    if(file_dati.hdr.len > configurazione.MaxFileSize * 1024){
        //faccio la free del messaggio e restituisco 1 così da mandare msg_toolong
        free(file_dati.buf);
        return 1;
    }

    char*file_id_copy = malloc(strlen(file_id)+1);
    if(file_id_copy == NULL){
        printf("malloc fallita\n");
        return -1;
    }
    memset(file_id_copy, 0, strlen(file_id)+1);
    strcpy(file_id_copy, file_id);

    //trovo il nome del file
    memset(path, 0, UNIX_PATH_MAX);
    strcat(path, configurazione.DirName);
    strcat(path, "/");
    char* tok = strtok(file_id_copy, "/");
    char* ptr;
    while(tok != NULL){
        ptr= tok;
        tok = strtok(NULL, "/");
    }
    strcpy(file_id, ptr);
    strncat(path, file_id, strlen(file_id) + strlen(path));

    free(file_id_copy);

    //creo
    file = fopen(path, "w");

    if(file == NULL){
        free(file_dati.buf);
        return -1;
    }

    res = fwrite(file_dati.buf, 1, file_dati.hdr.len, file);

    free(file_dati.buf);

    if(res < 0){
        fclose(file);
        return -1;
    }

    //aggiorno le statistiche 

    //aggiunta la lock
    pthread_mutex_lock(&mutex_stat);
    ht->stat->nfilenotdelivered++;
    pthread_mutex_unlock(&mutex_stat);

    fclose(file);

    return 0;
}

//funzione che restituisce il file di un utente
int getFile(char *sndr, char *file_id, hash_t *ht){

    //controllo iniziale dei parametri
    if(sndr == NULL || file_id == NULL || ht == NULL){
        errno = EINVAL;
        return -1;
    }

    int res;
    FILE *file;
    char path[UNIX_PATH_MAX];
    //controllo se l' utente sender è online
    utenti_t *sender = checkSndr(sndr, ht);

    //errori nel sender non trovato o non online
    if(sender == NULL){
        res = msgErr(sender->fd, OP_FAIL, ht);
    }
    //cerco il file
    else{
        memset(path, 0, UNIX_PATH_MAX);
        strcat(path, configurazione.DirName);
        strcat(path, "/");

        char* tok = strtok(file_id, "/");
        
        while(tok != NULL){
            file_id= tok;
            tok = strtok(NULL, "/");
        }
        strncat(path, file_id, strlen(file_id) + strlen(path));

        file = fopen(path, "r");
        //file non esistente
        if(file == NULL){
            res = msgErr(sender->fd, OP_NO_SUCH_FILE, ht);
            return -1;
        }
        
        //giusto una prova
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind (file);
        char* file22 = malloc (sizeof(char)*(size));
        if(!file22){
            printf("malloc fallita\n");
            return -1;
        }
        memset(file22,0,sizeof(char)*(size));
        fread(file22,1,(sizeof(char)*(size)),file);

        res = msgOk(sender->fd,file22,(int)size);

        free(file22);

        pthread_mutex_lock(&mutex_stat);
        ht->stat->nfiledelivered++;
        ht->stat->nfilenotdelivered--;
        pthread_mutex_unlock(&mutex_stat);

    }

    return res;
}

//funzione di invio messaggio
int inviaMessaggio(char *sndr, char *rcvr, char* msg, int size_msg, tipo_messaggio_t tipo, hash_t *ht){

    //controllo iniziale dei parametri
    if(sndr == NULL || rcvr == NULL || msg == NULL || size_msg < 0 || ht == NULL){
        errno = EINVAL;
        return -1;
    }

    int res;
    int mutex_idx_receiver = (( *ht->hash_function)(rcvr) % ht ->numero_buckets) % 20;
    
    //controllo se l' utente sender è online
    utenti_t *sender = checkSndr(sndr, ht);
    //errore nel sender
    if(sender == NULL){
        //incremento numero di errori nelle statistiche
        perror("sender == NULL");
        res = msgErr(sender->fd, OP_FAIL, ht);
        return -1;
    }
    
    //controllo la dimensione del messaggio
    else if(tipo == TEXT_TYPE && size_msg > configurazione.MaxMsgSize){
        printf("messaggio troppo lungo\n");
        res = msgErr(sender->fd, OP_MSG_TOOLONG, ht);
        return -1;
    }

    //caso in cui il sender sia ok e il messaggio sia ok
    else{
        //prendo la mutex per l'utente
        pthread_mutex_lock(&(ht->mutex[mutex_idx_receiver]));
        utenti_t *receiver = find_user(ht, rcvr);

        //controllo errori nel receiver
        if(receiver == NULL){
            printf("receiver == NULL\n");
            res = msgErr(sender->fd, OP_FAIL, ht);
            pthread_mutex_unlock(&(ht->mutex[mutex_idx_receiver]));
            return -1;
        }

        //tutto ok
        else{
            if(tipo == FILE_TYPE){
                res = postFile(sender->fd, msg, ht);
                if(res == 1){
                    printf("messaggio troppo grande");
                    msgErr(sender->fd, OP_MSG_TOOLONG, ht);
                    pthread_mutex_unlock(&(ht->mutex[mutex_idx_receiver]));
                    return -1;
                }
            }

            message_t messaggio;
            
            if(tipo == TEXT_TYPE){
                setHeader(&messaggio.hdr, TXT_MESSAGE, sndr);
            }
            else{
                setHeader(&messaggio.hdr, FILE_MESSAGE, sndr);
            }
            setData(&messaggio.data, "", msg, size_msg);
            res = invia(receiver, messaggio, ht);
            pthread_mutex_unlock(&(ht->mutex[mutex_idx_receiver]));

            if(res == -1){
                printf("errore nell'invio messaggio");
                return -1;
            }
            //caso buon fine mando l'ok
            res = msgOk(sender->fd,"", 0);

        }

        
    }

    if(res == -1){
            printf("invio messaggio non ok");
            return -1;
        }

    return 0;
}

//funzione di invio messaggio broadcast
int inviaBroadcast(char *sndr, char * msg, int size_msg, hash_t *ht){

    //controllo parametri iniziali
    if(sndr == NULL || msg == NULL || size_msg < 0 || ht == NULL){
        errno = EINVAL;
        return -1;
    }

    int res;
    int mutex_idx_buckets;
    entry_t *entry;

    //controllo se l' utente sender è online
    utenti_t *sender = checkSndr(sndr, ht);

    //errori nel sender non trovato o non online
    if(sender == NULL){
        res = msgErr(sender->fd, OP_FAIL, ht);
    }
    //dimensione messaggio troppo lunga
    else if(size_msg > configurazione.MaxMsgSize){
        res = msgErr(sender->fd, OP_MSG_TOOLONG, ht);
    }
    else{
        
        //preparo il messaggio
        message_t messaggio;

        setHeader(&messaggio.hdr, TXT_MESSAGE, sndr);
        setData(&messaggio.data, "",msg, size_msg);

        //scorro tutti gli utenti registrati
        for(int i = 0; i <ht->numero_buckets; i++){
            mutex_idx_buckets = i % 20;
            fflush(stdout);
            pthread_mutex_lock(&(ht->mutex[mutex_idx_buckets]));
            
            for(entry = ht->utenti[i]; entry != NULL; entry = entry->next){
                if(entry->utente != NULL){
                    res = invia(entry->utente,messaggio, ht);
                }
            }
            pthread_mutex_unlock(&(ht->mutex[mutex_idx_buckets]));
            fflush(stdout);
        }
        res = msgOk(sender->fd, "", 0);
    }

    if(res == -1){
            printf("errore messaggio riposta sender");
            return -1;
        }

    return 0;
}



//funzione che dice al client che l'invio è ok
int msgOk(int fd, char *buffer, int len){
    
    message_t risposta;
    int res;
    //setto l'header
    setHeader(&risposta.hdr, OP_OK, "");
    //mando l'header nella lock
    pthread_mutex_lock(&mutex_writes);
    res = sendHeader(fd,&risposta.hdr);

    //controllo di non aver errori nell'invio del messaggio
    if(res < 0) {
        pthread_mutex_unlock(&mutex_writes);
        return -1;
    }
    //se ho qualcosa nel buffer allora lo invio come data
    if(len != 0 || strcmp(buffer,"") != 0)
    {
        setData(&risposta.data,"",buffer,len);
        //mando il data
        res = sendData(fd,&risposta.data);
        //errore nel sendData
        if(res == -1){
            printf("errore nel send data");
            pthread_mutex_unlock(&mutex_writes);
            return -1;
        }
    }
    //lascio la lock
    pthread_mutex_unlock(&mutex_writes);
    return res;
}

//funzione che comunica al client che l'invio è non ok
int msgErr(int fd, op_t OP_FAIL, hash_t *ht){
    
    message_t risposta;
    int res;

    //preparo l'header
    setHeader(&risposta.hdr, OP_FAIL, "");
    pthread_mutex_lock(&mutex_writes);
    //invio l'header
    res = sendHeader(fd, &risposta.hdr);
    pthread_mutex_unlock(&mutex_writes);
    if(res == -1) return -1;

    //aumento il numero degli errori
    pthread_mutex_lock(&mutex_stat);

    ht->stat->nerrors++;

    pthread_mutex_unlock(&mutex_stat);

    return 0;

}

//funzione che invia al client la lista degli utenti online
int OnlineUsers(int fd, hash_t *ht){
    //controllo iniziale dei parametri
    if(fd <= 0 || ht == NULL){
        errno = EINVAL;
        return -1;
    }
    pthread_mutex_lock(&mutex_stat);
    int tot_users = ht->stat->nonline;
    pthread_mutex_unlock(&mutex_stat);
    int len = MAX_NAME_LENGTH + 1;
    char utenti_online[len * tot_users];

    int res;
    int tok = 0;

    char compare[len];
    memset(compare, 0, len);

    pthread_mutex_lock(&mutex_connections);
    for(int i = 0; i<configurazione.MaxConnections; i++){
        if(strcmp(conn[i].nick,compare)!= 0){
            strncpy(&utenti_online[tok],conn[i].nick, len);
            tok = tok + len;
        }
    }

    pthread_mutex_unlock(&mutex_connections);
    res = msgOk(fd, utenti_online, len * tot_users);
    return res;

}

//funzione che invia ha history di messaggi dell'utente
int sendHistory(char *sndr, hash_t * ht){

    int res;
    int num_history;
    //controllo iniziale dei parametri
    if(sndr == NULL || ht == NULL){
        errno = EINVAL;
        return -1;
    }
    
    utenti_t *sender = checkSndr(sndr, ht);

    if(sender == NULL){
        res = msgErr(sender->fd, OP_FAIL, ht);
        return -1;
    }

    //recupero i messaggi di testo
    else{
        
        int mutex_idx_sender = (( *ht->hash_function)(sndr) % ht->numero_buckets) % 20;
        pthread_mutex_lock(&(ht->mutex[mutex_idx_sender]));

        //se è == NULL allora mi fermo, se next è == NULL allora ho l'array pieno
        if(sender->messages[sender->next] == NULL){
            num_history = sender->next;
        }
        else{
            num_history = configurazione.MaxHistMsgs;
        }

        //invio il numero di messaggi da inviare
        res = msgOk(sender->fd, (char*)&num_history, sizeof(num_history));

        if(res == -1){
            pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));
            return -1;
        }
        
        int i = 0;
        while(i<num_history && sender->messages[i] != NULL){
            //devo mandare tutti i messaggi
            //mando la richiesta
            pthread_mutex_lock(&mutex_writes);
            sendRequest(sender->fd, sender->messages[i]);
            pthread_mutex_unlock(&mutex_writes);
            //aumento le statistiche per i messaggi di testo mandati
            pthread_mutex_lock(&mutex_stat);
            ht->stat->ndelivered++;
            ht->stat->nnotdelivered--;
            pthread_mutex_unlock(&mutex_stat);

            //libero la memoria
            free((*sender->messages[i]).data.buf);
            free(sender->messages[i]);
            sender->messages[i] = NULL;
            i++;

        }
        //resetto il puntatore della prima posizione libera nell'array di messaggi
        sender->next = 0;
        pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));
    }

    return 0;
}
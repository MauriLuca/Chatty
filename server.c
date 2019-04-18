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

#include "server.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include "configurazione.h"
#include "connections.h"
#include "threadpool.h"
#include "hash.h"
#include "message.h"
#include "messaggi.h"
#include <signal.h>

//mutex per accedere alla struttura delle statistiche
pthread_mutex_t mutex_stat;
//mutex per accedere all'array delle connessioni
pthread_mutex_t mutex_connections;
//mutex per non le write multiple
pthread_mutex_t mutex_writes;
//hash table
hash_t *ht;
//threadPool
threadpool_t *tp;

extern volatile sig_atomic_t toExit;

void termina();

int worker(void *arg){
    long fd = (long)arg;
    int err;
    int res = 0;
    int etc = 0;//variabile per disconnettere e non aggiungere alla coda di ritorno 
    message_t message;
    utenti_t *sender;
    int disc = 0; // variabile da settare per non aggiungere il fd alla coda di ritorno
    //legge pacchetto
    err = readMsg(fd, &message);
    if(err == -1){
        //perror("errore nella lettura");
        return -1;
    }
    else if(err == 0){
        printf("client disconnesso\n");
        //disconnetto il client
        pthread_mutex_lock(&mutex_connections);
        int i = 0;
        while(i < configurazione.MaxConnections && fd != conn[i].fd){
            i++;
        }
        if(i < configurazione.MaxConnections) {
            if(disconnect_user(ht, conn[i].nick) == 0){
                pthread_mutex_lock(&mutex_stat);
                ht->stat->nonline--;
                pthread_mutex_unlock(&mutex_stat);
            }
            conn[i].fd = -1;
            memset(conn[i].nick, 0, MAX_NAME_LENGTH + 1);
            pthread_mutex_unlock(&mutex_connections);
        }
        close(fd);
        return -1;
    }
    else{
        //indici per le mutex dell'hash table
        int mutex_idx_sender = (( *ht->hash_function)(message.hdr.sender) % ht->numero_buckets) % 20;
        //devo gestire la richiesta
        switch(message.hdr.op){

        case REGISTER_OP: {
            pthread_mutex_lock(&(ht->mutex[mutex_idx_sender]));
            res = add_user(ht, message.hdr.sender, fd);
            pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));
            //utente già registrato
            if(res == -1){
                res = msgErr(fd, OP_NICK_ALREADY, ht);
                close(fd);
                return -1;
            }

            else{
                pthread_mutex_lock(&mutex_connections);
                int i = 0;
                //ciclo per trovare una posizione dell'array dove poter inserire la connessione
                while(i< configurazione.MaxConnections && conn[i].fd != -1){
                    i++;
                }
                if(i< configurazione.MaxConnections){
                    conn[i].fd = fd;
                    strncpy(conn[i].nick, message.hdr.sender, MAX_NAME_LENGTH + 1);
                    pthread_mutex_unlock(&mutex_connections);
                    pthread_mutex_lock(&mutex_stat);
                    ht->stat->nusers++;
                    ht->stat->nonline++;
                    pthread_mutex_unlock(&mutex_stat);
                    res = OnlineUsers(fd, ht);
                }
                else{
                    res = msgErr(fd, OP_FAIL, ht);
                    close(fd);
                    return -1;
                }
            }

        }break;

        case CONNECT_OP: {
            pthread_mutex_lock(&(ht->mutex[mutex_idx_sender]));
            res = connect_user(ht, message.hdr.sender, fd);
            pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));
            //utente non trovato
            if(res == -1){
                //printf("utente non trovato");
                //fflush(stdout);
                res = msgErr(fd, OP_NICK_UNKNOWN, ht);
                etc = 1; 
                break;
            }
            else{
                pthread_mutex_lock(&mutex_stat);
                ht->stat->nonline++;
                pthread_mutex_unlock(&mutex_stat);
                pthread_mutex_lock(&mutex_connections);
                //cerco una posizione libera
                int i = 0;
                while(i< configurazione.MaxConnections && conn[i].fd != -1){
                    i++;
                }
                if(i< configurazione.MaxConnections){
                    conn[i].fd = fd;
                    strncpy(conn[i].nick, message.hdr.sender, MAX_NAME_LENGTH + 1);
                    pthread_mutex_unlock(&mutex_connections);
                    res = OnlineUsers(fd, ht);
                }else {
                    res = msgErr(fd, OP_FAIL, ht);
                    close(fd);
                    return -1;
                }
            }
        }break;

        case POSTTXT_OP: {
            res = inviaMessaggio(message.hdr.sender, message.data.hdr.receiver, message.data.buf, message.data.hdr.len, TEXT_TYPE, ht);
            if(res == -1){
                printf("errore POSTTXT_OP\n");
                etc = 1; 
                break;
            }
        }break;

        case POSTTXTALL_OP: {
            res = inviaBroadcast(message.hdr.sender, message.data.buf, message.data.hdr.len, ht);
            if(res == -1){
                printf("errore POSTTXTALL_OP\n");
                etc = 1; 
                break;
            }
        }break;

        case POSTFILE_OP: {
            res = inviaMessaggio(message.hdr.sender, message.data.hdr.receiver, message.data.buf, message.data.hdr.len, FILE_TYPE, ht);
            if(res == -1){
                printf("POSTFILE_OP_ERR\n");
                fflush(stdout);
                etc = 1; 
                break;
            }
        }break;

        case GETFILE_OP: {
            res = getFile(message.hdr.sender, message.data.buf, ht);
            if(res == -1){
                printf("GETFILE_OP_ERR\n");
                fflush(stdout);
                etc = 1; break;
            }
        }break;

        case GETPREVMSGS_OP: {
            res = sendHistory(message.hdr.sender, ht);

            if(res == -1){
                printf("errore GETPREVMSG_OP\n");
                etc = 1; 
                break;
            }
            
        }break;

        case USRLIST_OP: {
            sender = checkSndr(message.hdr.sender, ht);
            if(sender == NULL){
                res = msgErr(fd, OP_FAIL, ht);
                etc = 1; 
                break;
            }

            else{
                res = OnlineUsers(fd, ht);
            }

            if(res == -1){
                printf("errore USRLIST_OP\n");
                etc = 1; 
                break;
            }
        }break;

        case UNREGISTER_OP: {
            pthread_mutex_lock(&mutex_connections);
            //prima se lo trovo disconnetto l'utente dall'array di connessioni
            int i = 0;
            int trovato = 0;
            while(i < configurazione.MaxConnections && trovato == 0){
                if(conn[i].fd == fd){
                    trovato = 1;
                }
                else{
                i++;
                }
            }
            if(trovato){
                //setto la variabile per non aggiunger in coda di ritorno
                disc = 1;
                //chiudo il fd e lo setto a -1
                msgOk(fd, "", 0);
                close(fd);
                conn[i].fd = -1;
                memset(conn[i].nick, 0, MAX_NAME_LENGTH +1);
                
            }
            pthread_mutex_unlock(&mutex_connections);
            //rimuovo l'utente
            pthread_mutex_lock(&(ht->mutex[mutex_idx_sender]));
            res = remove_user(ht, message.hdr.sender, NULL, NULL);
            pthread_mutex_unlock(&(ht->mutex[mutex_idx_sender]));

            //utente non registrato
            if(res == -1){
                printf("utente non registrato\n");
                msgErr(fd, OP_NICK_UNKNOWN, ht);
                close(fd);
                return -1;
            }
            //utente deregistrato diminuisco le statistiche
            else if(res == 0){
                pthread_mutex_lock(&mutex_stat);
                ht->stat->nonline--;
                ht->stat->nusers--;
                pthread_mutex_unlock(&mutex_stat);
            }

        }break;

        case DISCONNECT_OP: {
            pthread_mutex_lock(&mutex_connections);
            //prima se lo trovo disconnetto l'utente dall'array di connessioni
            int i = 0;
            int trovato = 0;
            while(i < configurazione.MaxConnections && fd != conn[i].fd){
                if(conn[i].fd == fd){
                    trovato = 1;
                }
                else{
                i++;
                }
            }
            if(trovato){
                //setto la variabile per non aggiunger in coda di ritorno
                disc = 1;
                //chiudo il fd e lo setto a -1
                msgOk(fd, "", 0);
                close(fd);
                conn[i].fd = -1;
                memset(conn[i].nick, 0, MAX_NAME_LENGTH +1);
                //disconnetto l'utente
                res = disconnect_user(ht,message.hdr.sender);
            }
            pthread_mutex_unlock(&mutex_connections);

            //utente non registrato
            if(res == -1){
                printf("utente non registrato\n");
                msgErr(fd, OP_NICK_UNKNOWN, ht);
                close(fd);
                return -1;
            }
            else{   
                pthread_mutex_lock(&mutex_stat);
                ht->stat->nonline--;
                pthread_mutex_unlock(&mutex_stat);
            }
        }break;

        default: break;

        }

    }
    if(message.data.buf != NULL)
        free(message.data.buf);
    //se ho disconnesso o deregistrato ritorno -1 in modo da non aggiungere il fd alla coda di ritorno
    if(disc){
        return -1;
    }
    //caso in cui l'utente non venga trovato
    if(etc == 1){
        //disconnetto il client
        pthread_mutex_lock(&mutex_connections);
        int i = 0;
        while(i < configurazione.MaxConnections && fd != conn[i].fd){
            i++;
        }
        if(i < configurazione.MaxConnections) {
            res = disconnect_user(ht, conn[i].nick);
            if(res == 0){
                pthread_mutex_lock(&mutex_stat);
                ht->stat->nonline--;
                pthread_mutex_unlock(&mutex_stat);
            }
            conn[i].fd = -1;
            memset(conn[i].nick, 0, MAX_NAME_LENGTH + 1);
        }
        pthread_mutex_unlock(&mutex_connections);
        close(fd);
        return -1;
    }
        
    return 0;
}

void listener(struct statistics *stat){ 
    printf("listener partito... %s\n", configurazione.UnixPath);
    fflush(stdout);
    if(unlink(configurazione.UnixPath)<0)
        errno = 0;
    //creo socket
    int fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);//socket della connessione
    int fd_num = 0; //max fd attivo
    int fd_c; //socket di I/O con il client
    struct sockaddr_un sa;
    strncpy(sa.sun_path, configurazione.UnixPath, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

    if(bind(fd_skt, (struct sockaddr *)&sa, sizeof(sa)) < 0){
        perror("errore binding");
        return;
    }

    if(listen(fd_skt, SOMAXCONN) < 0){
        perror("errore listen");
        return;
    }
     
    long fd; //indice per verificare il risultato della select
    fd_set read_set, set; //insieme dei file descriptor
    FD_ZERO(&set);
    //creo hash table
    ht = crea_hash(100,stat);
    //creo threadpool
    tp = crea_threadpool(configurazione.ThreadsInPool);
    //inizializzo la mutex per le statistiche
    if(pthread_mutex_init(&mutex_stat, NULL) != 0){
        perror("errore nella mutex statistiche");
        return;
    }
    //inizializzo la mutex per le connessioni
    if(pthread_mutex_init(&mutex_connections, NULL) != 0){
        perror("errore nella mutex connessioni");
        return;
    }
    
    //inizializzo la mutex per le write multiple
    if(pthread_mutex_init(&mutex_writes, NULL) != 0){
        perror("errore nella mutex writes");
        return;
    }

    //inizializzo l'array di connessioni

    conn = malloc(sizeof(connections_t)* configurazione.MaxConnections);
    for(int i = 0; i < configurazione.MaxConnections; i++){
        conn[i].fd = -1;
        memset(conn[i].nick, 0, MAX_NAME_LENGTH +1);
    }

    //aggiungo al set
    //set set fd_skt
    FD_SET(fd_skt,&set);
    if(fd_num < fd_skt) fd_num = fd_skt;
    //loop select
    
    while(!toExit){
        void*tmp=remove_node_nowait(tp->coda_back);
        while(tmp != NULL) {
            long fd = *(long*)tmp;
            if(fd_num < fd) fd_num = fd;
            FD_SET(fd, &set);
            free(tmp);
            tmp=remove_node_nowait(tp->coda_back);
        }
        struct timeval tv;
        tv.tv_sec=0;
        tv.tv_usec= 1; //50 000 microsecondi
        read_set = set;
        //select
        int err;
        if((err = select(fd_num +1, &read_set, NULL, NULL, &tv) == -1)){
            //CONTROLLA SE è UN EINTR E VA BENE
            if(errno != EINTR){
                perror("CRASH");
            }
            //ALTRIMENTI è CRASHATO
            continue;
            perror("errore nella select");
        }
        
        //test sulla socket principale
        for(fd = 0; fd<= fd_num + 1; fd++){

            if(FD_ISSET(fd,&read_set)){

                if(fd == fd_skt){/*sock connect pronto*/
                    //accept
                    fd_c = accept(fd_skt, NULL, 0);

                    if(fd_c == -1){
                        perror("errrore nella accept");
                    }
                    
                    if(ht->stat->nonline>configurazione.MaxConnections){
                        perror("troppi client connessi");
                        //chiudo il file descriptor
                        close(fd);
                        continue;
                    }
                    //set
                    FD_SET(fd_c, &set);
                    if(fd_num < fd_c) fd_num = fd_c;

                }else{
                    //test sulle altre
                    //test se connessione aperta o chiusa

                        //rimuovo dal set perchè adesso delego al threadpool la gestione
                        FD_CLR(fd,&set);

                        //push dentro threadpool.codatask
                        add_task(tp,worker,(void*)fd);
                }
            }
        }       
    }

    //termino il server
    if(toExit){
        termina();
    }
}

//funzione che dealloca tutte le strutture
void termina(){
    //rimuovo il threadpool che pensa lui a fare broadcast e join sui thread
    //e rimuove anche la coda
    if(tp != NULL){
	remove_threadpool(tp);
    }
   
    free(conn);

    pthread_mutex_destroy(&mutex_writes);
	pthread_mutex_destroy(&mutex_connections);
    pthread_mutex_destroy(&mutex_stat);
    //rimuovo l'hash table
    if(ht != NULL){
        remove_hash(ht, NULL, NULL);
    }

}
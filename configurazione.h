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


#if !defined(CONFIGURAZIONE_H_)
#define CONFIGURAZIONE_H_

typedef struct{
  char *UnixPath;
  int MaxConnections;
  int ThreadsInPool;
  int MaxMsgSize;
  int MaxFileSize;
  int MaxHistMsgs;
  char *DirName;
  char *StatFileName;
}t_param;

extern t_param configurazione;

/**
 * Cerca nel file di configurazione i parametri di configurazione
 *
 * @param list -- struttura contenente i parametri 
 * @param pas -- path dove trovo i file di configurazione
 *
 */
 
void fill(t_param *list, char *pas);

#endif /* CONFIGURAZIONE_H_ */
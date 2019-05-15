#!/bin/bash

###
# NOME: LUCA
#
# COGNOME: MAURI
#
# MAIL: LUCA.MAURI95@TISCALI.IT
#
# DICHIARO CHE IL PROGRAMMA è IN OGNI SUA PARTE, OPERA ORIGINALE DEL SUO AUTORE
###


#caso requisiti mancanti:
if [ $# = 0 ]; then
    echo "Usage: ./script.sh [configuration file] [time in minutes]"
    exit
elif
    [ "$1" == "-help" ]; then
    echo "Usage: ./script.sh [configuration file] [time in minutes]"
    exit
fi
#variabile contenente il path del file di configurazione:
CONFILE=$1
#variabile che indica il tempo in minuti:
t=$2
#variabile che contiene il path scritto dentro DirName nel file di configurazione:
DIR=$(grep DirName $CONFILE)
DIR=${DIR##*=}
#mi sposto nella cartella definita dal path
cd $DIR
#fichiaro un array con tutti i file nella cartella
declare -a FILEINDIR=(*)
#dichiaro un array vuoto che conterrà tutti i file più vecchi di t
declare -a OLDFILE=()
#se t == 0 stampo i file nella cartella
if [ "$t" == "0" ]
    then
        for i in "${FILEINDIR[@]}" ; do
	        echo $i
        done
#se t è diverso da 0 allora scorro gli elementi dell'array e li analizzo uno ad uno:
else
    for i in "${FILEINDIR[@]}" ; do

        #secondi passati da epoch a quando viene modificato l'ultima volata:
        time_since_mod=$(stat -c %Y $i)
        
        #secondi che son trascorsi da epoch:
        epoch=$(date +%s)
        
        #differenza in secondi tra epoch e l'ultima modifica:
        diff=$((epoch - time_since_mod))
        
        #converto il tempo in minuti:
        diff=$((diff/60))

        #se la differenza è maggiore del tempo t inserisco nell'array OLDFILE
        if [ "$diff" -gt "$t" ]
            then
                OLDFILE+=($i)
        fi

    done
    
    #se il numero di file è > 0 li inserisco in un archivio
    if [ ${#OLDFILE[@]} -gt 0 ];
        then
            #creo l'archivio 
            tar -czvf tar_old_files.tar.gz ${OLDFILE[*]}
            #rimuovo i file
            for i in "${OLDFILE[@]}" ; do
                rm -rf $i
            done
    else 
        echo "No elements older then $t minutes"        
    fi

fi
 





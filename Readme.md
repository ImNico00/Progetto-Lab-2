### STUDENTE ###
Nome Completo: NICOLO' PIETRO OGGIANO 
Matricola: 647447

### STRUTTURA PROGETTO ###
Tipo progetto: Semplificato
Files presenti: 
 - 3 files .c -> archivio.c - funThreads.c - xerrori.c
 - 1 file .py -> server.py
 - 2 files senza estensione -> client1 - client2
 - 3 files .h presenti nella cartella "dependency" -> archivio.h - funThreads.h - xerrori.h
 - 1 file makefile con funzioni di clean, zip e compilazione dei 3 files .c
Implementazione delle specifiche richieste per nome file.

### ARCHIVIO.C ###
Nel file "archivio.c" sono presenti 5 funzioni:
 - main -> Dove vengono effettuate le operazioni di avvio dei threads per la gestione dei segnali, per il capo scrittori ed il capo lettori con creazione dell'HashTable
 - crea_entry -> Creazione delle entry per l'HashTable
 - distruggi_entry -> Deallocazione di tutto quello presente nell'oggetto della struct ENTRY
 - aggiungi -> Dove avviene l'inserimento delle ENTRY nell'HashTable e, nel caso già inserita, l'incremento del valore di esso
 - conta -> Dove viene ritornato il valore dell'ENTRY presenti nell'HashTable

### FUNTHREADS.C ###
Nel file "funThreads.c" sono presenti 6 funzioni:
 - creaStringa -> Viene ritornata una stringa presa tramite una pipe passata come argomento della funzione insieme alla lunghezza richiesta
 - capoScrittori -> Produttore dei threads "scrittori", qui avviene la creazione dei threads scrittori con rispettivo buffer, la creazione delle due CV "condFreeSlots" e "dataSlots" ed il loro mutex, l'apertura e chiusura della pipe "caposc" con chiamata a funzione "creaStringa" ed infine la join di tutti i threads creati da esso
 - capoLettori -> Produttore dei threads "lettori", qui avviene la creazione dei threads lettori con rispettivo buffer, la creazione delle due CV "condFreeSlots" e "dataSlots" ed il loro mutex, l'apertura e chiusura della pipe "capolet" con chiamata a funzione "creaStringa" ed infine la join di tutti i threads creati da esso
 - scrittori -> Consumatori del thread "capoScrittori", qui avviene la lettura dal buffer e la chiamata alla funzione "aggiungi"
 - lettori -> Consumatori del thread "capoLettori", qui avviene la lettura dal buffer e la chiamata alla funzione "conta" con scrittura nel file "lettori.log" del relativo valore ritornato dalla funzione
 - controllore -> Qui avviene la gestione dei segnali con relativa join dei threads "capoScrittori" e "capoLettori" nel caso di segnale SIGTERM

### XERRORI.C ###
Nel file "xerrori.c" sono presenti le funzioni di libreria modificate viste nel corso dell'anno

### SERVER.PY ###
Nel file "server.py" sono presenti 3 funzioni:
 - main -> Qua vengono parsati gli argomenti passati, create le pipes "caposc" e "capolet", avviato l'eseguibile "archivio" in modi differenti nel caso dell'invio dell'argomento "-v", creato il file "server.log" dove vengono scritti tutti i log che avvengono nel mentre che il programma è in esecuzione e gestite le connessioni al server con più threads con numero inviato tramite argomento all'avvio
 - gestisci_connessione -> Qua vengono gestite le connessioni e vengono ricevuti 8 byte tra cui 4 byte per il tipo di connessione e 4 per capire la lunghezza della sequenza o inutili
 - recv_all -> Qua avviene la ricezione delle sequenze e, nel caso la sequenza fosse di lunghezza nulla, allora viene ritornato il valore letto fino a quel punto
Nel caso di segnale SIGINT il programma viene stoppato facendo stoppare "archivio" con un segnale SIGTERM aspettandolo per 10 secondi. Al termine vengono cancellate le pipes e chiuso il logger

### CLIENT1 ###
Nel file "clienti1" è presente 1 funzione:
 - main -> Dove avvengono le chiamate al server per ogni riga presente nel file passato come argomento. 
Esso invia 8 byte tra cui 4 per dichiarare il suo tipo di connessione (TIPO A) e 4 per dichiarare la lunghezza della sequenza inviata.

### CLIENT2 ###
Nel file "clienti2" è presente 1 funzione:
 - main -> Dove avvengono le chiamate al server per ogni file passato come argomento impiegando più threads insieme. Ogni thread apre un file e manda più connessioni inviando una quantità <= al massimo dei bytes inviabili. Se la quantità dei bytes è superiore al limite allora viene salvata la stringa vista e rimandata successivamente con il restante delle stringhe presenti nel file, rimanendo sempre al di sotto della sequenza massima inviabile.
Esso invia 8 byte tra cui 4 per dichiarare il suo tipo di connessione (TIPO B) e 4 inutili.

### CARTELLA DEPENDENCY ###
In questa cartella sono presenti i files "archivio.h", "funThreads.h" e "xerrori.h":
 - archivio.h -> Sono presenti tutte le define utilizzate, le varie struct ed il prototipo delle funzioni utilizzate del file archivio.c
 - funThreads.h -> Sono presenti il prototipo delle funzioni utilizzate del file funThreads.c
 - xerrori.h -> Sono presenti il prototipo delle funzioni utilizzate del file xerrori.c

### MAKEFILE ###
In questo file è possibile utilizzare 3 funzioni bash:
 - make -> Creazione dei files .o dei files .c presenti sopracitati ed eseguibile denominato "archivio"
 - clean -> Cancella tutto quello creato dalla funzione make ed in più tutto quello creato dall'eseguibile "archivio"
 - zip -> Creazione del file zip per gli elementi presenti nella cartella
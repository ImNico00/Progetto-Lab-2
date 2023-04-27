void *capoScrittori(void *arg); //FUNZIONE PER IL THREAD CAPO SCRITTORI
void *scrittori(void *arg); //FUNZIONE PER I THREAD SCRITTORI
void *capoLettori(void *arg); //FUNZIONE PER IL THREAD CAPO LETTORI
void *lettori(void *arg); //FUNZIONE PER I THREAD LETTORI
void *controllore(void *arg); //FUNZIONE PER IL THREAD SEGNALI
char *creaStringa(int pipe, int letto); //FUNZIONE PER LA CREAZIONE DI STRINGHE
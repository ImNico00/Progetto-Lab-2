#include "./dependency/archivio.h"
#include "./dependency/xerrori.h"
#include "./dependency/funThreads.h"

//CREAZIONE STRINGHE CON PIPE
char *creaStringa(int pipe, int letto) {
  if (letto > Max_sequence_length) termina("La lunghezza della stringa è maggiore della sequenza massima");
  char *stringa = malloc(sizeof(char)*letto+1);
  if (stringa == NULL) termina("Errore allocazione stringa");
  char carattere;
  for (int i = 0; i < letto; i++) {
    ssize_t e = read(pipe, &carattere, sizeof(char));
    if (e == 0) termina("Errore con la lettura della stringa");
    stringa[i] = carattere;
  }
  stringa[letto] = 0;
  return stringa;
}

/*

          INIZIO FUNZIONE PER THREAD CAPO SCRITTORI

*/   
void *capoScrittori(void *arg) {
  capoAggiungi_t *a = (capoAggiungi_t *) arg;

  int freeSlots = PC_buffer_len, //SLOTS FREE BUFFER PER AGGIUNGI
      indexScrittori = 0,
      nThreads = (a->numThreads),
      index = 0;

  pthread_mutex_t mutexCond = PTHREAD_MUTEX_INITIALIZER,
                  mutex = PTHREAD_MUTEX_INITIALIZER;
  
  pthread_cond_t condFreeSlots = PTHREAD_COND_INITIALIZER,
                 dataSlots = PTHREAD_COND_INITIALIZER;
   
  char *bufScrittori[PC_buffer_len];
  aggiungi_t arrAggiungi[nThreads-1]; //ARRAY DI STRUCT PER THREADS AGGIUNGI / SCRITTORI

  //LANCIO GLI SCRITTORI
  for (int i = 0; i < nThreads-1; i++) {
    arrAggiungi[i].freeSlots = &freeSlots;
    arrAggiungi[i].mutex = &mutex;
    arrAggiungi[i].mutexCond = &mutexCond;
    arrAggiungi[i].condFree = &condFreeSlots;
    arrAggiungi[i].Buffer = bufScrittori;
    arrAggiungi[i].numEntry = a->numEntry;
    arrAggiungi[i].condData = &dataSlots;
    arrAggiungi[i].indexThreads = &indexScrittori;
    arrAggiungi[i].condLettoriScrittori = a->condLettoriScrittori;
    arrAggiungi[i].mutexLettoriScrittori = a->mutexLettoriScrittori;
    arrAggiungi[i].numLettori = a->numLettori;
    xpthread_create(&a->arrayThreads[i+1],NULL,&scrittori,i+arrAggiungi,QUI);
  }

  int pipe = open("caposc", O_RDONLY);
  if (pipe < 0) termina("Errore apertura pipe scrittori");
  while (true) {
    int letto;
    ssize_t e = read(pipe, &letto, sizeof(int)); //leggo prima la len della stringa

    if (e != sizeof(int)) {
      fprintf(stdout, "LA PIPE SCRITTORI E' STATA CHIUSA!\n");
      for (int i = 0; i < nThreads-1; i++) {
        
        xpthread_mutex_lock(&mutexCond,QUI);
        while (freeSlots == 0) xpthread_cond_wait(&condFreeSlots, &mutexCond,QUI);
        if (freeSlots < 0 || freeSlots > PC_buffer_len) termina("Errore freeSlots uscito dal range");
        
        bufScrittori[index++ % PC_buffer_len] = NULL;
        //DECREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
        freeSlots -= 1;
        xpthread_cond_broadcast(&dataSlots,QUI);
        xpthread_mutex_unlock(&mutexCond,QUI);

      }
      break;
    }
    //fprintf(stdout, "\nCAPO HA LETTO LA LUNGHEZZA: %d\n",letto);
    char *stringa = creaStringa(pipe, letto);
    char *token = strtok(stringa, ".,:; \n\r\t");
    while(token != NULL) {

      xpthread_mutex_lock(&mutexCond,QUI);
      while (freeSlots == 0) xpthread_cond_wait(&condFreeSlots, &mutexCond,QUI);
      if (freeSlots < 0 || freeSlots > PC_buffer_len) termina("Errore freeSlots uscito dal range");

      bufScrittori[index++ % PC_buffer_len] = strdup(token);
      //DECREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
      freeSlots -= 1;
      xpthread_cond_broadcast(&dataSlots,QUI);
      xpthread_mutex_unlock(&mutexCond,QUI);

      token = strtok(NULL, ".,:; \n\r\t");
    }
    free(stringa);
  }

  //ATTENDO IL TERMINE DEI THREAD SCRITTORI
  for (int i = 1; i < nThreads; i++) { // parto da 1 perché lo 0 è il capo
    xpthread_join(a->arrayThreads[i], NULL, QUI);
    printf("IL CAPO SCRITTORE HA ATTESO LA FINE DI: %d\n", i);
  }

  //NON SERVE FARE LA DESTROY DEI MUTEX E CONDVAR PERCHE' SONO ALLOCATI STATICAMENTE 
  //E VENGONO DISTRUTTI A TERMINAZIONE PROCESSO/THREAD
  xpthread_mutex_destroy(&mutexCond,QUI);
  xpthread_mutex_destroy(&mutex,QUI);
  xpthread_cond_destroy(&condFreeSlots,QUI);
  xpthread_cond_destroy(&dataSlots,QUI);
  xclose(pipe,QUI);
  printf("ER CAPO SCRITTORE HA FINITO\n");
  return NULL;
}
/*

          FINE FUNZIONE PER THREAD CAPO SCRITTORI

*/   


/*

          INIZIO FUNZIONE PER THREAD CAPO LETTORI

*/
void *capoLettori(void *arg) {
  capoConta_t *a = (capoConta_t *) arg;

  int freeSlots = PC_buffer_len,  //SLOTS FREE BUFFER PER CONTA
      indexLettori = 0,
      nThreads = (a->numThreads);

  pthread_mutex_t mutexCond = PTHREAD_MUTEX_INITIALIZER,
                  mutex = PTHREAD_MUTEX_INITIALIZER;

  pthread_cond_t condFreeSlots = PTHREAD_COND_INITIALIZER,
                 dataSlots = PTHREAD_COND_INITIALIZER;

  conta_t arrConta[nThreads-1]; //ARRAY DI STRUCT PER THREADS CONTA / LETTORI

  char *bufLettori[PC_buffer_len];
  FILE *f = xfopen("lettori.log", "w",QUI);
  if (f == NULL) termina("Errore creazione file lettori.log");

  //LANCIO I LETTORI
  for (int i = 0; i < nThreads-1; i++) {
    arrConta[i].freeSlots = &freeSlots;
    arrConta[i].mutexCond = &mutexCond;
    arrConta[i].mutex = &mutex;
    arrConta[i].condFree = &condFreeSlots;
    arrConta[i].Buffer = bufLettori;
    arrConta[i].condData = &dataSlots;
    arrConta[i].indexThreads = &indexLettori;
    arrConta[i].file = f;
    arrConta[i].condLettoriScrittori = a->condLettoriScrittori;
    arrConta[i].mutexLettoriScrittori = a->mutexLettoriScrittori;
    arrConta[i].numLettori = a->numLettori;
    xpthread_create(&a->arrayThreads[i+1],NULL,&lettori,i+arrConta,QUI);
  }

  int pipe = open("capolet", O_RDONLY);
  if (pipe < 0) termina("Errore apertura pipe lettori");
  int index = 0;
  while (true) {
    int letto;
    ssize_t e = read(pipe, &letto, sizeof(int)); //leggo prima la len della stringa
    
    if (e != sizeof(int)) {
      fprintf(stdout, "LA PIPE LETTORI E' STATA CHIUSA!\n");
      for (int i = 0; i < nThreads-1; i++) {

        xpthread_mutex_lock(&mutexCond,QUI);
        while (freeSlots == 0) xpthread_cond_wait(&condFreeSlots, &mutexCond,QUI);
        if (freeSlots < 0 || freeSlots > PC_buffer_len) termina("Errore freeSlots uscito dal range");

        bufLettori[index++ % PC_buffer_len] = NULL;
        //DECREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
        freeSlots -= 1;
        xpthread_cond_broadcast(&dataSlots,QUI);
        xpthread_mutex_unlock(&mutexCond,QUI);
      }
      break;
    }

    fprintf(stdout, "\nCAPO HA LETTO LA LUNGHEZZA: %d\n",letto);
    char *stringa = creaStringa(pipe, letto);
    char *token = strtok(stringa, ".,:; \n\r\t");

    while(token != NULL) {

      xpthread_mutex_lock(&mutexCond,QUI);
      while (freeSlots == 0) xpthread_cond_wait(&condFreeSlots, &mutexCond,QUI);
      if (freeSlots < 0 || freeSlots > PC_buffer_len) termina("Errore freeSlots uscito dal range");

      bufLettori[index++ % PC_buffer_len] = strdup(token);
      //DECREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
      freeSlots -= 1;
      xpthread_cond_broadcast(&dataSlots,QUI);
      xpthread_mutex_unlock(&mutexCond,QUI);

      token = strtok(NULL, ".,:; \n\r\t");
    }
    free(stringa);
  }
  
  for (int i = 1; i < nThreads; i++) { // parto da 1 perché lo 0 è il capo
    xpthread_join(a->arrayThreads[i], NULL,QUI);
    printf("IL CAPO LETTORE HA ATTESO LA FINE DI: %d\n", i);
  }
  
  //NON SERVE FARE LA DESTROY DEI MUTEX E CONDVAR PERCHE' SONO ALLOCATI STATICAMENTE 
  //E VENGONO DISTRUTTI A TERMINAZIONE PROCESSO/THREAD
  xpthread_mutex_destroy(&mutexCond,QUI);
  xpthread_mutex_destroy(&mutex,QUI);
  xpthread_cond_destroy(&condFreeSlots,QUI);
  xpthread_cond_destroy(&dataSlots,QUI);
  fclose(f);
  xclose(pipe,QUI);
  printf("ER CAPO LETTORE HA FINITO\n");
  return NULL;
}
/*

          FINE FUNZIONE PER THREAD CAPO LETTORI

*/


/*

          INIZIO FUNZIONE PER THREAD SCRITTORI

*/
void *scrittori(void *arg) {
  aggiungi_t *a = (aggiungi_t *) arg;
  while(true) {

    //LOCK MUTEX CV
    xpthread_mutex_lock(a->mutexCond,QUI);
    while (*(a->freeSlots) == PC_buffer_len) xpthread_cond_wait(a->condData, a->mutexCond,QUI);
    if (*(a->freeSlots) < 0 || *(a->freeSlots) > PC_buffer_len) termina("Errore freeSlots uscito dal range");
    
    //LOCK MUTEX THREADS CONCORRENTI
    xpthread_mutex_lock(a->mutex,QUI);
    char *stringa = a->Buffer[*(a->indexThreads) % PC_buffer_len];
    if (stringa == NULL) {

      //INCREMENTO L'INDEX PER I VARI THREADS
      *(a->indexThreads) += 1;
      xpthread_mutex_unlock(a->mutex,QUI);

      //INCREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
      *(a->freeSlots) += 1;
      xpthread_cond_signal(a->condFree,QUI);
      xpthread_mutex_unlock(a->mutexCond,QUI);

      break;
    }

    //fprintf(stdout, "[SCRITTORE] AGGIUNGO: %s\n", stringa);
    //DENTRO LA LOCK PERCHE' LA VARIABILE numEntry POTREBBE AVERE MODIFICHE INDESIDERATE

    //CONTROLLO CHE NON CI SIANO LETTORI A LEGGERE
    while (*(a->numLettori) > 0) xpthread_cond_wait(a->condLettoriScrittori, a->mutexLettoriScrittori,QUI);
    aggiungi(stringa, a->numEntry);

    //INCREMENTO L'INDEX PER I VARI THREADS
    *(a->indexThreads) += 1;
    xpthread_mutex_unlock(a->mutex,QUI);

    //INCREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
    *(a->freeSlots) += 1;
    xpthread_cond_signal(a->condFree,QUI);
    xpthread_mutex_unlock(a->mutexCond,QUI);

    free(stringa);
    //fprintf(stdout, "SONO A: %d\n", *(a->indexThreads));
  }
  fprintf(stdout, "Scrittore termina\n");
  return NULL;
}
/*

          FINE FUNZIONE PER THREAD SCRITTORI

*/

/*

          INIZIO FUNZIONE PER THREAD LETTORI

*/
void *lettori(void *arg) { 
  conta_t *a = (conta_t *) arg;
  while(true) {

    //LOCK MUTEX CV
    xpthread_mutex_lock(a->mutexCond,QUI);
    while (*(a->freeSlots) == PC_buffer_len) xpthread_cond_wait(a->condData, a->mutexCond,QUI);
    if (*(a->freeSlots) < 0 || *(a->freeSlots) > PC_buffer_len) termina("Errore freeSlots uscito dal range");

    //LOCK MUTEX THREADS CONCORRENTI
    xpthread_mutex_lock(a->mutex,QUI);
    char *stringa = a->Buffer[*(a->indexThreads) % PC_buffer_len];
    if (stringa == NULL) {
      //INCREMENTO L'INDEX PER I VARI THREADS
      *(a->indexThreads) += 1;
      xpthread_mutex_unlock(a->mutex,QUI);

      //INCREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
      *(a->freeSlots) += 1;
      xpthread_cond_signal(a->condFree,QUI);
      xpthread_mutex_unlock(a->mutexCond,QUI);

      break;
    }
    //fprintf(stderr, "L'HO LETTA: %s\n",stringa);

    //INCREMENTO L'INDEX PER I VARI THREADS
    *(a->indexThreads) += 1;
    xpthread_mutex_unlock(a->mutex,QUI);

    //INCREMENTO IL NUMERO DELLE SLOT "LIBERE" DEL BUFFER PER CV
    *(a->freeSlots) += 1;
    xpthread_cond_signal(a->condFree,QUI);
    xpthread_mutex_unlock(a->mutexCond,QUI);

    xpthread_mutex_lock(a->mutexLettoriScrittori,QUI);
    *(a->numLettori) += 1;
    xpthread_cond_broadcast(a->condLettoriScrittori,QUI);
    xpthread_mutex_unlock(a->mutexLettoriScrittori,QUI);

    int n = conta(stringa);

    xpthread_mutex_lock(a->mutexLettoriScrittori,QUI);
    *(a->numLettori) -= 1;
    xpthread_cond_broadcast(a->condLettoriScrittori,QUI);
    xpthread_mutex_unlock(a->mutexLettoriScrittori,QUI);

    fprintf(a->file, "%s %d\n", stringa, n);
    free(stringa);
  }
  fprintf(stdout, "Scrittore termina\n");
  return NULL;
}
/*

          FINE FUNZIONE PER THREAD LETTORI

*/


/*

          INIZIO FUNZIONE PER THREAD CONTROLLO SEGNALI

*/
void *controllore(void *arg) { 
  controlla_t *a = (controlla_t *) arg;
  int s;
  while(true) {
    int e = sigwait(a->mask,&s);
    if(e != 0) perror("Errore sigwait");
    printf("Thread gestore svegliato dal segnale %d\n",s);
    if(s==SIGINT) {
      fprintf(stderr, 
              "Ce stanno %d stringhe nell'HashMap\n",
              *(a->numEntry));
    } else if (s==SIGTERM) {
      fprintf(stdout, 
        "Ce stanno %d stringhe nell'HashMap\n",
        *(a->numEntry));

      //FACCIO LA JOIN DEL CAPO DEI THREAD LETTORI
      xpthread_join(*a->capoConta, NULL, QUI);
      printf("I thread Lettori hanno finito.\n");

      //FACCIO LA JOIN DEL CAPO DEI THREAD SCRITTORI
      xpthread_join(*a->capoAggiungi, NULL, QUI);
      printf("I thread Scrittori hanno finito.\n");

      a->continua = false;
      printf("\nSTO CHIUDENDO.\n\n");
      break;
    }
  }
  return NULL;
}
/*

          FINE FUNZIONE PER THREAD CONTROLLO SEGNALI

*/
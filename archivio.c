#include "./dependency/archivio.h"
#include "./dependency/xerrori.h"
#include "./dependency/funThreads.h"

int main(int argc, char* argv[]) {
  
  if (argc != 3) {
    fprintf(stderr, "Utilizzo %s n_scrittori n_lettori\n", argv[0]);
    exit(1);
  }

  sigset_t *mask = malloc(sizeof(sigset_t));
  sigfillset(mask); 
  sigaddset(mask, SIGINT);
  sigaddset(mask, SIGTERM);
  pthread_sigmask(SIG_BLOCK,mask,NULL); // blocco solo i segnali richiesti
  
  int w = atoi(argv[1]), //SCRITTORI USANO AGGIUNGI
      r = atoi(argv[2]); //LETTORI USANO CONTA

  pthread_t tAggiungi[w], tConta[r], gestore; //ARRAY CON I THREADS
  capoAggiungi_t CapoAggiungi; //STRUCT PER CAPO AGGIUNGI / SCRITTORI
  capoConta_t CapoConta; //STRUCT PER CAPO CONTA / LETTORI
  controlla_t c; // STRUCT PER CONTROLLORE
  int entryHash = 0;

  c.numEntry = &entryHash;
  c.continua = true;
  c.capoAggiungi = &tAggiungi[0];
  c.capoConta = &tConta[0];
  c.mask = mask;
  xpthread_create(&gestore,NULL,&controllore,&c,QUI);

  int ht = hcreate(Num_elem); //CREO HASHTABLE DI GRANDEZZA NUM_ELEM
  if(ht == 0) termina("Errore creazione HT");

  //CAPO SCRITTORI
  CapoAggiungi.numEntry = &entryHash;
  CapoAggiungi.numThreads = w;
  CapoAggiungi.arrayThreads = tAggiungi;
  xpthread_create(&tAggiungi[0],NULL,&capoScrittori,&CapoAggiungi,QUI);

  //CAPO LETTORI
  CapoConta.numThreads = r;
  CapoConta.arrayThreads = tConta;
  xpthread_create(&tConta[0],NULL,&capoLettori,&CapoConta,QUI);

  do { // loop apparentemente senza uscita
    sleep(10); 
    puts("Mi sono svegliato");
  } while(c.continua);

  xpthread_join(gestore, NULL, QUI); // JOIN GIUSTO PER TOGLIERE UN ERRORE DA VALGRIND
  fprintf(stdout, "Il thread Controllore è stato terminato.\n");

  hdestroy();
  free(mask);
  fprintf(stdout, "Ho terminato tutto con il thread main\n");
  return 0;
}

//TUTTO PER LA HASHTABLE
ENTRY *crea_entry(char *s) {
  ENTRY *e = malloc(sizeof(ENTRY));
  if(e==NULL) termina("Errore malloc entry 1");
  e->key = strdup(s); // salva copia di s
  e->data = (int *) malloc(sizeof(int));
  if(e->key==NULL || e->data==NULL)
    termina("Errore malloc entry 2");
  *((int *)e->data) = 1;
  return e;
}

void distruggi_entry(ENTRY *e) {
  free(e->key); free(e->data); free(e);
}

    
void aggiungi(char *s, int *numEntry) {
  ENTRY *e = crea_entry(s);
  ENTRY *r = hsearch(*e, FIND);
  if (r == NULL) { // la entry è nuova
    r = hsearch(*e, ENTER);
    if(r == NULL) termina("Errore o tabella piena");
    *(numEntry) += 1;
  } else {
    assert(strcmp(e->key, r->key) == 0);
    int *d = (int *) r->data;
    *d += 1;
    distruggi_entry(e);
  }
}


int conta(char *s) {
  ENTRY *e = crea_entry(s);
  ENTRY *r = hsearch(*e, FIND);
  if(r == NULL) { // la entry è nuova
    distruggi_entry(e);
    return 0;
  }
  else {
    assert(strcmp(e->key, r->key) == 0);
    int *d = (int *) r->data;
    distruggi_entry(e);
    return *d;
  }
}
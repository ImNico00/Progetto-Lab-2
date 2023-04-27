#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>    // permette di usare scanf printf etc ...
#include <stdlib.h>   // conversioni stringa/numero exit() etc ...
#include <stdbool.h>  // gestisce tipo bool (variabili booleane)
#include <assert.h>   // permette di usare la funzione assert
#include <string.h>   // confronto/copia/etc di stringhe
#include <errno.h>
#include <search.h>
#include <signal.h>
#include <unistd.h>  // per sleep 
#include <pthread.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#define Num_elem 1000000
#define PC_buffer_len 10
#define PORT 57447
#define Max_sequence_length 2048
#define QUI __LINE__,__FILE__

typedef struct {
  int *freeSlots;
  pthread_mutex_t *mutexCond;
  pthread_mutex_t *mutex;
  pthread_cond_t *condFree;
  pthread_cond_t *condData;
  char **Buffer;
  int *numEntry;
  int *indexThreads;
} aggiungi_t;

typedef struct {
  int *numEntry;
  int numThreads;
  pthread_t *arrayThreads;
} capoAggiungi_t;

typedef struct {
  int *freeSlots;
  pthread_mutex_t *mutexCond;
  pthread_mutex_t *mutex;
  pthread_cond_t *condFree;
  pthread_cond_t *condData;
  char **Buffer;
  int *indexThreads;
  FILE *file;
} conta_t;

typedef struct {
  int numThreads;
  pthread_t *arrayThreads;
} capoConta_t;

typedef struct {
  int *numEntry;
  bool continua;
  pthread_t *capoConta;
  pthread_t *capoAggiungi;
  sigset_t *mask;
} controlla_t;


void aggiungi(char *s, int *entry);
int conta(char *s);
void distruggi_entry(ENTRY *e); //DISTRUZIONE ENTRY
ENTRY *crea_entry(char *s); //CREAZIONE ENTRY

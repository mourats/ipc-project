#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/sem.h>
#include <sys/types.h>

// função que guardará o valor de cada semáforo
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
};

struct Topic {
    int id;
    int pubs_subs_count; // publishers and subscribers size
    int pid_pub[50]; // size max of publishers 
    int pid_sub[50][2]; // size max of subscribers and flag read
    int msg_count; // messages counter
    int msg[5]; // array of messages
    int msg_index; // index of end of messages on array
    int querem_ler; // indica quantos processos estão bloqueados esperando para ler
    int querem_escrever;  // indica quantos processos estão bloqueados esperando para escrever

    // id's dos três semáforos que serão usados, e semval, é uma variável que vai ser usada pra guardar
    // os valores dos semáforoa temporariamente
    int semid_mut, semid_cond_read, semid_cond_pub, semval;

    // estrutura para cada um dos semáforos
    struct sembuf mutex, cond_read, cond_pub;
    // variáveis que guardarão o valor de cada um dos semáforos
    union semun arg_mut, arg_cond_read, arg_cond_pub;
};

struct Pub {
    int topics[5]; // array of topics ids
    int topics_count; // topics counter
    int pos_topic; // free topic index
};

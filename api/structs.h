#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/sem.h>
#include <sys/types.h>

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
    int msg[3]; // array of messages
    int msg_index; // index of end of messages on array
    int querem_ler;
    int querem_escrever;
    int semid_mut, semid_cond_read, semid_cond_pub, semval;
    struct sembuf mutex, cond_read, cond_pub;
    union semun arg_mut, arg_cond_read, arg_cond_pub;
};

struct Pub {
    int topics[20]; // array of topics ids
    int topics_count; // topics counter
    int pos_topic; // free topic index
};

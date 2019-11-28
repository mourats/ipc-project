#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/types.h>

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
};

struct Pub {
    int topics[20]; // array of topics ids
    int topics_count; // topics counter
    int pos_topic; // free topic index
};

union {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
} arg_mut;

union {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
} arg_cond_read;

union {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
} arg_cond_pub;

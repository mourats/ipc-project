#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

struct Topic {
    int id;
    int pubs_subs_count; // publishers and subscribers size
    int pid_pub[50]; // size max of publishers 
    int pid_sub[50][2]; // size max of subscribers
    int msg_count; // count max of messages
    int msg[100]; // size max of messages
    int id_index; // index of head messages on array
    int msg_index; // index of end of messages on array
};

struct Pub {
    int id;
    struct Topic *tLink; // linked list to dinamyc size of different topics
};
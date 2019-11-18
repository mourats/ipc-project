#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/types.h>

struct Topic {
    int id;
    int pubs_subs_count; // publishers and subscribers size
    int pid_pub[50]; // size max of publishers 
    int pid_sub[50][2]; // size max of subscribers
    int msg_count; // count max of messages
    int msg[3]; // size max of messages
    int id_index; // index of head messages on array
    int msg_index; // index of end of messages on array
};

struct Pub {
    int id;
    struct Topic * t[2]; // linked list to dinamyc size of different topics
    int pos_topic;
};

struct Pub * pubsub_init();
int get_shmid_segment(int topic_id);
struct Topic * open_shm_segment(int topic_id);
int pubsub_create_topic(int topic_id);
int pubsub_join(int topic_id);
int pubsub_subscribe(int topic_id);
int pubsub_cancel(int topic_id);
int pubsub_publish(int topic_id, int msg);
int getpos_pub(pid_t pid, struct Topic * t);
int getpos_sub(pid_t pid, struct Topic * t);
int pubsub_read(int topic_id);

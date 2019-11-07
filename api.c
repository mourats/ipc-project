#include <stdio.h>
#include <stdlib.h>

//Estrutura base do nó.
struct topic {
    int id;
    int pubs_count; // publishers number
    int subs_count; // subscribers number
    int pid[50]; // size max of publishers + subscribers
    int msg[100]; // size max of messages
    int head_index; // index of head messages on array
    int tail_index; // index of end of messages on array
};

struct hub {
    int id;
    struct topic *tLink; // linked list to dinamyc size of different topics
};

int pubsub_init();
int pubsub_create_topic(int topic_id);
int pubsub_join(int topic_id);
int pubsub_subscribe(int topic_id);
int pubsub_cancel(int topic_id);
int pubsub_publish(int topic_id, int msg);

int main(void)
{
    struct hub *hub;
    hub = (struct hub*) malloc(sizeof(struct hub*));

    struct topic *t1; // topic 1
    t1 = (struct topic*) malloc(sizeof(struct topic*));

    hub->tLink = t1;

    t1->pid[0] = 20;
    
    printf("test %d\n", hub->tLink->pid[0]);

    printf("size of complete Hub struct: %zu\n", sizeof(struct hub));
    printf("size of single Topic struct: %zu\n", sizeof(hub->tLink));
  
  return 0;
}
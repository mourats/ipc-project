#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include "structs.h"

#define TRUE 1
#define FALSE 0

struct Pub *p;

int pubsub_init() {
    p = (struct Pub*) malloc(sizeof(struct Pub*));
    p->id = 0;
}

// return a pointer above shared memory segment
struct Topic * open_shm_segment(int topic_id) {
    int shm_id;
    int size = 1024;
    
    /* ftok to generate unique key
    key_t key; 
    char const *path = "shmfile";
    #define KEY 61
    key = ftok(path, KEY);
    printf("Segmento associado a chave unica: %d\n", key);
    */

    // shmget returns an identifier in shmid
    shm_id = shmget((key_t) topic_id, sizeof(struct Topic), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Erro shmget()");
        exit(1);
    }
    /* Report */
    // printf("ID do segmento de memoria: %d\n", shm_id);
    //printf("Segmento associado a chave unica: %d\n", key);
  
    // shmat to attach to shared memory
    // return the pointer to shared memory segment
    return ((struct Topic*) shmat(shm_id, NULL,0));
}

int close_shm_segment(struct Topic *t) {
    return shmdt(t);
}

int get_shmid_segment(int topic_id) {
    int shm_id;
    shm_id = shmget((key_t) topic_id, sizeof(struct Topic), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Erro shmget()");
        exit(1);
    }
    return shm_id;
}

int destroy_shm_segment(int topic_id) {
    int shm = get_shmid_segment(topic_id);
    return shmctl(shm, IPC_RMID, NULL);
}

void fill_pids(struct Topic *t) {
    for(int i = 0; i < t->pubs_subs_count; i++) {
        t->pid_pub[i] = -1;
        t->pid_sub[i][0] = -1;
        t->pid_sub[i][1] = -1;
    }
}

int pubsub_create_topic(int topic_id) {
    struct Topic *t = open_shm_segment(topic_id);
    t->pubs_subs_count = sizeof t->pid_pub / sizeof *t->pid_pub;
    t->id = topic_id;
    fill_pids(t);
    p->tLink = t;

    //detach from shared memory
    return close_shm_segment(t);
}

int pubsub_join(int topic_id) {
    pid_t pub_id = getpid();
    struct Topic *t = open_shm_segment(topic_id);
    
    int fit = FALSE;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_pub[i] == -1) {
            t->pid_pub[i] = pub_id;
            fit = TRUE;
            break;
        }
    }

    if(!fit) {
        perror("error pubsub_join");
        exit(1);
    }

    return pub_id;
}

int pubsub_subscribe(int topic_id);
int pubsub_cancel(int topic_id);

int contain_pub(pid_t pid, struct Topic *t) {
    int contain = FALSE;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_pub[i] == pid) {
            contain = TRUE;
            break;
        }
    }

    return contain;
}

int pubsub_publish(int topic_id, int msg) {
    struct Topic *t = open_shm_segment(topic_id);

    pid_t pub_id = getpid();
    if(!contain_pub(pub_id, t)) {
        perror("error pubsub_publish");
        exit(1);
    }

    t->msg[t->msg_index] = msg;
    t->msg_index++;
    
    //detach from shared memory
    return close_shm_segment(t);
}

int main(void)
{
    pubsub_init();
    pubsub_create_topic(11);

    printf("size of complete Pub struct: %zu\n", sizeof(struct Pub));
    printf("size of single Topic struct: %zu\n", sizeof(p->tLink));

    struct Topic *t = open_shm_segment(11);

    // pubsub_join(11);
    printf("%d\n", t->pid_pub[0]);

    printf("before publish %d\n", t->msg[0]);
    pubsub_publish(11, 5028); // escreve no topico 11 criado anterior mente a msg 5028
    // pubsub_publish(11, 3000);
    // pubsub_publish(11, 4666);
    printf("after publish %d\n", t->msg[0]);
    // printf("after publish %d\n", t->msg[1]);
    // printf("after publish %d\n", t->msg[2]);
    // printf("%d", destroy_shm_segment(11)); // se nao quiser excluir so usar o metodo close_shm_segment() que as infos ficam salvas;
  
  return 0;
}

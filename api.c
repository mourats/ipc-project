#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include "structs.h"
#include <semaphore.h>

#define TRUE 1
#define FALSE 0

struct Pub *p;
sem_t mutex;

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
    sem_init(&mutex, 0, 1);

    struct Topic *t = open_shm_segment(topic_id);
    t->pubs_subs_count = sizeof t->pid_pub / sizeof *t->pid_pub;
    t->msg_count = sizeof t->msg / sizeof *t->msg;
    t->id = topic_id;
    fill_pids(t);
    p->tLink = t;

    //detach from shared memory
    return close_shm_segment(t);
}

int pubsub_join(int topic_id) {
    struct Topic *t = open_shm_segment(topic_id);

    sem_wait(&mutex);
    pid_t pub_id = getpid();
    
    int fit = FALSE;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_pub[i] == -1) {
            t->pid_pub[i] = pub_id;
            fit = TRUE;
            break;
        }
    }

    if(!fit) {
        sem_post(&mutex);
        perror("error pubsub_join");
        exit(1);
    }
    sem_post(&mutex);

    return pub_id;
}

int pubsub_subscribe(int topic_id);

int pubsub_cancel(int topic_id) {
    struct Topic *t = open_shm_segment(topic_id);

    sem_wait(&mutex);
    pid_t pubsub_id = getpid();
    
    for(int i = 0; i < t->pubs_subs_count; i++) {
        int breakyes = FALSE;
        if(t->pid_pub[i] == pubsub_id) {
            t->pid_pub[i] = -1;
            breakyes = TRUE;
        }
        if(t->pid_sub[i][0] == pubsub_id) {
            t->pid_sub[i][0] = -1;
            breakyes = TRUE;
        }

        if(breakyes) break;
    }
    sem_post(&mutex);

    return pubsub_id;
}

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

    sem_wait(&mutex);
    pid_t pub_id = getpid();
    if(!contain_pub(pub_id, t)) {
        sem_post(&mutex);
        perror("error pubsub_publish");
        exit(1);
    }

    if(t->msg_count == t->msg_index) {
        sem_post(&mutex);
        perror("buffer cheio");
        exit(1);
    }

    t->msg[t->msg_index] = msg;
    t->msg_index++;
    sem_post(&mutex);
    
    //detach from shared memory
    return close_shm_segment(t);
}

int main(void)
{
    pubsub_init();
    pubsub_create_topic(12);

    printf("size of complete Pub struct: %zu\n", sizeof(struct Pub));
    printf("size of single Topic struct: %zu\n", sizeof(p->tLink));

    struct Topic *t = open_shm_segment(12);

    pubsub_join(12);
    printf("%d\n", t->pid_pub[0]);

    printf("before publish %d\n", t->msg[t->msg_index]);
    pubsub_publish(12, 5028); // escreve no topico 11 criado anterior mente a msg 5028
    printf("after publish %d\n", t->msg[t->msg_index - 1]);

    pubsub_cancel(12);
    printf("before publish %d\n", t->msg[t->msg_index]);
    pubsub_publish(12, 666); // escreve no topico 11 criado anterior mente a msg 5028
    printf("after publish %d\n", t->msg[t->msg_index - 1]);

    // pubsub_publish(12, 3000);
    // pubsub_publish(12, 4666);
    // printf("after publish %d\n", t->msg[1]);
    // printf("after publish %d\n", t->msg[2]);
    // printf("%d", destroy_shm_segment(11)); // se nao quiser excluir so usar o metodo close_shm_segment() que as infos ficam salvas;
  
  return 0;
}

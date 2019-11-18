#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
// #include <semaphore.h>
#include <pthread.h>

#include "structs.h"

#define TRUE 1
#define FALSE 0

struct Pub *p;
// sem_t mutex;
pthread_mutex_t mutex;

struct Pub* pubsub_init() {
    p = (struct Pub*) malloc(sizeof(struct Pub*));
    p->id = 0;
    p->pos_topic = 0;
    return p;
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
    // sem_init(&mutex, 0, 1);
    pthread_mutex_init(&mutex, NULL);

    struct Topic *t = open_shm_segment(topic_id);
    t->pubs_subs_count = sizeof t->pid_pub / sizeof *t->pid_pub;
    t->msg_count = sizeof t->msg / sizeof *t->msg;
    t->msg_index = 0;
    t->id = topic_id;
    fill_pids(t);
    p->t[p->pos_topic] = t;
    p->pos_topic++;

    //detach from shared memory
    return close_shm_segment(t);
}

int pubsub_join(int topic_id) {
    struct Topic *t = open_shm_segment(topic_id);

    // sem_wait(&mutex);
    pthread_mutex_lock(&mutex);
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
        printf("erro pubsub_join\n");
        pthread_mutex_unlock(&mutex);
        // sem_post(&mutex);
        return 0;
    }
    // sem_post(&mutex);
    pthread_mutex_unlock(&mutex);

    return pub_id;
}

int pubsub_subscribe(int topic_id) {
    struct Topic *t = open_shm_segment(topic_id);

    // sem_wait(&mutex);
    pthread_mutex_lock(&mutex);
    pid_t sub_id = getpid();
    int fit = FALSE;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_sub[i][0] == -1) {
            t->pid_sub[i][0] = sub_id;
            t->pid_sub[i][1] = t->msg_index;
            fit = TRUE;
            break;
        }
    }

    if(!fit) {
        printf("error pubsub_subscribe\n");
        // sem_post(&mutex);
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    // sem_post(&mutex);
    pthread_mutex_unlock(&mutex);

    return sub_id;
}

// um processo subscriber que vai cancelar a subscrição 
// feita para o tópico
int pubsub_cancel(int topic_id) {
    struct Topic *t = open_shm_segment(topic_id);

    // sem_wait(&mutex);
    pthread_mutex_lock(&mutex);
    pid_t pubsub_id = getpid();
    
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_sub[i][0] == pubsub_id) {
            t->pid_sub[i][0] = -1;
            break;
        }
    }
    // sem_post(&mutex);
    pthread_mutex_unlock(&mutex);

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

// conferir se todos os subcribers leram até a última mensagem antes
// da última mensagem publicada
int did_everyone_read(struct Topic *t) {
    int everybody_read = TRUE;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_sub[i][0] != -1 && t->pid_sub[i][1] < t->msg_index) {
            everybody_read = FALSE;
            break;
        }
    }

    return everybody_read;
}

// caso o buffer esteja na posição máx, é conferido se todo mundo leu, caso sim
// ele volta para o começo, caso não, é lançado um erro
int pubsub_publish(int topic_id, int msg) {
    struct Topic *t = open_shm_segment(topic_id);

    // sem_wait(&mutex);
    pthread_mutex_lock(&mutex);
    pid_t pub_id = getpid();
    if(!contain_pub(pub_id, t)) {
        printf("error pubsub_publish\n");
        // sem_post(&mutex);
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    // se for atingido o limite do buffer, espera até que todo mundo tenha lido
    // a última mensagem
    if(t->msg_index % t->msg_count == 0) {
        if(!did_everyone_read(t)) {
            printf("buffer cheio, tente mais tarde\n");
            // sem_post(&mutex);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }

    t->msg[t->msg_index % t->msg_count] = msg;
    t->msg_index++;
    // sem_post(&mutex);
    pthread_mutex_unlock(&mutex);
    
    //detach from shared memory
    return close_shm_segment(t);
}

// retorna a posição no array de publishers do publisher com o pid passado
// como no parâmetro
int getpos_pub(pid_t pid, struct Topic *t) {
    int contain = -1;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_pub[i] == pid) {
            contain = i;
            break;
        }
    }

    return contain;
}

// retorna a posição no array de subscribers do subscriber com o pid passado
// como no parâmetro
int getpos_sub(pid_t pid, struct Topic *t) {
    int contain = -1;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_sub[i][0] == pid) {
            contain = i;
            break;
        }
    }

    return contain;
}

int pubsub_read(int topic_id) {
    struct Topic *t = open_shm_segment(topic_id);

    // sem_wait(&mutex);
    pthread_mutex_lock(&mutex);
    pid_t sub_id = getpid();
    int pos_sub = getpos_sub(sub_id, t);
    if(pos_sub == -1) {
        printf("error pubsub_read\n");
        // sem_post(&mutex);
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    int index_msg_read = t->pid_sub[pos_sub][1];

    // caso a próx mensagem ainda não tenha sido publicada, é
    // lançado um erro
    if(index_msg_read >= t->msg_index) {
        printf("sem novas mensagens, volte mais tarde\n");
        // sem_post(&mutex);
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    
    int msg = t->msg[index_msg_read % t->msg_count];
    t->pid_sub[pos_sub][1]++;
    // sem_post(&mutex);
    pthread_mutex_unlock(&mutex);

    return msg;
}

// int main(void)
// {
//     pubsub_init();
//     pubsub_create_topic(15);

//     printf("size of complete Pub struct: %zu\n", sizeof(struct Pub));
//     printf("size of single Topic struct: %zu\n", sizeof(p->tLink));

//     struct Topic *t = open_shm_segment(15);

//     pubsub_join(15);
//     printf("pid pub %d\n", t->pid_pub[0]);

//     pubsub_subscribe(15);
//     printf("pid sub %d\n", t->pid_sub[0][0]);

//     pubsub_publish(15, 100);
//     printf("msg published 0 %d\n", t->msg[0]);
//     pubsub_publish(15, 200);
//     printf("msg published 1 %d\n", t->msg[1]);
//     pubsub_publish(15, 300);
//     printf("msg published 2 %d\n", t->msg[2]);

//     printf("msg read 0 %d\n", pubsub_read(15));
//     printf("msg read 1 %d\n", pubsub_read(15));
//     printf("msg read 2 %d\n", pubsub_read(15));

//     pubsub_publish(15, 600);
//     printf("msg published 3 %d\n", t->msg[0]);
//     printf("msg read 3 %d\n", pubsub_read(15));
//     printf("msg read 4 %d\n", pubsub_read(15));
    

//     // pubsub_publish(3, 200);
//     // printf("msg publish 0 %d\n", t->msg[0]);
//     // printf("msg publish 1 %d\n", t->msg[1]);

//     // printf("msg read 0 %d\n", pubsub_read(3));
//     // printf("msg read 1 %d\n", pubsub_read(3));
//     // printf("msg read 2 %d\n", pubsub_read(3));

//     // pubsub_join(12);
//     // printf("%d\n", t->pid_pub[0]);

//     // printf("before publish %d\n", t->msg[t->msg_index]);
//     // pubsub_publish(12, 5028); // escreve no topico 11 criado anterior mente a msg 5028
//     // printf("after publish %d\n", t->msg[t->msg_index - 1]);

//     // pubsub_cancel(12);
//     // printf("before publish %d\n", t->msg[t->msg_index]);
//     // pubsub_publish(12, 666); // escreve no topico 11 criado anterior mente a msg 5028
//     // printf("after publish %d\n", t->msg[t->msg_index - 1]);

//     // pubsub_publish(12, 3000);
//     // pubsub_publish(12, 4666);
//     // printf("after publish %d\n", t->msg[1]);
//     // printf("after publish %d\n", t->msg[2]);
//     // printf("%d", destroy_shm_segment(11)); // se nao quiser excluir so usar o metodo close_shm_segment() que as infos ficam salvas;
  
//   return 0;
// }

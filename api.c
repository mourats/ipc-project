#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <pthread.h>
#include "structs.h"

#define TRUE 1
#define FALSE 0

struct Pub *p;
pthread_mutex_t mutex;

int pubsub_init() {
    p = (struct Pub*) malloc(sizeof(struct Pub*));
    p->pos_topic = 0;
    return 0;
}

key_t get_ftok() {
    char const *path = "shmfile";
    return ftok("shmfile", 51);
}

struct Topic * open_shm_segment(int topic_id) {
    // shmget returns an identifier in shmid
    int shm_id = shmget(get_ftok(), sizeof(struct Topic), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Erro shmget()");
        exit(1);
    }

    // shmat to attach and return the pointer to shared memory segment
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
    int mensagem_retorno = t->msg[t->msg_index];
    t->msg_index++;
    // sem_post(&mutex);
    pthread_mutex_unlock(&mutex);
    
    //detach from shared memory
    //return close_shm_segment(t);

    close_shm_segment(t);
    return mensagem_retorno;
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

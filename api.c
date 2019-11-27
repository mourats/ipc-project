#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
// #include <pthread.h>
#include <sys/sem.h>
#include "structs.h"

#define TRUE 1
#define FALSE 0
#define PUB_KEY 51
#define MUT_KEY 666
#define COND_KEY 999

// pthread_mutex_t mutex;
int semid_mut, semid_cond;

int semval;

struct sembuf mutex;
struct sembuf cond;

char *path_mut = "arquivo";
char *path_cond = "arquivo2";
// char *path_mem = "shmfile";

union {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
} arg_mut;

union {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
} arg_cond;

// GENERAL FUNCTIONS TO SEGMENT
key_t get_ftok(int key) {
    char const *path = "shmfile";
    return ftok("shmfile", key);
}

int get_shmid_segment(int key) {
    int shm_id;
    shm_id = shmget(get_ftok(key), sizeof(struct Pub), 0666 | IPC_CREAT);
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

// PUB OPEN SEGMENT
struct Pub * pub_open_shm_segment() {
    int shm_id = shmget(get_ftok(PUB_KEY), sizeof(struct Pub), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Erro shmget()");
        exit(1);
    }

    return ((struct Pub*) shmat(shm_id, NULL,0));
}

int pub_close_shm_segment(struct Pub *pub) {
    return shmdt(pub);
}

int open_semaforo(key_t key, char *path) {
    int sem_id;
    if((sem_id = semget(ftok(path, (key_t)key), 1, 0)) == -1) {
        perror("erro ao tentar abrir semáforo");
        exit(1);
    }
    printf("abrindo o semáforo %d\n", sem_id);

    return sem_id;
}

void atualiza_semaforo(int sem_op, int semid, struct sembuf sem) {
    sem.sem_num = 0;
    sem.sem_op = sem_op;
    sem.sem_flg = SEM_UNDO;
    if(semop(semid, &sem, 1) == -1) {
        perror("operação de decremento no semáforo não realizada");
        exit(1);
    }
}

// TOPIC OPEN SEGMENT
struct Topic * topic_open_shm_segment(int key) {
    int shm_id = shmget(key, sizeof(struct Topic), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Erro shmget()");
        exit(1);
    }

    return ((struct Topic*) shmat(shm_id, NULL,0));
}

int topic_close_shm_segment(struct Topic *topic) {
    return shmdt(topic);
}

// API
void fill_topics(struct Pub *p) {
    for(int i = 0; i < sizeof p->topics; i++)
        p->topics[i] =  -1;
}

int pubsub_init() {
    struct Pub *pub = pub_open_shm_segment();
    printf("Segmento de memoria numero: %d\n", get_shmid_segment(PUB_KEY));
    fill_topics(pub);

    if((semid_mut = semget(ftok(path_mut, (key_t)MUT_KEY), 1, IPC_CREAT|0666)) == -1) {
        perror("Impossível criar semáforo");
        exit(1);
    }
    printf("Criado o mutex com %d\n", semid_mut);

    if((semid_cond = semget(ftok(path_cond, (key_t)COND_KEY), 1, IPC_CREAT|0666)) == -1) {
        perror("Impossível criar semáforo");
        exit(1);
    }
    printf("Criado a condicional com %d\n", semid_cond);

    arg_mut.val = 1; //colocando 1 no semáforo
    if(semctl(semid_mut, 0, SETVAL, arg_mut) == -1) {
        perror("erro semctl SETVAL");
        exit(1);
    }

    arg_cond.val = 0; //colocando 0 na condicional
    if(semctl(semid_cond, 0, SETVAL, arg_cond) == -1) {
        perror("erro semctl SETVAL");
        exit(1);
    }

    if((semval = semctl(semid_mut, 0, GETVAL, arg_mut)) == -1) {
        perror("erro semctl GETVAL");
        exit(1);
    } else {
        printf("O valor do mutex é %d\n", semval);
    }

    if((semval = semctl(semid_cond, 0, GETVAL, arg_cond)) == -1) {
        perror("erro semctl GETVAL");
        exit(1);
    } else {
        printf("O valor da condicional é %d\n", semval);
    }

    pub_close_shm_segment(pub);
    return 0;
}

void fill_pids(struct Topic *t) {
    for(int i = 0; i < t->pubs_subs_count; i++) {
        t->pid_pub[i] = -1;
        t->pid_sub[i][0] = -1;
        t->pid_sub[i][1] = -1;
    }
}

int array_is_full(struct Pub * pub) {
    if (pub->topics_count > 0 && pub->topics_count == sizeof pub->topics / sizeof pub->topics[0])
        return 1;
    return 0;
}

int pubsub_create_topic(int topic_id) {
    // pthread_mutex_init(&mutex, NULL);
    struct Pub *pub = pub_open_shm_segment();
    struct Topic *t = topic_open_shm_segment(topic_id);

    if (array_is_full(pub)) return -1;

    t->pubs_subs_count = sizeof t->pid_pub / sizeof *t->pid_pub;
    t->msg_count = sizeof t->msg / sizeof *t->msg;
    t->msg_index = 0;
    t->id = topic_id;
    t->querem_ler = 0;
    fill_pids(t);

    pub->topics[pub->pos_topic] = t->id;
    pub->pos_topic++;
    pub_close_shm_segment(pub);

    return topic_close_shm_segment(t);
}

int pubsub_join(int topic_id) {
    struct Pub *pub = pub_open_shm_segment();
    struct Topic *t = topic_open_shm_segment(topic_id);
    semid_mut = open_semaforo(MUT_KEY, path_mut);

    if (t == NULL) return -1;

    //demanda de recurso
    atualiza_semaforo(-1, semid_mut, mutex);

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
        //liberando o recurso
        atualiza_semaforo(1, semid_mut, mutex);
        return 0;
    }
    
    //liberando o recurso
    atualiza_semaforo(1, semid_mut, mutex);

    pub_close_shm_segment(pub);
    topic_close_shm_segment(t);

    return pub_id;
}

int pubsub_subscribe(int topic_id) {
    struct Pub *pub = pub_open_shm_segment(topic_id);
    struct Topic *t = topic_open_shm_segment(topic_id);

    semid_mut = open_semaforo(MUT_KEY, path_mut);

    //decrementando do semáforo
    atualiza_semaforo(-1, semid_mut, mutex);

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
        //liberando o recurso
        atualiza_semaforo(1, semid_mut, mutex);
        return 0;
    }
    
    //liberando o recurso
    atualiza_semaforo(1, semid_mut, mutex);

    pub_close_shm_segment(pub);
    topic_close_shm_segment(t);

    return sub_id;
}

// um processo subscriber que vai cancelar a subscrição 
// feita para o tópico
int pubsub_cancel(int topic_id) {
    struct Pub *pub = pub_open_shm_segment(topic_id);
    struct Topic *t = topic_open_shm_segment(topic_id);
    semid_mut = open_semaforo(MUT_KEY, path_mut);

    //demanda de recurso do semáforo
    atualiza_semaforo(-1, semid_mut, mutex);

    pid_t pubsub_id = getpid();
    
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_sub[i][0] == pubsub_id) {
            t->pid_sub[i][0] = -1;
            break;
        }
    }
    
    //libera o semáforo
    atualiza_semaforo(1, semid_mut, mutex);

    pub_close_shm_segment(pub);
    topic_close_shm_segment(t);

    return pubsub_id;
}

int contain_sub(pid_t pid, struct Topic *t) {
    int contain = FALSE;
    for(int i = 0; i < t->pubs_subs_count; i++) {
        if(t->pid_sub[i][0] == pid) {
            contain = TRUE;
            break;
        }
    }

    return contain;
}

int pubsub_cancel_semid() {
    struct Pub *pub = pub_open_shm_segment();
    pid_t sub_id = getpid();

    for(int i = 0; i < sizeof pub->topics; i++) {
        struct Topic *t = topic_open_shm_segment(pub->topics[i]);
        if(contain_sub(sub_id, t)) {
            pubsub_cancel(pub->topics[i]);
            break;
        }
    }
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
    struct Pub *pub = pub_open_shm_segment(topic_id);
    struct Topic *t = topic_open_shm_segment(topic_id);
    semid_mut = open_semaforo(MUT_KEY, path_mut);
    semid_cond =  open_semaforo(COND_KEY, path_cond);
    
    //lock
    atualiza_semaforo(-1, semid_mut, mutex);

    pid_t pub_id = getpid();
    if(!contain_pub(pub_id, t)) {
        printf("error pubsub_publish\n");
        //unlock
        atualiza_semaforo(1, semid_mut, mutex);
        return 0;
    }

    // se for atingido o limite do buffer, espera até que todo mundo tenha lido
    // a última mensagem
    if(t->msg_index % t->msg_count == 0 && !did_everyone_read(t)) {
        printf("buffer cheio, tente mais tarde\n");
        //unlock
        atualiza_semaforo(1, semid_mut, mutex);
        return 0;
    }

    t->msg[t->msg_index % t->msg_count] = msg;
    int mensagem_retorno = t->msg[t->msg_index % t->msg_count];
    t->msg_index++;
    
    //unlock
    atualiza_semaforo(1, semid_mut, mutex);

    if(t->querem_ler > 0) {
        atualiza_semaforo(t->querem_ler, semid_cond, cond);

        t->querem_ler = 0;
        arg_cond.val = 0;
        if(semctl(semid_cond, 0, SETVAL, arg_cond) == -1) {
            perror("erro semctl SETVAL");
            exit(1);
        }
    }

    pub_close_shm_segment(pub);
    topic_close_shm_segment(t);

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
    struct Pub *pub = pub_open_shm_segment(topic_id);
    struct Topic *t = topic_open_shm_segment(topic_id);
    semid_mut = open_semaforo(MUT_KEY, path_mut);
    semid_cond = open_semaforo(COND_KEY, path_cond);

    //lock
    atualiza_semaforo(-1, semid_mut, mutex);

    pid_t sub_id = getpid();
    int pos_sub = getpos_sub(sub_id, t);
    if(pos_sub == -1) {
        printf("error pubsub_read\n");
        //unlock
        atualiza_semaforo(1, semid_mut, mutex);
        return 0;
    }

    int index_msg_read = t->pid_sub[pos_sub][1];

    while(index_msg_read >= t->msg_index) {
        printf("sem mensagens\n");
        //unlock
        atualiza_semaforo(1, semid_mut, mutex);
        t->querem_ler++;
        atualiza_semaforo(-1, semid_cond, cond);
        atualiza_semaforo(-1, semid_mut, mutex);
    }
    
    int msg = t->msg[index_msg_read % t->msg_count];
    t->pid_sub[pos_sub][1]++;
    
    //unlock
    atualiza_semaforo(1, semid_mut, mutex);

    pub_close_shm_segment(pub);
    topic_close_shm_segment(t);

    return msg;
}

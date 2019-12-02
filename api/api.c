#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <sys/sem.h>
#include "structs.h"

#define TRUE 1
#define FALSE 0
#define PUB_KEY 52  

key_t get_ftok(int key) {
    char const *path = "files/shmfile";
    return ftok(path, key);
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
    if((sem_id = semget(ftok(path, (key_t)key), 1, IPC_CREAT|0666)) == -1) {
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

int contain_topic(int topic_id) {
    int contain = FALSE;
    struct Pub* pub = pub_open_shm_segment();

    for(int i = 0; i < sizeof pub->topics / sizeof *pub->topics; i++) {
        if(pub->topics[i] == topic_id) {
            contain = TRUE;
            break;
        }
    }
    return contain;
}

struct Topic * topic_open_shm_segment(int key) {
    int shm_id = shmget(key, sizeof(struct Topic), 0666|IPC_CREAT);
    if (shm_id == -1) {
        perror("Erro shmget() topic");
        exit(1);
    }

    return ((struct Topic*) shmat(shm_id, NULL,0));
}

int topic_close_shm_segment(struct Topic *topic) {
    return shmdt(topic);
}

void fill_topics(struct Pub *p) {
    for(int i = 0; i < sizeof p->topics; i++)
        p->topics[i] =  -1;
}

int pubsub_init() {
    struct Pub *pub = pub_open_shm_segment();
    printf("Segmento de memoria numero: %d\n", get_shmid_segment(PUB_KEY));
    fill_topics(pub);
    pub->pos_topic = 0;

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
    if(contain_topic(topic_id)) {
        perror("Tópico já existe");
        return 0;
    }

    struct Pub *pub = pub_open_shm_segment();
    struct Topic *t = topic_open_shm_segment(topic_id);

    if (array_is_full(pub)) return -1;

    t->pubs_subs_count = sizeof t->pid_pub / sizeof *t->pid_pub;
    t->msg_count = sizeof t->msg / sizeof *t->msg;
    t->msg_index = 0;
    t->id = topic_id;
    t->querem_ler = 0;
    t->querem_escrever = 0;
    fill_pids(t);
    t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");
    t->semid_cond_read = open_semaforo(topic_id, "files/file_shm_cond_read");
    t->semid_cond_pub = open_semaforo(topic_id, "files/file_shm_cond_pub");

    t->arg_mut.val = 1;
    if(semctl(t->semid_mut, 0, SETVAL, t->arg_mut) == -1) {
        perror("erro semctl SETVAL 0");
        exit(1);
    }

    t->arg_cond_read.val = 0;
    if(semctl(t->semid_cond_read, 0, SETVAL, t->arg_cond_read) == -1) {
        perror("erro semctl SETVAL 1");
        exit(1);
    }

    t->arg_cond_pub.val = 0;
    if(semctl(t->semid_cond_pub, 0, SETVAL, t->arg_cond_pub) == -1) {
        perror("erro semctl SETVAL 2");
        exit(1);
    }

    pub->topics[pub->pos_topic] = t->id;
    pub->pos_topic++;
    pub_close_shm_segment(pub);

    return topic_close_shm_segment(t);
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

int valida_topico(int topic_id) {
    if(!contain_topic(topic_id)) {
        printf("Tópico não existe.\n");
        return 0;
    }
    return 1;
}

int existe_em_topico(pid_t pid, int topic_id) {
    struct Pub *pub = pub_open_shm_segment();
    int existe = FALSE;

    for(int i = 0; i < sizeof pub->topics / sizeof *pub->topics; i++) {
        struct Topic *t = topic_open_shm_segment(pub->topics[i]);
        if(contain_pub(pid, t) | contain_sub(pid, t)) {
            existe = TRUE;
            break;
        }
    }

    return existe;
}

int pubsub_join(int topic_id) {
    if(valida_topico(topic_id)) {
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");

      if (t == NULL) return -1;

      atualiza_semaforo(-1, t->semid_mut, t->mutex);
      pid_t pub_id = getpid();
    
      if(existe_em_topico(pub_id, topic_id)) {
          perror("Estais em um tópico");
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }

      int fit = FALSE;
      for(int i = 0; i < t->pubs_subs_count; i++) {
          if(t->pid_pub[i] == -1) {
              t->pid_pub[i] = pub_id;
              fit = TRUE;
              break;
          }
      }

      if(!fit) {
          perror("erro pubsub_join");
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }
      
      atualiza_semaforo(1, t->semid_mut, t->mutex);

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return pub_id;
    }
}

int pubsub_subscribe(int topic_id) {
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");

      atualiza_semaforo(-1, t->semid_mut, t->mutex);
      pid_t sub_id = getpid();

      if(existe_em_topico(sub_id, topic_id)) {
          perror("Estais em um tópico");
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }

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
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }
      
      atualiza_semaforo(1, t->semid_mut, t->mutex);

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return sub_id;
    }
}

int pubsub_cancel(int topic_id) {
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");

      atualiza_semaforo(-1, t->semid_mut, t->mutex);

      pid_t pubsub_id = getpid();
      
      if(contain_pub(pubsub_id, t)) {
          for(int i = 0; i < t->pubs_subs_count; i++) {
              if(t->pid_pub[i] == pubsub_id) {
                  t->pid_pub[i] = -1;
                  break;
              }
          }
      } else if(contain_sub(pubsub_id, t)) {
          for(int i = 0; i < t->pubs_subs_count; i++) {
              if(t->pid_sub[i][0] == pubsub_id) {
                  t->pid_sub[i][0] = -1;
                  break;
              }
          }
      }
      
      atualiza_semaforo(1, t->semid_mut, t->mutex);

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return pubsub_id;
    }
}

int pubsub_cancel_semid() {
    struct Pub *pub = pub_open_shm_segment();
    pid_t pid = getpid();

    for(int i = 0; i < sizeof pub->topics; i++) {
        struct Topic *t = topic_open_shm_segment(pub->topics[i]);
        if(contain_sub(pid, t) | contain_pub(pid, t) ) {
            pubsub_cancel(pub->topics[i]);
            break;
        }
    }
}

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

int pubsub_publish(int topic_id, int msg) {
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");
      t->semid_cond_read =  open_semaforo(topic_id, "files/file_shm_cond_read");
      t->semid_cond_pub =  open_semaforo(topic_id, "files/file_shm_cond_pub");
      
      //lock
      atualiza_semaforo(-1, t->semid_mut, t->mutex);

      pid_t pub_id = getpid();
      if(!contain_pub(pub_id, t)) {
          printf("error pubsub_publish\n");
          //unlock
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }

      if(t->msg_index % t->msg_count == 0 && !did_everyone_read(t)) {
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          printf("buffer cheio\n");
          t->querem_escrever++;
          atualiza_semaforo(-1, t->semid_cond_pub, t->cond_pub);
          atualiza_semaforo(-1, t->semid_mut, t->mutex);
      }

      t->msg[t->msg_index % t->msg_count] = msg;
      int mensagem_retorno = t->msg[t->msg_index % t->msg_count];
      t->msg_index++;
      
      //unlock
      atualiza_semaforo(1, t->semid_mut, t->mutex);

      printf("querem ler %d\n", t->querem_ler);
      if(t->querem_ler > 0) {
          atualiza_semaforo(t->querem_ler, t->semid_cond_read, t->cond_read);

          t->querem_ler = 0;
          t->arg_cond_read.val = 0;
          if(semctl(t->semid_cond_read, 0, SETVAL, t->arg_cond_read) == -1) {
              perror("erro semctl SETVAL");
              exit(1);
          }
      }

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return mensagem_retorno;
    }
}

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
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");
      t->semid_cond_read =  open_semaforo(topic_id, "files/file_shm_cond_read");
      t->semid_cond_pub =  open_semaforo(topic_id, "files/file_shm_cond_pub");

      //lock
      atualiza_semaforo(-1, t->semid_mut, t->mutex);

      pid_t sub_id = getpid();
      int pos_sub = getpos_sub(sub_id, t);
      if(pos_sub == -1) {
          printf("Error pubsub_read\n");
          //unlock
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }

      int index_msg_read = t->pid_sub[pos_sub][1];

      while(index_msg_read >= t->msg_index) {
          atualiza_semaforo(1, t->semid_mut, t->mutex);
          printf("Sem mensagens.\n");
          t->querem_ler++;
          atualiza_semaforo(-1, t->semid_cond_read, t->cond_read);
          atualiza_semaforo(-1, t->semid_mut, t->mutex);
      }
      
      int msg = t->msg[index_msg_read % t->msg_count];
      t->pid_sub[pos_sub][1]++;
      
      //unlock
      atualiza_semaforo(1, t->semid_mut, t->mutex);

      if(did_everyone_read(t) && t->querem_escrever > 0) {
          atualiza_semaforo(t->querem_escrever, t->semid_cond_pub, t->cond_pub);

          t->querem_escrever = 0;
          t->arg_cond_pub.val = 0;
          if(semctl(t->semid_cond_pub, 0, SETVAL, t->arg_cond_pub) == -1) {
              perror("erro semctl SETVAL");
              exit(1);
          }
          return msg;
      }

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return msg;
    }
}

char* pubsub_list_topics(){
    struct Pub *pub = pub_open_shm_segment();
    int topicInt;
    int flag = 1;
    printf("Lista de tópicos: \n");
    for(int i = 0; i < sizeof pub->topics / sizeof *pub->topics; i++){
        topicInt = pub->topics[i];
        if(topicInt != -1) {
            printf("Tópico id: %d\n", topicInt);
            flag = 0;
        }
    }
    if(flag) {
        printf("Nenhum tópico encontrado. \n");
    }
    pub_close_shm_segment(pub);
}

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
        printf("Error shmget().\n");
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
        printf("Erro shmget().\n");
        exit(1);
    }

    return ((struct Pub*) shmat(shm_id, NULL,0));
}

int pub_close_shm_segment(struct Pub *pub) {
    return shmdt(pub);
}

// cria um conjunto de semáforos ou obtém um conjunto de semáforos já existentes, retornando o seu id
// tem como parametros uma chave e o caminho para um arquivo
int open_semaforo(key_t key, char *path) {
    int sem_id;
    // semget
    // o primeiro argumento é a chave, 
    // o segundo argumento é o número de semáforos do conjunto, no caso será 1
    // o terceiro argumento é uma flag especificando os direitos de acesso
    if((sem_id = semget(ftok(path, (key_t)key), 1, IPC_CREAT|0666)) == -1) {
        printf("Erro ao tentar abrir semáforo.\n");
        return 0;
    }
    return sem_id;
}


// efetua operações sobre o semáforo
// o primeiro argumento é um inteiro, que se for negativo será um pedido de recurso e se for positivo será uma restituição de recurso
// o segundo argumento é o id do conjunto de semáforos
// o terceiro argumento é uma estrutura que especifica a posição, a operação e as flags de controle da operação sobre o semáforo
void efetua_operacao_semaforo(int sem_op, int semid, struct sembuf sem) {
    // posição do semáforo, como cada conjunto de semáforos tem apenas um semáforo, a posição é a primeira/0
    sem.sem_num = 0;
    sem.sem_op = sem_op;
    sem.sem_flg = SEM_UNDO;
    if(semop(semid, &sem, 1) == -1) {
        printf("Operação de decremento no semáforo não realizada.\n");
    }
}

// atualiza o valor de um semáforo
// o primeiro argumento é o id do semáforo
// o segundo argumento é a posição do semáforo no conjunto de semáforos
// o terceiro argumento é uma variável do tipo union semun que contém o valor do semáforo
// o quarto argumento é o valor a ser atualizado no semáforo
void atualiza_valor_semaforo(int id, int pos, union semun arg, int valor) {
    arg.val = valor;
    if(semctl(id, pos, SETVAL, arg) == -1) {
        printf("Erro ao atualizar valor do semáforo\n");
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
        printf("Error shmget() topic.\n");
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

// cria um tópico passando o id do tópico como parâmetro
int pubsub_create_topic(int topic_id) {
    if(contain_topic(topic_id)) {
        printf("Tópico já existe.\n");
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
    // abre os três semáforos e guarda o id de cada um deles na variável correspondente
    t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");
    t->semid_cond_read = open_semaforo(topic_id, "files/file_shm_cond_read");
    t->semid_cond_pub = open_semaforo(topic_id, "files/file_shm_cond_pub");

    // inicializa o valor do semáforo mutex com 1
    atualiza_valor_semaforo(t->semid_mut, 0, t->arg_mut, 1);

    // inicializa o valor do semáforo da variável condicional de leitura com 0
    atualiza_valor_semaforo(t->semid_cond_read, 0, t->arg_cond_read, 0);

    // inicializa o valor do semáforo da variável condicional de escrita com 0
    atualiza_valor_semaforo(t->semid_cond_read, 0, t->arg_cond_read, 0);

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

// junta um processo que enviará mensagens ao tópico passando o id do tópico como parâmetro
int pubsub_join(int topic_id) {
    if(valida_topico(topic_id)) {
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      // abre o mutex e guarda o id na variável correspondente
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");

      if (t == NULL) return -1;

      // bloqueia o mutex
      efetua_operacao_semaforo(-1, t->semid_mut, t->mutex);
      pid_t pub_id = getpid();
    
      if(existe_em_topico(pub_id, topic_id)) {
          printf("Estais em um tópico.\n");
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
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
          printf("Error in pubsub_join.\n");
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }
      
      // libera o mutex
      efetua_operacao_semaforo(1, t->semid_mut, t->mutex);

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return pub_id;
    }
}

// junta um processo que receberá mensagens ao tópico passando o id do tópico como parâmetro
int pubsub_subscribe(int topic_id) {
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      // abre o mutex e guarda o id na variável correspondente
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");

      // bloqueia o mutex
      efetua_operacao_semaforo(-1, t->semid_mut, t->mutex);
      pid_t sub_id = getpid();

      if(existe_em_topico(sub_id, topic_id)) {
          printf("Estais em um tópico.\n");
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
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
          printf("Error pubsub_subscribe\n");
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }
      
      // libera o mutex
      efetua_operacao_semaforo(1, t->semid_mut, t->mutex);

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return sub_id;
    }
}

// cancela a subscrição de um processo ao tópico passando o id do tópico como parâmetro
int pubsub_cancel(int topic_id) {
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      // abre o mutex e guarda o id na variável correspondente
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");

      // bloqueia o mutex
      efetua_operacao_semaforo(-1, t->semid_mut, t->mutex);

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
      
      // libera o mutex
      efetua_operacao_semaforo(1, t->semid_mut, t->mutex);

      pub_close_shm_segment(pub);
      topic_close_shm_segment(t);

      return pubsub_id;
    }
}

int pubsub_cancel_semid() {
    struct Pub *pub = pub_open_shm_segment();
    pid_t pid = getpid();

    for(int i = 0; i < sizeof pub->topics; i++) {
        if(pub->topics[i] != -1) {
            struct Topic *t = topic_open_shm_segment(pub->topics[i]);
            if(contain_sub(pid, t) | contain_pub(pid, t) ) {
                pubsub_cancel(pub->topics[i]);
                break;
        	}
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

// um processo publisher envia uma mensagem ao tópico cujo id é passado como parâmetro
int pubsub_publish(int topic_id, int msg) {
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      // abre os três semáforos e guarda os ids nas variáveis correspondentes
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");
      t->semid_cond_read =  open_semaforo(topic_id, "files/file_shm_cond_read");
      t->semid_cond_pub =  open_semaforo(topic_id, "files/file_shm_cond_pub");
      
      // bloqueia o mutex
      efetua_operacao_semaforo(-1, t->semid_mut, t->mutex);

      pid_t pub_id = getpid();
      if(!contain_pub(pub_id, t)) {
          printf("Error pubsub_publish.\n");
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }

      // caso o buffer esteja cheio, o processo é bloqueado até que todos os processos
      // subscribers tenham lido todas as mensagens
      while(t->msg_index % t->msg_count == 0 && !did_everyone_read(t)) {
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
          printf("Buffer cheio.\n");
          // atualiza quantos processos estão esperando para escrever
          t->querem_escrever++;
          // bloqueia o semáforo de escrita 
          efetua_operacao_semaforo(-1, t->semid_cond_pub, t->cond_pub);
          // bloqueia o mutex
          efetua_operacao_semaforo(-1, t->semid_mut, t->mutex);
      }

      t->msg[t->msg_index % t->msg_count] = msg;
      int mensagem_retorno = t->msg[t->msg_index % t->msg_count];
      t->msg_index++;
      
      // libera o mutex
      efetua_operacao_semaforo(1, t->semid_mut, t->mutex);

      // acorda eventuais processos subscribers que estejam bloqueados esperando por mensagens
      if(t->querem_ler > 0) {
          // libera os semáforos de leitura que estiverem bloqueados
          efetua_operacao_semaforo(t->querem_ler, t->semid_cond_read, t->cond_read);

          // atualiza a quantidade de processos bloqueados para leitura em 0
          t->querem_ler = 0;
          // altera o valor do semáforo de leitura para 0
          atualiza_valor_semaforo(t->semid_cond_read, 0, t->arg_cond_read, 0);
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

// um processo subscriber recebe uma mensagem do tópico cujo id é passado como parâmetro
int pubsub_read(int topic_id) {
    if(valida_topico(topic_id)){
      struct Pub *pub = pub_open_shm_segment();
      struct Topic *t = topic_open_shm_segment(topic_id);
      // abre os três semáforos e guarda os id's nas variáveis correspondentes
      t->semid_mut = open_semaforo(topic_id, "files/file_shm_mutex");
      t->semid_cond_read =  open_semaforo(topic_id, "files/file_shm_cond_read");
      t->semid_cond_pub =  open_semaforo(topic_id, "files/file_shm_cond_pub");

      // bloqueia o mutex
      efetua_operacao_semaforo(-1, t->semid_mut, t->mutex);

      pid_t sub_id = getpid();
      int pos_sub = getpos_sub(sub_id, t);
      if(pos_sub == -1) {
          printf("Error pubsub_read\n");
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
          return 0;
      }

      int index_msg_read = t->pid_sub[pos_sub][1];

      // caso não haja novas mensagens, o processo é bloqueado 
      // até que chegue alguma nova mensagem no tópico
      while(index_msg_read >= t->msg_index) {
          // libera o mutex
          efetua_operacao_semaforo(1, t->semid_mut, t->mutex);
          printf("Sem mensagens.\n");
          // atualiza quantos processos estão esperando para ler
          t->querem_ler++;
          // bloqueia o semáforo de leitura
          efetua_operacao_semaforo(-1, t->semid_cond_read, t->cond_read);
          // bloqueia o mutex
          efetua_operacao_semaforo(-1, t->semid_mut, t->mutex);
      }
      
      int msg = t->msg[index_msg_read % t->msg_count];
      t->pid_sub[pos_sub][1]++;
      
      // libera o mutex
      efetua_operacao_semaforo(1, t->semid_mut, t->mutex);

      // acorda eventuais processos publishers que estejam boqueados esperando que 
      // todos os processos subscribers tenham lido todas as mensagens
      if(did_everyone_read(t) && t->querem_escrever > 0) {
          // libera os semáforos de escrita que estiverem bloqueados
          efetua_operacao_semaforo(t->querem_escrever, t->semid_cond_pub, t->cond_pub);
          // atualiza a quantidade de processos bloqueados para escrita em 0
          t->querem_escrever = 0;
          // altera o valor do semáforo de escrita em 0
          atualiza_valor_semaforo(t->semid_cond_pub, 0, t->arg_cond_pub, 0);
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

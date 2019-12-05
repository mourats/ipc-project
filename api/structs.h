#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/sem.h>
#include <sys/types.h>

// função que guardará o valor de cada semáforo
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
};

// tópico que contém as mensagens publicadas além dos ids de seus publishers e subscribers 
// cada tópico também contém 3 semáforos, um para simular um mutex, outro para simular uma 
// variável condicional de escrita e outro uma variável condicional de leitura
struct Topic {
    int id;
    int pubs_subs_count; // tamanho de cada array de publishers e subscribers
    int pid_pub[50]; // array com os pids dos publishers
    int pid_sub[50][2]; // array com os pids dos subscribers e a posição da última mensagem lida para cada subscriber
    int msg_count; // total de mensagens
    int msg[5]; // array com as mensagens
    int msg_index; // posição da última mensagem publicada
    int querem_ler; // indica quantos processos estão bloqueados esperando para ler
    int querem_escrever;  // indica quantos processos estão bloqueados esperando para escrever

    // id's dos três semáforos que serão usados
    int semid_mut, semid_cond_read, semid_cond_pub;
    int semval;
    // estrutura para efetuar operações em cada um dos semáforos
    struct sembuf mutex, cond_read, cond_pub;
    // variáveis que guardarão o valor de cada um dos semáforos
    union semun arg_mut, arg_cond_read, arg_cond_pub;
};

struct Pub {
    int topics[5]; // array of topics ids
    int topics_count; // topics counter
    int pos_topic; // free topic index
};

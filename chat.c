#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "structs.h"

int get_msg(struct Topic * t) {
    int pos_msg = t->msg_count - 1;
    if(t->msg_index % t->msg_count != 0) 
        pos_msg = (t->msg_index % t->msg_count) - 1;
    
    return t->msg[pos_msg];
}

int main(void) {
    while(1) {
        int opcao;
        printf("Opções: \n");
        printf("1- Iniciar pub\n");
        printf("2- Criar tópico\n");
        printf("3- Join um tópico\n");
        printf("4- Subscribe um tópico\n");
        printf("5- Publish\n");
        printf("6- Read\n");
        printf("7- Cancelar subscrição\n");
        printf("8- Sair\n");
        scanf("%d", &opcao);

        int id;
        int msg;
        int pos;
        struct Topic * t;
        switch (opcao)
        {
        case 1:
            pubsub_init();
            break;
        case 2:
            printf("Digite o id do tópico\n");
            scanf("%d", &id);
            pubsub_create_topic(id);
            t = open_shm_segment(id);
            printf("shmid do tópico criado: %d\n", get_shmid_segment(id));
            break;
        case 3:
            printf("Digite o id do tópico\n");
            scanf("%d", &id);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            pubsub_join(id);
            pos = getpos_pub(getpid(), t);
            printf("pub pid: %d\n", t->pid_pub[pos]);
            break;
        case 4:
            printf("Digite o id do tópico\n");
            scanf("%d", &id);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            pubsub_subscribe(id);
            pos = getpos_sub(getpid(), t);
            printf("sub pid: %d\n", t->pid_sub[pos][0]);
            break;
        case 5:
            printf("Digite o id do tópico e a mensagem\n");
            scanf("%d %d", &id, &msg);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            pubsub_publish(id, msg);
            printf("mensagem publicada: %d\n", get_msg(t));
            break;
        case 6:
            printf("Digite o id do tópico\n");
            scanf("%d", &id);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            pubsub_read(id);
            break;
        case 7:
            printf("Digite o id do tópico\n");
            scanf("%d", &id);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            pos = getpos_sub(getpid(), t);
            printf("sub pid antes: %d\n", t->pid_sub[pos][0]);
            pubsub_cancel(id);
            printf("sub pid depois: %d\n", t->pid_sub[pos][0]);
            break;
        default:
            return 0;
        }
    }
    // pubsub_init();
    // pubsub_create_topic(12);
    // pubsub_join(12);
    // pubsub_subscribe(12);

    // printf("lendo mensagem: %d\n", pubsub_read(12));

    // pubsub_publish(12, 500);
    // pubsub_publish(12, 250);
    // printf("lendo mensagem: %d\n", pubsub_read(12));
    // imprintf("lendo mensagem: %d\n", pubsub_read(12));

    // pubsub_cancel(int topic_id);
    // pubsub_publish(int topic_id, int msg);
    // pubsub_read(int topic_id);
    return 0;

}

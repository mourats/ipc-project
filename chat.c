#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "structs.h"

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
        printf("7- Sair\n");
        scanf("%d", &opcao);

        int id;
        int msg;
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
            printf("pub pid: %d\n", t->pid_pub[0]);
            break;
        case 4:
            printf("Digite o id do tópico\n");
            scanf("%d", &id);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            pubsub_subscribe(id);
            printf("sub pid: %d\n", t->pid_sub[0][0]);
            break;
        case 5:
            printf("Digite o id do tópico e a mensagem\n");
            scanf("%d %d", &id, &msg);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            pubsub_publish(id, msg);
            printf("mensagem publicada: %d\n", t->msg[t->msg_index - 1]);
            break;
        case 6:
            printf("Digite o id do tópico\n");
            scanf("%d", &id);
            t = open_shm_segment(id);
            printf("shmid do tópico: %d\n", get_shmid_segment(id));
            printf("mensagem lida: %d\n", pubsub_read(id));
            break;
        default:
            return 0;
        }

        // printf("Quer iniciar o pub? 1- sim 2- não\n");
        // scanf("%d", &arm);
        
        // if(arm == 1) {
        //     struct Pub * p = pubsub_init();
        //     printf("%d\n", p->id);
        // } else {
        //     printf("Quer criar um tópico? 1- sim 2- não\n");
        //     scanf("%d", &arm);

        //     if(arm == 1) {
        //         pubsub_create_topic(32);
        //         struct Topic *t = open_shm_segment(32);
        //         printf("tópico criado: %d\n", t->id);
        //         printf("%d\n", get_shmid_segment(32));
        //     } else {
        //         struct Topic *t = open_shm_segment(32);
        //         printf("abri o tópico de id %d\n", t->id);
        //         printf("%d\n", get_shmid_segment(32));
                
        //         printf("join ou subscribe 1- join 2- subscribe\n");
        //         scanf("%d", &arm);

        //         if(arm == 1) {
        //             printf("pid pub %d\n", t->pid_pub[0]);
        //             pubsub_join(32);
        //             printf("pid pub %d\n", t->pid_pub[0]);
        //             int i;
        //             scanf("%d", &i);
        //             pubsub_publish(32, 100);
        //             printf("msg published 0 %d\n", t->msg[0]);
        //         } else {
        //             printf("pid sub %d\n", t->pid_sub[0][0]);
        //             pubsub_subscribe(32);
        //             printf("pid sub %d\n", t->pid_sub[0][0]);
        //             pubsub_read(32);
        //             int j;
        //             scanf("%d", &j);
        //             printf("msg read 0 %d\n", pubsub_read(32));
        //         }
        //     }
        // }
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

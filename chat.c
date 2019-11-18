#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"
#include "api.h"

int options() {
    printf("======= Opções: =======\n");
    printf("0- Imprimir opções\n");
    printf("1- Iniciar pub\n");
    printf("2- Criar tópico\n");
    printf("3- Join um tópico\n");
    printf("4- Subscribe um tópico\n");
    printf("5- Publish\n");
    printf("6- Read\n");
    printf("7- Cancelar subscrição\n");
    printf("8- Sair\n");
    printf("========================\n");
    return 0;
}

int main(void) {
    options();
    while(1) {
        int opcao;        
        printf("Opção: ");
        scanf("%d", &opcao);

        int id;
        int msg;
        int pos;
        struct Topic * t;
        switch (opcao)
        {
        case 0:
            options();
            break;
        case 1: // inicia pub
            pubsub_init(); // aqui é onde devemos printar o id do segmento
            break;
        case 2: // cria topico
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            pubsub_create_topic(id);
            t = open_shm_segment(id);
            printf("shmid do tópico criado: %d\n", get_shmid_segment(id));
            break;
        case 3: // join topico
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            t = open_shm_segment(id);
            pubsub_join(id);
            pos = getpos_pub(getpid(), t);
            printf("pub pid: %d\n", t->pid_pub[pos]);
            break;
        case 4: // se inscreve no topico
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            t = open_shm_segment(id);
            pubsub_subscribe(id);
            pos = getpos_sub(getpid(), t);
            printf("sub pid: %d\n", t->pid_sub[pos][0]);
            break;
        case 5: // publica nova mensagem
            printf("Digite o id do tópico e a mensagem: ");
            scanf("%d %d", &id, &msg);
            printf("mensagem publicada: %d\n", pubsub_publish(id, msg));
            break;
        case 6: // ler mensagens
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            printf("nova mensagem: %d\n", pubsub_read(id));
            break;
        case 7: // cancelar subscricao
            printf("Digite o id do tópico: ");
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
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "api/api.h"

void sighandler(int signum) {
    pubsub_cancel_semid();
    printf("\nSaindo do chat, tchau...\n");
    exit(1);
}

int options() {
    printf("======= Opções: =======\n");
    printf("0- Iniciar Pub\n");
    printf("1- Listar tópicos\n");
    printf("2- Criar tópico\n");
    printf("3- Join um tópico\n");
    printf("4- Subscribe um tópico\n");
    printf("5- Publish\n");
    printf("6- Read\n");
    printf("7- Cancelar subscrição\n");
    printf("8- Imprimir opções\n");
    printf("9- Sair\n");
    printf("========================\n");
    return 0;
}

int main(void) {
    signal(SIGINT, sighandler);
    options();
    while(1) {
        int opcao;        
        printf("Opção: ");
        scanf("%d", &opcao);

        int id;
        int msg;
        int pos;
        switch (opcao)
        {
        case 0:
            pubsub_init();
            options();
            break;
        case 1: // listar topicos
            pubsub_list_topics();
            break;
        case 2: // cria topico
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            pubsub_create_topic(id);
            options();
            break;
        case 3: // join topico
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            pubsub_join(id);
            options();
            break;
        case 4: // se inscreve no topico
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            pubsub_subscribe(id);
            options();
            break;
        case 5: // publica nova mensagem
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            printf("Digite a mensagem: ");
            scanf("%d", &msg);
            printf("Mensagem publicada: %d\n", pubsub_publish(id, msg));
            break;
        case 6: // ler mensagens
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            printf("Nova mensagem: %d\n", pubsub_read(id));
            break;
        case 7: // cancelar subscricao
            printf("Digite o id do tópico: ");
            scanf("%d", &id);
            pubsub_cancel(id);
            options();
            break;
        case 8: // listar opções
            options();
            break;
        case 9: // sair
            printf("Saindo.\n");
            sighandler(0);
        default:
            printf("Opção inválida.\n");
            options();
            break;
        }
    }
    return 0;
}

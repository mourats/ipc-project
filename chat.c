#include <stdio.h>
#include <stdlib.h>
#include "api.h"

int main(void) {
    pubsub_init();
    pubsub_create_topic(12);
    pubsub_join(12);
    pubsub_subscribe(12);

    printf("lendo mensagem: %d\n", pubsub_read(12));

    pubsub_publish(12, 500);
    pubsub_publish(12, 250);
    printf("lendo mensagem: %d\n", pubsub_read(12));
    printf("lendo mensagem: %d\n", pubsub_read(12));

    // pubsub_cancel(int topic_id);
    // pubsub_publish(int topic_id, int msg);
    // pubsub_read(int topic_id);
    return 0;
}
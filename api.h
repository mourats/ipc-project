extern int pubsub_init();
extern int pubsub_create_topic(int topic_id);
extern int pubsub_join(int topic_id);
extern int pubsub_subscribe(int topic_id);
extern int pubsub_cancel(int topic_id);
extern int pubsub_cancel_semid();
extern int pubsub_publish(int topic_id, int msg);
extern int pubsub_read(int topic_id);

#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
  
int main() 
{
    int shmd_id;
    key_t key;
    int size = 1024;
    char const *path = "shmfile";
    #define KEY 65

    struct Data {
        int id;
        int arr[2];
    };
    struct Data *p;

    // ftok to generate unique key 
    key = ftok(path, KEY);
  
    // shmget returns an identifier in shmid 
    shmd_id = shmget(key, sizeof(struct Data), 0666 | IPC_CREAT);
  
    // shmat to attach to shared memory
    p = (struct Data*) shmat(shmd_id, NULL,0);

    p->id = 5;
    p->arr[0] = 3;
    p->arr[1] = 2;
    p->arr[2] = 1;

    printf("Identificador do segmento: %d\n", shmd_id);
    
    //detach from shared memory  
    shmdt(p);
  
    return 0;
} 


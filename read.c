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

    printf("struct id is: %d\n", p->id);

    int i = 0;
    while(i != 3) {
        printf("value of struct data in index %d is: %d\n", i, p->arr[i]);
        i++;
    }

    printf("Identificador do segmento: %d\n", shmd_id);
    
    //detach from shared memory  
    shmdt(p);

    // destroy the shared memory 
    //shmctl(shmid,IPC_RMID,NULL); 
  
    return 0; 
} 


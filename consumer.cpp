#include <iostream>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <random>
#include <stdlib.h>
#include <unistd.h>

#define SHM_NAME "/shared_buffer"
#define SEM_NAME "/shared_sem"

typedef struct {
    string commname;
    double price;
} SharedBuffer;

using namespace std;
int main(int argc, char* argv[]){
    int size=atoi(argv[1]);

    sem_t mutex;
    sem_t *buffer = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }
    ftruncate(shm_fd, size);

    SharedBuffer *buff=static_cast<SharedBuffer *>(mmap(NULL, size*sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)); //normal func(no cast) doesnt work in c++ :( only c
    close(shm_fd);

    while(true){
        printf("+--------------------------------------+\n");
        printf("| Currency  |   Price   |   AvgPrice   |\n");
        printf("+--------------------------------------+\n");
        // !!critical section!!
        sem_wait(&mutex);
        while(true){
            printf("| ALUMINIUM |   199.00  |   249.00     |\n");

        }
        sem_post(&mutex);
        printf("+--------------------------------------+\n");
    }

    return 0;
}
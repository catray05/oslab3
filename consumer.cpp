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
    sem_t mutex;
    char name[10];
    double price;
} SharedBuffer;

using namespace std;
int main(int argc, char* argv[]){
    int size=atoi(argv[1]);

    sem_t *buffer = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }
    ftruncate(shm_fd, size);

    SharedBuffer *buff=static_cast<SharedBuffer *>(mmap(NULL, size*sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)); //normal func(no cast) doesnt work in c++ :( only c
    if (buff == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    close(shm_fd);
    double avg;
    while(true){
        sem_wait(buffer);
        printf("+--------------------------------------+\n");
        printf("| Currency  |   Price   |   AvgPrice   |\n");
        printf("+--------------------------------------+\n");
        // !!critical section!!
        sem_wait(&buff->mutex);
        while(true){
            printf("| %s |   %f  |   %f     |\n",buff->name,buff->price,avg);

        }
        sem_post(&buff->mutex);
        printf("+--------------------------------------+\n");
    }

    return 0;
}
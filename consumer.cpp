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
    char name[11];
    double price;
    int elements;
    sem_t empty;
    sem_t full;
} SharedBuffer;

using namespace std;
int main(int argc, char* argv[]){
    int size=atoi(argv[1]);
    double avg;

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
    fprintf(stdout,"+--------------------------------------+\n");
    fprintf(stdout,"| Currency  |   Price   |   AvgPrice   |\n");
    fprintf(stdout,"+--------------------------------------+\n");
    sem_wait(buffer);
    while(buff->elements){
        // !!critical section!!
        sem_wait(&buff->mutex);
        fprintf(stdout,"| %s |   %f  |   %f     |\n",buff->name,buff->price,avg);
        buff->elements--;
        sem_post(&buff->mutex);
    }
    sem_post(buffer);
    fprintf(stdout,"+--------------------------------------+\n");
    sem_close(&buff->mutex);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
    return 0;
}
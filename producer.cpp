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
    string commname=argv[1];
    double pricemean=atof(argv[2]);
    double pricestddev=atof(argv[3]);
    int sleepint=atoi(argv[4]);
    int size=atoi(argv[5]);

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

    default_random_engine generator;
    normal_distribution<double> distribution(pricemean,pricestddev);
    double price=distribution(generator);

    struct timespec currtime;
    while(true){
        sem_wait(buffer);
        clock_gettime(CLOCK_MONOTONIC,&currtime);
        printf("%s %s: generating a new value %d\n", currtime, buff->commname, price);
        clock_gettime(CLOCK_MONOTONIC,&currtime);
        printf("%s %s: trying to get mutex on shared buffer\n", currtime, buff->commname);
        // !!critical section!!
        sem_wait(&mutex);
        clock_gettime(CLOCK_MONOTONIC,&currtime);
        printf("%s %s: placing %d on shared buffer\n", currtime, buff->commname, price);
        buff->price=price;
        sem_post(&mutex);
        // !!end of critical section!!
        clock_gettime(CLOCK_MONOTONIC,&currtime);
        printf("%s %s: sleeping for %d ms", currtime, buff->commname, sleepint);
        sleep(sleepint/1000);
        sem_post(buffer);
    }
    return 0;
}
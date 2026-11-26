#include <iostream>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <random>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <chrono>

#define SHM_NAME "/shared_buffer"
#define SEM_NAME "/shared_sem"

typedef struct{
    sem_t mutex;
    sem_t empty;
    sem_t full;
    int read;
    int write;
}SharedControl;
typedef struct {
    char name[11];
    double price;
} SharedBuffer;
typedef struct{
    char name[11];
    double currprice=0.00;
    double prevprice=0.00;
    double last[5];
}Dashboard;

using namespace std;
int main(int argc, char* argv[]){
    int size=atoi(argv[1]);
    double avg;
    Dashboard *dash[11];

    // sem_t *buffer = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(SharedControl)+size*sizeof(SharedBuffer));

    SharedControl *cont=static_cast<SharedControl *>(mmap(NULL, sizeof(SharedControl)+size*sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)); //normal func(no cast) doesnt work in c++ :( only c
    if (cont == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    SharedBuffer *buff=(SharedBuffer *)((char*)cont+sizeof(SharedControl));
    close(shm_fd);

    fprintf(stdout,"\e[1;1H\e[2J");
    fprintf(stdout,"+--------------------------------------+\n");
    fprintf(stdout,"| Currency  |   Price   |   AvgPrice   |\n");
    fprintf(stdout,"+--------------------------------------+\n");
    while(true){
        sem_wait(&cont->full);
        // !!critical section!!
        sem_wait(&cont->mutex);
        fprintf(stdout,"| %s |   %f  |   %f     |\n",buff[cont->read].name,buff[cont->read].price,avg);
        // strcpy(dash[cont->read]->name,buff[cont->read].name);
        
        // dash[cont->read]->currprice=buff[cont->read].price;

        cont->read=(cont->read+1)%size;
        sem_post(&cont->mutex);
        // !!end of critical section!!
        sem_post(&cont->empty);
        fprintf(stdout,"+--------------------------------------+\n");
    }
    sem_close(&cont->mutex);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
    return 0;
}
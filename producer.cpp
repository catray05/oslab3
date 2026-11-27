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
#define SEM_MUTEX "/shared_mutex"
#define SEM_FULL "/shared_full"
#define SEM_EMPTY "/shared_empty"


typedef struct{
    int read;
    int write;
}SharedControl;
typedef struct {
    char name[11];
    double price;
} SharedBuffer;

void timern(timespec ts){
    struct tm *t=localtime(&ts.tv_sec);
    int year=t->tm_year+1900;
    int month=t->tm_mon+1;
    int day=t->tm_mday;
    int hour=t->tm_hour;
    int min=t->tm_min;
    int sec=t->tm_sec;
    long millisec=ts.tv_nsec/1e+6;
    fprintf(stderr,"[%02d/%02d/%d %02d:%02d:%02d.%03ld]",month,day,year,hour,min,sec,millisec);
}

using namespace std;
int main(int argc, char* argv[]){
    string commname=argv[1];
    for(int i=0;i<commname.length();i++)
        commname[i]=toupper(commname[i]);
    double pricemean=atof(argv[2]);
    double pricestddev=atof(argv[3]);
    int sleepint=atoi(argv[4]);
    int size=atoi(argv[5]);
    default_random_engine generator;
    normal_distribution<double> distribution(pricemean,pricestddev);
    double price;
    struct timespec currtime;

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
    sem_t *mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    sem_t *empty = sem_open(SEM_EMPTY, O_CREAT, 0666, size);
    sem_t *full = sem_open(SEM_FULL, O_CREAT, 0666, 0);
    
    // EL MFROOD EL PRINTING DAH FL STDERR AGGHDFJKWEGFK
    while(true){
        sem_wait(empty);
        clock_gettime(CLOCK_REALTIME,&currtime);
        timern(currtime);
        price=distribution(generator);
        fprintf(stderr," %s: generating a new value %7.2lf\n", commname.c_str(), price);
        clock_gettime(CLOCK_REALTIME,&currtime);
        timern(currtime);
        fprintf(stderr," %s: trying to get mutex on shared buffer\n", commname.c_str());
        // !!critical section!!
        sem_wait(mutex);
        clock_gettime(CLOCK_REALTIME,&currtime);
        timern(currtime);
        fprintf(stderr," %s: placing %7.2lf on shared buffer\n", commname.c_str(), price);
        buff[cont->write].price=price;
        strcpy(buff[cont->write].name,commname.c_str());
        cont->write=(cont->write+1)%size;
        sem_post(mutex);
        // !!end of critical section!!
        sem_post(full);
        clock_gettime(CLOCK_REALTIME,&currtime);
        timern(currtime);
        fprintf(stderr," %s: sleeping for %d ms\n", commname.c_str(), sleepint);
        // sleep(sleepint/1000); lw 200 hy sleep 0 :[
        this_thread::sleep_for(chrono::milliseconds(sleepint));
    }
    return 0;
}
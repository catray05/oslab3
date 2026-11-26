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


typedef struct {
    sem_t mutex;
    char name[11];
    double price;
    int elements;
    sem_t empty;
    sem_t full;
} SharedBuffer;

void timern(time_t now, timespec ts){
    struct tm *t=localtime(&now);
    int year=t->tm_year+1900;
    int month=t->tm_mon+1;
    int day=t->tm_mday;
    int hour=t->tm_hour;
    int min=t->tm_min;
    int sec=t->tm_sec;
    long millisec=ts.tv_nsec/1000;
    fprintf(stderr,"[%d/%d/%d %d:%d:%d.%ld]",month,day,year,hour,min,sec,millisec);
}

using namespace std;
int main(int argc, char* argv[]){
    string commname=argv[1];
    double pricemean=atof(argv[2]);
    double pricestddev=atof(argv[3]);
    int sleepint=atoi(argv[4]);
    int size=atoi(argv[5]);
    default_random_engine generator;
    normal_distribution<double> distribution(pricemean,pricestddev);
    double price;
    struct timespec currtime;
    time_t now;

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(SharedBuffer));

    SharedBuffer *buff=static_cast<SharedBuffer *>(mmap(NULL, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)); //normal func(no cast) doesnt work in c++ :( only c
    if (buff == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    close(shm_fd);

    sem_init(&buff->mutex,1,1);
    sem_init(&buff->empty,1,size);
    sem_init(&buff->full,1,0);
    // EL MFROOD EL PRINTING DAH FL STDERR AGGHDFJKWEGFK
    while(buff->elements!=size){
        sem_wait(&buff->empty);
        clock_gettime(CLOCK_REALTIME,&currtime);
        now=time(NULL);
        timern(now,currtime);
        price=distribution(generator);
        fprintf(stderr," %s: generating a new value %7.2lf\n", commname.c_str(), price);
        clock_gettime(CLOCK_REALTIME,&currtime);
        now=time(NULL);
        timern(now,currtime);
        fprintf(stderr," %s: trying to get mutex on shared buffer\n", commname.c_str());
        // !!critical section!!
        sem_wait(&buff->mutex);
        clock_gettime(CLOCK_REALTIME,&currtime);
        now=time(NULL);
        timern(now,currtime);
        fprintf(stderr," %s: placing %7.2lf on shared buffer\n", commname.c_str(), price);
        buff->price=price;
        strcpy(buff->name,commname.c_str());
        buff->elements++;
        sem_post(&buff->mutex);
        // !!end of critical section!!
        clock_gettime(CLOCK_REALTIME,&currtime);
        now=time(NULL);
        timern(now,currtime);
        fprintf(stderr," %s: sleeping for %d ms\n", commname.c_str(), sleepint);
        // sleep(sleepint/1000); lw 200 hy sleep 0 :[
        this_thread::sleep_for(chrono::milliseconds(sleepint));
        sem_post(&buff->full);
    }
    sem_close(&buff->mutex);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
    return 0;
}
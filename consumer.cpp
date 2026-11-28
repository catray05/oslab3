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
#include <signal.h>

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
typedef struct{
    char name[11];
    double currprice;
    double prevprice;
    double prevavg;
    double last[5];
    int next;
    int count;
    int avgarw;
    int pricearw;
}Dashboard;
int stop=0;
void handler(int sig){
    stop=1;
}

void drawdash(Dashboard dash[]){
    fprintf(stdout,"\e[1;1H\e[2J");
        fprintf(stdout,"+---------------------------------------+\n");
        fprintf(stdout,"| Currency   |   Price   |   AvgPrice   |\n");
        fprintf(stdout,"+---------------------------------------+\n");
        const char* pricearw;
        const char* avgarw;
        int pricecolor;
        int avgcolor;
        for(int i=0;i<11;i++){
            pricecolor=36;
            avgcolor=36;
            if(dash[i].pricearw==1){
                pricearw="↑";
                pricecolor=32;
            }
            else if(dash[i].pricearw==-1){
                pricearw="↓";
                pricecolor=31;
            }
            else{
                pricearw=" ";
            }
            if(dash[i].avgarw==1){
                avgarw="↑";
                avgcolor=32;
            }
            else if(dash[i].avgarw==-1){
                avgarw="↓";
                avgcolor=31;
            }
            else{
                avgarw=" ";
            }
            fprintf(stdout,"| %-10s |\033[;%dm%7.2lf%s\033[0m   |\033[%dm%7.2lf%s\033[0m      |\n",dash[i].name,pricecolor,dash[i].currprice,pricearw,avgcolor,dash[i].prevavg,avgarw);
        }
        fprintf(stdout,"+---------------------------------------+\n");
}

using namespace std;
int main(int argc, char* argv[]){
    struct sigaction sa;
    sa.sa_handler=&handler;
    sa.sa_flags=0;
    sigaction(SIGINT,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);
    int size=atoi(argv[1]);
    double avg;
    char name[11];
    double price;
    int idx;
    double sum;
    Dashboard dash[11]={
        {"ALUMINIUM"},
        {"COPPER"},
        {"COTTON"},
        {"CRUDEOIL"},
        {"GOLD"},
        {"LEAD"},
        {"MENTHAOIL"},
        {"NATURALGAS"},
        {"NICKEL"},
        {"SILVER"},
        {"ZINC"}
    };
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
    sem_t *mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    sem_t *empty = sem_open(SEM_EMPTY, O_CREAT, 0666, size);
    sem_t *full = sem_open(SEM_FULL, O_CREAT, 0666, 0);
    drawdash(dash);
    while(!stop){
        if(sem_wait(full)==-1)
            break;
        // !!critical section!!
        if(sem_wait(mutex)==-1)
            break;
        idx=-1;
        strcpy(name,buff[cont->read].name);
        price=buff[cont->read].price;
        cont->read=(cont->read+1)%size;
        sem_post(mutex);
        // !!end of critical section!!
        sem_post(empty);
        for(int i=0;i<11;i++){
            if(!strcmp(name,dash[i].name))
                idx=i;
        }
        if(idx==-1)
            continue;
        dash[idx].last[dash[idx].next]=price;
        dash[idx].next=(dash[idx].next+1)%5;
        dash[idx].count=min((dash[idx].count+1),5);
        dash[idx].prevprice=dash[idx].currprice;
        dash[idx].currprice=price;
        sum=0.00;
        for(int i=0;i<dash[idx].count;i++){
            sum+=dash[idx].last[i];
        }
        avg=sum/dash[idx].count;
        if(dash[idx].count>1){
            if(dash[idx].currprice>dash[idx].prevprice)
            dash[idx].pricearw=1;
            else if(dash[idx].currprice<dash[idx].prevprice)
            dash[idx].pricearw=-1;
            else dash[idx].pricearw=0;
            if(avg>dash[idx].prevavg)
            dash[idx].avgarw=1;
            else if(avg<dash[idx].prevavg)
            dash[idx].avgarw=-1;
            else dash[idx].avgarw=0;
        }
        else{
            dash[idx].avgarw=0;
            dash[idx].pricearw=0;
        }
        dash[idx].prevavg=avg;
        drawdash(dash);
    }
    sem_close(mutex);
    sem_close(full);
    sem_close(empty);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);
    munmap(cont,sizeof(SharedControl)+size*sizeof(SharedBuffer));
    shm_unlink(SHM_NAME);
    return 0;
}
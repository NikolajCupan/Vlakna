#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>


// spolocne
typedef struct kava
{
    int horkost;
    int kyslost;
} KV;

typedef struct spolData
{
    bool isKava;
    KV * kava;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
} SD;


// vlastne
typedef struct doktorand
{
    SD * data;
    int aktualnaSpokojnost;
};

typedef struct barista
{
    SD * data;
};


// metody
void * doktorandD(void * arg)
{

}

void * baristaF(void * arg)
{

}


int main() {
    printf("\nMAIN: START\n\n\n\n\n");
    srand(time(NULL));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;


    printf("\n\n\n\n\nMAIN: END\n");
    return 0;
}

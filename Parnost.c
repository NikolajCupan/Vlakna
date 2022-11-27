#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>


typedef struct dataSpol{
    const int pocet;
    int * pole;
    const int kapacita;
    int aktualnyPocet;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * vyberaj;
} SPOL;


typedef struct dataGen {
    SPOL * data;
}GEN;

typedef struct dataPar {
    SPOL * data;
    int pocetParnych;
}PAR;


void * generujCisla(void * arg) {
    printf("G: zacinam\n");
    GEN * data = arg;
    int cislo;
    for( int i = 0; i < data->data->pocet; i++) {
        cislo = 1 + rand()%(100-1+1);
        pthread_mutex_lock(data->data->mut);
        while(data->data->aktualnyPocet == data->data->kapacita) {
            printf("G %d\n",data->data->aktualnyPocet);
            pthread_cond_signal(data->data->vyberaj);
            pthread_cond_wait(data->data->generuj,data->data->mut);
        }
        data->data->pole[data->data->aktualnyPocet++] = cislo;
        if(i == data->data->pocet-1) {
            pthread_cond_signal(data->data->vyberaj);
        }
        pthread_mutex_unlock(data->data->mut);
        printf("G %d. cislo je %d\n",i+1,cislo);
    }
    printf("G: koncim\n");
    pthread_exit(NULL);
}

void * zistiParne(void * arg)
{
    printf("P: zacinam\n");

    PAR * data = arg;
    int cislo;

    for(int i = 0; i < data->data->pocet; i++)
    {
        pthread_mutex_lock(data->data->mut);
        while(data->data->aktualnyPocet == 0) {
            printf("P %d\n",data->data->aktualnyPocet);
            pthread_cond_signal(data->data->generuj);
            pthread_cond_wait(data->data->vyberaj,data->data->mut);
        }
        cislo = data->data->pole[--data->data->aktualnyPocet];
        printf("P %d\n",data->data->aktualnyPocet);
        if(i == data->data->pocet-1) {
            pthread_cond_signal(data->data->generuj);
        }
        pthread_mutex_unlock(data->data->mut);
        printf("P %d. cislo je %d.",i+1,cislo);
        if(cislo%2==0) {
            printf("Je parne!");
            data->pocetParnych++;
        }
        printf("\n");
    }
    printf("P: koncim, parnych cisel bolo %d!\n", data->pocetParnych);
    pthread_exit(NULL);
}


int main()
{
    srand(time(NULL));

    int pocet = 100;
    const int kapacita = 5;

    int * pole = calloc(kapacita,sizeof(int));

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj, vyberaj = PTHREAD_COND_INITIALIZER;
    pthread_cond_init(&generuj,NULL);

    SPOL dataS = {
            pocet, pole, kapacita,0, &mutex, &generuj, &vyberaj
    };

    GEN dataG = {
            &dataS
    };
    PAR dataP;

    dataP.data = &dataS;
    dataP.pocetParnych = 0;

    pthread_t vlaknoG,vlaknoP;
    pthread_create(&vlaknoG,NULL,generujCisla,&dataG);
    pthread_create(&vlaknoP,NULL,zistiParne,&dataP);



    pthread_join(vlaknoG,NULL);
    pthread_join(vlaknoP,NULL);


    printf("HV: Koniec!\n");


    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&vyberaj);
    free(pole);

    return 0;
}
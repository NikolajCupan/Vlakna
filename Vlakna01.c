#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct buffer {
     int cislo;
} BUF;

typedef struct spolData
{
    BUF * buffer;
    int velkostNastenky;
    int maxPocetCisel;
    int curPocetCisel;
    int maximum;
    int minimum;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
} SD;

typedef struct generator {
    char * meno;
    SD * data;
} GEN;

typedef struct zistovac {
    char * meno;
    SD * data;
} ZIS;


void * genF(void * arg)
{
    GEN * dataG = arg;
    BUF tempB;

    printf("Moje meno je: %s a zacinam pracovat\n", dataG->meno);

    for (int i = 0; i < dataG->data->maxPocetCisel; i++)
    {
        int nahoda = rand() % (dataG->data->maximum + 1 - dataG->data->minimum) + dataG->data->minimum;
        printf("Generator vygeneroval cislo: %d\n", nahoda);

        pthread_mutex_lock(dataG->data->mut);

        while(dataG->data->curPocetCisel >= dataG->data->velkostNastenky)
        {
            printf("Generator musi cakat, v buffri je %d cisel, cislo, ktore bude vkladat dalej je %d\n", dataG->data->curPocetCisel, nahoda);
            pthread_cond_wait(dataG->data->generuj, dataG->data->mut);
        }

        tempB.cislo = nahoda;
        dataG->data->buffer[dataG->data->curPocetCisel] = tempB;
        dataG->data->curPocetCisel++;

        printf("Generator vlozil cislo do buffru, v buffri je momentalne %d cisel\n", dataG->data->curPocetCisel);

        pthread_cond_signal(dataG->data->odoberaj);
        pthread_mutex_unlock(dataG->data->mut);
    }

    printf("Moje meno je: %s a koncim s pracou\n", dataG->meno);
}

void * zisF(void * arg)
{
    ZIS * dataZ = arg;
    BUF tempB;

    printf("Moje meno je: %s a zacinam pracovat\n", dataZ->meno);

    for (int i = 0; i < dataZ->data->maxPocetCisel; i++)
    {
        pthread_mutex_lock(dataZ->data->mut);

        while (dataZ->data->curPocetCisel <= 0)
        {
            printf("Zistovac musi cakat, v buffri je %d cisel\n", dataZ->data->curPocetCisel);
            pthread_cond_wait(dataZ->data->odoberaj, dataZ->data->mut);
        }

        dataZ->data->curPocetCisel--;
        tempB = dataZ->data->buffer[dataZ->data->curPocetCisel];
        printf("Zistovac vytiahol cislo %d\n", tempB.cislo);

        if (dataZ->data->curPocetCisel == 0)
        {
            pthread_cond_signal(dataZ->data->generuj);
        }

        pthread_mutex_unlock(dataZ->data->mut);
    }

    printf("Moje meno je: %s a koncim s pracou\n", dataZ->meno);
}


int main(int argc, char * argv[]) {
    printf("MAIN: zaciatok\n\n");

    if (argc < 4)
    {
        fprintf(stderr, "Nedostatocny pocet parametrov\n");
        return 1;
    }
    else
    {
        printf("Jednotlive parametre su:\n");

        for (int i = 0; i < argc; i++)
        {
            printf("%d. parameter: %s\n", i, argv[i]);
        }

        printf("\n");
    }

    int velkostNastenky = 10;
    int minimum = atoi(argv[1]);
    int maximum = atoi(argv[2]);
    int pocet = atoi(argv[3]);

    BUF * buffer = calloc(pocet, sizeof(BUF));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;

    SD spData = {
            buffer, 10, pocet, 0, maximum, minimum, &mut, &generuj, &odoberaj
    };

    GEN genD = { "generator", &spData };
    GEN zisD = { "zistovac", &spData };

    pthread_t generator;
    pthread_t zistovac;

    pthread_create(&generator, NULL, genF, &genD);
    pthread_create(&zistovac, NULL, zisF, &zisD);

    pthread_join(generator, NULL);
    pthread_join(zistovac, NULL);


    free(buffer);
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);


    printf("MAIN: koniec\n\n");
    return 0;
}

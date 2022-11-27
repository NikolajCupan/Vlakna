#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <stdbool.h>


// spolocne
typedef struct spolocneData
{
    int * pole;
    int aktualneSpracovanych;
    int maximalneSpracovanych;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
    int aktualnePrvkov;
    int maximalnePrvkov;
} SD;


// vlastne
typedef struct generator
{
    SD * data;
    int maximalneVygeneruje;
} GEN;

typedef struct zistovac
{
    int id;
    SD * data;
    int aktualneZistil;
    int pocetParnych;
    int pocetNeparnych;
} ZIS;


// metody
void * generatorF(void * arg)   // producent
{
    GEN * dataG = arg;
    int tempC;

    printf("GENERATOR: START\n");

    for (int i = 0; i < dataG->maximalneVygeneruje; i++)
    {
        tempC = rand() % 1001 + 1;

        pthread_mutex_lock(dataG->data->mut);

        while (dataG->data->aktualnePrvkov >= dataG->data->maximalnePrvkov)
        {
            printf("Buffer je plny (%d prvkov), generator caka\n", dataG->data->aktualnePrvkov);
            pthread_cond_wait(dataG->data->generuj, dataG->data->mut);
        }

        dataG->data->pole[dataG->data->aktualnePrvkov] = tempC;
        dataG->data->aktualnePrvkov++;
        printf("(pridanie: %d) Generator pridal cislo %d do buffru, je tam %d prvkov\n", (i + 1), tempC, dataG->data->aktualnePrvkov);

        pthread_cond_signal(dataG->data->odoberaj);
        pthread_mutex_unlock(dataG->data->mut);
    }

    printf("GENERATOR: END\n");
}

void * zistovacF(void * arg)    // konzument
{
    ZIS * dataZ = arg;
    int tempC;

    printf("ZISTOVAC ID %d: START\n", dataZ->id);

    while(true)
    {
        pthread_mutex_lock(dataZ->data->mut);

        if (dataZ->data->aktualneSpracovanych >= dataZ->data->maximalneSpracovanych)
        {
            printf("Zistovac s ID %d konci, spracovanych prvkov: %d, max bude spracovanych %d\n", dataZ->id, dataZ->data->aktualneSpracovanych, dataZ->data
                    ->aktualneSpracovanych);
            pthread_cond_signal(dataZ->data->generuj);
            pthread_mutex_unlock(dataZ->data->mut);

            break;
        }


        while (dataZ->data->aktualnePrvkov <= 0)
        {
            if (dataZ->data->aktualneSpracovanych >= dataZ->data->maximalneSpracovanych)
            {
                printf("Zistovac s ID %d konci, spracovanych prvkov: %d, max bude spracovanych %d\n", dataZ->id, dataZ->data->aktualneSpracovanych, dataZ->data
                        ->aktualneSpracovanych);
                pthread_cond_signal(dataZ->data->generuj);
                pthread_mutex_unlock(dataZ->data->mut);

                break;
            }

            printf("Zistovac s ID %d caka, nakolko v buffri je %d prvkov\n", dataZ->id, dataZ->data->aktualnePrvkov);
            pthread_cond_wait(dataZ->data->odoberaj, dataZ->data->mut);
        }

        if (dataZ->data->aktualneSpracovanych >= dataZ->data->maximalneSpracovanych)
        {
            printf("Zistovac s ID %d konci, spracovanych prvkov: %d, max bude spracovanych %d\n", dataZ->id, dataZ->data->aktualneSpracovanych, dataZ->data
                    ->aktualneSpracovanych);
            pthread_cond_signal(dataZ->data->generuj);
            pthread_mutex_unlock(dataZ->data->mut);

            break;
        }

        dataZ->data->aktualnePrvkov--;
        tempC = dataZ->data->pole[dataZ->data->aktualnePrvkov];
        dataZ->data->aktualneSpracovanych++;
        printf("(spracovanie: %d) Zistovac s ID %d, vytiahol cislo %d z buffru, v buffri ostalo %d prvkov\n", dataZ->data->aktualneSpracovanych, dataZ->id, tempC, dataZ->data->aktualnePrvkov);
        pthread_cond_signal(dataZ->data->generuj);
        pthread_mutex_unlock(dataZ->data->mut);
    }

    printf("ZISTOVAC ID %d: END\n", dataZ->id);
}


int main()
{
    printf("\nMAIN: START\n\n\n\n");


    int pocetZistovacov = 1;
    int pocetCisel = 381;

    int * pole = calloc(pocetCisel, sizeof(int));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;


    SD spolocneData = {
            pole, 0, pocetCisel, &mut, &generuj, &odoberaj, 0, 6
    };


    pthread_t generator;
    GEN generatorD = {
            &spolocneData, pocetCisel
    };

    pthread_create(&generator, NULL, generatorF, &generatorD);


    pthread_t zistovaci[pocetZistovacov];
    ZIS zistovaciD[pocetZistovacov];

    for (int i = 0; i < pocetZistovacov; i++)
    {
        zistovaciD[i].id = (i + 1);
        zistovaciD[i].data = &spolocneData;
        zistovaciD[i].aktualneZistil = 0;
        zistovaciD[i].pocetNeparnych = 0;
        zistovaciD[i].pocetParnych = 0;

        pthread_create(&zistovaci[i], NULL, zistovacF, &zistovaciD[i]);
    }



    pthread_join(generator, NULL);

    for (int i = 0; i < pocetZistovacov; i++)
    {
        pthread_join(zistovaci[i], NULL);
    }


    free(pole);
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);

    printf("\nMAIN: END\n\n\n\n");
    return 0;
}

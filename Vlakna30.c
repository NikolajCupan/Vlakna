#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>


// spolocne
typedef struct objednavka
{
    int cena;
} OBJ;

typedef struct spolData
{
    OBJ * box;
    int maximalnaKapacita;
    int aktualnaKapacita;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
    bool nedostatok;
} SD;

// vlastne
typedef struct zberatel
{
    SD * data;
    int pocetZlatiek;
    int prijem;
    int pocetTovarov;
    int magickeSlovo;
} ZB;

typedef struct generator
{
    SD * data;
} GEN;


// metody
void * generatorF(void * arg)   // producent
{
    GEN * dataG = arg;
    OBJ tempO;

    printf("GENERATOR: START\n");

    while(true)
    {
        pthread_mutex_lock(dataG->data->mut);

        if (dataG->data->nedostatok)
        {
            printf("Generator konci, zberatel nema peniaze\n");

            pthread_cond_signal(dataG->data->odoberaj);
            pthread_mutex_unlock(dataG->data->mut);

            break;
        }


        if (dataG->data->aktualnaKapacita <= 0)
        {
            printf("Box bol vyprazdneny, pocet tovarov %d, zacina generovanie\n", dataG->data->aktualnaKapacita);

            while (dataG->data->aktualnaKapacita < dataG->data->maximalnaKapacita)
            {
                int cena = rand() % (4200 + 1 - 420) + 420;
                tempO.cena = cena;

                dataG->data->box[dataG->data->aktualnaKapacita] = tempO;
                dataG->data->aktualnaKapacita++;

                printf("Vygenerovany tovar s cenou %d, je ich tam %d\n", cena, dataG->data->aktualnaKapacita);

                pthread_cond_signal(dataG->data->odoberaj);
                pthread_mutex_unlock(dataG->data->mut);

                pthread_mutex_lock(dataG->data->mut);
            }

            printf("Box je plny, pocet tovarov %d\n", dataG->data->aktualnaKapacita);

            usleep(100);

            pthread_cond_signal(dataG->data->odoberaj);
            pthread_mutex_unlock(dataG->data->mut);
        }

        pthread_cond_signal(dataG->data->odoberaj);
        pthread_mutex_unlock(dataG->data->mut);
    }

    printf("GENERATOR: START\n");
}

void * zberatelF(void * arg)    // konzument
{
    ZB * dataZ = arg;
    OBJ tempO;

    printf("ZBERATEL: START\n");

    while (true)
    {
        bool zlyhanie = false;


        pthread_mutex_lock(dataZ->data->mut);

        while (dataZ->data->aktualnaKapacita <= 0)
        {
            printf("V boxe nic nie je (tovarov: %d), zberatel caka\n", dataZ->data->aktualnaKapacita);
            pthread_cond_wait(dataZ->data->odoberaj, dataZ->data->mut);
        }

        dataZ->data->aktualnaKapacita--;
        tempO = dataZ->data->box[dataZ->data->aktualnaKapacita];

        if (tempO.cena > dataZ->prijem)
        {
            printf("Nedostatok penazi, ma %d, treba %d, koniec\n", dataZ->prijem, tempO.cena);
            dataZ->data->nedostatok = true;

            pthread_cond_signal(dataZ->data->generuj);
            pthread_mutex_unlock(dataZ->data->mut);

            break;
        }


        int nahoda = rand() % 100;

        if (nahoda < 75)
        {
            printf("Slnecne pocasie\n");

            nahoda = rand() % 100;

            if (nahoda < 45)
            {
                printf("Magicke slovo\n");
                dataZ->magickeSlovo++;
            }
            else
            {
                printf("Caka sekundu\n");
                sleep(1);
            }
        }

        printf("Zberatel si zobral vec, ostalo tam %d tovarov\n", dataZ->data->aktualnaKapacita);

        pthread_cond_signal(dataZ->data->generuj);
        pthread_mutex_unlock(dataZ->data->mut);


        nahoda = rand() % 100;




        dataZ->pocetTovarov++;
        dataZ->prijem = dataZ->prijem - tempO.cena;


        printf("Zberatel ma %d tovarov, %d penazi\n", dataZ->pocetTovarov, dataZ->prijem);

        if (tempO.cena > 1200)
        {
            int zlatky = tempO.cena / 100 / 5;
            dataZ->pocetZlatiek = dataZ->pocetZlatiek + zlatky;
        }
    }

    printf("ZBERATEL: END\n");
}


int main() {
    printf("\nMAIN: START\n\n\n\n\n");
    srand(time(NULL));

    int maximalnaKapacita = rand() % 11 + 10;
    int prijem = rand() % (750000 + 1 - 180000) + 180000;
    printf("Prijem: %d\n", prijem);
    printf("Kapacita boxu: %d\n\n", maximalnaKapacita);

    OBJ * box = calloc(maximalnaKapacita, sizeof(OBJ));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;


    SD spData = {
            box, maximalnaKapacita, 0, &mut, &generuj, &odoberaj, false
    };

    pthread_t generator;
    GEN generatorD = {
            &spData
    };

    pthread_create(&generator, NULL, generatorF, &generatorD);

    pthread_t zberatel;
    ZB zberatelD = {
            &spData, 0, prijem, 0,  0
    };

    pthread_create(&zberatel, NULL, zberatelF, &zberatelD);



    pthread_join(zberatel, NULL);
    pthread_join(generator, NULL);


    free(box);
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);

    printf("\n\n\n\n\nMAIN: END\n");
    return 0;
}

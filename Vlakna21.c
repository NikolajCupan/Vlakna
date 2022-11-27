#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>


// spolocne data
typedef struct darcek
{
    char * nazov;
} DAR;

typedef struct spolData
{
    DAR * regal;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
    int maximalnaKapacitaPultu;
    int aktualnaKapacitaPultu;
    int vykladajuPri;
    int prestanuVykladatPri;
} SD;


// vlakna
typedef struct zakaznik
{
    SD * data;
    int aktualneStravenaDoba;
    int maximalneStravenaDoba;
} ZAK;

typedef struct pracovnik
{
    SD * data;
    int aktualnyPocetVylozeni;
    int maximalnyPocetVylozeni;
} PRAC;


// metody
void * zakaznikF(void * arg)    // konzument
{
    printf("ZAKAZNIK: START\n");

    ZAK * dataZ = arg;
    DAR tempD;
    int prichod;
    int pocetDarcekov;

    while (true)
    {
        printf("Doposial bol obchod otvoreny %d sekund\n", dataZ->aktualneStravenaDoba);

        prichod = rand() % (3 + 1 - 2) + 2;
        pocetDarcekov = rand() % (5 + 1 - 1) + 1;

        dataZ->aktualneStravenaDoba = dataZ->aktualneStravenaDoba + prichod;

        if (dataZ->aktualneStravenaDoba > dataZ->maximalneStravenaDoba)
        {
            printf("Dalsi zakaznik by prisiel az v case %d, vtedy je uz obchod ale zavrety\n", dataZ->aktualneStravenaDoba);
            break;
        }
        else
        {
            printf("Zakaznik sa vydava na nakup, bude mu to trvat %d sekund, v obchode bude v case %d\n", prichod, dataZ->aktualneStravenaDoba);
            sleep(prichod);
        }


        pthread_mutex_lock(dataZ->data->mut);

        if (dataZ->data->aktualnaKapacitaPultu == 0)
        {
            printf("Na pulte je %d darcekov, zakaznik pozaduje %d darcekov, zakaznik je velmi nespokojny\n", dataZ->data->aktualnaKapacitaPultu, pocetDarcekov);
        }
        else if (dataZ->data->aktualnaKapacitaPultu < pocetDarcekov)
        {
            printf("Zakaznik chcel %d darcekov, avsak v regali je iba %d darcekov\n", pocetDarcekov, dataZ->data->aktualnaKapacitaPultu);

            dataZ->data->aktualnaKapacitaPultu = 0;
            printf("Zakaznik berie vsetky darceky na pulte ostalo %d darcekov\n", dataZ->data->aktualnaKapacitaPultu);
        }
        else if (dataZ->data->aktualnaKapacitaPultu == pocetDarcekov)
        {
            printf("Na pulte (%d) je presne tolko darcekov, kolko zakaznik chce (%d), berie vsetky\n", dataZ->data->aktualnaKapacitaPultu, pocetDarcekov);
            dataZ->data->aktualnaKapacitaPultu = 0;
        }
        else
        {
            printf("Na pulte (%d) je viac darcekov ako zakaznik chce (%d), zakaznik si berie tolko kolko chce\n", dataZ->data->aktualnaKapacitaPultu, pocetDarcekov);

            dataZ->data->aktualnaKapacitaPultu = dataZ->data->aktualnaKapacitaPultu - pocetDarcekov;
            printf("Na pulte ostalo %d darcekov\n", dataZ->data->aktualnaKapacitaPultu);
        }


        pthread_cond_signal(dataZ->data->generuj);
        pthread_mutex_unlock(dataZ->data->mut);
    }

    printf("ZAKAZNIK: END\n");
}

void * pracovnikF(void * arg)
{
    printf("PRACOVNIK: START\n");

    PRAC * dataP = arg;
    DAR tempD;

    while (dataP->aktualnyPocetVylozeni < dataP->maximalnyPocetVylozeni)
    {
        printf("Pracovnik sa ide pozriet, ci ma vylozit darceky v case %d\n", dataP->aktualnyPocetVylozeni);
        bool pracuj = false;


        pthread_mutex_lock(dataP->data->mut);

        if (dataP->data->aktualnaKapacitaPultu < dataP->data->vykladajuPri)
        {
            printf("Na pulte je %d darcekov, zacinaju pracovat, ked je pocet darcekov pod %d, cize zacina praca, (cas = %d)\n", dataP->data->aktualnaKapacitaPultu, dataP->data->vykladajuPri, dataP->aktualnyPocetVylozeni);
            pracuj = true;


            while (pracuj)
            {
                if (dataP->aktualnyPocetVylozeni >= dataP->maximalnyPocetVylozeni)
                {
                    printf("Obchod sa zatvara, (cas = %d)\n", dataP->aktualnyPocetVylozeni);

                    pthread_mutex_unlock(dataP->data->mut);
                    pthread_cond_signal(dataP->data->odoberaj);

                    break;
                }


                int budeDarcekov;
                budeDarcekov = dataP->data->aktualnaKapacitaPultu + 2;

                if (budeDarcekov > dataP->data->maximalnaKapacitaPultu)
                {
                    printf("Na pult sa nezmestia az 2 darceky, (cas = %d)\n", dataP->aktualnyPocetVylozeni);

                    if ((budeDarcekov - 1) > dataP->data->maximalnaKapacitaPultu)
                    {
                        printf("Na pult sa dokonca nezmesti ani 1 darcek, (cas = %d)\n", dataP->aktualnyPocetVylozeni);
                    }
                    else
                    {
                        dataP->data->aktualnaKapacitaPultu = dataP->data->aktualnaKapacitaPultu + 1;
                        printf("Na pult sa zmesti 1 darcek, pracovnik ho tam dava a na pulte je teda %d darcekov, (cas = %d)\n", dataP->data->aktualnaKapacitaPultu, dataP->aktualnyPocetVylozeni);
                    }
                }
                else
                {
                    dataP->data->aktualnaKapacitaPultu = dataP->data->aktualnaKapacitaPultu + 2;
                    printf("Na pult sa zmestia oba darceky, pracovnik ich tam dal a teda teraz tam je %d darcekov, (cas = %d)\n", dataP->data->aktualnaKapacitaPultu, dataP->aktualnyPocetVylozeni);
                }

                pthread_mutex_unlock(dataP->data->mut);
                pthread_cond_signal(dataP->data->odoberaj);

                printf("Pracovnik dokoncil toto vylozenie, (cas = %d)\n", dataP->aktualnyPocetVylozeni);
                sleep(1);
                dataP->aktualnyPocetVylozeni++;


                if (dataP->data->aktualnaKapacitaPultu > dataP->data->prestanuVykladatPri)
                {
                    printf("Na pulte je %d darcekov, konci pracu nad %d, cize konci, (cas = %d)\n", dataP->data->aktualnaKapacitaPultu, dataP->data->prestanuVykladatPri, dataP->aktualnyPocetVylozeni);
                    pracuj = false;
                }
            }
        }
        else
        {
            printf("Na pulte je %d darcekov, zacina pracu ked je pod %d, cize oddychuje, (cas = %d)\n", dataP->data->aktualnaKapacitaPultu, dataP->data->vykladajuPri, dataP->aktualnyPocetVylozeni);

            pthread_mutex_unlock(dataP->data->mut);
            pthread_cond_signal(dataP->data->odoberaj);


            if (dataP->aktualnyPocetVylozeni >= dataP->maximalnyPocetVylozeni)
            {
                printf("Obchod sa zatvara, (cas = %d)\n", dataP->aktualnyPocetVylozeni);
                break;
            }


            sleep(1);
            dataP->aktualnyPocetVylozeni++;
        }
    }

    printf("PRACOVNIK: END, (cas = %d)\n", dataP->aktualnyPocetVylozeni);
}


int main(int argc, char * argv[]) {
    printf("\nMAIN: START\n\n\n\n\n");
    srand(time(NULL));

    int kapacitaRegalu;
    if (argc < 2)
    {
        fprintf(stderr, "Nezadany parameter programu\n");
        return 1;
    }
    else
    {
        int parameter = atoi(argv[1]);

        if (parameter < 3)
        {
            fprintf(stderr, "Hodnota parametru je prilis mala\n");
            return 2;
        }

        kapacitaRegalu = parameter;
    }

    int vykladajuPri = kapacitaRegalu / 3;
    int konciaVykladaniePri = 2 * kapacitaRegalu / 3;
    int prvotneNaplenene = kapacitaRegalu / 2;

    printf("Kapacita regalu: %d\n", kapacitaRegalu);
    printf("Vykladaju ked je na pulte: %d\n", vykladajuPri);
    printf("Prestanu vykladat ked je na pulte: %d\n", konciaVykladaniePri);
    printf("Na zaciatku je v regali: %d\n\n", prvotneNaplenene);


    DAR * regal = calloc(kapacitaRegalu, sizeof(DAR));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;

    SD spData = {
            regal, &mut, &generuj, &odoberaj, kapacitaRegalu, prvotneNaplenene, vykladajuPri, konciaVykladaniePri
    };

    pthread_t zakaznik;
    ZAK zakaznikD = {
            &spData, 0, 48
    };

    pthread_t pracovnik;
    PRAC pracovnikD = {
            &spData, 0, 48
    };


    pthread_create(&zakaznik, NULL, zakaznikF, &zakaznikD);
    pthread_create(&pracovnik, NULL, pracovnikF, &pracovnikD);


    pthread_join(zakaznik, NULL);
    pthread_join(pracovnik, NULL);


    free(regal);
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);


    printf("\n\n\n\n\nMAIN: END\n");
    return 0;
}
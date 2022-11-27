#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// baliky
typedef struct balik
{
    char * nazov;
} BALIK;


// miesta
typedef struct prveMiesto
{
    BALIK * baliky;
    int aktualnaKapacita;
    int maximalnaKapacita;
    pthread_mutex_t * mutPrve;
    pthread_cond_t * generujPrve;
    pthread_cond_t * odoberajPrve;
} PRVE;

typedef struct druheMiesto
{
    BALIK * baliky;
    int aktualnaKapacita;
    int maximalnaKapacita;
    pthread_mutex_t * mutDruhe;
    pthread_cond_t * generujDruhe;
    pthread_cond_t * odoberajDruhe;
} DRUHE;


// vlakna
typedef struct balic
{
    int trvanie;
    int aktualneZabalil;
    int maximalneZabali;
    PRVE * data;
} BAL;

typedef struct prepravca
{
    int trvanie;
    int aktualnePrepravil;
    int maximalnePrepravi;
    PRVE * dataPrve;
    DRUHE * dataDruhe;
    int aktualneNesie;
    BALIK inventar[2];
} PREP;

typedef struct vykladac
{
    int trvanie;
    int aktualneVylozil;
    int maximalneVylozi;
    DRUHE * data;
} VYK;


// metody
void * balicF(void * arg)
{
    printf("BALIC: ZACIATOK\n");

    BAL * dataB = arg;
    BALIK tempB;
    tempB.nazov = "balik";

    for (int i = 0; i < dataB->maximalneZabali; i++)    // producent
    {
        int casBalenia = rand() % (4 + 1 - 2) + 2;
        int nahoda = rand() % 100;

        if (nahoda < 15)
        {
            casBalenia = casBalenia * 2;
        }

        printf("Balic dostal do ruk %d. balik, ktory bude balit %d sekund\n", (dataB->aktualneZabalil + 1), casBalenia);


        pthread_mutex_lock(dataB->data->mutPrve);
        while (dataB->data->aktualnaKapacita >= dataB->data->maximalnaKapacita)
        {
            printf("Balic musi cakat, prve miesto je plne, je tam %d balikov\n", dataB->data->aktualnaKapacita);
            pthread_cond_wait(dataB->data->generujPrve, dataB->data->mutPrve);
        }

        sleep(casBalenia);
        dataB->data->baliky[dataB->data->aktualnaKapacita] = tempB;
        dataB->data->aktualnaKapacita++;
        printf("Balic vlozil balik, momentalne tam je %d balikov\n", dataB->data->aktualnaKapacita);

        pthread_cond_signal(dataB->data->odoberajPrve);
        pthread_mutex_unlock(dataB->data->mutPrve);

        dataB->trvanie = dataB->trvanie + casBalenia;
        dataB->aktualneZabalil++;
    }

    printf("BALIC: KONIEC\n");
}

void * prepravcaF(void * arg)   // 1. konzument, 2. producent
{
    printf("PREPRAVCA: ZACIATOK\n");

    PREP * dataP = arg;

    while (dataP->aktualnePrepravil != dataP->maximalnePrepravi)
    {
        // vyber balika -> konzument
        pthread_mutex_lock(dataP->dataPrve->mutPrve);

        while (dataP->dataPrve->aktualnaKapacita <= 0)
        {
            printf("Na prvom mieste nie je ziadny balik, prepravca caka, pocet balikov je %d\n", dataP->dataPrve->aktualnaKapacita);
            pthread_cond_wait(dataP->dataPrve->odoberajPrve, dataP->dataPrve->mutPrve);
        }

        if (dataP->dataPrve->aktualnaKapacita == 1)
        {
            printf("Na prvom mieste sa nachadza iba %d balik, prepravca si ho berie\n", dataP->dataPrve->aktualnaKapacita);
            dataP->dataPrve->aktualnaKapacita--;
            dataP->inventar[0] = dataP->dataPrve->baliky[dataP->dataPrve->aktualnaKapacita];
            dataP->aktualneNesie = 1;
        }
        else if (dataP->dataPrve->aktualnaKapacita >= 2)
        {
            printf("Na prvom mieste sa nachadza %d balikov, prepravca si berie 2 baliky\n", dataP->dataPrve->aktualnaKapacita);
            dataP->dataPrve->aktualnaKapacita--;
            dataP->inventar[0] = dataP->dataPrve->baliky[dataP->dataPrve->aktualnaKapacita];
            dataP->dataPrve->aktualnaKapacita--;
            dataP->inventar[1] = dataP->dataPrve->baliky[dataP->dataPrve->aktualnaKapacita];
            dataP->aktualneNesie = 2;
        }

        pthread_cond_signal(dataP->dataPrve->generujPrve);
        pthread_mutex_unlock(dataP->dataPrve->mutPrve);


        // preprava
        int vozidlo = rand() % 100;
        int cesta = 0;

        if (vozidlo < 75)
        {
            for (int j = 0; j < dataP->aktualneNesie; j++)
            {
                int nahoda = rand() % (4 + 1 - 2) + 2;
                cesta = cesta + nahoda;
            }

            int nahoda = rand() % (2 + 1 - 1) + 1;
            cesta = cesta + nahoda;
        }
        else
        {
            cesta = 4;
        }

        cesta = cesta * 3;
        printf("Prepravca sa vydal na cestu, bude mu to trvat %d sekund\n", cesta);
        sleep(cesta);
        dataP->trvanie = dataP->trvanie + cesta;


        // vylozenie balika -> producent
        pthread_mutex_lock(dataP->dataDruhe->mutDruhe);

        while (dataP->dataDruhe->aktualnaKapacita >= dataP->dataDruhe->maximalnaKapacita)
        {
            printf("Druhe miesto je plne, je tam %d balikov, prepravca caka\n", dataP->dataDruhe->aktualnaKapacita);
            pthread_cond_wait(dataP->dataDruhe->generujDruhe, dataP->dataDruhe->mutDruhe);
        }

        // vyloz prvy balik
        BALIK tempB1;
        tempB1.nazov = "balik";
        dataP->dataDruhe->baliky[dataP->dataDruhe->aktualnaKapacita] = tempB1;
        dataP->dataDruhe->aktualnaKapacita++;
        dataP->aktualneNesie--;
        dataP->aktualnePrepravil++;

        printf("Prepravca vylozil balik na druhom mieste, momentalne tam je %d balikov\n", dataP->dataDruhe->aktualnaKapacita);

        if (dataP->aktualneNesie >= 1)
        {
            while (dataP->dataDruhe->aktualnaKapacita >= dataP->dataDruhe->maximalnaKapacita)
            {
                printf("Druhe miesto je plne, je tam %d balikov, prepravca caka\n", dataP->dataDruhe->aktualnaKapacita);
                pthread_cond_wait(dataP->dataDruhe->generujDruhe, dataP->dataDruhe->mutDruhe);
            }

            // vyloz druhy balik
            BALIK tempB2;
            tempB2.nazov = "balik";
            dataP->dataDruhe->baliky[dataP->dataDruhe->aktualnaKapacita] = tempB2;
            dataP->dataDruhe->aktualnaKapacita++;
            dataP->aktualneNesie--;
            dataP->aktualnePrepravil++;

            printf("Prepravca vylozil balik na druhom mieste, momentalne tam je %d balikov\n", dataP->dataDruhe->aktualnaKapacita);
        }

        if (dataP->aktualneNesie != 0)
        {
            printf("------------------------------------------------------------------------------------\n");
        }

        pthread_cond_signal(dataP->dataDruhe->odoberajDruhe);
        pthread_mutex_unlock(dataP->dataDruhe->mutDruhe);
    }

    printf("PREPRAVCA: KONIEC\n");
}

void * vykladacF(void * arg)    // konzument
{
    printf("VYKLADAC: ZACIATOK\n");

    VYK * dataV = arg;
    BALIK tempB;

    for (int i = 0; i < dataV->maximalneVylozi; i++)
    {
        pthread_mutex_lock(dataV->data->mutDruhe);

        while (dataV->data->aktualnaKapacita <= 0)
        {
            printf("Na druhom mieste je %d balikov, vykladac caka\n", dataV->data->aktualnaKapacita);
            pthread_cond_wait(dataV->data->odoberajDruhe, dataV->data->mutDruhe);
        }

        dataV->data->aktualnaKapacita--;
        tempB = dataV->data->baliky[dataV->data->aktualnaKapacita];
        printf("Vykladac zobral balik, momentalne sa na druhom mieste nachadza %d balikov\n", dataV->data->aktualnaKapacita);

        pthread_cond_signal(dataV->data->generujDruhe);
        pthread_mutex_unlock(dataV->data->mutDruhe);

        int prenos = rand() % (3 + 1 - 1) + 1;
        int dvojnasobok = rand() % 2;
        if (dvojnasobok == 0)
        {
            prenos = prenos * 2;
        }

        dataV->trvanie = dataV->trvanie + prenos;

        printf("Vykladac vyklada balik, bude mu to trvat %d sekund\n", prenos);
        sleep(prenos);
    }

    printf("VYKLADAC: KONIEC\n");
}


int main(int argc, char * argv[]) {
    printf("\nMAIN: ZACIATOK\n\n\n\n\n");
    srand(time(NULL));


    // vstupne argumenty
    int k;
    int n;

    if (argc < 2)
    {
        k = 5;
        n = 25;
    }
    else if (argc < 3)
    {
        k = atoi(argv[1]);
        n = 25;
    }
    else
    {
        k = atoi(argv[1]);
        n = atoi(argv[2]);
    }

    printf("k: %d\n", k);
    printf("n: %d\n", n);

    // inicializacia
    BALIK * prvySklad = calloc(k, sizeof(BALIK));
    BALIK * druhySklad = calloc(k, sizeof(BALIK));

    pthread_mutex_t mutPrvy = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutDruhy = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generujPrvy = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberajPrvy = PTHREAD_COND_INITIALIZER;
    pthread_cond_t generujDruhy = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberajDruhy = PTHREAD_COND_INITIALIZER;

    PRVE spPrve = {
            prvySklad, 0, k, &mutPrvy, &generujPrvy, &odoberajPrvy
    };

    DRUHE spDruhe = {
            druhySklad, 0, k, &mutDruhy, &generujDruhy, &odoberajDruhy
    };

    pthread_t balic;
    pthread_t prepravca;
    pthread_t vykladac;

    BAL balicD = {
          0, 0, n, &spPrve
    };

    PREP prepravcaD = {
            0, 0, n, &spPrve, &spDruhe, 0
    };

    VYK vykladacD = {
        0, 0, n, &spDruhe
    };


    // vykonavanie
    printf("So it begins\n");

    pthread_create(&balic, NULL, balicF, &balicD);
    pthread_create(&prepravca, NULL, prepravcaF, &prepravcaD);
    pthread_create(&vykladac, NULL, vykladacF, &vykladacD);


    // cakanie
    pthread_join(balic, NULL);
    pthread_join(prepravca, NULL);
    pthread_join(vykladac, NULL);


    // ukoncenie
    printf("It is done\n");

    printf("Balic pracoval %d sekund\n", balicD.trvanie);
    printf("Prepravca pracoval %d sekund\n", prepravcaD.trvanie);
    printf("Vykladac pracoval %d sekund\n", vykladacD.trvanie);


    // upratanie
    free(prvySklad);
    free(druhySklad);

    pthread_mutex_destroy(&mutPrvy);
    pthread_mutex_destroy(&mutDruhy);
    pthread_cond_destroy(&generujPrvy);
    pthread_cond_destroy(&generujDruhy);
    pthread_cond_destroy(&odoberajPrvy);
    pthread_cond_destroy(&odoberajDruhy);

    // ukoncenie
    printf("\n\n\n\n\nMAIN: KONIEC\n");
    return 0;
}

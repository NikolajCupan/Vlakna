#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// spolocne
typedef struct hra
{
    char * nazov;
} HRA;

typedef struct databaza
{
    int hodnotenie;
} DB;

typedef struct spolData
{
    HRA * pult;
    DB * databaza;
    int pocetHodnoteniVDB;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
    int pocetHierNaPulte;
    int kapacitaPultu;
    int zisk;
    int pocetNormalny;
    int pocetZberatel;
} SD;


// vlastne
typedef struct hrac
{
    SD * data;
    int id;
    int casDoPredajne;
    int casHrania;
    int hodnotenie;
    char * typHraca;
} HRAC;

typedef struct predajca
{
    SD * data;
    int dokopyVylozi;
    int doterazVylozil;
} PR;


// metody
void * predajcaF(void * arg)    // producent -> generuj
{
    // inicializacia
    PR * dataP = arg;
    HRA tempH;
    int caka = 0;

    printf("PREDAJCA: start!\n");


    // vykonavanie
    printf("Predajca zacina vykladanie hier, dokopy vylozi hier %d\n", dataP->dokopyVylozi);

    for (int i = 0; i < dataP->dokopyVylozi; i++)
    {
        tempH.nazov = "hra";
        printf("Predajca vygeneroval v poradi %d. hru\n", (dataP->doterazVylozil + 1));

        pthread_mutex_lock(dataP->data->mut);
        while (dataP->data->pocetHierNaPulte >= dataP->data->kapacitaPultu)
        {
            caka = 1;
            printf("Pult je plny, je na nom hier: %d\n", dataP->data->pocetHierNaPulte);
            pthread_cond_wait(dataP->data->generuj, dataP->data->mut);
        }

        if (caka == 1)
        {
            while (dataP->data->pocetHierNaPulte > 3)
            {
                printf("Predavac by uz chcel vykladat, ale caka kym na pulte budu iba 3 hry\n");
                pthread_cond_wait(dataP->data->generuj, dataP->data->mut);
            }

            caka = 0;
        }

        dataP->data->pult[dataP->data->pocetHierNaPulte] = tempH;
        dataP->data->pocetHierNaPulte++;
        printf("Predajca polozil hru na pult, momentalne je na pulte hier: %d\n", dataP->data->pocetHierNaPulte);
        dataP->doterazVylozil++;

        pthread_cond_signal(dataP->data->odoberaj);
        pthread_mutex_unlock(dataP->data->mut);
    }

    printf("Predajca ukoncil vykladanie hier, dokopy vylozil hier %d\n", dataP->doterazVylozil);


    // ukoncenie
    printf("PREDAJCA: end!\n");
}

void * hracF(void * arg)    // konzument -> odoberaj
{
    // inicializacia
    HRAC * dataH = arg;
    HRA tempH;

    printf("HRAC %d: start!\n", dataH->id);


    // vykonavanie
    printf("Hrac s ID %d ide do predajne, bude mu to trvat sekund: %d\n", dataH->id, dataH->casDoPredajne);
    sleep(dataH->casDoPredajne);

    pthread_mutex_lock(dataH->data->mut);

    while (dataH->data->pocetHierNaPulte <= 0)
    {
        printf("Hrac s ID %d musi cakat, nakolko na pulte je hier: %d\n", dataH->id, dataH->data->pocetHierNaPulte);
        pthread_cond_wait(dataH->data->odoberaj, dataH->data->mut);
    }

    dataH->data->pocetHierNaPulte--;
    tempH = dataH->data->pult[dataH->data->pocetHierNaPulte];
    printf("Hrac s ID %d zobral hru z pultu, na pulte ostalo hier: %d\n", dataH->id, dataH->data->pocetHierNaPulte);

    pthread_cond_signal(dataH->data->generuj);
    pthread_mutex_unlock(dataH->data->mut);

    printf("Hrac s ID %d ide domov, bude mu to trvat sekund: %d\n", dataH->id, dataH->casDoPredajne);
    sleep(dataH->casDoPredajne);

    printf("Hrac s ID %d ide hrat hru, bude mu to trvat sekund: %d\n", dataH->id, dataH->casHrania);
    sleep(dataH->casHrania);


    pthread_mutex_lock(dataH->data->mut);

    printf("Momentalne je v databaze hodnoteni: %d\n", dataH->data->pocetHodnoteniVDB);
    dataH->data->databaza[dataH->data->pocetHodnoteniVDB].hodnotenie = dataH->hodnotenie;
    dataH->data->pocetHodnoteniVDB++;
    printf("Hrac s ID %d, vlozil do DB hodnotenie: %d, momentalne je v DB hodnoteni: %d\n", dataH->id, dataH->hodnotenie, dataH->data->pocetHodnoteniVDB);

    if (strcmp(dataH->typHraca, "normalny") == 0)
    {
        dataH->data->zisk = dataH->data->zisk + 60;
        dataH->data->pocetNormalny++;
    }
    else
    {
        dataH->data->zisk = dataH->data->zisk + 75;
        dataH->data->pocetZberatel++;
    }

    pthread_cond_signal(dataH->data->generuj);
    pthread_mutex_unlock(dataH->data->mut);


    // ukoncenie
    printf("HRAC %d: end!\n", dataH->id);
}


// zaklad
int main(int argc, char * argv[]) {
    printf("\nMAIN: ZACIATOK\n\n\n\n\n");

    srand(time(NULL));


    // kontrola parametrov
    int pocetHracov;
    if (argc >= 2)
    {
        pocetHracov = atoi(argv[1]);
    }
    else
    {
        pocetHracov = 15;
    }


    // inicializacia
    int velkostPultu = 6;
    HRA * pult = calloc(velkostPultu, sizeof(HRA));
    DB * databaza = calloc(pocetHracov, sizeof(int));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;

    SD spData = { pult, databaza, 0, &mut, &generuj, &odoberaj, 0, 6, 0, 0, 0};

    pthread_t predajca;
    PR predajcaD = {
            &spData, pocetHracov, 0
    };

    pthread_t hraci[pocetHracov];
    HRAC hraciD[pocetHracov];


    // zaciatok
    printf("Predaj spusteny, kupte si svoje gca++!\n");


    // vytvorenie vlakien
    pthread_create(&predajca, NULL, predajcaF, &predajcaD);

    int nahoda;
    for (int i = 0; i < pocetHracov; i++)
    {
        hraciD[i].id = (i + 1);
        hraciD[i].data = &spData;

        nahoda = rand() % (5 + 1 - 1) + 1;
        hraciD[i].casDoPredajne = nahoda;

        nahoda = rand() % 100;
        if (nahoda < 65)
        {
            hraciD[i].typHraca = "normalny";
        }
        else
        {
            hraciD[i].typHraca = "zberatel";
        }

        nahoda = rand() % (3 + 1 - 1) + 1;
        hraciD[i].casHrania = nahoda;

        nahoda = rand() % (10 + 1 - 1) + 1;
        hraciD[i].hodnotenie = nahoda;

        pthread_create(&hraci[i], NULL, hracF, &hraciD[i]);
    }


    // cakanie
    for (int i = 0; i < pocetHracov; i++)
    {
        pthread_join(hraci[i], NULL);
    }
    pthread_join(predajca, NULL);


    // koniec
    printf("Predaj skonceny! No refunds!\n");


    // statistika
    printf("--------------------------------\n");

    int min = spData.databaza[0].hodnotenie;
    int max = spData.databaza[0].hodnotenie;
    int sum = 0;

    for (int i = 0; i < pocetHracov; i++)
    {
        int cur = spData.databaza[i].hodnotenie;

        if (cur < min)
        {
            min = cur;
        }

        if (cur > max)
        {
            max = cur;
        }

        sum = sum + cur;
    }
    float mean = (float)(sum / pocetHracov);

    printf("Minimalne hodnotenie: %d\n", min);
    printf("Maximalne hodnotenie: %d\n", max);
    printf("Priemerne hodnotenie: %f\n", mean);


    // vsetky hodnotenia
    printf("\n\n\n\n\n");
    for (int i = 0; i < pocetHracov; i++)
    {
        printf("%d: %d\n", i, spData.databaza[i].hodnotenie);
    }


    // upratanie
    free(pult);
    free(databaza);
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);


    printf("\n\n\n\n\nMAIN: KONIEC\n");
    return 0;
}
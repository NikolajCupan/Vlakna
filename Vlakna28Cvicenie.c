#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

typedef struct suradnica
{
    int x;
    int y;
} SUR;

typedef struct spolData
{
    SUR * nastenka;
    int maxPocetSuradnic;
    int pocetSuradnic;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odober;
} SD;

typedef struct banik
{
    int id;
    int pocetTazieb;
    unsigned int zarobok;
    SD * data;
} BANIK;

typedef struct prospektor
{
    int pocetSuradnic;
    SD * data;
} PROS;

void * banikF(void * arg)   // konzument -> odoberaj
{
    BANIK * dataB = arg;
    SUR tempS;
    double vzdialenost, casPresunu;
    int nahoda, typ;

    printf("Banik %d nastupuje na sichtu!\n", dataB->id);

    for (int i = 0; i < dataB->pocetTazieb; i++)
    {
        printf("Banik %d ide pre %d. suradnicu!\n", dataB->id, (i + 1));


        pthread_mutex_lock(dataB->data->mut);

        while (dataB->data->pocetSuradnic <= 0)
        {
            printf("Banik %d, tabulka je prazdna %d!\n", dataB->id, dataB->data->pocetSuradnic);
            pthread_cond_wait(dataB->data->odober, dataB->data->mut);
        }
        dataB->data->pocetSuradnic--;
        tempS = dataB->data->nastenka[dataB->data->pocetSuradnic];
        printf("Banik %d ziskal suradnicu, nastenka ma teraz %d suradnic!\n", dataB->id, dataB->data->pocetSuradnic);

        // posielam spravu iba vtedy, ked je nastenka prazdna
        if (dataB->data->pocetSuradnic == 0)
        {
            pthread_cond_signal(dataB->data->generuj);
        }

        pthread_mutex_unlock(dataB->data->mut);


        printf("Banik %d ziskal suradnicu x = %d, y = %d!\n", dataB->id, tempS.x, tempS.y);
        vzdialenost = sqrt(pow(tempS.x, 2) + pow(tempS.y, 2));
        casPresunu = vzdialenost / 5 * 1000000;
        printf("Banik %d sa vydal na cestu dlhu %.3lf dm, ktora bude trvat %.3lf sekund!\n", dataB->id, vzdialenost, casPresunu);

        usleep((__useconds_t)casPresunu);

        // vyber rudy, ktoru bude tazit
        nahoda = rand() % 100;

        if (nahoda < 45)
        {
            // ZELEZO
            typ = 5;
            nahoda = 1 + rand() % 10;
        }
        else if (nahoda < 75)
        {
            // ZLATO
            typ = 15;
            nahoda = 1 + rand() % 6;
        }
        else if (nahoda < 90)
        {
            // DIAMANTY
            typ = 50;
            nahoda = 1 + rand() % 3;
        }
        else
        {
            // NIC
            typ = 0;
            nahoda = 0;
        }

        printf("Banik %d: vytazil som %d rudy o cene %d, idem spat!\n", dataB->id, nahoda, (nahoda * typ));

        if ((rand() % 100) < 25)
        {
            printf("Banik %d hlasi stret s Creepsterom!\n", dataB->id);
            typ = 0;
            nahoda = 0;
        }

        usleep((__useconds_t)casPresunu);

        dataB->zarobok += (typ * nahoda);
        printf("Banik %d ma momentalne po navrate na konte %u!\n", dataB->id, dataB->zarobok);
    }

    printf("Banik %d konci sichtu, celkovo zarobil %u!\n", dataB->id, dataB->zarobok);
}

void * prospektorF(void * arg)  // producent -> generuj
{
    PROS * dataP = arg;
    SUR tempS;

    for (int i = 0; i < dataP->pocetSuradnic; i++)
    {
        tempS.x = -20 + rand() % 41;
        tempS.y = -50 + rand() % 101;
        printf("Prospektor vygeneroval %d. suradnice x = %d, y = %d!\n", (i + 1), tempS.x, tempS.y);


        pthread_mutex_lock(dataP->data->mut);
        while (dataP->data->pocetSuradnic >= dataP->data->maxPocetSuradnic)
        {
            printf("Prospektor musi cakat, na nastanke je %d suradnic!\n", dataP->data->pocetSuradnic);
            pthread_cond_wait(dataP->data->generuj, dataP->data->mut);
        }

        dataP->data->nastenka[dataP->data->pocetSuradnic] = tempS;
        dataP->data->pocetSuradnic++;
        printf("Prospektor vlozil suradnicu, na nastenke je %d suradnic!\n", dataP->data->pocetSuradnic);

        pthread_cond_signal(dataP->data->odober);
        pthread_mutex_unlock(dataP->data->mut);


        usleep(1 + rand() % 3);
    }

    printf("Prospektor nastupuje na sichtu!\n");
}

int main(int argc, char * argv[]) {
    if (argc < 3)
    {
        fprintf(stderr, "Nedostatocny pocet parametrov!\n");
        return 1;
    }

    int pocetBanikov = atoi(argv[1]);
    int velkostNastenky = atoi(argv[2]);
    int pocetTazieb = 20;

    SUR * nastenka = calloc(velkostNastenky, sizeof(SUR));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odober = PTHREAD_COND_INITIALIZER;

    SD spData = {
            nastenka, velkostNastenky, 0, &mut, &generuj, &odober
    };

    pthread_t prospektor;
    PROS prospektorD = {
            pocetBanikov * pocetTazieb, &spData
    };

    pthread_t banici[pocetBanikov];
    BANIK baniciD[pocetBanikov];

    printf("MAIN: Zacina banicka cinnost v 2DrinkStone!\n");

    pthread_create(&prospektor, NULL, prospektorF, &prospektorD);
    for (int i = 0; i < pocetBanikov; i++)
    {
        baniciD[i].id = (i + 1);
        baniciD[i].pocetTazieb = pocetTazieb;
        baniciD[i].zarobok = 0;
        baniciD[i].data = &spData;

        pthread_create(&banici[i], NULL, banikF, &baniciD[i]);
    }

    for (int i = 0; i < pocetBanikov; i++)
    {
        pthread_join(banici[i], NULL);
    }
    pthread_join(prospektor, NULL);

    printf("MAIN: Konci banicka cinnost v 2DrinkStone!\n");

    free(nastenka);
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odober);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>


// spolocne
typedef struct suradnica
{
    int x;
    int y;
} SUR;

typedef struct spolocneData
{
    SUR * nastenka;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
    int aktualneSuradnic;
    int maximalneSuradnic;
} SD;


// vlastne
typedef struct banik
{
    SD * data;
    int id;
    int aktualneTazieb;
    int maximalneTazieb;
    int aktualnyZarobok;
} BAN;

typedef struct prospektor
{
    SD * data;
    int aktualneVygeneroval;
    int maximalneVygeneruje;
} PROS;


// metody
void * banikF(void * arg)   // konzument, odoberaj
{
    BAN * dataB = arg;
    SUR tempS;

    printf("BANIK s ID %d: START\n", dataB->id);

    for (int i = 0; i < dataB->maximalneTazieb; i++)
    {
        pthread_mutex_lock(dataB->data->mut);

        while (dataB->data->aktualneSuradnic <= 0)
        {
            printf("Banik s ID %d caka, nakolko na nastenke je %d suradnic\n", dataB->id, dataB->data->aktualneSuradnic);
            pthread_cond_wait(dataB->data->odoberaj, dataB->data->mut);
        }

        dataB->data->aktualneSuradnic--;
        tempS = dataB->data->nastenka[dataB->data->aktualneSuradnic];

        printf("Banik s ID %d si zobral suradnice (x = %d, y = %d z nastenky), ostalo tam %d suradnic\n", dataB->id, tempS.x, tempS.y, dataB->data
        ->aktualneSuradnic);

        pthread_cond_signal(dataB->data->generuj);
        pthread_mutex_unlock(dataB->data->mut);

        double vzdialenost = sqrt(pow(tempS.x, 2) + pow(tempS.y, 2));
        double trvanie = vzdialenost / 5;
        int mikrosekundy = (trvanie * 1000000);

        printf("Banik s ID %d sa vydava na x = %d, y = %d, bude mu to trvat %d\n", dataB->id, tempS.x, tempS.y, mikrosekundy);
        usleep(mikrosekundy);

        int nahoda = rand() % 100;
        int pocet;
        int ruda;
        int zisk;

        if (nahoda < 45)
        {
            ruda = 1;
            pocet = rand() % 10 + 1;
            zisk = pocet * 5;
        }
        else if (nahoda < 75)
        {
            ruda = 2;
            pocet = rand() % 6 + 1;
            zisk = pocet * 15;
        }
        else if (nahoda < 90)
        {
            ruda = 3;
            pocet = rand() % 3 + 1;
            zisk = pocet * 50;
        }
        else
        {
            ruda = 4;
            pocet = 0;
            zisk = 0;
        }

        nahoda = rand() % 100;

        if (nahoda < 25)
        {
            printf("Banik s ID %d stretol Creepra, stratil vsetky rudy\n", dataB->id);
            ruda = 0;
            pocet = 0;
            zisk = 0;
        }

        printf("Banik s ID %d ide domov, bude mu to trvat %d\n", dataB->id, mikrosekundy);
        usleep(mikrosekundy);

        dataB->aktualnyZarobok = dataB->aktualnyZarobok + zisk;
        printf("Banik s ID %d je doma, ruda: %d, pocet: %d, zisk %d, celkovy zisk: %d\n", dataB->id, ruda, pocet, zisk, dataB->aktualnyZarobok);

        dataB->aktualneTazieb++;
    }

    printf("BANIK s ID %d: KONIEC, celkovy zarobok: %d\n", dataB->id, dataB->aktualnyZarobok);
}

void * prospektorF(void * arg)  // producent, generuj
{
    PROS * dataP = arg;
    SUR tempS;

    printf("PROSPEKTOR: START\n");

    for (int i = 0; i < dataP->maximalneVygeneruje; i++)
    {
        int cakanie = rand() % 3 + 1;
        tempS.x = rand() % 41 - 20;
        tempS.y = rand() % 101 - 50;

        while (dataP->data->aktualneSuradnic >= dataP->data->maximalneSuradnic)
        {
            printf("Nastenka je plna, je na nej %d suradnic, prospektor caka\n", dataP->data->aktualneSuradnic);
            pthread_cond_wait(dataP->data->generuj, dataP->data->mut);
        }

        dataP->data->nastenka[dataP->data->aktualneSuradnic] = tempS;
        dataP->data->aktualneSuradnic++;
        printf("Prospektor pridal suradnicu na nastenku, momentalne tam je %d suradnic\n", dataP->data->aktualneSuradnic);

        pthread_cond_signal(dataP->data->odoberaj);
        pthread_mutex_unlock(dataP->data->mut);


        dataP->aktualneVygeneroval++;

        double spanok = rand() % 3 + 1;
        printf("Prospektor spi %f\n", spanok);
        spanok = spanok * 1000000;
        usleep(spanok);
    }

    printf("PROSPEKTOR: KONIEC\n");
}


int main(int argc, char * argv[]) {
    printf("\nMAIN: ZACIATOK\n\n\n\n\n");

    int kapacitaNastenky;
    int pocetBanikov;
    if (argc < 3)
    {
        fprintf(stderr, "Nedostatok parametrov\n");
        return 1;
    }

    kapacitaNastenky = atoi(argv[1]);
    pocetBanikov = atoi(argv[2]);

    printf("Kapacita nastenky: %d\n", kapacitaNastenky);
    printf("Pocet banikov: %d\n\n", pocetBanikov);

    SUR * nastenka = calloc(kapacitaNastenky, sizeof(SUR));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;


    SD spData = {
            nastenka, &mut, &generuj, &odoberaj, 0, kapacitaNastenky
    };

    pthread_t prospektor;
    PROS prospektorD = {
            &spData, 0, (pocetBanikov * 20)
    };

    pthread_create(&prospektor, NULL, prospektorF, &prospektorD);


    pthread_t banici[pocetBanikov];
    BAN baniciD[pocetBanikov];

    for (int i = 0; i < pocetBanikov; i++)
    {
        baniciD[i].id = (i + 1);
        baniciD[i].data = &spData;
        baniciD[i].aktualneTazieb = 0;
        baniciD[i].maximalneTazieb = 20;
        baniciD[i].aktualnyZarobok = 0;

        pthread_create(&banici[i], NULL, banikF, &baniciD[i]);
    }



    pthread_join(prospektor, NULL);

    for (int i = 0; i < pocetBanikov; i++)
    {
        pthread_join(banici[i], NULL);
    }


    free(nastenka);

    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);


    printf("\n\n\n\n\nMAIN: KONIEC\n");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>


// spolocne
typedef struct volebnyHarok
{
    char * meno;
} VH;

typedef struct spolData
{
    int pocetVolicov;
    VH * stol;
    VH * databaza;
    int aktualnaKapacitaStola;
    int maximalnaKapacitaStola;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;

    pthread_mutex_t * mutDatabaza;
    pthread_cond_t * hlasuj;
    int hlasuje;
    int pocetHlasov;
} SD;


// vlastne
typedef struct volic
{
    SD * data;
    int id;
    char * preferencia;
    int kracanie;
} VOL;

typedef struct komisia
{
    SD * data;
    int maximalneVylozi;
} KOM;


// metody
void * volicF(void * arg)   // konzument
{
    VOL * dataV = arg;
    VH tempVH;
    tempVH.meno = dataV->preferencia;

    printf("Volic s ID %d start\n", dataV->id);

    printf("Volic s ID %d ide hlasovat, cesta mu bude trvat sekund: %d\n", dataV->id, dataV->kracanie);
    sleep(dataV->kracanie);

    pthread_mutex_lock(dataV->data->mut);

    while (dataV->data->aktualnaKapacitaStola <= 0)
    {
        printf("Volic s ID %d caka, na stole je %d listkov\n", dataV->id, dataV->data->aktualnaKapacitaStola);
        pthread_cond_wait(dataV->data->odoberaj, dataV->data->mut);
    }

    dataV->data->aktualnaKapacitaStola--;
    tempVH = dataV->data->stol[dataV->data->aktualnaKapacitaStola];
    printf("Volic s ID %d si vzal listok, na stole ich ostalo %d\n", dataV->id, dataV->data->aktualnaKapacitaStola);

    pthread_cond_signal(dataV->data->generuj);
    pthread_mutex_unlock(dataV->data->mut);


    pthread_mutex_lock(dataV->data->mutDatabaza);

    while (dataV->data->hlasuje > 0)
    {
        printf("Volic s ID %d caka pred kabinkou, nakolko niekto hlasuje\n", dataV->id);
        pthread_cond_wait(dataV->data->hlasuj, dataV->data->mutDatabaza);
    }

    printf("Kabinka sa uvolnila, volic s ID %d ide hlasovat\n", dataV->id);
    tempVH.meno = dataV->preferencia;
    dataV->data->databaza[dataV->data->pocetHlasov] = tempVH;
    dataV->data->pocetHlasov++;

    int cakanie = rand() % (150 + 1 - 100) + 100;

    dataV->data->hlasuje = 1;
    usleep(cakanie);
    dataV->data->hlasuje = 0;

    pthread_cond_signal(dataV->data->hlasuj);
    pthread_mutex_unlock(dataV->data->mutDatabaza);


    printf("Volic s ID %d koniec\n", dataV->id);
}

void * komisiaF(void * arg) // producent
{
    KOM * dataK = arg;
    VH tempVH;
    tempVH.meno = "listok";
    int caka = 0;

    printf("Komisia start\n");

    for (int i = 0; i < dataK->data->pocetVolicov; i++)
    {
        pthread_mutex_lock(dataK->data->mut);

        while (dataK->data->aktualnaKapacitaStola >= dataK->data->maximalnaKapacitaStola)
        {
            caka = 1;
            printf("Stol je plny %d, cakaju\n", dataK->data->aktualnaKapacitaStola);
            pthread_cond_wait(dataK->data->generuj, dataK->data->mut);
        }

        if (caka == 1)
        {
            while (dataK->data->aktualnaKapacitaStola > 2)
            {
                printf("Na stole je %d, ale cakaju kym bude pod 3\n", dataK->data->aktualnaKapacitaStola);
                pthread_cond_wait(dataK->data->generuj, dataK->data->mut);
            }

            caka = 0;
        }


        dataK->data->stol[dataK->data->aktualnaKapacitaStola] = tempVH;
        dataK->data->aktualnaKapacitaStola++;
        printf("Komisia pridala listok na stol, momentalne tam je %d listkov\n", dataK->data->aktualnaKapacitaStola);


        pthread_cond_signal(dataK->data->odoberaj);
        pthread_mutex_unlock(dataK->data->mut);


        printf("");
    }

    printf("Komisia koniec\n");
}


int main(int argc, char * argv[]) {
    printf("\nMAIN: START\n\n\n\n\n");
    srand(time(NULL));

    int kapacitaStola = 5;
    int pocetVolicov;
    if (argc < 2)
    {
        pocetVolicov = 15;
    }
    else
    {
        pocetVolicov = atoi(argv[1]);
    }


    VH * stol = calloc(kapacitaStola, sizeof(VH));
    VH * databaza = calloc(pocetVolicov, sizeof(VH));


    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutDruhy = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t hlasuj = PTHREAD_COND_INITIALIZER;


    SD spData = {
            pocetVolicov, stol, databaza, 0, kapacitaStola, &mut, &generuj, &odoberaj,
          &mutDruhy, &hlasuj, 0, 0
    };

    pthread_t komisia;
    KOM komisiaD = {
        &spData, pocetVolicov
    };

    pthread_t volici[pocetVolicov];
    VOL voliciD[pocetVolicov];


    printf("Hlasovanie zacina\n");


    pthread_create(&komisia, NULL, komisiaF, &komisiaD);

    for (int i = 0; i < pocetVolicov; i++)
    {
        voliciD[i].id = (i + 1);
        voliciD[i].data = &spData;

        int nahoda = rand() % 100;

        if (nahoda < 65)
        {
            voliciD[i].preferencia = "Puttyn";
        }
        else
        {
            voliciD[i].preferencia = "Bash";
        }

        int kracanie = rand() % (10 + 1 - 1) + 1;
        voliciD[i].kracanie = kracanie;

        pthread_create(&volici[i], NULL, volicF, &voliciD[i]);
    }


    pthread_join(komisia, NULL);

    for (int i = 0; i < pocetVolicov; i++)
    {
        pthread_join(volici[i], NULL);
    }

    printf("Volby ukoncene\n");


    for (int i = 0; i < pocetVolicov; i++)
    {
        printf("%s\n", spData.databaza[i].meno);
    }


    free(stol);
    free(databaza);
    pthread_mutex_destroy(&mut);
    pthread_mutex_destroy(&mutDruhy);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);
    pthread_cond_destroy(&hlasuj);


    printf("\n\n\n\n\nMAIN: END\n");
    return 0;
}

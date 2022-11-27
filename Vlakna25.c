#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


typedef struct huba
{
    char * nazovHuby;
    int cenaHuby;
} HUB;

typedef struct spolData
{
    HUB * pult;
    pthread_mutex_t * mut;
    pthread_cond_t * generuj;
    pthread_cond_t * odoberaj;
    int aktualneHub;
    int maxHub;
} SD;


typedef struct hubar
{
    int id;
    SD * data;
    int zarobok;
    int aktualnyPocetHub;
    int prvotnyPocetHub;
    float casCesty;
} HUBAR;

typedef struct susic
{
    SD * data;
    int poctyHub[4];
    int presunJedlej;
    int presunJedovatej;
    int spracujeHub;
    int spracovalHub;
} SUSIC;


void * susicF(void * arg)
{
    SUSIC * dataS = arg;
    HUB tempH;

    HUB inventar[5];
    int pocetHubVInventari;

    printf("Susic sa narodil!\n");

    while (dataS->spracovalHub != dataS->spracujeHub)
    {
        pthread_mutex_lock(dataS->data->mut);

        while (dataS->data->aktualneHub <= 0)
        {
            printf("Susic musi cakat, na pulte je %d hub!\n", dataS->data->aktualneHub);
            pthread_cond_wait(dataS->data->odoberaj, dataS->data->mut);
        }

        printf("Susic presuva hub: %d do svojho inventara!\n", dataS->data->aktualneHub);
        pocetHubVInventari = dataS->data->aktualneHub;
        for (int j = 0; j < pocetHubVInventari; j++)
        {
            inventar[j] = dataS->data->pult[j];
        }
        dataS->data->aktualneHub = 0;
        printf("Huby boli presunute na pulte ostalo hub: %d\n", dataS->data->aktualneHub);

        pthread_cond_broadcast(dataS->data->generuj);
        pthread_mutex_unlock(dataS->data->mut);


        printf("Susic berie na susenie hub: %d!\n", pocetHubVInventari);
        for (int j = 0; j < pocetHubVInventari; j++)
        {
            tempH.nazovHuby = inventar[j].nazovHuby;
            tempH.cenaHuby = inventar[j].cenaHuby;

            printf("Susic zobral hubu %s zo svojho inventara, jej hodnota je %d!\n", tempH.nazovHuby, tempH.cenaHuby);

            if (strcmp(tempH.nazovHuby, "muchotravka") != 0)
            {
                printf("Huba je jedla, susic ju vklada do prepravky!\n");
                sleep(dataS->presunJedlej);
                dataS->spracovalHub++;

                if (strcmp(tempH.nazovHuby, "bedla") == 0)
                {
                    dataS->poctyHub[0] = dataS->poctyHub[0] + 1;
                }
                else if (strcmp(tempH.nazovHuby, "dubak") == 0)
                {
                    dataS->poctyHub[1] = dataS->poctyHub[1] + 1;
                }
                else if (strcmp(tempH.nazovHuby, "kozak") == 0)
                {
                    dataS->poctyHub[2] = dataS->poctyHub[2] + 1;
                }
            }
            else
            {
                printf("Huba nie je jedla, susic ju vyhadzuje!\n");
                sleep(dataS->presunJedovatej);
                dataS->spracovalHub++;

                dataS->poctyHub[3] = dataS->poctyHub[3] + 1;
            }
        }
        printf("Susic dokoncil susenie, vracia sa k pultu, dokopy uz spracoval hub: %d!\n", dataS->spracovalHub);
    }
    printf("Cinnost susica konci!\n");
    printf("Bedla: %d, Dubak: %d, Kozak: %d, Muchotravka: %d\n", dataS->poctyHub[0], dataS->poctyHub[1], dataS->poctyHub[2], dataS->poctyHub[3]);
    printf("Susic zanikol!\n");
}

void * hubarF(void * arg)
{
    HUBAR * dataH = arg;
    HUB tempH;

    printf("Hubar s ID %d sa narodil!\n", dataH->id);

    for (int i = 0; i < dataH->prvotnyPocetHub; i++)
    {
        int nahoda = rand() % 100;

        if (nahoda < 25)
        {
            tempH.nazovHuby = "bedla";
            tempH.cenaHuby = 10;
        }
        else if (nahoda < 35)
        {
            tempH.nazovHuby = "dubak";
            tempH.cenaHuby = 20;
        }
        else if (nahoda < 60)
        {
            tempH.nazovHuby = "kozak";
            tempH.cenaHuby = 5;
        }
        else
        {
            tempH.nazovHuby = "muchotravka";
            tempH.cenaHuby = 0;
        }

        printf("Hubar s ID %d, zobral hubu %s a ide do predajne, bude mu to trvat sekund: %f!\n", dataH->id, tempH.nazovHuby, dataH->casCesty);
        int microsekundy = (dataH->casCesty * 1000000);
        usleep(microsekundy);


        pthread_mutex_lock(dataH->data->mut);

        while (dataH->data->aktualneHub >= dataH->data->maxHub)
        {
            printf("Hubar s ID %d musi cakat, na pulte je %d hub!\n", dataH->id, dataH->data->aktualneHub);
            pthread_cond_wait(dataH->data->generuj, dataH->data->mut);
        }

        dataH->data->pult[dataH->data->aktualneHub] = tempH;
        dataH->data->aktualneHub++;
        printf("Hubar s ID %d polozil na pult hubu, na pulte je hub: %d!\n", dataH->id, dataH->data->aktualneHub);
        dataH->zarobok = dataH->zarobok + tempH.cenaHuby;

        pthread_cond_signal(dataH->data->odoberaj);
        pthread_mutex_unlock(dataH->data->mut);


        printf("Hubar s ID %d, sa vracia ku autu, bude mu to trvat sekund: %f!\n", dataH->id, dataH->casCesty);
    }
    printf("Hubar s ID %d predal vsetky svoje huby!\n", dataH->id);
}


int main(int argc, char * argv[]) {
    printf("MAIN: zaciatok\n\n\n\n\n");


    srand(time(NULL));


    int velkostPultu = 5;
    int pocetHubarov;
    if (argc >= 2)
    {
        pocetHubarov = atoi(argv[1]);
    }
    else
    {
        pocetHubarov = 5;
    }

    HUB * pult = calloc(velkostPultu, sizeof(HUB));

    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t generuj = PTHREAD_COND_INITIALIZER;
    pthread_cond_t odoberaj = PTHREAD_COND_INITIALIZER;

    SD spData = {
            pult, &mut, &generuj, &odoberaj, 0, velkostPultu
    };

    pthread_t susic;
    SUSIC susicD = {
            .data = &spData, .presunJedlej = 2, .presunJedovatej = 0, .spracovalHub = 0, .spracujeHub = pocetHubarov * 15
    };

    pthread_t hubari[pocetHubarov];
    HUBAR hubariD[pocetHubarov];

    printf("Cas susenia nastal, hor na to!\n");


    pthread_create(&susic, NULL, susicF, &susicD);

    for (int i = 0; i < pocetHubarov; i++)
    {
        float rychlost = ((float)rand() / RAND_MAX) + 1;

        hubariD[i].id = (i + 1);
        hubariD[i].data = &spData;
        hubariD[i].zarobok = 0;
        hubariD[i].aktualnyPocetHub = 15;
        hubariD[i].prvotnyPocetHub = 15;
        hubariD[i].casCesty = rychlost;

        pthread_create(&hubari[i], NULL, hubarF, &hubariD[i]);
    }


    // cakanie na vlakna
    for (int i = 0; i < pocetHubarov; i++)
    {
        pthread_join(hubari[i], NULL);
    }
    pthread_join(susic, NULL);

    printf("Je nasusene, parada!\n");


    // statistiky hubarov
    int celkovyZarobok = 0;
    for (int i = 0; i < pocetHubarov; i++)
    {
        celkovyZarobok = celkovyZarobok + hubariD[i].zarobok;
    }

    printf("Celkovy zarobok bol %d, z toho:\n", celkovyZarobok);
    for (int i = 0; i < pocetHubarov; i++)
    {
        float percento;
        percento = (float)(hubariD[i].zarobok / celkovyZarobok);
        printf("Hubar s ID %d zarobil %d, co tvori %f percent z celkoveho zarobku!\n", hubariD[i].id, hubariD[i].zarobok, percento);
    }

    // upratanie
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&generuj);
    pthread_cond_destroy(&odoberaj);

    printf("\n\n\n\n\nMAIN: koniec\n");
    return 0;
}
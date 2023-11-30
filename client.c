#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>


#include "utils.h"
#include "myassert.h"

#include "client_master.h"


/************************************************************************
 * chaines possibles pour le premier paramètre de la ligne de commande
 ************************************************************************/
#define TK_STOP        "stop"             // arrêter le master
#define TK_HOW_MANY    "howmany"          // combien d'éléments dans l'ensemble
#define TK_MINIMUM     "min"              // valeur minimale de l'ensemble
#define TK_MAXIMUM     "max"              // valeur maximale de l'ensemble
#define TK_EXIST       "exist"            // test d'existence d'un élément, et nombre d'exemplaires
#define TK_SUM         "sum"              // somme de tous les éléments
#define TK_INSERT      "insert"           // insertion d'un élément
#define TK_INSERT_MANY "insertmany"       // insertions de plusieurs éléments aléatoires
#define TK_PRINT       "print"            // debug : demande aux master/workers d'afficher les éléments
#define TK_LOCAL       "local"            // lancer un calcul local (sans master) en multi-thread


/************************************************************************
 * structure stockant les paramètres du client
 * - les infos pour communiquer avec le master
 * - les infos pour effectuer le travail (cf. ligne de commande)
 *   (note : une union permettrait d'optimiser la place mémoire)
 ************************************************************************/
typedef struct
{
    // communication avec le master
    //TODO

    int clientToMaster;      // Premier tube pour communiquer avec le master (client -> master)
    int masterToClient;      // Deuxième tube pour communiquer avec le master (master -> client)
    int semCM;    // Sémaphore pour synchroniser avec le master

    // infos pour le travail à faire (récupérées sur la ligne de commande)
    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL
} Data;


/************************************************************************
 * Usage
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usages : %s <ordre> [[[<param1>] [<param2>] ...]]\n", exeName);
    fprintf(stderr, "   $ %s " TK_STOP "\n", exeName);
    fprintf(stderr, "          arrêt master\n");
    fprintf(stderr, "   $ %s " TK_HOW_MANY "\n", exeName);
    fprintf(stderr, "          combien d'éléments dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MINIMUM "\n", exeName);
    fprintf(stderr, "          plus petite valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MAXIMUM "\n", exeName);
    fprintf(stderr, "          plus grande valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_EXIST " <elt>\n", exeName);
    fprintf(stderr, "          l'élement <elt> est-il présent dans l'ensemble ?\n");
    fprintf(stderr, "   $ %s " TK_SUM "\n", exeName);
    fprintf(stderr, "           somme des éléments de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT " <elt>\n", exeName);
    fprintf(stderr, "          ajout de l'élement <elt> dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT_MANY " <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          ajout de <nb> élements (dans [<min>,<max>[) aléatoires dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_PRINT "\n", exeName);
    fprintf(stderr, "          affichage trié (dans la console du master)\n");
    fprintf(stderr, "   $ %s " TK_LOCAL " <nbThreads> <elt> <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          combien d'exemplaires de <elt> dans <nb> éléments (dans [<min>,<max>[)\n"
            "          aléatoires avec <nbThreads> threads\n");

    if (message != NULL)
        fprintf(stderr, "message :\n    %s\n", message);

    exit(EXIT_FAILURE);
}


/************************************************************************
 * Analyse des arguments passés en ligne de commande
 ************************************************************************/
static void parseArgs(int argc, char * argv[], Data *data)
{
    data->order = CM_ORDER_NONE;

    if (argc == 1)
        usage(argv[0], "Il faut préciser une commande");

    // première vérification : la commande est-elle correcte ?
    if (strcmp(argv[1], TK_STOP) == 0)
        data->order = CM_ORDER_STOP;
    else if (strcmp(argv[1], TK_HOW_MANY) == 0)
        data->order = CM_ORDER_HOW_MANY;
    else if (strcmp(argv[1], TK_MINIMUM) == 0)
        data->order = CM_ORDER_MINIMUM;
    else if (strcmp(argv[1], TK_MAXIMUM) == 0)
        data->order = CM_ORDER_MAXIMUM;
    else if (strcmp(argv[1], TK_EXIST) == 0)
        data->order = CM_ORDER_EXIST;
    else if (strcmp(argv[1], TK_SUM) == 0)
        data->order = CM_ORDER_SUM;
    else if (strcmp(argv[1], TK_INSERT) == 0)
        data->order = CM_ORDER_INSERT;
    else if (strcmp(argv[1], TK_INSERT_MANY) == 0)
        data->order = CM_ORDER_INSERT_MANY;
    else if (strcmp(argv[1], TK_PRINT) == 0)
        data->order = CM_ORDER_PRINT;
    else if (strcmp(argv[1], TK_LOCAL) == 0)
        data->order = CM_ORDER_LOCAL;
    else
        usage(argv[0], "commande inconnue");

    // deuxième vérification : nombre de paramètres correct ?
    if ((data->order == CM_ORDER_STOP) && (argc != 2))
        usage(argv[0], TK_STOP " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_HOW_MANY) && (argc != 2))
        usage(argv[0], TK_HOW_MANY " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MINIMUM) && (argc != 2))
        usage(argv[0], TK_MINIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MAXIMUM) && (argc != 2))
        usage(argv[0], TK_MAXIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_EXIST) && (argc != 3))
        usage(argv[0], TK_EXIST " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_SUM) && (argc != 2))
        usage(argv[0], TK_SUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_INSERT) && (argc != 3))
        usage(argv[0], TK_INSERT " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_INSERT_MANY) && (argc != 5))
        usage(argv[0], TK_INSERT_MANY " : il faut 3 arguments après la commande");
    if ((data->order == CM_ORDER_PRINT) && (argc != 2))
        usage(argv[0], TK_PRINT " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_LOCAL) && (argc != 7))
        usage(argv[0], TK_LOCAL " : il faut 5 arguments après la commande");

    // extraction des arguments
    if (data->order == CM_ORDER_EXIST)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT_MANY)
    {
        data->nb = strtol(argv[2], NULL, 10);
        data->min = strtof(argv[3], NULL);
        data->max = strtof(argv[4], NULL);
        if (data->nb < 1)
            usage(argv[0], TK_INSERT_MANY " : nb doit être strictement positif");
        if (data->max < data->min)
            usage(argv[0], TK_INSERT_MANY " : max ne doit pas être inférieur à min");
    }
    else if (data->order == CM_ORDER_LOCAL)
    {
        data->nbThreads = strtol(argv[2], NULL, 10);
        data->elt = strtof(argv[3], NULL);
        data->nb = strtol(argv[4], NULL, 10);
        data->min = strtof(argv[5], NULL);
        data->max = strtof(argv[6], NULL);
        if (data->nbThreads < 1)
            usage(argv[0], TK_LOCAL " : nbThreads doit être strictement positif");
        if (data->nb < 1)
            usage(argv[0], TK_LOCAL " : nb doit être strictement positif");
        if (data->max <= data->min)
            usage(argv[0], TK_LOCAL " : max ne doit être strictement supérieur à min");
    }
}


/************************************************************************
 * Partie multi-thread
 ************************************************************************/
//TODO Une structure pour les arguments à passer à un thread (aucune variable globale autorisée)

//TODO
// Code commun à tous les threads
// Un thread s'occupe d'une portion du tableau et compte en interne le nombre de fois
// où l'élément recherché est présent dans cette portion. On ajoute alors,
// en section critique, ce nombre au compteur partagé par tous les threads.
// Le compteur partagé est la variable "result" de "lauchThreads".
// A vous de voir les paramètres nécessaires  (aucune variable globale autorisée)
//END TODO

// Structure pour les arguments à passer à un thread
typedef struct
{
    float *tab;    // Pointeur vers le tableau
    int startIdx;  // Indice de début de la portion du tableau à traiter
    int endIdx;    // Indice de fin de la portion du tableau à traiter
    float elt;     // Elément recherché
    int *result;   // Compteur partagé par tous les threads
    pthread_mutex_t *mutex;  // Mutex pour la section critique
} ThreadArgs;

// Code commun à tous les threads
void *threadFunction(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int localCount = 0;

    // Parcours de la portion du tableau assignée au thread
    for (int i = threadArgs->startIdx; i < threadArgs->endIdx; ++i)
    {
        if (threadArgs->tab[i] == threadArgs->elt)
        {
            localCount++;
        }
    }

    // Section critique : ajout du résultat local au compteur partagé
    pthread_mutex_lock(threadArgs->mutex);
    *(threadArgs->result) += localCount;
    pthread_mutex_unlock(threadArgs->mutex);

    pthread_exit(NULL);
}


void lauchThreads(const Data *data)
{
    //TODO déclarations nécessaires : mutex, ...
    int result = 0;
    float * tab = ut_generateTab(data->nb, data->min, data->max, 0);

    // Initialisation du mutex
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    //TODO lancement des threads
    pthread_t *threads = malloc(data->nbThreads * sizeof(pthread_t));
    ThreadArgs *threadArgsArray = malloc(data->nbThreads * sizeof(ThreadArgs));

    int portionSize = data->nb / data->nbThreads;
    int remainder = data->nb % data->nbThreads;
    int currentIndex = 0;

    for (int i = 0; i < data->nbThreads; ++i)
    {
        threadArgsArray[i].tab = tab;
        threadArgsArray[i].startIdx = currentIndex;
        threadArgsArray[i].endIdx = currentIndex + portionSize + (i < remainder ? 1 : 0);
        threadArgsArray[i].elt = data->elt;
        threadArgsArray[i].result = &result;
        threadArgsArray[i].mutex = &mutex;

        pthread_create(&threads[i], NULL, threadFunction, (void *)&threadArgsArray[i]);

        currentIndex = threadArgsArray[i].endIdx;
    }

    //TODO attente de la fin des threads
    for (int i = 0; i < data->nbThreads; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    // résultat (result a été rempli par les threads)
    // affichage du tableau si pas trop gros
    if (data->nb <= 20)
    {
        printf("[");
        for (int i = 0; i < data->nb; i++)
        {
            if (i != 0)
                printf(" ");
            printf("%g", tab[i]);
        }
        printf("]\n");
    }
    // recherche linéaire pour vérifier
    int nbVerif = 0;
    for (int i = 0; i < data->nb; i++)
    {
        if (tab[i] == data->elt)
            nbVerif ++;
    }
    printf("Elément %g présent %d fois (%d attendu)\n", data->elt, result, nbVerif);
    if (result == nbVerif)
        printf("=> ok ! le résultat calculé par les threads est correct\n");
    else
        printf("=> PB ! le résultat calculé par les threads est incorrect\n");

    //TODO libération des ressources
    free(threads);
    free(threadArgsArray);
    free(tab);
    pthread_mutex_destroy(&mutex);
}


/************************************************************************
 * Partie communication avec le master
 ************************************************************************/
// envoi des données au master
void sendData(const Data *data)
{
    //TODO
    // - envoi de l'ordre au master (cf. CM_ORDER_* dans client_master.h)
    // - envoi des paramètres supplémentaires au master (pour CM_ORDER_EXIST,
    //   CM_ORDER_INSERT et CM_ORDER_INSERT_MANY)
    //END TODO

    int ret;

    // Envoi de l'ordre au master
    ret = write(data->clientToMaster, &(data->order), sizeof(data->order));
    myassert(ret == sizeof(int), "Erreur");

    // Envoi des paramètres supplémentaires au master
    switch (data->order)
    {
    case CM_ORDER_EXIST:
        ret = write(data->clientToMaster, &(data->elt), sizeof(data->elt));
        myassert(ret == sizeof(int), "Erreur");
        break;

    case CM_ORDER_INSERT:
        ret = write(data->clientToMaster, &(data->elt), sizeof(data->elt));
        myassert(ret == sizeof(int), "Erreur");
        break;

    case CM_ORDER_INSERT_MANY:
        ret = write(data->clientToMaster, &(data->nb), sizeof(data->nb));
        myassert(ret == sizeof(int), "Erreur");
        ret = write(data->clientToMaster, &(data->min), sizeof(data->min));
        myassert(ret == sizeof(int), "Erreur");
        ret = write(data->clientToMaster, &(data->max), sizeof(data->max));
        myassert(ret == sizeof(int), "Erreur");
        break;

    default:
        break;
    }
}

// attente de la réponse du master
void receiveAnswer(const Data *data)
{
    //TODO
    // - récupération de l'accusé de réception du master (cf. CM_ANSWER_* dans client_master.h)
    // - selon l'ordre et l'accusé de réception :
    //      . récupération de données supplémentaires du master si nécessaire
    // - affichage du résultat
    //END TODO
    int ret;

    int ack;
    ret = read(data->masterToClient, &ack, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

    switch(ack)
    {
    case CM_ANSWER_STOP_OK:
        printf("STOP_OK\n");
        break;

    case CM_ANSWER_HOW_MANY_OK:
    {
        int res1, res2;

        ret = read(data->masterToClient, &res1, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        ret = read(data->masterToClient, &res2, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        printf("nombre d'élement: %d\n", res1);
        printf("nombre d'element disttinc: %d \n", res2);

    }

    break;

    case CM_ANSWER_MAXIMUM_OK:
    {
        int max;
        ret = read(data->masterToClient, &max, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
        printf("Max : %d\n", max);
    }

    break;

    case CM_ANSWER_MAXIMUM_EMPTY:
        printf("L'ensemble est vide\n");
        break;

    case CM_ANSWER_MINIMUM_OK:
    {
        int min;
        ret = read(data->masterToClient, &min, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
        printf("Min: %d \n", min);
    }
    break;

    case CM_ANSWER_MINIMUM_EMPTY:
        printf("L'ensemble est vide\n");
        break;

    case CM_ANSWER_EXIST_NO:
        printf("L'element %f n'existe pas\n", data->elt);
        break;

    case CM_ANSWER_EXIST_YES:
    {
        int nb;
        ret = read(data->masterToClient, &nb, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
        printf("%f: %d \n", data->elt, nb);
    }
    break;

    case CM_ANSWER_SUM_OK:
    {
        int sum;
        ret = read(data->masterToClient, &sum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
        printf("Sum: %d \n", sum);
    }
    break;

    case CM_ANSWER_INSERT_OK:
        printf("c'est fait!\n");
        break;

    case CM_ANSWER_INSERT_MANY_OK:
        printf("c'est fait!\n");
        break;
    case CM_ANSWER_PRINT_OK:
        break;

    default:
        break;

    }
}


/************************************************************************
 * Fonction principale
 ************************************************************************/
int main(int argc, char * argv[])
{
    Data data;
    parseArgs(argc, argv, &data);
    int ret;

    if (data.order == CM_ORDER_LOCAL)
        lauchThreads(&data);
    else
    {
        //TODO
        // - entrer en section critique :
        //       . pour empêcher que 2 clients communiquent simultanément
        //       . le mutex est déjà créé par le master
        // - ouvrir les tubes nommés (ils sont déjà créés par le master)
        //       . les ouvertures sont bloquantes, il faut s'assurer que
        //         le master ouvre les tubes dans le même ordre
        //END TODO

        // Entrer en section critique
        //data.semCM = recupSem();


        // Ouvrir les tubes nommés
        data.masterToClient = open(MASTER_TO_CLIENT, O_RDONLY);
        myassert(data.masterToClient != -1,"Erreur");

        data.clientToMaster = open(CLIENT_TO_MASTER, O_WRONLY);
        myassert(data.clientToMaster != -1, "Erreur");

        sendData(&data);
        receiveAnswer(&data);

        //TODO
        // - sortir de la section critique
        // - libérer les ressources (fermeture des tubes, ...)
        // - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
        //
        // Une fois que le master a envoyé la réponse au client, il se bloque
        // sur un sémaphore ; le dernier point permet donc au master de continuer
        //
        // N'hésitez pas à faire des fonctions annexes ; si la fonction main
        // ne dépassait pas une trentaine de lignes, ce serait bien.
        //sortirSC(data.semCM);


    // Fermeture des tubes
    ret = close(data.clientToMaster);
    myassert(ret == 0, "tubeClientToMaster n'est pas fermé");
    ret = close(data.masterToClient);
    myassert(ret == 0, "tubeMasterToClient n'est pas fermé");

    }

    return EXIT_SUCCESS;
}

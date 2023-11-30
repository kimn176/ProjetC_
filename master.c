//#if defined HAVE_CONFIG_H
//#include "config.h"
//#endif

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include "utils.h"
#include "myassert.h"

#include "client_master.h"
#include "master_worker.h"

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/
typedef struct
{
    // communication avec le client
    int masterToClient;
    int clientToMaster;

    // données internes
    pid_t firstWorkerPid;           // Process ID du premier worker
    int semWait;

    // communication avec le premier worker (double tubes)
    int masterToFirstWorker[2];
    int firstWorkerToMaster[2];

    // communication en provenance de tous les workers (un seul tube en lecture)
    int workersToMaster[2];

} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/************************************************************************
 * initialisation complète
 ************************************************************************/
void init(Data *data)
{
    int ret;

    ret = pipe(data->firstWorkerToMaster);
    myassert(ret == 0, "Erreur");

    ret = pipe(data->masterToFirstWorker);
    myassert(ret == 0, "Erreur");

    ret = pipe(data->workersToMaster);
    myassert(ret == 0, "Erreur");

    myassert(data != NULL, "il faut l'environnement d'exécution");

    data->firstWorkerPid = -1;
}


/************************************************************************
 * fin du master
 ************************************************************************/
void orderStop(Data *data)
{
    TRACE0("[master] ordre stop\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre de fin (cf. master_worker.h)
    // - attendre sa fin
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO

    // Si ensemble vide (pas de premier worker)

    if (data->firstWorkerPid == -1)
    {
        printf("\nIl n'y a pas de premier worker\n");
    }
    else
    {
        // fermer les extrémités des tubes inutiles
        close(data->masterToFirstWorker[0]);
        close(data->firstWorkerToMaster[1]);

        // Envoyer au premier worker l'ordre de fin (cf. master_worker.h)
        writeToWorker(MW_ORDER_STOP, data->masterToFirstWorker[1]);

        // Attendre la fin du premier worker
        waitpid(data->firstWorkerPid, NULL, 0);

    }

    // Envoyer l'accusé de réception au client (cf. client_master.h)
    int ack = CM_ANSWER_STOP_OK;
    int ret = write(data->masterToClient, &ack, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

}


/************************************************************************
 * quel est la cardinalité de l'ensemble
 ************************************************************************/
void orderHowMany(Data *data)
{
    TRACE0("[master] ordre how many\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre howmany (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultats (deux quantités) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer les résultats au client
    //END TODO

    int ret;

    // Si ensemble vide (pas de premier worker), envoyer l'accusé de réception dédié au client
    if (data->firstWorkerPid == -1)
    {
        // - envoyer l'accusé de réception au client (cf. client_master.h)
        int ack = CM_ANSWER_HOW_MANY_OK;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer les résultats au client l'ensemble est vide
        int res = 0;
        int car = 0;
        ret = write(data->masterToClient, &res, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        ret = write(data->masterToClient, &car, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }
    else
    {
        // Envoyer au premier worker ordre howmany (cf. master_worker.h)
        writeToWorker(MW_ORDER_HOW_MANY, data->masterToFirstWorker[1]);

        // Recevoir accusé de réception vanant du premier worker (cf. master_worker.f)
        ret = readWorker(data->firstWorkerToMaster[0]);
        myassert(ret == MW_ANSWER_HOW_MANY, "Erreur");

        // Recevoir résultats (deux quantités) venant du premier worker
        int res1, res2;
        ret = read(data->firstWorkerToMaster[0], &res1, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        ret = read(data->firstWorkerToMaster[0], &res2, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // - envoyer l'accusé de réception au client (cf. client_master.h)
        int ack = CM_ANSWER_HOW_MANY_OK;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer les résultats au client
        int res[2] = {res1, res2};
        ret = write(data->masterToClient, res, 2 * sizeof(int));
        myassert(ret == 2 * sizeof(int), "Erreur");
    }
}


/************************************************************************
 * quel est la minimum de l'ensemble
 ************************************************************************/
void orderMinimum(Data *data)
{
    TRACE0("[master] ordre minimum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre minimum (cf. master_worker.h)
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . recevoir résultat (la valeur) venant du worker concerné
    //       . envoyer l'accusé de réception au client (cf. client_master.h)
    //       . envoyer le résultat au client
    //END TODO


    int ret;


    // Si ensemble vide (pas de premier worker), envoyer l'accusé de réception dédié au client
    if (data->firstWorkerPid == -1)
    {
        // - envoyer l'accusé de réception dédié au client (cf. client_master.h)
        int ack = CM_ANSWER_MINIMUM_EMPTY;
        int ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }
    else
    {
        // Envoyer au premier worker l'ordre minimum (cf. master_worker.h)
        writeToWorker(MW_ORDER_MINIMUM, data->masterToFirstWorker[1]);

        // Recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
        ret = readWorker(data->workersToMaster[0]);
        myassert(ret == MW_ANSWER_MINIMUM, "Erreur");

        // Recevoir résultat (la valeur) venant du worker concerné
        int resultMinimum;
        ret = read(data->workersToMaster[0], &resultMinimum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer l'accusé de réception au client (cf. client_master.h)
        int ack = CM_ANSWER_MINIMUM_OK;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer le résultat au client
        ret = write(data->masterToClient, &resultMinimum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }

}


/************************************************************************
 * quel est la maximum de l'ensemble
 ************************************************************************/
void orderMaximum(Data *data)
{
    TRACE0("[master] ordre maximum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    int ret;

    // Si ensemble vide (pas de premier worker), envoyer l'accusé de réception dédié au client
    if (data->firstWorkerPid == -1)
    {
        // - envoyer l'accusé de réception dédié au client (cf. client_master.h)
        int ack = CM_ANSWER_MAXIMUM_EMPTY;
        int ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }
    else
    {
        // Envoyer au premier worker l'ordre maximum (cf. master_worker.h)
        writeToWorker(MW_ORDER_MAXIMUM, data->masterToFirstWorker[1]);

        // Recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
        ret = readWorker(data->workersToMaster[0]);
        myassert(ret == MW_ANSWER_MAXIMUM, "Erreur");

        // Recevoir résultat (la valeur) venant du worker concerné
        int resultMaximum;
        ret = read(data->workersToMaster[0], &resultMaximum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer l'accusé de réception au client (cf. client_master.h)
        int ack = CM_ANSWER_MAXIMUM_OK;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer le résultat au client
        ret = write(data->masterToClient, &resultMaximum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }

}


/************************************************************************
 * test d'existence
 ************************************************************************/
void orderExist(Data *data)
{
    TRACE0("[master] ordre existence\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre existence (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à tester
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . si élément non présent
    //             . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    //       . sinon
    //             . recevoir résultat (une quantité) venant du worker concerné
    //             . envoyer l'accusé de réception au client (cf. client_master.h)
    //             . envoyer le résultat au client
    //END TODO

    // Recevoir l'élément à tester en provenance du client
    int elementToTest;
    int ret;

    ret = read(data->clientToMaster, &elementToTest, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

    // Si ensemble vide (pas de premier worker)
    if (data->firstWorkerPid == -1)
    {
        // - envoyer l'accusé de réception dédié au client (cf. client_master.h)
        int ack = CM_ANSWER_EXIST_NO;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }
    else
    {
        // Envoyer au premier worker l'ordre existence (cf. master_worker.h)
        writeToWorker(MW_ORDER_EXIST, data->masterToFirstWorker[1]);

        // Envoyer au premier worker l'élément à tester
        writeToWorker(elementToTest, data->masterToFirstWorker[1]);

        // Recevoir l'accusé de réception du worker concerné
        ret = readWorker(data->workersToMaster[0]);
        myassert(ret == sizeof(int), "Erreur");

        if (ret == MW_ANSWER_EXIST_NO)
        {

            // Si élément non présent, envoyer l'accusé de réception dédié au client
            int ack = CM_ANSWER_EXIST_NO;
            ret = write(data->masterToClient, &ack, sizeof(int));
            myassert(ret == sizeof(int), "Erreur");

        }
        else
        {

            // Si élément présent, recevoir le résultat (une quantité) du worker concerné
            int quantity;
            ret = read(data->workersToMaster[0], &quantity, sizeof(int));
            myassert(ret == sizeof(int), "Erreur");

            // Envoyer l'accusé de réception au client (cf. client_master.h)
            int ack = CM_ANSWER_EXIST_YES;
            ret = write(data->masterToClient, &ack, sizeof(int));
            myassert(ret == sizeof(int), "Erreur");

            // Envoyer le résultat au client
            ret = write(data->masterToClient, &quantity, sizeof(int));
            myassert(ret == sizeof(int), "Erreur");
        }
    }
}

/************************************************************************
 * somme
 ************************************************************************/
void orderSum(Data *data)
{
TRACE0("[master] ordre somme\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    int ret;

    // Si ensemble vide (pas de premier worker), la somme est alors 0
    if (data->firstWorkerPid == -1)
    {
        // Envoyer l'accusé de réception au client (cf. client_master.h)
        int ack = CM_ANSWER_SUM_OK;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        int sum = 0;
        ret = write(data->masterToClient, &sum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }
    else
    {
        // Envoyer au premier worker l'ordre existence (cf. master_worker.h)
        writeToWorker(MW_ORDER_SUM, data->masterToFirstWorker[1]);

        // Recevoire accusé de réception venant du premier worker
        ret = readWorker(data->firstWorkerToMaster[0]);
        myassert(ret == MW_ANSWER_SUM, "Erreur");

        // Recevoir le résultat (la somme) venant du premier worker
        int resultSum;
        ret = read(data->firstWorkerToMaster[0], &resultSum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer l'accusé de réception au client (cf. client_master.h)
        int ack = CM_ANSWER_SUM_OK;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer le résultat au client
        ret = write(data->masterToClient, &resultSum, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }

}

/************************************************************************
 * insertion d'un élément
 ************************************************************************/

//TODO voir si une fonction annexe commune à orderInsert et orderInsertMany est justifiée

void orderInsert(Data *data)
{
    TRACE0("[master] ordre insertion\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . créer le premier worker avec l'élément reçu du client
    // - sinon
    //       . envoyer au premier worker ordre insertion (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à insérer
    // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO

    // - recevoir l'élément à insérer en provenance du client
    int elementToInsert;
    int ret;

    ret = read(data->clientToMaster, &elementToInsert, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

    if (data->firstWorkerPid == -1)
    {
        // - si ensemble vide (pas de premier worker)
        //       . créer le premier worker avec l'élément reçu du client
        data->firstWorkerPid = fork();
        myassert(data->firstWorkerPid!= -1, "fork n'a pas fonctionné");

        if (data->firstWorkerPid == 0)
        {
            createWorker(elementToInsert, data->masterToFirstWorker[0], data->firstWorkerToMaster[1], data->workersToMaster[1] );
            myassert(false, "Erreur");
        }
    }
    else
    {
        // Envoyer au premier worker l'ordre insertion (cf. master_worker.h)
        writeToWorker(MW_ORDER_INSERT, data->masterToFirstWorker[1]);

        // Envoyer au premier worker l'élément à insérer
        writeToWorker(elementToInsert, data->masterToFirstWorker[1]);

    }

    // Recevoir l'accusé de réception venant du worker concerné (cf. master_worker.h)
    ret = readWorker(data->workersToMaster[0]);
    myassert(ret == MW_ANSWER_INSERT, "Erreur");

    // Envoyer l'accusé de réception au client (cf. client_master.h)
    int ack = CM_ANSWER_INSERT_OK;
    ret = write(data->masterToClient, &ack, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");
}


/************************************************************************
 * insertion d'un tableau d'éléments
 ************************************************************************/
void orderInsertMany(Data *data)
{
    TRACE0("[master] ordre insertion tableau\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir le tableau d'éléments à insérer en provenance du client
    // - pour chaque élément du tableau
    //       . l'insérer selon l'algo vu dans orderInsert (penser à factoriser le code)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO

    // - recevoir le tableau d'éléments à insérer en provenance du client
    int nbOfElements;
    int ret;
    ret  = read(data->clientToMaster, &nbOfElements, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

    int* elements = (int*)malloc(nbOfElements * sizeof(int));
    ret = read(data->clientToMaster, elements, nbOfElements * sizeof(int));
    myassert(ret == -1, "Erreur");

    // Insérer chaque élément du tableau
    for (int i = 0; i < nbOfElements; ++i)
    {
        // Envoyer au premier worker l'ordre insertion (cf. master_worker.h)
        writeToWorker(MW_ORDER_INSERT, data->masterToFirstWorker[1]);

        // Envoyer au premier worker l'élément à insérer
        ret = write(data->masterToFirstWorker[1], &elements[i], sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }

    // Envoyer l'accusé de réception au client (cf. client_master.h)
    int ack = CM_ANSWER_INSERT_OK;
    ret = write(data->masterToClient, &ack, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

    // Libérer la mémoire allouée pour le tableau
    free(elements);

}


/************************************************************************
 * affichage ordonné
 ************************************************************************/
void orderPrint(Data *data)
{
    TRACE0("[master] ordre affichage\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre print (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    //   note : ce sont les workers qui font les affichages
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO

    int ret;

    // Si ensemble vide (pas de premier worker), la somme est alors 0
    if (data->firstWorkerPid == -1)
    {

        close(data->masterToFirstWorker[0]);
        close(data->firstWorkerToMaster[1]);
    }
    else
    {

        // Sinon, envoyer au premier worker l'ordre print (cf. master_worker.h)
        writeToWorker(MW_ORDER_PRINT, data->masterToFirstWorker[1]);

        // Recevoir l'accusé de réception venant du premier worker (cf. master_worker.h)
        ret = readWorker(data->firstWorkerToMaster[0]);
        myassert(ret == MW_ANSWER_PRINT, "Erreur");

        // Envoyer l'accusé de réception au client (cf. client_master.h)
        int ack = CM_ANSWER_PRINT_OK;
        ret = write(data->masterToClient, &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

    }
}


/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;

    init(data);


    while (! end)
    {
        int ret;

        //TODO ouverture des tubes avec le client (cf. explications dans client.c)
        // Ouvrir le tube vers le client (à compléter avec le nom du tube approprié)
        data->masterToClient = open(MASTER_TO_CLIENT, O_WRONLY);
        myassert(data->masterToClient != -1, "Erreur");

        // Ouvrir le tube depuis le client (à compléter avec le nom du tube approprié)
        data->clientToMaster = open(CLIENT_TO_MASTER, O_RDONLY);
        myassert(data->clientToMaster != -1, "Erreur");


        //TODO pour que ça ne boucle pas, mais recevoir l'ordre du client
        int order;
        ret = read(data->clientToMaster, &order, sizeof(int));
        myassert(ret != -1, "clientToMaster n'est pas lu");

        switch(order)
        {
        case CM_ORDER_STOP:
            orderStop(data);
            end = true;
            break;
        case CM_ORDER_HOW_MANY:
            orderHowMany(data);
            break;
        case CM_ORDER_MINIMUM:
            orderMinimum(data);
            break;
        case CM_ORDER_MAXIMUM:
            orderMaximum(data);
            break;
        case CM_ORDER_EXIST:
            orderExist(data);
            break;
        case CM_ORDER_SUM:
            orderSum(data);
            break;
        case CM_ORDER_INSERT:
            orderInsert(data);
            break;
        case CM_ORDER_INSERT_MANY:
            orderInsertMany(data);
            break;
        case CM_ORDER_PRINT:
            orderPrint(data);
            break;
        default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        //TODO fermer les tubes nommés
        ret = close(data->masterToClient);
        myassert(ret == 0, "tubeMasterToClient n'est pas fermé");

        ret = close(data->clientToMaster);
        myassert(ret == 0, "tubeClientToMaster n'est pas fermé");
        //     il est important d'ouvrir et fermer les tubes nommés à chaque itération
        //     voyez-vous pourquoi ?
        //TODO attendre ordre du client avant de continuer (sémaphore pour une précédence)
        //entrerSC(data->semWait);

        sleep(1);

        TRACE0("[master] fin ordre\n");
    }
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

//TODO N'hésitez pas à faire des fonctions annexes ; si les fonctions main
//TODO et loop pouvaient être "courtes", ce serait bien

int main(int argc, char * argv[])
{
    if (argc != 1){
        usage(argv[0], NULL);
    }

    TRACE0("[master] début\n");

    Data data;
    int ret;

    //TODO
    // - création des sémaphores
    //data.semWait = creatSem(PROJ_ID, 1);

    // - création des tubes nommés
    ret = mkfifo(MASTER_TO_CLIENT, 0644);
    myassert(ret != -1, "Erreur");

    ret = mkfifo(CLIENT_TO_MASTER, 0644);
    myassert(ret != -1, "Erreur");

    //END TODO
    loop(&data);

    //TODO destruction des tubes nommés, des sémaphores, ...
    ret = unlink(MASTER_TO_CLIENT);
    myassert(ret != -1, "Erreur");

    ret = unlink(CLIENT_TO_MASTER);
    myassert(ret != -1, "Erreur");

    // Détruire les sémaphores
    //destroySemaphore(data.semWait);


    ret = close(data.masterToFirstWorker[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.firstWorkerToMaster[1]);
    myassert(ret == 0, "tubeClientToMaster n'est pas fermé");

    ret = close(data.masterToFirstWorker[1]);
    myassert(ret == 0, "tube n'est pas fermé");
    ret = close(data.firstWorkerToMaster[0]);
    myassert(ret == 0, "tube n'est pas fermé");

    ret = close(data.workersToMaster[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.workersToMaster[1]);
    myassert(ret == 0, "tube n'est pas fermé");


    TRACE0("[master] terminaison\n");
    return EXIT_SUCCESS;
}

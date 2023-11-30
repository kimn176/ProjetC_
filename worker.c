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

#include "utils.h"
#include "myassert.h"

#include "master_worker.h"


/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/
typedef struct
{
    // données internes (valeur de l'élément, cardinalité)
    float elt;
    int cardinality;
    pid_t leftChildPid;
    pid_t rightChildPid;

    // communication avec le père (2 tubes)
    int parentToWorker[2];
    int workerToParent[2];

    // communication avec le master (1 tube en écriture)
    int workerToMaster[2];

    // communication avec le fils gauche s'il existe (2 tubes)
    int leftChildToWorker[2];
    int workerToLeftChild[2];

    // communication avec le fils droit s'il existe (2 tubes)
    int rightChildToWorker[2];
    int workerToRightChild[2];

} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <elt> <fdIn> <fdOut> <fdToMaster>\n", exeName);
    fprintf(stderr, "   <elt> : élément géré par le worker\n");
    fprintf(stderr, "   <fdIn> : canal d'entrée (en provenance du père)\n");
    fprintf(stderr, "   <fdOut> : canal de sortie (vers le père)\n");
    fprintf(stderr, "   <fdToMaster> : canal de sortie directement vers le master\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}
static void parseArgs(int argc, char * argv[], Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");

    if (argc != 5)
        usage(argv[0], "Nombre d'arguments incorrect");

    //TODO initialisation data

    data->elt = strtof(argv[1], NULL);
    data->leftChildPid = -1;
    data->rightChildPid = -1;

    // Communication avec le père (2 tubes)
    data->parentToWorker[1] = -1;
    data->parentToWorker[0] = atoi(argv[2]);

    data->workerToParent[1] = atoi(argv[3]);
    data->workerToParent[0] = -1;

    // Communication avec le master (1 tube en écriture)
    data->workerToMaster[0] = -1;
    data->workerToMaster[1] = atoi(argv[4]);

//    // Communication avec le fils gauche s'il existe (2 tubes)
//    ret = pipe(data->leftChildToWorker);
//    myassert(ret == 0, "Erreur");
//    ret = pipe(data->workerToLeftChild);
//    myassert(ret == 0, "Erreur");

//    // Communication avec le fils droit s'il existe (2 tubes)
//    ret = pipe(data->rightChildToWorker);
//    myassert(ret == 0, "Erreur");
//    ret = pipe(data->workerToRightChild);
//    myassert(ret == 0, "Erreur");

    //END TODO
}


/************************************************************************
 * Stop
 ************************************************************************/
void stopAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre stop\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - envoyer au worker gauche ordre de fin (cf. master_worker.h)
    // - envoyer au worker droit ordre de fin (cf. master_worker.h)
    // - attendre la fin des deux fils
    //END TODO
    int ret;

    // Traiter les cas où les fils n'existent pas
    if (data->leftChildPid == -1 && data->rightChildPid == -1){
        // Rien à faire
    }

    // Envoyer l'ordre de fin au worker gauche s'il existe
    if (data->leftChildPid != -1){
        writeToWorker(MW_ORDER_STOP, data->workerToLeftChild[1]);

        ret = waitpid(data->leftChildPid, NULL, 0);
        myassert(ret != -1, "Erreur");
    }

    // Envoyer l'ordre de fin au worker droit s'il existe
    if (data->rightChildPid != -1){
        writeToWorker(MW_ORDER_STOP, data->workerToRightChild[1]);

        ret = waitpid(data->rightChildPid, NULL, 0);
        myassert(ret != -1, "Erreur");
    }
}


/************************************************************************
 * Combien d'éléments
 ************************************************************************/
static void howManyAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre how many\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre howmany (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir deux résultats (nb elts, nb elts distincts) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer les résultats (les cumuls des deux quantités + la valeur locale) au père
    //END TODO

    int ret;
    int nbElements = 1;  // first woerker
    int nbDistinctElements = 1;  // first worker

    // Traiter les cas où les fils n'existent pas
    if (data->leftChildPid == -1 && data->rightChildPid == -1)
    {
        ret = write(data->workerToMaster[1], &nbElements, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        ret = write(data->workerToMaster[1], &nbDistinctElements, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
    }

    // à chaque fils
    if (data->leftChildPid != -1)
    {
        // Envoyer ordre howmany
        writeToWorker(data->workerToLeftChild[1], MW_ORDER_HOW_MANY);

        // Recevoir accusé de réception du fils gauche
        int ackLeft = readWorker(data->leftChildToWorker[0]);
        myassert(ackLeft == MW_ANSWER_HOW_MANY, "Erreur");

        // Recevoir deux résultats du fils gauche
        int nbElementsLeft = readWorker(data->leftChildToWorker[0]);
        int nbDistinctElementsLeft = readWorker(data->leftChildToWorker[0]);

        // Cumuler les résultats du fils gauche
        nbElements += nbElementsLeft;
        nbDistinctElements += nbDistinctElementsLeft;
    }

    if (data->rightChildPid != -1)
    {

        // Envoyer ordre howmany
        writeToWorker(data->workerToLeftChild[1], MW_ORDER_HOW_MANY);

        // Recevoir accusé de réception du fils droit
        int ackRight = readWorker(data->rightChildToWorker[0]);
        myassert(ackRight == MW_ANSWER_HOW_MANY, "Erreur");

        // Recevoir deux résultats du fils gauche
        int nbElementsRight = readWorker(data->rightChildToWorker[0]);
        int nbDistinctElementsRight = readWorker(data->rightChildToWorker[0]);

        // Cumuler les résultats du fils gauche
        nbElements += nbElementsRight;
        nbDistinctElements += nbDistinctElementsRight;
    }

    // Envoyer l'accusé de réception au père
    writeToWorker(data->workerToParent[1], MW_ANSWER_HOW_MANY);

    // Envoyer les résultats cumulés au père
    writeToWorker(data->workerToParent[1], nbElements);
    writeToWorker(data->workerToParent[1], nbDistinctElements);


}


/************************************************************************
 * Minimum
 ************************************************************************/
static void minimumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre minimum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche n'existe pas (on est sur le minimum)
    //       . envoyer l'accusé de réception au master (cf. master_worker.h)
    //       . envoyer l'élément du worker courant au master
    // - sinon
    //       . envoyer au worker gauche ordre minimum (cf. master_worker.h)
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO

    // Si le fils gauche n'existe pas (on est sur le minimum)
    if (data->leftChildPid == -1)
    {
        writeToWorker(data->workerToParent[1], MW_ANSWER_MINIMUM);
        writeToWorker(data->workerToParent[1], data->elt);
    }
    else
    {
        // Envoyer au worker gauche l'ordre minimum
        writeToWorker(data->workerToLeftChild[1], MW_ORDER_MINIMUM);

    }

}


/************************************************************************
 * Maximum
 ************************************************************************/
static void maximumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre maximum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // cf. explications pour le minimum
    //END TODO

    if (data->rightChildPid == -1)
    {
        writeToWorker(data->workerToParent[1], MW_ANSWER_MAXIMUM);
        writeToWorker(data->workerToParent[1], data->elt);
    }
    else
    {
        // Envoyer au worker droit l'ordre maximum
        writeToWorker(data->workerToRightChild[1], MW_ORDER_MAXIMUM);

    }

}


/************************************************************************
 * Existence
 ************************************************************************/
static void existAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre exist\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du père
    // - si élément courant == élément à tester
    //       . envoyer au master l'accusé de réception de réussite (cf. master_worker.h)
    //       . envoyer cardinalité de l'élément courant au master
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant)
    //       . envoyer au worker gauche ordre exist (cf. master_worker.h)
    //       . envoyer au worker gauche élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    // - sinon (donc elt à tester > elt courant)
    //       . envoyer au worker droit ordre exist (cf. master_worker.h)
    //       . envoyer au worker droit élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO

    int ret;

    // Recevoir l'élément à tester en provenance du père
    int eltToTest;
    ret = read(data->parentToWorker[0], &eltToTest, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

    // Si élément courant == élément à tester
    if (data->elt == eltToTest)
    {
        // Envoyer au master l'accusé de réception de réussite
        int ack = MW_ANSWER_EXIST_YES;
        ret = write(data->workerToMaster[1], &ack, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

        // Envoyer la cardinalité de l'élément courant au master
        ret = write(data->workerToMaster[1], &(data->cardinality), sizeof(int));
        myassert(ret == sizeof(int), "Erreur");

    }
    else if (eltToTest < data->elt)
    {
        if (data->leftChildPid == -1)
        {
            // Envoyer au master l'accusé de réception d'échec
            int ackNoL = MW_ANSWER_EXIST_NO;
            ret = write(data->workerToMaster[1], &ackNoL, sizeof(int));
            myassert(ret == sizeof(int), "Erreur");
        }
        else
        {
            // Envoyer au worker gauche l'ordre exist
            writeToWorker(data->workerToLeftChild[1], MW_ORDER_EXIST);

            // Envoyer au worker gauche l'élément à tester
            writeToWorker(data->workerToLeftChild[1], eltToTest);

        }
    }
    else // eltToTest > data->elt
    {
        if (data->rightChildPid == -1)
        {
            // Envoyer au master l'accusé de réception d'échec
            int ackNoR = MW_ANSWER_EXIST_NO;
            int ret = write(data->workerToMaster[1], &ackNoR, sizeof(int));
            myassert(ret == sizeof(int), "Erreur");
        }
        else
        {
            // Envoyer au worker droit l'ordre exist
             writeToWorker(data->workerToRightChild[1], MW_ORDER_EXIST);

            // Envoyer au worker droit l'élément à tester
             writeToWorker(data->workerToRightChild[1], eltToTest);
        }
    }
}


/************************************************************************
 * Somme
 ************************************************************************/
static void sumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre sum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre sum (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir résultat (somme du fils) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer le résultat (le cumul des deux quantités + la valeur locale) au père
    //END TODO

    int ret;
    int sumLocal = data->elt * data->cardinality;

    // Traiter les cas où les fils n'existent pas
    if (data->leftChildPid == -1 && data->rightChildPid == -1)
    {
        writeToWorker(data->workerToMaster[1], sumLocal);

    }
    // Si le fils gauche existe
    if (data->leftChildPid != -1)
    {
        // Envoyer au worker gauche l'ordre sum
        writeToWorker(data->leftChildToWorker[1], MW_ORDER_SUM);

        // Recevoir l'accusé de réception du worker gauche
        ret = readWorker(data->leftChildToWorker[0]);
        myassert(ret == MW_ANSWER_SUM, "Erreur");

        int sumLeft;
        // Recevoir la somme du fils
        ret = read(data->leftChildToWorker[0], &sumLeft, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
        sumLocal += sumLeft;
    }

    // Si le fils droit existe
    if (data->rightChildPid != -1)
    {
        // Envoyer au worker gauche l'ordre sum
        writeToWorker(data->rightChildToWorker[1], MW_ORDER_SUM);

        // Recevoir l'accusé de réception du worker gauche
        ret = readWorker(data->rightChildToWorker[0]);
        myassert(ret == MW_ANSWER_SUM, "Erreur");

        // Recevoir la somme du fils
        int sumRight;
        ret = read(data->rightChildToWorker[0], &sumRight, sizeof(int));
        myassert(ret == sizeof(int), "Erreur");
        sumLocal += sumRight;
    }

    // Envoyer l'accusé de réception au père
    writeToWorker(data->workerToMaster[1], MW_ANSWER_SUM);

    // Envoyer le résultat au père
    writeToWorker(data->workerToParent[1], sumLocal);

}


/************************************************************************
 * Insertion d'un nouvel élément
 ************************************************************************/
static void insertAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre insert\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du père
    // - si élément courant == élément à tester
    //       . incrémenter la cardinalité courante
    //       . envoyer au master l'accusé de réception (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . créer un worker à gauche avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . créer un worker à droite avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à insérer < elt courant)
    //       . envoyer au worker gauche ordre insert (cf. master_worker.h)
    //       . envoyer au worker gauche élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    // - sinon (donc elt à insérer > elt courant)
    //       . envoyer au worker droit ordre insert (cf. master_worker.h)
    //       . envoyer au worker droit élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    //END TODO

    int ret;

    // Recevoir l'élément à insérer en provenance du père
    int elementToInsert;
    ret = read(data->parentToWorker[0], &elementToInsert, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");

    // Comparer l'élément à insérer avec l'élément courant
    if (elementToInsert == data->elt)
    {
        // Incrémenter la cardinalité courante
        data->cardinality++;
        // Envoyer au master l'accusé de réception (cf. master_worker.h)
        writeToWorker(data->workerToParent[1], MW_ANSWER_INSERT);
    }
    else if (elementToInsert < data->elt)
    {
        // Si pas de fils gauche, créer un worker à gauche avec l'élément reçu du client
        if (data->leftChildPid == -1)
        {
             // Communication avec le fils gauche s'il existe (2 tubes)
            ret = pipe(data->leftChildToWorker);
            myassert(ret == 0, "Erreur");
            ret = pipe(data->workerToLeftChild);
            myassert(ret == 0, "Erreur");

            data->leftChildPid = fork();
            myassert(data->leftChildPid != -1, "fork n'a pas fonctionné");

            if (data->leftChildPid == 0)
            {
                createWorker(elementToInsert, data->workerToLeftChild[0], data->leftChildToWorker[1], data->workerToMaster[1] );
                myassert(false, "Erreur");
            }
        }
        else
        {
            // Envoyer au worker gauche ordre insert (cf. master_worker.h)
            writeToWorker(MW_ORDER_INSERT, data->leftChildToWorker[1]);

            // Envoyer au worker gauche l'élément à insérer
            writeToWorker(elementToInsert, data->leftChildToWorker[1]);
        }
    }
    else // (elementToInsert > data->elt)
    {
        // Si pas de fils droit, créer un worker à droite avec l'élément reçu du client
        if (data->rightChildPid == -1)
        {
             // Communication avec le fils droit s'il existe (2 tubes)
            ret = pipe(data->rightChildToWorker);
            myassert(ret == 0, "Erreur");
            ret = pipe(data->workerToRightChild);
            myassert(ret == 0, "Erreur");

            data->rightChildPid = fork();
            myassert(data->rightChildPid != -1, "fork n'a pas fonctionné");

            if (data->rightChildPid == 0)
            {
                createWorker(elementToInsert, data->workerToRightChild[0], data->rightChildToWorker[1], data->workerToMaster[1] );
                myassert(false, "Erreur");
            }
        }
        else
        {
            // Envoyer au worker gauche ordre insert (cf. master_worker.h)
            writeToWorker(MW_ORDER_INSERT, data->rightChildToWorker[1]);

            // Envoyer au worker gauche l'élément à insérer
            writeToWorker(elementToInsert, data->rightChildToWorker[1]);
        }
    }
}


/************************************************************************
 * Affichage
 ************************************************************************/
static void printAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre print\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - afficher l'élément courant avec sa cardinalité
    // - si le fils droit existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    //END TODO

    int ret;

    // Si le fils gauche existe
    if (data->leftChildPid != -1)
    {
        // Envoyer ordre print au fils gauche
        writeToWorker(MW_ORDER_PRINT, data->workerToLeftChild[1]);

        // Recevoir accusé de réception du fils gauche
        ret = readWorker(data->leftChildToWorker[0]);
        myassert(ret == sizeof(int), "Erreur");
    }

    // Afficher l'élément courant avec sa cardinalité
    printf("Element: %g, Cardinality: %d\n", data->elt, data->cardinality);

    // Si le fils droit existe
    if (data->rightChildPid != -1)
    {
        // Envoyer ordre print au fils droit
        writeToWorker(MW_ORDER_PRINT, data->workerToRightChild[1]);

        // Recevoir accusé de réception du fils droit
        ret = readWorker(data->rightChildToWorker[0]);
        myassert(ret == sizeof(int), "Erreur");
    }

    // Envoyer l'accusé de réception au père
    writeToWorker(MW_ANSWER_PRINT, data->workerToMaster[1]);
}


/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;

    while (! end)
    {
        int order = readWorker(data->parentToWorker[0]) ;  //TODO pour que ça ne boucle pas, mais recevoir l'ordre du père
        myassert(order != -1, "clientToMaster n'est pas lu");

        switch(order)
        {
          case MW_ORDER_STOP:
            stopAction(data);
            end = true;
            break;
          case MW_ORDER_HOW_MANY:
            howManyAction(data);
            break;
          case MW_ORDER_MINIMUM:
            minimumAction(data);
            break;
          case MW_ORDER_MAXIMUM:
            maximumAction(data);
            break;
          case MW_ORDER_EXIST:
            existAction(data);
            break;
          case MW_ORDER_SUM:
            sumAction(data);
            break;
          case MW_ORDER_INSERT:
            insertAction(data);
            break;
          case MW_ORDER_PRINT:
            printAction(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        TRACE3("    [worker (%d, %d) {%g}] : fin ordre\n", getpid(), getppid(), data->elt /*TODO élément*/);
    }
}


/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    Data data;
    parseArgs(argc, argv, &data);
    TRACE3("    [worker (%d, %d) {%g}] : début worker\n", getpid(), getppid(), data.elt /*TODO élément*/);

    //TODO envoyer au master l'accusé de réception d'insertion (cf. master_worker.h)
    //TODO note : en effet si je suis créé c'est qu'on vient d'insérer un élément : moi

    int ret;

    int ack = MW_ANSWER_INSERT;
    ret = write(data.workerToMaster[1], &ack, sizeof(int));
    myassert(ret == sizeof(int), "Erreur");


    loop(&data);

    //TODO fermer les tubes
    ret = close(data.parentToWorker[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.parentToWorker[1]);
    myassert(ret == 0, "tuben'est pas fermé");

    ret = close(data.workerToParent[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.workerToParent[1]);
    myassert(ret == 0, "tuben'est pas fermé");

    ret = close(data.workerToMaster[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.workerToMaster[1]);
    myassert(ret == 0, "tuben'est pas fermé");

    ret = close(data.leftChildToWorker[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.leftChildToWorker[1]);
    myassert(ret == 0, "tuben'est pas fermé");

    ret = close(data.workerToLeftChild[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.workerToLeftChild[1]);
    myassert(ret == 0, "tuben'est pas fermé");

    ret = close(data.rightChildToWorker[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    ret = close(data.rightChildToWorker[1]);
    myassert(ret == 0, "tuben'est pas fermé");

    close(data.workerToRightChild[0]);
    myassert(ret == 0, "tuben'est pas fermé");
    close(data.workerToRightChild[1]);
    myassert(ret == 0, "tuben'est pas fermé");


    TRACE3("    [worker (%d, %d) {%g}] : fin worker\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    return EXIT_SUCCESS;
}

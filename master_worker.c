#if defined HAVE_CONFIG_H
#include "config.h"
#endif

//TODO include selon ce qu'il y a dans le .h

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include "utils.h"
#include "myassert.h"

#include "master_worker.h"


//TODO fonctions selon ce qu'il y a dans le .h

void writeToWorker(int message, int fdWorkerWrite)
{
	int ret = write(fdWorkerWrite, &message, sizeof(int));
	myassert(ret == sizeof(int), "Erreur");
}

int readWorker(int fdWorkerRead)
{
	int message;
	int ret = read(fdWorkerRead, &message, sizeof(int));
	myassert(ret == sizeof(int), "Erreur");
	return message;
}


void createWorker(int value, int fdIn, int fdOut, int fdToMaster)
{
	char *argv[5];
	char elt = (char)value;
    char fdI = (char)fdIn;
    char fdO = (char)fdOut;
    char fdToM = (char)fdToMaster;
    argv[0] = "worker";
    argv[1] = &elt;
    argv[2] = &fdI;
    argv[3] = &fdO;
    argv[4] = &fdToM;
    argv[5] = NULL;
 	execv(argv[0], argv);
}


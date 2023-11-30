#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#define _XOPEN_SOURCE

//TODO include selon ce qu'il y a dans le .h
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>



#include "utils.h"
#include "myassert.h"

#include "client_master.h"

//TODO fonctions selon ce qu'il y a dans le .h

int creatSem(int ftok_param, int taille)
{
   key_t key = ftok(SEM, ftok_param);
   myassert(key != -1, "Erreur");

   int semId = semget(key, taille, IPC_CREAT | IPC_EXCL | 0641);
   myassert(key != -1, "Erreur");

   int ret = semctl(semId, 0, SETVAL, 0);
   myassert(ret != -1, "Erruer");

   return semId;
}

int recupSem(){
    key_t key;
    int semId;

    key = ftok(SEM, PROJ_ID);
    myassert(key != -1, "Erreur");

    semId = semget(key, 1, 0);
    myassert(semId != -1, "Erreur");

    return semId;
}

void entrerSC(int semId){
    int ret;

    struct sembuf opMoins = {0,-1,0};
    ret = semop(semId, &opMoins, 1);
    myassert(ret != -1, "Erreur");
}

void sortirSC(int semId){
    int ret;

    struct sembuf opPlus = {0,1,0};
    ret = semop(semId, &opPlus, 1);
    myassert(ret != -1, "Erreur");
}

void destroySemaphore(int semId)
{
    int ret;

    ret = semctl(semId, -1, IPC_RMID);
    myassert(ret != -1, "Erreur");
}



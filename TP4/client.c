#include "segdef.h"
#include <stdio.h>
#include <stdlib.h>


struct sembuf sop;

int init(int*shmid, int*semid, segment**seg)
{
   *shmid = shmget(cle, segsize, 0);
   if (*shmid == -1) {
      perror("shmget");
      return -1;
   }

   *semid = semget(cle, 3 ,0);
   if (*semid == -1) {
      perror("semget");
      return -1;
   }

   *seg = (segment*)shmat(*shmid, NULL, 0);
   if (*seg == (segment*)-1) {
      perror("shmat");
      return -1;
   }

   init_rand();

   return 0;
}


int main(void) { 
    int shmid, semid;
    segment *seg;

    if (init(&shmid, &semid, &seg) == -1)
        exit(EXIT_FAILURE);
}
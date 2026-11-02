#include "segdef.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_REQUESTS 1000000

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
    // 0 : Init
    if (init(&shmid, &semid, &seg) == -1)
        exit(EXIT_FAILURE);
    
    fork();fork();fork();fork();
    for (int request =0; request < MAX_REQUESTS; request++) {
        //1 : Aquire seg_dispo
        acq_sem(semid, seg_dispo);

        //2 : Initialize the segment
        seg->pid = getpid();
        seg->req = request;

        // Sum up the random values
        long sum = 0;
        for (int i = 0; i < maxval; i++) {
            seg->tab[i] = getrand();
            sum += seg->tab[i];
        }
        long average = sum/maxval;

        acq_sem(semid, seg_init);

        //3 : Wait for res_ok
        wait_sem(semid, res_ok);

        //4 : Print the result and free seg_init
        printf("Request %d: Result: %ld\n", request, average);
        lib_sem(semid, seg_init);

        //5 : Wait for res_ok to be free
        acq_sem(semid, res_ok);
        lib_sem(semid, res_ok);

        //6 : Free seg_dispo
        lib_sem(semid, seg_dispo);

    }
    

    if (shmdt(seg) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    return 0;
}
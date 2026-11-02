#include "segdef.h"
#include "sem.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

// function to set up semaphore operations
static void do_sem(int semid, int sem, int op)
{
    struct sembuf sop;

    sop.sem_num = sem;
    sop.sem_op  = op;
    sop.sem_flg = 0;

    // loop until semaphore operation is successful or error
    while (semop(semid, &sop, 1) == -1) {
        if (errno == EINTR)
            continue;               
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

// wait for semaphore to be available
void wait_sem(int semid, int sem) { do_sem(semid, sem, 0); }

// acquire semaphore: decrement, and block if not available
void acq_sem(int semid, int sem)  { do_sem(semid, sem, -1); }

// release semaphore: increment
void lib_sem(int semid, int sem)  { do_sem(semid, sem, +1); }

// initialize random number based on current time and process IDs
void init_rand(void)
{
    srandom(time(NULL) ^ getpid());
}

// function to generate a random long integer by combining multiple calls to random()
long getrand(void)
{
    return (random() << 32) ^ (random() << 1) ^ random();
}
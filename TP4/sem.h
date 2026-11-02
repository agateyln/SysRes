#ifndef SEM_H
#define SEM_H

void wait_sem(int semid, int sem);
void acq_sem(int semid, int sem);
void lib_sem(int semid, int sem);

void init_rand(void);
long getrand(void);

#endif
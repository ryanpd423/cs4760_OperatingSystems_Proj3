//
// Created by Ryan Patrick Davis on 2/10/17.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "proj3.h"

int slave_id;

int shmid; //holds the shared memory segment id #global
shared_clock_t* shm_clock_ptr; //pointer to starting address in shared memory of the shared memory structure (see .h)

//slave.c signal handler for slave processes
void signalCallback (int signum)
{
    printf("\nSIGTERM received by slave %d\n", slave_id);
    // Cleanup
    shmdt(shm_clock_ptr);
    // shmdt(sharedNum2);
    exit(0);
}

int main(int argc, char* argv[])
{

    slave_id = atoi(argv[1]);

    if (signal(SIGTERM, signalCallback) == SIG_ERR) {
        perror("Error: slave: signal().\n");
        exit(errno);
    }

    //slave process is assigned shared memory segment; 
    if ((shmid = shmget(SHM_KEY, sizeof(shared_clock_t), 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }

    //attach each process's shared memory pointer to the shared_clock_t struct - - critical
    shm_clock_ptr = shmat(shmid, NULL, 0);
    //attach each process's shared memory pointer to the shared_clock_t struct - - critical

    while(1){
        printf("slave %d seconds: %d nano_seconds: %d\n", slave_id, shm_clock_ptr->seconds, shm_clock_ptr->nano_seconds);
        sleep(2); //just for visual help
    }

    //Clean up
    shmdt(shm_clock_ptr);
    int fclose();
    return 0;
}

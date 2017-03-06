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
#include <sys/msg.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "proj3.h"

//Ctrl+` in VisualStudio Code to display the integrated terminal


int slave_id;

int shmid; //holds the shared memory segment id #global
shared_clock_t* shm_clock_ptr; //pointer to starting address in shared memory of the shared memory structure (see .h)
static int message_queue_id;


//slave.c signal handler for slave processes
void signalCallback (int signum)
{
    printf("\nSIGTERM received by slave %d\n", slave_id);
    // Cleanup
    shmdt(shm_clock_ptr);
    // shmdt(sharedNum2);
    exit(0);
}

void mail_the_message(int destination_address){
    static int size_of_message;
    message_t message;
    message.message_address = destination_address;
    //specifies the number of bytes in the message contents, not counting the address variable size
    size_of_message = sizeof(message) - sizeof(long);
    msgsnd(message_queue_id, &message, size_of_message, 0);
}

void receive_the_message(int destination_address){
    static int size_of_message;
    message_t message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(message_queue_id, &message, size_of_message, destination_address, 0);
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

    //slave process is assigned the message queue id created by the master process
    if ((message_queue_id = msgget(MESSAGE_QUEUE_KEY, 0600)) < 0) {
        perror("Error: msgget");
        exit(errno);
    }

    //attach each process's shared memory pointer to the shared_clock_t struct - - critical
    shm_clock_ptr = shmat(shmid, NULL, 0);
    //attach each process's shared memory pointer to the shared_clock_t struct - - critical

    while(1){
        receive_the_message(slave_id);
        printf("slave %d seconds: %d nano_seconds: %d\n", slave_id, shm_clock_ptr->seconds, shm_clock_ptr->nano_seconds);
        sleep(2); //just for visual help
        mail_the_message(400);
    }

    //Clean up
    shmdt(shm_clock_ptr);
    int fclose();
    return 0;
}

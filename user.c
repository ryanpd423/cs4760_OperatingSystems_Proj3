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
#include "proj5.h"

//Ctrl+` in VisualStudio Code to display the integrated terminal

int slave_id;
int shmid; //holds the shared memory segment id #global
shared_memory_object_t* shm_ptr; //pointer to starting address in shared memory of the shared memory structure (see .h)
static int message_queue_id;

//slave.c signal handler for slave processes
void signalCallback (int signum)
{
    printf("\nUSER: SIGTERM received by slave %d\n", slave_id);
    // Cleanup
    shmdt(shm_ptr);
    // shmdt(sharedNum2);
    exit(0);
}

void mail_the_message(int destination_address, int user_process_state){
    static int size_of_message;
shm_ptr->var += 1; //tracer for the msg queue
printf("user: mailer ~ var = %d\n", shm_ptr->var);
    message_t message;
    message.message_address = destination_address; //address of master (from user.c to oss.c)
    message.dead_or_done = user_process_state;
    message.return_address = slave_id; //initializes the message.return_address member so the master can decide who the message is from
    message.clock_info.seconds = shm_ptr->clock_info.seconds;
    message.clock_info.nano_seconds = shm_ptr->clock_info.nano_seconds;
    size_of_message = sizeof(message) - sizeof(long); //specifies the number of bytes in the message contents, not counting the address variable size
    if ((msgsnd(message_queue_id, &message, size_of_message, 0)) < 0) {
       perror("USER: msgsnd: failed");
       exit(errno); 
    }
}

void receive_the_message(int destination_address){
    // shm_ptr->clock_info.nano_seconds += 12500; //increments the nano_seconds on each iteration
    //     if(shm_ptr->clock_info.nano_seconds > 1000000000){
    //         shm_ptr->clock_info.seconds++;
    //         shm_ptr->clock_info.nano_seconds -= 1000000000;
    //     } //@9:09; 4-17
shm_ptr->var += 1; //tracer for the msg queue
printf("user: receiver ~ var = %d\n", shm_ptr->var);
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
    if ((shmid = shmget(SHM_KEY, sizeof(system_clock_t), 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }

    //slave process is assigned the message queue id created by the master process
    if ((message_queue_id = msgget(MESSAGE_QUEUE_KEY, 0600)) < 0) {
        perror("Error: msgget");
        exit(errno);
    }

    //attach each process's shared memory pointer to the system_clock_t struct - - critical
    shm_ptr = shmat(shmid, NULL, 0);
    //attach each process's shared memory pointer to the system_clock_t struct - - critical

    srand(time(NULL));
    unsigned long long run_time_limit; //this represents how long the child should run
    run_time_limit = (rand() % 100000) +1;
    //put run_time_limit in terms of entrance, exit, and critical sections times:
    unsigned long long scaled_run_time_limit = run_time_limit * 1000;
    //printf("unscaled random time limit = %llu\n", run_time_limit);
    unsigned long long time_in_critical_section = 0;
    while(1){
        receive_the_message(slave_id); // LOCKED until first mail() from OSS sets mtype (message.message_address) to long > 0 ~ at which point USER removes that mtype message from the queue and the queue becomes LOCKED again; RECEIVE message from master to UNLOCK

        unsigned long long entrance_time = (shm_ptr->clock_info.seconds * 1000000000) + shm_ptr->clock_info.nano_seconds;

        int process_state = 1; //process still has time to run basically

        mail_the_message(400, process_state); // UNLOCKED until first mail() from USER sets mytpe back (message.message_address) to a new long > 0 (previous was removed from queue from msgrcv) ~ at which point USER enters a new mtype message into the queue and the becomes UNLOCKED again; SEND message to master to LOCK

        //printf("entrance time = %llu\n", entrance_time);

        printf("slave %d seconds: %d nano_seconds: %d\n", slave_id, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nano_seconds);

        receive_the_message(slave_id); //see above receive_the_message for explanation of UNIX functionality

        unsigned long long exit_time = (shm_ptr->clock_info.seconds * 1000000000) + shm_ptr->clock_info.nano_seconds;
        
        //printf("exit time = %llu\n", exit_time);

        time_in_critical_section += (exit_time - entrance_time);

        //printf("critical section time = %llu\n", time_in_critical_section);

        //printf("scaled_run_time_limit = %llu\n", scaled_run_time_limit);

        if (time_in_critical_section > scaled_run_time_limit) {
            process_state = 0; //if 0 then the process ran out of time //
        }
        else 
            process_state = 1; //if 1 then the process finished with time to spare 
        //sleep(2); //just for visual help
        mail_the_message(400, process_state); //see above mail_the_message for explanation of UNIX functionality
    }

    //Clean up
    shmdt(shm_ptr);
    int fclose();
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include "proj3.h"

//Ctrl+` in VisualStudio Code to display the integrated terminal

/*getopt() flags:*/
static int h_flag = 0;
static int maxSlaveProcessesSpawned = 5; //the x in -s x; user sets a maximum for the number of slave processes spawned with '-s x' flag where x sets maxSlaveProcessesSpawned
static char* fileName = "log_file.txt"; //the filename in -l filename; declaring char* fileName character pointer as a const string affords us the priviledge of not having to end our string with a NULL terminator '\0'
static int computationShotClock = 60; //the z in -t z; is the time in seconds when the master will terminate itself and all children
static int message_queue_id; 
/*getopt() flags:*/

pid_t *slavePointer;

int shmid; //holds the shared memory segment id #global
shared_clock_t* shm_clock_ptr; //pointer to starting address in shared memory of the shared memory structure (see .h)

//function detaches the shared memory segment shmaddr and then removes the shared memory segment specified by shmid
int detachandremove (int shmid, void* shmaddr){
    int error = 0;

    if (shmdt(shmaddr) == - 1)
        error = errno;
    if ((shmctl(shmid, IPC_RMID, NULL) == -1) && !error)
        error = errno;
    if (!error)
        return 0;
    errno = error;
    return -1;
}

//function that spawns slave processes
void makeSlaveProcesses(int numberOfSlaves)
{
    char slave_id[3];
    int i;

    //Allocate space for slave process array
    slavePointer = (pid_t*) malloc(sizeof(pid_t) * maxSlaveProcessesSpawned);

    // Fork loop
    for (i = 0; i < numberOfSlaves; i++)
    {
        slavePointer[i] = fork();

        if (slavePointer[i] < 0) {
            //fork failed
            perror("Fork failed");
            exit(errno);
        }
        else if (slavePointer[i] == 0) {
            //slave (child) process code - - remember that fork() returns twice...
            sprintf(slave_id, "%d", i + 1);
            execl("./user", "user", slave_id, NULL); //replaces the memory copied to each process after fork to start over with nothing
            perror("Child failed to execl slave exe");
            printf("THIS SHOULD NEVER EXECUTE\n");
        }
    }
}

//master.c signal handler for master process
void signalCallback (int signum)
{
    int i, status;

    if (signum == SIGINT)
        printf("\nSIGINT received by master\n");
    else
        printf("\nSIGALRM received by master\n");

   for (i = 0; i < maxSlaveProcessesSpawned; i++){
        kill(slavePointer[i], SIGTERM);
    }
    while(wait(&status) > 0) { 
        if (WIFEXITED(status))	/* process exited normally */
		printf("slave process exited with value %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))	/* child exited on a signal */
		printf("slave process exited due to signal %d\n", WTERMSIG(status));
	else if (WIFSTOPPED(status))	/* child was stopped */
		printf("slave process was stopped by signal %d\n", WIFSTOPPED(status));
    }

    //clean up program before exit (via interrupt signal)
    detachandremove(shmid,shm_clock_ptr); //cleans up shared memory IPC
    msgctl(message_queue_id, IPC_RMID, NULL); //cleans up message queue IPC
    free(slavePointer); 
    exit(0);
}

void mail_the_message(int destination_address){
    static int size_of_message;
    message_t message;
    message.message_address = destination_address;
    //specifies the number of bytes in the message contents, not counting the address variable size
    size_of_message = sizeof(message) - sizeof(message.message_address);
    msgsnd(message_queue_id, &message, size_of_message, 0);
}

void receive_the_message(int destination_address){
    static int size_of_message;
    message_t message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(message_queue_id, &message, size_of_message, destination_address, 0);

    if(!message.dead_or_done){
        // print to file, kill(pid[slave_id]), pid[slave_id] = fork(), execl
        printf("Master: Child is terminating at time [ENTER THIS LATER]\n");
    }
}

int main(int argc, char* argv[])
{
    //holds the return value of getopt()
    int c; 
    while ( ( c = getopt(argc, argv, "hs:l:t:")) != -1)
    {
        switch(c) {
            case 'h':
                h_flag = 1;
                break;
            case 's':
                maxSlaveProcessesSpawned = atoi(optarg);
                break;
            case 'l':
                fileName = optarg;
                break;
            case 't':
                computationShotClock = atoi(optarg);
                break;
            case '?':
                if (optopt == 's' || optopt == 'l' || optopt == 't')
                    fprintf(stderr, "Option %c requires an argument.\n", optopt);
                else
                    fprintf(stderr, "Unknown option -%c\n", optopt);
                return -1;
        }
    }

    if (h_flag == 1)
    {
        printf("'-h' flag: This provides a help menu\n");
        printf("'-s x' flag: This sets the number of slave processes spawned (default value = %d).  Current value = %d\n", maxSlaveProcessesSpawned,maxSlaveProcessesSpawned);
        printf("'-l filename' flag: This sets the name of the file that the slave processes will write into (default file name = %s).  Current name  is %s\n", fileName, fileName);
        printf("'-t z' flag: This flag determines the amount time that will pass until the master process terminates itself (default value = %d). Current value of the timer variable = %d\n", computationShotClock, computationShotClock );
        exit(0);
    }

    //generate SIGINT via Ctrl+c
    if (signal(SIGINT, signalCallback) == SIG_ERR) {
        perror("Error: master: signal(): SIGINT\n");
        exit(errno);
    }
    //generate via alarm()
    if (signal(SIGALRM, signalCallback) == SIG_ERR) {
        perror("Error: slave: signal(): SIGALRM\n");
        exit(errno);
    }

    //this timer generates SIGALRM after computationShotClock seconds
    alarm(computationShotClock);

    //master process creates and assigns shared memory segment; assigns id to shmid_sharedNum
    if ((shmid = shmget(SHM_KEY, sizeof(shared_clock_t), IPC_CREAT | 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }

    //master process creates a message queue; it returns a queue_id which you can use to access the message queue
    if ((message_queue_id = msgget(MESSAGE_QUEUE_KEY, IPC_CREAT | 0600)) < 0) {
        perror("Error: mssget");
        exit(errno);
    }

    //attach shared memory pointer to shared_clock_t struct; we've initialized our 
    shm_clock_ptr = shmat(shmid, NULL, 0);

    //master initializes shared memory variables in shared memory structure; has to be done AFTER shmat
    shm_clock_ptr->seconds = 0;
    shm_clock_ptr->nano_seconds = 0;

    //spawn slave process
    makeSlaveProcesses(maxSlaveProcessesSpawned); //fork / execli
    //master will keep changing the shared memory at the same time the slaves are reading it
    //infinite loop won't terminate without a signal (wont terminate naturally)
    while(shm_clock_ptr->seconds <= 2){
        int i;
        for(i = 1; i <= maxSlaveProcessesSpawned; i++){
            mail_the_message(i);
            receive_the_message(400);

        shm_clock_ptr->nano_seconds += 11000; //increments the nano_seconds on each iteration
        if(shm_clock_ptr->nano_seconds > 1000000000){
            shm_clock_ptr->seconds++;
            shm_clock_ptr->nano_seconds -= 1000000000;
        }
            mail_the_message(i);
            receive_the_message(400);
            //sleep(1);
        }
      //sleep(3); //just sleep so you can keep track of iterations visually
    }

    int i, status; //i is for-loop iterator, status is to hold the exit signal after master invokes kill on the infinitely running slave processes
    for (i = 0; i < maxSlaveProcessesSpawned; i++){
        kill(slavePointer[i], SIGTERM);
    }

    while(wait(&status) > 0) { 
        if (WIFEXITED(status))	/* process exited normally */
		printf("slave process exited with value %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))	/* child exited on a signal */
		printf("slave process exited due to signal %d\n", WTERMSIG(status));
	else if (WIFSTOPPED(status))	/* child was stopped */
		printf("slave process was stopped by signal %d\n", WIFSTOPPED(status));
    }

    

    sleep(10);
    printf("[master]: Done\n");

    //Cleanup
    detachandremove(shmid,shm_clock_ptr); 
    msgctl(message_queue_id, IPC_RMID, NULL);
    free(slavePointer);
    return 0;
}

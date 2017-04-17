#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include "proj5.h"

//Ctrl+` in VisualStudio Code to display the integrated terminal

/*getopt() flags:*/
static int h_flag = 0;
static int process_amt = 5; //the x in -s x; user sets a maximum for the number of slave processes spawned with '-s x' flag where x sets process_amt
static char* fileName = "log_file.txt"; //the filename in -l filename; declaring char* fileName character pointer as a const string affords us the priviledge of not having to end our string with a NULL terminator '\0'
static int computationShotClock = 20; //the z in -t z; is the time in seconds when the master will terminate itself and all children
static int message_queue_id; 
int next_logical_process;
int processes_terminated_so_far = 0;
int processes_created_so_far = 0; //@7:30
int active_processes = 0; //@7:30
/*getopt() flags:*/

process_t* slavePointer; //~@7:26; 4-16
int shmid; //holds the shared memory segment id #global
shared_memory_object_t* shm_ptr; //pointer to starting address in shared memory of the shared memory structure (see .h)

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

//this function controls the logical clock (controlled by oss) that is meant to represent the system clock controlled by the operating system
void clock_incrementation_function(system_clock_t *destinationClock, system_clock_t sourceClock, int additional_nano_seconds) {
    //changes the first parameter, the system_clock_t object* (passed by reference) value 
    //takes the the second parameter, the system_clock_t object (the current logical clock time calculated thus far) to use as the current state (from a value of seconds and nanoseconds properties standpoint) to update that same clock with additional nanoseconds, which is the third parameter
    destinationClock->nano_seconds = sourceClock.nano_seconds + additional_nano_seconds;
    if(destinationClock->nano_seconds > 1000000000) {
        destinationClock->seconds++;
        destinationClock->nano_seconds -= 1000000000;
    }
    //because of pass by reference this function takes care of our logical implementation of a system clock simulator without passing data heavy copies of arguments to and from our function that needs to be run many many times #memory_efficient
}

//function that spawns slave processes
void forkProcesses(int slaves_to_be_forked){
    char slave_id[3];
    int i;

    //Allocate space for slave process array
    slavePointer = (process_t*) malloc(sizeof(process_t) * process_amt); //~@7:26; 4-16
    
    // Fork loop
    for (i = 0; i < slaves_to_be_forked; i++){  //~@7:26; 4-16
    //for (i = 1; i < slaves_to_be_forked + 1; i++) {  //@7:30 ~ system_processes[20] is 1's based array indexing    
        slavePointer[i].logical_num = i + 1; //~@7:26; 4-16
        
        shm_ptr->system_processes[i].logical_num = slavePointer[i].logical_num; //@9:53; 4-17 //will be keeping this inline with slavePointer, just not storing slavePointer's pid_t values because of buggy reasons
        

        slavePointer[i].actual_pid = fork(); //~@7:26; 4-16
processes_created_so_far++; //@7:30
        if (slavePointer[i].actual_pid < 0) { //~@7:26; 4-16
        //if (shm_ptr->system_processes[i].actual_pid < 0) {
            //fork failed
            perror("Fork: failed");
            exit(errno);
        }
        else if (slavePointer[i].actual_pid == 0) { //~@7:26; 4-16
        //else if (shm_ptr->system_processes[i].actual_pid == 0) { //@7:30
            //slave (child) process code - - remember that fork() returns twice...
            sprintf(slave_id, "%d", slavePointer[i].logical_num); //~@7:26; 4-16
            //sprintf(slave_id, "%d", shm_ptr->system_processes[i].logical_num); //@7:30
            execl("./user", "user", slave_id, NULL); //replaces the memory copied to each process after fork to start over with nothing
            perror("Execl: failed");
            printf("THIS SHOULD NEVER EXECUTE ~ exiting now\n");
            exit(errno);
        }
    }
}

srand(time(NULL));


void initialize_resources(void) {
    int i;
    for (i = 0; i < 20; i++) {
        shm_ptr->system_resources[i].requested_by = 0;
        shm_ptr->system_resources[i].allocated_to = 0;
        shm_ptr->system_resources[i].resource_instances = rand() % 10 + 1;
        int sharable;
        if (i % 2 == 0) {
            sharable = rand() % 2 + 1; 
            if (sharable % 2 == 0) {
            shm_ptr->system_resources[i].shared = 1; //this resource, i, is a sharable type resource
            shm_ptr->system_resources[i].resource_instances = 0; //no need to have this be a positive number because it will never be decremented
            }
        }
        else {
            shm_ptr->system_resources[i].shared = 0; //this resource, i, is not a sharable type resoure and its resource instance quantity can be decremented
        }
    }
}

//master.c signal handler for master process
void signalCallback (int signum)
{
    int i, status;
    // if (signum == SIGINT)
    //     printf("\nSIGINT received by master\n");
    // else
    //     printf("\nSIGALRM received by master\n"); //@9:07pm
if (signum == SIGTERM) printf("sigterm == %d\n", signum); //@9:32pm added before registered SIGTERM signal handler to verify that that's what was causing the program to terminate without cleaning up first and cause ipcs
   //for (i = 1; i < process_amt + 1; i++){
    for (i = 0; i < process_amt; i++) { //@7:26; 4-16
        kill(slavePointer[i].actual_pid, SIGTERM); //~@7:26; 4-16
        //kill(shm_ptr->system_processes[i].actual_pid, SIGTERM); //@7:30
processes_terminated_so_far++; //@7:30
active_processes = processes_created_so_far - processes_terminated_so_far; //@7:30
printf("active processes = %d", active_processes); //@7:30
    }

    if (signum == SIGINT) //@9:08pm
        printf("\nOSS: SIGINT received by master\n");
    else
        printf("\nOSS: SIGALRM received by master\n");

    while(wait(&status) > 0) { 
        if (WIFEXITED(status))	/* process exited normally */
		printf("slave process exited with value %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))	/* child exited on a signal */
		printf("slave process exited due to signal %d\n", WTERMSIG(status));
	else if (WIFSTOPPED(status))	/* child was stopped */
		printf("slave process was stopped by signal %d\n", WIFSTOPPED(status));
    }

    //clean up program before exit (via interrupt signal)
    detachandremove(shmid,shm_ptr); //cleans up shared memory IPC
    msgctl(message_queue_id, IPC_RMID, NULL); //cleans up message queue IPC
printf("oss: signal -> msgctl and detach ~ exiting\n");
    free(slavePointer); //@7:26; 4-16
    exit(0);
}

void mail_the_message(int destination_address){ //this should be in sync with our slave_id from the forkProcesses(num_processes) method
shm_ptr->var += 1; //tracer for the msg queue
printf("oss: mailer ~ var = %d\n", shm_ptr->var);
    static int size_of_message;
    message_t message;
    message.message_address = destination_address; //the message to be sent 
    size_of_message = sizeof(message) - sizeof(message.message_address); //specifies the number of bytes in the message contents, not counting the address variable size
    if ((msgsnd(message_queue_id, &message, size_of_message, 0)) < 0) { //inserts messages into the message queue with msgsnd
        perror("OSS: msgsnd: failed");
        exit(errno);
        //might have to free some msg related memory here on failure
    }
}

void receive_the_message(int destination_address){
    char slave_id[3];
shm_ptr->var += 1; //tracer for the msg queue
printf("oss: receiver ~ var = %d\n", shm_ptr->var);
    static int size_of_message;
    //int process_that_terminated;
    message_t message;
    size_of_message = sizeof(message) - sizeof(long);
    msgrcv(message_queue_id, &message, size_of_message, destination_address, 0); //a program can remove a message from a message queue with msgrcv

    shm_ptr->clock_info.nano_seconds += 12500; //increments the nano_seconds on each iteration
        if(shm_ptr->clock_info.nano_seconds > 1000000000){
            shm_ptr->clock_info.seconds++;
            shm_ptr->clock_info.nano_seconds -= 1000000000;
        }
//sleep(1); //experimenting
    if(!message.dead_or_done){
        // print to file, kill(pid[slave_id]), pid[slave_id] = fork(), execl
        FILE* fp;
        fp = fopen(fileName, "a");
        fprintf(fp, "Master: Child %d is terminating at my time: %d %d because it reached %d %d \n", message.return_address, shm_ptr->clock_info.seconds, shm_ptr->clock_info.nano_seconds, message.clock_info.seconds, message.clock_info.nano_seconds);
        int j; //~@7:26; 4-16
        int index = -1; //~@7:26; 4-16
        //int index = message.return_address; //@7:30
        for (j = 0; j < process_amt; j++){ //~slavePointer starts at index 0 but its logical num at index 0 = 1 which is why we start at i = 0 here
            if (slavePointer[j].logical_num == message.return_address) { //~@7:26; 4-16
            //if (shm_ptr->system_processes[j].logical_num == message.return_address) {//@7:30
            index = j; 
            break;    
             }
         } //~@7:26; 4-16
printf("99asdjsadfjsdf\n");
        kill(slavePointer[index].actual_pid, SIGINT); //@7:26; 4-16 //was SIGINT @10:17; 4-17
        //kill(shm_ptr->system_processes[index].actual_pid, SIGINT); //@7:30 
        //@9:53; 4-17; if we wanted to keep shm_ptr->system_processes in sync with 
        int status;
printf("asdjsadfjsdf\n");
        waitpid(slavePointer[index].actual_pid, &status, 0); //~@7:26; 4-16
        //waitpid(shm_ptr->system_processes[index].actual_pid, &status, 0); //@7:30
printf("asdjsadfjsdf\n");
processes_terminated_so_far++;
active_processes = processes_created_so_far - processes_terminated_so_far; //@7:30
printf("active processes = %d", active_processes); //@7:30
        //printf("kill() cmd runs from RECEIVE()\n");
        if (active_processes < MAX_PROCESS_NUM + 1) {
printf("tracer\n");
        //if (active_processes < MAX_PROCESS_NUM + 1) { //@7:30 ~~ one thing I didn't change back during "the change back :( "
            slavePointer[index].logical_num = next_logical_process++; //~@7:26; 4-16; using [index] array position because that is a "free" spot in the array because the process_t object that was formerly at [index] position in the slavePointer array was just terminated
            
            shm_ptr->system_processes[index].logical_num = next_logical_process++; //@7:30 //overwriting what was previously the logical_num corresponding to the [index] element position in the system_processes process_t arraykeeping the system_processes process_t array inline with the slavePointer process_t array
            
            slavePointer[index].actual_pid = fork(); //~@7:26; 4-16 //replacing the pid_t at the [index] because the former pid_t val at the [index] in the process_t array, slavePointer, was just terminated
            //shm_ptr->system_processes[index].actual_pid = fork(); //@7:30
processes_created_so_far++; //@7:30
            //printf("new replacement slave process #%d from RECEIVE()\n", slavePointer[index].process_Num);
            if (slavePointer[index].actual_pid < 0) { //~@7:26; 4-16
            //if (shm_ptr->system_processes[index].actual_pid < 0) { //@7:30
            //fork failed
            perror("Fork: failed");
            exit(errno);
            }
            else if (slavePointer[index].actual_pid == 0) { //~@7:26; 4-16
            //else if (shm_ptr->system_processes[index].actual_pid == 0) {  //@7:30
                //slave (child) process code - - remember that fork() returns twice...
                sprintf(slave_id, "%d", slavePointer[index].logical_num); //~@7:26; 4-16
                //sprintf(slave_id, "%d", shm_ptr->system_processes[index].logical_num); //@7:30
                execl("./user", "user", slave_id, NULL); //replaces the memory copied to each process after fork to start over with nothing
                perror("Execl: failed");
                printf("THIS SHOULD NEVER EXECUTE ~ exiting now\n");
                exit(errno);
            }
        }
        else {
printf("dshasdhfahsd\n");
            slavePointer[index].logical_num = -1; //~@7:26; 4-16
            //shm_ptr->system_processes[index].logical_num = -1; //@7:30 //see while loop if with != -1
            }
        }
    }


int main(int argc, char* argv[]) {
    //holds the return value of getopt()
    int c; 
    while ( ( c = getopt(argc, argv, "hs:l:t:")) != -1)
    {
        switch(c) {
            case 'h':
                h_flag = 1;
                break;
            case 's':
                process_amt = atoi(optarg);
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
        printf("'-s x' flag: This sets the number of slave processes spawned (default value = %d).  Current value = %d\n", process_amt,process_amt);
        printf("'-l filename' flag: This sets the name of the file that the slave processes will write into (default file name = %s).  Current name  is %s\n", fileName, fileName);
        printf("'-t z' flag: This flag determines the amount time that will pass until the master process terminates itself (default value = %d). Current value of the timer variable = %d\n", computationShotClock, computationShotClock );
        exit(0);
    }

    //info for your receive method
    next_logical_process = process_amt + 1;

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
    if (signal(SIGTERM, signalCallback) == SIG_ERR) {
        perror("Error: slave: signal(): SIGALRM\n");
        exit(errno);
    }

    //this timer generates SIGALRM after computationShotClock seconds
    alarm(computationShotClock);

    //master process creates and assigns shared memory segment; assigns id to shmid_sharedNum
    if ((shmid = shmget(SHM_KEY, sizeof(shared_memory_object_t), IPC_CREAT | 0600)) < 0) {
        perror("Error: shmget");
        exit(errno);
    }

    //master process creates a message queue; it returns a queue_id which you can use to access the message queue
    if ((message_queue_id = msgget(MESSAGE_QUEUE_KEY, IPC_CREAT | 0600)) < 0) {
        perror("Error: mssget");
        exit(errno);
    }

    //attach shared memory pointer to shared_memory_object_t struct; we've initialized our 
    shm_ptr = shmat(shmid, NULL, 0);

    //master initializes shared memory variables in shared memory structure; has to be done AFTER shmat
    shm_ptr->clock_info.seconds = 0;
    shm_ptr->clock_info.nano_seconds = 0;

    /*initialize system resources*/
    initialize_resources(void); //initializes the system resources inside the array of system resoucrce descriptor structs (R[0] -> R[19]) stored in shared memory
    /*initialize system resources*/

printf("oss: ~ pre-while loop ~ initialized clock\n");
    //spawn slave process
    forkProcesses(process_amt); //fork / execli

    // ~ oss continues execution ***PARENT SECTION*** below...

    //master will keep changing the shared memory at the same time the slaves are reading it
    //infinite loop won't terminate without a signal (wont terminate naturally)

sleep(1); //experimenting: sleep blocks the oss for a nanosecond so that the user processes have control of the cpu
printf("oss: ~ pre-while loop ~ initialized clock\n");
    //while(shm_ptr->clock_info.seconds <= 2 && processes_terminated_so_far < MAX_PROCESS_NUM + 1){ //@7:26; 4-16
    while(shm_ptr->clock_info.seconds <= 2) { //@7:30
        int i;
        for(i = 0; i < process_amt; i++){ //~@7:26; 4-16
        //for (i = 1; i < process_amt + 1; i++) { //@7:30
            if (slavePointer[i].logical_num != -1){ //~@7:26; 4-16
            //if (shm_ptr->system_processes[i].logical_num != -1)
//sleep(1); //@10:54
                mail_the_message(slavePointer[i].logical_num); //~@7:26; 4-16
                //mail_the_message(shm_ptr->system_processes[i].logical_num); //@7:30
//sleep(1); //@10:54
                receive_the_message(400);
                
//sleep(1); //@10:54
                mail_the_message(slavePointer[i].logical_num); //~@7:26; 4-16
                //mail_the_message(shm_ptr->system_processes[i].logical_num); //@7:30
//sleep(1); //@10:54
                receive_the_message(400);
                //sleep(1);
            }
        }
      //sleep(3); //just sleep so you can keep track of iterations visually
    }

    int i, status; //i is for-loop iterator, status is to hold the exit signal after master invokes kill on the infinitely running slave processes
    for (i = 0; i < process_amt; i++){  //~@7:26; 4-16
    //for (i = 1; i < process_amt + 1; i++) { //@7:30
printf("123321djashdfasd\n");
        kill(slavePointer[i].actual_pid, SIGTERM); //@7:26; 4-16
        //kill(shm_ptr->system_processes[i].actual_pid, SIGTERM); //@7:30 getting ipcs
    }

    while(wait(&status) > 0) { 
        if (WIFEXITED(status))	/* process exited normally */
		printf("slave process exited with value %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))	/* child exited on a signal */
		printf("slave process exited due to signal %d\n", WTERMSIG(status));
	else if (WIFSTOPPED(status))	/* child was stopped */
		printf("slave process was stopped by signal %d\n", WIFSTOPPED(status));
    }

    //sleep(10);
active_processes = processes_created_so_far - processes_terminated_so_far; //@7:30
printf("active processes = %d", active_processes); //@7:30

printf("number of processes run = %d\n", next_logical_process);

printf("OSS.c is done running...\n");

    //Cleanup
    detachandremove(shmid,shm_ptr); 
    msgctl(message_queue_id, IPC_RMID, NULL);
    free(slavePointer); //@7:30
    return 0;
}

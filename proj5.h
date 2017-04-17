#pragma once
#define SHM_KEY 9823
#define MESSAGE_QUEUE_KEY 3318
#define MAX_PROCESS_NUM 18
#define TIME_FOR_RESOURCE_ACTION 50000 //nanos //random between [0, 50,000 nanos] see the CONTEXT_SWITCH param from project 4 parameter is meant to be a bound for when a process should request/let go of a resource; 
#define RANDOM_FORK_TIME_BOUND 5000000000 //nanos //random between [0, 500,000,000 nanos] amount of between each new process is forked()
#define RANDOM_NANO_INCREMENTER 50000
#define MASTER_PROCESS_ADDRESS 999
// #define LINE_MAX 10000

typedef struct system_clock {
    int seconds; //#sharedmemory
    int nano_seconds; //#sharememory
} system_clock_t;

//message queue implementation
typedef struct message {
    long message_address; // mtype ~ the type of message (must be greater than zero) ~ process id of the receiving process
    int dead_or_done; //process id has terminated or finished running, whichever comes first
    system_clock_t clock_info;
    int return_address; //address of the person who sent the message (allows the master process to ID the process that sent the message)
} message_t;

typedef struct process_info {
    int logical_num; //1's based array indexing
    pid_t actual_pid; //0's based array indexing
    int resource_requested [20]; //initialize to all zeros and then by which position in the system_resource_t array can just have a switch statement for each resource value 1 through 20 to match up
    int resource_granted [20]; //same logic as above
    //int resource_released [20]; // dont' have to track this per project 5 instructions
} process_t;

typedef struct system_state {
    int Q[MAX_PROCESS_NUM + 1][21]; //Request Matrix: Q[j][i]; if i[21] = 1 then process has been marked as having no allocated resources and thus cannot participate in a deadlock if i[21]=0 then it's deadlock eligible
    int L[MAX_PROCESS_NUM + 1][21]; //Allocation Matrix: L[j][i]; if i[21
    int V[20]; //Allocaiton Vector: V[i] total amount of each resource not allocated to any process
    int R[MAX_PROCESS_NUM + 1]; //Resource Vector: R[i] total amount of each resource in the system
    int W[20]; //Temporary Vector: W[i] equal to the Available Vector (continue with logic at step 2; pg. 277)
    int dead_locked_processes [MAX_PROCESS_NUM + 1]; // (step D in notes) initialize all of these to zero at step D; this will hold a 1 at each position j corresponding to each process, P[j], that is deadlocked
    int system_dead_lock_count; //set to zero originally increment each time dead_lock_processes[M_N_P +1] scanned and has non-zero element vals; when system_dead_lock_count >= 5 terminate program
} system_state_t;

typedef struct resource_descriptor {
    int resource_instances; //each resource object will have an assocaited number of instances of itself [1,10] assigned randomly
    int requested_by [MAX_PROCESS_NUM + 1]; //
    int allocated_to [MAX_PROCESS_NUM + 1]; //
    //pid_t released_by [MAX_PROCESS_NUM + 1]; //don't have to track this per project 5 instructions
    int shared; //will be 0 or 1 (if = 1 then sharable) determined by rand() % 2 which will be performed on every other resource that is created (which should account to 20ish percent of the system's resources)
} resource_t;

typedef struct shared_memory_object {
    //anything else we need to store in shared memory?
    int var; //tracer for the message queue; 
    system_state_t system_state; //used to keep the state of the system in shared memory
    resource_t system_resources [20]; //start at i=0 equals 1st resource...i=19 20th resource (Resource_0 => Resouce_19)
    process_t system_processes [MAX_PROCESS_NUM + 1]; // ~ not keeping track of pid_t vals, this will be the job of slavePointer, but we will keep this array in sync with slavePointer's logical_num value array of process_t objects to keep track of which process is which and who is requesting or has been allocated what
    system_clock_t clock_info; //meant to simulate a systems internal clock, implemented as a logic clock for human readability
} shared_memory_object_t; //this is the system data structure that will be placed in the shared memory (which is allocated by the OSS) to facilitate IPC





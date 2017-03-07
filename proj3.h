#pragma once
#define SHM_KEY 9823
#define MESSAGE_QUEUE_KEY 3318

typedef struct shared_clock {
    int seconds; //#sharedmemory
    int nano_seconds; //#sharememory
} shared_clock_t;

//message queue implementation
typedef struct message {
    long message_address; //process id of the receiving process
    int dead_or_done; //process id has terminated or finished running, whichever comes first
    shared_clock_t clock_info;
    int sender; //consider changing name
} message_t;

typedef struct process_info {
    int logical; //change these names
    pid_t literal; //change these names
} process_t;

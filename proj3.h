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
    //shared_clock_t clock_info;
} message_t;

#pragma once
#define SHM_KEY 9823

typedef struct shared_clock {
    int seconds; //#sharedmemory
    int nano_seconds; //#sharememory
} shared_clock_t;



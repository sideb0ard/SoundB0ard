#pragma once

#include <stdbool.h>
#include <pthread.h>

#include "defjams.h"

typedef struct background_worker {
    bool running;
    bool have_midi_tick;
    pthread_mutex_t midi_tick_mutex;
    pthread_cond_t midi_tick_cond;
    broadcast_event event;
} background_worker;

void *background_worker_run(void *arg);

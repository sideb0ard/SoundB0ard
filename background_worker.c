#include <stdlib.h>

#include "background_worker.h"
#include "mixer.h"

extern mixer *mixr;

void *background_worker_run(void *arg)
{
    background_worker *w = (background_worker *)arg;
    while (w->running)
    {
        pthread_mutex_lock(&w->midi_tick_mutex);
        while (!w->have_midi_tick)
            pthread_cond_wait(&w->midi_tick_cond, &w->midi_tick_mutex);

        mixr->worker.have_midi_tick = false;
        pthread_mutex_unlock(&w->midi_tick_mutex);
        pthread_cond_signal(&w->midi_tick_cond);

        for (int i = 0; i < mixr->algorithm_num; ++i)
        {
            algorithm *a = mixr->algorithms[i];
            if (a != NULL)
                algorithm_event_notify(a, w->event);
        }


    }
    return NULL;
}

#include <stdlib.h>

#include "background_worker.h"
#include "defjams.h"
#include "mixer.h"

extern mixer *mixr;

void *worker_run(void *arg)
{
    worker *w = (worker *)arg;
    while (w->running)
    {
    }
    return NULL;
}

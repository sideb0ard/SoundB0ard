#include <stdbool.h>

typedef struct worker {
    bool running;
} worker;

void *worker_run(void *arg);

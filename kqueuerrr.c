#include <stdio.h>
#include <sys/event.h>
#include <sys/time.h>

#include "defjams.h"
#include "kqueuerrr.h"

extern int kernelq;
extern struct kevent synthz[10];

void *kqueue_run()
{
    printf("KErnel queue is %d\n", kernelq);

    int nev;

    struct kevent eventz[10];

    while (1) {
        nev = kevent(kernelq, synthz, 10, eventz, 10, NULL);
        printf("NEV! %d\n", nev);
    }

    return NULL;
}

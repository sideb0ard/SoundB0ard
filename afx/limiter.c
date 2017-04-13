#include "../defjams.h"
#include "limiter.h"

void limiter_init(limiter *l, double attack_time, double release_time)
{
    envelope_follower_init(&l->e, attack_time, release_time);
}

double limiter_process(limiter *l, double *in)
{
    double out = *in;
    envelope_follower_process(&l->e, in);
    if (l->e.envelope > 1)
        out = *in / l->e.envelope;

    return out;
}



